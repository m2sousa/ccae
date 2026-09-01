#include <stddef.h>

#include <ccae/calc/derivative.h>
#include <ccae/config.h>
#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/expr.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

// Copies into `out` the factors of `mono` but the one at `factor_pos`. Applies
// the power rule on the term to derive, if needed.
// Returns false on a refused factor.
static bool split_off_factor(FacRef* out, size_t* nbr_fac, Rational* coef,
							 FacRef* base, Context* ctx, const Monomial* mono,
							 u64 factor_pos) {
	Factor fac = vec_at(ctx->factors, mono->fac[factor_pos].val);

	*nbr_fac = 0;

	for (u64 i = 0; i < mono->nbr_fac; ++i) {
		if (i != factor_pos)
			out[(*nbr_fac)++] = mono->fac[i];
	}

	*base = mono->fac[factor_pos];

	if (rat_is_one(fac.exponent))
		return true;

	*coef = rat_mul(*coef, fac.exponent);

	Rational lower = rat_sub(fac.exponent, rat_from_int(1));
	if (!rat_is_zero(lower)) {
		out[*nbr_fac] = factor_intern(ctx, fac.head, lower, NULL);

		if (out[*nbr_fac].val == 0)
			return false;

		*nbr_fac += 1;
	}

	// Get the factor without the exponent here.
	*base = factor_intern(ctx, fac.head, rat_from_int(1), NULL);

	return base->val != 0;
}

// Writes into `out` the term with its factor at `factor_pos` differentiated.
// Returns false on a refused factor or monomial.
static bool derive_factor(Term* out, Context* ctx, Term term_to_derive,
						  u64 factor_pos, Index derivative_index) {
	Monomial mono = vec_at(ctx->monomials, term_to_derive.mono.val);

	// Note the +2 there, it is needed for the chain rule of a monomial with
	// already `CCAE_MAX_FACTORS` factors, otherwise we would seg fault applying
	// the rule.
	FacRef facs[CCAE_MAX_FACTORS + 2];
	size_t nbr_fac = 0;
	Rational coef = term_to_derive.coef;
	FacRef base = {0};

	if (!split_off_factor(facs, &nbr_fac, &coef, &base, ctx, &mono, factor_pos))
		return false;

	facs[nbr_fac] = factor_derive(ctx, base, derivative_index);
	if (facs[nbr_fac++].val == 0)
		return false;

	out->coef = coef;
	out->mono = mono_intern(ctx, facs, nbr_fac);

	return out->mono.val != 0;
}

static bool derive_argument(Term* out, Context* ctx, Term term_to_derive,
							u64 factor_pos, u8 arg, Index derivative_index) {
	Monomial mono = vec_at(ctx->monomials, term_to_derive.mono.val);

	FacRef facs[CCAE_MAX_FACTORS + 2];
	size_t nbr_fac = 0;
	Rational coef = term_to_derive.coef;
	FacRef base = {0};

	if (!split_off_factor(facs, &nbr_fac, &coef, &base, ctx, &mono, factor_pos))
		return false;

	Factor fac = vec_at(ctx->factors, base.val);
	SymRef arg_sym = vec_at(ctx->symbols, fac.head.val).args[arg];

	facs[nbr_fac] = factor_partial(ctx, base, arg);
	if (facs[nbr_fac++].val == 0)
		return false;

	FacRef arg_one = factor_intern(ctx, arg_sym, rat_from_int(1), NULL);
	facs[nbr_fac] = factor_derive(ctx, arg_one, derivative_index);
	if (facs[nbr_fac++].val == 0)
		return false;

	out->coef = coef;
	out->mono = mono_intern(ctx, facs, nbr_fac);

	return out->mono.val != 0;
}

ExprRef deriv_expr(Context* ctx, ExprRef expr_ref, Index derivative_index) {
	ExprRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (expr_ref.val == 0 || expr_ref.val >= ctx->exprs.length)
		die("CCAE (deriv_expr): expression handle out of range.");

	if (derivative_index.kind != IDX_FREE) {
		err_set(&ctx->err, CCAE_INVALID_ARG,
				"derivative: the index must be free.");
		return null_ref;
	}

	Expr expr = vec_at(ctx->exprs, expr_ref.val);

	size_t nbr_term_max = 0;
	for (size_t t = 0; t < expr.nbr_term; ++t) {
		Monomial mono = vec_at(ctx->monomials, expr.terms[t].mono.val);

		for (u64 i = 0; i < mono.nbr_fac; ++i) {
			SymRef head = vec_at(ctx->factors, mono.fac[i].val).head;
			const Symbol* symbol = &vec_at(ctx->symbols, head.val);

			nbr_term_max += symbol->kind == SYM_UNDET_FN ? symbol->nbr_arg : 1;
		}
	}

	if (nbr_term_max == 0)
		return expr_intern(ctx, NULL, 0);

	ArenaMark mark = arena_save(ctx->scratch);

	Term* terms = arena_alloc_array(ctx->scratch, Term, nbr_term_max);
	if (!terms)
		die("CCAE (deriv_expr): out of memory.");

	size_t nbr_term = 0;

	for (size_t t = 0; t < expr.nbr_term; ++t) {
		Term term_to_derive = expr.terms[t];
		Monomial mono = vec_at(ctx->monomials, term_to_derive.mono.val);

		// One term per factor: `factor_pos` is the one that gets ∂.
		for (u64 factor_pos = 0; factor_pos < mono.nbr_fac; ++factor_pos) {
			Factor fac = vec_at(ctx->factors, mono.fac[factor_pos].val);
			const Symbol* symbol = &vec_at(ctx->symbols, fac.head.val);

			// This is the chain rule block.
			if (symbol->kind == SYM_UNDET_FN) {
				for (u8 arg = 0; arg < symbol->nbr_arg; ++arg) {
					SymRef arg_sym = symbol->args[arg];
					if (vec_at(ctx->symbols, arg_sym.val).flags &
						F_CONST_IN_SPACE)
						continue;

					if (!derive_argument(&terms[nbr_term], ctx, term_to_derive,
										 factor_pos, arg, derivative_index)) {
						arena_restore(ctx->scratch, mark);
						return null_ref;
					}

					++nbr_term;
				}

				continue;
			}

			// Constant in space, therefore derivative is zero, skip.
			if (symbol->flags & F_CONST_IN_SPACE)
				continue;

			if (!derive_factor(&terms[nbr_term], ctx, term_to_derive,
							   factor_pos, derivative_index)) {
				arena_restore(ctx->scratch, mark);
				return null_ref;
			}

			++nbr_term;
		}
	}

	ExprRef result = expr_intern(ctx, terms, nbr_term);

	arena_restore(ctx->scratch, mark);

	return result;
}

// Writes into `out` the term with its factor at `factor_pos` differentiated.
// Returns false on a refused factor or monomial.
static bool partial_factor(Term* out, Context* ctx, Term term_to_derive,
						   u64 factor_pos, bool is_var, u8 arg) {
	Monomial mono = vec_at(ctx->monomials, term_to_derive.mono.val);

	FacRef facs[CCAE_MAX_FACTORS + 1];
	size_t nbr_fac = 0;
	Rational coef = term_to_derive.coef;
	FacRef base = {0};

	if (!split_off_factor(facs, &nbr_fac, &coef, &base, ctx, &mono, factor_pos))
		return false;

	if (!is_var) {
		facs[nbr_fac] = factor_partial(ctx, base, arg);
		if (facs[nbr_fac++].val == 0)
			return false;
	}

	out->coef = coef;
	out->mono = mono_intern(ctx, facs, nbr_fac);

	return out->mono.val != 0;
}

ExprRef partial_expr(Context* ctx, ExprRef expr_ref, SymRef var) {
	ExprRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (expr_ref.val == 0 || expr_ref.val >= ctx->exprs.length)
		die("CCAE (partial_expr): expression handle out of range.");

	if (var.val == 0 || var.val >= ctx->symbols.length)
		die("CCAE (partial_expr): symbol handle out of range.");

	if (vec_at(ctx->symbols, var.val).kind != SYM_HOMOG) {
		err_set(&ctx->err, CCAE_INVALID_ARG,
				"partial: '%s' is not a homogeneous variable.",
				sp_get(ctx->sp, vec_at(ctx->symbols, var.val).name));
		return null_ref;
	}

	Expr expr = vec_at(ctx->exprs, expr_ref.val);

	size_t nbr_term_max = 0;
	for (size_t t = 0; t < expr.nbr_term; ++t)
		nbr_term_max += vec_at(ctx->monomials, expr.terms[t].mono.val).nbr_fac;

	if (nbr_term_max == 0)
		return expr_intern(ctx, NULL, 0);

	ArenaMark mark = arena_save(ctx->scratch);

	Term* terms = arena_alloc_array(ctx->scratch, Term, nbr_term_max);
	if (!terms)
		die("CCAE (partial_expr): out of memory.");

	size_t nbr_term = 0;

	for (size_t t = 0; t < expr.nbr_term; ++t) {
		Term term_to_derive = expr.terms[t];
		Monomial mono = vec_at(ctx->monomials, term_to_derive.mono.val);

		for (u64 factor_pos = 0; factor_pos < mono.nbr_fac; ++factor_pos) {
			Factor fac = vec_at(ctx->factors, mono.fac[factor_pos].val);
			const Symbol* symbol = &vec_at(ctx->symbols, fac.head.val);

			bool is_var = fac.head.val == var.val;

			u8 arg = 0;
			while (arg < symbol->nbr_arg && symbol->args[arg].val != var.val)
				++arg;

			if (!is_var && arg == symbol->nbr_arg)
				continue;

			if (!partial_factor(&terms[nbr_term], ctx, term_to_derive,
								factor_pos, is_var, arg)) {
				arena_restore(ctx->scratch, mark);
				return null_ref;
			}

			++nbr_term;
		}
	}

	ExprRef result = expr_intern(ctx, terms, nbr_term);

	arena_restore(ctx->scratch, mark);

	return result;
}

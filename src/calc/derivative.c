#include <stddef.h>

#include <ccae/calc/derivative.h>
#include <ccae/config.h>
#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/expr.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

// Writes into `out` the term with its factor at `factor_pos` differentiated.
// Returns false on a refused factor or monomial.
static bool derive_factor(Term* out, Context* ctx, Term term_to_derive,
						  u64 factor_pos, Index derivative_index) {
	const Monomial* mono = &vec_at(ctx->monomials, term_to_derive.mono.val);
	Factor fac = vec_at(ctx->factors, mono->fac[factor_pos].val);

	FacRef facs[CCAE_MAX_FACTORS];
	size_t nbr_fac = 0;

	for (u64 i = 0; i < mono->nbr_fac; ++i) {
		if (i != factor_pos)
			facs[nbr_fac++] = mono->fac[i];
	}

	Rational coef = term_to_derive.coef;
	FacRef base = mono->fac[factor_pos];

	if (!rat_is_one(fac.exponent)) {
		coef = rat_mul(coef, fac.exponent);

		Rational lower = rat_sub(fac.exponent, rat_from_int(1));
		if (!rat_is_zero(lower)) {
			facs[nbr_fac] = factor_intern(ctx, fac.head, lower, NULL);

			if (facs[nbr_fac].val == 0)
				return false;

			nbr_fac += 1;
		}

		// Get the factor without the exponent here.
		base = factor_intern(ctx, fac.head, rat_from_int(1), NULL);
		if (base.val == 0)
			return false;
	}

	facs[nbr_fac] = factor_derive(ctx, base, derivative_index);
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

	// At most one term per factor.
	size_t nbr_fac_total = 0;
	for (size_t t = 0; t < expr.nbr_term; ++t)
		nbr_fac_total += vec_at(ctx->monomials, expr.terms[t].mono.val).nbr_fac;

	if (nbr_fac_total == 0)
		return expr_intern(ctx, NULL, 0);

	ArenaMark mark = arena_save(ctx->scratch);

	Term* terms = arena_alloc_array(ctx->scratch, Term, nbr_fac_total);
	if (!terms)
		die("CCAE (deriv_expr): out of memory.");

	size_t nbr_term = 0;

	for (size_t t = 0; t < expr.nbr_term; ++t) {
		Term term_to_derive = expr.terms[t];
		Monomial mono = vec_at(ctx->monomials, term_to_derive.mono.val);

		// One Leibniz term per factor: `factor_pos` is the one that gets ∂.
		for (u64 factor_pos = 0; factor_pos < mono.nbr_fac; ++factor_pos) {
			Factor fac = vec_at(ctx->factors, mono.fac[factor_pos].val);

			// Constant in space, therefore derivative is zero, skip.
			if (vec_at(ctx->symbols, fac.head.val).flags & F_CONST_IN_SPACE)
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

#include <stdlib.h>
#include <string.h>

#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/expr.h>
#include <ccae/kernel/monomial.h>

static bool have_same_terms(const Expr* expr, const Term* terms,
							size_t nbr_term) {
	if (expr->nbr_term != nbr_term)
		return false;

	// As for factors, this relies on the fact that terms are sorted before
	// passed as a parameter to this function.
	for (size_t i = 0; i < nbr_term; ++i) {
		Term a = expr->terms[i];
		Term b = terms[i];

		bool same_coef = a.coef.negative == b.coef.negative &&
						 a.coef.num == b.coef.num && a.coef.den == b.coef.den;

		if (a.mono.val != b.mono.val || !same_coef)
			return false;
	}

	return true;
}

static int compare_by_monomial(const void* a, const void* b) {
	u32 mono_a = ((const Term*)a)->mono.val;
	u32 mono_b = ((const Term*)b)->mono.val;

	return (mono_a > mono_b) - (mono_a < mono_b);
}

// Simply merges into a single term the terms that have the same monomial.
// This relies on the sorting of the terms through their monomials.
static size_t merge_terms(Term* terms, size_t nbr_term) {
	size_t nbr_kept = 0;
	size_t i = 0;

	while (i < nbr_term) {
		Term current = terms[i];

		for (++i; i < nbr_term && terms[i].mono.val == current.mono.val; ++i)
			current.coef = rat_add(current.coef, terms[i].coef);

		if (!rat_is_zero(current.coef))
			terms[nbr_kept++] = current;
	}

	return nbr_kept;
}

static ExprRef intern_terms(Context* ctx, const Term* terms, size_t nbr_term) {
	// TODO: This is a linear search, setup an hashmap to get O(1) accesses.
	for (size_t i = 1; i < ctx->exprs.length; ++i) {
		if (have_same_terms(&vec_at(ctx->exprs, i), terms, nbr_term)) {
			ExprRef found = {.val = (u32)i};
			return found;
		}
	}

	Expr expr = {.nbr_term = nbr_term, .terms = NULL};

	if (nbr_term > 0) {
		expr.terms = arena_alloc_array(ctx->perm, Term, nbr_term);
		if (!expr.terms)
			die("CCAE (expr_intern): out of memory.");

		memcpy(expr.terms, terms, nbr_term * sizeof(Term));
	}

	vec_append(ctx->perm, ctx->exprs, expr);

	ExprRef result = {.val = (u32)(ctx->exprs.length - 1)};

	return result;
}

ExprRef expr_intern(Context* ctx, Term* terms, size_t nbr_term) {
	ExprRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	for (size_t i = 0; i < nbr_term; ++i) {
		if (terms[i].mono.val == 0 ||
			terms[i].mono.val >= ctx->monomials.length)
			die("CCAE (expr_intern): monomial handle out of range.");
	}

	if (nbr_term == 0)
		return intern_terms(ctx, NULL, 0);

	for (size_t i = 1; i < nbr_term; ++i) {
		if (!mono_have_same_free_indices(ctx, terms[0].mono, terms[i].mono)) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"expression: terms with different free indices.");
			return null_ref;
		}
	}

	qsort(terms, nbr_term, sizeof(Term), compare_by_monomial);

	size_t nbr_kept = merge_terms(terms, nbr_term);

	return intern_terms(ctx, terms, nbr_kept);
}

ExprRef expr_add(Context* ctx, ExprRef a, ExprRef b) {
	ExprRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (a.val == 0 || a.val >= ctx->exprs.length || b.val == 0 ||
		b.val >= ctx->exprs.length)
		die("CCAE (expr_add): expression handle out of range.");

	Expr expr_a = vec_at(ctx->exprs, a.val);
	Expr expr_b = vec_at(ctx->exprs, b.val);
	size_t nbr_term = expr_a.nbr_term + expr_b.nbr_term;

	if (nbr_term == 0)
		return expr_intern(ctx, NULL, 0);

	Term* terms = arena_alloc_array(ctx->scratch, Term, nbr_term);
	if (!terms)
		die("CCAE (expr_add): out of memory.");

	if (expr_a.nbr_term > 0)
		memcpy(terms, expr_a.terms, expr_a.nbr_term * sizeof(Term));

	if (expr_b.nbr_term > 0)
		memcpy(terms + expr_a.nbr_term, expr_b.terms,
			   expr_b.nbr_term * sizeof(Term));

	ExprRef sum = expr_intern(ctx, terms, nbr_term);

	arena_reset(ctx->scratch);

	return sum;
}

ExprRef expr_scale(Context* ctx, ExprRef e, Rational r) {
	ExprRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (e.val == 0 || e.val >= ctx->exprs.length)
		die("CCAE (expr_scale): expression handle out of range.");

	Expr expr = vec_at(ctx->exprs, e.val);

	if (expr.nbr_term == 0 || rat_is_zero(r))
		return expr_intern(ctx, NULL, 0);

	Term* terms = arena_alloc_array(ctx->scratch, Term, expr.nbr_term);
	if (!terms)
		die("CCAE (expr_scale): out of memory.");

	for (size_t i = 0; i < expr.nbr_term; ++i) {
		terms[i].coef = rat_mul(expr.terms[i].coef, r);
		terms[i].mono = expr.terms[i].mono;
	}

	ExprRef scaled = expr_intern(ctx, terms, expr.nbr_term);

	arena_reset(ctx->scratch);

	return scaled;
}

ExprRef expr_neg(Context* ctx, ExprRef e) {
	return expr_scale(ctx, e, rat_from_int(-1));
}

ExprRef expr_mul(Context* ctx, ExprRef a, ExprRef b) {
	ExprRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (a.val == 0 || a.val >= ctx->exprs.length || b.val == 0 ||
		b.val >= ctx->exprs.length)
		die("CCAE (expr_mul): expression handle out of range.");

	Expr expr_a = vec_at(ctx->exprs, a.val);
	Expr expr_b = vec_at(ctx->exprs, b.val);
	size_t nbr_term = expr_a.nbr_term * expr_b.nbr_term;

	if (nbr_term == 0)
		return expr_intern(ctx, NULL, 0);

	Term* terms = arena_alloc_array(ctx->scratch, Term, nbr_term);
	if (!terms)
		die("CCAE (expr_mul): out of memory.");

	size_t nbr_filled = 0;

	for (size_t i = 0; i < expr_a.nbr_term; ++i) {
		for (size_t j = 0; j < expr_b.nbr_term; ++j) {
			Term ta = expr_a.terms[i];
			Term tb = expr_b.terms[j];

			MonoRef mono = mono_mul(ctx, ta.mono, tb.mono);
			// `mono_mul` returns the null handle on errors, we propagate here.
			if (mono.val == 0) {
				arena_reset(ctx->scratch);
				return null_ref;
			}

			terms[nbr_filled].coef = rat_mul(ta.coef, tb.coef);
			terms[nbr_filled].mono = mono;
			++nbr_filled;
		}
	}

	ExprRef product = expr_intern(ctx, terms, nbr_term);

	arena_reset(ctx->scratch);

	return product;
}

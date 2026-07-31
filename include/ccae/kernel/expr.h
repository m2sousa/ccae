#ifndef CCAE_EXPR_H
#define CCAE_EXPR_H

#include <stddef.h>

#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/fwd.h>
#include <ccae/types.h>

typedef struct {
	Rational coef;
	MonoRef mono;
} Term;

typedef struct {
	size_t nbr_term;
	Term* terms;
} Expr;

typedef Vec(Expr) ExprVec;

/* Returns the handle of the sum of the `nbr_term` terms at `terms`, in any
 * order, storing it the first time. Terms with the same monomial are
 * collected, and zero coefficients dropped: no term gives 0.
 * Sides: Sorts and collects `terms` in place.
 *
 * Example:
 *		`ExprRef e = expr_intern(ctx, (Term[]){{half, x}, {half, x}}, 2);` */
[[nodiscard]] ExprRef expr_intern(Context* ctx, Term* terms, size_t nbr_term);

/* Returns the handle of `a + b`.
 *
 * Example:
 *		`ExprRef sum = expr_add(ctx, x, y);` */
[[nodiscard]] ExprRef expr_add(Context* ctx, ExprRef a, ExprRef b);

/* Returns the handle of `r * e`; 0 when `r` is 0.
 *
 * Example:
 *		`ExprRef half_e = expr_scale(ctx, e, rat_new(1, 2));` */
[[nodiscard]] ExprRef expr_scale(Context* ctx, ExprRef e, Rational r);

/* Returns the handle of `-e`.
 *
 * Example:
 *		`ExprRef minus_e = expr_neg(ctx, e);` */
[[nodiscard]] ExprRef expr_neg(Context* ctx, ExprRef e);

/* Returns the handle of `a * b`.
 * Fails like `mono_mul` when a product of monomials has too many factors
 * (CCAE_MAX_FACTORS).
 *
 * Example:
 *		`ExprRef square = expr_mul(ctx, x_plus_y, x_plus_y);` */
[[nodiscard]] ExprRef expr_mul(Context* ctx, ExprRef a, ExprRef b);

#endif

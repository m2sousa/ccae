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

#endif

#ifndef CCAE_DERIVATIVE_H
#define CCAE_DERIVATIVE_H

#include <ccae/kernel/expr.h>
#include <ccae/kernel/fwd.h>
#include <ccae/kernel/index.h>

/* Returns the handle of `∂_a expr_ref`, `a` being `derivative_index`.
 * Sets `ctx->err` and returns the null handle when `derivative_index` is not
 * free, on a refused factor or monomial, or after an earlier error.
 *
 * Example:
 *		`ExprRef dX = deriv_expr(ctx, X, (Index){IDX_FREE, 'a'});` */
[[nodiscard]] ExprRef deriv_expr(Context* ctx, ExprRef expr_ref,
								 Index derivative_index);

/* Returns the handle of `∂_var expr_ref`, for the homogeneous variable `var`.
 * Sets `ctx->err` and returns the null handle when `var` is not homogeneous,
 * on a refused factor or monomial, or after an earlier error.
 *
 * Example:
 *		`ExprRef dH = partial_expr(ctx, H, p);` */
[[nodiscard]] ExprRef partial_expr(Context* ctx, ExprRef expr_ref, SymRef var);

#endif

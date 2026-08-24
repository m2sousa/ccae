#ifndef CCAE_FACTOR_H
#define CCAE_FACTOR_H

#include <ccae/config.h>
#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/fwd.h>
#include <ccae/kernel/index.h>

typedef struct {
	SymRef head;
	Rational exponent;
	Index indices[CCAE_MAX_SLOT];
	u8 nbr_der;
	Index der[CCAE_MAX_DERIVATIVES];
} Factor;

typedef Vec(Factor) FactorVec;

/* Returns the handle of `head^exponent`, storing it the first time.
 * `indices` has one index per slot of `head`, NULL for a scalar.
 * Sets `ctx->err` and returns the null handle on an exponent that `head`'s
 * slots and flags forbid, or on a free index not of its slot's bundle.
 *
 * Example:
 *		`FacRef sqrt_p = factor_intern(ctx, p, rat_new(1, 2), NULL);` */
[[nodiscard]] FacRef factor_intern(Context* ctx, SymRef head, Rational exponent,
								   const Index* indices);

/* Returns the handle of `fac` with `a` added to its derivative indices, kept
 * sorted: `∂_a X` from `X`. The exponent is kept.
 * Sets `ctx->err` and returns the null handle when no bundle carries the
 * derivative, when `a` is free and not of that bundle, when `fac` is constant
 * in space, or when it would have more than `CCAE_MAX_DERIVATIVES`.
 * Note that derivatives of constants in space to zero are handled on
 * expression, not on factors, therefore returning an error.
 *
 * Example:
 *		`FacRef dX = factor_derive(ctx, X_c, (Index){IDX_FREE, 'a'});` */
[[nodiscard]] FacRef factor_derive(Context* ctx, FacRef fac, Index a);

#endif

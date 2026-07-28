#ifndef CCAE_FACTOR_H
#define CCAE_FACTOR_H

#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/fwd.h>

typedef struct {
	SymRef head;
	Rational power;
} Factor;

typedef Vec(Factor) FactorVec;

/* Returns the handle of `head^power`, storing it the first time.
 * Sets `ctx->err` and returns the null handle on a power that `head`'s slots
 * and flags forbid.
 *
 * Example:
 *		`FacRef sqrt_p = factor_intern(ctx, p, rat_new(1, 2));` */
[[nodiscard]] FacRef factor_intern(Context* ctx, SymRef head, Rational power);

#endif

#ifndef CCAE_FACTOR_H
#define CCAE_FACTOR_H

#include <ccae/config.h>
#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/fwd.h>
#include <ccae/kernel/index.h>

typedef struct {
	SymRef head;
	Rational power;
	Index indices[CCAE_MAX_SLOT];
} Factor;

typedef Vec(Factor) FactorVec;

/* Returns the handle of `head^power`, storing it the first time.
 * `indices` has one index per slot of `head`, NULL for a scalar.
 * Sets `ctx->err` and returns the null handle on a power that `head`'s slots
 * and flags forbid, or on a free index not of its slot's bundle.
 *
 * Example:
 *		`FacRef sqrt_p = factor_intern(ctx, p, rat_new(1, 2), NULL);` */
[[nodiscard]] FacRef factor_intern(Context* ctx, SymRef head, Rational power,
								   const Index* indices);

#endif

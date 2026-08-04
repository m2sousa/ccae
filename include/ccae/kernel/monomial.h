#ifndef CCAE_MONOMIAL_H
#define CCAE_MONOMIAL_H

#include <stddef.h>

#include <ccae/config.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/fwd.h>
#include <ccae/types.h>

typedef struct {
	u64 nbr_fac;
	FacRef fac[CCAE_MAX_FACTORS];
} Monomial;

typedef Vec(Monomial) MonomialVec;

/* Returns the handle of the product of the `nbr_fac` factors at `facs`, in any
 * order, storing it the first time.
 * An index used twice, once up and once down in the same bundle, becomes a
 * dummy, numbered from 0.
 * Sets `ctx->err` and returns the null handle on more
 * than `CCAE_MAX_FACTORS` factors, or on an index repeated any other way.
 *
 * Example:
 *		`MonoRef m = mono_intern(ctx, (FacRef[]){sqrt_p, dK}, 2);` */
[[nodiscard]] MonoRef mono_intern(Context* ctx, const FacRef* facs,
								  size_t nbr_fac);

/* Returns the handle of `a * b`. Fails like `mono_intern`.
 *
 * Example:
 *		`MonoRef m = mono_mul(ctx, sqrt_p, dK);` */
[[nodiscard]] MonoRef mono_mul(Context* ctx, MonoRef a, MonoRef b);

#endif

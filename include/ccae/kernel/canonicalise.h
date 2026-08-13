#ifndef CCAE_CANONICALISE_H
#define CCAE_CANONICALISE_H

#include <ccae/kernel/fwd.h>

/* Returns the canonical form of the monomial `m`, and sets `*sign` to +1 or -1
 * so that `m = *sign * result`.
 * Returns the null handle after an earlier error or dies on a bad handle.
 *
 * Example:
 *		with `A` declared "antisym(0,1)", `canon_mono(ctx, A_ba, &sign)` is
 *		`A_ab`, with `sign` -1. */
[[nodiscard]] MonoRef canon_mono(Context* ctx, MonoRef m, int* sign);

#endif

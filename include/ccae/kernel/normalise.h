#ifndef CCAE_NORMALISE_H
#define CCAE_NORMALISE_H

#include <ccae/kernel/expr.h>
#include <ccae/kernel/fwd.h>

/* Returns `term` normalised: its monomial canonical, with every Kronecker delta
 * absorbed, and its coefficient changed to match.
 * Sets `ctx->err` and returns a null monomial after `CCAE_MAX_NORMALISE_PASSES`
 * passes or an earlier error.
 *
 * Example:
 *		with `delta` the Kronecker delta of a bundle of dimension 3,
 *		`normalise_term(ctx, (Term){one, delta_aa})` is `{3, one_mono}`. */
[[nodiscard]] Term normalise_term(Context* ctx, Term term);

#endif

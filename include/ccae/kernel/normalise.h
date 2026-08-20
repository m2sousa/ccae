#ifndef CCAE_NORMALISE_H
#define CCAE_NORMALISE_H

#include <ccae/kernel/expr.h>
#include <ccae/kernel/fwd.h>

/* Writes into `out` the terms whose sum is `term` normalised.
 * Returns how many terms, 0 for a zero term.
 *
 * Sets `ctx->err` and returns 0 when more than `capacity` terms come out, after
 * `CCAE_MAX_NORMALISE_PASSES` passes, or after an earlier error.
 *
 * Example:
 *		with `delta` the Kronecker delta of a bundle of dimension 3,
 *		`normalise_term(out, 1, ctx, (Term){one, delta_aa});` */
[[nodiscard]] size_t normalise_term(Term* out, size_t capacity, Context* ctx,
									Term term);

#endif

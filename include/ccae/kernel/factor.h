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

/* Returns the handle of `fac` with new indices `indices` (slots then
 * derivatives, in order). Head and exponent are kept, the derivative indices
 * sorted again.
 * Sets `ctx->err` and returns the null handle on a free index not
 * of its place's bundle.
 *
 * Example:
 *		`factor_replace_indices(ctx, dX, (Index[]){{IDX_DUMMY, 0}, {IDX_FREE,
 * 'b'}})` turns `∂_b X^a` into `∂_b X^$0`. */
[[nodiscard]] FacRef factor_replace_indices(Context* ctx, FacRef fac,
											const Index* indices);

/* Returns -1, 0 or 1 as `a` comes before, is, or comes after `b`: by head
 * handle, then derivative indices, then exponent, then indices slot by slot.
 *
 * Example:
 *		`factor_cmp(ctx, x_one, x_two)` is -1: `x` comes before `x²`. */
[[nodiscard]] int factor_cmp(const Context* ctx, FacRef a, FacRef b);

/* Returns how many indices `fac` holds: one per slot and one per derivative
 * index.
 *
 * Example:
 *		for `∂_a ∂_b X^c`, returns 3. */
[[nodiscard]] size_t factor_count_indices(const Context* ctx, FacRef fac);

/* Writes into `out` the indices of `fac` (slots, then derivatives).
 * Returns how many indices.
 *
 * Example:
 *		`∂_a ∂_b X^c`, writes (c, a, b) and returns 3. */
size_t factor_build_indices(Index* out, const Context* ctx, FacRef fac);

#endif

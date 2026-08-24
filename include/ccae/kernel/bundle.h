#ifndef CCAE_BUNDLE_H
#define CCAE_BUNDLE_H

#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/fwd.h>
#include <ccae/types.h>

typedef struct {
	StringRef name;
	StringRef index_names;
	u8 dim;
	SymRef kronecker;
	bool derivative;
	SymRef eps_up;
	// FIXME: keep a single Levi-Civita symbol once the metric is implemented
	SymRef eps_down;
} Bundle;

typedef Vec(Bundle) BundleVec;

/* Declares the bundle `name` of dimension `dim`, whose index names are the
 * characters of `index_names`.
 * Sets `ctx->err` and returns the null handle on
 * an empty or taken name or index name, a non-letter, a `dim` of 0, or an
 * earlier error.
 *
 * Example:
 *		`BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh", 3);` */
[[nodiscard]] BundleRef bundle_declare(Context* ctx, const char* name,
									   const char* index_names, u8 dim);

/* Registers `sym` as the Kronecker delta of `bundle`.
 * Sets `ctx->err` when `sym` is not a constant with two slots of `bundle`, up
 * then down, or when `bundle` already has one. Does nothing after an earlier
 * error; dies on a bad handle.
 *
 * Example:
 *		`bundle_set_kronecker(ctx, spatial, delta);` */
void bundle_set_kronecker(Context* ctx, BundleRef bundle, SymRef sym);

/* Registers `sym` as the upper or the lower Levi-Civita symbol of `bundle`.
 * Sets `ctx->err` when `sym` is not a constant with one slot of `bundle` per
 * dimension, all up or all down, antisymmetric over every slot, or when
 * `bundle` already has one in that position. Does nothing after an earlier
 * error; dies on a bad handle.
 *
 * Example:
 *		`bundle_set_epsilon(ctx, spatial, eps_down);` */
void bundle_set_epsilon(Context* ctx, BundleRef bundle, SymRef sym);

// TODO: Going for more generalities, we could add derivative to bundles
// regardless of other bundles, for whatever reasons.
/* Registers `bundle` as the one the spatial derivative `∂_a`.
 * Sets `ctx->err` when a bundle already has it. Does nothing after an earlier
 * error; dies on a bad handle.
 *
 * Example:
 *		`bundle_set_derivative(ctx, spatial);` */
void bundle_set_derivative(Context* ctx, BundleRef bundle);

/* Returns the bundle the spatial derivative acts on, or the null handle.
 *
 * Example:
 *		`BundleRef space = bundle_find_derivative(ctx);` */
[[nodiscard]] BundleRef bundle_find_derivative(const Context* ctx);

/* Returns the bundle owning `index_name`, or the null handle.
 *
 * Example:
 *		`BundleRef b = bundle_find_by_index_name(ctx, 'a');` */
[[nodiscard]] BundleRef bundle_find_by_index_name(const Context* ctx,
												  char index_name);

#endif

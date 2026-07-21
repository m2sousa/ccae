#ifndef CCAE_BUNDLE_H
#define CCAE_BUNDLE_H

#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/fwd.h>

typedef struct {
	StringRef name;
	StringRef index_names;
} Bundle;

typedef Vec(Bundle) BundleVec;

/* Declares the bundle `name`, whose indices are named by the letters of
 * `index_names`. Sets `ctx->err` and returns the null handle on an empty or
 * taken name or index name, a non-letter, or an earlier error.
 *
 * Example:
 *		`BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh");` */
[[nodiscard]] BundleRef bundle_declare(Context* ctx, const char* name,
									   const char* index_names);

/* Returns the bundle owning `index_name`, or the null handle.
 *
 * Example:
 *		`BundleRef b = bundle_find_by_index_name(ctx, 'a');` */
[[nodiscard]] BundleRef bundle_find_by_index_name(const Context* ctx,
												  char index_name);

#endif

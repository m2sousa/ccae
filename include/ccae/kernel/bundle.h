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

/* Declares a bundle (an index universe, e.g. "spatial") named `name`, whose
 * indices are named by the ASCII letters of `index_names`, one name per
 * letter. An index name belongs to one bundle only, so it tells which bundle an
 * index lives in.
 * Sets `ctx->err` and returns the null handle if `name` is empty or already
 * declared, if `index_names` is empty, holds a non-letter or a name already
 * used, or if `ctx` already holds an error.
 *
 * Example:
 *		`BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh");` */
[[nodiscard]] BundleRef bundle_declare(Context* ctx, const char* name,
									   const char* index_names);

/* Returns the bundle that has `index_name` among its index names, or the null
 * handle.
 *
 * Example:
 *		`BundleRef b = bundle_find_by_index_name(ctx, 'a');` */
[[nodiscard]] BundleRef bundle_find_by_index_name(const Context* ctx,
												  char index_name);

#endif

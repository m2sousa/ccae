#ifndef CCAE_BUNDLE_H
#define CCAE_BUNDLE_H

#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/fwd.h>

typedef struct {
	StringRef name;
} Bundle;

typedef Vec(Bundle) Bundles;

/* Declares a bundle (an index universe, e.g. "spatial") named `name`.
 * Sets `ctx->err` and returns the null handle if `name` is empty or already
 * declared, or if `ctx` already holds an error.
 *
 * Example:
 *		`BundleRef spatial = bundle_declare(ctx, "spatial");` */
[[nodiscard]] BundleRef bundle_declare(Context* ctx, const char* name);

#endif

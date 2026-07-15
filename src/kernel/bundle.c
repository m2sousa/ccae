#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>

BundleRef bundle_declare(Context* ctx, const char* name) {
	BundleRef null = {.val = 0};

	if (!ctx_ok(ctx))
		return null;

	if (!*name) {
		err_set(&ctx->err, CCAE_INVALID_ARG, "bundle name is empty.");
		return null;
	}

	StringRef name_ref = sp_intern(ctx->sp, name);

	for (size_t i = 1; i < ctx->bundles.length; ++i) {
		if (vec_at(ctx->bundles, i).name.val == name_ref.val) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"bundle '%s' is already declared.", name);
			return null;
		}
	}

	Bundle bundle = {.name = name_ref};
	vec_append(ctx->perm, ctx->bundles, bundle);

	BundleRef br = {.val = (u32)(ctx->bundles.length - 1)};
	return br;
}

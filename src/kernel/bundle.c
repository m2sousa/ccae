#include <string.h>

#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/utils.h>

static bool check_index_names(Context* ctx, const char* name,
							  const char* index_names) {
	if (!*index_names) {
		err_set(&ctx->err, CCAE_INVALID_ARG, "bundle '%s' has no index names.",
				name);
		return false;
	}

	for (const char* l = index_names; *l; ++l) {
		if (!is_letter(*l)) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"bundle '%s': '%c' is not a letter.", name, *l);
			return false;
		}

		if (bundle_find_by_index_name(ctx, *l).val != 0 || strchr(l + 1, *l)) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"bundle '%s': index name '%c' is already used.", name, *l);
			return false;
		}
	}

	return true;
}

BundleRef bundle_declare(Context* ctx, const char* name,
						 const char* index_names) {
	BundleRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (!*name) {
		err_set(&ctx->err, CCAE_INVALID_ARG, "bundle name is empty.");
		return null_ref;
	}

	StringRef name_ref = sp_intern(ctx->sp, name);

	for (size_t i = 1; i < ctx->bundles.length; ++i) {
		if (vec_at(ctx->bundles, i).name.val == name_ref.val) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"bundle '%s' is already declared.", name);
			return null_ref;
		}
	}

	if (!check_index_names(ctx, name, index_names))
		return null_ref;

	Bundle bundle = {.name = name_ref,
					 .index_names = sp_intern(ctx->sp, index_names)};
	vec_append(ctx->perm, ctx->bundles, bundle);

	BundleRef result = {.val = (u32)(ctx->bundles.length - 1)};
	return result;
}

BundleRef bundle_find_by_index_name(const Context* ctx, char index_name) {
	BundleRef found = {.val = 0};

	if (!index_name)
		return found;

	for (size_t i = 1; i < ctx->bundles.length; ++i) {
		const char* names =
			sp_get(ctx->sp, vec_at(ctx->bundles, i).index_names);
		if (strchr(names, index_name)) {
			found.val = (u32)i;
			break;
		}
	}

	return found;
}

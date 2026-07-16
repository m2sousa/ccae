#include <stddef.h>
#include <string.h>

#include <ccae/core/error.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>

#include "t_utils.h"

static size_t test_bundle_declare(void);
static size_t test_bundle_duplicate(void);
static size_t test_bundle_empty_name(void);
static size_t test_bundle_after_error(void);
static size_t test_bundle_index_names(void);

size_t test_bundle(void) {
	size_t fails = 0;
	fails += test_bundle_declare();
	fails += test_bundle_duplicate();
	fails += test_bundle_empty_name();
	fails += test_bundle_after_error();
	fails += test_bundle_index_names();

	return fails;
}

static size_t test_bundle_declare(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh");
	BundleRef internal = bundle_declare(ctx, "internal", "ijklmn");

	CHECK(ctx_ok(ctx));
	CHECK(spatial.val != 0);
	CHECK(internal.val != 0);
	CHECK(spatial.val != internal.val);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_bundle_duplicate(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef first = bundle_declare(ctx, "spatial", "abcdefgh");
	BundleRef second = bundle_declare(ctx, "spatial", "abcdefgh");

	CHECK(first.val != 0);
	CHECK(second.val == 0);
	CHECK(ctx->err.code == CCAE_INVALID_ARG);
	CHECK(strcmp(ctx->err.msg, "bundle 'spatial' is already declared.") == 0);

	ctx_clear_err(ctx);
	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_bundle_empty_name(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef empty = bundle_declare(ctx, "", "abc");

	CHECK(empty.val == 0);
	CHECK(ctx->err.code == CCAE_INVALID_ARG);

	ctx_destroy(ctx);

	return fails;
}

// While `ctx` holds an error, every declaration is refused until it is cleared.
static size_t test_bundle_after_error(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef empty = bundle_declare(ctx, "", "abc");
	BundleRef refused = bundle_declare(ctx, "spatial", "abcdefgh");

	CHECK(empty.val == 0);
	CHECK(refused.val == 0);

	ctx_clear_err(ctx);
	BundleRef accepted = bundle_declare(ctx, "spatial", "abcdefgh");

	CHECK(accepted.val != 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_bundle_index_names(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abc");

	CHECK(bundle_find_by_index_name(ctx, 'b').val == spatial.val);
	CHECK(bundle_find_by_index_name(ctx, 'z').val == 0);

	CHECK(bundle_declare(ctx, "internal", "ija").val == 0);
	CHECK(strcmp(ctx->err.msg,
				 "bundle 'internal': index name 'a' is already used.") == 0);
	ctx_clear_err(ctx);

	CHECK(bundle_declare(ctx, "internal", "").val == 0);
	ctx_clear_err(ctx);

	CHECK(bundle_declare(ctx, "internal", "i^").val == 0);
	ctx_clear_err(ctx);

	CHECK(bundle_declare(ctx, "internal", "iji").val == 0);
	ctx_clear_err(ctx);

	CHECK(bundle_declare(ctx, "internal", "ijk").val != 0);

	ctx_destroy(ctx);

	return fails;
}

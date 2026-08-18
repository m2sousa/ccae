#include <stddef.h>
#include <string.h>

#include <ccae/core/error.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>

#include "t_utils.h"

static size_t test_bundle_declare(void);
static size_t test_bundle_duplicate(void);
static size_t test_bundle_empty_name(void);
static size_t test_bundle_after_error(void);
static size_t test_bundle_index_names(void);
static size_t test_bundle_dim(void);
static size_t test_bundle_kronecker(void);

size_t test_bundle(void) {
	size_t fails = 0;
	fails += test_bundle_declare();
	fails += test_bundle_duplicate();
	fails += test_bundle_empty_name();
	fails += test_bundle_after_error();
	fails += test_bundle_index_names();
	fails += test_bundle_dim();
	fails += test_bundle_kronecker();

	return fails;
}

static size_t test_bundle_declare(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh", 3);
	BundleRef internal = bundle_declare(ctx, "internal", "ijklmn", 3);

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

	BundleRef first = bundle_declare(ctx, "spatial", "abcdefgh", 3);
	BundleRef second = bundle_declare(ctx, "spatial", "abcdefgh", 3);

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

	BundleRef empty = bundle_declare(ctx, "", "abc", 3);

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

	BundleRef empty = bundle_declare(ctx, "", "abc", 3);
	BundleRef refused = bundle_declare(ctx, "spatial", "abcdefgh", 3);

	CHECK(empty.val == 0);
	CHECK(refused.val == 0);

	ctx_clear_err(ctx);
	BundleRef accepted = bundle_declare(ctx, "spatial", "abcdefgh", 3);

	CHECK(accepted.val != 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_bundle_index_names(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abc", 3);

	CHECK(bundle_find_by_index_name(ctx, 'b').val == spatial.val);
	CHECK(bundle_find_by_index_name(ctx, 'z').val == 0);

	CHECK(bundle_declare(ctx, "internal", "ija", 3).val == 0);
	CHECK(strcmp(ctx->err.msg,
				 "bundle 'internal': index name 'a' is already used.") == 0);
	ctx_clear_err(ctx);

	CHECK(bundle_declare(ctx, "internal", "", 3).val == 0);
	ctx_clear_err(ctx);

	CHECK(bundle_declare(ctx, "internal", "i^", 3).val == 0);
	ctx_clear_err(ctx);

	CHECK(bundle_declare(ctx, "internal", "iji", 3).val == 0);
	ctx_clear_err(ctx);

	CHECK(bundle_declare(ctx, "internal", "ijk", 3).val != 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_bundle_dim(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh", 3);
	CHECK(spatial.val != 0);
	CHECK(vec_at(ctx->bundles, spatial.val).dim == 3);

	CHECK(bundle_declare(ctx, "internal", "ijk", 0).val == 0);
	CHECK(!ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static bool kronecker_refused(Context* ctx, BundleRef bundle, SymRef sym) {
	bundle_set_kronecker(ctx, bundle, sym);
	bool refused = !ctx_ok(ctx);

	ctx_clear_err(ctx);

	return refused;
}

static size_t test_bundle_kronecker(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	BundleRef internal = bundle_declare(ctx, "internal", "ijk", 3);

	SymRef delta =
		symbol_declare_tensor(ctx, "delta", SYM_CONST, "^a_b", "", 0);
	SymRef field = symbol_declare_tensor(ctx, "f", SYM_FIELD, "^a_b", "", 0);
	SymRef both_up = symbol_declare_tensor(ctx, "u", SYM_CONST, "^a^b", "", 0);
	SymRef down_up = symbol_declare_tensor(ctx, "w", SYM_CONST, "_a^b", "", 0);
	SymRef mixed = symbol_declare_tensor(ctx, "m", SYM_CONST, "^a_i", "", 0);
	SymRef one_slot = symbol_declare_tensor(ctx, "v", SYM_CONST, "^a", "", 0);
	CHECK(ctx_ok(ctx));

	CHECK(kronecker_refused(ctx, spatial, field));
	CHECK(kronecker_refused(ctx, spatial, both_up));
	CHECK(kronecker_refused(ctx, spatial, down_up));
	CHECK(kronecker_refused(ctx, spatial, mixed));
	CHECK(kronecker_refused(ctx, spatial, one_slot));
	CHECK(kronecker_refused(ctx, internal, delta));

	CHECK(!kronecker_refused(ctx, spatial, delta));
	CHECK(vec_at(ctx->bundles, spatial.val).kronecker.val == delta.val);

	// One delta per bundle.
	CHECK(kronecker_refused(ctx, spatial, delta));

	ctx_destroy(ctx);

	return fails;
}

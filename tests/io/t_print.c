#include <stddef.h>
#include <string.h>

#include <ccae/io/print.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>

#include "t_utils.h"

static size_t test_print_tensor(void);
static size_t test_print_scalar(void);
static size_t test_print_truncate(void);

size_t test_print(void) {
	size_t fails = 0;
	fails += test_print_tensor();
	fails += test_print_scalar();
	fails += test_print_truncate();

	return fails;
}

static size_t test_print_tensor(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh");
	BundleRef internal = bundle_declare(ctx, "internal", "ijklmn");
	CHECK(spatial.val != 0 && internal.val != 0);

	SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a",
									  F_PHASE_SPACE | F_ZERO_MODE_FREE);
	SymRef eps = symbol_declare_tensor(ctx, "eps", SYM_CONST, "_c_a_b", 0);

	char buf[64];

	print_symbol(ctx, dK, buf, sizeof(buf));
	CHECK(strcmp(buf, "dK^i_a field PHASE_SPACE|ZERO_MODE_FREE") == 0);

	print_symbol(ctx, eps, buf, sizeof(buf));
	CHECK(strcmp(buf, "eps_a_b_c const CONST_IN_SPACE") == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_print_scalar(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef kappa = symbol_declare_const(ctx, "kappa", F_POSITIVE);

	char buf[64];
	print_symbol(ctx, kappa, buf, sizeof(buf));

	CHECK(strcmp(buf, "kappa const CONST_IN_SPACE|POSITIVE") == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_print_truncate(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef kappa = symbol_declare_const(ctx, "kappa", 0);

	char buf[4];
	print_symbol(ctx, kappa, buf, sizeof(buf));

	CHECK(strcmp(buf, "kap") == 0);

	ctx_destroy(ctx);

	return fails;
}

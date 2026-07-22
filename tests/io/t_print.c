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
static size_t test_print_latex(void);
static size_t test_print_table(void);

size_t test_print(void) {
	size_t fails = 0;
	fails += test_print_tensor();
	fails += test_print_scalar();
	fails += test_print_truncate();
	fails += test_print_latex();
	fails += test_print_table();

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

	print_symbol(ctx, dK, PRINT_ASCII, buf, sizeof(buf));
	CHECK(strcmp(buf, "dK^i_a field PHASE_SPACE|ZERO_MODE_FREE") == 0);

	print_symbol(ctx, eps, PRINT_ASCII, buf, sizeof(buf));
	CHECK(strcmp(buf, "eps_a_b_c const CONST_IN_SPACE") == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_print_scalar(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef kappa = symbol_declare_const(ctx, "kappa", F_POSITIVE | F_NONZERO);

	char buf[64];
	print_symbol(ctx, kappa, PRINT_ASCII, buf, sizeof(buf));

	CHECK(strcmp(buf, "kappa const CONST_IN_SPACE|POSITIVE|NONZERO") == 0);

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
	print_symbol(ctx, kappa, PRINT_ASCII, buf, sizeof(buf));

	CHECK(strcmp(buf, "kap") == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_print_latex(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh");
	BundleRef internal = bundle_declare(ctx, "internal", "ijklmn");
	CHECK(spatial.val != 0 && internal.val != 0);

	SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", 0);
	SymRef eps = symbol_declare_tensor(ctx, "eps", SYM_CONST, "_a_b_c", 0);
	symbol_set_latex(ctx, dK, "\\delta K");

	char buf[64];

	print_symbol(ctx, dK, PRINT_LATEX, buf, sizeof(buf));
	CHECK(strcmp(buf, "\\delta K^{i}{}_{a} field") == 0);

	print_symbol(ctx, eps, PRINT_LATEX, buf, sizeof(buf));
	CHECK(strcmp(buf, "eps_{abc} const CONST_IN_SPACE") == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_print_table(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	char buf[128];

	print_symbol_table(ctx, PRINT_ASCII, buf, sizeof(buf));
	CHECK(strcmp(buf, "") == 0);

	SymRef kappa = symbol_declare_const(ctx, "kappa", F_POSITIVE);
	SymRef p = symbol_declare_homog(ctx, "p", F_PHASE_SPACE);
	CHECK(kappa.val != 0 && p.val != 0);

	print_symbol_table(ctx, PRINT_ASCII, buf, sizeof(buf));
	CHECK(strcmp(buf, "kappa const CONST_IN_SPACE|POSITIVE\n"
					  "p homog CONST_IN_SPACE|PHASE_SPACE\n") == 0);

	ctx_destroy(ctx);

	return fails;
}

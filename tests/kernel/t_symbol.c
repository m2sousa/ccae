#include <stddef.h>
#include <string.h>

#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>
#include <ccae/types.h>

#include "t_utils.h"

static size_t test_symbol_declare(void);
static size_t test_symbol_duplicate(void);
static size_t test_symbol_bad_slotspec(void);
static size_t test_symbol_bad_flags(void);
static size_t test_symbol_latex(void);

size_t test_symbol(void) {
	size_t fails = 0;
	fails += test_symbol_declare();
	fails += test_symbol_duplicate();
	fails += test_symbol_bad_slotspec();
	fails += test_symbol_bad_flags();
	fails += test_symbol_latex();

	return fails;
}

// A context with a spatial (a…h) and an internal (i…n) bundle.
static Context* ctx_with_bundles(void) {
	Context* ctx = ctx_create();
	if (!ctx)
		return NULL;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh");
	BundleRef internal = bundle_declare(ctx, "internal", "ijklmn");

	if (!spatial.val || !internal.val) {
		ctx_destroy(ctx);
		return NULL;
	}

	return ctx;
}

// Declares one symbol in a fresh context, and tells whether it was refused
// with a message.
static bool is_rejected(SymKind kind, const char* slotspec, u16 flags) {
	Context* ctx = ctx_with_bundles();
	if (!ctx)
		return false;

	SymRef sym = symbol_declare_tensor(ctx, "T", kind, slotspec, flags);
	bool rejected = sym.val == 0 && ctx->err.code == CCAE_INVALID_ARG &&
					ctx->err.msg[0] != '\0';

	ctx_destroy(ctx);

	return rejected;
}

static size_t test_symbol_declare(void) {
	size_t fails = 0;

	Context* ctx = ctx_with_bundles();
	if (!ctx)
		return 1;

	SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a",
									  F_PHASE_SPACE | F_ZERO_MODE_FREE);
	SymRef eps = symbol_declare_tensor(ctx, "eps", SYM_CONST, "_a_b_c", 0);
	SymRef dphi = symbol_declare_tensor(ctx, "dphi", SYM_FIELD, "",
										F_PHASE_SPACE | F_ZERO_MODE_FREE);

	CHECK(ctx_ok(ctx));
	CHECK(dK.val != 0 && eps.val != 0 && dphi.val != 0);

	Symbol dK_sym = vec_at(ctx->symbols, dK.val);
	BundleRef spatial = bundle_find_by_index_name(ctx, 'a');
	BundleRef internal = bundle_find_by_index_name(ctx, 'i');

	CHECK(dK_sym.nslot == 2);
	CHECK(dK_sym.slot_bundle[0].val == internal.val);
	CHECK(dK_sym.slot_pos[0] == SLOT_UP);
	CHECK(dK_sym.slot_bundle[1].val == spatial.val);
	CHECK(dK_sym.slot_pos[1] == SLOT_DOWN);

	CHECK(vec_at(ctx->symbols, eps.val).flags & F_CONST_IN_SPACE);
	CHECK(vec_at(ctx->symbols, dphi.val).nslot == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_symbol_duplicate(void) {
	size_t fails = 0;

	Context* ctx = ctx_with_bundles();
	if (!ctx)
		return 1;

	SymRef first = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "^a_i", 0);
	SymRef second = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "^a_i", 0);

	CHECK(first.val != 0);
	CHECK(second.val == 0);
	CHECK(ctx->err.code == CCAE_INVALID_ARG);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_symbol_bad_slotspec(void) {
	size_t fails = 0;

	CHECK(is_rejected(SYM_FIELD, "a", 0));				// no ^ or _
	CHECK(is_rejected(SYM_FIELD, "^", 0));				// no index name
	CHECK(is_rejected(SYM_FIELD, "^z", 0));				// z in no bundle
	CHECK(is_rejected(SYM_FIELD, "_a_b_c_d_e_f_g", 0)); // 7 slots

	return fails;
}

static size_t test_symbol_bad_flags(void) {
	size_t fails = 0;

	CHECK(is_rejected(SYM_FIELD, "^a", F_POSITIVE));
	CHECK(is_rejected(SYM_CONST, "", F_PHASE_SPACE));
	CHECK(is_rejected(SYM_FIELD, "", F_CONST_IN_SPACE));
	CHECK(is_rejected(SYM_CONST, "", F_ZERO_MODE_FREE));
	CHECK(is_rejected(SYM_FIELD, "", 1 << 15));

	CHECK(!is_rejected(SYM_CONST, "", F_POSITIVE));
	CHECK(!is_rejected(SYM_TESTFN, "", F_CONST_IN_SPACE));

	return fails;
}

static size_t test_symbol_latex(void) {
	size_t fails = 0;

	Context* ctx = ctx_with_bundles();
	if (!ctx)
		return 1;

	SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", 0);

	StringRef before = vec_at(ctx->symbols, dK.val).latex;
	CHECK(strcmp(sp_get(ctx->sp, before), "") == 0);

	symbol_set_latex(ctx, dK, "\\delta K");

	StringRef after = vec_at(ctx->symbols, dK.val).latex;
	CHECK(strcmp(sp_get(ctx->sp, after), "\\delta K") == 0);

	ctx_destroy(ctx);

	return fails;
}

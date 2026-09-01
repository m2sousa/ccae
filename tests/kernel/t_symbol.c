#include <stddef.h>
#include <string.h>

#include <ccae/config.h>
#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>
#include <ccae/kernel/symmetry.h>
#include <ccae/types.h>

#include "t_utils.h"

static size_t test_symbol_declare(void);
static size_t test_symbol_duplicate(void);
static size_t test_symbol_bad_slotspec(void);
static size_t test_symbol_bad_flags(void);
static size_t test_symbol_latex(void);
static size_t test_symbol_homog(void);
static size_t test_symbol_const(void);
static size_t test_symbol_too_many_slots(void);
static size_t test_symbol_symmetry(void);
static size_t test_symbol_inverse(void);
static size_t test_symbol_undet_fn(void);

size_t test_symbol(void) {
	size_t fails = 0;
	fails += test_symbol_declare();
	fails += test_symbol_duplicate();
	fails += test_symbol_bad_slotspec();
	fails += test_symbol_bad_flags();
	fails += test_symbol_latex();
	fails += test_symbol_homog();
	fails += test_symbol_const();
	fails += test_symbol_too_many_slots();
	fails += test_symbol_symmetry();
	fails += test_symbol_inverse();
	fails += test_symbol_undet_fn();

	return fails;
}

// A context with a spatial (a…h) and an internal (i…n) bundle.
static Context* create_ctx_with_bundles(void) {
	Context* ctx = ctx_create();
	if (!ctx)
		return NULL;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh", 3);
	BundleRef internal = bundle_declare(ctx, "internal", "ijklmn", 3);

	if (!spatial.val || !internal.val) {
		ctx_destroy(ctx);
		return NULL;
	}

	return ctx;
}

// Declares one symbol in a fresh context, and tells whether it was refused
// with a message.
static bool is_rejected(SymKind kind, const char* slotspec, u16 flags) {
	Context* ctx = create_ctx_with_bundles();
	if (!ctx)
		return false;

	SymRef sym = symbol_declare_tensor(ctx, "T", kind, slotspec, "", flags);
	bool rejected = sym.val == 0 && ctx->err.code == CCAE_INVALID_ARG &&
					ctx->err.msg[0] != '\0';

	ctx_destroy(ctx);

	return rejected;
}

static size_t test_symbol_declare(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_bundles();
	if (!ctx)
		return 1;

	SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", "",
									  F_PHASE_SPACE | F_ZERO_MODE_FREE);
	SymRef eps = symbol_declare_tensor(ctx, "eps", SYM_CONST, "_a_b_c", "", 0);
	SymRef dphi = symbol_declare_tensor(ctx, "dphi", SYM_FIELD, "", "",
										F_PHASE_SPACE | F_ZERO_MODE_FREE);

	CHECK(ctx_ok(ctx));
	CHECK(dK.val != 0 && eps.val != 0 && dphi.val != 0);

	Symbol dK_sym = vec_at(ctx->symbols, dK.val);
	BundleRef spatial = bundle_find_by_index_name(ctx, 'a');
	BundleRef internal = bundle_find_by_index_name(ctx, 'i');

	CHECK(dK_sym.nbr_slot == 2);
	CHECK(dK_sym.slot_bundle[0].val == internal.val);
	CHECK(dK_sym.slot_pos[0] == SLOT_UP);
	CHECK(dK_sym.slot_bundle[1].val == spatial.val);
	CHECK(dK_sym.slot_pos[1] == SLOT_DOWN);

	CHECK(vec_at(ctx->symbols, eps.val).flags & F_CONST_IN_SPACE);
	CHECK(vec_at(ctx->symbols, dphi.val).nbr_slot == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_symbol_duplicate(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_bundles();
	if (!ctx)
		return 1;

	SymRef first = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "^a_i", "", 0);
	SymRef second = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "^a_i", "", 0);

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
	CHECK(is_rejected(SYM_CONST, "^a", F_NONZERO));
	CHECK(is_rejected(SYM_CONST, "", F_PHASE_SPACE));
	CHECK(is_rejected(SYM_FIELD, "", F_CONST_IN_SPACE));
	CHECK(is_rejected(SYM_CONST, "", F_ZERO_MODE_FREE));
	CHECK(is_rejected(SYM_FIELD, "", 1 << 15));

	CHECK(!is_rejected(SYM_CONST, "", F_POSITIVE));
	CHECK(!is_rejected(SYM_CONST, "", F_NONZERO));
	CHECK(!is_rejected(SYM_TESTFN, "", F_CONST_IN_SPACE));

	return fails;
}

static size_t test_symbol_latex(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_bundles();
	if (!ctx)
		return 1;

	SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", "", 0);

	StringRef before = vec_at(ctx->symbols, dK.val).latex;
	CHECK(strcmp(sp_get(ctx->sp, before), "") == 0);

	symbol_set_latex(ctx, dK, "\\delta K");

	StringRef after = vec_at(ctx->symbols, dK.val).latex;
	CHECK(strcmp(sp_get(ctx->sp, after), "\\delta K") == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_symbol_homog(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef p = symbol_declare_homog(ctx, "p", F_PHASE_SPACE | F_POSITIVE);
	CHECK(p.val != 0);

	Symbol p_sym = vec_at(ctx->symbols, p.val);
	CHECK(p_sym.kind == SYM_HOMOG);
	CHECK(p_sym.nbr_slot == 0);
	CHECK(p_sym.flags & F_CONST_IN_SPACE);
	CHECK(p_sym.flags & F_PHASE_SPACE);

	CHECK(symbol_declare_homog(ctx, "c", F_ZERO_MODE_FREE).val == 0);
	ctx_clear_err(ctx);

	CHECK(symbol_declare_tensor(ctx, "c", SYM_HOMOG, "", "", 0).val == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_symbol_const(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef kappa = symbol_declare_const(ctx, "kappa", F_POSITIVE);
	CHECK(kappa.val != 0);

	Symbol kappa_sym = vec_at(ctx->symbols, kappa.val);
	CHECK(kappa_sym.kind == SYM_CONST);
	CHECK(kappa_sym.nbr_slot == 0);
	CHECK(kappa_sym.flags & F_CONST_IN_SPACE);
	CHECK(kappa_sym.flags & F_POSITIVE);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_symbol_too_many_slots(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef small = bundle_declare(ctx, "small", "xy", 2);
	CHECK(small.val != 0);

	CHECK(symbol_declare_tensor(ctx, "A", SYM_FIELD, "_x_y", "", 0).val != 0);

	CHECK(symbol_declare_tensor(ctx, "B", SYM_FIELD, "_x_y_x", "", 0).val == 0);
	CHECK(strcmp(ctx->err.msg, "slotspec '_x_y_x': more slots than index "
							   "names in bundle 'small'.") == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_symbol_symmetry(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_bundles();
	if (!ctx)
		return 1;

	SymRef g =
		symbol_declare_tensor(ctx, "g", SYM_FIELD, "_a_b", "sym(0,1)", 0);
	CHECK(g.val != 0);

	Generators g_gens = vec_at(ctx->symbols, g.val).generators;
	CHECK(g_gens.nbr_generator == 1);
	CHECK(signed_perm_equal(g_gens.generators[0], signed_perm_swap(0, 1, 1)));

	SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", "", 0);
	CHECK(vec_at(ctx->symbols, dK.val).generators.nbr_generator == 0);

	CHECK(vec_at(ctx->symbols, g.val).orbit->nbr_perm == 2);
	CHECK(vec_at(ctx->symbols, dK.val).orbit->nbr_perm == 1);

	CHECK(ctx_ok(ctx));

	// Slots 0 and 1 are of different bundles, then of different positions.
	SymRef A =
		symbol_declare_tensor(ctx, "A", SYM_FIELD, "^i_a", "sym(0,1)", 0);
	CHECK(A.val == 0);
	ctx_clear_err(ctx);

	SymRef B =
		symbol_declare_tensor(ctx, "B", SYM_FIELD, "^a_b", "sym(0,1)", 0);
	CHECK(B.val == 0);
	ctx_clear_err(ctx);

	SymRef C = symbol_declare_tensor(ctx, "C", SYM_FIELD, "_a_b", "sym(0,", 0);
	CHECK(C.val == 0);
	ctx_clear_err(ctx);

	// Slot 1 is internal, but the clause only exchanges slots 0 and 2.
	SymRef D =
		symbol_declare_tensor(ctx, "D", SYM_FIELD, "_a_i_b", "sym(0,2)", 0);
	CHECK(D.val != 0);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static bool inverse_refused(Context* ctx, SymRef sym, SymRef inverse) {
	symbol_set_inverse(ctx, sym, inverse);
	bool refused = !ctx_ok(ctx);

	ctx_clear_err(ctx);

	return refused;
}

static size_t test_symbol_inverse(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_bundles();
	if (!ctx)
		return 1;

	BundleRef plane = bundle_declare(ctx, "plane", "xy", 2);
	CHECK(plane.val != 0);

	SymRef triad =
		symbol_declare_tensor(ctx, "triad", SYM_CONST, "^a_i", "", 0);
	SymRef cotriad =
		symbol_declare_tensor(ctx, "cotriad", SYM_CONST, "_a^i", "", 0);
	SymRef E = symbol_declare_tensor(ctx, "E", SYM_FIELD, "^a_i", "", 0);
	SymRef same_pos = symbol_declare_tensor(ctx, "s", SYM_CONST, "^a_i", "", 0);
	SymRef swapped = symbol_declare_tensor(ctx, "w", SYM_CONST, "_i^a", "", 0);
	SymRef one_slot = symbol_declare_tensor(ctx, "v", SYM_CONST, "_a", "", 0);
	SymRef up_x = symbol_declare_tensor(ctx, "ux", SYM_CONST, "^a_x", "", 0);
	SymRef down_x = symbol_declare_tensor(ctx, "dx", SYM_CONST, "_a^x", "", 0);
	CHECK(ctx_ok(ctx));

	CHECK(inverse_refused(ctx, triad, same_pos));
	CHECK(inverse_refused(ctx, triad, swapped));
	CHECK(inverse_refused(ctx, triad, one_slot));

	// Spatial is 3-d and plane 2-d: no inverse on both sides.
	CHECK(inverse_refused(ctx, up_x, down_x));

	CHECK(!inverse_refused(ctx, triad, cotriad));
	CHECK(vec_at(ctx->symbols, triad.val).inverse.val == cotriad.val);
	CHECK(vec_at(ctx->symbols, cotriad.val).inverse.val == triad.val);

	// One inverse each.
	CHECK(inverse_refused(ctx, E, cotriad));

	ctx_destroy(ctx);

	return fails;
}

static bool undet_fn_refused(Context* ctx, const char* name, u8 nbr_arg,
							 const SymRef* args) {
	SymRef fn = symbol_declare_undet_fn(ctx, name, nbr_arg, args, 0);
	bool refused = fn.val == 0 && !ctx_ok(ctx);

	ctx_clear_err(ctx);

	return refused;
}

static size_t test_symbol_undet_fn(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_bundles();
	if (!ctx)
		return 1;

	SymRef c = symbol_declare_homog(ctx, "c", 0);
	SymRef p = symbol_declare_homog(ctx, "p", 0);
	SymRef E = symbol_declare_tensor(ctx, "E", SYM_FIELD, "^a_i", "", 0);
	CHECK(ctx_ok(ctx));

	// g(c, p): the arguments are kept in order.
	SymRef g = symbol_declare_undet_fn(ctx, "g", 2, (SymRef[]){c, p}, 0);
	CHECK(g.val != 0);

	Symbol symbol = vec_at(ctx->symbols, g.val);
	CHECK(symbol.kind == SYM_UNDET_FN && symbol.nbr_slot == 0);
	CHECK(symbol.nbr_arg == 2);
	CHECK(symbol.args[0].val == c.val && symbol.args[1].val == p.val);

	// Refused: no argument, too many, an argument with slots, a repeated one.
	SymRef many[CCAE_MAX_ARGS + 1];
	for (size_t k = 0; k <= CCAE_MAX_ARGS; ++k)
		many[k] = c;

	CHECK(undet_fn_refused(ctx, "f0", 0, NULL));
	CHECK(undet_fn_refused(ctx, "f1", CCAE_MAX_ARGS + 1, many));
	CHECK(undet_fn_refused(ctx, "f2", 1, (SymRef[]){E}));
	CHECK(undet_fn_refused(ctx, "f3", 2, (SymRef[]){c, c}));

	// The name is checked as for any symbol.
	CHECK(undet_fn_refused(ctx, "g", 1, (SymRef[]){c}));

	ctx_destroy(ctx);

	return fails;
}

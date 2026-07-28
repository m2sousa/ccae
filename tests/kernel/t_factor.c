#include <stddef.h>

#include <ccae/core/rational.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/symbol.h>

#include "t_utils.h"

static size_t test_factor_intern_once(void);
static size_t test_factor_distinct_powers(void);
static size_t test_factor_bad_powers(void);

size_t test_factor(void) {
	size_t fails = 0;
	fails += test_factor_intern_once();
	fails += test_factor_distinct_powers();
	fails += test_factor_bad_powers();

	return fails;
}

static size_t test_factor_intern_once(void) {
	constexpr size_t NBR_INTERNS = 100'000;

	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef p = symbol_declare_homog(ctx, "p", F_POSITIVE | F_NONZERO);
	FacRef first = factor_intern(ctx, p, rat_new(1, 2));

	bool all_same = true;

	for (size_t i = 0; i < NBR_INTERNS; ++i) {
		if (factor_intern(ctx, p, rat_new(2, 4)).val != first.val)
			all_same = false;
	}

	CHECK(first.val != 0 && all_same);

	// The null factor and p^{1/2}.
	CHECK(ctx->factors.length == 2);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_factor_distinct_powers(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef p = symbol_declare_homog(ctx, "p", F_POSITIVE | F_NONZERO);
	SymRef c = symbol_declare_homog(ctx, "c", 0);

	FacRef p_half = factor_intern(ctx, p, rat_new(1, 2));
	FacRef p_minus_half = factor_intern(ctx, p, rat_new(-1, 2));
	FacRef p_one = factor_intern(ctx, p, rat_from_int(1));
	FacRef c_one = factor_intern(ctx, c, rat_from_int(1));

	CHECK(p_half.val != 0 && p_minus_half.val != 0);
	CHECK(p_one.val != 0 && c_one.val != 0);

	CHECK(p_half.val != p_minus_half.val);
	CHECK(p_half.val != p_one.val);
	CHECK(p_one.val != c_one.val);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_factor_bad_powers(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abc");
	SymRef c = symbol_declare_homog(ctx, "c", 0);
	SymRef dE = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "_a", 0);
	CHECK(spatial.val != 0 && ctx_ok(ctx));

	CHECK(factor_intern(ctx, c, rat_from_int(2)).val != 0);

	CHECK(factor_intern(ctx, c, rat_from_int(0)).val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	CHECK(factor_intern(ctx, dE, rat_from_int(2)).val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	CHECK(factor_intern(ctx, c, rat_new(1, 2)).val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	CHECK(factor_intern(ctx, c, rat_from_int(-1)).val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	ctx_destroy(ctx);

	return fails;
}

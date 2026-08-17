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
static size_t test_factor_indices(void);
static size_t test_factor_bad_indices(void);

size_t test_factor(void) {
	size_t fails = 0;
	fails += test_factor_intern_once();
	fails += test_factor_distinct_powers();
	fails += test_factor_bad_powers();
	fails += test_factor_indices();
	fails += test_factor_bad_indices();

	return fails;
}

static size_t test_factor_intern_once(void) {
	constexpr size_t NBR_INTERNS = 100'000;

	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef p = symbol_declare_homog(ctx, "p", F_POSITIVE | F_NONZERO);
	FacRef first = factor_intern(ctx, p, rat_new(1, 2), NULL);

	bool all_same = true;

	for (size_t i = 0; i < NBR_INTERNS; ++i) {
		if (factor_intern(ctx, p, rat_new(2, 4), NULL).val != first.val)
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

	FacRef p_half = factor_intern(ctx, p, rat_new(1, 2), NULL);
	FacRef p_minus_half = factor_intern(ctx, p, rat_new(-1, 2), NULL);
	FacRef p_one = factor_intern(ctx, p, rat_from_int(1), NULL);
	FacRef c_one = factor_intern(ctx, c, rat_from_int(1), NULL);

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

	BundleRef spatial = bundle_declare(ctx, "spatial", "abc", 3);
	SymRef c = symbol_declare_homog(ctx, "c", 0);
	SymRef dE = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "_a", "", 0);
	CHECK(spatial.val != 0 && ctx_ok(ctx));

	CHECK(factor_intern(ctx, c, rat_from_int(2), NULL).val != 0);

	CHECK(factor_intern(ctx, c, rat_from_int(0), NULL).val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	CHECK(factor_intern(ctx, dE, rat_from_int(2), (Index[]){{IDX_FREE, 'a'}})
			  .val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	CHECK(factor_intern(ctx, c, rat_new(1, 2), NULL).val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	CHECK(factor_intern(ctx, c, rat_from_int(-1), NULL).val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_factor_indices(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abc", 3);
	BundleRef internal = bundle_declare(ctx, "internal", "ijk", 3);
	SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", "", 0);
	CHECK(spatial.val != 0 && internal.val != 0 && ctx_ok(ctx));

	Rational one = rat_from_int(1);

	FacRef dK_ia = factor_intern(ctx, dK, one,
								 (Index[]){{IDX_FREE, 'i'}, {IDX_FREE, 'a'}});
	FacRef dK_ib = factor_intern(ctx, dK, one,
								 (Index[]){{IDX_FREE, 'i'}, {IDX_FREE, 'b'}});
	FacRef dK_dummy =
		factor_intern(ctx, dK, one, (Index[]){{IDX_FREE, 'i'}, {IDX_DUMMY, 0}});

	CHECK(dK_ia.val != 0 && dK_ib.val != 0 && dK_dummy.val != 0);
	CHECK(dK_ia.val != dK_ib.val);
	CHECK(dK_ia.val != dK_dummy.val);

	FacRef again = factor_intern(ctx, dK, one,
								 (Index[]){{IDX_FREE, 'i'}, {IDX_FREE, 'a'}});
	CHECK(again.val == dK_ia.val);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_factor_bad_indices(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abc", 3);
	BundleRef internal = bundle_declare(ctx, "internal", "ijk", 3);
	SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", "", 0);
	CHECK(spatial.val != 0 && internal.val != 0 && ctx_ok(ctx));

	Rational one = rat_from_int(1);

	CHECK(factor_intern(ctx, dK, one, NULL).val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	// `a` is spatial, but the first slot of `dK` is internal.
	CHECK(
		factor_intern(ctx, dK, one, (Index[]){{IDX_FREE, 'a'}, {IDX_FREE, 'a'}})
			.val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	// `z` belongs to no bundle.
	CHECK(
		factor_intern(ctx, dK, one, (Index[]){{IDX_FREE, 'i'}, {IDX_FREE, 'z'}})
			.val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	ctx_destroy(ctx);

	return fails;
}

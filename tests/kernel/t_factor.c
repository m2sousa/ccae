#include <stddef.h>

#include <ccae/config.h>
#include <ccae/core/rational.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

#include "t_kernel_utils.h"
#include "t_utils.h"

static size_t test_factor_intern_once(void);
static size_t test_factor_distinct_powers(void);
static size_t test_factor_bad_powers(void);
static size_t test_factor_indices(void);
static size_t test_factor_bad_indices(void);
static size_t test_factor_derive(void);
static size_t test_factor_cmp(void);
static size_t test_factor_replace_indices(void);
static size_t test_factor_build_indices(void);
static size_t test_factor_partial(void);

size_t test_factor(void) {
	size_t fails = 0;
	fails += test_factor_intern_once();
	fails += test_factor_distinct_powers();
	fails += test_factor_bad_powers();
	fails += test_factor_indices();
	fails += test_factor_bad_indices();
	fails += test_factor_derive();
	fails += test_factor_cmp();
	fails += test_factor_replace_indices();
	fails += test_factor_build_indices();
	fails += test_factor_partial();

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

static size_t test_factor_derive(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	BundleRef internal = bundle_declare(ctx, "internal", "ijk", 3);
	SymRef X = symbol_declare_tensor(ctx, "X", SYM_FIELD, "^c", "", 0);
	SymRef p = symbol_declare_homog(ctx, "p", 0);
	CHECK(ctx_ok(ctx));

	Index a = {IDX_FREE, 'a'};
	Index b = {IDX_FREE, 'b'};
	Index i = {IDX_FREE, 'i'};

	FacRef X_c = with_free_indices(ctx, X, "c");
	FacRef p_one = factor_intern(ctx, p, rat_from_int(1), NULL);

	// No bundle carries the derivative yet.
	CHECK(factor_derive(ctx, X_c, a).val == 0);
	ctx_clear_err(ctx);

	bundle_set_derivative(ctx, spatial);
	CHECK(ctx_ok(ctx));

	// One derivative bundle only.
	bundle_set_derivative(ctx, internal);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	// ∂_a ∂_b X^c and ∂_b ∂_a X^c are the same factor.
	FacRef d_a = factor_derive(ctx, X_c, a);
	FacRef d_ab = factor_derive(ctx, d_a, b);
	FacRef d_ba = factor_derive(ctx, factor_derive(ctx, X_c, b), a);
	CHECK(d_a.val != 0 && d_a.val != X_c.val);
	CHECK(d_ab.val != 0 && d_ab.val == d_ba.val);
	CHECK(vec_at(ctx->factors, d_ab.val).nbr_der == 2);

	// Refused: a power, an internal index, a constant in space, too many
	// indices.
	SymRef y = symbol_declare_tensor(ctx, "y", SYM_FIELD, "", "", 0);
	FacRef y_two = factor_intern(ctx, y, rat_from_int(2), NULL);
	CHECK(factor_derive(ctx, y_two, a).val == 0);
	ctx_clear_err(ctx);

	CHECK(factor_derive(ctx, X_c, i).val == 0);
	ctx_clear_err(ctx);

	CHECK(factor_derive(ctx, p_one, a).val == 0);
	ctx_clear_err(ctx);

	FacRef many = X_c;
	for (size_t k = 0; k < CCAE_MAX_DERIVATIVES; ++k)
		many = factor_derive(ctx, many, a);
	CHECK(many.val != 0);
	CHECK(factor_derive(ctx, many, a).val == 0);
	ctx_clear_err(ctx);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_factor_cmp(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	bundle_set_derivative(ctx, spatial);

	SymRef x = symbol_declare_tensor(ctx, "x", SYM_FIELD, "", "", 0);
	SymRef T = symbol_declare_tensor(ctx, "T", SYM_FIELD, "^a^b", "", 0);
	CHECK(ctx_ok(ctx));

	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef x_two = factor_intern(ctx, x, rat_from_int(2), NULL);
	FacRef dx_a = factor_derive(ctx, x_one, (Index){IDX_FREE, 'a'});
	FacRef dx_b = factor_derive(ctx, x_one, (Index){IDX_FREE, 'b'});
	FacRef T_ab = with_free_indices(ctx, T, "ab");
	FacRef T_ba = with_free_indices(ctx, T, "ba");

	// The head first: x is declared before T.
	CHECK(factor_cmp(ctx, x_two, T_ab) == -1);

	// Then the derivative indices, before the exponent: x² comes before ∂_a x.
	CHECK(factor_cmp(ctx, x_two, dx_a) == -1);
	CHECK(factor_cmp(ctx, dx_a, dx_b) == -1);

	// Then the exponent, then the indices slot by slot.
	CHECK(factor_cmp(ctx, x_one, x_two) == -1);
	CHECK(factor_cmp(ctx, T_ba, T_ab) == 1);
	CHECK(factor_cmp(ctx, T_ab, T_ab) == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_factor_replace_indices(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	CHECK(bundle_declare(ctx, "internal", "ijk", 3).val != 0);
	bundle_set_derivative(ctx, spatial);

	SymRef X = symbol_declare_tensor(ctx, "X", SYM_FIELD, "^c", "", 0);
	CHECK(ctx_ok(ctx));

	Index a = {IDX_FREE, 'a'};
	Index b = {IDX_FREE, 'b'};
	Index c = {IDX_FREE, 'c'};
	Index d = {IDX_FREE, 'd'};
	Index i = {IDX_FREE, 'i'};
	Index d0 = {IDX_DUMMY, 0};

	FacRef X_c = with_free_indices(ctx, X, "c");
	FacRef dX_ab = factor_derive(ctx, factor_derive(ctx, X_c, a), b);

	// The slot first, then the derivative indices: ∂_a ∂_b X^c is (c, a, b).
	CHECK(factor_replace_indices(ctx, dX_ab, (Index[]){c, a, b}).val ==
		  dX_ab.val);

	// A dummy in the slot: ∂_a ∂_b X^$0.
	FacRef dX_ab_0 = factor_replace_indices(ctx, dX_ab, (Index[]){d0, a, b});
	CHECK(dX_ab_0.val != 0 && dX_ab_0.val != dX_ab.val);
	CHECK(vec_at(ctx->factors, dX_ab_0.val).nbr_der == 2);

	// The derivative indices are sorted again: (c, d, a) is ∂_a ∂_d X^c.
	FacRef dX_ad = factor_derive(ctx, factor_derive(ctx, X_c, a), d);
	CHECK(factor_replace_indices(ctx, dX_ab, (Index[]){c, d, a}).val ==
		  dX_ad.val);

	// Refused: an internal index in the slot, or as a derivative index.
	CHECK(factor_replace_indices(ctx, dX_ab, (Index[]){i, a, b}).val == 0);
	ctx_clear_err(ctx);

	CHECK(factor_replace_indices(ctx, dX_ab, (Index[]){c, i, b}).val == 0);
	ctx_clear_err(ctx);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_factor_build_indices(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	bundle_set_derivative(ctx, spatial);

	SymRef T = symbol_declare_tensor(ctx, "T", SYM_FIELD, "^c_d", "", 0);
	SymRef x = symbol_declare_tensor(ctx, "x", SYM_FIELD, "", "", 0);
	CHECK(ctx_ok(ctx));

	Index a = {IDX_FREE, 'a'};
	Index b = {IDX_FREE, 'b'};

	FacRef dT = factor_derive(
		ctx, factor_derive(ctx, with_free_indices(ctx, T, "cd"), b), a);

	Index indices[CCAE_MAX_FACTOR_INDICES];
	CHECK(factor_build_indices(indices, ctx, dT) == 4);
	CHECK(factor_count_indices(ctx, dT) == 4);
	CHECK(indices[0].id == 'c' && indices[1].id == 'd');
	CHECK(indices[2].id == 'a' && indices[3].id == 'b');

	// A scalar without derivative indices has none.
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	CHECK(factor_build_indices(indices, ctx, x_one) == 0);
	CHECK(factor_count_indices(ctx, x_one) == 0);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_factor_partial(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef c = symbol_declare_homog(ctx, "c", 0);
	SymRef p = symbol_declare_homog(ctx, "p", 0);
	SymRef x = symbol_declare_tensor(ctx, "x", SYM_FIELD, "", "", 0);
	SymRef g = symbol_declare_undet_fn(ctx, "g", 2, (SymRef[]){c, p}, 0);
	CHECK(ctx_ok(ctx));

	FacRef g_one = factor_intern(ctx, g, rat_from_int(1), NULL);
	FacRef g_c = factor_partial(ctx, g_one, 0);
	CHECK(g_c.val != 0 && g_c.val != g_one.val);

	// Partials commute ∂_p ∂_c g and ∂_c ∂_p g are one factor.
	FacRef g_cp = factor_partial(ctx, g_c, 1);
	FacRef g_pc = factor_partial(ctx, factor_partial(ctx, g_one, 1), 0);
	CHECK(g_cp.val != 0 && g_cp.val == g_pc.val);
	CHECK(vec_at(ctx->factors, g_cp.val).partial_order[0] == 1);
	CHECK(vec_at(ctx->factors, g_cp.val).partial_order[1] == 1);

	FacRef g_two = factor_intern(ctx, g, rat_from_int(2), NULL);
	CHECK(factor_cmp(ctx, g_one, g_two) == -1);
	CHECK(factor_cmp(ctx, g_two, g_c) == -1);

	CHECK(vec_at(ctx->monomials,
				 mono_intern(ctx, (FacRef[]){g_one, g_one}, 2).val)
			  .nbr_fac == 1);
	CHECK(
		vec_at(ctx->monomials, mono_intern(ctx, (FacRef[]){g_one, g_c}, 2).val)
			.nbr_fac == 2);

	// Refused: not a function, an unknown argument, a power.
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	CHECK(factor_partial(ctx, x_one, 0).val == 0);
	ctx_clear_err(ctx);

	CHECK(factor_partial(ctx, g_one, 2).val == 0);
	ctx_clear_err(ctx);

	CHECK(factor_partial(ctx, g_two, 0).val == 0);
	ctx_clear_err(ctx);

	ctx_destroy(ctx);

	return fails;
}

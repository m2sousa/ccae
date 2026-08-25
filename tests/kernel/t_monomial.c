#include <stddef.h>

#include <ccae/config.h>
#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

#include "t_kernel_utils.h"
#include "t_utils.h"

static size_t test_monomial_order(void);
static size_t test_monomial_one(void);
static size_t test_monomial_too_many(void);
static size_t test_monomial_merge(void);
static size_t test_monomial_tensors_stay(void);
static size_t test_monomial_mul(void);
static size_t test_monomial_dummies(void);
static size_t test_monomial_bad_dummies(void);
static size_t test_monomial_mul_dummies(void);
static size_t test_monomial_content_order(void);
static size_t test_monomial_derivatives(void);

size_t test_monomial(void) {
	size_t fails = 0;
	fails += test_monomial_order();
	fails += test_monomial_one();
	fails += test_monomial_too_many();
	fails += test_monomial_merge();
	fails += test_monomial_tensors_stay();
	fails += test_monomial_mul();
	fails += test_monomial_dummies();
	fails += test_monomial_bad_dummies();
	fails += test_monomial_mul_dummies();
	fails += test_monomial_content_order();
	fails += test_monomial_derivatives();

	return fails;
}

static size_t test_monomial_order(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef c = symbol_declare_homog(ctx, "c", 0);
	SymRef p = symbol_declare_homog(ctx, "p", F_POSITIVE);

	FacRef c_one = factor_intern(ctx, c, rat_from_int(1), NULL);
	FacRef sqrt_p = factor_intern(ctx, p, rat_new(1, 2), NULL);

	MonoRef c_p = mono_intern(ctx, (FacRef[]){c_one, sqrt_p}, 2);
	MonoRef p_c = mono_intern(ctx, (FacRef[]){sqrt_p, c_one}, 2);

	CHECK(c_p.val != 0 && c_p.val == p_c.val);

	// c is declared before p, so it comes first.
	Monomial stored = vec_at(ctx->monomials, c_p.val);
	CHECK(stored.nbr_fac == 2);
	CHECK(stored.fac[0].val == c_one.val && stored.fac[1].val == sqrt_p.val);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_monomial_one(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef c = symbol_declare_homog(ctx, "c", 0);
	FacRef c_one = factor_intern(ctx, c, rat_from_int(1), NULL);

	MonoRef one = mono_intern(ctx, NULL, 0);
	MonoRef just_c = mono_intern(ctx, (FacRef[]){c_one}, 1);

	CHECK(one.val != 0 && just_c.val != 0);
	CHECK(one.val != just_c.val);
	CHECK(mono_intern(ctx, NULL, 0).val == one.val);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_monomial_too_many(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abc", 3);
	SymRef X = symbol_declare_tensor(ctx, "X", SYM_FIELD, "^a", "", 0);
	SymRef dE = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "_a", "", 0);
	FacRef dE_a =
		factor_intern(ctx, dE, rat_from_int(1), (Index[]){{IDX_FREE, 'a'}});
	CHECK(spatial.val != 0 && X.val != 0 && dE_a.val != 0);

	// `X^d dE_d` pairs, one dummy `d` each.
	FacRef facs[CCAE_MAX_FACTORS + 1];
	for (u16 i = 0; i < CCAE_MAX_FACTORS / 2; ++i) {
		Index dummy = {.kind = IDX_DUMMY, .id = i};
		facs[2 * i] = factor_intern(ctx, X, rat_from_int(1), &dummy);
		facs[2 * i + 1] = factor_intern(ctx, dE, rat_from_int(1), &dummy);
	}
	facs[CCAE_MAX_FACTORS] = dE_a;

	MonoRef full = mono_intern(ctx, facs, CCAE_MAX_FACTORS);
	MonoRef just_dE = mono_intern(ctx, &dE_a, 1);
	CHECK(full.val != 0 && just_dE.val != 0);

	CHECK(mono_intern(ctx, facs, CCAE_MAX_FACTORS + 1).val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	CHECK(mono_mul(ctx, full, just_dE).val == 0);
	CHECK(!ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_monomial_merge(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	SymRef y = symbol_declare_homog(ctx, "y", 0);
	SymRef p = symbol_declare_homog(ctx, "p", F_POSITIVE | F_NONZERO);

	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef x_two = factor_intern(ctx, x, rat_from_int(2), NULL);
	FacRef y_one = factor_intern(ctx, y, rat_from_int(1), NULL);
	FacRef p_half = factor_intern(ctx, p, rat_new(1, 2), NULL);
	FacRef p_minus_half = factor_intern(ctx, p, rat_new(-1, 2), NULL);

	MonoRef x_x = mono_intern(ctx, (FacRef[]){x_one, x_one}, 2);
	CHECK(x_x.val == mono_intern(ctx, (FacRef[]){x_two}, 1).val);

	MonoRef x_y_x = mono_intern(ctx, (FacRef[]){x_one, y_one, x_one}, 3);
	CHECK(x_y_x.val == mono_intern(ctx, (FacRef[]){x_two, y_one}, 2).val);

	MonoRef p_cancel = mono_intern(ctx, (FacRef[]){p_half, p_minus_half}, 2);
	CHECK(p_cancel.val == mono_intern(ctx, NULL, 0).val);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_monomial_tensors_stay(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abc", 3);
	SymRef dE = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "_a", "", 0);
	FacRef dE_one =
		factor_intern(ctx, dE, rat_from_int(1), (Index[]){{IDX_FREE, 'a'}});
	FacRef dE_b =
		factor_intern(ctx, dE, rat_from_int(1), (Index[]){{IDX_FREE, 'b'}});
	CHECK(spatial.val != 0 && dE_one.val != 0 && dE_b.val != 0);

	MonoRef dE_dE = mono_intern(ctx, (FacRef[]){dE_one, dE_b}, 2);
	CHECK(vec_at(ctx->monomials, dE_dE.val).nbr_fac == 2);

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef x_two = factor_intern(ctx, x, rat_from_int(2), NULL);

	MonoRef x_dE_x = mono_intern(ctx, (FacRef[]){x_one, dE_one, x_one}, 3);
	CHECK(x_dE_x.val == mono_intern(ctx, (FacRef[]){x_two, dE_one}, 2).val);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_monomial_mul(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	SymRef y = symbol_declare_homog(ctx, "y", 0);
	SymRef p = symbol_declare_homog(ctx, "p", F_POSITIVE | F_NONZERO);

	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef x_two = factor_intern(ctx, x, rat_from_int(2), NULL);
	FacRef y_one = factor_intern(ctx, y, rat_from_int(1), NULL);
	FacRef p_half = factor_intern(ctx, p, rat_new(1, 2), NULL);
	FacRef p_minus_half = factor_intern(ctx, p, rat_new(-1, 2), NULL);

	MonoRef one = mono_intern(ctx, NULL, 0);
	MonoRef just_x = mono_intern(ctx, (FacRef[]){x_one}, 1);
	MonoRef x_y = mono_intern(ctx, (FacRef[]){x_one, y_one}, 2);
	MonoRef x2_y = mono_intern(ctx, (FacRef[]){x_two, y_one}, 2);
	MonoRef sqrt_p = mono_intern(ctx, (FacRef[]){p_half}, 1);
	MonoRef inverse_sqrt_p = mono_intern(ctx, (FacRef[]){p_minus_half}, 1);

	CHECK(mono_mul(ctx, sqrt_p, inverse_sqrt_p).val == one.val);
	CHECK(mono_mul(ctx, just_x, x_y).val == x2_y.val);
	CHECK(mono_mul(ctx, x_y, just_x).val == x2_y.val);
	CHECK(mono_mul(ctx, one, x_y).val == x_y.val);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_monomial_dummies(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef dE = {0};
	SymRef dK = {0};
	CHECK(declare_canonical_pair(ctx, &dE, &dK));

	Index a = {IDX_FREE, 'a'};
	Index b = {IDX_FREE, 'b'};
	Index i = {IDX_FREE, 'i'};
	Index j = {IDX_FREE, 'j'};
	Index d0 = {IDX_DUMMY, 0};
	Index d1 = {IDX_DUMMY, 1};

	// dE^a_i dK^i_b: `i` becomes dummy 0.
	MonoRef by_name = mono_intern(
		ctx,
		(FacRef[]){with_indices(ctx, dE, a, i), with_indices(ctx, dK, i, b)},
		2);
	MonoRef by_number = mono_intern(
		ctx,
		(FacRef[]){with_indices(ctx, dE, a, d0), with_indices(ctx, dK, d0, b)},
		2);
	CHECK(by_name.val != 0 && by_name.val == by_number.val);

	// The name of a dummy does not matter: dE^a_j dK^j_b is the same.
	MonoRef renamed = mono_intern(
		ctx,
		(FacRef[]){with_indices(ctx, dE, a, j), with_indices(ctx, dK, j, b)},
		2);
	CHECK(renamed.val == by_name.val);

	// dE^a_i dK^i_a: two dummies.
	MonoRef trace = mono_intern(
		ctx,
		(FacRef[]){with_indices(ctx, dE, a, i), with_indices(ctx, dK, i, a)},
		2);
	MonoRef trace_numbered =
		mono_intern(ctx,
					(FacRef[]){with_indices(ctx, dE, d0, d1),
							   with_indices(ctx, dK, d1, d0)},
					2);
	CHECK(trace.val != 0 && trace.val == trace_numbered.val);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_monomial_bad_dummies(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef dE = {0};
	SymRef dK = {0};
	CHECK(declare_canonical_pair(ctx, &dE, &dK));

	Index a = {IDX_FREE, 'a'};
	Index b = {IDX_FREE, 'b'};
	Index c = {IDX_FREE, 'c'};
	Index i = {IDX_FREE, 'i'};
	Index d0 = {IDX_DUMMY, 0};

	// `i` down twice: dE^a_i dE^b_i.
	CHECK(mono_intern(ctx,
					  (FacRef[]){with_indices(ctx, dE, a, i),
								 with_indices(ctx, dE, b, i)},
					  2)
			  .val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	// `i` three times: dE^a_i dK^i_b dK^i_c.
	CHECK(mono_intern(ctx,
					  (FacRef[]){with_indices(ctx, dE, a, i),
								 with_indices(ctx, dK, i, b),
								 with_indices(ctx, dK, i, c)},
					  3)
			  .val == 0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	// A dummy with no partner.
	CHECK(mono_intern(ctx, (FacRef[]){with_indices(ctx, dE, a, d0)}, 1).val ==
		  0);
	CHECK(!ctx_ok(ctx));
	ctx_clear_err(ctx);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_monomial_mul_dummies(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef dE = {0};
	SymRef dK = {0};
	CHECK(declare_canonical_pair(ctx, &dE, &dK));

	Index a = {IDX_FREE, 'a'};
	Index b = {IDX_FREE, 'b'};
	Index c = {IDX_FREE, 'c'};
	Index d = {IDX_FREE, 'd'};
	Index i = {IDX_FREE, 'i'};
	Index j = {IDX_FREE, 'j'};

	FacRef dE_ai = with_indices(ctx, dE, a, i);
	FacRef dK_ib = with_indices(ctx, dK, i, b);
	FacRef dE_ci = with_indices(ctx, dE, c, i);
	FacRef dK_id = with_indices(ctx, dK, i, d);
	FacRef dE_cj = with_indices(ctx, dE, c, j);
	FacRef dK_jd = with_indices(ctx, dK, j, d);
	FacRef dE_bj = with_indices(ctx, dE, b, j);
	FacRef dK_jc = with_indices(ctx, dK, j, c);

	MonoRef left = mono_intern(ctx, (FacRef[]){dE_ai, dK_ib}, 2);
	MonoRef same_dummy = mono_intern(ctx, (FacRef[]){dE_ci, dK_id}, 2);
	MonoRef contracting = mono_intern(ctx, (FacRef[]){dE_bj, dK_jc}, 2);

	MonoRef apart = mono_intern(ctx, (FacRef[]){dE_ai, dK_ib, dE_cj, dK_jd}, 4);
	CHECK(apart.val != 0);
	CHECK(mono_mul(ctx, left, same_dummy).val == apart.val);

	MonoRef chain = mono_intern(ctx, (FacRef[]){dE_ai, dK_ib, dE_bj, dK_jc}, 4);
	CHECK(chain.val != 0);
	CHECK(mono_mul(ctx, left, contracting).val == chain.val);

	CHECK(mono_mul(ctx, left, left).val == 0);
	CHECK(!ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_monomial_content_order(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef dE = {0};
	SymRef dK = {0};
	CHECK(declare_canonical_pair(ctx, &dE, &dK));

	// dK^j_a is interned first, so it has the smaller handle.
	FacRef dK_ja = with_free_indices(ctx, dK, "ja");
	FacRef dK_ib = with_free_indices(ctx, dK, "ib");
	CHECK(dK_ja.val < dK_ib.val);

	// Same head dK: the first index decides, therefore i comes before j.
	MonoRef mono = mono_intern(ctx, (FacRef[]){dK_ja, dK_ib}, 2);
	Monomial stored = vec_at(ctx->monomials, mono.val);

	CHECK(stored.nbr_fac == 2);
	CHECK(stored.fac[0].val == dK_ib.val);
	CHECK(stored.fac[1].val == dK_ja.val);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_monomial_derivatives(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	bundle_set_derivative(ctx, spatial);

	SymRef T = symbol_declare_tensor(ctx, "T", SYM_FIELD, "^a^b", "", 0);
	SymRef Y = symbol_declare_tensor(ctx, "Y", SYM_FIELD, "_a", "", 0);
	SymRef x = symbol_declare_tensor(ctx, "x", SYM_FIELD, "", "", 0);
	SymRef V = symbol_declare_tensor(ctx, "V", SYM_FIELD, "^a", "", 0);
	CHECK(ctx_ok(ctx));

	Index a = {IDX_FREE, 'a'};
	Index b = {IDX_FREE, 'b'};
	Index d0 = {IDX_DUMMY, 0};

	// ∂_a T^{ab}: the derivative index pairs with the up slot. Written with a
	// free `a` twice or with a dummy, it is the same monomial.
	FacRef dT_aab = factor_derive(ctx, with_free_indices(ctx, T, "ab"), a);
	FacRef dT_00b = factor_derive(
		ctx, factor_intern(ctx, T, rat_from_int(1), (Index[]){d0, b}), d0);

	MonoRef by_names = mono_intern(ctx, &dT_aab, 1);
	MonoRef by_dummy = mono_intern(ctx, &dT_00b, 1);
	CHECK(by_names.val != 0 && by_names.val == by_dummy.val);

	// Its only free index is `b`, up.
	FacRef V_b = with_free_indices(ctx, V, "b");
	CHECK(mono_free_indices_equal(ctx, by_names, mono_intern(ctx, &V_b, 1)));

	// ∂_a Y_a: both down, refused.
	FacRef dY_aa = factor_derive(ctx, with_free_indices(ctx, Y, "a"), a);
	CHECK(mono_intern(ctx, &dY_aa, 1).val == 0);
	ctx_clear_err(ctx);

	// x·x merges into x², but x·∂_a x and ∂_a x·∂_b x stay two factors.
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef dx_a = factor_derive(ctx, x_one, a);
	FacRef dx_b = factor_derive(ctx, x_one, b);

	CHECK(vec_at(ctx->monomials,
				 mono_intern(ctx, (FacRef[]){x_one, x_one}, 2).val)
			  .nbr_fac == 1);
	CHECK(
		vec_at(ctx->monomials, mono_intern(ctx, (FacRef[]){x_one, dx_a}, 2).val)
			.nbr_fac == 2);
	CHECK(
		vec_at(ctx->monomials, mono_intern(ctx, (FacRef[]){dx_a, dx_b}, 2).val)
			.nbr_fac == 2);

	// x · ∂_a x · x² keeps x and x² together: x³ · ∂_a x.
	FacRef x_two = factor_intern(ctx, x, rat_from_int(2), NULL);
	FacRef x_three = factor_intern(ctx, x, rat_from_int(3), NULL);
	CHECK(mono_intern(ctx, (FacRef[]){x_one, dx_a, x_two}, 3).val ==
		  mono_intern(ctx, (FacRef[]){x_three, dx_a}, 2).val);

	// The dummies of a product's right side are shifted, derivative ones too.
	FacRef dT_ccd = factor_derive(ctx, with_free_indices(ctx, T, "cd"),
								  (Index){IDX_FREE, 'c'});
	MonoRef first = mono_intern(ctx, &dT_aab, 1);
	MonoRef second = mono_intern(ctx, &dT_ccd, 1);
	CHECK(mono_mul(ctx, first, second).val ==
		  mono_intern(ctx, (FacRef[]){dT_aab, dT_ccd}, 2).val);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

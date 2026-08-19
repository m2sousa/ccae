#include <stddef.h>

#include <ccae/core/rational.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/canonicalise.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

#include "t_kernel_utils.h"
#include "t_utils.h"

static size_t test_canonicalise_one_factor(void);
static size_t test_canonicalise_dummies(void);
static size_t test_canonicalise_zero(void);
static size_t test_canonicalise_identical_factors(void);
static size_t test_canonicalise_relabellings(void);

size_t test_canonicalise(void) {
	size_t fails = 0;
	fails += test_canonicalise_one_factor();
	fails += test_canonicalise_dummies();
	fails += test_canonicalise_zero();
	fails += test_canonicalise_identical_factors();
	fails += test_canonicalise_relabellings();

	return fails;
}

static MonoRef one_factor(Context* ctx, SymRef head, const char* names) {
	FacRef fac = with_free_indices(ctx, head, names);

	return mono_intern(ctx, &fac, 1);
}

static MonoRef two_factors(Context* ctx, SymRef first, const char* first_names,
						   SymRef second, const char* second_names) {
	FacRef facs[] = {with_free_indices(ctx, first, first_names),
					 with_free_indices(ctx, second, second_names)};

	return mono_intern(ctx, facs, 2);
}

static bool canonicalises_to(Context* ctx, MonoRef m, MonoRef expected,
							 int expected_sign) {
	int sign = 0;
	MonoRef canonical = canon_mono(ctx, m, &sign);

	return canonical.val == expected.val && sign == expected_sign;
}

static size_t test_canonicalise_one_factor(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	SymRef g =
		symbol_declare_tensor(ctx, "g", SYM_FIELD, "_a_b", "sym(0,1)", 0);
	SymRef A =
		symbol_declare_tensor(ctx, "A", SYM_FIELD, "_a_b", "antisym(0,1)", 0);
	SymRef eps = symbol_declare_tensor(ctx, "eps", SYM_CONST, "_a_b_c",
									   "antisym(0,1,2)", 0);
	SymRef x = symbol_declare_homog(ctx, "x", 0);
	CHECK(spatial.val != 0 && ctx_ok(ctx));

	MonoRef g_ab = one_factor(ctx, g, "ab");
	CHECK(canonicalises_to(ctx, one_factor(ctx, g, "ba"), g_ab, 1));
	CHECK(canonicalises_to(ctx, g_ab, g_ab, 1));

	MonoRef A_ab = one_factor(ctx, A, "ab");
	CHECK(canonicalises_to(ctx, one_factor(ctx, A, "ba"), A_ab, -1));
	CHECK(canonicalises_to(ctx, A_ab, A_ab, 1));

	// A cycle of three slots is even, one swap is odd.
	MonoRef eps_abc = one_factor(ctx, eps, "abc");
	CHECK(canonicalises_to(ctx, one_factor(ctx, eps, "cab"), eps_abc, 1));
	CHECK(canonicalises_to(ctx, one_factor(ctx, eps, "bac"), eps_abc, -1));

	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	MonoRef x_mono = mono_intern(ctx, &x_one, 1);
	CHECK(canonicalises_to(ctx, x_mono, x_mono, 1));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_canonicalise_dummies(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	SymRef T =
		symbol_declare_tensor(ctx, "T", SYM_FIELD, "_a_b", "sym(0,1)", 0);
	SymRef S = symbol_declare_tensor(ctx, "S", SYM_FIELD, "^a^b", "", 0);
	CHECK(spatial.val != 0 && ctx_ok(ctx));

	// T is symmetric, so T_{ab} S^{ba} = T_{ab} S^{ab}.
	MonoRef crossed = two_factors(ctx, T, "ab", S, "ba");
	MonoRef straight = two_factors(ctx, T, "ab", S, "ab");
	MonoRef relabelled = two_factors(ctx, T, "cd", S, "dc");

	int sign = 0;
	MonoRef canonical = canon_mono(ctx, straight, &sign);

	CHECK(canonicalises_to(ctx, crossed, canonical, 1));
	CHECK(canonicalises_to(ctx, relabelled, canonical, 1));
	CHECK(canonicalises_to(ctx, canonical, canonical, 1));
	CHECK(sign == 1);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static int canonical_sign(Context* ctx, MonoRef m) {
	int sign = 2;
	MonoRef canonical = canon_mono(ctx, m, &sign);

	return canonical.val != 0 ? sign : 2;
}

static size_t test_canonicalise_zero(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	SymRef eps = symbol_declare_tensor(ctx, "eps", SYM_CONST, "_a_b_c",
									   "antisym(0,1,2)", 0);
	SymRef T =
		symbol_declare_tensor(ctx, "T", SYM_FIELD, "^a^b", "sym(0,1)", 0);
	SymRef A =
		symbol_declare_tensor(ctx, "A", SYM_FIELD, "^a^b", "antisym(0,1)", 0);
	SymRef S = symbol_declare_tensor(ctx, "S", SYM_FIELD, "^a^b", "", 0);
	CHECK(spatial.val != 0 && ctx_ok(ctx));

	// Antisymmetric slots b, c contracted with symmetric ones: zero.
	CHECK(canonical_sign(ctx, two_factors(ctx, eps, "abc", T, "bc")) == 0);

	// Antisymmetric with antisymmetric, or with no symmetry: not zero.
	CHECK(canonical_sign(ctx, two_factors(ctx, eps, "abc", A, "bc")) == 1);
	CHECK(canonical_sign(ctx, two_factors(ctx, eps, "abc", S, "bc")) == 1);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_canonicalise_identical_factors(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	SymRef A =
		symbol_declare_tensor(ctx, "A", SYM_FIELD, "_a_b", "antisym(0,1)", 0);
	SymRef x = symbol_declare_tensor(ctx, "x", SYM_FIELD, "^a", "", 0);
	SymRef y = symbol_declare_tensor(ctx, "y", SYM_FIELD, "^a", "", 0);
	CHECK(spatial.val != 0 && ctx_ok(ctx));

	// Swapping the two x is free, swapping A's slots costs -1: zero.
	FacRef A_x_x[] = {with_free_indices(ctx, A, "ab"),
					  with_free_indices(ctx, x, "a"),
					  with_free_indices(ctx, x, "b")};
	CHECK(canonical_sign(ctx, mono_intern(ctx, A_x_x, 3)) == 0);

	// x and y are not identical, so they cannot trade places.
	FacRef A_x_y[] = {with_free_indices(ctx, A, "ab"),
					  with_free_indices(ctx, x, "a"),
					  with_free_indices(ctx, y, "b")};
	CHECK(canonical_sign(ctx, mono_intern(ctx, A_x_y, 3)) == 1);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

// The monomial dE dE dK dK, given the index names of each factor:
// "ai", "bj", "ja", "ib" gives dE^a_i dE^b_j dK^j_a dK^i_b.
static MonoRef four_factors(Context* ctx, SymRef dE, SymRef dK,
							const char* first, const char* second,
							const char* third, const char* fourth) {
	FacRef facs[] = {
		with_free_indices(ctx, dE, first), with_free_indices(ctx, dE, second),
		with_free_indices(ctx, dK, third), with_free_indices(ctx, dK, fourth)};

	return mono_intern(ctx, facs, 4);
}

static size_t test_canonicalise_relabellings(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef dE = {0};
	SymRef dK = {0};
	CHECK(declare_canonical_pair(ctx, &dE, &dK));

	// dE^a_i dE^b_j dK^j_a dK^i_b: one loop through the four factors.
	MonoRef loop = four_factors(ctx, dE, dK, "ai", "bj", "ja", "ib");
	int sign = 0;
	MonoRef canonical = canon_mono(ctx, loop, &sign);
	CHECK(sign == 1);

	// The same loop, written with other names or in another order.
	CHECK(canonicalises_to(
		ctx, four_factors(ctx, dE, dK, "bj", "ai", "ib", "ja"), canonical, 1));
	CHECK(canonicalises_to(
		ctx, four_factors(ctx, dE, dK, "ck", "dj", "jc", "kd"), canonical, 1));
	CHECK(canonicalises_to(
		ctx, four_factors(ctx, dE, dK, "aj", "bi", "ia", "jb"), canonical, 1));

	CHECK(canonicalises_to(ctx, canonical, canonical, 1));

	// dE^a_i dE^b_j dK^i_a dK^j_b: two separate loops, a different monomial.
	MonoRef two_loops = four_factors(ctx, dE, dK, "ai", "bj", "ia", "jb");
	CHECK(canon_mono(ctx, two_loops, &sign).val != canonical.val);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

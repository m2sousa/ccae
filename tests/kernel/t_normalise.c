#include <stddef.h>

#include <ccae/core/rational.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/expr.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/normalise.h>
#include <ccae/kernel/symbol.h>

#include "t_kernel_utils.h"
#include "t_utils.h"

static size_t test_normalise_absorb(void);
static size_t test_normalise_trace(void);
static size_t test_normalise_free_delta(void);
static size_t test_normalise_epsilon_symmetric(void);
static size_t test_normalise_epsilon_pair(void);
static size_t test_normalise_inverse(void);

size_t test_normalise(void) {
	size_t fails = 0;
	fails += test_normalise_absorb();
	fails += test_normalise_trace();
	fails += test_normalise_free_delta();
	fails += test_normalise_epsilon_symmetric();
	fails += test_normalise_epsilon_pair();
	fails += test_normalise_inverse();

	return fails;
}

static Context* create_ctx_with_delta(SymRef* delta) {
	Context* ctx = ctx_create();
	if (!ctx)
		return NULL;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh", 3);
	*delta = symbol_declare_tensor(ctx, "delta", SYM_CONST, "^a_b", "", 0);
	bundle_set_kronecker(ctx, spatial, *delta);

	if (!ctx_ok(ctx)) {
		ctx_destroy(ctx);
		return NULL;
	}

	return ctx;
}

static bool normalises_to(Context* ctx, MonoRef mono, i64 coef,
						  MonoRef expected) {
	Term out[1];
	size_t nbr_out = normalise_term(out, 1, ctx, (Term){rat_from_int(1), mono});

	return nbr_out == 1 && out[0].mono.val == expected.val &&
		   rat_cmp(out[0].coef, rat_from_int(coef)) == 0;
}

static size_t test_normalise_absorb(void) {
	size_t fails = 0;

	SymRef delta = {0};
	Context* ctx = create_ctx_with_delta(&delta);
	if (!ctx)
		return 1;

	SymRef X = symbol_declare_tensor(ctx, "X", SYM_FIELD, "^a", "", 0);
	SymRef Y = symbol_declare_tensor(ctx, "Y", SYM_FIELD, "_a", "", 0);

	FacRef X_a = with_free_indices(ctx, X, "a");
	MonoRef X_up_a = mono_intern(ctx, &X_a, 1);

	// δ^a_b δ^b_c X^c = X^a.
	FacRef chain[] = {with_free_indices(ctx, delta, "ab"),
					  with_free_indices(ctx, delta, "bc"),
					  with_free_indices(ctx, X, "c")};
	CHECK(normalises_to(ctx, mono_intern(ctx, chain, 3), 1, X_up_a));

	// δ^a_b Y_a = Y_b.
	FacRef Y_b = with_free_indices(ctx, Y, "b");
	FacRef lowered[] = {with_free_indices(ctx, delta, "ab"),
						with_free_indices(ctx, Y, "a")};
	CHECK(normalises_to(ctx, mono_intern(ctx, lowered, 2), 1,
						mono_intern(ctx, &Y_b, 1)));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_normalise_trace(void) {
	size_t fails = 0;

	SymRef delta = {0};
	Context* ctx = create_ctx_with_delta(&delta);
	if (!ctx)
		return 1;

	MonoRef one = mono_intern(ctx, NULL, 0);

	// δ^a_a = 3, the dimension of the bundle.
	FacRef trace = with_free_indices(ctx, delta, "aa");
	CHECK(normalises_to(ctx, mono_intern(ctx, &trace, 1), 3, one));

	// δ^a_b δ^b_a = δ^a_a = 3.
	FacRef loop[] = {with_free_indices(ctx, delta, "ab"),
					 with_free_indices(ctx, delta, "ba")};
	CHECK(normalises_to(ctx, mono_intern(ctx, loop, 2), 3, one));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_normalise_free_delta(void) {
	size_t fails = 0;

	SymRef delta = {0};
	Context* ctx = create_ctx_with_delta(&delta);
	if (!ctx)
		return 1;

	// Both indices free: nothing to absorb.
	FacRef delta_ab = with_free_indices(ctx, delta, "ab");
	MonoRef free_delta = mono_intern(ctx, &delta_ab, 1);
	CHECK(normalises_to(ctx, free_delta, 1, free_delta));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_normalise_epsilon_symmetric(void) {
	size_t fails = 0;

	SymRef delta = {0};
	Context* ctx = create_ctx_with_delta(&delta);
	if (!ctx)
		return 1;

	SymRef eps = symbol_declare_tensor(ctx, "eps", SYM_CONST, "_a_b_c",
									   "antisym(0,1,2)", 0);
	SymRef S =
		symbol_declare_tensor(ctx, "S", SYM_FIELD, "^a^b", "sym(0,1)", 0);
	SymRef A = symbol_declare_tensor(ctx, "A", SYM_FIELD, "^a^b", "", 0);
	CHECK(ctx_ok(ctx));

	// ε_{abc} S^{ab} = 0: swapping a and b gives back the same term, negated.
	FacRef eps_S[] = {with_free_indices(ctx, eps, "abc"),
					  with_free_indices(ctx, S, "ab")};
	Term eps_S_term = {rat_from_int(1), mono_intern(ctx, eps_S, 2)};

	Term out[1];
	CHECK(normalise_term(out, 1, ctx, eps_S_term) == 0);

	ExprRef empty = expr_intern(ctx, NULL, 0);
	CHECK(expr_intern(ctx, &eps_S_term, 1).val == empty.val);

	// A has no symmetry: ε_{abc} A^{ab} stays.
	FacRef eps_A[] = {with_free_indices(ctx, eps, "abc"),
					  with_free_indices(ctx, A, "ab")};
	Term eps_A_term = {rat_from_int(1), mono_intern(ctx, eps_A, 2)};
	CHECK(normalise_term(out, 1, ctx, eps_A_term) == 1);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static bool epsilon_pair_is(Context* ctx, const char* down_names,
							const char* up_names, Term* expected,
							size_t nbr_expected) {
	Bundle bundle =
		vec_at(ctx->bundles, bundle_find_by_index_name(ctx, 'a').val);

	FacRef pair[] = {with_free_indices(ctx, bundle.eps_down, down_names),
					 with_free_indices(ctx, bundle.eps_up, up_names)};
	Term term = {rat_from_int(1), mono_intern(ctx, pair, 2)};

	ExprRef result = expr_intern(ctx, &term, 1);

	return result.val != 0 &&
		   result.val == expr_intern(ctx, expected, nbr_expected).val;
}

static size_t test_normalise_epsilon_pair(void) {
	size_t fails = 0;

	SymRef delta = {0};
	Context* ctx = create_ctx_with_delta(&delta);
	if (!ctx)
		return 1;

	BundleRef spatial = bundle_find_by_index_name(ctx, 'a');
	SymRef eps_down = symbol_declare_tensor(ctx, "epsDown", SYM_CONST, "_a_b_c",
											"antisym(0,1,2)", 0);
	SymRef eps_up = symbol_declare_tensor(ctx, "epsUp", SYM_CONST, "^a^b^c",
										  "antisym(0,1,2)", 0);
	bundle_set_epsilon(ctx, spatial, eps_down);
	bundle_set_epsilon(ctx, spatial, eps_up);
	CHECK(ctx_ok(ctx));

	MonoRef one = mono_intern(ctx, NULL, 0);
	FacRef delta_da = with_free_indices(ctx, delta, "da");
	FacRef delta_eb = with_free_indices(ctx, delta, "eb");
	FacRef delta_ea = with_free_indices(ctx, delta, "ea");
	FacRef delta_db = with_free_indices(ctx, delta, "db");

	// ε_{abc} ε^{abc} = 6.
	CHECK(epsilon_pair_is(ctx, "abc", "abc", (Term[]){{rat_from_int(6), one}},
						  1));

	// ε_{abc} ε^{dbc} = 2 δ^d_a.
	Term twice_delta = {rat_from_int(2), mono_intern(ctx, &delta_da, 1)};
	CHECK(epsilon_pair_is(ctx, "abc", "dbc", &twice_delta, 1));

	// ε_{abc} ε^{dec} = δ^d_a δ^e_b − δ^e_a δ^d_b.
	Term two_deltas[] = {
		{rat_from_int(1), mono_intern(ctx, (FacRef[]){delta_da, delta_eb}, 2)},
		{rat_from_int(-1), mono_intern(ctx, (FacRef[]){delta_ea, delta_db}, 2)},
	};
	CHECK(epsilon_pair_is(ctx, "abc", "dec", two_deltas, 2));

	// No dummy shared: ε_{abc} ε^{def} stays one term.
	FacRef no_dummy[] = {with_free_indices(ctx, eps_down, "abc"),
						 with_free_indices(ctx, eps_up, "def")};
	Term no_dummy_term = {rat_from_int(1), mono_intern(ctx, no_dummy, 2)};

	Term out[1];
	CHECK(normalise_term(out, 1, ctx, no_dummy_term) == 1);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_normalise_inverse(void) {
	size_t fails = 0;

	SymRef delta = {0};
	Context* ctx = create_ctx_with_delta(&delta);
	if (!ctx)
		return 1;

	BundleRef internal = bundle_declare(ctx, "internal", "ijklmn", 3);
	SymRef delta_internal =
		symbol_declare_tensor(ctx, "deltaInternal", SYM_CONST, "^i_j", "", 0);
	bundle_set_kronecker(ctx, internal, delta_internal);

	SymRef triad =
		symbol_declare_tensor(ctx, "triad", SYM_CONST, "^a_i", "", 0);
	SymRef cotriad =
		symbol_declare_tensor(ctx, "cotriad", SYM_CONST, "_a^i", "", 0);
	symbol_set_inverse(ctx, triad, cotriad);

	SymRef eps = symbol_declare_tensor(ctx, "eps", SYM_CONST, "_i_j_k",
									   "antisym(0,1,2)", 0);
	CHECK(ctx_ok(ctx));

	MonoRef one = mono_intern(ctx, NULL, 0);
	FacRef delta_ab = with_free_indices(ctx, delta, "ab");
	FacRef delta_ji = with_free_indices(ctx, delta_internal, "ji");

	// δ^a_i δ_b^i = δ^a_b.
	FacRef second_slots[] = {with_free_indices(ctx, triad, "ai"),
							 with_free_indices(ctx, cotriad, "bi")};
	CHECK(normalises_to(ctx, mono_intern(ctx, second_slots, 2), 1,
						mono_intern(ctx, &delta_ab, 1)));

	// δ^a_i δ_a^j = δ^j_i.
	FacRef first_slots[] = {with_free_indices(ctx, triad, "ai"),
							with_free_indices(ctx, cotriad, "aj")};
	CHECK(normalises_to(ctx, mono_intern(ctx, first_slots, 2), 1,
						mono_intern(ctx, &delta_ji, 1)));

	// δ^a_i δ_a^i = δ^i_i = 3.
	FacRef both_slots[] = {with_free_indices(ctx, triad, "ai"),
						   with_free_indices(ctx, cotriad, "ai")};
	CHECK(normalises_to(ctx, mono_intern(ctx, both_slots, 2), 3, one));

	// ε_{ijk} δ^j_a δ^k_b δ^a_l δ^b_m = ε_{ilm}.
	FacRef chain[] = {
		with_free_indices(ctx, eps, "ijk"),
		with_free_indices(ctx, cotriad, "aj"),
		with_free_indices(ctx, cotriad, "bk"),
		with_free_indices(ctx, triad, "al"),
		with_free_indices(ctx, triad, "bm"),
	};
	FacRef eps_ilm = with_free_indices(ctx, eps, "ilm");
	CHECK(normalises_to(ctx, mono_intern(ctx, chain, 5), 1,
						mono_intern(ctx, &eps_ilm, 1)));

	// A lone triad stays.
	FacRef triad_ai = with_free_indices(ctx, triad, "ai");
	MonoRef lone = mono_intern(ctx, &triad_ai, 1);
	CHECK(normalises_to(ctx, lone, 1, lone));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

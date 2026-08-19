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

size_t test_normalise(void) {
	size_t fails = 0;
	fails += test_normalise_absorb();
	fails += test_normalise_trace();
	fails += test_normalise_free_delta();

	return fails;
}

static Context* create_ctx_with_delta(SymRef* delta) {
	Context* ctx = ctx_create();
	if (!ctx)
		return NULL;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
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
	Term term = normalise_term(ctx, (Term){rat_from_int(1), mono});

	return term.mono.val == expected.val &&
		   rat_cmp(term.coef, rat_from_int(coef)) == 0;
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

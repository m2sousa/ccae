#include <stddef.h>

#include <ccae/config.h>
#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

#include "t_utils.h"

static size_t test_monomial_order(void);
static size_t test_monomial_one(void);
static size_t test_monomial_too_many(void);
static size_t test_monomial_merge(void);
static size_t test_monomial_tensors_stay(void);

size_t test_monomial(void) {
	size_t fails = 0;
	fails += test_monomial_order();
	fails += test_monomial_one();
	fails += test_monomial_too_many();
	fails += test_monomial_merge();
	fails += test_monomial_tensors_stay();

	return fails;
}

static size_t test_monomial_order(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef c = symbol_declare_homog(ctx, "c", 0);
	SymRef p = symbol_declare_homog(ctx, "p", F_POSITIVE);

	FacRef c_one = factor_intern(ctx, c, rat_from_int(1));
	FacRef sqrt_p = factor_intern(ctx, p, rat_new(1, 2));

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
	FacRef c_one = factor_intern(ctx, c, rat_from_int(1));

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

	BundleRef spatial = bundle_declare(ctx, "spatial", "abc");
	SymRef dE = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "_a", 0);
	FacRef dE_one = factor_intern(ctx, dE, rat_from_int(1));
	CHECK(spatial.val != 0 && dE_one.val != 0);

	FacRef facs[CCAE_MAX_FACTORS + 1];
	for (size_t i = 0; i < CCAE_MAX_FACTORS + 1; ++i)
		facs[i] = dE_one;

	CHECK(mono_intern(ctx, facs, CCAE_MAX_FACTORS).val != 0);

	CHECK(mono_intern(ctx, facs, CCAE_MAX_FACTORS + 1).val == 0);
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

	FacRef x_one = factor_intern(ctx, x, rat_from_int(1));
	FacRef x_two = factor_intern(ctx, x, rat_from_int(2));
	FacRef y_one = factor_intern(ctx, y, rat_from_int(1));
	FacRef p_half = factor_intern(ctx, p, rat_new(1, 2));
	FacRef p_minus_half = factor_intern(ctx, p, rat_new(-1, 2));

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

	BundleRef spatial = bundle_declare(ctx, "spatial", "abc");
	SymRef dE = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "_a", 0);
	FacRef dE_one = factor_intern(ctx, dE, rat_from_int(1));
	CHECK(spatial.val != 0 && dE_one.val != 0);

	MonoRef dE_dE = mono_intern(ctx, (FacRef[]){dE_one, dE_one}, 2);
	CHECK(vec_at(ctx->monomials, dE_dE.val).nbr_fac == 2);

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1));
	FacRef x_two = factor_intern(ctx, x, rat_from_int(2));

	MonoRef x_dE_x = mono_intern(ctx, (FacRef[]){x_one, dE_one, x_one}, 3);
	CHECK(x_dE_x.val == mono_intern(ctx, (FacRef[]){x_two, dE_one}, 2).val);

	ctx_destroy(ctx);

	return fails;
}

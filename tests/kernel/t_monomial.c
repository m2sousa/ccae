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

size_t test_monomial(void) {
	size_t fails = 0;
	fails += test_monomial_order();
	fails += test_monomial_one();
	fails += test_monomial_too_many();

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

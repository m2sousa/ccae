#include <stddef.h>

#include <ccae/config.h>
#include <ccae/core/error.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/monomial.h>

static bool comes_before(const Context* ctx, FacRef a, FacRef b) {
	u32 head_a = vec_at(ctx->factors, a.val).head.val;
	u32 head_b = vec_at(ctx->factors, b.val).head.val;

	if (head_a != head_b)
		return head_a < head_b;

	return a.val < b.val;
}

static void sort_factors(const Context* ctx, Monomial* mono) {
	// Insertion sort.
	for (u64 i = 1; i < mono->nbr_fac; ++i) {
		FacRef current = mono->fac[i];
		u64 j = i;

		while (j > 0 && comes_before(ctx, current, mono->fac[j - 1])) {
			mono->fac[j] = mono->fac[j - 1];
			--j;
		}

		mono->fac[j] = current;
	}
}

static bool same_factors(const Monomial* a, const Monomial* b) {
	if (a->nbr_fac != b->nbr_fac)
		return false;

	for (u64 i = 0; i < a->nbr_fac; ++i) {
		if (a->fac[i].val != b->fac[i].val)
			return false;
	}

	return true;
}

MonoRef mono_intern(Context* ctx, const FacRef* facs, size_t nbr_fac) {
	MonoRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (nbr_fac > CCAE_MAX_FACTORS) {
		err_set(&ctx->err, CCAE_INVALID_ARG, "monomial: more than %zu factors.",
				CCAE_MAX_FACTORS);
		return null_ref;
	}

	Monomial mono = {.nbr_fac = (u64)nbr_fac};

	for (size_t i = 0; i < nbr_fac; ++i) {
		if (facs[i].val == 0 || facs[i].val >= ctx->factors.length)
			die("CCAE (mono_intern): factor handle out of range.");

		mono.fac[i] = facs[i];
	}

	sort_factors(ctx, &mono);

	// TODO: This is a linear search, setup an hashmap to get O(1) accesses.
	for (size_t i = 1; i < ctx->monomials.length; ++i) {
		if (same_factors(&vec_at(ctx->monomials, i), &mono)) {
			MonoRef found = {.val = (u32)i};
			return found;
		}
	}

	vec_append(ctx->perm, ctx->monomials, mono);

	MonoRef mr = {.val = (u32)(ctx->monomials.length - 1)};

	return mr;
}

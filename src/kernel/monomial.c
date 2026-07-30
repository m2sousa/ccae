#include <stddef.h>

#include <ccae/config.h>
#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

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

// Merge scalar factor (ex: x*x => x^2) by adding their power,
// do nothing otherwise.
static bool merge_powers(Context* ctx, Monomial* mono) {
	size_t nbr_kept = 0;
	size_t i = 0;

	while (i < mono->nbr_fac) {
		Factor first = vec_at(ctx->factors, mono->fac[i].val);
		u64 nbr_slot = vec_at(ctx->symbols, first.head.val).nbr_slot;

		if (nbr_slot > 0) {
			mono->fac[nbr_kept++] = mono->fac[i++];
			continue;
		}

		Rational power = first.power;

		for (++i; i < mono->nbr_fac; ++i) {
			Factor next = vec_at(ctx->factors, mono->fac[i].val);

			if (next.head.val != first.head.val)
				break;

			power = rat_add(power, next.power);
		}

		if (rat_is_zero(power))
			continue;

		FacRef merged = factor_intern(ctx, first.head, power);
		if (merged.val == 0)
			return false;

		mono->fac[nbr_kept++] = merged;
	}

	mono->nbr_fac = (u64)nbr_kept;

	return true;
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

	if (!merge_powers(ctx, &mono))
		return null_ref;

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

MonoRef mono_mul(Context* ctx, MonoRef a, MonoRef b) {
	MonoRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (a.val == 0 || a.val >= ctx->monomials.length || b.val == 0 ||
		b.val >= ctx->monomials.length)
		die("CCAE (mono_mul): monomial handle out of range.");

	Monomial mono_a = vec_at(ctx->monomials, a.val);
	Monomial mono_b = vec_at(ctx->monomials, b.val);

	if (mono_a.nbr_fac + mono_b.nbr_fac > CCAE_MAX_FACTORS) {
		err_set(&ctx->err, CCAE_INVALID_ARG, "monomial: more than %zu factors.",
				CCAE_MAX_FACTORS);
		return null_ref;
	}

	FacRef facs[CCAE_MAX_FACTORS];
	size_t nbr_fac = 0;

	for (u64 i = 0; i < mono_a.nbr_fac; ++i)
		facs[nbr_fac++] = mono_a.fac[i];

	for (u64 i = 0; i < mono_b.nbr_fac; ++i)
		facs[nbr_fac++] = mono_b.fac[i];

	return mono_intern(ctx, facs, nbr_fac);
}

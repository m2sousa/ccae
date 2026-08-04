#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/symbol.h>

static bool reject_factor(Context* ctx, const Symbol* sym,
						  const char* problem) {
	err_set(&ctx->err, CCAE_INVALID_ARG, "factor '%s': %s.",
			sp_get(ctx->sp, sym->name), problem);

	return false;
}

static bool check_power(Context* ctx, const Symbol* sym, Rational power) {
	if (rat_is_zero(power))
		return reject_factor(ctx, sym, "power 0");

	if (sym->nbr_slot > 0 && !rat_is_one(power))
		return reject_factor(ctx, sym,
							 "power other than 1 on a symbol with slots");

	if (power.den != 1 && !(sym->flags & F_POSITIVE))
		return reject_factor(ctx, sym, "non-integer power without F_POSITIVE");

	if (power.negative && !(sym->flags & F_NONZERO))
		return reject_factor(ctx, sym, "negative power without F_NONZERO");

	return true;
}

static bool check_indices(Context* ctx, const Symbol* sym,
						  const Index* indices) {
	if (sym->nbr_slot == 0)
		return true;

	if (!indices)
		return reject_factor(ctx, sym, "no indices on a symbol with slots");

	for (u8 k = 0; k < sym->nbr_slot; ++k) {
		// Dummies are not checked: dummies must be checked by the monomial to
		// ensure their repeated, they are uninsteresting at the factor level.
		if (indices[k].kind != IDX_FREE)
			continue;

		char name = (char)indices[k].id;
		BundleRef owner = bundle_find_by_index_name(ctx, name);

		if (owner.val != sym->slot_bundle[k].val) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"factor '%s': index '%c' is not part of its slot's bundle.",
					sp_get(ctx->sp, sym->name), name);
			return false;
		}
	}

	return true;
}

static bool have_same_indices(const Index* a, const Index* b, u8 nbr_slot) {
	for (u8 k = 0; k < nbr_slot; ++k) {
		if (!index_equal(a[k], b[k]))
			return false;
	}

	return true;
}

FacRef factor_intern(Context* ctx, SymRef head, Rational power,
					 const Index* indices) {
	FacRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (head.val == 0 || head.val >= ctx->symbols.length)
		die("CCAE (factor_intern): symbol handle out of range.");

	const Symbol* sym = &vec_at(ctx->symbols, head.val);

	if (!check_power(ctx, sym, power) || !check_indices(ctx, sym, indices))
		return null_ref;

	// TODO: This is a linear search, setup an hashmap to get O(1) accesses.
	for (size_t i = 1; i < ctx->factors.length; ++i) {
		Factor fac = vec_at(ctx->factors, i);

		// Rationals are normalised on creation, so equal factors must
		// have strictly equal fields.
		bool same_power = fac.power.negative == power.negative &&
						  fac.power.num == power.num &&
						  fac.power.den == power.den;

		if (fac.head.val == head.val && same_power &&
			have_same_indices(fac.indices, indices, sym->nbr_slot)) {
			FacRef found = {.val = (u32)i};
			return found;
		}
	}

	Factor fac = {.head = head, .power = power};

	for (u8 k = 0; k < sym->nbr_slot; ++k)
		fac.indices[k] = indices[k];
	vec_append(ctx->perm, ctx->factors, fac);

	FacRef fr = {.val = (u32)(ctx->factors.length - 1)};

	return fr;
}

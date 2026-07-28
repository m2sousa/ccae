#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/symbol.h>

static bool reject_power(Context* ctx, const Symbol* sym, const char* problem) {
	err_set(&ctx->err, CCAE_INVALID_ARG, "factor '%s': %s.",
			sp_get(ctx->sp, sym->name), problem);

	return false;
}

static bool check_power(Context* ctx, const Symbol* sym, Rational power) {
	if (rat_is_zero(power))
		return reject_power(ctx, sym, "power 0");

	if (sym->nslot > 0 && !rat_is_one(power))
		return reject_power(ctx, sym,
							"power other than 1 on a symbol with slots");

	if (power.den != 1 && !(sym->flags & F_POSITIVE))
		return reject_power(ctx, sym, "non-integer power without F_POSITIVE");

	if (power.negative && !(sym->flags & F_NONZERO))
		return reject_power(ctx, sym, "negative power without F_NONZERO");

	return true;
}

FacRef factor_intern(Context* ctx, SymRef head, Rational power) {
	FacRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (head.val == 0 || head.val >= ctx->symbols.length)
		die("CCAE (factor_intern): symbol handle out of range.");

	if (!check_power(ctx, &vec_at(ctx->symbols, head.val), power))
		return null_ref;

	// TODO: This is a linear search, setup an hashmap to get O(1) accesses.
	for (size_t i = 1; i < ctx->factors.length; ++i) {
		Factor fac = vec_at(ctx->factors, i);

		// Rationals are normalised on creation, so equal factors must
		// have strictly equal fields.
		bool same_power = fac.power.negative == power.negative &&
						  fac.power.num == power.num &&
						  fac.power.den == power.den;

		if (fac.head.val == head.val && same_power) {
			FacRef found = {.val = (u32)i};
			return found;
		}
	}

	Factor fac = {.head = head, .power = power};
	vec_append(ctx->perm, ctx->factors, fac);

	FacRef fr = {.val = (u32)(ctx->factors.length - 1)};

	return fr;
}

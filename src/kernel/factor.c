#include <stdlib.h>

#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/symbol.h>

static bool reject_factor(Context* ctx, const Symbol* symbol,
						  const char* problem) {
	err_set(&ctx->err, CCAE_INVALID_ARG, "factor '%s': %s.",
			sp_get(ctx->sp, symbol->name), problem);

	return false;
}

static bool check_exponent(Context* ctx, const Symbol* symbol,
						   Rational exponent) {
	if (rat_is_zero(exponent))
		return reject_factor(ctx, symbol, "exponent 0");

	if (symbol->nbr_slot > 0 && !rat_is_one(exponent))
		return reject_factor(ctx, symbol,
							 "exponent other than 1 on a symbol with slots");

	if (exponent.den != 1 && !(symbol->flags & F_POSITIVE))
		return reject_factor(ctx, symbol,
							 "non-integer exponent without F_POSITIVE");

	if (exponent.negative && !(symbol->flags & F_NONZERO))
		return reject_factor(ctx, symbol,
							 "negative exponent without F_NONZERO");

	return true;
}

static bool check_indices(Context* ctx, const Symbol* symbol,
						  const Index* indices) {
	if (symbol->nbr_slot == 0)
		return true;

	if (!indices)
		return reject_factor(ctx, symbol, "no indices on a symbol with slots");

	for (u8 k = 0; k < symbol->nbr_slot; ++k) {
		// Dummies are not checked: dummies must be checked by the monomial to
		// ensure their repeated, they are uninsteresting at the factor level.
		if (indices[k].kind != IDX_FREE)
			continue;

		char name = (char)indices[k].id;
		BundleRef owner = bundle_find_by_index_name(ctx, name);

		if (owner.val != symbol->slot_bundle[k].val) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"factor '%s': index '%c' is not part of its slot's bundle.",
					sp_get(ctx->sp, symbol->name), name);
			return false;
		}
	}

	return true;
}

static bool indices_equal(const Index* a, const Index* b, u8 nbr_slot) {
	for (u8 k = 0; k < nbr_slot; ++k) {
		if (!index_equal(a[k], b[k]))
			return false;
	}

	return true;
}

FacRef factor_intern(Context* ctx, SymRef head, Rational exponent,
					 const Index* indices) {
	FacRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (head.val == 0 || head.val >= ctx->symbols.length)
		die("CCAE (factor_intern): symbol handle out of range.");

	const Symbol* symbol = &vec_at(ctx->symbols, head.val);

	if (!check_exponent(ctx, symbol, exponent) ||
		!check_indices(ctx, symbol, indices))
		return null_ref;

	// TODO: This is a linear search, setup an hashmap to get O(1) accesses.
	for (size_t i = 1; i < ctx->factors.length; ++i) {
		Factor fac = vec_at(ctx->factors, i);

		// Rationals are normalised on creation, so equal factors must
		// have strictly equal fields.
		bool same_exponent = fac.exponent.negative == exponent.negative &&
							 fac.exponent.num == exponent.num &&
							 fac.exponent.den == exponent.den;

		if (fac.head.val == head.val && same_exponent && fac.nbr_der == 0 &&
			indices_equal(fac.indices, indices, symbol->nbr_slot)) {
			FacRef found = {.val = (u32)i};
			return found;
		}
	}

	Factor fac = {.head = head, .exponent = exponent};

	for (u8 k = 0; k < symbol->nbr_slot; ++k)
		fac.indices[k] = indices[k];
	vec_append(ctx->perm, ctx->factors, fac);

	FacRef result = {.val = (u32)(ctx->factors.length - 1)};

	return result;
}

static int compare_indices(const void* a, const void* b) {
	return index_cmp(*(const Index*)a, *(const Index*)b);
}

static bool factors_equal(const Factor* a, const Factor* b, u8 nbr_slot) {
	return a->head.val == b->head.val &&
		   rat_cmp(a->exponent, b->exponent) == 0 &&
		   indices_equal(a->indices, b->indices, nbr_slot) &&
		   a->nbr_der == b->nbr_der &&
		   indices_equal(a->der, b->der, a->nbr_der);
}

static bool check_derivative_index(Context* ctx, const Symbol* symbol,
								   const Factor* fac, Index a) {
	BundleRef space = bundle_find_derivative(ctx);
	if (space.val == 0)
		return reject_factor(ctx, symbol, "no bundle carries the derivative");

	if (symbol->flags & F_CONST_IN_SPACE)
		return reject_factor(ctx, symbol, "derivative of a constant in space");

	if (a.kind == IDX_FREE &&
		bundle_find_by_index_name(ctx, (char)a.id).val != space.val)
		return reject_factor(ctx, symbol,
							 "derivative index not of the derivative's bundle");

	if (fac->nbr_der == CCAE_MAX_DERIVATIVES)
		return reject_factor(ctx, symbol, "too many derivative indices");

	return true;
}

FacRef factor_derive(Context* ctx, FacRef fac_ref, Index a) {
	FacRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (fac_ref.val == 0 || fac_ref.val >= ctx->factors.length)
		die("CCAE (factor_derive): factor handle out of range.");

	Factor fac = vec_at(ctx->factors, fac_ref.val);
	const Symbol* symbol = &vec_at(ctx->symbols, fac.head.val);

	if (!check_derivative_index(ctx, symbol, &fac, a))
		return null_ref;

	fac.der[fac.nbr_der] = a;
	fac.nbr_der += 1;

	// We take advantage of the commutation of the derivatives to sort them and
	// have a single FacRef with the derivatives.
	qsort(fac.der, fac.nbr_der, sizeof(Index), compare_indices);

	// TODO: the same linear search as `factor_intern`.
	for (size_t i = 1; i < ctx->factors.length; ++i) {
		if (factors_equal(&vec_at(ctx->factors, i), &fac, symbol->nbr_slot)) {
			FacRef found = {.val = (u32)i};
			return found;
		}
	}

	vec_append(ctx->perm, ctx->factors, fac);

	FacRef result = {.val = (u32)(ctx->factors.length - 1)};

	return result;
}

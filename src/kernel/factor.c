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

static bool factors_equal(const Factor* a, const Factor* b, u8 nbr_slot) {
	return a->head.val == b->head.val &&
		   rat_cmp(a->exponent, b->exponent) == 0 &&
		   indices_equal(a->indices, b->indices, nbr_slot) &&
		   a->nbr_der == b->nbr_der &&
		   indices_equal(a->der, b->der, a->nbr_der);
}

// Returns the handle of `fac`, storing it the first time. Every factor is
// interned here.
static FacRef intern_factor(Context* ctx, const Factor* fac, u8 nbr_slot) {
	// TODO: This is a linear search, setup an hashmap to get O(1) accesses.
	for (size_t i = 1; i < ctx->factors.length; ++i) {
		if (factors_equal(&vec_at(ctx->factors, i), fac, nbr_slot)) {
			FacRef found = {.val = (u32)i};
			return found;
		}
	}

	vec_append(ctx->perm, ctx->factors, *fac);

	FacRef result = {.val = (u32)(ctx->factors.length - 1)};

	return result;
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

	Factor fac = {.head = head, .exponent = exponent};

	for (u8 k = 0; k < symbol->nbr_slot; ++k)
		fac.indices[k] = indices[k];

	return intern_factor(ctx, &fac, symbol->nbr_slot);
}

static int compare_indices(const void* a, const void* b) {
	return index_cmp(*(const Index*)a, *(const Index*)b);
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

	return intern_factor(ctx, &fac, symbol->nbr_slot);
}

FacRef factor_replace_indices(Context* ctx, FacRef fac_ref,
							  const Index* indices) {
	FacRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (fac_ref.val == 0 || fac_ref.val >= ctx->factors.length)
		die("CCAE (factor_replace_indices): factor handle out of range.");

	Factor fac = vec_at(ctx->factors, fac_ref.val);
	const Symbol* symbol = &vec_at(ctx->symbols, fac.head.val);

	if (!check_indices(ctx, symbol, indices))
		return null_ref;

	for (u8 k = 0; k < symbol->nbr_slot; ++k)
		fac.indices[k] = indices[k];

	BundleRef space = bundle_find_derivative(ctx);

	for (u8 k = 0; k < fac.nbr_der; ++k) {
		Index a = indices[symbol->nbr_slot + k];

		bool other_bundle =
			a.kind == IDX_FREE &&
			bundle_find_by_index_name(ctx, (char)a.id).val != space.val;

		if (other_bundle) {
			reject_factor(ctx, symbol,
						  "derivative index not of the derivative's bundle");
			return null_ref;
		}

		fac.der[k] = a;
	}

	// Replaced dummies may break the order: sort again.
	qsort(fac.der, fac.nbr_der, sizeof(Index), compare_indices);

	return intern_factor(ctx, &fac, symbol->nbr_slot);
}

int factor_cmp(const Context* ctx, FacRef a, FacRef b) {
	Factor fac_a = vec_at(ctx->factors, a.val);
	Factor fac_b = vec_at(ctx->factors, b.val);

	if (fac_a.head.val != fac_b.head.val)
		return fac_a.head.val < fac_b.head.val ? -1 : 1;

	if (fac_a.nbr_der != fac_b.nbr_der)
		return fac_a.nbr_der < fac_b.nbr_der ? -1 : 1;

	for (u8 k = 0; k < fac_a.nbr_der; ++k) {
		int der_order = index_cmp(fac_a.der[k], fac_b.der[k]);
		if (der_order != 0)
			return der_order;
	}

	int exponent_order = rat_cmp(fac_a.exponent, fac_b.exponent);
	if (exponent_order != 0)
		return exponent_order;

	u8 nbr_slot = vec_at(ctx->symbols, fac_a.head.val).nbr_slot;

	for (u8 k = 0; k < nbr_slot; ++k) {
		int index_order = index_cmp(fac_a.indices[k], fac_b.indices[k]);
		if (index_order != 0)
			return index_order;
	}

	return 0;
}

size_t factor_build_indices(Index* out, const Context* ctx, FacRef fac_ref) {
	if (fac_ref.val == 0 || fac_ref.val >= ctx->factors.length)
		die("CCAE (factor_build_indices): factor handle out of range.");

	const Factor* fac = &vec_at(ctx->factors, fac_ref.val);
	u8 nbr_slot = vec_at(ctx->symbols, fac->head.val).nbr_slot;

	for (u8 k = 0; k < nbr_slot; ++k)
		out[k] = fac->indices[k];

	for (u8 k = 0; k < fac->nbr_der; ++k)
		out[nbr_slot + k] = fac->der[k];

	return nbr_slot + fac->nbr_der;
}

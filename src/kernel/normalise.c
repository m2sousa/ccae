#include <stddef.h>

#include <ccae/config.h>
#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/canonicalise.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/normalise.h>
#include <ccae/kernel/symbol.h>

static bool is_kronecker(const Context* ctx, const Factor* fac) {
	const Symbol* symbol = &vec_at(ctx->symbols, fac->head.val);
	if (symbol->nbr_slot != 2)
		return false;

	const Bundle* bundle = &vec_at(ctx->bundles, symbol->slot_bundle[0].val);

	return bundle->kronecker.val == fac->head.val;
}

// Replaces the other occurrence of the dummy `dummy`, outside the factor at
// `skipped`, by `replacement`. Returns false if there is none.
static bool replace_partner(Context* ctx, FacRef* facs, size_t nbr_fac,
							size_t skipped, Index dummy, Index replacement) {
	for (size_t i = 0; i < nbr_fac; ++i) {
		if (i == skipped)
			continue;

		Factor fac = vec_at(ctx->factors, facs[i].val);
		u8 nbr_slot = vec_at(ctx->symbols, fac.head.val).nbr_slot;

		for (u8 k = 0; k < nbr_slot; ++k) {
			if (!index_equal(fac.indices[k], dummy))
				continue;

			fac.indices[k] = replacement;
			facs[i] = factor_intern(ctx, fac.head, fac.exponent, fac.indices);

			if (facs[i].val == 0)
				die("CCAE (normalise_term): factor rejected after absorption.");

			return true;
		}
	}

	return false;
}

// Absorbs one Kronecker delta of `*term`, if any.
// Returns a bool whether one was absorbed.
static bool absorb_kronecker(Term* term, Context* ctx) {
	Monomial mono = vec_at(ctx->monomials, term->mono.val);

	FacRef facs[CCAE_MAX_FACTORS];
	for (u64 i = 0; i < mono.nbr_fac; ++i)
		facs[i] = mono.fac[i];

	for (u64 i = 0; i < mono.nbr_fac; ++i) {
		Factor fac = vec_at(ctx->factors, facs[i].val);
		if (!is_kronecker(ctx, &fac))
			continue;

		Index up = fac.indices[0];
		Index down = fac.indices[1];

		bool absorbed = false;

		if (index_equal(up, down) && up.kind == IDX_DUMMY) {
			const Symbol* symbol = &vec_at(ctx->symbols, fac.head.val);
			u8 dim = vec_at(ctx->bundles, symbol->slot_bundle[0].val).dim;

			term->coef = rat_mul(term->coef, rat_from_int(dim));
			absorbed = true;
		} else if (up.kind == IDX_DUMMY) {
			absorbed = replace_partner(ctx, facs, mono.nbr_fac, i, up, down);
		} else if (down.kind == IDX_DUMMY) {
			absorbed = replace_partner(ctx, facs, mono.nbr_fac, i, down, up);
		}

		if (!absorbed)
			continue;

		// Drop the kronecker from the term.
		for (u64 j = i + 1; j < mono.nbr_fac; ++j)
			facs[j - 1] = facs[j];

		term->mono = mono_intern(ctx, facs, mono.nbr_fac - 1);

		return true;
	}

	return false;
}

size_t normalise_term(Term* out, size_t capacity, Context* ctx, Term term) {
	if (!ctx_ok(ctx))
		return 0;

	if (term.mono.val == 0 || term.mono.val >= ctx->monomials.length)
		die("CCAE (normalise_term): monomial handle out of range.");

	for (size_t pass = 0; pass < CCAE_MAX_NORMALISE_PASSES; ++pass) {
		int sign = 0;
		term.mono = canon_mono(ctx, term.mono, &sign);
		if (term.mono.val == 0 || sign == 0)
			return 0;

		term.coef = rat_mul(term.coef, rat_from_int(sign));

		if (absorb_kronecker(&term, ctx)) {
			if (term.mono.val == 0)
				return 0;

			continue;
		}

		if (capacity == 0) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"normalisation: more than %zu terms.", capacity);
			return 0;
		}

		out[0] = term;

		return 1;
	}

	err_set(&ctx->err, CCAE_INVALID_ARG, "normalisation: more than %zu passes.",
			CCAE_MAX_NORMALISE_PASSES);

	return 0;
}

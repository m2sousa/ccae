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
#include <ccae/kernel/symmetry.h>

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

// Contracts one symbol of `*term` with its inverse over a slot. Needs the
// Kronecker delta of the other slot's bundle.
static bool contract_inverse(Term* term, Context* ctx) {
	Monomial mono = vec_at(ctx->monomials, term->mono.val);

	for (u64 i = 0; i < mono.nbr_fac; ++i) {
		Factor fac = vec_at(ctx->factors, mono.fac[i].val);
		const Symbol* symbol = &vec_at(ctx->symbols, fac.head.val);
		if (symbol->inverse.val == 0)
			continue;

		for (u64 j = 0; j < mono.nbr_fac; ++j) {
			Factor partner = vec_at(ctx->factors, mono.fac[j].val);
			if (partner.head.val != symbol->inverse.val)
				continue;

			for (u8 k = 0; k < 2; ++k) {
				bool contracted =
					fac.indices[k].kind == IDX_DUMMY &&
					index_equal(fac.indices[k], partner.indices[k]);
				if (!contracted)
					continue;

				u8 other = (u8)(1 - k);
				const Bundle* bundle =
					&vec_at(ctx->bundles, symbol->slot_bundle[other].val);
				if (bundle->kronecker.val == 0)
					continue;

				// The Kronecker takes the two indices left.
				Index indices[] = {fac.indices[other], partner.indices[other]};
				if (symbol->slot_pos[other] == SLOT_DOWN) {
					indices[0] = partner.indices[other];
					indices[1] = fac.indices[other];
				}

				FacRef facs[CCAE_MAX_FACTORS];
				size_t nbr_kept = 0;

				for (u64 f = 0; f < mono.nbr_fac; ++f) {
					if (f != i && f != j)
						facs[nbr_kept++] = mono.fac[f];
				}

				facs[nbr_kept] = factor_intern(ctx, bundle->kronecker,
											   rat_from_int(1), indices);
				if (facs[nbr_kept].val == 0)
					die("CCAE (normalise_term): Kronecker delta rejected.");

				term->mono = mono_intern(ctx, facs, nbr_kept + 1);

				return true;
			}
		}
	}

	return false;
}

// FIXME: This might be deleted once metrics are introduced and eps_down/_up are
// removed for a single eps.
typedef struct {
	size_t eps_down;
	size_t eps_up;
} EpsilonPair;

static bool share_dummy(const Factor* a, const Factor* b, u8 nbr_slot) {
	for (u8 k = 0; k < nbr_slot; ++k) {
		if (a->indices[k].kind != IDX_DUMMY)
			continue;

		for (u8 l = 0; l < nbr_slot; ++l) {
			if (index_equal(a->indices[k], b->indices[l]))
				return true;
		}
	}

	return false;
}

static bool check_and_find_epsilon_pair(EpsilonPair* out, const Context* ctx,
										const Monomial* mono) {
	for (u64 i = 0; i < mono->nbr_fac; ++i) {
		Factor eps_down = vec_at(ctx->factors, mono->fac[i].val);
		const Symbol* symbol = &vec_at(ctx->symbols, eps_down.head.val);
		if (symbol->nbr_slot == 0)
			continue;

		const Bundle* bundle =
			&vec_at(ctx->bundles, symbol->slot_bundle[0].val);
		if (bundle->eps_down.val != eps_down.head.val ||
			bundle->kronecker.val == 0)
			continue;

		for (u64 j = 0; j < mono->nbr_fac; ++j) {
			Factor eps_up = vec_at(ctx->factors, mono->fac[j].val);
			if (eps_up.head.val != bundle->eps_up.val)
				continue;

			if (share_dummy(&eps_down, &eps_up, symbol->nbr_slot)) {
				out->eps_down = i;
				out->eps_up = j;
				return true;
			}
		}
	}

	return false;
}

static size_t expand_epsilon_pair(Term* out, size_t capacity, Context* ctx,
								  Term term, EpsilonPair pair) {
	Monomial mono = vec_at(ctx->monomials, term.mono.val);
	Factor eps_down = vec_at(ctx->factors, mono.fac[pair.eps_down].val);
	Factor eps_up = vec_at(ctx->factors, mono.fac[pair.eps_up].val);

	const Symbol* symbol = &vec_at(ctx->symbols, eps_up.head.val);
	const Bundle* bundle = &vec_at(ctx->bundles, symbol->slot_bundle[0].val);

	// One Kronecker δ per slot of ε in each term of the sum.
	// ε_{a₁..aₙ} ε^{b₁..bₙ} = Σ_σ sign(σ) δ^{b_σ(1)}_{a₁} ... δ^{b_σ(n)}_{aₙ}.
	u8 nbr_kronecker = symbol->nbr_slot;

	FacRef facs[CCAE_MAX_FACTORS];
	size_t nbr_kept = 0;
	for (u64 i = 0; i < mono.nbr_fac; ++i) {
		if (i != pair.eps_down && i != pair.eps_up)
			facs[nbr_kept++] = mono.fac[i];
	}

	if (nbr_kept + nbr_kronecker > CCAE_MAX_FACTORS) {
		err_set(&ctx->err, CCAE_INVALID_ARG,
				"normalisation: more than %zu factors.", CCAE_MAX_FACTORS);
		return 0;
	}

	size_t nbr_out = 0;
	for (u16 p = 0; p < symbol->orbit->nbr_perm; ++p) {
		SignedPerm perm = symbol->orbit->perms[p];

		for (u8 k = 0; k < nbr_kronecker; ++k) {
			Index indices[] = {eps_up.indices[perm.image[k]],
							   eps_down.indices[k]};

			facs[nbr_kept + k] =
				factor_intern(ctx, bundle->kronecker, rat_from_int(1), indices);
			if (facs[nbr_kept + k].val == 0)
				die("CCAE (normalise_term): Kronecker delta rejected.");
		}

		Term expanded = {.coef = rat_mul(term.coef, rat_from_int(perm.sign)),
						 .mono =
							 mono_intern(ctx, facs, nbr_kept + nbr_kronecker)};
		if (expanded.mono.val == 0)
			return 0;

		nbr_out +=
			normalise_term(out + nbr_out, capacity - nbr_out, ctx, expanded);
		if (!ctx_ok(ctx))
			return 0;
	}

	return nbr_out;
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

		if (absorb_kronecker(&term, ctx) || contract_inverse(&term, ctx)) {
			if (term.mono.val == 0)
				return 0;

			continue;
		}

		EpsilonPair pair = {};
		const Monomial* mono = &vec_at(ctx->monomials, term.mono.val);

		if (check_and_find_epsilon_pair(&pair, ctx, mono))
			return expand_epsilon_pair(out, capacity, ctx, term, pair);

		if (capacity == 0) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"normalisation: too many terms.");
			return 0;
		}

		out[0] = term;

		return 1;
	}

	err_set(&ctx->err, CCAE_INVALID_ARG, "normalisation: more than %zu passes.",
			CCAE_MAX_NORMALISE_PASSES);

	return 0;
}

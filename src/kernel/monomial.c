#include <stddef.h>

#include <ccae/config.h>
#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

typedef struct {
	Index index;
	BundleRef bundle;
	SlotPos pos;
} Occurrence;

static bool factor_comes_before(const Context* ctx, FacRef a, FacRef b) {
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

		while (j > 0 && factor_comes_before(ctx, current, mono->fac[j - 1])) {
			mono->fac[j] = mono->fac[j - 1];
			--j;
		}

		mono->fac[j] = current;
	}
}

// Merge scalar factor (ex: x*x => x^2) by adding their power,
// do nothing otherwise. Might fail if a factor is rejected on interning.
static bool check_and_merge_powers(Context* ctx, Monomial* mono) {
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

		FacRef merged = factor_intern(ctx, first.head, power, NULL);
		if (merged.val == 0)
			return false;

		mono->fac[nbr_kept++] = merged;
	}

	mono->nbr_fac = (u64)nbr_kept;

	return true;
}

static bool reject_index(Context* ctx, Index index, const char* problem) {
	if (index.kind == IDX_FREE)
		err_set(&ctx->err, CCAE_INVALID_ARG, "monomial: index '%c' %s.",
				(char)index.id, problem);
	else
		err_set(&ctx->err, CCAE_INVALID_ARG, "monomial: dummy %u %s.",
				(unsigned)index.id, problem);

	return false;
}

// Constructs the occurences: index and its associated bundle & symbol.
// Returns the number of occurences.
static size_t build_occurrences(Occurrence* out, const Context* ctx,
								const Monomial* mono) {
	size_t nbr_occ = 0;

	for (u64 i = 0; i < mono->nbr_fac; ++i) {
		Factor fac = vec_at(ctx->factors, mono->fac[i].val);
		const Symbol* sym = &vec_at(ctx->symbols, fac.head.val);

		for (u8 k = 0; k < sym->nbr_slot; ++k) {
			Occurrence occ = {.index = fac.indices[k],
							  .bundle = sym->slot_bundle[k],
							  .pos = sym->slot_pos[k]};
			out[nbr_occ++] = occ;
		}
	}

	return nbr_occ;
}

// Returns how many occurrences hold the same index as `occ[i]`.
// `out` gets the last occurence index, or `i` if there is none.
static size_t count_uses(size_t* out, const Occurrence* occ, size_t nbr_occ,
						 size_t i) {
	size_t nbr_use = 0;
	*out = i;

	for (size_t j = 0; j < nbr_occ; ++j) {
		if (!index_equal(occ[j].index, occ[i].index))
			continue;

		++nbr_use;
		if (j != i)
			*out = j;
	}

	return nbr_use;
}

static bool check_repeated_indices(Context* ctx, const Occurrence* occ,
								   size_t nbr_occ) {
	for (size_t i = 0; i < nbr_occ; ++i) {
		size_t partner = i;
		size_t nbr_use = count_uses(&partner, occ, nbr_occ, i);

		if (nbr_use > 2)
			return reject_index(ctx, occ[i].index, "used more than twice");

		if (nbr_use == 1) {
			if (occ[i].index.kind == IDX_DUMMY)
				return reject_index(ctx, occ[i].index, "used once");
			continue;
		}

		if (occ[partner].pos == occ[i].pos)
			return reject_index(ctx, occ[i].index,
								"used twice in the same position");

		if (occ[partner].bundle.val != occ[i].bundle.val)
			return reject_index(ctx, occ[i].index, "used in two bundles");
	}

	return true;
}

// Writes into `out` the index of each occurrence. Dummies are marked
// in the process.
static void build_indices_with_dummies(Index* out, const Occurrence* occ,
									   size_t nbr_occ) {
	u16 nbr_dummy = 0;

	for (size_t i = 0; i < nbr_occ; ++i) {
		size_t partner = i;
		size_t nbr_use = count_uses(&partner, occ, nbr_occ, i);

		if (nbr_use == 1) {
			out[i] = occ[i].index;
		} else if (partner < i) {
			// We have already written the partner of the pair, thus we copy.
			out[i] = out[partner];
		} else {
			// First index from a pair, marks it as dummy.
			Index dummy = {.kind = IDX_DUMMY, .id = nbr_dummy++};
			out[i] = dummy;
		}
	}
}

// Replaces each tensor factor of `mono` by the same factor with its slice of
// `indices_with_dummies`, laid out as `build_occurrences` lists them.
static void rebuild_factors_with_dummies(Context* ctx, Monomial* mono,
										 const Index* indices_with_dummies) {
	size_t offset = 0;

	for (u64 i = 0; i < mono->nbr_fac; ++i) {
		Factor fac = vec_at(ctx->factors, mono->fac[i].val);
		u8 nbr_slot = vec_at(ctx->symbols, fac.head.val).nbr_slot;

		if (nbr_slot == 0)
			continue;

		FacRef rebuilt = factor_intern(ctx, fac.head, fac.power,
									   &indices_with_dummies[offset]);

		// The power and free indices were already accepted, and dummies are
		// not checked: this cannot fail.
		if (rebuilt.val == 0)
			die("CCAE (mono_intern): rebuilt factor rejected.");

		mono->fac[i] = rebuilt;
		offset += nbr_slot;
	}
}

// Numbers the dummies of `mono` from 0 in order of first appearance.
// Fails on any other repeated index.
static bool check_and_build_dummies(Context* ctx, Monomial* mono) {
	Occurrence occ[CCAE_MAX_INDICES];
	size_t nbr_occ = build_occurrences(occ, ctx, mono);

	if (!check_repeated_indices(ctx, occ, nbr_occ))
		return false;

	Index indices_with_dummies[CCAE_MAX_INDICES];
	build_indices_with_dummies(indices_with_dummies, occ, nbr_occ);
	rebuild_factors_with_dummies(ctx, mono, indices_with_dummies);

	return true;
}

static bool have_same_factors(const Monomial* a, const Monomial* b) {
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

	if (!check_and_merge_powers(ctx, &mono))
		return null_ref;

	if (!check_and_build_dummies(ctx, &mono))
		return null_ref;

	// Building up the dummies changes the factor `rebuild_factors_with_dummies`
	// therefore we sort the handles again.
	sort_factors(ctx, &mono);

	// TODO: This is a linear search, setup an hashmap to get O(1) accesses.
	for (size_t i = 1; i < ctx->monomials.length; ++i) {
		if (have_same_factors(&vec_at(ctx->monomials, i), &mono)) {
			MonoRef found = {.val = (u32)i};
			return found;
		}
	}

	vec_append(ctx->perm, ctx->monomials, mono);

	MonoRef mr = {.val = (u32)(ctx->monomials.length - 1)};

	return mr;
}

static u16 count_dummies(const Context* ctx, const Monomial* mono) {
	u16 nbr_dummy = 0;

	for (u64 i = 0; i < mono->nbr_fac; ++i) {
		Factor fac = vec_at(ctx->factors, mono->fac[i].val);
		u8 nbr_slot = vec_at(ctx->symbols, fac.head.val).nbr_slot;

		for (u8 k = 0; k < nbr_slot; ++k) {
			Index index = fac.indices[k];

			if (index.kind == IDX_DUMMY && index.id >= nbr_dummy)
				nbr_dummy = (u16)(index.id + 1);
		}
	}

	return nbr_dummy;
}

static FacRef shift_dummies(Context* ctx, FacRef f, u16 id_shift) {
	Factor fac = vec_at(ctx->factors, f.val);
	u8 nbr_slot = vec_at(ctx->symbols, fac.head.val).nbr_slot;

	if (nbr_slot == 0 || id_shift == 0)
		return f;

	for (u8 k = 0; k < nbr_slot; ++k) {
		if (fac.indices[k].kind == IDX_DUMMY)
			fac.indices[k].id = (u16)(fac.indices[k].id + id_shift);
	}

	FacRef shifted = factor_intern(ctx, fac.head, fac.power, fac.indices);

	if (shifted.val == 0)
		die("CCAE (mono_mul): id shifted factor rejected.");

	return shifted;
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

	u16 id_shift = count_dummies(ctx, &mono_a);

	for (u64 i = 0; i < mono_b.nbr_fac; ++i)
		facs[nbr_fac++] = shift_dummies(ctx, mono_b.fac[i], id_shift);

	return mono_intern(ctx, facs, nbr_fac);
}

static size_t keep_free_occurrences(Occurrence* occ, size_t nbr_occ) {
	size_t nbr_kept = 0;

	for (size_t i = 0; i < nbr_occ; ++i) {
		if (occ[i].index.kind == IDX_FREE)
			occ[nbr_kept++] = occ[i];
	}

	return nbr_kept;
}

static bool holds_occurrence(const Occurrence* occ, size_t nbr_occ,
							 Occurrence wanted) {
	for (size_t i = 0; i < nbr_occ; ++i) {
		if (index_equal(occ[i].index, wanted.index) && occ[i].pos == wanted.pos)
			return true;
	}

	return false;
}

bool mono_have_same_free_indices(const Context* ctx, MonoRef a, MonoRef b) {
	if (a.val == 0 || a.val >= ctx->monomials.length || b.val == 0 ||
		b.val >= ctx->monomials.length)
		die("CCAE (mono_have_same_free_indices): monomial handle out of "
			"range.");

	Occurrence occ_a[CCAE_MAX_INDICES];
	Occurrence occ_b[CCAE_MAX_INDICES];

	size_t nbr_a =
		build_occurrences(occ_a, ctx, &vec_at(ctx->monomials, a.val));
	size_t nbr_b =
		build_occurrences(occ_b, ctx, &vec_at(ctx->monomials, b.val));

	nbr_a = keep_free_occurrences(occ_a, nbr_a);
	nbr_b = keep_free_occurrences(occ_b, nbr_b);

	if (nbr_a != nbr_b)
		return false;

	// By construction, a free index appears **at most** once in a monomial, so
	// equal counts and every index of `a` found in `b` mean the same set.
	for (size_t i = 0; i < nbr_a; ++i) {
		if (!holds_occurrence(occ_b, nbr_b, occ_a[i]))
			return false;
	}

	return true;
}

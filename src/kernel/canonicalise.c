#include <stddef.h>
#include <stdint.h>

#include <ccae/config.h>
#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/canonicalise.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>
#include <ccae/kernel/symmetry.h>

// One arrangement of a monomial: the indices of its tensor factors in reading
// order, dummies renumbered from 0, and the sign the arrangement costs.
typedef struct {
	Index indices[CCAE_MAX_INDICES];
	int sign;
} Arrangement;

static void renumber_dummies(Index* indices, size_t nbr_index) {
	constexpr u16 UNSEEN = UINT16_MAX;

	u16 renamed[CCAE_MAX_INDICES];
	for (size_t i = 0; i < nbr_index; ++i)
		renamed[i] = UNSEEN;

	u16 nbr_dummy = 0;

	for (size_t i = 0; i < nbr_index; ++i) {
		if (indices[i].kind != IDX_DUMMY)
			continue;

		u16 old_id = indices[i].id;
		if (old_id >= nbr_index)
			die("CCAE (canon_mono): dummy number out of range.");

		if (renamed[old_id] == UNSEEN)
			renamed[old_id] = nbr_dummy++;

		indices[i].id = renamed[old_id];
	}
}

// Place f holds the tensor factor `tensors[tensor_order[f]]`, rearranged by
// the element `tensor_perm_number[f]` of its orbit, then its derivative indices
// in the order `der_order[f]`.
static void build_arrangement(Arrangement* out, const Context* ctx,
							  const Factor* tensors, const u8* tensor_order,
							  const u16* tensor_perm_number,
							  const u8 (*der_order)[CCAE_MAX_DERIVATIVES],
							  size_t nbr_tensor) {
	size_t offset = 0;
	out->sign = 1;

	for (size_t f = 0; f < nbr_tensor; ++f) {
		const Factor* fac = &tensors[tensor_order[f]];
		const Symbol* symbol = &vec_at(ctx->symbols, fac->head.val);

		SignedPerm perm = symbol->nbr_slot > 0
							  ? symbol->orbit->perms[tensor_perm_number[f]]
							  : signed_perm_identity();

		for (u8 k = 0; k < symbol->nbr_slot; ++k)
			out->indices[offset + perm.image[k]] = fac->indices[k];

		out->sign *= perm.sign;
		offset += symbol->nbr_slot;

		// Derivatives commute: any order of them, with no sign.
		for (u8 k = 0; k < fac->nbr_der; ++k)
			out->indices[offset + k] = fac->der[der_order[f][k]];

		offset += fac->nbr_der;
	}

	renumber_dummies(out->indices, offset);
}

// Returns -1, 0 or 1 as `index_cmp`. Signs are not compared.
static int arrangement_cmp(const Arrangement* a, const Arrangement* b,
						   size_t nbr_index) {
	for (size_t i = 0; i < nbr_index; ++i) {
		int order = index_cmp(a->indices[i], b->indices[i]);
		if (order != 0)
			return order;
	}

	return 0;
}

static void reverse(u8* values, size_t first, size_t end) {
	while (first + 1 < end) {
		--end;

		u8 kept = values[first];
		values[first] = values[end];
		values[end] = kept;

		++first;
	}
}

static bool are_interchangeable(const Factor* a, const Factor* b) {
	return a->head.val == b->head.val &&
		   rat_cmp(a->exponent, b->exponent) == 0 && a->nbr_der == b->nbr_der;
}

// Moves `order[first .. end)` to its next permutation in dictionary order.
// Returns false, back on the increasing order, after the last one.
static bool next_permutation(u8* order, size_t first, size_t end) {
	// Find the last place whose value is smaller than the next one's.
	size_t pivot = end - 1;
	while (pivot > first && order[pivot - 1] >= order[pivot])
		--pivot;

	if (pivot == first) {
		reverse(order, first, end);
		return false;
	}

	// Swap it with the last larger value, then sort what follows it.
	size_t larger = end - 1;
	while (order[larger] <= order[pivot - 1])
		--larger;

	u8 kept = order[pivot - 1];
	order[pivot - 1] = order[larger];
	order[larger] = kept;

	reverse(order, pivot, end);

	return true;
}

static bool next_tensor_perm_number(u16* tensor_perm_number,
									u8 (*der_order)[CCAE_MAX_DERIVATIVES],
									const Context* ctx, const Factor* tensors,
									size_t nbr_tensor) {
	for (size_t f = 0; f < nbr_tensor; ++f) {
		u8 nbr_der = tensors[f].nbr_der;
		if (nbr_der > 1 && next_permutation(der_order[f], 0, nbr_der))
			return true;

		const Symbol* symbol = &vec_at(ctx->symbols, tensors[f].head.val);

		// A scalar has no orbit, only the identity.
		u16 nbr_perm = symbol->nbr_slot > 0 ? symbol->orbit->nbr_perm : 1;

		tensor_perm_number[f] += 1;
		if (tensor_perm_number[f] < nbr_perm)
			return true;

		tensor_perm_number[f] = 0;
	}

	return false;
}

static bool next_tensor_order(u8* tensor_order, const Factor* tensors,
							  size_t nbr_tensor) {
	size_t first = 0;

	while (first < nbr_tensor) {
		size_t end = first + 1;
		while (end < nbr_tensor &&
			   are_interchangeable(&tensors[first], &tensors[end]))
			++end;

		if (end - first > 1 && next_permutation(tensor_order, first, end))
			return true;

		first = end;
	}

	return false;
}

// Interns the monomial `mono` with its tensor factors indices written as the
// `best` arrangement. Place f takes the indices of `tensor_refs[f].
static MonoRef build_monomial(Context* ctx, const Monomial* mono,
							  const FacRef* tensor_refs, size_t nbr_tensor,
							  const Arrangement* best) {
	FacRef facs[CCAE_MAX_FACTORS];
	size_t nbr_fac = 0;

	for (u64 i = 0; i < mono->nbr_fac; ++i) {
		if (factor_count_indices(ctx, mono->fac[i]) == 0)
			facs[nbr_fac++] = mono->fac[i];
	}

	size_t offset = 0;

	for (size_t f = 0; f < nbr_tensor; ++f) {
		size_t nbr_index = factor_count_indices(ctx, tensor_refs[f]);

		facs[nbr_fac] =
			factor_replace_indices(ctx, tensor_refs[f], &best->indices[offset]);
		if (facs[nbr_fac].val == 0)
			die("CCAE (canon_mono): arranged factor rejected.");

		++nbr_fac;
		offset += nbr_index;
	}

	return mono_intern(ctx, facs, nbr_fac);
}

MonoRef canon_mono(Context* ctx, MonoRef m, int* sign) {
	MonoRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (m.val == 0 || m.val >= ctx->monomials.length)
		die("CCAE (canon_mono): monomial handle out of range.");

	Monomial mono = vec_at(ctx->monomials, m.val);

	Factor tensors[CCAE_MAX_FACTORS] = {};
	FacRef tensor_refs[CCAE_MAX_FACTORS] = {};
	size_t nbr_tensor = 0;
	size_t nbr_index = 0;

	for (u64 i = 0; i < mono.nbr_fac; ++i) {
		size_t nbr_fac_index = factor_count_indices(ctx, mono.fac[i]);

		if (nbr_fac_index == 0)
			continue;

		tensor_refs[nbr_tensor] = mono.fac[i];
		tensors[nbr_tensor++] = vec_at(ctx->factors, mono.fac[i].val);
		nbr_index += nbr_fac_index;
	}

	// `tensor_order` is an array for which entry represents its position in the
	// product. It starts with each entry being its read position in factors of
	// the monomial.
	u8 tensor_order[CCAE_MAX_FACTORS];
	for (size_t f = 0; f < nbr_tensor; ++f)
		tensor_order[f] = (u8)f;

	// `tensor_perm_number` is an array for which entry represents an index of
	// the signed permutation into the orbit of the factor. It starts with each
	// entry being zero: all permutation being the identity (symbol->orbit[0] is
	// **always** the identify by construction).
	u16 tensor_perm_number[CCAE_MAX_FACTORS] = {};

	// `der_order[f]` is the order in which place f writes its derivative
	// indices. It starts as the stored order.
	u8 der_order[CCAE_MAX_FACTORS][CCAE_MAX_DERIVATIVES];
	for (size_t f = 0; f < nbr_tensor; ++f) {
		for (u8 k = 0; k < tensors[f].nbr_der; ++k)
			der_order[f][k] = k;
	}

	Arrangement best = {};
	Arrangement current = {};

	// Therefore, at first, the best entry is the one with identities applied
	// and at their initial position.
	build_arrangement(&best, ctx, tensors, tensor_order, tensor_perm_number,
					  der_order, nbr_tensor);

	bool equals_its_negative = false;

	do {
		do {
			build_arrangement(&current, ctx, tensors, tensor_order,
							  tensor_perm_number, der_order, nbr_tensor);

			int order = arrangement_cmp(&current, &best, nbr_index);

			if (order < 0) {
				best = current;
				equals_its_negative = false;
			} else if (order == 0 && current.sign != best.sign) {
				// Same arrangement of indices, but the other sign: x = -x.
				equals_its_negative = true;
			}
		} while (next_tensor_perm_number(tensor_perm_number, der_order, ctx,
										 tensors, nbr_tensor));
	} while (next_tensor_order(tensor_order, tensors, nbr_tensor));

	*sign = equals_its_negative ? 0 : best.sign;

	return build_monomial(ctx, &mono, tensor_refs, nbr_tensor, &best);
}

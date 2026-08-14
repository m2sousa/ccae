#include <stddef.h>
#include <stdint.h>

#include <ccae/config.h>
#include <ccae/core/error.h>
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

static void build_arrangement(Arrangement* out, const Context* ctx,
							  const Factor* tensors,
							  const u16* tensor_perm_number,
							  size_t nbr_tensor) {
	size_t offset = 0;
	out->sign = 1;

	for (size_t f = 0; f < nbr_tensor; ++f) {
		const Symbol* symbol = &vec_at(ctx->symbols, tensors[f].head.val);
		SignedPerm perm = symbol->orbit->perms[tensor_perm_number[f]];

		for (u8 k = 0; k < symbol->nbr_slot; ++k)
			out->indices[offset + perm.image[k]] = tensors[f].indices[k];

		out->sign *= perm.sign;
		offset += symbol->nbr_slot;
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

static bool next_tensor_perm_number(u16* tensor_perm_number, const Context* ctx,
									const Factor* tensors, size_t nbr_tensor) {
	for (size_t f = 0; f < nbr_tensor; ++f) {
		const Symbol* symbol = &vec_at(ctx->symbols, tensors[f].head.val);

		tensor_perm_number[f] += 1;
		if (tensor_perm_number[f] < symbol->orbit->nbr_perm)
			return true;

		tensor_perm_number[f] = 0;
	}

	return false;
}

// Interns the monomial `mono` with its tensor factors indices written as the
// `best` arrangement.
static MonoRef build_monomial(Context* ctx, const Monomial* mono,
							  const Arrangement* best) {
	FacRef facs[CCAE_MAX_FACTORS];
	size_t offset = 0;

	for (u64 i = 0; i < mono->nbr_fac; ++i) {
		Factor fac = vec_at(ctx->factors, mono->fac[i].val);
		u8 nbr_slot = vec_at(ctx->symbols, fac.head.val).nbr_slot;

		if (nbr_slot == 0) {
			facs[i] = mono->fac[i];
			continue;
		}

		facs[i] =
			factor_intern(ctx, fac.head, fac.exponent, &best->indices[offset]);

		if (facs[i].val == 0)
			die("CCAE (canon_mono): arranged factor rejected.");

		offset += nbr_slot;
	}

	return mono_intern(ctx, facs, mono->nbr_fac);
}

MonoRef canon_mono(Context* ctx, MonoRef m, int* sign) {
	MonoRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (m.val == 0 || m.val >= ctx->monomials.length)
		die("CCAE (canon_mono): monomial handle out of range.");

	Monomial mono = vec_at(ctx->monomials, m.val);

	Factor tensors[CCAE_MAX_FACTORS] = {};
	size_t nbr_tensor = 0;
	size_t nbr_index = 0;

	for (u64 i = 0; i < mono.nbr_fac; ++i) {
		Factor fac = vec_at(ctx->factors, mono.fac[i].val);
		u8 nbr_slot = vec_at(ctx->symbols, fac.head.val).nbr_slot;

		if (nbr_slot == 0)
			continue;

		tensors[nbr_tensor++] = fac;
		nbr_index += nbr_slot;
	}

	// `tensor_perm_number` is an array for which entry represents an index of
	// the signed permutation into the orbit of the factor. It starts with each
	// entry being zero: all permutation being the identity (symbol->orbit[0] is
	// **always** the identify by construction).
	u16 tensor_perm_number[CCAE_MAX_FACTORS] = {};
	Arrangement best = {};
	Arrangement current = {};

	// Therefore, at first, the best entry is the one with identities applied.
	build_arrangement(&best, ctx, tensors, tensor_perm_number, nbr_tensor);

	bool equals_its_negative = false;

	while (
		next_tensor_perm_number(tensor_perm_number, ctx, tensors, nbr_tensor)) {
		build_arrangement(&current, ctx, tensors, tensor_perm_number,
						  nbr_tensor);

		int order = arrangement_cmp(&current, &best, nbr_index);

		if (order < 0) {
			best = current;
			equals_its_negative = false;
		} else if (order == 0 && current.sign != best.sign) {
			// Same arrangement of indices, but the other sign: x = -x.
			equals_its_negative = true;
		}
	}

	*sign = equals_its_negative ? 0 : best.sign;

	return build_monomial(ctx, &mono, &best);
}

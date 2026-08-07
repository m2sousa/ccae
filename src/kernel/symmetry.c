#include <stddef.h>
#include <string.h>

#include <ccae/config.h>
#include <ccae/core/error.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symmetry.h>
#include <ccae/utils.h>

SignedPerm signed_perm_identity(void) {
	SignedPerm id = {.sign = 1};

	for (u8 k = 0; k < CCAE_MAX_SLOT; ++k)
		id.image[k] = k;

	return id;
}

SignedPerm signed_perm_compose(SignedPerm a, SignedPerm b) {
	SignedPerm composed = {.sign = (i8)(a.sign * b.sign)};

	for (size_t k = 0; k < CCAE_MAX_SLOT; ++k)
		composed.image[k] = a.image[b.image[k]];

	return composed;
}

bool signed_perm_equal(SignedPerm a, SignedPerm b) {
	if (a.sign != b.sign)
		return false;

	for (size_t k = 0; k < CCAE_MAX_SLOT; ++k) {
		if (a.image[k] != b.image[k])
			return false;
	}

	return true;
}

SignedPerm signed_perm_swap(u8 first, u8 second, i8 sign) {
	SignedPerm swap = signed_perm_identity();

	swap.image[first] = second;
	swap.image[second] = first;
	swap.sign = sign;

	return swap;
}

static bool reject_symmetry(Context* ctx, const char* symmetry,
							const char* problem) {
	err_set(&ctx->err, CCAE_INVALID_ARG, "symmetry '%s': %s.", symmetry,
			problem);

	return false;
}

static bool read_sym_or_antisym(Generators* out, bool* used, Context* ctx,
								const char* symmetry, u8 nbr_slot,
								const char** s) {
	constexpr char SYM_PREFIX[] = "sym(";
	constexpr char ANTISYM_PREFIX[] = "antisym(";
	i8 sign = 0;

	if (strncmp(*s, SYM_PREFIX, strlen(SYM_PREFIX)) == 0) {
		sign = 1;
		*s += strlen(SYM_PREFIX);
	} else if (strncmp(*s, ANTISYM_PREFIX, strlen(ANTISYM_PREFIX)) == 0) {
		sign = -1;
		*s += strlen(ANTISYM_PREFIX);
	} else {
		return reject_symmetry(ctx, symmetry, "expected 'sym(' or 'antisym('");
	}

	u8 slots[CCAE_MAX_SLOT];
	u8 nbr_clause_slot = 0;

	while (true) {
		if (!is_digit(**s))
			return reject_symmetry(ctx, symmetry, "expected a slot number");

		u8 slot = (u8)(**s - '0');
		++*s;

		if (slot >= nbr_slot)
			return reject_symmetry(ctx, symmetry, "slot out of range");

		if (used[slot])
			return reject_symmetry(ctx, symmetry, "slot in two places");

		used[slot] = true;
		slots[nbr_clause_slot++] = slot;

		if (**s == ')')
			break;

		if (**s != ',')
			return reject_symmetry(ctx, symmetry, "expected ',' or ')'");

		++*s;
	}

	++*s;

	if (nbr_clause_slot < 2)
		return reject_symmetry(ctx, symmetry,
							   "a clause needs at least two slots");

	for (u8 k = 0; k + 1 < nbr_clause_slot; ++k) {
		out->generators[out->nbr_generator++] =
			signed_perm_swap(slots[k], slots[k + 1], sign);
	}

	return true;
}

bool symmetry_check_and_read(Generators* out, Context* ctx,
							 const char* symmetry, u8 nbr_slot) {
	if (!ctx_ok(ctx))
		return false;

	if (nbr_slot > CCAE_MAX_SLOT)
		die("CCAE (symmetry_check_and_read): more than CCAE_MAX_SLOT slots.");

	Generators read = {.nbr_generator = 0};
	bool used[CCAE_MAX_SLOT] = {};
	const char* s = symmetry;

	while (true) {
		while (*s == ' ')
			++s;

		if (*s == '\0')
			break;

		if (!read_sym_or_antisym(&read, used, ctx, symmetry, nbr_slot, &s))
			return false;

		if (*s != ' ' && *s != '\0')
			return reject_symmetry(ctx, symmetry,
								   "expected a space between clauses");
	}

	*out = read;

	return true;
}

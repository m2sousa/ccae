#include <stddef.h>

#include <ccae/config.h>
#include <ccae/kernel/symmetry.h>

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

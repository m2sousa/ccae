#ifndef CCAE_SYMMETRY_H
#define CCAE_SYMMETRY_H

#include <ccae/config.h>
#include <ccae/types.h>

typedef struct {
	u8 image[CCAE_MAX_SLOT];
	i8 sign;
} SignedPerm;

/* Returns the signed permutation that moves no slot with sign +1.
 *
 * Example:
 *		`SignedPerm id = signed_perm_identity();` */
SignedPerm signed_perm_identity(void);

/* Returns the composed permutation of `b` then `a`.
 *
 * Example:
 *		`Signed perm c = signed_perm_compose(a, b)` */
SignedPerm signed_perm_compose(SignedPerm a, SignedPerm b);

/* Returns whether `a` and `b` are the same signed permutations.
 *
 * Example:
 *		`signed_perm_equal(id, signed_perm_identity())` is true. */
bool signed_perm_equal(SignedPerm a, SignedPerm b);

#endif

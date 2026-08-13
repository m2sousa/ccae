#ifndef CCAE_SYMMETRY_H
#define CCAE_SYMMETRY_H

#include <ccae/config.h>
#include <ccae/kernel/fwd.h>
#include <ccae/types.h>

typedef struct {
	u8 image[CCAE_MAX_SLOT];
	i8 sign;
} SignedPerm;

typedef struct {
	u8 nbr_generator;
	SignedPerm generators[CCAE_MAX_GENERATORS];
} Generators;

typedef struct {
	u16 nbr_perm;
	SignedPerm perms[CCAE_MAX_ORBIT];
} Orbit;

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

/* Returns the signed permutation that exchanges slots `first` and `second`,
 * with sign `sign`.
 *
 * Example:
 *		`SignedPerm antisym_01 = signed_perm_swap(0, 1, -1);` */
SignedPerm signed_perm_swap(u8 first, u8 second, i8 sign);

/* Reads the symmetry string `symmetry` of a symbol with `nbr_slot`.
 * Builds generators `out`: each clause of n slots gives the n - 1 swaps of
 * neighbours in the clause, with sign +1 for `sym` and -1 for `antisym`.
 * Sets `ctx->err` and returns false on bad syntax.
 *
 * Example:
 *		`symmetry_check_and_read(&gens, ctx, "sym(0,1) antisym(2,3)", 4)` */
[[nodiscard]] bool symmetry_check_and_read(Generators* out, Context* ctx,
										   const char* symmetry, u8 nbr_slot);

/* Builds into `out` the orbit of the generators `gens`.
 * Dies if one permutation is reached with both signs.
 *
 * Example:
 *		the generators of "antisym(0,1,2)" give an orbit of 6. */
void symmetry_build_orbit(Orbit* out, const Generators* gens);

#endif

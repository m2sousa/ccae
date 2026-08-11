#ifndef CCAE_TEST_KERNEL_UTILS_H
#define CCAE_TEST_KERNEL_UTILS_H

#include <ccae/kernel/context.h>
#include <ccae/kernel/index.h>

/* Declares a spatial ("abcd") and an internal ("ijk") bundle, then the
 * canonical pair `dE^a_i` and `dK^i_a`. Tells whether all went well.
 *
 * Example:
 *		`CHECK(declare_canonical_pair(ctx, &dE, &dK));` */
bool declare_canonical_pair(Context* ctx, SymRef* dE, SymRef* dK);

/* Returns the factor `head` with exponent 1 and the two indices given.
 *
 * Example:
 *		`with_indices(ctx, dE, a, d0)` is `dE^a_$0`. */
FacRef with_indices(Context* ctx, SymRef head, Index first, Index second);

/* Returns the factor `head` with exponent 1 and two free indices, given by
 * their index names.
 *
 * Example:
 *		`with_free_indices(ctx, dE, 'a', 'i')` is `dE^a_i`. */
FacRef with_free_indices(Context* ctx, SymRef head, char first, char second);

#endif

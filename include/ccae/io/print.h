#ifndef CCAE_PRINT_H
#define CCAE_PRINT_H

#include <stddef.h>

#include <ccae/kernel/fwd.h>

/* Writes `sym` in ASCII into `buf`, which holds `size` bytes: its name and
 * slots, its kind, then its flags joined by `|`. The output is truncated to fit
 * and always ends with '\0' (unless `size` is 0).
 *
 * A symbol stores the bundle of each slot, not the index name it was declared
 * with, so the k-th slot in a bundle is printed with that bundle's k-th index
 * name.
 *
 * Dies if `sym` is the null handle or out of range.
 *
 * Example:
 *		`print_symbol(ctx, dK, buf, sizeof(buf));` gives
 *		"dK^i_a field PHASE_SPACE|ZERO_MODE_FREE". */
void print_symbol(const Context* ctx, SymRef sym, char* buf, size_t size);

#endif

#ifndef CCAE_PRINT_H
#define CCAE_PRINT_H

#include <stddef.h>

#include <ccae/kernel/fwd.h>

/* Writes `sym` into `buf` (`size` bytes, truncated, '\0'-terminated): name
 * and slots, kind, flags. The k-th slot of a bundle gets its k-th index name.
 * Dies on a bad handle.
 *
 * Example:
 *		`print_symbol(ctx, dK, buf, sizeof(buf));` gives
 *		"dK^i_a field PHASE_SPACE|ZERO_MODE_FREE". */
void print_symbol(const Context* ctx, SymRef sym, char* buf, size_t size);

#endif

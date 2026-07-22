#ifndef CCAE_PRINT_H
#define CCAE_PRINT_H

#include <stddef.h>

#include <ccae/kernel/fwd.h>
#include <ccae/types.h>

typedef enum : u8 {
	PRINT_ASCII,
	PRINT_LATEX,
} PrintMode;

/* Writes `sym` into `buf` (`size` bytes, truncated, '\0'-terminated): name
 * and slots, kind, flags. The k-th slot of a bundle gets its k-th index name.
 * `PRINT_LATEX` uses the LaTeX rendering (or the name if unset) and braced
 * slots. Dies on a bad handle.
 *
 * Example:
 *		`print_symbol(ctx, dK, PRINT_ASCII, buf, sizeof(buf));` gives
 *		"dK^i_a field PHASE_SPACE|ZERO_MODE_FREE";
 *		with `PRINT_LATEX`, "\delta K^{i}{}_{a} field PHASE_SPACE|…". */
void print_symbol(const Context* ctx, SymRef sym, PrintMode mode, char* buf,
				  size_t size);

/* Writes every symbol into `buf`, one `print_symbol` line each, ending with
 * '\n', in declaration order. Same truncation as `print_symbol`.
 *
 * Example:
 *		`print_symbol_table(ctx, PRINT_ASCII, buf, sizeof(buf));` */
void print_symbol_table(const Context* ctx, PrintMode mode, char* buf,
						size_t size);

#endif

#ifndef CCAE_PRINT_H
#define CCAE_PRINT_H

#include <stddef.h>

#include <ccae/core/rational.h>
#include <ccae/kernel/fwd.h>
#include <ccae/types.h>

typedef enum : u8 {
	PRINT_ASCII,
	PRINT_LATEX,
} PrintMode;

/* Writes `sym` into `out` (`capacity` bytes, truncated, '\0'-terminated): name
 * and slots, kind, flags. The k-th slot of a bundle gets its k-th index name.
 * `PRINT_LATEX` uses the LaTeX rendering (or the name if unset) and braced
 * slots. Dies on a bad handle.
 *
 * Example:
 *		`print_symbol(buf, sizeof(buf), ctx, dK, PRINT_ASCII);` gives
 *		"dK^i_a field PHASE_SPACE|ZERO_MODE_FREE";
 *		with `PRINT_LATEX`, "\delta K^{i}{}_{a} field PHASE_SPACE|…". */
void print_symbol(char* out, size_t capacity, const Context* ctx, SymRef sym,
				  PrintMode mode);

/* Writes every symbol into `out`, one `print_symbol` line each, ending with
 * '\n', in declaration order. Same truncation as `print_symbol`.
 *
 * Example:
 *		`print_symbol_table(buf, sizeof(buf), ctx, PRINT_ASCII);` */
void print_symbol_table(char* out, size_t capacity, const Context* ctx,
						PrintMode mode);

/* Writes `r` into `out`, same truncation as `print_symbol`. Integers drop
 * the denominator; `PRINT_LATEX` writes the others with `\frac`.
 *
 * Example:
 *		`print_rational(buf, sizeof(buf), rat_new(-1, 2), PRINT_ASCII);` gives
 *		"-1/2"; with `PRINT_LATEX`, "-\frac{1}{2}". */
void print_rational(char* out, size_t capacity, Rational r, PrintMode mode);

#endif

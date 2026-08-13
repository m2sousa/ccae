#ifndef CCAE_SEXPR_H
#define CCAE_SEXPR_H

#include <stddef.h>

#include <ccae/kernel/fwd.h>

/* Writes `e` as an s-expression into `out`.
 * Dies on a bad handle.
 *
 * Example:
 *		`print_sexpr(buf, sizeof(buf), ctx, e);` gives
 *		"(+ (* 2 x) (^ p -1/2))" */
void print_sexpr(char* out, size_t capacity, const Context* ctx, ExprRef e);

/* Returns the handle of the expression written in `text`, in the form
 * of an s-expression.
 * Sets `ctx->err` and returns the null handle on bad input.
 *
 * Example:
 *		`ExprRef e = read_sexpr(ctx, "(* (+ x y) (+ x y))");`
 *		`ExprRef e = read_sexpr(ctx, "(* dE^a_i dK^i_b)");` */
[[nodiscard]] ExprRef read_sexpr(Context* ctx, const char* text);

#endif

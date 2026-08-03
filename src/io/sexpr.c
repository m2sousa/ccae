#include <stddef.h>

#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/io/print.h>
#include <ccae/io/print_buffer.h>
#include <ccae/io/sexpr.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/expr.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

static void put_rational(PrintBuffer* out, Rational r) {
	constexpr size_t RATIONAL_LENGTH = 48;
	char text[RATIONAL_LENGTH];

	print_rational(r, PRINT_ASCII, text, sizeof(text));
	put_string(out, text);
}

static void put_factor_as_sexpr(PrintBuffer* out, const Context* ctx,
								FacRef f) {
	Factor factor = vec_at(ctx->factors, f.val);
	const Symbol* head = &vec_at(ctx->symbols, factor.head.val);
	const char* name = sp_get(ctx->sp, head->name);

	if (rat_is_one(factor.power)) {
		put_string(out, name);
		return;
	}

	put_string(out, "(^ ");
	put_string(out, name);
	put_char(out, ' ');
	put_rational(out, factor.power);
	put_char(out, ')');
}

static void put_monomial_as_sexpr(PrintBuffer* out, const Context* ctx,
								  const Monomial* mono) {
	for (size_t i = 0; i < mono->nbr_fac; ++i) {
		put_char(out, ' ');
		put_factor_as_sexpr(out, ctx, mono->fac[i]);
	}
}

static void put_term_as_sexpr(PrintBuffer* out, const Context* ctx, Term term) {
	const Monomial* mono = &vec_at(ctx->monomials, term.mono.val);

	if (mono->nbr_fac == 0) {
		put_rational(out, term.coef);
		return;
	}

	bool unit_coef = rat_is_one(term.coef);

	if (unit_coef && mono->nbr_fac == 1) {
		put_factor_as_sexpr(out, ctx, mono->fac[0]);
		return;
	}

	put_string(out, "(*");

	if (!unit_coef) {
		put_char(out, ' ');
		put_rational(out, term.coef);
	}

	put_monomial_as_sexpr(out, ctx, mono);
	put_char(out, ')');
}

void print_sexpr(const Context* ctx, ExprRef e, char* buf, size_t size) {
	if (e.val == 0 || e.val >= ctx->exprs.length)
		die("CCAE (print_sexpr): expression handle out of range.");

	if (size == 0)
		return;

	buf[0] = '\0';

	PrintBuffer out = {.buf = buf, .size = size, .length = 0};
	Expr expr = vec_at(ctx->exprs, e.val);

	if (expr.nbr_term == 0) {
		put_char(&out, '0');
		return;
	}

	if (expr.nbr_term == 1) {
		put_term_as_sexpr(&out, ctx, expr.terms[0]);
		return;
	}

	put_string(&out, "(+");

	for (size_t i = 0; i < expr.nbr_term; ++i) {
		put_char(&out, ' ');
		put_term_as_sexpr(&out, ctx, expr.terms[i]);
	}

	put_char(&out, ')');
}

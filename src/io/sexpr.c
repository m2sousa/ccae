#include <stdckdint.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <ccae/config.h>
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
#include <ccae/kernel/index.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>
#include <ccae/utils.h>

static void put_rational(PrintBuffer* out, Rational r) {
	constexpr size_t RATIONAL_LENGTH = 48;
	char text[RATIONAL_LENGTH];

	print_rational(r, PRINT_ASCII, text, sizeof(text));
	put_string(out, text);
}

static void put_indices_as_sexpr(PrintBuffer* out, const Symbol* symbol,
								 const Index* indices) {
	for (u8 k = 0; k < symbol->nbr_slot; ++k) {
		put_char(out, symbol->slot_pos[k] == SLOT_UP ? '^' : '_');

		if (indices[k].kind == IDX_FREE) {
			put_char(out, (char)indices[k].id);
			continue;
		}

		put_char(out, '$');
		put_u64(out, indices[k].id);
	}
}

static void put_factor_as_sexpr(PrintBuffer* out, const Context* ctx,
								FacRef f) {
	Factor factor = vec_at(ctx->factors, f.val);
	const Symbol* symbol = &vec_at(ctx->symbols, factor.head.val);
	const char* name = sp_get(ctx->sp, symbol->name);

	if (rat_is_one(factor.exponent)) {
		put_string(out, name);
		put_indices_as_sexpr(out, symbol, factor.indices);
		return;
	}

	put_string(out, "(^ ");
	put_string(out, name);
	put_indices_as_sexpr(out, symbol, factor.indices);
	put_char(out, ' ');
	put_rational(out, factor.exponent);
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

typedef struct {
	Context* ctx;
	const char* pos;
} Reader;

static ExprRef read_expr(Reader* r);

static bool ends_token(char c) {
	return c == '\0' || c == '(' || c == ')' || is_space(c);
}

static void skip_spaces(Reader* r) {
	while (is_space(*r->pos))
		++r->pos;
}

// This function has a side effect: moves the position of the reader while
// getting the length of the token.
static size_t token_length(const char** out, Reader* r) {
	*out = r->pos;

	while (!ends_token(*r->pos))
		r->pos += 1;

	return (size_t)(r->pos - *out);
}

static bool looks_like_number(const char* token, size_t length) {
	if (length == 0)
		return false;

	if (token[0] == '-')
		return length > 1 && is_digit(token[1]);

	return is_digit(token[0]);
}

static bool read_digits(i64* out, const char** s, const char* end) {
	if (*s == end || !is_digit(**s))
		return false;

	*out = 0;

	for (; *s < end && is_digit(**s); ++*s) {
		if (ckd_mul(out, *out, 10) || ckd_add(out, *out, **s - '0'))
			return false;
	}

	return true;
}

static bool read_number(Rational* out, const char* token, size_t length) {
	const char* s = token;
	const char* end = token + length;
	bool negative = false;

	if (*s == '-') {
		negative = true;
		++s;
	}

	i64 num = 0;
	i64 den = 1;

	if (!read_digits(&num, &s, end))
		return false;

	if (s < end && *s == '/') {
		++s;

		if (!read_digits(&den, &s, end) || den == 0)
			return false;
	}

	if (s != end)
		return false;

	*out = rat_new(negative ? -num : num, den);

	return true;
}

static SymRef find_symbol(const Context* ctx, const char* name, size_t length) {
	for (size_t i = 1; i < ctx->symbols.length; ++i) {
		const char* candidate = sp_get(ctx->sp, vec_at(ctx->symbols, i).name);

		if (strlen(candidate) == length &&
			memcmp(candidate, name, length) == 0) {
			SymRef found = {.val = (u32)i};
			return found;
		}
	}

	SymRef null_ref = {.val = 0};

	return null_ref;
}

static bool reject_indices(Context* ctx, const Symbol* symbol, const char* text,
						   size_t length) {
	err_set(&ctx->err, CCAE_INVALID_ARG,
			"s-expression: bad indices '%.*s' for '%s'.", (int)length, text,
			sp_get(ctx->sp, symbol->name));

	return false;
}

static bool read_index(Index* out, const char** s, const char* end) {
	if (*s == end)
		return false;

	if (**s == '$') {
		++*s;

		i64 id = 0;

		if (!read_digits(&id, s, end) || id > UINT16_MAX)
			return false;

		Index dummy = {.kind = IDX_DUMMY, .id = (u16)id};
		*out = dummy;

		return true;
	}

	char name = **s;

	if (!is_letter(name))
		return false;

	Index index = {.kind = IDX_FREE, .id = (u16)name};
	*out = index;
	++*s;

	return true;
}

static bool read_indices(Index* out, Context* ctx, const Symbol* symbol,
						 const char* text, size_t length) {
	const char* s = text;
	const char* end = text + length;

	for (u8 k = 0; k < symbol->nbr_slot; ++k) {
		char wanted_pos = symbol->slot_pos[k] == SLOT_UP ? '^' : '_';

		if (s == end || *s != wanted_pos)
			return reject_indices(ctx, symbol, text, length);

		++s;

		if (!read_index(&out[k], &s, end))
			return reject_indices(ctx, symbol, text, length);
	}

	if (s != end)
		return reject_indices(ctx, symbol, text, length);

	return true;
}

static FacRef read_factor(Reader* r, const char* token, size_t length,
						  Rational exponent) {
	FacRef null_ref = {.val = 0};

	size_t name_length = 0;

	while (name_length < length && token[name_length] != '^' &&
		   token[name_length] != '_')
		++name_length;

	SymRef sym = find_symbol(r->ctx, token, name_length);

	if (sym.val == 0) {
		err_set(&r->ctx->err, CCAE_INVALID_ARG,
				"s-expression: unknown symbol '%.*s'.", (int)name_length,
				token);
		return null_ref;
	}

	Symbol symbol = vec_at(r->ctx->symbols, sym.val);
	Index indices[CCAE_MAX_SLOT];

	if (!read_indices(indices, r->ctx, &symbol, token + name_length,
					  length - name_length))
		return null_ref;

	return factor_intern(r->ctx, sym, exponent,
						 symbol.nbr_slot > 0 ? indices : NULL);
}

static ExprRef build_expr_from_rational(Context* ctx, Rational coef) {
	MonoRef one = mono_intern(ctx, NULL, 0);

	return expr_intern(ctx, (Term[]){{coef, one}}, 1);
}

static ExprRef build_expr_from_factors(Context* ctx, const FacRef* facs,
									   size_t nbr_fac) {
	MonoRef mono = mono_intern(ctx, facs, nbr_fac);

	return expr_intern(ctx, (Term[]){{rat_from_int(1), mono}}, 1);
}

static ExprRef build_expr_from_factor(Context* ctx, FacRef f) {
	ExprRef null_ref = {.val = 0};

	if (f.val == 0)
		return null_ref;

	return build_expr_from_factors(ctx, &f, 1);
}

static ExprRef read_atom(Reader* r) {
	ExprRef null_ref = {.val = 0};

	const char* token = NULL;
	size_t length = token_length(&token, r);

	if (looks_like_number(token, length)) {
		Rational value;

		if (!read_number(&value, token, length)) {
			err_set(&r->ctx->err, CCAE_INVALID_ARG,
					"s-expression: bad number '%.*s'.", (int)length, token);
			return null_ref;
		}

		return build_expr_from_rational(r->ctx, value);
	}

	return build_expr_from_factor(
		r->ctx, read_factor(r, token, length, rat_from_int(1)));
}

static FacRef read_factor_with_exponent(Reader* r) {
	FacRef null_ref = {.val = 0};

	skip_spaces(r);

	const char* base = NULL;
	size_t base_length = token_length(&base, r);

	if (base_length == 0) {
		err_set(&r->ctx->err, CCAE_INVALID_ARG,
				"s-expression: '^' needs a symbol.");
		return null_ref;
	}

	skip_spaces(r);

	const char* exponent_text = NULL;
	size_t exponent_length = token_length(&exponent_text, r);
	Rational exponent;

	if (!looks_like_number(exponent_text, exponent_length) ||
		!read_number(&exponent, exponent_text, exponent_length)) {
		err_set(&r->ctx->err, CCAE_INVALID_ARG,
				"s-expression: '^' needs a number, got '%.*s'.",
				(int)exponent_length, exponent_text);
		return null_ref;
	}

	skip_spaces(r);

	if (*r->pos != ')') {
		err_set(&r->ctx->err, CCAE_INVALID_ARG,
				"s-expression: missing ')' after '^'.");
		return null_ref;
	}

	++r->pos;

	return read_factor(r, base, base_length, exponent);
}

static bool next_is_factor(const Reader* r) {
	Reader peek = *r;
	bool is_list = *peek.pos == '(';

	if (is_list) {
		peek.pos += 1;
		skip_spaces(&peek);
	}

	const char* token = NULL;
	size_t length = token_length(&token, &peek);

	if (is_list)
		return length == 1 && *token == '^';

	return !looks_like_number(token, length);
}

static FacRef read_factor_operand(Reader* r) {
	if (*r->pos == '(') {
		r->pos += 1;
		skip_spaces(r);
		r->pos += 1;

		return read_factor_with_exponent(r);
	}

	const char* token = NULL;
	size_t length = token_length(&token, r);

	return read_factor(r, token, length, rat_from_int(1));
}

static ExprRef read_operands(Reader* r, char op) {
	ExprRef null_ref = {.val = 0};

	// Start to zero for sums, one for mult.
	ExprRef result = op == '+'
						 ? expr_intern(r->ctx, NULL, 0)
						 : build_expr_from_rational(r->ctx, rat_from_int(1));

	FacRef facs[CCAE_MAX_FACTORS];
	size_t nbr_fac = 0;

	while (true) {
		skip_spaces(r);

		if (*r->pos == ')') {
			r->pos += 1;

			if (op == '*')
				result =
					expr_mul(r->ctx, result,
							 build_expr_from_factors(r->ctx, facs, nbr_fac));

			return result;
		}

		if (*r->pos == '\0') {
			err_set(&r->ctx->err, CCAE_INVALID_ARG,
					"s-expression: missing ')'.");
			return null_ref;
		}

		if (op == '*' && next_is_factor(r)) {
			if (nbr_fac == CCAE_MAX_FACTORS) {
				err_set(&r->ctx->err, CCAE_INVALID_ARG,
						"s-expression: more than %zu factors.",
						CCAE_MAX_FACTORS);
				return null_ref;
			}

			facs[nbr_fac] = read_factor_operand(r);
			if (facs[nbr_fac].val == 0)
				return null_ref;

			++nbr_fac;
			continue;
		}

		ExprRef operand = read_expr(r);
		if (operand.val == 0)
			return null_ref;

		result = op == '+' ? expr_add(r->ctx, result, operand)
						   : expr_mul(r->ctx, result, operand);
		if (result.val == 0)
			return null_ref;
	}
}

static ExprRef read_list(Reader* r) {
	ExprRef null_ref = {.val = 0};

	r->pos += 1;
	skip_spaces(r);

	const char* op = NULL;
	size_t length = token_length(&op, r);

	if (length == 1 && (*op == '+' || *op == '*'))
		return read_operands(r, *op);

	if (length == 1 && *op == '^')
		return build_expr_from_factor(r->ctx, read_factor_with_exponent(r));

	err_set(&r->ctx->err, CCAE_INVALID_ARG,
			"s-expression: unknown operator '%.*s'.", (int)length, op);

	return null_ref;
}

static ExprRef read_expr(Reader* r) {
	ExprRef null_ref = {.val = 0};

	skip_spaces(r);

	if (*r->pos == '(')
		return read_list(r);

	if (*r->pos == '\0' || *r->pos == ')') {
		err_set(&r->ctx->err, CCAE_INVALID_ARG,
				"s-expression: expected an expression.");
		return null_ref;
	}

	return read_atom(r);
}

ExprRef read_sexpr(Context* ctx, const char* text) {
	ExprRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	Reader r = {.ctx = ctx, .pos = text};
	ExprRef e = read_expr(&r);

	if (e.val == 0)
		return null_ref;

	skip_spaces(&r);

	if (*r.pos != '\0') {
		err_set(&ctx->err, CCAE_INVALID_ARG,
				"s-expression: unexpected text after the expression.");
		return null_ref;
	}

	return e;
}

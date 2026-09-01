#include <stddef.h>

#include <ccae/calc/derivative.h>
#include <ccae/core/rational.h>
#include <ccae/io/sexpr.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/expr.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/symbol.h>

#include "t_utils.h"

static size_t test_derivative_commute(void);
static size_t test_derivative_divergence(void);
static size_t test_derivative_leibniz(void);
static size_t test_derivative_multinomial(void);
static size_t test_derivative_refused(void);

size_t test_derivative(void) {
	size_t fails = 0;
	fails += test_derivative_commute();
	fails += test_derivative_divergence();
	fails += test_derivative_leibniz();
	fails += test_derivative_multinomial();
	fails += test_derivative_refused();

	return fails;
}

static Context* create_ctx_with_fields(void) {
	Context* ctx = ctx_create();
	if (!ctx)
		return NULL;

	BundleRef spatial = bundle_declare(ctx, "spatial", "abcdefgh", 3);
	bundle_set_derivative(ctx, spatial);

	const char* scalars[] = {"x", "y", "z", "w", "X"};
	for (size_t i = 0; i < 5; ++i) {
		SymRef scalar =
			symbol_declare_tensor(ctx, scalars[i], SYM_FIELD, "", "", 0);
		if (scalar.val == 0)
			break;
	}

	SymRef T = symbol_declare_tensor(ctx, "T", SYM_FIELD, "^a^b", "", 0);
	SymRef p = symbol_declare_homog(ctx, "p", 0);

	if (T.val == 0 || p.val == 0 || !ctx_ok(ctx)) {
		ctx_destroy(ctx);
		return NULL;
	}

	return ctx;
}

static ExprRef derive(Context* ctx, ExprRef e, char a) {
	return deriv_expr(ctx, e, (Index){IDX_FREE, (u16)a});
}

// Whether `∂_a` of the s-expression `text` is the s-expression `expected`.
static bool derives_to(Context* ctx, const char* text, char a,
					   const char* expected) {
	ExprRef result = derive(ctx, read_sexpr(ctx, text), a);

	return result.val != 0 && result.val == read_sexpr(ctx, expected).val;
}

static size_t test_derivative_commute(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_fields();
	if (!ctx)
		return 1;

	ExprRef X = read_sexpr(ctx, "X");
	ExprRef ab = derive(ctx, derive(ctx, X, 'b'), 'a');
	ExprRef ba = derive(ctx, derive(ctx, X, 'a'), 'b');
	CHECK(ab.val != 0 && ab.val == ba.val);
	CHECK(expr_add(ctx, ab, expr_neg(ctx, ba)).val ==
		  expr_intern(ctx, NULL, 0).val);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_derivative_divergence(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_fields();
	if (!ctx)
		return 1;

	CHECK(derives_to(ctx, "T^a^b", 'a', "T^$0^b,$0"));
	CHECK(derives_to(ctx, "T^a^c", 'a', "T^$0^c,$0"));

	CHECK(derives_to(ctx, "T^a^b", 'c', "T^a^b,c"));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_derivative_leibniz(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_fields();
	if (!ctx)
		return 1;

	CHECK(
		derives_to(ctx, "(* x T^b^c)", 'a', "(+ (* x,a T^b^c) (* x T^b^c,a))"));

	CHECK(derives_to(ctx, "(^ x 3)", 'a', "(* 3 (^ x 2) x,a)"));

	CHECK(derives_to(ctx, "p", 'a', "0"));
	CHECK(derives_to(ctx, "(* p x)", 'a', "(* p x,a)"));

	CHECK(derives_to(ctx, "(+ x (* 2 y))", 'a', "(+ x,a (* 2 y,a))"));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_derivative_multinomial(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_fields();
	if (!ctx)
		return 1;

	// ∂_a ∂_b ∂_c (x y z w): each of the three derivatives lands on one of the
	// four factors, 4³ = 64 distinct terms.
	ExprRef product = read_sexpr(ctx, "(* x y z w)");
	ExprRef third =
		derive(ctx, derive(ctx, derive(ctx, product, 'c'), 'b'), 'a');
	CHECK(third.val != 0);
	CHECK(vec_at(ctx->exprs, third.val).nbr_term == 64);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_derivative_refused(void) {
	size_t fails = 0;

	Context* ctx = create_ctx_with_fields();
	if (!ctx)
		return 1;

	ExprRef x = read_sexpr(ctx, "x");

	// A dummy as the derivative index.
	CHECK(deriv_expr(ctx, x, (Index){IDX_DUMMY, 0}).val == 0);
	ctx_clear_err(ctx);

	// ∂_b ∂_b x: the index `b` twice down is refused.
	ExprRef x_b = derive(ctx, x, 'b');
	CHECK(x_b.val != 0);
	CHECK(derive(ctx, x_b, 'b').val == 0);
	ctx_clear_err(ctx);

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

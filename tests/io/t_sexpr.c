#include <stddef.h>
#include <string.h>

#include <ccae/core/rational.h>
#include <ccae/io/sexpr.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/expr.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

#include "t_utils.h"

static size_t test_sexpr_print_single(void);
static size_t test_sexpr_print_products(void);
static size_t test_sexpr_print_sums(void);
static size_t test_sexpr_print_truncate(void);

size_t test_sexpr(void) {
	size_t fails = 0;
	fails += test_sexpr_print_single();
	fails += test_sexpr_print_products();
	fails += test_sexpr_print_sums();
	fails += test_sexpr_print_truncate();

	return fails;
}

// Helper function to avoid those strcmp everywhere.
static bool prints_as(const Context* ctx, ExprRef e, const char* expected) {
	char buf[128];
	print_sexpr(ctx, e, buf, sizeof(buf));

	return strcmp(buf, expected) == 0;
}

static size_t test_sexpr_print_single(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1));

	MonoRef mono_one = mono_intern(ctx, NULL, 0);
	MonoRef mono_x = mono_intern(ctx, (FacRef[]){x_one}, 1);

	ExprRef zero = expr_intern(ctx, NULL, 0);
	ExprRef one = expr_intern(ctx, (Term[]){{rat_from_int(1), mono_one}}, 1);
	ExprRef minus_half =
		expr_intern(ctx, (Term[]){{rat_new(-1, 2), mono_one}}, 1);
	ExprRef just_x = expr_intern(ctx, (Term[]){{rat_from_int(1), mono_x}}, 1);

	CHECK(prints_as(ctx, zero, "0"));
	CHECK(prints_as(ctx, one, "1"));
	CHECK(prints_as(ctx, minus_half, "-1/2"));
	CHECK(prints_as(ctx, just_x, "x"));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_sexpr_print_products(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	SymRef p = symbol_declare_homog(ctx, "p", F_POSITIVE | F_NONZERO);
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1));
	FacRef p_minus_half = factor_intern(ctx, p, rat_new(-1, 2));
	FacRef p_minus_three_halves = factor_intern(ctx, p, rat_new(-3, 2));

	MonoRef mono_x = mono_intern(ctx, (FacRef[]){x_one}, 1);
	MonoRef mono_p = mono_intern(ctx, (FacRef[]){p_minus_half}, 1);
	MonoRef mono_x_p =
		mono_intern(ctx, (FacRef[]){p_minus_three_halves, x_one}, 2);

	ExprRef minus_x = expr_intern(ctx, (Term[]){{rat_from_int(-1), mono_x}}, 1);
	ExprRef root = expr_intern(ctx, (Term[]){{rat_from_int(1), mono_p}}, 1);
	ExprRef half_x_p = expr_intern(ctx, (Term[]){{rat_new(1, 2), mono_x_p}}, 1);

	CHECK(prints_as(ctx, minus_x, "(* -1 x)"));
	CHECK(prints_as(ctx, root, "(^ p -1/2)"));
	CHECK(prints_as(ctx, half_x_p, "(* 1/2 x (^ p -3/2))"));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_sexpr_print_sums(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	SymRef y = symbol_declare_homog(ctx, "y", 0);
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1));
	FacRef y_one = factor_intern(ctx, y, rat_from_int(1));

	MonoRef mono_one = mono_intern(ctx, NULL, 0);
	MonoRef mono_x = mono_intern(ctx, (FacRef[]){x_one}, 1);
	MonoRef mono_y = mono_intern(ctx, (FacRef[]){y_one}, 1);

	Rational one = rat_from_int(1);
	Rational two = rat_from_int(2);

	ExprRef two_x_plus_y =
		expr_intern(ctx, (Term[]){{two, mono_x}, {one, mono_y}}, 2);
	ExprRef x_plus_one =
		expr_intern(ctx, (Term[]){{one, mono_x}, {one, mono_one}}, 2);

	CHECK(prints_as(ctx, two_x_plus_y, "(+ (* 2 x) y)"));
	CHECK(prints_as(ctx, x_plus_one, "(+ 1 x)"));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_sexpr_print_truncate(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	SymRef y = symbol_declare_homog(ctx, "y", 0);
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1));
	FacRef y_one = factor_intern(ctx, y, rat_from_int(1));

	MonoRef mono_x = mono_intern(ctx, (FacRef[]){x_one}, 1);
	MonoRef mono_y = mono_intern(ctx, (FacRef[]){y_one}, 1);

	Rational one = rat_from_int(1);
	ExprRef x_plus_y =
		expr_intern(ctx, (Term[]){{one, mono_x}, {one, mono_y}}, 2);

	char buf[5];
	print_sexpr(ctx, x_plus_y, buf, sizeof(buf));
	CHECK(strcmp(buf, "(+ x") == 0);

	ctx_destroy(ctx);

	return fails;
}

#include <stddef.h>

#include <ccae/core/rational.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/expr.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

#include "t_utils.h"

static size_t test_expr_handles(void);
static size_t test_expr_collect(void);
static size_t test_expr_add(void);

size_t test_expr(void) {
	size_t fails = 0;
	fails += test_expr_handles();
	fails += test_expr_collect();
	fails += test_expr_add();

	return fails;
}

static size_t test_expr_handles(void) {
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
	ExprRef just_x = expr_intern(ctx, (Term[]){{rat_from_int(1), mono_x}}, 1);

	CHECK(zero.val != 0 && one.val != 0 && just_x.val != 0);

	CHECK(zero.val != one.val);
	CHECK(zero.val != just_x.val);
	CHECK(one.val != just_x.val);

	CHECK(expr_intern(ctx, NULL, 0).val == zero.val);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_expr_collect(void) {
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
	Rational half = rat_new(1, 2);
	Rational minus_one = rat_from_int(-1);

	ExprRef x_plus_y =
		expr_intern(ctx, (Term[]){{one, mono_x}, {one, mono_y}}, 2);
	ExprRef y_plus_x =
		expr_intern(ctx, (Term[]){{one, mono_y}, {one, mono_x}}, 2);
	CHECK(x_plus_y.val == y_plus_x.val);

	ExprRef halves =
		expr_intern(ctx, (Term[]){{half, mono_x}, {half, mono_x}}, 2);
	CHECK(halves.val == expr_intern(ctx, (Term[]){{one, mono_x}}, 1).val);

	ExprRef cancel =
		expr_intern(ctx, (Term[]){{one, mono_x}, {minus_one, mono_x}}, 2);
	CHECK(cancel.val == expr_intern(ctx, NULL, 0).val);

	Rational third = rat_new(1, 3);

	ExprRef thirds = expr_intern(
		ctx,
		(Term[]){
			{third, mono_x}, {one, mono_y}, {third, mono_x}, {third, mono_x}},
		4);
	CHECK(thirds.val == x_plus_y.val);

	ExprRef four_cancel = expr_intern(ctx,
									  (Term[]){{one, mono_x},
											   {one, mono_x},
											   {minus_one, mono_x},
											   {minus_one, mono_x}},
									  4);
	CHECK(four_cancel.val == expr_intern(ctx, NULL, 0).val);

	ctx_destroy(ctx);

	return fails;
}

static size_t test_expr_add(void) {
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
	Rational half = rat_new(1, 2);
	Rational minus_one = rat_from_int(-1);

	ExprRef zero = expr_intern(ctx, NULL, 0);
	ExprRef just_x = expr_intern(ctx, (Term[]){{one, mono_x}}, 1);
	ExprRef just_y = expr_intern(ctx, (Term[]){{one, mono_y}}, 1);
	ExprRef half_x = expr_intern(ctx, (Term[]){{half, mono_x}}, 1);
	ExprRef minus_x = expr_intern(ctx, (Term[]){{minus_one, mono_x}}, 1);

	ExprRef x_plus_y =
		expr_intern(ctx, (Term[]){{one, mono_x}, {one, mono_y}}, 2);

	CHECK(expr_add(ctx, just_x, just_y).val == x_plus_y.val);
	CHECK(expr_add(ctx, just_y, just_x).val == x_plus_y.val);
	CHECK(expr_add(ctx, half_x, half_x).val == just_x.val);
	CHECK(expr_add(ctx, just_x, minus_x).val == zero.val);
	CHECK(expr_add(ctx, zero, just_x).val == just_x.val);
	CHECK(expr_add(ctx, zero, zero).val == zero.val);

	ctx_destroy(ctx);

	return fails;
}

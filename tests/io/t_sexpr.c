#include <stddef.h>
#include <string.h>

#include <ccae/core/rational.h>
#include <ccae/io/sexpr.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/expr.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

#include "t_kernel_utils.h"
#include "t_utils.h"

static size_t test_sexpr_print_single(void);
static size_t test_sexpr_print_products(void);
static size_t test_sexpr_print_sums(void);
static size_t test_sexpr_print_truncate(void);
static size_t test_sexpr_read_atoms(void);
static size_t test_sexpr_read_collect(void);
static size_t test_sexpr_round_trip(void);
static size_t test_sexpr_read_errors(void);
static size_t test_sexpr_print_indices(void);
static size_t test_sexpr_read_indices(void);
static size_t test_sexpr_bad_indices(void);
static size_t test_sexpr_read_dummies(void);
static size_t test_sexpr_bad_dummies(void);

size_t test_sexpr(void) {
	size_t fails = 0;
	fails += test_sexpr_print_single();
	fails += test_sexpr_print_products();
	fails += test_sexpr_print_sums();
	fails += test_sexpr_print_truncate();
	fails += test_sexpr_read_atoms();
	fails += test_sexpr_read_collect();
	fails += test_sexpr_round_trip();
	fails += test_sexpr_read_errors();
	fails += test_sexpr_print_indices();
	fails += test_sexpr_read_indices();
	fails += test_sexpr_bad_indices();
	fails += test_sexpr_read_dummies();
	fails += test_sexpr_bad_dummies();

	return fails;
}

// Helper function to avoid those strcmp everywhere.
static bool prints_as(const Context* ctx, ExprRef e, const char* expected) {
	char buf[128];
	print_sexpr(buf, sizeof(buf), ctx, e);

	return strcmp(buf, expected) == 0;
}

static size_t test_sexpr_print_single(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);

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
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef p_minus_half = factor_intern(ctx, p, rat_new(-1, 2), NULL);
	FacRef p_minus_three_halves = factor_intern(ctx, p, rat_new(-3, 2), NULL);

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
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef y_one = factor_intern(ctx, y, rat_from_int(1), NULL);

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
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef y_one = factor_intern(ctx, y, rat_from_int(1), NULL);

	MonoRef mono_x = mono_intern(ctx, (FacRef[]){x_one}, 1);
	MonoRef mono_y = mono_intern(ctx, (FacRef[]){y_one}, 1);

	Rational one = rat_from_int(1);
	ExprRef x_plus_y =
		expr_intern(ctx, (Term[]){{one, mono_x}, {one, mono_y}}, 2);

	char buf[5];
	print_sexpr(buf, sizeof(buf), ctx, x_plus_y);
	CHECK(strcmp(buf, "(+ x") == 0);

	ctx_destroy(ctx);

	return fails;
}

// Tells whether `text` reads back as the same handle as `expected`.
static bool reads_as(Context* ctx, const char* text, ExprRef expected) {
	ExprRef e = read_sexpr(ctx, text);

	return e.val != 0 && e.val == expected.val;
}

// Tells whether `text` is exactly the output of printing as s-expr.
static bool round_trips(Context* ctx, const char* text) {
	ExprRef e = read_sexpr(ctx, text);

	return e.val != 0 && prints_as(ctx, e, text);
}

// Tells whether reading `text` fails with an error, then clears it.
static bool fails_to_read(Context* ctx, const char* text) {
	ExprRef e = read_sexpr(ctx, text);
	bool failed = e.val == 0 && !ctx_ok(ctx);

	ctx_clear_err(ctx);

	return failed;
}

static size_t test_sexpr_read_atoms(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	SymRef p = symbol_declare_homog(ctx, "p", F_POSITIVE | F_NONZERO);
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef p_minus_half = factor_intern(ctx, p, rat_new(-1, 2), NULL);

	MonoRef mono_one = mono_intern(ctx, NULL, 0);
	MonoRef mono_x = mono_intern(ctx, (FacRef[]){x_one}, 1);
	MonoRef mono_p = mono_intern(ctx, (FacRef[]){p_minus_half}, 1);

	ExprRef zero = expr_intern(ctx, NULL, 0);
	ExprRef minus_half =
		expr_intern(ctx, (Term[]){{rat_new(-1, 2), mono_one}}, 1);
	ExprRef just_x = expr_intern(ctx, (Term[]){{rat_from_int(1), mono_x}}, 1);
	ExprRef root = expr_intern(ctx, (Term[]){{rat_from_int(1), mono_p}}, 1);

	CHECK(reads_as(ctx, "0", zero));
	CHECK(reads_as(ctx, "-1/2", minus_half));
	CHECK(reads_as(ctx, "-2/4", minus_half));
	CHECK(reads_as(ctx, "x", just_x));
	CHECK(reads_as(ctx, "(^ p -1/2)", root));
	CHECK(reads_as(ctx, "  (^\tp\n-1/2 )  ", root));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_sexpr_read_collect(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	SymRef y = symbol_declare_homog(ctx, "y", 0);
	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef y_one = factor_intern(ctx, y, rat_from_int(1), NULL);

	MonoRef mono_x = mono_intern(ctx, (FacRef[]){x_one}, 1);
	MonoRef mono_y = mono_intern(ctx, (FacRef[]){y_one}, 1);

	Rational one = rat_from_int(1);
	Rational two = rat_from_int(2);

	ExprRef zero = expr_intern(ctx, NULL, 0);
	ExprRef x_plus_y =
		expr_intern(ctx, (Term[]){{one, mono_x}, {one, mono_y}}, 2);
	ExprRef two_x_plus_y =
		expr_intern(ctx, (Term[]){{two, mono_x}, {one, mono_y}}, 2);
	ExprRef square = expr_mul(ctx, x_plus_y, x_plus_y);

	CHECK(reads_as(ctx, "(+ x y x)", two_x_plus_y));
	CHECK(reads_as(ctx, "(+ x (* -1 x))", zero));
	CHECK(reads_as(ctx, "(* (+ x y) (+ x y))", square));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_sexpr_round_trip(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	CHECK(symbol_declare_homog(ctx, "x", 0).val != 0);
	CHECK(symbol_declare_homog(ctx, "y", 0).val != 0);
	CHECK(symbol_declare_homog(ctx, "p", F_POSITIVE | F_NONZERO).val != 0);

	CHECK(round_trips(ctx, "0"));
	CHECK(round_trips(ctx, "-1/2"));
	CHECK(round_trips(ctx, "x"));
	CHECK(round_trips(ctx, "(* -1 x)"));
	CHECK(round_trips(ctx, "(^ p -1/2)"));
	CHECK(round_trips(ctx, "(* 1/2 x (^ p -3/2))"));
	CHECK(round_trips(ctx, "(+ (* 2 x) y)"));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_sexpr_read_errors(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	CHECK(symbol_declare_homog(ctx, "x", 0).val != 0);
	CHECK(symbol_declare_homog(ctx, "y", 0).val != 0);

	CHECK(fails_to_read(ctx, ""));
	CHECK(fails_to_read(ctx, ")"));
	CHECK(fails_to_read(ctx, "z"));
	CHECK(fails_to_read(ctx, "x y"));
	CHECK(fails_to_read(ctx, "(+ x"));
	CHECK(fails_to_read(ctx, "()"));
	CHECK(fails_to_read(ctx, "(- x y)"));
	CHECK(fails_to_read(ctx, "1/0"));
	CHECK(fails_to_read(ctx, "99999999999999999999"));
	CHECK(fails_to_read(ctx, "(^ (+ x y) 2)"));
	CHECK(fails_to_read(ctx, "(^ x y)"));
	CHECK(fails_to_read(ctx, "(^ x 2 3)"));

	// `x` is not F_POSITIVE: must be rejected (cannot take square root),
	// therefore no handle should exist.
	CHECK(fails_to_read(ctx, "(^ x 1/2)"));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static ExprRef intern_product(Context* ctx, const FacRef* facs,
							  size_t nbr_fac) {
	MonoRef mono = mono_intern(ctx, facs, nbr_fac);

	return expr_intern(ctx, (Term[]){{rat_from_int(1), mono}}, 1);
}

static size_t test_sexpr_print_indices(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	SymRef dE = {0};
	SymRef dK = {0};
	CHECK(declare_canonical_pair(ctx, &dE, &dK));

	FacRef x_one = factor_intern(ctx, x, rat_from_int(1), NULL);
	FacRef dE_ai = with_free_indices(ctx, dE, 'a', 'i');
	FacRef dE_aj = with_free_indices(ctx, dE, 'a', 'j');
	FacRef dK_ib = with_free_indices(ctx, dK, 'i', 'b');
	FacRef dK_jb = with_free_indices(ctx, dK, 'j', 'b');
	FacRef dK_ic = with_free_indices(ctx, dK, 'i', 'c');

	ExprRef single = intern_product(ctx, (FacRef[]){dE_ai}, 1);
	ExprRef with_scalar = intern_product(ctx, (FacRef[]){x_one, dE_ai}, 2);
	ExprRef by_i = intern_product(ctx, (FacRef[]){dE_ai, dK_ib}, 2);
	ExprRef by_j = intern_product(ctx, (FacRef[]){dE_aj, dK_jb}, 2);
	ExprRef i_taken = intern_product(ctx, (FacRef[]){dE_aj, dK_jb, dK_ic}, 3);
	CHECK(ctx_ok(ctx));

	CHECK(prints_as(ctx, single, "dE^a_i"));
	CHECK(prints_as(ctx, with_scalar, "(* x dE^a_i)"));

	// A dummy prints as its number, whatever name it was built with.
	CHECK(prints_as(ctx, by_i, "(* dE^a_$0 dK^$0_b)"));
	CHECK(prints_as(ctx, by_j, "(* dE^a_$0 dK^$0_b)"));

	CHECK(prints_as(ctx, i_taken, "(* dE^a_$0 dK^i_c dK^$0_b)"));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_sexpr_read_indices(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	SymRef dE = {0};
	SymRef dK = {0};
	CHECK(declare_canonical_pair(ctx, &dE, &dK));

	FacRef dE_ai = with_free_indices(ctx, dE, 'a', 'i');
	FacRef dK_ib = with_free_indices(ctx, dK, 'i', 'b');

	ExprRef single = intern_product(ctx, (FacRef[]){dE_ai}, 1);
	ExprRef by_i = intern_product(ctx, (FacRef[]){dE_ai, dK_ib}, 2);
	CHECK(x.val != 0 && ctx_ok(ctx));

	CHECK(reads_as(ctx, "dE^a_i", single));

	// A name used up and down becomes a dummy, whatever the name.
	CHECK(reads_as(ctx, "(* dE^a_i dK^i_b)", by_i));
	CHECK(reads_as(ctx, "(* dE^a_k dK^k_b)", by_i));

	CHECK(round_trips(ctx, "dE^a_i"));
	CHECK(round_trips(ctx, "(* x dE^a_i)"));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_sexpr_bad_indices(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef x = symbol_declare_homog(ctx, "x", 0);
	SymRef dE = {0};
	SymRef dK = {0};
	CHECK(declare_canonical_pair(ctx, &dE, &dK));
	CHECK(x.val != 0);

	CHECK(fails_to_read(ctx, "dK"));
	CHECK(fails_to_read(ctx, "dK^i"));
	CHECK(fails_to_read(ctx, "dK^i_a_b"));
	CHECK(fails_to_read(ctx, "dK_i^a"));
	CHECK(fails_to_read(ctx, "dK^a_i"));
	CHECK(fails_to_read(ctx, "dK^i_1"));
	CHECK(fails_to_read(ctx, "x^a"));
	CHECK(fails_to_read(ctx, "(^ dK^i_a 2)"));
	CHECK(fails_to_read(ctx, "(* dK^i_a dK^i_b)"));
	CHECK(fails_to_read(ctx, "(+ dK^i_a dK^j_a)"));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_sexpr_read_dummies(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef dE = {0};
	SymRef dK = {0};
	CHECK(declare_canonical_pair(ctx, &dE, &dK));

	FacRef dE_ai = with_free_indices(ctx, dE, 'a', 'i');
	FacRef dK_ib = with_free_indices(ctx, dK, 'i', 'b');

	ExprRef by_i = intern_product(ctx, (FacRef[]){dE_ai, dK_ib}, 2);
	CHECK(ctx_ok(ctx));

	CHECK(reads_as(ctx, "(* dE^a_$0 dK^$0_b)", by_i));

	// Dummies are renumbered from 0.
	CHECK(reads_as(ctx, "(* dE^a_$5 dK^$5_b)", by_i));

	CHECK(round_trips(ctx, "(* dE^a_$0 dK^$0_b)"));
	CHECK(round_trips(ctx, "(* 2 dE^a_$0 dK^$0_b)"));
	CHECK(round_trips(ctx, "(* dE^a_$0 dK^$0_b dK^i_c)"));

	// dE^a_i dE^b_j dK^i_b: `i` and `b` are both dummies.
	FacRef dE_bj = with_free_indices(ctx, dE, 'b', 'j');
	ExprRef two_dummies =
		intern_product(ctx, (FacRef[]){dE_ai, dE_bj, dK_ib}, 3);
	CHECK(ctx_ok(ctx));

	CHECK(prints_as(ctx, two_dummies, "(* dE^a_$0 dE^$1_j dK^$0_$1)"));
	CHECK(reads_as(ctx, "(* dE^a_$0 dE^$1_j dK^$0_$1)", two_dummies));

	// Numbered the other way round, they are renumbered by first appearance.
	CHECK(reads_as(ctx, "(* dE^a_$1 dE^$0_j dK^$1_$0)", two_dummies));

	CHECK(round_trips(ctx, "(* dE^a_$0 dE^$1_j dK^$0_$1)"));

	ctx_destroy(ctx);

	return fails;
}

static size_t test_sexpr_bad_dummies(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	SymRef dE = {0};
	SymRef dK = {0};
	CHECK(declare_canonical_pair(ctx, &dE, &dK));

	CHECK(fails_to_read(ctx, "dE^a_$0"));
	CHECK(fails_to_read(ctx, "dE^a_$"));
	CHECK(fails_to_read(ctx, "dE^a_$x"));
	CHECK(fails_to_read(ctx, "dE^a_$99999"));

	// The halves of `$0` are in two monomials.
	CHECK(fails_to_read(ctx, "(* dE^a_$0 (+ dK^$0_b))"));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

#include <stddef.h>
#include <stdint.h>

#include <ccae/core/rational.h>

#include "t_utils.h"

static size_t test_rational_new(void);
static size_t test_rational_from_int(void);
static size_t test_rational_zero_one(void);
static size_t test_rational_neg(void);
static size_t test_rational_mul(void);
static size_t test_rational_div(void);
static size_t test_rational_add(void);
static size_t test_rational_sub(void);
static size_t test_rational_cmp(void);

size_t test_rational(void) {
	size_t fails = 0;
	fails += test_rational_new();
	fails += test_rational_from_int();
	fails += test_rational_zero_one();
	fails += test_rational_neg();
	fails += test_rational_mul();
	fails += test_rational_div();
	fails += test_rational_add();
	fails += test_rational_sub();
	fails += test_rational_cmp();

	return fails;
}

static bool is_equal(Rational r, bool negative, u64 num, u64 den) {
	return r.negative == negative && r.num == num && r.den == den;
}

static size_t test_rational_new(void) {
	size_t fails = 0;

	CHECK(is_equal(rat_new(2, 4), false, 1, 2));
	CHECK(is_equal(rat_new(3, -6), true, 1, 2));
	CHECK(is_equal(rat_new(-4, -8), false, 1, 2));
	CHECK(is_equal(rat_new(0, -5), false, 0, 1));

	CHECK(is_equal(rat_new(INT64_MIN, 2), true, 1ULL << 62, 1));
	CHECK(is_equal(rat_new(2, INT64_MIN), true, 1, 1ULL << 62));
	CHECK(is_equal(rat_new(INT64_MIN, INT64_MIN), false, 1, 1));

	return fails;
}

static size_t test_rational_from_int(void) {
	size_t fails = 0;

	CHECK(is_equal(rat_from_int(-7), true, 7, 1));
	CHECK(is_equal(rat_from_int(INT64_MIN), true, 1ULL << 63, 1));

	return fails;
}

static size_t test_rational_zero_one(void) {
	size_t fails = 0;

	CHECK(rat_is_zero(rat_from_int(0)));
	CHECK(!rat_is_zero(rat_from_int(1)));

	CHECK(rat_is_one(rat_from_int(1)));
	CHECK(!rat_is_one(rat_from_int(-1)));
	CHECK(!rat_is_one(rat_from_int(0)));

	return fails;
}

static size_t test_rational_neg(void) {
	size_t fails = 0;

	CHECK(is_equal(rat_neg(rat_from_int(5)), true, 5, 1));
	CHECK(is_equal(rat_neg(rat_from_int(-5)), false, 5, 1));
	CHECK(is_equal(rat_neg(rat_from_int(0)), false, 0, 1));
	CHECK(is_equal(rat_neg(rat_from_int(INT64_MIN)), false, 1ULL << 63, 1));

	return fails;
}

static size_t test_rational_mul(void) {
	size_t fails = 0;

	CHECK(is_equal(rat_mul(rat_new(2, 3), rat_new(3, 4)), false, 1, 2));
	CHECK(is_equal(rat_mul(rat_new(-2, 3), rat_new(3, 4)), true, 1, 2));
	CHECK(is_equal(rat_mul(rat_new(-2, 3), rat_new(-3, 4)), false, 1, 2));
	CHECK(is_equal(rat_mul(rat_new(-2, 3), rat_from_int(0)), false, 0, 1));

	Rational big = rat_from_int(INT64_MIN);
	CHECK(is_equal(rat_mul(big, rat_new(1, 2)), true, 1ULL << 62, 1));
	CHECK(is_equal(rat_mul(big, rat_from_int(-1)), false, 1ULL << 63, 1));

	return fails;
}

static size_t test_rational_div(void) {
	size_t fails = 0;

	CHECK(is_equal(rat_div(rat_new(1, 2), rat_new(3, 4)), false, 2, 3));
	CHECK(is_equal(rat_div(rat_new(1, 2), rat_new(-3, 4)), true, 2, 3));
	CHECK(is_equal(rat_div(rat_from_int(0), rat_new(-3, 4)), false, 0, 1));
	CHECK(is_equal(rat_div(rat_new(3, 4), rat_new(3, 4)), false, 1, 1));

	return fails;
}

static size_t test_rational_add(void) {
	size_t fails = 0;

	CHECK(is_equal(rat_add(rat_new(1, 6), rat_new(1, 3)), false, 1, 2));
	CHECK(is_equal(rat_add(rat_new(1, 2), rat_new(1, 3)), false, 5, 6));
	CHECK(is_equal(rat_add(rat_new(-1, 2), rat_new(-1, 3)), true, 5, 6));

	CHECK(is_equal(rat_add(rat_new(1, 2), rat_new(-1, 3)), false, 1, 6));
	CHECK(is_equal(rat_add(rat_new(1, 3), rat_new(-1, 2)), true, 1, 6));
	CHECK(is_equal(rat_add(rat_new(-1, 2), rat_new(1, 2)), false, 0, 1));

	CHECK(is_equal(rat_add(rat_new(3, 4), rat_from_int(0)), false, 3, 4));

	Rational big = rat_from_int(INT64_MIN);
	CHECK(is_equal(rat_add(big, rat_from_int(1)), true, (1ULL << 63) - 1, 1));

	return fails;
}

static size_t test_rational_sub(void) {
	size_t fails = 0;

	CHECK(is_equal(rat_sub(rat_new(1, 2), rat_new(1, 3)), false, 1, 6));
	CHECK(is_equal(rat_sub(rat_new(1, 3), rat_new(1, 2)), true, 1, 6));
	CHECK(is_equal(rat_sub(rat_new(3, 4), rat_new(3, 4)), false, 0, 1));
	CHECK(is_equal(rat_sub(rat_new(-1, 2), rat_new(1, 2)), true, 1, 1));

	return fails;
}

static size_t test_rational_cmp(void) {
	size_t fails = 0;

	CHECK(rat_cmp(rat_new(1, 3), rat_new(1, 2)) == -1);
	CHECK(rat_cmp(rat_new(1, 2), rat_new(1, 3)) == 1);
	CHECK(rat_cmp(rat_new(2, 4), rat_new(1, 2)) == 0);

	CHECK(rat_cmp(rat_new(-1, 2), rat_new(-1, 3)) == -1);
	CHECK(rat_cmp(rat_new(-1, 2), rat_new(1, 3)) == -1);
	CHECK(rat_cmp(rat_from_int(0), rat_new(-1, 3)) == 1);
	CHECK(rat_cmp(rat_from_int(0), rat_neg(rat_from_int(0))) == 0);

	Rational big = rat_from_int(INT64_MIN);
	CHECK(rat_cmp(big, rat_from_int(INT64_MIN + 1)) == -1);

	return fails;
}

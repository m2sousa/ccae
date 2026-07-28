#include <stdckdint.h>

#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/types.h>
#include <ccae/utils.h>

Rational rat_new(i64 num, i64 den) {
	if (den == 0)
		die("CCAE (rat_new): zero denominator.");

	u64 abs_num = absolute_value(num);
	u64 abs_den = absolute_value(den);
	u64 divisor = gcd(abs_num, abs_den);

	Rational r = {.negative = num != 0 && (num < 0) != (den < 0),
				  .num = abs_num / divisor,
				  .den = abs_den / divisor};

	return r;
}

Rational rat_from_int(i64 n) {
	Rational r = {.negative = n < 0, .num = absolute_value(n), .den = 1};

	return r;
}

bool rat_is_zero(Rational r) { return r.num == 0; }

bool rat_is_one(Rational r) { return !r.negative && r.num == 1 && r.den == 1; }

Rational rat_neg(Rational r) {
	if (r.num != 0)
		r.negative = !r.negative;

	return r;
}

static bool multiply(Rational* out, Rational a, Rational b) {
	u64 g_ab = gcd(a.num, b.den);
	u64 g_ba = gcd(b.num, a.den);

	u64 num = 0;
	u64 den = 0;

	if (ckd_mul(&num, a.num / g_ab, b.num / g_ba))
		return false;

	if (ckd_mul(&den, a.den / g_ba, b.den / g_ab))
		return false;

	out->negative = (num != 0 && a.negative != b.negative);
	out->num = num;
	out->den = den;

	return true;
}

Rational rat_mul(Rational a, Rational b) {
	Rational r = {};

	if (!multiply(&r, a, b))
		die("CCAE (rat_mul): rational overflow.");

	return r;
}

Rational rat_div(Rational a, Rational b) {
	if (b.num == 0)
		die("CCAE (rat_div): division by zero.");

	Rational inverse = {.negative = b.negative, .num = b.den, .den = b.num};
	Rational r = {};

	if (!multiply(&r, a, inverse))
		die("CCAE (rat_div): rational overflow.");

	return r;
}

// Stealed from Knuth, TAOCP vol. 2, 4.5.1, thanks GPT for the ref. :)
static bool add(Rational* out, Rational a, Rational b) {
	u64 g = gcd(a.den, b.den);

	u64 part_a = 0;
	u64 part_b = 0;

	if (ckd_mul(&part_a, a.num, b.den / g))
		return false;

	if (ckd_mul(&part_b, b.num, a.den / g))
		return false;

	u64 sum = 0;
	bool negative = a.negative;

	if (a.negative == b.negative) {
		if (ckd_add(&sum, part_a, part_b))
			return false;
	} else if (part_a >= part_b) {
		sum = part_a - part_b;
	} else {
		sum = part_b - part_a;
		negative = b.negative;
	}

	u64 g_sum = gcd(sum, g);
	u64 den = 0;

	if (ckd_mul(&den, a.den / g, b.den / g_sum))
		return false;

	out->negative = (sum != 0 && negative);
	out->num = sum / g_sum;
	out->den = den;

	return true;
}

Rational rat_add(Rational a, Rational b) {
	Rational r = {};

	if (!add(&r, a, b))
		die("CCAE (rat_add): rational overflow.");

	return r;
}

Rational rat_sub(Rational a, Rational b) {
	Rational r = {};

	if (!add(&r, a, rat_neg(b)))
		die("CCAE (rat_sub): rational overflow.");

	return r;
}

int rat_cmp(Rational a, Rational b) {
	if (a.negative != b.negative)
		return a.negative ? -1 : 1;

	u64 left = 0;
	u64 right = 0;

	// FIXME: Note that products over 64 bits would die.
	if (ckd_mul(&left, a.num, b.den) || ckd_mul(&right, b.num, a.den))
		die("CCAE (rat_cmp): rational overflow.");

	int order = (left > right) - (left < right);

	return a.negative ? -order : order;
}

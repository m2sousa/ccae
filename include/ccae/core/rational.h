#ifndef CCAE_RATIONAL_H
#define CCAE_RATIONAL_H

#include <ccae/types.h>

typedef struct {
	bool negative;
	u64 num;
	u64 den;
} Rational;

/* Returns `num/den` normalised: `den > 0`, `gcd(num, den) = 1`.
 * Dies if `den == 0`.
 *
 * Example:
 *		`Rational half = rat_new(-3, -6);` */
Rational rat_new(i64 num, i64 den);

/* Returns the rational `n/1`.
 *
 * Example:
 *		`Rational three = rat_from_int(3);` */
Rational rat_from_int(i64 n);

/* Returns whether `r` is 0.
 *
 * Example:
 *		`if (rat_is_zero(c)) continue;` */
bool rat_is_zero(Rational r);

/* Returns whether `r` is 1.
 *
 * Example:
 *		`if (rat_is_one(c)) put_mono(out, m);` */
bool rat_is_one(Rational r);

/* Returns `-r`.
 *
 * Example:
 *		`Rational minus_c = rat_neg(c);` */
Rational rat_neg(Rational r);

/* Returns `a * b`. Dies on overflow.
 *
 * Example:
 *		`Rational c = rat_mul(rat_new(2, 3), rat_new(3, 4));` is 1/2. */
Rational rat_mul(Rational a, Rational b);

/* Returns `a / b`. Dies if `b` is 0 or on overflow.
 *
 * Example:
 *		`Rational c = rat_div(rat_new(1, 2), rat_new(3, 4));` is 2/3. */
Rational rat_div(Rational a, Rational b);

/* Returns `a + b`. Dies on overflow.
 *
 * Example:
 *		`Rational c = rat_add(rat_new(1, 6), rat_new(1, 3));` is 1/2. */
Rational rat_add(Rational a, Rational b);

/* Returns `a - b`. Dies on overflow.
 *
 * Example:
 *		`Rational c = rat_sub(rat_new(1, 2), rat_new(1, 3));` is 1/6. */
Rational rat_sub(Rational a, Rational b);

#endif

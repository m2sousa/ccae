#ifndef CCAE_UTILS_H
#define CCAE_UTILS_H

#include <stdbit.h>

#include <ccae/types.h>

/* Returns whether `c` is in [a-zA-Z].
 *
 * Example:
 *		`is_letter('a')` is true, `is_letter('1')` is false. */
static inline bool is_letter(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/* Returns `|x|`.
 *
 * Example:
 *		`absolute_value(-3) is 3.` */
static inline u64 absolute_value(i64 x) {
	return x < 0 ? (u64)0 - (u64)x : (u64)x;
}

/* Returns the greatest common divisor of `a` and `b`.
 *
 * Example:
 *		`gcd(12, 18)` is 6. */
static inline u64 gcd(u64 a, u64 b) {
	if (a == 0)
		return b;

	if (b == 0)
		return a;

	// how many factor of 2 are shared between `a` and `b`.
	unsigned shift = stdc_trailing_zeros(a | b);

	// from there, a is odd.
	a >>= stdc_trailing_zeros(a);

	while (b != 0) {
		// case odd/even, dividing the even one by 2 does not change the gcd.
		b >>= stdc_trailing_zeros(b);

		// for simplicity: gcd(a,b) = gcd(b,a).
		if (a > b) {
			u64 tmp = a;
			a = b;
			b = tmp;
		}

		// gcd(a,b) = gcd(|a-b|, min(a,b)) => gcd(a,b) = gcd(a, b-a).
		b -= a;
	}

	return a << shift;
}

#endif

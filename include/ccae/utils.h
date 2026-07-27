#ifndef CCAE_UTILS_H
#define CCAE_UTILS_H

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
u64 gcd(u64 a, u64 b);

#endif

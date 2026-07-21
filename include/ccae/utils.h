#ifndef CCAE_UTILS_H
#define CCAE_UTILS_H

/* Returns whether `c` is in [a-zA-Z].
 *
 * Example:
 *		`is_letter('a')` is true, `is_letter('1')` is false. */
static inline bool is_letter(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

#endif

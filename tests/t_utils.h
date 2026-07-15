#ifndef CCAE_TEST_UTILS_H
#define CCAE_TEST_UTILS_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
	const char* name;
	size_t (*fn)(void);
} CCAETest;

#define DECLARE_T(t_name) size_t test_##t_name(void)
#define STRUCT_T(t_name) {.name = #t_name, .fn = test_##t_name}

/* Check that `cond` holds. On failure, print `file:line` and the failed
 * expression to `stderr` and increment `fails`, a `size_t` that the calling
 * case declares. Never aborts, so one run reports every failure.
 *
 * Example:
 *		`size_t fails = 0;`
 *		`CHECK(v.length == 3);` */
#define CHECK(cond)                                                            \
	do {                                                                       \
		if (!(cond)) {                                                         \
			fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
					#cond);                                                    \
			++fails;                                                           \
		}                                                                      \
	} while (0)

#endif

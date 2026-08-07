#ifndef CCAE_TEST_REGISTRY_H
#define CCAE_TEST_REGISTRY_H

#include "t_utils.h"

#define TESTS_LIST(X)                                                          \
	X(error)                                                                   \
	X(arena)                                                                   \
	X(vec)                                                                     \
	X(hash)                                                                    \
	X(strpool)                                                                 \
	X(rational)                                                                \
	X(context)                                                                 \
	X(bundle)                                                                  \
	X(symbol)                                                                  \
	X(index)                                                                   \
	X(symmetry)                                                                \
	X(factor)                                                                  \
	X(monomial)                                                                \
	X(expr)                                                                    \
	X(print)                                                                   \
	X(sexpr)                                                                   \
	X(dbm24)

// Holy f*ck, that's my first (useful) X-Macro !
#define DECLARE_TEST(t_name) DECLARE_T(t_name);
TESTS_LIST(DECLARE_TEST)
#undef DECLARE_TEST

#define TESTS_STRUCT(t_name) STRUCT_T(t_name),
static const CCAETest tests[] = {TESTS_LIST(TESTS_STRUCT)};
#undef TESTS_STRUCT

#define NBR_TESTS (sizeof(tests) / sizeof(*tests))

#endif

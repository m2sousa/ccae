#include <stddef.h>

#include <ccae/kernel/symmetry.h>

#include "t_utils.h"

static size_t test_symmetry_identity(void);
static size_t test_symmetry_compose(void);

size_t test_symmetry(void) {
	size_t fails = 0;
	fails += test_symmetry_identity();
	fails += test_symmetry_compose();

	return fails;
}

static size_t test_symmetry_identity(void) {
	size_t fails = 0;

	SignedPerm id = signed_perm_identity();
	SignedPerm expected = {.image = {0, 1, 2, 3, 4, 5}, .sign = 1};
	SignedPerm swap_01 = {.image = {1, 0, 2, 3, 4, 5}, .sign = 1};

	CHECK(signed_perm_equal(id, expected));
	CHECK(!signed_perm_equal(id, swap_01));

	SignedPerm minus_id = {.image = {0, 1, 2, 3, 4, 5}, .sign = -1};
	CHECK(!signed_perm_equal(id, minus_id));

	CHECK(signed_perm_equal(signed_perm_compose(id, swap_01), swap_01));
	CHECK(signed_perm_equal(signed_perm_compose(swap_01, id), swap_01));

	return fails;
}

static size_t test_symmetry_compose(void) {
	size_t fails = 0;

	SignedPerm id = signed_perm_identity();
	SignedPerm swap_01 = {.image = {1, 0, 2, 3, 4, 5}, .sign = -1};
	SignedPerm swap_12 = {.image = {0, 2, 1, 3, 4, 5}, .sign = -1};

	CHECK(signed_perm_equal(signed_perm_compose(swap_01, swap_01), id));

	SignedPerm after_12 = {.image = {1, 2, 0, 3, 4, 5}, .sign = 1};
	SignedPerm after_01 = {.image = {2, 0, 1, 3, 4, 5}, .sign = 1};

	CHECK(signed_perm_equal(signed_perm_compose(swap_01, swap_12), after_12));
	CHECK(signed_perm_equal(signed_perm_compose(swap_12, swap_01), after_01));

	return fails;
}

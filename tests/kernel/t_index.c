#include <stddef.h>

#include <ccae/kernel/index.h>

#include "t_utils.h"

size_t test_index(void) {
	size_t fails = 0;

	Index a = {IDX_FREE, 'a'};
	Index b = {IDX_FREE, 'b'};
	Index dummy_a = {IDX_DUMMY, 'a'};

	CHECK(index_equal(a, a));
	CHECK(!index_equal(a, b));
	CHECK(!index_equal(a, dummy_a));

	return fails;
}

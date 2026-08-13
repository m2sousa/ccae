#include <stddef.h>

#include <ccae/kernel/index.h>

#include "t_utils.h"

static size_t test_index_equal(void);
static size_t test_index_cmp(void);

size_t test_index(void) {
	size_t fails = 0;
	fails += test_index_equal();
	fails += test_index_cmp();

	return fails;
}

static size_t test_index_equal(void) {
	size_t fails = 0;

	Index a = {IDX_FREE, 'a'};
	Index b = {IDX_FREE, 'b'};
	Index dummy_a = {IDX_DUMMY, 'a'};

	CHECK(index_equal(a, a));
	CHECK(!index_equal(a, b));
	CHECK(!index_equal(a, dummy_a));

	return fails;
}

static size_t test_index_cmp(void) {
	size_t fails = 0;

	Index a = {IDX_FREE, 'a'};
	Index b = {IDX_FREE, 'b'};
	Index dummy_0 = {IDX_DUMMY, 0};
	Index dummy_1 = {IDX_DUMMY, 1};

	CHECK(index_cmp(a, a) == 0);
	CHECK(index_cmp(a, b) == -1);
	CHECK(index_cmp(b, a) == 1);
	CHECK(index_cmp(b, dummy_0) == -1);
	CHECK(index_cmp(dummy_0, dummy_1) == -1);
	CHECK(index_cmp(dummy_1, a) == 1);

	return fails;
}

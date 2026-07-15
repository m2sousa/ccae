#include <stddef.h>

#include <ccae/config.h>
#include <ccae/core/arena.h>
#include <ccae/core/vec.h>
#include <ccae/types.h>

#include "t_utils.h"

typedef Vec(u32) U32s;

static size_t test_vec_append(void);
static size_t test_vec_pop(void);
static size_t test_vec_old_buffer(void);

size_t test_vec(void) {
	size_t fails = 0;
	fails += test_vec_append();
	fails += test_vec_pop();
	fails += test_vec_old_buffer();

	return fails;
}

static size_t test_vec_append(void) {
	constexpr size_t NBR_VALUES = 10 * CCAE_VEC_BASE_CAP;
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	U32s values = {0};
	vec_init(values);

	for (size_t i = 0; i < NBR_VALUES; ++i)
		vec_append(a, values, (u32)i);

	CHECK(values.length == NBR_VALUES);
	CHECK(values.capacity >= NBR_VALUES);

	size_t wrong = 0;
	for (size_t i = 0; i < NBR_VALUES; ++i) {
		if (vec_at(values, i) != i)
			++wrong;
	}

	CHECK(wrong == 0);

	arena_destroy(a);

	return fails;
}

static size_t test_vec_pop(void) {
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	U32s values = {0};
	vec_init(values);

	vec_append(a, values, 1);
	vec_append(a, values, 2);

	CHECK(vec_pop(values) == 2);
	CHECK(values.length == 1);
	CHECK(vec_pop(values) == 1);
	CHECK(values.length == 0);

	arena_destroy(a);

	return fails;
}

static size_t test_vec_old_buffer(void) {
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	U32s values = {0};
	vec_init(values);

	for (size_t i = 0; i < CCAE_VEC_BASE_CAP; ++i)
		vec_append(a, values, (u32)i);

	u32* old_data = values.data;
	vec_append(a, values, 0);

	CHECK(values.data != old_data);

	size_t wrong = 0;
	for (size_t i = 0; i < CCAE_VEC_BASE_CAP; ++i) {
		if (old_data[i] != i)
			++wrong;
	}

	CHECK(wrong == 0);

	arena_destroy(a);

	return fails;
}

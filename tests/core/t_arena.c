#include <stddef.h>
#include <stdint.h>

#include <ccae/config.h>
#include <ccae/core/arena.h>
#include <ccae/types.h>

#include "t_utils.h"

static size_t test_arena_alignment(void);
static size_t test_arena_large(void);
static size_t test_arena_zero_size(void);
static size_t test_arena_reset(void);
static size_t test_arena_restore(void);

size_t test_arena(void) {
	size_t fails = 0;
	fails += test_arena_alignment();
	fails += test_arena_large();
	fails += test_arena_zero_size();
	fails += test_arena_reset();
	fails += test_arena_restore();

	return fails;
}

static bool is_zeroed(const u8* p, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		if (p[i])
			return false;
	}

	return true;
}

static size_t test_arena_alignment(void) {
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	u8* byte = arena_alloc(a, u8);
	u64* word = arena_alloc(a, u64);
	max_align_t* widest = arena_alloc(a, max_align_t);

	CHECK((uintptr_t)word % alignof(u64) == 0);
	CHECK((uintptr_t)widest % alignof(max_align_t) == 0);

	CHECK((uintptr_t)word >= (uintptr_t)(byte + 1));
	CHECK((uintptr_t)widest >= (uintptr_t)(word + 1));

	arena_destroy(a);

	return fails;
}

static size_t test_arena_large(void) {
	constexpr size_t BIG_SIZE = CCAE_ARENA_NODE_CAP + 1;
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	u8* big = arena_alloc_array(a, u8, BIG_SIZE);
	CHECK(big != NULL);

	if (big) {
		CHECK(is_zeroed(big, BIG_SIZE));
		big[BIG_SIZE - 1] = 1;
	}

	u64* after_big = arena_alloc(a, u64);
	CHECK(after_big != NULL);
	CHECK(after_big && *after_big == 0);

	arena_destroy(a);

	return fails;
}

static size_t test_arena_zero_size(void) {
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	void* first = arena_zalloc_(a, 0, 1);
	void* second = arena_zalloc_(a, 0, 1);

	CHECK(first != NULL);
	CHECK(second != NULL);
	CHECK(first != second);

	arena_destroy(a);

	return fails;
}

static size_t test_arena_reset(void) {
	constexpr size_t BIG_SIZE = CCAE_ARENA_NODE_CAP + 1;
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	arena_reset(a);

	u64* first = arena_alloc(a, u64);
	arena_reset(a);
	CHECK(arena_alloc(a, u64) == first);

	u8* big = arena_alloc_array(a, u8, BIG_SIZE);
	CHECK(big != NULL);
	arena_reset(a);
	CHECK(arena_alloc(a, u64) == first);

	arena_destroy(a);

	return fails;
}

static size_t test_arena_restore(void) {
	constexpr size_t BIG_SIZE = CCAE_ARENA_NODE_CAP + 1;
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	// A mark taken before the first allocation.
	ArenaMark empty = arena_save(a);
	u64* first = arena_alloc(a, u64);
	arena_restore(a, empty);
	CHECK(arena_alloc(a, u64) == first);

	u64* kept = arena_alloc(a, u64);
	*kept = 42;

	ArenaMark mark = arena_save(a);
	u64* after_mark = arena_alloc(a, u64);

	// Opens a new block, freed by the restore.
	u8* big = arena_alloc_array(a, u8, BIG_SIZE);
	CHECK(big != NULL);

	arena_restore(a, mark);
	CHECK(arena_alloc(a, u64) == after_mark);
	CHECK(*kept == 42);

	arena_destroy(a);

	return fails;
}

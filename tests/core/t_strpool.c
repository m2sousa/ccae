#include <stddef.h>
#include <string.h>

#include <ccae/core/arena.h>
#include <ccae/core/strpool.h>

#include "t_utils.h"

static size_t test_strpool_same(void);
static size_t test_strpool_different(void);
static size_t test_strpool_empty(void);
static size_t test_strpool_repeat(void);
static size_t test_strpool_get(void);

size_t test_strpool(void) {
	size_t fails = 0;
	fails += test_strpool_same();
	fails += test_strpool_different();
	fails += test_strpool_empty();
	fails += test_strpool_repeat();
	fails += test_strpool_get();

	return fails;
}

static size_t test_strpool_same(void) {
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	StringPool* sp = sp_create(a);
	if (!sp) {
		arena_destroy(a);
		return 1;
	}

	StringRef first = sp_intern(sp, "foo");
	StringRef second = sp_intern(sp, "foo");

	CHECK(first.val != 0);
	CHECK(first.val == second.val);

	sp_destroy(sp);
	arena_destroy(a);

	return fails;
}

static size_t test_strpool_different(void) {
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	StringPool* sp = sp_create(a);
	if (!sp) {
		arena_destroy(a);
		return 1;
	}

	StringRef spatial = sp_intern(sp, "foo");
	StringRef internal = sp_intern(sp, "bar");

	CHECK(spatial.val != internal.val);

	sp_destroy(sp);
	arena_destroy(a);

	return fails;
}

static size_t test_strpool_empty(void) {
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	StringPool* sp = sp_create(a);
	if (!sp) {
		arena_destroy(a);
		return 1;
	}

	CHECK(sp_intern(sp, "").val == 0);

	sp_destroy(sp);
	arena_destroy(a);

	return fails;
}

// The pool hands out handles in order, so if 10^5 calls with the same string
// stored it only once, the next new string gets the very next handle.
static size_t test_strpool_repeat(void) {
	constexpr size_t NBR_CALLS = 100'000;
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	StringPool* sp = sp_create(a);
	if (!sp) {
		arena_destroy(a);
		return 1;
	}

	StringRef first = sp_intern(sp, "foo");

	size_t wrong = 0;
	for (size_t i = 0; i < NBR_CALLS; ++i) {
		if (sp_intern(sp, "foo").val != first.val)
			++wrong;
	}

	StringRef next = sp_intern(sp, "bar");

	CHECK(wrong == 0);
	CHECK(next.val == first.val + 1);

	sp_destroy(sp);
	arena_destroy(a);

	return fails;
}

static size_t test_strpool_get(void) {
	size_t fails = 0;

	Arena* a = arena_create();
	if (!a)
		return 1;

	StringPool* sp = sp_create(a);
	if (!sp) {
		arena_destroy(a);
		return 1;
	}

	StringRef spatial = sp_intern(sp, "spatial");

	CHECK(strcmp(sp_get(sp, spatial), "spatial") == 0);
	CHECK(strcmp(sp_get(sp, (StringRef){.val = 0}), "") == 0);

	sp_destroy(sp);
	arena_destroy(a);

	return fails;
}

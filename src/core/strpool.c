#include <stdlib.h>
#include <string.h>

#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/types.h>

typedef struct {
	char* data;
	size_t length;
	u64 hash;
} StringEntry;

typedef Vec(StringEntry) Strings;

struct StringPool {
	Arena* arena;
	Strings entries;
};

static u64 FNV_1a(const char* s) {
	// Parameters have been taken from :
	// en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function#FNV-1a_hash
	u64 hash = 0xcbf29ce484222325;

	for (; *s; ++s) {
		hash ^= (u8)*s;
		hash *= 0x00000100000001b3;
	}

	return hash;
}

StringPool* sp_create(Arena* a) {
	StringPool* sp = malloc(sizeof(*sp));
	if (!sp)
		return NULL;

	sp->arena = a;

	Strings entries = {0};
	vec_init(entries);

	StringEntry null = {.data = "", .length = 0, .hash = FNV_1a("")};
	vec_append(a, entries, null);

	sp->entries = entries;

	return sp;
}

StringRef sp_intern(StringPool* sp, const char* s) {
	size_t length = strlen(s);
	u64 hash = FNV_1a(s);

	// TODO: This is a linear search, setup an hashmap to get O(1) accesses.
	for (size_t i = 0; i < sp->entries.length; ++i) {
		StringEntry elem = vec_at(sp->entries, i);
		if (hash == elem.hash && length == elem.length &&
			memcmp(s, elem.data, length) == 0) {
			StringRef sr = {.val = (u32)i};
			return sr;
		}
	}

	// Allocate one more byte to allocate for the null terminator byte.
	char* data = arena_alloc_array(sp->arena, char, length + 1);
	if (!data)
		die("CCAE (sp_intern): out of memory.");

	memcpy(data, s, length + 1);

	StringEntry se = {.data = data, .length = length, .hash = hash};
	vec_append(sp->arena, sp->entries, se);

	StringRef sr = {.val = (u32)(sp->entries.length - 1)};
	return sr;
}

void sp_destroy(StringPool* sp) { free(sp); }

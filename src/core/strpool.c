#include <stdlib.h>
#include <string.h>

#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/hash.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/types.h>

typedef struct {
	char* data;
	size_t length;
	u64 hash;
} StringEntry;

typedef Vec(StringEntry) StringEntryVec;

struct StringPool {
	Arena* arena;
	StringEntryVec entries;
};

StringPool* sp_create(Arena* a) {
	StringPool* sp = malloc(sizeof(*sp));
	if (!sp)
		return NULL;

	sp->arena = a;

	StringEntryVec entries = {0};
	vec_init(entries);

	StringEntry null = {.data = "", .length = 0, .hash = hash_bytes("", 0)};
	vec_append(a, entries, null);

	sp->entries = entries;

	return sp;
}

StringRef sp_intern(StringPool* sp, const char* s) {
	size_t length = strlen(s);
	u64 hash = hash_bytes(s, length);

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

const char* sp_get(const StringPool* sp, StringRef sr) {
	if (sr.val >= sp->entries.length)
		die("CCAE (sp_get): handle out of range.");

	return vec_at(sp->entries, sr.val).data;
}

void sp_destroy(StringPool* sp) { free(sp); }

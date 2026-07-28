#include <stddef.h>

#include <ccae/core/hash.h>
#include <ccae/types.h>

u64 hash_bytes(const void* data, size_t length) {
	// Parameters have been taken from :
	// en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function#FNV-1a_hash
	u64 hash = 0xcbf29ce484222325;
	const u8* bytes = data;

	for (size_t i = 0; i < length; ++i) {
		hash ^= bytes[i];
		hash *= 0x00000100000001b3;
	}

	return hash;
}

#include <stddef.h>

#include <ccae/core/hash.h>

#include "t_utils.h"

size_t test_hash(void) {
	size_t fails = 0;

	CHECK(hash_bytes("", 0) == 0xcbf29ce484222325);
	CHECK(hash_bytes("a", 1) == 0xaf63dc4c8601ec8c);
	CHECK(hash_bytes("foobar", 6) == 0x85944171f73967e8);

	CHECK(hash_bytes("ab", 2) != hash_bytes("ba", 2));

	return fails;
}

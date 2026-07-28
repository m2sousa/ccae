#ifndef CCAE_HASH_H
#define CCAE_HASH_H

#include <stddef.h>

#include <ccae/types.h>

/* Returns the 64-bit FNV-1a hash of the `length` bytes at `data`.
 *
 * Example:
 *		`u64 h = hash_bytes(s, strlen(s));` */
u64 hash_bytes(const void* data, size_t length);

#endif

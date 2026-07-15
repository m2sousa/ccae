#ifndef CCAE_ARENA_H
#define CCAE_ARENA_H

#include <stddef.h>

typedef struct Arena Arena;

/* Creates & returns a heap-allocated `Arena`.
 * Returns `NULL` on failed allocations.
 *
 * Example:
 *		`Arena* a = arena_create();` */
[[nodiscard]] Arena* arena_create(void);
void arena_destroy(Arena* a);

/* Allocate `size` bytes aligned to `alignment`, which must be a power of two no
 * larger than `alignof(max_align_t)` (else `die`). The memory is not zeroed.
 * A zero `size` still returns a unique pointer.
 * Returns `NULL` on failed allocations.
 *
 * Example:
 *		`u8* p = arena_alloc_(a, 24, 8);` */
[[nodiscard]] void* arena_alloc_(Arena* a, size_t size, size_t alignment);

/* Same as `arena_alloc_`, with the memory zeroed.
 *
 * Example:
 *		`u8* p = arena_zalloc_(a, 24, 8);` */
[[nodiscard]] void* arena_zalloc_(Arena* a, size_t size, size_t alignment);

/* Allocate zero-initialized memory for one object of type `T`, using `T`'s
 * natural alignment. `T` must be a type name, not an expression.
 *
 * Example:
 *		`T* t = arena_alloc(a, T);` */
#define arena_alloc(a, T) ((T*)arena_zalloc_((a), sizeof(T), alignof(T)))

/* Allocate zero-initialized memory for an array of type `T` of sized `sz` using
 * `T`'s natural alignment. `T` must be a type name, not an expression.
 *
 * Example:
 *		`T* t = arena_alloc_array(a, T, size);` */
#define arena_alloc_array(a, T, sz)                                            \
	((T*)arena_zalloc_((a), sizeof(T) * (sz), alignof(T)))

#endif

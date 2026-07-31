#ifndef CCAE_ARENA_H
#define CCAE_ARENA_H

#include <stddef.h>

typedef struct Arena Arena;

/* Creates an `Arena`. Returns `NULL` on failed allocation.
 *
 * Example:
 *		`Arena* a = arena_create();` */
[[nodiscard]] Arena* arena_create(void);
void arena_destroy(Arena* a);

/* Frees every allocation of `a` at once and keeps its first node for reuse.
 *
 * Example:
 *		`arena_reset(ctx->scratch);` */
void arena_reset(Arena* a);

/* Allocates `size` bytes aligned to `alignment`, not zeroed. `alignment` is a
 * power of two up to `alignof(max_align_t)`, else dies. Returns `NULL` on
 * failed allocation.
 *
 * Example:
 *		`u8* p = arena_alloc_(a, 24, 8);` */
[[nodiscard]] void* arena_alloc_(Arena* a, size_t size, size_t alignment);

/* Same as `arena_alloc_`, zeroed.
 *
 * Example:
 *		`u8* p = arena_zalloc_(a, 24, 8);` */
[[nodiscard]] void* arena_zalloc_(Arena* a, size_t size, size_t alignment);

/* Allocates one zeroed `T` (a type name, not an expression).
 *
 * Example:
 *		`T* t = arena_alloc(a, T);` */
#define arena_alloc(a, T) ((T*)arena_zalloc_((a), sizeof(T), alignof(T)))

/* Allocates `sz` zeroed `T`s (a type name, not an expression).
 *
 * Example:
 *		`T* t = arena_alloc_array(a, T, size);` */
#define arena_alloc_array(a, T, sz)                                            \
	((T*)arena_zalloc_((a), sizeof(T) * (sz), alignof(T)))

#endif

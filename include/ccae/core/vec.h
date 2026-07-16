#ifndef CCAE_VEC_H
#define CCAE_VEC_H

#include <assert.h>
#include <stddef.h>

#include <ccae/core/arena.h>

/* A growable array of `T`, stored in an arena. Two expansions of `Vec(T)` are
 * different types, so name it once with a `typedef`.
 *
 * Example:
 *		`typedef Vec(StringEntry) StringEntryVec;` */
#define Vec(T)                                                                 \
	struct {                                                                   \
		T* data;                                                               \
		size_t length;                                                         \
		size_t capacity;                                                       \
	}

/* Make `v` empty, with no buffer.
 *
 * Example:
 *		`vec_init(entries);` */
#define vec_init(v)                                                            \
	do {                                                                       \
		(v).data = NULL;                                                       \
		(v).length = 0u;                                                       \
		(v).capacity = 0u;                                                     \
	} while (0)

/* Append `x` to `v`, growing it in `a` when full. The old buffer is left in the
 * arena as garbage, so pointers into it stay valid but go stale.
 *
 * Example:
 *		`vec_append(arena, entries, entry);` */
#define vec_append(a, v, x)                                                    \
	do {                                                                       \
		if ((v).length == (v).capacity)                                        \
			(v).data =                                                         \
				vec_grow_((a), (v).data, (v).length, &(v).capacity,            \
						  sizeof(*(v).data), alignof(typeof(*(v).data)));      \
		(v).data[(v).length++] = (x);                                          \
	} while (0)

/* The element of `v` at index `i`. No bounds check.
 *
 * Example:
 *		`StringEntry e = vec_at(entries, i);` */
#define vec_at(v, i) ((v).data[i])

/* Remove the last element of `v` and return it. `v` must not be empty.
 *
 * Example:
 *		`StringEntry last = vec_pop(entries);` */
#define vec_pop(v) (assert((v).length > 0), (v).data[--(v).length])

/* Allocate a buffer twice as large as `*capacity` (or `CCAE_VEC_BASE_CAP`
 * elements when it is 0) in `a`, copy the `length` first elements of `data`
 * into it, update `*capacity` and return it. Dies on overflow or when the
 * arena is out of memory. Called by `vec_append`, not directly. */
[[nodiscard]] void* vec_grow_(Arena* a, const void* data, size_t length,
							  size_t* capacity, size_t elem_size,
							  size_t elem_align);

#endif

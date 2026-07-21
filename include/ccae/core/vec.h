#ifndef CCAE_VEC_H
#define CCAE_VEC_H

#include <assert.h>
#include <stddef.h>

#include <ccae/core/arena.h>

/* A growable array of `T` in an arena. Two expansions of `Vec(T)` are
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

/* Empties `v`, with no buffer.
 *
 * Example:
 *		`vec_init(entries);` */
#define vec_init(v)                                                            \
	do {                                                                       \
		(v).data = NULL;                                                       \
		(v).length = 0u;                                                       \
		(v).capacity = 0u;                                                     \
	} while (0)

/* Appends `x` to `v`, growing it in `a` when full. Pointers into the old
 * buffer go stale.
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

/* Element `i` of `v`, without bounds check.
 *
 * Example:
 *		`StringEntry e = vec_at(entries, i);` */
#define vec_at(v, i) ((v).data[i])

/* Removes and returns the last element of `v`, which must not be empty.
 *
 * Example:
 *		`StringEntry last = vec_pop(entries);` */
#define vec_pop(v) (assert((v).length > 0), (v).data[--(v).length])

/* Behind `vec_append`, not called directly: returns a copy of `data` with
 * twice the capacity (`CCAE_VEC_BASE_CAP` at first) and updates `*capacity`.
 * Dies on overflow or out of memory. */
[[nodiscard]] void* vec_grow_(Arena* a, const void* data, size_t length,
							  size_t* capacity, size_t elem_size,
							  size_t elem_align);

#endif

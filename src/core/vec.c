#include <stdckdint.h>
#include <stddef.h>
#include <string.h>

#include <ccae/config.h>
#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/vec.h>

void* vec_grow_(Arena* a, const void* data, size_t length, size_t* capacity,
				size_t elem_size, size_t elem_align) {
	size_t new_capacity = CCAE_VEC_BASE_CAP;
	if (*capacity && ckd_mul(&new_capacity, *capacity, 2))
		die("CCAE (vec_grow_): capacity overflow.");

	size_t new_size;
	if (ckd_mul(&new_size, new_capacity, elem_size))
		die("CCAE (vec_grow_): size overflow.");

	void* new_data = arena_alloc_(a, new_size, elem_align);
	if (!new_data)
		die("CCAE (vec_grow_): out of memory.");

	if (length)
		memcpy(new_data, data, length * elem_size);

	*capacity = new_capacity;

	return new_data;
}

#include <stdbit.h>
#include <stdckdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ccae/config.h>
#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/types.h>

struct ArenaNode_s {
	ArenaNode* next;
	size_t used, capacity;
	alignas(max_align_t) u8 data[];
};

struct Arena {
	ArenaNode* head;
	ArenaNode* current;
};

static ArenaNode* node_init(size_t capacity) {
	size_t bytes;
	if (ckd_add(&bytes, sizeof(ArenaNode), capacity))
		return NULL;

	ArenaNode* node = malloc(bytes);
	if (!node)
		return NULL;

	node->next = NULL;
	node->used = 0;
	node->capacity = capacity;

	return node;
}

Arena* arena_create(void) {
	Arena* a = malloc(sizeof(*a));
	if (!a)
		return NULL;

	a->head = NULL;
	a->current = NULL;

	return a;
}

void* arena_alloc_(Arena* a, size_t size, size_t alignment) {
	// A fresh node is only aligned to `max_align_t`, so more cannot be
	// honoured.
	if (!stdc_has_single_bit(alignment) || alignment > alignof(max_align_t))
		die("CCAE (arena_alloc_): alignment must be a power of two, at most "
			"alignof(max_align_t).");

	// One byte instead of zero, so every allocation gets its own address.
	if (size == 0)
		size = 1;

	ArenaNode* current = a->current;

	if (current) {
		uintptr_t base = (uintptr_t)current->data + current->used;
		size_t padding =
			(size_t)(((base + (alignment - 1)) & ~(alignment - 1)) - base);
		size_t left = current->capacity - current->used;

		if (padding <= left && size <= left - padding) {
			current->used += padding + size;
			return (void*)(base + padding);
		}
	}

	size_t capacity = size > CCAE_ARENA_NODE_CAP ? size : CCAE_ARENA_NODE_CAP;

	ArenaNode* node = node_init(capacity);
	if (!node)
		return NULL;

	if (!a->head)
		a->head = node;
	else
		a->current->next = node;

	a->current = node;
	node->used = size;

	return node->data;
}

void* arena_zalloc_(Arena* a, size_t size, size_t alignment) {
	void* p = arena_alloc_(a, size, alignment);
	if (!p)
		return NULL;

	memset(p, 0, size);

	return p;
}

void arena_reset(Arena* a) {
	if (!a->head)
		return;

	ArenaNode* node = a->head->next;

	while (node) {
		ArenaNode* next = node->next;
		free(node);
		node = next;
	}

	a->head->next = NULL;
	a->head->used = 0;
	a->current = a->head;
}

ArenaMark arena_save(const Arena* a) {
	ArenaMark mark = {.node = a->current, .used = 0};

	if (a->current)
		mark.used = a->current->used;

	return mark;
}

void arena_restore(Arena* a, ArenaMark mark) {
	if (!mark.node) {
		arena_reset(a);
		return;
	}

	ArenaNode* node = mark.node->next;

	while (node) {
		ArenaNode* next = node->next;
		free(node);
		node = next;
	}

	mark.node->next = NULL;
	mark.node->used = mark.used;
	a->current = mark.node;
}

void arena_destroy(Arena* a) {
	while (a->head) {
		ArenaNode* node = a->head;
		a->head = node->next;
		free(node);
	}

	free(a);
}

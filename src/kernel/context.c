#include <stdlib.h>

#include <ccae/core/arena.h>
#include <ccae/core/strpool.h>
#include <ccae/kernel/context.h>

struct Context {
	Arena* perm;
	StringPool* sp;
};

Context* ctx_create(void) {
	Context* ctx = malloc(sizeof(*ctx));
	if (!ctx)
		return NULL;

	Arena* arena = arena_create();
	if (!arena) {
		free(ctx);
		return NULL;
	}

	StringPool* sp = sp_create(arena);
	if (!sp) {
		arena_destroy(arena);
		free(ctx);
		return NULL;
	}

	ctx->perm = arena;
	ctx->sp = sp;

	return ctx;
}

void ctx_destroy(Context* ctx) {
	sp_destroy(ctx->sp);
	arena_destroy(ctx->perm);

	free(ctx);
}

#include <stdlib.h>

#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>

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

	Bundles bundles = {0};
	vec_init(bundles);

	Bundle null_bundle = {0};
	vec_append(arena, bundles, null_bundle); // slot 0 = null handle

	ctx->perm = arena;
	ctx->sp = sp;
	ctx->bundles = bundles;
	err_clear(&ctx->err);

	return ctx;
}

void ctx_destroy(Context* ctx) {
	sp_destroy(ctx->sp);
	arena_destroy(ctx->perm);

	free(ctx);
}

bool ctx_ok(const Context* ctx) { return ctx->err.code == CCAE_OK; }

void ctx_clear_err(Context* ctx) { err_clear(&ctx->err); }

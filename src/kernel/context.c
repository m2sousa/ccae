#include <stdlib.h>

#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>

// TODO: Those two helpers function are very similar, could do a macro there to
// simplify. Will to it later when improving on readibility. Want to focus on
// the rewrite for now.
static void init_bundles(Context* ctx, Arena* arena) {
	vec_init(ctx->bundles);

	Bundle null_bundle = {0};
	vec_append(arena, ctx->bundles, null_bundle);
}

static void init_symbols(Context* ctx, Arena* arena) {
	vec_init(ctx->symbols);

	Symbol null_symbol = {0};
	vec_append(arena, ctx->symbols, null_symbol);
}

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

	init_bundles(ctx, arena);
	init_symbols(ctx, arena);

	ctx->perm = arena;
	ctx->sp = sp;

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

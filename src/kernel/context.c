#include <stdlib.h>

#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/expr.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/monomial.h>
#include <ccae/kernel/symbol.h>

// TODO: Those five helpers function are very similar, could do a macro there
// to simplify. Will to it later when improving on readibility. Want to focus on
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

static void init_factors(Context* ctx, Arena* arena) {
	vec_init(ctx->factors);

	Factor null_factor = {0};
	vec_append(arena, ctx->factors, null_factor);
}

static void init_monomials(Context* ctx, Arena* arena) {
	vec_init(ctx->monomials);

	Monomial null_monomial = {0};
	vec_append(arena, ctx->monomials, null_monomial);
}

static void init_exprs(Context* ctx, Arena* arena) {
	vec_init(ctx->exprs);

	Expr null_expr = {0};
	vec_append(arena, ctx->exprs, null_expr);
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
	init_factors(ctx, arena);
	init_monomials(ctx, arena);
	init_exprs(ctx, arena);

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

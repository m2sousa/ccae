#ifndef CCAE_CONTEXT_H
#define CCAE_CONTEXT_H

#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/fwd.h>
#include <ccae/kernel/symbol.h>

struct Context {
	Arena* perm;
	StringPool* sp;
	Error err;
	BundleVec bundles;
	SymbolVec symbols;
};

/* Creates a `Context`. Returns `NULL` on failed allocation.
 *
 * Example:
 *		`Context* ctx = ctx_create();` */
[[nodiscard]] Context* ctx_create(void);
void ctx_destroy(Context* ctx);

/* Returns whether `ctx` holds no error.
 *
 * Example:
 *		`if (!ctx_ok(ctx)) return null;` */
bool ctx_ok(const Context* ctx);

/* Clears the error held by `ctx`.
 *
 * Example:
 *		`ctx_clear_err(ctx);` */
void ctx_clear_err(Context* ctx);

#endif

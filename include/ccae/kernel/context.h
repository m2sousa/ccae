#ifndef CCAE_CONTEXT_H
#define CCAE_CONTEXT_H

typedef struct Context Context;

/* Creates a `Context`, which owns the session's arena and string pool.
 * Returns `NULL` on failed allocations.
 *
 * Example:
 *		`Context* ctx = ctx_create();` */
[[nodiscard]] Context* ctx_create(void);
void ctx_destroy(Context* ctx);

#endif

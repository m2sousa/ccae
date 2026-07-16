#ifndef CCAE_STRPOOL_H
#define CCAE_STRPOOL_H

#include <ccae/core/arena.h>
#include <ccae/types.h>

typedef struct StringPool StringPool;

typedef struct {
	u32 val;
} StringRef;

/* Creates a `StringPool` whose strings live in `a` (borrowed, not owned).
 * Handle 0 is the empty string. Returns `NULL` on failed allocations.
 *
 * Example:
 *		`StringPool* sp = sp_create(a);` */
[[nodiscard]] StringPool* sp_create(Arena* a);
void sp_destroy(StringPool* sp);

/* Returns the handle of `s`, copying `s` into the pool the first time it is
 * seen. Equal strings always get the same handle, different strings different
 * ones.
 *
 * Example:
 *		`StringRef name = sp_intern(sp, "spatial");` */
[[nodiscard]] StringRef sp_intern(StringPool* sp, const char* s);

/* Returns the string behind `sr`. Dies if `sr` does not belong to `sp`.
 *
 * Example:
 *		`const char* name = sp_get(sp, bundle.name);` */
const char* sp_get(const StringPool* sp, StringRef sr);

#endif

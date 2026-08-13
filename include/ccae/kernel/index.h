#ifndef CCAE_INDEX_H
#define CCAE_INDEX_H

#include <ccae/types.h>

typedef enum : u8 {
	IDX_FREE,
	IDX_DUMMY,
} IndexKind;

typedef struct {
	IndexKind kind;
	u16 id;
} Index;

/* Returns whether `a` and `b` are the same index: same kind and same id.
 *
 * Example:
 *		`index_equal((Index){IDX_FREE, 'a'}, (Index){IDX_FREE, 'a'})` is true.
 * */
bool index_equal(Index a, Index b);

/* Returns -1, 0 or 1 as `a` comes before, is, or comes after `b`.
 * Free indices first, by index name, then dummies, by number.
 *
 * Example:
 *		`index_cmp((Index){IDX_FREE, 'b'}, (Index){IDX_DUMMY, 0})` is -1. */
int index_cmp(Index a, Index b);

#endif

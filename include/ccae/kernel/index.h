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

#endif

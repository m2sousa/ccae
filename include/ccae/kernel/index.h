#ifndef CCAE_INDEX_H
#define CCAE_INDEX_H

#include <ccae/types.h>

typedef enum : u8 {
	IDX_FREE,
	IDX_DUMMY,
} IndexKind;

typedef struct {
	IndexKind kind;
	u8 id;
} Index;

#endif

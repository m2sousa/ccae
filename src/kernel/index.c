#include <ccae/kernel/index.h>

bool index_equal(Index a, Index b) { return a.kind == b.kind && a.id == b.id; }

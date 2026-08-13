#include <ccae/kernel/index.h>

bool index_equal(Index a, Index b) { return a.kind == b.kind && a.id == b.id; }

int index_cmp(Index a, Index b) {
	if (a.kind != b.kind)
		return a.kind < b.kind ? -1 : 1;

	if (a.id != b.id)
		return a.id < b.id ? -1 : 1;

	return 0;
}

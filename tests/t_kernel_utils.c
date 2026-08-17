#include <ccae/core/rational.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/factor.h>
#include <ccae/kernel/index.h>
#include <ccae/kernel/symbol.h>

#include "t_kernel_utils.h"

bool declare_canonical_pair(Context* ctx, SymRef* dE, SymRef* dK) {
	BundleRef spatial = bundle_declare(ctx, "spatial", "abcd", 3);
	BundleRef internal = bundle_declare(ctx, "internal", "ijk", 3);
	*dE = symbol_declare_tensor(ctx, "dE", SYM_FIELD, "^a_i", "", 0);
	*dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", "", 0);

	return spatial.val != 0 && internal.val != 0 && ctx_ok(ctx);
}

FacRef with_indices(Context* ctx, SymRef head, Index first, Index second) {
	return factor_intern(ctx, head, rat_from_int(1), (Index[]){first, second});
}

FacRef with_free_indices(Context* ctx, SymRef head, char first, char second) {
	Index indices[] = {{IDX_FREE, (u16)first}, {IDX_FREE, (u16)second}};

	return factor_intern(ctx, head, rat_from_int(1), indices);
}

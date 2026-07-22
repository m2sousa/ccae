#include <stddef.h>
#include <string.h>

#include <ccae/io/print.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>

#include "t_utils.h"

// Checks against M. De Sousa, A. Barrau, K. Martineau, arXiv:2409.06441.

static size_t test_dbm24_declarations(void);

size_t test_dbm24(void) {
	size_t fails = 0;
	fails += test_dbm24_declarations();

	return fails;
}

// The declaration set of DBM24 (doc/spec.md, M1.5) and its table dump.
static size_t test_dbm24_declarations(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	CHECK(bundle_declare(ctx, "spatial", "abcdefgh").val != 0);
	CHECK(bundle_declare(ctx, "internal", "ijklmn").val != 0);

	// Linking object, Levi-Civita and Kronecker tensors.
	CHECK(symbol_declare_tensor(ctx, "triad", SYM_CONST, "^a_i", 0).val != 0);
	CHECK(symbol_declare_tensor(ctx, "cotriad", SYM_CONST, "_a^i", 0).val != 0);
	CHECK(
		symbol_declare_tensor(ctx, "epsSpatial", SYM_CONST, "_a_b_c", 0).val !=
		0);
	CHECK(
		symbol_declare_tensor(ctx, "epsInternal", SYM_CONST, "_i_j_k", 0).val !=
		0);
	CHECK(symbol_declare_tensor(ctx, "kroneckerSpatial", SYM_CONST, "^a_b", 0)
			  .val != 0);
	CHECK(symbol_declare_tensor(ctx, "kroneckerInternal", SYM_CONST, "^i_j", 0)
			  .val != 0);

	// Background: homogeneous variables, lapse and constants.
	CHECK(symbol_declare_homog(ctx, "p", F_POSITIVE | F_NONZERO | F_PHASE_SPACE)
			  .val != 0);
	CHECK(symbol_declare_homog(ctx, "c", F_PHASE_SPACE).val != 0);
	CHECK(symbol_declare_homog(ctx, "phi0", F_PHASE_SPACE).val != 0);
	CHECK(symbol_declare_homog(ctx, "pi0", F_PHASE_SPACE).val != 0);
	CHECK(symbol_declare_tensor(ctx, "N0", SYM_TESTFN, "",
								F_CONST_IN_SPACE | F_NONZERO)
			  .val != 0);
	CHECK(symbol_declare_const(ctx, "kappa", F_POSITIVE | F_NONZERO).val != 0);
	CHECK(symbol_declare_const(ctx, "gamma", F_POSITIVE | F_NONZERO).val != 0);
	CHECK(symbol_declare_const(ctx, "V", F_POSITIVE | F_NONZERO).val != 0);

	// Perturbations: fields and local test functions.
	CHECK(symbol_declare_tensor(ctx, "dE", SYM_FIELD, "^a_i", F_ZERO_MODE_FREE)
			  .val != 0);
	CHECK(symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", F_ZERO_MODE_FREE)
			  .val != 0);
	CHECK(symbol_declare_tensor(ctx, "dphi", SYM_FIELD, "", F_ZERO_MODE_FREE)
			  .val != 0);
	CHECK(symbol_declare_tensor(ctx, "dpi", SYM_FIELD, "", F_ZERO_MODE_FREE)
			  .val != 0);
	CHECK(symbol_declare_tensor(ctx, "dN", SYM_TESTFN, "", F_ZERO_MODE_FREE)
			  .val != 0);
	CHECK(symbol_declare_tensor(ctx, "dNs", SYM_TESTFN, "^a", F_ZERO_MODE_FREE)
			  .val != 0);
	CHECK(symbol_declare_tensor(ctx, "dLam", SYM_TESTFN, "^i", F_ZERO_MODE_FREE)
			  .val != 0);

	char buf[1024];
	print_symbol_table(ctx, PRINT_ASCII, buf, sizeof(buf));

	CHECK(strcmp(buf, "triad^a_i const CONST_IN_SPACE\n"
					  "cotriad_a^i const CONST_IN_SPACE\n"
					  "epsSpatial_a_b_c const CONST_IN_SPACE\n"
					  "epsInternal_i_j_k const CONST_IN_SPACE\n"
					  "kroneckerSpatial^a_b const CONST_IN_SPACE\n"
					  "kroneckerInternal^i_j const CONST_IN_SPACE\n"
					  "p homog CONST_IN_SPACE|PHASE_SPACE|POSITIVE|NONZERO\n"
					  "c homog CONST_IN_SPACE|PHASE_SPACE\n"
					  "phi0 homog CONST_IN_SPACE|PHASE_SPACE\n"
					  "pi0 homog CONST_IN_SPACE|PHASE_SPACE\n"
					  "N0 testfn CONST_IN_SPACE|NONZERO\n"
					  "kappa const CONST_IN_SPACE|POSITIVE|NONZERO\n"
					  "gamma const CONST_IN_SPACE|POSITIVE|NONZERO\n"
					  "V const CONST_IN_SPACE|POSITIVE|NONZERO\n"
					  "dE^a_i field ZERO_MODE_FREE\n"
					  "dK^i_a field ZERO_MODE_FREE\n"
					  "dphi field ZERO_MODE_FREE\n"
					  "dpi field ZERO_MODE_FREE\n"
					  "dN testfn ZERO_MODE_FREE\n"
					  "dNs^a testfn ZERO_MODE_FREE\n"
					  "dLam^i testfn ZERO_MODE_FREE\n") == 0);

	ctx_destroy(ctx);

	return fails;
}

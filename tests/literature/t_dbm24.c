#include <stddef.h>
#include <string.h>

#include <ccae/io/print.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>

#include "t_utils.h"

// Checks against M. De Sousa, A. Barrau, K. Martineau, arXiv:2409.06441.

static size_t test_dbm24_ascii(void);
static size_t test_dbm24_latex(void);

size_t test_dbm24(void) {
	size_t fails = 0;
	fails += test_dbm24_ascii();
	fails += test_dbm24_latex();

	return fails;
}

// Gives `sym` its LaTeX rendering, and tells whether it was declared.
static bool with_latex(Context* ctx, SymRef sym, const char* latex) {
	if (!sym.val)
		return false;

	symbol_set_latex(ctx, sym, latex);

	return true;
}

// Declares the DBM24 declaration set (doc/spec.md, M1.5) into `ctx`, each
// symbol with its LaTeX rendering. Returns the number of failed checks.
static size_t declare_dbm24(Context* ctx) {
	size_t fails = 0;

	CHECK(bundle_declare(ctx, "spatial", "abcdefgh").val != 0);
	CHECK(bundle_declare(ctx, "internal", "ijklmn").val != 0);

	// Linking object, Levi-Civita and Kronecker tensors.
	CHECK(with_latex(ctx,
					 symbol_declare_tensor(ctx, "triad", SYM_CONST, "^a_i", 0),
					 "\\delta"));
	CHECK(with_latex(
		ctx, symbol_declare_tensor(ctx, "cotriad", SYM_CONST, "_a^i", 0),
		"\\delta"));
	CHECK(with_latex(
		ctx, symbol_declare_tensor(ctx, "epsSpatial", SYM_CONST, "_a_b_c", 0),
		"\\epsilon"));
	CHECK(with_latex(
		ctx, symbol_declare_tensor(ctx, "epsInternal", SYM_CONST, "_i_j_k", 0),
		"\\epsilon"));
	CHECK(with_latex(
		ctx,
		symbol_declare_tensor(ctx, "kroneckerSpatial", SYM_CONST, "^a_b", 0),
		"\\delta"));
	CHECK(with_latex(
		ctx,
		symbol_declare_tensor(ctx, "kroneckerInternal", SYM_CONST, "^i_j", 0),
		"\\delta"));

	// Background: homogeneous variables, lapse and constants.
	CHECK(with_latex(
		ctx,
		symbol_declare_homog(ctx, "p", F_POSITIVE | F_NONZERO | F_PHASE_SPACE),
		"\\frak{p}"));
	CHECK(with_latex(ctx, symbol_declare_homog(ctx, "c", F_PHASE_SPACE),
					 "\\frak{c}"));
	CHECK(with_latex(ctx, symbol_declare_homog(ctx, "phi0", F_PHASE_SPACE),
					 "\\bm{\\phi}"));
	CHECK(with_latex(ctx, symbol_declare_homog(ctx, "pi0", F_PHASE_SPACE),
					 "\\bm{\\pi}"));
	CHECK(with_latex(ctx,
					 symbol_declare_tensor(ctx, "N0", SYM_TESTFN, "",
										   F_CONST_IN_SPACE | F_NONZERO),
					 "\\mathbf{N}"));
	CHECK(with_latex(ctx,
					 symbol_declare_const(ctx, "kappa", F_POSITIVE | F_NONZERO),
					 "\\kappa"));
	CHECK(with_latex(ctx,
					 symbol_declare_const(ctx, "gamma", F_POSITIVE | F_NONZERO),
					 "\\gamma"));
	CHECK(with_latex(ctx,
					 symbol_declare_const(ctx, "V", F_POSITIVE | F_NONZERO),
					 "\\mathcal{V}"));

	// Perturbations: fields and local test functions.
	CHECK(with_latex(
		ctx,
		symbol_declare_tensor(ctx, "dE", SYM_FIELD, "^a_i", F_ZERO_MODE_FREE),
		"\\delta E"));
	CHECK(with_latex(
		ctx,
		symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", F_ZERO_MODE_FREE),
		"\\delta K"));
	CHECK(with_latex(
		ctx,
		symbol_declare_tensor(ctx, "dphi", SYM_FIELD, "", F_ZERO_MODE_FREE),
		"\\delta\\phi"));
	CHECK(with_latex(
		ctx, symbol_declare_tensor(ctx, "dpi", SYM_FIELD, "", F_ZERO_MODE_FREE),
		"\\delta\\pi"));
	CHECK(with_latex(
		ctx, symbol_declare_tensor(ctx, "dN", SYM_TESTFN, "", F_ZERO_MODE_FREE),
		"\\delta N"));
	CHECK(with_latex(
		ctx,
		symbol_declare_tensor(ctx, "dNs", SYM_TESTFN, "^a", F_ZERO_MODE_FREE),
		"\\delta N"));
	CHECK(with_latex(
		ctx,
		symbol_declare_tensor(ctx, "dLam", SYM_TESTFN, "^i", F_ZERO_MODE_FREE),
		"\\delta\\Lambda"));

	return fails;
}

static size_t test_dbm24_ascii(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	fails += declare_dbm24(ctx);

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

static size_t test_dbm24_latex(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	fails += declare_dbm24(ctx);

	char buf[1024];
	print_symbol_table(ctx, PRINT_LATEX, buf, sizeof(buf));

	CHECK(strcmp(buf,
				 "\\delta^{a}{}_{i} const CONST_IN_SPACE\n"
				 "\\delta_{a}{}^{i} const CONST_IN_SPACE\n"
				 "\\epsilon_{abc} const CONST_IN_SPACE\n"
				 "\\epsilon_{ijk} const CONST_IN_SPACE\n"
				 "\\delta^{a}{}_{b} const CONST_IN_SPACE\n"
				 "\\delta^{i}{}_{j} const CONST_IN_SPACE\n"
				 "\\frak{p} homog CONST_IN_SPACE|PHASE_SPACE|POSITIVE|NONZERO\n"
				 "\\frak{c} homog CONST_IN_SPACE|PHASE_SPACE\n"
				 "\\bm{\\phi} homog CONST_IN_SPACE|PHASE_SPACE\n"
				 "\\bm{\\pi} homog CONST_IN_SPACE|PHASE_SPACE\n"
				 "\\mathbf{N} testfn CONST_IN_SPACE|NONZERO\n"
				 "\\kappa const CONST_IN_SPACE|POSITIVE|NONZERO\n"
				 "\\gamma const CONST_IN_SPACE|POSITIVE|NONZERO\n"
				 "\\mathcal{V} const CONST_IN_SPACE|POSITIVE|NONZERO\n"
				 "\\delta E^{a}{}_{i} field ZERO_MODE_FREE\n"
				 "\\delta K^{i}{}_{a} field ZERO_MODE_FREE\n"
				 "\\delta\\phi field ZERO_MODE_FREE\n"
				 "\\delta\\pi field ZERO_MODE_FREE\n"
				 "\\delta N testfn ZERO_MODE_FREE\n"
				 "\\delta N^{a} testfn ZERO_MODE_FREE\n"
				 "\\delta\\Lambda^{i} testfn ZERO_MODE_FREE\n") == 0);

	ctx_destroy(ctx);

	return fails;
}

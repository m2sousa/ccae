#include <stddef.h>

#include <ccae/kernel/context.h>
#include <ccae/kernel/symmetry.h>

#include "t_utils.h"

static size_t test_symmetry_identity(void);
static size_t test_symmetry_compose(void);
static size_t test_symmetry_read(void);
static size_t test_symmetry_bad_strings(void);

size_t test_symmetry(void) {
	size_t fails = 0;
	fails += test_symmetry_identity();
	fails += test_symmetry_compose();
	fails += test_symmetry_read();
	fails += test_symmetry_bad_strings();

	return fails;
}

static size_t test_symmetry_identity(void) {
	size_t fails = 0;

	SignedPerm id = signed_perm_identity();
	SignedPerm expected = {.image = {0, 1, 2, 3, 4, 5}, .sign = 1};
	SignedPerm swap_01 = {.image = {1, 0, 2, 3, 4, 5}, .sign = 1};

	CHECK(signed_perm_equal(id, expected));
	CHECK(!signed_perm_equal(id, swap_01));

	SignedPerm minus_id = {.image = {0, 1, 2, 3, 4, 5}, .sign = -1};
	CHECK(!signed_perm_equal(id, minus_id));

	CHECK(signed_perm_equal(signed_perm_compose(id, swap_01), swap_01));
	CHECK(signed_perm_equal(signed_perm_compose(swap_01, id), swap_01));

	return fails;
}

static size_t test_symmetry_compose(void) {
	size_t fails = 0;

	SignedPerm id = signed_perm_identity();
	SignedPerm swap_01 = {.image = {1, 0, 2, 3, 4, 5}, .sign = -1};
	SignedPerm swap_12 = {.image = {0, 2, 1, 3, 4, 5}, .sign = -1};

	CHECK(signed_perm_equal(signed_perm_compose(swap_01, swap_01), id));

	SignedPerm after_12 = {.image = {1, 2, 0, 3, 4, 5}, .sign = 1};
	SignedPerm after_01 = {.image = {2, 0, 1, 3, 4, 5}, .sign = 1};

	CHECK(signed_perm_equal(signed_perm_compose(swap_01, swap_12), after_12));
	CHECK(signed_perm_equal(signed_perm_compose(swap_12, swap_01), after_01));

	return fails;
}

static size_t test_symmetry_read(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	Generators gens = {.nbr_generator = 0};

	CHECK(symmetry_check_and_read(&gens, ctx, "", 2));
	CHECK(gens.nbr_generator == 0);

	CHECK(symmetry_check_and_read(&gens, ctx, "sym(0,1)", 2));
	CHECK(gens.nbr_generator == 1);
	CHECK(signed_perm_equal(gens.generators[0], signed_perm_swap(0, 1, 1)));

	CHECK(symmetry_check_and_read(&gens, ctx, "antisym(0,1,2)", 3));
	CHECK(gens.nbr_generator == 2);
	CHECK(signed_perm_equal(gens.generators[0], signed_perm_swap(0, 1, -1)));
	CHECK(signed_perm_equal(gens.generators[1], signed_perm_swap(1, 2, -1)));

	CHECK(symmetry_check_and_read(&gens, ctx, " sym(0,1)  antisym(2,3) ", 4));
	CHECK(gens.nbr_generator == 2);
	CHECK(signed_perm_equal(gens.generators[0], signed_perm_swap(0, 1, 1)));
	CHECK(signed_perm_equal(gens.generators[1], signed_perm_swap(2, 3, -1)));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

static bool fails_to_read(Context* ctx, const char* symmetry, u8 nbr_slot) {
	Generators gens = {.nbr_generator = 0};
	bool failed = !symmetry_check_and_read(&gens, ctx, symmetry, nbr_slot) &&
				  !ctx_ok(ctx);

	ctx_clear_err(ctx);

	return failed;
}

static size_t test_symmetry_bad_strings(void) {
	size_t fails = 0;

	Context* ctx = ctx_create();
	if (!ctx)
		return 1;

	CHECK(fails_to_read(ctx, "sym(0)", 2));
	CHECK(fails_to_read(ctx, "sym()", 2));
	CHECK(fails_to_read(ctx, "sym(0,2)", 2));
	CHECK(fails_to_read(ctx, "sym(0,0)", 2));
	CHECK(fails_to_read(ctx, "sym(0,1) sym(1,2)", 3));
	CHECK(fails_to_read(ctx, "sym(0,1", 2));
	CHECK(fails_to_read(ctx, "sym(0;1)", 2));
	CHECK(fails_to_read(ctx, "sym(0,12)", 3));
	CHECK(fails_to_read(ctx, "foo(0,1)", 2));
	CHECK(fails_to_read(ctx, "sym(0,1)sym(2,3)", 4));

	CHECK(ctx_ok(ctx));

	ctx_destroy(ctx);

	return fails;
}

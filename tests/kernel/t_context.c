#include <stddef.h>

#include <ccae/kernel/context.h>

#include "t_utils.h"

static size_t test_context_create(void);

size_t test_context(void) {
	size_t fails = 0;
	fails += test_context_create();

	return fails;
}

static size_t test_context_create(void) {
	constexpr size_t NBR_CONTEXTS = 1'000;
	size_t fails = 0;

	size_t failed_creates = 0;
	for (size_t i = 0; i < NBR_CONTEXTS; ++i) {
		Context* ctx = ctx_create();
		if (!ctx) {
			++failed_creates;
			continue;
		}

		ctx_destroy(ctx);
	}

	CHECK(failed_creates == 0);

	return fails;
}

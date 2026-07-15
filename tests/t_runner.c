#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "t_registry.h"

static bool is_selected(const char* name, int argc, char** argv) {
	if (argc < 2)
		return true;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], name) == 0)
			return true;
	}

	return false;
}

int main(int argc, char** argv) {
	size_t total = 0;

	for (size_t i = 0; i < NBR_TESTS; ++i) {
		if (!is_selected(tests[i].name, argc, argv))
			continue;

		size_t fails = tests[i].fn();
		printf("%s: %zu failed\n", tests[i].name, fails);

		// Failures go to stderr: flush so the two streams stay in order.
		// thanks GPT for the debug there :).
		fflush(stdout);

		total += fails;
	}

	printf("Total: %zu failed\n", total);

	return total ? EXIT_FAILURE : EXIT_SUCCESS;
}

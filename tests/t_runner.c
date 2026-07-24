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

static void print_suite_line(const char* name, size_t fails) {
	constexpr size_t NAME_WIDTH = 16;

	printf("%s ", name);

	for (size_t length = strlen(name) + 1; length < NAME_WIDTH; ++length)
		putchar('.');

	if (fails)
		printf(" %zu failed\n", fails);
	else
		printf(" ok\n");
}

int main(int argc, char** argv) {
	size_t total = 0;
	size_t nbr_run = 0;
	size_t nbr_failed = 0;

	for (size_t i = 0; i < NBR_TESTS; ++i) {
		if (!is_selected(tests[i].name, argc, argv))
			continue;

		size_t fails = tests[i].fn();
		print_suite_line(tests[i].name, fails);

		++nbr_run;
		if (fails)
			++nbr_failed;

		// Failures go to stderr: flush so the two streams stay in order.
		// thanks GPT for the debug there :).
		fflush(stdout);

		total += fails;
	}

	printf("\n%zu suites: %zu passed, %zu failed", nbr_run,
		   nbr_run - nbr_failed, nbr_failed);

	if (total)
		printf(" (%zu checks)", total);

	printf("\n");

	return total ? EXIT_FAILURE : EXIT_SUCCESS;
}

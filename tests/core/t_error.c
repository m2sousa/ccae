#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ccae/config.h>
#include <ccae/core/error.h>

#include "t_utils.h"

static size_t test_error_set(void);
static size_t test_error_truncate(void);
static size_t test_error_clear(void);

size_t test_error(void) {
	size_t fails = 0;
	fails += test_error_set();
	fails += test_error_truncate();
	fails += test_error_clear();

	return fails;
}

static size_t test_error_set(void) {
	size_t fails = 0;

	Error err = {0};
	err_set(&err, CCAE_INVALID_ARG, "bad %s at %d.", "slot", 3);

	CHECK(err.code == CCAE_INVALID_ARG);
	CHECK(strcmp(err.msg, "bad slot at 3.") == 0);

	return fails;
}

static size_t test_error_truncate(void) {
	size_t fails = 0;

	char long_msg[2 * CCAE_ERR_MSG_LENGTH];
	memset(long_msg, 'x', sizeof(long_msg) - 1);
	long_msg[sizeof(long_msg) - 1] = '\0';

	Error err = {0};
	err_set(&err, CCAE_INVALID_ARG, "%s", long_msg);

	CHECK(strlen(err.msg) == CCAE_ERR_MSG_LENGTH - 1);

	return fails;
}

static size_t test_error_clear(void) {
	size_t fails = 0;

	Error err = {0};
	err_set(&err, CCAE_INVALID_ARG, "err.");
	err_clear(&err);

	CHECK(err.code == CCAE_OK);
	CHECK(err.msg[0] == '\0');

	return fails;
}

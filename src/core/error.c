#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <ccae/core/error.h>

void err_set(Error* err, ErrorCode code, const char* fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vsnprintf(err->msg, sizeof(err->msg), fmt, args);
	va_end(args);

	err->code = code;
}

void err_clear(Error* err) {
	Error none = {.code = CCAE_OK, .msg = ""};
	*err = none;
}

void die(const char* msg) {
	fprintf(stderr, "%s\n", msg);
	abort();
}

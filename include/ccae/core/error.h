#ifndef CCAE_ERROR_H
#define CCAE_ERROR_H

#include <ccae/config.h>

typedef enum {
	CCAE_OK = 0,
	CCAE_INVALID_ARG,
} ErrorCode;

typedef struct {
	ErrorCode code;
	char msg[CCAE_ERR_MSG_LENGTH];
} Error;

/* Sets `err` to `code` with a printf-style message, truncated to fit.
 *
 * Example:
 *		`err_set(&err, CCAE_INVALID_ARG, "unknown bundle '%s'.", name);` */
void err_set(Error* err, ErrorCode code, const char* fmt, ...);

/* Resets `err` to `CCAE_OK` with an empty message.
 *
 * Example:
 *		`err_clear(&err);` */
void err_clear(Error* err);

/* Prints `msg` to `stderr` and aborts. For broken invariants only: bad input
 * goes through `err_set`.
 *
 * Example:
 *		`die("CCAE (vec_append): capacity overflow.");` */
[[noreturn]] void die(const char* msg);

#endif

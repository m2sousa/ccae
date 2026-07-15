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

/* Set `err` to `code`, with a printf-style message. A message longer than
 * `CCAE_ERR_MSG_LENGTH - 1` characters is truncated.
 *
 * Example:
 *		`err_set(&err, CCAE_INVALID_ARG, "unknown bundle '%s'.", name);` */
void err_set(Error* err, ErrorCode code, const char* fmt, ...);

/* Reset `err` to "no error": `{.code = CCAE_OK, .msg = ""}`.
 *
 * Example:
 *		`err_clear(&err);` */
void err_clear(Error* err);

/* Print `msg` to `stderr` and `abort()`. Only for broken invariants, never for
 * bad input (use `err_set` for that).
 *
 * Example:
 *		`die("CCAE (vec_append): capacity overflow.");` */
[[noreturn]] void die(const char* msg);

#endif

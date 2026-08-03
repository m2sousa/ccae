#ifndef CCAE_PRINT_BUFFER_H
#define CCAE_PRINT_BUFFER_H

#include <stddef.h>

#include <ccae/types.h>

typedef struct {
	char* buf;
	size_t size;
	size_t length;
} PrintBuffer;

void put_char(PrintBuffer* out, char c);
void put_string(PrintBuffer* out, const char* s);
void put_u64(PrintBuffer* out, u64 n);

#endif

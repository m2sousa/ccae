#ifndef CCAE_CONFIG_H
#define CCAE_CONFIG_H

#include <stddef.h>

constexpr size_t CCAE_ERR_MSG_LENGTH = 256;
constexpr size_t CCAE_VEC_BASE_CAP = 64;

// NOTE: This is purely empiric, perf/alloc tests must be done at some point.
constexpr size_t CCAE_ARENA_NODE_CAP = 4'194'304;

#endif

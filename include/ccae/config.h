#ifndef CCAE_CONFIG_H
#define CCAE_CONFIG_H

#include <stddef.h>

constexpr size_t CCAE_ERR_MSG_LENGTH = 256;
constexpr size_t CCAE_VEC_BASE_CAP = 64;

// Well, the maximum slots I've got during my research is six.. so
// here we are..
constexpr size_t CCAE_MAX_SLOT = 6;

// This represent a fixed number of factor to compose a monomial.
// Therefore going higher in perturbation order would probably mean increasing
// this constant. Evidently, it results in more memory usage.
// Note that, the number of factor (Monomial->nbr_fac) at given time is a u64.
constexpr size_t CCAE_MAX_FACTORS = 128;

// The most indices a monomial can hold: one per slot of each factor.
constexpr size_t CCAE_MAX_INDICES = CCAE_MAX_FACTORS * CCAE_MAX_SLOT;
constexpr size_t CCAE_MAX_DUMMIES = CCAE_MAX_INDICES / 2;

// This is purely empiric, perf/alloc tests must be done at some point.
constexpr size_t CCAE_ARENA_NODE_CAP = 4'194'304;

#endif

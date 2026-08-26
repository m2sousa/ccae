#ifndef CCAE_CONFIG_H
#define CCAE_CONFIG_H

#include <stddef.h>

constexpr size_t CCAE_ERR_MSG_LENGTH = 256;
constexpr size_t CCAE_VEC_BASE_CAP = 64;

// Well, the maximum slots I've got during my research is six.. so
// here we are..
constexpr size_t CCAE_MAX_SLOT = 6;

// A clause of n slots takes n - 1 generators, and clauses share no slot.
constexpr size_t CCAE_MAX_GENERATORS = CCAE_MAX_SLOT - 1;

// The largest orbit is one clause over every slot: CCAE_MAX_SLOT! signed
// permutations.
constexpr size_t CCAE_MAX_ORBIT = 720;
static_assert(CCAE_MAX_SLOT == 6 && CCAE_MAX_ORBIT == 720,
			  "CCAE_MAX_ORBIT must be CCAE_MAX_SLOT!.");

// The most derivative indices on one factor.
constexpr size_t CCAE_MAX_DERIVATIVES = 8;

// This represent a fixed number of factor to compose a monomial.
// Therefore going higher in perturbation order would probably mean increasing
// this constant. Evidently, it results in more memory usage.
// Note that, the number of factor (Monomial->nbr_fac) at given time is a u64.
constexpr size_t CCAE_MAX_FACTORS = 128;

// The most indices one factor holds: its slots, then its derivative indices.
constexpr size_t CCAE_MAX_FACTOR_INDICES = CCAE_MAX_SLOT + CCAE_MAX_DERIVATIVES;

// Each pass of normalisation removes a factor or stops. This is a cap on steps:
// two rules might undo each other, we avoid this via this constant.
constexpr size_t CCAE_MAX_NORMALISE_PASSES = 2 * CCAE_MAX_FACTORS;

// The most terms the expansions on an expression might add through normalise.
constexpr size_t CCAE_MAX_NORMALISED_TERMS = 4096;

// The most indices a monomial can hold: one per slot and per derivative index
// of each factor.
constexpr size_t CCAE_MAX_INDICES = CCAE_MAX_FACTORS * CCAE_MAX_FACTOR_INDICES;

// This is purely empiric, perf/alloc tests must be done at some point.
constexpr size_t CCAE_ARENA_NODE_CAP = 4'194'304;

#endif

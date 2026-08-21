#include <string.h>

#include <ccae/config.h>
#include <ccae/core/arena.h>
#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>
#include <ccae/kernel/symmetry.h>

static bool check_and_read_slotspec(Symbol* out, Context* ctx,
									const char* slotspec) {
	for (const char* s = slotspec; *s; s += 2) {
		const char mark = s[0];
		const char index_name = s[1];

		if (mark != '^' && mark != '_') {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"slotspec '%s': expected '^' or '_', got '%c'.", slotspec,
					mark);
			return false;
		}

		BundleRef bundle_ref = bundle_find_by_index_name(ctx, index_name);
		if (!bundle_ref.val) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"slotspec '%s': no bundle has an index name after '%c'.",
					slotspec, mark);
			return false;
		}

		if (out->nbr_slot == CCAE_MAX_SLOT) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"slotspec '%s': more than %zu slots.", slotspec,
					CCAE_MAX_SLOT);
			return false;
		}

		size_t nbr_slot_in_bundle = 0;
		for (u8 prev = 0; prev < out->nbr_slot; ++prev) {
			if (out->slot_bundle[prev].val == bundle_ref.val)
				++nbr_slot_in_bundle;
		}

		Bundle bundle = vec_at(ctx->bundles, bundle_ref.val);
		if (nbr_slot_in_bundle >= strlen(sp_get(ctx->sp, bundle.index_names))) {
			err_set(
				&ctx->err, CCAE_INVALID_ARG,
				"slotspec '%s': more slots than index names in bundle '%s'.",
				slotspec, sp_get(ctx->sp, bundle.name));
			return false;
		}

		out->slot_bundle[out->nbr_slot] = bundle_ref;
		out->slot_pos[out->nbr_slot] = mark == '^' ? SLOT_UP : SLOT_DOWN;
		++out->nbr_slot;
	}

	return true;
}

static bool reject_flag(Context* ctx, const char* name, const char* problem) {
	err_set(&ctx->err, CCAE_INVALID_ARG, "symbol '%s': %s.", name, problem);

	return false;
}

static bool check_flags(Context* ctx, const char* name, const Symbol* symbol) {
	constexpr u16 ALL_FLAGS = F_CONST_IN_SPACE | F_PHASE_SPACE | F_POSITIVE |
							  F_NONZERO | F_ZERO_MODE_FREE;

	u16 flags = symbol->flags;

	if (flags & ~ALL_FLAGS)
		return reject_flag(ctx, name, "unknown flag");

	if ((flags & F_POSITIVE) && symbol->nbr_slot > 0)
		return reject_flag(ctx, name, "F_POSITIVE on a symbol with slots");

	// Divisions are only done with scalars, no tensors.
	if ((flags & F_NONZERO) && symbol->nbr_slot > 0)
		return reject_flag(ctx, name, "F_NONZERO on a symbol with slots");

	if ((flags & F_PHASE_SPACE) && symbol->kind != SYM_FIELD &&
		symbol->kind != SYM_HOMOG)
		return reject_flag(ctx, name,
						   "F_PHASE_SPACE on a symbol that is neither a field "
						   "nor homogeneous");

	if ((flags & F_CONST_IN_SPACE) && symbol->kind == SYM_FIELD)
		return reject_flag(ctx, name, "F_CONST_IN_SPACE on a field");

	if ((flags & F_ZERO_MODE_FREE) && (flags & F_CONST_IN_SPACE))
		return reject_flag(ctx, name,
						   "F_ZERO_MODE_FREE on a symbol constant in space");

	return true;
}

// A slot symmetry may only exchange slots of the same bundle and position.
static bool check_symmetry_slots(Context* ctx, const char* name,
								 const char* symmetry, const Symbol* symbol) {
	for (u8 g = 0; g < symbol->generators.nbr_generator; ++g) {
		SignedPerm perm = symbol->generators.generators[g];

		for (u8 k = 0; k < symbol->nbr_slot; ++k) {
			u8 target = perm.image[k];

			bool same_bundle =
				symbol->slot_bundle[k].val == symbol->slot_bundle[target].val;
			bool same_pos = symbol->slot_pos[k] == symbol->slot_pos[target];

			if (!same_bundle || !same_pos) {
				err_set(&ctx->err, CCAE_INVALID_ARG,
						"symbol '%s': symmetry '%s' exchanges slots of "
						"different bundles or positions.",
						name, symmetry);
				return false;
			}
		}
	}

	return true;
}

static SymRef declare_symbol(Context* ctx, const char* name, SymKind kind,
							 const char* slotspec, const char* symmetry,
							 u16 flags) {
	SymRef null_ref = {.val = 0};

	if (!*name) {
		err_set(&ctx->err, CCAE_INVALID_ARG, "symbol name is empty.");
		return null_ref;
	}

	StringRef name_ref = sp_intern(ctx->sp, name);

	for (size_t i = 1; i < ctx->symbols.length; ++i) {
		if (vec_at(ctx->symbols, i).name.val == name_ref.val) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"symbol '%s' is already declared.", name);
			return null_ref;
		}
	}

	// If the symbol is a constant, we set the flag that kills it (i.e.
	// F_CONST_IN_SPACE) under spatial derivatives by default.
	if (kind == SYM_CONST)
		flags |= F_CONST_IN_SPACE;

	Symbol symbol = {.name = name_ref, .kind = kind, .flags = flags};

	if (!check_and_read_slotspec(&symbol, ctx, slotspec))
		return null_ref;

	if (!symmetry_check_and_read(&symbol.generators, ctx, symmetry,
								 symbol.nbr_slot))
		return null_ref;

	if (!check_symmetry_slots(ctx, name, symmetry, &symbol) ||
		!check_flags(ctx, name, &symbol))
		return null_ref;

	if (symbol.nbr_slot > 0) {
		Orbit* orbit = arena_alloc(ctx->perm, Orbit);
		if (!orbit)
			die("CCAE (declare_symbol): out of memory.");

		symmetry_build_orbit(orbit, &symbol.generators);
		symbol.orbit = orbit;
	}

	vec_append(ctx->perm, ctx->symbols, symbol);

	SymRef result = {.val = (u32)(ctx->symbols.length - 1)};
	return result;
}

SymRef symbol_declare_tensor(Context* ctx, const char* name, SymKind kind,
							 const char* slotspec, const char* symmetry,
							 u16 flags) {
	SymRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (kind != SYM_FIELD && kind != SYM_TESTFN && kind != SYM_CONST) {
		err_set(&ctx->err, CCAE_INVALID_ARG,
				"symbol '%s': kind not accepted by symbol_declare_tensor.",
				name);
		return null_ref;
	}

	return declare_symbol(ctx, name, kind, slotspec, symmetry, flags);
}

SymRef symbol_declare_homog(Context* ctx, const char* name, u16 flags) {
	SymRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	return declare_symbol(ctx, name, SYM_HOMOG, "", "",
						  (u16)(flags | F_CONST_IN_SPACE));
}

SymRef symbol_declare_const(Context* ctx, const char* name, u16 flags) {
	SymRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	return declare_symbol(ctx, name, SYM_CONST, "", "", flags);
}

void symbol_set_latex(Context* ctx, SymRef sym, const char* latex) {
	if (!ctx_ok(ctx))
		return;

	if (sym.val == 0 || sym.val >= ctx->symbols.length)
		die("CCAE (symbol_set_latex): handle out of range.");

	vec_at(ctx->symbols, sym.val).latex = sp_intern(ctx->sp, latex);
}

static bool reject_inverse(Context* ctx, const Symbol* symbol,
						   const char* problem) {
	err_set(&ctx->err, CCAE_INVALID_ARG, "inverse of symbol '%s': %s.",
			sp_get(ctx->sp, symbol->name), problem);

	return false;
}

static bool check_inverse(Context* ctx, const Symbol* symbol,
						  const Symbol* inverse) {
	if (symbol->nbr_slot != 2 || inverse->nbr_slot != 2)
		return reject_inverse(ctx, symbol, "not two slots each");

	for (u8 k = 0; k < 2; ++k) {
		if (inverse->slot_bundle[k].val != symbol->slot_bundle[k].val)
			return reject_inverse(ctx, symbol, "slots of other bundles");

		if (inverse->slot_pos[k] == symbol->slot_pos[k])
			return reject_inverse(ctx, symbol,
								  "slots not in opposite positions");
	}

	u8 first_dim = vec_at(ctx->bundles, symbol->slot_bundle[0].val).dim;
	u8 second_dim = vec_at(ctx->bundles, symbol->slot_bundle[1].val).dim;
	if (first_dim != second_dim)
		return reject_inverse(ctx, symbol, "bundles of different dimensions");

	if (symbol->inverse.val != 0 || inverse->inverse.val != 0)
		return reject_inverse(ctx, symbol, "already registered");

	return true;
}

void symbol_set_inverse(Context* ctx, SymRef sym, SymRef inverse) {
	if (!ctx_ok(ctx))
		return;

	if (sym.val == 0 || sym.val >= ctx->symbols.length || inverse.val == 0 ||
		inverse.val >= ctx->symbols.length)
		die("CCAE (symbol_set_inverse): handle out of range.");

	Symbol* symbol = &vec_at(ctx->symbols, sym.val);
	Symbol* inverse_symbol = &vec_at(ctx->symbols, inverse.val);

	if (!check_inverse(ctx, symbol, inverse_symbol))
		return;

	symbol->inverse = inverse;
	inverse_symbol->inverse = sym;
}

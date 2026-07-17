#include <ccae/config.h>
#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>

static bool parse_slotspec(Context* ctx, const char* slotspec, Symbol* sym) {
	for (const char* s = slotspec; *s; s += 2) {
		if (*s != '^' && *s != '_') {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"slotspec '%s': expected '^' or '_', got '%c'.", slotspec,
					*s);
			return false;
		}

		BundleRef bundle = bundle_find_by_index_name(ctx, s[1]);
		if (!bundle.val) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"slotspec '%s': no bundle has an index name after '%c'.",
					slotspec, *s);
			return false;
		}

		if (sym->nslot == CCAE_MAX_SLOT) {
			err_set(&ctx->err, CCAE_INVALID_ARG,
					"slotspec '%s': more than %zu slots.", slotspec,
					CCAE_MAX_SLOT);
			return false;
		}

		sym->slot_bundle[sym->nslot] = bundle;
		sym->slot_pos[sym->nslot] = *s == '^' ? SLOT_UP : SLOT_DOWN;
		++sym->nslot;
	}

	return true;
}

static bool reject_flag(Context* ctx, const char* name, const char* problem) {
	err_set(&ctx->err, CCAE_INVALID_ARG, "symbol '%s': %s.", name, problem);

	return false;
}

static bool check_flags(Context* ctx, const char* name, const Symbol* sym) {
	constexpr u16 ALL_FLAGS =
		F_CONST_IN_SPACE | F_PHASE_SPACE | F_POSITIVE | F_ZERO_MODE_FREE;

	u16 flags = sym->flags;

	if (flags & ~ALL_FLAGS)
		return reject_flag(ctx, name, "unknown flag");

	if ((flags & F_POSITIVE) && sym->nslot > 0)
		return reject_flag(ctx, name, "F_POSITIVE on an indexed symbol");

	if ((flags & F_PHASE_SPACE) && sym->kind != SYM_FIELD)
		return reject_flag(ctx, name,
						   "F_PHASE_SPACE on a symbol that is not a field");

	if ((flags & F_CONST_IN_SPACE) && sym->kind == SYM_FIELD)
		return reject_flag(ctx, name, "F_CONST_IN_SPACE on a field");

	if ((flags & F_ZERO_MODE_FREE) && (flags & F_CONST_IN_SPACE))
		return reject_flag(ctx, name,
						   "F_ZERO_MODE_FREE on a symbol constant in space");

	return true;
}

SymRef symbol_declare_tensor(Context* ctx, const char* name, SymKind kind,
							 const char* slotspec, u16 flags) {
	SymRef null_ref = {.val = 0};

	if (!ctx_ok(ctx))
		return null_ref;

	if (!*name) {
		err_set(&ctx->err, CCAE_INVALID_ARG, "symbol name is empty.");
		return null_ref;
	}

	if (kind != SYM_FIELD && kind != SYM_TESTFN && kind != SYM_CONST) {
		err_set(&ctx->err, CCAE_INVALID_ARG, "symbol '%s': unknown kind.",
				name);
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

	Symbol sym = {.name = name_ref, .kind = kind, .flags = flags};

	if (!parse_slotspec(ctx, slotspec, &sym) || !check_flags(ctx, name, &sym))
		return null_ref;

	vec_append(ctx->perm, ctx->symbols, sym);

	SymRef sr = {.val = (u32)(ctx->symbols.length - 1)};
	return sr;
}

void symbol_set_latex(Context* ctx, SymRef sym, const char* latex) {
	if (!ctx_ok(ctx))
		return;

	if (sym.val == 0 || sym.val >= ctx->symbols.length)
		die("CCAE (symbol_set_latex): handle out of range.");

	vec_at(ctx->symbols, sym.val).latex = sp_intern(ctx->sp, latex);
}

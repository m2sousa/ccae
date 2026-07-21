#include <stddef.h>
#include <string.h>

#include <ccae/core/error.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/io/print.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>
#include <ccae/types.h>

typedef struct {
	char* buf;
	size_t size;
	size_t length;
} PrintBuffer;

static const struct {
	SymFlag flag;
	const char* name;
} FLAG_NAMES[] = {
	{F_CONST_IN_SPACE, "CONST_IN_SPACE"},
	{F_PHASE_SPACE, "PHASE_SPACE"},
	{F_POSITIVE, "POSITIVE"},
	{F_ZERO_MODE_FREE, "ZERO_MODE_FREE"},
};

static void put_char(PrintBuffer* out, char c) {
	// Keep the last byte for '\0'.
	if (out->length + 1 >= out->size)
		return;

	out->buf[out->length++] = c;
	out->buf[out->length] = '\0';
}

static void put_string(PrintBuffer* out, const char* s) {
	for (; *s; ++s)
		put_char(out, *s);
}

static const char* kind_name(SymKind kind) {
	switch (kind) {
	case SYM_FIELD:
		return "field";
	case SYM_HOMOG:
		return "homog";
	case SYM_TESTFN:
		return "testfn";
	case SYM_CONST:
		return "const";
	}

	return "?";
}

static char slot_name(const Context* ctx, const Symbol* sym, u8 slot) {
	BundleRef bundle_ref = sym->slot_bundle[slot];

	// Count how many slots/indices from the same bundle are already taken into
	// account in order to get the next one.
	size_t count = 0;
	for (u8 prev = 0; prev < slot; ++prev) {
		if (sym->slot_bundle[prev].val == bundle_ref.val)
			++count;
	}

	Bundle bundle = vec_at(ctx->bundles, bundle_ref.val);
	const char* indices_string = sp_get(ctx->sp, bundle.index_names);

	if (count >= strlen(indices_string))
		die("CCAE (print_symbol): more slots than slots/index names in a "
			"bundle.");

	return indices_string[count];
}

static char slot_sign(const Symbol* sym, u8 slot) {
	return sym->slot_pos[slot] == SLOT_UP ? '^' : '_';
}

static void put_slots_ascii(PrintBuffer* out, const Context* ctx,
							const Symbol* sym) {
	for (u8 slot = 0; slot < sym->nslot; ++slot) {
		put_char(out, slot_sign(sym, slot));
		put_char(out, slot_name(ctx, sym, slot));
	}
}

// Slots with the same position share one group: "_{abc}". A "{}" between
// groups keeps the slot order visible: "^{i}{}_{a}".
static void put_slots_latex(PrintBuffer* out, const Context* ctx,
							const Symbol* sym) {
	for (u8 slot = 0; slot < sym->nslot; ++slot) {
		// If first slot/index or if index is not as the same position (up/down)
		// than the last one, open a group.
		bool must_open_group =
			slot == 0 || sym->slot_pos[slot] != sym->slot_pos[slot - 1];

		if (must_open_group) {
			if (slot > 0)
				put_string(out, "}{}");

			put_char(out, slot_sign(sym, slot));
			put_char(out, '{');
		}

		put_char(out, slot_name(ctx, sym, slot));
	}

	if (sym->nslot > 0)
		put_char(out, '}');
}

static void put_flags(PrintBuffer* out, u16 flags) {
	constexpr size_t NBR_FLAGS = sizeof(FLAG_NAMES) / sizeof(*FLAG_NAMES);

	bool first = true;

	for (size_t i = 0; i < NBR_FLAGS; ++i) {
		if (!(flags & FLAG_NAMES[i].flag))
			continue;

		put_char(out, first ? ' ' : '|');
		put_string(out, FLAG_NAMES[i].name);
		first = false;
	}
}

void print_symbol(const Context* ctx, SymRef sym, PrintMode mode, char* buf,
				  size_t size) {
	if (sym.val == 0 || sym.val >= ctx->symbols.length)
		die("CCAE (print_symbol): handle out of range.");

	if (size == 0)
		return;

	buf[0] = '\0';

	PrintBuffer out = {.buf = buf, .size = size, .length = 0};
	const Symbol* symbol = &vec_at(ctx->symbols, sym.val);

	if (mode == PRINT_LATEX) {
		StringRef head = symbol->latex.val ? symbol->latex : symbol->name;

		put_string(&out, sp_get(ctx->sp, head));
		put_slots_latex(&out, ctx, symbol);
	} else {
		put_string(&out, sp_get(ctx->sp, symbol->name));
		put_slots_ascii(&out, ctx, symbol);
	}

	put_char(&out, ' ');
	put_string(&out, kind_name(symbol->kind));

	put_flags(&out, symbol->flags);
}

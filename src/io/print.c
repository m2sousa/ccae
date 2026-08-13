#include <stddef.h>
#include <string.h>

#include <ccae/core/error.h>
#include <ccae/core/rational.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/io/print.h>
#include <ccae/io/print_buffer.h>
#include <ccae/kernel/bundle.h>
#include <ccae/kernel/context.h>
#include <ccae/kernel/symbol.h>
#include <ccae/types.h>

static const struct {
	SymFlag flag;
	const char* name;
} FLAG_NAMES[] = {
	{F_CONST_IN_SPACE, "CONST_IN_SPACE"},
	{F_PHASE_SPACE, "PHASE_SPACE"},
	{F_POSITIVE, "POSITIVE"},
	{F_NONZERO, "NONZERO"},
	{F_ZERO_MODE_FREE, "ZERO_MODE_FREE"},
};

void put_char(PrintBuffer* out, char c) {
	// Keep the last byte for '\0'.
	if (out->length + 1 >= out->capacity)
		return;

	out->buf[out->length++] = c;
	out->buf[out->length] = '\0';
}

void put_string(PrintBuffer* out, const char* s) {
	for (; *s; ++s)
		put_char(out, *s);
}

void put_u64(PrintBuffer* out, u64 n) {
	constexpr size_t MAX_DIGITS = 20;

	char digits[MAX_DIGITS];
	size_t nbr_digits = 0;

	do {
		digits[nbr_digits++] = (char)('0' + n % 10);
		n /= 10;
	} while (n != 0);

	while (nbr_digits > 0)
		put_char(out, digits[--nbr_digits]);
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

static char slot_index_name(const Context* ctx, const Symbol* symbol, u8 slot) {
	BundleRef bundle_ref = symbol->slot_bundle[slot];

	size_t nbr_slot_in_bundle = 0;
	for (u8 prev = 0; prev < slot; ++prev) {
		if (symbol->slot_bundle[prev].val == bundle_ref.val)
			++nbr_slot_in_bundle;
	}

	Bundle bundle = vec_at(ctx->bundles, bundle_ref.val);
	const char* index_names = sp_get(ctx->sp, bundle.index_names);

	if (nbr_slot_in_bundle >= strlen(index_names))
		die("CCAE (print_symbol): more slots than index names in a "
			"bundle.");

	return index_names[nbr_slot_in_bundle];
}

static char slot_mark(const Symbol* symbol, u8 slot) {
	return symbol->slot_pos[slot] == SLOT_UP ? '^' : '_';
}

static void put_slots_ascii(PrintBuffer* out, const Context* ctx,
							const Symbol* symbol) {
	for (u8 slot = 0; slot < symbol->nbr_slot; ++slot) {
		put_char(out, slot_mark(symbol, slot));
		put_char(out, slot_index_name(ctx, symbol, slot));
	}
}

// Slots with the same position share one group: "_{abc}". A "{}" between
// groups keeps the slot order visible: "^{i}{}_{a}".
static void put_slots_latex(PrintBuffer* out, const Context* ctx,
							const Symbol* symbol) {
	for (u8 slot = 0; slot < symbol->nbr_slot; ++slot) {
		// If first slot or if slot is not at the same position (up/down)
		// as the last one, open a group.
		bool must_open_group =
			slot == 0 || symbol->slot_pos[slot] != symbol->slot_pos[slot - 1];

		if (must_open_group) {
			if (slot > 0)
				put_string(out, "}{}");

			put_char(out, slot_mark(symbol, slot));
			put_char(out, '{');
		}

		put_char(out, slot_index_name(ctx, symbol, slot));
	}

	if (symbol->nbr_slot > 0)
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

static void put_symbol(PrintBuffer* out, const Context* ctx,
					   const Symbol* symbol, PrintMode mode) {
	if (mode == PRINT_LATEX) {
		StringRef shown_name = symbol->latex.val ? symbol->latex : symbol->name;

		put_string(out, sp_get(ctx->sp, shown_name));
		put_slots_latex(out, ctx, symbol);
	} else {
		put_string(out, sp_get(ctx->sp, symbol->name));
		put_slots_ascii(out, ctx, symbol);
	}

	put_char(out, ' ');
	put_string(out, kind_name(symbol->kind));

	put_flags(out, symbol->flags);
}

void print_symbol(char* out, size_t capacity, const Context* ctx, SymRef sym,
				  PrintMode mode) {
	if (sym.val == 0 || sym.val >= ctx->symbols.length)
		die("CCAE (print_symbol): handle out of range.");

	if (capacity == 0)
		return;

	out[0] = '\0';

	PrintBuffer buffer = {.buf = out, .capacity = capacity, .length = 0};
	put_symbol(&buffer, ctx, &vec_at(ctx->symbols, sym.val), mode);
}

void print_symbol_table(char* out, size_t capacity, const Context* ctx,
						PrintMode mode) {
	if (capacity == 0)
		return;

	out[0] = '\0';

	PrintBuffer buffer = {.buf = out, .capacity = capacity, .length = 0};

	for (size_t i = 1; i < ctx->symbols.length; ++i) {
		put_symbol(&buffer, ctx, &vec_at(ctx->symbols, i), mode);
		put_char(&buffer, '\n');
	}
}

void print_rational(char* out, size_t capacity, Rational r, PrintMode mode) {
	if (capacity == 0)
		return;

	out[0] = '\0';

	PrintBuffer buffer = {.buf = out, .capacity = capacity, .length = 0};

	if (r.negative)
		put_char(&buffer, '-');

	if (r.den == 1) {
		put_u64(&buffer, r.num);
	} else if (mode == PRINT_LATEX) {
		put_string(&buffer, "\\frac{");
		put_u64(&buffer, r.num);
		put_string(&buffer, "}{");
		put_u64(&buffer, r.den);
		put_char(&buffer, '}');
	} else {
		put_u64(&buffer, r.num);
		put_char(&buffer, '/');
		put_u64(&buffer, r.den);
	}
}

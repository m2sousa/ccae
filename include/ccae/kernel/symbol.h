#ifndef CCAE_SYMBOL_H
#define CCAE_SYMBOL_H

#include <ccae/config.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/fwd.h>
#include <ccae/types.h>

typedef enum : u8 {
	SYM_FIELD,
	SYM_HOMOG,
	SYM_TESTFN,
	SYM_CONST,
} SymKind;

typedef enum : u16 {
	F_CONST_IN_SPACE = 1 << 0,
	F_PHASE_SPACE = 1 << 1,
	F_POSITIVE = 1 << 2,
	F_ZERO_MODE_FREE = 1 << 3,
} SymFlag;

typedef enum : u8 {
	SLOT_UP,
	SLOT_DOWN,
} SlotPos;

typedef struct {
	StringRef name;
	StringRef latex;
	SymKind kind;
	u16 flags;
	u8 nslot;
	BundleRef slot_bundle[CCAE_MAX_SLOT];
	SlotPos slot_pos[CCAE_MAX_SLOT];
} Symbol;

typedef Vec(Symbol) SymbolVec;

/* Declares a symbol named `name`, of kind `kind` and with `flags` (a set of
 * `SymFlag`s).
 *
 * `slotspec` lists its slots, one `^` (up) or `_` (down) followed by a bundle
 * index name each: "^i_a" is slot 0 internal up, slot 1 spatial down. ""
 * declares a scalar.
 *
 * `kind` is `SYM_FIELD`, `SYM_TESTFN` or `SYM_CONST`; homogeneous symbols go
 * through `symbol_declare_homog`.
 *
 * Sets `ctx->err` and returns the null handle on another kind, a malformed
 * `slotspec`, a flag that does not fit the kind or the slots, a duplicate name,
 * or if `ctx` already holds an error.
 *
 * Example:
 *		`SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", 0);` */
[[nodiscard]] SymRef symbol_declare_tensor(Context* ctx, const char* name,
										   SymKind kind, const char* slotspec,
										   u16 flags);

/* Declares a homogeneous symbol named `name`: a scalar constant in space, such
 * as a background variable of a cosmological model. `F_CONST_IN_SPACE` is
 * always added to `flags`.
 *
 * Sets `ctx->err` and returns the null handle on an empty or duplicate name, a
 * flag that does not fit a homogeneous symbol, or if `ctx` already holds an
 * error.
 *
 * Example:
 *		`SymRef p = symbol_declare_homog(ctx, "p", F_PHASE_SPACE);` */
[[nodiscard]] SymRef symbol_declare_homog(Context* ctx, const char* name,
										  u16 flags);

/* Sets the LaTeX rendering of `sym` to `latex`, replacing any earlier one.
 * Does nothing if `ctx` already holds an error. Dies if `sym` is the null
 * handle or out of range.
 *
 * Example:
 *		`symbol_set_latex(ctx, dK, "\\delta K");` */
void symbol_set_latex(Context* ctx, SymRef sym, const char* latex);

#endif

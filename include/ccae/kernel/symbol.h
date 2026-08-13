#ifndef CCAE_SYMBOL_H
#define CCAE_SYMBOL_H

#include <ccae/config.h>
#include <ccae/core/strpool.h>
#include <ccae/core/vec.h>
#include <ccae/kernel/fwd.h>
#include <ccae/kernel/symmetry.h>
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
	F_NONZERO = 1 << 3,
	F_ZERO_MODE_FREE = 1 << 4,
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
	u8 nbr_slot;
	BundleRef slot_bundle[CCAE_MAX_SLOT];
	SlotPos slot_pos[CCAE_MAX_SLOT];
	Generators generators;
	const Orbit* orbit;
} Symbol;

typedef Vec(Symbol) SymbolVec;

/* Declares a `SYM_FIELD`, `SYM_TESTFN` or `SYM_CONST` symbol. `slotspec` has
 * a `^` (up) or `_` (down) and an index name per slot: "^i_a" is internal up,
 * spatial down; "" is a scalar. `symmetry` is a symmetry string; the slots of
 * a clause must share the same bundle and position.
 * Sets `ctx->err` and returns the null handle on another kind, a bad
 * `slotspec`, `symmetry` or flag, a duplicate name, or an earlier error.
 *
 * Example:
 *		`SymRef g = symbol_declare_tensor(ctx, "g", SYM_FIELD, "_a_b",
 * "sym(0,1)", 0);` */
[[nodiscard]] SymRef symbol_declare_tensor(Context* ctx, const char* name,
										   SymKind kind, const char* slotspec,
										   const char* symmetry, u16 flags);

/* Declares a homogeneous scalar, constant **in space**: `F_CONST_IN_SPACE`
 * is added to `flags`. Fails like `symbol_declare_tensor`.
 *
 * Example:
 *		`SymRef p = symbol_declare_homog(ctx, "p", F_PHASE_SPACE);` */
[[nodiscard]] SymRef symbol_declare_homog(Context* ctx, const char* name,
										  u16 flags);

/* Declares a scalar constant. Fails like `symbol_declare_tensor`.
 *
 * Example:
 *		`SymRef kappa = symbol_declare_const(ctx, "kappa", F_POSITIVE);` */
[[nodiscard]] SymRef symbol_declare_const(Context* ctx, const char* name,
										  u16 flags);

/* Sets the LaTeX rendering of `sym`. Does nothing after an earlier error;
 * dies on a bad handle.
 *
 * Example:
 *		`symbol_set_latex(ctx, dK, "\\delta K");` */
void symbol_set_latex(Context* ctx, SymRef sym, const char* latex);

#endif

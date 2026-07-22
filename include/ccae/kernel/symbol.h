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
	u8 nslot;
	BundleRef slot_bundle[CCAE_MAX_SLOT];
	SlotPos slot_pos[CCAE_MAX_SLOT];
} Symbol;

typedef Vec(Symbol) SymbolVec;

/* Declares a `SYM_FIELD`, `SYM_TESTFN` or `SYM_CONST` symbol. `slotspec` has
 * a `^` (up) or `_` (down) and an index name per slot: "^i_a" is internal up,
 * spatial down; "" is a scalar. Sets `ctx->err` and returns the null handle on
 * another kind, a bad `slotspec` or flag, a duplicate name, or an earlier
 * error.
 *
 * Example:
 *		`SymRef dK = symbol_declare_tensor(ctx, "dK", SYM_FIELD, "^i_a", 0);` */
[[nodiscard]] SymRef symbol_declare_tensor(Context* ctx, const char* name,
										   SymKind kind, const char* slotspec,
										   u16 flags);

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

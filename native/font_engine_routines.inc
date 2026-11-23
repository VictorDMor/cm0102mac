// The font engine (tools/font_engine_audit.py): entry, end, results, digest of the original body, hook.
// FONT_ENGINE_ROUTINE runs in production; FONT_ENGINE_DRAFT only in the check tools. Entries not listed run generated. The
// hooks read the guest ABI's arguments and set r3.
// Each block appends the #include of its font_engine_routines_<block>.inc below.
#include "font_engine_routines_util.inc"
#include "font_engine_routines_stream.inc"
#include "font_engine_routines_tsimem.inc"
#include "font_engine_routines_glyph.inc"
#include "font_engine_routines_t2ktt_head.inc"
#include "font_engine_routines_truetype_tables.inc"
#include "font_engine_routines_style.inc"
#include "font_engine_routines_truetype_glyph.inc"
#include "font_engine_routines_t2kextra.inc"
#include "font_engine_routines_fnt_c.inc"
#include "font_engine_routines_t2ksc.inc"
#include "font_engine_routines_fnt_a.inc"
#include "font_engine_routines_t2k_core.inc"
#include "font_engine_routines_fnt_b.inc"
#include "font_engine_routines_autogrid_b.inc"
#include "font_engine_routines_autogrid_a.inc"
#include "font_engine_routines_ghints.inc"
#include "font_engine_routines_hstrat97.inc"

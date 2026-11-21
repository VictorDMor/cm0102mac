// PLAYER_REGEN.CPP (tools/player_regen_audit.py): entry, end, results, digest of the original body, hook.
// PLAYER_REGEN_ROUTINE runs in production; PLAYER_REGEN_DRAFT only in the check tools. Entries not listed run
// generated. The hooks read the guest ABI's arguments and set r3.
// Each block appends the #include of its player_regen_routines_<block>.inc below.
#include "player_regen_routines_regen_queries.inc"
#include "player_regen_routines_candidate_score.inc"
#include "player_regen_routines_regen_lists.inc"
#include "player_regen_routines_club_placement.inc"
#include "player_regen_routines_detach.inc"
#include "player_regen_routines_retire_decision.inc"
#include "player_regen_routines_persistence.inc"

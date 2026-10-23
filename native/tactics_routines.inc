// The original tactics system's routines that run as C++ (FORMATION.CPP,
// TACTICS.CPP, TACTICS_SCREENS.CPP), one file per group. One line per
// routine:
//   TACTICS_ROUTINE(entry, end, results, digest, hook)  verified; the game runs it
//   TACTICS_DRAFT(entry, end, results, digest, hook)    only check tools run it
// `end` bounds the original body (tools/tactics_audit.py ROUTINES), whose
// instructions the generator pins by `digest` (generation fails if the export
// differs). `results` lists the registers callers read back (R3, R3_R4, F1,
// NONE). `hook` reads the arguments from the registers, runs the C++ routine
// and sets the result registers. Any order; the adapter sorts them.
#include "tactics_routines_formation.inc"
#include "tactics_routines_formation_zones.inc"
#include "tactics_routines_formation_record.inc"
#include "tactics_routines_tactics_teams.inc"
#include "tactics_routines_screens_build.inc"
#include "tactics_routines_screens_main.inc"
#include "tactics_routines_screens_instructions.inc"
#include "tactics_routines_screens_misc.inc"
#include "tactics_routines_leaves.inc"
#include "tactics_routines_tactics_setup.inc"

// The original's SHORTLIST_MANAGER.CPP routines that run as C++ (shortlist_manager.hpp).
// One line per routine:
//   SHORTLIST_MANAGER_ROUTINE(entry, end, results, digest, hook)  verified; the game runs it
//   SHORTLIST_MANAGER_DRAFT(entry, end, results, digest, hook)    only check tools run it
// `end` bounds the original body (tools/shortlist_manager_audit.py ROUTINES), whose
// instructions the generator pins by `digest` (generation fails if the export
// differs). `results` lists the registers callers read back (R3, F1 or NONE).
// `hook` reads the arguments from the registers, runs the C++ routine and
// sets the result registers. Any order; the adapter sorts them. One file per
// group of routines (shortlist_manager_<group>.hpp declares its C++).
#include "shortlist_manager_routines_compare.inc"
#include "shortlist_manager_routines_storage.inc"
#include "shortlist_manager_routines_links.inc"
#include "shortlist_manager_routines_policy.inc"
#include "shortlist_manager_routines_recruitment.inc"
#include "shortlist_manager_routines_assessment.inc"
#include "shortlist_manager_routines_interest.inc"
#include "shortlist_manager_routines_context.inc"
#include "shortlist_manager_routines_candidates.inc"
#include "shortlist_manager_routines_squads.inc"
#include "shortlist_manager_routines_daily.inc"

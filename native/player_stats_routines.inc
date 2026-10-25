// The original's PLAYER_STATS.CPP routines that run as C++ (player_stats.hpp).
// One line per routine:
//   PLAYER_STATS_ROUTINE(entry, end, results, digest, hook)  verified; the game runs it
//   PLAYER_STATS_DRAFT(entry, end, results, digest, hook)    only check tools run it
// `end` bounds the original body (tools/player_stats_audit.py ROUTINES), whose
// instructions the generator pins by `digest` (generation fails if the export
// differs). `results` lists the registers callers read back (R3, F1 or NONE).
// `hook` reads the arguments from the registers, runs the C++ routine and
// sets the result registers. Any order; the adapter sorts them. One file per
// group of routines (player_stats_<group>.hpp declares its C++).
#include "player_stats_routines_relations.inc"
#include "player_stats_routines_queries.inc"
#include "player_stats_routines_text.inc"
#include "player_stats_routines_records.inc"
#include "player_stats_routines_daily.inc"
#include "player_stats_routines_files.inc"

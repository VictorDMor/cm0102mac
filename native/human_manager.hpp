#pragma once
// The original's HUMAN_MANAGER.CPP: the records of the human managers (one 0xa65c-byte record per
// human, indexed by the staff id), their five tactic slots, settings and lists, and their file.
// 62 routines, 0x10145af0..0x1014d560 (tools/human_manager_audit.py). Contract and quirks:
// docs/cpp-rewrites.md.
#include "human_manager_records.hpp"
#include "human_manager_lifecycle.hpp"
#include "human_manager_file.hpp"
#include "human_manager_lists.hpp"
#include "human_manager_schedule.hpp"
#include "human_manager_tactics.hpp"

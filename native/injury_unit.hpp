#pragma once
// The original's INJURY.CPP: injuries and illnesses. The status row each person has, the injury
// records, how a new injury is chosen and how it heals, the news it makes and its form in a saved
// game. 47 routines, 0x101840e0..0x10190180 (tools/injury_unit_audit.py). Contract and quirks:
// docs/cpp-rewrites.md. (native/injury_* without "unit" and actor_attributes are the older
// instruction transcriptions these files replace.)
#include "injury_unit_status.hpp"
#include "injury_unit_onset.hpp"
#include "injury_unit_course.hpp"
#include "injury_unit_file.hpp"
#include "injury_unit_text.hpp"

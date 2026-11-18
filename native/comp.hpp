#pragma once
// COMP.CPP (tools/comp_audit.py): the base competition class. Every league, cup and stage object of the
// game derives from it: its constructor and destructor, the base class's virtual defaults, the walk over a
// competition's stages, the news items and texts every competition shares, the fixture dates, the nation
// descriptors' codes, and two comparators the Argentine leagues sort with. comp_detail.hpp names the
// object's fields. Each block of the module declares its routines in its own header, included at the end.
#include "guest_abi.hpp"

namespace cm::aot::comp {
using guest::Cpu;
// The object (comp_object.cpp). The constructor and destructor return the object, as CodeWarrior's do.
std::uint32_t construct(Cpu&, std::uint32_t competition);                                          // 0x1044fed0
// `deleting` is the r4 word: its low halfword, signed, above 0 frees the object.
std::uint32_t destroy(Cpu&, std::uint32_t competition, std::uint32_t deleting);                   // 0x1044ff60
// Stage `index` (-1 the competition itself); outside -1..last stage, the assertion and 0.
std::uint32_t stage(Cpu&, std::uint32_t competition, std::int32_t index);                         // 0x10450b60
// The descriptor code of the nation the competition's holder names (descriptor::none when none matches).
std::uint32_t descriptor_code(Cpu&, std::uint32_t competition);                                    // 0x10451a30 (comp_regions.cpp)
// The Argentine leagues' sort orders (comp_order.cpp): rows of a word id at +0 and, for the first, a float
// at +4; for the second, four halfword columns at +0xa. Larger values first, then the ids ascending.
std::int32_t compare_by_value(Cpu&, std::uint32_t left, std::uint32_t right);                      // 0x104532c0
std::int32_t compare_by_column(Cpu&, std::uint32_t left, std::uint32_t right);                     // 0x10453300
}
// Blocks append their header here.
#include "comp_stages.hpp"
#include "comp_calendar.hpp"
#include "comp_news.hpp"

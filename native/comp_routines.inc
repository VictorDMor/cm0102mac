// COMP.CPP (tools/comp_audit.py): entry, end, results, digest of the original body, hook.
// COMP_ROUTINE runs in production; COMP_DRAFT only in the check tools. Entries not listed run generated.
// The object (comp_object.cpp).
COMP_ROUTINE(0x1044fed0, 0x1044ff58, R3, "fa83dcbcc185cd2b", [](Cpu& c) { c.set(3, comp::construct(c, c.r(3))); })
COMP_ROUTINE(0x1044ff60, 0x1044ffdc, R3, "70fe51b6c85acf52", [](Cpu& c) { c.set(3, comp::destroy(c, c.r(3), c.r(4))); })
COMP_ROUTINE(0x10450b60, 0x10450c1c, R3, "cd7fdda32069f203", [](Cpu& c) { c.set(3, comp::stage(c, c.r(3), std::int32_t(c.r(4)))); })
// The base class's virtual defaults: `li r3,N; blr` (R3) or `blr` (NONE: r3 stays the object). The comment
// is the slot in the vtable 0x108cf608 (0x10450a00: the unreferenced table 0x108cf690, slot +0xc).
COMP_ROUTINE(0x1044ffe0, 0x1044ffe8, R3, "9ede408bdfe5a611", [](Cpu& c) { c.set(3, 1); })      // +0x0c
COMP_ROUTINE(0x1044fff0, 0x1044fff8, R3, "20fdf3fea3768977", [](Cpu& c) { c.set(3, 0); })      // +0x44
COMP_ROUTINE(0x10450000, 0x10450004, NONE, "689ecda2bae7e35e", [](Cpu&) {})                    // +0x10
COMP_ROUTINE(0x10450010, 0x10450018, R3, "e287e26e5e67809e", [](Cpu& c) { c.set(3, 0); })      // +0x48
COMP_ROUTINE(0x10450020, 0x10450028, R3, "3cbd448fcdea48df", [](Cpu& c) { c.set(3, 0); })      // +0x4c
COMP_ROUTINE(0x10450700, 0x10450704, NONE, "88dbdcab3538ce51", [](Cpu&) {})                    // +0x38
COMP_ROUTINE(0x104509c0, 0x104509c8, R3, "b3433c5754b1225a", [](Cpu& c) { c.set(3, 0); })      // +0x18
COMP_ROUTINE(0x104509d0, 0x104509d8, R3, "b7568200b1e2c1a7", [](Cpu& c) { c.set(3, 0); })      // +0x34
COMP_ROUTINE(0x104509e0, 0x104509e4, NONE, "f1deb3d12dd90992", [](Cpu&) {})                    // +0x30
COMP_ROUTINE(0x104509f0, 0x104509f8, R3, "3bc34d57aea8e9d2", [](Cpu& c) { c.set(3, 1); })      // +0x1c
COMP_ROUTINE(0x10450a00, 0x10450a08, R3, "145cf4adec45baa9", [](Cpu& c) { c.set(3, 1); })      // (0x108cf690) +0x0c
COMP_ROUTINE(0x10450a10, 0x10450a14, NONE, "4119de576746e960", [](Cpu&) {})                    // +0x24
COMP_ROUTINE(0x10450a20, 0x10450a28, R3, "b671f92e8dcb8c2e", [](Cpu& c) { c.set(3, 0); })      // +0x20
COMP_ROUTINE(0x10450a30, 0x10450a34, NONE, "27b7268d05043624", [](Cpu&) {})                    // +0x2c
COMP_ROUTINE(0x10450a40, 0x10450a48, R3, "fff598b06ffd4b9c", [](Cpu& c) { c.set(3, 0); })      // +0x28
COMP_ROUTINE(0x10451c90, 0x10451c98, R3, "39273133149bb5b8", [](Cpu& c) { c.set(3, 2); })      // +0x58
COMP_ROUTINE(0x10451d00, 0x10451d08, R3, "9b9400dc37943eee", [](Cpu& c) { c.set(3, 20); })     // +0x5c
COMP_ROUTINE(0x10453370, 0x10453374, NONE, "b47420bb1ff391d9", [](Cpu&) {})                    // +0x64
COMP_ROUTINE(0x10453380, 0x10453388, R3, "732e6e4d3da0654e", [](Cpu& c) { c.set(3, 1); })      // +0x6c
COMP_ROUTINE(0x10453390, 0x10453394, NONE, "72e2fb2115ecb3de", [](Cpu&) {})                    // +0x74
// The nation descriptors (comp_regions.cpp).
COMP_ROUTINE(0x10451a30, 0x10451ab4, R3, "b72713e66e30f2a1", [](Cpu& c) { c.set(3, comp::descriptor_code(c, c.r(3))); })
// The Argentine leagues' sort orders (comp_order.cpp).
COMP_ROUTINE(0x104532c0, 0x104532f4, R3, "e776dd81df45e611", [](Cpu& c) { c.set(3, std::uint32_t(comp::compare_by_value(c, c.r(3), c.r(4)))); })
COMP_ROUTINE(0x10453300, 0x10453364, R3, "50d2cbcc624518cf", [](Cpu& c) { c.set(3, std::uint32_t(comp::compare_by_column(c, c.r(3), c.r(4)))); })
// Blocks append the #include of their comp_routines_<block>.inc below.
#include "comp_routines_stages.inc"
// The fixture calendar and the nation descriptors (comp_calendar.cpp, comp_regions.cpp).
#include "comp_routines_calendar.inc"
// The news items and their texts (comp_news.cpp, comp_texts*.cpp).
#include "comp_routines_news.inc"

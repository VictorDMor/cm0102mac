// CUP.CPP (tools/cup_audit.py): entry, end, results, digest of the original body, hook.
// CUP_BASE_ROUTINE runs in production; CUP_BASE_DRAFT only in the check tools. Entries not listed run
// generated. The comment after a virtual is its slot in the vtable 0x108d7724.
// The object (cup_base_object.cpp).
CUP_BASE_ROUTINE(0x10478f20, 0x10478f6c, R3, "c9de01d6d1489f5a", [](Cpu& c) { c.set(3, cup_base::construct(c, c.r(3))); })
CUP_BASE_ROUTINE(0x10478f70, 0x10478fcc, R3, "1ad955f4dbb52c37", [](Cpu& c) { c.set(3, cup_base::destroy(c, c.r(3), c.r(4))); })  // +0x08
// The virtuals a derived cup must override: the assertion, then a result.
CUP_BASE_ROUTINE(0x10478fd0, 0x1047905c, R3, "4e970eb5673e83f8", [](Cpu& c) { c.set(3, cup_base::unsupported_44(c)); })  // +0x44
CUP_BASE_ROUTINE(0x10479060, 0x104790ec, R3, "71a2a1c0bb6aeaba", [](Cpu& c) { c.set(3, cup_base::unsupported_94(c)); })  // +0x94
// Leaves on the cup's fields. 0x104790f0 is void: r3 stays the object.
CUP_BASE_ROUTINE(0x104790f0, 0x104790fc, NONE, "8040498c241f7ca8", [](Cpu& c) { cup_base::clear_b0(c, c.r(3)); })  // +0x10
CUP_BASE_ROUTINE(0x10486050, 0x10486094, R3, "721ec79a15072b2f", [](Cpu& c) { c.set(3, cup_base::has_entrant(c, c.r(3), c.r(4))); })  // +0x88
// Sort orders (cup_base_order.cpp).
CUP_BASE_ROUTINE(0x10481e70, 0x10481e88, R3, "00fd360c4c17f4a9", [](Cpu& c) { c.set(3, std::uint32_t(cup_base::compare_by_byte_6(c, c.r(3), c.r(4)))); })
CUP_BASE_ROUTINE(0x10485830, 0x10485868, R3, "b8aec5634b30f1aa", [](Cpu& c) { c.set(3, std::uint32_t(cup_base::compare_by_state_descending(c, c.r(3), c.r(4)))); })
CUP_BASE_ROUTINE(0x10485870, 0x104858a8, R3, "3d746d023267cc32", [](Cpu& c) { c.set(3, std::uint32_t(cup_base::compare_by_state_ascending(c, c.r(3), c.r(4)))); })
// Blocks append the #include of their cup_base_routines_<block>.inc below.
#include "cup_base_routines_draws.inc"
#include "cup_base_routines_storage.inc"
#include "cup_base_routines_result.inc"
#include "cup_base_routines_tree.inc"
#include "cup_base_routines_daily.inc"
#include "cup_base_routines_queries.inc"
#include "cup_base_routines_rounds.inc"

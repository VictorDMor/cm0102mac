#pragma once
// HUMAN_MANAGER.CPP: a human's record and its fields, and whether a human runs a club.
// `humans` is the human-manager table (r3 in the original); `staff` a human's staff record, or
// null for the human whose turn it is. A routine that cannot find the record reports the failed
// check (the game's assertion dialog), clears the shared success flag and returns 0.
#include "human_manager_detail.hpp"

namespace cm::aot::human_manager {
using guest::Cpu;
// Addresses in the record (0 when it cannot be found).
std::uint32_t current_tactic_slot(Cpu&, std::uint32_t humans, std::uint32_t staff);  // 0x101466c0
// Routines that return one address in the current tactic slot, or in the record: the routine, the field and
// its two checks' sites.
struct FieldRoutine { std::uint32_t entry, offset; Checks checks; };
namespace field {
inline constexpr FieldRoutine
    tactic_18e0{0x10146a20, tactic::field_18e0, {0x10146a7c, 0x18a, 0x10146afc, 0x196}},
    tactic_18e4{0x10146b90, tactic::field_18e4, {0x10146bec, 0x1b0, 0x10146c6c, 0x1bc}},
    tactic_18e8{0x10146d00, tactic::field_18e8, {0x10146d5c, 0x1d6, 0x10146ddc, 0x1e2}},
    tactic_18e9{0x10146e70, tactic::field_18e9, {0x10146ecc, 0x1fc, 0x10146f4c, 0x208}},
    tactic_18ea{0x10146fe0, tactic::field_18ea, {0x1014703c, 0x222, 0x101470bc, 0x22e}},
    contracts{0x101487d0, record::contracts, {0x1014882c, 0x4f3, 0x101488ac, 0x4ff}},
    first_offer{0x10148930, record::first_offer, {0x1014898c, 0x519, 0x10148a0c, 0x525}},
    second_offer{0x10148a90, record::second_offer, {0x10148aec, 0x53f, 0x10148b6c, 0x54b}},
    first_name{0x10148eb0, record::first_name, {0x10148f0c, 0x5b5, 0x10148f8c, 0x5c1}},
    second_name{0x10149010, record::second_name, {0x1014906c, 0x5db, 0x101490ec, 0x5e7}},
    screens{0x10149c90, record::screens, {0x10149cec, 0x750, 0x10149d6c, 0x75c}},
    list{0x1014a1a0, record::list, {0x1014a1fc, 0x7e9, 0x1014a27c, 0x7f5}};
}
std::uint32_t current_tactic_field(Cpu&, const FieldRoutine&, std::uint32_t humans, std::uint32_t staff);  // 0x10146a20..0x10146fe0
std::uint32_t record_field(Cpu&, const FieldRoutine&, std::uint32_t humans, std::uint32_t staff);  // 0x101487d0, 0x10148930, ...
// Values.
std::int32_t short_setting(Cpu&, std::uint32_t humans, std::uint32_t staff);                          // 0x10147bc0
void set_short_setting(Cpu&, std::uint32_t humans, std::uint16_t value, std::uint32_t staff);         // 0x10147d20
std::uint8_t away(Cpu&, std::uint32_t humans, std::uint32_t staff);                                   // 0x10147e70
std::uint8_t holiday(Cpu&, std::uint32_t humans, std::uint32_t staff);                                // 0x10149170
std::int8_t runs_club(Cpu&, std::uint32_t humans, std::uint32_t staff);                               // 0x101481d0
std::int8_t runs_nation(Cpu&, std::uint32_t humans, std::uint32_t staff, std::uint32_t refused);     // 0x101484b0
std::uint32_t setting(Cpu&, std::uint32_t humans, std::uint32_t staff);                               // 0x10148bf0
std::uint32_t set_setting(Cpu&, std::uint32_t humans, std::uint32_t value, std::uint32_t staff);      // 0x10148d50
std::uint8_t flag(Cpu&, std::uint32_t humans, std::int8_t which, std::uint32_t staff);                // 0x10149df0
// The folder for a kind of file (1 tactics, 2 search, 3 training, other print), as a path in the
// unit's buffer.
std::uint32_t folder(Cpu&, std::uint32_t humans, std::uint32_t staff, std::int8_t kind);             // 0x10147150
// Whether a human runs the club: 0x1014c2e0 for a given manager (null: the club's own), with the
// absence and option checks it may skip; 0x1014c490 for the human whose turn it is. A national
// side's question goes to the nation's team when the club itself does not settle it.
std::uint32_t runs(Cpu&, std::uint32_t club, std::uint32_t ignore_absence, std::uint32_t assume_options,
                   std::uint32_t manager, std::uint32_t assume_options_too);                           // 0x1014c2e0
std::uint32_t current_human_runs(Cpu&, std::uint32_t club, std::uint32_t ignore_absence,
                                 std::uint32_t assume_options);                                        // 0x1014c490
}

#pragma once
// MATCH_MAN.CPP lineup_b block: offsets and entries the block names. Meanings are
// inferred from how the routines use the fields.
#include <cstdint>

namespace cm::aot::match_man {
// The lineup record the choice fills (r3 of 0x102065f0): a match side, 0xd8 bytes at
// match+0x7aac + side * 0xd8 (match_detail.hpp, match_man_callbacks_detail.hpp).
namespace lineup_side {
constexpr std::uint32_t ratings = 0x0;      // three halfwords copied from the manager's extra record
constexpr std::uint32_t match = 0x8;        // (+0x8, +0xc, +0x10 are cleared)
constexpr std::uint32_t match_object = 0x14;  // the match object
constexpr std::uint32_t slot = 0x1a;        // a byte: 0 home, 1 away
constexpr std::uint32_t kind = 0x1b;        // a byte: 1 when the club is the fixture's side that picks its own lineup
constexpr std::uint32_t mode = 0x1c;        // a byte: 0 with kind 1, else 3
constexpr std::uint32_t manager = 0x20;     // the person choosing
constexpr std::uint32_t manager_extra = 0x24;  // the manager's record at +0x1c
constexpr std::uint32_t players = 0x28;     // the match's player records (0x1c4 each)
constexpr std::uint32_t squad_list = 0x2c;
constexpr std::uint32_t team_slot = 0x30;   // match object + 0x7c6c + slot * 0x18e0
constexpr std::uint32_t club = 0x34;
constexpr std::uint32_t rival = 0x38;       // the opposing club
constexpr std::uint32_t fixture = 0x3c;
constexpr std::uint32_t fixture_state = 0x40;
constexpr std::uint32_t squad_text = 0x44;  // an embedded record handed to the registry (0x103b1b10)
constexpr std::uint32_t team_slot_size = 0x18e0, team_slot_base = 0x7c6c;
}
// The fixture record (r29 of 0x102065f0, the caller's stack parameter at sp+0x38).
namespace lineup_fixture {
constexpr std::uint32_t home = 0x1c;
constexpr std::uint32_t away = 0x20;
constexpr std::uint32_t home_hold = 0x40;   // a byte, tested as signed
constexpr std::uint32_t away_hold = 0x41;
constexpr std::uint32_t bench = 0x4e;       // high nibble + 11 = players counted; low nibble = bench limit
}
// The match object's first byte: non-0 once the match is under way (the choice then only counts).
constexpr std::uint32_t match_started = 0x0;
// The manager's person-like record.
namespace lineup_manager {
constexpr std::uint32_t id = 0x0;           // compared against the people count
constexpr std::uint32_t human_flag = 0x6b;  // a byte, tested as signed
}
// A club.
namespace lineup_club {
constexpr std::uint32_t squad_head = 0x2c;
constexpr std::uint32_t head_of_staff = 0x90;  // a person handed to the discipline/registry queries
}
// A match player (match_man_detail.hpp's match_player::person is +0x6c).
constexpr std::uint32_t person_id = 0x0;
// Routines of other modules the choice calls.
namespace lineup_entries {
constexpr std::uint32_t club_is_computer = 0x100c96e0, club_state = 0x1014c2e0, club_flag = 0x100ce250,
                        club_squad_head = 0x100c76c0, club_check_a = 0x100c7d00, registry_a = 0x10146830,
                        registry_b = 0x101466c0, copy = 0x10739ef0, registry_c = 0x101481d0,
                        registry_d = 0x101484b0, next_manager = 0x1014c5f0, register_squad = 0x103b1b10,
                        exit_game = 0x1076f7d0, random = 0x1077b770;
constexpr std::uint32_t prepare_squad_actors = 0x10206d80, computer_lineup = 0x10206ea0;
}
// The pool of doubles at TOC -0x5278 the random scaler reads.
namespace random_pool {
constexpr std::uint32_t conversion = 0x90;  // 2^52 + 2^31
constexpr std::uint32_t divisor = 0x98;     // the scale 65535 (inferred)
}
}

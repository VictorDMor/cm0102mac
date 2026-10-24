// tactics_teams: routines written by an author, drafts until verified (see tactics_routines.inc).
// TACTICS.CPP: team records, squads, captains, set pieces, backups (tactics.hpp).
TACTICS_ROUTINE(0x103af060, 0x103af130, R3, "fa792cad658f3da0",
    [](Cpu& c) { c.set(3, tactics::destroy(c, c.r(3), c.r(4))); })
TACTICS_ROUTINE(0x103af130, 0x103af480, R3, "0d89f59b2f5268c4",
    [](Cpu& c) { c.set(3, tactics::record_result(c, c.r(3), c.r(4))); })
TACTICS_ROUTINE(0x103af480, 0x103af570, R3, "bf7ccfd68abed85a",
    [](Cpu& c) { c.set(3, tactics::formation_at(c, c.r(3), std::int16_t(c.r(4)), c.r(5))); })
TACTICS_ROUTINE(0x103af570, 0x103af6c0, R3, "26c4cb7451c737c6",
    [](Cpu& c) { c.set(3, tactics::team_formation(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103af6c0, 0x103af800, R3, "57a27800fff2fd75",
    [](Cpu& c) { c.set(3, tactics::set_team_formation(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x103af800, 0x103af9d0, R3, "8af816465b7e63ba",
    [](Cpu& c) { c.set(3, tactics::squad_player(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x103af9d0, 0x103afb90, R3, "a1af563dd3b32066",
    [](Cpu& c) { c.set(3, tactics::set_squad_player(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x103afb90, 0x103afe90, R3, "93123ea867a5f61c",
    [](Cpu& c) { c.set(3, tactics::remove_player(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x103afe90, 0x103afff0, R3, "0b747cf855769f30",
    [](Cpu& c) { c.set(3, tactics::clear_squad_player(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x103afff0, 0x103b04a0, R3, "8c8f7f8fd58047be",
    [](Cpu& c) { c.set(3, tactics::backup_squad(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x103b04a0, 0x103b0540, R3, "138bbbdcd4f9d7cd",
    [](Cpu& c) { c.set(3, tactics::has_backup(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103b0540, 0x103b05c0, NONE, "07f7e538aede6066",
    [](Cpu& c) { tactics::forget_backup(c, c.r(3), c.r(4), c.r(5)); })
TACTICS_ROUTINE(0x103b05c0, 0x103b07c0, R3, "cc32bea885bb4d22",
    [](Cpu& c) { c.set(3, std::uint32_t(tactics::squad_slot_of(c, c.r(3), c.r(4), c.r(5), c.r(6)))); })
TACTICS_ROUTINE(0x103b07c0, 0x103b0a00, R3, "f6ea6396176c871b",
    [](Cpu& c) { c.set(3, tactics::swap_squad_players(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x103b0a00, 0x103b0b20, R3, "c65eab95caf05532",
    [](Cpu& c) { c.set(3, std::uint32_t(tactics::pick_slot(c, c.r(3), c.r(4), c.r(5), c.r(6)))); })
TACTICS_ROUTINE(0x103b0b20, 0x103b0bf0, R3, "a1758221b8f9c197",
    [](Cpu& c) { c.set(3, tactics::pick(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103b0bf0, 0x103b0d40, R3, "29dbc06cb2eb9a75",
    [](Cpu& c) { c.set(3, tactics::set_pick_slot(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x103b0d40, 0x103b0e70, R3, "1a19161172e41c1c",
    [](Cpu& c) { c.set(3, tactics::set_pick(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x103b0e70, 0x103b10f0, R3, "2bc8e1f184a20388",
    [](Cpu& c) { c.set(3, tactics::set_piece_taker(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x103b10f0, 0x103b1370, R3, "1fd50c170615d638",
    [](Cpu& c) { c.set(3, tactics::set_set_piece_taker(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8))); })
TACTICS_ROUTINE(0x103b1370, 0x103b14d0, R3, "ed6906fe57c1d9f5",
    [](Cpu& c) { c.set(3, tactics::captain(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x103b14d0, 0x103b1670, R3, "2ce3f1b5459f9ab7",
    [](Cpu& c) { c.set(3, tactics::set_captain(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x103b1670, 0x103b17b0, R3, "bc79b3ce950df992",
    [](Cpu& c) { c.set(3, tactics::lineup_slot(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x103b17b0, 0x103b1960, R3, "ff1d3dd78e3db2eb",
    [](Cpu& c) { c.set(3, tactics::set_lineup_slot(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x103b1960, 0x103b1b10, R3, "cfde82a1e2f190a3",
    [](Cpu& c) { c.set(3, std::uint32_t(tactics::team_slot(c, c.r(4), c.r(5)))); })
TACTICS_ROUTINE(0x103b1b10, 0x103b1c00, R3, "995a902514d9e268",
    [](Cpu& c) { c.set(3, tactics::copy_team(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x103b1c00, 0x103b1d00, R3, "4e377ac3c994817f",
    [](Cpu& c) { c.set(3, tactics::restore_team(c, c.r(3), c.r(4), c.r(5), c.r(6))); })

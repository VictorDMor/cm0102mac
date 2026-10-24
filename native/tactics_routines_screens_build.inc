// screens_build: routines written by an author, drafts until verified (see tactics_routines.inc).
// TACTICS_SCREENS.CPP: the tactics screen's construction (tactics_screens_build.hpp).
TACTICS_ROUTINE(0x103baa00, 0x103baa70, R3, "a34e9fa4f9de631b",
    [](Cpu& c) { c.set(3, tactics_screens::build::shared_spot(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103baa70, 0x103baae0, R3, "28c9e5cf18a3eaf8",
    [](Cpu& c) { c.set(3, tactics_screens::build::copy_cell(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103bab00, 0x103bab40, R3, "0902288976ce57a1",
    [](Cpu& c) { c.set(3, tactics_screens::build::player_at(c, c.r(3), c.r(4))); })
TACTICS_ROUTINE(0x103bab40, 0x103baba0, R3, "6debe7f04b0ee043",
    [](Cpu& c) { c.set(3, tactics_screens::build::central_in_line(c, c.r(3), c.r(4))); })
TACTICS_ROUTINE(0x103baba0, 0x103babe0, R3, "b6d95f3012f5650c",
    [](Cpu& c) { c.set(3, tactics_screens::build::position(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103babe0, 0x103bac20, R3, "cc13d71704fba6e2",
    [](Cpu& c) { c.set(3, tactics_screens::build::displaced(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103bad10, 0x103bad40, R3, "a46875ca1dc7bb39",
    [](Cpu& c) { c.set(3, tactics_screens::build::not_positions_view(c, c.r(3))); })
TACTICS_ROUTINE(0x103bad40, 0x103bada0, R3, "87680cc9639d9392",
    [](Cpu& c) { c.set(3, tactics_screens::build::free_kick_view(c, c.r(3))); })
TACTICS_ROUTINE(0x103bada0, 0x103badd0, R3, "d3c24770d4733c80",
    [](Cpu& c) { c.set(3, tactics_screens::build::not_positions_view(c, c.r(3))); })
TACTICS_ROUTINE(0x103bade0, 0x103bae10, R3, "8a75cdaaf6902946",
    [](Cpu& c) { c.set(3, tactics_screens::build::formation_locked(c, c.r(3))); })
TACTICS_ROUTINE(0x103bae10, 0x103bae40, R3, "fc50dfa8a12de4af",
    [](Cpu& c) { c.set(3, tactics_screens::build::formation_flag(c, c.r(3))); })
TACTICS_ROUTINE(0x103bae50, 0x103bae80, R3, "4ff4f9eca9ad0e5b",
    [](Cpu& c) { c.set(3, tactics_screens::build::formation_saved(c, c.r(3))); })
TACTICS_ROUTINE(0x103bae90, 0x103baec0, R3, "34b8f5ed26645758",
    [](Cpu& c) { c.set(3, tactics_screens::build::formation_total(c, c.r(3), c.r(4))); })
TACTICS_ROUTINE(0x103baec0, 0x103baee0, R3, "6465cd2e5b9220c6",
    [](Cpu& c) { tactics_screens::build::open_screen(c); })
TACTICS_ROUTINE(0x103c5960, 0x103c5990, R3, "b58ea3fb23ab7ce2",
    [](Cpu& c) { c.set(3, tactics_screens::build::match_side_byte(c, c.r(3), c.r(4))); })
TACTICS_ROUTINE(0x103c5990, 0x103c59c0, R3, "8022f4eead2ae4db",
    [](Cpu& c) { c.set(3, tactics_screens::build::match_away_club(c, c.r(3))); })
TACTICS_ROUTINE(0x103c59c0, 0x103c59f0, R3, "a2b2369ba4f2aec1",
    [](Cpu& c) { c.set(3, tactics_screens::build::match_home_club(c, c.r(3))); })
TACTICS_ROUTINE(0x103c59f0, 0x103c5a40, R3, "05f478cd46ff0a4f",
    [](Cpu& c) { c.set(3, tactics_screens::build::construct_formation(c, c.r(3))); })
TACTICS_ROUTINE(0x103c5a40, 0x103c5a90, R3, "e39774f648e285b1",
    [](Cpu& c) { c.set(3, tactics_screens::build::construct_teams(c, c.r(3))); })
TACTICS_ROUTINE(0x103c5e60, 0x103c5f00, R3, "9291e609b2966de9",
    [](Cpu& c) { c.set(3, tactics_screens::build::select_club(c, c.r(3))); })
TACTICS_ROUTINE(0x103c5f00, 0x103c60a0, R3, "ce42c46133bb31bd",
    [](Cpu& c) { c.set(3, tactics_screens::build::title(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
TACTICS_ROUTINE(0x103c6180, 0x103c6210, R3, "213ea598b74f62d6",
    [](Cpu& c) { c.set(3, tactics_screens::build::selected_entry(c, c.r(3))); })
TACTICS_ROUTINE(0x103c6450, 0x103c64c0, R3, "7a50b62793db23c4",
    [](Cpu& c) { c.set(3, tactics_screens::build::substitutes_left(c, c.r(3))); })
TACTICS_ROUTINE(0x103c6bd0, 0x103c6cb0, R3, "f17ce789cec1faca",
    [](Cpu& c) { c.set(3, tactics_screens::build::kind_is_five(c, c.r(3))); })
TACTICS_ROUTINE(0x103c72c0, 0x103c7380, R3, "13e195194480dbdb",
    [](Cpu& c) { c.set(3, tactics_screens::build::lineup_byte(c, c.r(3), c.r(4))); })
TACTICS_ROUTINE(0x103cea00, 0x103cf110, R3, "174c6468ef0c6ad8",
    [](Cpu& c) { c.set(3, tactics_screens::build::bit_index(c, c.r(3), c.r(4))); })
// --- lists ---
TACTICS_ROUTINE(0x103c78a0, 0x103c79d0, R3, "873152d33742cbf6",
    [](Cpu& c) { c.set(3, tactics_screens::build::list_spot(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103c79d0, 0x103c7ad0, R3, "6e28d18e9e85aa72",
    [](Cpu& c) { c.set(3, tactics_screens::build::shown_spot(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103c8b60, 0x103c8c70, R3, "fbaea126722b0ceb",
    [](Cpu& c) { c.set(3, tactics_screens::build::custom_spot(c, c.r(3), c.r(4))); })
TACTICS_ROUTINE(0x103c9970, 0x103c9aa0, R3, "d15d49d0ae94dd36",
    [](Cpu& c) { c.set(3, tactics_screens::build::entry_unavailable(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103c9aa0, 0x103c9bd0, R3, "18b977451e09db1b",
    [](Cpu& c) { c.set(3, tactics_screens::build::entry_byte_24(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103c9bd0, 0x103c9d00, R3, "fc4aca6b65cdc3a5",
    [](Cpu& c) { c.set(3, tactics_screens::build::entry_byte_23(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103c9d00, 0x103c9f40, R3, "afc1e71a917e66a8",
    [](Cpu& c) { c.set(3, tactics_screens::build::entry_label(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8))); })
TACTICS_ROUTINE(0x103c9f40, 0x103ca120, R3, "223b174794bc13f8",
    [](Cpu& c) { c.set(3, tactics_screens::build::entry_rating(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103ca120, 0x103ca260, R3, "b994a99b194cb669",
    [](Cpu& c) { c.set(3, tactics_screens::build::entry_byte_1b(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103ca260, 0x103ca390, R3, "b14b7a53d00b3a6c",
    [](Cpu& c) { c.set(3, tactics_screens::build::entry_byte_0c(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103ca390, 0x103ca4e0, R3, "d333dd09919e8096",
    [](Cpu& c) { c.set(3, tactics_screens::build::entry_plays_for_match_club(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103ca730, 0x103ca8d0, R3, "51040eb140203982",
    [](Cpu& c) { c.set(3, tactics_screens::build::entry_with_status(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
TACTICS_ROUTINE(0x103cad40, 0x103caf20, R3, "5c2b54168b2ca87c",
    [](Cpu& c) { c.set(3, tactics_screens::build::entry_staff(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103caf20, 0x103cb0b0, R3, "f40cf945cd32c98c",
    [](Cpu& c) { c.set(3, tactics_screens::build::paired_entry(c, c.r(3), c.r(4), c.r(5))); })
TACTICS_ROUTINE(0x103ce330, 0x103ce790, R3, "d1095b230dc0b926",
    [](Cpu& c) { c.set(3, tactics_screens::build::position_point(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10))); })
TACTICS_ROUTINE(0x103ce790, 0x103cea00, R3, "f9ff62d5f01761f7",
    [](Cpu& c) { c.set(3, tactics_screens::build::arrow_head(c, c.r(3), c.r(4), c.r(5), c.r(6), c.f(1), c.r(9), c.r(10))); })
// --- end lists ---
// --- setup ---
TACTICS_ROUTINE(0x103c4b10, 0x103c5940, R3, "62cdea5c9f5859aa",
    [](Cpu& c) { c.set(3, tactics_screens::build::construct(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9))); })
TACTICS_ROUTINE(0x103c7690, 0x103c78a0, R3, "1b25b07c6b9bdba5",
    [](Cpu& c) { c.set(3, tactics_screens::build::pick_lineup(c, c.r(3))); })
TACTICS_ROUTINE(0x103cb0b0, 0x103cb3c0, R3, "71b2f52fa2bb2cd4",
    [](Cpu& c) { c.set(3, tactics_screens::build::list_opponents(c, c.r(3))); })
// --- end setup ---
// --- screen ---
TACTICS_ROUTINE(0x103b3c50, 0x103baa00, R3, "0ad0d329d5072527",
    [](Cpu& c) { tactics_screens::build::build_screen(c); })
// --- end screen ---

// screens_main: TACTICS_SCREENS.CPP, the tactics screen's handler and its edits (tactics_screens_main.hpp).
// Drafts until verified (see tactics_routines.inc).
TACTICS_DRAFT(0x103b3830, 0x103b3b20, R3, "70baec59aff2b609",
    [](Cpu& c) { c.set(3, tactics_screens::main::open(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be420, 0x103be490, R3, "22a25b653bbfa86e",
    [](Cpu& c) { c.set(3, tactics_screens::main::player_at_spot(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be490, 0x103be4e0, R3, "cd4cfc5a8a32b824",
    [](Cpu& c) { c.set(3, tactics_screens::main::control_1c(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be4e0, 0x103be550, R3, "018610cfa78aafef",
    [](Cpu& c) { c.set(3, tactics_screens::main::page_control_1c(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be560, 0x103be5b0, R3, "3e642e1cc96a96e3",
    [](Cpu& c) { c.set(3, tactics_screens::main::control_18(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be5b0, 0x103be620, R3, "199660312e245fa3",
    [](Cpu& c) { c.set(3, tactics_screens::main::page_control_18(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be630, 0x103be680, R3, "9029efd69bfb6f43",
    [](Cpu& c) { c.set(3, tactics_screens::main::control_14(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be680, 0x103be6f0, R3, "3044a905e306c50d",
    [](Cpu& c) { c.set(3, tactics_screens::main::page_control_14(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be700, 0x103be750, R3, "782a1bd20d67aa1b",
    [](Cpu& c) { c.set(3, tactics_screens::main::control_10(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be750, 0x103be7c0, R3, "735578b7fa9a64c9",
    [](Cpu& c) { c.set(3, tactics_screens::main::page_control_10(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be7d0, 0x103be820, R3, "53a37cb99dddb6c1",
    [](Cpu& c) { c.set(3, tactics_screens::main::set_control_60(c, c.r(3), c.r(4), c.r(5))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be820, 0x103be880, R3, "a390bfb33c8fc996",
    [](Cpu& c) { c.set(3, tactics_screens::main::set_page_control_60(c, c.r(3), c.r(4), c.r(5))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be880, 0x103be8c0, R3, "2c0c55ed6d5aa840",
    [](Cpu& c) { c.set(3, tactics_screens::main::copy_control_60(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be8c0, 0x103be910, R3, "b195731d5a6b5a66",
    [](Cpu& c) { c.set(3, tactics_screens::main::control_76(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be910, 0x103be980, R3, "dc722d0d489562ae",
    [](Cpu& c) { c.set(3, tactics_screens::main::page_control_76(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103be9c0, 0x103be9f0, R3, "ba01164378186c92",
    [](Cpu& c) { c.set(3, tactics_screens::main::formation_changed(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103bea00, 0x103bea50, R3, "b5664d04928d745d",
    [](Cpu& c) { c.set(3, tactics_screens::main::captain_selected(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103bea50, 0x103bea90, R3, "3b1a753928414f4b",
    [](Cpu& c) { c.set(3, tactics_screens::main::save_preset(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103bea90, 0x103beae0, R3_R4, "9c5a4a78b8a774a8",
    [](Cpu& c) { const auto r = tactics_screens::main::club_event(c, c.r(3), c.r(4), c.r(5)); c.set({{3, std::uint32_t(r >> 32)}, {4, std::uint32_t(r)}}); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103bebb0, 0x103becb0, R3, "cca7ac31e7483475",
    [](Cpu& c) { tactics_screens::main::open_team_instructions(c, c.r(3)); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c1f30, 0x103c2010, R3, "11394ed276508f10",
    [](Cpu& c) { tactics_screens::main::open_player_instructions(c, c.r(3), c.r(4)); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c5a90, 0x103c5b70, R3, "b4b0f7b444acd8e0",
    [](Cpu& c) { c.set(3, tactics_screens::main::copy_construct(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c60a0, 0x103c6180, R3, "c9ae77bafb283ebc",
    [](Cpu& c) { c.set(3, tactics_screens::main::set_view(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c6210, 0x103c6310, R3, "7d89640d7af5ad04",
    [](Cpu& c) { c.set(3, tactics_screens::main::set_captain(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c6310, 0x103c6450, R3, "53eb1438b2fe48fe",
    [](Cpu& c) { c.set(3, tactics_screens::main::choose_captain(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c64c0, 0x103c6bd0, R3, "dd2f9a361bf5c967",
    [](Cpu& c) { c.set(3, tactics_screens::main::match_changes(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c6cb0, 0x103c6d80, R3, "d13d58d20eb59dc5",
    [](Cpu& c) { c.set(3, tactics_screens::main::set_cell(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c7260, 0x103c72c0, R3, "5fe7c0b0ff53385f",
    [](Cpu& c) { c.set(3, tactics_screens::main::undo(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c7380, 0x103c7550, R3, "87076194c9153f90",
    [](Cpu& c) { c.set(3, tactics_screens::main::set_lineup(c, c.r(3), c.r(4), c.r(5))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c7550, 0x103c7690, R3, "1fa0fcee74367df3",
    [](Cpu& c) { c.set(3, tactics_screens::main::swap_lineup(c, c.r(3), c.r(4), c.r(5))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c7ad0, 0x103c7c10, R3, "6d37f60cb7fa9f43",
    [](Cpu& c) { c.set(3, tactics_screens::main::load(c, c.r(3), c.r(4), c.r(5))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c7c10, 0x103c7df0, R3, "d3a33828c4d2f496",
    [](Cpu& c) { c.set(3, tactics_screens::main::save(c, c.r(3), c.r(4), c.r(5))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c7df0, 0x103c80b0, R3, "bf9ac10ff46bf5ac",
    [](Cpu& c) { c.set(3, tactics_screens::main::set_default(c, c.r(3), c.r(4))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c8160, 0x103c81e0, R3, "2d76b41d89fc4789",
    [](Cpu& c) { c.set(3, tactics_screens::main::unlock(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c81e0, 0x103c82a0, R3, "dce4f3ff364f8e30",
    [](Cpu& c) { c.set(3, tactics_screens::main::lock(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c8360, 0x103c83f0, R3, "7fa403d991cf3e2f",
    [](Cpu& c) { c.set(3, tactics_screens::main::copy_cell(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c83f0, 0x103c8480, R3, "d5570ef69d8b3df9",
    [](Cpu& c) { c.set(3, tactics_screens::main::copy_all_cells(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c8480, 0x103c85b0, R3, "87921eaa211b90f4",
    [](Cpu& c) { c.set(3, tactics_screens::main::paste(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c85b0, 0x103c8680, R3, "7ecde3b35506d249",
    [](Cpu& c) { c.set(3, tactics_screens::main::mirror_cell(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c8680, 0x103c87b0, R3, "c9ab6474e968df60",
    [](Cpu& c) { c.set(3, tactics_screens::main::swap_players(c, c.r(3), c.r(4), c.r(5))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c87b0, 0x103c88c0, R3, "d1bfc804ff84d9a5",
    [](Cpu& c) { c.set(3, tactics_screens::main::set_position(c, c.r(3), c.r(4), c.r(5), c.r(6))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c88c0, 0x103c8a10, R3, "671c76bda5cf02bf",
    [](Cpu& c) { c.set(3, tactics_screens::main::swap_spots(c, c.r(3), c.r(4), c.r(5))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103c8a10, 0x103c8b60, R3, "89ba534785812f6c",
    [](Cpu& c) { c.set(3, tactics_screens::main::move_spot(c, c.r(3), c.r(4), c.r(5))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103ca8d0, 0x103cac10, R3, "fd738a314ca792c4",
    [](Cpu& c) { c.set(3, tactics_screens::main::swap_entries(c, c.r(3), c.r(4), c.r(5), c.r(6))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103cb3c0, 0x103cb760, R3, "1e65100eff653804",
    [](Cpu& c) { c.set(3, tactics_screens::main::substitute(c, c.r(3), c.r(4), c.r(5))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103cba70, 0x103cbb30, R3, "b1c202673d219659",
    [](Cpu& c) { c.set(3, tactics_screens::main::best_player(c, c.r(3))); c.set(0, c.lr()); })
TACTICS_DRAFT(0x103cf110, 0x103cf5f0, R3_R4, "4fbe4745606142df",
    [](Cpu& c) { tactics_screens::main::instruction_bit_registers(c); })
TACTICS_DRAFT(0x103baee0, 0x103be410, R3, "bcb1b123765ccb58",
    [](Cpu& c) { c.set(3, tactics_screens::main::handle(c, c.r(3))); c.set(0, c.lr()); })

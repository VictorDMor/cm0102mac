#include "tactics_screens_instructions.hpp"
#include "formation.hpp"
#include "tactics_leaves.hpp"
#include <utility>

namespace cm::aot::tactics_screens::instructions {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::int32_t pool_slot = -0x4c90;   // TACTICS_SCREENS.CPP's string pool
constexpr std::int32_t game_slot = -0x7bb0;   // -> the game's screen manager

// The game's assert macro at `at` (the first call), with the name and
// extension buffers at sp+0x3d and sp+0x38, as every assert here has them.
void fail(Cpu& c, const Frame& frame, std::uint32_t at, std::uint32_t line) {
    const auto pool = c.toc(pool_slot);
    guest::report_assertion(c, {at, pool, pool + 0x14, pool + 0x21, frame.at(0x3d), frame.at(0x38), line});
}
std::uint32_t index(std::uint32_t value, std::uint32_t size) { return std::uint32_t(std::int32_t(std::int8_t(value))) * size; }

// Memberwise copies, as the compiler wrote them: the fields' bytes, padding
// left alone. Floats go through a register (lfs/stfs), which the generated
// code converts to double and back.
void copy_bytes(Cpu& c, std::uint32_t to, std::uint32_t from, std::uint32_t first, std::uint32_t end) {
    for (auto offset = first; offset < end; ++offset) c.put8(to + offset, c.u8(from + offset));
}
void copy_float(Cpu& c, std::uint32_t to, std::uint32_t from, std::uint32_t offset) {
    c.set_f(0, double(c.f32(from + offset)));
    c.put_f32(to + offset, float(c.f(0)));
}
// 0x10771110: copies `size` bytes (a char array member).
void copy_array(Cpu& c, std::uint32_t to, std::uint32_t from, std::uint32_t size, std::uint32_t back) {
    c.set({{3, to}, {4, from}, {5, size}});
    c.call(0x10771110, back);
}
// Calls of the two-halfword copy leaf 0x103baae0 over [first, end).
void copy_pairs(Cpu& c, std::uint32_t to, std::uint32_t from, std::uint32_t first, std::uint32_t end,
                std::uint32_t back) {
    for (auto offset = first; offset < end; offset += 4) {
        c.set({{3, to + offset}, {4, from + offset}});
        c.link(back);
        tactics_leaf_103baae0(c.s);
    }
}
// Keeps the tactics before an edit (0x103c1340 into the undo copy).
void keep_undo(Cpu& c, std::uint32_t screen, std::uint32_t back) {
    c.set({{3, screen + field::undo}, {4, screen + field::tactics}});
    c.link(back);
    copy_tactics(c, screen + field::undo, screen + field::tactics);
    c.put8(screen + field::undo_valid, 1);
}
}

std::uint32_t club_query(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103baca0, 0x40, {.first_gpr = 31});
    c.set({{31, screen}, {3, c.u32(screen + field::club)}});
    c.call(0x100c7d00, 0x103bacbc);
    return c.r(3);
}

std::uint32_t destroy(Cpu& c, std::uint32_t screen, std::uint32_t dispose) {
    Frame frame(c, 0x103b3b50, 0x40, {.first_gpr = 31});
    c.set(31, screen);
    const auto flag = frame.caller() + 0x1e;  // sth r4 into the caller's frame
    c.put16(flag, dispose);
    compare_unsigned(c, screen, 0);
    if (!screen) return screen;
    std::uint32_t back = 0x103b3b80;
    for (const auto record : {field::saved, field::undo, field::tactics}) {
        c.set({{3, screen + record}, {4, 0xffffffffu}});
        c.link(back);
        destroy_tactics(c, screen + record, 0xffffffffu);
        back += 0xc;
    }
    compare(c, c.i16(flag), 0);
    if (c.i16(flag) > 0) {
        c.set(3, screen);
        c.call(0x107735a0, 0x103b3bac);
    }
    return screen;
}

std::uint32_t destroy_tactics(Cpu& c, std::uint32_t tactics, std::uint32_t dispose) {
    Frame frame(c, 0x103b3bd0, 0x40, {.first_gpr = 31});
    c.set(31, tactics);
    const auto flag = frame.caller() + 0x1e;
    c.put16(flag, dispose);
    compare_unsigned(c, tactics, 0);
    if (!tactics) return tactics;
    // The player entries' array destructor, then the formation's.
    c.set({{3, tactics + field::record::players}, {4, c.toc(-0x6408)}, {5, player_size}, {6, 20}});
    c.call(0x10773420, 0x103b3c04);
    c.set({{3, tactics}, {4, 0xffffffffu}});
    c.call(0x10002070, 0x103b3c10);
    compare(c, c.i16(flag), 0);
    if (c.i16(flag) > 0) {
        c.set(3, tactics);
        c.call(0x107735a0, 0x103b3c24);
    }
    return tactics;
}

std::uint32_t team_options(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c0cc0, 0x40, {});
    c.put32(frame.caller() + 0x18, screen);  // stw r3 into the caller's frame
    c.set(3, screen + field::tactics);
    c.call(0x101d56f0, 0x103c0cdc);
    return c.r(3);
}

void open(Cpu& c) {
    Frame frame(c, 0x103c0cf0, 0x40, {});
    c.link(0x103c0d00);
    build(c);
}

std::uint32_t handle(Cpu& c, std::uint32_t event) {
    Frame frame(c, 0x103c0d10, 0x70, {.first_gpr = 21});
    const auto game = c.toc(game_slot);
    c.set({{30, event}, {31, game}});
    // The screen objects: 0 this screen's copy, 1 the tactics screen's.
    const auto screen_object = [&](std::uint32_t which, std::uint32_t back) {
        c.set({{3, game}, {4, which}});
        c.call(0x1073ef70, back);
        return c.r(3);
    };
    // The control the event concerns, asked again at every test.
    const auto control = [&](std::uint32_t back) {
        c.set({{3, game}, {4, event}});
        c.call(0x10329bf0, back);
        return std::int32_t(c.r(3));
    };
    const auto value = [&](std::uint32_t back) {
        c.set({{3, game}, {4, event}});
        c.call(0x1031acc0, back);
        return c.r(3);
    };
    const auto copy = screen_object(0, 0x103c0d34);
    c.set(29, copy);
    const auto tactics_screen = screen_object(1, 0x103c0d44);
    c.set(21, tactics_screen);
    const auto close = [&](std::uint32_t back) {
        c.set({{3, copy}, {4, 1}});
        c.link(back);
        destroy(c, copy, 1);
        return std::uint32_t(-9);
    };
    if (control(0x103c0d54) == 1) return close(0x103c0d68);  // Cancel
    if (control(0x103c0d7c) == 2) {                          // OK: the edits go back to the tactics screen
        c.set(3, copy);
        c.link(0x103c0d8c);
        tactics_leaf_103b3c40(c.s);
        if (!std::int8_t(c.r(3))) {
            c.set(3, copy);
            c.link(0x103c0da0);
            tactics_leaf_103c1f20(c.s);
            c.set({{3, tactics_screen}, {4, copy}});
            c.link(0x103c0dac);
            copy_screen(c, tactics_screen, copy);
        }
        return close(0x103c0db8);
    }
    if (control(0x103c0dcc) == 3) {                          // a team option
        const auto option = value(0x103c0de0);
        c.set({{3, copy}, {4, option}});
        c.link(0x103c0dec);
        set_team_option(c, copy, option);
        return std::uint32_t(-0xb);
    }
    // Controls 4..13: the first and second taker of set piece (control - 4) / 2;
    // 14, 15: the first and alternative playmaker. Each group's code lies 0x98
    // bytes after the previous one's and keeps the screen in its own register.
    for (std::uint32_t group = 0; group < 6; ++group) {
        const auto base = 0x103c0dfc + 0x98 * group;
        const auto first = std::int32_t(4 + 2 * group);
        if (control(base + 0x4) != first && control(base + 0x18) != first + 1) continue;
        const auto object = screen_object(0, base + 0x2c);
        c.set(27 - group, object);
        const auto order = control(base + 0x3c) == first ? 0u : 1u;
        if (group < 5) {
            const auto staff = value(base + (order ? 0x74 : 0x50));
            c.set({{3, object}, {4, group}, {5, order}, {6, staff}});
            c.link(base + (order ? 0x88 : 0x64));
            set_taker(c, object, group, order, staff);
        } else {
            const auto staff = value(base + (order ? 0x70 : 0x50));
            c.set({{3, object}, {4, order}, {5, staff}});
            c.link(base + (order ? 0x80 : 0x60));
            set_playmaker(c, object, order, staff);
        }
        return std::uint32_t(-0xb);
    }
    // Anything else: the dialog's default handling.
    c.set(3, event);
    c.call(0x102a83a0, 0x103c1184);
    c.set(28, c.r(3));
    compare(c, std::int32_t(c.r(3)), 0);
    return c.r(3);
}

std::uint32_t copy_screen(Cpu& c, std::uint32_t to, std::uint32_t from) {
    Frame frame(c, 0x103c11b0, 0x50, {.first_gpr = 26});
    c.set({{31, to}, {30, from}});
    copy_bytes(c, to, from, 0x0, 0xa);
    copy_bytes(c, to, from, 0xc, 0x11);
    c.set({{3, to + field::teams}, {4, from + field::teams}});
    c.link(0x103c1204);
    copy_teams(c, to + field::teams, from + field::teams);
    c.set({{3, to + field::settings}, {4, from + field::settings}});
    c.link(0x103c1210);
    copy_settings(c, to + field::settings, from + field::settings);
    copy_bytes(c, to, from, 0xe60, 0xe61);
    copy_bytes(c, to, from, 0xe64, 0xe6b);
    copy_bytes(c, to, from, 0xe6c, 0xec1);
    c.s.wr<4>(guest::reg::ctr, 0);  // the bdnz loop over 0xe70..0xec0
    for (const auto [record, back] : {std::pair{field::tactics, 0x103c1278u}, std::pair{field::undo, 0x103c1284u},
                                      std::pair{field::saved, 0x103c1298u}}) {
        c.set({{3, to + record}, {4, from + record}});
        c.link(back);
        copy_tactics(c, to + record, from + record);
    }
    copy_pairs(c, to, from, field::cells, field::spots, 0x103c12b4);
    copy_pairs(c, to, from, field::spots, field::free_kick, 0x103c12e8);
    copy_bytes(c, to, from, field::free_kick, field::undo_valid + 1);
    c.set(4, from + field::free_kick - 4);  // the last pair copied
    compare_unsigned(c, 0, 0);               // the pair loop's last test
    return to;
}

std::uint32_t copy_tactics(Cpu& c, std::uint32_t to, std::uint32_t from) {
    Frame frame(c, 0x103c1340, 0x50, {.first_gpr = 28});
    c.set({{31, to}, {30, from}, {3, to}, {4, from}});
    c.link(0x103c1370);
    copy_formation(c, to, from);
    copy_bytes(c, to, from, 0x18e0, 0x18e2);
    copy_pairs(c, to, from, 0x18e2, 0x18e6, 0x103c138c);
    copy_pairs(c, to, from, 0x18e6, 0x18ea, 0x103c1398);
    copy_bytes(c, to, from, 0x18ea, 0x18eb);
    for (std::uint32_t entry = 0; entry < 20; ++entry) {
        const auto offset = field::record::players + entry * player_size;
        c.set({{29, to + offset}, {28, from + offset}, {3, to + offset}, {4, from + offset}});
        c.link(0x103c13b4);
        copy_player(c, to + offset, from + offset);
    }
    c.set({{29, to + 0x3c3c}, {28, from + 0x3c3c}});
    copy_array(c, to + 0x3c3c, from + 0x3c3c, 9, 0x103c13d8);
    copy_array(c, to + 0x3c45, from + 0x3c45, 9, 0x103c13e8);
    copy_bytes(c, to, from, 0x3c4e, 0x3c4f);
    copy_bytes(c, to, from, 0x3c50, 0x3c84);
    copy_array(c, to + 0x3c84, from + 0x3c84, 0xb, 0x103c1468);
    copy_bytes(c, to, from, field::record::changed, field::record::changed + 1);
    return to;
}

std::uint32_t copy_player(Cpu& c, std::uint32_t to, std::uint32_t from) {
    // Frameless; bytes 0x29, 0x4e, 0x11d and 0x1a2 are padding.
    copy_bytes(c, to, from, 0x0, 0x29);
    copy_bytes(c, to, from, 0x2a, 0x4e);
    copy_float(c, to, from, 0x50);
    copy_float(c, to, from, 0x54);
    copy_bytes(c, to, from, 0x58, 0x5c);
    copy_float(c, to, from, 0x5c);
    copy_bytes(c, to, from, 0x60, 0x74);
    for (std::uint32_t offset = 0x74; offset < 0xfc; offset += 4) copy_float(c, to, from, offset);
    copy_bytes(c, to, from, 0xfc, 0x11d);
    copy_bytes(c, to, from, 0x11e, 0x1a2);
    copy_bytes(c, to, from, 0x1a4, 0x1a8);
    copy_float(c, to, from, 0x1a8);
    copy_bytes(c, to, from, 0x1ac, 0x1c4);
    // The registers the copy leaves: its loop pointers and the last words.
    c.set({{0, std::uint32_t(std::int32_t(c.i16(from + 0x1c2)))}, {5, c.u32(from + 0x1b4)}, {6, from + 0x192},
           {7, to + 0x192}});
    c.s.wr<4>(guest::reg::ctr, 0);  // its bdnz loop's count
    return to;
}

std::uint32_t copy_formation(Cpu& c, std::uint32_t to, std::uint32_t from) {
    Frame frame(c, 0x103c1980, 0x50, {.first_gpr = 28});
    c.set({{31, to}, {30, from}});
    // Name, description and notes; 0x1887 is padding.
    copy_array(c, to, from, 0x33, 0x103c19b4);
    copy_array(c, to + 0x33, from + 0x33, 0x32, 0x103c19c4);
    copy_array(c, to + 0x65, from + 0x65, 0xc8, 0x103c19d4);
    copy_bytes(c, to, from, 0x12e, 0x144);
    copy_array(c, to + 0x144, from + 0x144, 0xb, 0x103c1a14);
    copy_bytes(c, to, from, 0x150, 0x166);
    copy_pairs(c, to, from, 0x166, 0x586, 0x103c1a58);
    c.set({{29, to + 0x586}, {28, from + 0x586}});
    copy_bytes(c, to, from, 0x588, 0x5b8);
    copy_array(c, to + 0x5b8, from + 0x5b8, 0xb, 0x103c1ae4);
    copy_bytes(c, to, from, 0x5c4, 0x1887);
    copy_bytes(c, to, from, 0x1888, formation_size);
    c.set({{4, from + 0x18dc}, {5, to + 0x18dc}});
    c.s.wr<4>(guest::reg::ctr, 0);  // its bdnz loops' count
    return to;
}

std::uint32_t copy_settings(Cpu& c, std::uint32_t to, std::uint32_t from) {
    Frame frame(c, 0x103c1c00, 0x40, {.first_gpr = 30});
    c.set({{31, to}, {30, from}});
    copy_bytes(c, to, from, 0x0, 0x43);
    c.set({{3, to + 0x43}, {4, from + 0x43}});
    c.link(0x103c1d10);
    copy_ten(c, to + 0x43, from + 0x43);
    copy_bytes(c, to, from, 0x4d, 0x50);
    return to;
}

std::uint32_t copy_ten(Cpu& c, std::uint32_t to, std::uint32_t from) {
    copy_bytes(c, to, from, 0, 10);
    return to;
}

std::uint32_t copy_teams(Cpu& c, std::uint32_t to, std::uint32_t from) {
    Frame frame(c, 0x103c1da0, 0x50, {.first_gpr = 28});
    c.set({{31, to}, {30, from}});
    copy_bytes(c, to, from, 0x0, 0x4d);  // 0x4d is padding
    copy_bytes(c, to, from, 0x4e, 0x55);
    // 50 four-byte members (0x10291ba0), the two sides (0x10291830), two more blocks.
    for (std::uint32_t offset = 0x55; offset < 0x11d; offset += 4) {
        c.set({{29, to + offset}, {28, from + offset}, {3, to + offset}, {4, from + offset}});
        c.call(0x10291ba0, 0x103c1eb8);
    }
    c.set({{29, to + 0x11d}, {28, from + 0x11d}});
    const auto member = [&](std::uint32_t offset, std::uint32_t target, std::uint32_t back) {
        c.set({{3, to + offset}, {4, from + offset}});
        c.call(target, back);
    };
    member(0x120, 0x10291830, 0x103c1ed8);
    member(0x784, 0x10291830, 0x103c1ee4);
    member(0xde8, 0x10291800, 0x103c1ef0);
    member(0xdf8, 0x102917d0, 0x103c1efc);
    return to;
}

std::uint32_t taker(Cpu& c, std::uint32_t screen, std::uint32_t kind, std::uint32_t order) {
    Frame frame(c, 0x103c6d80, 0x70, {.first_gpr = 28});
    c.set({{28, screen}, {29, kind}, {30, order}, {31, c.toc(pool_slot)}});
    const auto k = std::int8_t(kind), o = std::int8_t(order);
    if (k < 0 || k > 4 || o < 0 || o > 1) {
        fail(c, frame, 0x103c6df0, 0x15eb);
        return 0;
    }
    compare(c, o, 1);
    return c.u32(screen + field::takers + index(kind, 8) + index(order, 4));
}

std::uint32_t set_taker(Cpu& c, std::uint32_t screen, std::uint32_t kind, std::uint32_t order, std::uint32_t staff) {
    Frame frame(c, 0x103c6e80, 0x80, {.first_gpr = 26});
    c.set({{31, screen}, {29, kind}, {28, order}, {26, staff}, {27, c.toc(pool_slot)}});
    const auto k = std::int8_t(kind), o = std::int8_t(order);
    if (k < 0 || k > 4 || o < 0 || o > 1) {
        fail(c, frame, 0x103c6ee8, 0x1603);
        return 0;
    }
    compare(c, c.i8(screen + field::read_only), 0);
    if (c.i8(screen + field::read_only)) return 0;
    keep_undo(c, screen, 0x103c6f54);
    const auto takers = screen + field::takers + index(kind, 8);
    c.put32(takers + index(order, 4), staff);
    // Clearing the first taker moves the second up (the original searches
    // the later slots, there is only one; its last test stays in CR0).
    compare_unsigned(c, staff, 0);
    if (!staff) {
        compare(c, o, 1);
        if (o == 0) {
            const auto second = c.u32(takers + 4);
            compare_unsigned(c, second, 0);
            if (second) {
                c.put32(takers, second);
                c.put32(takers + 4, 0);
                c.set(4, 0);
            } else {
                compare(c, 2, 2);
            }
        }
    }
    c.put8(screen + field::changed, 1);
    return 1;
}

std::uint32_t playmaker(Cpu& c, std::uint32_t screen, std::uint32_t which) {
    Frame frame(c, 0x103c7050, 0x70, {.first_gpr = 29});
    c.set({{29, screen}, {30, which}, {31, c.toc(pool_slot)}});
    const auto w = std::int8_t(which);
    if (w < 0 || w > 1) {
        fail(c, frame, 0x103c70a0, 0x1632);
        return 0;
    }
    compare(c, w, 1);
    return c.u32(screen + field::playmakers + index(which, 4));
}

std::uint32_t set_playmaker(Cpu& c, std::uint32_t screen, std::uint32_t which, std::uint32_t staff) {
    Frame frame(c, 0x103c7120, 0x70, {.first_gpr = 28});
    c.set({{31, screen}, {30, which}, {28, staff}, {29, c.toc(pool_slot)}});
    const auto w = std::int8_t(which);
    if (w < 0 || w > 1) {
        fail(c, frame, 0x103c7178, 0x1647);
        return 0;
    }
    compare(c, c.i8(screen + field::read_only), 0);
    if (c.i8(screen + field::read_only)) return 0;
    keep_undo(c, screen, 0x103c71e4);
    const auto playmakers = screen + field::playmakers;
    c.put32(playmakers + index(which, 4), staff);
    // Clearing the first playmaker moves the alternative up.
    compare_unsigned(c, staff, 0);
    if (!staff) {
        compare(c, w, 0);
        if (!w) {
            compare_unsigned(c, c.u32(playmakers + 4), 0);
            if (c.u32(playmakers + 4)) {
                c.put32(playmakers, c.u32(playmakers + 4));
                c.put32(playmakers + 4, 0);
            }
        }
    }
    c.put8(screen + field::changed, 1);
    return 1;
}

std::uint32_t set_team_option(Cpu& c, std::uint32_t screen, std::uint32_t option) {
    Frame frame(c, 0x103c80b0, 0x40, {.first_gpr = 30});
    c.set(31, screen);
    const auto spill = frame.caller() + 0x1c;  // stw r4 into the caller's frame
    c.put32(spill, option);
    compare(c, c.i8(screen + field::read_only), 0);
    if (c.i8(screen + field::read_only)) return 0;
    keep_undo(c, screen, 0x103c80f0);
    c.set({{3, screen + field::tactics}, {4, c.u32(spill)}});
    c.call(0x10113b60, 0x103c8108);
    const auto set = c.r(3);
    c.set(30, set);
    compare(c, std::int8_t(set), 0);
    if (!std::int8_t(set)) {
        // Refused: the undo copy goes back.
        c.set({{3, screen + field::tactics}, {4, screen + field::undo}});
        c.link(0x103c8124);
        copy_tactics(c, screen + field::tactics, screen + field::undo);
        c.put8(screen + field::undo_valid, 0);
        return 0;
    }
    c.put8(screen + field::changed, 1);
    return 1;
}
}

namespace cm::aot::tactics_screens::instructions {
namespace {
constexpr std::int32_t staff_slot = -0x7be0;        // -> staff records (0x70 bytes)
constexpr std::int32_t staff_count_slot = -0x7be4;  // -> int: number of staff
constexpr std::int32_t tactics_slot = -0x7a6c;      // -> CTACTICS
constexpr std::int32_t selection_slot = -0x7a98;    // -> the object 0x104ae570 / 0x104add10 ask
// The in-match player entry of a slot: the match's copy for the opponent
// (0x101f9440), else the screen's own.
std::uint32_t match_entry(Cpu& c, std::uint32_t screen, std::uint32_t slot, bool opponent, std::uint32_t back) {
    if (!opponent) return screen + field::players + index(slot, player_size);
    c.set({{3, c.u32(screen + field::match)}, {4, c.u8(screen + field::opponent_side)}, {5, slot}});
    c.call(0x101f9440, back);
    return c.r(3);
}
// A slot of the match lists: our side's, or the opponent's.
std::uint32_t list_slot(Cpu& c, std::uint32_t screen, std::uint32_t slot, bool opponent) {
    const auto side = c.i8(screen + (opponent ? field::opponent_side : field::side));
    return screen + std::uint32_t(std::int32_t(side)) * side_size + index(slot, slot_size) + field::match_lists;
}
// The club: ours, or the opponent.
std::uint32_t club_of(Cpu& c, std::uint32_t screen, bool opponent) {
    return c.u32(screen + (opponent ? field::opponent : field::club));
}
}

std::uint32_t slot_available(Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t out,
                             std::uint32_t opponent) {
    Frame frame(c, 0x103c9810, 0x50, {.first_gpr = 27});
    c.set({{31, screen}, {28, slot}, {30, out}, {27, opponent}});
    const auto s = std::int8_t(slot);
    const bool theirs = std::int8_t(opponent) != 0;
    // Each return leaves CR0 from the original's last test.
    if (s < 0 || s > 19) {
        compare(c, s, s < 0 ? 0 : 19);
        return 0;
    }
    if (theirs && !c.u32(screen + field::opponent)) {
        compare_unsigned(c, 0, 0);
        return 0;
    }
    if (!c.u32(screen + field::match)) {
        // Outside a match only the first eleven play.
        compare(c, s, 11);
        if (s >= 11) return 0;
        compare_unsigned(c, out, 0);
        if (out) c.put8(out, slot);
        return 1;
    }
    const auto entry = match_entry(c, screen, slot, theirs, 0x103c9898);
    c.set(29, entry);
    const auto state = [&](std::uint32_t back) {
        c.set(3, entry);
        c.call(0x101d3b50, back);
        return std::int8_t(c.r(3));
    };
    // A negative state counts only as -2 and only when the match allows it.
    if (const auto first = state(0x103c98b8); first < 0) {
        c.set(3, c.u32(screen + field::match));
        c.link(0x103c98cc);
        tactics_leaf_103c9960(c.s);
        compare(c, std::int8_t(c.r(3)), 0);
        if (std::int8_t(c.r(3))) return 0;
        const auto again = state(0x103c98e0);
        compare(c, again, -2);
        if (again != -2) return 0;
    }
    compare_unsigned(c, out, 0);
    if (out) {
        c.put8(out, std::uint32_t(state(0x103c98fc)));
        compare(c, c.i8(out), -2);
        if (c.i8(out) == -2) {
            c.set(3, entry);
            c.call(0x101e3f00, 0x103c9918);
            c.put8(out, c.r(3));
        }
    }
    return 1;
}

std::uint32_t slot_player(Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t opponent) {
    Frame frame(c, 0x103ca4e0, 0x80, {.first_gpr = 26});
    c.set({{31, screen}, {29, slot}, {27, opponent}, {26, c.toc(pool_slot)}});
    const auto s = std::int8_t(slot);
    const bool theirs = std::int8_t(opponent) != 0;
    if (s < 0 || s > 19) {
        fail(c, frame, 0x103ca52c, 0x1db1);
        return 0;
    }
    // Each return leaves CR0 (and r4) from the original's last test.
    if (theirs && !c.u32(screen + field::opponent)) {
        compare_unsigned(c, 0, 0);
        return 0;
    }
    if (c.i32(screen + field::match_state) > -1) {
        // The match lists name the player; he must still qualify (0x100cbde0).
        const auto entry = list_slot(c, screen, slot, theirs);
        c.set(30, entry);
        compare(c, c.i8(entry + 0x1d), -5);
        if (c.i8(entry + 0x1d) == -5) return 0;
        const auto id = c.i32(entry);
        compare(c, id, 0);
        if (id < 0) return 0;
        c.set(4, std::uint32_t(id));
        compare(c, id, c.i32(c.toc(staff_count_slot)));
        if (id >= c.i32(c.toc(staff_count_slot))) return 0;
        const auto staff = [&] { return c.u32(c.toc(staff_slot)) + std::uint32_t(c.i32(entry)) * 0x70; };
        c.set({{3, staff()}, {4, c.u32(entry + 0x8)}, {5, c.u32(entry + 0xc)}, {6, c.u32(entry + 0x10)},
               {7, entry + 0x14}});
        c.call(0x100cbde0, 0x103ca648);
        compare(c, std::int8_t(c.r(3)), 0);
        if (!std::int8_t(c.r(3))) return 0;
        return staff();
    }
    if (c.u32(screen + field::match)) {
        const auto entry = match_entry(c, screen, slot, theirs, 0x103ca69c);
        c.set({{28, entry}, {3, entry}});
        c.call(0x101d3b50, 0x103ca6bc);
        compare(c, std::int8_t(c.r(3)), -5);
        if (std::int8_t(c.r(3)) == -5) return 0;
        c.set(3, entry);
        c.call(0x101e5070, 0x103ca6d0);
        return c.r(3);
    }
    compare(c, std::int8_t(opponent), 0);
    if (theirs) return c.u32(screen + field::opponent_squad + index(slot, 4));
    const auto tactics = c.u32(c.toc(tactics_slot)), club = c.u32(screen + field::club);
    const auto managed = c.u8(screen + field::managed);
    c.set({{3, tactics}, {4, club}, {5, slot}, {6, managed}});
    // Through the dispatcher until tactics::squad_player's draft leaves CR0
    // as the original does.
    c.call(0x103af800, 0x103ca70c);
    return c.r(3);
}

std::uint32_t slot_number(Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t text, std::uint32_t size,
                          std::uint32_t no_position, std::uint32_t opponent) {
    Frame frame(c, 0x103c8e70, 0xa0, {.first_gpr = 17});
    const auto selection = c.toc(selection_slot), pool = c.toc(pool_slot);
    c.set({{31, screen}, {30, slot}, {29, text}, {17, size}, {26, opponent}, {24, selection}, {28, pool}});
    const auto flag = frame.caller() + 0x2b;  // stb r7 into the caller's frame
    c.put8(flag, no_position);
    const auto s = std::int8_t(slot);
    const bool theirs = std::int8_t(opponent) != 0;
    if (s < 0 || s > 19 || !text || std::int32_t(size) < 10) {
        fail(c, frame, 0x103c8edc, 0x1a47);
        return 0;
    }
    c.put8(text, 0);
    if (theirs && !c.u32(screen + field::opponent)) {
        compare_unsigned(c, 0, 0);
        return 0;
    }
    const auto print = [&](std::uint32_t number, std::uint32_t back) {
        c.set({{3, text}, {4, pool + 0x2c4}, {5, number}});  // "%d"
        c.call(0x1077b720, back);
    };
    const auto slot_number = std::uint32_t(std::int32_t(s) + 1);
    if (c.i32(screen + field::match_state) > -1) {
        // The shirt number in the match lists, else the slot's number.
        const auto entry = list_slot(c, screen, slot, theirs);
        c.set(25, entry);
        if (c.i8(entry + 0x1d) != -5 && c.i8(entry + 0x1c) > 0)
            print(std::uint32_t(std::int32_t(c.i8(entry + 0x1c))), 0x103c8fdc);
        else
            print(slot_number, 0x103c8ff4);
        return 1;
    }
    // The squad number a club gives a player (0x104add10), when it has one.
    const auto squad_number = [&](std::uint32_t staff, unsigned club_register, std::uint32_t back) {
        c.set({{club_register, club_of(c, screen, theirs)}});
        c.set({{3, c.u32(selection)}, {4, staff}, {5, c.r(club_register)}});
        c.call(0x104add10, back);
        return c.r(3);
    };
    const auto print_squad_number = [&](std::uint32_t staff, unsigned club_register, std::uint32_t back) {
        squad_number(staff, club_register, back);
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
        c.call(0x10775540, back + 0x8);
        c.set({{5, c.r(3)}, {3, text}, {4, pool + 0x2c4}});
        c.call(0x1077b720, back + 0x18);
    };
    if (c.i8(screen + field::managed)) {
        print(slot_number, 0x103c9248);
        return 1;
    }
    c.set(22, club_of(c, screen, theirs));
    c.set({{3, c.u32(selection)}, {4, c.r(22)}, {5, 0}});
    c.call(0x104ae570, 0x103c9030);
    if (!std::int8_t(c.r(3))) {
        print(slot_number, 0x103c9248);
        return 1;
    }
    if (c.u32(screen + field::match)) {
        const auto entry = match_entry(c, screen, slot, theirs, 0x103c9064);
        c.set({{23, entry}, {3, entry}});
        c.call(0x101d3b50, 0x103c9084);
        compare(c, std::int8_t(c.r(3)), -5);
        if (std::int8_t(c.r(3)) == -5) return 1;
        const auto staff = c.u32(entry + 0x6c);
        c.set(27, staff);
        compare_unsigned(c, staff, 0);
        if (!staff) return 1;
        if (!std::int8_t(squad_number(staff, 21, 0x103c90c4))) {
            compare(c, 0, 0);
            return 1;
        }
        print_squad_number(staff, 20, 0x103c90f8);
        return 1;
    }
    std::uint32_t staff;
    if (theirs) {
        staff = c.u32(screen + field::opponent_squad + index(slot, 4));
    } else {
        const auto tactics = c.u32(c.toc(tactics_slot)), club = c.u32(screen + field::club);
        const auto managed = c.u8(screen + field::managed);
        c.set({{3, tactics}, {4, club}, {5, slot}, {6, managed}});
        c.call(0x103af800, 0x103c914c);  // see slot_player
        staff = c.r(3);
    }
    c.set(27, staff);
    if (staff && std::int8_t(squad_number(staff, 19, 0x103c9180))) print_squad_number(staff, 18, 0x103c91b4);
    // No number: the position's name for the first eleven, "SB<n>" for the substitutes.
    compare(c, c.i8(text), 0);
    if (c.i8(text)) return 1;
    compare(c, c.i8(flag), 0);
    if (c.i8(flag)) return 1;
    if (s < 11) {
        c.set({{3, screen + field::tactics}, {4, slot}, {5, 0}});
        c.link(0x103c9208);
        const auto position = formation::position(c, screen + field::tactics, s, false);
        c.set({{3, std::uint32_t(position)}, {4, text}, {5, size}, {6, 1}});
        c.call(0x10116c90, 0x103c9218);
    } else {
        c.set({{3, text}, {4, pool + 0x1094}, {5, std::uint32_t(std::int32_t(s) - 10)}});
        c.call(0x10727890, 0x103c9230);
    }
    return 1;
}
}

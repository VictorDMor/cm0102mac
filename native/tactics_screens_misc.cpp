#include "tactics_screens_misc.hpp"
#include "formation.hpp"
#include "formation_zones.hpp"
#include "tactics.hpp"
#include "tactics_leaves.hpp"
#include <compare>
#include <initializer_list>
#include <tuple>

namespace cm::aot::tactics_screens::misc {
using guest::Cpu;
using guest::Frame;
namespace {
// TOC slots of the globals TACTICS_SCREENS.CPP reads.
constexpr std::int32_t pool_slot = -0x4c90;      // TACTICS_SCREENS.CPP's string pool
constexpr std::int32_t game_slot = -0x7bb0;      // -> game state (screen history, controls)
constexpr std::int32_t staff_slot = -0x7be0;     // -> staff records
constexpr std::int32_t tactics_slot = -0x7a6c;   // -> CTACTICS
constexpr std::int32_t settings_slot = -0x7a54;  // -> the object holding the team settings
constexpr std::uint32_t staff_size = 0x70;

// The original's compares into cr0 (LT 8, GT 4, EQ 2, and XER's summary
// overflow). Callers and callees that save the condition register store it
// whole, so each compare the original makes is made here too, in its order.
std::strong_ordering set_cr0(Cpu& c, std::strong_ordering order) {
    const std::uint8_t bits = order < 0 ? 8 : order > 0 ? 4 : 2;
    c.set_cr(0, std::uint8_t(bits | (c.s.r<1>(guest::reg::xer_so) & 1)));
    return order;
}
std::strong_ordering compare(Cpu& c, std::int32_t a, std::int32_t b) { return set_cr0(c, a <=> b); }  // cmpw
std::strong_ordering compare_logical(Cpu& c, std::uint32_t a, std::uint32_t b) { return set_cr0(c, a <=> b); }
// `extsb; cmpwi 0`: whether a byte flag is set.
bool flag(Cpu& c, std::uint32_t value) { return compare(c, std::int8_t(value), 0) != 0; }
// A char argument within [low, high]: `extsb; cmpwi low; blt; extsb; cmpwi high; bgt`.
bool in_range(Cpu& c, std::uint32_t value, std::int32_t low, std::int32_t high) {
    const auto v = std::int8_t(value);
    return compare(c, v, low) >= 0 && compare(c, v, high) <= 0;
}

// The module's assert: "TACTICS_SCREENS.CPP" at the pool's start, the format
// at +0x14, "Error" at +0x21.
void fail(Cpu& c, const Frame& frame, std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
    const auto pool = c.toc(pool_slot);
    guest::report_assertion(c, {at, pool, pool + 0x14, pool + 0x21, frame.at(name), frame.at(extension), line});
}
bool read_only(Cpu& c, std::uint32_t screen) { return flag(c, c.u8(screen + field::read_only)); }
// Copies the edited formation into the undo copy (0x103c1340) and marks it valid.
void keep_undo(Cpu& c, std::uint32_t screen, std::uint32_t back) {
    c.set({{3, screen + field::undo}, {4, screen + field::formation}});
    c.call(0x103c1340, back);
    c.put8(screen + field::undo_valid, 1);
}
}

std::uint32_t in_match(Cpu& c, std::uint32_t screen) {
    // Frameless: r31 goes to the red zone and comes back.
    c.put32(c.sp() - 4, c.r(31));
    return compare_logical(c, c.u32(screen + field::match), 0) != 0;
}

std::uint32_t slot_role(Cpu& c, std::uint32_t screen, std::uint32_t slot) {
    Frame frame(c, 0x103bac20, 0x40, {});
    c.put32(frame.at(0x58), screen);
    c.put8(frame.at(0x5f), slot);
    c.set({{3, screen + field::formation}, {4, c.u8(frame.at(0x5f))}});
    tactics_leaf_101149f0(c.s);
    return c.r(3);
}

std::uint64_t slot_instructions(Cpu& c, std::uint32_t screen, std::uint32_t slot) {
    Frame frame(c, 0x103bac60, 0x40, {});
    c.put32(frame.at(0x58), screen);
    c.put8(frame.at(0x5f), slot);
    c.set({{3, screen + field::formation}, {4, c.u8(frame.at(0x5f))}});
    tactics_leaf_10113e10(c.s);
    return std::uint64_t(c.r(3)) << 32 | c.r(4);
}

std::uint32_t slot_staff(Cpu& c, std::uint32_t screen, std::uint32_t slot) {
    Frame frame(c, 0x103c4830, 0x40, {});
    c.put32(frame.at(0x58), screen);
    c.put8(frame.at(0x5f), slot);
    const auto record = screen + field::formation;
    c.set({{3, record}, {4, c.u8(frame.at(0x5f))}});
    c.link(0x103c4854);
    return formation::staff_record(c, c.r(3), std::int8_t(c.r(4)));
}

namespace {
// The history callbacks: the screen object is the current screen's parameter 0.
// `back` holds the return addresses of the four calls.
void leave(Cpu& c, std::uint32_t entry, bool commit, const std::uint32_t (&back)[4]) {
    Frame frame(c, entry, 0x40, {.first_gpr = 31});
    c.set({{3, c.toc(game_slot)}, {4, 0}});
    c.call(0x1073ef70, back[0]);
    const auto screen = c.r(3);
    c.set(31, screen);
    if (compare_logical(c, screen, 0) == 0) return;
    if (commit) {
        c.set(3, screen);
        c.link(back[1]);
        c.set(3, apply(c, screen));
    }
    c.set({{3, screen}, {4, 1}});
    c.call(0x103b3b50, back[2]);
    c.set({{3, c.toc(game_slot)}, {4, 0}, {5, 0}, {6, 0}});
    c.call(0x1073f1e0, back[3]);
}
}

void leave_read_only(Cpu& c) { leave(c, 0x103beae0, false, {0x103beafc, 0, 0x103beb14, 0x103beb28}); }
void leave_editable(Cpu& c) { leave(c, 0x103beb40, true, {0x103beb5c, 0x103beb70, 0x103beb7c, 0x103beb90}); }

std::uint32_t set_slot_instructions(Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t high,
                                    std::uint32_t low) {
    Frame frame(c, 0x103c82a0, 0x40, {.first_gpr = 30});
    c.set(31, screen);
    c.put8(frame.at(0x5f), slot);
    c.put32(frame.at(0x60), high);
    c.put32(frame.at(0x64), low);
    if (read_only(c, screen)) return 0;
    keep_undo(c, screen, 0x103c82e8);
    c.set({{3, screen + field::formation}, {4, c.u8(frame.at(0x5f))}, {5, c.u32(frame.at(0x60))},
           {6, c.u32(frame.at(0x64))}});
    c.call(0x10113e30, 0x103c8308);
    c.set(30, c.r(3));
    if (!flag(c, c.r(30))) {
        // Refused: the undo copy goes back.
        c.set({{3, screen + field::formation}, {4, screen + field::undo}});
        c.call(0x103c1340, 0x103c8324);
        c.put8(screen + field::undo_valid, 0);
        return 0;
    }
    c.put8(screen + field::changed, 1);
    return 1;
}

std::uint32_t set_slot_role(Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t role) {
    Frame frame(c, 0x103c8c70, 0x70, {.first_gpr = 29});
    c.set({{31, screen}, {29, slot}});
    c.put8(frame.at(0x93), role);
    c.set(30, c.toc(pool_slot));
    if (!in_range(c, slot, 0, 10)) {
        fail(c, frame, 0x103c8cc4, 0x3d, 0x38, 0x19f8);
        return 0;
    }
    if (read_only(c, screen)) return 0;
    keep_undo(c, screen, 0x103c8d30);
    c.set({{3, screen + field::formation}, {4, slot}, {5, c.u8(frame.at(0x93))}});
    c.call(0x10114a00, 0x103c8d4c);
    c.put8(screen + field::changed, 1);
    return 1;
}

std::uint32_t set_slot_staff(Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t staff) {
    Frame frame(c, 0x103c8d80, 0x70, {.first_gpr = 29});
    c.set({{29, screen}, {30, slot}});
    c.put32(frame.at(0x90), staff);
    c.set(31, c.toc(pool_slot));
    if (!in_range(c, slot, 0, 10)) {
        fail(c, frame, 0x103c8dd4, 0x3d, 0x38, 0x1a1c);
        return 0;
    }
    if (read_only(c, screen)) return 0;
    const auto record = screen + field::formation;
    c.set({{3, record}, {4, slot}, {5, c.u32(frame.at(0x90))}});
    c.link(0x103c8e44);
    formation::set_staff(c, c.r(3), std::int8_t(c.r(4)), c.r(5));
    return 1;
}

std::uint32_t slot_in_use(Cpu& c, std::uint32_t screen, std::uint32_t slot) {
    Frame frame(c, 0x103c9720, 0x70, {.first_gpr = 29});
    c.set({{29, screen}, {30, slot}, {31, c.toc(pool_slot)}});
    if (!in_range(c, slot, 0, squad_slots - 1)) {
        fail(c, frame, 0x103c9770, 0x3d, 0x38, 0x1b61);
        return 0;
    }
    if (!flag(c, c.u8(screen + field::limit_squad))) return 1;
    // 11 players plus the substitutes allowed (the nibble), as an int8.
    const auto in_use = std::int8_t((c.u8(screen + field::substitutes) >> 4) + 11);
    c.set(4, std::uint32_t(std::int32_t(std::int8_t(slot))));  // left in r4, as the original's compare
    return compare(c, std::int8_t(slot), in_use) < 0 ? 1 : 0;
}

std::uint32_t match_player(Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t opponent) {
    Frame frame(c, 0x103cac10, 0x70, {.first_gpr = 28});
    c.set({{31, screen}, {30, slot}, {28, opponent}, {29, c.toc(pool_slot)}});
    if (!in_range(c, slot, 0, squad_slots - 1)) {
        fail(c, frame, 0x103cac68, 0x3d, 0x38, 0x1eb8);
        return 0xffffffffu;
    }
    if (flag(c, opponent) && compare_logical(c, c.u32(screen + field::opponent), 0) == 0) return 0xffffffffu;
    const auto match = c.u32(screen + field::match);
    if (compare_logical(c, match, 0) == 0) return 0xffffffffu;
    const bool theirs = flag(c, opponent);
    c.set({{3, match}, {4, c.u8(screen + (theirs ? field::opponent_side : field::side))}, {5, slot}});
    c.call(0x101fd100, theirs ? 0x103cacf8 : 0x103cad0c);
    return c.r(3);
}

std::uint32_t slot_name(Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t text, std::uint32_t size,
                        std::uint32_t no_position, std::uint32_t format, std::uint32_t opponent) {
    Frame frame(c, 0x103c9260, 0x170, {.first_gpr = 17});
    c.set({{31, screen}, {28, slot}, {29, text}, {26, size}, {17, no_position}, {18, format}, {27, opponent},
           {21, c.toc(pool_slot)}});
    if (!in_range(c, slot, 0, squad_slots - 1) || compare_logical(c, text, 0) == 0 ||
        compare(c, std::int32_t(size), 100) < 0) {
        fail(c, frame, 0x103c92cc, 0x3e, 0x39, 0x1aca);
        return 0;
    }
    c.put8(text, 0);
    if (flag(c, opponent) && compare_logical(c, c.u32(screen + field::opponent), 0) == 0) return 0;
    c.set({{3, screen}, {4, slot}, {5, opponent}});
    c.call(0x103ca4e0, 0x103c934c);
    const auto player = c.r(3);
    c.set(23, player);
    // No player: the name of the slot's formation position, unless told not to.
    const auto position_name = [&](std::uint32_t back) -> std::uint32_t {
        if (flag(c, no_position)) return 1;
        c.set({{3, screen}, {4, slot}, {5, frame.at(0x38)}, {6, opponent}});
        c.call(0x103c9810, back);
        if (!flag(c, c.r(3))) return 1;
        c.set({{3, screen + field::formation}, {4, c.u8(frame.at(0x38))}, {5, 0}});
        c.link(back + 0x1c);
        c.set(3, std::uint32_t(formation::position(c, c.r(3), std::int8_t(c.r(4)), std::int8_t(c.r(5)))));
        c.set({{4, text}, {5, size}, {6, 1}});
        c.call(0x10116970, back + 0x2c);
        return c.r(3);
    };
    if (compare(c, c.i32(screen + field::match_state), -1) > 0) {
        // The match lists: each side's squad slots.
        const auto list =
            std::uint32_t(std::int32_t(c.i8(screen + (flag(c, opponent) ? field::opponent_side : field::side))));
        const auto entry = screen + list * side_size + std::uint32_t(std::int32_t(std::int8_t(slot))) * slot_size +
                           field::match_lists;
        c.set(30, entry);
        if (compare(c, c.i8(entry + field::slot_state), -5) == 0) return position_name(0x103c9400);
        c.set({{3, c.u32(entry + field::slot_name)}, {4, c.u32(entry + field::slot_name + 4)},
               {5, c.u32(entry + field::slot_name + 8)}, {6, text}, {7, std::uint32_t(std::int32_t(std::int16_t(size)))},
               {8, format}, {9, c.u32(entry + field::slot_player)}});
        c.call(0x100cb590, 0x103c93dc);
    } else {
        if (compare_logical(c, player, 0) == 0) return position_name(0x103c9478);
        c.set({{3, player}, {4, text}, {5, std::uint32_t(std::int32_t(std::int16_t(size)))}, {6, format}});
        c.call(0x100caeb0, 0x103c9454);
    }
    if (compare_logical(c, format & 0xff, 5) != 0) return 1;
    // Format 5: when another player of the club (its 50 players, then those
    // of the club 0x100ce320 links, once) has the same short name, use
    // format 2 instead.
    auto club = c.u32(screen + (flag(c, opponent) ? field::opponent : field::club));
    c.set({{19, club}, {24, club}, {20, 0}});
    for (bool again = false;; again = true) {
        c.set(25, 0);
        for (std::uint32_t index = 0; compare(c, std::int8_t(index), 50) < 0; c.set(25, ++index)) {
            const auto other = c.u32(club + std::uint32_t(std::int32_t(std::int8_t(index))) * 4 + 0x98);
            c.set(22, other);
            if (compare_logical(c, other, 0) == 0 || compare_logical(c, other, player) == 0) continue;
            const auto name = frame.at(0x5d);
            c.set({{3, other}, {4, name}, {5, 200}, {6, 5}});
            c.call(0x100caeb0, 0x103c9518);
            c.set({{3, name}, {4, text}, {5, 1}, {6, 0}});
            c.call(0x107465f0, 0x103c952c);
            if (compare(c, std::int32_t(c.r(3)), 0) == 0) {
                c.set({{3, screen}, {4, slot}, {5, text}, {6, size}, {7, no_position}, {8, 2}, {9, opponent}});
                c.link(0x103c9554);
                return slot_name(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9));
            }
        }
        c.set({{3, club}, {4, 0}, {5, 1}});
        c.call(0x100ce320, 0x103c9578);
        club = c.r(3);
        c.set(24, club);
        if (compare_logical(c, club, 0) == 0 || flag(c, again)) return 1;
        c.set(20, 1);
    }
}

std::uint32_t slot_label(Cpu& c, std::uint32_t screen, std::uint32_t slot, std::uint32_t text, std::uint32_t size,
                         std::uint32_t format, std::uint32_t opponent) {
    Frame frame(c, 0x103c95b0, 0x210, {.first_gpr = 27});
    c.set({{27, screen}, {29, slot}, {30, text}, {28, opponent}});
    c.put32(frame.at(0x234), size);
    c.put8(frame.at(0x23b), format);
    const auto pool = c.toc(pool_slot);
    c.set(31, pool);
    if (!in_range(c, slot, 0, squad_slots - 1) || compare_logical(c, text, 0) == 0 ||
        compare(c, c.i32(frame.at(0x234)), 100) < 0) {
        fail(c, frame, 0x103c961c, 0x3d, 0x38, 0x1b3f);
        return 0;
    }
    const auto position = frame.at(0x124), name = frame.at(0x5c);
    c.set({{3, screen}, {4, slot}, {5, position}, {6, 200}, {7, 1}, {8, opponent}});
    c.call(0x103c8e70, 0x103c9680);
    c.set({{3, screen}, {4, slot}, {5, name}, {6, 200}, {7, 0}, {8, c.u8(frame.at(0x23b))}, {9, opponent}});
    c.link(0x103c96a0);
    c.set(3, slot_name(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)));
    if (flag(c, c.u8(position)) && flag(c, c.u8(name))) {
        // sprintf: r7-r10 stay as slot_name left them.
        c.set({{3, text}, {4, pool + 0xdca}, {5, position}, {6, name}});
        c.call(0x1077b720, 0x103c96d4);
    } else if (flag(c, c.u8(position))) {
        c.set({{3, text}, {4, position}});
        c.call(0x1077dbf0, 0x103c96f4);
    } else {
        c.set({{3, text}, {4, name}});
        c.call(0x1077dbf0, 0x103c9704);
    }
    return 1;
}

std::uint32_t apply(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c5b70, 0x50, {.first_gpr = 26});
    const auto settings = c.toc(settings_slot), tactics_object = c.toc(tactics_slot);
    c.set({{31, screen}, {27, settings}, {30, tactics_object}});
    if (read_only(c, screen)) return 0;
    const auto formation = screen + field::formation;
    c.set(3, formation);
    c.link(0x103c5bac);
    c.set(3, formation::update_zone_grid(c, c.r(3)));
    if (const auto linked = c.u32(screen + field::linked); compare_logical(c, linked, 0) != 0) {
        c.set({{3, linked}, {4, formation}});
        c.call(0x103c1980, 0x103c5bc4);
    }
    // The team settings: two bytes, two pairs of halfwords, a byte, each
    // written where the settings object's accessor points.
    const auto setting = [&](std::uint32_t accessor, std::uint32_t back) {
        c.set({{3, c.u32(settings)}, {4, 0}});
        c.call(accessor, back);
        return c.r(3);
    };
    const auto byte_setting = [&](std::uint32_t offset, std::uint32_t accessor, std::uint32_t back) {
        c.set(26, c.u8(screen + offset));
        c.put8(setting(accessor, back), c.r(26));
    };
    const auto pair_setting = [&](std::uint32_t offset, std::uint32_t accessor, std::uint32_t back) {
        c.set({{3, setting(accessor, back)}, {4, screen + offset}});
        tactics_leaf_103baae0(c.s);
    };
    byte_setting(field::team_settings, 0x10146d00, 0x103c5bd4);
    byte_setting(field::team_settings + 1, 0x10146e70, 0x103c5be8);
    pair_setting(field::team_settings + 2, 0x10146a20, 0x103c5bf8);
    pair_setting(field::team_settings + 6, 0x10146b90, 0x103c5c0c);
    byte_setting(field::team_settings + 0xa, 0x10146fe0, 0x103c5c24);

    const auto tactics = [&] { return c.u32(tactics_object); };
    const auto club = [&] { return c.u32(screen + field::club); };
    const auto managed = [&] { return std::uint32_t(c.u8(screen + field::managed)); };
    if (compare(c, c.i32(screen + field::pick), -1) != 0) {
        c.set({{3, tactics()}, {4, club()},
               {5, c.u32(c.toc(staff_slot)) + std::uint32_t(c.i32(screen + field::pick)) * staff_size},
               {6, managed()}});
        c.link(0x103c5c58);
        c.set(3, tactics::set_pick(c, c.r(3), c.r(4), c.r(5), c.r(6)));
    }
    c.set(29, 0);
    for (std::uint32_t kind = 0; compare(c, std::int8_t(kind), 5) < 0; c.set(29, ++kind)) {
        c.set(28, 0);
        for (std::uint32_t order = 0; compare(c, std::int8_t(order), 2) < 0; c.set(28, ++order)) {
            c.set({{3, tactics()}, {4, club()}, {5, kind}, {6, order},
                   {7, c.u32(screen + field::takers + kind * 8 + order * 4)}, {8, managed()}});
            c.link(0x103c5c9c);
            c.set(3, tactics::set_set_piece_taker(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)));
        }
    }
    c.set(29, 0);
    for (std::uint32_t place = 0; compare(c, std::int8_t(place), 11) < 0; c.set(29, ++place)) {
        c.set({{3, tactics()}, {4, club()}, {5, place}, {6, c.u8(screen + field::lineup + place)}, {7, managed()}});
        c.link(0x103c5ce4);
        c.set(3, tactics::set_lineup_slot(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)));
    }
    c.set(29, 0);
    for (std::uint32_t which = 0; compare(c, std::int8_t(which), 2) < 0; c.set(29, ++which)) {
        c.set({{3, tactics()}, {4, club()}, {5, which}, {6, c.u32(screen + field::captains + which * 4)},
               {7, managed()}});
        c.link(0x103c5d20);
        c.set(3, tactics::set_captain(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)));
    }
    if (compare_logical(c, c.u32(screen + field::match), 0) == 0) return 1;
    const auto match = [&] { return c.u32(screen + field::match); };
    const auto side = [&] { return std::uint32_t(c.u8(screen + field::side)); };
    const auto count = [&] { return c.i8(screen + field::substitution_count); };
    if (compare(c, count(), 0) != 0) {
        // The substitutions' news item (three or more, two, one), then each one.
        const auto news = [&](std::uint32_t kind, std::uint32_t at) {
            c.set(3, match());
            c.call(0x101d5870, at + 4);
            c.set({{4, kind}, {5, match()}, {6, side()}, {7, 0}});
            c.call(0x101d5810, at + 0x18);
        };
        if (compare(c, count(), 2) > 0) news(0x1f59, 0x103c5d60);
        else if (compare(c, count(), 2) == 0) news(0x1f5c, 0x103c5d90);
        else if (compare(c, count(), 0) != 0) news(0x1f5a, 0x103c5dc0);
        c.set(29, 0);
        for (std::uint32_t index = 0;; c.set(29, ++index)) {
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(index))));
            if (compare(c, std::int8_t(index), count()) >= 0) break;
            const auto at = std::uint32_t(std::int32_t(std::int8_t(index)));
            c.set({{3, screen}, {4, c.u8(screen + field::substitution_from + at)},
                   {5, c.u8(screen + field::substitution_to + at)}});
            c.link(0x103c5e00);
            c.set(3, substitute(c, c.r(3), c.r(4), c.r(5)));
        }
    }
    c.set({{3, match()}, {4, side()}});
    c.call(0x101f1de0, 0x103c5e24);
    c.set({{3, match()}, {4, side()}});
    c.call(0x101f1e10, 0x103c5e30);
    c.set({{3, match()}, {4, side()}});
    c.call(0x101d6a90, 0x103c5e3c);
    c.set(3, match());
    c.call(0x101d6f90, 0x103c5e44);
    return 1;
}

std::uint32_t substitute(Cpu& c, std::uint32_t screen, std::uint32_t from, std::uint32_t to) {
    Frame frame(c, 0x103cb760, 0x90, {.first_gpr = 22});
    c.set({{31, screen}, {27, from}, {28, to}, {26, c.toc(pool_slot)}});
    if (!in_range(c, from, 0, squad_slots - 1) || !in_range(c, to, 0, squad_slots - 1)) {
        fail(c, frame, 0x103cb7c4, 0x3d, 0x38, 0x2028);
        return 0;
    }
    if (compare_logical(c, c.u32(screen + field::match), 0) == 0) return 0;
    const auto match = [&] { return c.u32(screen + field::match); };
    const auto side = [&] { return std::uint32_t(c.u8(screen + field::side)); };
    // The two players' match records.
    c.set({{3, match()}, {4, side()}, {5, from}});
    c.call(0x101f9440, 0x103cb830);
    const auto leaving = c.r(3);
    c.set(30, leaving);
    c.set({{3, match()}, {4, side()}, {5, to}});
    c.call(0x101f9440, 0x103cb844);
    const auto coming = c.r(3);
    c.set(29, coming);
    c.set({{3, match()}, {4, leaving}});
    c.call(0x101f1880, 0x103cb854);
    // The minute (the match clock counts 11 per minute), capped at the end of
    // the period the match length names.
    c.set(3, match());
    c.call(0x101e8920, 0x103cb85c);
    c.put8(leaving + 0x1f, std::uint32_t(std::int32_t(std::int16_t(c.r(3))) / 11));
    c.set(3, match());
    c.call(0x101e8900, 0x103cb888);
    const auto cap = [&](unsigned reg, std::int8_t last) {
        const auto minute = c.i8(leaving + 0x1f);
        const auto value = std::uint32_t(std::int32_t(compare(c, minute, last) < 0 ? minute : last));
        c.set(reg, value);
        c.put8(leaving + 0x1f, value);
    };
    // The original's compare tree on the length.
    const std::int32_t length = std::int16_t(c.r(3));
    if (const auto order = compare(c, length, 0x483); order == 0) cap(23, 105);
    else if (order > 0) {
        if (compare(c, length, 0x528) == 0) cap(22, 120);
    } else if (const auto below = compare(c, length, 0x3de); below == 0) cap(24, 90);
    else if (below < 0 && compare(c, length, 0x1ef) == 0) cap(25, 45);
    c.set(3, coming);
    c.call(0x101d3b50, 0x103cb970);
    if (compare(c, std::int8_t(c.r(3)), -2) == 0) {
        c.set(3, coming);
        c.call(0x101e3f00, 0x103cb984);
        c.set({{4, c.r(3)}, {3, leaving}});
        c.call(0x101f1870, 0x103cb990);
    } else {
        c.set(3, coming);
        c.call(0x101d3b50, 0x103cb99c);
        c.set({{4, c.r(3)}, {3, leaving}});
        c.call(0x101f1870, 0x103cb9a8);
    }
    c.set({{3, match()}, {4, coming}, {5, 0xffffffffu}});
    c.call(0x101f1950, 0x103cb9b8);
    c.put8(coming + 0x20, c.u8(leaving + 0x1f));
    c.set({{3, coming}, {4, 0xfffffffcu}});
    c.call(0x101f1870, 0x103cb9cc);
    c.put8(coming + 0x1e, 1);
    // One substitution fewer left for the side.
    c.set({{3, match()}, {4, side()}});
    c.call(0x101f93d0, 0x103cb9e0);
    tactics_leaf_103cba60(c.s);
    // The news item: kind 0x20d7, or 0x1f5b when the player coming on has no
    // value at +0x22.
    const bool listed = compare(c, c.i8(coming + 0x22), -1) > 0;
    const auto at = listed ? 0x103cb9f8u : 0x103cba24u;
    c.set(3, match());
    c.call(0x101d5870, at + 4);
    c.set({{4, listed ? 0x20d7u : 0x1f5bu}, {5, match()}, {6, from}, {7, side()}, {8, to}, {9, side()}, {10, 0}});
    c.call(0x10201180, at + 0x24);
    return 1;
}

std::uint32_t instructions_event(Cpu& c, std::uint32_t control) {
    Frame frame(c, 0x103c4870, 0x60, {.first_gpr = 25});
    const auto game = c.toc(game_slot);
    c.set({{29, control}, {31, game}});
    // The dialog's parameters: the tactics screen, the formation slot and the
    // copy of the screen's formation taken when the dialog opened.
    const auto parameter = [&](std::uint32_t index, std::uint32_t back) {
        c.set({{3, game}, {4, index}});
        c.call(0x1073ef70, back);
        return c.r(3);
    };
    const auto screen = parameter(0, 0x103c4894);
    c.set(30, screen);
    const auto slot = std::uint32_t(std::int32_t(std::int8_t(parameter(1, 0x103c48a4))));
    c.set(28, slot);
    const auto copy = parameter(2, 0x103c48b4);
    c.set(27, copy);
    // The control's action, asked again before each test as the original does.
    const auto action = [&](std::uint32_t back, std::int32_t wanted) {
        c.set({{3, game}, {4, control}});
        c.call(0x10329bf0, back);
        return compare(c, std::int32_t(c.r(3)), wanted) == 0;
    };
    const auto value = [&](std::uint32_t back) {
        c.set({{3, game}, {4, control}});
        c.call(0x1031acc0, back);
        return c.r(3);
    };
    // The formation place the dialog's squad slot plays, into sp+0x38.
    const auto place = [&](std::uint32_t back) {
        c.set({{3, screen}, {4, slot}, {5, frame.at(0x38)}, {6, 0}});
        c.call(0x103c9810, back);
    };
    const auto set_role = [&](std::uint32_t role, std::uint32_t back) {
        c.set({{3, screen}, {4, c.u8(frame.at(0x38))}, {5, role}});
        c.link(back);
        c.set(3, set_slot_role(c, c.r(3), c.r(4), c.r(5)));
    };
    const auto set_staff = [&](std::uint32_t staff, std::uint32_t back) {
        c.set({{3, screen}, {4, c.u8(frame.at(0x38))}, {5, staff}});
        c.link(back);
        c.set(3, set_slot_staff(c, c.r(3), c.r(4), c.r(5)));
    };
    if (action(0x103c48c4, 1)) {
        // Another squad slot chosen: it becomes parameter 1.
        c.set({{5, value(0x103c48d8)}, {3, game}, {4, 1}, {6, 0}});
        c.call(0x1073f1e0, 0x103c48ec);
        return 0xfffffff5u;
    }
    if (action(0x103c4900, 2)) {
        // Cancel: the screen takes back the copy made when the dialog opened
        // (0x103c11b0), which is destroyed.
        c.set({{3, screen}, {4, copy}});
        c.call(0x103c11b0, 0x103c4914);
        c.set({{3, copy}, {4, 1}});
        c.call(0x103b3b50, 0x103c4920);
        return 0xfffffff7u;
    }
    if (action(0x103c4934, 3)) {
        // OK: the edits stay; the undo copy is dropped and the copy destroyed.
        c.set(3, screen);
        tactics_leaf_103c1f20(c.s);
        c.set({{3, copy}, {4, 1}});
        c.call(0x103b3b50, 0x103c4950);
        return 0xfffffff7u;
    }
    if (action(0x103c4964, 4)) {
        place(0x103c4980);
        c.set({{6, value(0x103c498c)}, {5, 0}, {3, screen}, {4, c.u8(frame.at(0x38))}});
        c.link(0x103c49a0);
        c.set(3, set_slot_instructions(c, c.r(3), c.r(4), c.r(5), c.r(6)));
        return 0xfffffff5u;
    }
    if (action(0x103c49b4, 7)) {
        place(0x103c49d0);
        c.set({{5, value(0x103c49dc)}, {6, 0}, {3, screen}, {4, c.u8(frame.at(0x38))}});
        c.link(0x103c49f4);
        c.set(3, set_slot_instructions(c, c.r(3), c.r(4), c.r(5), c.r(6)));
        return 0xfffffff5u;
    }
    if (action(0x103c4a08, 5)) {
        place(0x103c4a24);
        set_role(std::uint32_t(std::int32_t(std::int8_t(value(0x103c4a30)))), 0x103c4a40);
        return 0xfffffff5u;
    }
    if (action(0x103c4a54, 6)) {
        // A player picked for the slot (role 8) or the slot emptied (role 0x10).
        place(0x103c4a70);
        const auto staff = value(0x103c4a7c);
        c.set(25, staff);
        if (compare_logical(c, staff, 0) != 0) {
            set_role(8, 0x103c4a98);
            set_staff(staff, 0x103c4aa8);
        } else {
            set_role(0x10, 0x103c4abc);
            set_staff(0, 0x103c4acc);
        }
        return 0xfffffff5u;
    }
    c.set(3, control);
    c.call(0x102a83a0, 0x103c4adc);
    c.set(26, c.r(3));
    compare(c, std::int32_t(c.r(26)), 0);
    return c.r(26);
}

namespace {
// One line of the dialog's instruction grid: a heading (column 0) that starts
// a row, or an option in a column of the current row. An option the player's
// instruction word has shows as checked (0x107445a0); any other is a button
// (0x107444e0) whose action sets it: 4 for a bit of the low word, 7 for the
// high word. Each label is formatted first (0x10727890, a varargs routine: it
// spills r4-r10, so the registers the original leaves there matter); `fill`
// passes it "" for its %s. The return addresses are the original call sites;
// `*_copy` is the argument register the original copies the call's result
// into before the next label (0 for none that matters).
struct GridLine {
    std::uint16_t label;
    bool fill;
    std::uint32_t high, low;
    std::uint8_t column;
    std::uint32_t label_back, shown_back;
    std::uint8_t shown_copy;
    std::uint32_t button_back;
    std::uint8_t button_copy;
};
constexpr GridLine grid[] = {
    {0x15e, false, 0, 0, 0, 0x103c24cc, 0x103c24fc, 0, 0, 0},  // Passing
    {0x2c7, true, 0x0, 0x1, 1, 0x103c250c, 0x103c256c, 0, 0x103c25b4, 0},  // Team
    {0x2f0, true, 0x0, 0x2, 2, 0x103c25c4, 0x103c2620, 0, 0x103c2668, 0},  // Mixed
    {0x31a, true, 0x0, 0x4, 3, 0x103c2678, 0x103c26d8, 0, 0x103c2720, 0},  // Short
    {0x344, true, 0x0, 0x8, 4, 0x103c2730, 0x103c2790, 0, 0x103c27d8, 0},  // Direct
    {0x36f, true, 0x0, 0x10, 5, 0x103c27e8, 0x103c2848, 0, 0x103c2890, 0},  // Long
    {0x167, false, 0, 0, 0, 0x103c28a0, 0x103c28d4, 0, 0, 0},  // Tackling
    {0x398, true, 0x0, 0x200, 1, 0x103c28e8, 0x103c2944, 0, 0x103c298c, 0},  // Team
    {0x3b6, true, 0x0, 0x400, 2, 0x103c299c, 0x103c29fc, 0, 0x103c2a44, 0},  // Normal
    {0x3d6, true, 0x0, 0x800, 3, 0x103c2a54, 0x103c2ab4, 0, 0x103c2afc, 0},  // Easy
    {0x3f4, true, 0x0, 0x1000, 4, 0x103c2b0c, 0x103c2b6c, 0, 0x103c2bb4, 0},  // Hard
    {0x171, false, 0, 0, 0, 0x103c2bc4, 0x103c2bf8, 0, 0, 0},  // Pressing
    {0x412, true, 0x0, 0x2000, 1, 0x103c2c0c, 0x103c2c68, 0, 0x103c2cb0, 0},  // Team
    {0x430, true, 0x0, 0x4000, 2, 0x103c2cc4, 0x103c2d20, 0, 0x103c2d68, 0},  // No
    {0x44c, true, 0x0, 0x8000, 3, 0x103c2d78, 0x103c2ddc, 0, 0x103c2e28, 0},  // Yes
    {0x17b, false, 0, 0, 0, 0x103c2e38, 0x103c2e6c, 5, 0, 0},  // Pass To
    {0x469, true, 0x0, 0x20, 1, 0x103c2e80, 0x103c2ee0, 0, 0x103c2f28, 0},  // R/L/C
    {0x487, true, 0x0, 0x40, 2, 0x103c2f38, 0x103c2f98, 0, 0x103c2fe0, 0},  // Left
    {0x4a4, true, 0x0, 0x80, 3, 0x103c2ff0, 0x103c3050, 0, 0x103c3098, 0},  // Centre
    {0x4c3, true, 0x0, 0x100, 4, 0x103c30a8, 0x103c3108, 0, 0x103c3150, 0},  // Right
    {0x184, false, 0, 0, 0, 0x103c3160, 0x103c3194, 0, 0, 0},  // Set Pieces (A)
    {0x4e1, true, 0x0, 0x10000, 1, 0x103c31a8, 0x103c3208, 0, 0x103c3254, 0},  // Normal
    {0x521, true, 0x0, 0x40000000, 2, 0x103c3264, 0x103c32c4, 0, 0x103c3310, 0},  // Back
    {0x559, true, 0x0, 0x20000, 3, 0x103c3320, 0x103c3380, 0, 0x103c33cc, 0},  // Forward
    {0x194, false, 0, 0, 0, 0x103c33dc, 0x103c3410, 0, 0, 0},  // Set Pieces (D)
    {0x597, true, 0x0, 0x40000, 1, 0x103c3420, 0x103c3480, 0, 0x103c34cc, 0},  // Normal
    {0x5d7, true, 0x0, 0x80000, 2, 0x103c34dc, 0x103c353c, 0, 0x103c3588, 0},  // Back
    {0x60f, true, 0x0, 0x80000000, 3, 0x103c3598, 0x103c35f8, 0, 0x103c3644, 0},  // Forward
    {0x1a4, false, 0, 0, 0, 0x103c3654, 0x103c3684, 0, 0, 0},  // Free Role
    {0x64d, false, 0x0, 0x100000, 1, 0x103c3690, 0x103c36f4, 0, 0x103c3740, 0},  // No
    {0x650, false, 0x0, 0x200000, 2, 0x103c374c, 0x103c37ac, 0, 0x103c37f8, 0},  // Yes
    {0x1af, false, 0, 0, 0, 0x103c3808, 0x103c3838, 0, 0, 0},  // Forward Runs
    {0x64d, false, 0x0, 0x400000, 1, 0x103c3844, 0x103c38a4, 9, 0x103c38f4, 0},  // No
    {0x650, false, 0x0, 0x800000, 2, 0x103c3900, 0x103c3960, 0, 0x103c39b0, 0},  // Yes
    {0x1bd, false, 0, 0, 0, 0x103c39c0, 0x103c39f0, 0, 0, 0},  // Run With Ball
    {0x64d, false, 0x0, 0x1000000, 1, 0x103c39fc, 0x103c3a5c, 0, 0x103c3aa8, 5},  // No
    {0x650, false, 0x0, 0x2000000, 2, 0x103c3ab8, 0x103c3b1c, 0, 0x103c3b68, 0},  // Yes
    {0x1cc, false, 0, 0, 0, 0x103c3b7c, 0x103c3bb0, 0, 0, 0},  // Hold Up Ball
    {0x64d, false, 0x0, 0x4000000, 1, 0x103c3bc0, 0x103c3c20, 0, 0x103c3c70, 0},  // No
    {0x650, false, 0x0, 0x8000000, 2, 0x103c3c80, 0x103c3ce0, 0, 0x103c3d2c, 0},  // Yes
    {0x1da, false, 0, 0, 0, 0x103c3d40, 0x103c3d70, 0, 0, 0},  // Long Shots
    {0x64d, false, 0x0, 0x10000000, 1, 0x103c3d7c, 0x103c3ddc, 5, 0x103c3e2c, 0},  // No
    {0x650, false, 0x0, 0x20000000, 2, 0x103c3e38, 0x103c3e98, 0, 0x103c3ee8, 0},  // Yes
    {0x1e6, false, 0, 0, 0, 0x103c3efc, 0x103c3f2c, 0, 0, 0},  // Try Through Balls
    {0x64d, false, 0x2, 0x0, 1, 0x103c3f38, 0x103c3f94, 5, 0x103c3fe0, 0},  // No
    {0x650, false, 0x1, 0x0, 2, 0x103c3fec, 0x103c4048, 5, 0x103c4094, 0},  // Yes
    {0x1f9, false, 0, 0, 0, 0x103c40a4, 0x103c40d4, 0, 0, 0},  // Cross Ball
    {0x64d, false, 0x8, 0x0, 1, 0x103c40e4, 0x103c4144, 0, 0x103c418c, 0},  // No
    {0x650, false, 0x4, 0x0, 2, 0x103c4198, 0x103c41f4, 0, 0x103c423c, 0},  // Yes
};
// The marking options: role bits of the formation slot, set by action 5.
struct MarkingLine {
    std::uint16_t label;
    std::uint8_t bit, column;
    std::uint32_t label_back, shown_back, button_back;
};
constexpr MarkingLine marking[] = {
    {0x674, 0x2, 1, 0x103c42f0, 0x103c4338, 0x103c4384},  // Zonal
    {0x654, 0x1, 2, 0x103c4394, 0x103c43dc, 0x103c4424},  // None
    {0x6d1, 0x4, 3, 0x103c4434, 0x103c447c, 0x103c44c4},  // Man
};
}

std::uint32_t instructions_build(Cpu& c) {
    Frame frame(c, 0x103c2010, 0x190, {.first_gpr = 16});
    // Colours (halfwords behind TOC pointers), the shared text buffer, the game.
    const auto panel_colour = c.toc(-0x7bc8), checked_colour = c.toc(-0x7bd0), heading_colour = c.toc(-0x7b10);
    const auto buffer = c.toc(-0x7bb8), game = c.toc(game_slot), pool = c.toc(pool_slot);
    c.set({{16, panel_colour}, {31, buffer}, {27, checked_colour}, {18, heading_colour}, {30, game}, {26, pool}});
    // A halfword (a colour or text style) behind a TOC pointer.
    const auto style = [&](std::int32_t toc_slot) { return std::uint32_t(c.u16(c.toc(toc_slot))); };
    const auto stack_args = [&](std::initializer_list<std::uint32_t> words) {
        std::int32_t at = 0x38;
        for (const auto word : words) c.put32(frame.at(at), word), at += 4;
    };
    // The buttons' colour, made on the first call (statics next to the TOC).
    const auto statics = c.r(2);
    c.set(3, c.u8(statics - 0x382e));
    if (!flag(c, c.r(3))) {
        c.set({{3, 0}, {4, 0xc0}, {5, 0}, {6, 0}});
        c.call(0x1076bb20, 0x103c205c);
        c.put16(statics - 0x382c, c.r(3));
        c.set(4, 1);
        c.put8(statics - 0x382e, 1);
    }
    const auto button_colour = [&] { return std::uint32_t(c.u16(statics - 0x382c)); };
    // Locals: a halfword and two 6-byte tables, each with a pointer to it.
    c.put16(frame.at(0x54), c.u16(statics - 0x3838));
    c.put32(frame.at(0x64), frame.at(0x54));
    for (const auto& [table, to, pointer] : {std::tuple{-0x4cdc, 0x82, 0x60}, {-0x4ce0, 0x7c, 0x5c}}) {
        const auto from = c.toc(table);
        c.put32(frame.at(to), c.u32(from));
        c.put16(frame.at(to + 4), c.u16(from + 4));
        c.put32(frame.at(pointer), frame.at(to));
    }
    c.set(5, c.u16(c.toc(-0x4ce0) + 4));

    // The dialog's parameters: the tactics screen and the squad slot.
    c.set({{3, game}, {4, 0}});
    c.call(0x1073ef70, 0x103c20c8);
    const auto screen = c.r(3);
    c.set(22, screen);
    c.set({{3, game}, {4, 1}});
    c.call(0x1073ef70, 0x103c20d8);
    c.put8(frame.at(0x53), c.r(3));
    const auto slot = [&] { return std::uint32_t(c.u8(frame.at(0x53))); };
    c.set(3, 0);
    c.call(0x10712e60, 0x103c20e8);

    // The panel, and the frame inside it (r20 r23 r17 r19: left top right bottom).
    std::uint32_t left = 0x91, top = 0x37, right = left + 0x257, bottom = top + 0x1e9;
    const auto set_box = [&] { c.set({{20, left}, {23, top}, {17, right}, {19, bottom}}); };
    const auto inset = [&](std::uint32_t by) { left += by, top += by, right -= by, bottom -= by, set_box(); };
    stack_args({0, 2, 0, 0xffffffff});
    c.set({{3, game}, {4, 0}, {5, 0}, {6, 0x31f}, {7, 0x257}, {8, 1}, {9, 0}, {10, 1}});
    c.call(0x10323c00, 0x103c212c);
    set_box();
    c.set({{3, 0}, {4, 0x38}, {5, 0}, {6, 0}});
    c.call(0x1076bb20, 0x103c2150);
    c.set(5, c.r(3));
    stack_args({0, 0x30, c.r(5), 0xffffffff});
    c.set({{3, game}, {4, left}, {5, top}, {6, right}, {7, bottom}, {8, 1}, {9, 0}, {10, 1}});
    c.call(0x10323c00, 0x103c2194);
    inset(10);

    // A slot's label ("position name"), by the C++ routine, into `text`.
    const auto label_slot = [&](std::uint32_t which, std::uint32_t text, std::uint32_t size, std::uint32_t format,
                                std::uint32_t opponent, std::uint32_t back) {
        c.set({{3, screen}, {4, which}, {5, text}, {6, size}, {7, format}, {8, opponent}});
        c.link(back);
        c.set(3, slot_label(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)));
    };
    // The title: the slot's label.
    label_slot(slot(), buffer, 0x7d0, 1, 0, 0x103c21c0);
    c.set(5, c.r(3));
    stack_args({4, style(-0x7bc8), buffer});
    c.set({{3, game}, {4, left}, {5, top}, {6, right}, {7, top + 0x2d}, {8, 0x30}, {9, style(-0x7bd4)}, {10, 0xc}});
    c.call(0x10744390, 0x103c2200);

    // A menu of the other slots that play (made with the first), to switch to.
    c.put16(frame.at(0x58), 0xffff);
    for (std::uint32_t other = 0; compare(c, std::int8_t(other), squad_slots) < 0; ++other) {
        c.set(21, other);
        if (compare(c, std::int8_t(other), std::int8_t(slot())) == 0) continue;
        c.set({{3, screen}, {4, other}, {5, 0}, {6, 0}});
        c.call(0x103c9810, 0x103c2238);
        if (!flag(c, c.r(3))) continue;
        label_slot(other, buffer, 0x7d0, 1, 0, 0x103c2260);
        if (compare(c, c.i16(frame.at(0x58)), -1) == 0) {
            stack_args({1, style(-0x7bc8), 0, 0, 0});
            c.set({{3, game}, {4, left + 5}, {5, top + 5}, {6, left + 0x11}, {7, top + 0x15}, {8, 0x30},
                   {9, style(-0x7bd4)}, {10, 0xc}});
            c.call(0x10744430, 0x103c22bc);
            c.set({{4, c.r(3)}, {3, game}});
            c.call(0x103239c0, 0x103c22c8);
            c.put16(frame.at(0x58), c.r(3));
        }
        c.set({{3, game}, {4, std::uint32_t(std::int32_t(c.i16(frame.at(0x58))))}, {5, buffer}, {6, 1},
               {7, std::uint32_t(std::int32_t(std::int8_t(other)))}});
        c.call(0x10323a40, 0x103c22e4);
    }
    c.set(21, squad_slots);
    top += 0x37;
    set_box();

    // Cancel and Ok in a strip at the bottom.
    const auto label = [&](std::uint32_t id, bool fill, std::uint32_t back) {
        c.set({{3, buffer}, {4, pool + id}});
        if (fill) c.set(5, pool + 0x22b);
        c.call(0x10727890, back);
    };
    stack_args({0, 1, 0, 0xffffffff});
    c.set({{3, game}, {4, left}, {5, bottom - 0x1e}, {6, right}, {7, bottom}, {8, 2}, {9, frame.at(0x54)}, {10, 1}});
    c.call(0x10323c00, 0x103c233c);
    c.set(28, c.r(3));
    for (const auto& [id, action, column, label_back, back] :
         {std::tuple{0x6fbu, 2u, 0u, 0x103c234cu, 0x103c2394u}, {0x702u, 3u, 1u, 0x103c23a0u, 0x103c23e8u}}) {
        label(id, false, label_back);
        stack_args({style(-0x7bc8), buffer, action, 0});
        c.set({{3, game}, {4, c.r(28)}, {5, column}, {6, 0}, {7, 0x30}, {8, style(-0x7bc4)}, {9, 0xc}, {10, 3}});
        c.call(0x107444e0, back);
    }
    bottom -= 0x28;
    inset(10);
    stack_args({1, c.r(28)});
    c.set({{3, left}, {4, top}, {5, right}, {6, bottom}, {7, frame.at(0x78)}, {8, frame.at(0x74)}, {9, frame.at(0x70)},
           {10, frame.at(0x6c)}, {11, c.toc(-0x4cac)}});
    c.set_f(1, double(c.f32(c.r(11))));
    c.call(0x103cbb30, 0x103c2434);
    inset(5);

    // The grid of instructions, in a list control.
    const auto list = [&](std::uint32_t name, std::uint32_t back) {
        stack_args({0, 1, 0, 0xffffffff});
        c.set({{3, game}, {4, left}, {5, top}, {6, right}, {7, bottom}, {8, 6}, {9, frame.at(name)}, {10, 0xe}});
        c.call(0x10323c00, back);
        c.set(28, c.r(3));
    };
    list(0x82, 0x103c2488);
    std::uint32_t row = 0;
    c.set(29, row);
    c.set({{3, screen}, {4, slot()}, {5, frame.at(0x52)}, {6, 0}});
    c.call(0x103c9810, 0x103c24a4);
    c.set(6, c.r(3));
    const auto place = [&] { return std::uint32_t(c.u8(frame.at(0x52))); };
    c.set({{3, screen}, {4, place()}});
    c.link(0x103c24b4);
    const auto word = slot_instructions(c, c.r(3), c.r(4));
    c.set({{24, std::uint32_t(word >> 32)}, {25, std::uint32_t(word)}});
    const auto heading = [&](std::uint32_t back) {
        stack_args({style(-0x7b10), buffer});
        c.set({{3, game}, {4, c.r(28)}, {5, 0}, {6, row}, {7, 1}, {8, 0}, {9, 1}, {10, 3}});
        c.call(0x107445a0, back);
    };
    const auto checked = [&](std::uint32_t column, std::uint32_t back) {
        stack_args({style(-0x7bd0), buffer});
        c.set({{3, game}, {4, c.r(28)}, {5, column}, {6, row}, {7, 0x221}, {8, style(-0x7bd0)}, {9, 0xc}, {10, 2}});
        c.call(0x107445a0, back);
    };
    const auto button = [&](std::uint32_t column, std::uint32_t action, std::uint32_t value, std::uint32_t back) {
        stack_args({button_colour(), buffer, action, value});
        c.set({{3, game}, {4, c.r(28)}, {5, column}, {6, row}, {7, 1}, {8, 0}, {9, 0xc}, {10, 2}});
        c.call(0x107444e0, back);
    };
    const auto copy = [&](std::uint8_t reg) { if (reg) c.set(reg, c.r(3)); };
    for (const auto& line : grid) {
        if (!line.column) {
            if (&line != grid) c.set(29, ++row);
            label(line.label, line.fill, line.label_back);
            heading(line.shown_back);
            copy(line.shown_copy);
            continue;
        }
        label(line.label, line.fill, line.label_back);
        if (compare(c, std::int32_t((std::uint32_t(word >> 32) & line.high) | (std::uint32_t(word) & line.low)), 0) != 0) {
            checked(line.column, line.shown_back);
            copy(line.shown_copy);
        } else {
            button(line.column, line.high ? 7 : 4, line.high ? line.high : line.low, line.button_back);
            copy(line.button_copy);
        }
    }

    // Marking, from the slot's role bits, in a list of its own.
    list(0x7c, 0x103c4280);
    c.set(29, ++row);
    c.set({{3, screen}, {4, place()}});
    c.link(0x103c4294);
    c.set(3, slot_role(c, c.r(3), c.r(4)));
    c.put8(frame.at(0x51), c.r(3));
    const auto role = [&] { return c.u8(frame.at(0x51)); };
    label(0x205, true, 0x103c42ac);
    heading(0x103c42e0);
    for (const auto& line : marking) {
        label(line.label, true, line.label_back);
        if (compare(c, role() & line.bit, 0) != 0) checked(line.column, line.shown_back);
        else button(line.column, 5, line.bit, line.button_back);
    }
    c.set({{5, role()}, {4, role() & 0x4u}});
    if (compare(c, role() & 0x4, 0) == 0) return c.r(3);

    // Man marking: whom to mark, a button and a list of the opponent's players.
    stack_args({style(-0x7bc8), 0, 0, 0});
    c.set({{3, game}, {4, c.r(28)}, {5, 5}, {6, row}, {7, 0x30}, {8, style(-0x7bd4)}, {9, 0xc}, {10, 1}});
    c.call(0x107444e0, 0x103c4520);
    c.set(4, c.r(3));
    stack_args({0, 0x500030, style(-0x7bcc), c.r(4)});
    c.set({{3, game}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0}});
    c.call(0x10323c00, 0x103c456c);
    c.set(11, c.r(3));
    c.put16(frame.at(0x56), c.r(3));
    const auto opponents = [&] { return std::uint32_t(std::int32_t(c.i16(frame.at(0x56)))); };
    const auto marked = frame.at(0x68);
    for (std::uint32_t other = 0; compare(c, std::int8_t(other), squad_slots) < 0; ++other) {
        c.set(21, other);
        c.set({{3, screen}, {4, other}});
        c.link(0x103c4588);
        c.set(3, slot_in_use(c, c.r(3), c.r(4)));
        if (!flag(c, c.r(3))) continue;
        c.set({{3, screen}, {4, other}, {5, 0}, {6, 1}});
        c.call(0x103c9810, 0x103c45a8);
        if (!flag(c, c.r(3))) {
            // In a match, only players with a place in the opponent's formation.
            c.set(3, screen);
            c.link(0x103c45bc);
            c.set(5, in_match(c, c.r(3)));
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(5)))));
            if (flag(c, c.r(3))) continue;
        }
        c.set({{3, screen}, {4, other}, {5, 1}});
        c.call(0x103ca4e0, 0x103c45dc);
        c.set(6, c.r(3));
        c.put32(marked, c.r(3));
        c.set(4, c.u32(marked));
        if (compare_logical(c, c.r(4), 0) == 0) continue;
        if (compare(c, role() & 0x8, 0) != 0) {
            // The player the slot marks now: his name, and a button to keep him.
            c.set({{3, screen}, {4, place()}});
            c.link(0x103c4610);
            c.set(3, slot_staff(c, c.r(3), c.r(4)));
            if (compare_logical(c, c.u32(marked), c.r(3)) == 0) {
                c.set({{3, c.u32(marked)}, {4, frame.at(0x88)}, {5, 0xc8}, {6, 2}});
                c.call(0x100caeb0, 0x103c4630);
                c.set({{3, buffer}, {4, pool + 0xde8}, {5, frame.at(0x88)}});
                c.call(0x1077b720, 0x103c4640);
                c.set(3, screen);
                c.call(0x103baca0, 0x103c464c);
                c.set({{4, c.r(3)}, {5, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))}});
                const bool own = flag(c, c.r(3));
                stack_args({style(-0x7bd0), buffer, own ? 0x7dcu : 0x7d7u, c.u32(marked)});
                c.set({{3, game}, {4, c.r(28)}, {5, 4}, {6, row}, {7, 1}, {8, 0}, {9, 1}, {10, 2}});
                c.call(0x107444e0, own ? 0x103c469c : 0x103c46e4);
                c.set(5, c.r(3));
            }
        }
        label_slot(other, buffer, 0x7d0, 1, 1, 0x103c4704);
        c.set({{3, game}, {4, opponents()}, {5, buffer}, {6, 6}, {7, c.u32(marked)}});
        c.call(0x10323a40, 0x103c4720);
        c.set(5, c.r(3));
    }
    c.set(21, squad_slots);
    label(0xded, false, 0x103c4740);
    // The opponent facing the slot, when there is one.
    c.set({{3, screen}, {4, slot()}, {5, 0}});
    c.link(0x103c4750);
    c.set(3, match_player(c, c.r(3), c.r(4), c.r(5)));
    c.put8(frame.at(0x50), c.r(3));
    c.set(4, std::uint32_t(std::int32_t(c.i8(frame.at(0x50)))));
    if (compare(c, c.i8(frame.at(0x50)), -1) > 0) {
        c.set({{3, screen}, {4, c.u8(frame.at(0x50))}, {5, 1}});
        c.call(0x103ca4e0, 0x103c4774);
        c.put32(marked, c.r(3));
        c.set(5, c.u32(marked));
        if (compare_logical(c, c.r(5), 0) != 0) {
            label_slot(c.u8(frame.at(0x50)), frame.at(0x88), 0xc8, 6, 1, 0x103c47a4);
            c.set({{3, buffer}, {4, pool + 0xdfa}, {5, frame.at(0x88)}});
            c.call(0x10727890, 0x103c47b4);
            c.set(4, c.r(3));
        }
    }
    if (compare(c, role() & 0x10, 0) != 0) {
        stack_args({style(-0x7bd0), buffer});
        c.set({{3, game}, {4, c.r(28)}, {5, 4}, {6, row}, {7, 1}, {8, 0}, {9, 1}, {10, 2}});
        c.call(0x107445a0, 0x103c47fc);
        c.set(5, c.r(3));
    }
    c.set({{3, game}, {4, opponents()}, {5, buffer + 2}, {6, 6}, {7, 0}});
    c.call(0x10323a40, 0x103c4818);
    c.set(4, c.r(3));
    return c.r(3);
}
}

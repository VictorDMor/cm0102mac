#include "tactics_screens_build.hpp"
#include "tactics_leaves.hpp"

namespace cm::aot::tactics_screens::build {
using guest::Cpu;
using guest::Frame;
using detail::fail;
using detail::red_zone;
namespace {
bool is(Cpu& c, std::uint32_t address, std::int8_t value) { return c.i8(address) == value; }
std::uint32_t formation_of(std::uint32_t screen) { return screen + field::formation; }
}

// The wrappers over FORMATION.CPP routines call them through the dispatcher:
// each returns with the callee's volatile registers and CR, as the original.

std::uint32_t shared_spot(Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t later) {
    Frame frame(c, 0x103baa00, 0x40, {.first_gpr = 30});
    c.put8(frame.at(0x5f), player);
    c.put32(frame.at(0x60), later);
    c.compare(0, c.i8(screen + field::view), 2);
    const std::uint32_t with_ball = is(c, screen + field::view, 2);
    c.set({{31, screen}, {30, with_ball}, {3, formation_of(screen)}, {4, c.u8(frame.at(0x5f))}, {5, with_ball},
           {6, c.u32(screen + field::cell)}, {7, later}});
    c.call(0x10113940, 0x103baa54);
    return c.r(3);
}

std::uint32_t copy_cell(Cpu& c, std::uint32_t screen, std::uint32_t out, std::uint32_t with_ball) {
    Frame frame(c, 0x103baa70, 0x40, {.first_gpr = 30});
    c.put8(frame.at(0x63), with_ball);
    c.set({{30, screen}, {31, out}});
    c.compare(0, std::int8_t(with_ball), 0);
    const auto from = screen + (std::int8_t(with_ball) ? field::ball_cell : field::cell);
    c.set({{3, out}, {4, from}, {0, c.u16(from + 2)}});
    tactics_leaf_103baae0(c.s);
    return out;
}

std::uint32_t player_at(Cpu& c, std::uint32_t screen, std::uint32_t position) {
    Frame frame(c, 0x103bab00, 0x40, {});
    c.put32(frame.at(0x58), screen);
    c.put16(frame.at(0x5e), position);
    c.set({{3, formation_of(screen)}, {4, std::uint32_t(std::int32_t(c.i16(frame.at(0x5e))))}});
    c.call(0x10111840, 0x103bab24);
    return c.r(3);
}

std::uint32_t central_in_line(Cpu& c, std::uint32_t screen, std::uint32_t position) {
    Frame frame(c, 0x103bab40, 0x40, {.first_gpr = 30});
    c.put16(frame.at(0x5e), position);
    c.compare_logical(0, c.u32(screen + field::match), 0);
    const auto table = c.u32(screen + field::match) ? screen + field::players : 0;
    c.set({{31, screen}, {30, table}, {3, formation_of(screen)},
           {4, std::uint32_t(std::int32_t(c.i16(frame.at(0x5e))))}, {5, table}});
    c.call(0x10111630, 0x103bab84);
    return c.r(3);
}

std::uint32_t position(Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t with_ball) {
    Frame frame(c, 0x103baba0, 0x40, {});
    c.put32(frame.at(0x58), screen);
    c.put8(frame.at(0x5f), player);
    c.put8(frame.at(0x63), with_ball);
    c.set({{3, formation_of(screen)}, {4, c.u8(frame.at(0x5f))}, {5, c.u8(frame.at(0x63))}});
    c.call(0x10111240, 0x103babcc);
    return c.r(3);
}

std::uint32_t displaced(Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t out) {
    Frame frame(c, 0x103babe0, 0x40, {});
    c.put32(frame.at(0x58), screen);
    c.put8(frame.at(0x5f), player);
    c.put32(frame.at(0x60), out);
    c.set({{3, formation_of(screen)}, {4, c.u8(frame.at(0x5f))}, {5, out}});
    c.call(0x10111d30, 0x103bac0c);
    return c.r(3);
}

// The frameless tests leave cr0 from their last compare.
std::uint32_t not_positions_view(Cpu& c, std::uint32_t screen) {
    red_zone(c, {{31, -4}});
    c.set(0, std::uint32_t(std::int32_t(c.i8(screen + field::view))));
    c.compare(0, c.i8(screen + field::view), 1);
    return !is(c, screen + field::view, 1);
}

std::uint32_t free_kick_view(Cpu& c, std::uint32_t screen) {
    red_zone(c, {{31, -4}, {30, -8}});
    bool result = false;
    c.set(0, std::uint32_t(std::int32_t(c.i8(screen + field::view))));
    if (!is(c, screen + field::view, 1)) {
        c.set({{4, screen + 0x10000}, {0, std::uint32_t(std::int32_t(c.i8(screen + field::free_kick)))}});
        result = c.i8(screen + field::free_kick) != 0;
    }
    c.compare(0, result, 0);
    return result;
}

std::uint32_t formation_locked(Cpu& c, std::uint32_t screen) {
    red_zone(c, {{31, -4}});
    c.set(0, std::uint32_t(std::int32_t(c.i8(formation_of(screen) + field::formation_locked))));
    c.compare(0, c.i8(formation_of(screen) + field::formation_locked), 0);
    return c.i8(formation_of(screen) + field::formation_locked) != 0;
}

std::uint32_t formation_flag(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103bae10, 0x40, {});
    c.put32(frame.at(0x58), screen);
    return c.u8(formation_of(screen) + field::formation_flag);
}

std::uint32_t formation_saved(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103bae50, 0x40, {});
    c.put32(frame.at(0x58), screen);
    return c.u8(formation_of(screen) + field::formation_saved);
}

std::uint32_t formation_total(Cpu& c, std::uint32_t tactics, std::uint32_t first_only) {
    red_zone(c, {{31, -4}});
    c.compare(0, std::int8_t(first_only), 0);
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(first_only))));
    const auto first = c.u32(tactics + field::formation_count);
    if (std::int8_t(first_only)) return first;
    c.set({{5, first}, {0, c.u32(tactics + field::extra_formation_count)}});
    return first + c.u32(tactics + field::extra_formation_count);
}

void open_screen(Cpu& c) {
    Frame frame(c, 0x103baec0, 0x40, {});
    c.link(0x103baed0);
    build_screen(c);
}

std::uint32_t match_side_byte(Cpu& c, std::uint32_t match, std::uint32_t side) {
    red_zone(c, {{31, -4}});
    c.compare(0, std::int8_t(side), 0);
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(side))));
    return c.u8(match + (std::int8_t(side) == 0 ? 0x77c5 : 0x77c6));
}

std::uint32_t match_away_club(Cpu& c, std::uint32_t match) {
    red_zone(c, {{31, -4}});
    const auto fixture = c.u32(match + 0x3118);
    c.compare_logical(0, fixture, 0);
    c.set(0, fixture);
    if (!fixture) return 0;
    c.set(4, fixture);
    return c.u32(fixture + field::match_away);
}

std::uint32_t match_home_club(Cpu& c, std::uint32_t match) {
    red_zone(c, {{31, -4}});
    const auto fixture = c.u32(match + 0x3118);
    c.compare_logical(0, fixture, 0);
    c.set(0, fixture);
    if (!fixture) return 0;
    c.set(4, fixture);
    return c.u32(fixture + field::match_home);
}

std::uint32_t construct_formation(Cpu& c, std::uint32_t formation) {
    Frame frame(c, 0x103c59f0, 0x40, {});
    c.put32(frame.at(0x58), formation);
    c.set(3, formation);
    c.call(0x10110c80, 0x103c5a08);
    // The 20 player entries after the record (constructor, destructor, size, count).
    c.set({{3, formation + 0x18ec}, {4, c.toc(-0x6404)}, {5, c.toc(-0x6408)}, {6, 0x1c4}, {7, 0x14}});
    c.call(0x10773320, 0x103c5a24);
    return formation;
}

std::uint32_t construct_teams(Cpu& c, std::uint32_t object) {
    Frame frame(c, 0x103c5a40, 0x40, {.first_gpr = 31});
    c.set({{31, object}, {3, object + 0x120}, {4, c.toc(-0x7acc)}, {5, 0}, {6, 0x664}, {7, 2}});
    c.call(0x10773320, 0x103c5a6c);
    return object;
}

std::uint32_t select_club(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c5e60, 0x40, {.first_gpr = 30});
    c.set(31, screen);
    c.compare(0, c.i8(screen + field::limit_squad), 0);
    if (c.i8(screen + field::limit_squad) == 0) {
        c.set({{3, c.u32(screen + field::club)}, {4, 0}, {5, 0}});
        c.call(0x10713050, 0x103c5edc);
        return 1;
    }
    // A list selection first (+0xe28), else the other control (+0xe24).
    c.set(30, 0);
    const auto list = c.u32(screen + 0xe28);
    c.compare_logical(0, list, 0);
    if (list) {
        c.set({{3, list}, {4, 0}, {5, 1}});
        c.call(0x10713560, 0x103c5ea8);
        c.set(30, c.r(3));
    }
    c.compare(0, std::int8_t(c.r(30)), 0);
    if (std::int8_t(c.r(30)) == 0) {
        c.set({{3, c.u32(screen + 0xe24)}, {4, 0}, {5, 0}});
        c.call(0x10713260, 0x103c5ec8);
    }
    return 1;
}

std::uint32_t title(Cpu& c, std::uint32_t screen, std::uint32_t text, std::uint32_t size, std::uint32_t first,
                    std::uint32_t second) {
    Frame frame(c, 0x103c5f00, 0x80, {.first_gpr = 27});
    c.put32(frame.at(0xa0), size);
    const auto pool = c.toc(detail::pool_slot);
    c.set({{31, screen}, {29, text}, {27, first}, {28, second}, {30, pool}});
    // The argument checks, each into cr0 until one fails.
    const bool valid = [&] {
        c.compare_logical(0, text, 0);
        if (!text) return false;
        c.compare(0, std::int32_t(size), 100);
        if (std::int32_t(size) < 100) return false;
        c.compare_logical(0, first, 0);
        if (!first) return false;
        c.compare_logical(0, second, 0);
        return second != 0;
    }();
    if (!valid) {
        fail(c, frame, 0x103c5f60, 0x3d, 0x38, 0x1425);
        return 0;
    }
    c.set({{3, c.u32(screen + field::club)}, {4, c.u8(screen + field::team_kind)}, {5, first}, {6, second}});
    c.call(0x100c7a50, 0x103c5fc0);
    // "%s"-style formats with the club's name (+0x1ee) and a byte of it (+0x242) twice.
    const auto format = [&](std::uint32_t offset, std::uint32_t back) {
        const auto club = c.u32(screen + field::club);
        const auto letter = std::uint32_t(std::int32_t(c.i8(club + 0x242)));
        c.set({{3, text}, {4, pool + offset}, {5, letter}, {6, letter}, {7, club + 0x1ee}});
        c.call(0x10727890, back);
    };
    c.compare(0, c.i8(screen + field::national), 0);
    if (c.i8(screen + field::national) != 0) {
        format(0xe23, 0x103c5ffc);
        return 1;
    }
    c.compare(0, c.i8(screen + field::opened_from), 0);
    if (c.i8(screen + field::opened_from) != 0) {
        c.set(3, c.u32(screen + field::club));
        c.call(0x100c7d00, 0x103c6018);
        c.compare(0, std::int8_t(c.r(3)), 0);
        if (std::int8_t(c.r(3)) == 0) {
            format(0xe47, 0x103c6050);
            return 1;
        }
    }
    format(0xe6c, 0x103c6080);
    return 1;
}

std::uint32_t selected_entry(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c6180, 0x50, {.first_gpr = 29});
    c.set({{29, screen}, {31, 0}});
    for (std::uint32_t entry = 0;; ++entry) {
        c.compare(0, std::int8_t(entry), 20);
        if (std::int8_t(entry) >= 20) break;
        c.set({{31, entry}, {3, screen}, {4, entry}, {5, 0}});
        c.call(0x103ca4e0, 0x103c61b4);
        const auto player = c.r(3);
        c.set(30, player);
        c.compare_logical(0, player, 0);
        if (!player) continue;
        c.compare(0, c.i32(player), c.i32(screen + 0x4b14));
        if (c.u32(player) == c.u32(screen + 0x4b14)) return entry;
    }
    return 0xffffffffu;
}

std::uint32_t substitutes_left(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c6450, 0x40, {.first_gpr = 31});
    c.set(31, screen);
    const auto match = c.u32(screen + field::match);
    c.compare_logical(0, match, 0);
    if (!match) return 0;
    c.compare(0, c.i8(screen + field::in_match), 0);
    if (c.i8(screen + field::in_match) == 0) return 0;
    c.set({{3, match}, {4, c.u8(screen + field::side)}});
    c.call(0x101f93d0, 0x103c648c);
    c.call(0x101f1860, 0x103c6490);
    return std::uint32_t(std::int32_t(std::int8_t(c.r(3) - c.u8(screen + 0x4b12))));
}

std::uint32_t kind_is_five(Cpu& c, std::uint32_t screen) {
    Frame frame(c, 0x103c6bd0, 0x40, {.first_gpr = 31});
    c.set(31, screen);
    // The competition for team kinds 1-3 (club +0x48, +0x50, +0x58).
    constexpr struct { std::int8_t kind; std::uint32_t offset, back; } kinds[] = {
        {1, 0x48, 0x103c6c10}, {2, 0x50, 0x103c6c48}, {3, 0x58, 0x103c6c80}};
    for (const auto& [kind, offset, back] : kinds) {
        c.compare(0, c.i8(screen + field::team_kind), kind);
        if (!is(c, screen + field::team_kind, kind)) continue;
        const auto competition = c.u32(c.u32(screen + field::club) + offset);
        c.compare_logical(0, competition, 0);
        if (!competition) continue;
        c.set(3, competition);
        c.call(0x100cdae0, back);
        c.compare(0, std::int8_t(c.r(3)), 5);
        if (std::int8_t(c.r(3)) == 5) return 1;
    }
    return 0;
}

std::uint32_t lineup_byte(Cpu& c, std::uint32_t screen, std::uint32_t index) {
    Frame frame(c, 0x103c72c0, 0x70, {.first_gpr = 29});
    c.set({{29, screen}, {30, index}, {31, c.toc(detail::pool_slot)}});
    c.compare(0, std::int8_t(index), 0);
    const bool valid = std::int8_t(index) >= 0 && (c.compare(0, std::int8_t(index), 10), std::int8_t(index) <= 10);
    if (!valid) {
        fail(c, frame, 0x103c7310, 0x3d, 0x38, 0x168c);
        return 0xffffffffu;
    }
    return c.u8(screen + field::lineup + std::uint32_t(std::int32_t(std::int8_t(index))));
}

std::uint32_t bit_index(Cpu& c, std::uint32_t high, std::uint32_t low) {
    // The original tests the bits from 0 up, each with the same xor/or sequence;
    // r5 and r0 keep the last test's values, r6 the mask of the last one.
    const std::uint64_t value = std::uint64_t(high) << 32 | low;
    for (unsigned n = 0; n < 48; ++n) {
        if (value != std::uint64_t(1) << n) continue;
        c.set({{5, 0}, {0, 0}});
        if (n == 47) c.set(6, 0x8000);
        c.compare(0, 0, 0);
        return n;
    }
    c.set({{5, low}, {0, low | (high ^ 0x8000)}, {6, 0x8000}});
    c.compare(0, std::int32_t(c.r(0)), 0);
    return 0xffffffffu;
}
}

#include "tactics.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <tuple>
#include <utility>

// Besides memory and the saved registers, each routine leaves r4-r10 and cr0
// as the original leaves them on the path taken (they reach memory through
// the caller's later varargs calls and CR saves): the "leftovers" below are
// the original's last writes to them.
namespace cm::aot::tactics {
using guest::Cpu;
using guest::Frame;
namespace {
// TOC slots of the globals TACTICS.CPP reads.
constexpr std::int32_t pool_slot = -0x4cf0;          // TACTICS.CPP's string pool
constexpr std::int32_t message_slot = -0x7ba8;       // -> assertion message buffer
constexpr std::int32_t success_slot = -0x7bac;       // -> shared success flag
constexpr std::int32_t staff_slot = -0x7be0;         // -> staff records
constexpr std::int32_t staff_count_slot = -0x7be4;   // -> int: number of staff (humans hold the last 16 ids)
constexpr std::int32_t club_count_slot = -0x7b50;    // -> int: number of clubs
constexpr std::int32_t game_slot = -0x7bb0;          // -> game state: current human at +0x2c58
constexpr std::int32_t humans_slot = -0x7a54;        // -> human manager table
constexpr std::int32_t selection_slot = -0x7a64;     // -> the object 0x102b7c00 asks
constexpr std::int32_t formation_destructor_slot = -0x6418;
constexpr std::int32_t int_to_double_slot = -0x4cf4; // -> 2^52 + 2^31

void fail(Cpu& c, const Frame& frame, std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line,
          bool flag_in_r4 = false) {
    const auto pool = c.toc(pool_slot);
    guest::report_assertion(c, {at, pool + 0x4e, pool + 0x5a, pool + 0x67, frame.at(name), frame.at(extension), line});
    if (flag_in_r4) c.set(4, c.toc(success_slot));  // these sites clear the flag through r4
}
// A char argument within [low, high], as the original's extsb compares.
bool in_range(std::uint32_t value, std::int32_t low, std::int32_t high) {
    const auto v = std::int8_t(value);
    return v >= low && v <= high;
}
// cr0 as the original's last compare leaves it (a record form compares with
// 0): LT, GT or EQ, and XER's summary overflow. Pass int32 values for cmpw,
// uint32 values for cmplw.
void compare(Cpu& c, std::int64_t a, std::int64_t b = 0) {
    c.set_cr(0, std::uint8_t((a < b ? 8 : a > b ? 4 : 2) | (c.s.r<1>(guest::reg::xer_so) & 1)));
}
std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }
std::uint32_t sign8(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
// The byte offset of a char index the original sign-extends.
std::uint32_t scaled(std::uint32_t value, std::uint32_t size) { return sign8(value) * size; }
std::uint32_t staff_record(Cpu& c, std::int32_t id) { return c.u32(c.toc(staff_slot)) + std::uint32_t(id) * staff_size; }
constexpr std::uint32_t none = 0xffffffffu;

// `bl team_slot` at the call returning to `back`, its `cmpwi r3,-1`, and the
// team record it names (none for -1): how most routines start.
std::optional<Team> team_record(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed, std::uint32_t back) {
    c.link(back);
    const auto slot = team_slot(c, club, managed);
    compare(c, slot, -1);
    if (slot == -1) return std::nullopt;
    return Tactics(c, tactics).team(slot);
}

// The current human manager, whose backup is used: game +0x2c58 picks a
// 0x2c0-byte entry holding the manager at +4. Humans hold the last 16 staff
// ids, so the backup index is the staff count - the manager's id - 1.
std::uint32_t human_entry(Cpu& c) {
    const auto game = c.toc(game_slot);
    return game + std::uint32_t(std::int32_t(c.i16(game + 0x2c58))) * 0x2c0;
}
std::uint32_t human_distance(Cpu& c, std::uint32_t manager) { return c.u32(c.toc(staff_count_slot)) - c.u32(manager); }

// Double arithmetic as the generated code does it, with its FPSCR bits (one
// byte per bit at 1280 + n): the result class, the sticky invalid (signalling
// NaN, 0/0), zero-divide and inexact bits, and the summaries.
class Fpu {
public:
    explicit Fpu(Cpu& c) : s(c.s) {}
    double sub(double a, double b) { const double r = a - b; status(r, a, b, false); return r; }
    double mul(double a, double b) { const double r = a * b; status(r, a, b, true); return r; }
    double div(double a, double b) {
        const double r = a / b;
        set(zx, get(zx) | (b == 0));
        set(vxzdz, get(vxzdz) | (a == 0 && b == 0));
        status(r, a, b, true);
        return r;
    }

private:
    static constexpr std::size_t fx = 1280, fex = 1281, vx = 1282, ox = 1283, ux = 1284, zx = 1285, xx = 1286,
                                 vxsnan = 1287, vxzdz = 1290, fi = 1294, fl = 1296, fg = 1297, fe = 1298, fu = 1299;
    std::uint8_t get(std::size_t bit) const { return std::uint8_t(s.r<1>(bit)); }
    void set(std::size_t bit, std::uint32_t value) { s.wr<1>(bit, value); }
    void status(double r, double a, double b, bool inexact) {
        set(fl, r < 0); set(fg, 0 < r); set(fe, r == 0); set(fu, std::isnan(r));
        if (inexact) set(xx, get(xx) | get(fi));
        set(vxsnan, get(vxsnan) | std::isnan(a) | std::isnan(b));
        std::uint8_t invalid = 0;
        for (const std::size_t bit : {1287u, 1288u, 1289u, 1290u, 1291u, 1292u, 1301u, 1302u, 1303u}) invalid |= get(bit);
        set(vx, invalid);
        set(fx, get(fx) | get(ox) | get(ux) | get(zx) | get(xx));
        std::uint8_t enabled = 0;
        for (const std::size_t bit : {vx, ox, ux, zx, xx}) enabled ^= get(bit) & get(bit + 22);
        set(fex, enabled);
    }
    State& s;
};
}

std::uint32_t destroy(Cpu& c, std::uint32_t tactics, std::uint32_t dispose) {
    Frame frame(c, 0x103af060, 0x50, {.first_gpr = 29});
    const auto destructor = c.toc(formation_destructor_slot);
    c.set({{29, tactics}, {30, dispose}, {31, destructor}});
    compare(c, std::int32_t(tactics));
    if (!tactics) return 0;
    // An array and its count, freed by `target` (the formation arrays destroy
    // each record), then cleared.
    const auto release = [&](std::uint32_t array, std::uint32_t target, std::uint32_t back, bool records) {
        const auto address = c.u32(tactics + array);
        if (!address) return;
        c.set(3, address);
        if (records) c.set(4, destructor);
        c.call(target, back);
        c.put32(tactics + array, 0);
        c.put32(tactics + array + 4, 0);
    };
    release(field::extra_formations, 0x107734a0, 0x103af09c, true);
    release(field::formations, 0x107734a0, 0x103af0bc, true);
    release(field::teams, 0x10775170, 0x103af0d8, false);
    release(field::results, 0x10773610, 0x103af0f4, false);
    compare(c, std::int16_t(dispose));
    if (std::int16_t(dispose) > 0) {
        c.set(3, tactics);
        c.call(0x107735a0, 0x103af110);
    }
    return tactics;
}

std::uint32_t record_result(Cpu& c, std::uint32_t tactics, std::uint32_t fixture) {
    Frame frame(c, 0x103af130, 0xf0, {.first_gpr = 25});
    c.set({{25, tactics}, {26, fixture}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    const auto score = frame.at(0x88);  // the fixture's score, as 0x1030d4a0 reports it
    c.set(3, score);
    c.call(0x10715880, 0x103af15c);
    if (!fixture) {
        fail(c, frame, 0x103af178, 0x68, 0x40, 0x1f9);
        return 0;
    }
    // Nothing is counted when a human controls either club.
    for (const auto& [offset, back] : {std::pair{0x1cu, 0x103af1dcu}, {0x20u, 0x103af204u}}) {
        const auto club = c.u32(fixture + offset);
        if (!club) continue;
        c.set({{3, club}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
        c.call(0x1014c2e0, back);
        compare(c, std::int8_t(c.r(3)));
        if (std::int8_t(c.r(3))) return 0;
    }
    const Tactics t(c, tactics);
    Fpu fpu(c);
    // A count as a double, at least 1, converted as the original does through
    // a stack temporary at sp+offset.
    const auto count = [&](const Results& results, std::uint32_t at, std::int32_t offset) {
        const std::int32_t value = std::max<std::int32_t>(results.count(at), 1);
        c.put32(frame.at(offset), 0x43300000);
        c.put32(frame.at(offset + 4), std::uint32_t(value) ^ 0x80000000u);
        return fpu.sub(c.f64(frame.at(offset)), c.f64(c.toc(int_to_double_slot)));
    };
    for (std::uint32_t side = 0; side < 2; ++side) {
        c.set(28, side);
        const auto club = c.u32(fixture + (side ? 0x20 : 0x1c));
        if (!club) {
            fail(c, frame, 0x103af24c, 0x48, 0x38, 0x20a);
            continue;
        }
        const auto results = t.results(t.team(c.i32(club)).formation());
        c.set({{27, results.address}, {3, fixture}, {4, club}, {5, score}, {6, t.teams()}, {7, c.u32(tactics + field::results)}});
        c.call(0x1030d4a0, 0x103af2b4);
        const std::int32_t scored = c.i8(score + 0x1c), conceded = c.i8(score + 0x1d);
        results.add(field::played, 1);
        if (scored > conceded) {
            results.add(field::won, 1);
            results.add(field::points, 3);
        } else if (scored == conceded) {
            results.add(field::drawn, 1);
            results.add(field::points, 1);
        } else {
            results.add(field::lost, 1);
        }
        results.add(field::scored, scored);
        results.add(field::conceded, conceded);
        // Ratings from goals for and against and points per game, in the
        // original's order of operations.
        const double goals_for = count(results, field::scored, 0xc4), goals_against = count(results, field::conceded, 0xbc);
        const double points = count(results, field::points, 0xb4), played = count(results, field::played, 0xac);
        const double attack = fpu.div(goals_for, played), defence = fpu.div(played, goals_against);
        const double attack2 = fpu.mul(attack, attack), both = fpu.mul(attack, defence);
        const double form = fpu.div(points, played);
        const double a = fpu.mul(defence, attack2), b = fpu.mul(form, both), d = fpu.mul(defence, both);
        const double first = fpu.mul(form, fpu.mul(form, a));
        const double second = fpu.mul(form, fpu.mul(form, b));
        const double third = fpu.mul(form, fpu.mul(form, d));
        results.set_rating(0, first);
        results.set_rating(1, second);
        results.set_rating(2, third);
        // Leftovers: the constant's address the last conversion loaded.
        c.set({{4, c.toc(int_to_double_slot)}, {5, c.toc(int_to_double_slot)}});
    }
    compare(c, 2, 2);
    return 1;
}

std::uint32_t formation_at(Cpu& c, std::uint32_t tactics, std::int16_t index, std::uint32_t first_only) {
    Frame frame(c, 0x103af480, 0x70, {.first_gpr = 30});
    c.set({{30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    const Tactics t(c, tactics);
    compare(c, index);
    if (index >= 0) {
        compare(c, std::int8_t(first_only));
        c.set(6, word(index));
        const auto first = t.formation_count();
        if (!std::int8_t(first_only)) c.set(5, first);
        const auto count = std::int32_t(std::int8_t(first_only) ? first : first + t.extra_count());
        compare(c, index, count);
        if (index < count) {
            compare(c, index, std::int32_t(first));
            return t.formation(index);
        }
    }
    fail(c, frame, 0x103af4e0, 0x40, 0x38, 0x265, true);
    return 0;
}

std::uint32_t team_formation(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t setting) {
    Frame frame(c, 0x103af570, 0x80, {.first_gpr = 26});
    const auto humans = c.toc(humans_slot);
    c.set({{26, tactics}, {27, club}, {28, setting}, {29, humans}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103af5b0, 0x40, 0x38, 0x27f, true);
        return 0;
    }
    c.set({{3, club}, {4, 1}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x103af610);
    bool human = std::int8_t(c.r(3));
    if (human) {
        c.set({{3, c.u32(humans)}, {4, c.u32(club + 0x90)}});
        c.call(0x10147e70, 0x103af624);
        if (std::int8_t(c.r(3))) {
            c.set({{3, c.u32(humans)}, {4, 2}, {5, c.u32(club + 0x90)}});
            c.call(0x10149df0, 0x103af63c);
            human = std::int8_t(c.r(3));
        }
    }
    if (human) {
        c.set({{3, c.u32(humans)}, {4, setting}, {5, club}, {6, c.u32(club + 0x90)}});
        c.call(0x10146830, 0x103af658);
        c.set({{3, c.u32(humans)}, {4, c.u32(club + 0x90)}});
        c.call(0x101466c0, 0x103af664);
        return c.r(3);
    }
    const Tactics t(c, tactics);
    const auto index = t.team(c.i32(club)).formation();
    c.set(4, t.formation_count());
    compare(c, index, std::int32_t(t.formation_count()));
    return t.formation(index);
}

std::uint32_t set_team_formation(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t index,
                                 std::uint32_t managed) {
    Frame frame(c, 0x103af6c0, 0xa0, {.first_gpr = 27});
    c.set({{27, tactics}, {28, index}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103af700, 0x68, 0x40, 0x2a5);
        return 0;
    }
    const Tactics t(c, tactics);
    const auto last = std::int32_t(t.formation_count() + (t.extra_count() - 1));
    if (std::int16_t(index) < 0 || std::int16_t(index) > last) {
        fail(c, frame, 0x103af77c, 0x48, 0x38, 0x2ad);
        return 0;
    }
    // Leftovers of the range check, passed on through team_slot.
    c.set({{5, managed}, {7, t.formation_count()}, {8, word(std::int16_t(index))}});
    const auto team = team_record(c, tactics, club, managed, 0x103af7c8);
    if (!team) return 0;
    team->set_formation(index);
    c.set(4, t.teams());
    return 1;
}

std::uint32_t squad_player(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t slot, std::uint32_t managed) {
    Frame frame(c, 0x103af800, 0xb0, {.first_gpr = 25});
    c.set({{25, slot}, {26, c.toc(success_slot)}, {27, tactics}, {28, club}, {29, c.toc(pool_slot)},
           {30, c.toc(message_slot)}, {31, c.toc(staff_slot)}});
    if (!club) {
        fail(c, frame, 0x103af844, 0x68, 0x40, 0x2d1);
        return 0;
    }
    if (!in_range(slot, 0, squad_size - 1)) {
        fail(c, frame, 0x103af8b0, 0x48, 0x38, 0x2d9);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103af8fc);
    if (!team) return 0;
    const std::int32_t index = std::int8_t(slot);
    c.set(4, team->address + scaled(slot, 4));  // the slot's entry less 4
    const auto id = team->player(index);
    compare(c, id, -1);
    if (id <= -1) return 0;
    const auto staff_present = c.u32(staff_record(c, id) + 0x14);
    compare(c, staff_present, 0u);
    if (!staff_present) {
        team->set_player(index, none);
        return 0;
    }
    // The original keeps these offsets in r26, r29, r30 across the call.
    c.set({{26, std::uint32_t(id) * staff_size}, {29, std::uint32_t(team->index) * team_size}, {30, scaled(slot, 4)},
           {3, club}});
    c.call(0x100c7d00, 0x103af960);
    compare(c, std::int8_t(c.r(3)));
    if (std::int8_t(c.r(3))) {
        // A national team keeps only players whose record at staff +0x20
        // has the team's +0x2c.
        const auto record = c.u32(staff_record(c, id) + 0x20);
        compare(c, record, 0u);
        bool kept = false;
        if (record) {
            compare(c, c.u32(record + 0x2c), c.u32(club + 0x2c));
            kept = c.u32(record + 0x2c) == c.u32(club + 0x2c);
        }
        if (!kept) {
            const auto again = Tactics(c, tactics).team(team->index);  // re-read after the call
            c.set({{4, again.address + scaled(slot, 4)}, {5, none}});
            again.set_player(index, none);
            return 0;
        }
    }
    return staff_record(c, id);
}

std::uint32_t set_squad_player(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t slot, std::uint32_t staff,
                               std::uint32_t managed) {
    Frame frame(c, 0x103af9d0, 0xd0, {.first_gpr = 24});
    c.set({{24, tactics}, {25, club}, {26, slot}, {27, staff}, {28, managed}, {29, c.toc(success_slot)},
           {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103afa18, 0x90, 0x48, 0x315);
        return 0;
    }
    if (!in_range(slot, 0, squad_size - 1)) {
        fail(c, frame, 0x103afa84, 0x70, 0x40, 0x31d);
        return 0;
    }
    if (!staff) {
        fail(c, frame, 0x103afae4, 0x50, 0x38, 0x325);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103afb30);
    if (!team) return 0;
    team->set_player(std::int8_t(slot), c.u32(staff));
    c.set({{4, club}, {5, managed}, {6, team->address + scaled(slot, 4)}, {7, c.u32(staff)}});
    c.link(0x103afb70);
    forget_backup(c, tactics, club, managed);
    return 1;
}

std::uint32_t remove_player(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t staff, std::uint32_t managed,
                            std::uint32_t roles) {
    Frame frame(c, 0x103afb90, 0xb0, {.first_gpr = 24});
    c.set({{24, c.toc(success_slot)}, {25, c.toc(message_slot)}, {26, c.toc(pool_slot)}, {27, tactics}, {28, club},
           {29, staff}, {30, managed}, {31, roles}});
    if (!club) {
        fail(c, frame, 0x103afbd8, 0x68, 0x40, 0x34d);
        return 0;
    }
    if (!staff) {
        fail(c, frame, 0x103afc38, 0x48, 0x38, 0x355);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103afc84);
    if (!team) return 0;
    std::uint32_t removed = 0;
    for (std::int32_t slot = 0; slot < squad_size; ++slot) {
        if (c.i32(staff) != team->player(slot)) continue;
        team->set_player(slot, none);
        removed = 1;
    }
    // Leftovers of the unrolled loop; r24 holds `removed` across the calls.
    c.set({{24, removed}, {4, Tactics(c, tactics).teams() + 0x50}, {5, std::uint32_t(team->index) * team_size},
           {6, c.u32(staff)}, {7, 0x50}, {8, 0x10}});
    compare(c, std::int8_t(roles));
    if (std::int8_t(roles)) {
        for (const auto& [which, ask, clear] : {std::tuple{0u, 0x103afda0u, 0x103afdc0u}, {1u, 0x103afdd4u, 0x103afdf4u}}) {
            c.set({{4, club}, {5, which}, {6, managed}});
            c.link(ask);
            if (captain(c, tactics, club, which, managed) != staff) continue;
            c.set({{4, club}, {5, which}, {6, 0}, {7, managed}});
            c.link(clear);
            set_captain(c, tactics, club, which, 0, managed);
        }
        for (std::uint32_t kind = 0; kind < std::uint32_t(set_pieces); ++kind) {
            for (std::uint32_t order = 0; order < 2; ++order) {
                c.set({{25, kind}, {26, order}, {4, club}, {5, kind}, {6, order}, {7, managed}});
                c.link(0x103afe1c);
                if (set_piece_taker(c, tactics, club, kind, order, managed) != staff) continue;
                c.set({{4, club}, {5, kind}, {6, order}, {7, 0}, {8, managed}});
                c.link(0x103afe40);
                set_set_piece_taker(c, tactics, club, kind, order, 0, managed);
                removed = 1;
                c.set(24, removed);
            }
        }
    }
    c.set({{4, club}, {5, managed}});
    c.link(0x103afe74);
    forget_backup(c, tactics, club, managed);
    return removed;
}

std::uint32_t clear_squad_player(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t slot,
                                 std::uint32_t managed) {
    Frame frame(c, 0x103afe90, 0xb0, {.first_gpr = 25});
    c.set({{25, tactics}, {26, club}, {27, slot}, {28, managed}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)},
           {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103afed4, 0x68, 0x40, 0x3a8);
        return 0;
    }
    if (!in_range(slot, 0, squad_size - 1)) {
        fail(c, frame, 0x103aff40, 0x48, 0x38, 0x3b0);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103aff8c);
    if (!team) return 0;
    team->set_player(std::int8_t(slot), none);
    c.set({{4, club}, {5, managed}});
    c.link(0x103affcc);
    forget_backup(c, tactics, club, managed);
    return 1;
}

std::uint32_t backup_squad(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed, std::uint32_t keep,
                           std::uint32_t restore) {
    Frame frame(c, 0x103afff0, 0x90, {.first_gpr = 23});
    c.set({{23, keep}, {24, restore}, {25, c.toc(game_slot)}, {26, c.toc(message_slot)}, {27, c.toc(pool_slot)},
           {28, tactics}, {29, club}, {30, managed}, {31, c.toc(staff_count_slot)}});
    if (!club) {
        fail(c, frame, 0x103b003c, 0x40, 0x38, 0x3d5, true);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b0088);
    if (!team) return 0;
    const Tactics t(c, tactics);
    const auto team_offset = std::uint32_t(team->index) * team_size;
    compare(c, std::int8_t(restore));
    if (!std::int8_t(restore)) {
        // Saves the squad in the current human's backup when `keep`.
        const auto manager = c.u32(human_entry(c) + 4);
        if (manager && std::int8_t(keep)) {
            const auto index = std::int32_t(human_distance(c, manager) - 1);
            if (index >= 0 && index < human_count) {
                const auto backup = t.backup(index);
                if (!backup.active()) t.add_backup_count(1);
                backup.set_team(club, managed);
                backup.set_active(true);
                for (std::int32_t slot = 0; slot < squad_size; ++slot) backup.set_player(slot, word(team->player(slot)));
                c.set(7, tactics + std::uint32_t(index) * backup_size + 0x3c);  // the copy's leftover
            }
        }
        for (std::int32_t slot = 0; slot < squad_size; ++slot) team->set_player(slot, none);
        c.set({{4, none}, {5, team_offset}, {6, 0x50}});
        compare(c, 0, squad_size);
        return 1;
    }
    compare(c, std::int8_t(keep));
    if (!std::int8_t(keep)) {
        c.set({{4, club}, {5, managed}});
        c.link(0x103b047c);
        forget_backup(c, tactics, club, managed);
        return 1;
    }
    // Puts the current human's backup of this team back, and drops it.
    const auto manager = c.u32(human_entry(c) + 4);
    c.set(4, manager);
    compare(c, manager, 0u);
    if (!manager) return 1;
    const auto distance = human_distance(c, manager);
    const auto index = std::int32_t(distance - 1);
    c.set(4, distance);
    compare(c, index);
    if (index < 0) return 1;
    compare(c, index, human_count);
    if (index >= human_count) return 1;
    const auto backup = t.backup(index);
    const auto base = tactics + std::uint32_t(index) * backup_size;  // the original's pointer (backup - 0x30)
    c.set({{4, base}, {5, sign8(c.u8(backup.address + field::backup_active))}});
    if (backup.active()) {
        const auto owner = c.u32(backup.address + field::backup_club);
        c.set(5, owner);
        if (owner == club) {
            const auto flag = std::int8_t(c.u8(backup.address + field::backup_managed));
            c.set({{5, sign8(managed)}, {6, word(flag)}});
            if (flag == std::int8_t(managed)) {
                for (std::int32_t slot = 0; slot < squad_size; ++slot) team->set_player(slot, backup.player(slot));
                c.set({{5, backup.player(squad_size - 1)}, {6, base + 0x3c}, {7, team_offset}, {8, 0x50}});
            }
        }
    }
    compare(c, std::int8_t(c.u8(backup.address + field::backup_active)));
    if (backup.active()) t.add_backup_count(-1);
    backup.set_active(false);
    return 1;
}

std::uint32_t has_backup(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed) {
    const Tactics t(c, tactics);
    c.set(6, c.toc(game_slot));
    compare(c, t.backup_count());
    if (!t.backup_count()) return 0;
    const auto entry = human_entry(c), manager = c.u32(entry + 4);
    c.set({{6, entry}, {7, manager}});
    compare(c, manager, 0u);
    if (!manager) return 0;
    const auto distance = human_distance(c, manager);
    const auto index = std::int32_t(distance - 1);
    c.set({{6, distance}, {7, c.u32(manager)}});
    compare(c, index);
    if (index < 0) return 0;
    compare(c, index, human_count);
    if (index >= human_count) return 0;
    const auto backup = t.backup(index);
    compare(c, std::int8_t(c.u8(backup.address + field::backup_active)));
    if (!backup.active()) return 0;
    const auto owner = c.u32(backup.address + field::backup_club);
    compare(c, owner, club);
    if (owner != club) return 0;
    const auto flag = std::int8_t(c.u8(backup.address + field::backup_managed));
    compare(c, flag, std::int8_t(managed));
    return flag == std::int8_t(managed);
}

bool forget_backup(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed) {
    const Tactics t(c, tactics);
    compare(c, t.backup_count());
    if (!t.backup_count()) return false;
    std::uint32_t last = 0;  // r5: the last backup's pointer, or the count it decremented
    for (std::int32_t human = 0; human < human_count; ++human) {
        const auto backup = t.backup(human);
        last = tactics + std::uint32_t(human) * backup_size;
        if (!backup.holds(club, managed)) continue;
        backup.set_active(false);
        last = c.u8(tactics + field::backup_count);
        t.add_backup_count(-1);
    }
    c.set({{5, last}, {6, 0}, {7, sign8(managed)}, {8, std::uint32_t(human_count) * backup_size}, {9, word(human_count)}});
    compare(c, human_count, human_count);
    return true;
}

std::int32_t squad_slot_of(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t staff, std::uint32_t managed) {
    Frame frame(c, 0x103b05c0, 0xa0, {.first_gpr = 27});
    c.set({{27, tactics}, {28, staff}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103b0600, 0x68, 0x40, 0x48a);
        return 0;
    }
    if (!staff) {
        fail(c, frame, 0x103b0660, 0x48, 0x38, 0x492);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b06ac);
    if (!team) return 0;
    c.set({{4, std::uint32_t(team->index) * team_size}, {5, Tactics(c, tactics).teams()}});
    for (std::int32_t slot = 0; slot < squad_size; ++slot) {
        const auto id = c.i32(staff), entry = team->player(slot);
        c.set(6, word(id));
        compare(c, id, entry);
        if (id != entry) continue;
        c.set({{7, word(slot) * 4}, {8, word(slot)}});
        return slot;
    }
    c.set({{7, word(squad_size) * 4}, {8, word(squad_size)}});
    return -1;
}

std::uint32_t swap_squad_players(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t first,
                                 std::uint32_t second, std::uint32_t managed) {
    Frame frame(c, 0x103b07c0, 0xd0, {.first_gpr = 24});
    c.set({{24, first}, {25, second}, {26, c.toc(success_slot)}, {27, c.toc(message_slot)}, {28, c.toc(pool_slot)},
           {29, tactics}, {30, club}, {31, managed}});
    if (!club) {
        fail(c, frame, 0x103b0808, 0x90, 0x48, 0x4bf);
        return 0;
    }
    if (!in_range(first, 0, squad_size - 1)) {
        fail(c, frame, 0x103b0874, 0x70, 0x40, 0x4c7);
        return 0;
    }
    if (!in_range(second, 0, squad_size - 1)) {
        fail(c, frame, 0x103b08e0, 0x50, 0x38, 0x4cf);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b092c);
    if (!team) return 0;
    c.set({{3, team->squad_entry(std::int8_t(first))}, {4, team->squad_entry(std::int8_t(second))}, {5, 4},
           {6, Tactics(c, tactics).teams()}});
    c.call(0x10745e00, 0x103b0970);  // swaps the two words
    // The original inlines forget_backup here, with other registers.
    const auto r8 = c.r(8), r9 = c.r(9);
    if (forget_backup(c, tactics, club, managed))
        c.set({{4, 0}, {5, sign8(managed)}, {6, word(human_count)}, {7, std::uint32_t(human_count) * backup_size}, {8, r8},
               {9, r9}});
    return 1;
}

std::int32_t pick_slot(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed, std::uint32_t missing) {
    Frame frame(c, 0x103b0a00, 0x80, {.first_gpr = 27});
    c.set({{27, tactics}, {28, missing}, {29, c.toc(staff_slot)}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103b0a40, 0x40, 0x38, 0x4f5, true);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b0a8c);
    if (!team) return 0;
    c.set({{4, c.u32(c.toc(staff_slot))}, {6, std::uint32_t(team->index) * team_size}, {8, team->address}});
    for (std::int32_t slot = 0; slot < squad_size; ++slot) {
        const auto id = team->player(slot);
        c.set({{5, word(slot) * 4 + 4}, {7, word(slot) * 4}});
        if (id == -1) continue;
        c.set(5, word(team->pick()));
        if (team->pick() == c.i32(staff_record(c, id))) {
            compare(c, 0, 0);
            return slot;
        }
    }
    c.set(7, word(squad_size) * 4);
    compare(c, squad_size, squad_size);
    return std::int8_t(missing) ? -1 : 0;
}

std::uint32_t pick(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t managed) {
    Frame frame(c, 0x103b0b20, 0x70, {.first_gpr = 29});
    c.set({{29, tactics}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103b0b60, 0x40, 0x38, 0x527, true);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b0bac);
    return team ? word(team->pick()) : 0;
}

std::uint32_t set_pick_slot(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t slot, std::uint32_t managed) {
    Frame frame(c, 0x103b0bf0, 0xa0, {.first_gpr = 27});
    c.set({{27, tactics}, {28, slot}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103b0c30, 0x68, 0x40, 0x547);
        return 0;
    }
    if (!in_range(slot, 0, squad_size - 1)) {
        fail(c, frame, 0x103b0c9c, 0x48, 0x38, 0x54f);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b0ce8);
    if (!team) return 0;
    const auto id = team->player(std::int8_t(slot));
    c.set(4, team->address + scaled(slot, 4));
    compare(c, id, -1);
    if (id == -1) return 0;
    team->set_pick(word(id));
    return 1;
}

std::uint32_t set_pick(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t staff, std::uint32_t managed) {
    Frame frame(c, 0x103b0d40, 0xa0, {.first_gpr = 27});
    c.set({{27, tactics}, {28, staff}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103b0d80, 0x68, 0x40, 0x579);
        return 0;
    }
    if (!staff) {
        fail(c, frame, 0x103b0de0, 0x48, 0x38, 0x581);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b0e2c);
    if (!team) return 0;
    team->set_pick(c.u32(staff));
    c.set({{4, team->address}, {5, c.u32(staff)}});
    return 1;
}

std::uint32_t set_piece_taker(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t kind, std::uint32_t order,
                              std::uint32_t managed) {
    Frame frame(c, 0x103b0e70, 0xd0, {.first_gpr = 24});
    c.set({{24, kind}, {25, tactics}, {26, club}, {27, c.toc(pool_slot)}, {28, c.toc(message_slot)},
           {29, c.toc(success_slot)}, {30, order}, {31, c.toc(staff_slot)}});
    if (!club) {
        fail(c, frame, 0x103b0eb8, 0x90, 0x48, 0x5a5);
        return 0;
    }
    if (!in_range(kind, 0, set_pieces - 1)) {
        fail(c, frame, 0x103b0f24, 0x70, 0x40, 0x5ad);
        return 0;
    }
    if (!in_range(order, 0, 1)) {
        fail(c, frame, 0x103b0f90, 0x50, 0x38, 0x5b5);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b0fdc);
    if (!team) return 0;
    const std::int32_t set_piece = std::int8_t(kind), position = std::int8_t(order);
    // The entry less 0x60, and the team record (passed on to 0x100c7d00).
    c.set({{4, team->address + scaled(kind, 8) + scaled(order, 4)}, {5, team->address}});
    const auto id = team->taker(set_piece, position);
    // Quirk: an id is out of range only beyond the staff count plus 0x52d.
    if (id > std::int32_t(c.u32(c.toc(staff_count_slot)) + 0x52d)) {
        team->set_taker(set_piece, position, none);
        compare(c, -1, -1);
        return 0;
    }
    compare(c, id, -1);
    if (id <= -1) return 0;
    const auto staff_present = c.u32(staff_record(c, id) + 0x14);
    if (!staff_present) {
        compare(c, 0u, 0u);
        team->set_taker(set_piece, position, none);
        return 0;
    }
    // A taker who left the club stays only for a national team or when
    // 0x102b7c00 still accepts him.
    if (c.u32(staff_record(c, id) + 0x24) != club) {
        // The original keeps these in r27-r30 across the calls.
        c.set({{27, word(id)}, {28, scaled(order, 4)}, {29, std::uint32_t(team->index) * team_size}, {30, scaled(kind, 8)},
               {3, club}});
        c.call(0x100c7d00, 0x103b107c);
        if (!std::int8_t(c.r(3))) {
            c.set({{3, c.u32(c.toc(selection_slot))}, {4, club}});
            c.call(0x102b7c00, 0x103b1094);
            if (!std::int8_t(c.r(3))) {
                compare(c, std::int8_t(c.r(3)));
                const auto again = Tactics(c, tactics).team(team->index);  // re-read after the calls
                c.set({{4, again.address + scaled(kind, 8) + scaled(order, 4)}, {5, none}});
                again.set_taker(set_piece, position, none);
                return 0;
            }
        }
        compare(c, id, -1);
    }
    return staff_record(c, id);
}

std::uint32_t set_set_piece_taker(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t kind, std::uint32_t order,
                                  std::uint32_t staff, std::uint32_t managed) {
    Frame frame(c, 0x103b10f0, 0xd0, {.first_gpr = 25});
    c.set({{25, kind}, {26, staff}, {27, c.toc(success_slot)}, {28, c.toc(message_slot)}, {29, c.toc(pool_slot)},
           {30, tactics}, {31, order}});
    if (!club) {
        fail(c, frame, 0x103b1138, 0x90, 0x48, 0x605);
        return 0;
    }
    if (!in_range(kind, 0, set_pieces - 1)) {
        fail(c, frame, 0x103b11a4, 0x70, 0x40, 0x60d);
        return 0;
    }
    if (!in_range(order, 0, 1)) {
        fail(c, frame, 0x103b1210, 0x50, 0x38, 0x615);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b125c);
    if (!team) return 0;
    const std::int32_t set_piece = std::int8_t(kind), position = std::int8_t(order);
    const auto teams = Tactics(c, tactics).teams(), team_offset = std::uint32_t(team->index) * team_size;
    compare(c, staff, 0u);
    if (staff) {
        team->set_taker(set_piece, position, c.u32(staff));
        c.set({{4, team->address}, {5, teams}, {6, c.u32(staff)}});
        return 1;
    }
    team->set_taker(set_piece, position, none);
    c.set({{4, none}, {6, team_offset}, {7, scaled(kind, 8)}, {9, scaled(order, 4)}});
    compare(c, position, 1);
    // Clearing the first taker moves the next one set up.
    for (std::int32_t next = std::int8_t(order + 1); next < 2; ++next) {
        c.set({{4, team_offset + scaled(kind, 8)}, {5, word(next) * 4}, {8, teams}, {10, word(next)}});
        compare(c, team->taker(set_piece, next), -1);
        if (team->taker(set_piece, next) <= -1) {
            c.set({{5, word(next + 1) * 4}, {10, word(next + 1)}});
            compare(c, next + 1, 2);
            continue;
        }
        team->set_taker(set_piece, position, word(team->taker(set_piece, next)));
        team->set_taker(set_piece, next, none);
        c.set(4, none);
        break;
    }
    return 1;
}

std::uint32_t captain(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t which, std::uint32_t managed) {
    Frame frame(c, 0x103b1370, 0xa0, {.first_gpr = 27});
    c.set({{27, tactics}, {28, which}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103b13b0, 0x68, 0x40, 0x64a);
        return 0;
    }
    if (!in_range(which, 0, 1)) {
        fail(c, frame, 0x103b141c, 0x48, 0x38, 0x652);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b1468);
    if (!team) return 0;
    const auto id = team->captain(std::int8_t(which));
    c.set(4, Tactics(c, tactics).teams());
    compare(c, id, -1);
    return id > -1 ? staff_record(c, id) : 0;
}

std::uint32_t set_captain(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t which, std::uint32_t staff,
                          std::uint32_t managed) {
    Frame frame(c, 0x103b14d0, 0xa0, {.first_gpr = 26});
    c.set({{26, which}, {27, staff}, {28, c.toc(success_slot)}, {29, c.toc(message_slot)}, {30, c.toc(pool_slot)},
           {31, tactics}});
    if (!club) {
        fail(c, frame, 0x103b1514, 0x68, 0x40, 0x675);
        return 0;
    }
    if (!in_range(which, 0, 1)) {
        fail(c, frame, 0x103b1580, 0x48, 0x38, 0x67d);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b15cc);
    if (!team) return 0;
    const std::int32_t role = std::int8_t(which);
    if (staff) {
        team->set_captain(role, c.u32(staff));
        c.set({{4, Tactics(c, tactics).teams()}, {5, c.u32(staff)}});
        compare(c, staff, 0u);
        return 1;
    }
    team->set_captain(role, none);
    if (role == 0) {  // the vice-captain moves up
        team->set_captain(0, word(team->captain(1)));
        team->set_captain(1, none);
    }
    c.set({{4, none}, {5, std::uint32_t(team->index) * team_size}});
    compare(c, role);
    return 1;
}

std::uint32_t lineup_slot(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t place, std::uint32_t managed) {
    Frame frame(c, 0x103b1670, 0xa0, {.first_gpr = 27});
    c.set({{27, tactics}, {28, place}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103b16b0, 0x68, 0x40, 0x6a9);
        return 0;
    }
    if (!in_range(place, 0, 10)) {
        fail(c, frame, 0x103b171c, 0x48, 0x38, 0x6b1);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b1768);
    if (!team) return 0;
    c.set(4, Tactics(c, tactics).teams());
    return team->lineup(std::int8_t(place));
}

std::uint32_t set_lineup_slot(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t place, std::uint32_t slot,
                              std::uint32_t managed) {
    Frame frame(c, 0x103b17b0, 0xd0, {.first_gpr = 26});
    c.set({{26, tactics}, {27, place}, {28, slot}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)},
           {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103b17f4, 0x90, 0x48, 0x6d4);
        return 0;
    }
    if (!in_range(place, 0, 10)) {
        fail(c, frame, 0x103b1860, 0x70, 0x40, 0x6dc);
        return 0;
    }
    if (!in_range(slot, -1, squad_size - 1)) {
        fail(c, frame, 0x103b18cc, 0x50, 0x38, 0x6e4);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b1918);
    if (!team) return 0;
    team->set_lineup(std::int8_t(place), slot);
    c.set({{4, team->address + sign8(place)}, {5, Tactics(c, tactics).teams()}});
    return 1;
}

std::int32_t team_slot(Cpu& c, std::uint32_t club, std::uint32_t managed) {
    Frame frame(c, 0x103b1960, 0xb0, {.first_gpr = 25});
    const auto staff_count = c.toc(staff_count_slot), club_count = c.toc(club_count_slot);
    c.set({{25, managed}, {26, staff_count}, {27, club}, {28, club_count}, {29, c.toc(success_slot)},
           {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    c.set({{3, club}, {4, 1}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x103b19a4);
    compare(c, std::int8_t(c.r(3)));
    if (!std::int8_t(c.r(3))) return c.i32(club);
    compare(c, std::int8_t(managed));
    if (!std::int8_t(managed)) return c.i32(club);
    c.set(3, club);
    c.call(0x100ce250, 0x103b19bc);
    compare(c, std::int8_t(c.r(3)));
    if (std::int8_t(c.r(3))) return -1;
    const auto manager = c.u32(club + 0x90);
    c.set(25, manager);
    if (!manager) {
        fail(c, frame, 0x103b19ec, 0x68, 0x40, 0x742);
        return c.i32(club);
    }
    c.set(3, club);
    c.call(0x100c7d00, 0x103b1a38);
    // Humans hold the last 16 staff ids: their clubs' records follow the
    // clubs', their national teams' records follow those.
    const auto national = std::int8_t(c.r(3)) != 0;
    const auto clubs = c.u32(club_count);
    const auto slot = std::int32_t(c.u32(staff_count) - c.u32(manager) + clubs + (national ? 15u : 0u - 1u));
    const auto limit = std::int32_t(clubs + 0x20);
    if (slot >= 0 && slot < limit) {
        c.set(4, clubs);
        compare(c, slot, limit);
        return slot;
    }
    fail(c, frame, 0x103b1aa0, 0x48, 0x38, 0x755);
    return c.i32(club);
}

std::uint32_t copy_team(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t destination,
                        std::uint32_t managed) {
    Frame frame(c, 0x103b1b10, 0x70, {.first_gpr = 28});
    c.set({{28, tactics}, {29, destination}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103b1b58, 0x40, 0x38, 0x778, true);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b1ba8);
    if (!team) return 0;
    c.set({{3, team->address}, {4, destination}, {5, team_size}});
    c.call(0x1077fc28, 0x103b1bd0);  // BlockMoveData through the glue, which saves r2 at sp+0x14
    c.set(2, c.u32(frame.at(0x14)));
    return 1;
}

std::uint32_t restore_team(Cpu& c, std::uint32_t tactics, std::uint32_t club, std::uint32_t source, std::uint32_t managed) {
    Frame frame(c, 0x103b1c00, 0x70, {.first_gpr = 28});
    c.set({{28, tactics}, {29, source}, {30, c.toc(message_slot)}, {31, c.toc(pool_slot)}});
    if (!club) {
        fail(c, frame, 0x103b1c48, 0x40, 0x38, 0x79d, true);
        return 0;
    }
    const auto team = team_record(c, tactics, club, managed, 0x103b1c98);
    if (!team) return 0;
    const auto count = Tactics(c, tactics).team_count();
    compare(c, team->index, count);
    if (team->index >= count) return 0;
    c.set({{3, source}, {4, team->address}, {5, team_size}});
    c.call(0x1077fc28, 0x103b1cd4);
    c.set(2, c.u32(frame.at(0x14)));
    return 1;
}
}

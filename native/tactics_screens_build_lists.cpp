// TACTICS_SCREENS.CPP: the callbacks the tactics screen's player lists and
// pitch call (tactics_screens_build.hpp, section "lists").
#include "tactics_screens_build.hpp"
#include "floating_point.hpp"
#include "formation.hpp"
#include "formation_zones.hpp"
#include "tactics.hpp"
#include "tactics_leaves.hpp"
#include <cmath>

namespace cm::aot::tactics_screens::build {
using guest::Cpu;
using guest::Frame;
using detail::fail;
namespace {
// Screen state fields these callbacks read.
constexpr std::uint32_t opponent = 0xe64;      // -> the opposing club, 0 none
constexpr std::uint32_t live_side = 0xe69;     // int8: the side of the match whose live entries the lists show
constexpr std::uint32_t match_club = 0x3100;
constexpr std::uint32_t role_bits = 0x5b8;     // formation record: uint8[11] role bits per slot   // match: a club id compared with a live entry's club
constexpr std::int32_t text_slot = -0x7bb8;    // -> the shared text buffer
constexpr std::int32_t constants_slot = -0x4cb0;  // -> TACTICS_SCREENS.CPP's double constants
constexpr std::int32_t tactics_slot = -0x7a6c;    // -> CTACTICS
constexpr std::int32_t players_slot = -0x7ba0;    // -> the object 0x10187680 / 0x101878d0 ask

std::uint32_t pool(Cpu& c) { return c.toc(detail::pool_slot); }
std::uint32_t widened(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
std::uint32_t not_live(std::uint32_t live) { return std::int8_t(live) == 0; }
// cmpwi / cmplwi into cr0, as the original leaves it.
void compare(Cpu& c, std::int32_t a, std::int32_t b) { c.compare(0, a, b); }
void compare_logical(Cpu& c, std::uint32_t a, std::uint32_t b) { c.compare_logical(0, a, b); }
// The index check every reader opens with (extsb; cmpwi 0; blt; cmpwi last; ble).
bool outside(Cpu& c, std::uint32_t index, std::int8_t last) {
    compare(c, std::int8_t(index), 0);
    if (std::int8_t(index) < 0) return true;
    compare(c, std::int8_t(index), last);
    return std::int8_t(index) > last;
}
// `live` readers need an opponent, and every reader a match.
bool opponent_ok(Cpu& c, std::uint32_t screen, std::uint32_t live) {
    compare(c, std::int8_t(live), 0);
    if (!std::int8_t(live)) return true;
    compare_logical(c, c.u32(screen + opponent), 0);
    return c.u32(screen + opponent) != 0;
}
bool in_match(Cpu& c, std::uint32_t screen) {
    compare_logical(c, c.u32(screen + field::match), 0);
    return c.u32(screen + field::match) != 0;
}
// The screen's own copy of a squad entry.
std::uint32_t own_entry(std::uint32_t screen, std::uint32_t index) {
    return screen + field::players + widened(index) * field::player_entry_size;
}
// A squad entry: the match's live one (0x101f9440, for the side shown) or the
// screen's copy; `reg` holds it afterwards, as in the original.
std::uint32_t entry_of(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live, unsigned reg,
                       std::uint32_t back) {
    compare(c, std::int8_t(live), 0);
    std::uint32_t entry = own_entry(screen, index);
    if (std::int8_t(live)) {
        c.set({{3, c.u32(screen + field::match)}, {4, c.u8(screen + live_side)}, {5, index}});
        c.call(0x101f9440, back);
        entry = c.r(3);
    }
    c.set(reg, entry);
    return entry;
}
// A squad entry's status (0x101d3b50): -5 empty, -1 and -2 unavailable kinds.
std::int8_t status(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    c.set(3, entry);
    c.call(0x101d3b50, back);
    return std::int8_t(c.r(3));
}
// The formation record of a match side (leaf 0x103c5940, which leaves the
// offset in r5).
std::uint32_t side_formation(Cpu& c, std::uint32_t match, std::uint32_t side) {
    c.set(5, std::uint32_t(std::int32_t(std::int8_t(side)) * 0x18e0));
    return match + c.r(5) + 0x7c6c;
}

// Double arithmetic as the generated code does it, with its FPSCR bits (one
// byte per bit at 1280 + n): the result class, the sticky invalid bit for a
// NaN operand, and the summaries. fadd and fmul also fold FI into XX; fsub
// does not.
class Fpu {
public:
    explicit Fpu(Cpu& c) : s(c.s) {}
    double add(double a, double b) { return status(a + b, a, b, true); }
    double sub(double a, double b) { return status(a - b, a, b, false); }
    double mul(double a, double b) { return status(a * b, a, b, true); }
    // fctiwz into f0, as the generated code lowers it; returns f0's 64 bits.
    std::uint64_t truncate(double value) {
        s.wr<8>(guest::reg::fpr(0), std::bit_cast<std::uint64_t>(value));
        original_fctiwz(s, guest::reg::fpr(0), guest::reg::fpr(0));
        return s.r<8>(guest::reg::fpr(0));
    }

private:
    static constexpr std::size_t fx = 1280, fex = 1281, vx = 1282, ox = 1283, ux = 1284, zx = 1285, xx = 1286,
                                 vxsnan = 1287, fi = 1294, fl = 1296, fg = 1297, fe = 1298, fu = 1299;
    std::uint8_t get(std::size_t bit) const { return std::uint8_t(s.r<1>(bit)); }
    void set(std::size_t bit, std::uint32_t value) { s.wr<1>(bit, value); }
    double status(double r, double a, double b, bool inexact) {
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
        return r;
    }
    State& s;
};

// The original's 32-bit integer steps (division by constants), with the
// carry (XER CA) srawi and addze leave.
std::int32_t mulhw(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * b) >> 32); }
std::int32_t sign(std::int32_t value) { return std::int32_t(std::uint32_t(value) >> 31); }
std::int32_t times(std::int32_t value, std::int32_t factor) { return std::int32_t(std::uint32_t(value) * std::uint32_t(factor)); }
std::int32_t srawi(Cpu& c, std::int32_t value, unsigned shift) {
    c.s.wr<1>(guest::reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << shift) - 1)) != 0);
    return value >> shift;
}
std::int32_t addze(Cpu& c, std::int32_t value) {
    const auto carry = std::uint32_t(c.s.r<1>(guest::reg::xer_ca) & 1);
    c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(value) == 0xffffffffu && carry);
    return std::int32_t(std::uint32_t(value) + carry);
}
// value * factor / 6, as mulhw 0x2aaaaaab and the sign correction compute it.
std::int32_t sixths(std::int32_t value, std::int32_t factor) {
    const auto q = mulhw(0x2aaaaaab, times(value, factor));
    return q + sign(q);
}
// value * factor / 10.
std::int32_t tenths(Cpu& c, std::int32_t value, std::int32_t factor) {
    const auto q = srawi(c, mulhw(0x66666667, times(value, factor)), 2);
    return q + sign(q);
}
// value * factor / 8 (srawi 3; addze).
std::int32_t eighths(Cpu& c, std::int32_t value, std::int32_t factor) {
    return addze(c, srawi(c, times(value, factor), 3));
}
}

namespace {
// formation spot (0x101124c0) through the dispatcher: a player's spot for a
// ball state and cell into out[2].
void spot(Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t with_ball, std::uint32_t cell,
          std::uint32_t out, std::uint32_t back) {
    c.set({{3, screen + field::formation}, {4, player}, {5, widened(with_ball)}, {6, cell}, {7, out}});
    c.call(0x101124c0, back);
}
}

std::uint32_t list_spot(Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t out) {
    Frame frame(c, 0x103c78a0, 0x80, {.first_gpr = 26});
    c.set({{31, screen}, {30, player}, {26, out}, {29, pool(c)}});
    if (outside(c, player, 10)) {
        fail(c, frame, 0x103c78ec, 0x3d, 0x38, 0x1757);
        return 0;
    }
    // The ball state is the screen's view in list view 1, the list view itself otherwise.
    compare(c, c.i8(screen + field::list_view), 1);
    if (c.i8(screen + field::list_view) == 1) {
        compare(c, c.i8(screen + field::view), 2);
        const std::uint32_t with_ball = c.i8(screen + field::view) == 2;
        c.set(27, with_ball);
        spot(c, screen, player, with_ball, c.u32(screen + field::ball_cell), out, 0x103c79b0);
    } else {
        compare(c, c.i8(screen + field::list_view), 2);
        const std::uint32_t with_ball = c.i8(screen + field::list_view) == 2;
        c.set(28, with_ball);
        spot(c, screen, player, with_ball, c.u32(screen + field::ball_cell), out, 0x103c7978);
    }
    return 1;
}

std::uint32_t shown_spot(Cpu& c, std::uint32_t screen, std::uint32_t player, std::uint32_t out) {
    Frame frame(c, 0x103c79d0, 0x70, {.first_gpr = 28});
    c.put32(frame.at(0x90), out);
    c.set({{31, screen}, {28, player}, {30, pool(c)}});
    if (outside(c, player, 10)) {
        fail(c, frame, 0x103c7a28, 0x3d, 0x38, 0x1772);
        return 0;
    }
    compare(c, c.i8(screen + field::view), 2);
    const std::uint32_t with_ball = c.i8(screen + field::view) == 2;
    c.set(29, with_ball);
    spot(c, screen, player, with_ball, c.u32(screen + field::cell), c.u32(frame.at(0x90)), 0x103c7aa4);
    return 1;
}

std::uint32_t custom_spot(Cpu& c, std::uint32_t screen, std::uint32_t player) {
    Frame frame(c, 0x103c8b60, 0x70, {.first_gpr = 28});
    c.set({{31, screen}, {30, player}, {29, pool(c)}});
    if (outside(c, player, 10)) {
        fail(c, frame, 0x103c8bb4, 0x3d, 0x38, 0x19e0);
        return 0;
    }
    const auto formation = screen + field::formation;
    compare(c, c.i8(screen + field::view), 1);
    if (c.i8(screen + field::view) == 1) {
        c.set({{3, formation}, {4, player}});
        c.call(0x10113430, 0x103c8c18);
        return c.r(3);
    }
    compare(c, c.i8(screen + field::view), 2);
    const std::uint32_t with_ball = c.i8(screen + field::view) == 2;
    c.set({{28, with_ball}, {3, formation}, {4, player}, {5, with_ball}, {6, c.u32(screen + field::cell)}});
    c.call(0x10113550, 0x103c8c4c);
    return c.r(3);
}

std::uint32_t entry_unavailable(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live) {
    Frame frame(c, 0x103c9970, 0x80, {.first_gpr = 27});
    c.set({{31, screen}, {30, index}, {27, live}, {29, pool(c)}});
    if (outside(c, index, 19)) {
        fail(c, frame, 0x103c99bc, 0x3d, 0x38, 0x1bd1);
        return 0;
    }
    if (!opponent_ok(c, screen, live) || !in_match(c, screen)) return 0;
    const auto entry = entry_of(c, screen, index, live, 28, 0x103c9a4c);
    const auto state = status(c, entry, 0x103c9a6c);
    compare(c, state, -1);
    return state == -1;
}

namespace {
// 0x103c9aa0 / 0x103c9bd0 / 0x103ca120 / 0x103ca260: the same opening, then a
// byte of the entry. Registers: r31 screen, r29 index, r27 live, r28 pool, r30 entry.
struct ByteReader {
    std::uint32_t entry_point, assert_at, line, entry_back, status_back;
};
template <class Read>
std::uint32_t read_entry(Cpu& c, const ByteReader& r, std::uint32_t screen, std::uint32_t index, std::uint32_t live,
                         Read&& read) {
    Frame frame(c, r.entry_point, 0x80, {.first_gpr = 27});
    c.set({{31, screen}, {29, index}, {27, live}, {28, pool(c)}});
    if (outside(c, index, 19)) {
        fail(c, frame, r.assert_at, 0x3d, 0x38, r.line);
        return 0;
    }
    if (!opponent_ok(c, screen, live) || !in_match(c, screen)) return 0;
    const auto entry = entry_of(c, screen, index, live, 30, r.entry_back);
    const auto state = status(c, entry, r.status_back);
    compare(c, state, -5);
    if (state == -5) return 0;
    return read(entry);
}
}

std::uint32_t entry_byte_24(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live) {
    return read_entry(c, {0x103c9aa0, 0x103c9aec, 0x1c04, 0x103c9b7c, 0x103c9b9c}, screen, index, live,
                      [&](std::uint32_t entry) { return std::uint32_t(c.u8(entry + 0x24)); });
}

std::uint32_t entry_byte_23(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live) {
    return read_entry(c, {0x103c9bd0, 0x103c9c1c, 0x1c37, 0x103c9cac, 0x103c9ccc}, screen, index, live,
                      [&](std::uint32_t entry) { return std::uint32_t(c.u8(entry + 0x23)); });
}

std::uint32_t entry_byte_1b(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live) {
    return read_entry(c, {0x103ca120, 0x103ca16c, 0x1d0b, 0x103ca1fc, 0x103ca21c}, screen, index, live,
                      [&](std::uint32_t entry) {
                          // Status asked again; nothing for an unavailable (-1) entry.
                          const auto again = status(c, entry, 0x103ca230);
                          compare(c, again, -1);
                          if (again == -1) return 0u;
                          return std::uint32_t(c.u8(entry + 0x1b));
                      });
}

std::uint32_t entry_byte_0c(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live) {
    return read_entry(c, {0x103ca260, 0x103ca2ac, 0x1d42, 0x103ca33c, 0x103ca35c}, screen, index, live,
                      [&](std::uint32_t entry) { return widened(c.u8(entry + 0xc)); });
}

std::uint32_t entry_label(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t flag, std::uint32_t size,
                          std::uint32_t colour, std::uint32_t live) {
    Frame frame(c, 0x103c9d00, 0x90, {.first_gpr = 23});
    c.put32(frame.at(0xb4), size);
    const auto text = c.toc(text_slot), highlight = c.toc(-0x7b10);
    c.set({{29, screen}, {26, index}, {24, flag}, {30, colour}, {23, live}, {27, text}, {25, highlight},
           {28, pool(c)}});
    // The checks stop at the first failure, which leaves its compare in cr0.
    const auto bad = [&] {
        if (outside(c, index, 19)) return true;
        compare_logical(c, flag, 0);
        if (!flag) return true;
        compare(c, std::int32_t(c.u32(frame.at(0xb4))), 20);
        if (std::int32_t(c.u32(frame.at(0xb4))) < 20) return true;
        compare_logical(c, colour, 0);
        return !colour;
    };
    if (bad()) {
        fail(c, frame, 0x103c9d7c, 0x3d, 0x38, 0x1c6d);
        return 0;
    }
    c.put8(flag, 0);
    c.put16(colour, c.u16(c.toc(-0x7bd4)));
    if (!opponent_ok(c, screen, live)) return 0;
    if (!in_match(c, screen)) return 1;
    const auto entry = entry_of(c, screen, index, live, 31, 0x103c9e20);
    const auto state = status(c, entry, 0x103c9e40);
    compare(c, state, -5);
    if (state == -5) return 1;
    // The first of four numbered markers (+0x22 down to +0x1f) the entry
    // holds, formatted with its number from 1, and its colour.
    constexpr struct { std::uint32_t at, format, back; } markers[] = {
        {0x22, 0x10af, 0x103c9e74}, {0x21, 0x10c1, 0x103c9ea8}, {0x20, 0x10d3, 0x103c9edc}, {0x1f, 0x10e5, 0x103c9f10}};
    for (const auto& [at, format, back] : markers) {
        compare(c, c.i8(entry + at), -1);
        if (c.i8(entry + at) <= -1) continue;
        c.set({{3, text}, {4, pool(c) + format}, {5, widened(c.u8(entry + at)) + 1}});
        c.call(0x10727890, back);
        c.put16(colour, c.u16(at == 0x1f ? c.toc(-0x7bf4) : highlight));
        break;
    }
    return 1;
}

std::uint32_t entry_rating(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live) {
    Frame frame(c, 0x103c9f40, 0x90, {.first_gpr = 25});
    c.set({{31, screen}, {30, index}, {26, live}, {27, pool(c)}});
    if (outside(c, index, 19)) {
        fail(c, frame, 0x103c9f8c, 0x3d, 0x38, 0x1cc3);
        return 0;
    }
    if (!opponent_ok(c, screen, live)) return 0;
    if (in_match(c, screen)) {
        const auto entry = entry_of(c, screen, index, live, 29, 0x103ca01c);
        const auto state = status(c, entry, 0x103ca03c);
        compare(c, state, -5);
        if (state == -5) return 0;
        // The entry's float at +0x50 scaled by a constant, truncated.
        Fpu fpu(c);
        const auto scaled = fpu.mul(c.f64(c.toc(constants_slot) + 0x10), double(c.f32(entry + 0x50)));
        c.put64(frame.at(0x5c), fpu.truncate(scaled));
        return c.u32(frame.at(0x60));
    }
    // Outside a match, only without a preset: the squad player's own value.
    compare(c, c.i32(screen + field::preset), -1);
    if (c.i32(screen + field::preset) != -1) return 0;
    compare(c, std::int8_t(live), 0);
    std::uint32_t staff;
    if (std::int8_t(live)) {
        staff = c.u32(screen + 0xe70 + widened(index) * 4);
    } else {
        c.set({{3, c.u32(c.toc(tactics_slot))}, {4, c.u32(screen + field::club)}, {5, index},
               {6, c.u8(screen + field::opened_from)}});
        c.call(0x103af800, 0x103ca0ac);
        staff = c.r(3);
    }
    c.set(28, staff);
    compare_logical(c, staff, 0);
    if (!staff) return 0;
    c.set({{3, c.u32(c.toc(players_slot))}, {4, staff}});
    c.call(0x10187680, 0x103ca0c8);
    c.set(25, c.r(3));
    compare(c, std::int8_t(c.r(3)), 2);
    if (std::int8_t(c.r(3)) == 2) return 0xffffffffu;
    c.set({{3, c.u32(c.toc(players_slot))}, {4, staff}, {5, 1}});
    c.call(0x101878d0, 0x103ca0f4);
    return widened(c.r(3));
}

std::uint32_t entry_plays_for_match_club(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live) {
    Frame frame(c, 0x103ca390, 0x80, {.first_gpr = 26});
    c.set({{31, screen}, {28, index}, {26, live}, {27, pool(c)}});
    if (outside(c, index, 19)) {
        fail(c, frame, 0x103ca3dc, 0x3d, 0x38, 0x1d76);
        return 0;
    }
    if (!opponent_ok(c, screen, live) || !in_match(c, screen)) return 0;
    const auto entry = entry_of(c, screen, index, live, 30, 0x103ca46c);
    const auto state = status(c, entry, 0x103ca48c);
    compare(c, state, -5);
    if (state == -5) return 0;
    c.set(3, entry);
    c.call(0x101e5070, 0x103ca4a0);
    const auto club = c.r(3);
    c.set(29, club);
    compare_logical(c, club, 0);
    if (!club) return 0;
    c.set(4, c.u32(club));
    const auto match_id = c.u32(c.u32(screen + field::match) + match_club);
    compare(c, std::int32_t(c.r(4)), std::int32_t(match_id));
    return c.r(4) == match_id;
}

std::uint32_t entry_with_status(Cpu& c, std::uint32_t screen, std::uint32_t value, std::uint32_t found,
                                std::uint32_t live) {
    Frame frame(c, 0x103ca730, 0x80, {.first_gpr = 25});
    c.set({{31, screen}, {28, value}, {27, found}, {25, live}, {26, pool(c)}});
    if (outside(c, value, 10)) {
        fail(c, frame, 0x103ca780, 0x3d, 0x38, 0x1dfe);
        return 0xffffffffu;
    }
    if (!opponent_ok(c, screen, live)) return 0xffffffffu;
    c.put8(found, 0);
    // Outside a match the value itself (the whole register) comes back.
    if (!in_match(c, screen)) return value;
    for (std::uint32_t index = 0;; ++index) {
        compare(c, std::int8_t(index), 20);
        if (std::int8_t(index) >= 20) break;
        c.set(30, index);
        const auto entry = entry_of(c, screen, index, live, 29, 0x103ca820);
        const auto state = status(c, entry, 0x103ca840);
        compare(c, std::int8_t(value), state);
        if (std::int8_t(value) == state) return index;
        const auto again = status(c, entry, 0x103ca860);
        compare(c, again, -2);
        if (again != -2) continue;
        c.set(3, entry);
        c.call(0x101e3f00, 0x103ca874);
        compare(c, std::int8_t(value), std::int8_t(c.r(3)));
        if (std::int8_t(value) == std::int8_t(c.r(3))) {
            c.put8(found, 1);
            return index;
        }
    }
    return 0xffffffffu;
}

std::uint32_t entry_staff(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live) {
    Frame frame(c, 0x103cad40, 0x80, {.first_gpr = 26});
    c.set({{31, screen}, {28, index}, {30, live}, {27, pool(c)}});
    if (outside(c, index, 19)) {
        fail(c, frame, 0x103cad8c, 0x3e, 0x39, 0x1ee4);
        return 0;
    }
    compare_logical(c, c.u32(screen + opponent), 0);
    if (!c.u32(screen + opponent)) return 0;
    const auto slot_at = frame.at(0x38);  // the formation slot 0x103c9810 finds
    c.set({{3, screen}, {4, index}, {5, slot_at}, {6, live}});
    c.call(0x103c9810, 0x103cadfc);
    compare(c, std::int8_t(c.r(3)), 0);
    if (std::int8_t(c.r(3)) == 0) return 0;
    c.set({{3, screen}, {4, index}, {5, live}});
    c.call(0x103ca4e0, 0x103cae18);
    compare_logical(c, c.r(3), 0);
    if (!c.r(3)) return 0;
    // The formation record: the screen's own, or the match side's (leaf 0x103c5940).
    const auto formation = [&] {
        compare(c, std::int8_t(live), 0);
        if (std::int8_t(live) == 0) return screen + field::formation;
        c.set(4, c.u8(screen + live_side));
        return side_formation(c, c.u32(screen + field::match), c.r(4));
    };
    // The slot's role byte (leaf 0x101149f0).
    const auto record = formation();
    c.set(4, c.u8(slot_at));
    const std::uint32_t role = c.u8(record + role_bits + widened(c.r(4)));
    c.set({{3, role}, {29, role}});
    const auto bit = [&](std::int32_t mask) {
        const auto value = std::int32_t(std::int8_t(role)) & mask;
        compare(c, value, 0);
        return value != 0;
    };
    if (!bit(0x4)) return 0;
    if (bit(0x8)) {
        const bool own = std::int8_t(live) == 0;
        c.set({{3, formation()}, {4, c.u8(slot_at)}});
        c.call(0x10114b10, own ? 0x103cae90 : 0x103caea8);
        return c.r(3);
    }
    if (!bit(0x10)) return 0;
    c.set({{3, screen}, {4, index}, {5, live}});
    c.call(0x103cac10, 0x103caecc);
    const auto other = c.r(3);
    c.set(26, other);
    compare(c, std::int8_t(other), -1);
    if (std::int8_t(other) <= -1) return 0;
    c.set({{3, screen}, {4, other}, {5, not_live(live)}});
    c.call(0x103ca4e0, 0x103caef8);
    return c.r(3);
}

std::uint32_t paired_entry(Cpu& c, std::uint32_t screen, std::uint32_t index, std::uint32_t live) {
    Frame frame(c, 0x103caf20, 0x80, {.first_gpr = 26});
    c.set({{30, screen}, {28, index}, {29, live}, {27, pool(c)}});
    if (outside(c, index, 19)) {
        fail(c, frame, 0x103caf6c, 0x3d, 0x38, 0x1f29);
        return 0;
    }
    compare_logical(c, c.u32(screen + opponent), 0);
    if (!c.u32(screen + opponent)) return 0;
    c.set({{3, screen}, {4, index}, {5, 0}, {6, live}});
    c.call(0x103c9810, 0x103cafdc);
    compare(c, std::int8_t(c.r(3)), 0);
    if (std::int8_t(c.r(3)) == 0) return 0;
    c.set({{3, screen}, {4, index}, {5, live}});
    c.call(0x103ca4e0, 0x103caff8);
    const auto player = c.r(3);
    c.set(26, player);
    compare_logical(c, player, 0);
    if (!player) return 0;
    // The other side's entry whose staff (0x103cad40) is this entry's player.
    const auto other = not_live(live);
    for (std::uint32_t entry = 0;; ++entry) {
        c.set(31, entry);
        compare(c, std::int8_t(entry), 20);
        if (std::int8_t(entry) >= 20) break;
        c.set({{3, screen}, {4, entry}, {5, 0}, {6, other}});
        c.call(0x103c9810, 0x103cb02c);
        compare(c, std::int8_t(c.r(3)), 0);
        if (std::int8_t(c.r(3)) == 0) continue;
        c.set({{3, screen}, {4, entry}, {5, other}});
        c.link(0x103cb054);
        const auto staff = entry_staff(c, screen, entry, other);
        compare_logical(c, staff, player);
        if (staff != player) continue;
        c.set({{3, screen}, {4, entry}, {5, other}});
        c.call(0x103ca4e0, 0x103cb078);
        return c.r(3);
    }
    return 0;
}

std::uint32_t position_point(Cpu& c, std::uint32_t code, std::uint32_t left, std::uint32_t top, std::uint32_t right,
                             std::uint32_t bottom, std::uint32_t out_x, std::uint32_t out_y, std::uint32_t mirrored) {
    Frame frame(c, 0x103ce330, 0xb0, {.first_gpr = 22});
    c.set({{27, code}, {30, left}, {24, top}, {26, right}, {28, bottom}, {31, out_x}, {29, out_y}, {22, mirrored},
           {23, c.toc(-0x7ba8)}, {25, pool(c)}});
    const auto bits = std::uint32_t(std::int32_t(std::int16_t(code)));
    // Each test compares the code's bit with 0 into cr0.
    const auto has = [&](std::uint32_t bit) {
        compare(c, std::int32_t(bits & bit), 0);
        return (bits & bit) != 0;
    };
    const auto x0 = std::int32_t(left), y1 = std::int32_t(bottom);
    const auto width = std::int32_t(right - left), height = std::int32_t(bottom - top);
    // The goalkeeper: centred, 10 pixels above the bottom edge.
    if (has(0x1)) {
        c.put32(out_x, std::uint32_t(x0 + srawi(c, width + sign(width), 1)));
        c.put32(out_y, bottom - 10);
        return 1;
    }
    // The line: a fraction of the height up from the bottom edge.
    std::int32_t y;
    if (has(0x2)) {
        const auto q = srawi(c, mulhw(0x2aaaaaab, height), 1);
        y = y1 - (q + sign(q)) - 15;
    } else if (has(0x4)) y = y1 - sixths(height, 1) - 20;
    else if (has(0x8)) y = y1 - sixths(height, 2) - 10;
    else if (has(0x10)) y = y1 - sixths(height, 3);
    else if (has(0x20)) y = y1 - sixths(height, 4);
    else if (has(0x40)) y = y1 - sixths(height, 5);
    else {
        fail(c, frame, 0x103ce50c, 0x61, 0x3d, 0x217b);
        return 0;
    }
    c.put32(out_y, std::uint32_t(y));
    // The side: tenths of the width when mirrored, eighths when not (the
    // centre is always half); the mirrored flag's compare comes last.
    const auto tens = [&] {
        compare(c, std::int8_t(mirrored), 0);
        return std::int8_t(mirrored) != 0;
    };
    std::int32_t x;
    if (has(0x80)) x = tens() ? tenths(c, width, 1) : eighths(c, width, 1);
    else if (has(0x100)) x = tens() ? tenths(c, width, 3) : eighths(c, width, 3);
    else if (has(0x200)) x = tenths(c, width, 5);
    else if (has(0x400)) x = tens() ? tenths(c, width, 7) : eighths(c, width, 5);
    else if (has(0x800)) x = tens() ? tenths(c, width, 9) : eighths(c, width, 7);
    else {
        fail(c, frame, 0x103ce72c, 0x42, 0x38, 0x21b0);
        return 0;
    }
    c.put32(out_x, std::uint32_t(x0 + x));
    return 1;
}

std::uint32_t arrow_head(Cpu& c, std::uint32_t x0, std::uint32_t y0, std::uint32_t x1, std::uint32_t y1, double spread,
                         std::uint32_t length, std::uint32_t out_x1) {
    // f27-f31 are saved by 0x10773804 below the GPRs' 8-byte-aligned block.
    Frame frame(c, 0x103ce790, 0x100, {.first_gpr = 25, .gpr_offset = -0x4c, .first_fpr = 27});
    c.put32(frame.at(0x118), x0);
    c.put32(frame.at(0x11c), y0);
    const auto out_y1 = c.u32(frame.at(0x138)), out_x2 = c.u32(frame.at(0x13c)), out_y2 = c.u32(frame.at(0x140));
    c.set({{29, x1}, {30, y1}, {31, length}, {25, out_x1}, {26, out_y1}, {27, out_x2}, {28, out_y2}});
    c.set_f(27, spread);
    Fpu fpu(c);
    const auto constants = c.toc(constants_slot);
    const auto magic = c.f64(constants + 0x8);  // 2^52 + 2^31: int to double through a stack temporary
    const auto to_double = [&](std::int32_t offset, std::uint32_t value) {
        c.put32(frame.at(offset + 4), value ^ 0x80000000u);
        c.put32(frame.at(offset), 0x43300000);
        return fpu.sub(c.f64(frame.at(offset)), magic);
    };
    // A math import through its glue (0x1078xxxx stores r2 at sp+0x14); the TOC comes back from there.
    const auto math = [&](std::uint32_t glue, std::uint32_t back) {
        c.set(3, constants);
        c.call(glue, back);
        c.set(2, c.u32(frame.at(0x14)));
        return c.f(1);
    };
    const auto store = [&](std::int32_t offset, double value, std::uint32_t out) {
        c.put64(frame.at(offset), fpu.truncate(value));
        c.put32(out, c.u32(frame.at(offset + 4)));
    };
    const auto dy = to_double(0xa0, y0 - y1), dx = to_double(0x98, x1 - x0);
    c.set_f(1, dy);
    c.set_f(2, dx);
    const auto angle = math(0x10781038, 0x103ce824);  // atan2
    c.set_f(28, angle);
    // The first side of the head.
    auto f1 = fpu.sub(c.f64(constants + 0x50), angle);
    auto f0 = fpu.sub(f1, c.f64(constants + 0x58));
    auto f31 = fpu.sub(f0, spread);
    c.set_f(31, f31);
    c.set_f(0, f0);
    c.set_f(1, f31);
    auto cosine = math(0x10780558, 0x103ce850);
    f0 = to_double(0x90, length);
    auto f30 = fpu.mul(f0, cosine);
    c.set_f(30, f30);
    c.set_f(0, f0);
    c.set_f(2, magic);
    c.set_f(1, f31);
    auto sine = math(0x10781050, 0x103ce880);
    auto f29 = fpu.mul(to_double(0x88, length), sine);
    c.set_f(29, f29);
    store(0x78, fpu.sub(to_double(0x80, x1), f30), out_x1);
    store(0x68, fpu.add(to_double(0x70, y1), f29), out_y1);
    // The second side.
    f31 = fpu.sub(angle, spread);
    c.set_f(31, f31);
    c.set_f(1, f31);
    cosine = math(0x10780558, 0x103ce91c);
    f0 = to_double(0x60, length);
    f30 = fpu.mul(f0, cosine);
    c.set_f(30, f30);
    c.set_f(0, f0);
    c.set_f(2, magic);
    c.set_f(1, f31);
    sine = math(0x10781050, 0x103ce94c);
    f29 = fpu.mul(to_double(0x58, length), sine);
    c.set_f(29, f29);
    store(0x48, fpu.sub(to_double(0x50, x1), f29), out_x2);
    store(0x38, fpu.add(to_double(0x40, y1), f30), out_y2);
    c.set_f(1, magic);
    c.set(11, frame.caller());  // `or r11,r1,r1` before the FPR restore (0x10773854)
    return 1;
}
}

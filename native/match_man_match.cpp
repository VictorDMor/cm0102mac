// MATCH_MAN.CPP match block (match_man_match.hpp): applying a lineup to a side
// (0x102104d0) and the inlined random draw the block's routines share.
#include "match_man_match.hpp"
#include "guest_values.hpp"
#include "match_man_adapter.hpp"
#include "match_man_match_detail.hpp"

namespace cm::aot::match_man {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Frame;
using guest::Kept;
using guest::nonzero;
using guest::nonzero_logical;
constexpr std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
constexpr std::int32_t s16(std::uint32_t value) { return std::int16_t(value); }

// v / 65535 as the compiler forms it (magic 0x80008001, shift 15), with the sign bit it adds.
struct Quotient { std::int32_t value; std::uint32_t sign; };
std::int32_t multiply_high(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * std::int64_t(b)) >> 32); }
Quotient by_65535(std::uint32_t v) {
    const auto sum = std::uint32_t(multiply_high(std::int32_t(0x80008001), std::int32_t(v))) + v;
    const auto shifted = std::uint32_t(std::int32_t(sum) >> 15), sign = shifted >> 31;
    return {std::int32_t(shifted + sign), sign};
}
// (v + 5) / 10 (magic 0x66666667, shift 2).
Quotient rounded_tenth(std::int32_t v) {
    const auto shifted = std::uint32_t(multiply_high(0x66666667, std::int32_t(std::uint32_t(v) + 5)) >> 2), sign = shifted >> 31;
    return {std::int32_t(shifted + sign), sign};
}
}

std::int32_t random_spread(Cpu& c, guest::Fpu& fpu, const RandomStream& stream, std::int32_t n, std::uint32_t pool_address,
                           const SpreadSite& site) {
    const auto set = [&](std::int32_t value) {
        c.set(site.result_register, std::uint32_t(value));
        return value;
    };
    // Whether n fits 16 bits (the two compares build their bounds in r3).
    c.set(3, 0x10000);
    bool narrow = false;
    if (compare(c, n, 0xffff) <= 0) {
        c.set(3, 0xffff0000);
        narrow = compare(c, n, -0xffff) >= 0;
    }
    if (narrow) {
        // The seed's remainder by n (divw; the original's remainder idiom), negated unless n > 0.
        const bool positive = compare(c, n) > 0;
        const auto x = guest::result(c, stream.draw(c));
        const auto quotient = std::uint32_t(divide_signed<4>(x, std::uint32_t(n)));
        const auto rest = x - quotient * std::uint32_t(n);
        return set(std::int32_t(positive ? rest : 0u - rest));
    }
    const auto bias = c.f64(pool_address + pool::conversion);
    const auto divisor = c.f64(pool_address + 0x98);  // 65535.0
    const auto entry = std::uint32_t(c.u16(stream.at(c)));
    const auto salt = c.u32(stream.seed_slot);
    const auto x = entry + salt;
    const double share = fpu.fdiv(converted(c, fpu, site.n_at, n, bias), divisor);
    const auto whole = by_65535(x);
    const auto multiple = std::uint32_t(whole.value) * 0xffff;
    const auto tenfold = std::int32_t(std::uint32_t(n) * 10);
    const auto coarse = by_65535(std::uint32_t(tenfold));
    const double rest = converted(c, fpu, site.rest_at, std::int32_t(x - multiple), bias);
    const auto bits = fpu.fctiwz(0, fpu.fmul(share, rest));
    c.put64(site.part_at, bits);
    c.set_f(1, share);
    const auto part = std::int32_t(std::uint32_t(bits));
    set(part);
    c.set({{3, std::uint32_t(coarse.value)}, {4, multiple}, {5, 0xffff}, {6, 0x80008001}, {7, x}, {8, entry}});
    match_man::call(c, lineup_entries::random_bounded, site.bounded_back);
    const auto sampled = std::int32_t(c.r(3));
    const auto tenth = rounded_tenth(sampled);
    const auto limit = std::int32_t(std::uint32_t(n) - 1);
    if (site.limit_in_r4) c.set({{3, tenth.sign}, {4, std::uint32_t(limit)}});
    else c.set({{3, std::uint32_t(tenth.value)}, {4, tenth.sign}});
    const auto total = std::int32_t(std::uint32_t(std::int32_t(c.r(site.result_register))) + std::uint32_t(tenth.value));
    set(total);
    // Quirk: only the upper bound is clamped.
    if (compare(c, total, limit) > 0) set(limit);
    return std::int32_t(c.r(site.result_register));
}

// 0x102104d0. Frame 0x170 saving r24-r31. r25 the side, r31 the TOC word -0x7a54. Locals: the
// persons of the players on the pitch at 0xcc, the positions of the ones to move at 0x58,
// their categories (0x88), roles (0x70), ratings (0xf8, 8 bytes each) and forms (0x64), and
// the assignment 0x10210c50 returns at 0xa0.
void apply_lineup(Cpu& c, std::uint32_t side) {
    constexpr std::uint32_t entry = 0x102104d0;
    using namespace lineup_entries;
    c.set(0, c.lr());
    Frame frame(c, entry, 0x170, {.first_gpr = 24});
    const auto persons = frame.at(0xcc), positions = frame.at(0x58), categories = frame.at(0x88), roles = frame.at(0x70),
               ratings = frame.at(0xf8), forms = frame.at(0x64), assigned = frame.at(0xa0);
    guest::keep(c, {{25, side}, {31, c.toc(-0x7a54)}});
    const auto tactics = [&] { return c.u32(side + lineup_side::tactics); };

    // The persons of the players on the pitch (position >= 0).
    std::uint32_t listed = 0, last_person = c.r(4);
    auto record = c.u32(side + lineup_side::players);
    for (unsigned n = 0; n < 20; ++n, record += match_player::size) {
        if (c.i8(record + lineup_player::position) < 0) continue;
        last_person = c.u32(record + match_player::person);
        c.put32(persons + std::uint32_t(s8(listed)) * 4, last_person);
        ++listed;
    }
    c.set({{3, persons}, {4, last_person}, {5, listed}, {6, record}, {7, 20}});
    compare(c, 20, 20);
    if (compare(c, s8(listed), 11) == 0) {
        // A full eleven: have 0x1020d7d0 pick for them, and announce the result when it answers.
        c.set({{3, side}, {4, persons}, {6, 0x8c1}, {7, 2}});
        call(c, pick, 0x10210554);
        if (nonzero(c, s8(c.r(3)))) {
            const auto match = c.u32(side + lineup_side::match);
            c.set({{5, match}, {4, 0x2007}, {6, 0xffffffff}, {8, 0xffffffff}, {9, 0xffffffff}, {10, 0}});
            c.set(3, c.u32(match + lineup_match::events));
            c.set(7, std::uint32_t(c.i8(side + lineup_side::index)));
            c.call(events, 0x10210584);
        }
    }

    // The players that can move: their positions, categories, roles, ratings and forms.
    Kept<28> moving(c, 0);
    Kept<27> player(c, c.u32(side + lineup_side::players));
    Kept<26> index(c, 0);
    Kept<30> half_offset(c, 0);
    Kept<29> wide_offset(c, 0);
    Kept<24> slot(c, c.r(24));
    for (; compare(c, s8(index), 20) < 0; index = index + 1, player = player + match_player::size) {
        const auto position = c.u8(player + lineup_player::position);
        if (compare(c, s8(position)) < 0) continue;
        c.set(3, player);
        c.call(player_category, 0x102105b0);
        if (compare(c, s16(c.r(3)), 1) == 0) continue;
        // The original reloads the position byte after each call (0x102105bc, 0x102105ec, 0x1021060c).
        const auto position_now = [&] { return c.u8(player + lineup_player::position); };
        slot = std::uint32_t(s8(moving));
        c.set(4, positions);
        c.set(3, player);
        c.put8(positions + slot, position_now());
        c.call(player_category, 0x102105d4);
        c.set(4, categories);
        c.put16(categories + half_offset, c.r(3));
        c.set(3, player);
        c.call(player_role, 0x102105e4);
        c.set(4, roles);
        c.put16(roles + half_offset, c.r(3));
        c.set({{3, tactics()}, {4, std::uint32_t(s8(position_now()))}});
        c.call(person_rating, 0x102105fc);
        c.set(5, ratings + wide_offset);
        c.put32(ratings + wide_offset + 4, c.r(4));
        c.put32(ratings + wide_offset, c.r(3));
        c.set({{3, tactics()}, {4, std::uint32_t(s8(position_now()))}});
        c.call(person_form, 0x1021061c);
        c.set(4, forms);
        c.put8(forms + slot, c.r(3));
        c.set(3, persons);
        half_offset = half_offset + 2;
        wide_offset = wide_offset + 8;
        c.set(4, c.u32(player + match_player::person));
        moving = moving + 1;
        c.put32(persons + (slot << 2), c.r(4));
    }

    // The flags for the assignment: 0x8800, plus 0x400000 when the club's manager is a person
    // the registry holds (the checks of 0x1014c2e0, 0x10147e70 and 0x10149df0).
    std::uint32_t flags = 0x10000 - 0x7800;
    slot = flags;
    c.set({{3, c.u32(side + lineup_side::club)}, {4, 1}, {5, 1}, {6, 0}, {7, 0}});
    c.call(squad_check, 0x10210674);
    if (nonzero(c, s8(c.r(3))) && !nonzero(c, c.i8(side + lineup_side::kind)) &&
        nonzero_logical(c, c.u32(c.u32(side + lineup_side::club) + 0x90))) {
        c.set(4, c.u32(c.u32(side + lineup_side::club) + 0x90));  // the manager tested stays in r4 for the call
        c.set(3, c.u32(c.r(31)));
        c.call(registry_check, 0x102106a0);
        if (nonzero(c, s8(c.r(3)))) {
            c.set({{5, c.u32(c.u32(side + lineup_side::club) + 0x90)}, {4, 3}, {3, c.u32(c.r(31))}});
            c.call(registry_member, 0x102106bc);
            if (nonzero(c, s8(c.r(3)))) flags |= 0x400000;
        }
    }
    slot = flags;
    c.put32(frame.at(0x38), side);
    c.put32(frame.at(0x3c), c.u32(side + lineup_side::manager));
    c.put32(frame.at(0x40), 0);
    c.put32(frame.at(0x44), ratings);
    c.put32(frame.at(0x48), forms);
    c.put32(frame.at(0x4c), c.u32(tactics() + 0x588));
    c.put32(frame.at(0x50), roles);
    c.set({{3, persons}, {4, moving}, {5, categories}, {6, moving}, {7, 0}, {8, assigned}, {9, flags},
           {10, c.u32(side + lineup_side::club)}});
    call(c, assign_positions, 0x10210724);

    // Each moved player takes the position the assignment gave its person.
    Kept<27> moved(c, std::uint32_t(s8(moving)));
    Kept<26> cursor(c, persons);
    slot = 0;
    for (; compare(c, s8(slot), std::int32_t(std::uint32_t(moved))) < 0; slot = slot + 1, cursor = cursor + 4) {
        c.set({{4, c.u32(cursor)}, {3, side}});
        call(c, find_player, 0x10210740);
        const auto found = c.r(3);
        std::uint32_t pointer = assigned, j = 0;
        c.set({{4, pointer}, {7, 0}, {6, 0}});
        for (; compare(c, s8(j), std::int32_t(std::uint32_t(moved))) < 0; ++j, pointer += 4) {
            const auto candidate = c.u32(pointer);
            const auto who = c.u32(found + match_player::person);
            c.set({{5, candidate}, {4, pointer}, {6, j * 4}, {7, j}});
            compare_logical(c, candidate, who);
            if (candidate == who) {
                c.put8(found + lineup_player::position, c.u8(positions + std::uint32_t(s8(j))));
                c.set({{4, 0}, {3, found}});
                c.call(0x1022ba10, 0x1021077c);
                break;
            }
            c.set({{6, j * 4 + 4}, {4, pointer + 4}, {7, j + 1}});
        }
    }
    compare(c, s8(slot), std::int32_t(std::uint32_t(moved)));
}
}

// DATABASE.CPP's search block, part 2: the squad scans. Each takes a club (a
// record with 50 person pointers at +0x98 and a 16-bit rating at +0x1ec),
// derives a rating threshold from the club's rating, and counts (or finds)
// persons of the squad whose player record fits a position test, who pass the
// status query 0x10187680 and whose id is below the global at TOC -0x7be4 and
// whose player rating (16-bit at +4) exceeds the threshold (database_search.hpp).
#include "database_search.hpp"
#include "database_search_detail.hpp"
#include <array>
#include <span>
#include "guest_fpu.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
using namespace search;
using guest::compare;
using guest::compare_logical;

constexpr std::uint32_t status_query = 0x10187680;  // generated: (global, person) -> status byte (2 excludes)
constexpr std::uint32_t squad_size = 50;

// The threshold from the club's rating: (rating * magic >> 32) >> shift, plus its sign bit (rounding
// toward zero), plus `add`, as a signed halfword, at least 0x28 and at most `cap`. Left in register
// `target` (0: none; cr0 is the last compare, XER[CA] the shift's).
std::uint32_t threshold(Cpu& c, std::uint32_t club, std::uint32_t magic, unsigned shift, std::uint32_t add,
                        std::uint32_t cap, unsigned target) {
    const auto rating = extend_half(c.u16(club + 0x1ec));
    const auto high = shift_right_algebraic(c, mulhw(magic, rating), shift);
    const auto value = extend_half(high + (high >> 31) + add);
    if (target) c.set(target, value);
    if (compare(c, std::int32_t(value), 0x28) < 0) {
        if (target) c.set(target, 0x28);
        return 0x28;
    }
    // `extsh r0,rT; cmpwi r0,cap; ble`
    if (compare(c, std::int32_t(value), std::int32_t(cap)) <= 0) return value;
    if (target) c.set(target, cap);
    return cap;
}

// What the tail computes from the signed-byte count: count > limit (the xor idiom), count <= 1 (the
// adde idiom), count > 0.
enum class Tail { count_above_limit, count_at_most_one, count_positive };

// An extra test on the player's record after the rating: a byte (sign-extended) above the second
// threshold, or (not against_second_threshold) above an immediate.
struct Extra {
    std::uint32_t offset;
    bool against_second_threshold;
    std::int32_t limit;
};

// What the squad scans differ in.
struct Scan {
    std::uint32_t entry, frame;
    unsigned first_gpr, club, index, person, count, low, high;  // registers (high 0: no second threshold)
    std::uint32_t magic, shift, add, cap;
    std::uint32_t magic2, shift2, cap2;
    std::span<const std::uint32_t> role_bytes;  // any of these bytes of the player record equals 20 (empty: no test)
    std::span<const Extra> extras;
    std::uint32_t status_return;  // the address after the bl to the status query
    Tail tail;
    std::uint32_t count_limit;    // the xor tail: count > this
    // The team-quality prefix (divisor_offset 0: none): the club's team record, looked up in the table at
    // TOC -0x7b5c, goes through the attributes owner (0x10459b50); a quotient of two of the answer's bytes
    // (the one at `divisor_offset` over the one at +4), as a single, is compared with the pool constant at
    // `cutoff_offset`; on the wrong side (above it with `stop_when_above`, below otherwise) the answer is 0.
    std::uint32_t divisor_offset, cutoff_offset, actor_return;
    bool stop_when_above;
};

// The tail: a comparison of the signed-byte count, built from the compiler's idioms.
std::int32_t finish(Cpu& c, const Scan& s, std::uint32_t count) {
    const auto signed_count = extend_byte(count);
    std::uint32_t result = 0;
    switch (s.tail) {
    case Tail::count_above_limit: {
        c.set(0, s.count_limit);
        c.set(4, signed_count);
        const auto mixed = signed_count ^ s.count_limit;
        c.set(0, mixed);
        const auto half = shift_right_algebraic(c, mixed, 1);
        c.set(3, half);
        const auto both = mixed & signed_count;
        c.set(0, both);
        const auto difference = half - both;
        c.set(0, difference >> 31);
        result = (difference >> 31) & 1;
        c.set(0, result);
        break;
    }
    case Tail::count_at_most_one: {
        // li r0,1; r4 = srawi(r0, 31) = 0; r3 = sign of the count; subfc r0,r5,r0; adde r3,r4,r3.
        const std::uint32_t one = 1;
        c.set(5, signed_count);
        c.set(0, one);
        const auto zero = shift_right_algebraic(c, one, 31);
        c.set(4, zero);
        const auto sign = signed_count >> 31;
        c.set(3, sign);
        const auto difference = one - signed_count;
        c.set(0, difference);
        const bool carry = one >= signed_count;  // subfc: carry out of one + ~count + 1
        result = zero + sign + carry;
        c.s.wr<1>(guest::reg::xer_ca, (std::uint64_t(zero) + sign + carry) >> 32);
        break;
    }
    case Tail::count_positive: {
        c.set(3, signed_count);
        const auto negated = std::uint32_t(0) - signed_count;
        const auto bit = (negated & ~signed_count) >> 31;
        c.set(0, bit);
        result = bit;
        break;
    }
    }
    result = extend_byte(result);
    c.set(3, result);
    return std::int32_t(result);
}

// The team's table entry, through two pointers, each tested; false when the quality
// quotient stops the scan. Every missing link just skips the test (answers true).
bool team_quality_allows(Cpu& c, std::uint32_t local, const Scan& s, std::uint32_t club, std::uint32_t table_word) {
    const auto team_of = [&] { return c.u32(club + 0x30); };
    auto team = team_of();
    c.set(0, team);
    if (compare_logical(c, team) == 0) return true;
    // Entry of the team in the table: the team's first word indexes it.
    c.set(3, team_of());
    c.set(4, c.u32(table_word));
    c.set(0, c.u32(team));
    c.set(0, c.r(0) << 2);
    auto entry = c.u32(c.r(4) + c.r(0));
    c.set(0, entry);
    if (compare_logical(c, entry) == 0) return true;
    c.set(3, team_of());
    c.set(4, c.u32(table_word));
    c.set(0, c.u32(c.r(3)));
    c.set(0, c.r(0) << 2);
    c.set(3, c.u32(c.r(4) + c.r(0)));
    const auto inner = c.u32(c.r(3) + 8);
    c.set(0, inner);
    if (compare_logical(c, inner) == 0) return true;
    c.set(3, team_of());
    c.set(4, club);
    c.set(5, c.u32(table_word));
    c.set(0, c.u32(c.r(3)));
    c.set(0, c.r(0) << 2);
    c.set(3, c.u32(c.r(5) + c.r(0)));
    c.set(3, c.u32(c.r(3) + 8));
    c.call_in_place(0x10459b50, s.actor_return);
    const auto answer = c.r(3);
    if (compare_logical(c, answer) == 0) return true;
    auto byte = std::uint32_t(c.u8(answer + 4));
    c.set(0, byte);
    if (compare_logical(c, byte) == 0) return true;
    byte = std::uint32_t(c.u8(answer + 4));
    c.set(0, byte);
    compare_logical(c, byte);  // beq to a path no byte can reach: the same byte was just found non-zero
    const auto numerator = std::uint32_t(c.u8(answer + s.divisor_offset));
    c.set(5, numerator);
    c.set(0, 0x43300000);
    c.set(4, byte);
    const auto pool = c.toc(-0x5614);
    c.set(3, pool);
    const auto quotient = std::uint32_t(std::int32_t(numerator) / std::int32_t(byte));
    c.set(4, quotient);
    c.put32(local, 0x43300000);
    const double conversion = c.f64(pool + 0x18);
    c.set_f(1, conversion);
    const auto flipped = quotient ^ 0x80000000u;
    c.set(0, flipped);
    c.put32(local + 4, flipped);
    const double loaded = c.f64(local);
    c.set_f(0, loaded);
    guest::Fpu fpu(c);
    const double quality = fpu.fsubs(loaded, conversion);
    c.set_f(1, quality);
    const auto pool_again = c.toc(-0x5614);
    c.set(3, pool_again);
    const double cutoff = c.f64(pool_again + s.cutoff_offset);
    c.set_f(0, cutoff);
    fpu.fcmp(0, quality, cutoff);
    const bool less = c.cr(0) & 8, greater = c.cr(0) & 4;
    // dcd90: `bge` continues; dca00: `ble` continues.
    return s.stop_when_above ? !greater : !less;
}

std::int32_t run(Cpu& c, std::uint32_t club, const Scan& s) {
    guest::Frame frame(c, s.entry, s.frame, {.first_gpr = s.first_gpr});
    const auto status_word = c.toc(-0x7ba0), limit_word = c.toc(-0x7be4);
    guest::keep(c, {{30, status_word}, {31, limit_word}});
    c.set(s.club, club);
    c.set(s.count, 0);
    const auto table_word = s.divisor_offset ? c.toc(-0x7b5c) : 0;
    if (s.divisor_offset) c.set(5, table_word);
    if (compare(c, std::int32_t(club)) == 0) {
        c.set(3, 0);
        return 0;
    }
    if (s.divisor_offset && !team_quality_allows(c, frame.at(0x40), s, club, table_word)) {
        c.set(3, 0);
        return 0;
    }
    const auto low = threshold(c, club, s.magic, s.shift, s.add, s.cap, s.low);
    std::uint32_t high = 0;
    if (s.high) {
        // The second threshold: the same scaling, plus 5, capped (no lower clamp).
        const auto rating = extend_half(c.u16(club + 0x1ec));
        const auto scaled = shift_right_algebraic(c, mulhw(s.magic2, rating), s.shift2);
        const auto value = extend_half(extend_half(scaled + (scaled >> 31)) + 5);
        c.set(s.high, value);
        high = value;
        if (compare(c, std::int32_t(value), std::int32_t(s.cap2)) > 0) {
            c.set(s.high, s.cap2);
            high = s.cap2;
        }
    }
    std::uint32_t found = 0;
    c.set(s.index, 0);
    for (std::uint32_t index = 0;; ++index) {
        if (compare(c, std::int32_t(index), std::int32_t(squad_size)) >= 0) break;
        const auto person = c.u32(club + index * 4 + 0x98);
        c.set(s.person, person);
        if (compare_logical(c, person) != 0) {
            auto player = c.u32(person + 0x14);
            c.set(0, player);
            bool pass = compare_logical(c, player) != 0;
            // The role test: the first matching byte, in order.
            if (pass && !s.role_bytes.empty()) {
                pass = false;
                for (const auto offset : s.role_bytes) {
                    player = c.u32(person + 0x14);
                    c.set(3, player);
                    const auto byte = std::uint32_t(c.u8(player + offset));
                    c.set(0, byte);
                    if (compare(c, std::int32_t(byte), 20) == 0) {
                        pass = true;
                        break;
                    }
                }
            }
            if (pass) {
                c.set(3, c.u32(status_word));
                c.set(4, person);
                c.call(status_query, s.status_return);
                const auto status = extend_byte(c.r(3));
                c.set(0, status);
                pass = compare(c, std::int32_t(status), 2) != 0;
            }
            if (pass) {
                // cmplwi person,0 (never null here), then the id against the global.
                compare_logical(c, person);
                const auto id = c.u32(person);
                c.set(3, id);
                const auto bound = c.u32(limit_word);
                c.set(0, bound);
                pass = compare(c, std::int32_t(id), std::int32_t(bound)) < 0;
            }
            if (pass) {
                player = c.u32(person + 0x14);
                c.set(3, player);
                c.set(0, extend_half(low));
                const auto rating = extend_half(c.u16(player + 4));
                c.set(3, rating);
                pass = compare(c, std::int32_t(rating), std::int32_t(extend_half(low))) > 0;
            }
            for (const auto& extra : s.extras) {
                if (!pass) break;
                player = c.u32(person + 0x14);
                c.set(3, player);
                if (extra.against_second_threshold) {
                    c.set(0, extend_half(high));
                    const auto value = extend_byte(c.u8(player + extra.offset));
                    c.set(3, value);
                    pass = compare(c, std::int32_t(value), std::int32_t(extend_half(high))) > 0;
                } else {
                    const auto value = extend_byte(c.u8(player + extra.offset));
                    c.set(0, value);
                    pass = compare(c, std::int32_t(value), extra.limit) > 0;
                }
            }
            if (pass) {
                ++found;
                c.set(s.count, found);
            }
        }
        c.set(s.index, index + 1);
    }
    return finish(c, s, found);
}

// Role bytes (player record offsets, 20 = natural) and extra tests of each scan.
constexpr std::array<std::uint32_t, 3> midfield_roles{0x14, 0x12, 0x13};
constexpr std::array<std::uint32_t, 1> attacker_role{0x15}, defender_role{0x11};
constexpr std::array<Extra, 1> field_44_over_14{{{0x44, false, 0xe}}}, field_2e_over_second{{{0x2e, true, 0}}},
    field_36_over_second{{{0x36, true, 0}}};
constexpr std::array<Extra, 2> fields_44_3e{{{0x44, true, 0}, {0x3e, true, 0}}}, fields_43_37{{{0x43, true, 0}, {0x37, true, 0}}};

// Scans with the first threshold only (rating * 0x7e07e07f >> 37).
constexpr Scan midfielders_scan{0x100dcc30, 0x60, 25, 25, 26, 29, 28, 27, 0, 0x7e07e07f, 5, 0x28, 0x91, 0, 0, 0,
                                midfield_roles, {}, 0x100dcd04, Tail::count_above_limit, 2, 0, 0, 0, false};
constexpr Scan field_44_scan{0x100dcfc0, 0x60, 25, 26, 27, 25, 29, 28, 0, 0x7e07e07f, 5, 0x28, 0x8c, 0, 0, 0,
                             {}, field_44_over_14, 0x100dd064, Tail::count_above_limit, 4, 0, 0, 0, false};
// With the team-quality prefix.
constexpr Scan defenders_with_team_scan{0x100dca00, 0x70, 25, 26, 27, 25, 29, 28, 0, 0x7e07e07f, 5, 0x28, 0x91, 0, 0, 0,
                                        defender_role, {}, 0x100dcba8, Tail::count_above_limit, 2, 0x6d, 0x130, 0x100dca98, true};
constexpr Scan attackers_with_team_scan{0x100dcd90, 0x70, 25, 26, 27, 25, 29, 28, 0, 0x7e07e07f, 5, 0x28, 0x96, 0, 0, 0,
                                        attacker_role, {}, 0x100dcf38, Tail::count_above_limit, 1, 0x6c, 0x90, 0x100dce28, false};
// With a second threshold (rating * 0x10624dd3 >> 37, plus 5) the player's field must exceed.
constexpr Scan second_threshold_scan(std::uint32_t entry, std::uint32_t cap2, std::span<const std::uint32_t> roles,
                                     std::span<const Extra> extras, Tail tail, std::uint32_t status_return) {
    return {entry, 0x60, 24, 24, 25, 29, 28, 27, 26, 0x1b4e81b5, 3, 0x19, 0x87, 0x10624dd3, 5, cap2,
            roles, extras, status_return, tail, 1, 0, 0, 0, false};
}
constexpr Scan attacker_2e_scan = second_threshold_scan(0x100dd200, 0xd, attacker_role, field_2e_over_second, Tail::count_at_most_one, 0x100dd2e8);
constexpr Scan defender_2e_scan = second_threshold_scan(0x100dd380, 0xd, defender_role, field_2e_over_second, Tail::count_at_most_one, 0x100dd468);
constexpr Scan defender_36_scan = second_threshold_scan(0x100dd500, 0xa, defender_role, field_36_over_second, Tail::count_at_most_one, 0x100dd5e8);
constexpr Scan attacker_36_scan = second_threshold_scan(0x100dd680, 0x11, attacker_role, field_36_over_second, Tail::count_positive, 0x100dd768);
constexpr Scan midfielder_44_3e_scan = second_threshold_scan(0x100dd7f0, 0xe, midfield_roles, fields_44_3e, Tail::count_above_limit, 0x100dd8f8);
constexpr Scan midfielder_43_37_scan = second_threshold_scan(0x100dd9b0, 0xe, midfield_roles, fields_43_37, Tail::count_above_limit, 0x100ddab8);
}

// Answers (inferred names; each says what the instructions compute from the signed-byte count; 0 for no club):
std::int32_t squad_has_more_than_two_midfielders(Cpu& c, std::uint32_t club) { return run(c, club, midfielders_scan); }
std::int32_t squad_has_more_than_four_with_field_44(Cpu& c, std::uint32_t club) { return run(c, club, field_44_scan); }
std::int32_t squad_has_more_than_two_defenders_with_team_check(Cpu& c, std::uint32_t club) { return run(c, club, defenders_with_team_scan); }
std::int32_t squad_has_more_than_one_attacker_with_team_check(Cpu& c, std::uint32_t club) { return run(c, club, attackers_with_team_scan); }
std::int32_t squad_has_at_most_one_attacker_by_field_2e(Cpu& c, std::uint32_t club) { return run(c, club, attacker_2e_scan); }
std::int32_t squad_has_at_most_one_defender_by_field_2e(Cpu& c, std::uint32_t club) { return run(c, club, defender_2e_scan); }
std::int32_t squad_has_at_most_one_defender_by_field_36(Cpu& c, std::uint32_t club) { return run(c, club, defender_36_scan); }
std::int32_t squad_has_attacker_by_field_36(Cpu& c, std::uint32_t club) { return run(c, club, attacker_36_scan); }
std::int32_t squad_has_more_than_one_midfielder_by_fields_44_3e(Cpu& c, std::uint32_t club) { return run(c, club, midfielder_44_3e_scan); }
std::int32_t squad_has_more_than_one_midfielder_by_fields_43_37(Cpu& c, std::uint32_t club) { return run(c, club, midfielder_43_37_scan); }

// 0x100dd100: whether the club has a player with byte 0xf equal to 20 whose status is
// not 2 and whose id is below the global; the threshold is computed (cap 0x96) and
// not used, but its compares leave cr0 and its shift XER[CA].
std::int32_t squad_has_goalkeeper(Cpu& c, std::uint32_t club) {
    guest::Frame frame(c, 0x100dd100, 0x50, {.first_gpr = 27});
    const auto status_word = c.toc(-0x7ba0), limit_word = c.toc(-0x7be4);
    guest::keep(c, {{30, status_word}, {31, limit_word}});
    c.set(29, club);
    if (compare(c, std::int32_t(club)) == 0) {
        c.set(3, 0);
        return 0;
    }
    threshold(c, club, 0x7e07e07f, 5, 0x28, 0x96, 0);
    c.set(27, 0);
    for (std::uint32_t index = 0; compare(c, std::int32_t(index), std::int32_t(squad_size)) < 0; c.set(27, ++index)) {
        const auto person = c.u32(club + index * 4 + 0x98);
        c.set(28, person);
        if (compare_logical(c, person) == 0) continue;
        const auto linked = c.u32(person + 0x14);
        c.set(0, linked);
        if (compare_logical(c, linked) == 0) continue;
        c.set(3, linked);
        const auto role = std::uint32_t(c.u8(linked + 0xf));
        c.set(0, role);
        if (compare(c, std::int32_t(role), 20) != 0) continue;
        c.set(3, c.u32(status_word));
        c.set(4, person);
        c.call(status_query, 0x100dd1a4);
        const auto status = extend_byte(c.r(3));
        c.set(0, status);
        if (compare(c, std::int32_t(status), 2) == 0) continue;
        compare_logical(c, person);
        const auto id = c.u32(person);
        c.set(3, id);
        const auto bound = c.u32(limit_word);
        c.set(0, bound);
        if (compare(c, std::int32_t(id), std::int32_t(bound)) >= 0) continue;
        c.set(3, 1);
        return 1;
    }
    c.set(3, 0);
    return 0;
}
}

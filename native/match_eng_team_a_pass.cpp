// MATCH_ENG.CPP, team_a: scoring every player of the side in possession as the receiver of a pass.
#include "match_eng_space.hpp"
#include "match_eng_team_a.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
constexpr std::uint32_t pending_at = 0xe000;     // the player about to act
constexpr std::uint32_t last_touch_at = 0x7924;  // the side that touched the ball last
constexpr std::uint32_t shot_state_at = 0x791f;
constexpr std::uint32_t team_tactics = 0x7c6c, team_tactics_size = 0x18e0;
constexpr std::uint32_t player_side = 0x27;
constexpr std::uint32_t random_below = 0x101d3c90;
constexpr std::uint32_t player_flags = 0x102864b0;  // 64 trait bits of a player: r3 high word, r4 low word

// Frame slots of 0x101e8950 (relative to the new stack pointer).
namespace slot {
constexpr std::uint32_t index = 0x38, near_count = 0x39, special_a = 0x3a, end_own = 0x3b, end_other = 0x3c, origin_own = 0x3d,
                        origin_other = 0x3e, special_b = 0x3f, dest_column = 0x40, dest_row = 0x41, moved = 0x42, chosen = 0x43,
                        nearby_ok = 0x44, scratch = 0x45, half = 0x46, position_value = 0x48, previous = 0x4c, stats = 0x50,
                        best = 0x54, third = 0x58, second = 0x5c, first = 0x60;
}

class Receiver {
public:
    Receiver(Cpu& cpu, Frame& f, std::uint32_t match_, std::uint32_t origin_column_)
        : c(cpu), frame(f), fpu(cpu), match(match_), origin_column(origin_column_),
          cand(cpu, cpu.r(28)), r16(cpu, cpu.r(16)), r17(cpu, cpu.r(17)), r18(cpu, cpu.r(18)), r19(cpu, cpu.r(19)),
          r20(cpu, cpu.r(20)), r23(cpu, cpu.r(23)), r24(cpu, cpu.r(24)), r25(cpu, cpu.r(25)), r26(cpu, cpu.r(26)),
          r27(cpu, cpu.r(27)) {}
    std::uint32_t run();

private:
    // Pool constants (r31: single floats, r29: doubles).
    double F(std::uint32_t at) const { return c.f32(c.r(31) + at); }
    double D(std::uint32_t at) const { return c.f64(c.r(29) + at); }
    std::uint32_t pending() const { return c.u32(match + pending_at); }
    std::int32_t side() const { return s8(c.u8(match + last_touch_at)); }
    std::uint32_t other_side() const { return std::uint32_t(side() == 0); }
    std::int32_t kind() const { return s8(c.u8(frame.caller() + 0x3b)); }
    std::uint32_t slot_u8(std::uint32_t at) const { return c.u8(frame.at(at)); }
    std::int32_t slot_s8(std::uint32_t at) const { return s8(c.u8(frame.at(at))); }
    std::int32_t slot_s16(std::uint32_t at) const { return c.i16(frame.at(at)); }
    std::uint32_t slot_u32(std::uint32_t at) const { return c.u32(frame.at(at)); }
    std::uint32_t out_score_pointer() const { return c.u32(frame.caller() + 0x34); }
    std::uint32_t team_of_side() const { return match + u(side() * std::int32_t(team_tactics_size)) + team_tactics; }

    void call_to(std::uint32_t entry, std::uint32_t back) { c.call(entry, back); }
    std::uint32_t rnd(std::uint32_t n, std::uint32_t back) {
        c.set(3, n);
        call_to(random_below, back);
        return c.r(3);
    }
    // A routine of this module not yet reconstructed (or another block's).
    void other_routine(std::uint32_t entry, std::uint32_t back) {
        call(c, entry, back);
    }
    // A 64-bit trait word test after the call returning at `back`.
    bool has_flag(std::uint32_t player, std::uint32_t back, std::uint32_t hi_mask, std::uint32_t lo_mask) {
        c.set(3, player);
        call_to(player_flags, back);
        trait_low = c.r(4);
        return compare(c, std::int32_t((c.r(3) & hi_mask) | (c.r(4) & lo_mask))) != 0;
    }
    // `bl entry; extsb; cmpwi 0` on a routine of another module.
    bool byte_flag(std::uint32_t entry, std::uint32_t player, std::uint32_t back) {
        c.set(3, player);
        call_to(entry, back);
        return compare(c, s8(c.r(3))) != 0;
    }
    std::int16_t position_value_of(std::uint32_t player, std::uint32_t back) {
        enter(c, back, {match, player});
        const std::int16_t value = std::int16_t(position_value_a(c, match, player));
        return value;
    }
    double widen(std::uint32_t at, std::int32_t value, bool single) {
        c.put32(frame.at(at) + 4, std::uint32_t(value) ^ 0x80000000u);
        c.put32(frame.at(at), 0x43300000);
        const double magic = c.f64(c.r(29) + 0x60);
        return single ? fpu.fsubs(c.f64(frame.at(at)), magic) : fpu.fsub(c.f64(frame.at(at)), magic);
    }
    std::uint32_t truncate(std::uint32_t at, double value, unsigned reg = 0) {
        const auto bits = fpu.fctiwz(reg, value);
        c.put64(frame.at(at), bits);
        return std::uint32_t(bits);
    }
    // fcmpo then cror: "<=" and ">=" as the branch on EQ sees them (unordered is neither).
    bool less_equal(double a, double b) {
        fpu.fcmp(0, a, b);
        auto cr = c.cr(0);
        if (cr & 8) cr |= 2;
        c.set_cr(0, cr);
        return (cr & 2) != 0;
    }
    bool greater_equal(double a, double b) {
        fpu.fcmp(0, a, b);
        auto cr = c.cr(0);
        if (cr & 4) cr |= 2;
        c.set_cr(0, cr);
        return (cr & 2) != 0;
    }
    bool less(double a, double b) { fpu.fcmp(0, a, b); return (c.cr(0) & 8) != 0; }
    bool greater(double a, double b) { fpu.fcmp(0, a, b); return (c.cr(0) & 4) != 0; }
    template <unsigned R> static void bump(Kept<R>& k, std::int32_t amount) { k = k + u(amount); }
    // The score from a float through the pool's scale: trunc(D30 * (a * b)). f1 is left as the
    // original leaves it: the first float, or (the last conversion) the product.
    std::uint32_t scaled(std::uint32_t site, std::uint32_t a_at, std::uint32_t b_at, bool product_in_f1 = false) {
        const double f2 = D(0x30);
        const double a = c.f32(cand + a_at);
        const double b = c.f32(cand + b_at);
        const double f0 = fpu.fmuls(a, b);
        const double scaled_value = fpu.fmul(f2, f0);
        c.set_f(1, product_in_f1 ? scaled_value : a);
        return truncate(site, scaled_value);
    }
    std::uint32_t origin_row_byte() const { return c.u8(frame.caller() + 0x1f); }
    std::uint32_t team_word(std::uint32_t back_flag, std::uint32_t back_team, std::uint32_t back_call, std::uint32_t mode);
    bool no_progress();
    bool start();
    bool candidate();
    bool reachable();
    void special_traits();
    void team_scores();
    void destination();
    void crowding();
    void exposure();
    bool kind_bonuses();
    void trait_scores();
    void distance_adjustments();
    void lane_scores();
    bool own_costs();
    void choose_kind();

    std::uint32_t trait_low = 0;
    Cpu& c;
    Frame& frame;
    Fpu fpu;
    std::uint32_t match, origin_column;
    Kept<28> cand;
    Kept<16> r16;
    Kept<17> r17;  // the destination row of the candidate's move
    Kept<18> r18;
    Kept<19> r19;
    Kept<20> r20;
    Kept<23> r23;
    Kept<24> r24;
    Kept<25> r25;
    Kept<26> r26;  // the destination column
    Kept<27> r27;  // the candidate's running score
};

bool Receiver::start() {
    c.set(3, 0);
    c.put32(frame.at(slot::best), 0);
    c.set(3, 0);
    c.put8(frame.at(slot::special_b), 0);
    c.set(3, 0);
    c.put8(match + 0x7928, 0);
    c.set_f(29, F(0xb0));
    // The noise scale (f29) depends on the pending player's competition.
    const auto id = [&] { return c.u32(c.u32(c.u32(pending() + 0x6c) + 0xc) + 0x68); };
    const auto is = [&](std::int32_t toc) { return compare(c, std::int32_t(id()), std::int32_t(c.u32(c.toc(toc)))) == 0; };
    if (compare_logical(c, c.u32(c.u32(pending() + 0x6c) + 0xc)) != 0) {
        bool quiet = is(-0x7a10) || is(-0x7a1c);
        if (!quiet) {
            c.set(7, c.u32(pending() + 0x6c));  // lwz r7,0x6c(r6) of the third comparison, never overwritten
            quiet = is(-0x757c) || is(-0x7578) || is(-0x7574) || is(-0x7504);
        }
        if (quiet) c.set_f(29, F(0x10));
        else if (is(-0x7514) || is(-0x750c) || is(-0x7af4)) c.set_f(29, F(0xa4));
    }
    // The score to beat: noise around a figure of the pending player.
    const auto first = rnd(500, 0x101e8b20);
    c.set(5, pending());  // lwz r5,-0x2000(r4) of the pending player's word
    c.set(6, u(std::int32_t(c.i16(pending() + 0x2a))));  // lha r6,0x2a(r5)
    c.set(15, first + u(std::int32_t(c.i16(pending() + 0x2a)) * 3) - 0xbb8);
    const auto second = rnd(500, 0x101e8b40);
    c.set(3, c.r(15) - second);
    c.set(4, out_score_pointer());
    c.put32(out_score_pointer(), c.r(3));

    if (!byte_flag(0x102721a0, pending(), 0x101e8b58)) {
        c.set({{3, match}, {4, pending()}});
        other_routine(0x101ee770, 0x101e8b74);
        c.set(14, c.r(3));
        c.put8(frame.at(slot::special_b), c.r(14));
    }
    if (compare(c, slot_s8(slot::special_b), 1) == 0) {
        c.set(14, match + 0x10000);  // addis r14,r30,1
        c.set(3, pending());
        call_to(0x10237420, 0x101e8b98);
        c.set(4, u(s8(c.r(3))));
        if (compare(c, s8(c.r(4))) != 0) return false;
    }
    return true;
}

std::uint32_t Receiver::team_word(std::uint32_t back_flag, std::uint32_t back_team, std::uint32_t back_call, std::uint32_t mode) {
    const auto player_side_byte = [&] { return c.u8(cand + player_side); };
    enter(c, back_flag, {match, player_side_byte()});
    c.set(15, fixture_flag_40(c, match, player_side_byte()));
    enter(c, back_team, {match, player_side_byte()});
    c.set(4, fixture_team(c, match, player_side_byte()));
    c.set({{3, c.u32(c.r(22))}, {5, mode}, {6, c.r(15)}});
    call_to(0x103b1370, back_call);
    return c.r(3);
}

// Skips for players who cannot take the pass, then the distance the opposition is from the player.
bool Receiver::reachable() {
    if (compare(c, s8(c.u8(cand + 0x19))) < 0) return false;
    if (compare_logical(c, cand, pending()) == 0) return false;
    c.set(5, match + 0x10000);
    const auto value = position_value_of(pending(), 0x101e8c84);
    c.set(4, u(std::int32_t(value)));
    if (compare(c, value, 1) == 0) {
        if (compare(c, side()) == 0) {
            const auto col = column(c, cand);
            c.set(5, u(s8(origin_column)));
            if (compare(c, s8(origin_column), s8(col)) > 0) return false;
        }
        if (compare(c, side(), 1) == 0) {
            const auto col = column(c, cand);
            c.set(15, u(s8(origin_column)));
            if (compare(c, s8(origin_column), s8(col)) < 0) return false;
        }
    }
    const auto col = column(c, cand);
    c.set(14, col);
    const auto rw = row(c, cand);
    c.set(5, rw);
    enter(c, 0x101e8d20, {match, other_side(), rw, col, 1});
    c.set_f(31, nearest_distance(c, match, other_side(), rw, col, 1));
    c.set(3, 0);
    c.put8(frame.at(slot::scratch), 0);
    if (compare(c, kind(), 0x36) != 0 && compare(c, kind(), 0x37) != 0) {
        c.set(15, column(c, cand));
        c.set(4, row(c, cand));
        c.set(5, c.r(15));
        c.set({{3, pending()}, {6, 1}, {9, frame.at(slot::scratch)}});
        c.set_f(1, c.f(28));
        c.set_f(2, c.f(31));
        call_to(0x10244c60, 0x101e8d88);
        c.set(4, u(s8(c.r(3))));
        if (compare(c, s8(c.r(4))) == 0) return false;
    }
    return true;
}

void Receiver::special_traits() {
    const auto value = position_value_of(cand, 0x101e8da4);
    c.set(5, c.r(3));
    c.put16(frame.at(slot::position_value), c.r(3));
    (void)value;
    {
        const double f2 = F(0xd0);
        const double f0 = fpu.fmuls(c.f(31), c.f(31));
        const double f1 = F(0xc8);
        c.set_f(1, f1);
        r19 = truncate(0x7c, fpu.multiply_add(0x101e8db8, f2, f0, f1));
    }
    if (byte_flag(0x10272100, cand, 0x101e8dd0)) {
        bump(r19, -0x5dc);
    } else if (byte_flag(0x102721a0, cand, 0x101e8df0)) {
        const auto col = column(c, cand);
        c.set(5, col);
        c.set(15, u(side()));
        enter(c, 0x101e8e24, {match, other_side(), col});
        const auto beyond = beyond_row(c, other_side(), col);
        if (compare(c, s8(beyond)) != 0) bump(r19, 0x2710);
        else bump(r19, 0xbb8);
    }
    if (compare(c, slot_s16(slot::position_value), 1) != 0) {
        const double fit = fitness(c, cand);
        c.set_f(1, fit);
        const double f2 = D(0x60);
        (void)f2;
        double f0 = widen(0x7c, c.i16(cand + 0x36), true);
        double f1 = fpu.fadds(f0, fit);
        f0 = F(0xd4);
        f1 = fpu.fsubs(f1, f0);
        f0 = F(0xc4);
        f0 = fpu.fmuls(f1, f0);
        c.set_f(1, f1);
        r27 = truncate(0x74, f0);
    } else {
        r27 = 0;
    }
    c.set(3, cand);
    call_to(0x102721a0, 0x101e8ea0);
    c.set(14, u(s8(c.r(3))));
    if (compare(c, s8(c.r(14))) == 0) {
        c.set({{3, match}, {4, cand}});
        other_routine(0x101ee770, 0x101e8eb8);
        c.put8(frame.at(slot::special_a), c.r(3));
        const auto special = slot_s8(slot::special_a);
        if (compare(c, special, 1) == 0) {
            bump(r27, 0x7d0);
            bump(r19, -0x7d0);
            if (has_flag(pending(), 0x101e8ee0, 1, 0x0)) { bump(r27, 0x7d0); bump(r19, -0x7d0); }
        } else if (compare(c, special, 2) == 0) {
            bump(r27, 0xfa0);
            bump(r19, -0xfa0);
            if (has_flag(pending(), 0x101e8f40, 1, 0x0)) { bump(r27, 0xfa0); bump(r19, -0xfa0); }
        } else if (has_flag(pending(), 0x101e8f88, 1, 0x0)) {
            bump(r27, 0x1f4);
            bump(r19, -0x1f4);
        }
    }
}

void Receiver::team_scores() {
    c.set(3, 0);
    c.put32(frame.at(slot::first), 0);
    r16 = 0;
    c.set(15, 0);
    c.put32(frame.at(slot::second), c.r(15));
    c.set(3, 0);
    c.put32(frame.at(slot::third), 0);
    const auto fixture_record = c.u32(match + fixture_at);
    if (compare_logical(c, c.u32(fixture_record + 0x14)) != 0) {
        c.set(15, c.u32(c.u32(match + fixture_at) + 0x14));
        r25 = u(std::int32_t(c.i16(c.r(15) + 0x18)) * 0x19 - 0x12c);
    } else {
        r25 = 0;
    }
    r23 = (0 - std::uint32_t(r25)) << 2;
    c.set(20, 0);

    if (compare_logical(c, c.u32(cand + 0x6c), team_word(0x101e901c, 0x101e902c, 0x101e9040, 0)) == 0) {
        bump(r27, 0x3e8);
    } else if (compare_logical(c, c.u32(cand + 0x6c), team_word(0x101e9060, 0x101e9070, 0x101e9084, 1)) == 0) {
        const auto third = team_word(0x101e909c, 0x101e90ac, 0x101e90c0, 0);
        c.set(4, third);
        const auto stats = match + u(s8(c.u8(pending() + player_side)) * 0xd8) + 0x7aac;
        c.set(3, stats);
        call_to(0x102107c0, 0x101e90e4);
        c.put32(frame.at(slot::stats), c.r(3));
        c.set(3, slot_u32(slot::stats));
        if (compare_logical(c, c.r(3)) == 0 || compare(c, s8(position(c, slot_u32(slot::stats)))) < 0) bump(r27, 0x3e8);
    } else {
        r25 = r25 + scaled(0x74, 0xf4, 0xf4);
        r20 = r20 + scaled(0x7c, 0xf4, 0xf4);
    }

    // The side that does not defend this half: scores from the player's stats.
    {
        const auto o = std::uint32_t(s8(c.u8(cand + player_side)) == 0);
        c.set(5, o);
        c.set({{3, cand}, {4, o}});
        call_to(0x10239d10, 0x101e9174);
        if (compare(c, s8(c.r(3))) != 0) {
            r25 = r25 + scaled(0x74, 0x9c, 0x9c);
            r20 = r20 + scaled(0x7c, 0x9c, 0x9c);
            r25 = r25 + scaled(0x6c, 0xc4, 0xe8);
            r20 = r20 + scaled(0x64, 0xc4, 0xe8, true);
        }
    }
    // Tactical flags of the pending player and of the team.
    if (!byte_flag(0x10272100, pending(), 0x101e921c)) {
        const auto team_flags = [&] {
            c.set(3, team_of_side());
            return tactical_flags(c, team_of_side());
        };
        const bool low_bit = has_flag(pending(), 0x101e9234, 0, 0x1);
        c.set(7, trait_low & 1);  // the bit test's scratch
        c.set(6, trait_low & 1);
        if (low_bit) {
            if (compare(c, std::int32_t(team_flags() & 8)) != 0) { bump(r25, -0x3e8); bump(r23, 0x3e8); }
            else if (compare(c, std::int32_t(team_flags() & 4)) != 0) { bump(r25, -0x2ee); bump(r23, 0x1f4); }
            else if (compare(c, std::int32_t(team_flags() & 2)) != 0) { bump(r25, 0xfa); bump(r23, -0xfa); }
        } else if (has_flag(pending(), 0x101e92fc, 0, 0x10)) {
            bump(r25, -0x3e8);
            bump(r23, 0x3e8);
        } else if (has_flag(pending(), 0x101e9340, 0, 0x8)) {
            bump(r25, -0x2ee);
            bump(r23, 0x1f4);
        } else {
            const bool bit = has_flag(pending(), 0x101e9384, 0, 0x4);
            c.set(15, trait_low & 4);  // xor r15,r5,r0; or r15,r3,r15 of the bit test
            if (bit) {
                bump(r25, 0xfa);
                bump(r23, -0xfa);
            }
        }
    }
}

// The cell the player would move to, or stays on.
void Receiver::destination() {
    bool moves = false;
    if (compare(c, slot_s16(slot::position_value), 1) != 0) {
        const auto draw = rnd(r19 * 6, 0x101e93d0);
        const double chance = widen(0x64, std::int32_t(draw), false);
        c.set_f(26, chance);
        const auto trait = s8(trait_10e(c, pending()));
        std::uint32_t r14 = u(trait * 0x26);
        c.set(14, r14);
        const auto primary = s8(primary_rating(c, cand));
        r14 = u(primary) * r14;
        c.set(14, r14);
        const auto secondary = s8(secondary_rating(c, cand));
        const std::uint32_t product = u(secondary) * r14;
        const double f1 = widen(0x6c, std::int32_t(product), false);
        c.set_f(1, f1);
        const double f0 = fpu.fdiv(f1, D(0x48));
        moves = greater_equal(f0, chance);
    }
    if (moves) {
        c.set({{3, match}, {4, cand}, {5, frame.at(slot::dest_row)}, {6, frame.at(slot::dest_column)}, {7, 0}});
        other_routine(0x101d8260, 0x101e946c);
        c.set(4, c.r(3));
        moves = compare(c, s8(c.r(3))) != 0;
    }
    if (!moves) {
        c.put8(frame.at(slot::moved), 0);
        r17 = row(c, cand);
        r26 = column(c, cand);
        return;
    }
    // Clamp the cell onto the pitch. The bytes come from 0x101d8260, which only stores cells it has checked (rows 0..8,
    // columns 0..11) and answers 0 otherwise, so no clamp ever applies; the compares are kept as the original has them.
    if (compare(c, slot_s8(slot::dest_row)) < 0) { c.set(14, 0); c.put8(frame.at(slot::dest_row), 0); }
    else if (compare(c, slot_s8(slot::dest_row), 8) > 0) { c.set(0, 8); c.put8(frame.at(slot::dest_row), 8); }
    if (compare(c, slot_s8(slot::dest_column)) < 0) { c.set(4, 0); c.put8(frame.at(slot::dest_column), 0); }
    else if (compare(c, slot_s8(slot::dest_column), 0xb) > 0) { c.set(3, 0xb); c.put8(frame.at(slot::dest_column), 0xb); }
    const auto cand_row = s8(row(c, cand));
    bool stays = false;
    if (compare(c, slot_s8(slot::dest_row), cand_row) == 0) {
        const auto cand_column = s8(column(c, cand));
        stays = compare(c, slot_s8(slot::dest_column), cand_column) == 0;
    }
    if (stays) {
        c.put8(frame.at(slot::moved), 0);
        r17 = row(c, cand);
        r26 = column(c, cand);
        return;
    }
    c.put8(frame.at(slot::moved), 1);
    r17 = slot_u8(slot::dest_row);
    r26 = slot_u8(slot::dest_column);
    enter(c, 0x101e9554, {match, other_side(), r17, r26, 1});
    c.set_f(31, nearest_distance(c, match, other_side(), r17, r26, 1));
    c.set(3, cand);
    call_to(0x10289560, 0x101e9560);
    c.set(4, u(std::int32_t(std::int16_t(c.r(3)))));
    if (compare(c, std::int32_t(std::int16_t(c.r(3)) & 0x40)) != 0) {
        c.set(15, u(s8(secondary_rating(c, cand))));
        c.set(14, primary_rating(c, cand));
        bump(r27, s8(c.r(14)) * s8(c.r(15)) * 0x32);
    }
}


// How crowded the destination is, and the chance the player has to be marked there.
void Receiver::crowding() {
    c.set(5, column(c, cand));
    enter(c, 0x101e9600, {match, c.u8(cand + player_side), c.r(5)});
    c.set(15, column_excess(c, match, c.u8(cand + player_side), c.r(5)));
    c.set(4, c.r(15) + 1);
    r18 = u(s8(c.r(4)));
    if (compare(c, s8(r18), -2) > 0) {
        c.set(3, match);
        c.set(4, u(s8(c.u8(cand + player_side)) == 0));
        other_routine(0x101d7d30, 0x101e9630);
        c.set(0, c.r(3));
        if (compare(c, s8(c.r(3))) != 0) {
            c.set(5, adjustment_byte(c, pending()));
            if (compare(c, s8(c.r(5))) != 0) {
                c.set(15, rnd(0x32, 0x101e9664));
                if (compare(c, s8(trait_10a(c, cand)), std::int32_t(c.r(15))) < 0) r18 = r18 + 1;
            }
        }
    }
    if (compare(c, kind(), 0x36) == 0) r18 = u(-4);
    if (compare(c, s8(r18)) >= 0) {
        r18 = u(s8(r18 * u(s8(r18))));
        r18 = r18 + 1;
        c.set(14, rnd(u(s8(r18)) * 10 + 10, 0x101e96c4));
        // The draw is compared as a whole word: a large bound can pass 127.
        if (compare(c, s8(trait_10a(c, cand)), std::int32_t(c.r(14))) < 0) {
            const auto t = s8(trait_10e(c, pending()));
            bump(r27, -std::int32_t(u(s8(r18)) * u(s8(r18)) * u(t * 0x190 + 0x1f4)));
        }
    }
}

// Whether the destination lies in an end area, how many opponents are near, and what that costs.
void Receiver::exposure() {
    const auto side_raw = c.u8(match + last_touch_at);
    enter(c, 0x101e9730, {match, other_side(), r17, r26});
    c.put8(frame.at(slot::end_other), inside_end_area(c, other_side(), r17, r26));
    enter(c, 0x101e9748, {match, side_raw, r17, r26});
    c.put8(frame.at(slot::end_own), inside_end_area(c, side_raw, r17, r26));
    c.set({{3, match}, {4, c.u8(frame.caller() + 0x1f)}, {5, origin_column}, {6, r17}, {7, r26}});
    other_routine(0x101ef8d0, 0x101e9768);
    c.set_f(30, c.f(1));
    enter(c, 0x101e9794, {match, other_side(), r17, r26, 1});
    c.put8(frame.at(slot::near_count), count_nearby_players(c, match, other_side(), r17, r26, 1));
    bump(r27, 0x7d0 - slot_s8(slot::near_count) * 0x190);
    if (compare(c, slot_s8(slot::end_other)) != 0 && compare(c, slot_s8(slot::near_count), 3) < 0 && compare(c, s8(r18), 1) < 0) {
        const double f1 = widen(0x64, 3 - slot_s8(slot::near_count), false);
        c.set_f(1, f1);
        c.set_f(2, D(0x40));
        enter(c, 0x101e9808);
        double result = power(c, f1, D(0x40));
        result = fpu.fmul(D(0x28), result);
        r27 = r27 + truncate(0x6c, result);
    }
    c.set({{3, pending()}, {4, 0x5d}, {5, c.u8(frame.caller() + 0x1f)}, {6, origin_column}, {7, r17}, {8, r26}, {10, frame.at(0x4a)}});
    c.set_f(1, c.f(30));
    call_to(0x102818f0, 0x101e9848);
    c.set(15, u(s8(c.r(3))));
    if (compare(c, s8(c.u8(pending() + 0x2d)), s8(c.r(15))) != 0) {
        c.set(15, match + 0x10000);  // addis r15,r30,1
        c.set(15, rnd(u(s8(c.u8(pending() + 0x2e)) * 0x190), 0x101e987c));
        r27 = r27 - c.r(15);
    }
}

// Bonuses by the kind of play asked for, and the adjustments by the pending player's gifts.
bool Receiver::kind_bonuses() {
    const auto fused_sum = [&](std::uint32_t addend, std::uint32_t site, std::uint32_t truncate_site) {
        double f0 = F(addend);
        f0 = fpu.multiply_add(site, F(0x20), c.f32(cand + 0xa0), f0);
        f0 = fpu.multiply_add(site + 4, F(0x2c), c.f32(cand + 0x90), f0);
        f0 = fpu.multiply_add(site + 8, F(0x2c), c.f32(cand + 0x9c), f0);
        c.set_f(1, c.f32(cand + 0xa0));
        r27 = r27 + truncate(truncate_site, f0);
    };
    bool special = compare(c, kind(), 0x34) == 0;
    if (!special) {
        c.set(15, u(kind()));
        special = compare(c, kind(), 0x37) == 0;
    }
    if (special) {
        if (compare(c, slot_s8(slot::end_other)) != 0) {
            c.set(3, cand);
            call_to(0x10245160, 0x101e98bc);
            const double f1 = c.f(1);
            const double f2 = fpu.multiply_add(0x101e98c4, F(0xdc), f1, F(0xd8));
            const double f0 = fpu.multiply_add(0x101e98d0, F(0x20), c.f32(cand + 0xb0), f2);
            c.set_f(1, F(0x20));
            r27 = r27 + truncate(0x64, f0);
        } else if (less(c.f(30), D(0x40))) {
            c.set(15, slot_u8(slot::near_count));
            if (compare(c, s8(c.r(15))) == 0) {
                fused_sum(0xe0, 0x101e9920, 0x64);
            } else {
                const double f1 = widen(0x64, slot_s8(slot::near_count), false);
                c.set_f(1, f1);
                if (less(f1, D(0x40))) fused_sum(0xe4, 0x101e998c, 0x64);
            }
        }
    }
    if (greater(c.f(31), D(0x1b8)) && compare(c, slot_s8(slot::end_other)) == 0) {
        std::int32_t scaled_adjustment = (s8(adjustment_byte(c, pending())) + 5) * 0x2bc;
        double f0 = widen(0x64, scaled_adjustment, true);
        f0 = fpu.fdivs(f0, c.f(29));
        r23 = r23 - truncate(0x6c, f0);
        scaled_adjustment = (s8(adjustment_byte(c, pending())) + 5) * 0x1f4;
        f0 = widen(0x74, scaled_adjustment, true);
        f0 = fpu.fdivs(f0, c.f(29));
        r16 = r16 - truncate(0x7c, f0);
        c.set_f(1, D(0x60));
    } else if (compare(c, slot_s8(slot::moved)) == 0) {
        const auto weighted = [&](std::uint32_t widen_site, std::uint32_t scale_at, std::uint32_t truncate_site) {
            const auto gift = s8(trait_10e(c, pending()));
            const double f2 = widen(widen_site, 0x1e - gift, true);
            double f1 = F(0x28);
            double f0 = c.f32(cand + 0xb0);
            f0 = fpu.fsubs(f1, f0);
            f1 = fpu.fmuls(f0, f2);
            c.set_f(1, f1);
            f0 = F(scale_at);
            f0 = fpu.fmuls(f0, f1);
            f0 = fpu.fmuls(c.f(29), f0);
            return truncate(truncate_site, f0);
        };
        r23 = r23 - weighted(0x64, 0xe8, 0x6c);
        c.set(7, 0x43300000);  // `lis r7,0x4330` of the int-to-double; later calls pass r7 on
        r16 = r16 - weighted(0x74, 0x38, 0x7c);
    }
    if (compare(c, slot_s16(slot::position_value), 1) == 0) {
        c.set(5, match + 0x10000);  // addis r5,r30,1 of the pending player's address
        const bool pending_flag = byte_flag(0x102721a0, pending(), 0x101e9b3c);
        c.set(6, u(s8(c.r(3))));  // extsb r6,r4 of the result
        if (!pending_flag) {
            if (compare(c, std::int32_t(rnd(0x20000 - 0x7960, 0x101e9b58))) != 0) return false;
        }
        if (!byte_flag(0x10272050, pending(), 0x101e9b70)) {
            const double f1 = widen(0x64, s8(adjustment_byte(c, pending())) * 0x3e8, false);
            c.set_f(1, f1);
            double f0 = D(0x1c0);
            f0 = fpu.fadd(f0, c.f(31));
            f0 = fpu.fmul(f0, f1);
            r27 = r27 - truncate(0x6c, f0);
        }
        r16 = r16 - u(s8(adjustment_byte(c, pending())) * 0x898 + 0x3e8);
        r23 = r23 - u(s8(adjustment_byte(c, pending())) * 0x1388 + 0x7d0);
        r20 = r20 - u(s8(adjustment_byte(c, pending())) * 0x5dc);
    }
    if (compare(c, position_value_of(pending(), 0x101e9c30), 1) == 0) bump(r27, -0x3e8);
    if (compare(c, slot_s8(slot::origin_own)) != 0) {
        if (less(c.f(31), D(0x98))) {
            c.set(3, pending());
            call_to(0x10289560, 0x101e9c68);
            if (compare(c, std::int32_t(std::int16_t(c.r(3))), 1) == 0 && byte_flag(0x102721a0, cand, 0x101e9c80)) bump(r27, -0x7d0);
        }
        if (compare(c, slot_s8(slot::end_own)) != 0) {
            const auto gift = s8(adjustment_byte(c, pending()));
            c.set(15, u(gift) ^ 0x80000000u);
            const double f2 = widen(0x64, gift, true);
            double f0 = fpu.fmuls(c.f(31), c.f(31));
            f0 = fpu.fmuls(F(0x20), f0);
            const double f1 = fpu.fmuls(f0, f2);
            c.set_f(1, f1);
            f0 = fpu.fmuls(F(0xec), f1);
            r27 = r27 - truncate(0x6c, f0);
        }
    }
    if (compare(c, slot_s8(slot::end_own)) != 0) {
        const auto gift = s8(adjustment_byte(c, pending()));
        c.set(14, u(gift) << 5);
        c.set_f(1, D(0x60));
        const double f0 = widen(0x64, gift * 32 + 0xc8, true);
        c.set(14, 0x43300000);
        r27 = r27 - truncate(0x6c, fpu.fmuls(c.f(31), f0));
    }
    return true;
}

// Scores for the pending player's own traits and where the destination cell lies.
void Receiver::trait_scores() {
    c.set({{3, match}, {4, pending()}, {5, r17}, {6, r26}});
    other_routine(0x101eeac0, 0x101e9d6c);
    c.set(5, c.r(3));
    c.put8(frame.at(slot::nearby_ok), c.r(5));
    if (compare(c, slot_s8(slot::nearby_ok)) != 0) {
        const auto gift = [&](std::uint32_t site) { return widen(site, s8(adjustment_byte(c, pending())), true); };
        double f3 = gift(0x64);
        c.set(14, 0x43300000);
        double f0 = fpu.multiply_add(0x101e9dbc, F(0x20), c.f32(cand + 0xb0), F(0xf0));
        double f1 = fpu.fmuls(f0, f3);
        double f2 = fpu.fdivs(f1, F(0x28));
        f0 = fpu.multiply_add(0x101e9ddc, F(0x20), c.f32(pending() + 0x90), f2);
        c.set_f(1, F(0x20));
        c.put32(frame.at(slot::second), slot_u32(slot::second) + truncate(0x6c, f0));
        f3 = gift(0x74);
        f0 = fpu.multiply_add(0x101e9e30, F(0x20), c.f32(cand + 0xb0), F(0xe0));
        f1 = fpu.fmuls(f0, f3);
        f2 = fpu.fdivs(f1, F(0x28));
        f0 = fpu.multiply_add(0x101e9e50, F(0x20), c.f32(pending() + 0x90), f2);
        c.set_f(1, F(0x20));
        c.put32(frame.at(slot::third), slot_u32(slot::third) + truncate(0x7c, f0));
        if (has_flag(pending(), 0x101e9e78, 4, 0x0)) {
            c.set(15, slot_u32(slot::second));
            c.set(14, match + 0x10000);
            c.set(5, pending());
            f0 = fpu.multiply_add(0x101e9ebc, F(0x20), c.f32(pending() + 0xe4), F(0xf0));
            c.put32(frame.at(slot::second), c.r(15) + truncate(0x64, f0));
            const auto third = slot_u32(slot::third);
            c.set(4, match + 0x10000);
            c.set(3, pending());
            f0 = fpu.multiply_add(0x101e9eec, F(0x20), c.f32(pending() + 0xe4), F(0xf0));
            c.set_f(1, c.f32(pending() + 0xe4));
            c.put32(frame.at(slot::third), third + truncate(0x6c, f0));
        }
        bump(r23, -0x7d0);
        bump(r20, -0x7d0);
        bump(r25, -0x7d0);
    }
    const bool flag_40 = has_flag(pending(), 0x101e9f1c, 0, 0x40);
    c.set(15, c.r(3));  // or r15,r3,r3: the trait word's high half
    if (flag_40) {
        bool extra = false;
        c.set(4, c.u8(match + last_touch_at));
        c.set(5, u(side()));
        if (compare(c, side()) == 0) {
            const auto r = s8(row(c, cand));
            c.set(0, u(r));
            c.set(15, u(r));
            extra = compare(c, r, 2) <= 0;
        }
        if (!extra) {
            c.set(0, u(side()));
            if (compare(c, side(), 1) == 0) {
                const auto r = s8(row(c, cand));
                c.set(0, u(r));
                extra = compare(c, r, 6) >= 0;
            }
        }
        if (extra) bump(r27, 0x3e8);
    }
    const bool flag_100 = has_flag(pending(), 0x101e9fac, 0, 0x100);
    c.set(14, trait_low & 0x100);  // and r14,r4,r0
    if (flag_100) {
        bool extra = false;
        c.set(7, c.u8(match + last_touch_at));
        c.set(5, u(side()));
        if (compare(c, side()) == 0) {
            const auto r = s8(row(c, cand));
            c.set(6, u(r));
            extra = compare(c, r, 6) >= 0;
        }
        if (!extra) {
            c.set(0, c.u8(match + last_touch_at));
            c.set(3, u(side()));
            if (compare(c, side(), 1) == 0) {
                c.set(4, row(c, cand));
                const auto r = s8(c.r(4));
                c.set(5, u(r));
                extra = compare(c, r, 2) <= 0;
            }
        }
        if (extra) bump(r27, 0x3e8);
    }
    if (has_flag(pending(), 0x101ea038, 0, 0x80)) {
        const bool flagged = byte_flag(0x102720d0, cand, 0x101ea070);
        c.set(4, c.r(3));
        c.set(3, u(s8(c.r(4))));
        if (!flagged) {
            c.set(0, slot_u8(slot::nearby_ok));
            c.set(3, u(slot_s8(slot::nearby_ok)));
            if (compare(c, slot_s8(slot::nearby_ok)) == 0) bump(r27, 0x3e8);
        }
    }
}

// Whether the candidate's destination is no further ahead than the origin, as the side attacks.
bool Receiver::no_progress() {
    if (compare(c, side()) == 0 && compare(c, s8(origin_column), s8(r26)) >= 0) return true;
    return compare(c, side(), 1) == 0 && compare(c, s8(origin_column), s8(r26)) <= 0;
}

// The distance figure f31 as adjusted for where the origin and the destination lie, and the
// cost of moving the ball backward.
void Receiver::distance_adjustments() {
    const auto side_raw = [&] { return c.u8(match + last_touch_at); };
    const auto origin_row_s8 = [&] { return s8(c.u8(frame.caller() + 0x1f)); };
    const auto edge = [&](std::uint32_t entry_back, std::uint32_t column) {
        enter(c, entry_back, {match, side_raw(), column});
        const auto answer = s8(beyond_row(c, side_raw(), column));
        return answer;
    };
    bool settled = false;
    if (compare(c, slot_s8(slot::origin_other)) != 0 && compare(c, slot_s8(slot::end_other)) == 0) {
        c.set_f(31, F(0xc));
        bump(r27, -0xfa0);
        settled = true;
    } else if (compare(c, slot_s8(slot::end_other)) != 0 && compare(c, slot_s8(slot::origin_other)) == 0) {
        const double f1 = D(0x58);
        c.set_f(1, f1);
        const double f0 = fpu.fmuls(c.f(31), c.f(31));
        c.set_f(31, fpu.frsp(fpu.fadd(f1, f0)));
        settled = true;
    }
    if (!settled) {
        bool forward = false;
        if (compare(c, origin_row_s8(), 2) > 0) {
            bool far = true;
            c.set(15, u(slot_s8(slot::origin_other)));  // extsb r15,r3
            if (compare(c, slot_s8(slot::origin_other)) == 0) {
                far = compare(c, edge(0x101ea124, origin_column)) != 0 && compare(c, origin_row_s8(), 6) >= 0;
            }
            if (far) {
                enter(c, 0x101ea150, {match, side_raw(), r26});
                c.set(15, beyond_row(c, side_raw(), r26));
                if (compare(c, s8(c.r(15))) != 0 && compare(c, s8(r17), 2) < 0) {
                    c.set_f(31, F(0xc));
                    bump(r27, -0x7d0);
                    forward = true;
                }
            }
        }
        if (!forward) {
            c.set(15, u(origin_row_s8()));
            if (compare(c, origin_row_s8(), 6) < 0) {
                bool near = true;
                if (compare(c, slot_s8(slot::origin_other)) == 0) {
                    const auto answer = edge(0x101ea1a8, origin_column);
                    c.set(15, u(answer));
                    near = compare(c, answer) != 0 && compare(c, origin_row_s8(), 2) <= 0;
                }
                if (near) {
                    enter(c, 0x101ea1d8, {match, side_raw(), r26});
                    const auto answer = s8(beyond_row(c, side_raw(), r26));
                    c.set(15, u(answer));
                    if (compare(c, answer) != 0 && compare(c, s8(r17), 6) > 0) {
                        c.set_f(31, F(0xc));
                        bump(r27, -0x7d0);
                    }
                }
            }
        }
    }
    if (no_progress() && byte_flag(0x102721a0, cand, 0x101ea244)) {
        bump(r23, -0x1388);
        bump(r20, -0xbb8);
        bump(r16, -0xbb8);
    }
    const auto hit_penalty = [&](double step, std::uint32_t truncate_site) {
        c.set_f(1, step);
        const double adjusted = fpu.frsp(fpu.fsub(c.f(31), fpu.fsub(step, D(0x90))));
        c.set_f(31, adjusted);
        if (less(adjusted, D(0x50))) {
            const double f0 = fpu.fmuls(F(0xf0), fpu.fmuls(adjusted, adjusted));
            c.set_f(1, F(0xf0));
            r27 = r27 - truncate(truncate_site, f0);
            c.set_f(31, F(0xc));
        }
    };
    if (compare(c, side()) == 0 && compare(c, s8(origin_column), s8(r26)) > 0) {
        hit_penalty(widen(0x64, s8(origin_column) - s8(r26), false), 0x64);
    } else if (compare(c, side(), 1) == 0 && compare(c, s8(origin_column), s8(r26)) < 0) {
        hit_penalty(widen(0x64, s8(r26) - s8(origin_column), false), 0x64);
    } else {
        c.set({{3, match}, {4, r26}, {5, side_raw()}, {6, 0}});
        other_routine(0x101ee680, 0x101ea380);
        c.set(7, u(s8(c.r(3))));  // extsb r7,r6 of the result
        if (compare(c, s8(c.r(3))) == 0) {
            const double f1 = D(0x58);
            c.set_f(1, f1);
            const double f0 = fpu.fmuls(F(0x18), c.f(31));
            c.set_f(31, fpu.frsp(fpu.fadd(f1, f0)));
        } else {
            enter(c, 0x101ea3b4, {match, r17});
            const auto central = outside_central_rows(c, r17);
            if (compare(c, s8(central)) != 0) {
                if (!byte_flag(0x102721a0, cand, 0x101ea3cc) && has_flag(cand, 0x101ea3e0, 4, 0x0)) {
                    const double f0 = fpu.multiply_add(0x101ea418, D(0x1b8), c.f(31), D(0x58));
                    c.set_f(1, D(0x1b8));
                    c.set_f(31, fpu.frsp(f0));
                }
            }
        }
    }
    if (compare(c, s8(r18)) >= 0) {
        const double f2 = widen(0x64, s8(r18), true);
        const double adjusted = fpu.fsubs(c.f(31), f2);
        c.set_f(31, adjusted);
        if (less(adjusted, D(0x50))) {
            const double f0 = fpu.fmuls(F(0xec), fpu.fmuls(adjusted, adjusted));
            r27 = r27 - truncate(0x64, f0, 1);
            c.set_f(31, F(0xc));
        }
    }
    {
        const double f0 = fpu.fmuls(F(0x20), fpu.fmuls(c.f(31), c.f(31)));
        r27 = r27 + truncate(0x64, f0, 1);
    }
}

// Penalties for a pass that goes toward the side's own goal, or into a crowd.
void Receiver::lane_scores() {
    const auto side_raw = [&] { return c.u8(match + last_touch_at); };
    bool handled = false;
    if (no_progress() && less(c.f(31), D(0x270))) {
        enter(c, 0x101ea4f4, {match, side_raw(), r26});
        const auto beyond = beyond_row(c, side_raw(), r26);
        if (compare(c, s8(beyond)) == 0) {
            bool open = compare(c, s8(adjustment_byte(c, pending()))) != 0;
            if (!open) open = compare(c, s8(trait_10e(c, pending()))) != 0;
            if (open) {
                handled = true;
                const auto edge_cost = [&](bool second) {
                    const auto half = std::int32_t(c.i16(frame.at(slot::half)));
                    std::int32_t cells;
                    if (compare(c, side()) == 0) {
                        cells = s8(origin_column) + 1 - s8(r26);
                        if (second) c.set(14, u(s8(origin_column)));
                    } else if (compare(c, side(), 1) == 0) {
                        cells = s8(r26) + 1 - s8(origin_column);
                        if (!second) c.set(15, u(s8(r26) + 1));
                    } else {
                        return;
                    }
                    const auto cost = u(half) * u(cells);
                    if (second && side() == 0) c.set(14, cost);
                    r27 = r27 - cost;
                };
                if (byte_flag(0x10272050, cand, 0x101ea544)) {
                    const double f0 = fpu.multiply_add(0x101ea55c, F(0xf8), c.f(31), F(0xf4));
                    c.set_f(1, F(0xf8));
                    c.put16(frame.at(slot::half), truncate(0x64, f0));
                    edge_cost(false);
                } else {
                    const double f0 = fpu.multiply_add(0x101ea5d8, F(0xfc), c.f(31), F(0xdc));
                    c.set_f(1, F(0xdc));
                    c.put16(frame.at(slot::half), truncate(0x64, f0));
                    edge_cost(true);
                }
            }
        }
    }
    if (!handled) {
        bool penalised = false;
        if (less(c.f(31), F(0x18))) {
            penalised = byte_flag(0x102721a0, cand, 0x101ea660);
            c.set(14, u(s8(c.r(3))));
        }
        if (penalised) {
            bump(r27, -0x5dc);
        } else if (compare(c, slot_s16(slot::position_value), 1) != 0) {
            const auto gift = [&] { return s8(trait_10e(c, pending())); };
            const auto weigh = [&](std::int32_t gift_plus, std::uint32_t widen_site, std::uint32_t truncate_site) {
                const double f1 = widen(widen_site, gift_plus, true);
                c.set_f(1, f1);
                const double f0 = fpu.fmuls(fpu.fmuls(c.f(31), c.f(31)), f1);
                r27 = r27 + truncate(truncate_site, f0);
            };
            weigh(gift() + 10, 0x64, 0x6c);
            enter(c, 0x101ea6dc, {match, side_raw(), r26});
            const auto beyond = beyond_row(c, side_raw(), r26);
            if (compare(c, s8(beyond)) != 0) {
                c.set({{3, match}, {4, cand}});
                other_routine(0x101eecb0, 0x101ea6f4);
                if (compare(c, s8(c.r(3))) != 0) {
                    bump(r27, (gift() + 10) * 0x14);
                } else {
                    c.set({{3, match}, {4, cand}, {5, 0}});
                    other_routine(0x101d8d70, 0x101ea730);
                    if (compare(c, s8(c.r(3))) != 0) bump(r27, (gift() + 10) * 0x32);
                }
            } else if (!byte_flag(0x102721a0, cand, 0x101ea768)) {
                c.set({{3, match}, {4, cand}});
                other_routine(0x101eecb0, 0x101ea780);
                c.set(15, c.r(3));
                c.set(14, u(s8(c.r(15))));
                if (compare(c, s8(c.r(14))) != 0) {
                    bump(r27, (gift() + 10) << 3);
                } else {
                    c.set({{3, match}, {4, cand}, {5, 0}});
                    other_routine(0x101d8d70, 0x101ea7c4);
                    if (compare(c, s8(c.r(3))) != 0) {
                        c.set(15, u(gift() + 10));
                        bump(r27, (gift() + 10) * 0x14);
                    }
                }
            }
            bool behind = false;
            if (compare(c, side()) == 0) behind = compare(c, s8(r26), s8(origin_column)) > 0;
            if (!behind && compare(c, side(), 1) == 0) behind = compare(c, s8(r26), s8(origin_column)) < 0;
            if (behind) {
                c.set(15, u(gift()));
                weigh(gift() + 10, 0x64, 0x6c);
            }
        }
    }
}

// Costs that depend on the candidate's own figures, and the checks that drop the candidate.
bool Receiver::own_costs() {
    if (compare(c, s8(c.u8(cand + 0x2c)), 1) > 0) {
        const std::uint32_t gift = u(s8(trait_10e(c, pending())) * 0x32);
        c.set(14, gift);
        const auto restriction = s8(c.u8(cand + 0x2c));
        r27 = r27 - u(restriction) * u(restriction) * (gift + 0xc8);
        c.set(7, u(restriction) * u(restriction));
    }
    c.set(3, u(slot_s8(slot::scratch)));
    const auto drawn = rnd(u(slot_s8(slot::scratch)), 0x101ea8cc);
    c.set(6, u(s8(drawn)));
    c.put8(frame.at(slot::scratch), drawn);
    const auto n = u(slot_s8(slot::scratch));
    const auto square = n * n;
    r16 = r16 - square * 0x9b;
    r25 = r25 - square * 0x1f4;
    r20 = r20 - square * 0x9b;
    r23 = r23 - square * 0x19;
    const auto wheel = s8(c.u8(word_6c(c, pending()) + 0x67));
    if (compare(c, std::int32_t(rnd(u(wheel + 1), 0x101ea960))) == 0) {
        c.set(15, word_6c(c, pending()));
        c.set(4, c.r(15));
        c.set(3, word_6c(c, cand));
        call_to(0x100d9580, 0x101ea988);
        if (compare(c, s8(c.r(3))) != 0) return false;
        c.set(15, word_6c(c, pending()));
        c.set(4, c.r(15));
        c.set(3, word_6c(c, cand));
        call_to(0x100d9520, 0x101ea9b4);
        if (compare(c, s8(c.r(3))) != 0) bump(r27, 0x3e8);
    }
    return true;
}

// The score for the kind of pass asked for, kept as the candidate's result when it is the best so far.
void Receiver::choose_kind() {
    const auto raw_kind = [&] { return c.u8(frame.caller() + 0x3b); };
    r24 = u(-100000);
    c.set(15, u(-1));
    c.put8(frame.at(slot::chosen), c.r(15));
    const auto pend = [&] { return pending(); };
    // A window of the pending player's speed figure, read from the pool.
    const auto window = [&](double low, double high) { return less_equal(c.f(30), low) && greater_equal(c.f(30), high); };
    if (compare(c, kind(), -1) > 0) {
        c.set(15, u(kind()));
        switch (kind()) {
        case 0x36: {
            const double f3 = F(0x10);
            c.set_f(1, D(0x48));
            const double f0 = fpu.frsp(fpu.fdiv(c.f32(pend() + 0xec), D(0x48)));
            const double f27 = fpu.fadds(f3, f0);
            c.set_f(27, f27);
            if (less_equal(c.f(30), f27)) r24 = r27;
            break;
        }
        case 0x1e:
        case 0x5a:
        case 0x37: {
            const bool is_37 = kind() == 0x37;
            const double f3 = F(8);
            const double f0 = F(0x100);
            if (!is_37) c.set(15, pend());
            const double f2 = c.f32(pend() + 0xdc);
            double f1 = fpu.fsubs(f0, f2);
            c.set_f(1, f1);
            if (is_37) {
                f1 = fpu.fdiv(f1, D(0x88));
                c.set_f(1, f1);
            }
            const double scaled_down = fpu.frsp(is_37 ? f1 : fpu.fdiv(f1, D(0x88)));
            const double limit = fpu.fsubs(f3, scaled_down);
            if (is_37) c.set_f(26, limit);
            if (less_equal(c.f(30), limit)) {
                if (is_37) c.set_f(1, F(0x104));
                if (greater_equal(c.f(30), F(0x104))) {
                    r24 = r23 + r27;
                    break;
                }
            }
            if (is_37) r24 = r25 + r27;
            break;
        }
        case 0x1f:
            if (less_equal(c.f(30), F(0x108))) {
                c.set_f(1, D(0x1b8));
                if (greater_equal(c.f(30), D(0x1b8))) r24 = r16 + r27;
            }
            break;
        case 0x3c:
            if (less_equal(c.f(30), F(0x104)) && greater_equal(c.f(30), F(0x108))) r24 = r20 + r27;
            break;
        case 0x34:
            c.set_f(1, D(0x278));
            if (less_equal(c.f(30), D(0x278)) && (c.set_f(1, D(0x40)), greater_equal(c.f(30), D(0x40))) && compare(c, s8(r17), 1) > 0 &&
                compare(c, s8(r17), 7) < 0 && (compare(c, s8(r26), 3) < 0 || compare(c, s8(r26), 8) > 0))
                r24 = r20 + r27;
            else
                r24 = r25 + r27;
            break;
        case 0xb:
            c.set_f(1, F(0x18));
            if (less_equal(c.f(30), F(0x18))) r24 = r25 + r27;
            break;
        case 6:
        case 8:
            if (less_equal(c.f(30), F(8))) {
                c.set_f(1, F(0x10));  // lfs f1,0x10(r31): kind 7 loads f0 instead
                if (greater_equal(c.f(30), F(0x10))) r24 = slot_u32(slot::second) + r27;
            }
            break;
        case 7:
            if (less_equal(c.f(30), F(8)) && greater_equal(c.f(30), F(0x10))) r24 = slot_u32(slot::third) + r27;
            break;
        case 0x3d:
            if (less_equal(c.f(30), F(0x18))) r24 = slot_u32(slot::first) + r27;
            break;
        default:
            r24 = r27;
            break;
        }
        c.set(3, raw_kind());
        c.put8(frame.at(slot::chosen), raw_kind());
    } else {
        const auto in_state = [&] { return s8(c.u8(match + shot_state_at)); };
        if (compare(c, in_state(), 4) == 0) {
            c.set_f(1, F(0x18));
            if (less_equal(c.f(30), F(0x18))) {
                r24 = slot_u32(slot::first) + r27;
                c.put8(frame.at(slot::chosen), 0x3d);
            }
        } else {
            double f0 = F(0x100);
            const double f2 = c.f32(pend() + 0xdc);
            const double f1 = fpu.fsubs(f0, f2);
            c.set_f(1, f1);
            f0 = fpu.frsp(fpu.fdiv(f1, D(0x88)));
            f0 = fpu.fsubs(F(8), f0);
            if (less_equal(c.f(30), f0) && greater_equal(c.f(30), F(0x104))) {
                r24 = r23;
                c.put8(frame.at(slot::chosen), 0x1e);
            }
            if (compare(c, slot_s8(slot::nearby_ok)) != 0 && window(F(8), F(0x10)) && compare(c, std::int32_t(slot_u32(slot::second)), std::int32_t(r24)) > 0) {
                r24 = slot_u32(slot::second);
                c.put8(frame.at(slot::chosen), 8);
            }
            if (compare(c, slot_s8(slot::nearby_ok)) != 0 && window(F(8), F(0x10)) && compare(c, std::int32_t(slot_u32(slot::third)), std::int32_t(r24)) > 0) {
                r24 = slot_u32(slot::third);
                c.put8(frame.at(slot::chosen), 7);
            }
            c.set_f(1, F(0x18));
            if (less_equal(c.f(30), F(0x18)) && compare(c, std::int32_t(r25), std::int32_t(r24)) > 0 && compare(c, in_state(), 3) < 0) {
                r24 = r25;
                c.put8(frame.at(slot::chosen), 0xb);
            }
            if (less_equal(c.f(30), F(0x104)) && greater_equal(c.f(30), F(0x108)) && compare(c, std::int32_t(r20), std::int32_t(r24)) > 0) {
                r24 = r20;
                c.put8(frame.at(slot::chosen), 0x3c);
            }
            if (less_equal(c.f(30), F(0x108)) && greater_equal(c.f(30), D(0x1b8)) && compare(c, std::int32_t(r16), std::int32_t(r24)) > 0) {
                r24 = r16;
                if (compare(c, in_state()) > 0) {
                    c.set(7, 0x20);
                    c.put8(frame.at(slot::chosen), 0x20);
                }
                c.put8(frame.at(slot::chosen), 0x1f);
            }
            r24 = r24 + r27;
        }
    }
    // The best candidate so far is remembered with the figures of its move.
    c.set(5, slot_u8(slot::chosen));
    c.set(3, u(slot_s8(slot::chosen)));
    bool better = false;
    if (compare(c, slot_s8(slot::chosen), -1) > 0) {
        c.set(6, c.u32(frame.caller() + 0x34));
        c.set(4, c.u32(out_score_pointer()));
        better = compare(c, std::int32_t(r24), std::int32_t(c.r(4))) > 0;
    }
    if (better) {
        c.put32(frame.at(slot::best), cand);
        c.set(6, c.u32(frame.caller() + 0x34));
        c.put32(out_score_pointer(), r24);
        c.set(3, slot_u8(slot::chosen));
        c.set(5, c.u32(frame.caller() + 0x24));
        c.put8(c.r(5), c.r(3));
        c.set(3, slot_u8(slot::moved));
        c.set(5, c.u32(frame.caller() + 0x28));
        c.put8(c.r(5), c.r(3));
        c.set(4, c.u32(frame.caller() + 0x2c));
        c.put8(c.r(4), r17);
        c.set(6, c.u32(frame.caller() + 0x30));
        c.put8(c.r(6), r26);
    }
}

bool Receiver::candidate() {
    if (!reachable()) return false;
    special_traits();
    team_scores();
    destination();
    crowding();
    exposure();
    if (!kind_bonuses()) return false;
    trait_scores();
    distance_adjustments();
    lane_scores();
    if (!own_costs()) return false;
    choose_kind();
    return true;
}

std::uint32_t Receiver::run() {
    if (!start()) return result(c, 0);
    const auto other = other_side();
    const auto rw = origin_row_byte();
    enter(c, 0x101e8bd0, {match, other, rw, origin_column});
    c.put8(frame.at(slot::origin_other), inside_end_area(c, other, rw, origin_column));
    enter(c, 0x101e8be8, {match, c.u8(match + last_touch_at), rw, origin_column});
    c.put8(frame.at(slot::origin_own), inside_end_area(c, c.u8(match + last_touch_at), rw, origin_column));
    c.set(14, column(c, pending()));
    c.set(5, row(c, pending()));
    enter(c, 0x101e8c30, {match, other_side(), c.r(5), c.r(14), 1});
    c.set_f(28, nearest_distance(c, match, other_side(), c.r(5), c.r(14), 1));
    cand = match + u(side() * std::int32_t(side_size)) + players_at;
    c.set(3, 0);
    c.put8(frame.at(slot::index), 0);
    for (;;) {
        c.set(3, u(slot_s8(slot::index)));
        if (compare(c, slot_s8(slot::index), 0x14) >= 0) break;
        candidate();
        const auto index = slot_u8(slot::index);
        c.set(4, index);
        c.set(3, index + 1);
        c.put8(frame.at(slot::index), index + 1);
        c.set(3, cand);
        cand = cand + player_size;
        c.put32(frame.at(slot::previous), c.r(3));
    }
    return result(c, slot_u32(slot::best));
}
}  // namespace

double power(Cpu& c, double base, double exponent) {
    Frame frame(c, 0x101eaf10, 0x40, {});
    c.put64(frame.at(0x58), std::bit_cast<std::uint64_t>(base));
    c.put64(frame.at(0x60), std::bit_cast<std::uint64_t>(exponent));
    c.set_f(1, c.f64(frame.at(0x58)));
    c.set_f(2, c.f64(frame.at(0x60)));
    c.call(0x10780fc0, 0x101eaf30);
    c.set(2, c.u32(frame.at(0x14)));  // lwz r2,0x14(r1): the glue saved the TOC pointer there
    return c.f(1);
}

std::uint32_t best_pass_receiver(Cpu& c, std::uint32_t match, std::uint32_t origin_row, std::uint32_t origin_column,
                                 std::uint32_t out_action, std::uint32_t out_flag, std::uint32_t out_row,
                                 std::uint32_t out_column, std::uint32_t out_score) {
    Frame frame(c, 0x101e8950, 0x100, {.first_gpr = 14, .first_fpr = 26});
    c.put8(frame.caller() + 0x1f, origin_row);
    c.put32(frame.caller() + 0x24, out_action);
    c.put32(frame.caller() + 0x28, out_flag);
    c.put32(frame.caller() + 0x2c, out_row);
    c.put32(frame.caller() + 0x30, out_column);
    c.put32(frame.caller() + 0x34, out_score);
    keep(c, {{22, c.toc(-0x7a6c)}, {31, c.toc(-0x52c8)}, {29, c.toc(-0x52c4)}, {30, match}, {21, origin_column}});
    Receiver receiver(c, frame, match, origin_column);
    return receiver.run();
}
}  // namespace cm::aot::match_eng

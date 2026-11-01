// MATCH_ENG.CPP, team_a: choosing the best player of a side in the ball's cell for a kind of action.
#include "match_eng_space.hpp"
#include "match_eng_team_a.hpp"

namespace cm::aot::match_eng {
using namespace guest;

namespace {
constexpr std::uint32_t shot_state_at = 0x791f;
constexpr std::uint32_t ball_row_at = 0x791d, ball_column_at = 0x791e;  // as the players' +0x105 / +0x106
constexpr std::uint32_t team_tactics = 0x7c6c, team_tactics_size = 0x18e0;
constexpr std::uint32_t random_below = 0x101d3c90;
constexpr std::uint32_t player_traits = 0x10286470;  // 64 trait bits: r3 high word, r4 low word
constexpr std::uint32_t player_kind = 0x10289560;

namespace slot {
constexpr std::uint32_t flag_a = 0x38, count_b = 0x39, special = 0x3a, result = 0x3b, half_c = 0x3c, half_e = 0x3e, half_40 = 0x40,
                        word_44 = 0x44, word_48 = 0x48, index = 0x4c, best = 0x50;
}

class Chooser {
public:
    Chooser(Cpu& cpu, Frame& f, std::uint32_t match_, std::uint32_t reference_)
        : c(cpu), frame(f), fpu(cpu), match(match_), reference(reference_), cand(cpu, cpu.r(30)), r16(cpu, cpu.r(16)),
          r17(cpu, cpu.r(17)), r18(cpu, cpu.r(18)), r19(cpu, cpu.r(19)), r22(cpu, cpu.r(22)), r23(cpu, cpu.r(23)),
          r26(cpu, cpu.r(26)) {}
    std::uint32_t run();

private:
    double F(std::uint32_t at) const { return c.f32(c.r(29) + at); }
    double D(std::uint32_t at) const { return c.f64(c.r(31) + at); }
    std::int32_t kind() const { return s8(c.u8(frame.caller() + 0x27)); }
    std::int32_t flag_10() const { return s8(c.u8(frame.caller() + 0x3f)); }
    std::uint32_t slot_u32(std::uint32_t at) const { return c.u32(frame.at(at)); }
    std::uint32_t slot_u8(std::uint32_t at) const { return c.u8(frame.at(at)); }
    std::int32_t slot_s8(std::uint32_t at) const { return s8(c.u8(frame.at(at))); }
    std::int32_t slot_s16(std::uint32_t at) const { return c.i16(frame.at(at)); }
    std::uint32_t flag_pointer() const { return c.r(20); }
    std::int32_t state() const { return s8(c.u8(match + shot_state_at)); }

    void call_to(std::uint32_t entry, std::uint32_t back) { c.call(entry, back); }
    // A routine of this module (or another block's).
    void module_call(std::uint32_t entry, std::uint32_t back) {
        call(c, entry, back);
    }
    std::uint32_t rnd(std::uint32_t n, std::uint32_t back) {
        c.set(3, n);
        call_to(random_below, back);
        return c.r(3);
    }
    double widen(std::uint32_t at, std::int32_t value, bool single) {
        c.put32(frame.at(at) + 4, std::uint32_t(value) ^ 0x80000000u);
        c.put32(frame.at(at), 0x43300000);
        const double magic = c.f64(c.r(31) + 0x60);
        return single ? fpu.fsubs(c.f64(frame.at(at)), magic) : fpu.fsub(c.f64(frame.at(at)), magic);
    }
    std::uint32_t truncate(std::uint32_t at, double value, unsigned reg = 0) {
        const auto bits = fpu.fctiwz(reg, value);
        c.put64(frame.at(at), bits);
        return std::uint32_t(bits);
    }
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
    // A trait test on 0x10286470 after the call returning at `back`.
    bool has_trait(std::uint32_t player, std::uint32_t back, std::uint32_t lo_mask) {
        c.set(3, player);
        call_to(player_traits, back);
        const std::uint32_t bits = c.r(4) & lo_mask;
        // The compiler's mask test leaves the masked bits in scratch registers: r5 and r6 for the first
        // test, r4 (and zero in r5) for the others.
        c.set(3, 0);
        if (lo_mask == 0x200) { c.set(4, 0); c.set(5, bits); c.set(6, bits); }
        else { c.set(4, bits); c.set(5, 0); }
        if (lo_mask == 0x800) c.set(14, 0);  // li r14,0 of the test (0x101eb264)
        return compare(c, std::int32_t(bits)) != 0;
    }
    // 0x10289560 answers 1 for the players the checks below treat alike (extsh; cmpwi 1).
    bool kind_is_one(std::uint32_t player, std::uint32_t back, bool into_r14 = false) {
        c.set(3, player);
        call_to(player_kind, back);
        if (into_r14) c.set(14, u(std::int32_t(std::int16_t(c.r(3)))));  // extsh r14,r3
        return compare(c, std::int32_t(std::int16_t(c.r(3))), 1) == 0;
    }
    bool byte_flag(std::uint32_t entry, std::uint32_t player, std::uint32_t back) {
        c.set(3, player);
        call_to(entry, back);
        return compare(c, s8(c.r(3))) != 0;
    }
    // The action code's bit 0x100 (rlwinm 23,23 of the halfword).
    bool wide_action(bool into_r14 = false) {
        const auto code = std::int16_t(action_code(c, cand));
        if (into_r14) c.set(14, u(std::int32_t(code)));  // extsh r14,r3
        return compare(c, std::int32_t(code & 0x100)) != 0;
    }
    std::uint32_t team_of_side() const { return match + u(s8(r22) * std::int32_t(team_tactics_size)) + team_tactics; }
    std::uint32_t tactical() {
        c.set(3, team_of_side());
        return tactical_flags(c, team_of_side());
    }
    std::int32_t count_in_cell() const { return s8(c.u8(r19 + u(s8(r22)) + 0x58)); }
    void assertion(std::uint32_t at, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
        const Assertion a{.at = at, .file = c.r(28) + 0x45, .format = c.r(28) + 0x53, .title = c.r(28) + 0x60, .name = name,
                          .extension = extension, .line = line};
        report_assertion(c, a);
    }
    // The mark lands in r14 at one site (0x101eb85c), in r4 at another (0x101ebd94) and nowhere else.
    void accept_with_mark(std::uint32_t code, std::uint32_t back, unsigned mark_register = 0) {
        r23 = 1;
        r17 = code;
        enter(c, back, {match});
        c.set(3, random_mark(c));
        if (mark_register) c.set(mark_register, c.r(3));
        r16 = u(s8(c.r(3)));
        r18 = 0x2c;
    }
    // The rolled draw shared by most kinds: a random number up to weight / f31.
    std::uint32_t weighted_draw(std::uint32_t widen_site, std::uint32_t truncate_site, std::uint32_t back) {
        const double f0 = fpu.fdivs(widen(widen_site, std::int32_t(std::int16_t(r26)), true), c.f(31));
        return rnd(truncate(truncate_site, f0), back);
    }
    // status + gift of the candidate, as a double, less the pending speed figure f30 (single).
    double standing(std::uint32_t widen_site, bool single, std::uint32_t status_back, std::uint32_t gift_back);

    bool start();
    void candidate();
    void speed_factor();
    void reference_factors();
    bool kind_case();
    void case_a();
    void case_b();
    void case_c();
    void case_d();
    void case_e();
    void case_fg(bool second);
    void case_h();
    bool tail();

    Cpu& c;
    Frame& frame;
    Fpu fpu;
    std::uint32_t match, reference;
    Kept<30> cand;
    Kept<16> r16;
    Kept<17> r17;
    Kept<18> r18;
    Kept<19> r19;  // the ball's cell in the match object
    Kept<22> r22;  // the side being chosen from
    Kept<23> r23;  // the kind accepts the candidate
    Kept<26> r26;  // the weight the roll below is made with
};

bool Chooser::start() {
    c.set(3, 0);
    c.put32(frame.at(slot::best), 0);
    c.set(3, 0);
    c.put8(frame.at(slot::special), 0);
    c.set(0, 1);
    c.put8(frame.at(slot::count_b), 1);
    c.set(3, 0);
    c.put8(frame.at(slot::flag_a), 0);
    c.set(14, 0);
    c.put8(flag_pointer(), 0);
    // The side searched: the one that is not the reference's (or not the given side).
    if (compare_logical(c, reference) == 0) {
        const auto given = s8(c.u8(frame.caller() + 0x23));
        c.set(3, u(given));
        c.set(14, std::uint32_t(given == 0));
        r22 = u(s8(c.r(14)));
    } else {
        c.set(3, reference);
        call_to(0x10283690, 0x101eb004);
        c.set(0, u(s8(c.r(3))));
        c.set(4, std::uint32_t(s8(c.r(3)) == 0));
        c.set(5, c.r(4));
        r22 = u(s8(c.r(5)));
    }
    if (compare(c, state(), -1) == 0) {
        c.set(4, match + 0x10000);
        if (compare_logical(c, reference, c.u32(match + 0xe01c)) == 0) return false;
        c.set(4, match + 0x10000);
        if (compare_logical(c, reference, c.u32(match + 0xe020)) == 0) return false;
    }
    if (compare(c, s8(r22), 1) == 0) {
        c.set({{3, match}, {4, c.u8(match + ball_row_at)}, {5, c.u8(match + ball_column_at)}, {6, 4}, {7, 0xb}});
        module_call(0x101ef8d0, 0x101eb070);
    } else {
        c.set({{3, match}, {4, c.u8(match + ball_row_at)}, {5, c.u8(match + ball_column_at)}, {6, 4}, {7, 0}});
        module_call(0x101ef8d0, 0x101eb098);
    }
    c.set_f(30, fpu.frsp(fpu.fadd(D(0x90), c.f(1))));
    if (greater(c.f(30), D(0x58))) {
        const double f3 = D(0x58);
        const double f2 = D(0x90);
        const double f1 = fpu.fmul(c.f(30), D(0x90));
        c.set_f(1, f1);
        double f0 = fpu.fmul(c.f(30), D(0x90));
        f0 = fpu.fmul(f1, f0);
        f0 = fpu.multiply_add(0x101eb0cc, c.f(30), f2, f0);
        c.set_f(30, fpu.frsp(fpu.fadd(f3, f0)));
    }
    c.set(5, u(s8(c.u8(match + ball_row_at))) * 0x450);
    c.set(0, u(s8(c.u8(match + ball_column_at))) * 0x5c);
    c.set(3, c.r(5) + c.r(0));
    r19 = match + c.r(3) + 0xa08;
    c.set(0, 0);
    c.put32(frame.at(slot::index), 0);
    return true;
}

// The factor f31 the candidate's traits and the team's tactics give him.
void Chooser::speed_factor() {
    if (compare(c, state(), 4) == 0) {
        c.set_f(31, F(0x10c));
        return;
    }
    if (has_trait(cand, 0x101eb1c4, 0x200)) {
        c.set(15, tactical());
        if (compare(c, std::int32_t(c.r(15) & 0x20)) != 0) {
            c.set_f(31, F(0x54));
        } else {
            c.set(14, u(s8(r22)));
            c.set_f(31, compare(c, std::int32_t(tactical() & 0x40)) != 0 ? F(0x104) : F(0x18));
        }
    } else if (has_trait(cand, 0x101eb254, 0x800)) {
        c.set_f(31, F(0x54));
    } else {
        c.set_f(31, has_trait(cand, 0x101eb290, 0x1000) ? F(0x104) : F(0x18));
    }
}

// The factor scaled by how the candidate relates to the reference player.
void Chooser::reference_factors() {
    if (greater(c.f(30), D(0x30))) c.set_f(31, fpu.fmuls(c.f(31), F(0x110)));
    if (compare_logical(c, reference) != 0) {
        if (compare_logical(c, reference, word_1ac(c, cand)) == 0) c.set_f(31, fpu.fmuls(c.f(31), F(0x114)));
        const auto target = marking_target(c, cand);
        c.set(4, target);
        if (compare_logical(c, target, reference) == 0) c.set_f(31, fpu.fmuls(c.f(31), F(0x18)));
        if (compare(c, state(), 4) < 0) {
            c.set(0, adjustment_byte(c, cand));
            const auto first = rnd(u(0x1e - s8(c.r(0))), 0x101eb344);
            c.set(15, first);
            c.set(14, adjustment_byte(c, cand));
            const auto second = rnd(u(0x1e - s8(c.r(14))), 0x101eb360);
            c.set(14, (first - second) * 6);
            const auto pressure = pressure_adjustment(c, reference);
            c.set(0, pressure + c.r(14));
            c.set(3, u(std::int32_t(std::int16_t(c.r(0)))));
            c.put16(frame.at(slot::half_c), c.r(3));
            const double f2 = D(0x280);
            const double f1 = widen(0xb4, slot_s16(slot::half_c), false);
            c.set_f(1, f1);
            double f0 = D(0x58);
            f0 = fpu.multiply_add(0x101eb3a4, f2, f1, f0);
            c.set_f(31, fpu.fmuls(c.f(31), fpu.frsp(f0)));
        }
    }
    const auto adjustment = s8(adjustment_byte(c, cand));
    c.set(0, u(s8(c.u8(cand + 0x24))) * u(adjustment));
    const double f1 = D(0x60);
    c.set_f(1, f1);
    const double f2 = widen(0xb4, std::int32_t(c.r(0)), true);
    const double f1b = D(0x280);
    const double f0b = D(0x288);
    c.set_f(1, f1b);
    const double f2b = fpu.multiply_add(0x101eb3ec, f1b, f2, f0b);
    const double f1c = c.f32(cand + 0x78);
    c.set_f(1, f1c);
    double f0 = fpu.multiply_add(0x101eb3f8, f1c, f2b, D(0x90));
    c.set_f(31, fpu.fmuls(c.f(31), fpu.frsp(f0)));
    if (!kind_is_one(cand, 0x101eb40c)) {
        c.set_f(29, fitness(c, cand));
        const double f1d = fitness(c, cand);
        f0 = fpu.frsp(fpu.fmul(fpu.fdiv(f1d, D(0x290)), c.f(29)));
        c.set_f(31, fpu.fmuls(c.f(31), f0));
    }
    c.set(14, u(s8(r22)));
    if (compare(c, s8(c.u8(match + c.r(14) + 0x1c4)), 5) == 0) c.set_f(31, fpu.frsp(fpu.fmul(c.f(31), D(0xe0))));
    if (less(c.f(31), D(0x1b0))) c.set_f(31, F(0x110));
}

// The chance roll of the kinds below: the candidate's standing (status plus gift, less f30) beats f29.
double Chooser::standing(std::uint32_t widen_site, bool single, std::uint32_t status_back, std::uint32_t gift_back) {
    (void)status_back;
    (void)gift_back;
    const auto status = s8(status_110(c, cand));
    c.set(14, u(status));
    const auto gift = s8(trait_10d(c, cand));
    const double f0 = widen(widen_site, gift + status, single);
    return f0;
}

void Chooser::case_a() {
    if (kind_is_one(cand, 0x101eb514)) {
        bool alternate = false;
        if (compare(c, s8(row(c, cand)), 4) == 0) {
            alternate = compare(c, s8(column(c, cand))) == 0 || compare(c, s8(column(c, cand)), 0xb) == 0;
        }
        const auto grown = [&](std::uint32_t count_site, std::uint32_t gift_site, double addend, std::uint32_t site,
                               std::uint32_t truncate_site, std::uint32_t pow_back, bool magic_in_r14) {
            double f1 = widen(count_site, count_in_cell(), false);
            if (magic_in_r14) c.set(14, 0x43300000);  // lis r14,0x4330 (0x101eb578)
            const double f2 = D(0x30);
            c.set_f(1, f1);
            c.set_f(2, f2);
            enter(c, pow_back);
            f1 = power(c, f1, f2);
            c.set_f(29, f1);
            const double f1b = widen(gift_site, s8(adjustment_byte(c, cand)), false);
            c.set_f(1, f1b);
            const double f0 = fpu.multiply_add(site, f1b, c.f(29), addend);
            r26 = truncate(truncate_site, f0);
        };
        if (!alternate) grown(0xb4, 0xac, D(0x40), 0x101eb5c0, 0xa4, 0x101eb590, true);
        else grown(0xa4, 0xac, D(0x48), 0x101eb638, 0xb4, 0x101eb608, false);
    } else {
        c.put32(frame.at(slot::word_48), less(c.f(30), D(0x98)) ? 0x64 : 0xa);
        const auto draw = rnd(slot_u32(slot::word_48), 0x101eb674);
        c.set(14, draw ^ 0x80000000u);  // xoris r14,r3,0x8000
        const double f0 = widen(0xa4, std::int32_t(draw), true);
        if (less(c.f(30), f0)) {
            r23 = 1;
        } else if (wide_action()) {
            r26 = 1;
        } else if (compare_logical(c, reference) == 0) {
            r26 = 3;
        } else {
            r26 = kind_is_one(reference, 0x101eb6c8) ? 1 : 3;
        }
    }
    if (compare(c, s8(r23)) == 0) {
        c.set(15, rnd(u(std::int32_t(std::int16_t(r26))), 0x101eb6f4));
        const double f1 = F(0x18);
        c.set_f(1, f1);
        const double f0 = fpu.fmuls(f1, c.f32(cand + 0xb0));
        const auto limit = rnd(truncate(0xa4, f0), 0x101eb714);
        c.set(14, limit);
        const auto gift = s8(trait_10d(c, cand));
        c.set(3, u(gift) + limit);
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(15))) <= 0) return;
    }
    r23 = 1;
    r17 = 0x15;
    r16 = 0x2e;
    const auto draw = rnd(5, 0x101eb744);
    c.put32(frame.at(slot::word_44), compare(c, std::int32_t(draw)) != 0 ? u(-1) : 0x2f);
    r18 = u(s8(slot_u32(slot::word_44)));
}

void Chooser::case_b() {
    if (compare_logical(c, reference) != 0 && kind_is_one(reference, 0x101eb77c)) r26 = 3;
    else r26 = wide_action() ? 5 : 0xa;
    const auto draw = weighted_draw(0xa4, 0xac, 0x101eb7e8);
    c.set_f(29, widen(0xb4, std::int32_t(draw), true));
    c.set(14, u(s8(status_110(c, cand))));
    c.set(15, trait_10d(c, cand));
    const double f0 = widen(0x9c, s8(c.r(15)) + s8(c.r(14)), true);
    c.set_f(1, D(0x60));  // the int-to-double's magic number stays in f1
    if (!greater(fpu.fsubs(f0, c.f(30)), c.f(29))) return;
    accept_with_mark(0x21, 0x101eb85c, 14);
}

void Chooser::case_c() {
    if (compare_logical(c, reference) != 0 && kind_is_one(reference, 0x101ec060)) r26 = 3;
    else r26 = wide_action() ? 0x14 : 0x3c;
    const auto draw = weighted_draw(0x9c, 0xa4, 0x101ec0cc);
    c.set_f(29, widen(0xac, std::int32_t(draw), true));
    c.set(14, u(s8(status_110(c, cand))));
    const double f0 = widen(0xb4, s8(trait_10d(c, cand)) + s8(c.r(14)), true);
    c.set_f(1, D(0x60));  // the int-to-double's magic number stays in f1
    if (!greater(fpu.fsubs(f0, c.f(30)), c.f(29))) return;
    r23 = 1;
    r17 = 0xc;
    r16 = 0x32;
    r18 = 0x32;
}

void Chooser::case_d() {
    if (kind_is_one(cand, 0x101ebdac, true)) {
        c.set({{3, cand}, {4, r22}});
        call_to(0x10239d10, 0x101ebdc4);
        if (compare(c, s8(c.r(3))) == 0) {
            r23 = 1;
        } else if (compare(c, s8(c.u8(match + 0x7929))) != 0) {
            r26 = 5;
        } else {
            c.set(5, u(flag_10()));
            r26 = compare(c, flag_10()) != 0 ? 6 : 0x1e;
        }
    } else if (compare(c, flag_10()) != 0) {
        if (compare(c, s8(secondary_rating(c, cand))) != 0) r23 = 1;
        else r26 = 2;
    } else {
        const bool wide = wide_action();
        c.set(5, c.r(3));  // or r5,r3,r3 of the action code
        r26 = wide ? 2 : 4;
    }
    if (compare(c, s8(r23)) == 0) {
        const auto draw = weighted_draw(0x9c, 0xa4, 0x101ebeb0);
        c.set_f(29, widen(0xac, std::int32_t(draw), true));
        c.set(15, status_110(c, cand));
        c.set(14, u(s8(c.r(15))));
        const double f0 = widen(0xb4, s8(trait_10d(c, cand)) + s8(c.r(14)), true);
        c.set(14, 0x43300000);  // lis r14,0x4330 (0x101ebefc)
        c.set_f(1, D(0x60));  // the int-to-double's magic number stays in f1
        if (!greater(fpu.fsubs(f0, c.f(30)), c.f(29))) return;
    }
    if (compare(c, state(), 2) < 0) {
        accept_with_mark(0x22, 0x101ebf38);
        return;
    }
    r23 = 1;
    r17 = 0xc;
    r16 = 0x2e;
    r18 = 0x2e;
}

void Chooser::case_e() {
    if (compare_logical(c, reference) != 0 && kind_is_one(reference, 0x101ebc70)) r26 = 0xa;
    else r26 = wide_action() ? 0x1f4 : 0x3e8;
    enter(c, 0x101ebcc4, {match, r22, c.u8(match + ball_row_at), c.u8(match + ball_column_at)});
    const auto inside = inside_end_area(c, r22, c.u8(match + ball_row_at), c.u8(match + ball_column_at));
    if (compare(c, s8(inside)) != 0) r26 = r26 + 0x514;
    c.set(5, u(std::int32_t(std::int16_t(r26))) ^ 0x80000000u);  // xoris r5,r4,0x8000 of the int-to-double
    const auto draw = weighted_draw(0x9c, 0xa4, 0x101ebd08);
    c.set_f(29, widen(0xac, std::int32_t(draw), false));
    c.set(15, 0x43300000);  // lis r15,0x4330 (0x101ebd14)
    c.set(14, u(s8(status_110(c, cand))));
    const double f2 = widen(0xb4, s8(trait_10d(c, cand)) + s8(c.r(14)), false);
    const double f1 = c.f32(cand + 0x78);
    c.set_f(1, f1);
    double f0 = fpu.fsub(D(0x298), c.f(30));
    f0 = fpu.multiply_add(0x101ebd68, f1, f0, f2);
    if (!greater(f0, c.f(29))) return;
    c.set(5, u(state()));  // extsb r5,r0 (0x101ebd78)
    if (compare(c, state(), 4) >= 0) return;
    accept_with_mark(0x22, 0x101ebd94, 4);
}

void Chooser::case_fg(bool second) {
    // 0x11 (G) and 0x12 (F) differ in the weights and in the scratch registers they leave behind.
    const std::uint32_t weak = second ? 4 : 8, base = second ? 8 : 0xc;
    if (kind_is_one(cand, second ? 0x101eba68 : 0x101eb874)) {
        bool crowded = compare_logical(c, slot_u32(slot::best)) != 0;
        if (!crowded) {
            if (second) c.set(14, u(s8(r22)) + 0x58);  // addi r14,r3,0x58 (0x101eba84)
            crowded = count_in_cell() > 1;
        }
        if (crowded) {
            r26 = 0xc;
        } else {
            bool decided = false;
            if (second) c.set(14, u(s8(r22)));  // extsb r14,r22 (0x101ebaa0)
            if (compare(c, s8(r22)) == 0) {
                if (compare(c, s8(column(c, cand))) > 0 || compare(c, s8(row(c, cand)), 4) != 0) {
                    r23 = 1;
                    decided = true;
                }
            }
            if (!decided) {
                if (compare(c, s8(r22), 1) == 0) {
                    if (compare(c, s8(column(c, cand)), 0xb) < 0 || compare(c, s8(row(c, cand)), 4) != 0) r23 = 1;
                    else r26 = 2;
                } else {
                    r26 = 2;
                }
            }
        }
    } else if (compare(c, flag_10()) != 0) {
        r26 = 2;
    } else if (compare_logical(c, reference) != 0 && kind_is_one(reference, second ? 0x101ebb54 : 0x101eb954)) {
        r26 = 3;
    } else {
        r26 = wide_action(!second) ? weak : base;  // only kind 0x11 reads the code into r14
    }
    if (compare(c, s8(r23)) == 0) {
        if (!second) c.set(15, u(std::int32_t(std::int16_t(r26))));  // extsh r15,r26 (0x101eb998)
        const auto draw = weighted_draw(0x9c, 0xa4, second ? 0x101ebbcc : 0x101eb9cc);
        if (second) c.set(15, draw);  // or r15,r3,r3 (0x101ebbcc)
        else c.set(14, draw);         // or r14,r3,r3 (0x101eb9cc)
        const auto rating = s8(primary_rating(c, reference));
        if (!second) c.set(15, u(rating + std::int32_t(draw)) ^ 0x80000000u);  // xoris r15,r0,0x8000 (0x101eb9e4)
        c.set_f(29, widen(0xac, rating + std::int32_t(draw), false));
        if (second) c.set(5, 0x43300000);  // `lis r5,0x4330` of the int-to-double (case F only)
        const auto status = s8(status_110(c, cand));
        c.set(second ? 15 : 14, u(status));  // extsb r15,r3 (0x101ebc04) / extsb r14,r3 (0x101eba04)
        const double f1 = widen(0xb4, s8(trait_10d(c, cand)) + status, false);
        c.set_f(1, f1);
        const double f0 = fpu.multiply_add(second ? 0x101ebc38 : 0x101eba38, c.f(30), D(0x90), f1);
        if (!greater(f0, c.f(29))) return;
    }
    accept_with_mark(0x13, second ? 0x101ebc54 : 0x101eba54);
}

void Chooser::case_h() {
    if (compare_logical(c, reference) != 0 && kind_is_one(reference, 0x101ebf68)) r26 = 3;
    else r26 = wide_action() ? 0xa : 0x14;
    const auto draw = weighted_draw(0x9c, 0xa4, 0x101ebfd4);
    c.set_f(29, widen(0xac, std::int32_t(draw), true));
    c.set(14, u(s8(status_110(c, cand))));
    const double f0 = widen(0xb4, s8(trait_10d(c, cand)) + s8(c.r(14)), true);
    c.set_f(1, D(0x60));  // the int-to-double's magic number stays in f1
    if (!greater(fpu.fsubs(f0, c.f(30)), c.f(29))) return;
    accept_with_mark(0x22, 0x101ec044);
}

bool Chooser::kind_case() {
    r23 = 0;
    switch (kind()) {
    case 0x2e: case 0x25: case 0x15: case 0x38: case_a(); break;
    case 0x67: case 0xf: case 0x10: case_b(); break;
    case 0x3b: case_c(); break;
    case 0x6d: case_d(); break;
    case 0x6c: case_e(); break;
    case 0x12: case_fg(true); break;
    case 0x11: case_fg(false); break;
    case 0xc: case 0xd: case 0xe: case_h(); break;
    default:
        c.set({{3, c.toc(-0x7bb8)}, {4, c.r(28) + 0x412}, {5, u(kind())}});
        call_to(0x1077b720, 0x101ec154);
        assertion(0x101ec168, frame.at(0x5e), frame.at(0x54), 0x2eaf);
        break;
    }
    return true;
}

// The rest of the candidate's evaluation once the kind accepts him: the chance he gets, and the
// result kept if it is the best so far. False when the candidate is dropped.
bool Chooser::tail() {
    if (compare(c, s8(r23)) == 0) return false;
    if (compare_logical(c, reference) != 0 && compare(c, slot_s8(slot::flag_a)) == 0 && compare(c, kind(), 0x3b) != 0 &&
        compare(c, kind(), 0x15) != 0) {
        c.set({{3, match}, {4, reference}});
        module_call(0x101ee770, 0x101ec1f4);
        c.put8(frame.at(slot::special), c.r(3));
        c.set(3, reference);
        call_to(0x10283690, 0x101ec200);
        c.set(14, c.r(3));
        c.set(4, column(c, reference));
        c.set({{3, match}, {5, c.r(14)}, {6, 0}});
        module_call(0x101ee680, 0x101ec220);
        c.put8(frame.at(slot::count_b), c.r(3));
        c.set(0, 1);
        c.put8(frame.at(slot::flag_a), 1);
    }
    if (compare(c, slot_s8(slot::special)) != 0) {
        c.set(0, 1);
        c.put8(flag_pointer(), 1);
    } else {
        bool chance = false;
        c.set({{3, match}, {4, cand}});
        module_call(0x101f0dd0, 0x101ec254);
        c.set(15, u(s8(c.r(3))));
        if (compare(c, s8(c.r(15))) != 0) {
            c.set(14, rnd(10, 0x101ec268));
            chance = compare(c, std::int32_t(c.r(14))) == 0;
        }
        if (!chance) {
            enter(c, 0x101ec280, {match, cand});
            const auto value = std::int16_t(position_value_a(c, match, cand));
            if (compare(c, value, 1) == 0)
                chance = compare(c, std::int32_t(rnd(2, 0x101ec294))) == 0;
        }
        if (chance && compare(c, slot_s8(slot::count_b)) == 0) {
            if (!byte_flag(0x102720d0, cand, 0x101ec2b4) || !byte_flag(0x10272050, cand, 0x101ec2c8)) {
                c.set(3, 1);
                c.put8(flag_pointer(), 1);
            }
        }
    }
    c.set({{3, cand}, {4, reference}});
    call_to(0x102875f0, 0x101ec2e8);
    c.set(3, u(s8(c.r(3))));
    const auto roll = rnd(c.r(3), 0x101ec2f0);
    c.set(0, u(std::int32_t(std::int16_t(roll))));
    c.put16(frame.at(slot::half_40), c.r(0));
    if (compare_logical(c, reference) != 0 && !byte_flag(0x10272100, reference, 0x101ec308) &&
        compare(c, s8(c.u8(flag_pointer()))) == 0) {
        c.set(14, u(s8(c.u8(cand + 0x24))));
        if (compare(c, s8(c.r(14))) == 0) {
            c.set(15, rnd(0xc, 0x101ec33c));
            const double f0 = c.f32(cand + 0x78);
            const auto scaled = truncate(0x9c, f0);
            const auto drawn = rnd(scaled, 0x101ec354);
            if (compare(c, std::int32_t(drawn), std::int32_t(c.r(15))) > 0) {
                bool take = false;
                const auto base = rnd(1000, 0x101ec364);
                c.set(0, base + 0x1d4c);
                c.set(3, u(std::int32_t(c.i16(reference + 0x36))));
                if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0) {
                    take = true;
                } else {
                    c.set(14, rnd(10000, 0x101ec37c));
                    c.set({{3, c.u32(reference + 0x6c)}, {4, c.u32(cand + 0x6c)}, {5, 0}});
                    call_to(0x100cdef0, 0x101ec390);
                    c.set(0, u(std::int32_t(std::int16_t(c.r(3)))));
                    take = compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(14))) > 0;
                }
                if (take) {
                    const auto extra = rnd(10, 0x101ec3a4);
                    c.set(3, u(std::int32_t(std::int16_t(extra))));
                    c.set(0, u(slot_s16(slot::half_40)) + c.r(3));
                    c.put16(frame.at(slot::half_40), c.r(0));
                    c.set(3, cand);
                    if (compare(c, s8(adjustment_byte(c, cand))) != 0) r18 = u(-1);
                }
            }
        }
    }
    bool kept_kind = false;
    if (compare(c, kind(), 0x15) == 0) {
        c.set({{3, cand}, {4, r22}});
        call_to(0x10239d10, 0x101ec3e8);
        c.set(4, u(s8(c.r(3))) * 0x5dc);
        c.set(3, c.u32(cand + 0x6c));
        c.set(0, u(s8(c.u8(c.r(3) + 0x68))) * 0xc8);
        c.set(3, c.r(0) + c.r(4) + 0xfa);
        c.set(3, rnd(c.r(3), 0x101ec40c));
        if (compare(c, std::int32_t(c.r(3))) == 0) {
            c.put8(frame.at(slot::result), 0x25);
            kept_kind = true;
        }
    }
    if (!kept_kind) {
        c.set({{3, cand}, {4, r22}});
        call_to(0x10239d10, 0x101ec42c);
        c.set(14, u(s8(c.r(3))) * 0x15e);
        c.set(15, u(s8(adjustment_byte(c, cand))) * c.r(14));
        const auto adjustment = s8(adjustment_byte(c, cand));
        c.set(14, c.u8(flag_pointer()));
        c.set(0, u(s8(c.r(14))) * 0x1f4 * u(adjustment));
        c.set(15, c.r(15) + c.r(0));
        c.set(14, u(s8(adjustment_byte(c, cand))) + 5);
        c.set(0, u(s8(c.u8(cand + 0x24))) * 0x19 * c.r(14));
        c.set(3, c.r(15) + c.r(0) + 0x14);
        const auto limit = rnd(c.r(3), 0x101ec490);
        if (compare(c, slot_s16(slot::half_40), std::int32_t(limit)) > 0) {
            c.put8(frame.at(slot::result), r16);
        } else {
            bool done = false;
            if (compare(c, s8(r18), -1) > 0) {
                c.set(14, u(s8(adjustment_byte(c, cand))) * 0x1f4 + 0x1388);
                c.set({{3, cand}, {4, r22}});
                call_to(0x10239d10, 0x101ec4d0);
                c.set(14, u(s8(c.r(3))) * c.r(14));
                const auto adjustment_b = s8(adjustment_byte(c, cand));
                const auto stand = s8(c.u8(flag_pointer()));
                c.set(4, c.u32(cand + 0x6c));
                c.set(0, u(s8(c.u8(c.r(4) + 0x68))) * 0x50 + 0x1f40);
                c.set(0, u(stand) * c.r(0) * u(adjustment_b));
                c.set(3, c.r(14) + c.r(0) + 0x1388);
                c.set(4, rnd(c.r(3), 0x101ec514));
                c.set(3, u(slot_s16(slot::half_40)));
                c.set(5, c.r(3));  // lha r5,0x40(r1) (0x101ec51c)
                c.set(0, c.r(3) * c.r(3));
                if (compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(4))) > 0) {
                    c.put8(frame.at(slot::result), r18);
                    done = true;
                }
            }
            if (!done) {
                if (compare(c, s8(c.u8(flag_pointer()))) != 0 && !kind_is_one(cand, 0x101ec54c) && !less(c.f(30), D(0x98))) {
                    c.set(15, c.u8(match + shot_state_at));
                    if (compare(c, s8(c.r(15)), 4) != 0 && compare(c, s8(adjustment_byte(c, cand))) != 0) return false;
                }
                c.put8(frame.at(slot::result), r17);
            }
        }
    }
    // The candidate's chosen result and its figure.
    c.set({{3, cand}, {4, frame.at(slot::result)}, {5, c.u32(frame.caller() + 0x28)}, {6, c.u32(frame.caller() + 0x2c)}});
    call_to(0x10270c70, 0x101ec5a4);
    c.set(5, u(std::int32_t(std::int16_t(c.r(3)))));
    c.set(15, u(std::int32_t(std::int16_t(rnd(c.r(5), 0x101ec5b0)))));
    c.put16(frame.at(slot::half_e), c.r(15));
    if (compare(c, kind(), 0x11) == 0 && compare(c, s8(c.u8(frame.at(slot::result))), 0x29) != 0) {
        const auto rating = s8(primary_rating(c, cand));
        const double f1 = widen(0x9c, rating, true);
        c.set_f(1, f1);
        double f0 = fpu.fmuls(c.f32(cand + 0x74), f1);
        const auto bonus = truncate(0xa4, f0);
        const auto extra = rnd(bonus, 0x101ec61c);
        c.set(6, u(std::int32_t(std::int16_t(extra))));
        c.set(0, u(slot_s16(slot::half_e)) + c.r(6));
        c.put16(frame.at(slot::half_e), c.r(0));
    } else if (compare(c, kind(), 0x12) == 0 && compare(c, s8(c.u8(frame.at(slot::result))), 0x29) != 0) {
        c.set(15, u(s8(primary_rating(c, cand))));
        c.set(14, u(s8(primary_rating(c, cand))));
        c.set(3, u(std::int32_t(std::int16_t(c.r(14) * c.r(15)))));
        c.set(4, u(slot_s16(slot::half_e)) + c.r(3));
        c.put16(frame.at(slot::half_e), c.r(4));
    }
    if (compare(c, slot_s16(slot::half_e)) <= 0) {
        c.set({{3, cand}, {4, c.u8(cand + 0x27)}});
        call_to(0x10239d10, 0x101ec698);
        c.set(5, u(s8(c.r(3))));
        if (compare(c, s8(c.r(5))) == 0) {
            c.set(3, cand);
            if (compare(c, s8(adjustment_byte(c, cand))) != 0) return false;
            c.set(14, rnd(100, 0x101ec6c8));
            if (compare(c, s8(status_110(c, cand)), std::int32_t(c.r(14))) <= 0) return false;
        }
    }
    if (compare_logical(c, slot_u32(slot::best)) != 0) {
        c.set(4, u(slot_s16(slot::half_e)));
        c.set(3, c.r(4));
        c.set(5, c.u32(frame.caller() + 0x30));
        c.set(0, u(std::int32_t(c.i16(c.r(5)))));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) <= 0) return false;
    }
    c.set(4, slot_u8(slot::result));
    c.set(3, c.u32(frame.caller() + 0x34));
    c.put8(c.r(3), c.r(4));
    c.put32(frame.at(slot::best), cand);
    c.set(0, u(slot_s16(slot::half_e)));
    c.set(5, c.u32(frame.caller() + 0x30));
    c.put16(c.r(5), c.r(0));
    return true;
}

void Chooser::candidate() {
    c.set(15, u(s8(r22)));
    c.set(3, c.r(15) * 0x2c);
    c.set(0, slot_u32(slot::index) << 2);
    c.set(3, c.r(3) + c.r(0));
    cand = c.u32(r19 + c.r(3));
    if (compare_logical(c, cand) == 0) {
        assertion(0x101eb13c, frame.at(0x7d), frame.at(0x59), 0x2d4a);
        c.set(15, 0);
        c.set(3, u(-1));
        call_to(0x1076f7d0, 0x101eb180);
    }
    if (compare(c, s8(position(c, cand))) < 0) return;
    if (compare(c, s8(c.u8(cand + 0x2c)), 2) >= 0) return;
    speed_factor();
    reference_factors();
    kind_case();
    tail();
}

std::uint32_t Chooser::run() {
    if (!start()) return result(c, slot_u32(slot::best));
    for (;;) {
        c.set(3, slot_u32(slot::index));
        c.set(4, u(s8(r22)));
        c.set(0, u(s8(c.u8(r19 + c.r(4) + 0x58))));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) >= 0) break;
        candidate();
        c.put32(frame.at(slot::index), slot_u32(slot::index) + 1);
    }
    if (compare_logical(c, slot_u32(slot::best)) != 0) {
        c.set(3, slot_u32(slot::best));
        c.set_f(1, F(0x118));
        c.set(4, c.u32(frame.caller() + 0x34));
        c.set(5, u(s8(c.u8(c.r(4)))));
        module_call(0x10239590, 0x101ec770);
    }
    return result(c, slot_u32(slot::best));
}
}  // namespace

std::uint32_t best_player_in_ball_cell(Cpu& c, std::uint32_t match, std::uint32_t reference, std::uint32_t side,
                                       std::uint32_t kind, std::uint32_t arg_7, std::uint32_t arg_8,
                                       std::uint32_t out_score, std::uint32_t out_kind) {
    Frame frame(c, 0x101eaf60, 0x130, {.first_gpr = 14, .gpr_offset = -0x68, .first_fpr = 29});
    c.put8(frame.caller() + 0x23, side);
    c.put8(frame.caller() + 0x27, kind);
    c.put32(frame.caller() + 0x28, arg_7);
    c.put32(frame.caller() + 0x2c, arg_8);
    c.put32(frame.caller() + 0x30, out_score);
    c.put32(frame.caller() + 0x34, out_kind);
    keep(c, {{27, match}, {25, reference}, {20, c.u32(frame.caller() + 0x38)}, {21, c.toc(-0x7bac)}, {24, c.toc(-0x7ba8)},
             {28, c.toc(-0x52cc)}, {29, c.toc(-0x52c8)}, {31, c.toc(-0x52c4)}});
    Chooser chooser(c, frame, match, reference);
    return chooser.run();
}
}  // namespace cm::aot::match_eng

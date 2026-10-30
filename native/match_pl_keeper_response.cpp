// Keeper response sampling shares one activation across geometric and RNG gates.
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::detail {
using namespace guest;

class KeeperResponse {
public:
    KeeperResponse(Cpu&, std::uint32_t player, std::uint32_t column,
                   std::uint32_t row, std::uint32_t alternate_skill,
                   std::uint32_t action, std::uint32_t reason,
                   std::uint32_t resampled);
    std::uint32_t run();
private:
    Cpu& c;
    Frame frame;
    Fpu fp;
    Player actor;
    const std::uint32_t doubles, seed, begin, end, cursor;
    std::uint32_t at(unsigned offset) const { return frame.at(offset); }
    std::uint32_t player() const { return actor.address; }
    std::int8_t kind() const { return std::int8_t(c.r(24)); }
    static std::int32_t high(std::uint32_t word, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(word)) * multiplier) >> 32);
    }
    std::uint32_t shifted_sign(std::int32_t word, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, word < 0 && (std::uint32_t(word) & ((1u << bits) - 1)));
        return unsigned(word < 0);
    }
    void advance(std::uint32_t back);
    bool measure_distance();
    void initial_sample(std::int32_t bound, unsigned output, std::uint32_t back, bool second);
    void select_reaction();
    void trait_bonus();
    void attribute_draw();
    void waiting_draw();
    std::uint32_t rating(bool resample);
};

KeeperResponse::KeeperResponse(Cpu& cpu, std::uint32_t address, std::uint32_t column,
    std::uint32_t row, std::uint32_t alternate_skill, std::uint32_t action,
    std::uint32_t reason, std::uint32_t resampled)
    : c(cpu), frame(c, 0x10251270, 0xe0, {.first_gpr = 18, .gpr_offset = -0x48, .first_fpr = 31}),
      fp(c), actor{c, address}, doubles(c.toc(-0x51e8)), seed(c.toc(-0x7b90)),
      begin(c.toc(-0x7b8c)), end(c.toc(-0x7b88)), cursor(c.toc(-0x7b84)) {
    keep(c, {{25, reason}, {19, resampled}, {29, address}, {20, column},
             {23, row}, {24, action}, {30, doubles}, {27, seed},
             {22, begin}, {21, end}, {28, cursor}});
    // Both bytes precede the skill reads, including aliases with the record.
    c.put8(reason, 0); c.set(8, 0); compare(c, std::int8_t(alternate_skill));
    c.put8(resampled, 0);
    c.set(26, std::uint32_t(std::int32_t(c.i16(address + (std::int8_t(alternate_skill) ? 0x16e : 0x16c)))));
    c.set(31, std::uint32_t(std::int32_t(std::int16_t(c.r(26) + 220))));
}

void KeeperResponse::advance(std::uint32_t back) {
    const auto next = c.u32(cursor) + 2; c.set(3, next);
    const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
    if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
}

bool KeeperResponse::measure_distance() {
    const auto first_match = actor.match(), second_match = actor.match();
    c.set(4, second_match);
    const auto ball_column = c.i8(first_match + 0x791d), actor_column = actor.column();
    c.set(18, c.u8(second_match + 0x791e));
    c.set(3, std::uint32_t(int(actor_column) - ball_column));
    c.call(0x10775540, 0x10251300);
    const auto row = actor.row(); c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(18)))));
    c.set(18, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
    c.set(3, std::uint32_t(int(row) - std::int32_t(c.r(4)))); c.call(0x10775540, 0x10251318);
    c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(18)))));
    c.set(5, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
    const auto delay = c.i8(player() + 0x2c);
    c.set(3, c.r(4) * 4); c.set(4, c.r(5) * 36); c.set(5, c.toc(-0x6424));
    const bool busy = compare(c, delay, 1) > 0;
    c.set_f(31, c.f32(c.r(5) + c.r(4) + c.r(3))); return busy;
}

void KeeperResponse::initial_sample(std::int32_t bound, unsigned output, std::uint32_t back, bool second) {
    if (compare(c, bound) == 0) { c.set(output, 0); return; }
    advance(back); c.set(3, 65535); compare(c, bound, 65535);
    c.set(3, std::uint32_t(-65535)); compare(c, bound, -65535);
    // The initial signed16 score and its oriented residual keep both bounds
    // within [-65535,65535]. No expanded wide sampler can run here.
    c.set(5, c.u32(cursor)); c.set(3, std::uint32_t(bound)); c.set(4, c.u32(seed));
    compare(c, bound); const auto pool = c.u16(c.r(5));
    if (second) c.set(5, pool);
    c.set(4, std::uint32_t(pool) + c.r(4));
    const auto residual = c.r(4) - std::uint32_t(divide_signed<4>(c.r(4), std::uint32_t(bound))) * std::uint32_t(bound);
    c.set(output, bound > 0 ? residual : 0u - residual);
}

void KeeperResponse::trait_bonus() {
    c.set_f(0, c.f64(doubles + 0x278)); fp.fcmp(0, c.f(31), c.f(0));
    if (!(c.cr(0) & 4)) return;
    c.set(3, std::uint32_t(std::int32_t(c.i16(player() + 0x3c))));
    c.put32(at(0x78), 0x43300000); c.put32(at(0x7c), c.r(3) ^ 0x80000000);
    c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + 0x170));
    c.set_f(0, c.f64(at(0x78))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.put64(at(0x80), fp.fctiwz(0, c.f(0)));
    c.set(31, c.r(31) + c.u32(at(0x84)) + 10);
}

void KeeperResponse::select_reaction() {
    if (compare(c, kind(), 51) == 0) {
        initial_sample(std::int16_t(c.r(31)), 20, 0x1025139c, false);
        initial_sample(std::int32_t(std::uint32_t(std::int32_t(std::int16_t(c.r(31)))) - c.r(20)), 23, 0x10251528, true);
        c.set(31, std::uint32_t(std::int32_t(std::int16_t(c.r(23))))); return;
    }
    if (compare(c, actor.side(), 1) == 0) {
        c.set(3, std::uint32_t(std::int32_t(actor.row())));
        if (compare(c, std::int32_t(c.r(3)), std::int8_t(c.r(23))) < 0) { c.put8(c.r(25), 1); c.set(31, 0); return; }
    }
    if (compare(c, std::int8_t(actor.side())) == 0) {
        c.set(3, std::uint32_t(std::int32_t(actor.row())));
        if (compare(c, std::int32_t(c.r(3)), std::int8_t(c.r(23))) > 0) { c.put8(c.r(25), 1); c.set(31, 0); return; }
    }
    const bool upper = compare(c, actor.side(), 1) == 0; c.set(3, 0);
    bool area = compare(c, actor.column(), 2) >= 0;
    if (area) area = compare(c, actor.column(), 6) <= 0;
    if (area) area = upper ? compare(c, actor.row(), 10) >= 0 : compare(c, actor.row()) >= 0;
    if (area) area = compare(c, actor.row(), upper ? 11 : 1) <= 0;
    if (area) c.set(3, 1);
    if (compare(c, int(area)) == 0) { c.put8(c.r(25), 3); c.set(31, 0); return; }
    enter(c, 0x102517d4, {player(), c.r(20), c.r(23)}); can_cover_goal(c, player(), c.r(20), c.r(23));
    if (compare(c, std::int8_t(c.r(3))) == 0) {
        c.put8(c.r(25), 2); enter(c, 0x102517f0, {player(), c.r(31)}); sample_decision(c, player(), c.r(31));
        c.set(31, std::uint32_t(std::int32_t(std::int16_t(c.r(3) - 200)))); return;
    }
    bool goal_line = compare(c, c.u8(player() + 0x106), 11) == 0;
    if (!goal_line) goal_line = compare(c, actor.row()) == 0;
    std::uint32_t back = goal_line ? 0x102519ac : 0x102518e4;
    auto score = c.r(31); unsigned bonus = 0; bool close_bonus = false, ordinary = false;
    if (goal_line) {
        if (compare(c, kind(), 27) == 0) { back = 0x10251984; bonus = 125; close_bonus = true; }
        else if (kind() < 27 && compare(c, kind(), 26) >= 0) { back = 0x1025195c; bonus = 75; close_bonus = true; }
        else ordinary = true;
    } else {
        const auto order = compare(c, kind(), 5);
        if (order == 0) { back = 0x10251870; score = std::uint32_t(std::int32_t(std::int16_t(score - 150))); }
        else if (order < 0 && compare(c, kind(), 3) == 0) { back = 0x10251858; score = std::uint32_t(std::int32_t(std::int16_t(score - 150))); }
        else if (order > 0) {
            const auto high_kind = compare(c, kind(), 27);
            if (high_kind == 0) { back = 0x102518c0; score = std::uint32_t(std::int32_t(std::int16_t(score - 200))); close_bonus = true; }
            else if (high_kind < 0 && compare(c, kind(), 26) >= 0) { back = 0x10251890; score = std::uint32_t(std::int32_t(std::int16_t(score - 150))); close_bonus = true; }
            else ordinary = true;
        } else ordinary = true;
        if (close_bonus) c.put8(c.r(25), 4);
    }
    enter(c, back, {player(), score}); sample_decision(c, player(), score);
    c.set(31, std::uint32_t(std::int32_t(std::int16_t(c.r(3) + bonus))));
    if (close_bonus) {
        c.set_f(0, c.f64(doubles + 0x10)); fp.fcmp(0, c.f(31), c.f(0));
        if (c.cr(0) & 8) c.set(31, c.r(31) + 100);
    } else if (ordinary) trait_bonus();
}
void KeeperResponse::attribute_draw() {
    c.set(4, c.toc(-0x51e0)); c.set(3, c.u8(player() + 0xf));
    c.set_f(1, c.f32(c.r(4) + 0x2c)); c.set_f(0, c.f32(player() + 0xd4));
    c.put32(at(0x7c), c.r(3)); c.set_f(2, fp.fmuls(c.f(1), c.f(0)));
    c.set_f(3, c.f64(doubles + 0xb8)); c.put32(at(0x78), 0x43300000);
    c.set_f(1, c.f64(doubles + 0xf0)); c.set_f(0, c.f64(at(0x78)));
    c.set_f(2, fp.fadd(c.f(3), c.f(2))); c.set(3, actor.match());
    c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set(20, c.u8(c.r(3) + 0x7920));
    c.set_f(0, fp.fadd(c.f(2), c.f(0))); c.put64(at(0x80), fp.fctiwz(0, c.f(0)));
    c.set(24, c.u32(at(0x84))); const auto bound = std::int32_t(c.r(24));
    if (compare(c, bound) == 0) { c.set(23, 0); return; }
    advance(0x10251a74); c.set(3, 65535); bool narrow = compare(c, bound, 65535) <= 0;
    if (narrow) { c.set(3, std::uint32_t(-65535)); narrow = compare(c, bound, -65535) >= 0; }
    if (narrow) {
        c.set(3, c.u32(cursor)); compare(c, bound);
        c.set(3, std::uint32_t(c.u16(c.r(3))) + c.u32(seed));
        const auto residual = c.r(3) - std::uint32_t(divide_signed<4>(c.r(3), c.r(24))) * c.r(24);
        c.set(23, bound > 0 ? residual : 0u - residual); return;
    }
    c.put32(at(0x7c), c.r(24) ^ 0x80000000); c.put32(at(0x78), 0x43300000);
    c.set(7, c.u32(cursor)); c.set(6, 0x80008001); c.set(4, 0x43300000); c.set(5, 65535); c.set(3, 0x80008001);
    c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, c.f64(at(0x78))); c.set_f(0, c.f64(doubles));
    c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set(8, c.u16(c.r(7))); c.set(7, c.u32(seed));
    c.put32(at(0x80), c.r(4)); c.set_f(2, fp.fdiv(c.f(1), c.f(0)));
    c.set(7, c.r(8) + c.r(7)); c.set_f(1, c.f64(doubles + 0x80));
    const auto word = c.r(7), numerator = c.r(24) * 10;
    const auto product = std::int32_t(std::uint32_t(high(word, std::int32_t(0x80008001))) + word);
    c.set(6, shifted_sign(product, 15));
    c.set(23, word - std::uint32_t(std::int32_t(word) / 65535) * 65535u);
    c.set(4, c.r(23) ^ 0x80000000); c.put32(at(0x84), c.r(4));
    c.set_f(0, c.f64(at(0x80))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    shifted_sign(std::int32_t(std::uint32_t(high(numerator, std::int32_t(0x80008001))) + numerator), 15);
    c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.set(3, std::uint32_t(std::int32_t(numerator) / 65535));
    c.put64(at(0x88), fp.fctiwz(0, c.f(0))); c.set(23, c.u32(at(0x8c)));
    enter(c, 0x10251b4c, {c.r(3)}); sample_range(c, c.r(3));
    const auto correction = c.r(3) + 5;
    const auto correction_high = high(correction, 0x66666667);
    c.set(3, std::uint32_t(std::int32_t(correction) / 10)); c.set(4, shifted_sign(correction_high, 2));
    c.set(23, c.r(23) + c.r(3));
    if (compare(c, std::int32_t(c.r(23)), std::int32_t(c.r(24) - 1)) > 0) c.set(23, c.r(24) - 1);
}
void KeeperResponse::waiting_draw() {
    const auto wait_word = [&] { return std::uint32_t(std::int32_t(std::int8_t(c.r(20)))) ^ 0x80000000; };
    c.set(3, wait_word()); c.put32(at(0x7c), c.r(3)); c.put32(at(0x78), 0x43300000);
    c.set_f(0, c.f64(doubles + 0xc8)); c.set_f(2, c.f64(doubles + 0x80));
    c.set_f(3, fp.fsub(c.f(0), c.f(31))); c.set_f(4, c.f64(doubles + 0xb8));
    c.set_f(1, c.f64(at(0x78))); c.set_f(0, c.f64(doubles + 0x88));
    c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set_f(1, fp.fmul(c.f(3), c.f(1)));
    c.set_f(1, fp.fmul(c.f(4), c.f(1))); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 2) { c.set(21, 0); return; }
    advance(0x10251c18);
    const auto check = [&](std::int32_t limit) {
        c.set(3, wait_word()); c.put32(at(0x7c), c.r(3));
        c.set_f(0, c.f64(doubles + 0xc8)); c.set_f(1, c.f64(doubles + 0x80));
        c.set_f(2, fp.fsub(c.f(0), c.f(31))); c.put32(at(0x78), 0x43300000); c.set(3, std::uint32_t(limit));
        c.set_f(3, c.f64(doubles + 0xb8)); c.set_f(0, c.f64(at(0x78)));
        c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
        c.set_f(0, fp.fmul(c.f(3), c.f(0))); c.put64(at(0x80), fp.fctiwz(0, c.f(0)));
        return compare(c, c.i32(at(0x84)), limit);
    };
    bool narrow = check(65535) <= 0; if (narrow) narrow = check(-65535) >= 0;
    if (narrow) {
        c.put32(at(0x3c), wait_word()); c.set(4, 0x43300000);
        c.set_f(0, c.f64(doubles + 0xc8)); c.put32(at(0x38), c.r(4));
        c.set_f(2, c.f64(doubles + 0x80)); c.set_f(3, fp.fsub(c.f(0), c.f(31)));
        c.set_f(1, c.f64(at(0x38))); c.set_f(0, c.f64(doubles + 0xc8));
        c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set_f(4, c.f64(doubles + 0xb8));
        c.set(5, c.u16(c.u32(cursor))); c.set_f(2, fp.fsub(c.f(0), c.f(31))); c.set(4, c.u32(seed));
        c.set_f(0, fp.fmul(c.f(3), c.f(1))); c.set(5, c.r(5) + c.r(4));
        c.put32(at(0x4c), wait_word()); c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x48), 0x43300000);
        c.set_f(4, fp.fmul(c.f(4), c.f(0))); c.set_f(0, c.f64(at(0x48))); c.set_f(3, c.f64(doubles + 0xb8));
        c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.put64(at(0x40), fp.fctiwz(1, c.f(4)));
        c.set(4, c.u32(at(0x44))); c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.set_f(0, fp.fmul(c.f(3), c.f(0)));
        // A nonzero floating bound may truncate to zero. Its quotient is
        // multiplied by zero and overwritten before any call or return.
        const auto quotient = c.r(4) ? std::uint32_t(divide_signed<4>(c.r(5), c.r(4))) : 0u;
        c.set(3, quotient); c.put64(at(0x50), fp.fctiwz(0, c.f(0)));
        const auto sign = compare(c, c.i32(at(0x54)));
        const auto residual = c.r(5) - quotient * c.r(4);
        c.set(21, sign > 0 ? residual : 0u - residual); return;
    }
    c.put32(at(0x7c), wait_word()); c.set(3, 0x43300000); c.put32(at(0x78), c.r(3));
    c.set(9, c.u32(cursor)); c.set(6, 0x43300000); c.put32(at(0x88), c.r(6));
    c.set_f(0, c.f64(doubles + 0xc8)); c.set_f(3, fp.fsub(c.f(0), c.f(31)));
    c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, c.f64(at(0x78)));
    c.set_f(0, c.f64(doubles + 0xc8)); c.set(8, 0x80008001); c.set(7, 65535); c.set(5, 0x43300000); c.set(4, 0x80008001);
    c.set_f(6, c.f64(doubles + 0xb8)); c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    c.set_f(1, fp.fmul(c.f(3), c.f(1))); c.put32(at(0x64), wait_word());
    c.set_f(5, c.f64(doubles + 0x80)); c.set_f(2, fp.fsub(c.f(0), c.f(31)));
    c.put32(at(0x60), 0x43300000); c.set_f(0, fp.fmul(c.f(6), c.f(1)));
    c.set_f(1, c.f64(doubles + 0x80)); c.set_f(4, c.f64(doubles));
    c.set(9, c.u16(c.r(9))); const auto first = fp.fctiwz(0, c.f(0)); c.set(6, c.u32(seed));
    c.put32(at(0x70), c.r(5)); c.set(6, c.r(9) + c.r(6)); c.put64(at(0x80), first);
    const auto word = c.r(6); const auto product = std::int32_t(std::uint32_t(high(word, std::int32_t(0x80008001))) + word);
    c.set_f(0, c.f64(at(0x60))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set_f(1, c.f64(doubles + 0xb8)); c.put32(at(0x8c), c.u32(at(0x84)) ^ 0x80000000);
    c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.set(3, shifted_sign(product, 15));
    c.set_f(3, c.f64(at(0x88))); c.set_f(2, fp.fsub(c.f(3), c.f(5)));
    c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.set_f(3, c.f64(doubles + 0x80));
    c.set_f(4, fp.fdiv(c.f(2), c.f(4)));
    c.set(21, word - std::uint32_t(std::int32_t(word) / 65535) * 65535u); c.put32(at(0x74), c.r(21) ^ 0x80000000);
    c.set_f(2, c.f64(at(0x70))); const auto second = fp.fctiwz(0, c.f(0));
    c.set_f(1, fp.fsub(c.f(2), c.f(3))); c.put64(at(0x58), second);
    c.set_f(1, fp.fmul(c.f(4), c.f(1))); const auto numerator = c.u32(at(0x5c)) * 10;
    c.put64(at(0x68), fp.fctiwz(0, c.f(1))); c.set(21, c.u32(at(0x6c)));
    shifted_sign(std::int32_t(std::uint32_t(high(numerator, std::int32_t(0x80008001))) + numerator), 15);
    c.set(3, std::uint32_t(std::int32_t(numerator) / 65535));
    enter(c, 0x10251df0, {c.r(3)}); sample_range(c, c.r(3));
    const auto correction = c.r(3) + 5;
    c.put32(at(0x54), wait_word()); c.set_f(0, c.f64(doubles + 0xc8)); c.set(4, 0x66666667);
    c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, fp.fsub(c.f(0), c.f(31))); c.put32(at(0x50), 0x43300000);
    c.set_f(3, c.f64(doubles + 0xb8)); c.set_f(0, c.f64(at(0x50)));
    c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    c.set(3, shifted_sign(high(correction, 0x66666667), 2)); c.set_f(0, fp.fmul(c.f(3), c.f(0)));
    c.set(21, c.r(21) + std::uint32_t(std::int32_t(correction) / 10)); c.put64(at(0x48), fp.fctiwz(0, c.f(0)));
    c.set(3, c.u32(at(0x4c)));
    if (compare(c, std::int32_t(c.r(21)), std::int32_t(c.r(3) - 1)) > 0) {
        c.put32(at(0x44), wait_word()); c.set_f(0, c.f64(doubles + 0xc8)); c.set_f(1, c.f64(doubles + 0x80));
        c.set_f(2, fp.fsub(c.f(0), c.f(31))); c.put32(at(0x40), 0x43300000);
        c.set_f(3, c.f64(doubles + 0xb8)); c.set_f(0, c.f64(at(0x40)));
        c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
        c.set_f(0, fp.fmul(c.f(3), c.f(0))); c.put64(at(0x38), fp.fctiwz(0, c.f(0)));
        c.set(3, c.u32(at(0x3c))); c.set(21, c.r(3) - 1);
    }
}

std::uint32_t KeeperResponse::rating(bool resample) {
    if (resample) {
        c.put8(c.r(19), 1); enter(c, 0x10251fe8, {player(), c.r(31)}); sample_decision(c, player(), c.r(31));
    }
    const auto score = resample ? std::int16_t(c.r(3)) : std::int16_t(c.r(31));
    c.put32(at(0x3c), std::uint32_t(std::int32_t(score)) ^ 0x80000000); c.set(5, 0x43300000);
    c.put32(at(0x38), c.r(5)); c.set_f(2, c.f64(doubles + 0x80));
    c.set(3, std::uint32_t(std::int32_t(std::int16_t(c.r(26))))); c.set(4, 0x2aaaaaab);
    const auto product = high(c.r(3), 0x2aaaaaab); c.set(3, std::uint32_t(product));
    c.set_f(0, c.f64(doubles + 0x118)); c.set_f(1, c.f64(at(0x38))); c.put32(at(0x40), 0x43300000);
    const auto skill = std::int32_t(std::int16_t(c.r(26))) / 6;
    c.put32(at(0x44), std::uint32_t(skill) ^ 0x80000000);
    if (resample) {
        c.set_f(3, fp.fsub(c.f(1), c.f(2))); c.set_f(2, c.f64(doubles + 0x40)); c.set_f(1, c.f64(doubles + 0x80));
        c.set_f(3, fp.fdiv(c.f(3), c.f(0))); c.set_f(0, c.f64(at(0x40)));
        c.set_f(2, fp.fadd(c.f(2), c.f(3))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
        c.set_f(0, fp.fadd(c.f(0), c.f(2)));
    } else {
        c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set_f(3, c.f64(doubles + 0x40));
        c.set_f(2, fp.fdiv(c.f(1), c.f(0))); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(at(0x40)));
        c.set_f(2, fp.fadd(c.f(3), c.f(2))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
        c.set_f(0, fp.fadd(c.f(2), c.f(0)));
    }
    c.put64(at(0x48), fp.fctiwz(0, c.f(0))); return result(c, c.u32(at(0x4c)));
}

std::uint32_t KeeperResponse::run() {
    if (measure_distance()) return result(c, 0);
    enter(c, 0x1025134c, {player()}); current_role(c, player());
    if (compare(c, std::int16_t(c.r(3)), 1) != 0) return result(c, 0);
    select_reaction(); attribute_draw(); waiting_draw();
    return rating(compare(c, std::int32_t(c.r(23)), std::int32_t(c.r(21))) <= 0);
}
}

namespace cm::aot::match_pl {
std::uint32_t sample_keeper_response(Cpu& c, std::uint32_t player, std::uint32_t column,
    std::uint32_t row, std::uint32_t alternate_skill, std::uint32_t action,
    std::uint32_t reason, std::uint32_t resampled) {
    detail::KeeperResponse response(c, player, column, row, alternate_skill, action, reason, resampled);
    return response.run();
}
}

// MATCH_ENG.CPP, team_b: 0x101ec890, the ball arriving on a cell. The two players the cell's
// search finds (one per side) contest it; the better one is tried first, and each attempt
// (a tackle, interception or a lost ball) may decide the contest and announce it. Without a
// decision the winner's action is rated and his next action is chosen.
//
// The compiler gave every copy of the announcements its own scratch registers (r13-r15, r6);
// the callees save them in the stack image, so each copy leaves them as the original does.
#include "match_eng_team_b.hpp"
#include <bit>

namespace cm::aot::match_eng {
using namespace guest;

namespace {
std::int32_t s16(std::uint32_t value) { return std::int16_t(value); }
std::uint32_t x8(std::uint32_t value) { return u(std::int8_t(value)); }    // extsb
std::uint32_t x16(std::uint32_t value) { return u(std::int16_t(value)); }  // extsh


class Contest {
public:
    Contest(Cpu& cpu, Frame& stack, std::uint32_t match_, std::uint32_t row_, std::uint32_t column_)
        : c(cpu), frame(stack), match(match_), row(row_), column(column_), fpu(cpu) {}
    std::uint32_t run();

private:
    // Stack locals (relative to sp): outcome byte, the search's team and cell bytes, etc.
    std::uint32_t at(std::int32_t offset) const { return frame.at(offset); }
    std::uint32_t outcome() const { return c.u8(at(0x44)); }
    std::int32_t outcome_s() const { return s8(outcome()); }
    void bl(std::uint32_t address, std::uint32_t entry) {
        match_eng::call(c, entry, address + 4);
    }
    std::uint32_t team(std::uint32_t player, std::uint32_t address) {  // 0x10283690
        c.set(3, player);
        bl(address, 0x10283690);
        return c.r(3);
    }
    std::uint32_t number(std::uint32_t player) { return byte_2f(c, player); }  // 0x101d6770
    std::uint32_t random(std::uint32_t bound, std::uint32_t address) { return draw(c, bound, address + 4); }  // 0x101d3c90
    void set_phase(std::uint32_t value) {  // 0x101d66b0
        c.set({{3, match}, {4, value}});
        set_byte_7928(c, match, value);
    }
    // 0x10201180 and 0x101d6710: a match message with players' teams and numbers.
    void say(std::uint32_t address, std::uint32_t id, std::uint32_t r6, std::uint32_t r7, std::uint32_t r8, std::uint32_t r9) {
        leave(c, {{3, c.u32(match + message_target)}, {4, id}, {5, match}, {6, r6}, {7, r7}, {8, r8}, {9, r9}, {10, 0}});
        bl(address, 0x10201180);
    }
    void say_short(std::uint32_t address, std::uint32_t id, std::uint32_t r6, std::uint32_t r7, std::uint32_t r8) {
        leave(c, {{3, c.u32(match + message_target)}, {4, id}, {5, match}, {6, r6}, {7, r7}, {8, r8}});
        bl(address, 0x101d6710);
    }
    // 0x101d5810: the message to the match's commentary target.
    void comment(std::uint32_t address, std::uint32_t id, std::uint32_t r6) {
        word_79f4(c, match);
        leave(c, {{4, id}, {5, match}, {6, r6}, {7, 0}});
        bl(address, 0x101d5810);
    }
    // Teams and numbers of two players, in the registers the compiler used.
    template <unsigned RT1, unsigned RN1, unsigned RT2>
    std::uint32_t names(std::uint32_t p1, std::uint32_t at1, std::uint32_t p2, std::uint32_t at2) {
        keep(c, {{RT1, team(p1, at1)}});
        keep(c, {{RN1, number(p1)}});
        keep(c, {{RT2, team(p2, at2)}});
        return number(p2);
    }
    // The comparison chain over an action kind (cr0 follows it): the kinds with their own message, else 0.
    std::uint32_t kind_case(std::int32_t k) {
        const auto first = compare(c, k, 0x15);
        if (first == 0) return 0x15;
        if (first > 0) {
            const auto second = compare(c, k, 0x2e);
            if (second == 0) return 0x2e;
            if (second > 0) return 0;
            return compare(c, k, 0x25) == 0 ? 0x25 : 0;
        }
        const auto third = compare(c, k, 0xc);
        if (third == 0) return 0xc;
        if (third > 0) return 0;
        return compare(c, k, 1) == 0 ? 1 : 0;
    }
    // The outcome's comment id by its ladder (0: none, so the random one); `five` is whether outcome 5 has one.
    std::int32_t outcome_message(bool five) {
        const auto b = [&] { return outcome_s(); };
        if (const auto order = compare(c, b(), 4); order == 0) return 0x1ff8;
        else if (order > 0) {
            const auto next = compare(c, b(), 6);
            if (next == 0) return 0x1ff5;
            if (next > 0 || !five) return -1;
            return 0x1ff6;
        }
        const auto low = compare(c, b(), 2);
        if (low == 0) return 0x1ff9;
        if (low > 0) return 0x1ff7;
        return -1;
    }
    // Whether the outcome is 6, 3, 4 or 5; the compiler left the extended byte in a register at one position.
    bool outcome_in_set(int r15_at = -1, int r13_at = -1, int r6_at = -1) {
        static constexpr std::int32_t kinds[] = {6, 3, 4, 5};
        for (int i = 0; i < 4; ++i) {
            if (i == r15_at) keep(c, {{15, u(outcome_s())}});
            if (i == r13_at) keep(c, {{13, u(outcome_s())}});
            if (i == r6_at) c.set(6, outcome());
            if (compare(c, outcome_s(), kinds[i]) == 0) return true;
        }
        return false;
    }
    void ball_to_cell() {
        c.put8(match + ball_row_at, row);
        c.put8(match + ball_column_at, column);
    }
    void attempt_first();
    void attempt_second();
    void announce_first();
    void announce_second();
    void announce_single();
    std::uint32_t after_attempts();

    Cpu& c;
    Frame& frame;
    std::uint32_t match, row, column;
    Fpu fpu;
    std::uint32_t pool = 0, winner = 0, rival = 0, kind = 0, rival_kind = 0, margin = 0, winner_flag = 0, rival_flag = 0;
};

// ecae0..ecd18: the winner against the rival.
void Contest::attempt_first() {
    c.put16(winner + 0x36, u(c.i16(winner + 0x36) + 4));
    c.set({{3, match}, {4, winner}, {5, rival}, {6, kind}, {7, 0xffffffffu}, {8, at(0x43)}, {9, at(0x42)}, {10, at(0x4c)}});
    bl(0x101ecb0c, 0x101f5050);
    c.put8(at(0x44), c.r(3));
    if (compare(c, s8(byte_7931(c, match))) != 0) {
        const auto side = team(winner, 0x101ecb2c);
        keep(c, {{13, side}});
        c.set({{3, winner}, {4, side}});
        bl(0x101ecb3c, 0x10239d10);
        if (compare(c, s8(c.r(3))) != 0) {
            c.set({{3, match}, {4, kind}});
            bl(0x101ecb54, 0x101ee530);
            bool speak = compare(c, s8(c.r(3))) != 0 || compare(c, s8(c.u8(at(0x4c)))) != 0;
            if (!speak) {
                c.set(3, u(c.i16(winner + 0x32)));
                bl(0x101ecb78, 0x101d3c90);
                speak = compare(c, std::int32_t(c.r(3)), -10) < 0;
            }
            if (speak && compare(c, std::int32_t(random(3, 0x101ecb88))) != 0) {
                keep(c, {{13, team(winner, 0x101ecb98)}});
                const auto number_ = number(winner);
                say_short(0x101ecbc0, 0x2168, number_, c.r(13), 0);
            }
        }
    }
    if (compare(c, outcome_s()) != 0) {
        c.set(3, winner);
        bl(0x101ecbd8, 0x10245160);
        const auto weight = fpu.multiply_add(0x101ecbe4, c.f32(pool + 0x28), c.f(1), c.f32(pool + 0x18));
        c.put64(at(0x68), fpu.fctiwz(0, weight));
        c.set(3, c.u32(at(0x6c)));
        bl(0x101ecbf4, 0x101d3c90);
        if (compare(c, std::int32_t(c.r(3))) == 0 && compare(c, s8(winner_flag)) == 0) {
            c.put32(at(0x38), c.u8(at(0x43)));
            c.put32(at(0x3c), c.u8(at(0x42)));
            c.set({{3, match}, {4, x8(kind)}, {5, winner}, {6, rival}, {7, x8(winner_flag)}, {8, outcome()}, {9, row}, {10, column}});
            bl(0x101ecc3c, 0x101d68f0);
            c.set(3, 0);
            c.put8(at(0x44), 0);
        } else {
            c.set(3, 0);
            c.put32(at(0x38), 0);
            c.set({{3, match}, {4, winner}, {5, rival}, {6, at(0x44)}, {7, c.u8(at(0x43))}, {8, c.u8(at(0x42))}, {9, winner_flag}, {10, at(0x64)}});
            bl(0x101ecc74, 0x101f5110);
        }
    } else {
        c.set({{3, match}, {4, winner}, {5, x8(kind)}, {6, at(0x43)}, {7, at(0x42)}});
        bl(0x101ecc90, 0x101f50c0);
        c.put8(at(0x44), c.r(3));
        if (compare(c, outcome_s()) != 0) {
            if (compare(c, std::int32_t(random(10, 0x101eccac))) == 0) {
                c.put32(at(0x38), c.u8(at(0x43)));
                c.put32(at(0x3c), c.u8(at(0x42)));
                c.set({{3, match}, {4, x8(kind)}, {5, winner}, {6, 0}, {7, 0}, {8, outcome()}, {9, row}, {10, column}});
                bl(0x101ecce8, 0x101d68f0);
                c.put8(at(0x44), 0);
            } else {
                c.set({{3, match}, {4, winner}, {5, at(0x44)}, {6, c.u8(at(0x43))}, {7, c.u8(at(0x42))}, {8, 0}, {9, at(0x64)}});
                bl(0x101ecd14, 0x101f5180);
            }
        }
    }
}

// ed384..ed50c: the rival tries instead, against the winner.
void Contest::attempt_second() {
    c.set({{3, match}, {4, rival}, {5, winner}, {6, rival_kind}, {7, 0xffffffffu}, {8, at(0x43)}, {9, at(0x42)}, {10, at(0x4c)}});
    bl(0x101ed3a4, 0x101f5050);
    c.put8(at(0x44), c.r(3));
    if (compare(c, outcome_s()) != 0) {
        c.set(3, rival);
        bl(0x101ed3c0, 0x10245160);
        const auto weight = fpu.multiply_add(0x101ed3cc, c.f32(pool + 0x28), c.f(1), c.f32(pool + 0x18));
        c.put64(at(0x68), fpu.fctiwz(0, weight));
        keep(c, {{13, c.u32(at(0x6c))}});
        c.set(3, c.r(13));
        bl(0x101ed3e0, 0x101d3c90);
        keep(c, {{14, c.r(3)}});
        if (compare(c, std::int32_t(c.r(14))) == 0 && (keep(c, {{15, x8(rival_flag)}}), compare(c, s8(rival_flag))) == 0) {
            c.set(6, c.u8(at(0x43)));
            c.put32(at(0x38), c.r(6));
            keep(c, {{14, c.u8(at(0x42))}});
            c.put32(at(0x3c), c.r(14));
            c.set({{3, match}, {4, x8(rival_kind)}, {5, rival}, {6, winner}, {7, rival_flag}, {8, outcome()}, {9, row}, {10, column}});
            bl(0x101ed42c, 0x101d68f0);
            keep(c, {{13, 0}});
            c.put8(at(0x44), 0);
        } else {
            keep(c, {{15, 0}});
            c.put32(at(0x38), 0);
            c.set({{3, match}, {4, rival}, {5, winner}, {6, at(0x44)}, {7, c.u8(at(0x43))}, {8, c.u8(at(0x42))}, {9, rival_flag}, {10, at(0x64)}});
            bl(0x101ed464, 0x101f5110);
        }
    } else {
        c.set({{3, match}, {4, rival}, {5, x8(rival_kind)}, {6, at(0x43)}, {7, at(0x42)}});
        bl(0x101ed480, 0x101f50c0);
        c.set(6, c.r(3));
        c.put8(at(0x44), c.r(3));
        if (compare(c, outcome_s()) != 0) {
            if (compare(c, std::int32_t(random(10, 0x101ed4a0))) == 0) {
                c.put32(at(0x38), c.u8(at(0x43)));
                c.set(3, c.u8(at(0x42)));
                c.put32(at(0x3c), c.r(3));
                c.set({{3, match}, {4, x8(rival_kind)}, {5, rival}, {6, 0}, {7, 0}, {8, outcome()}, {9, row}, {10, column}});
                bl(0x101ed4dc, 0x101d68f0);
                c.put8(at(0x44), 0);
            } else {
                c.set({{3, match}, {4, rival}, {5, at(0x44)}, {6, c.u8(at(0x43))}, {7, c.u8(at(0x42))}, {8, 0}, {9, at(0x64)}});
                bl(0x101ed508, 0x101f5180);
            }
        }
    }
}

// ecd28..ed37c: the winner's attempt decided something; announce and apply it.
void Contest::announce_first() {
    const auto m = rival, n = winner;
    keep(c, {{15, x8(kind)}});
    switch (kind_case(s8(kind))) {
    case 1: { const auto r6 = names<14, 13, 15>(m, 0x101ecd6c, n, 0x101ecd84); say(0x101ecdb4, 0x1ff1, r6, c.r(15), c.r(13), c.r(14)); break; }
    case 0x15: { const auto r6 = names<15, 14, 13>(m, 0x101ecdc0, n, 0x101ecdd8); say(0x101ece08, 0x1ff0, r6, c.r(13), c.r(14), c.r(15)); break; }
    case 0x2e: { const auto r6 = names<15, 14, 13>(m, 0x101ece14, n, 0x101ece2c); say(0x101ece5c, 0x1ff3, r6, c.r(13), c.r(14), c.r(15)); break; }
    case 0x25: { keep(c, {{15, team(n, 0x101ece68)}}); const auto r6 = number(n); say_short(0x101ece90, 0x1fe8, r6, c.r(15), 0); break; }
    case 0xc: { const auto r6 = names<15, 14, 13>(m, 0x101ece9c, n, 0x101eceb4); say(0x101ecee4, 0x1ff4, r6, c.r(13), c.r(14), c.r(15)); break; }
    default: { const auto r6 = names<15, 14, 13>(m, 0x101ecef0, n, 0x101ecf08); say(0x101ecf38, 0x1fe9, r6, c.r(13), c.r(14), c.r(15)); break; }
    }
    keep(c, {{28, 0xffffffffu}});
    if (const auto id = outcome_message(true); id >= 0) keep(c, {{28, u(id)}});
    else {
        const auto draw = random(7, 0x101ecfa0);
        keep(c, {{15, draw}});
        c.set({{3, draw + 2}, {4, x8(c.u8(at(0x42)))}});
        if (compare(c, s8(c.u8(at(0x42))), std::int32_t(draw + 2)) > 0) keep(c, {{28, 0x210a}});
    }
    if (compare(c, outcome_s(), 7) == 0) { keep(c, {{27, 0x1f79}}); set_phase(7); }
    else {
        const auto side = team(n, 0x101ecfe8);
        c.set({{3, n}, {4, side}});
        bl(0x101ecff4, 0x10239d10);
        if (compare(c, s8(c.r(3))) == 0) { keep(c, {{27, 0x1f79}}); set_phase(6); }
        else {
            if (compare(c, s16(c.r(28)), -1) == 0) {
                const auto draw = random(4, 0x101ed014);
                if (compare(c, std::int32_t(draw), 1) == 0) { keep(c, {{14, team(m, 0x101ed060)}}); comment(0x101ed080, 0x20ec, c.r(14)); }
                else if (std::int32_t(draw) < 1 && compare(c, std::int32_t(draw)) >= 0) { keep(c, {{14, team(m, 0x101ed034)}}); comment(0x101ed054, 0x20e9, c.r(14)); }
            }
            keep(c, {{27, 0x1f7a}});
            set_phase(5);
            c.put32(match + 0x3110, m);
            keep(c, {{14, u(c.i16(n + 0x36))}});
            c.set(3, c.r(14) - 0x1f4);
            c.put16(n + 0x36, c.r(3));
        }
    }
    ball_to_cell();
    keep(c, {{15, std::uint32_t(s8(team(n, 0x101ed0c4)) == 0)}});
    c.set({{3, match}, {4, x8(c.r(15))}});
    set_possession_side(c, match, c.r(4));
    c.set({{3, match}, {4, 0}});
    set_word_e000(c, match, 0);
    if (compare(c, s16(c.r(27)), 0x1f7a) == 0) { keep(c, {{15, team(m, 0x101ed0fc)}}); comment(0x101ed11c, c.r(27), c.r(15)); }
    if (compare(c, s16(c.r(28)), -1) > 0) {
        if (compare(c, std::int32_t(random(4, 0x101ed130))) == 0) {
            keep(c, {{15, random(0x14, 0x101ed140)}});
            const auto bound = random(x8(c.u8(at(0x42))), 0x101ed150);
            if (compare(c, std::int32_t(bound), std::int32_t(c.r(15))) > 0) { word_79f4(c, match); say_short(0x101ed178, 0x20f5, 0, 0, 0); }
            word_79f4(c, match);
            say_short(0x101ed198, 0x2109, c.u8(n + 0x2f), c.u8(n + 0x27), 0);
        }
        word_79f4(c, match);
        say_short(0x101ed1b8, c.r(28), c.u8(n + 0x2f), c.u8(n + 0x27), 0);
        if (outcome_in_set(2)) { c.set({{3, match}, {4, n}, {5, 0}}); bl(0x101ed208, 0x101f1530); }
    }
    if (compare(c, s16(c.r(27)), 0x1f7a) != 0) { keep(c, {{15, team(m, 0x101ed21c)}}); comment(0x101ed23c, c.r(27), c.r(15)); }
    c.set({{3, match}, {4, kind}});
    bl(0x101ed248, 0x101ee530);
    if (compare(c, s8(c.r(3))) == 0) {
        if (compare(c, std::int32_t(random(9, 0x101ed25c))) == 0) {
            const auto flag = u(s8(c.u8(m + 0x27)) == 0);
            comment(0x101ed290, 0x20df, flag);
            if (compare(c, std::int32_t(random(3, 0x101ed298))) == 0) comment(0x101ed2cc, 0x20e0, flag);
            if (outcome_in_set() || compare(c, s16(c.r(27)), 0x1f7a) == 0) {
                c.set(4, 1);
                c.put8(m + 0x16, 1);
                if (compare(c, std::int32_t(random(7, 0x101ed328))) == 0) {
                    keep(c, {{14, c.u8(m + 0x27)}});
                    keep(c, {{13, x8(c.r(14))}});
                    c.set(6, u(std::countl_zero(c.r(13))));
                    keep(c, {{15, c.r(6) >> 5}});
                    comment(0x101ed35c, 0x20d9, x8(c.r(15)));
                }
            }
        }
        c.set_f(1, c.f32(at(0x64)));
        c.set({{3, match}, {4, n}});
        bl(0x101ed36c, 0x101d6780);
        c.set({{3, n}, {4, 0}});
        bl(0x101ed378, 0x102884d0);
    }
}

// ed51c..edb80: the rival's attempt decided something.
void Contest::announce_second() {
    const auto m = rival, n = winner;
    switch (kind_case(s8(rival_kind))) {
    case 1: { const auto r6 = names<13, 14, 15>(n, 0x101ed560, m, 0x101ed578); say(0x101ed5a8, 0x1ff1, r6, c.r(15), c.r(14), c.r(13)); break; }
    case 0x15: { const auto r6 = names<13, 14, 15>(n, 0x101ed5b4, m, 0x101ed5cc); say(0x101ed5fc, 0x1ff0, r6, c.r(15), c.r(14), c.r(13)); break; }
    case 0x25: { keep(c, {{13, team(m, 0x101ed608)}}); const auto r6 = number(m); say_short(0x101ed630, 0x1fe8, r6, c.r(13), 0); break; }
    case 0x2e: { const auto r6 = names<13, 14, 15>(n, 0x101ed63c, m, 0x101ed654); say(0x101ed684, 0x1ff3, r6, c.r(15), c.r(14), c.r(13)); break; }
    case 0xc: {
        keep(c, {{13, team(n, 0x101ed690)}});
        c.put32(at(0x70), number(n));
        keep(c, {{14, team(m, 0x101ed6a8)}});
        keep(c, {{15, number(m)}});
        say(0x101ed6dc, 0x1ff4, c.r(15), c.r(14), c.u32(at(0x70)), c.r(13));
        break;
    }
    default: { const auto r6 = names<14, 13, 15>(n, 0x101ed6e8, m, 0x101ed700); say(0x101ed730, 0x1fe9, r6, c.r(15), c.r(13), c.r(14)); break; }
    }
    keep(c, {{28, 0xffffffffu}});
    if (const auto id = outcome_message(true); id >= 0) keep(c, {{28, u(id)}});
    else {
        const auto draw = random(7, 0x101ed798);
        c.set(3, draw + 2);
        if (compare(c, s8(c.u8(at(0x42))), std::int32_t(draw + 2)) > 0) keep(c, {{28, 0x210a}});
    }
    if (compare(c, outcome_s(), 7) == 0) { keep(c, {{27, 0x1f79}}); set_phase(7); }
    else {
        const auto side = team(m, 0x101ed7dc);
        c.set({{3, m}, {4, side}});
        bl(0x101ed7e8, 0x10239d10);
        if (compare(c, s8(c.r(3))) == 0) { keep(c, {{27, 0x1f79}}); set_phase(6); }
        else {
            if (compare(c, s16(c.r(28)), -1) == 0) {
                const auto draw = random(4, 0x101ed808);
                if (compare(c, std::int32_t(draw), 1) == 0) {
                    const auto t = team(m, 0x101ed868);
                    c.set(4, x8(t));
                    keep(c, {{13, std::uint32_t(s8(t) == 0)}});
                    comment(0x101ed894, 0x20ec, c.r(13));
                } else if (std::int32_t(draw) < 1 && compare(c, std::int32_t(draw)) >= 0) {
                    keep(c, {{20, std::uint32_t(s8(team(m, 0x101ed828)) == 0)}});
                    comment(0x101ed85c, 0x20e9, c.r(20));
                }
            }
            keep(c, {{27, 0x1f7a}});
            set_phase(5);
            c.put32(match + 0x3110, n);
            c.set(3, u(c.i16(m + 0x36)));
            c.put16(m + 0x36, c.r(3) - 0x1f4);
        }
    }
    ball_to_cell();
    const auto flag_of_rival = [&](std::uint32_t at_team) { return u(s8(team(m, at_team)) == 0); };
    c.set({{3, match}, {4, flag_of_rival(0x101ed8d8)}});
    set_possession_side(c, match, c.r(4));
    c.set({{3, match}, {4, 0}});
    set_word_e000(c, match, 0);
    if (compare(c, s16(c.r(27)), 0x1f7a) == 0) { keep(c, {{13, flag_of_rival(0x101ed910)}}); comment(0x101ed93c, c.r(27), c.r(13)); }
    if (compare(c, s16(c.r(28)), -1) > 0) {
        if (compare(c, std::int32_t(random(4, 0x101ed950))) == 0) {
            keep(c, {{13, random(0x14, 0x101ed960)}});
            const auto bound = random(x8(c.u8(at(0x42))), 0x101ed970);
            if (compare(c, std::int32_t(bound), std::int32_t(c.r(13))) > 0) { word_79f4(c, match); say_short(0x101ed998, 0x20f5, 0, 0, 0); }
            word_79f4(c, match);
            say_short(0x101ed9b8, 0x2109, c.u8(m + 0x2f), c.u8(m + 0x27), 0);
        }
        word_79f4(c, match);
        say_short(0x101ed9e0, c.r(28), c.u8(m + 0x2f), c.u8(m + 0x27), 0);
        if (outcome_in_set()) { c.set({{3, match}, {4, m}, {5, 0}}); bl(0x101eda30, 0x101f1530); }
    }
    if (compare(c, s16(c.r(27)), 0x1f7a) != 0) { keep(c, {{13, flag_of_rival(0x101eda44)}}); comment(0x101eda70, c.r(27), c.r(13)); }
    c.set({{3, match}, {4, rival_kind}});
    bl(0x101eda7c, 0x101ee530);
    if (compare(c, s8(c.r(3))) == 0) {
        if (compare(c, std::int32_t(random(9, 0x101eda90))) == 0) {
            comment(0x101edab4, 0x20df, c.u8(m + 0x27));
            if (compare(c, std::int32_t(random(3, 0x101edabc))) == 0) comment(0x101edae0, 0x20e0, c.u8(m + 0x27));
            if (outcome_in_set(-1, 0, 1) || compare(c, s16(c.r(27)), 0x1f7a) == 0) {
                c.put8(m + 0x16, 1);
                if (compare(c, std::int32_t(random(7, 0x101edb3c))) == 0) comment(0x101edb60, 0x20d9, c.u8(m + 0x27));
            }
        }
        c.set_f(1, c.f32(at(0x64)));
        c.set({{3, match}, {4, m}});
        bl(0x101edb70, 0x101d6780);
        c.set({{3, m}, {4, 0}});
        bl(0x101edb7c, 0x102884d0);
    }
}

// edc44..ee0f4: the winner alone (no rival) decided something.
void Contest::announce_single() {
    const auto n = winner;
    keep(c, {{15, team(n, 0x101edc48)}});
    keep(c, {{14, number(n)}});
    say_short(0x101edc74, 0x1fe8, c.r(14), c.r(15), 0);
    keep(c, {{28, 0xffffffffu}});
    keep(c, {{13, outcome()}});
    keep(c, {{15, x8(outcome())}});
    if (const auto id = outcome_message(false); id >= 0) keep(c, {{28, u(id)}});
    else {
        const auto draw = random(7, 0x101edcd0);
        c.set(6, draw);
        keep(c, {{15, draw + 2}});
        keep(c, {{14, c.u8(at(0x42))}});
        keep(c, {{13, x8(c.r(14))}});
        if (compare(c, std::int32_t(c.r(13)), std::int32_t(c.r(15))) > 0) keep(c, {{28, 0x210a}});
    }
    c.set(6, outcome());
    keep(c, {{15, x8(outcome())}});
    if (compare(c, s8(c.r(15)), 7) == 0) { keep(c, {{27, 0x1f79}}); set_phase(7); }
    else {
        keep(c, {{14, team(n, 0x101edd18)}});
        c.set({{3, n}, {4, c.r(14)}});
        bl(0x101edd28, 0x10239d10);
        keep(c, {{13, c.r(3)}});
        c.set(6, x8(c.r(13)));
        if (compare(c, s8(c.r(6))) == 0) { keep(c, {{27, 0x1f79}}); set_phase(6); }
        else {
            keep(c, {{15, x16(c.r(28))}});
            if (compare(c, s16(c.r(15)), -1) == 0) {
                const auto draw = random(4, 0x101edd4c);
                c.set(6, draw);
                if (compare(c, std::int32_t(draw), 1) == 0) {
                    keep(c, {{13, team(n, 0x101eddb4)}});
                    c.set(6, x8(c.r(13)));
                    keep(c, {{15, x8(u(std::countl_zero(c.r(6)) >> 5))}});
                    comment(0x101edde4, 0x20ec, c.r(15));
                } else if (std::int32_t(draw) < 1 && compare(c, std::int32_t(draw)) >= 0) {
                    keep(c, {{15, team(n, 0x101edd70)}});
                    keep(c, {{14, x8(c.r(15))}});
                    c.set(6, u(std::countl_zero(c.r(14))));
                    keep(c, {{13, c.r(6) >> 5}});
                    keep(c, {{15, x8(c.r(13))}});
                    keep(c, {{14, word_79f4(c, match)}});
                    comment(0x101edda8, 0x20e9, c.r(15));
                }
            }
            keep(c, {{27, 0x1f7a}});
            set_phase(5);
            c.put32(match + 0x3110, 0);
            c.set(4, u(c.i16(n + 0x36)));
            c.put16(n + 0x36, c.r(4) - 0x1f4);
        }
    }
    ball_to_cell();
    const auto flag = [&](std::uint32_t at_team) { return u(s8(team(n, at_team)) == 0); };
    c.set(4, flag(0x101ede2c));
    c.set(3, match);
    set_possession_side(c, match, c.r(4));
    c.set({{3, match}, {4, 0}});
    set_word_e000(c, match, 0);
    if (compare(c, s16(c.r(27)), 0x1f7a) == 0) { keep(c, {{14, flag(0x101ede64)}}); comment(0x101ede94, c.r(27), c.r(14)); }
    if (compare(c, s16(c.r(28)), -1) > 0) {
        if (compare(c, std::int32_t(random(4, 0x101edea8))) == 0) {
            keep(c, {{15, random(0x14, 0x101edeb8)}});
            const auto bound = random(x8(c.u8(at(0x42))), 0x101edec8);
            if (compare(c, std::int32_t(bound), std::int32_t(c.r(15))) > 0) {
                c.set(4, word_79f4(c, match));
                say_short(0x101edef8, 0x20f5, 0, 0, 0);
            }
            word_79f4(c, match);
            say_short(0x101edf18, 0x2109, c.u8(n + 0x2f), c.u8(n + 0x27), 0);
        }
        keep(c, {{15, word_79f4(c, match)}});
        say_short(0x101edf40, c.r(28), c.u8(n + 0x2f), c.u8(n + 0x27), 0);
        if (outcome_in_set(1)) { c.set({{3, match}, {4, n}, {5, 0}}); bl(0x101edf90, 0x101f1530); }
    }
    if (compare(c, s16(c.r(27)), 0x1f7a) != 0) { keep(c, {{15, flag(0x101edfa4)}}); comment(0x101edfd0, c.r(27), c.r(15)); }
    c.set({{3, match}, {4, kind}});
    bl(0x101edfdc, 0x101ee530);
    if (compare(c, s8(c.r(3))) == 0) {
        keep(c, {{15, random(9, 0x101edff0)}});
        if (compare(c, std::int32_t(c.r(15))) == 0) {
            comment(0x101ee018, 0x20df, c.u8(n + 0x27));
            if (compare(c, std::int32_t(random(3, 0x101ee020))) == 0) comment(0x101ee044, 0x20e0, c.u8(n + 0x27));
            if (outcome_in_set() || compare(c, s16(c.r(27)), 0x1f7a) == 0) {
                c.put8(n + 0x16, 1);
                if (compare(c, std::int32_t(random(7, 0x101ee0a0))) == 0) comment(0x101ee0d0, 0x20d9, c.u8(n + 0x27));
            }
        }
        c.set_f(1, c.f32(at(0x64)));
        c.set({{3, match}, {4, n}});
        bl(0x101ee0e0, 0x101d6780);
        c.set({{3, n}, {4, 0}});
        bl(0x101ee0ec, 0x102884d0);
    }
}

// ee0f8..ee4ec: nothing was decided; the winner's action is rated and the next one chosen.
std::uint32_t Contest::after_attempts() {
    const auto n = winner, m = rival;
    const auto finish = [&]() -> std::uint32_t {
        c.set({{3, n}, {4, x8(kind)}});
        set_action_code(c, n, c.r(4));
        const auto drawn = random(200, 0x101ee37c);
        c.put32(at(0x70), drawn);
        keep(c, {{14, secondary_rating(c, n)}});
        keep(c, {{15, x8(c.r(14))}});
        keep(c, {{14, trait_10f(c, n)}});
        keep(c, {{13, x8(c.r(14))}});
        keep(c, {{15, c.r(13) * c.r(15)}});
        bool edge = compare(c, std::int32_t(c.r(15)), std::int32_t(c.u32(at(0x70)))) > 0;
        if (!edge) {
            c.set(3, n);
            bl(0x101ee3b8, 0x10245160);
            c.set(6, c.toc(-0x52c4));
            fpu.fcmp(0, c.f(1), c.f64(c.r(6) + 0x98));
            edge = (c.cr(0) & 8) != 0;
            if (!edge) {
                c.set({{3, match}, {4, n}, {5, 0}});
                bl(0x101ee3d8, 0x101d8d70);
                edge = compare(c, s8(c.r(3))) != 0;
                if (!edge) {
                    c.set({{3, n}, {4, row}, {5, column}, {6, c.u8(match + 0x1e1)}, {7, c.u8(match + 0x1e2)}});
                    bl(0x101ee494, 0x10237830);
                }
            }
        }
        if (edge) {
            if (compare(c, s8(c.u8(n + 0x27))) == 0) {
                const auto reach = compare(c, s8(column), 9) < 0 ? s8(column) + 3 : 0xb;
                c.put32(at(0x5c), u(reach));
                c.set({{3, n}, {4, row}, {5, column}, {6, 4}, {7, x8(c.u32(at(0x5c)))}});
                bl(0x101ee434, 0x10237830);
            } else {
                const auto reach = compare(c, s8(column), 3) > 0 ? s8(column) - 3 : 0;
                c.put32(at(0x58), u(reach));
                c.set({{3, n}, {4, row}, {5, column}, {6, 4}, {7, x8(c.u32(at(0x58)))}});
                bl(0x101ee478, 0x10237830);
            }
        }
        c.set(3, n);
        bl(0x101ee49c, 0x1022e3a0);
        keep(c, {{20, margin - u(c.i16(at(0x50)))}});
        c.set({{3, n}, {4, x16(c.r(20) - 0xe1)}});
        set_pressure_adjustment(c, n, c.r(4));
        if (compare_logical(c, m) != 0) {
            c.set({{3, m}, {4, c.u8(at(0x45))}});
            bl(0x101ee4c8, 0x10285ed0);
        }
        c.set(3, n);
        bl(0x101ee4d0, 0x102775e0);
        return result(c, n);
    };
    if (compare(c, s8(kind), 1) == 0) {
        if (compare_logical(c, m) == 0) {
            c.set({{3, n}, {4, x16(margin + 0x11)}});
            bl(0x101ee118, 0x10270320);
            if (compare(c, s8(c.r(3))) == 0) return result(c, 0);
        }
        if (compare_logical(c, m) == 0) return finish();
        c.set({{3, n}, {4, x16(margin - u(c.i16(at(0x50))))}});
        bl(0x101ee140, 0x10270320);
        if (compare(c, s8(c.r(3))) == 0) return result(c, 0);
        return finish();
    }
    if (compare_logical(c, m) == 0) return finish();
    const auto draw = random(6, 0x101ee164);
    c.set(4, c.toc(-0x52c4));
    c.put32(at(0x6c), draw ^ 0x80000000u);
    c.put32(at(0x68), 0x43300000);
    const auto magic = fpu.fsubs(c.f64(at(0x68)), c.f64(c.r(4) + 0x60));
    const auto sum = fpu.fadds(c.f32(pool + 0x108), c.f32(n + 0xb0));
    const auto total = fpu.fadds(sum, magic);
    c.set_f(1, total);
    fpu.fcmp(0, total, c.f32(m + 0xb0));
    if (c.cr(0) & 8) {
        const auto r6 = names<14, 13, 15>(m, 0x101ee1a8, n, 0x101ee1c0);
        say(0x101ee1f0, 0x2164, r6, c.r(15), c.r(13), c.r(14));
    } else {
        c.set(3, m);
        bl(0x101ee1fc, 0x10289560);
        if (compare(c, s16(c.r(3)), 1) == 0 && compare(c, std::int32_t(random(5, 0x101ee210))) != 0) {
            const auto r6 = names<13, 14, 15>(m, 0x101ee220, n, 0x101ee238);
            say(0x101ee268, 0x214f, r6, c.r(15), c.r(14), c.r(13));
        } else {
            const auto r6 = names<14, 13, 15>(m, 0x101ee274, n, 0x101ee28c);
            say(0x101ee2bc, 0x1f5d, r6, c.r(15), c.r(13), c.r(14));
        }
    }
    c.put8(n + 7, c.u8(n + 7) + 1u);
    c.put8(n + 8, c.u8(n + 8) + 1u);
    c.put16(n + 0x36, u(c.i16(n + 0x36) - 10));
    c.put16(n + 0x36, u(c.i16(n + 0x36) + 0x32));
    keep(c, {{13, c.u8(m + 7) + 1u}});
    c.put8(m + 7, c.r(13));
    c.set(3, m);
    bl(0x101ee300, 0x10289560);
    keep(c, {{14, c.r(3)}});
    keep(c, {{15, x16(c.r(14))}});
    if (compare(c, s16(c.r(15)), 1) != 0) { c.set(6, u(-10)); c.put32(at(0x60), c.r(6)); }
    else { keep(c, {{13, u(-0x258)}}); c.put32(at(0x60), c.r(13)); }
    keep(c, {{14, u(c.i16(m + 0x36))}});
    keep(c, {{15, c.u32(at(0x60))}});
    c.set(6, x16(c.r(15)));
    keep(c, {{13, c.r(14) + c.r(6)}});
    c.put16(m + 0x36, c.r(13));
    c.set(3, n);
    bl(0x101ee340, 0x10270ba0);
    c.set(6, c.r(3));
    keep(c, {{13, x8(c.r(6))}});
    if (compare(c, s8(c.r(13))) != 0) {
        keep(c, {{14, c.u8(n + 9)}});
        keep(c, {{15, c.r(14) + 1}});
        c.put8(n + 9, c.r(15));
        c.set(6, u(c.i16(n + 0x36)));
        keep(c, {{13, c.r(6) + 0x12c}});
        c.put16(n + 0x36, c.r(13));
    }
    return finish();
}

std::uint32_t Contest::run() {
    pool = c.toc(-0x52c8);
    keep(c, {{31, match}, {24, row}, {26, column}, {21, pool}});
    c.put16(at(0x54), 0);
    c.put16(at(0x52), 0);
    c.put8(at(0x4b), 0xff);
    c.put8(at(0x4a), 0xff);
    if (compare(c, s8(row)) < 0 || compare(c, s8(column)) < 0 || compare(c, s8(row), 9) >= 0 || compare(c, s8(column), 12) >= 0) {
        leave(c, {{3, match}, {4, c.u8(match + last_touch_side)}});
        bl(0x101ec908, 0x101f0c00);
        return result(c, 0);
    }
    // The two searches (one per side): who stands on the cell and how far his bearing is from the turn.
    c.put32(at(0x38), at(0x41));
    c.put32(at(0x3c), 0);
    c.set({{3, match}, {4, 0}, {5, 1}, {6, 0x15}, {7, at(0x49)}, {8, at(0x47)}, {9, at(0x54)}, {10, at(0x4b)}});
    bl(0x101ec944, 0x101eaf60);
    keep(c, {{20, c.r(3)}});
    const auto first = c.r(3);
    if (compare_logical(c, first) != 0) {
        c.set(3, first);
        bl(0x101ec958, 0x10237b00);
        const auto turn = c.i16(match + 0x7950);
        leave(c, {{3, u(s16(c.r(3)) - turn)}, {4, u(turn)}});
        bl(0x101ec96c, 0x10775540);
        keep(c, {{23, x16(c.r(3))}});
        if (compare(c, s16(c.r(23)), 0xb4) > 0) {
            keep(c, {{13, 0x168 - c.r(23)}});
            keep(c, {{23, x16(c.r(13))}});
        }
        c.put16(at(0x4e), 0x168 - (c.r(23) << 1));
        c.put16(at(0x54), u(c.i16(at(0x54)) - c.i16(at(0x4e))));
    }
    c.put32(at(0x38), at(0x40));
    c.put32(at(0x3c), 0);
    c.set({{3, match}, {4, 0}, {5, 0}, {6, 0x15}, {7, at(0x48)}, {8, at(0x46)}, {9, at(0x52)}, {10, at(0x4a)}});
    bl(0x101ec9d8, 0x101eaf60);
    keep(c, {{19, c.r(3)}});
    const auto second = c.r(3);
    if (compare_logical(c, second) != 0) {
        c.set(3, second);
        bl(0x101ec9ec, 0x10237b00);
        const auto turn = c.i16(match + 0x7950);
        keep(c, {{13, u(s16(c.r(3)) - turn)}});
        c.set(3, c.r(13));
        bl(0x101eca00, 0x10775540);
        keep(c, {{23, x16(c.r(3))}});
        if (compare(c, s16(c.r(23)), 0xb4) > 0) keep(c, {{23, x16(0x168 - c.r(23))}});
        keep(c, {{13, 0x168 - (c.r(23) << 1)}});
        c.put16(at(0x4e), c.r(13));
        c.put16(at(0x52), u(c.i16(at(0x52)) - c.i16(at(0x4e))));
    }
    // The better bearing is the winner (the first search wins ties' reverse).
    if (compare(c, c.i16(at(0x54)), c.i16(at(0x52))) > 0) {
        winner = first;
        kind = c.u8(at(0x4b));
        margin = u(c.i16(at(0x54)));
        c.put16(at(0x50), c.u16(at(0x52)));
        rival = second;
        rival_kind = c.u8(at(0x4a));
        c.put8(at(0x45), c.u8(at(0x47)));
        winner_flag = c.u8(at(0x41));
        rival_flag = c.u8(at(0x40));
    } else {
        winner = second;
        kind = c.u8(at(0x4a));
        margin = u(c.i16(at(0x52)));
        c.put16(at(0x50), c.u16(at(0x54)));
        rival = first;
        rival_kind = c.u8(at(0x4b));
        c.put8(at(0x45), c.u8(at(0x46)));
        winner_flag = c.u8(at(0x40));
        rival_flag = c.u8(at(0x41));
    }
    keep(c, {{30, winner}, {25, kind}, {18, margin}, {29, rival}, {22, rival_kind}, {17, winner_flag}, {16, rival_flag}});
    if (compare_logical(c, winner) != 0) {
        c.set({{3, match}, {4, winner}, {5, x8(kind)}, {6, rival}, {7, x8(rival_kind)}, {8, 0}});
        bl(0x101ecacc, 0x101f5350);
    }
    if (compare_logical(c, winner) != 0 && compare_logical(c, rival) != 0) {
        attempt_first();
        if (compare(c, outcome_s()) != 0) { announce_first(); return result(c, 0); }
        attempt_second();
        if (compare(c, outcome_s()) != 0) { announce_second(); return result(c, 0); }
    }
    if (compare_logical(c, winner) == 0) {
        ball_to_cell();
        c.set({{3, 0}, {4, match + 0x10000}});
        c.put32(match + 0xdff4, 0);
        return result(c, winner);
    }
    if (compare_logical(c, rival) == 0) {
        c.set({{3, match}, {4, winner}, {5, x8(kind)}, {6, at(0x43)}, {7, at(0x42)}});
        bl(0x101edbac, 0x101f50c0);
        c.put8(at(0x44), c.r(3));
        if (compare(c, outcome_s()) != 0) {
            if (compare(c, std::int32_t(random(10, 0x101edbc8))) == 0) {
                c.put32(at(0x38), c.u8(at(0x43)));
                c.put32(at(0x3c), c.u8(at(0x42)));
                c.set({{3, match}, {4, x8(kind)}, {5, winner}, {6, 0}, {7, 0}, {8, outcome()}, {9, row}, {10, column}});
                bl(0x101edc04, 0x101d68f0);
                c.put8(at(0x44), 0);
            } else {
                c.set({{3, match}, {4, winner}, {5, at(0x44)}, {6, c.u8(at(0x43))}, {7, c.u8(at(0x42))}, {8, 0}, {9, at(0x64)}});
                bl(0x101edc30, 0x101f5180);
            }
        }
        if (compare(c, outcome_s()) != 0) { announce_single(); return result(c, 0); }
    }
    return after_attempts();
}
}  // namespace

std::uint32_t resolve_contest(Cpu& c, std::uint32_t match, std::uint32_t row, std::uint32_t column) {
    Frame frame(c, 0x101ec890, 0xd0, {.first_gpr = 13});
    return Contest(c, frame, match, row, column).run();
}
}

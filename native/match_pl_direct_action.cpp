#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
// One contest activation owns all random draws, stack out-parameters and the
// original live player views. Helpers do not introduce additional guest frames.
struct DirectAction {
    Cpu& c;
    Frame frame;
    Fpu fp;
    Player actor;
    std::uint32_t doubles, seed, begin, end, cursor, strings, singles;

    DirectAction(Cpu& cpu, std::uint32_t player, std::uint32_t mode)
        : c(cpu), frame(c, 0x10274170, 0x120, {.first_gpr = 17, .gpr_offset = -0x4c, .first_fpr = 30}),
          fp(c), actor{c, player}, doubles(c.toc(-0x51e8)), seed(c.toc(-0x7b90)),
          begin(c.toc(-0x7b8c)), end(c.toc(-0x7b88)), cursor(c.toc(-0x7b84)),
          strings(c.toc(-0x51e4)), singles(c.toc(-0x51e0)) {
        keep(c, {{31, player}, {26, mode}, {19, c.toc(-0x7ba8)}, {27, doubles},
                 {28, seed}, {25, begin}, {24, end}, {29, cursor}, {21, strings}, {22, singles}});
    }
    std::uint32_t player() const { return actor.address; }
    std::uint32_t opponent() const { return c.r(30); }
    std::uint32_t at(unsigned offset) const { return frame.at(offset); }
    int byte(unsigned offset) const { return c.i8(at(offset)); }
    static std::int32_t high(std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    }
    std::uint32_t sign_after_shift(std::int32_t value, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << bits) - 1)));
        return unsigned(value < 0);
    }
    std::uint32_t sample() const { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); }
    template<class Before> void advance(std::uint32_t back, Before&& before) {
        const auto next = c.u32(cursor) + 2; c.set(3, next); before();
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    }
    void advance(std::uint32_t back) { advance(back, [] {}); }
    // The signed word, rather than an unsigned host random number, is reduced.
    std::int32_t modulo(unsigned divisor) {
        c.set(4, sample()); std::int32_t product = 0; unsigned shift = 0; bool add_word = false;
        switch (divisor) {
        case 3: product = high(c.r(4), 0x55555556); break;
        case 5: product = high(c.r(4), 0x66666667); shift = 1; break;
        case 7: product = high(c.r(4), std::int32_t(0x92492493)); shift = 2; add_word = true; break;
        case 9: product = high(c.r(4), 0x38e38e39); shift = 1; break;
        case 20: product = high(c.r(4), 0x66666667); shift = 3; break;
        case 30: product = high(c.r(4), std::int32_t(0x88888889)); shift = 4; add_word = true; break;
        case 50: product = high(c.r(4), 0x51eb851f); shift = 4; break;
        default: throw std::logic_error("Unsupported direct-action remainder");
        }
        if (add_word) product = std::int32_t(std::uint32_t(product) + c.r(4));
        c.set(3, shift ? sign_after_shift(product, shift) : std::uint32_t(product));
        return std::int32_t(c.r(4)) % int(divisor);
    }
    std::int32_t binary(bool four = false) {
        c.set(3, c.u32(cursor)); const auto value = sample();
        c.set(3, unsigned(std::int32_t(value) < 0)); return std::int32_t(value) % (four ? 4 : 2);
    }
    void range(unsigned bound_register, unsigned draw_register, std::uint32_t back,
               std::uint32_t correction, bool known_narrow = false) {
        const auto bound = [&] { return std::int32_t(c.r(bound_register)); };
        if (compare(c, bound()) == 0) { c.set(draw_register, 0); return; }
        advance(back); c.set(3, 65535); bool narrow = compare(c, bound(), 65535) <= 0;
        if (narrow) { c.set(3, std::uint32_t(-65535)); narrow = compare(c, bound(), -65535) >= 0; }
        if (narrow) {
            c.set(3, c.u32(cursor)); compare(c, bound()); c.set(3, sample());
            const auto residual = c.r(3) - std::uint32_t(divide_signed<4>(c.r(3), c.r(bound_register))) * c.r(bound_register);
            c.set(draw_register, bound() > 0 ? residual : 0u - residual); return;
        }
        if (known_narrow) throw std::logic_error("Direct-action narrow-range invariant");
        c.set(3, c.r(bound_register) ^ 0x80000000); c.put32(at(0x7c), c.r(3)); c.put32(at(0x78), 0x43300000);
        c.set(6, 0x80008001); c.set(7, c.u32(cursor)); c.set(4, 0x43300000); c.set(5, 65535); c.set(3, 0x80008001);
        c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, c.f64(at(0x78))); c.set_f(0, c.f64(doubles));
        c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set(8, c.u16(c.r(7))); c.set(7, c.u32(seed)); c.put32(at(0x80), c.r(4));
        c.set_f(2, fp.fdiv(c.f(1), c.f(0))); c.set(7, c.r(8) + c.r(7)); c.set_f(1, c.f64(doubles + 0x80));
        const auto raw = c.r(7); const auto product = std::int32_t(std::uint32_t(high(raw, std::int32_t(0x80008001))) + raw);
        c.set(6, sign_after_shift(product, 15)); c.set(draw_register, raw - std::uint32_t(std::int32_t(raw) / 65535) * 65535u);
        c.set(4, c.r(draw_register) ^ 0x80000000); c.put32(at(0x84), c.r(4)); c.set_f(0, fp.fsub(c.f64(at(0x80)), c.f(1)));
        const auto reduced_word = c.r(bound_register) * 10; c.set_f(0, fp.fmul(c.f(2), c.f(0)));
        const auto reduced_product = std::int32_t(std::uint32_t(high(reduced_word, std::int32_t(0x80008001))) + reduced_word);
        sign_after_shift(reduced_product, 15); c.set(3, std::uint32_t(std::int32_t(reduced_word) / 65535));
        c.put64(at(0x88), fp.fctiwz(0, c.f(0))); c.set(draw_register, c.u32(at(0x8c)));
        enter(c, correction, {c.r(3)}); sample_range(c, c.r(3));
        const auto numerator = c.r(3) + 5; c.set(4, sign_after_shift(high(numerator, 0x66666667), 2));
        c.set(3, std::uint32_t(std::int32_t(numerator) / 10)); c.set(draw_register, c.r(draw_register) + c.r(3));
        if (compare(c, std::int32_t(c.r(draw_register)), std::int32_t(c.r(bound_register) - 1)) > 0) c.set(draw_register, c.r(bound_register) - 1);
    }
    // The actor's side chooses the area even when testing opponent coordinates.
    // Opposite side remains a boolean for unusual signed side bytes.
    bool opposite_area(std::uint32_t address) {
        const Player p{c, address}; const bool upper = compare(c, int(std::int8_t(actor.side())) == 0, 1) == 0;
        c.set(3, 0); bool inside = compare(c, p.column(), 2) >= 0;
        if (inside) inside = compare(c, p.column(), 6) <= 0;
        if (inside) inside = upper ? compare(c, p.row(), 10) >= 0 : compare(c, p.row()) >= 0;
        if (inside) inside = compare(c, p.row(), upper ? 11 : 1) <= 0;
        if (inside) c.set(3, 1); return compare(c, std::int8_t(c.r(3))) != 0;
    }
    bool defensive_area(std::uint32_t address) {
        const Player p{c, address}; const bool upper = compare(c, p.side(), 1) == 0;
        c.set(3, 0); bool inside = compare(c, p.column(), 2) >= 0;
        if (inside) inside = compare(c, p.column(), 6) <= 0;
        if (inside) inside = upper ? compare(c, p.row(), 10) >= 0 : compare(c, p.row()) >= 0;
        if (inside) inside = compare(c, p.row(), upper ? 11 : 1) <= 0;
        if (inside) c.set(3, 1); return compare(c, std::int8_t(c.r(3))) != 0;
    }
    bool attacking_row(int bottom, int top) {
        bool yes = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), bottom) < 0;
        if (!yes) yes = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), top) > 0;
        return compare(c, int(yes)) != 0;
    }
    void paired_message(unsigned id, std::uint32_t back, bool reverse = false) {
        const Player first{c, reverse ? player() : opponent()}, second{c, reverse ? opponent() : player()};
        leave(c, {{3, c.u32(actor.match() + 0x79f4)}, {4, id}, {5, actor.match()},
                  {6, c.u8(first.address + 0x2f)}, {7, first.side()}, {8, c.u8(second.address + 0x2f)}, {9, second.side()}, {10, 0}});
        c.call(0x10201180, back);
    }
    void player_message(unsigned id, std::uint32_t address, std::uint32_t back) {
        leave(c, {{3, c.u32(actor.match() + 0x79f4)}, {4, id}, {5, actor.match()}, {6, c.u8(address + 0x2f)},
                  {7, c.u8(address + 0x27)}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, back);
    }
    void side_message(unsigned id, std::uint32_t side, std::uint32_t back) {
        leave(c, {{3, c.u32(actor.match() + 0x79f4)}, {4, id}, {5, actor.match()}, {6, UINT32_MAX},
                  {7, side}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, back);
    }
    bool protected_action(std::uint32_t back) {
        leave(c, {{3, actor.match()}, {4, c.u8(at(0x47))}}); c.call(0x101ee530, back); return compare(c, std::int8_t(c.r(3))) != 0;
    }
    void condition_delta(std::uint32_t address, int amount) { c.set(3, std::uint32_t(std::int32_t(c.i16(address + 0x36)))); c.put16(address + 0x36, c.r(3) + std::uint32_t(amount)); }
    void clamp_bias(std::uint32_t address) {
        if (compare(c, c.i16(address + 0x1a0), 200) > 0) c.put16(address + 0x1a0, 200);
        else if (compare(c, c.i16(address + 0x1a0), -200) < 0) c.put16(address + 0x1a0, std::uint32_t(-200));
    }
    void publish_side() {
        c.set(6, actor.side()); c.set(5, actor.match()); compare(c, std::int8_t(c.r(6)), -1); c.put8(c.r(5) + 0x7921, c.r(6));
        if (std::int8_t(c.r(6)) == -1) return;
        const auto side = std::int32_t(std::int8_t(c.r(6))), opposite = int(side == 0);
        c.set(3, std::uint32_t(side)); c.set(4, c.r(5) + c.r(3)); c.set(3, c.u8(c.r(4) + 0x77bc)); c.put8(c.r(4) + 0x77bc, c.r(3) + 1);
        c.set(3, c.r(5) + unsigned(opposite)); if (compare(c, c.i8(c.r(3) + 0x77bc)) > 0) {
            c.set(4, c.r(5) + unsigned(opposite)); c.set(3, c.u8(c.r(4) + 0x77bc)); c.put8(c.r(4) + 0x77bc, c.r(3) - 10);
        }
    }
    void action_message(bool failed);
    void foul();
    void win();
    void lose();
    void initial_draw();
    void run();
};

void DirectAction::initial_draw() {
    c.set(5, 0x14f8b589); c.put16(at(0x48), 0);
    c.set_f(31, c.f32(player() + 0x50)); c.set_f(30, c.f32(player() + 0x50));
    const auto pair = [&](unsigned first, unsigned second) {
        const auto a = fp.fctiwz(1, c.f(31)), b = fp.fctiwz(0, c.f(30));
        c.put64(at(first), a); c.put64(at(second), b);
        const auto x = std::uint32_t(10000 - std::int32_t(std::int16_t(c.u32(at(first + 4)))));
        const auto y = std::uint32_t(10000 - std::int32_t(std::int16_t(c.u32(at(second + 4)))));
        c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(std::int16_t(c.u32(at(second + 4))))) <= 10000);
        const auto product = high(x * y, 0x14f8b589); sign_after_shift(product, 12);
        return std::uint32_t(std::int32_t(x * y) / 50000);
    };
    const auto initial = pair(0xc8, 0xc0); c.set(3, unsigned(std::int32_t(initial) < 0));
    if (compare(c, std::int32_t(initial)) == 0) { c.set(20, 0); return; }
    advance(0x10274230);
    // Both conversions narrow to signed halfwords before subtraction. The
    // product fits int32 and the quotient lies in[-19473,36582].
    const auto checked_bound = [&](std::int32_t limit) {
        const auto value = pair(0xc0, 0xc8); c.set(5, 0x14f8b589); c.set(3, std::uint32_t(limit));
        c.set(4, unsigned(std::int32_t(value) < 0)); compare(c, std::int32_t(value), limit);
    };
    checked_bound(65535); checked_bound(-65535);
    const auto a = fp.fctiwz(0, c.f(31)); c.set(3, c.u32(cursor));
    const auto b = fp.fctiwz(2, c.f(30)), d = fp.fctiwz(1, c.f(31));
    c.set(7, 0x14f8b589); c.set(5, 0x14f8b589); c.put64(at(0x78), a);
    const auto e = fp.fctiwz(0, c.f(30)); c.set(8, c.u16(c.r(3))); c.put64(at(0x80), b);
    c.set(3, std::uint32_t(10000 - std::int32_t(std::int16_t(c.u32(at(0x7c)))))); c.set(4, c.u32(seed));
    c.put64(at(0x88), d); c.put64(at(0x90), e);
    const auto product = c.r(3) * std::uint32_t(10000 - std::int32_t(std::int16_t(c.u32(at(0x84)))));
    const auto value = c.r(8) + c.r(4), bound = std::uint32_t(std::int32_t(product) / 50000);
    const auto h = high(product, 0x14f8b589); sign_after_shift(h, 12); c.set(7, bound);
    c.set(4, std::uint32_t(10000 - std::int32_t(std::int16_t(c.u32(at(0x8c))))));
    c.set(3, std::uint32_t(10000 - std::int32_t(std::int16_t(c.u32(at(0x94))))));
    const auto sign_product = high(c.r(4) * c.r(3), 0x14f8b589); sign_after_shift(sign_product, 12);
    c.set(6, std::uint32_t(divide_signed<4>(value, bound))); c.set(5, c.r(6) * bound);
    c.set(3, std::uint32_t(sign_product >> 12)); c.set(4, unsigned(sign_product < 0));
    const auto remainder = value - c.r(5); c.set(20, compare(c, std::int32_t(c.r(3) + c.r(4))) > 0 ? remainder : 0u - remainder);
}

void DirectAction::action_message(bool failed) {
    const auto action = byte(0x47); compare_logical(c, std::uint32_t(action), 46);
    unsigned id = 0x1fe9; std::uint32_t back = failed ? 0x1027684c : 0x10275044;
    switch (action) {
    case 1: id = 0x1ff1; back = failed ? 0x1027679c : 0x10274f94; break;
    case 12: id = 0x1ff4; back = failed ? 0x10276820 : 0x10275018; break;
    case 19: case 33: id = 0x1fee; back = failed ? 0x10276718 : 0x10274f10; break;
    case 21: id = 0x1ff0; back = failed ? 0x102767c8 : 0x10274fc0; break;
    case 34: id = 0x1ff2; back = failed ? 0x10276744 : 0x10274f3c; break;
    case 41: id = 0x1fef; back = failed ? 0x10276770 : 0x10274f68; break;
    case 42: id = 0x1feb; back = failed ? 0x10276694 : 0x10274e8c; break;
    case 43: id = 0x1fec; back = failed ? 0x102766c0 : 0x10274eb8; break;
    case 44: id = 0x1fed; back = failed ? 0x102766ec : 0x10274ee4; break;
    case 45: id = 0x1fea; back = failed ? 0x10276668 : 0x10274e60; break;
    case 46: id = 0x1ff3; back = failed ? 0x102767f4 : 0x10274fec; break;
    }
    paired_message(id, back);
}

void DirectAction::run() {
    initial_draw(); leave(c, {{3, player()}, {5, 0}, {4, 0}});
    c.put8(at(0x47), 0); c.set(23, std::uint32_t(std::int32_t(std::int16_t(c.r(20))))); c.set(20, 0);
    c.put8(at(0x46), 0); c.put8(at(0x45), 0); c.put8(at(0x40), 0);
    enter(c, 0x102745b8, {player()}); has_possession(c, player());
    if (compare(c, std::int8_t(c.r(3))) == 0) return;
    if (compare(c, std::int8_t(c.r(26))) != 0) {
        leave(c, {{3, actor.match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}}); c.set_f(1, c.f32(singles + 0x110)); c.call(0x101d6860, 0x102745e8);
        c.set(3, actor.match()); c.set(4, c.r(3) + std::uint32_t(std::int32_t(std::int8_t(actor.side()))));
        c.set(3, c.u8(c.r(4) + 0x77bc)); c.put8(c.r(4) + 0x77bc, c.r(3) + 10);
    }
    c.set(3, actor.match()); c.set(30, std::uint32_t(std::int32_t(c.i16(player() + 0x13e)))); c.set(18, c.u8(c.r(3) + 0x791f));
    const auto phase_bound = std::uint32_t(std::int32_t(std::int8_t(c.r(18))) * 50);
    if (compare(c, std::int32_t(phase_bound)) == 0) c.set(17, 0);
    else {
        advance(0x10274648); c.set(3, 65535); compare(c, std::int32_t(phase_bound), 65535);
        c.set(3, std::uint32_t(-65535)); compare(c, std::int32_t(phase_bound), -65535);
        c.set(4, phase_bound); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(18))))); c.set(6, c.u16(c.u32(cursor))); c.set(5, c.r(6) + c.u32(seed));
        c.set(3, std::uint32_t(divide_signed<4>(c.r(5), c.r(4))) * c.r(4)); const auto residual = c.r(5) - c.r(3);
        c.set(17, compare(c, std::int32_t(phase_bound)) > 0 ? residual : 0u - residual);
    }
    c.set(3, std::uint32_t(std::int32_t(c.i16(player() + 0x1a0)))); c.set(4, std::uint32_t(std::int32_t(std::int16_t(c.r(23)))));
    c.set(18, c.r(3) + std::uint32_t(std::int32_t(std::int16_t(c.r(30)))) - c.r(17) - c.r(4));
    range(18, 17, 0x10274804, 0x102748dc);
    c.set(3, at(0x44)); c.put32(at(0x38), c.r(3)); c.set(23, std::uint32_t(std::int32_t(std::int16_t(c.r(17))))); c.put32(at(0x3c), 0);
    leave(c, {{3, actor.match()}, {4, player()}, {5, actor.side()}, {6, 16}, {7, at(0x46)}, {8, at(0x45)}, {9, at(0x48)}, {10, at(0x47)}});
    c.call(0x101eaf60, 0x10274978); c.set(30, c.r(3));
    leave(c, {{3, actor.match()}, {4, player()}, {5, 16}, {6, opponent()}, {7, std::uint32_t(byte(0x47))}, {8, 0}}); c.call(0x101f5350, 0x1027499c);
    if (compare_logical(c, opponent()) != 0) {
        bool query = compare(c, c.i16(at(0x48)), std::int16_t(c.r(23))) >= 0;
        if (!query) { advance(0x102749d4); query = compare(c, modulo(5)) == 0; }
        if (query) {
            leave(c, {{3, actor.match()}, {4, opponent()}, {5, player()}, {6, c.u8(at(0x47))}, {7, UINT32_MAX}, {8, at(0x42)}, {9, at(0x41)}, {10, at(0x40)}});
            c.call(0x101f5050, 0x10274a34); c.put8(at(0x43), c.r(3)); c.set(3, actor.match());
            if (compare(c, c.i8(c.r(3) + 0x7931)) != 0 && opposite_area(opponent())) {
                bool complain = protected_action(0x10274b10);
                if (!complain) complain = compare(c, byte(0x40)) != 0;
                if (!complain) { c.set(18, std::uint32_t(std::int32_t(c.i16(opponent() + 0x32)))); range(18, 17, 0x10274b58, 0x10274c30, true); complain = compare(c, std::int32_t(c.r(17)), -10) < 0; }
                if (complain) { advance(0x10274cbc); if (compare(c, modulo(3)) != 0) player_message(0x2168, opponent(), 0x10274d1c); }
            }
        } else c.put8(at(0x43), 0);
        bool event_only = false;
        if (compare(c, byte(0x43)) != 0) {
            advance(0x10274d54);
            if (compare(c, modulo(30)) == 0 && compare(c, byte(0x44)) == 0) {
                c.put32(at(0x38), c.u8(at(0x42))); c.put32(at(0x3c), c.u8(at(0x41)));
                leave(c, {{3, actor.match()}, {4, std::uint32_t(byte(0x47))}, {5, opponent()}, {6, player()}, {7, c.u8(at(0x44))}, {8, c.u8(at(0x43))}, {9, c.u8(player() + 0x105)}, {10, c.u8(player() + 0x106)}});
                c.call(0x101d68f0, 0x10274dd8); event_only = true;
            } else { foul(); return; }
        }
        if (!event_only && compare(c, byte(0x40)) != 0) {
            bool incident = opposite_area(player()); if (!incident) incident = compare(c, byte(0x44)) != 0;
            if (incident) c.set(20, 1);
        }
        c.put16(opponent() + 0x19c, std::uint32_t(byte(0x47))); c.set(3, c.u8(opponent() + 4)); c.put8(opponent() + 4, c.r(3) + 1);
        leave(c, {{3, actor.match()}, {4, opponent()}}); c.call(0x101e3d50, 0x10275e80);
        bool specialized = compare(c, std::int32_t(c.r(3) & 0x30)) != 0;
        if (specialized) { leave(c, {{3, actor.match()}, {4, opponent()}}); c.call(0x101e3d50, 0x10275e94); specialized = compare(c, std::int32_t(std::int16_t(c.r(3))) & 0x880) != 0; }
        condition_delta(opponent(), specialized ? -59 : -70);
    }
    if (compare_logical(c, opponent()) == 0 || compare(c, std::int16_t(c.r(23)), c.i16(at(0x48))) > 0) win();
    else lose();
}

void DirectAction::win() {
    if (!opposite_area(player())) {
        c.set(17, std::uint32_t(std::int32_t(c.i16(player() + 0x32)))); range(17, 18, 0x10275fc4, 0x1027609c, true);
        if (compare(c, std::int32_t(c.r(18)), -10) < 0) {
            c.set_f(1, c.f32(singles + 0x18)); enter(c, 0x10276114, {player()}); scale_intensity(c, player(), c.f(1));
            side_message(0x211d, unsigned(std::int8_t(actor.side()) == 0), 0x1027614c);
        }
    }
    publish_side(); c.set(3, std::uint32_t(std::int32_t(std::int16_t(c.r(23) - std::uint32_t(std::int32_t(c.i16(at(0x48))))))));
    c.put16(player() + 0x1a0, std::uint32_t(std::int32_t(c.i16(player() + 0x1a0))) + c.r(3)); clamp_bias(player());
    if (compare_logical(c, opponent()) != 0) { c.set(3, c.u8(opponent() + 0x2c)); c.put8(opponent() + 0x2c, c.r(3) + c.u8(at(0x45))); }
    c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x791f), -1) == 0) return;
    if (compare(c, c.i16(player() + 0x19c), 16) == 0) return;
    if (compare(c, std::int8_t(c.r(26))) != 0) player_message(0x2133, player(), 0x1027625c);
    else if (compare_logical(c, opponent()) != 0) {
        leave(c, {{3, c.u32(actor.match() + 0x79f4)}, {4, 0x216d}, {5, actor.match()}, {6, c.u8(player() + 0x2f)}, {7, actor.side()}, {8, c.u8(opponent() + 0x2f)}, {9, unsigned(std::int8_t(actor.side()) == 0)}, {10, 0}}); c.call(0x10201180, 0x102762a0);
    } else {
        enter(c, 0x102762ac, {player()}); current_role(c, player()); bool ordinary = compare(c, std::int32_t(c.r(3) & 1)) == 0;
        if (ordinary) ordinary = !attacking_row(3, 8);
        if (ordinary) { leave(c, {{3, actor.match()}, {4, actor.side()}, {5, c.u8(player() + 0x105)}, {6, c.u8(player() + 0x106)}, {7, 1}}); c.call(0x101e4020, 0x10276318); ordinary = compare(c, std::int8_t(c.r(3))) == 0; }
        player_message(ordinary ? 0x21de : 0x1f40, player(), ordinary ? 0x10276348 : 0x10276374);
    }
    c.set(3, actor.match()); c.put8(c.r(3) + 0x791f, 0);
}

void DirectAction::foul() {
    compare(c, byte(0x43));
    c.put32(at(0x38), c.u8(at(0x40)));
    leave(c, {{3, actor.match()}, {4, opponent()}, {5, player()}, {6, at(0x43)},
              {7, c.u8(at(0x42))}, {8, c.u8(at(0x41))}, {9, c.u8(at(0x44))}, {10, at(0x4c)}});
    c.call(0x101f5110, 0x10274e14); action_message(false); c.set(26, UINT32_MAX);
    const auto severity = byte(0x43);
    if (compare(c, severity, 4) == 0) c.set(26, 0x1ff8);
    else if (severity < 4 && compare(c, severity, 2) == 0) c.set(26, 0x1ff9);
    else if (severity < 4 && severity > 2) c.set(26, 0x1ff7);
    else if (severity > 4 && compare(c, severity, 6) < 0) c.set(26, 0x1ff6);
    else {
        advance(0x102750b4); const auto value = sample(); c.set(5, value); c.set(6, c.u16(c.u32(cursor)));
        c.set(3, std::uint32_t(byte(0x41))); c.set(4, sign_after_shift(std::int32_t(std::uint32_t(high(value, std::int32_t(0x92492493))) + value), 2));
        c.set(4, std::uint32_t(std::int32_t(value) % 7)); if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(4) + 2)) > 0) c.set(26, 0x210a);
    }
    if (compare(c, c.u8(at(0x43)), 7) == 0) { c.set(3, actor.match()); c.set(23, 0x1f79); c.put8(c.r(3) + 0x7928, 7); }
    else if (opposite_area(player())) {
        if (compare(c, std::int16_t(c.r(26)), -1) == 0) {
            advance(0x10275210); const auto choice = binary(true);
            if (compare(c, choice, 1) == 0) side_message(0x20ec, actor.side(), 0x102752a8);
            else if (choice < 1 && compare(c, choice) >= 0) side_message(0x20e9, actor.side(), 0x1027527c);
        }
        c.set(3, actor.match()); c.set(23, 0x1f7a); c.put8(c.r(3) + 0x7928, 5);
        c.set(3, actor.match()); c.put32(c.r(3) + 0x3110, player()); condition_delta(opponent(), -500);
    } else { c.set(3, actor.match()); c.set(23, 0x1f79); c.put8(c.r(3) + 0x7928, 6); }
    const bool penalty = compare(c, std::int16_t(c.r(23)), 0x1f7a) == 0;
    c.set(4, c.u8(player() + 0x105)); c.set(3, actor.match()); c.put8(c.r(3) + 0x791d, c.r(4));
    c.set(4, c.u8(player() + 0x106)); c.set(3, actor.match()); c.put8(c.r(3) + 0x791e, c.r(4));
    c.set(4, actor.side()); c.set(3, actor.match()); c.put8(c.r(3) + 0x7924, c.r(4)); c.set(3, actor.match() + 0x10000); c.put32(c.r(3) - 0x2000, 0);
    if (penalty) side_message(c.r(23), std::uint32_t(std::int32_t(std::int8_t(actor.side()))), 0x1027534c);
    if (compare(c, std::int16_t(c.r(26)), -1) > 0) {
        bool specific = compare(c, c.u8(at(0x47)), 44) == 0;
        if (!specific) specific = compare(c, c.u8(at(0x47)), 47) == 0;
        if (specific && compare(c, byte(0x40)) != 0) {
            const bool last = compare(c, c.u8(at(0x47)), 47) == 0;
            paired_message(last ? 0x21da : 0x21d9, last ? 0x102753b0 : 0x102753dc, true);
            const auto span = std::uint8_t(c.u8(at(0x43)) - 3);
            if (compare_logical(c, span, 3) <= 0 || compare(c, std::int16_t(c.r(23)), 0x1f7a) == 0) c.put8(opponent() + 0x16, 1);
        }
        advance(0x10275424);
        if (compare(c, binary(true)) == 0) {
            c.set(18, std::uint32_t(byte(0x41))); range(18, 17, 0x10275488, 0x10275560, true);
            advance(0x102755e4); const auto choice = modulo(20);
            if (compare(c, std::int32_t(c.r(17)), choice) > 0) {
                const auto controller = [&] {
                    const bool second = compare(c, std::int8_t(actor.side())) != 0;
                    c.set(3, c.u32(c.u32(actor.match() + 0x3118) + (second ? 0x20 : 0x1c))); return c.u32(c.r(3) + 0x2c);
                };
                if (compare_logical(c, controller()) != 0) {
                    c.set(3, controller());
                    if (compare(c, c.u8(c.r(3) + 0x119), 8) == 0) {
                        leave(c, {{3, c.u32(actor.match() + 0x79f4)}, {4, 0x20f5}, {5, actor.match()}, {6, 0}, {7, 0}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, 0x102756b0);
                    }
                }
            }
            player_message(0x2109, opponent(), 0x102756d8);
        }
        player_message(c.r(26), opponent(), 0x10275700);
        bool discipline = compare(c, c.u8(at(0x43)), 6) == 0;
        if (!discipline) discipline = compare_logical(c, std::uint8_t(c.u8(at(0x43)) - 3), 1) <= 0;
        if (!discipline) discipline = compare(c, c.u8(at(0x43)), 5) == 0;
        if (discipline) { leave(c, {{3, actor.match()}, {4, opponent()}, {5, 0}}); c.call(0x101f1530, 0x1027573c); }
    }
    if (compare(c, std::int16_t(c.r(23)), 0x1f7a) != 0) {
        leave(c, {{3, actor.match()}, {4, actor.side()}, {5, c.u8(actor.match() + 0x7928)}, {6, 0}}); c.call(0x101f3a30, 0x10275760); c.set(27, c.r(3));
        if (compare(c, std::int32_t(c.r(27))) == 0) side_message(c.r(23), std::uint32_t(std::int32_t(std::int8_t(actor.side()))), 0x10275794);
        else {
            const auto taker_message = [&](unsigned id, std::uint32_t back) {
                leave(c, {{3, c.u32(actor.match() + 0x79f4)}, {4, id}, {5, actor.match()}, {6, c.u8(c.r(27) + 0x2f)}, {7, std::uint32_t(std::int32_t(std::int8_t(actor.side())))}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, back);
            };
            if (compare(c, c.u8(at(0x43)), 7) == 0) taker_message(0x21e2, 0x102757d0);
            else side_message(c.r(23), std::uint32_t(std::int32_t(std::int8_t(actor.side()))), 0x10275800);
            if (compare(c, std::int16_t(c.r(23)), 0x1f79) == 0 && compare(c, byte(0x40)) == 0) {
                if (compare(c, c.u8(at(0x43)), 7) == 0) {
                    if (attacking_row(4, 7) && !opposite_area(player())) taker_message(0x21e1, 0x10275958);
                } else {
                    c.set(3, c.u8(player() + 0x105)); const auto col = std::int8_t(c.r(3));
                    bool wide = compare(c, col, 2) < 0;
                    if (!wide) wide = compare(c, col, 6) > 0;
                    if (compare(c, int(wide)) != 0 && attacking_row(4, 7)) taker_message(0x21df, 0x10275a04);
                    else if (attacking_row(4, 7)) taker_message(0x21e0, 0x10275a80);
                }
            }
        }
    }
    if (!protected_action(0x10275a8c)) {
        const auto card_or_penalty = [&] {
            return compare_logical(c, std::uint8_t(c.u8(at(0x43)) - 3), 3) <= 0 || compare(c, std::int16_t(c.r(23)), 0x1f7a) == 0;
        };
        if (compare(c, byte(0x40)) != 0) {
            player_message(0x20dd, player(), 0x10275ac8);
            bool caution = compare(c, c.u8(at(0x43)), 4) == 0;
            if (!caution) caution = compare(c, c.u8(at(0x43)), 5) == 0;
            if (caution) c.put8(opponent() + 0x16, 1);
            if (card_or_penalty()) { advance(0x10275b28); if (compare(c, binary()) == 0) side_message(0x20d9, unsigned(std::int8_t(actor.side()) == 0), 0x10275b8c); }
        } else {
            advance(0x10275bb0);
            if (compare(c, modulo(9)) == 0) {
                side_message(0x20df, unsigned(std::int8_t(actor.side()) == 0), 0x10275c24);
                advance(0x10275c44); if (compare(c, modulo(3)) == 0) side_message(0x20e0, unsigned(std::int8_t(actor.side()) == 0), 0x10275cb4);
                if (card_or_penalty()) { advance(0x10275cf4); if (compare(c, binary()) == 0) side_message(0x20d9, unsigned(std::int8_t(actor.side()) == 0), 0x10275d58); }
            }
        }
        leave(c, {{3, actor.match()}, {4, opponent()}}); c.set_f(1, c.f32(at(0x4c))); c.call(0x101d6780, 0x10275d68);
        enter(c, 0x10275d74, {opponent(), 0}); resolve_incident_response(c, opponent(), 0);
    }
    condition_delta(opponent(), -25);
}

void DirectAction::lose() {
    if (compare_logical(c, opponent()) == 0) {
        report_assertion(c, {0x10277560, strings + 0x8f, strings + 0x9c, strings + 0xa9, at(0x58), at(0x50), 0x4b18}); return;
    }
    bool foul_comment = protected_action(0x10276398);
    if (!foul_comment) foul_comment = compare(c, byte(0x40)) != 0;
    if (!foul_comment) {
        c.set(18, std::uint32_t(std::int32_t(c.i16(opponent() + 0x32)))); range(18, 17, 0x102763e0, 0x102764b8, true);
        foul_comment = compare(c, std::int32_t(c.r(17)), -10) < 0;
    }
    if (foul_comment) foul_comment = defensive_area(opponent());
    if (foul_comment) { advance(0x102765f0); foul_comment = compare(c, binary()) == 0; }
    if (foul_comment) {
        action_message(true); advance(0x1027686c); const auto choice = modulo(50);
        compare_logical(c, std::uint32_t(choice), 10);
        const bool even = choice >= 0 && choice <= 10 && choice % 2 == 0;
        const auto id = even ? 0x20ebu : choice == 1 ? 0x20eau : 0x20e8u;
        const auto back = even ? 0x102768f8u : choice == 1 ? 0x10276934u : 0x10276970u;
        side_message(id, unsigned(std::int8_t(c.u8(opponent() + 0x27)) == 0), back);
    } else {
        const auto action = byte(0x47); compare_logical(c, std::uint32_t(action), 46);
        switch (action) {
        case 1:
            enter(c, 0x102769ac, {opponent(), std::uint32_t(std::int32_t(std::int16_t(std::uint32_t(std::int32_t(c.i16(at(0x48)))) - c.r(23))))});
            react_to_contact_failure(c, opponent(), std::int16_t(c.r(4))); if (compare(c, std::int8_t(c.r(3))) == 0) return; break;
        case 19: case 33: case 45: paired_message(0x1f62, 0x10276a0c); break;
        case 34: case 44: paired_message(0x2150, 0x10276a38); break;
        case 41: paired_message(0x1f61, 0x102769e0); break;
        case 42: case 43: paired_message(0x1f64, 0x10276a64); break;
        case 21: case 46: {
            advance(0x10276a88); c.set(4, sample()); c.set(5, c.u16(c.u32(cursor))); c.set(3, std::uint32_t(high(c.r(4), 0x2aaaaaab)));
            c.put32(at(0x78), 0x43300000); c.set_f(1, c.f32(singles + 0xe4)); c.set_f(0, c.f32(opponent() + 0xb0)); c.set_f(2, c.f64(doubles + 0x80));
            c.set_f(3, fp.fadds(c.f(1), c.f(0))); c.set_f(0, c.f32(player() + 0xb0));
            c.put32(at(0x7c), std::uint32_t(std::int32_t(c.r(4)) % 6) ^ 0x80000000); c.set_f(1, c.f64(at(0x78)));
            c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(1, fp.fadds(c.f(3), c.f(1))); fp.fcmp(0, c.f(1), c.f(0));
            const bool shorter = (c.cr(0) & 8) != 0; paired_message(shorter ? 0x2164 : 0x1f5d, shorter ? 0x10276b1c : 0x10276b48); break;
        }
        }
    }
    enter(c, 0x10276b50, {opponent()}); acquire_ball(c, opponent());
    c.set(3, std::uint32_t(std::int32_t(c.i16(at(0x48)))) - c.r(23)); c.put16(opponent() + 0x1a0, c.r(3) - 200); clamp_bias(opponent());
    const auto action = byte(0x47); c.set(23, 0); c.set(4, 100);
    if (compare(c, action, 33) == 0 || (action > 33 && compare(c, action, 45) == 0)) c.set(4, 250);
    else if (action < 33 && compare(c, action, 19) == 0) c.set(4, 400);
    c.set(3, std::uint32_t(std::int32_t(c.i16(opponent() + 0x1a0))));
    bool request = compare(c, std::int32_t(c.r(3)), std::int16_t(c.r(4))) < 0;
    if (request) { c.set(3, actor.match()); request = compare(c, c.u8(c.r(3) + 0x791f), 4) != 0; }
    if (request) { c.set(3, actor.match()); request = compare(c, c.i8(c.r(3) + 0x791f), -1) != 0; }
    if (request) {
        leave(c, {{3, actor.match()}, {4, c.u8(player() + 0x105)}, {5, c.u8(player() + 0x106)}, {6, 1}, {7, unsigned(std::int8_t(actor.side()) == 0)}, {8, 0}, {9, 1}, {10, 0}}); c.call(0x101f1100, 0x10276c34);
        c.set(3, actor.match());
        if (compare(c, c.i8(c.r(3) + 0x7928)) == 0) {
            advance(0x10276c6c, [&] { keep(c, {{19, c.u8(opponent() + 0x10f)}, {20, c.u8(opponent() + 0x110)}}); });
            c.set(5, sample()); c.set(6, c.u16(c.u32(cursor))); c.set(4, 0x66666667);
            const auto first = c.r(5), next = c.u32(cursor) + 2; c.set(3, next); c.put32(cursor, next);
            c.set(4, std::uint32_t(high(first, 0x66666667))); const auto order = compare_logical(c, next, c.u32(end));
            const auto first_high = std::int32_t(c.r(4)); sign_after_shift(first_high, 2); c.set(3, unsigned(first_high < 0)); c.set(21, std::uint32_t(std::int32_t(first) % 10));
            if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, 0x10276cc4); c.put32(seed, c.r(3) & 0xffff); }
            c.set(3, c.u32(cursor)); c.set(4, 0x66666667); c.set(5, c.u32(seed)); c.set(7, c.u16(c.r(3)) + c.r(5));
            const auto second_high = high(c.r(7), 0x66666667); sign_after_shift(second_high, 2); c.set(5, unsigned(second_high < 0));
            c.set(4, std::uint32_t(std::int32_t(c.r(7)) / 10) * 10); c.set(22, std::uint32_t(std::int32_t(c.r(7)) % 10));
            // These bit tricks in the source are two strict signed comparisons.
            // They reduce the bound to1..3, so its zero and wide arms cannot run.
            const auto better = unsigned(std::int32_t(std::int8_t(c.r(19))) > std::int32_t(c.r(21)));
            const auto other = unsigned(std::int32_t(std::int8_t(c.r(20))) > std::int32_t(c.r(22)));
            const auto before_y = std::uint32_t(std::int32_t(std::int8_t(c.r(20)))) ^ c.r(22);
            c.set(4, before_y & std::uint32_t(std::int32_t(std::int8_t(c.r(20)))));
            c.set(5, std::uint32_t(std::int32_t(before_y) >> 1));
            c.set(6, std::uint32_t(std::int32_t(std::int8_t(c.r(20)))));
            const auto bound = 3u - better - other; c.s.wr<1>(reg::xer_ca, 1); compare(c, std::int32_t(bound)); advance(0x10276d60);
            c.set(3, 65535); compare(c, std::int32_t(bound), 65535); c.set(3, std::uint32_t(-65535)); compare(c, std::int32_t(bound), -65535);
            c.set(6, std::uint32_t(std::int32_t(std::int8_t(c.r(20))))); c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(19)))));
            c.set(3, (c.r(6) ^ c.r(22))); c.set(8, c.u32(cursor)); c.set(5, c.r(6)); const auto x = c.r(4) ^ c.r(21), y = c.r(3);
            c.set(7, c.u16(c.r(8))); c.set(4, c.u32(seed)); c.set(6, other); c.set(9, c.r(7) + c.r(4)); c.set(8, bound);
            c.set(7, std::uint32_t(divide_signed<4>(c.r(9), bound)));
            c.set(6, std::uint32_t(std::int32_t(y) >> 1));
            c.set(4, c.r(6) - (y & std::uint32_t(std::int32_t(std::int8_t(c.r(20))))));
            c.set(5, c.r(7) * bound); c.set(3, other);
            // Final srawi is the signed xor of trait19 and its draw.
            sign_after_shift(std::int32_t(x), 1); c.s.wr<1>(reg::xer_ca, 1);
            compare(c, std::int32_t(bound)); c.set(17, c.r(9) - c.r(5)); c.put8(opponent() + 0x2c, c.r(17)); c.set(23, 1);
        } else if (compare(c, std::int8_t(c.r(20))) != 0) { enter(c, 0x10277074, {player()}); publish_incident(c, player()); }
    } else {
        c.set(3, actor.match()); bool clear_phase = compare(c, c.i8(c.r(3) + 0x791f), 4) < 0;
        if (clear_phase) { c.set(3, actor.match()); clear_phase = compare(c, c.i8(c.r(3) + 0x791f), -1) > 0; }
        if (clear_phase) { c.set(3, actor.match()); c.put8(c.r(3) + 0x791f, 0); }
        bool first_side = compare(c, c.i8(opponent() + 0x27)) == 0;
        if (first_side) { c.set(3, actor.match()); first_side = compare(c, c.i8(c.r(3) + 0x791f), -1) != 0; }
        const auto row = actor.row(); std::int32_t projected;
        if (first_side) projected = compare(c, row, 9) < 0 ? row + 3 : 11;
        else projected = compare(c, row, 3) > 0 ? row - 3 : 0;
        enter(c, first_side ? 0x10277108 : 0x10277148, {opponent(), c.u8(player() + 0x105), c.u8(player() + 0x106), 4, std::uint32_t(std::int32_t(std::int8_t(projected)))});
        set_heading(c, opponent(), c.r(4), c.r(5), 4, c.r(7));
        c.set(3, actor.match()); const auto elapsed = c.i16(c.r(3) + 0x7948); const auto product = high(std::uint32_t(std::int32_t(elapsed)), 0x2e8ba2e9);
        sign_after_shift(product, 1); c.set(3, unsigned(product < 0)); c.set(17, std::uint32_t(std::int32_t(std::int8_t(elapsed / 11))));
        const bool special = compare(c, byte(0x47), 21) == 0;
        const auto attempts = special ? 7u : 4u, successes = special ? 8u : 5u;
        if (compare_logical(c, c.u8(opponent() + attempts)) != 0) {
            c.set(5, c.u8(opponent() + successes)); c.set(3, c.u8(opponent() + attempts)); c.set(4, 0x43300000);
            c.put32(at(0x7c), c.r(5)); c.set_f(2, c.f64(doubles + 0xf0)); c.put32(at(0x78), 0x43300000); c.set_f(1, c.f64(doubles + 0xf0));
            c.set_f(0, c.f64(at(0x78))); c.put32(at(0x84), c.r(3)); c.set_f(2, fp.fsubs(c.f(0), c.f(2))); c.put32(at(0x80), 0x43300000);
            c.set_f(0, c.f64(at(0x80))); c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(30, fp.fdivs(c.f(2), c.f(0)));
        } else c.set_f(30, c.f32(singles));
        advance(special ? 0x102771ec : 0x10277350); c.set(5, sample()); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(17)))));
        const auto high20 = high(c.r(5), 0x66666667); sign_after_shift(high20, 3); c.set(4, std::uint32_t(std::int32_t(c.r(5)) % 20));
        if (compare(c, std::int32_t(c.r(3)), std::int8_t(c.r(4) + (special ? 55 : 65))) > 0) {
            const bool second = compare(c, c.i8(opponent() + 0x27)) != 0; c.set(3, c.u32(opponent() + 0x1a4) + 0x10000);
            bool announce = compare(c, c.i8(c.r(3) - (second ? 0x1fc3 : 0x1f8d))) == 0;
            if (announce) { c.set_f(0, c.f64(doubles + 0x288)); fp.fcmp(0, c.f(30), c.f(0)); announce = (c.cr(0) & 4) != 0; }
            if (announce) announce = compare_logical(c, c.u8(opponent() + (special ? 8 : 6)), special ? 3 : 1) > 0;
            if (announce && special) announce = compare_logical(c, c.u8(opponent() + 9)) != 0;
            if (announce) announce = compare(c, c.i16(opponent() + 0x36), 6750) > 0;
            if (announce) announce = compare(c, c.i8(opponent() + 0x100)) == 0;
            if (announce) { paired_message(special ? 0x21ce : 0x21cd, special ? 0x102772d4 : 0x1027742c); c.put8(opponent() + 0x100, 1); }
        }
    }
    c.set(4, c.u8(opponent() + 5)); c.put8(opponent() + 5, c.r(4) + 1);
    enter(c, 0x10277448, {opponent()}); current_role(c, opponent()); bool normal = compare(c, std::int32_t(c.r(3) & 4)) == 0;
    if (normal) { enter(c, 0x10277458, {opponent()}); current_role(c, opponent()); normal = compare(c, std::int32_t(c.r(3) & 2)) == 0; }
    const bool delayed = compare(c, std::int8_t(c.r(23))) != 0; c.set(3, std::uint32_t(std::int32_t(c.i16(opponent() + 0x36))));
    c.put16(opponent() + 0x36, c.r(3) + (delayed ? 120u : 200u) * (normal ? 2u : 1u));
    c.set(3, c.u8(player() + 0x2c)); compare(c, std::int8_t(c.r(23))); c.put8(player() + 0x2c, c.r(3) + c.u8(at(0x46)));
    if (delayed) { advance(0x102774e0); if (compare(c, modulo(7)) == 0) player_message(0x1f41, opponent(), 0x10277548); }
}
}

void resolve_direct_action(Cpu& c, std::uint32_t player, std::uint32_t mode) {
    DirectAction action(c, player, mode); action.run();
}
}

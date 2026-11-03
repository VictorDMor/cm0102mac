#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
class PeriodicBalance {
    Cpu& c;
    Frame frame;
    Fpu fp;
    Player actor;
    const std::uint32_t doubles, floats, seed, begin, end, cursor;
    std::uint32_t at(unsigned offset) const { return frame.at(offset); }
    unsigned count(unsigned offset) const { return c.u8(actor.address + offset); }
    void advance(std::uint32_t back) {
        const auto next = c.u32(cursor) + 2;
        c.set(3, next);
        const auto order = compare_logical(c, next, c.u32(end));
        c.put32(cursor, next);
        if (order > 0) {
            c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff);
        }
    }
    void initial_limits();
    bool role(std::uint32_t back, unsigned mask) {
        enter(c, back, {actor.address}); current_role(c, actor.address);
        return compare(c, int(c.r(3) & mask)) != 0;
    }
    int balance() const { return c.i16(actor.address + 0x36); }
    void change(int amount) {
        c.set(3, extended(c.i16(actor.address + 0x36)));
        c.put16(actor.address + 0x36, c.r(3) + std::uint32_t(amount));
    }
    int shifted(int value, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << bits) - 1)));
        return value >> bits;
    }
    static int high(std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    }
    std::uint32_t narrow_draw(unsigned bound_register, std::uint32_t back);
    void fixed_decay(unsigned bound, std::uint32_t back);
    int opposing_phase();
    void keeper_balance();
    void keeper_team_floor(unsigned multiplier, unsigned base, bool upper);
    void midfield_balance();
    void midfield_ratios();
    void ratio_adjustment(unsigned numerator, unsigned denominator, std::uint32_t back, unsigned coefficient);
    bool no_events(bool include_ten, bool include_twelve = false);
    void recover(unsigned lower, unsigned upper = 0);
    void late_balance(unsigned kind);
public:
    PeriodicBalance(Cpu& cpu, std::uint32_t player)
        : c(cpu), frame(c, 0x10237b50, 0x80, {.first_gpr = 24, .gpr_offset = -0x30, .first_fpr = 30}),
          fp(c), actor{c, player}, doubles(c.toc(-0x51e8)), floats(c.toc(-0x51e0)),
          seed(c.toc(-0x7b90)), begin(c.toc(-0x7b8c)), end(c.toc(-0x7b88)), cursor(c.toc(-0x7b84)) {
        keep(c, {{31, player}, {28, doubles}, {29, seed}, {27, begin}, {26, end}, {30, cursor}, {25, floats}});
    }
    void run();
};

void PeriodicBalance::initial_limits() {
    c.set(3, count(0xc));
    bool may_decay = compare(c, int(c.r(3) + count(0x10)), 2) < 0;
    if (may_decay) may_decay = compare_logical(c, count(6), 7) < 0;
    if (may_decay) may_decay = compare_logical(c, count(3), 4) < 0;
    if (may_decay) may_decay = compare_logical(c, count(0x11), 6) < 0;
    if (may_decay) {
        advance(0x10237c18);
        c.set(3, c.u32(cursor)); c.set(4, 0x057619f1);
        const auto random_seed = c.u32(seed);
        c.set(5, c.u16(c.r(3))); c.set(3, extended(c.i16(actor.address + 0x36)));
        const auto draw = c.r(5) + random_seed; c.set(5, draw);
        const auto high = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(draw)) * 0x057619f1) >> 32);
        const auto quotient = high >> 5;
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 31));
        c.set(4, quotient < 0);
        const auto remainder = draw - std::uint32_t(quotient + int(c.r(4))) * 1500;
        c.set(4, remainder);
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(remainder + 8250)) > 0) {
            c.set(3, extended(c.i16(actor.address + 0x36))); c.put32(at(0x48), 0x43300000);
            c.set_f(2, c.f64(doubles + 0x80)); c.set_f(3, c.f64(doubles + 0x2b0));
            c.put32(at(0x4c), (c.r(3) - 8250) ^ 0x80000000);
            c.set_f(0, c.f64(doubles + 0x2a8)); c.set_f(1, c.f64(at(0x48)));
            c.set_f(1, fp.fsub(c.f(1), c.f(2)));
            c.set_f(0, fp.multiply_add(0x10237c8c, c.f(3), c.f(1), c.f(0)));
            c.put64(at(0x40), fp.fctiwz(0, c.f(0)));
            c.put16(actor.address + 0x36, c.u32(at(0x44)));
        }
    }
    if (compare_logical(c, count(1), 50) > 0) {
        c.set(3, count(1)); c.set(4, count(3));
        const auto balance = c.i16(actor.address + 0x36);
        c.set(4, c.r(4) * 400); c.set(3, c.r(4) + (c.r(3) - 50) * 20 + 6500);
        if (compare(c, std::int32_t(c.r(3)), balance) > 0) {
            c.set(3, count(1)); c.set(4, count(3)); c.set(3, c.r(4) * 400 + (c.r(3) - 50) * 20 + 6500);
        } else c.set(3, extended(c.i16(actor.address + 0x36)));
        c.put16(actor.address + 0x36, c.r(3));
        if (compare_logical(c, count(2)) != 0) {
            c.set(5, count(1)); c.set(4, 0x43300000); c.set(3, count(2));
            c.put32(at(0x44), c.r(5)); c.set_f(1, c.f64(doubles + 0xf0));
            c.put32(at(0x40), c.r(4)); c.set_f(2, c.f64(doubles + 0xf0));
            c.set_f(0, c.f64(at(0x40))); c.put32(at(0x4c), c.r(3));
            c.set_f(3, fp.fsubs(c.f(0), c.f(1))); c.set_f(0, c.f64(doubles + 0x30));
            c.put32(at(0x48), 0x43300000); c.set_f(1, c.f64(at(0x48)));
            c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(4, fp.fdivs(c.f(3), c.f(1)));
            fp.fcmp(0, c.f(4), c.f(0));
            if (c.cr(0) & 4) {
                c.set(3, extended(c.i16(actor.address + 0x36))); c.put32(at(0x40), 0x43300000);
                c.set_f(3, c.f64(doubles + 0x80)); c.put32(at(0x44), c.r(3) ^ 0x80000000);
                c.set_f(1, c.f32(floats + 0xb0)); c.set_f(2, c.f64(at(0x40))); c.set_f(0, c.f32(floats + 0xac));
                c.set_f(2, fp.fsubs(c.f(2), c.f(3))); c.set_f(0, fp.multiply_add(0x10237d78, c.f(1), c.f(4), c.f(0)));
                fp.fcmp(0, c.f(2), c.f(0));
                if (c.cr(0) & 8) {
                    c.set(3, extended(c.i16(actor.address + 0x36))); c.put16(actor.address + 0x36, c.r(3) + 5);
                }
            }
        }
    }
    if (compare_logical(c, count(0x10)) != 0) {
        c.set(3, count(0x10)); const auto balance = c.i16(actor.address + 0x36);
        c.set(3, c.r(3) * 425 + 6000);
        std::uint32_t selected;
        if (compare(c, std::int32_t(c.r(3)), balance) > 0) {
            const auto attempts = count(0x10); c.set(3, attempts * 425); selected = c.r(3) + 6000;
        } else selected = extended(c.i16(actor.address + 0x36));
        c.put16(actor.address + 0x36, selected);
    }
    if (compare_logical(c, count(0xc)) != 0) {
        c.set(3, count(0xc));
        if (compare(c, int(c.r(3) + count(0x10)), 1) > 0) {
            c.set(4, count(0xc)); c.set(3, count(0x10)); const auto balance = c.i16(actor.address + 0x36);
            c.set(3, (c.r(4) + c.r(3)) * 375 + 6500);
            std::uint32_t selected;
            if (compare(c, std::int32_t(c.r(3)), balance) > 0) {
                c.set(3, count(0xc)); const auto attempts = count(0x10);
                c.set(3, (c.r(3) + attempts) * 375); selected = c.r(3) + 6500;
            } else selected = extended(c.i16(actor.address + 0x36));
            c.put16(actor.address + 0x36, selected);
        }
    }
}

void PeriodicBalance::run() {
    if (compare(c, c.i8(actor.address + 0x2c)) > 0) {
        c.set(3, count(0x2c)); c.put8(actor.address + 0x2c, c.r(3) - 1);
    } else if (compare(c, c.i8(actor.address + 0x2c)) < 0) c.put8(actor.address + 0x2c, 0);
    c.set_f(1, c.f32(floats + 0x10));
    enter(c, 0x10237bc0, {actor.address}); scale_intensity(c, actor.address, c.f(1));
    initial_limits();
    if (role(0x10237e34, 1)) { keeper_balance(); return; }
    if (role(0x102386a4, 2)) {
        fixed_decay(75, 0x102386cc);
        if (no_events(true) && compare(c, balance(), 7500) > 0) change(-10);
        if (compare(c, opposing_phase()) == 0) recover(5750);
        return;
    }
    if (role(0x102387b8, 4)) { midfield_balance(); return; }
    if (role(0x10238f10, 8)) {
        fixed_decay(500, 0x10238f38);
        if (no_events(false) && compare(c, balance(), 7500) > 0) change(-10);
        if (compare(c, opposing_phase()) == 0) {
            if (!no_events(true)) recover(5750, 6750);
            else recover(6250);
        }
        return;
    }
    if (role(0x10239080, 16)) { fixed_decay(1000, 0x102390a8); late_balance(16); return; }
    if (role(0x10239160, 32)) { fixed_decay(500, 0x10239188); late_balance(32); return; }
    if (role(0x10239280, 64)) { fixed_decay(150, 0x102392a8); late_balance(64); }
}

// The two byte-derived bounds never leave [0,65025]. The original wide
// sampler arms are unreachable even when the two byte reads differ.
std::uint32_t PeriodicBalance::narrow_draw(unsigned bound_register, std::uint32_t back) {
    if (compare(c, std::int32_t(c.r(bound_register))) == 0) return 0;
    advance(back);
    c.set(3, 65535); compare(c, std::int32_t(c.r(bound_register)), 65535);
    c.set(3, std::uint32_t(-65535)); compare(c, std::int32_t(c.r(bound_register)), -65535);
    c.set(3, c.u32(cursor)); const auto positive = compare(c, std::int32_t(c.r(bound_register))) > 0;
    const auto random_seed = c.u32(seed); c.set(3, c.u16(c.r(3)) + random_seed);
    const auto bound = c.r(bound_register);
    const auto quotient = std::uint32_t(divide_signed<4>(c.r(3), bound));
    const auto remainder = c.r(3) - quotient * bound;
    return positive ? remainder : 0u - remainder;
}

void PeriodicBalance::keeper_team_floor(unsigned multiplier, unsigned base, bool upper) {
    const auto side = std::int8_t(actor.side()); c.set(4, actor.match());
    const auto current = balance(); c.set(4, c.r(4) + 0xe03c);
    c.set(3, c.u8(c.r(4) + unsigned(side == 0) * 0x36 + 3) * multiplier + base);
    const auto order = compare(c, std::int32_t(c.r(3)), current);
    std::uint32_t selected;
    if (upper ? order < 0 : order > 0) {
        const auto reread_side = std::int8_t(actor.side()); c.set(3, actor.match());
        c.set(3, c.r(3) + 0xe03c + unsigned(reread_side == 0) * 0x36);
        c.set(3, c.u8(c.r(3) + 3) * multiplier); selected = c.r(3) + base;
    } else selected = extended(c.i16(actor.address + 0x36));
    c.put16(actor.address + 0x36, selected);
}

void PeriodicBalance::keeper_balance() {
    c.set(3, count(0xf)); keep(c, {{24, c.r(3) * count(0xf)}});
    keep(c, {{25, narrow_draw(24, 0x10237e74)}});
    if (compare(c, std::int32_t(c.r(25))) == 0 && compare(c, balance(), 7350) < 0) change(1);
    if (compare(c, c.i8(actor.address + 0x1f), -1) != 0 &&
        compare(c, c.i8(actor.address + 0x1f), 30) > 0) {
        if (compare_logical(c, count(0xf)) == 0) recover(7000);
        return;
    }
    keeper_team_floor(500, 7000, true);
    if (compare_logical(c, count(0xf)) == 0) {
        keeper_team_floor(300, 6000, false);
        const auto side = std::int8_t(actor.side()); c.set(3, actor.match());
        c.set(3, c.r(3) + 0xe03c + unsigned(side == 0) * 0x36);
        unsigned limit;
        if (compare_logical(c, c.u8(c.r(3) + 3), 2) < 0) {
            const auto reread_side = std::int8_t(actor.side()); c.set(3, actor.match());
            c.set(3, c.r(3) + 0xe03c + unsigned(reread_side == 0) * 0x36); limit = c.u8(c.r(3) + 3);
        } else limit = 2;
        c.set(3, limit * 250); c.set(4, extended(c.i16(actor.address + 0x36)));
        if (compare(c, std::int32_t(c.r(4)), int(c.r(3) + 7000)) < 0) change(1);
        return;
    }
    if (compare(c, balance(), 4250) <= 0) return;
    if (compare_logical(c, count(0xf), 1) > 0) {
        keep(c, {{25, count(0xf)}}); keep(c, {{24, narrow_draw(25, 0x102381fc)}});
        if (compare(c, std::int32_t(c.r(24))) != 0) {
            if (compare_logical(c, count(0xf), 2) > 0 && compare(c, balance(), 7750) > 0)
                c.put16(actor.address + 0x36, 7750);
            else if (compare(c, balance(), 8750) > 0) change(-25);
            else if (compare(c, balance(), 7250) > 0) {
                c.set(3, count(0xf)); const auto value = balance();
                c.put16(actor.address + 0x36, std::uint32_t(value) - c.r(3));
            } else change(-1);
            return;
        }
    }
    const auto side = std::int8_t(actor.side()); c.set(3, actor.match());
    c.set(4, extended(c.i16(actor.address + 0x36)));
    c.set(3, c.r(3) + 0xe03c + unsigned(side == 0) * 0x36);
    c.set(3, c.u8(c.r(3) + 3) * 250);
    if (compare(c, std::int32_t(c.r(4)), int(c.r(3) + 6250)) <= 0) return;
    c.set(4, actor.match()); c.set(3, 0x2e8ba2e9);
    keep(c, {{24, extended(c.i16(c.r(4) + 0x7948))}});
    const auto time_bound = [&] {
        const auto part = shifted(high(extended(std::int16_t(c.r(24))), 0x2e8ba2e9), 1);
        const auto quotient = part + int(part < 0);
        c.s.wr<1>(reg::xer_ca, std::uint32_t(quotient) <= 90);
        return 90 - quotient;
    };
    auto bound = time_bound(); c.set(3, std::int16_t(c.r(24)) < 0);
    if (compare(c, bound) == 0) keep(c, {{25, 0}});
    else {
        advance(0x10238448);
        c.set(4, 0x2e8ba2e9); c.set(3, 65535); bound = time_bound();
        c.set(4, std::int16_t(c.r(24)) < 0); compare(c, bound, 65535);
        c.set(4, 0x2e8ba2e9); c.set(3, std::uint32_t(-65535)); bound = time_bound();
        c.set(4, std::int16_t(c.r(24)) < 0); compare(c, bound, -65535);
        // Signed-halfword time gives 90-trunc(time/11) in [-2888,3068].
        // The original's third wide sampler arm is therefore unreachable.
        c.set(4, 0x2e8ba2e9); c.set(5, c.u32(cursor)); c.set(3, 0x2e8ba2e9);
        c.set(6, c.u16(c.r(5))); c.set(4, std::uint32_t(high(extended(std::int16_t(c.r(24))), 0x2e8ba2e9)));
        c.set(5, c.u32(seed)); c.set(6, c.r(6) + c.r(5));
        c.set(4, std::uint32_t(shifted(std::int32_t(c.r(4)), 1))); c.set(5, std::int32_t(c.r(4)) < 0);
        c.set(4, c.r(4) + c.r(5)); c.set(5, 90 - c.r(4));
        c.s.wr<1>(reg::xer_ca, c.r(4) <= 90);
        c.set(4, std::uint32_t(divide_signed<4>(c.r(6), c.r(5))) * c.r(5));
        bound = time_bound(); c.set(3, std::int16_t(c.r(24)) < 0);
        const auto remainder = c.r(6) - c.r(4);
        keep(c, {{25, compare(c, bound) > 0 ? remainder : 0u - remainder}});
    }
    if (compare(c, std::int32_t(c.r(25))) == 0) change(-1);
}

void PeriodicBalance::fixed_decay(unsigned bound, std::uint32_t back) {
    advance(back); c.set(4, c.u32(cursor));
    const auto multiplier = bound == 75 || bound == 150 ? 0x1b4e81b5 : 0x10624dd3;
    c.set(3, multiplier); const auto random_seed = c.u32(seed);
    c.set(4, c.u16(c.r(4)) + random_seed);
    const auto bits = bound == 75 ? 3u : bound == 150 ? 4u : bound == 500 ? 5u : 6u;
    const auto part = shifted(high(c.r(4), multiplier), bits); c.set(3, part < 0);
    const auto remainder = c.r(4) - std::uint32_t(part + int(c.r(3))) * bound;
    if (compare(c, std::int32_t(remainder)) == 0) change(-1);
}

int PeriodicBalance::opposing_phase() {
    const auto upper = compare(c, std::int8_t(actor.side())) == 0;
    c.set(3, actor.match() + 0x10000);
    return c.i8(c.r(3) - (upper ? 0x1f8d : 0x1fc3));
}

bool PeriodicBalance::no_events(bool include_ten, bool include_twelve) {
    if (compare_logical(c, count(6)) != 0) return false;
    if (compare_logical(c, count(9)) != 0) return false;
    if (compare_logical(c, count(3)) != 0) return false;
    if (include_ten && compare_logical(c, count(0x10)) != 0) return false;
    return !include_twelve || compare_logical(c, count(0xc)) == 0;
}

void PeriodicBalance::recover(unsigned lower, unsigned upper) {
    if (compare(c, balance(), int(lower)) < 0) change(upper ? 5 : 1);
    else if (upper && compare(c, balance(), int(upper)) < 0) change(1);
}

void PeriodicBalance::late_balance(unsigned kind) {
    bool reduced = compare_logical(c, count(0xc)) == 0;
    if (reduced) reduced = compare_logical(c, count(0x10)) == 0;
    if (reduced && kind == 64) reduced = compare_logical(c, count(0xb)) == 0;
    if (reduced) {
        bool empty = compare_logical(c, count(3)) == 0;
        if (empty) empty = compare_logical(c, count(kind == 64 ? 9 : 0xb)) == 0;
        if (compare(c, balance(), !empty && kind == 64 ? 7750 : 7250) > 0) change(empty ? -10 : -5);
        return;
    }
    if (kind == 16 || compare_logical(c, count(0xc)) == 0) return;
    const auto events = count(0xc); c.set(4, extended(c.i16(actor.address + 0x36))); c.set(3, events * 500);
    if (compare(c, std::int32_t(c.r(4)), int(c.r(3) + 6750)) < 0) {
        const auto reread_events = count(0xc); c.set(3, extended(c.i16(actor.address + 0x36)));
        c.put16(actor.address + 0x36, c.r(3) + std::uint32_t(std::int16_t(reread_events * 5)));
    }
}

void PeriodicBalance::ratio_adjustment(unsigned numerator, unsigned denominator, std::uint32_t back, unsigned coefficient) {
    if (compare_logical(c, count(denominator), 4) <= 0) return;
    c.set(5, count(numerator)); c.set(4, 0x43300000); c.set(3, count(denominator));
    c.put32(at(0x3c), c.r(5)); c.set_f(1, c.f64(doubles + 0xf0)); c.put32(at(0x38), c.r(4));
    c.set_f(2, c.f64(doubles + 0xf0)); c.set_f(0, c.f64(at(0x38))); c.put32(at(0x44), c.r(3));
    c.set_f(3, fp.fsubs(c.f(0), c.f(1))); c.set_f(0, c.f64(doubles + 0x200));
    c.put32(at(0x40), 0x43300000); c.set_f(1, c.f64(at(0x40)));
    c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(30, fp.fdivs(c.f(3), c.f(1)));
    fp.fcmp(0, c.f(30), c.f(0));
    if (!(c.cr(0) & 8)) return;
    const bool attacking = role(back, 0x880);
    c.set_f(0, c.f64(doubles + 0x200)); c.set_f(1, c.f64(doubles + (attacking ? coefficient : 0x110)));
    c.set_f(0, fp.fsub(c.f(30), c.f(0))); c.set_f(0, fp.fmul(c.f(1), c.f(0)));
    c.set_f(0, fp.frsp(c.f(0))); c.set_f(31, fp.fsubs(c.f(31), c.f(0)));
}

void PeriodicBalance::midfield_ratios() {
    c.set(3, count(4));
    if (compare(c, int(c.r(3) + count(7)), 9) <= 0) return;
    c.set(7, count(5)); c.set(5, 0x43300000); c.set(6, count(8));
    c.set(4, count(4)); c.set(3, count(7)); c.set(6, (c.r(7) + c.r(6)) ^ 0x80000000);
    c.put32(at(0x38), c.r(5)); c.set(3, c.r(4) + c.r(3)); c.set_f(2, c.f64(doubles + 0x80));
    c.set(3, c.r(3) ^ 0x80000000); c.put32(at(0x3c), c.r(6)); c.set_f(1, c.f64(doubles + 0x80));
    c.set_f(0, c.f64(at(0x38))); c.put32(at(0x44), c.r(3)); c.set_f(2, fp.fsubs(c.f(0), c.f(2)));
    c.set(3, count(9)); c.put32(at(0x40), 0x43300000); const auto events = count(6);
    c.set_f(0, c.f64(at(0x40))); const bool activity = compare(c, int(c.r(3) + events)) != 0;
    c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(31, fp.fdivs(c.f(2), c.f(0)));
    if (activity) {
        c.set(4, count(9)); c.set(3, count(6)); c.put32(at(0x48), 0x43300000);
        const auto total = c.r(4) + c.r(3); c.set_f(1, c.f64(doubles + 0x80));
        c.set_f(2, c.f64(doubles + 0x2b8)); c.put32(at(0x4c), total ^ 0x80000000);
        c.set_f(0, c.f64(at(0x48))); c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
        c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.set_f(0, fp.frsp(c.f(0)));
        c.set_f(31, fp.fadds(c.f(31), c.f(0)));
    }
    ratio_adjustment(5, 4, 0x10238934, 0x2c0);
    ratio_adjustment(8, 7, 0x102389d0, 0x138);
    const auto phase = opposing_phase(); c.set(3, std::uint32_t(phase));
    const auto scored = count(0xc); c.set(3, c.r(3) * 250);
    const bool add_score = compare_logical(c, scored) != 0;
    c.s.wr<1>(reg::xer_ca, c.r(3) <= 7250); c.set(4, extended(std::int16_t(7250 - c.r(3))));
    if (add_score) c.set(4, extended(std::int16_t(c.r(4) + std::uint32_t(std::int16_t(count(0xc) * 500)))));
    if (compare_logical(c, count(0x10)) != 0)
        c.set(4, extended(std::int16_t(c.r(4) + std::uint32_t(std::int16_t(count(0x10) * 250)))));
    c.set_f(0, c.f64(doubles + 0x2c8)); fp.fcmp(0, c.f(31), c.f(0));
    if (c.cr(0) & 8) {
        c.set(3, extended(c.i16(actor.address + 0x36)));
        if (compare(c, std::int32_t(c.r(3)), std::int16_t(c.r(4))) > 0) c.put16(actor.address + 0x36, c.r(4));
    } else {
        c.set_f(0, c.f64(doubles + 0x2d0)); fp.fcmp(0, c.f(31), c.f(0));
        if (c.cr(0) & 4) {
            c.set(3, extended(c.i16(actor.address + 0x36)));
            if (compare(c, std::int32_t(c.r(3)), std::int16_t(c.r(4))) < 0) c.put16(actor.address + 0x36, c.r(4));
        }
    }
}

void PeriodicBalance::midfield_balance() {
    fixed_decay(75, 0x102387e0); midfield_ratios();
    if (no_events(true) && compare(c, balance(), 7500) > 0) change(-10);
    if (compare(c, opposing_phase()) == 0) {
        if (!no_events(true, true)) { recover(5750, 6750); return; }
        if (compare_logical(c, count(2), 20) > 0) {
            c.set(5, count(1)); c.set(4, 0x43300000); c.set(3, count(2));
            c.put32(at(0x3c), c.r(5)); c.set_f(1, c.f64(doubles + 0xf0)); c.put32(at(0x38), c.r(4));
            c.set_f(2, c.f64(doubles + 0xf0)); c.set_f(0, c.f64(at(0x38))); c.put32(at(0x44), c.r(3));
            c.set_f(3, fp.fsubs(c.f(0), c.f(1))); c.set_f(0, c.f64(doubles + 0x30));
            c.put32(at(0x40), 0x43300000); c.set_f(1, c.f64(at(0x40)));
            c.set_f(1, fp.fsubs(c.f(1), c.f(2))); c.set_f(1, fp.fdivs(c.f(3), c.f(1)));
            fp.fcmp(0, c.f(1), c.f(0));
            if (c.cr(0) & 4) { recover(5750, 6750); return; }
        }
        recover(5750); return;
    }
    if (compare(c, opposing_phase(), 1) == 0) {
        if (!no_events(true, true)) recover(6250);
        else if (compare(c, balance(), 7250) > 0) change(-1);
        return;
    }
    const auto deduction = std::uint32_t(std::int16_t(opposing_phase() * 250));
    c.s.wr<1>(reg::xer_ca, deduction <= 7750);
    auto limit = std::int16_t(7750 - deduction);
    if (compare(c, limit, 6750) < 0) limit = 6750;
    c.set(3, extended(c.i16(actor.address + 0x36)));
    if (compare(c, std::int32_t(c.r(3)), limit) <= 0) return;
    if (role(0x10238da4, 0x880)) {
        if (!no_events(true, true)) change(-1);
        else {
            const auto phase = opposing_phase(); c.set(3, std::uint32_t(phase));
            const auto current = balance(); c.put16(actor.address + 0x36, std::uint32_t(current) - c.r(3));
        }
    } else if (!no_events(true, true)) {
        const auto phase = opposing_phase(); c.set(3, std::uint32_t(phase));
        const auto current = balance(); c.put16(actor.address + 0x36, std::uint32_t(current) - c.r(3));
    } else {
        const auto phase = opposing_phase(); c.set(3, std::uint32_t(phase));
        const auto current = balance(); c.set(3, c.r(3) * 5);
        c.put16(actor.address + 0x36, std::uint32_t(current) - c.r(3));
    }
}
}
void update_periodic_balance(Cpu& c, std::uint32_t player) { PeriodicBalance(c, player).run(); }
}

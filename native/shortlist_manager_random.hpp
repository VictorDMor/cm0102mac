#pragma once
// The game's random-number table as SHORTLIST_MANAGER.CPP's inlined draws
// use it. The compiler inlines the game's random(n) (the storage group's
// sample_range, 0x10356e40) and the table step at every site, with the same
// instructions but registers of the site's own; what differs per site is
// passed in: the return addresses of its calls, its divw, its stack
// temporaries and the scratch registers it leaves (WideDraw's schedules or
// a site's own callback).
#include "shortlist_manager_detail.hpp"

namespace cm::aot::shortlist_manager::detail {
namespace routine_address {
constexpr std::uint32_t rand = 0x1077b770;          // the C library's rand(): a leaf, writes only r0, r3, r4
constexpr std::uint32_t sample_range = 0x10356e40;  // the module's random(n) (storage group)
}

// What the original holds in r3/r4 after the step when the table does not
// wrap (rand overwrites both when it does).
struct StepRegisters {
    unsigned next = 3;            // the stepped cursor (addi rN,rM,2)
    bool previous_in_r4 = false;  // the cursor before the step (lwz r4; addi r3,r4,2)
    bool slot_in_r3 = false;      // the cursor variable's address (lwz r3,-0x7b84(r2))
};

// The table (slot::random_*): the addresses of its cursor, end, start and
// salt variables. The cursor steps two bytes through uint16 values; past
// the end it goes back to the start with a new salt, rand() & 0xffff. A
// draw is the value under the cursor plus the salt (0..0x1fffe).
struct RandomTable {
    Cpu& c;
    std::uint32_t cursor, end, table, salt;

    explicit RandomTable(Cpu& c)
        : RandomTable(c, c.toc(slot::random_cursor), c.toc(slot::random_end), c.toc(slot::random_table),
                      c.toc(slot::random_salt)) {}
    // From the registers a routine keeps the addresses in.
    RandomTable(Cpu& c, std::uint32_t cursor, std::uint32_t end, std::uint32_t table, std::uint32_t salt)
        : c(c), cursor(cursor), end(end), table(table), salt(salt) {}

    std::uint32_t position() const { return c.u32(cursor); }
    std::uint32_t entry() const { return c.u16(position()); }
    std::uint32_t salt_value() const { return c.u32(salt); }
    std::uint32_t value() const { return entry() + salt_value(); }

    // The step every draw starts with (cmplw into cr0): `rand_back` is the
    // address after the site's bl to rand. True when the table wrapped.
    bool advance(std::uint32_t rand_back, StepRegisters registers = {}) const {
        const std::uint32_t previous = position(), next = previous + 2;
        c.set(registers.next, next);
        if (registers.previous_in_r4) c.set(4, previous);
        if (registers.slot_in_r3) c.set(3, cursor);
        c.put32(cursor, next);
        if (compare_logical(c, next, c.u32(end)) <= 0) return false;
        c.put32(cursor, table);
        c.put32(salt, Program{c}.call(routine_address::rand, rand_back) & 0xffff);
        return true;
    }
};

// A quotient by a constant as the compiler forms it (a magic multiply and
// shift, plus the sign bit, which it leaves in a register).
struct Quotient {
    std::int32_t value;
    std::uint32_t sign;
};
// v / 65535: magic 0x80008001, shift 15.
inline Quotient by_65535(std::uint32_t v) {
    const auto t = std::int32_t(std::uint32_t(multiply_high(std::int32_t(0x80008001), std::int32_t(v))) + v) >> 15;
    const auto sign = std::uint32_t(t) >> 31;
    return {std::int32_t(std::uint32_t(t) + sign), sign};
}
// (sampled + 5) / 10: magic 0x66666667, shift 2 (the finer draw rounded to
// the coarser step).
inline Quotient rounded_tenth(std::int32_t sampled) {
    const auto t = multiply_high(0x66666667, std::int32_t(std::uint32_t(sampled) + 5)) >> 2;
    const auto sign = std::uint32_t(t) >> 31;
    return {std::int32_t(std::uint32_t(t) + sign), sign};
}

// Whether n fits the table's 16 bits (|n| <= 0xffff), by the original's two
// compares, which build their bounds in r3 (lis r3,1 / lis r3,-1).
inline bool within_table(Cpu& c, std::int32_t n) {
    c.set(3, 0x10000);
    if (compare(c, n, 0xffff) > 0) return false;
    c.set(3, 0xffff0000);
    return compare(c, n, -0xffff) >= 0;
}
// The draw's remainder by n (the divw at `division`), negated unless n is
// positive (cmpwi n,0 ... neg ... ble). Leaves the draw in r3.
inline std::int32_t narrow_below(Cpu& c, const RandomTable& random, std::uint32_t division, std::int32_t n) {
    const bool positive = compare(c, n) > 0;
    const std::uint32_t x = result(c, random.value());
    const auto rest = std::uint32_t(modulo(division, std::int32_t(x), n));
    return std::int32_t(positive ? rest : 0u - rest);
}

// A draw over a range beyond 16 bits, up to the call that refines it:
// share = n / 65535.0 (left in f1), part = trunc(share * (x mod 65535)),
// and the refinement's range, n * 10 / 65535 (32-bit wrapping). The
// int-to-double temporaries and fctiwz's result are stack locals.
struct WideDraw {
    std::uint32_t entry, salt, x;  // the table's uint16, the salt, their sum
    Quotient whole;                // x / 65535
    std::uint32_t multiple;        // whole * 65535
    std::int32_t tenfold;          // n * 10
    Quotient coarse;               // tenfold / 65535: the refinement's range
    double share;
    std::int32_t part;

    // The two schedules most sites leave in r4-r8 at the refining call.
    void leave_a(Cpu& c) const {
        leave(c, {{4, coarse.sign}, {5, multiple}, {6, whole.sign}, {7, salt}, {8, entry}});
    }
    void leave_b(Cpu& c) const { leave(c, {{4, multiple}, {5, 0xffff}, {6, 0x80008001}, {7, x}, {8, entry}}); }
};
inline WideDraw wide_draw(Cpu& c, Fpu& fpu, const RandomTable& random, std::int32_t n, std::uint32_t n_at,
                          std::uint32_t rest_at, std::uint32_t part_at) {
    WideDraw d{};
    d.share = fpu.fdiv(fpu.fsub(stacked_integer(c, n_at, signed_word(n)), k::signed_bias), k::u16_max);
    d.entry = random.entry();
    d.salt = random.salt_value();
    d.x = d.entry + d.salt;
    d.whole = by_65535(d.x);
    d.multiple = std::uint32_t(d.whole.value) * 0xffff;
    d.tenfold = std::int32_t(std::uint32_t(n) * 10);
    d.coarse = by_65535(std::uint32_t(d.tenfold));
    const double rest = fpu.fsub(stacked_integer(c, rest_at, signed_word(std::int32_t(d.x - d.multiple))), k::signed_bias);
    const auto bits = fpu.fctiwz(0, fpu.fmul(d.share, rest));
    c.put64(part_at, bits);
    d.part = std::int32_t(std::uint32_t(bits));
    c.set_f(1, d.share);
    return d;
}
// After the refining call: part + (sampled + 5) / 10, at most n - 1 (cmpw
// into cr0; quirk: no lower bound). Most sites leave the tenth in r3 and
// its sign in r4; `limit_in_r4` sites the sign in r3 and n - 1 in r4.
inline std::int32_t wide_total(Cpu& c, std::int32_t part, std::int32_t sampled, std::int32_t n, bool limit_in_r4) {
    const auto tenth = rounded_tenth(sampled);
    const auto limit = std::int32_t(std::uint32_t(n) - 1);
    if (limit_in_r4) leave(c, {{3, tenth.sign}, {4, std::uint32_t(limit)}});
    else leave(c, {{3, std::uint32_t(tenth.value)}, {4, tenth.sign}});
    const auto total = std::int32_t(std::uint32_t(part) + std::uint32_t(tenth.value));
    return compare(c, total, limit) > 0 ? limit : total;
}

// One inlined random(n) site: a draw in [0, n) (0 for n = 0; for a
// negative n within 16 bits, in (n, 0]; quirk: only the upper bound is
// clamped, so a wide negative n gives n - 1) into register
// `result_register`, kept there across the refining call as the original
// keeps it.
struct DrawSite {
    std::uint32_t rand_back;    // after the bl to rand
    std::uint32_t division;     // the narrow path's divw
    std::uint32_t sample_back;  // after the bl to sample_range (wide path)
    std::uint32_t n_at, rest_at, part_at;  // the wide path's stack temporaries
    bool limit_in_r4 = false;   // wide_total's schedule
    StepRegisters step = {};
};
// `at_sample(const WideDraw&)` leaves the site's scratch registers at the
// refining call (WideDraw::leave_a or leave_b, or the site's own).
template <class AtSample>
std::int32_t draw_below(Cpu& c, Fpu& fpu, const RandomTable& random, const DrawSite& site, std::int32_t n,
                        unsigned result_register, AtSample&& at_sample) {
    const auto set = [&](std::int32_t value) {
        c.set(result_register, std::uint32_t(value));
        return value;
    };
    if (!nonzero(c, n)) return set(0);
    random.advance(site.rand_back, site.step);
    if (within_table(c, n)) return set(narrow_below(c, random, site.division, n));
    const auto d = wide_draw(c, fpu, random, n, site.n_at, site.rest_at, site.part_at);
    set(d.part);
    at_sample(d);
    const auto sampled =
        std::int32_t(Program{c}.call(routine_address::sample_range, site.sample_back, {std::uint32_t(d.coarse.value)}));
    return set(wide_total(c, std::int32_t(c.r(result_register)), sampled, n, site.limit_in_r4));
}
}

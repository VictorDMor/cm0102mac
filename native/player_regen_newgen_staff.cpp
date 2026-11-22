// PLAYER_REGEN.CPP, block `newgen_staff`: 0x102e27e0, which regenerates a retired non-player staff person in place: his flags and
// links are reset, his ability and reputation halfwords drawn, his age by his job, a manager's attribute bytes by the reputation of his
// club, his birth day, his personality tier and (with a club and a job that takes a contract) a new contract end.
//
// The order of the draws is the whole contract: 61 inline draw sites, none in a loop, and which of them run depends on the data. The
// order of the original, which this file keeps:
//   1. a draw below trunc(0.75 * the staff record's halfword +0x26) (none when that is 0), the staff record's +0x24 = it + 1;
//      then below 750 (+0x28), below 200 (+0x2c), below 50 (+0x2a);
//   2. the age: one draw by the job (table `age_arms`);
//   3. a manager (job 1) with a club and a staff record: the attribute bytes by the club's reputation (tables `arm_a`..`arm_d`):
//      each in the table's order, a difference of two draws taking both draws before it stores, a tested attribute a test draw
//      then one of two draws;
//   4. below 365 (the birth day), below 125 (the personality tier);
//   5. with a club, a staff record and a job whose contract test (0x1007e4f0) passes: below 28 (the day), below 6 (the year).
//
// Fidelity. Every draw is the pooled generator's, at the original's refill return address (the call trace pairs them). The two
// floating-point steps are the original's: a double (fsub, fmul, fctiwz) for the bound of the first draw, and single precision (fsubs,
// fsubs, fmuls, fcmpo, fctiwz) for the comparison of +0x24 with half of +0x26 (the constant 0.5f from the unit's table at TOC -0x50a4).
// The bound lies within 0.75 * 32768 in magnitude, so the scaled form of Random (a bound of 0x10000 or more) and its call of
// 0x102da630 can never run. The job byte comes from the club side (+0x62) when the person has a club, else from the national side
// (+0x60) when he has a national link, else from the caller's r20 (`incoming_r20`: the caller 0x102d8920 holds the address of the pool's
// cursor cell, TOC -0x7b84, there for its whole run: its job byte is that address's low byte).
//
// Registers. The callees run with the arguments the original gives them (r3 and the registers the original sets just before the call);
// the callee-saved registers are mirrored (`Run::held`) so a callee's prologue saves the original's values. r4-r10 at return are what the
// last callee (0x100c3870) leaves, with r4 and r5 set as the original has them when the contract block is skipped after the personality
// draw (r4 the sum of that draw, r5 the cursor).
#include "player_regen_detail.hpp"
#include <array>
#include <span>

namespace cm::aot::player_regen {
namespace {
// Person record fields (0x70 bytes).
namespace p {
constexpr std::uint32_t id = 0x00, contract_year = 0x4e /* halfword, the year */, contract_day = 0x4c, birth_year = 0x36, birth_day = 0x34,
                        link_nation = 0x20, staff = 0x1c, club = 0x24, job_nation = 0x60, job_club = 0x62, role_5e = 0x5e, role_5f = 0x5f,
                        flag_6b = 0x6b, word_4 = 0x04, word_0c = 0x0c, word_28 = 0x28, word_2c = 0x2c, word_30 = 0x30,
                        age_5c = 0x5c, rating_61 = 0x61, contract_end_day = 0x54, contract_end_year = 0x56, contract_end_byte = 0x58;
}
// The staff record (the person's +0x1c).
namespace st {
constexpr std::uint32_t ability = 0x24 /* halfword */, potential = 0x26 /* halfword */, upper = 0x28, current = 0x2a, lower = 0x2c, personality = 0x43;
}
constexpr std::int32_t stats_cell = -0x7b3c, contracts_cell = -0x7b7c, kind_cell = -0x7b9c, news_cell = -0x7b48, humans_cell = -0x7a54,
                       single_constants = -0x50a4;
constexpr std::uint32_t half_single = 4, three_quarters = 0x38;  // offsets in the unit's tables

// Callees (the original's `bl` addresses + 4 are the return addresses below).
namespace fn {
constexpr std::uint32_t staff_reset = 0x10388cc0, kind_a = 0x100c4400, kind_b = 0x100c4cf0, kind_c = 0x100c53e0, humans = 0x1014b690,
                        stat_set = 0x10309850, stat_get = 0x103094b0, stat_level = 0x10309680, contracts_reset = 0x1007bd80,
                        birth_class = 0x100c6a10, contract_test = 0x1007e4f0, contract_clear = 0x10084a80, contract_month = 0x10089e40,
                        make_date = 0x107156f0, contract_of = 0x10082f20, contract_apply = 0x103727c0, finish = 0x100c3870;
}

// One run of the routine: the callee-saved registers as the original holds them (r19..r31, indexed by register number), set before every
// call and draw so a callee's prologue saves them.
struct Run {
    Cpu& c;
    Pool pool;
    std::array<std::uint32_t, 32> held{};
    explicit Run(Cpu& cpu) : c(cpu), pool(cpu) { for (unsigned n = 19; n < 32; ++n) held[n] = c.r(n); }
    void sync() { for (unsigned n = 19; n < 32; ++n) c.set(n, held[n]); }
    std::uint32_t call(std::uint32_t target, std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers) {
        sync();
        c.set(registers);
        const auto link = c.lr();
        c.call(target, back);
        c.set_lr(link);
        return c.r(3);
    }
    // One draw below `bound` (none for 0; the bounds here are below 0x10000 in magnitude: the scaled form never runs).
    std::int32_t draw(std::int32_t bound, std::uint32_t back) { sync(); return pool.draw(bound, back, refill_site::random_first); }
};

// One draw of a table: Random(modulus) + add, refilling (when the pool is at its end) with `back` as the return address.
struct Draw { std::int32_t modulus, add; std::uint32_t back; };

// The age of the regenerated person is a draw by his job byte: the birth year is the current year minus (Random(modulus) + offset),
// the jump table of the original (TOC -0x50bc, 11 words read from the executable): jobs 0, 5, 9 and every value outside 0..10 (the
// compare is unsigned, after extsb) take the default; 1 a manager, 2 and 3 share an arm.
inline constexpr Draw age_default{15, 39, 0x102e41a4};
inline constexpr std::array<Draw, 11> age_arms{{
    age_default,              // 0
    {20, 36, 0x102e2d8c},     // 1 (a manager: then his attribute bytes)
    {25, 40, 0x102e3f40},     // 2
    {25, 40, 0x102e3f40},     // 3
    {10, 37, 0x102e3fa4},     // 4
    age_default,              // 5
    {15, 35, 0x102e4008},     // 6
    {15, 35, 0x102e4070},     // 7
    {15, 32, 0x102e40d8},     // 8
    age_default,              // 9
    {25, 35, 0x102e4140},     // 10
}};

// A manager's attribute bytes (the staff record's +0x2f, +0x33, +0x34, +0x36, +0x39, +0x3d, +0x3b, +0x40, by the club's reputation: the
// four arms below). An attribute is one of: `plain` (the byte = Random(m) + a), `difference` (two draws below 100, the byte = the first
// minus the second, the first's remainder kept across the second draw) or `tested` (a draw below `test.modulus`: a nonzero value takes
// `first`, zero takes `second`, each one draw of its own; the test draw's own remainder is not stored).
enum class Form : std::uint8_t { plain, difference, tested };
struct Attribute {
    Form form;
    std::uint8_t offset;
    Draw first, second, test;
};
constexpr Attribute plain(std::uint8_t offset, std::int32_t modulus, std::int32_t add, std::uint32_t back) {
    return {Form::plain, offset, {modulus, add, back}, {}, {}};
}
constexpr Attribute difference(std::uint8_t offset, std::uint32_t first_back, std::uint32_t second_back) {
    return {Form::difference, offset, {100, 0, first_back}, {100, 0, second_back}, {}};
}
constexpr Attribute tested(std::uint8_t offset, Draw test, Draw nonzero, Draw zero) { return {Form::tested, offset, nonzero, zero, test}; }

// Club reputation (halfword +0x1ec) above 6250 and the club's flag byte +0x241 nonzero.
inline constexpr std::array<Attribute, 8> arm_a{{
    plain(0x2f, 11, 10, 0x102e2e18), plain(0x33, 20, 1, 0x102e2e7c), plain(0x34, 20, 1, 0x102e2ee0), plain(0x36, 20, 1, 0x102e2f44),
    difference(0x39, 0x102e2fa8, 0x102e2ffc), plain(0x3d, 20, 1, 0x102e3060), plain(0x3b, 20, 1, 0x102e30c4), plain(0x40, 15, 6, 0x102e3128)}};
// Above 6250 with the flag byte zero.
inline constexpr std::array<Attribute, 8> arm_b{{
    plain(0x2f, 20, 1, 0x102e3190), plain(0x33, 20, 1, 0x102e31f4), plain(0x34, 20, 1, 0x102e3258), plain(0x36, 20, 1, 0x102e32bc),
    difference(0x39, 0x102e3320, 0x102e3374), plain(0x3d, 20, 1, 0x102e33d8), plain(0x3b, 20, 1, 0x102e343c), plain(0x40, 11, 10, 0x102e34a0)}};
// Above 3250 up to 6250.
inline constexpr std::array<Attribute, 8> arm_c{{
    plain(0x2f, 20, 1, 0x102e350c), plain(0x33, 20, 1, 0x102e3570), plain(0x34, 20, 1, 0x102e35d4), plain(0x36, 20, 1, 0x102e3638),
    difference(0x39, 0x102e369c, 0x102e36f0),
    tested(0x3d, {5, 0, 0x102e3754}, {16, 5, 0x102e37ac}, {20, 1, 0x102e3804}),
    plain(0x3b, 20, 1, 0x102e3868),
    tested(0x40, {10, 0, 0x102e38cc}, {15, 1, 0x102e3924}, {15, 6, 0x102e398c})}};
// 3250 or less.
inline constexpr std::array<Attribute, 8> arm_d{{
    plain(0x2f, 20, 1, 0x102e39f4), plain(0x33, 20, 1, 0x102e3a58), plain(0x34, 20, 1, 0x102e3abc), plain(0x36, 20, 1, 0x102e3b20),
    difference(0x39, 0x102e3b84, 0x102e3bd8),
    tested(0x3d, {10, 0, 0x102e3c3c}, {15, 6, 0x102e3c94}, {20, 1, 0x102e3cfc}),
    difference(0x3b, 0x102e3d60, 0x102e3db4),
    tested(0x40, {100, 0, 0x102e3e18}, {15, 6, 0x102e3e70}, {10, 1, 0x102e3ed8})}};

// The personality tier: a draw below 125 (taken as a signed byte), the first threshold it is below gives the tier 0..8, else 9.
inline constexpr std::array<std::int32_t, 9> personality_limits{34, 40, 60, 90, 94, 96, 108, 111, 121};

void draw_attributes(Run& run, std::uint32_t person, std::span<const Attribute> arm) {
    auto& c = run.c;
    const auto store = [&](std::uint8_t offset, std::int32_t value) { c.put8(c.u32(person + p::staff) + offset, std::uint32_t(value)); };
    for (const auto& a : arm) {
        switch (a.form) {
        case Form::plain: store(a.offset, run.draw(a.first.modulus, a.first.back) + a.first.add); break;
        case Form::difference: {
            const auto first = run.draw(a.first.modulus, a.first.back);
            run.held[22] = std::uint32_t(first);  // r22 keeps the first remainder across the second draw
            store(a.offset, std::int32_t(std::uint32_t(first) - std::uint32_t(run.draw(a.second.modulus, a.second.back))));
            break;
        }
        case Form::tested: {
            const auto test = run.draw(a.test.modulus, a.test.back);
            if (test != 0) store(a.offset, run.draw(a.first.modulus, a.first.back) + a.first.add);
            else store(a.offset, run.draw(a.second.modulus, a.second.back) + a.second.add);
            break;
        }
        }
    }
}

// A manager with a club and a staff record: the attribute bytes by the club's reputation (halfword +0x1ec, signed) and its flag byte (+0x241).
void manager_attributes(Run& run, std::uint32_t person) {
    auto& c = run.c;
    const auto club = c.u32(person + p::club);
    if (club == 0) return;
    if (c.u32(person + p::staff) == 0) return;
    const auto reputation = sext16(c.u16(club + 0x1ec));
    if (reputation > 6250) {
        if (sext8(c.u8(club + 0x241)) != 0) draw_attributes(run, person, arm_a);
        else draw_attributes(run, person, arm_b);
        return;
    }
    if (reputation > 3250) draw_attributes(run, person, arm_c);
    else draw_attributes(run, person, arm_d);
}

// A contract end for a person with a club and a staff record whose job (the club side's byte) the contract manager says takes a contract: his
// contract is cleared and a new end date made: the day below 28 (+1), the month the club's contract record gives and the year plus a draw
// below 6 (+1); then the contract manager's holder is applied. `out` is the frame's date buffer (the original's sp + 0x38) the date routine fills.
void contract_end(Run& run, std::uint32_t person, std::uint32_t out) {
    auto& c = run.c;
    auto& h = run.held;
    const auto date = c.toc(toc::year_date);
    if (c.u32(person + p::club) == 0) return;
    if (c.u32(person + p::staff) == 0) return;
    if (sext8(run.call(fn::contract_test, 0x102e43d4, {{3, cell(c, contracts_cell)}, {4, c.u8(person + p::job_club)}, {5, 0}})) == 0) return;
    run.call(fn::contract_clear, 0x102e43e8, {{3, cell(c, contracts_cell)}, {4, person}});
    c.put32(person + p::word_4, 0);
    h[22] = c.u16(date + 2);
    const auto day = run.draw(28, 0x102e4414);
    h[19] = std::uint32_t(day);
    const auto cursor = cell(c, toc::rng_cursor);  // the first draw's sample address: r7 at the month's lookup
    const auto year = run.draw(6, 0x102e446c);
    h[20] = std::uint32_t(year);
    const auto sum = std::uint32_t(run.pool.sum());
    const auto month = run.call(fn::contract_month, 0x102e44ac,
                                {{3, cell(c, contracts_cell)}, {4, c.u32(person + p::club)},
                                 {5, std::uint32_t(mulhw(0x2aaaaaab, std::int32_t(sum)))}, {6, sum}, {7, cursor}});
    run.call(fn::make_date, 0x102e44d0,
             {{3, out}, {4, std::uint32_t(sext8(h[19] + 1))}, {5, month}, {6, (h[22] + h[20] + 1) & 0xffff}, {7, 0xffffffffu}});
    c.put16(person + p::contract_end_year, c.u16(out + 2));
    c.put16(person + p::contract_end_day, c.u16(out));
    c.put8(person + p::contract_end_byte, c.u8(out + 4));
    const auto contract = run.call(fn::contract_of, 0x102e44f4, {{3, cell(c, contracts_cell)}, {4, person}});
    if (contract != 0) run.call(fn::contract_apply, 0x102e4508, {{3, contract}, {4, person}, {5, 0}});
}
}

void regenerate_staff(Cpu& c, std::uint32_t owner, std::uint32_t person, std::uint32_t incoming_r20) {
    (void)owner;
    // 0x102e27e0: stmw r19, 0x90 bytes.
    Frame frame(c, 0x102e27e0, 0x90, {.first_gpr = 19});
    Run run(c);
    auto& h = run.held;
    h[20] = incoming_r20;
    h[22] = c.toc(stats_cell); h[23] = c.toc(toc::rng_salt); h[24] = c.toc(toc::rng_start); h[25] = c.toc(toc::rng_end);
    h[26] = c.toc(toc::rng_cursor); h[27] = c.toc(contracts_cell); h[28] = c.toc(toc::constants); h[29] = c.toc(toc::year_date);
    h[30] = person; h[31] = c.toc(kind_cell);
    const auto date = c.toc(toc::year_date);
    const auto constants = c.toc(toc::constants);
    const auto record = [&] { return c.u32(person + p::staff); };
    const auto row_of_kind = [&] { return cell(c, kind_cell) + c.u32(person + p::id) * 100u; };

    // The person's row in the kind table (100 bytes each): the class bits (0x0c of byte +0xc) are cleared, byte +0x6b is set, the staff
    // records' reset (0x10388cc0) runs, then the row's high nibble of +0xc is cleared, its +8 is 0 and its +9 is 0x9c.
    {
        const auto at = row_of_kind() + 0xc;
        const auto cleared = std::uint32_t(c.u8(at) & ~0x0cu);
        c.put8(at, cleared);
        c.put8(person + p::flag_6b, 1);
        run.call(fn::staff_reset, 0x102e284c,
                 {{3, cell(c, news_cell)}, {4, person}, {5, cleared}, {6, c.u32(person + p::id) * 100u + 0xc}, {7, cell(c, kind_cell)}, {8, 0}});
    }
    c.put8(person + p::role_5e, 0);
    c.put8(person + p::role_5f, 0);
    {
        const auto at = row_of_kind() + 0xc;
        c.put8(at, c.u8(at) & 0x0fu);
    }
    c.put8(row_of_kind() + 8, 0);
    c.put8(row_of_kind() + 9, 0x9c);
    // Three classifications of the person's nation-side word, the human managers' hook and the date words.
    c.put32(person + p::word_28, run.call(fn::kind_a, 0x102e28a8, {{3, c.u32(person + p::word_0c)}, {4, 0xffffff9cu}, {5, cell(c, kind_cell)}, {6, 0}}));
    c.put32(person + p::word_2c, run.call(fn::kind_b, 0x102e28b4, {{3, c.u32(person + p::word_0c)}}));
    c.put32(person + p::word_30, run.call(fn::kind_c, 0x102e28c0, {{3, c.u32(person + p::word_0c)}}));
    run.call(fn::humans, 0x102e28d4, {{3, cell(c, humans_cell)}, {4, person}});
    h[31] = date + 2;
    h[19] = 1;
    c.put16(person + p::contract_year, c.u16(date + 2));
    c.put16(person + p::contract_day, c.u16(date));
    // The 64 statistics: each set (0x10309850) from 1 to 64 (the loop counter is a byte compared signed).
    for (; sext8(h[19]) <= 0x40; ++h[19]) run.call(fn::stat_set, 0x102e2904, {{3, cell(c, stats_cell)}, {4, person}, {5, h[19]}});
    run.call(fn::contracts_reset, 0x102e2928, {{3, cell(c, contracts_cell)}, {4, person}, {5, 0}, {6, 0}});
    // Each statistic that reaches the threshold 200 - 10 * (the signed byte +0x61) is raised by its excess in steps of 75, at most 10.
    h[21] = 1;
    h[19] = std::uint32_t(sext16(std::uint32_t(200 - 10 * sext8(c.u8(person + p::rating_61)))));
    for (; sext8(h[21]) <= 0x40; ++h[21]) {
        const auto value = sext16(run.call(fn::stat_get, 0x102e2954, {{3, cell(c, stats_cell)}, {4, person}, {5, h[21]}}));
        if (value == 0) continue;
        if (value < std::int32_t(h[19])) continue;
        const auto excess = value - std::int32_t(h[19]);
        auto steps = mulhw(0x1b4e81b5, excess) >> 3;
        steps += std::int32_t(std::uint32_t(steps) >> 31);
        auto level = steps;  // the register starts at 10 and takes the quotient when that is not above 10
        if (steps > 10) level = 10;
        run.call(fn::stat_level, 0x102e29ac, {{3, cell(c, stats_cell)}, {4, person}, {5, h[21]}, {6, std::uint32_t(level)}, {7, 1}});
    }

    // The ability halfwords. The first draw's bound is trunc(0.75 * the halfword +0x26) in double precision (0 draws nothing).
    Fpu fp(c);
    const auto bias = c.f64(constants);
    const auto potential = sext16(c.u16(record() + st::potential));
    const auto bound = std::int32_t(std::uint32_t(fp.fctiwz(0, fp.fmul(c.f64(constants + three_quarters), fp.fsub(biased(potential), bias)))));
    h[22] = std::uint32_t(bound);
    {
        // (r21 holds the statistics loop's exit value 0x41 until the draw's result replaces it)
        const auto first = run.draw(bound, 0x102e2a24);
        h[21] = std::uint32_t(first);
        c.put16(record() + st::ability, std::uint32_t(first + 1));
    }
    {
        // +0x24 is raised to half of +0x26 when it is below it, in single precision: both values converted to single, the half taken
        // with the constant 0.5f, compared (fcmpo), and the half truncated into +0x24.
        const auto half = c.f32(c.toc(single_constants) + half_single);
        const auto rec = record();
        const auto a = fp.fsubs(biased(sext16(c.u16(rec + st::potential))), bias);
        const auto b = fp.fsubs(biased(sext16(c.u16(rec + st::ability))), bias);
        const auto limit = fp.fmuls(a, double(half));
        fp.fcmp(0, b, limit);
        if (b < limit) c.put16(rec + st::ability, std::uint32_t(fp.fctiwz(0, limit)));
    }
    c.put16(record() + st::upper, std::uint32_t(sext16(c.u16(record() + st::ability)) * 25 + run.draw(750, 0x102e2bd8) + 1));
    c.put16(record() + st::lower, std::uint32_t(sext16(c.u16(record() + st::ability)) * 15 + run.draw(200, 0x102e2c48) + 1));
    if (sext16(c.u16(record() + st::upper)) < sext16(c.u16(record() + st::lower))) {
        c.put16(record() + st::upper, std::uint32_t(sext16(c.u16(record() + st::lower)) + 10));
        c.put16(record() + st::lower, 1);
    }
    c.put16(record() + st::current, std::uint32_t(sext16(c.u16(record() + st::upper)) + run.draw(50, 0x102e2ce0)));

    // The job byte: the club side's, else the national side's, else the caller's r20 (its low byte as a signed byte; a value above 10
    // as an unsigned compare takes the default arm).
    if (c.u32(person + p::club) != 0) h[20] = c.u8(person + p::job_club);
    else if (c.u32(person + p::link_nation) != 0) h[20] = c.u8(person + p::job_nation);
    const auto job = sext8(h[20]);
    const Draw* age = &age_default;
    if (std::uint32_t(job) <= 10) age = &age_arms[std::size_t(job)];
    h[21] = std::uint32_t(sext8(std::uint32_t(run.draw(age->modulus, age->back) + age->add)));
    if (job == 1) manager_attributes(run, person);

    // The birth: the year (the current year minus the age), the day below 365, the day class (0x100c6a10), and the personality tier.
    c.put16(person + p::birth_year, c.u16(date + 2) - std::uint32_t(sext8(h[21])));
    {
        const auto day = run.draw(365, 0x102e4218);
        c.put16(person + p::birth_day, std::uint32_t(day));
        // The sum of the draw and the sign of its quotient are what r6 and r5 hold at the call.
        c.put16(person + p::age_5c, std::uint32_t(sext8(run.call(fn::birth_class, 0x102e4264,
                                                                    {{3, person}, {4, date}, {5, std::uint32_t(run.pool.sum()) >> 31}, {6, std::uint32_t(run.pool.sum())}}))));
    }
    const auto tier_draw = sext8(std::uint32_t(run.draw(125, 0x102e428c)));
    {
        std::uint32_t tier = 0;
        while (tier < personality_limits.size() && tier_draw >= personality_limits[tier]) ++tier;
        c.put8(record() + st::personality, tier);
    }
    c.set({{4, std::uint32_t(run.pool.sum())}, {5, cell(c, toc::rng_cursor)}});  // what the original's r4 and r5 hold after the draw

    contract_end(run, person, c.sp() + 0x38);
    run.call(fn::finish, 0x102e4510, {{3, person}});
}
}

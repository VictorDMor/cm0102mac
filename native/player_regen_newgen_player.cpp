// PLAYER_REGEN.CPP, block `newgen_player`: 0x102e4530, which creates a new young player in place of a retired one. The person record
// (0x70 bytes) is reset (kind-table row, links, flags, statistics), takes the nation and the player record the regen object's pool
// names, and gets a current ability (+4 of the player record), potential-based reputations (+8, +0xc, +0xa), an age, a birth day and the
// personality bytes of the player record; last he is placed with a club (or not) and the records are finished.
//
// The order of the draws is the whole contract: 131 inline draw sites, none in a loop, and which of them run depends on the data. The
// order of the original, which this file keeps:
//   1. the nation: a draw below 4 (0 may replace the planned nation, when its reputation is under 100, by a random one: a wide Random below
//      the nations' count), then the second nation by the plan's third index (draws below 100 and 3);
//   2. the ability: a nation without a flag whose reputation word (+0x6c) is under 350 (`modest_nation`) draws the age first (a wide Random
//      below the reputation against a draw below 1000, then below 5 plus 23 or below 7 plus 17), the ability by the age, then +8, +0xc, +0xa and
//      the age tier's clamps; every other nation (`strong_nation`) draws the ability by the position ratings, +8, +0xc, +0xa, then the age by
//      a draw below 10000 and clamps the ability by it;
//   3. the birth day below 365, then the personality bytes in the order of `personality` (+0x1b, +0x36, +0x1d, +0x2e, +0x34, +0x44, +0x3c,
//      +0x3d) and `personality_by_age` (+0x1e, +0x24, +0x2c, +0x2f, ...);
//   4. the placement: a draw below 4 for an age under 19 with a nation whose flag is set (the club pick 0x102e99f0, the signing 0x102dc000,
//      the news, a release), else a draw below 15 (only for a nation without the flag) and one below 10 gate the move (0x102ea3c0, 0x102dc000); a player left unattached under 20 has his
//      reputations capped.
//
// Fidelity. Every draw is the pooled generator's, at the original's refill return address (the call trace pairs them). The wide Random
// (a bound of 0x10000 or more in magnitude takes the scaled form) can have that bound only at two sites, where the bound is a word read from
// memory (the nations' count, the modest nation's reputation): `Regen::wide` is the inline copy, its second draw a real call of 0x102da630 at
// the copy's return address; every other bound is trunc(factor * a halfword), at most 0.6 * 32768 in magnitude, or 15. The ability
// arithmetic is the original's: doubles from the unit's table (TOC -0x50a8), the integer converted through the 2^52 bias, fctiwz for every
// truncation, and the multiply-add (fmadd) of the ability in one rounding, taking its kind from the instruction (`fp.multiply_add(site, ...)`).
// The comparisons are the original's `lbz`/`extsb` signed bytes and `lha` halfwords, read from the player record each time as the original does.
//
// Not written, with the proof. The decision tree of the age tiers inside `modest_nation` tests `age <= 20` first (`ble 0x102e5074`) and the
// code it jumps to asks again whether the age is above 26, above 23 and above 20: three of the five ability arms (above 26: factors 0.2 and
// 0.6, refill 0x102e50e4; above 23: 0.375 and 0.425, refill 0x102e52d0; above 20 again, refill 0x102e54bc) can run only when the age is
// both at most 20 and above it. Their entries (0x102e5074, 0x102e5260, 0x102e544c, 0x102e5638) have one predecessor each, the compare
// before it, and the age register is not written between: dead code (.local/rewrites/player_regen/player/cfg.py). Their three refill
// sites are therefore never reached; the live arms are the age above 20 (refill 0x102e4ef8) and at most 20 (0x102e56a0). Also unreachable:
// an age of 29 or more (`age() < 0x1d` in `personality_by_age`): every age is a remainder below its modulus plus a constant, at most 27.
//
// Registers. The callees run with the arguments the original gives them; the callee-saved registers the original keeps its values in are
// mirrored (`Run::held`) and set before each call and draw, so a callee's prologue saves the original's values. r14..r16 and r25 hold what
// the original's register allocation puts there where that is simple (the draw's result, a loop counter, the age); the stack contract has
// no reader of the others (tools/stack_contract.py). Several callees read r7..r10 as leftovers (tools/call_arguments.py: 0x1014b690 r7-r10,
// 0x10309850 r8-r10, 0x100c2800 and 0x100c2cb0 r9 and r10): r9 and r10 are never written by the entry itself; r7 and r8 are set where the
// original's `li r7,0` / `li r8,1` are. The caller reads no register after the call.
#include "player_regen_detail.hpp"
#include <array>
#include <cstdlib>

namespace cm::aot::player_regen {
namespace {
// The person record (0x70 bytes). +0xc is the person's nation record (0x128 bytes: reputation word +0x6c, flag byte +0x120), +0x10 the
// second nation, +0x14 the player record (0x48 bytes), +0x24 the club.
namespace p {
constexpr std::uint32_t id = 0x00, word_4 = 0x04, word_8 = 0x08, nation = 0x0c, second_nation = 0x10, player = 0x14, staff = 0x1c,
                        link_nation = 0x20, club = 0x24, word_28 = 0x28, word_2c = 0x2c, word_30 = 0x30, birth_day = 0x34, birth_year = 0x36,
                        contract_day = 0x4c, contract_year = 0x4e, age_5c = 0x5c, role_5e = 0x5e, role_5f = 0x5f, job_nation = 0x60,
                        rating_61 = 0x61, job_club = 0x62, flag_6b = 0x6b;
}
// The nation record.
namespace nat {
constexpr std::uint32_t size = 0x128, reputation = 0x6c /* word */, word_74 = 0x74, flag = 0x120 /* byte */;
}
// The club record the club pick returns (0x248 bytes): +0x80 a record whose word +0x1c is a record with a byte +0x36.
namespace club_record { constexpr std::uint32_t link = 0x80, link_record = 0x1c, link_byte = 0x36; }
// The player record (0x48 bytes).
namespace pl {
constexpr std::uint32_t size = 0x48, ability = 0x04 /* halfword */, potential = 0x06 /* halfword */, rep_high = 0x08, rep_low = 0x0c,
                        rep_mid = 0x0a /* halfwords */;
// Position ratings (signed bytes, 20 marks a natural position).
constexpr std::uint32_t pos_f = 0x0f, pos_10 = 0x10, pos_11 = 0x11, pos_12 = 0x12, pos_13 = 0x13, pos_14 = 0x14, pos_15 = 0x15, pos_19 = 0x19;
// Personality bytes.
constexpr std::uint32_t b1b = 0x1b, b1d = 0x1d, b1e = 0x1e, b21 = 0x21, b24 = 0x24, b2c = 0x2c, b2e = 0x2e, b2f = 0x2f, b34 = 0x34,
                        b36 = 0x36, b3c = 0x3c, b3d = 0x3d, b44 = 0x44;
}
// The unit's doubles (offsets in the table at TOC -0x50a8; +0 is the bias, +8 is 65535.0).
namespace k {
constexpr std::uint32_t constant_range = 0x08 /* 65535.0 */, half = 0x58, pot_55 = 0xa0, pot_25 = 0xa8, pot_60 = 0xb8, step_12 = 0xd0, step_135 = 0xd8, step_65 = 0xe0,
                        step_125 = 0xe8, pot_30 = 0xf0;
}
constexpr std::int32_t kind_cell = -0x7b9c, nations_cell = -0x7b98, report_cell = -0x7a64, stats_cell = -0x7b3c, news_cell = -0x7b48,
                       contracts_cell = -0x7b7c, hooks_c = -0x7ba0, hooks_b = -0x7aa4, hook_d = -0x7484, hook_a = -0x7978, hook_a_flag = -0x797c,
                       humans_cell = -0x7a54, human_nation = -0x7a18, other_nation = -0x6678, holder_cell = -0x7bf0, players_cell = -0x7b38;

namespace fn {
constexpr std::uint32_t report = 0x102b8180, pool_prepare = 0x102ded90, hook_a = 0x106fd9f0, stats_reset = 0x10309040, staff_reset = 0x10388cc0,
                        hook_b = 0x100e8660, hook_c = 0x1018fdf0, hook_d = 0x1070c7f0, forget_b = 0x100df9f0, forget_c = 0x10186ea0,
                        contract_clear = 0x10084a80, contract_b = 0x10084b10, kind_a = 0x100c4400, kind_b = 0x100c4cf0, kind_c = 0x100c53e0,
                        humans = 0x1014b690, stat_set = 0x10309850, stat_get = 0x103094b0, stat_level = 0x10309680, nation_check = 0x100de320,
                        birth_class = 0x100c6a10, club_pick = 0x102e99f0, sign = 0x102dc000, news_check = 0x102cdbf0, youth_news = 0x102df780,
                        release = 0x100d9960, move_pick = 0x102ea3c0, finish_a = 0x100c2cb0, finish_b = 0x100c2800;
}

// One run of the routine: the callee-saved registers as the original holds them (r14..r31, indexed by register number), set before every
// call and draw so a callee's prologue saves them.
struct Run {
    Cpu& c;
    Pool pool;
    std::array<std::uint32_t, 32> held{};
    explicit Run(Cpu& cpu) : c(cpu), pool(cpu) { for (unsigned n = 14; n < 32; ++n) held[n] = c.r(n); }
    void sync() { for (unsigned n = 14; n < 32; ++n) c.set(n, held[n]); }
    std::uint32_t call(std::uint32_t target, std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers) {
        sync();
        c.set(registers);
        const auto link = c.lr();
        c.call(target, back);
        c.set_lr(link);
        return c.r(3);
    }
    // One draw below `bound` (none for 0; a bound of 0x10000 or more in magnitude is the scaled form, whose second draw is a call of 0x102da630
    // refilling at `random_first`): the remainder (signed, negative for a negative bound). `back` is the original's refill return address.
    std::int32_t draw(std::int32_t bound, std::uint32_t back) { sync(); return pool.draw(bound, back, refill_site::random_first); }
};

struct Regen {
    Cpu& c;
    Run run;
    std::uint32_t param_1, person;
    Fpu fp;
    std::uint32_t constants;
    Regen(Cpu& cpu, std::uint32_t owner, std::uint32_t who) : c(cpu), run(cpu), param_1(owner), person(who), fp(cpu), constants(cpu.toc(toc::constants)) {}

    std::array<std::uint32_t, 32>& h = run.held;
    std::uint32_t record() const { return c.u32(person + p::player); }
    std::int32_t byte(std::uint32_t at) const { return sext8(c.u8(record() + at)); }
    std::int32_t half(std::uint32_t at) const { return sext16(c.u16(record() + at)); }
    void set_byte(std::uint32_t at, std::int32_t value) { c.put8(record() + at, std::uint32_t(value)); }
    void set_half(std::uint32_t at, std::int32_t value) { c.put16(record() + at, std::uint32_t(value)); }
    std::int32_t draw(std::int32_t bound, std::uint32_t back) { return run.draw(bound, back); }
    std::uint32_t nation() const { return c.u32(person + p::nation); }
    // The age (r25, kept as a signed byte).
    std::int32_t age() const { return sext8(h[25]); }
    void set_age(std::int32_t value) { h[25] = std::uint32_t(sext8(std::uint32_t(value))); }

    // The unit's double at `offset` of the table.
    double constant(std::uint32_t offset) const { return c.f64(constants + offset); }
    // trunc(factor * value) as the original computes it: the integer through the bias, fsub, fmul, fctiwz.
    std::int32_t scaled(std::uint32_t factor, std::int32_t value) {
        return std::int32_t(std::uint32_t(fp.fctiwz(0, fp.fmul(constant(factor), fp.fsub(biased(value), constant(0))))));
    }

    // The wide Random (a bound of 0x10000 or more in magnitude takes the scaled form: its second draw is a real call of 0x102da630): used where the
    // bound is a word from memory (the nations' count, the modest nation's reputation); every other bound is below 24576 in magnitude.
    // `count_form`: the copy at the nations' count (0x102e4844), whose registers are allocated differently from the other copy's (the modest nation's reputation).
    std::int32_t wide(std::int32_t bound, std::uint32_t refill, std::uint32_t call_back, bool count_form);
    void reset_person();
    void choose_nations(std::uint32_t slot, std::uint32_t nation_index, std::uint32_t second_index);
    void initialise_statistics();
    void modest_nation();
    void strong_nation();
    void ability_arm(std::uint32_t factor_n, std::uint32_t factor_ability, std::uint32_t refill, std::uint32_t site);
    void after_modest_ability();
    void age_clamps();
    void join_and_birth();
    void personality();
    void personality_by_age();
    void placement();
    void finish();
};

std::int32_t Regen::wide(std::int32_t bound, std::uint32_t refill, std::uint32_t call_back, bool count_form) {
    if (bound > -0x10000 && bound < 0x10000) return draw(bound, refill);
    run.sync();
    run.pool.advance(refill);
    const auto total = run.pool.sum();
    const auto sample = c.u16(cell(c, toc::rng_cursor));
    const auto bias = constant(0), range = constant(k::constant_range);
    // The inline copy of the scaled form: the bound over 65535 in a double, the sum's remainder by 65535 (the compiler's reciprocal multiplication),
    // the bound's tenfold over 65535 as the bound of the second draw.
    auto scale = fp.fdiv(fp.fsub(biased(bound), bias), range);
    constexpr std::int32_t reciprocal = std::int32_t(0x80008001);
    const auto quotient_floor = std::int32_t(std::uint32_t(mulhw(reciprocal, total)) + std::uint32_t(total)) >> 15;
    const auto quotient = quotient_floor + (quotient_floor < 0);
    const auto product = std::int32_t(std::uint32_t(quotient) * 0xffffu);
    const auto remainder = std::int32_t(std::uint32_t(total) - std::uint32_t(product));
    const auto tenfold = std::int32_t(std::uint32_t(bound) * 10u);
    const auto smaller_floor = std::int32_t(std::uint32_t(mulhw(reciprocal, tenfold)) + std::uint32_t(tenfold)) >> 15;
    const auto smaller = smaller_floor + (smaller_floor < 0);
    scale = fp.fmul(scale, fp.fsub(biased(remainder), bias));
    const auto scaled_part = std::int32_t(std::uint32_t(fp.fctiwz(0, scale)));
    // The registers the inline code leaves for the call (the callee reads r3 only, and leaves the others as they are for a second draw below 0x10000):
    // the copy at the count keeps the salt in r7, the product in r5 and the two signs in r4 and r6; the copy at the reputation the product in r4, 0xffff in
    // r5, the reciprocal in r6 and the sum in r7.
    const auto salt = c.u32(c.toc(toc::rng_salt));
    std::initializer_list<std::pair<unsigned, std::uint32_t>> registers_count = {
        {3, std::uint32_t(smaller)}, {4, smaller_floor < 0 ? 1u : 0u}, {5, std::uint32_t(product)}, {6, quotient_floor < 0 ? 1u : 0u}, {7, salt}, {8, sample}};
    std::initializer_list<std::pair<unsigned, std::uint32_t>> registers_reputation = {
        {3, std::uint32_t(smaller)}, {4, std::uint32_t(product)}, {5, 0xffffu}, {6, std::uint32_t(reciprocal)}, {7, std::uint32_t(total)}, {8, sample}};
    const auto second = std::int32_t(run.call(0x102da630, call_back, count_form ? registers_count : registers_reputation));
    const auto tenth_floor = mulhw(0x66666667, std::int32_t(std::uint32_t(second) + 5u)) >> 2;
    const auto tenth = tenth_floor + (tenth_floor < 0);
    auto result = std::int32_t(std::uint32_t(scaled_part) + std::uint32_t(tenth));
    const auto top = std::int32_t(std::uint32_t(bound) - 1u);
    if (result > top) result = top;
    c.set(4, tenth_floor < 0 ? 1u : 0u);
    return result;
}

// The kind-table row of the person (100 bytes each): the class bits live in byte +0xc.
std::uint32_t row_of(Cpu& c, std::uint32_t person) { return cell(c, kind_cell) + c.u32(person + p::id) * 100u; }

void Regen::reset_person() {
    const auto row = [&] { return row_of(c, person); };
    c.put8(row() + 0xc, (c.u8(row() + 0xc) & ~0x2u) | 2u);  // rlwimi: bit 1 set
    c.put8(person + p::flag_6b, 2);
    c.put32(row() + 0x10, 0);
    c.put8(row() + 0xd, c.u8(row() + 0xd) & ~0x40u);
    c.put8(row() + 0xc, c.u8(row() + 0xc) & ~0x1u);
    c.put8(row() + 0xd, c.u8(row() + 0xd) & ~0x80u);
    c.put8(row() + 9, 0x9c);
    if (cell(c, hook_a) != 0 && sext8(c.u8(c.toc(hook_a_flag))) != 0) run.call(fn::hook_a, 0x102e46c8, {{3, cell(c, hook_a)}, {4, person}});
    run.call(fn::stats_reset, 0x102e46d4, {{3, cell(c, stats_cell)}, {4, person}});
    run.call(fn::staff_reset, 0x102e46e4, {{3, cell(c, news_cell)}, {4, person}});
    run.call(fn::hook_b, 0x102e46f0, {{3, cell(c, hooks_b)}, {4, person}});
    run.call(fn::hook_c, 0x102e46fc, {{3, cell(c, hooks_c)}, {4, person}});
    run.call(fn::hook_d, 0x102e470c, {{3, cell(c, hook_d)}, {4, person}});
    run.call(fn::forget_b, 0x102e4718, {{3, cell(c, hooks_b)}, {4, c.u32(person + p::id)}});
    run.call(fn::forget_c, 0x102e4724, {{3, cell(c, hooks_c)}, {4, c.u32(person + p::id)}});
    run.call(fn::contract_clear, 0x102e4730, {{3, cell(c, contracts_cell)}, {4, person}});
    run.call(fn::contract_b, 0x102e473c, {{3, cell(c, contracts_cell)}, {4, person}});
    c.put8(person + p::role_5e, 0);
    c.put8(person + p::role_5f, 0);
    if (c.u32(person + p::staff) != 0) c.put32(person + p::staff, 0);
    c.set({{7, 0}});  // `li r7,0` before the row's flags: what the later callees' leftovers read
    c.put8(row() + 0xc, c.u8(row() + 0xc) & 0x0fu);
    c.put8(row() + 8, 0);
    c.put8(row() + 9, 0x9c);
}

// The nation: the planned one (index r20 of the nations table), replaced at random (a draw below 4 being 0, a wide draw below the count)
// when its reputation is under 100 by a nation that has word +0x74 and a reputation under 100; the second nation by the plan's third
// index (when it is in 1..count-1), else none, with draws deciding between those cases.
void Regen::choose_nations(std::uint32_t, std::uint32_t nation_index, std::uint32_t second_index) {
    const auto nations = [&] { return cell(c, nations_cell); };
    c.put32(person + p::nation, nations() + nation_index * nat::size);
    h[14] = c.toc(toc::nation_count);
    if (std::int32_t(c.u32(c.u32(person + p::nation) + nat::reputation)) < 100) {
        if (draw(4, 0x102e47e4) == 0) {
            const auto count = std::int32_t(cell(c, toc::nation_count));
            h[15] = std::uint32_t(count);
            std::int32_t pick = 0;
            if (count == 0) h[16] = 0;
            else {
                pick = wide(count, 0x102e4844, 0x102e490c, true);
                h[16] = std::uint32_t(pick);
            }
            const auto candidate = nations() + std::uint32_t(pick) * nat::size;
            if (c.u32(candidate + nat::word_74) != 0 && std::int32_t(c.u32(candidate + nat::reputation)) < 100) c.put32(person + p::nation, candidate);
        }
    }
    const auto count = std::int32_t(cell(c, toc::nation_count));
    const auto second = std::int32_t(second_index);
    if (second <= 0 || second >= count) {
        c.put32(person + p::second_nation, 0);
        return;
    }
    if (nation_index == cell(c, human_nation) && draw(100, 0x102e49d8) == 0) return;  // the old second nation stays
    if (second_index != cell(c, other_nation) && draw(3, 0x102e4a44) != 0) {
        c.put32(person + p::second_nation, 0);
        return;
    }
    c.put32(person + p::second_nation, nations() + second_index * nat::size);
}

// The 64 statistics are set (0x10309850), then each one at or above 200 - 10 * the signed byte +0x61 is raised by (its excess / 75), at most 10.
void Regen::initialise_statistics() {
    h[23] = c.toc(toc::year_date) + 2;
    c.put16(person + p::contract_year, c.u16(c.toc(toc::year_date) + 2));
    c.put16(person + p::contract_day, c.u16(c.toc(toc::year_date)));
    c.put32(person + p::club, 0);
    c.put32(person + p::link_nation, 0);
    c.put32(person + p::word_4, 0);
    c.put32(person + p::word_8, 0);
    for (h[14] = 1; sext8(h[14]) <= 0x40; ++h[14]) run.call(fn::stat_set, 0x102e4b1c, {{3, cell(c, stats_cell)}, {4, person}, {5, h[14]}});
    h[14] = 1;
    h[15] = std::uint32_t(sext16(std::uint32_t(200 - 10 * sext8(c.u8(person + p::rating_61)))));
    for (; sext8(h[14]) <= 0x40; ++h[14]) {
        const auto value = sext16(run.call(fn::stat_get, 0x102e4b58, {{3, cell(c, stats_cell)}, {4, person}, {5, h[14]}}));
        if (value == 0) continue;
        if (value < std::int32_t(h[15])) continue;
        const auto excess = value - std::int32_t(h[15]);
        auto steps = mulhw(0x1b4e81b5, excess) >> 3;
        steps += std::int32_t(std::uint32_t(steps) >> 31);
        auto level = steps;
        if (steps > 10) level = 10;
        run.call(fn::stat_level, 0x102e4bb0, {{3, cell(c, stats_cell)}, {4, person}, {5, h[14]}, {6, std::uint32_t(level)}, {7, 1}});
    }
}

// The ability of a person whose nation has no flag and a reputation under 350: the age first. A nation with reputation 0 draws no wide
// Random (the test value stays 0); otherwise the wide Random below the reputation is compared with a draw below 1000: below it the age is a
// draw below 5 plus 23, else below 7 plus 17. The ability is the original's `ability_arm` of the age tier (above 20, else at most 20), then +8,
// +0xc, +0xa and the age tier's clamps.
void Regen::modest_nation() {
    const auto reputation = std::int32_t(c.u32(nation() + nat::reputation));
    h[14] = std::uint32_t(reputation);
    std::int32_t test = 0;  // r15 holds 0 here (`li r15,0` after the statistics) when the wide draw is skipped
    h[15] = 0;
    if (reputation != 0) {
        test = wide(reputation, 0x102e4c3c, 0x102e4d04, false);
        h[15] = std::uint32_t(test);
    }
    const auto limit = draw(1000, 0x102e4d84);
    if (test < limit) set_age(draw(5, 0x102e4de0) + 0x17);
    else set_age(draw(7, 0x102e4e40) + 0x11);
    // The tiers. Above 20: factors 0.55 (the bound) and 0.25; at most 20 (the age is 17..27 here, so no other arm runs, see the file header):
    // 0.6 and 0.25. The other three arms of the original's tree are dead.
    if (age() > 20) ability_arm(k::pot_55, k::pot_25, 0x102e4ef8, 0x102e505c);
    else ability_arm(k::pot_60, k::pot_25, 0x102e56a0, 0x102e5804);
    after_modest_ability();
}

// The ability from the potential (+6): n = trunc(factor_n * potential), r = a wide draw below n (0 for n = 0), the ability = trunc(factor_ability
// * potential + r) in one rounding (the original's fmadd at `site`).
void Regen::ability_arm(std::uint32_t factor_n, std::uint32_t factor_ability, std::uint32_t refill, std::uint32_t site) {
    const auto n = scaled(factor_n, half(pl::potential));
    h[14] = std::uint32_t(n);
    std::int32_t r = 0;
    if (n != 0) r = draw(n, refill);
    h[15] = std::uint32_t(r);
    const auto potential = fp.fsub(biased(half(pl::potential)), constant(0));
    const auto drawn = fp.fsub(biased(r), constant(0));
    const auto value = fp.fctiwz(0, fp.multiply_add(site, constant(factor_ability), potential, drawn));
    c.put16(record() + pl::ability, std::uint32_t(value));
}

void Regen::after_modest_ability() {
    // +8 = 30 * the ability + a draw below 750 + 250; with a potential above 140, an ability above 100, +8 under 6500 and an age of 19..21 a
    // draw below 4 that is not 0 raises +8 by 1.2.
    set_half(pl::rep_high, half(pl::ability) * 30 + draw(750, 0x102e5838) + 0xfa);
    if (half(pl::potential) > 0x8c && half(pl::ability) > 100 && half(pl::rep_high) < 0x1964 && age() > 0x12 && age() < 0x16) {
        if (draw(4, 0x102e58e4) != 0) set_half(pl::rep_high, scaled(k::step_12, half(pl::rep_high)));
    }
    // +0xc by the age and the ability.
    if (age() < 0x13) {
        if (half(pl::ability) < 0x8c) set_half(pl::rep_low, half(pl::ability) * 5 + draw(200, 0x102e5b84) + 1);
        else set_half(pl::rep_low, half(pl::ability) * 10 + draw(200, 0x102e5bf8) + 0xfa);
    } else {
        const auto ability = half(pl::ability);
        if (ability < 0x78) set_half(pl::rep_low, ability * 0xf + draw(200, 0x102e598c) + 1);
        else if (age() < 0x16 || ability < 0x79) {
            if (ability < 0x8c) set_half(pl::rep_low, ability * 0x14 + draw(200, 0x102e5a8c) + 1);
            else set_half(pl::rep_low, ability * 0x14 + draw(200, 0x102e5b00) + 0xfa);
        } else set_half(pl::rep_low, ability * 0x19 + draw(200, 0x102e5a10) + 0xfa);
    }
    if (half(pl::rep_high) < 0x8ca) set_half(pl::rep_low, draw(5, 0x102e5c78) + 1);
    if (half(pl::rep_high) < half(pl::rep_low)) {
        set_half(pl::rep_high, half(pl::rep_low) + 10);
        set_half(pl::rep_low, 1);
    }
    set_half(pl::rep_mid, half(pl::rep_high) + draw(50, 0x102e5d04));
    // The age's clamps of the ability and the reputations.
    if (age() < 0x18) {
        if (age() < 0x14) {
            // below 20: the ability is raised to half the potential on a draw below 4 that is not 0; then capped at 120.
            if (half(pl::ability) < sext16(std::uint32_t(scaled(k::half, half(pl::potential))))) {
                if (draw(4, 0x102e5f24) != 0) set_half(pl::ability, scaled(k::half, half(pl::potential)));
            }
            if (age() < 0x12) {
                if (half(pl::ability) > 0x78 && draw(150, 0x102e5fcc) != 0) set_half(pl::ability, 0x78);
            } else if (half(pl::ability) > 0x78 && draw(50, 0x102e6048) != 0) set_half(pl::ability, 0x78);
        } else {
            // 20..23: +8 and +0xa raised by 1.25 when under 5750; the ability by 0.6 of the potential on a draw below 4; capped at 140.
            if (half(pl::rep_high) < 0x1676) set_half(pl::rep_high, scaled(k::step_125, half(pl::rep_high)));
            if (half(pl::rep_mid) < 0x1676) set_half(pl::rep_mid, scaled(k::step_125, half(pl::rep_mid)));
            const auto goal = sext16(std::uint32_t(scaled(k::pot_60, half(pl::potential))));
            if (half(pl::ability) < goal) {
                if (draw(4, 0x102e6184) != 0) set_half(pl::ability, scaled(k::pot_60, half(pl::potential)));
            }
            if (half(pl::ability) > 0x8c && draw(50, 0x102e6220) != 0) set_half(pl::ability, 0x8c);
        }
    } else {
        // 24 and over: +8 and +0xa raised by 1.35 when under 6000; the ability by 0.65 of itself on a draw below 4.
        if (half(pl::rep_high) < 6000) set_half(pl::rep_high, scaled(k::step_135, half(pl::rep_high)));
        if (half(pl::rep_mid) < 6000) set_half(pl::rep_mid, scaled(k::step_135, half(pl::rep_mid)));
        if (half(pl::ability) < sext16(std::uint32_t(scaled(k::step_65, half(pl::ability))))) {
            if (draw(4, 0x102e5e44) != 0) set_half(pl::ability, scaled(k::step_65, half(pl::ability)));
        }
    }
}

// The ability of the other persons: by the position ratings (natural position 20 in +0x15, +0x11 or +0x10, +0xf, else the default), then +8
// (the draw below 250 being 0 picks the other form), +0xc and +0xa, the age by a draw below 10000 and the ability's clamps by it.
void Regen::strong_nation() {
    const auto ratings = record();
    if (c.u8(ratings + pl::pos_15) == 0x14) ability_arm(k::pot_60, k::pot_30, 0x102e62e0, 0x102e6444);
    else if (c.u8(ratings + pl::pos_11) == 0x14 || c.u8(ratings + pl::pos_10) == 0x14) ability_arm(k::pot_55, k::pot_25, 0x102e64d8, 0x102e663c);
    else if (c.u8(ratings + pl::pos_f) == 0x14) ability_arm(k::half, k::pot_25, 0x102e66c4, 0x102e6828);
    else ability_arm(k::pot_60, k::pot_25, 0x102e68a4, 0x102e6a08);
    if (draw(250, 0x102e6a3c) == 0) {
        set_half(pl::rep_high, half(pl::ability) * 0xf + draw(750, 0x102e6b74) + 0x7d);
        set_half(pl::rep_low, 1);
    } else {
        set_half(pl::rep_high, half(pl::ability) * 0x14 + draw(750, 0x102e6a94) + 0xfa);
        set_half(pl::rep_low, half(pl::ability) * 5 + draw(20, 0x102e6b04) + 1);
    }
    if (half(pl::rep_high) < half(pl::rep_low)) {
        set_half(pl::rep_high, half(pl::rep_low) + 10);
        set_half(pl::rep_low, 1);
    }
    set_half(pl::rep_mid, half(pl::rep_high) + draw(50, 0x102e6c18));
    age_clamps();
}

// The age by a draw below 10000: 0 gives 15..18 (below 4 plus 15) and caps the ability at 80 (age 15) or 100 on a draw below 250 / 150 that
// is not 0; under 5000 gives below 3 plus 16 (ages 16 and 17 cap at 80, age 18 at 100); under 9976 gives below 4 plus 17 (cap 90 at age 17,
// else 110); else below 6 plus 21 (no cap).
void Regen::age_clamps() {
    const auto roll = draw(10000, 0x102e6c80);
    const auto cap = [&](std::int32_t above, std::int32_t to, std::int32_t denominator, std::uint32_t back) {
        if (half(pl::ability) > above && draw(denominator, back) != 0) set_half(pl::ability, to);
    };
    if (roll == 0) {
        set_age(draw(4, 0x102e6cd8) + 0xf);
        if (age() == 0xf) cap(0x50, 0x50, 250, 0x102e6d40);
        else cap(100, 100, 150, 0x102e6db8);
    } else if (roll < 5000) {
        set_age(draw(3, 0x102e6e28) + 0x10);
        if (age() < 0x12) cap(0x50, 0x50, 250, 0x102e6e98);
        else cap(100, 100, 150, 0x102e6f10);
    } else if (roll < 0x26f8) {
        set_age(draw(4, 0x102e6fdc) + 0x11);
        if (age() == 0x11) cap(0x5a, 0x5a, 250, 0x102e7044);
        else cap(0x6e, 0x6e, 150, 0x102e70bc);
    } else set_age(draw(6, 0x102e6f80) + 0x15);
}

// The part both branches share: the nation's check (0x100de320) may leave +0xc at 1 or scale it by 0.65, the birth year (the year minus the age),
// the birth day below 365 and its class (0x100c6a10).
void Regen::join_and_birth() {
    const auto date = c.toc(toc::year_date);
    if (sext8(run.call(fn::nation_check, 0x102e710c, {{3, c.u32(person + p::nation)}})) == 0) {
        if (age() < 0x19 && half(pl::ability) < 0x78) set_half(pl::rep_low, 1);
        else set_half(pl::rep_low, scaled(k::step_65, half(pl::rep_low)));
    }
    c.put16(person + p::birth_year, c.u16(date + 2) - std::uint32_t(age()));
    const auto day = draw(365, 0x102e71a8);
    c.put16(person + p::birth_day, std::uint32_t(day));
    // r5 is the salt word and r6 the sum the draw's remainder was taken from.
    c.put16(person + p::age_5c, std::uint32_t(sext8(run.call(fn::birth_class, 0x102e71f4,
                                                            {{3, person}, {4, date}, {5, c.u32(c.toc(toc::rng_salt))}, {6, std::uint32_t(run.pool.sum())}}))));
}

// The personality bytes of the player record, each in the original's order: +0x1b (by the position ratings, then adjusted), +0x36, +0x1d, +0x2e,
// +0x34, +0x44, +0x3c and +0x3d. The original reads a byte back with `lbz`+`extsb` before every test, as `byte` does.
void Regen::personality() {
    // +0x1b: with a potential above 100 and a natural position rating (+0x15 or +0x14) of 15 or more the draws below 3 pick the arm, else a draw
    // below 4 picks between below 16 plus 5 and below 10 plus 8.
    if (half(pl::potential) > 100 && (byte(pl::pos_15) >= 15 || byte(pl::pos_14) >= 15)) {
        if (draw(3, 0x102e724c) == 0) {
            if (draw(3, 0x102e7304) == 0) set_byte(pl::b1b, draw(7, 0x102e7358) + 0xe);
            else set_byte(pl::b1b, draw(16, 0x102e73c0) + 5);
        } else set_byte(pl::b1b, draw(11, 0x102e72a0) + 10);
    } else {
        if (draw(4, 0x102e741c) == 0) set_byte(pl::b1b, draw(16, 0x102e74cc) + 5);
        else set_byte(pl::b1b, draw(10, 0x102e7468) + 8);
    }
    if (byte(pl::b1b) < 16) {
        if (byte(pl::b1b) < 5) set_byte(pl::b1b, byte(pl::b1b) + draw(10, 0x102e75b0));
        else if (byte(pl::b1b) < 10) set_byte(pl::b1b, byte(pl::b1b) + draw(8, 0x102e7628));
    } else set_byte(pl::b1b, byte(pl::b1b) - draw(5, 0x102e7538));
    // +0x36 by +0x1b.
    if (byte(pl::b1b) < 16) {
        if (byte(pl::b1b) < 11) {
            if (byte(pl::b1b) < 5) set_byte(pl::b36, draw(10, 0x102e777c) + 1);
            else if (draw(4, 0x102e77e4) == 0) set_byte(pl::b36, draw(10, 0x102e7888) + 5);
            else set_byte(pl::b36, draw(16, 0x102e7830) + 5);
        } else set_byte(pl::b36, draw(10, 0x102e770c) + 8);
    } else set_byte(pl::b36, draw(11, 0x102e769c) + 10);
    // +0x1d: a natural position in +0xf draws below 5 first, and unless that is 0 below 20 against the potential less 100.
    bool pick = true;
    if (c.u8(record() + pl::pos_f) == 0x14) {
        if (draw(5, 0x102e78fc) != 0) {
            if (draw(20, 0x102e7954) + 100 < half(pl::potential)) set_byte(pl::b1d, draw(11, 0x102e79bc) + 10);
            else set_byte(pl::b1d, draw(16, 0x102e7a20) + 5);
            pick = false;
        }
    }
    if (pick) {
        set_byte(pl::b1d, draw(20, 0x102e7a7c) + 1);
        if (byte(pl::b1d) < 16) {
            if (byte(pl::b1d) < 5) set_byte(pl::b1d, byte(pl::b1d) + draw(6, 0x102e7b68));
        } else set_byte(pl::b1d, byte(pl::b1d) - draw(6, 0x102e7af4));
    }
    // +0x2e: a draw below 20 plus a wide draw below 3 times the bonus (-5, 0 or 5 by the position ratings), kept within 1..20.
    std::int32_t bonus = -5;
    if (byte(pl::pos_11) < 15) {
        if ((byte(pl::pos_13) < 15 && byte(pl::pos_12) < 15) || byte(pl::pos_19) < 15) {
            if (byte(pl::pos_f) < 15) {
                if (byte(pl::pos_15) > 14) bonus = 0;
            } else bonus = 0;
        } else bonus = 0;
    } else {
        bonus = 0;
        if (byte(pl::pos_19) > 14) bonus = 5;
    }
    h[14] = std::uint32_t(bonus);
    const auto base = draw(20, 0x102e7c68);
    h[14] = std::uint32_t(base);
    h[16] = std::uint32_t(bonus);
    h[15] = std::uint32_t(bonus * 3);
    const auto wide = draw(bonus * 3, 0x102e7cd4);
    h[16] = std::uint32_t(wide);
    set_byte(pl::b2e, base + wide);
    if (byte(pl::b2e) < 1) set_byte(pl::b2e, 1);
    else if (byte(pl::b2e) > 20) set_byte(pl::b2e, 20);
    // +0x34.
    set_byte(pl::b34, draw(20, 0x102e7e58) + 1);
    if (byte(pl::b34) < 16) {
        if (byte(pl::b34) < 8) set_byte(pl::b34, byte(pl::b34) + draw(10, 0x102e7f44));
    } else set_byte(pl::b34, byte(pl::b34) - draw(6, 0x102e7ed0));
    // +0x44: above 15 a draw below 3 that is not 0 lowers it by a draw below 5; every other case raises a low one.
    set_byte(pl::b44, draw(20, 0x102e7fb0) + 1);
    bool lowered = false;
    if (byte(pl::b44) >= 16 && draw(3, 0x102e8028) != 0) {
        set_byte(pl::b44, byte(pl::b44) - draw(5, 0x102e807c));
        lowered = true;
    }
    if (!lowered) {
        if (byte(pl::b44) < 5) set_byte(pl::b44, byte(pl::b44) + draw(10, 0x102e8100));
        else if (byte(pl::b44) < 10) set_byte(pl::b44, byte(pl::b44) + draw(5, 0x102e8178));
    }
    // +0x3c by the age, +0x3d by the age.
    if (age() < 0x12) {
        set_byte(pl::b3c, draw(20, 0x102e81f0) + 1);
        if (byte(pl::b3c) < 16) {
            if (byte(pl::b3c) < 5) set_byte(pl::b3c, byte(pl::b3c) + draw(8, 0x102e82d4));
            else if (byte(pl::b3c) < 10) set_byte(pl::b3c, byte(pl::b3c) + draw(6, 0x102e8340));
        } else set_byte(pl::b3c, byte(pl::b3c) - draw(8, 0x102e8268));
    } else if (age() < 0x18) {
        set_byte(pl::b3c, draw(18, 0x102e83b4) + 3);
        if (byte(pl::b3c) < 16) {
            if (byte(pl::b3c) < 5) set_byte(pl::b3c, byte(pl::b3c) + draw(10, 0x102e84a4));
            else if (byte(pl::b3c) < 10) set_byte(pl::b3c, byte(pl::b3c) + draw(5, 0x102e851c));
        } else set_byte(pl::b3c, byte(pl::b3c) - draw(5, 0x102e842c));
    } else {
        set_byte(pl::b3c, draw(15, 0x102e858c) + 6);
        if (byte(pl::b3c) < 16) {
            if (byte(pl::b3c) < 10) set_byte(pl::b3c, byte(pl::b3c) + draw(8, 0x102e867c));
        } else set_byte(pl::b3c, byte(pl::b3c) - draw(3, 0x102e8608));
    }
    if (byte(pl::b3c) < 10 && byte(pl::b44) > 15) set_byte(pl::b3c, byte(pl::b3c) + draw(5, 0x102e8700));
    if (age() < 0x12) {
        set_byte(pl::b3d, draw(20, 0x102e8778) + 1);
        if (byte(pl::b3d) < 16) {
            if (byte(pl::b3d) < 5) set_byte(pl::b3d, byte(pl::b3d) + draw(6, 0x102e8868));
        } else set_byte(pl::b3d, byte(pl::b3d) - draw(10, 0x102e87f0));
    } else if (age() < 0x18) {
        set_byte(pl::b3d, draw(20, 0x102e88dc) + 1);
        if (byte(pl::b3d) < 16) {
            if (byte(pl::b3d) < 5) set_byte(pl::b3d, byte(pl::b3d) + draw(8, 0x102e89c0));
        } else set_byte(pl::b3d, byte(pl::b3d) - draw(8, 0x102e8954));
    } else {
        set_byte(pl::b3d, draw(20, 0x102e8a24) + 1);
        if (byte(pl::b3d) < 16) {
            if (byte(pl::b3d) < 5) set_byte(pl::b3d, byte(pl::b3d) + draw(10, 0x102e8b14));
        } else set_byte(pl::b3d, byte(pl::b3d) - draw(5, 0x102e8a9c));
    }
    if ((c.u8(record() + pl::pos_11) == 0x14 || c.u8(record() + pl::pos_12) == 0x14) && byte(pl::b3d) < 10)
        set_byte(pl::b3d, byte(pl::b3d) + draw(5, 0x102e8bac));
    if (byte(pl::b2e) > 15 && byte(pl::b3d) < 10) set_byte(pl::b3d, byte(pl::b3d) + draw(5, 0x102e8c3c));
}

// The age tiers lower or raise the bytes +0x1e, +0x24, +0x2c, +0x2f (and +0x21, +0x3c, +0x3d) once more.
void Regen::personality_by_age() {
    if (age() < 0x18) {
        if (age() < 0x12) {
            set_byte(pl::b1e, byte(pl::b1e) - draw(40, 0x102e8cbc));
            set_byte(pl::b24, byte(pl::b24) - draw(40, 0x102e8d28));
            if (byte(pl::b2c) < 16) {
                if (byte(pl::b2c) > 10) set_byte(pl::b2c, byte(pl::b2c) - draw(5, 0x102e8e14));
            } else set_byte(pl::b2c, byte(pl::b2c) - draw(8, 0x102e8da8));
            if (byte(pl::b2f) < 16) {
                if (byte(pl::b2f) > 10) set_byte(pl::b2f, byte(pl::b2f) - draw(8, 0x102e8f0c));
            } else set_byte(pl::b2f, byte(pl::b2f) - draw(10, 0x102e8e94));
        } else {
            set_byte(pl::b1e, byte(pl::b1e) - draw(20, 0x102e8f70));
            set_byte(pl::b24, byte(pl::b24) - draw(20, 0x102e8fdc));
            if (byte(pl::b2c) > 15) set_byte(pl::b2c, byte(pl::b2c) - draw(5, 0x102e905c));
            if (byte(pl::b2f) < 16) {
                if (byte(pl::b2f) > 10) set_byte(pl::b2f, byte(pl::b2f) - draw(5, 0x102e9148));
            } else set_byte(pl::b2f, byte(pl::b2f) - draw(8, 0x102e90dc));
        }
        if (byte(pl::b2f) > 15) set_byte(pl::b2f, byte(pl::b2f) - draw(8, 0x102e91c8));
    } else if (age() > 0x19 && age() < 0x1d) {
        if (byte(pl::b3d) < 5) set_byte(pl::b3d, byte(pl::b3d) + draw(8, 0x102e9250));
        if (byte(pl::b21) < 5) set_byte(pl::b21, byte(pl::b21) + draw(8, 0x102e92c4));
        if (byte(pl::b3c) < 5) set_byte(pl::b3c, byte(pl::b3c) + draw(8, 0x102e9338));
        set_byte(pl::b1e, byte(pl::b1e) - draw(10, 0x102e9398));
        set_byte(pl::b24, byte(pl::b24) - draw(10, 0x102e9404));
        if (byte(pl::b2c) < 5) set_byte(pl::b2c, byte(pl::b2c) + draw(8, 0x102e9484));
        if (byte(pl::b2f) < 16) {
            if (byte(pl::b2f) > 10) set_byte(pl::b2f, byte(pl::b2f) - draw(3, 0x102e9570));
        } else set_byte(pl::b2f, byte(pl::b2f) - draw(5, 0x102e94f8));
    }
}

// The placement: a young player (under 19) of a nation with the flag set is placed with a club on three draws out of four (the club pick
// 0x102e99f0, the signing 0x102dc000, the news check and the youth promotion's news, and a release from the club's staff by a draw below 100
// against a byte); every other case moves him on a draw below 10, preceded by one below 15 only when he has a nation whose flag is clear (0x102ea3c0, 0x102dc000).
void Regen::placement() {
    const auto nation = [&] { return c.u32(person + p::nation); };
    bool placed = false;
    if (age() < 0x13 && nation() != 0 && sext8(c.u8(nation() + nat::flag)) != 0) {
        if (draw(4, 0x102e95fc) != 0) {
            placed = true;
            const auto club = run.call(fn::club_pick, 0x102e9638, {{3, param_1}, {4, person}});
            h[14] = club;
            if (club != 0 && sext8(run.call(fn::sign, 0x102e9650, {{3, param_1}, {4, person}, {5, club}})) != 0) {
                if (sext8(run.call(fn::news_check, 0x102e9668, {{3, cell(c, holder_cell)}, {4, club}})) != 0)
                    run.call(fn::youth_news, 0x102e9680, {{3, param_1}, {4, person}, {5, club}});
                const auto owner = c.u32(club + club_record::link);
                if (half(pl::potential) > 0x50 && owner != 0 && c.u32(owner + club_record::link_record) != 0) {
                    const auto roll = draw(100, 0x102e96c8);
                    if (sext8(c.u8(c.u32(owner + club_record::link_record) + club_record::link_byte)) > roll)
                        run.call(fn::release, 0x102e9724, {{3, owner}, {4, person}, {5, 0}});
                }
            }
        }
    }
    if (!placed) {
        bool move = false;
        if (nation() != 0 && sext8(c.u8(nation() + nat::flag)) == 0 && draw(15, 0x102e9760) != 0) move = true;
        if (!move && draw(10, 0x102e97c0) == 0) move = true;
        if (move) {
            const auto target = run.call(fn::move_pick, 0x102e9808, {{3, param_1}, {4, person}});
            h[14] = target;
            if (target != 0) run.call(fn::sign, 0x102e981c, {{3, param_1}, {4, person}, {5, target}});
        }
    }
}

// An unattached person under 20 has +8 and +0xa capped at 1250 and +0xc set to 1; then the records are finished (0x100c2cb0, 0x100c2800).
void Regen::finish() {
    if (c.u32(person + p::club) == 0 && age() < 0x14) {
        if (half(pl::rep_mid) > 0x4e2) set_half(pl::rep_mid, 0x4e2);
        if (half(pl::rep_high) > 0x4e2) set_half(pl::rep_high, 0x4e2);
        set_half(pl::rep_low, 1);
    }
    run.call(fn::finish_a, 0x102e9878, {{3, person}});
    run.call(fn::finish_b, 0x102e9880, {{3, person}});
}
}

void regenerate_player(Cpu& c, std::uint32_t owner, std::uint32_t person) {
    // 0x102e4530: stmw r14, 0xa0 bytes.
    Frame frame(c, 0x102e4530, 0xa0, {.first_gpr = 14});
    Regen r(c, owner, person);
    auto& h = r.h;
    h[14] = c.toc(toc::nation_count); h[16] = c.toc(hooks_c); h[17] = c.toc(hooks_b); h[18] = c.toc(contracts_cell); h[21] = c.toc(stats_cell);
    h[22] = c.toc(toc::year_date); h[23] = c.toc(nations_cell); h[24] = owner; h[25] = c.toc(kind_cell); h[26] = c.toc(toc::constants);
    h[27] = c.toc(toc::rng_end); h[28] = c.toc(toc::rng_start); h[29] = c.toc(toc::rng_salt); h[30] = person; h[31] = c.toc(toc::rng_cursor);
    const auto row = row_of(c, person) + 0xc;
    // The person must be marked for regeneration (class bits 2..3 of his kind row nonzero) and have no club; else the marker is cleared.
    if (((c.u8(row) >> 2) & 3) == 0 || c.u32(person + p::club) != 0) {
        c.put8(row, c.u8(row) & ~0x0cu);
        // What the original's last instructions leave in r3..r6 on this path.
        c.set({{3, 0}, {5, c.u32(c.toc(kind_cell))}, {6, c.u32(person + p::id) * 100u + 0xc}});
        return;
    }
    r.run.call(fn::report, 0x102e45c4, {{3, cell(c, report_cell)}});
    const auto plan = c.u32(owner + player_pool_list.entries);
    const auto slot = c.u32(plan), nation_index = c.u32(plan + 4), second_index = c.u32(plan + 8);
    h[15] = slot; h[20] = nation_index; h[19] = second_index;
    r.run.call(fn::pool_prepare, 0x102e45e0, {{3, owner}, {4, slot}});
    c.put32(person + p::player, cell(c, players_cell) + slot * pl::size);
    c.set({{8, 1}});  // `li r8,1` after the pool's preparation
    r.reset_person();
    r.choose_nations(slot, nation_index, second_index);
    r.run.call(fn::kind_a, 0x102e4aac, {{3, c.u32(person + p::nation)}});
    c.put32(person + p::word_28, c.r(3));
    r.run.call(fn::kind_b, 0x102e4ab8, {{3, c.u32(person + p::nation)}});
    c.put32(person + p::word_2c, c.r(3));
    r.run.call(fn::kind_c, 0x102e4ac4, {{3, c.u32(person + p::nation)}});
    c.put32(person + p::word_30, c.r(3));
    r.run.call(fn::humans, 0x102e4ad8, {{3, cell(c, humans_cell)}, {4, person}});
    r.initialise_statistics();
    h[15] = 0;
    c.put8(person + p::job_club, 0xb);
    c.put8(person + p::job_nation, 0xb);
    {
        const auto at = row_of(c, person) + 0xc;
        c.put8(at, c.u8(at) & ~0x0cu);
    }
    const auto nation = c.u32(person + p::nation);
    if (nation == 0 || sext8(c.u8(nation + nat::flag)) != 0 || std::int32_t(c.u32(nation + nat::reputation)) > 0x15d) r.strong_nation();
    else r.modest_nation();
    r.join_and_birth();
    r.personality();
    r.personality_by_age();
    r.placement();
    r.finish();
}
}

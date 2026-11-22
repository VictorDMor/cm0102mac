// PLAYER_REGEN.CPP, block `retirement_processor`: 0x102d8920, which walks list A of the regen object (the retirement entries, sorted by
// due date) and acts on each entry that is due. Called daily with the regen object.
//
// What it does, per entry (r31), after sorting list A (0x10768300 with the comparator at TOC -0x6328) and before sorting list B:
//   * the entry's second date (+0x10 day, +0x12 year) has passed: the retirement happens. A young person (age under 30, entry kind not 8)
//     with a club, or in a class of the kind table that blocks it, is dropped from the list; so is one the contract manager (0x10083110)
//     or the transfer manager (0x103f5e50) holds back, and one on a short contract that need not retire (a club's contract record with
//     no word +0x50, less than 210 days by the date difference (0x102d8b2c), age under 38). Otherwise the retirement is announced (0x102e0090), the person leaves the
//     contract, shortlist, national and dispute bookkeeping (the callees of the straight line), a player is detached from his club
//     (0x102e1f20) and a pure player (a player record, no staff record) may become a non-player: three draws decide whether he is given a
//     staff record from list C (the pool of spare staff records), the ability halfwords of that record being derived from the player's
//     record (pow-based, three variants); a person who has both records is moved to the pool of players (list D, 0x102dec50) and loses the
//     player record; one with neither a staff record is registered for regeneration (0x102df1a0) with a date 1..65 days (or on into the next
//     year) away, and one with a staff record is regenerated at once (0x102e27e0) or detached (0x102e21c0) and registered, by his club and job.
//     The entry is then removed from the list (0x102df020).
//   * otherwise (the second date not yet reached): an entry already announced ends the loop; one whose first date (+8, +0xa) has not passed
//     ends it; else its news (0x102e13c0, unless the person's class is already 1) is posted, it is marked announced (+0x19) and the class
//     of the person's kind-table row set to 1 (bits 2..3 of byte +0xc).
//
// Fidelity. Every draw is the pooled generator's, at the original's refill return address, in the original's order (the call trace pairs
// the refills by it). The loop bound is the count read once; a removal does not move the entry pointer (the next entry has moved into its
// place), an announcement does. A wide Random (a bound of 0x10000 or more in magnitude takes the scaled form) can have that bound at two
// sites only: the bound of the player-to-staff draw (a cube of a byte plus a halfword) and the nations' count; the other two inline draws
// below a person's byte (+0x67 and +0x63, signed) are within 128 in magnitude. Floating point is the original's operation order through
// guest::Fpu (conversions through the 2^52 bias, fsub, fadd, fmul, fcmpo, fctiwz, the constants of the unit's table at TOC -0x50a8); `pow`
// is the host's std::pow, as the CarbonLib bridge computes it (the import glue also saves r2 at sp+0x14).
//
// Registers. The callees run with the arguments the original gives them, the callee-saved registers the original keeps its values in are
// mirrored (`Run::held`, r13..r31) and set before every call and draw so a callee's prologue saves them, and the inline arithmetic's
// writes to r3..r8 are set where a later callee can read them as leftovers (r8-r10 are read by 0x102df020 and 0x10365590; r7-r10 by the
// sorts). r9 and r10 are never written here. The caller 0x101198e8 reads r6-r10 after the call (what the last sort leaves).
// regenerate_staff (0x102e27e0) reads the callee-saved r20 of this routine: the address of the pool's cursor cell, loaded once from TOC
// -0x7b84 and never written again.
#include "player_regen_detail.hpp"
#include <array>
#include <cmath>

namespace cm::aot::player_regen {
namespace {
// Person record fields (0x70 bytes).
namespace p {
constexpr std::uint32_t id = 0x00, nation = 0x0c, second_nation = 0x10, player = 0x14, staff = 0x1c, club = 0x24, contract_a = 0x4c,
                        contract_b = 0x50, age = 0x5c /* halfword, the low byte is the age */, ability_byte = 0x5e, job_club = 0x62,
                        byte_63 = 0x63, byte_67 = 0x67, flag_6b = 0x6b;
}
// The player record.
namespace pl {
constexpr std::uint32_t value_8 = 0x08, value_a = 0x0a, value_c = 0x0c, byte_f = 0x0f;
}
// The staff record (0x44 bytes, an element of the array at TOC -0x79f8).
namespace st {
constexpr std::uint32_t ability_b = 0x28, ability_a = 0x2a, ability_c = 0x2c, byte_30 = 0x30, byte_31 = 0x31;
constexpr std::uint32_t size = 0x44;
}
// The retirement entry (list A, 0x1c bytes).
namespace en {
constexpr std::uint32_t id = 0x00, club = 0x04, day_a = 0x08, year_a = 0x0a, day_b = 0x10, year_b = 0x12, reason = 0x18, announced = 0x19;
}
constexpr std::int32_t news_cell = -0x7b48, awol_cell = -0x7b24, managers_cell = -0x7b40, contracts_cell = -0x7b7c, kind_cell = -0x7b9c,
                       national_cell = -0x7a64, dispute_cell = -0x7484, stats_cell = -0x7b3c, discipline_cell = -0x7aa4,
                       injury_cell = -0x7ba0, staff_records_cell = -0x79f8, comparator_a = -0x6328, comparator_b = -0x632c;
constexpr std::uint32_t transfer_object_shortlist = 0x988, shortlist_flags = 0xdc, en_size = 0x1c, queue_size = 0xc;

// Constants of the unit's table beyond the bias (constant:: in the detail header).
namespace k {
constexpr std::uint32_t unsigned_bias = 0x70 /* 2^52 */, cube = 0x10 /* 3.0 */, scale_65535 = 0x18 /* 200000 / 65535 */, square = 0x20 /* 2.0 */;
constexpr std::uint32_t quarter3 = 0x38 /* 0.75 */;
}

// The callees (a `bl` at the original's address; the return address is that plus 4 at each use below).
namespace fn {
constexpr std::uint32_t date = 0x10715880, sort = 0x10768300, contract_hold = 0x10083110, transfer_hold = 0x103f5e50, days_between = 0x10718130,
                        contract_of = 0x10082f20, contract_guard = 0x100d3150, contract_release = 0x1007fa30, contract_clear = 0x10084a80,
                        contract_end = 0x10374ba0, national_report = 0x102b8180, shortlist_remove = 0x10365590, shortlist_forget = 0x10363b80,
                        awol_a = 0x10006600, awol_b = 0x10006830, dispute = 0x1070c7f0, finish = 0x100c3870, stats_reset = 0x10309040,
                        discipline = 0x100e8660, injury = 0x1018fdf0, contract_apply = 0x103727c0, staff_reset = 0x10388d90, human = 0x1014c2e0;
}

// The date: a signed halfword day (+0), an unsigned halfword year (+2).
// Whether (year_now, day_now) is after (year, day): the year compared unsigned, then the day signed (the original's xor/srawi/and/subf).
bool after(std::uint32_t year_now, std::int32_t day_now, std::uint32_t year, std::int32_t day) {
    if (year_now > year) return true;
    if (year_now < year) return false;
    return day_now > day;
}

// A double the original builds on its frame: the high word 0x43300000 beside an unsigned integer (the bias is 2^52).
double unsigned_biased(std::uint32_t value) { return std::bit_cast<double>((std::uint64_t(conversion_high) << 32) | value); }

// One run of the routine: the callee-saved registers as the original holds them (r13..r31, indexed by register number), set before every
// call and draw so a callee's prologue saves them.
struct Run {
    Cpu& c;
    Pool pool;
    Fpu fp;
    std::array<std::uint32_t, 32> held{};
    explicit Run(Cpu& cpu) : c(cpu), pool(cpu), fp(cpu) { for (unsigned n = 13; n < 32; ++n) held[n] = c.r(n); }
    void sync() { for (unsigned n = 13; n < 32; ++n) c.set(n, held[n]); }
    // `bl target` through the dispatcher (a routine outside the module, whether generated or C++) at the original's return address `back`.
    std::uint32_t call(std::uint32_t target, std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers) {
        sync();
        c.set(registers);
        const auto link = c.lr();
        c.call(target, back);
        c.set_lr(link);
        return c.r(3);
    }
    // `bl` of a routine of this module, called directly (player_regen.hpp): the registers first, then the link register its prologue saves.
    template <class Callee>
    void direct(std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers, Callee&& callee) {
        sync();
        c.set(registers);
        const auto link = c.lr();
        c.link(back);
        callee();
        c.set_lr(link);
    }
    // One draw below `bound` (none for 0). The bounds here are within 128 in magnitude: the scaled form never runs.
    std::int32_t draw(std::int32_t bound, std::uint32_t back) { sync(); return pool.draw(bound, back, refill_site::random_first); }
    // A modulo by a constant, drawing first: the remainder of the sum (C's `%`, as the compiler's reciprocal multiplication gives it).
    std::int32_t modulo(std::int32_t modulus, std::uint32_t back) {
        sync();
        pool.advance(back);
        return pool.sum() % modulus;
    }
};

// The three variants of a staff record's ability halfwords derived from a retiring player's record, by how good he was and a draw. Each
// halfword is min(factor * max(power, source), limit), truncated, where power is the ability byte squared (the third one, 0.75 times it) and
// source a halfword of the player record: the original evaluates the maximum twice (once to test against the limit, once for the value).
struct Ability {
    std::uint32_t staff_at, source_at;
    bool three_quarters;
    std::uint32_t factor_at, limit_at;
};
constexpr std::array<Ability, 3> strong{{{st::ability_a, pl::value_a, false, 0x28, 0x30},   // 0.9, 7500
                                         {st::ability_b, pl::value_8, false, 0x28, 0x30},
                                         {st::ability_c, pl::value_c, true, 0x28, 0x40}}};  // limit 5750
constexpr std::array<Ability, 3> medium{{{st::ability_a, pl::value_a, false, 0x38, 0x48},    // 0.75, 6000
                                         {st::ability_b, pl::value_8, false, 0x38, 0x48},
                                         {st::ability_c, pl::value_c, true, 0x38, 0x50}}};  // limit 3750
constexpr std::array<Ability, 3> weak{{{st::ability_a, pl::value_a, false, 0x58, 0x60},      // 0.5, 4500
                                       {st::ability_b, pl::value_8, false, 0x58, 0x60},
                                       {st::ability_c, pl::value_c, true, 0x58, 0x68}}};    // limit 1.0

struct Processor {
    Cpu& c;
    Run run;
    Frame& frame;
    std::uint32_t regen;
    std::array<std::uint32_t, 32>& h = run.held;
    Processor(Cpu& cpu, Frame& f, std::uint32_t object) : c(cpu), run(cpu), frame(f), regen(object) {}

    double constant(std::uint32_t offset) const { return c.f64(h[24] + offset); }
    std::uint32_t people() const { return c.u32(h[14]); }
    std::uint32_t table() const { return c.u32(h[26]); }
    std::uint32_t row(std::uint32_t id) const { return table() + id * 100u + 0xc; }  // the kind-table row's byte +0xc: bits 2..3 the class
    std::uint32_t contracts() const { return c.u32(h[23]); }
    std::uint32_t date() const { return h[25]; }
    void set_class(std::uint32_t id, std::uint32_t value) {
        const auto at = row(id);
        c.put8(at, (c.u8(at) & ~0x0cu) | (value << 2 & 0x0cu));
    }

    // pow(base, exponent) through the import glue: the glue saves the caller's r2 at sp+0x14.
    double power(double base, double exponent) {
        c.put32(c.sp() + 0x14, c.r(2));
        return std::pow(base, exponent);
    }
    // The ability byte (+0x5e, unsigned) minus the unsigned bias, as the original builds it on the frame.
    double ability_base(std::uint32_t person) { return run.fp.fsub(unsigned_biased(c.u8(person + p::ability_byte)), constant(k::unsigned_bias)); }
    // A halfword of the player record as a double (the original re-reads it each time).
    double player_half(std::uint32_t person, std::uint32_t at) {
        return run.fp.fsub(biased(sext16(c.u16(c.u32(person + p::player) + at))), constant(constant::bias));
    }
    // max(power, source) (or max(0.75 * power, source)) as the original evaluates it: the power, the source, a compare, and either the power
    // again or the source converted again.
    double larger(std::uint32_t person, const Ability& a) {
        auto& fp = run.fp;
        const auto raised = [&] { return power(ability_base(person), constant(k::square)); };
        if (!a.three_quarters) {
            const double f1 = raised();
            const double source = player_half(person, a.source_at);
            fp.fcmp(0, f1, source);
            if (f1 > source) return raised();
            return player_half(person, a.source_at);
        }
        const double f2 = fp.fmul(constant(k::quarter3), raised());
        const double source = player_half(person, a.source_at);
        fp.fcmp(0, f2, source);
        if (f2 > source) return fp.fmul(constant(k::quarter3), raised());
        return player_half(person, a.source_at);
    }
    // One halfword of the staff record: the product against the limit, then the product again (or the limit) truncated.
    void derive(std::uint32_t person, const Ability& a) {
        auto& fp = run.fp;
        const auto limit = constant(a.limit_at);
        const double product = fp.fmul(constant(a.factor_at), larger(person, a));
        fp.fcmp(0, product, limit);
        double value = limit;
        if (product < limit) value = fp.fmul(constant(a.factor_at), larger(person, a));
        c.put16(c.u32(person + p::staff) + a.staff_at, std::uint32_t(fp.fctiwz(0, value)));
    }

    // The wide Random (a bound of 0x10000 or more in magnitude takes the scaled form: its second draw is a real call of 0x102da630). Both
    // copies here keep the bound in r16 and leave their registers as the nation count's copy does (r4 and r6 the signs, r5 the product, r7 the
    // salt, r8 the sample); `refill` and `call_back` are the return addresses of the copy's refill and of its call of 0x102da630.
    std::int32_t wide(std::int32_t bound, std::uint32_t refill, std::uint32_t call_back);

    // The removal of the entry from list A (0x102df020) after the class bits of its row were cleared (`cancel`: the original's inline code has
    // r5 the row's offset, r6 the table and r7 0 then), `back` the return address of the call.
    void drop(std::uint32_t entry, std::uint32_t back, bool cancel);
    void clear_and_drop(std::uint32_t entry, std::uint32_t back);
    bool step();
    bool retire(std::uint32_t entry, std::uint32_t person);
    void make_staff(std::uint32_t person);
    void both_records(std::uint32_t entry, std::uint32_t person);
    void other_people(std::uint32_t entry, std::uint32_t person);
};

std::int32_t Processor::wide(std::int32_t bound, std::uint32_t refill, std::uint32_t call_back) {
    if (bound > -0x10000 && bound < 0x10000) return run.draw(bound, refill);
    auto& fp = run.fp;
    run.sync();
    run.pool.advance(refill);
    const auto total = run.pool.sum();
    const auto sample = c.u16(cell(c, toc::rng_cursor));
    const auto bias = constant(constant::bias), range = constant(constant::word_range);
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
    const auto salt = c.u32(c.toc(toc::rng_salt));
    const auto second = std::int32_t(run.call(0x102da630, call_back,
        {{3, std::uint32_t(smaller)}, {4, smaller_floor < 0 ? 1u : 0u}, {5, std::uint32_t(product)}, {6, quotient_floor < 0 ? 1u : 0u}, {7, salt}, {8, sample}}));
    const auto tenth_floor = mulhw(0x66666667, std::int32_t(std::uint32_t(second) + 5u)) >> 2;
    const auto tenth = tenth_floor + (tenth_floor < 0);
    auto result = std::int32_t(std::uint32_t(scaled_part) + std::uint32_t(tenth));
    const auto top = std::int32_t(std::uint32_t(bound) - 1u);
    if (result > top) result = top;
    c.set(4, tenth_floor < 0 ? 1u : 0u);
    return result;
}

void Processor::drop(std::uint32_t entry, std::uint32_t back, bool cancel) {
    if (cancel) {
        const auto at = row(c.u32(entry + en::id));
        run.direct(back, {{3, regen}, {4, entry}, {5, at - table()}, {6, table()}, {7, 0}}, [&] { remove_retirement(c, regen, entry); });
    } else {
        run.direct(back, {{3, regen}, {4, entry}}, [&] { remove_retirement(c, regen, entry); });
    }
}
// The person is not to retire after all: the class bits of the entry's row are cleared (rlwimi of 0 into bits 2..3), the entry removed.
void Processor::clear_and_drop(std::uint32_t entry, std::uint32_t back) {
    set_class(c.u32(entry + en::id), 0);
    drop(entry, back, true);
}

// A retirement that is due (0x102d89f8..0x102da530); true when the loop goes on (always).
bool Processor::retire(std::uint32_t entry, std::uint32_t person) {
    h[28] = person;
    // A young person (under 30, not an injury entry) with a club, or in a blocking class, is not retired.
    if (sext8(c.u16(person + p::age)) < 30 && c.u8(entry + en::reason) != 8) {
        if (c.u32(person + p::club) != 0 || (c.u8(row(c.u32(person + p::id))) >> 4) != 0) {
            drop(entry, 0x102d8a58, false);
            return true;
        }
    }
    if (run.call(fn::contract_hold, 0x102d8a68, {{3, contracts()}, {4, person}}) != 0) {
        clear_and_drop(entry, 0x102d8a9c);
        return true;
    }
    if (sext8(run.call(fn::transfer_hold, 0x102d8ab0, {{3, c.u32(h[22])}, {4, person}, {5, 0}})) != 0) {
        clear_and_drop(entry, 0x102d8ae4);
        return true;
    }
    // A person whose club differs from the entry's (the entry's club -1 meaning "was unattached") may be spared on a short contract: a contract
    // record that is missing, or guarded, or has a word +0x50, goes on to the announcement; otherwise the entry is dropped. Everyone else who has
    // no club while the entry has one gets the entry's club cleared.
    const auto club = c.u32(person + p::club);
    const bool changed_club = club != 0 && (c.u32(entry + en::club) == 0xffffffffu || c.u32(club) != c.u32(entry + en::club));
    bool club_fix_up = true;
    if (changed_club) {
        const auto days = run.call(fn::days_between, 0x102d8b2c, {{3, date()}, {4, c.u32(person + p::contract_a)}, {5, c.u32(person + p::contract_b)}});
        if (std::int32_t(days) < 0xd2 && sext8(c.u16(person + p::age)) < 0x26) {
            club_fix_up = false;
            h[15] = run.call(fn::contract_of, 0x102d8b50, {{3, contracts()}, {4, person}});
            if (h[15] != 0 && sext8(run.call(fn::contract_guard, 0x102d8b64, {{3, person}, {4, 0}})) == 0 && c.u32(h[15] + 0x50) == 0) {
                clear_and_drop(entry, 0x102d8ba4);
                return true;
            }
        }
    }
    if (club_fix_up && c.u32(person + p::club) == 0 && c.u32(entry + en::club) != 0xffffffffu) c.put32(entry + en::club, 0xffffffffu);

    // The retirement is announced; the person leaves the contract, national, shortlist and dispute bookkeeping.
    run.direct(0x102d8bdc, {{3, regen}, {4, person}, {5, c.u8(entry + en::reason)}, {6, c.u8(entry + en::announced)}},
               [&] { announce_retirement(c, person, c.u8(entry + en::reason), c.u8(entry + en::announced)); });
    run.call(fn::contract_release, 0x102d8be8, {{3, contracts()}, {4, person}});
    if (c.u32(person + p::player) != 0 && c.u32(person + p::staff) != 0) {
        if (run.call(fn::contract_of, 0x102d8c0c, {{3, contracts()}, {4, person}}) != 0) {
            run.call(fn::contract_of, 0x102d8c20, {{3, contracts()}, {4, person}});
            run.call(fn::contract_end, 0x102d8c24, {});
        }
    } else {
        run.call(fn::contract_clear, 0x102d8c34, {{3, contracts()}, {4, person}});
    }
    run.call(fn::national_report, 0x102d8c44, {{3, cell(c, national_cell)}, {4, person}});
    h[15] = c.u32(c.u32(h[22]) + transfer_object_shortlist);
    if (h[15] != 0) {
        const auto at_club = c.u32(person + p::club);
        if (at_club != 0)
            run.call(fn::shortlist_remove, 0x102d8c74, {{3, h[15]}, {4, person}, {5, at_club}, {6, 4}, {7, 2}});
        run.call(fn::shortlist_forget, 0x102d8c84, {{3, h[15]}, {4, person}, {5, 0}});
        const auto still = c.u32(person + p::club);
        if (still != 0) c.put8(c.u32(h[15] + shortlist_flags) + c.u32(still) * 12u + 9, 100);
    }
    if (c.u32(person + p::player) != 0) {
        run.direct(0x102d8cc0, {{3, regen}, {4, person}}, [&] { detach_player(c, regen, person); });
        if (c.u32(person + p::club) == 0) run.call(fn::contract_clear, 0x102d8cd8, {{3, contracts()}, {4, person}});
        run.call(fn::awol_a, 0x102d8ce4, {{3, c.u32(h[21])}, {4, person}});
        run.call(fn::awol_b, 0x102d8cf0, {{3, c.u32(h[21])}, {4, person}});
        run.call(fn::dispute, 0x102d8d00, {{3, cell(c, dispute_cell)}, {4, person}});
    }
    // A pure player (a player record, no staff record) may be given a staff record.
    if (c.u32(person + p::player) != 0 && c.u32(person + p::staff) == 0) make_staff(person);

    if (c.u32(person + p::staff) != 0 && c.u32(person + p::player) != 0) both_records(entry, person);
    else other_people(entry, person);
    drop(entry, 0x102da530, false);
    return true;
}

// 0x102d8d18..0x102da068: three draws decide whether the retired pure player becomes a non-player (a staff record from list C), and if so
// the record's ability halfwords are derived.
void Processor::make_staff(std::uint32_t person) {
    auto& fp = run.fp;
    // The first draw is below a byte (signed, so never wide).
    const auto byte_67 = sext8(c.u8(person + p::byte_67));
    h[16] = std::uint32_t(byte_67);
    h[15] = byte_67 == 0 ? 0 : std::uint32_t(run.draw(byte_67, 0x102d8d4c));
    // A draw modulo 100 below it ends the chance.
    if (std::int32_t(h[15]) <= run.modulo(100, 0x102d8e94)) return;
    // The second bound: trunc(a cube of the ability byte + the player's halfword), drawn from (wide for a bound of 0x10000 or more).
    const auto cubed = power(ability_base(person), constant(k::cube));
    const auto value = fp.fadd(player_half(person, pl::value_a), cubed);
    const auto bound = std::int32_t(std::uint32_t(fp.fctiwz(0, value)));
    h[16] = std::uint32_t(bound);
    h[15] = bound == 0 ? 0 : std::uint32_t(wide(bound, 0x102d8f60, 0x102d9028));
    // A draw modulo 50000: above it, the person is given a record; otherwise the second chance.
    const auto against = run.modulo(50000, 0x102d90a8);
    c.set(7, c.u16(cell(c, toc::rng_cursor)));  // the sample of that draw
    if (std::int32_t(h[15]) <= against) {
        // The second chance: a scaled draw (the sum's remainder by 65535 times 200000 / 65535) plus a tenth of a draw below 30, at most 199999.
        run.sync();
        run.pool.advance(0x102d910c);
        const auto total = run.pool.sum();
        const auto sample = c.u16(cell(c, toc::rng_cursor));
        constexpr std::int32_t reciprocal = std::int32_t(0x80008001);
        const auto quotient_floor = std::int32_t(std::uint32_t(mulhw(reciprocal, total)) + std::uint32_t(total)) >> 15;
        const auto quotient = quotient_floor + (quotient_floor < 0);
        const auto remainder = std::int32_t(std::uint32_t(total) - std::uint32_t(quotient) * 0xffffu);
        const auto scaled = fp.fmul(constant(k::scale_65535), fp.fsub(biased(remainder), constant(constant::bias)));
        h[15] = std::uint32_t(std::uint32_t(fp.fctiwz(0, scaled)));
        std::int32_t second = 0;
        run.direct(0x102d9188, {{3, 30}, {4, 0xffff}, {5, quotient_floor < 0 ? 1u : 0u}, {6, std::uint32_t(total)}, {7, sample}},
                   [&] { second = draw_below(c, 30); });
        const auto tenth_floor = mulhw(0x66666667, std::int32_t(std::uint32_t(second) + 5u)) >> 2;
        auto sum = std::int32_t(h[15] + std::uint32_t(tenth_floor + (tenth_floor < 0)));
        if (sum > 199999) sum = 199999;
        h[15] = std::uint32_t(sum);
        // r4 the nation, r5 the ability byte: the product of the byte and the nation's halfword (+0xac) must exceed the draw plus 200000.
        const auto nation = c.u32(person + p::nation);
        const auto product = std::int32_t(std::uint32_t(c.u8(person + p::ability_byte)) * std::uint32_t(sext16(c.u16(nation + 0xac))));
        if (product <= std::int32_t(h[15] + 200000u)) return;
    }
    // Given a record of list C (the pool of spare staff records), when it has more than two.
    if (c.i32(regen + staff_pool_list.count) <= 2) return;
    c.put32(person + p::staff, cell(c, staff_records_cell) + c.u32(c.u32(regen + staff_pool_list.entries)) * st::size);
    {
        const auto key = c.u32(c.u32(regen + staff_pool_list.entries));
        run.direct(0x102d9214, {{3, regen}, {4, key}}, [&] { remove_staff_pool_entry(c, regen, std::int32_t(key)); });
    }
    // A record whose player position byte (+0xf) is 20 has its two bytes +0x30, +0x31 put in order, and the second drawn again in one case in four.
    if (c.u8(c.u32(person + p::player) + pl::byte_f) == 0x14) {
        const auto staff = c.u32(person + p::staff);
        const auto first = c.u8(staff + st::byte_30), second = c.u8(staff + st::byte_31);
        if (sext8(first) > sext8(second)) {
            c.put8(staff + st::byte_31, first);
            c.put8(c.u32(person + p::staff) + st::byte_30, second);
            if (run.modulo(4, 0x102d926c) == 0) {
                const auto value = run.modulo(50, 0x102d92b8);
                c.put8(c.u32(person + p::staff) + st::byte_31, std::uint32_t(value + 0x32));
            }
        }
    }
    run.call(fn::finish, 0x102d9304, {{3, person}});
    // The ability halfwords: three variants by his potential (+0xa above 7500) and a draw below a byte of the person (+0x63, signed).
    if (sext16(c.u16(c.u32(person + p::player) + pl::value_a)) > 0x1d4c) {
        for (const auto& a : strong) derive(person, a);
        return;
    }
    const auto byte_63 = sext8(c.u8(person + p::byte_63));
    h[16] = std::uint32_t(byte_63);
    h[15] = byte_63 == 0 ? 0 : std::uint32_t(run.draw(byte_63, 0x102d9744));
    for (const auto& a : h[15] != 0 ? medium : weak) derive(person, a);
}

// A person with both a player and a staff record (0x102da080..0x102da524): he joins list D (the pool of players, with his nations' keys)
// and loses the player record; his club's contract is applied.
void Processor::both_records(std::uint32_t entry, std::uint32_t person) {
    run.call(fn::stats_reset, 0x102da090, {{3, cell(c, stats_cell)}, {4, person}});
    run.call(fn::discipline, 0x102da0a0, {{3, cell(c, discipline_cell)}, {4, person}});
    run.call(fn::injury, 0x102da0b0, {{3, cell(c, injury_cell)}, {4, person}});
    const auto nation = c.u32(person + p::nation);
    if (nation != 0) {
        const auto second = c.u32(person + p::second_nation);
        if (second != 0) {
            const auto id = c.u32(person + p::id);
            const auto first_key = c.u32(nation + 0x68), second_key = c.u32(second + 0x68);
            run.direct(0x102da0dc, {{3, regen}, {4, id}, {5, first_key}, {6, second_key}}, [&] { append_player_pool(c, regen, id, first_key, second_key); });
        } else {
            const auto id = c.u32(person + p::id);
            const auto first_key = c.u32(nation + 0x68);
            run.direct(0x102da0f4, {{3, regen}, {4, id}, {5, first_key}, {6, 0xffffffffu}}, [&] { append_player_pool(c, regen, id, first_key, 0xffffffffu); });
        }
    } else {
        // No nation: a random nation index below the nations' count (wide for a count of 0x10000 or more).
        const auto count = std::int32_t(cell(c, -0x7b04));
        h[16] = std::uint32_t(count);
        h[15] = count == 0 ? 0 : std::uint32_t(wide(count, 0x102da130, 0x102da1f8));
        const auto id = c.u32(person + p::id);
        run.direct(0x102da26c, {{3, regen}, {4, id}, {5, h[15]}, {6, 0xffffffffu}}, [&] { append_player_pool(c, regen, id, h[15], 0xffffffffu); });
    }
    c.set(6, 0);
    c.put32(person + p::player, 0);
    set_class(c.u32(entry + en::id), 0);
    c.put8(person + p::flag_6b, 1);
    if (c.u32(person + p::club) != 0) {
        const auto record = run.call(fn::contract_of, 0x102da2b0, {{3, contracts()}, {4, person}});
        if (record != 0) run.call(fn::contract_apply, 0x102da2c4, {{3, record}, {4, person}, {5, 0}});
    }
}

// A person with no staff record, or with no player record (0x102da2c8..0x102da524): the record {id, day, year} of his regeneration is built on
// the frame (today when he is under 30, else 1..65 days on, or the next year), then he is regenerated at once (a staff person of the right
// job) or detached and registered for later.
void Processor::other_people(std::uint32_t entry, std::uint32_t person) {
    const auto record = frame.at(0x50);
    c.put32(record, c.u32(entry + en::id));
    const auto day = c.u16(date()), year = c.u16(date() + 2);
    if (sext8(c.u16(person + p::age)) < 30) {
        c.put16(record + 4, day);
        c.put16(record + 6, year);
    } else {
        h[15] = std::uint32_t(sext16(day));
        c.put16(record + 4, h[15] + std::uint32_t(run.modulo(65, 0x102da318)) + 1);
        if (sext16(c.u16(record + 4)) > 0x16d) {
            c.put16(record + 6, std::uint32_t(c.u16(date() + 2) + 1));
            c.put16(record + 4, std::uint32_t(run.modulo(35, 0x102da384)));
        } else {
            c.put16(record + 6, c.u16(date() + 2));
        }
    }
    set_class(c.u32(entry + en::id), 2);
    const auto staff = c.u32(person + p::staff);
    const auto place = [&](std::uint32_t back) { run.direct(back, {{3, regen}, {4, record}}, [&] { register_regen(c, regen, record); }); };
    if (staff == 0) {
        place(0x102da524);
        return;
    }
    run.call(fn::staff_reset, 0x102da410, {{3, c.u32(h[13])}, {4, staff}});
    bool regenerates = false;
    if (c.u32(person + p::club) != 0 && c.u8(person + p::job_club) != 5) {
        const auto human = run.call(fn::human, 0x102da43c, {{3, c.u32(person + p::club)}, {4, 1}, {5, 1}, {6, 0}, {7, 0}});
        const auto job = sext8(c.u8(person + p::job_club));
        const bool draws = sext8(human) == 0 || job == 1 || std::uint8_t(c.u8(person + p::job_club) - 2) <= 1 || job == 4;
        if (draws) {
            if (run.modulo(4, 0x102da48c) != 0) {
                regenerates = true;
            } else {
                const auto again = sext8(c.u8(person + p::job_club));
                regenerates = again == 1 || std::uint8_t(c.u8(person + p::job_club) - 2) <= 2 || again == 10 || again == 9;
            }
        }
    }
    if (regenerates) {
        const auto incoming_r20 = h[20];
        run.direct(0x102da4f8, {{3, regen}, {4, person}}, [&] { regenerate_staff(c, regen, person, incoming_r20); });
        return;
    }
    run.direct(0x102da508, {{3, regen}, {4, person}}, [&] { detach_staff(c, regen, person); });
    place(0x102da514);
}

// One entry of the loop; false ends it.
bool Processor::step() {
    const auto entry = h[31];
    const auto first = c.u32(date()), second = c.u32(date() + 4);
    const auto sp = frame.base();
    c.put32(sp + 0x48, first); c.put32(sp + 0x38, first); c.put32(sp + 0x3c, second); c.put32(sp + 0x4c, second);
    const auto year = std::uint32_t(c.u16(date() + 2));
    const auto day = sext16(c.u16(date()));
    if (after(year, day, c.u16(entry + en::year_b), sext16(c.u16(entry + en::day_b)))) {
        const auto id = c.u32(entry + en::id);
        return retire(entry, people() + id * 0x70u);
    }
    // Not yet: an announced entry ends the loop, so does one whose announcement date is still to come.
    if (sext8(c.u8(entry + en::announced)) != 0) return false;
    c.put32(sp + 0x40, first);
    c.put32(sp + 0x44, second);
    if (!after(year, day, c.u16(entry + en::year_a), sext16(c.u16(entry + en::day_a)))) return false;
    const auto id = c.u32(entry + en::id);
    if (((c.u8(row(id)) >> 2) & 3) != 1) {
        const auto person = people() + id * 0x70u;
        const auto reason = c.u8(entry + en::reason);
        run.direct(0x102da5d4, {{3, regen}, {4, person}, {5, reason}}, [&] { announce_staff_retirement(c, person, reason); });
    }
    c.put8(entry + en::announced, 1);
    set_class(c.u32(entry + en::id), 1);
    h[31] += en_size;
    c.set({{3, c.u32(entry + en::id) * 100u + 0xc}, {4, table()}, {5, 1}});
    return true;
}
}

void process_retirements(Cpu& c, std::uint32_t regen) {
    // 0x102d8920: stmw r13, a frame of 0xd0 bytes.
    Frame frame(c, 0x102d8920, 0xd0, {.first_gpr = 13});
    Processor walk(c, frame, regen);
    auto& h = walk.h;
    h[13] = c.toc(news_cell); h[14] = c.toc(toc::people); h[17] = c.toc(toc::rng_salt); h[18] = c.toc(toc::rng_start); h[19] = c.toc(toc::rng_end);
    h[20] = c.toc(toc::rng_cursor); h[21] = c.toc(awol_cell); h[22] = c.toc(managers_cell); h[23] = c.toc(contracts_cell);
    h[24] = c.toc(toc::constants); h[25] = c.toc(toc::year_date); h[26] = c.toc(kind_cell); h[27] = regen;
    walk.run.call(fn::date, 0x102d8970, {{3, frame.at(0x54)}});
    walk.run.call(fn::sort, 0x102d8984,
                 {{3, c.u32(regen + retirement_list.entries)}, {4, c.u32(regen + retirement_list.count)}, {5, en_size}, {6, c.toc(comparator_a)}});
    h[31] = c.u32(regen + retirement_list.entries);
    h[30] = 0;
    h[29] = c.u32(regen + retirement_list.count);
    for (; std::int32_t(h[30]) < std::int32_t(h[29]); ++h[30])
        if (!walk.step()) break;
    walk.run.call(fn::sort, 0x102da61c,
                 {{3, c.u32(regen + queue_list.entries)}, {4, c.u32(regen + queue_list.count)}, {5, queue_size}, {6, c.toc(comparator_b)}});
}
}

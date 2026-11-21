// PLAYER_REGEN.CPP, block `retire_decision`: who retires and when. 0x102dcb20 (the daily scan's decision for any person) and 0x102dd910
// (the new game's scan: a person with a club only) decide whether one person retires and fill the 0x1c-byte retirement entry the caller
// passes (byte +0x18 the reason: 1 age or ability, 2 a player with no club and nothing to draw, 0x20 staff or nobody; +0x19 cleared;
// +4 the club's id or -1; the two dates at +8 and +0x10, each a halfword day, a halfword year and a byte). 0x102de6e0 does it for the
// unattached (a kind 0x10 entry dated today), 0x102de470 queues an injury retirement (kind 8).
//
// Fidelity. The decisions follow the listing branch by branch, re-reading each record field where the original does. Floating point:
// single precision (fsubs, fmuls, frsp) and double (fsub, fmul, fcmpo) as the instructions are, the one fused fnmsub through the
// builder's multiply-add (one rounding); constants come from the table at TOC -0x50a8 (+0 bias, +0x68 1.0, +0x80 0.075, +0x88 0.85,
// +0x90 0.1). Every draw is the pooled generator's, at the original's refill return address.
//
// r19 (0x102dcb20, 0x102dd910): both routines read the callee-saved r19 their caller held when no job byte (+0x62, then +0x60) of a staff
// member is set: `incoming_r19` is that register at the edge (the hooks read it). The callers 0x102d86c0 and 0x102d8830 never write r19:
// it is what their callers held. Only the low byte matters (extsb).
//
// No draw reaches the scaled form: the player's byte +0x3c times 25 lies within 3200 in magnitude, the halfword at +0xa of the player record
// (0x102de6e0) and half a halfword (0x102de470) are below 0x10000; the original's call of 0x102da630 after the scaled arithmetic is
// unreachable. r4-r10: the callers read r8-r10 (0x102dcb20) or r7-r10 (the others) only for what flows on to 0x102dee70, which is what the last
// callee left (player_regen_volatile_read.inc); the routines write none of them on a reachable path.
#include "player_regen_detail.hpp"
#include <array>

namespace cm::aot::player_regen {
namespace {
// Person record fields this block reads (database_init_detail.hpp names the first ones).
namespace p {
constexpr std::uint32_t id = 0x00, nation = 0x0c, player = 0x14, staff = 0x1c, national_link = 0x20, club = 0x24, date_word_a = 0x4c,
                        date_word_b = 0x50, age = 0x5c /* halfword, the low byte is the age */, job_national = 0x60, job_club = 0x62,
                        rating_63 = 0x63, level = 0x65;
}
namespace pl {  // the player record
constexpr std::uint32_t value_4 = 0x04, value_6 = 0x06, value_a = 0x0a, byte_f = 0x0f, byte_14 = 0x14, byte_15 = 0x15, byte_3c = 0x3c;
}
namespace ct {  // the contract record 0x10082f90 returns (inferred)
constexpr std::uint32_t wage = 0x0c, expiry_year = 0x32, byte_44 = 0x44, byte_45 = 0x45, word_50 = 0x50, byte_60 = 0x60;
}
namespace en {  // the retirement entry
constexpr std::uint32_t id = 0x00, club = 0x04, day_a = 0x08, year_a = 0x0a, byte_a = 0x0c, day_b = 0x10, year_b = 0x12, byte_b = 0x14,
                        reason = 0x18, announced = 0x19;
}
namespace cn {  // the constants table
constexpr std::uint32_t bias = 0x00, one = 0x68, rate = 0x80, ratio = 0x88, tenth = 0x90;
}
constexpr std::int32_t contract_manager_cell = -0x7b7c, contract_result_cell = -0x7b78, club_table_cell = -0x7be8, competition_table = -0x7b44,
                       link_table_cell = -0x7b5c, managers_cell = -0x7b40;

// Signed remainder of a (non-negative) pooled sum by n, as divw / mullw / subf give it.
std::int32_t remainder(std::int32_t sum, std::int32_t n) {
    return std::int32_t(std::uint32_t(sum) - std::uint32_t(divide_signed(sum, n)) * std::uint32_t(n));
}

// One run of a routine: the callee-saved registers as the original holds them (indexed by register number, from `first`): the
// callees' prologues save them into the stack image, so they are set before every call and draw.
struct Run {
    Cpu& c;
    unsigned first;
    Pool pool;
    std::array<std::uint32_t, 32> held{};
    Run(Cpu& cpu, unsigned first_gpr) : c(cpu), first(first_gpr), pool(cpu) { for (unsigned n = first; n < 32; ++n) held[n] = c.r(n); }
    void sync() { for (unsigned n = first; n < 32; ++n) c.set(n, held[n]); }
    std::uint32_t call(std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        sync();
        return invoke(c, target, back, arguments);
    }
    void advance(std::uint32_t back) { sync(); pool.advance(back); }
    // The draw below `bound` (none for 0). The bounds here are small: the scaled form never runs.
    std::int32_t draw(std::int32_t bound, std::uint32_t back) { sync(); return pool.draw(bound, back, refill_site::random_first); }
};

// The addresses after the original's `bl refill` and `bl callee` of one of the two scans' routines.
struct Sites {
    std::uint32_t job_a, job_b, job_club, job_none, mod3, rating_63, bound, rem95, rem60, rem30, rem30_second, rem20, rem10,
                  competition_club, competition_nation, days_between, days_in_fill;
};
constexpr Sites daily_sites{0x102dccc8, 0x102dcc50, 0x102dcd58, 0x102dcdd0, 0x102dd1e0, 0x102dd0b4, 0x102dd318, 0x102dd514, 0x102dd584,
                            0x102dd608, 0x102dd688, 0x102dd7e4, 0x102dd84c, 0x102dd4c4, 0x102dd4dc, 0x102dcfd8, 0x102dd7a0};
constexpr Sites initial_sites{0x102ddacc, 0x102dda54, 0x102ddb5c, 0x102ddbd4, 0x102ddd80, 0, 0x102ddea8, 0x102de0a4, 0x102de114,
                              0x102de198, 0x102de218, 0, 0, 0x102de054, 0x102de06c, 0, 0x102de330};

// The job decision of a staff member (no player record, a staff record): one draw taken against his age. `job` is the original's r19 (the
// club-side job byte, else the national-side one, else what the caller held). True when he retires (reason 0x20 stored).
bool staff_retires(Run& run, std::uint32_t person, std::uint32_t entry, std::int32_t age, std::int32_t job, const Sites& site) {
    auto& c = run.c;
    const auto against = [&](std::uint32_t back, std::int32_t modulus, std::int32_t base) {
        run.advance(back);
        if (age <= sext8(std::uint32_t(remainder(run.pool.sum(), modulus) + base))) return false;
        c.put8(entry + en::reason, 0x20);
        return true;
    };
    if (job == 4 || (job < 3 && job > 0)) return against(site.job_a, 10, 0x46);  // jobs 1, 2 and 4
    if (job >= 8 && job < 10) return against(site.job_b, 5, 0x3c);               // jobs 8 and 9
    // Any other job: tied to a club or national team (+0x24, +0x20), or free.
    if (c.u32(person + p::club) != 0 || c.u32(person + p::national_link) != 0) return against(site.job_club, 5, 0x3d);
    return against(site.job_none, 10, 0x37);
}

// The retirement entry of a person who retires: his id, his club's id (or -1) and the two dates, the first the announcement, the second
// the retirement. `comp` is the register (r17 or r19) the original keeps the competition of the club or nation in; the daily scan
// (`draws_out_of_range`) draws dates for a person with no competition, the initial scan dates both tomorrow.
void fill_entry(Run& run, std::uint32_t person, std::uint32_t entry, unsigned comp, const Sites& site, bool draws_out_of_range) {
    auto& c = run.c;
    const auto date = c.toc(toc::year_date);
    const auto table = c.toc(competition_table);
    c.put32(entry + en::id, c.u32(person + p::id));
    c.put8(entry + en::announced, 0);
    const auto club = c.u32(person + p::club);
    c.put32(entry + en::club, club != 0 ? c.u32(club) : 0xffffffffu);
    const auto tomorrow = [&] {
        c.put16(entry + en::day_a, std::uint32_t(sext16(c.u16(date)) + 1));
        c.put16(entry + en::year_a, c.u16(date + 2));
        c.put16(entry + en::day_b, std::uint32_t(sext16(c.u16(date)) + 1));
        c.put16(entry + en::year_b, c.u16(date + 2));
    };
    const auto club_again = c.u32(person + p::club);
    if (club_again == 0 && c.u32(person + p::nation) == 0) { tomorrow(); return; }
    // The competition the club (or, with none, the nation) plays in.
    run.held[comp] = 0xffffffffu;
    if (club_again != 0) {
        run.held[comp] = run.call(0x10191fd0, site.competition_club, {club_again});
    } else if (c.u32(person + p::nation) != 0) {
        run.held[comp] = run.call(0x10191f80, site.competition_nation, {c.u32(person + p::nation)});
    }
    const auto competition = sext8(run.held[comp]);
    if (competition < 0 || competition >= 0x22) {
        if (!draws_out_of_range) { tomorrow(); return; }
        // Outside the competitions: a draw below 20 days ahead, and one below 10 more when it runs past the year's end.
        run.advance(site.rem20);
        const auto day = sext16(std::uint32_t(sext16(c.u16(date)) + remainder(run.pool.sum(), 20)));
        if (day > 0x16d) {
            run.advance(site.rem10);
            c.put16(entry + en::day_b, std::uint32_t(remainder(run.pool.sum(), 10)));
            c.put16(entry + en::year_b, c.u16(date + 2) + 1u);
            c.put16(entry + en::day_a, std::uint32_t(sext16(c.u16(entry + en::day_b))));
            c.put16(entry + en::year_a, c.u16(entry + en::year_b));
        } else {
            c.put16(entry + en::day_a, std::uint32_t(day));
            c.put16(entry + en::year_a, c.u16(date + 2));
            c.put16(entry + en::day_b, std::uint32_t(day));
            c.put16(entry + en::year_b, c.u16(date + 2));
        }
        return;
    }
    // The competition's row (0x4c bytes at TOC -0x7b44): the announcement 1..95 days ahead, the retirement 0..29 days after the row's day
    // halfword (+0x16), a year later (or two) when the row's byte (+0x1c) says so.
    const auto row = table + std::uint32_t(competition) * 0x4c;
    run.advance(site.rem95);
    auto first_day = sext16(std::uint32_t(sext16(c.u16(date)) + remainder(run.pool.sum(), 95) + 1));
    std::int32_t first_year;
    if (first_day > 0x16d) {
        run.advance(site.rem60);
        first_year = sext16(c.u16(date + 2) + 1u);
        first_day = sext16(std::uint32_t(remainder(run.pool.sum(), 60)));
    } else {
        first_year = sext16(c.u16(date + 2));
    }
    c.put16(entry + en::day_a, std::uint32_t(first_day));
    c.put16(entry + en::year_a, std::uint32_t(first_year));
    run.advance(site.rem30);
    c.put16(entry + en::day_b, c.u16(row + 0x16) + std::uint32_t(remainder(run.pool.sum(), 30)));
    if (sext16(c.u16(entry + en::day_b)) > 0x16d) {
        run.advance(site.rem30_second);
        c.put16(entry + en::day_b, std::uint32_t(remainder(run.pool.sum(), 30)));
        c.put16(entry + en::year_b, c.u16(date + 2) + (sext8(c.u8(row + 0x1c)) != 0 ? 2u : 1u));
    } else {
        c.put16(entry + en::year_b, c.u16(date + 2) + (sext8(c.u8(row + 0x1c)) != 0 ? 1u : 0u));
    }
    // The retirement must not come before the announcement: an earlier one replaces it; in the same year the days are compared by
    // the original's bit trick (the sign of (a ^ b) / 2 - ((a ^ b) & a) over the two sign-extended day halfwords).
    const auto first_year_u = std::uint32_t(c.u16(entry + en::year_a)), second_year_u = std::uint32_t(c.u16(entry + en::year_b));
    bool swap;
    if (second_year_u < first_year_u) swap = true;
    else if (second_year_u > first_year_u) swap = false;
    else {
        const auto x = sext16(c.u16(entry + en::day_b)), y = sext16(c.u16(entry + en::day_a));
        const auto t = y ^ x;
        swap = ((t >> 1) - (t & y)) < 0;
    }
    if (swap) {
        c.put16(entry + en::year_b, first_year_u);
        c.put16(entry + en::day_b, std::uint32_t(sext16(c.u16(entry + en::day_a))));
        c.put8(entry + en::byte_b, c.u8(entry + en::byte_a));
    } else {
        const auto days = run.call(0x10718130, site.days_in_fill, {entry + en::day_b, c.u32(entry + en::day_a), c.u32(entry + 0x0c)});
        if (std::int32_t(days) < 0x2d) {
            c.put16(entry + en::year_a, c.u16(entry + en::year_b));
            c.put16(entry + en::day_a, std::uint32_t(sext16(c.u16(entry + en::day_b))));
            c.put8(entry + en::byte_a, c.u8(entry + en::byte_b));
        }
    }
}
}

bool decide_retirement(Cpu& c, std::uint32_t person, std::uint32_t entry, std::uint32_t incoming_r19) {
    // 0x102dcb20: the frame saves r17-r31 (stmw r17), 0xa0 bytes. r17 the contract manager's cell, then the person's contract record
    // (then the competition); r18 the cell then the contract's byte +0x44; r19 the job byte, then the value; r20 the age limit; r21 the age;
    // r22 the constants (then a draw's result); r23 the date; r24 the person; r25 the entry; r26 the verdict; r27 the competition rows.
    Frame frame(c, 0x102dcb20, 0xa0, {.first_gpr = 17});
    Run run(c, 17);
    auto& h = run.held;
    h[17] = c.toc(contract_manager_cell); h[18] = c.toc(contract_result_cell); h[19] = incoming_r19; h[22] = c.toc(toc::constants);
    h[23] = c.toc(toc::year_date); h[24] = person; h[25] = entry; h[26] = 0; h[27] = c.toc(competition_table); h[28] = c.toc(toc::rng_salt);
    h[29] = c.toc(toc::rng_start); h[30] = c.toc(toc::rng_end); h[31] = c.toc(toc::rng_cursor);
    Fpu fp(c);
    const auto date = c.toc(toc::year_date);
    const auto constants = c.toc(toc::constants);

    // The contract manager's holder (0x10082f20; its result is also stored in the cell TOC -0x7b78) and the person's contract record in
    // it (0x10082f90, given the holder's club row or 0); the record's byte +0x44 (-100 for none).
    const auto holder = run.call(0x10082f20, 0x102dcb68, {cell(c, contract_manager_cell)});
    c.put32(c.toc(contract_result_cell), holder);
    const auto holders_club = holder != 0 ? cell(c, club_table_cell) + c.u32(holder + 4) * 0x248 : 0u;
    const auto contract = run.call(0x10082f90, 0x102dcb9c, {cell(c, contract_manager_cell), person, holders_club});
    h[17] = contract;
    h[18] = contract != 0 ? std::uint32_t(c.u8(contract + ct::byte_44)) : std::uint32_t(-0x64);
    const auto byte_44 = [&] { return sext8(h[18]); };
    h[21] = std::uint32_t(sext8(std::uint32_t(c.i16(person + p::age))));
    const auto age = std::int32_t(sext8(h[21]));
    bool retire = false;
    const auto flag = [&](std::uint32_t reason) { retire = true; h[26] = 1; c.put8(entry + en::reason, reason); };

    [&] {
        const auto player = c.u32(person + p::player);
        if (player == 0 && c.u32(person + p::staff) != 0) {
            // A staff member with no player record: his job byte decides one draw against his age.
            if (sext8(c.u8(person + p::job_club)) != 0) h[19] = c.u8(person + p::job_club);
            else if (sext8(c.u8(person + p::job_national)) != 0) h[19] = c.u8(person + p::job_national);
            if (staff_retires(run, person, entry, age, sext8(h[19]), daily_sites)) { retire = true; h[26] = 1; }
            return;
        }
        if (player == 0) { flag(0x20); return; }  // neither a player nor staff
        // A player: the age limit by his positions' ratings (the bytes +0xf, +0x15, +0x14), lowered by the contract and the club.
        h[20] = sext8(c.u8(player + pl::byte_f)) >= 15 ? 0x22u
                : (sext8(c.u8(player + pl::byte_15)) >= 15 || sext8(c.u8(player + pl::byte_14)) >= 15) ? 0x1eu : 0x1fu;
        if (age <= sext8(h[20])) return;
        if (age > 0x21) {
            if (contract == 0) h[20]--;
            else if (c.u32(contract + ct::word_50) != 0) {
                if (player != 0 && (c.u8(player + pl::byte_15) == 0x14 || c.u8(player + pl::byte_14) == 0x14)) h[20]--;
                else if ((c.u8(contract + ct::byte_60) & 0xf) == 7) h[20]--;
            }
        }
        const auto club = c.u32(person + p::club);
        if (club != 0 && contract != 0) {
            const auto level = sext8(c.u8(person + p::level));
            if (c.i32(contract + ct::wage) > level * 0x2ee + 0x1d4c && c.u16(contract + ct::expiry_year) > c.u16(date + 2)) {
                h[20]--;
                if (level < 6) h[20]--;
            }
        }
        // His value: the halfword +0xa of the player record times 1.0 - 0.075 * (age - limit) in single precision, truncated (r19).
        const auto bias = c.f64(constants + cn::bias), rate = c.f64(constants + cn::rate), one = c.f64(constants + cn::one);
        const auto age_f = fp.fsubs(biased(age), bias);
        const auto limit_f = fp.fsubs(biased(sext8(h[20])), bias);
        const auto gap = fp.fsubs(age_f, limit_f);
        const auto value_f = fp.fsubs(biased(sext16(c.u16(player + pl::value_a))), bias);
        const auto factor = fp.frsp(fp.multiply_add(0x102dcf8c, rate, gap, one));  // fnmsub: 1.0 - rate * gap, one rounding
        h[19] = std::uint32_t(fp.fctiwz(0, fp.fmuls(value_f, factor)));
        const auto value = [&] { return sext16(h[19]); };
        if (age > 0x2d) { flag(1); return; }
        if (age > 0x22 && club == 0) {
            const auto days = run.call(0x10718130, daily_sites.days_between, {date, c.u32(person + p::date_word_a), c.u32(person + p::date_word_b)});
            if (std::int32_t(days) > 0x41) { flag(1); return; }
        }
        const auto limit = sext8(h[20]);
        if (age > limit + 2 && age > 0x1f && contract != 0 && sext8(c.u8(contract + ct::byte_45)) > 5) { flag(1); return; }
        if (age > limit + 3 && c.u32(person + p::club) == 0) { flag(1); return; }
        if (age > limit + 3 && contract != 0) {
            const auto contracts_left = sext16(run.call(0x10374d10, 0x102dd070, {contract}));
            if (contracts_left < 300 && byte_44() < 0x32) {
                if (c.u32(person + p::staff) != 0) { flag(1); return; }
                run.advance(daily_sites.rating_63);
                if (sext8(c.u8(person + p::rating_63)) > remainder(run.pool.sum(), 20)) {
                    // His halfword +4 against 0.85 of his +6, in doubles (fsub, fmul, fsub, fcmpo): below it he goes.
                    const auto record = c.u32(person + p::player);
                    const auto ratio = c.f64(constants + cn::ratio), zero = c.f64(constants + cn::bias);
                    const auto six = fp.fsub(biased(sext16(c.u16(record + pl::value_6))), zero);
                    const auto scaled = fp.fmul(ratio, six);
                    const auto four = fp.fsub(biased(sext16(c.u16(record + pl::value_4))), zero);
                    fp.fcmp(0, four, scaled);
                    if (four < scaled) { flag(1); return; }
                }
            }
        }
        if (value() > 0x157c) return;
        // Whether his place is settled: the club's link (+0x30) found in the link table or, with no club, the nation (+0xc) absent or
        // flagged (byte +0x120).
        const auto club2 = c.u32(person + p::club);
        bool settled;
        if (club2 != 0) {
            const auto link = c.u32(club2 + 0x30);
            settled = link != 0 && c.u32(cell(c, link_table_cell) + (c.u32(link) << 2)) != 0;
        } else {
            const auto nation = c.u32(person + p::nation);
            settled = nation == 0 || sext8(c.u8(nation + 0x120)) != 0;
        }
        if (!settled) {
            run.advance(daily_sites.mod3);
            if (remainder(run.pool.sum(), 3) == 0) { flag(1); return; }
            if (c.u32(person + p::club) != 0 && byte_44() < 0) { flag(1); return; }
            if (value() < 0xdac) flag(1);
            return;
        }
        const auto record = c.u32(person + p::player);
        const auto quality = sext16(c.u16(record + pl::value_4));
        if (quality > 0x64) return;
        if (quality < 10) { flag(1); return; }
        if (value() < 0x5dc) { flag(1); return; }
        if (club2 != 0 && byte_44() < 0) { flag(1); return; }
        if (club2 != 0) return;
        // No club: a draw below 25 times his byte +0x3c (a zero draws nothing): a zero retires him.
        h[21] = std::uint32_t(sext8(c.u8(record + pl::byte_3c)) * 0x19);
        if (std::int32_t(h[21]) != 0) h[22] = std::uint32_t(run.draw(std::int32_t(h[21]), daily_sites.bound));
        else h[22] = 0;
        if (std::int32_t(h[22]) == 0) flag(2);
    }();

    if (retire) fill_entry(run, person, entry, 17, daily_sites, true);
    return retire;
}

bool decide_retirement_with_club(Cpu& c, std::uint32_t person, std::uint32_t entry, std::uint32_t incoming_r19) {
    // 0x102dd910: the frame saves r19-r31 (stmw r19), 0xb0 bytes. r19 the job byte, then the competition; r20 the manager cell, then the
    // contract's byte +0x44; r21 the cell, then the age (then a draw's bound); r22 the competition rows; r23-r26 the pool's cells; r27 the
    // constants (then a draw's result); r28 the date; r29 the person; r30 the entry; r31 the verdict.
    Frame frame(c, 0x102dd910, 0xb0, {.first_gpr = 19});
    Run run(c, 19);
    auto& h = run.held;
    h[19] = incoming_r19; h[20] = c.toc(contract_manager_cell); h[21] = c.toc(contract_result_cell); h[22] = c.toc(competition_table);
    h[23] = c.toc(toc::rng_salt); h[24] = c.toc(toc::rng_start); h[25] = c.toc(toc::rng_end); h[26] = c.toc(toc::rng_cursor);
    h[27] = c.toc(toc::constants); h[28] = c.toc(toc::year_date); h[29] = person; h[30] = entry; h[31] = 0;
    Fpu fp(c);
    const auto constants = c.toc(toc::constants);

    // A person with no club does not retire here: nothing is called.
    if (c.u32(person + p::club) == 0) return false;
    const auto holder = run.call(0x10082f20, 0x102dd96c, {cell(c, contract_manager_cell)});
    c.put32(c.toc(contract_result_cell), holder);
    const auto holders_club = holder != 0 ? cell(c, club_table_cell) + c.u32(holder + 4) * 0x248 : 0u;
    const auto contract = run.call(0x10082f90, 0x102dd9a0, {cell(c, contract_manager_cell), person, holders_club});
    h[20] = contract != 0 ? std::uint32_t(c.u8(contract + ct::byte_44)) : std::uint32_t(-0x64);
    const auto byte_44 = [&] { return sext8(h[20]); };
    h[21] = std::uint32_t(sext8(std::uint32_t(c.i16(person + p::age))));
    const auto age = std::int32_t(sext8(h[21]));
    bool retire = false;
    const auto flag = [&](std::uint32_t reason) { retire = true; h[31] = 1; c.put8(entry + en::reason, reason); };

    [&] {
        const auto player = c.u32(person + p::player);
        if (player == 0 && c.u32(person + p::staff) != 0) {
            if (sext8(c.u8(person + p::job_club)) != 0) h[19] = c.u8(person + p::job_club);
            else if (sext8(c.u8(person + p::job_national)) != 0) h[19] = c.u8(person + p::job_national);
            if (staff_retires(run, person, entry, age, sext8(h[19]), initial_sites)) { retire = true; h[31] = 1; }
            return;
        }
        if (player == 0) { flag(0x20); return; }
        const auto limit = sext8(c.u8(player + pl::byte_f)) >= 15 ? 0x22
                           : (sext8(c.u8(player + pl::byte_15)) >= 15 || sext8(c.u8(player + pl::byte_14)) >= 15) ? 0x1e : 0x1f;
        if (age <= limit) return;
        // His score: 1.0 - 0.1 * (age - limit) (double precision, one rounding, truncated to a halfword), times the halfword +0xa of the
        // player record in single precision, truncated (r6).
        const auto bias = c.f64(constants + cn::bias), tenth = c.f64(constants + cn::tenth), one = c.f64(constants + cn::one);
        const auto age_f = fp.fsubs(biased(age), bias);
        const auto limit_f = fp.fsubs(biased(limit), bias);
        const auto gap = fp.fsubs(age_f, limit_f);
        const auto value_f = fp.fsubs(biased(sext16(c.u16(player + pl::value_a))), bias);
        const auto factor = fp.multiply_add(0x102ddce0, tenth, gap, one);  // fnmsub: 1.0 - 0.1 * gap, one rounding
        const auto whole = sext16(std::uint32_t(fp.fctiwz(0, factor)));
        const auto whole_f = fp.fsubs(biased(whole), bias);
        const auto score = sext16(std::uint32_t(fp.fctiwz(0, fp.fmuls(value_f, whole_f))));
        if (age > 0x2d) { flag(1); return; }
        const auto club = c.u32(person + p::club);
        bool draw_three;  // the club has no link (+0x30), or (no club) the nation exists without the flag (+0x120)
        if (club != 0) draw_three = c.u32(club + 0x30) == 0;
        else {
            const auto nation = c.u32(person + p::nation);
            draw_three = nation != 0 && sext8(c.u8(nation + 0x120)) == 0;
        }
        if (draw_three) {
            run.advance(initial_sites.mod3);
            if (remainder(run.pool.sum(), 3) != 0) { flag(1); return; }
            if (c.u32(person + p::club) != 0 && byte_44() < 0) flag(1);
            return;
        }
        if (score >= 0x1388) return;
        const auto quality = sext16(c.u16(player + pl::value_4));
        if (quality > 0x50) return;
        if (quality < 10) { flag(1); return; }
        if (score < 0x5dc) { flag(1); return; }
        if (club != 0 && byte_44() < 0) { flag(1); return; }
        if (club != 0) return;
        h[21] = std::uint32_t(sext8(c.u8(player + pl::byte_3c)) * 0x19);
        if (std::int32_t(h[21]) != 0) h[27] = std::uint32_t(run.draw(std::int32_t(h[21]), initial_sites.bound));
        else h[27] = 0;
        if (std::int32_t(h[27]) == 0) flag(2);
    }();

    if (retire) fill_entry(run, person, entry, 19, initial_sites, false);
    return retire;
}

bool decide_unattached_retirement(Cpu& c, std::uint32_t person, std::uint32_t entry) {
    // 0x102de6e0: the frame saves r23-r31 (stmw r23), 0x80 bytes. r23 the person, r24 the entry, r25 the verdict, r26 the managers' cell
    // (then the player's halfword), r27 the salt cell (then a draw's result), r28-r30 the pool's cells, r31 the date.
    Frame frame(c, 0x102de6e0, 0x80, {.first_gpr = 23});
    Run run(c, 23);
    auto& h = run.held;
    h[23] = person; h[24] = entry; h[25] = 0; h[26] = c.toc(managers_cell); h[27] = c.toc(toc::rng_salt); h[28] = c.toc(toc::rng_start);
    h[29] = c.toc(toc::rng_end); h[30] = c.toc(toc::rng_cursor); h[31] = c.toc(toc::year_date);
    const auto date = c.toc(toc::year_date);
    // The human manager's protect list (the object at +0x988 of the managers' cell) holds the person: he is left alone.
    const auto protects = [&](std::uint32_t back) {
        return sext8(run.call(0x10362d70, back, {c.u32(cell(c, managers_cell) + 0x988), person, 0})) != 0;
    };
    if (protects(0x102de724)) return false;
    const auto staff = c.u32(person + p::staff);
    if (staff == 0 && c.u32(person + p::player) == 0) h[25] = 1;
    if (staff != 0) {
        // Non-player staff: only with no club and no national link, and a byte +0x2a (halfword at the record's +0x2a) under 50: one in five.
        if (c.u32(person + p::club) != 0 || c.u32(person + p::national_link) != 0) return false;
        if (c.i16(staff + 0x2a) < 0x32) {
            run.advance(0x102de79c);
            if (remainder(run.pool.sum(), 5) == 0) h[25] = 1;
        }
    } else if (c.u32(person + p::player) != 0) {
        if (protects(0x102de808)) return false;
        // A player with no club and no national link for over a year (365 days): he goes when a draw below his halfword +0xa (nothing
        // for 0) is under 750.
        if (c.u32(person + p::club) == 0 && c.u32(person + p::national_link) == 0) {
            const auto years = std::int32_t(std::uint32_t(c.u16(date + 2)) - std::uint32_t(c.u16(person + 0x4e)));
            const auto idle = std::int32_t(std::uint32_t(years) * 0x16du + std::uint32_t(sext16(c.u16(date)) - sext16(c.u16(person + 0x4c))));
            if (idle > 0x16d) {
                h[26] = std::uint32_t(sext16(c.u16(c.u32(person + p::player) + 0x0a)));
                if (std::int32_t(h[26]) != 0) h[27] = std::uint32_t(run.draw(std::int32_t(h[26]), 0x102de890));
                else h[27] = 0;
                if (std::int32_t(h[27]) < 0x2ee) h[25] = 1;
            }
        }
    }
    if (sext8(h[25]) == 0) return false;
    // A kind 0x10 entry dated today (both dates).
    c.put32(entry + en::id, c.u32(person + p::id));
    c.put8(entry + en::reason, 0x10);
    c.put8(entry + en::announced, 0);
    const auto club = c.u32(person + p::club);
    c.put32(entry + en::club, club != 0 ? c.u32(club) : 0xffffffffu);
    c.put16(entry + en::day_a, std::uint32_t(sext16(c.u16(date))));
    c.put16(entry + en::year_a, c.u16(date + 2));
    c.put16(entry + en::day_b, std::uint32_t(sext16(c.u16(date))));
    c.put16(entry + en::year_b, c.u16(date + 2));
    return true;
}

void schedule_injury_retirement(Cpu& c, std::uint32_t regen, std::uint32_t person, std::uint32_t days_register) {
    // 0x102de470: the frame saves r27-r31 (stmw r27), 0x90 bytes; the entry it appends is built in the frame (+0x40..), the dates beside it
    // (+0x48 and +0x50 begun by 0x10715880, +0x38 the sum 0x10717890 makes). r27 the person, r28 the regen object, r29 the days (then half
    // of them), r30 the salt cell (then the draw), r31 the cursor cell.
    Frame frame(c, 0x102de470, 0x90, {.first_gpr = 27});
    Run run(c, 27);
    auto& h = run.held;
    h[27] = person; h[28] = regen; h[29] = days_register; h[30] = c.toc(toc::rng_salt); h[31] = c.toc(toc::rng_cursor);
    run.call(0x10715880, 0x102de4a0, {frame.at(0x48)});
    run.call(0x10715880, 0x102de4a8, {frame.at(0x50)});
    // A pending entry for the same person is replaced (0x102eb610 asks, 0x102eb670 removes: both C++ of this module).
    run.sync();
    c.set({{3, regen}, {4, person}});
    if (sext8(retirement_queued(c, regen, person)) != 0) {
        run.sync();
        c.set({{3, regen}, {4, person}});
        remove_retirement_by_id(c, regen, person);
    }
    c.put32(frame.at(0x40), c.u32(person + p::id));
    const auto club = c.u32(person + p::club);
    c.put32(frame.at(0x44), club != 0 ? c.u32(club) : 0xffffffffu);
    // The delay: a draw below half the injury's days (halfword, rounded toward zero; none for 0), plus one day.
    const auto days = sext16(h[29]);
    h[29] = std::uint32_t((days + (days < 0 ? 1 : 0)) >> 1);
    std::uint32_t delay = 0;  // r30 holds the salt cell's address through the draw and the result after it
    if (std::int32_t(h[29]) != 0) delay = std::uint32_t(run.draw(std::int32_t(h[29]), 0x102de534));
    h[30] = delay;
    run.call(0x10717890, 0x102de674, {frame.at(0x38), c.toc(-0x7b80), std::uint32_t(sext16(h[30] + 1))});
    // The new date: the sum's day halfword, year halfword and byte, for both dates of the entry (announcement and retirement).
    c.put16(frame.at(0x4a), c.u16(frame.at(0x3a)));
    c.put16(frame.at(0x48), c.u16(frame.at(0x38)));
    c.put8(frame.at(0x4c), c.u8(frame.at(0x3c)));
    c.put16(frame.at(0x52), c.u16(frame.at(0x4a)));
    c.put16(frame.at(0x50), c.u16(frame.at(0x48)));
    c.put8(frame.at(0x54), c.u8(frame.at(0x4c)));
    c.put8(frame.at(0x58), retirement_entry::injury_kind);
    c.put8(frame.at(0x59), 0);
    run.sync();
    // The registers the original's last writes leave for the append (and so, through it, for the caller, which passes them on): r5 the kind,
    // r6 the date's byte, r7 its day (sign-extended), r8 its year; r9 and r10 stay what the date routine left.
    c.set({{3, regen}, {4, frame.at(0x40)}, {5, retirement_entry::injury_kind}, {6, c.u8(frame.at(0x4c))},
           {7, std::uint32_t(sext16(c.u16(frame.at(0x48))))}, {8, c.u16(frame.at(0x4a))}});
    append_retirement(c, regen, frame.at(0x40));
}
}

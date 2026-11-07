// DATABASE.CPP's people block, part "staff": the staff-candidate score and its two users (database_people_staff.hpp).
#include "database.hpp"
#include "database_people_detail.hpp"
#include "database_people_staff.hpp"
#include "guest_fpu.hpp"
#include <array>
#include <optional>

namespace cm::aot::database {
namespace {
using namespace people;
using guest::Fpu;

constexpr std::uint32_t calendar_difference = 0x10718130;  // (calendar, date words) -> days between
constexpr std::uint32_t contract_with_club = 0x10082f90;   // CONTRACT_MANAGER.CPP: (contracts, person, club)
constexpr std::uint32_t transfer_check = 0x103f43b0;       // TRANSFER_MANAGER.CPP: (object, person, 0) -> byte
constexpr std::uint32_t player_stat = 0x10303560;          // PLAYER_STATS.CPP: (object, record, mode, number, f1 default) -> f1

// The TOC words both routines keep in registers (r20-r26, or r23-r29 in the second).
constexpr std::int32_t floats_slot = -0x56bc, transfers_slot = -0x7b40, stats_slot = -0x7b3c, contracts_slot = -0x7b7c,
                       calendar_slot = -0x7b80, count_slot = -0x7be4, doubles_slot = -0x5614;
struct Pool {
    std::uint32_t floats, transfers_cell, stats_cell, contracts_cell, calendar, count_cell, doubles;
};
Pool load_pool(Cpu& c, unsigned first_register) {
    const Pool pool{c.toc(floats_slot), c.toc(transfers_slot), c.toc(stats_slot), c.toc(contracts_slot),
                    c.toc(calendar_slot), c.toc(count_slot), c.toc(doubles_slot)};
    c.set(first_register, pool.floats);
    c.set(first_register + 1, pool.transfers_cell);
    c.set(first_register + 2, pool.stats_cell);
    c.set(first_register + 3, pool.contracts_cell);
    c.set(first_register + 4, pool.calendar);
    c.set(first_register + 5, pool.count_cell);
    c.set(first_register + 6, pool.doubles);
    return pool;
}

// Offsets (inferred): person and candidate records.
constexpr std::uint32_t id = 0x0, nation = 0xc, player_record = 0x14, preferences_record = 0x18, join_date_a = 0x4c,
                        join_date_b = 0x50, halfword_5c = 0x5c, byte_60 = 0x60, byte_63 = 0x63, byte_65 = 0x65,
                        byte_66 = 0x66, byte_67 = 0x67, club_slots = 0x98, club_nation = 0x2c, club_value = 0x1ec;
// The preferences record's lists of three: favoured staff (+0x1c) and disliked staff (+0x28).
constexpr std::array<std::uint32_t, 3> favoured_staff{0x1c, 0x20, 0x24}, disliked_staff{0x28, 0x2c, 0x30};
// The player record (+0x14): the value halfword, the other halfword and the bytes.
constexpr std::uint32_t record_value = 0xa, record_halfword_4 = 0x4, record_byte_21 = 0x21;
// The contract: a byte (+0x60, its low nibble picks an extra) and the flags word (+0x50).
constexpr std::uint32_t contract_flags_50 = 0x50;
// The statistics object's two doubles (inferred: a total and a count), and the pool entries.
constexpr std::uint32_t stats_a = 0x10, stats_b = 0x18;

// XER[CA] as `subfic` and `srawi` leave it (byte 1027 of the register file).
void set_carry(Cpu& c, bool carry) { c.s.wr<1>(guest::reg::xer_ca, carry); }
std::uint32_t arithmetic_shift(Cpu& c, std::uint32_t value, unsigned amount) {  // srawi
    set_carry(c, (value >> 31) && (value & ((1u << amount) - 1)) != 0);
    return std::uint32_t(std::int32_t(value) >> amount);
}

// Which registers and return addresses one of the two routines uses.
struct Backs {
    std::uint32_t days_head, position_first, position_second, contract, transfer, stat_first, stat_second, days_tail;
};
struct Context {
    Cpu& c;
    Pool k;
    std::uint32_t person, club;
    Backs back;
};

// `cmplwi r,0; li r4,0; beq; cmplwi owner,0; beq; lwz r0,0x18(owner); cmplwi r0,0; beq; then the three list
// slots against `candidate` (each reloads the record), r4 = 1 on a hit; finally cmpwi r4,0.
bool listed_in(Context& x, std::uint32_t candidate, const std::array<std::uint32_t, 3>& slots) {
    Cpu& c = x.c;
    compare_logical(c, candidate);
    c.set(4, 0);
    bool hit = false;
    if (candidate != 0 && compare_logical(c, x.person) != 0 && compare_logical(c, load32(c, 0, x.person + preferences_record)) != 0) {
        for (const auto slot : slots) {
            const auto record = load32(c, 3, x.person + preferences_record);
            const auto entry = load32(c, 0, record + slot);
            if (compare_logical(c, entry, candidate) == 0) {
                hit = true;
                break;
            }
        }
    }
    c.set(4, hit);
    compare(c, hit);
    return hit;
}

// One of the two ratio tests (the statistics object's +0x10 / +0x18, or the pool's constant when the divisor
// equals the pool's zero), single-rounded, added to the pool constant at `constant`, against f31.
bool exceeds_ratio(Context& x, Fpu& fpu, std::uint32_t constant) {
    Cpu& c = x.c;
    const auto object = load32(c, 3, x.k.stats_cell);
    c.set_f(0, c.f64(x.k.doubles + 0x40));
    c.set_f(1, c.f64(object + stats_b));
    c.set_f(2, c.f64(x.k.doubles + constant));
    fpu.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 2) {
        c.set_f(0, c.f64(x.k.doubles + 0x128));
    } else {
        c.set_f(1, c.f64(object + stats_a));
        c.set_f(0, c.f64(object + stats_b));
        c.set_f(0, fpu.fdiv(c.f(1), c.f(0)));
    }
    c.set_f(0, fpu.frsp(c.f(0)));
    c.set_f(0, fpu.fadd(c.f(2), c.f(0)));
    fpu.fcmp(0, c.f(31), c.f(0));
    return (c.cr(0) & 4) != 0;
}

// The score of the candidate held in register `Candidate` against the person and the club, with every
// register and CR field left as the original leaves them. `Score` and `Contract` are the registers the original
// keeps the running score and the contract in. nullopt: the candidate is skipped.
template <unsigned Candidate, unsigned Score, unsigned Contract>
std::optional<std::int32_t> score_candidate(Context& x, bool with_positions) {
    Cpu& c = x.c;
    const auto candidate = c.r(Candidate), person = x.person, club = x.club;
    std::int32_t score = 0;
    const auto add = [&](std::int32_t amount) {
        score += amount;
        c.set(Score, std::uint32_t(score));
    };
    if (compare_logical(c, candidate) == 0) return std::nullopt;
    const auto record = load32(c, 0, candidate + player_record);
    if (compare_logical(c, record) == 0) return std::nullopt;
    compare_logical(c, candidate);
    const auto candidate_id = load32(c, 3, candidate + id);
    const auto people_count = load32(c, 0, x.k.count_cell);
    if (compare(c, std::int32_t(candidate_id), std::int32_t(people_count)) >= 0) return std::nullopt;
    if (compare_logical(c, candidate, person) == 0) return std::nullopt;
    {  // (candidate id + person id) % 3 == 0 skips: the signed multiply-high division
        load32(c, 4, candidate + id);
        c.set(3, 0x55555556);
        const auto sum = c.r(4) + load32(c, 0, person + id);
        c.set(4, sum);
        const auto high = mulhw(0x55555556, sum);
        c.set(3, high);
        const auto remainder = sum - 3 * (high + (high >> 31));
        c.set(0, remainder);
        if (compare(c, std::int32_t(remainder)) == 0) return std::nullopt;
    }
    if (listed_in(x, candidate, disliked_staff)) return std::nullopt;
    if (with_positions) {
        const auto kind = extsb(load8(c, 0, candidate + byte_66));
        c.set(0, kind);
        if (compare(c, std::int32_t(kind), 0xb) < 0) {
            guest::enter(c, x.back.position_first, {person});
            position_mask(c, person);
            c.set(Score, c.r(3));
            guest::enter(c, x.back.position_second, {candidate});
            position_mask(c, candidate);
            if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(Score))) == 0) return std::nullopt;
        }
    }
    c.set({{3, c.u32(x.k.contracts_cell)}, {4, candidate}, {5, club}});
    c.call_in_place(contract_with_club, x.back.contract);
    c.set(Contract, c.r(3));
    if (compare(c, std::int32_t(c.r(3))) == 0) return std::nullopt;
    const auto contract = c.r(Contract);

    const auto halfword = [&](std::uint32_t base, std::uint32_t offset) { return guest::extended(c.i16(base + offset)); };
    {
        const auto r3 = load32(c, 3, candidate + player_record);
        c.set(0, halfword(r3, record_value));
        if (compare(c, std::int32_t(c.r(0)), 0x109a) <= 0) return std::nullopt;
    }
    {
        const auto r4 = load32(c, 4, candidate + player_record);
        c.set(3, halfword(club, club_value));
        c.set(4, halfword(r4, record_value));
        c.set(0, c.r(3) - 0x2ee);
        if (compare(c, std::int32_t(c.r(4)), std::int32_t(c.r(0))) <= 0) return std::nullopt;
    }
    c.set({{3, c.u32(x.k.transfers_cell)}, {4, candidate}, {5, 0}});
    c.call_in_place(transfer_check, x.back.transfer);
    c.set(0, extsb(c.r(3)));
    if (compare(c, std::int32_t(c.r(0))) != 0) return std::nullopt;
    {
        const auto r3 = load32(c, 3, candidate + player_record);
        c.set(0, halfword(r3, record_halfword_4));
        if (compare(c, std::int32_t(c.r(0)), 0x64) <= 0) return std::nullopt;
    }

    // Two statistics of the record: the first (number 0x11, mode 6) is kept in f31, the second (number 1)
    // is compared with the pool's third single.
    Fpu fpu(c);
    const auto call_stat = [&](std::uint32_t number, std::uint32_t back) {
        c.set(3, c.u32(x.k.stats_cell));
        c.set(5, 6);
        load32(c, 4, candidate + player_record);
        c.set(6, number);
        c.set_f(1, double(c.f32(x.k.floats)));
        c.call_in_place(player_stat, back);
    };
    call_stat(0x11, x.back.stat_first);
    c.s.wr<8>(guest::reg::fpr(0), c.s.r<8>(guest::reg::fpr(1)));
    c.s.wr<8>(guest::reg::fpr(31), c.s.r<8>(guest::reg::fpr(0)));  // fmr f31,f0 (after the second call's setup)
    call_stat(1, x.back.stat_second);
    c.set_f(0, double(c.f32(x.k.floats + 0x18)));
    const auto r3 = load32(c, 3, candidate + player_record);
    fpu.fcmp(0, c.f(1), c.f(0));
    score = std::int32_t(halfword(r3, record_value));
    c.set(Score, std::uint32_t(score));
    if (c.cr(0) & 4) {
        if (exceeds_ratio(x, fpu, 0x120)) add(0x1f4);
        else if (exceeds_ratio(x, fpu, 0x60)) add(0xfa);
    }
    if (listed_in(x, candidate, favoured_staff)) add(0x4e2);

    {
        const auto r3a = load32(c, 3, candidate + player_record);
        const auto byte = load8(c, 0, r3a + record_byte_21);
        if (compare(c, std::int32_t(byte), 0x14) == 0) {
            add(0x4b);
        } else {
            const auto r3b = load32(c, 3, candidate + player_record);
            c.set(0, extsb(load8(c, 0, r3b + record_byte_21)));
            if (compare(c, std::int32_t(c.r(0)), 0xf) > 0) add(0x32);
        }
    }
    {
        c.set(0, extsb(load8(c, 0, candidate + byte_67)));
        if (compare(c, std::int32_t(c.r(0)), 0xf) > 0) {
            add(0xfa);
        } else {
            c.set(0, extsb(load8(c, 0, candidate + byte_67)));
            if (compare(c, std::int32_t(c.r(0)), 6) < 0) add(-0x64);
        }
    }
    {  // (candidate id + person id) % 500, then the contract byte's low nibble picks an extra
        const auto first = load32(c, 5, candidate + id);
        c.set(3, 0x10624dd3);
        const auto second = load32(c, 4, person + id);
        const auto byte = load8(c, 0, contract + byte_60);
        const auto sum = first + second;
        c.set(4, sum);
        const auto high = mulhw(0x10624dd3, sum);
        c.set(3, high);
        const auto nibble = byte & 0xf;
        c.set(5, nibble);
        compare(c, std::int32_t(nibble), 2);
        auto quotient = arithmetic_shift(c, high, 5);
        c.set(0, quotient);
        c.set(3, quotient >> 31);
        quotient += quotient >> 31;
        c.set(0, quotient);
        quotient *= 500;
        c.set(0, quotient);
        const auto remainder = sum - quotient;
        c.set(0, remainder);
        add(std::int32_t(remainder));
        if (nibble == 2) {
            add(0x12c);
        } else if (nibble > 2) {
            compare(c, std::int32_t(nibble), 4);
            if (nibble < 4) add(0x96);
        } else {
            compare(c, std::int32_t(nibble), 1);
            if (nibble >= 1) add(0x2ee);
        }
    }
    {  // same nation as the person but not the club's: a contract flag test, else a date difference
        const auto r3 = load32(c, 3, person + nation);
        const auto r0 = load32(c, 0, club + club_nation);
        if (compare_logical(c, r3, r0) != 0) {
            const auto cand_nation = load32(c, 3, candidate + nation);
            const auto person_nation = load32(c, 0, person + nation);
            if (compare_logical(c, cand_nation, person_nation) == 0) {
                c.set(3, 0x20020);
                const auto flags = c.u32(contract + contract_flags_50) & 0x20020;
                c.set(0, flags);
                c.set(3, 0x20020);
                if (compare(c, std::int32_t(flags)) != 0) {
                    add(0x6d6);
                } else {
                    load32(c, 4, person + join_date_a);
                    c.set(3, x.k.calendar);
                    load32(c, 5, person + join_date_b);
                    c.call(calendar_difference, x.back.days_tail);
                    if (compare(c, std::int32_t(c.r(3)), 0x15e) >= 0) {
                        add(0x7d);
                    } else {
                        c.set(0, extsb(c.u16(person + halfword_5c)));
                        if (compare(c, std::int32_t(c.r(0)), 0x18) >= 0) add(0x7d);
                        else add(0xfa);
                    }
                }
            }
        }
    }
    return score;
}

// The checks both routines make before looking at a candidate (r27/r30 the person, r28/r31 the club): the
// person's two bytes against id-derived windows and the date difference against id % 28 + 14. False: both
// routines answer 0 (r3 is set).
bool person_is_eligible(Context& x) {
    Cpu& c = x.c;
    const auto fail = [&] {
        c.set(3, 0);
        return false;
    };
    if (compare_logical(c, load32(c, 0, x.k.stats_cell)) == 0) return fail();
    const auto six = [&](std::uint32_t byte_offset, bool upper) {
        c.set(3, 0x2aaaaaab);
        const auto person_id = load32(c, 4, x.person + id);
        const auto byte = load8(c, 0, x.person + byte_offset);
        const auto high = mulhw(0x2aaaaaab, person_id);
        c.set(5, extsb(byte));
        const auto remainder = person_id - 6 * (high + (high >> 31));
        if (upper) {
            c.set(3, remainder);
            c.set(0, remainder + 6);
            return compare(c, std::int32_t(extsb(byte)), std::int32_t(remainder + 6)) < 0;  // blt: fail
        }
        c.set(3, high);
        set_carry(c, 0xf >= remainder);  // subfic
        c.set(0, 0xf - remainder);
        return compare(c, std::int32_t(extsb(byte)), std::int32_t(0xf - remainder)) > 0;  // ble: pass
    };
    if (six(byte_65, true)) return fail();
    if (six(byte_63, false)) return fail();
    load32(c, 4, x.person + join_date_a);
    c.set(3, x.k.calendar);
    load32(c, 5, x.person + join_date_b);
    c.call(calendar_difference, x.back.days_head);
    const auto days = c.r(3);
    c.set(4, 0x92492493);
    const auto person_id = load32(c, 5, x.person + id);
    const auto high = mulhw(0x92492493, person_id);
    auto quotient = arithmetic_shift(c, high + person_id, 4);
    c.set(0, quotient);
    c.set(4, quotient >> 31);
    quotient += quotient >> 31;
    c.set(0, quotient);
    c.set(0, quotient * 0x1c);
    const auto remainder = person_id - quotient * 0x1c;
    c.set(4, remainder);
    c.set(0, remainder + 0xe);
    if (compare(c, std::int32_t(days), std::int32_t(remainder + 0xe)) < 0) return fail();
    return true;
}
}

// 0x100dc4d0: a frame of 0x90 saving r17-r31 (at -0x4c) and f31; r27 the person, r28 the club, r29 the slot
// index, r30 the best score so far (starts -10000), r31 the best candidate, r17 the running score, r18 the
// contract, r19 the candidate.
std::uint32_t best_staff_candidate(Cpu& c, std::uint32_t person, std::uint32_t club) {
    constexpr std::uint32_t entry = 0x100dc4d0;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x90, {.first_gpr = 17, .gpr_offset = -0x4c, .first_fpr = 31});
    const auto pool = load_pool(c, 20);
    c.set({{27, person}, {28, club}, {31, 0}, {30, 0xffffd8f0}});
    const auto fail = [&] {
        c.set(3, 0);
        return 0u;
    };
    if (compare(c, std::int32_t(person)) == 0) return fail();
    if (compare_logical(c, club) == 0) return fail();
    Context x{c, pool, person, club,
              {0x100dc5b8, 0x100dc6dc, 0x100dc6e8, 0x100dc700, 0x100dc740, 0x100dc770, 0x100dc790, 0x100dc9a0}};
    if (!person_is_eligible(x)) return 0;
    for (std::uint32_t slot = 0;; ++slot) {
        c.set(29, slot);
        if (compare(c, std::int32_t(slot), 0x32) >= 0) break;
        c.set(0, slot * 4);
        c.set(3, club + slot * 4);
        c.set(19, c.u32(club + slot * 4 + club_slots));
        const auto score = score_candidate<19, 17, 18>(x, true);
        if (!score) continue;
        if (compare(c, *score, std::int32_t(c.r(30))) > 0) {
            c.set(31, c.r(19));
            c.set(30, std::uint32_t(*score));
        }
    }
    c.set(3, c.r(31));
    return c.r(3);
}

// 0x100dbc20: a frame of 0x80 saving r18-r31 (at -0x48) and f31; r30 the person, r31 the club, r18 the
// candidate (then the slot index), r19 its score, r20 the running score of a slot's candidate, r21 the
// contract, r22 the slot's candidate.
bool no_better_staff_candidate(Cpu& c, std::uint32_t person, std::uint32_t club, std::uint32_t candidate) {
    constexpr std::uint32_t entry = 0x100dbc20;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x80, {.first_gpr = 18, .gpr_offset = -0x48, .first_fpr = 31});
    const auto pool = load_pool(c, 23);
    c.set({{30, person}, {31, club}, {18, candidate}});
    const auto fail = [&] {
        c.set(3, 0);
        return false;
    };
    if (compare(c, std::int32_t(person)) == 0) return fail();
    if (compare_logical(c, club) == 0) return fail();
    if (compare_logical(c, candidate) == 0) return fail();
    Context x{c, pool, person, club, {0x100dbd0c, 0, 0, 0, 0, 0, 0, 0}};
    if (!person_is_eligible(x)) return false;
    // The candidate itself (r18, score in r19).
    x.back = {0x100dbd0c, 0, 0, 0x100dbe14, 0x100dbe54, 0x100dbe84, 0x100dbea4, 0x100dc0b4};
    if (!score_candidate<18, 19, 21>(x, false)) return fail();
    // Every slot's candidate (r22, score in r20).
    x.back = {0x100dbd0c, 0, 0, 0x100dc1cc, 0x100dc20c, 0x100dc23c, 0x100dc25c, 0x100dc46c};
    for (std::uint32_t slot = 0;; ++slot) {
        c.set(18, slot);
        if (compare(c, std::int32_t(slot), 0x32) >= 0) break;
        c.set(0, slot * 4);
        c.set(3, club + slot * 4);
        c.set(22, c.u32(club + slot * 4 + club_slots));
        const auto score = score_candidate<22, 20, 21>(x, false);
        if (!score) continue;
        if (compare(c, *score, std::int32_t(c.r(19))) > 0) return fail();
    }
    c.set(3, 1);
    return true;
}
}

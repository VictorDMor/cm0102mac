// FINANCE.CPP, block value: recomputing an account's budget figures (0x100f4f80).
#include "finance_value.hpp"
#include <stdexcept>

namespace cm::aot::finance {
using guest::Frame;
using namespace value_detail;

namespace {
namespace callee_of_value {
constexpr std::uint32_t contract_of = 0x10082f90,   // (game, person, club): the person's contract with the club
    contract_by_index = 0x10082f20,                 // (game, person)
    signing_value = 0x1037b6f0, loan_record = 0x10083110;
}
constexpr std::uint32_t contracts_at = 0x98, scouts_at = 0x160, coaches_at = 0x174, staff_at = 0x190;
constexpr std::uint32_t total = 0x0c, largest = 0x10, budget = 0x14;
}

void refresh_account_budget(Cpu& c, std::uint32_t account) {
    Frame frame(c, 0x100f4f80, 0x80, {.first_gpr = 22});
    Numbers n(c);
    const auto game_cell = c.toc(-0x7b7c);
    const auto club = club_record(c, c.u32(account + account::club));
    c.put32(account + largest, 0);
    c.put32(account + total, 0);
    // Registers the original keeps its loop state in; the callees save them in their frames.
    std::uint32_t value = c.r(22), record = c.r(24);
    std::uint32_t count = 0, index = 0, offset = 0;
    const auto hold = [&](std::uint32_t r23) {
        c.set({{22, value}, {23, r23}, {24, record}, {25, account}, {26, count}, {27, index}, {28, club},
               {29, offset}, {30, game_cell}, {31, n.pool}});
    };
    const auto add_figure = [&](std::int32_t figure, std::uint32_t added) {
        if (c.i32(account + largest) < figure) c.put32(account + largest, std::uint32_t(figure));
        c.put32(account + total, c.u32(account + total) + added);
    };

    // Up to 50 player contracts: each person's contract with the club, valued unless it is a loan.
    for (; index < 0x32; ++index, offset += 4) {
        const auto slot = offset + contracts_at;
        const auto person = c.u32(club + slot);
        if (person == 0) continue;
        ++count;
        hold(slot);
        record = invoke(c, callee_of_value::contract_of, 0x100f4fe8, {c.u32(game_cell), person, club});
        if (record == 0) continue;
        value = c.u32(record + 0xc);
        if (std::int32_t(value) == -1) {
            hold(slot);
            invoke(c, callee_of_value::signing_value, 0x100f5018, {record, c.u32(club + slot), club, 0, 0xffffffffu, 0xb, 0});
            value = std::uint32_t(n.fpu.fctiwz(0, c.f(1)));
        }
        hold(slot);
        if (invoke(c, callee_of_value::loan_record, 0x100f5030, {c.u32(game_cell), c.u32(club + slot)}) != 0 &&
            (c.i8(record + 0x38) >> 2) != 6) {
            hold(slot);
            const auto loan = invoke(c, callee_of_value::loan_record, 0x100f505c, {c.u32(game_cell), c.u32(club + slot)});
            value -= c.u32(loan + 0xc);
        }
        add_figure(std::int32_t(value), value);
    }

    // Five, seven and three further rows (scouting, coaching and other staff): the largest and the
    // total take each row's figure. r23 counts the rows and r24 is four times it.
    struct Rows { std::uint32_t at, rows, back; };
    std::uint32_t scan = 0;
    for (const auto& group : {Rows{scouts_at, 5, 0x100f50b0}, Rows{coaches_at, 7, 0x100f510c}, Rows{staff_at, 3, 0x100f5168}}) {
        for (scan = 0, record = 0; scan < group.rows; ++scan, record += 4) {
            const auto person = c.u32(club + group.at + record);
            if (person == 0) continue;
            hold(scan);
            const auto found = invoke(c, callee_of_value::contract_by_index, group.back, {c.u32(game_cell), person});
            if (found == 0) continue;
            add_figure(c.i32(found + 0xc), c.u32(found + 0xc));
        }
    }

    // A club with fewer than 15 contracted players is counted as if the missing places were filled
    // at a figure from its reputation.
    const std::int32_t reputation = c.i16(club + club::reputation);
    if (std::int8_t(count) < 15) {
        std::int32_t figure;
        if (reputation > 0x128e) {
            figure = reputation - 0x79e;
        } else if (reputation > 0x8ca) {
            figure = n.truncate(n.fpu.multiply_add(0x100f51f4, n.constant(0x38), n.widen(reputation - 0x8ca),
                                                   n.constant(0x178)));
        } else {
            figure = n.truncate(n.fpu.fmul(n.constant(0x60), n.widen(reputation)));
        }
        if (c.u8(club + 0x23d) != 1) figure = n.scale(figure, 0x38);
        if (const auto league = c.u32(club + 0x2c); league != 0) figure = divide(figure, level_of(c, league));
        c.put32(account + total, c.u32(account + total) + std::uint32_t(figure) * std::uint32_t(15 - std::int8_t(count)));
    }

    // The total grows with the square of the reputation, by a factor from the club's league.
    const auto add_reputation_squared = [&](std::uint32_t factor, std::uint32_t site) {
        const auto squared = std::int32_t(std::uint32_t(reputation) * std::uint32_t(reputation));
        const auto widened_square = n.widen(squared);
        const auto widened_total = n.widen(c.i32(account + total));
        c.put32(account + total,
                std::uint32_t(n.truncate(n.fpu.multiply_add(site, n.constant(factor), widened_square, widened_total))));
    };
    if (c.u8(club + 0x23d) != 1) {
        if (reputation > 0xcb2) add_reputation_squared(0x1a0, 0x100f5518);
        else add_reputation_squared(0x1a8, 0x100f556c);
    } else {
        const auto league = c.u32(club + 0x2c);
        const bool in_top_leagues = league != 0 && (is_competition(c, c.u32(league + 0x68), cell_c) ||
                                                    is_competition(c, c.u32(league + 0x68), cell_b));
        if (!in_top_leagues) {
            add_reputation_squared(0x198, 0x100f54b8);
        } else if (reputation > 0x1482) {
            add_reputation_squared(0x180, 0x100f5358);
        } else if (reputation > 0xcb2) {
            add_reputation_squared(0x188, 0x100f53b4);
        } else {
            add_reputation_squared(0x190, 0x100f5408);
        }
        // 0x100f5420 (the variant for a type byte other than 1) follows a test of that very byte
        // against 1 at 0x100f5308 and is never reached.
    }
    hold(scan);
    c.link(0x100f5590);
    c.put32(account + budget, club_budget(c, account, 0, 0xffffffffu));
}
}

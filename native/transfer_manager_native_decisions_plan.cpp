// TRANSFER_MANAGER.CPP, block "decisions": planning an offer (0x103eed00).
#include "guest_fpu.hpp"
#include "transfer_manager_native_decisions.hpp"
#include "transfer_manager_native_detail.hpp"
#include <bit>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t find_contract = 0x10082f20, contract_of_person = 0x10082f90, contract_blocked = 0x10083110,
                        contract_cost = 0x10085860, list_construct = 0x10738c00, list_destroy = 0x10738c20,
                        list_clear = 0x10738ca0, list_next = 0x10738e00, list_add = 0x10738d10, allocate = 0x10773520,
                        construct_offer = 0x10411e60, fill_offer = 0x104164f0, candidates = 0x100d06f0,
                        position_of = 0x10117490, side_of = 0x100ce0a0, match_value = 0x10211e70,
                        player_rating = 0x10369080, shortlist_cost = 0x10361490;
constexpr std::uint32_t early_check_a = 0x103eaea0, early_check_b = 0x103eb020, early_check_c = 0x103eb1f0, wanted = 0x103f83c0;
constexpr std::int32_t pool_cell = -0x4ba0, people_count_cell = -0x7be4, status_cell = -0x7b9c;
// Frame slots of the original.
constexpr std::uint32_t seen_list = 0x64, candidate_array = 0x74, entry_at = 0x5c, cost_buffer = 0x58, stack_args = 0x38,
                        budget_slot = 0x14c, person_contract_slot = 0x150, club_slot = 0x1cc, person_slot = 0x1d0, terms_slot = 0x1d4;

// The original's r17..r21 and r28 carry values across iterations (and come in from the caller when the
// first pass has not set them): the same names here.
struct Plan {
    Cpu& c;
    guest::Fpu fpu;
    Frame& frame;
    std::uint32_t pool = c.toc(pool_cell);
    std::uint32_t r17 = c.r(17), r18 = c.r(18), r19 = c.r(19), r20 = c.r(20), r21 = c.r(21);

    std::uint32_t people() const { return c.u32(c.toc(toc::people)); }
    std::uint32_t club_of(std::uint32_t found) const { return found ? c.u32(c.toc(toc::clubs)) + c.u32(found + 4) * club_size : 0; }
    std::uint32_t contracts() const { return c.u32(c.toc(toc::contracts)); }

    // The seller's contract of `person` through its club, with the lookup's answer left in the shared cell.
    std::uint32_t contract_for(std::uint32_t person, std::uint32_t asked, std::uint32_t found_back, std::uint32_t contract_back) {
        const auto found = invoke(c, find_contract, found_back, {contracts(), asked});
        c.put32(c.toc(toc::found_contract), found);
        return invoke(c, contract_of_person, contract_back, {contracts(), person, club_of(found)});
    }
    // The flags word: r18's bits, plus 0x80 and the side code of the candidate's competition, as r17.
    void set_side_flags(std::uint32_t person, std::uint32_t club, std::uint32_t back) {
        if (std::int8_t(r21) != 5) {
            r18 |= 0x80;
            keep(c, 18, r18);
            const auto side = std::int8_t(invoke(c, side_of, back, {person, c.u32(club + 0x90)}));
            switch (side) {
            case 0: r17 = r18 | 0x40; break;
            case 1: r17 = r18 | 0x50; break;
            case 2: r17 = r18 | 0x10; break;
            case 3: r17 = r18 | 0x30; break;
            case 4: r17 = r18 | 0x20; break;
            default: break;  // other codes leave r17 as it was
            }
        } else {
            r17 = r18;
        }
        keep(c, 17, r17);
    }
    // The match value call; its six stack arguments are -1 and five zeros.
    std::uint32_t value_of(std::uint32_t person, std::uint32_t club, std::uint32_t back) {
        c.put32(frame.at(stack_args), 0xffffffff);
        for (unsigned n = 1; n < 7; ++n) c.put32(frame.at(stack_args + n * 4), 0);
        return invoke(c, match_value, back, {person, c.u32(person + 0x14), 0, r20, r17, club, 0, 0});
    }
};
}

std::uint32_t plan_offer(Cpu& c, std::uint32_t manager, std::uint32_t club_a, std::uint32_t person, std::uint32_t terms,
                         std::uint32_t out_offer, std::uint32_t out_reaction, std::uint32_t budget) {
    Frame frame(c, 0x103eed00, 0x1b0, {.first_gpr = 13});
    Plan plan{c, guest::Fpu(c), frame};
    keep(c, 14, c.toc(toc::contracts));
    keep(c, 30, manager);
    keep(c, 15, out_offer);
    keep(c, 31, out_reaction);
    keep(c, 13, budget);
    // The arguments are spilled into the caller's parameter area, where later code reads them as stale stack.
    c.put32(frame.at(club_slot), club_a);
    c.put32(frame.at(person_slot), person);
    c.put32(frame.at(terms_slot), terms);

    const auto found = invoke(c, find_contract, 0x103eed3c, {plan.contracts(), person});
    c.put32(c.toc(toc::found_contract), found);
    const auto seller = plan.club_of(found);
    keep(c, 27, seller);
    const auto seen = frame.at(seen_list);
    invoke(c, list_construct, 0x103eed70, {seen});
    const auto person_contract = invoke(c, contract_of_person, 0x103eed80, {plan.contracts(), c.u32(frame.at(person_slot)), seller});
    c.put32(frame.at(person_contract_slot), person_contract);
    // The seller club's row of the manager's table; a zero flag byte means nothing may be offered.
    const auto row = c.u32(c.u32(manager + 0x988) + 0xdc) + c.u32(seller) * 0xc;
    keep(c, 26, row);
    if (c.i8(row + 4) == 0) {
        invoke(c, list_destroy, 0x103eedb0, {seen, 0xffffffff});
        return 0;
    }
    invoke(c, candidates, 0x103eedc8, {c.u32(frame.at(club_slot)), frame.at(candidate_array), 0});
    auto offer = invoke(c, allocate, 0x103eedd0, {0x54});
    keep(c, 29, offer);
    if (offer) invoke(c, construct_offer, 0x103eeddc, {offer});
    c.put32(out_offer, offer);
    invoke(c, fill_offer, 0x103eedf4, {c.u32(out_offer), c.u32(frame.at(person_slot)), c.u32(frame.at(club_slot)), budget});

    // The budget as a float (single rounding) scaled to the working limit.
    const auto bits = (std::uint64_t(0x43300000) << 32) | (budget ^ 0x80000000u);
    keep_signed(c, 23, -1);
    keep_signed(c, 24, -0x2c);
    const auto diff = plan.fpu.fsubs(std::bit_cast<double>(bits), c.f64(plan.pool + 8));
    c.put8(out_reaction, 0x32);
    c.put32(offer + 0x20, budget);
    c.put32(frame.at(budget_slot), std::uint32_t(std::int32_t(plan.fpu.fctiwz(0, plan.fpu.fmul(c.f64(plan.pool + 0xb8), diff)))));

    std::int32_t group = -1, group_offset = -0x2c;  // the original's r23 and r24: the outer step and its offset
    const auto limit = [&] { return c.i32(frame.at(budget_slot)); };
    for (; std::int8_t(group) < 0x19; ++group, group_offset += 0x2c, keep_signed(c, 23, group), keep_signed(c, 24, group_offset)) {
        // Stop once the offer holds a list of kind 3.
        if (const auto list = c.u32(offer + 4); list != 0 && c.i16(list) == 3) break;
        std::uint32_t scout;  // the original's r16
        if (std::int8_t(group) != -1) {
            const auto index = c.i32(c.u32(row) + std::uint32_t(group_offset + 0x10));
            if (index <= -1) continue;
            if (c.u32(plan.people() + std::uint32_t(index) * person_size + 0x24) == seller) continue;
        }
        if (std::int8_t(group) == -1) {
            if ((c.u8(c.u32(frame.at(person_contract_slot)) + 0x60) & 0xf) == 7) continue;
            scout = c.u32(frame.at(person_slot));
        } else {
            scout = plan.people() + c.u32(c.u32(row) + std::uint32_t(group_offset + 0x10)) * person_size;
        }
        keep(c, 16, scout);
        const auto scout_contract = plan.contract_for(scout, scout, 0x103eeee8, 0x103eef20);
        keep(c, 13, scout_contract);
        std::int32_t step = -1, step_offset = -4;  // r22 and r25
        keep_signed(c, 22, step);
        keep_signed(c, 25, step_offset);
        std::uint32_t candidate = 0;  // r15
        for (; std::int8_t(step) < 0x32; ++step, step_offset += 4, keep_signed(c, 22, step), keep_signed(c, 25, step_offset)) {
            if (std::int8_t(step) == -1) {
                candidate = scout;
                keep(c, 15, candidate);
                if (c.u32(scout + 0x14) == 0) continue;
                plan.r20 = invoke(c, position_of, 0x103eef50, {c.u32(scout + 0x14)});
                keep(c, 20, plan.r20);
                plan.r18 = 0x4800;
                plan.r21 = std::uint32_t(std::int8_t(std::int8_t(group) == -1
                    ? c.u8(scout_contract + 0x60) & 0xf
                    : std::int8_t((c.u8(c.u32(row) + std::uint32_t(group_offset + 0x28)) >> 3) & 0xf)));
                keep(c, 21, plan.r21);
                if (std::int8_t(plan.r21) != 2 && std::int8_t(plan.r21) != 3) plan.r18 |= 0x100;
                keep(c, 18, plan.r18);
                plan.set_side_flags(scout, seller, 0x103eefbc);
                plan.r19 = plan.value_of(scout, seller, 0x103ef060);
                keep(c, 19, plan.r19);
            } else {
                candidate = c.u32(frame.at(candidate_array) + std::uint32_t(step_offset));
                keep(c, 15, candidate);
                if (candidate == 0) continue;
                const auto candidate_index = c.i32(candidate);
                if (candidate_index >= c.i32(c.toc(people_count_cell))) continue;
                if (c.u32(candidate + 0x14) == 0) continue;
            }
            if (std::int8_t(step) > -1) {
                const auto position = std::int16_t(invoke(c, position_of, 0x103ef0b0, {c.u32(candidate + 0x14)}));
                if (std::int16_t(plan.r20) != position) continue;
            }
            const auto contract = plan.contract_for(candidate, scout, 0x103ef0cc, 0x103ef104);
            keep(c, 28, contract);
            if (contract == 0) continue;
            if (c.u32(candidate + 0x24) != c.u32(frame.at(club_slot))) continue;
            if (invoke(c, contract_blocked, 0x103ef128, {plan.contracts(), candidate}) != 0) continue;
            if (((c.u8(c.u32(c.toc(status_cell)) + c.u32(candidate) * 0x64 + 0xc) >> 2) & 3) != 0) continue;
            if ((std::int8_t(c.u8(contract + 0x38)) >> 2) == 5) continue;
            const auto cost = std::int32_t(invoke(c, contract_cost, 0x103ef188,
                                                  {plan.contracts(), c.u32(frame.at(club_slot)), 0, candidate, 0, 1, 1}));
            if (cost > limit()) continue;
            invoke(c, list_clear, 0x103ef19c, {seen});
            const bool refused = invoke(c, early_check_a, 0x103ef1b0, {manager, candidate, 0, 0}) != 0 ||
                                 invoke(c, early_check_b, 0x103ef1c8, {manager, candidate, seen}) != 0 ||
                                 invoke(c, early_check_c, 0x103ef1e0, {manager, c.u32(frame.at(person_slot)), 0}) != 0;
            if (refused) {
                if (c.i16(seen) != 1) continue;
                c.put32(frame.at(0x70), c.u32(frame.at(0x68)));
                const auto first = invoke(c, list_next, 0x103ef204, {seen});
                const auto first_slot = c.i32(first);
                if (c.i32(c.u32(frame.at(terms_slot)) + 0x28) != first_slot) continue;
            }
            if (std::int8_t(invoke_native(c, wanted, 0x103ef230, {manager, candidate, seller, 0, 1})) == 0) continue;
            const auto rating_group = std::int8_t(group) == -1
                ? std::uint32_t(c.u8(scout_contract + 0x60) & 0xf)
                : std::uint32_t(std::int8_t((c.u8(c.u32(row) + std::uint32_t(group_offset + 0x28)) >> 3) & 0xf));
            invoke(c, player_rating, 0x103ef268, {c.u32(manager + 0x988), candidate, seller, 1, 0, rating_group, 0});
            const auto rating = c.f(1);
            plan.fpu.fcmp(0, rating, c.f64(plan.pool + 0xc0));
            if (rating < c.f64(plan.pool + 0xc0)) continue;
            plan.set_side_flags(candidate, seller, 0x103ef2d8);
            const auto value = plan.value_of(candidate, seller, 0x103ef37c);
            if (value == 0 || plan.r19 == 0) continue;
            const auto bits_value = (std::uint64_t(0x43300000) << 32) | (value ^ 0x80000000u);
            const auto bits_old = (std::uint64_t(0x43300000) << 32) | (plan.r19 ^ 0x80000000u);
            const auto magic = c.f64(plan.pool + 8);
            const auto numerator = plan.fpu.fsubs(std::bit_cast<double>(bits_value), magic);
            const auto denominator = plan.fpu.fsubs(std::bit_cast<double>(bits_old), magic);
            const auto ratio = plan.fpu.fdivs(numerator, denominator);
            plan.fpu.fcmp(0, ratio, c.f64(plan.pool + 0xc8));
            if (!(ratio > c.f64(plan.pool + 0xc8))) continue;
            // The reaction rises by 20 when the new candidate is worth more than the first, else by 5.
            const std::uint32_t rise = std::int32_t(value) > std::int32_t(plan.r19) ? 0x14 : 5;
            c.put8(out_reaction, std::uint8_t(c.u8(out_reaction) + rise));
            const auto reaction = c.i8(out_reaction);
            c.put8(out_reaction, std::uint8_t(reaction < 0 ? 0 : reaction > 0x64 ? 0x64 : reaction));
            if (c.u32(offer + 4) == 0) {
                auto created = invoke(c, allocate, 0x103ef460, {0x10});
                keep(c, 28, created);
                if (created) invoke(c, list_construct, 0x103ef46c, {created});
                c.put32(offer + 4, created);
            }
            std::uint32_t listed = 0;  // r28
            keep(c, 28, listed);
            c.put32(c.u32(offer + 4) + 0xc, c.u32(c.u32(offer + 4) + 4));
            for (;;) {
                const auto member = invoke(c, list_next, 0x103ef4a4, {c.u32(offer + 4)});
                if (member == 0) break;
                if (c.u32(member) == c.u32(candidate)) {
                    listed = 1;
                    keep(c, 28, listed);
                    break;
                }
            }
            if (std::int8_t(listed) != 0) continue;
            // The record added to the offer's list is 8 bytes: candidate, 0, category (+2 stale bytes).
            c.put32(frame.at(entry_at), c.u32(candidate));
            c.put8(frame.at(entry_at + 4), 0);
            c.put8(frame.at(entry_at + 5), std::uint8_t(plan.r21));
            invoke(c, list_add, 0x103ef4d8, {c.u32(offer + 4), frame.at(entry_at), 8});
            const auto saved = invoke(c, shortlist_cost, 0x103ef504,
                                      {c.u32(c.u32(c.toc(toc::manager)) + 0x988), seller, candidate,
                                       c.u32(row) + std::uint32_t(group_offset), frame.at(cost_buffer), 0xffffffff, 1});
            const auto old_fee = c.u32(offer + 0x20);
            c.put32(frame.at(budget_slot), c.u32(frame.at(budget_slot)) - saved);
            c.put32(offer + 0x20, old_fee - saved);
            if (c.i32(offer + 0x20) < 0) c.put32(offer + 0x20, 0);
            break;
        }
    }
    invoke(c, list_destroy, 0x103ef568, {seen, 0xffffffff});
    return 1;
}
}

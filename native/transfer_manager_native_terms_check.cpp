#include "transfer_manager_native_terms.hpp"
#include "debug_force_purchase.hpp"
#include "transfer_manager_native_stages_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace stages;

namespace {
constexpr std::uint32_t call_vector = 0x10781320, nation_objects = 0x9e0, nation_count = 27;
constexpr std::uint32_t verdict_refused = 0, verdict_over_budget = 1, verdict_accepted = 2;
constexpr std::int16_t code_over_budget = 3, code_too_late = 9;

// The original's test "is the offer's date (`own`) after the default date" when the second halfwords are equal:
// the sign of ((x >> 1) - (x & a)) with x = a ^ b over the sign-extended first halfwords, a being the offer's.
bool date_first_halfword_decides(std::int32_t a, std::int32_t b) {
    const auto x = a ^ b;
    return ((x >> 1) - (x & a)) < 0;
}
// The club object of the nation `nation` among the manager's 27, or 0.
std::uint32_t nation_object(Cpu& c, std::uint32_t manager, std::uint32_t nation) {
    for (std::uint32_t k = 0; k < nation_count; ++k) {
        const auto object = c.u32(manager + nation_objects + k * 4);
        if (c.u32(object + 0xc) == nation) return object;
    }
    return 0;
}
}

std::uint32_t check_offer_terms(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t club,
                                std::uint32_t code_out, std::uint32_t skip_rules) {
    Frame frame(c, 0x103f9d60, 0xa0, {.first_gpr = 20});
    const auto people_cell = c.toc(toc::people), clubs_cell = c.toc(toc::clubs), found_cell = c.toc(toc::found_contract);
    const auto default_date = c.toc(-0x7b80);
    keep(c, 21, manager);
    keep(c, 22, offer);
    keep(c, 23, club);
    keep(c, 24, code_out);
    keep(c, 25, skip_rules);
    keep(c, 28, default_date);
    keep(c, 29, people_cell);
    keep(c, 30, found_cell);
    keep(c, 31, clubs_cell);
    const auto today = frame.at(0x44);
    invoke(c, 0x10715880, 0x103f9d9c, {today});
    keep(c, 20, 0);
    const auto person = person_record(c, c.u32(offer + offer_person));
    const auto buyer = club_record(c, c.u32(offer + offer_club));
    keep(c, 26, person);
    keep(c, 27, buyer);
    const auto contract = invoke(c, find_contract, 0x103f9dd0, {c.u32(c.toc(toc::contracts)), person});
    c.put32(found_cell, contract);
    const bool has_club = contract && contract_kind(c, contract) != no_club_contract_kind;
    keep(c, 20, has_club ? 1 : 0);
    const auto holder = has_club ? club_record(c, c.u32(c.u32(found_cell) + 4)) : 0;
    keep(c, 30, holder);

    const auto offer_kind_nibble = std::int8_t(c.u8(offer + 0x38)) >> 4;
    if (offer_kind_nibble != 0 && c.u8(offer + 0x34) != 0x13) {
        // The date the terms take effect; a nonzero reason refuses at once, and a date before the default date
        // answers "not yet" (0).
        const auto reason = std::uint16_t(invoke_native(c, 0x103f89e0, 0x103f9e4c, {manager, offer, today}));
        if (code_out) c.put16(code_out, reason);
        if (std::int16_t(reason) != 0) return verdict_accepted;
        const auto default_second = c.u16(default_date + 2), own_second = c.u16(today + 2);
        bool not_yet;
        if (default_second < own_second) not_yet = true;
        else if (default_second > own_second) not_yet = false;
        else not_yet = date_first_halfword_decides(c.i16(today), c.i16(default_date));
        if (not_yet) return 0;
    }

    if (std::int8_t(skip_rules) == 0 && c.u32(person + 0x14) != 0 &&
        std::int8_t(invoke(c, 0x100c7a20, 0x103f9ee8, {c.u8(offer + 0x11)})) != 0) {
        const auto answered = [&](std::uint32_t back) {
            return std::int8_t(invoke_native(c, 0x103f5a50, back, {manager, offer, buyer, holder, person, club, code_out})) != 0;
        };
        if (club == holder && std::int8_t(c.u8(offer + 0x37)) == 0 &&
            std::int8_t(invoke_native(c, 0x103f4d10, 0x103f9f18, {manager, offer, 0, 0})) == 0) {
            // Keep the buyer's checks and the normal completion flow. The
            // generated 0x103f9f20 hook skips only the forced seller's review.
            if (!forced_purchase_for_seller(c.s, offer, holder) && answered(0x103f9f40)) return verdict_accepted;
        } else if (club == buyer) {
            if (answered(0x103f9f78)) return verdict_accepted;
        }
    }

    // What the buying club can spend, against the fee asked.
    const auto cash_view = invoke(c, 0x10103510, 0x103f9f98, {c.u32(c.toc(-0x7ab8)), buyer});
    invoke(c, 0x100f5770, 0x103f9fb0, {cash_view, person, 0, 0, 0, 0});
    const auto cash_high = std::int32_t(c.r(3));
    const auto cash_low = cash_high < 0 ? 0u : c.r(4);
    c.put32(frame.at(0x38), cash_low);
    const auto cash = invoke(c, 0x1000a060, 0x103f9fe4, {frame.at(0x38), 4});
    std::uint32_t sum_low = cash;
    std::uint32_t sum_high = std::uint32_t(std::int32_t(cash) >> 31);
    keep(c, 26, cash);
    keep(c, 20, sum_high);
    const auto fee = invoke(c, 0x10416710, 0x103fa004, {offer, 0, 1, 1, 0});
    keep(c, 31, fee);

    if (club == buyer) {
        invoke(c, 0x10773c80, 0x103fa01c, {sum_high, sum_low});
        const auto limits = c.toc(-0x4ba0);
        guest::Fpu fpu(c);
        const auto allowed = fpu.fmul(c.f64(limits + 0xb8), c.f(1));
        const auto asked = fpu.fsub(std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (fee ^ 0x80000000u)),
                                    c.f64(limits + 8));
        fpu.fcmp(0, allowed, asked);
        if (allowed < asked) {
            if (code_out) c.put16(code_out, std::uint16_t(code_over_budget));
            if (std::int8_t(invoke(c, club_test, 0x103fa074, {buyer, 0, 1, 0, 0})) != 0) return verdict_over_budget;
            // The club cannot cover it alone: the fees of the people it would sell (its shortlist) are added.
            const auto list = frame.at(0x4c);
            invoke(c, list_construct, 0x103fa084, {list});
            keep(c, 22, sum_low);
            invoke(c, 0x10362e00, 0x103fa098, {c.u32(manager + 0x988), buyer, list});
            rewind_list(c, list);
            while (const auto entry = invoke(c, next_listed, 0x103fa0ec, {list})) {
                const auto sale = invoke_native(c, 0x103f5d20, 0x103fa0c4,
                                                {manager, person_record(c, c.u32(entry)), buyer, 0});
                if (sale) {
                    const auto extra = c.u32(sale + 0x20);
                    const auto low = sum_low + extra;
                    sum_high = sum_high + std::uint32_t(std::int32_t(extra) >> 31) + (low < sum_low ? 1u : 0u);
                    sum_low = low;
                    keep(c, 22, sum_low);
                    keep(c, 20, sum_high);
                }
            }
            const auto fee_high = std::int32_t(fee) >> 31;
            const bool short_of_fee = std::int64_t((std::uint64_t(std::uint32_t(fee_high)) << 32) | fee) <
                                      std::int64_t((std::uint64_t(sum_high) << 32) | sum_low);
            invoke(c, list_destroy, short_of_fee ? 0x103fa120 : 0x103fa134, {list, 0xffffffff});
            return short_of_fee ? verdict_over_budget : verdict_accepted;
        }
    }
    if (club == buyer && std::int8_t(invoke_native(c, 0x103f4fd0, 0x103fa164, {manager, buyer, offer, skip_rules, code_out})) == 0)
        return verdict_over_budget;
    if (holder == club && std::int8_t(invoke_native(c, 0x103f4fd0, 0x103fa194, {manager, holder, offer, skip_rules, code_out})) == 0)
        return verdict_over_budget;

    if (code_out) c.put16(code_out, 0);
    if (c.u32(offer + offer_listed) != 0 || std::int8_t(invoke(c, 0x100c7a20, 0x103fa1c8, {c.u8(offer + 0x11)})) != 0) {
        if (const auto object = nation_object(c, manager, c.u32(club + 0x2c))) {
            c.set(12, c.u32(c.u32(object) + 0x18));
            c.set({{3, object}, {4, offer}, {5, club}, {6, default_date}, {7, code_out}});
            c.call(call_vector, 0x103fa234);
            c.set(2, c.u32(frame.at(0x14)));
            if (std::int8_t(c.r(3)) == 0) {
                if (c.i16(code_out) != code_too_late || std::int8_t(c.u8(offer + 0x38)) >> 4 != 5) return verdict_accepted;
            }
        }
    }
    return verdict_refused;
}
}

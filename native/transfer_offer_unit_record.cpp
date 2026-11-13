// TRANSFER_OFFER.CPP: the offer record (transfer_offer_unit_record.hpp).
#include "transfer_offer_unit_record.hpp"
#include "transfer_offer_unit_detail.hpp"

namespace cm::aot::transfer_offer_unit {
using guest::Cpu;
using guest::Frame;

namespace {
// Offer bytes the detail header does not name.
constexpr std::uint32_t fee_form = 0x24,       // byte: 1 in a plain offer, 0 in one with an amount
    club_limit = 0x28,                         // signed word: how many club indices the file form may name
    label_a = 0x2c, label_b = 0x30,            // words, -1 in a new offer
    offer_kind = 0x34, flag = 0x35, flag_a = 0x36, flag_b = 0x37, flags = 0x38;
// The globals of the offer-starting routines.
constexpr std::int32_t person_lookup_cell = -0x7b7c,  // the cell holding what 0x10082f20 and 0x10084be0 are asked with
    lookup_result_cell = -0x7b78;                     // where the lookup's answer is kept

// A person's club, as the original finds it: the lookup's answer names an entry whose word at +4 is the
// club index; null when nothing is found. Also leaves the answer in the shared cell.
std::uint32_t club_of(Cpu& c, std::uint32_t person, std::uint32_t back) {
    const auto found = invoke(c, 0x10082f20, back, {c.u32(c.toc(person_lookup_cell)), person});
    c.put32(c.toc(lookup_result_cell), found);
    if (found == 0) return 0;
    return c.u32(c.toc(toc::clubs)) + c.u32(found + 4) * club_size;
}

// The fields every started offer shares; `plain` offers pay nothing at once and have fee_form 1.
void fill_new_offer(Cpu& c, std::uint32_t offer, std::uint32_t person, std::uint32_t club, std::uint32_t amount, bool plain) {
    c.put32(offer + field::clubs, 0);
    c.put32(offer + field::exchange, 0);
    c.put32(offer + field::buyer, c.u32(club));
    c.put32(offer + field::player, c.u32(person));
    c.put8(offer + field::kind, 0xb);
    c.put32(offer + field::fee, amount);
    c.put8(offer + fee_form, plain ? 1 : 0);
    c.put32(offer + field::instalments, 0);
    c.put8(offer + field::instalment_months, 10);
    c.put32(offer + field::extra, 0);
    c.put8(offer + field::extra_count, 10);
    c.put8(offer + field::sell_on_percent, 0);
    c.put8(offer + field::months, 0);
    c.put32(offer + label_b, 0xffffffff);
    c.put32(offer + label_a, 0xffffffff);
    c.put8(offer + offer_kind, 0);
    c.put8(offer + field::status, 0);
    c.put8(offer + flag_a, 0);
    c.put8(offer + flag_b, 0);
    c.put8(offer + flags, c.u8(offer + flags) & 0x0f);
}
constexpr CheckSite plain_missing{0x10416410, 0x40, 0x38, 0x485}, amount_missing{0x1041654c, 0x40, 0x38, 0x4b1};
}

std::uint32_t construct(Cpu& c, std::uint32_t offer) {
    Frame frame(c, 0x10411e60, 0x40, {.first_gpr = 31});
    c.set(31, offer);
    invoke(c, callee::date_construct, 0x10411e7c, {offer + field::date});
    c.put32(offer + field::clubs, 0);
    c.put32(offer + field::exchange, 0);
    return offer;
}

std::int32_t deferred_amount(Cpu& c, std::uint32_t offer) {
    std::uint32_t total = 0;
    if (const auto instalments = c.i32(offer + field::instalments); instalments > 0) total = std::uint32_t(instalments);
    if (const auto extra = c.i32(offer + field::extra); extra > 0) total += std::uint32_t(extra);
    return std::int32_t(total);
}

std::uint32_t status_of(Cpu& c, std::uint32_t offer, std::uint32_t person) {
    Frame frame(c, 0x10416d60, 0x40, {.first_gpr = 30});
    c.set({{30, offer}, {31, person}});
    if (person == c.u32(offer + field::player)) {
        // Asking about a kind-3 offer's own player settles it as status 4.
        if (c.i8(offer + 0x34) == 3) c.put8(offer + field::status, 4);
        return c.u8(offer + field::status);
    }
    const auto list = c.u32(offer + field::exchange);
    if (list == 0) return 4;
    rewind(c, list);
    for (;;) {
        const auto row = invoke(c, callee::list_next, 0x10416de0, {c.u32(offer + field::exchange)});
        if (row == 0) return 4;
        if (c.u32(row + exchange_row::person) == person) return c.u8(row + exchange_row::status);
    }
}

std::uint32_t exchange_row_of(Cpu& c, std::uint32_t offer, std::uint32_t person) {
    Frame frame(c, 0x10417260, 0x40, {.first_gpr = 30});
    c.set({{30, offer}, {31, person}});
    const auto list = c.u32(offer + field::exchange);
    if (list == 0) return 0;
    rewind(c, list);
    for (;;) {
        const auto row = invoke(c, callee::list_next, 0x104172c0, {c.u32(offer + field::exchange)});
        if (row == 0) return 0;
        // The key is read again for each row, as the original does.
        if (c.u32(row + exchange_row::person) == c.u32(person)) return row;
    }
}
std::uint32_t construct_with(Cpu& c, std::uint32_t offer, std::uint32_t first, std::uint32_t second,
                             std::uint32_t third, std::uint32_t fourth, std::uint32_t fifth) {
    Frame frame(c, 0x10411ea0, 0x50, {.first_gpr = 26});
    // The arguments are kept in r26-r31 across both calls.
    c.set({{26, offer}, {27, first}, {28, second}, {29, third}, {30, fourth}, {31, fifth}});
    invoke(c, callee::date_construct, 0x10411ed0, {offer + field::date});
    invoke_unit(c, 0x104126d0, 0x10411ef0, {offer, first, second, third, fourth, fifth, 0});
    return offer;
}

std::uint32_t destroy(Cpu& c, std::uint32_t offer, std::uint32_t deleting) {
    Frame frame(c, 0x10411f10, 0x40, {.first_gpr = 30});
    // The original keeps the record and the flag in r30 and r31 across its calls. The callees save them in
    // their frames, and same-save-continue reads those slots later (without them the replay's heap compare
    // differs at 0x2175046c), so the callees must be called with these values.
    c.set({{30, offer}, {31, deleting}});
    if (offer == 0) return offer;
    if (const auto clubs = c.u32(offer + field::clubs); clubs != 0) {
        if (c.i16(clubs) == 1) invoke(c, callee::list_destroy, 0x10411f50, {clubs, 1});
        else invoke(c, callee::list_remove, 0x10411f5c, {clubs, offer + club_limit});
    }
    if (const auto exchange = c.u32(offer + field::exchange); exchange != 0)
        invoke(c, callee::list_destroy, 0x10411f70, {exchange, 1});
    if (std::int16_t(deleting) > 0) invoke(c, callee::free_object, 0x10411f80, {offer});
    return offer;
}

void start_plain(Cpu& c, std::uint32_t offer, std::uint32_t person, std::uint32_t club) {
    Frame frame(c, 0x104163d0, 0x70, {.first_gpr = 30});
    c.set({{30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    if (person == 0 || club == 0) return report_check(c, frame, plain_missing);
    c.put8(offer + flag, 0);
    fill_new_offer(c, offer, person, club, 0, true);
}

void start_with_amount(Cpu& c, std::uint32_t offer, std::uint32_t person, std::uint32_t club, std::uint32_t amount) {
    Frame frame(c, 0x104164f0, 0x90, {.first_gpr = 21});
    c.set({{21, c.toc(person_lookup_cell)}, {22, c.toc(lookup_result_cell)}, {23, c.toc(toc::clubs)},
           {24, c.toc(toc::message)}, {25, c.toc(toc::pool)}, {26, offer}, {27, person}, {28, club}, {29, amount},
           {30, c.toc(sample_table::base)}, {31, c.toc(sample_table::cursor)}});
    if (person == 0 || club == 0) return report_check(c, frame, amount_missing);
    // The flag is set only when the person's club has the buyer's label, the sample drawn is a
    // multiple of 20 and the person's attribute is above 7000.
    bool set = false;
    if (club_of(c, person, 0x10416598) != 0) {
        const auto own = club_of(c, person, 0x104165d0);
        // Both are club records: the word at +0x30 is the club's league.
        constexpr std::uint32_t club_league = 0x30;
        if (c.u32(own + club_league) == c.u32(club + club_league)) {
            const auto sample = std::int32_t(draw(c, 0x1041662c));
            if (sample % 20 == 0 && c.i16(c.u32(person + 0x14) + 10) > 7000) set = true;
        }
    }
    c.put8(offer + flag, set ? 1 : 0);
    fill_new_offer(c, offer, person, club, amount, false);
}

std::int32_t total(Cpu& c, std::uint32_t offer, std::uint32_t fee_only_flag, std::uint32_t without_exchange_flag,
                   std::uint32_t by_valuation_flag, std::uint32_t with_sell_on_flag) {
    Frame frame(c, 0x10416710, 0x50, {.first_gpr = 26});
    const auto fee_only = std::int8_t(fee_only_flag), without_exchange = std::int8_t(without_exchange_flag),
               by_valuation = std::int8_t(by_valuation_flag), with_sell_on = std::int8_t(with_sell_on_flag);
    const auto person = c.u32(c.toc(toc::people)) + c.u32(offer + field::player) * person_size;
    std::uint32_t sum = c.u32(offer + field::fee);
    // r27 keeps the valuation flag as passed, r28 the player's record, r29 the running total.
    c.set({{26, offer}, {27, by_valuation_flag}, {28, person}, {29, sum}, {30, c.toc(toc::people)},
           {31, c.toc(person_lookup_cell)}});
    if (fee_only == 0) {
        if (const auto later = c.i32(offer + field::instalments); later > 0) sum += std::uint32_t(later);
        if (const auto extra = c.i32(offer + field::extra); extra > 0) sum += std::uint32_t(extra);
        if (const auto percent = c.i8(offer + field::sell_on_percent); percent > 0 && with_sell_on != 0) {
            // The person's value times the percentage, divided by 100 towards zero.
            const auto share = std::int32_t(c.u32(person + 8) * std::uint32_t(std::int32_t(percent)));
            sum += std::uint32_t(share / 100);
        }
    }
    c.set(29, sum);
    if (c.u32(offer + field::exchange) != 0 && without_exchange == 0) {
        rewind(c, c.u32(offer + field::exchange));
        for (;;) {
            const auto row = invoke(c, callee::list_next, 0x10416814, {c.u32(offer + field::exchange)});
            if (row == 0) break;
            if (by_valuation != 0) {
                sum += invoke(c, 0x10084be0, 0x104167ec, {c.u32(c.toc(person_lookup_cell)), person, 0, 0, 0xffffffff});
            } else {
                sum += c.u32(c.u32(c.toc(toc::people)) + c.u32(row + exchange_row::person) * person_size + 8);
            }
            c.set(29, sum);
        }
    }
    return std::int32_t(sum);
}
}

// TRANSFER_OFFER.CPP: comparing and adjusting an offer's terms (0x10416e10, 0x104170f0, 0x104172f0).
#include "transfer_offer_unit_value.hpp"
#include "transfer_offer_unit_detail.hpp"

namespace cm::aot::transfer_offer_unit {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using namespace value_detail;

namespace {
constexpr std::uint32_t read_amount = 0x10009f20;  // (at, size) -> the amount's two words in r3 (high) and r4 (low)
constexpr std::uint32_t list_count = 0;            // a list's halfword count at +0
constexpr std::uint32_t row_person = 0;            // an exchange row's person index; a person record's own index too

struct Amount { std::uint32_t high, low; };
Amount amount_at(Cpu& c, std::uint32_t at, std::uint32_t back) {
    invoke(c, read_amount, back, {at, 4});
    return {c.r(3), c.r(4)};
}
}

std::uint32_t same_terms(Cpu& c, std::uint32_t offer, std::uint32_t other) {
    Frame frame(c, 0x10416e10, 0x70, {.first_gpr = 27});
    // The original keeps the two offers in r30 and r31; r27..r29 hold the amounts it has read and its flags, which
    // callees save in their frames, so they are set where the original sets them.
    c.set({{30, offer}, {31, other}});
    // An amount is compared through the buffer the game reads it from: the other offer's first.
    const auto read_pair = [&](std::uint32_t term, std::int32_t mine, std::int32_t theirs, std::uint32_t first_back,
                               std::uint32_t second_back, bool& same, bool for_extra = false) {
        c.put32(frame.at(theirs), c.u32(other + term));
        c.put32(frame.at(mine), c.u32(offer + term));
        const auto a = amount_at(c, frame.at(mine), first_back);
        // The original keeps the first read's words in r27/r28, the extra's in r28/r29.
        if (for_extra) c.set({{28, a.low}, {29, a.high}});
        else c.set({{27, a.high}, {28, a.low}});
        const auto b = amount_at(c, frame.at(theirs), second_back);
        same = a.high == b.high && a.low == b.low;
        return a;
    };
    bool same;
    read_pair(field::fee, 0x48, 0x4c, 0x10416e44, 0x10416e58, same);
    if (!same) return 0;
    // Instalments and the extra: a difference (in amount or in months/count) only matters when either side owes something.
    c.set(29, 0);
    read_pair(field::instalments, 0x40, 0x44, 0x10416e98, 0x10416eac, same);
    const bool instalments_differ = !same || c.i8(other + field::instalment_months) != c.i8(offer + field::instalment_months);
    if (instalments_differ) c.set(29, 1);
    if (instalments_differ && (c.i32(other + field::instalments) > 0 || c.i32(offer + field::instalments) > 0)) return 0;
    c.set(27, 0);
    read_pair(field::extra, 0x38, 0x3c, 0x10416f20, 0x10416f34, same, true);
    const bool extra_differ = !same || c.i8(other + field::extra_count) != c.i8(offer + field::extra_count);
    if (extra_differ) c.set(27, 1);
    if (extra_differ && (c.i32(other + field::extra) > 0 || c.i32(offer + field::extra) > 0)) return 0;
    for (const auto term : {field::sell_on_percent, field::months}) {
        const auto theirs = c.i8(other + term), mine = c.i8(offer + term);
        if (theirs != mine && (theirs > 0 || mine > 0)) return 0;
    }
    // How the deal is handled: the high nibble of the byte at +0x38, compared as a signed nibble.
    if ((std::int32_t(c.i8(other + 0x38)) >> 4) != (std::int32_t(c.i8(offer + 0x38)) >> 4)) return 0;
    // The players in exchange: both lists absent, or both present with the same count and every player of the
    // other's list in this one's (each walk starts from its list's head).
    const auto theirs = c.u32(other + field::exchange);
    if (theirs == 0 && c.u32(offer + field::exchange) == 0) return 1;
    if (theirs == 0 || c.u32(offer + field::exchange) == 0) return 0;
    const auto their_count = c.i16(theirs + list_count);
    if (their_count != c.i16(c.u32(offer + field::exchange) + list_count)) return 0;
    rewind(c, theirs);
    for (;;) {
        const auto row = invoke(c, callee::list_next, 0x104170d0, {c.u32(other + field::exchange)});
        if (row == 0) return 1;
        c.set(29, row);
        const auto mine = c.u32(offer + field::exchange);
        c.set(27, 0);
        rewind(c, mine);
        bool found = false;
        for (;;) {
            const auto candidate = invoke(c, callee::list_next, 0x104170b0, {c.u32(offer + field::exchange)});
            if (candidate == 0) break;
            if (c.i32(row + row_person) == c.i32(candidate + row_person)) { found = true; break; }
        }
        if (!found) return 0;
        c.set(27, 1);
    }
}

namespace {
namespace toc_cell {
inline constexpr std::int32_t finder = -0x7b7c, found = -0x7b78, players = -0x7b40, employers = -0x7ab8;
}
constexpr std::uint32_t remove_row = callee::list_remove, find_player = 0x10082f20, value_player = 0x10084be0,
                        find_employer = 0x10103510, employer_funds = 0x100f5770;
constexpr std::uint32_t player_table_counts = 0x9a0;  // the table's per-person row: 10 bytes, a count at +2
}

void remove_exchange_player(Cpu& c, std::uint32_t offer, std::uint32_t person) {
    Frame frame(c, 0x104170f0, 0x80, {.first_gpr = 26});
    // What the original keeps in r26..r31 (callees save them): the cells, the string pool, the arguments.
    c.set({{26, c.toc(toc_cell::found)}, {27, c.toc(toc::message)}, {28, c.toc(toc::pool)}, {29, offer}, {30, person},
           {31, c.toc(toc_cell::finder)}});
    const auto list = c.u32(offer + field::exchange);
    if (list == 0) {
        report_check(c, frame, {0x10417138, 0x40, 0x38, 0x677});
        return;
    }
    rewind(c, list);
    for (;;) {
        const auto row = invoke(c, callee::list_next, 0x104171a8, {c.u32(offer + field::exchange)});
        if (row == 0) break;
        if (c.i32(row + row_person) == c.i32(person + row_person))
            invoke(c, remove_row, 0x104171a0, {c.u32(offer + field::exchange), row});
    }
    if (c.i16(c.u32(offer + field::exchange) + list_count) == 0) {
        invoke(c, callee::list_destroy, 0x104171c8, {c.u32(offer + field::exchange), 1});
        c.put32(offer + field::exchange, 0);
    }
    const auto found = invoke(c, find_player, 0x104171dc, {c.u32(c.toc(toc_cell::finder)), person});
    c.put32(c.toc(toc_cell::found), found);
    const std::uint32_t club = found == 0 ? 0 : c.u32(c.toc(toc::clubs)) + c.u32(found + 4) * club_size;
    const auto value = invoke(c, value_player, 0x1041721c, {c.u32(c.toc(toc_cell::finder)), person, club, 0, 0xffffffffu});
    c.put32(offer + field::fee, c.u32(offer + field::fee) + value);
    const auto row_of_person = c.u32(c.u32(c.toc(toc_cell::players)) + player_table_counts) + c.u32(person + row_person) * 10;
    c.put8(row_of_person + 2, c.u8(row_of_person + 2) - 1u);
}

// 0x104172f0
namespace {
namespace d {  // the unit's double constants
inline constexpr std::uint32_t reshape_ratio = 0x30, share_ratio = 0xd8;
}
namespace person_field { inline constexpr std::uint32_t contract = 0x0c, profile = 0x14, settled = 0x5e; }
constexpr std::int32_t sell_on_share = 0x14, instalment_months = 0x14, extra_count = 10;
// `fee * 2` against the employer's 64-bit amount, signed: the subfc/subfe chain at 0x104173a0..0x104173b0 goes on
// when the doubled fee (as a signed word) is at least the amount (r3 the high word, r4 the low).
bool doubled_fee_covered(std::uint32_t fee, std::uint32_t high, std::uint32_t low) {
    const std::int64_t doubled = std::int32_t(fee << 1);
    const std::int64_t amount = std::int64_t(std::uint64_t(high) << 32 | low);
    return doubled >= amount;
}
}

void reshape_fee(Cpu& c, std::uint32_t offer, std::uint32_t person, std::uint32_t club) {
    Frame frame(c, 0x104172f0, 0x70, {.first_gpr = 28});
    Fpu fpu(c);
    const auto doubles = c.toc(toc::doubles);
    c.set({{28, offer}, {29, person}, {30, club}, {31, doubles}});  // what the original keeps in r28..r31
    const auto profile = c.u32(person + person_field::profile);
    if (profile == 0) return;
    const std::int32_t range_start = c.i16(profile + 4), range_end = c.i16(profile + 6);  // read in this order
    if (range_end - range_start < 0x19) return;
    if (c.i8(offer + field::sell_on_percent) != 0 || c.i32(offer + field::extra) != 0 || c.i32(offer + field::instalments) != 0) return;
    const auto employer = invoke(c, find_employer, 0x10417374, {c.u32(c.toc(toc_cell::employers)), club});
    // 0x100f5770 answers in r3:r4 (a 64-bit amount).
    invoke(c, employer_funds, 0x1041738c, {employer, 0, 0, 0, 1, 0});
    const auto high = c.r(3), low = c.r(4);
    const auto fee = c.u32(offer + field::fee);
    if (!doubled_fee_covered(fee, high, low)) return;
    const std::int32_t reputation = c.i16(club + 0x1ec);
    const auto per_500 = [&] { return divide(c.i16(c.u32(person + person_field::profile) + 0xa), 500); };
    const auto magic_word = [](std::uint32_t amount) { return amount ^ 0x80000000u; };
    // The int-to-double conversions of the fee leave their words in this frame (the stores at 0x10417518..0x10417524
    // and their siblings on the other paths): the later reader at 0x103f88c4 (TRANSFER_MANAGER) reads them, so they
    // and the fctiwz results stored next to them are written at the original's offsets, on each path.
    const auto convert = [&](std::int32_t high_at, std::int32_t low_at, std::uint32_t amount) {
        c.put32(frame.at(low_at), magic_word(amount));
        c.put32(frame.at(high_at), 0x43300000);
        return to_double(c, fpu, doubles, std::int32_t(amount));
    };
    const auto scaled = [&](std::uint32_t ratio, double converted, std::int32_t result_at) {
        const auto bits = fpu.fctiwz(0, fpu.fmul(c.f64(doubles + ratio), converted));
        c.put64(frame.at(result_at), bits);
        return std::uint32_t(bits);
    };
    if (reputation < 0xf && per_500() - reputation > 2) {
        // A club well below the player's standing takes a smaller fee and a sell-on share.
        const auto converted = convert(0x50, 0x54, fee);
        c.put32(offer + field::fee, scaled(d::reshape_ratio, converted, 0x48));
        c.put8(offer + field::sell_on_percent, std::uint32_t(sell_on_share));
        return;
    }
    const auto contract = c.u32(person + person_field::contract);
    if (contract != 0 && c.i16(contract + 0xac) > 0x1388 && c.u32(person + person_field::profile) != 0 &&
        c.i16(c.u32(person + person_field::profile) + 0xa) > 0x109a && c.u8(person + person_field::settled) == 0) {
        // A rich buyer for a player above the bar: part of the fee becomes an appearance extra.
        const auto converted = convert(0x48, 0x4c, fee);
        c.put32(frame.at(0x40), 0x43300000);
        c.put32(offer + field::extra, scaled(d::share_ratio, converted, 0x50));
        c.put8(offer + field::extra_count, std::uint32_t(extra_count));
        const auto again = c.u32(offer + field::fee);
        c.put32(frame.at(0x44), magic_word(again));
        const auto converted_again = to_double(c, fpu, doubles, std::int32_t(again));
        c.put32(offer + field::fee, scaled(d::reshape_ratio, converted_again, 0x38));
        return;
    }
    if (per_500() - reputation < -3) {
        // Otherwise a club well above the player's standing pays part over time.
        const auto converted = convert(0x38, 0x3c, fee);
        c.put32(frame.at(0x48), 0x43300000);
        c.put32(offer + field::instalments, scaled(d::share_ratio, converted, 0x40));
        c.put8(offer + field::instalment_months, std::uint32_t(instalment_months));
        const auto again = c.u32(offer + field::fee);
        c.put32(frame.at(0x4c), magic_word(again));
        const auto converted_again = to_double(c, fpu, doubles, std::int32_t(again));
        c.put32(offer + field::fee, scaled(d::reshape_ratio, converted_again, 0x50));
    }
}
}

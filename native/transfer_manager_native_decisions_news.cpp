// TRANSFER_MANAGER.CPP, block "decisions": the news item that announces an offer (0x103fdc20).
#include "transfer_manager_native_decisions.hpp"
#include "transfer_manager_native_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t find_contract = 0x10082f20, list_next = 0x10738e00;
constexpr std::uint32_t recipients_query = 0x103fb840;  // (manager, offer, 0x3e, array, 0x1e) -> how many
constexpr std::uint32_t offer_text_code = 0x10416710, player_flag_test = 0x100c7a20, bid_flag_test = 0x100ce250;
constexpr std::uint32_t club_check = 0x10012e90, date_construct = 0x10715880, date_query = 0x103fb130;
constexpr std::uint32_t news_kind = 4000;
constexpr std::int32_t date_global = -0x7b80, club_manager_cell = -0x7afc;
// Frame offsets of the original's locals.
constexpr std::uint32_t recipients_at = 0x50, item_at = 0xc8, date_at = 0x48;
constexpr std::int32_t no_club = -1;

// Bits 7..2 of a status byte as a signed number (the contract kind of a contract record).
std::int32_t kind_of(std::uint32_t status) { return std::int8_t(status) >> 2; }
}

void post_offer_news(Cpu& c, std::uint32_t manager, std::uint32_t offer) {
    Frame frame(c, 0x103fdc20, 0x1e0, {.first_gpr = 21});
    const auto contracts_word = c.toc(toc::contracts), date_word = c.toc(date_global);
    keep(c, 29, c.toc(toc::people));
    keep(c, 21, manager);
    keep(c, 31, contracts_word);
    keep(c, 22, offer);
    keep(c, 27, date_word);
    keep(c, 25, 0);
    keep(c, 28, c.toc(toc::news));
    keep(c, 23, 0);
    keep(c, 26, c.toc(toc::found_contract));
    const auto people = [&] { return c.u32(c.toc(toc::people)); };

    auto person = people() + c.u32(offer + 0xc) * person_size;
    keep(c, 24, person);
    const auto found = invoke(c, find_contract, 0x103fdc70, {c.u32(contracts_word), person});
    c.put32(c.toc(toc::found_contract), found);
    const bool buying = found != 0 && kind_of(c.u8(found + 0x38)) != 5;
    const auto club = buying ? c.u32(c.toc(toc::clubs)) + c.u32(c.u32(c.toc(toc::found_contract)) + 4) * club_size : 0;
    keep(c, 26, club);
    keep(c, 23, 0);
    const auto recipients = frame.at(recipients_at);
    const auto count = invoke(c, recipients_query, 0x103fdcdc, {manager, offer, 0x3e, recipients, 0x1e});
    keep(c, 30, count);
    if (std::int8_t(count) == 0) return;

    NewsItem item(c, frame.at(item_at), news_kind, 0x103fdcf8);
    item.field(0, c.u32(person), 0x103fdd08);
    item.field(1, c.u32(c.u32(person + 0x28) + 0x20), 0x103fdd1c);
    item.field(2, c.u32(c.u32(person + 0x2c) + 0x20), 0x103fdd30);
    item.field(3, c.u32(c.u32(person + 0x30) + 0x20), 0x103fdd44);
    item.field(4, c.u32(offer + 8), 0x103fdd54);
    item.field(5, club ? c.u32(club) : std::uint32_t(no_club), 0x103fdd74);
    item.field(6, c.u32(offer + 0x28), 0x103fdd84);
    item.field(7, c.u32(offer + 0x2c), 0x103fdd94);
    item.field(8, c.u32(offer + 0x20), 0x103fdda4);
    item.field(9, std::uint32_t(c.i8(offer + 0x34)), 0x103fddb8);
    const auto text_code = invoke(c, offer_text_code, 0x103fddd0, {offer, 0, 1, 0, 1});
    item.field(10, text_code, 0x103fdde0);
    item.field(0xb, std::uint32_t(c.i8(offer + 0x37)), 0x103fddf4);
    item.field(0xe, c.u8(offer + 0x3c) >> 4, 0x103fde08);

    // The player's registration window (fields 0xc, 0xd, 0xf), when the offer is a signing by a club.
    if (c.i8(offer + 0x37) != 0 && club != 0 && std::int8_t(invoke(c, player_flag_test, 0x103fde24, {c.u8(person + 0x62)})) != 0) {
        const auto contract = invoke(c, find_contract, 0x103fde38, {c.u32(contracts_word), person});
        keep(c, 31, contract);
        if (contract == 0) {
            item.field(0xc, 0, 0x103fdf0c);
            item.field(0xd, 0, 0x103fdf1c);
            item.field(0xf, 0, 0x103fdf2c);
        } else {
            // The game date (year, day) against the contract's expiry (+0x30 year, +0x32 day): the
            // original's signed/unsigned halfword compare, an expiry strictly after today.
            const auto today = c.u32(date_word);
            const std::uint32_t today_day = today & 0xffff, expiry_day = c.u16(contract + 0x32);
            bool in_future;
            if (today_day < expiry_day) in_future = true;
            else if (today_day > expiry_day) in_future = false;
            else {
                const std::int32_t year = std::int16_t(today >> 16), expiry_year = c.i16(contract + 0x30);
                const std::int32_t x = expiry_year ^ year;
                in_future = (std::uint32_t((x >> 1) - (x & expiry_year)) >> 31) != 0;
            }
            if (in_future) {
                item.field(0xc, std::uint32_t(c.i16(contract + 0x30)), 0x103fdeac);
                item.field(0xd, c.u16(contract + 0x32), 0x103fdebc);
            } else {
                item.field(0xc, 0, 0x103fded0);
                item.field(0xd, 0, 0x103fdee0);
            }
            item.field(0xf, std::uint32_t(kind_of(c.u8(contract + 0x38))), 0x103fdef8);
        }
    }

    // Up to three of the offer's other players, four numbers each from field 0x18 on.
    std::uint32_t slot = 0x18, seen = 0;
    keep(c, 26, slot);
    if (const auto list = c.u32(offer + 4); list != 0) {
        c.put32(list + 0xc, c.u32(list + 4));
        for (;;) {
            const auto member = invoke(c, list_next, 0x103fdfcc, {c.u32(offer + 4)});
            if (member == 0) break;
            person = people() + c.u32(member) * person_size;
            keep(c, 24, person);
            if (std::int8_t(seen) < 3) {
                keep(c, 26, slot + 1);
                item.field(slot, c.u32(person), 0x103fdf78);
                keep(c, 26, slot + 2);
                item.field(slot + 1, c.u32(c.u32(person + 0x28) + 0x20), 0x103fdf90);
                keep(c, 26, slot + 3);
                item.field(slot + 2, c.u32(c.u32(person + 0x2c) + 0x20), 0x103fdfa8);
                keep(c, 26, slot + 4);
                item.field(slot + 3, c.u32(c.u32(person + 0x30) + 0x20), 0x103fdfc0);
                slot += 4;
            }
            ++seen;
            keep(c, 25, seen);
        }
    }
    item.field(0x17, std::uint32_t(std::int8_t(seen)), 0x103fdfe4);
    // The original keeps the last listed player (the first when the list is empty) in r24 and reads
    // its +0x20 record here: field 0x11 names it when the flag test accepts it. `flags` is the original's
    // r23 (bit 0: that record, bit 1: the club check below).
    std::int32_t flags = 0;
    if (c.u32(person + 0x20) != 0 && std::int8_t(invoke(c, bid_flag_test, 0x103fdff4, {c.u32(person + 0x20)})) != 0) {
        flags = std::int8_t(flags | 1);
        keep_signed(c, 23, flags);
        item.field(0x11, c.u32(c.u32(person + 0x20)), 0x103fe018);
    } else {
        item.field(0x11, std::uint32_t(no_club), 0x103fe02c);
    }
    if (std::int8_t(invoke(c, club_check, 0x103fe040, {c.u32(c.toc(club_manager_cell)), offer, 1})) != 0) {
        flags = std::int8_t(flags | 2);
        keep_signed(c, 23, flags);
    }
    item.field(0x10, std::uint32_t(std::int8_t(flags)), 0x103fe060);
    item.field(slot, std::uint32_t(c.i8(offer + 0x11)), 0x103fe074);

    const std::int32_t total = std::int8_t(count);
    keep_signed(c, 24, total);
    std::uint32_t entry = recipients, sent = 0;
    keep(c, 23, entry);
    keep(c, 25, sent);
    while (std::int8_t(sent) < total) {
        item.post(c.u32(entry), 0x103fe098);
        entry += 4;
        keep(c, 23, entry);
        ++sent;
        keep(c, 25, sent);
    }

    if (c.i8(offer + 0x37) == 0) return;
    if ((std::int8_t(c.u8(offer + 0x38)) >> 4) != 5) {
        item.field(0x12, 3, 0x103fe180);
        return;
    }
    const auto date = frame.at(date_at);
    invoke(c, date_construct, 0x103fe0d8, {date});
    invoke_native(c, date_query, 0x103fe0e8, {manager, date, 0});
    const auto today = c.u32(date_word);
    const bool same_day = (today & 0xffff) == c.u16(date + 2) && std::int16_t(today >> 16) == c.i16(date);
    item.field(0x12, same_day ? 0 : 5, same_day ? 0x103fe138 : 0x103fe14c);
    item.field(0xc, std::uint32_t(c.i16(date)), 0x103fe15c);
    item.field(0xd, c.u16(date + 2), 0x103fe16c);
}
}

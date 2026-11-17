#include "transfer_manager_native_terms.hpp"
#include "transfer_manager_native_stages_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace stages;

namespace {
constexpr std::uint32_t news_kind = 0xfa5, news_club_slot = 0x10;
// The original's "is the contract's date `own` earlier than the default" (see term_date): the sign of
// ((x >> 1) - (x & a)) with x = a ^ b over sign-extended halfwords.
bool first_halfword_decides(std::int32_t a, std::int32_t b) {
    const auto x = a ^ b;
    return ((x >> 1) - (x & a)) < 0;
}
}

void post_term_news(Cpu& c, std::uint32_t manager, std::uint32_t record) {
    Frame frame(c, 0x104058e0, 0x160, {.first_gpr = 23});
    const auto contracts_cell = c.toc(toc::contracts), found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news);
    const auto clubs_cell = c.toc(toc::clubs);
    const auto news = c.u32(news_cell);
    keep(c, 23, found_cell);
    keep(c, 24, manager);
    keep(c, 25, record);
    keep(c, 26, 0);
    keep(c, 28, 0);
    keep(c, 29, news_cell);
    keep(c, 30, contracts_cell);
    keep(c, 31, clubs_cell);
    const auto person = person_record(c, c.u32(record + 0xc));
    keep(c, 27, person);
    const auto contract = invoke(c, find_contract, 0x10405930, {c.u32(contracts_cell), person});
    c.put32(found_cell, contract);
    const bool has_club = contract && contract_kind(c, contract) != no_club_contract_kind;
    keep(c, 26, has_club ? 1 : 0);
    auto club = has_club ? club_record(c, c.u32(c.u32(found_cell) + 4)) : 0;
    keep(c, 23, club);
    keep(c, 26, club);
    const auto buyer = club_record(c, c.u32(record + 8));
    keep(c, 31, buyer);
    const auto date = frame.at(0x40);
    invoke(c, 0x10715880, 0x10405998, {date});
    if (club) {
        const auto again = invoke(c, find_contract, 0x104059ac, {c.u32(contracts_cell), person});
        // The second lookup is read through without a test, as the original does.
        if (contract_kind(c, again) == no_club_contract_kind) club = 0;
        keep(c, 26, club);
    }
    NewsItem item(c, frame.at(0x48), news_kind, 0x104059d8);
    item.field(0, c.u32(person), 0x104059e8);
    item.field(1, c.u32(c.u32(person + 0x28) + 0x20), 0x104059fc);
    item.field(2, c.u32(c.u32(person + 0x2c) + 0x20), 0x10405a10);
    item.field(3, c.u32(c.u32(person + 0x30) + 0x20), 0x10405a24);
    item.field(4, c.u32(record + 8), 0x10405a34);
    item.field(5, club ? c.u32(club) : 0xffffffff, 0x10405a54);
    item.field(6, c.u32(record + 0x28), 0x10405a64);
    item.field(7, c.u32(record + 0x2c), 0x10405a74);
    item.field(8, c.u32(record + 0x20), 0x10405a84);
    item.field(9, std::uint32_t(std::int32_t(c.i8(record + 0x34))), 0x10405a98);
    const auto value = invoke(c, 0x10416710, 0x10405ab0, {record, 0, 1, 0, 1});
    item.field(10, value, 0x10405ac0);
    item.field(12, std::uint32_t(std::int32_t(c.i8(record + 0x37))), 0x10405ad4);
    item.field(0x11, std::uint32_t(std::int32_t(c.i8(record + 0x11))), 0x10405ae8);
    invoke_native(c, 0x103f89e0, 0x10405af8, {manager, record, date});
    const auto default_date = c.toc(-0x7b80);
    const auto default_second = c.u16(default_date + 2), own_second = c.u16(date + 2);
    bool dated;
    if (default_second < own_second) dated = true;
    else if (own_second < default_second) dated = false;
    else dated = first_halfword_decides(c.i16(date), c.i16(default_date));
    if (dated) {
        item.field(0xe, std::uint32_t(std::int32_t(c.i16(date))), 0x10405b68);
        item.field(0xf, c.u16(date + 2), 0x10405b78);
        item.field(0xd, std::uint32_t(std::int32_t(std::int8_t(c.u8(record + 0x38)) >> 4)), 0x10405b90);
    } else {
        item.field(0xe, 0, 0x10405ba4);
        item.field(0xf, 0, 0x10405bb4);
        item.field(0xd, 0, 0x10405bc4);
    }
    if (const auto list = c.u32(record + 4)) keep(c, 28, std::uint32_t(std::int32_t(std::int8_t(c.u16(list)))));
    item.field(0xb, c.r(28), 0x10405be8);
    if (club) {
        const auto wants = std::int8_t(invoke(c, 0x102cdbf0, 0x10405bfc, {news, club}));
        if (wants && c.u8(record + 0x34) != 0xc &&
            !(c.i8(record + 0x37) != 0 && (std::int8_t(c.u8(record + 0x38)) >> 4) != 0)) {
            item.field(news_club_slot, c.u32(club), 0x10405c40);
            item.post(club, 0x10405c50);
        }
    }
    if (std::int8_t(invoke(c, 0x102cdbf0, 0x10405c5c, {news, buyer}))) {
        item.field(news_club_slot, c.u32(buyer), 0x10405c74);
        item.post(buyer, 0x10405c84);
    }
}
}

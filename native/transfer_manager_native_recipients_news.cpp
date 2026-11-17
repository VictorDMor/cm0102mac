#include "transfer_manager_native_recipients.hpp"
#include "transfer_manager_native_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t find_contract = 0x10082f20;  // (contract manager, person) -> contract or 0
constexpr std::int32_t shortlist_cell = -0x7b28;     // the cell holding the object 0x102de3c0 tests
constexpr std::uint32_t person_name_words = 0x28;    // +0x28, +0x2c, +0x30: records whose word +0x20 names the person
}

void announce_signing(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t buyer, std::uint32_t kind,
                      std::uint32_t offer) {
    Frame frame(c, 0x10410000, 0x1a0, {.first_gpr = 23});
    const auto news = c.toc(toc::news), contracts = c.toc(toc::contracts), found = c.toc(toc::found_contract),
               clubs = c.toc(toc::clubs);
    keep(c, 23, manager);
    keep(c, 24, person);
    keep(c, 25, buyer);
    keep(c, 26, kind);
    keep(c, 27, offer);
    keep(c, 28, news);
    keep(c, 29, contracts);
    keep(c, 30, found);
    keep(c, 31, clubs);
    if (!person || !buyer) return;
    const auto club_of_person = [&](std::uint32_t back) {
        const auto contract = invoke(c, find_contract, back, {c.u32(contracts), person});
        c.put32(found, contract);
        return contract ? c.u32(clubs) + c.u32(contract + 4) * club_size : 0;
    };
    if (!club_of_person(0x10410048)) return;
    if (std::int8_t(invoke(c, 0x102de3c0, 0x10410088, {c.u32(c.toc(shortlist_cell)), person})) == 1) return;
    if (std::int8_t(invoke(c, 0x103f43b0, 0x104100a4, {manager, person, 0})) != 0) return;
    const auto returned = invoke_native(c, 0x103fc710, 0x104100c8, {manager, person, 10, frame.at(0x38), 0x14});
    const auto count = std::int8_t(returned);
    keep(c, 23, returned);
    if (count == 0) return;
    constexpr std::uint32_t item_at = 0x88;
    NewsItem item(c, frame.at(item_at), 0xfbf, 0x104100e4);
    item.field(0, c.u32(person), 0x104100f4);
    item.field(1, c.u32(c.u32(person + person_name_words) + 0x20), 0x10410108);
    item.field(2, c.u32(c.u32(person + person_name_words + 4) + 0x20), 0x1041011c);
    item.field(3, c.u32(c.u32(person + person_name_words + 8) + 0x20), 0x10410130);
    const auto club = club_of_person(0x1041013c);
    item.field(4, c.u32(club), 0x10410170);  // a person without a club reads guest address 0
    item.field(5, c.u32(buyer), 0x10410180);
    item.field(6, std::uint32_t(std::int32_t(std::int8_t(kind))), 0x10410190);
    item.field(7, c.u32(offer + 0x24), 0x104101a0);
    keep(c, 25, std::uint32_t(std::int32_t(count)));
    for (std::uint32_t i = 0; std::int8_t(i) < count; ++i) {
        keep(c, 23, i);
        keep(c, 24, frame.at(0x38) + 4 * i);
        item.post(c.u32(frame.at(0x38) + 4 * i), 0x104101c0);
    }
}
}

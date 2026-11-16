#include "transfer_manager_native_news_outcomes.hpp"
#include "transfer_manager_native_news_outcomes_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace news_outcomes;

namespace {
constexpr std::uint32_t all_ones = 0xffffffff;
constexpr std::uint32_t listed_words = 0x38, item_at = 0xb0, listed_max = 0x1e;  // frame offsets and capacity of the club list
}

void post_offer_news_fab(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t short_raw) {
    Frame frame(c, 0x10409200, 0x1c0, {.first_gpr = 24});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), clubs_cell = c.toc(toc::clubs);
    const auto person = offer_person(c, offer);
    keep(c, 24, 0);
    keep(c, 25, manager);
    keep(c, 26, offer);
    keep(c, 27, short_raw);
    keep(c, 28, person);
    keep(c, 29, found_cell);
    keep(c, 30, news_cell);
    keep(c, 31, clubs_cell);
    const auto lookup = contract_club(c, person, 0x10409250);
    keep(c, 24, lookup.found);
    keep(c, 29, lookup.club);
    const auto club_table = c.u32(clubs_cell);
    const auto offer_club_offset = c.u32(offer + 8) * club_size;
    keep(c, 24, club_table);
    keep(c, 31, offer_club_offset);
    const std::uint32_t kind = c.i8(offer + 0x34) == 0x11 ? 1 : 2;
    const auto listed_raw = invoke(c, offer_people, 0x104092d8, {manager, offer, kind, frame.at(listed_words), listed_max});
    keep(c, 25, listed_raw);
    const auto listed = std::int8_t(listed_raw);
    if (listed == 0) return;
    NewsItem item(c, frame.at(item_at), 0xfab, 0x104092f4);
    item.field(0, lookup.club ? c.u32(lookup.club) : all_ones, 0x10409314);
    item.field(1, c.u32(club_table + offer_club_offset), 0x10409324);
    item.field(2, c.u32(person), 0x10409334);
    item.field(3, name_value(c, person, 0x28), 0x10409348);
    item.field(4, name_value(c, person, 0x2c), 0x1040935c);
    item.field(5, name_value(c, person, 0x30), 0x10409370);
    item.field(6, sign8(c, offer + 0x34), 0x10409384);
    item.field(7, 0, 0x10409394);
    item.field(8, 0, 0x104093a4);
    item.field(9, c.u32(offer + 0x28), 0x104093b4);
    item.field(10, sign16(short_raw), 0x104093c4);
    keep(c, 26, std::uint32_t(std::int32_t(listed)));
    for (std::uint32_t i = 0; std::int8_t(i) < listed; ++i) {
        keep(c, 24, i);
        keep(c, 25, frame.at(listed_words) + i * 4);
        item.post(c.u32(frame.at(listed_words) + i * 4), 0x104093e4);
    }
}

void post_offer_news_fad(Cpu& c, std::uint32_t offer, std::uint32_t other_person, std::uint32_t value, std::uint32_t short_raw) {
    Frame frame(c, 0x104099f0, 0x150, {.first_gpr = 23});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), clubs_cell = c.toc(toc::clubs);
    const auto person = offer_person(c, offer);
    keep(c, 23, found_cell);
    keep(c, 24, clubs_cell);
    keep(c, 25, 0);
    keep(c, 26, offer);
    keep(c, 27, other_person);
    keep(c, 28, value);
    keep(c, 29, short_raw);
    keep(c, 30, person);
    keep(c, 31, news_cell);
    const auto lookup = contract_club(c, person, 0x10409a44);
    keep(c, 25, lookup.found);
    keep(c, 23, lookup.club);
    const auto offer_club = club_at(c, c.u32(offer + 8));
    keep(c, 24, offer_club);
    if (std::int8_t(invoke(c, human_club, 0x10409aac, {c.u32(news_cell), offer_club})) == 0) return;
    NewsItem item(c, frame.at(0x38), 0xfad, 0x10409ac4);
    // No guard for a missing club here: field 0 reads the word at guest address 0.
    item.field(0, c.u32(lookup.club), 0x10409ad4);
    item.field(1, c.u32(offer_club), 0x10409ae4);
    item.field(2, c.u32(person), 0x10409af4);
    item.field(3, name_value(c, person, 0x28), 0x10409b08);
    item.field(4, name_value(c, person, 0x2c), 0x10409b1c);
    item.field(5, name_value(c, person, 0x30), 0x10409b30);
    item.field(6, c.u32(other_person), 0x10409b40);
    item.field(7, name_value(c, other_person, 0x28), 0x10409b54);
    item.field(8, name_value(c, other_person, 0x2c), 0x10409b68);
    item.field(9, name_value(c, other_person, 0x30), 0x10409b7c);
    item.field(10, value, 0x10409b8c);
    item.field(11, c.u32(offer + 0x20), 0x10409b9c);
    item.field(12, sign16(short_raw), 0x10409bac);
    item.post(offer_club, 0x10409bbc);
}

void post_offer_news_fb0(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t other_person, std::uint32_t short_raw) {
    Frame frame(c, 0x1040a050, 0x1c0, {.first_gpr = 22});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), clubs_cell = c.toc(toc::clubs),
               contracts_cell = c.toc(toc::contracts);
    const auto person = offer_person(c, offer);
    keep(c, 22, found_cell);
    keep(c, 23, 0);
    keep(c, 24, clubs_cell);
    keep(c, 25, manager);
    keep(c, 26, offer);
    keep(c, 27, other_person);
    keep(c, 28, short_raw);
    keep(c, 29, person);
    keep(c, 30, news_cell);
    keep(c, 31, contracts_cell);
    const auto lookup = contract_club(c, person, 0x1040a0a4);
    keep(c, 23, lookup.found);
    auto club = lookup.club;
    keep(c, 22, club);
    const auto club_table = c.u32(clubs_cell);
    const auto offer_club_offset = c.u32(offer + 8) * club_size;
    keep(c, 23, club_table);
    keep(c, 24, offer_club_offset);
    const auto listed_raw = invoke(c, offer_people, 0x1040a114, {manager, offer, 0x2f, frame.at(listed_words), listed_max});
    keep(c, 25, listed_raw);
    const auto listed = std::int8_t(listed_raw);
    if (listed == 0) return;
    if (club != 0) {
        // A second lookup that is not stored; a missing contract here is read through (status at address 0x38).
        const auto contract = invoke(c, find_contract, 0x1040a134, {c.u32(contracts_cell), person});
        if ((c.i8(contract + contract_status) >> 2) == 5) {
            club = 0;
            keep(c, 22, 0);
        }
    }
    NewsItem item(c, frame.at(item_at), 0xfb0, 0x1040a160);
    item.field(0, club ? c.u32(club) : all_ones, 0x1040a180);
    item.field(1, c.u32(club_table + offer_club_offset), 0x1040a190);
    item.field(2, c.u32(person), 0x1040a1a0);
    item.field(3, name_value(c, person, 0x28), 0x1040a1b4);
    item.field(4, name_value(c, person, 0x2c), 0x1040a1c8);
    item.field(5, name_value(c, person, 0x30), 0x1040a1dc);
    item.field(6, other_person ? c.u32(other_person) : all_ones, 0x1040a1fc);
    item.field(7, sign16(short_raw), 0x1040a20c);
    item.field(8, sign8(c, offer + 0x24), 0x1040a220);
    const auto short_value = sign16(short_raw);
    keep(c, 28, short_value);
    keep(c, 26, std::uint32_t(std::int32_t(listed)));
    for (std::uint32_t i = 0; std::int8_t(i) < listed; ++i) {
        const auto entry_at = frame.at(listed_words) + i * 4;
        keep(c, 22, i);
        keep(c, 25, entry_at);
        const auto listed_club = c.u32(entry_at);
        if (listed_club != other_person || std::int32_t(short_value) != 0xb) item.post(listed_club, 0x1040a258);
    }
}

void post_offer_news_fb3(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t shorts) {
    Frame frame(c, 0x1040c220, 0x1d0, {.first_gpr = 21});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), people_cell = c.toc(toc::people),
               contracts_cell = c.toc(toc::contracts), clubs_cell = c.toc(toc::clubs);
    const auto person = offer_person(c, offer);
    keep(c, 22, offer);
    keep(c, 23, shorts);
    keep(c, 24, person);
    keep(c, 25, 0);
    keep(c, 26, news_cell);
    keep(c, 27, people_cell);
    keep(c, 28, contracts_cell);
    keep(c, 29, found_cell);
    keep(c, 30, clubs_cell);
    const auto listed_raw = invoke(c, offer_people, 0x1040c270, {manager, offer, 0xf, frame.at(listed_words), listed_max});
    keep(c, 31, listed_raw);
    const auto listed = std::int8_t(listed_raw);
    if (listed == 0) return;
    NewsItem item(c, frame.at(item_at), 0xfb3, 0x1040c28c);
    item.field(0, c.u32(person), 0x1040c29c);
    item.field(1, name_value(c, person, 0x28), 0x1040c2b0);
    item.field(2, name_value(c, person, 0x2c), 0x1040c2c4);
    item.field(3, name_value(c, person, 0x30), 0x1040c2d8);
    item.field(4, c.u32(offer + 8), 0x1040c2e8);
    keep(c, 21, 0);
    const auto first = contract_club(c, person, 0x1040c2f8);
    keep(c, 21, first.found);
    std::uint32_t club_id = all_ones;
    if (first.club != 0) {
        keep(c, 24, 0);
        const auto second = contract_club(c, person, 0x1040c35c);
        keep(c, 24, second.found);
        club_id = c.u32(second.club);  // read through when the second lookup finds none
    }
    item.field(5, club_id, 0x1040c3c0);
    item.field(6, c.u32(offer + 0x20), 0x1040c3d0);
    item.field(7, sign8(c, offer + 0x37), 0x1040c3e4);
    item.field(8, invoke(c, offer_value, 0x1040c3fc, {offer, 0, 1, 0, 1}), 0x1040c40c);
    item.field(9, std::uint32_t(std::int32_t(c.i8(offer + 0x38)) >> 4), 0x1040c424);
    item.field(10, std::uint32_t(std::int32_t(c.i16(shorts))), 0x1040c434);
    item.field(11, c.u16(shorts + 2), 0x1040c444);
    std::uint32_t slot = 0xd, seen = 0;
    keep(c, 23, slot);
    if (const auto list = c.u32(offer + 4); list != 0) {
        c.put32(list + 0xc, c.u32(list + 4));
        for (;;) {
            const auto entry = invoke(c, list_next, 0x1040c4e4, {c.u32(offer + 4)});
            if (entry == 0) break;
            const auto listed_person = c.u32(people_cell) + c.u32(entry) * person_size;
            keep(c, 21, listed_person);
            // The first three listed players are named; the count goes on (as a signed byte) past them.
            if (std::int8_t(seen) < 3) {
                keep(c, 23, slot + 1);
                item.field(slot++, c.u32(listed_person), 0x1040c490);
                keep(c, 23, slot + 1);
                item.field(slot++, name_value(c, listed_person, 0x28), 0x1040c4a8);
                keep(c, 23, slot + 1);
                item.field(slot++, name_value(c, listed_person, 0x2c), 0x1040c4c0);
                keep(c, 23, slot + 1);
                item.field(slot++, name_value(c, listed_person, 0x30), 0x1040c4d8);
            }
            keep(c, 25, ++seen);
        }
    }
    item.field(0xc, std::uint32_t(std::int32_t(std::int8_t(seen))), 0x1040c4fc);
    const auto human = invoke(c, human_club, 0x1040c514, {c.u32(news_cell), club_at(c, c.u32(offer + 8))});
    if (std::int8_t(human) != 0) {
        invoke(c, send_to_all, 0x1040c528, {c.u32(news_cell), item.at});
        return;
    }
    keep(c, 23, std::uint32_t(std::int32_t(listed)));
    for (std::uint32_t i = 0; std::int8_t(i) < listed; ++i) {
        const auto entry_at = frame.at(listed_words) + i * 4;
        keep(c, 21, i);
        keep(c, 22, entry_at);
        item.post(c.u32(entry_at), 0x1040c550);
    }
}

void post_offer_news_fb4(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t record) {
    Frame frame(c, 0x1040ce40, 0x1c0, {.first_gpr = 23});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), contracts_cell = c.toc(toc::contracts),
               clubs_cell = c.toc(toc::clubs);
    const auto person = offer_person(c, offer);
    keep(c, 24, offer);
    keep(c, 25, record);
    keep(c, 26, person);
    keep(c, 27, news_cell);
    keep(c, 28, contracts_cell);
    keep(c, 29, found_cell);
    keep(c, 30, clubs_cell);
    const auto listed_raw = invoke(c, offer_people, 0x1040ce8c, {manager, offer, 0xf, frame.at(listed_words), listed_max});
    keep(c, 31, listed_raw);
    const auto listed = std::int8_t(listed_raw);
    if (listed == 0) return;
    NewsItem item(c, frame.at(item_at), 0xfb4, 0x1040cea8);
    item.field(0, c.u32(person), 0x1040ceb8);
    item.field(1, name_value(c, person, 0x28), 0x1040cecc);
    item.field(2, name_value(c, person, 0x2c), 0x1040cee0);
    item.field(3, name_value(c, person, 0x30), 0x1040cef4);
    item.field(4, c.u32(offer + 8), 0x1040cf04);
    keep(c, 23, 0);
    const auto first = contract_club(c, person, 0x1040cf14);
    keep(c, 23, first.found);
    std::uint32_t club_id = all_ones;
    if (first.club != 0) {
        keep(c, 26, 0);
        const auto second = contract_club(c, person, 0x1040cf78);
        keep(c, 26, second.found);
        club_id = c.u32(second.club);  // read through when the second lookup finds none
    }
    item.field(5, club_id, 0x1040cfdc);
    item.field(6, c.u32(offer + 0x20), 0x1040cfec);
    item.field(7, sign8(c, offer + 0x37), 0x1040d000);
    item.field(8, invoke(c, offer_value, 0x1040d018, {offer, 0, 0, 0, 1}), 0x1040d028);
    item.field(9, c.u32(record + 0x68), 0x1040d038);
    item.field(10, sign8(c, offer + 0x34), 0x1040d04c);
    item.field(11, c.u32(offer + 0x28), 0x1040d05c);
    item.field(12, c.u32(offer + 0x2c), 0x1040d06c);
    item.field(13, sign8(c, offer + 0x24), 0x1040d080);
    keep(c, 25, std::uint32_t(std::int32_t(listed)));
    for (std::uint32_t i = 0; std::int8_t(i) < listed; ++i) {
        const auto entry_at = frame.at(listed_words) + i * 4;
        keep(c, 23, i);
        keep(c, 24, entry_at);
        item.post(c.u32(entry_at), 0x1040d0a0);
    }
}

void post_offer_news_fb6(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t record) {
    Frame frame(c, 0x1040ddc0, 0x1b0, {.first_gpr = 26});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news);
    const auto person = offer_person(c, offer);
    keep(c, 26, offer);
    keep(c, 27, found_cell);
    keep(c, 28, person);
    keep(c, 29, record);
    keep(c, 30, news_cell);
    const auto listed_raw = invoke(c, offer_people, 0x1040de04, {manager, offer, 0x5f, frame.at(listed_words), listed_max});
    keep(c, 31, listed_raw);
    const auto listed = std::int8_t(listed_raw);
    if (listed == 0) return;
    NewsItem item(c, frame.at(item_at), 0xfb6, 0x1040de20);
    item.field(0, c.u32(person), 0x1040de30);
    item.field(1, name_value(c, person, 0x28), 0x1040de44);
    item.field(2, name_value(c, person, 0x2c), 0x1040de58);
    item.field(3, name_value(c, person, 0x30), 0x1040de6c);
    item.field(4, c.u32(offer + 8), 0x1040de7c);
    keep(c, 28, 0);
    const auto lookup = contract_club(c, person, 0x1040de90);
    keep(c, 28, lookup.found);
    item.field(5, c.u32(lookup.club), 0x1040def0);  // no guard: a person without a club reads address 0
    item.field(7, 0, 0x1040df00);
    item.field(8, std::uint32_t(std::int32_t(c.i16(record + 0x30))), 0x1040df10);
    item.field(9, c.u16(record + 0x32), 0x1040df20);
    keep(c, 31, std::uint32_t(std::int32_t(listed)));
    for (std::uint32_t i = 0; std::int8_t(i) < listed; ++i) {
        const auto entry_at = frame.at(listed_words) + i * 4;
        keep(c, 27, i);
        keep(c, 29, entry_at);
        item.post(c.u32(entry_at), 0x1040df44);
    }
}

void post_offer_news_fb7(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t short_raw) {
    Frame frame(c, 0x1040e2d0, 0x1c0, {.first_gpr = 24});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), clubs_cell = c.toc(toc::clubs);
    const auto person = offer_person(c, offer);
    const auto offer_club = club_at(c, c.u32(offer + 8));
    keep(c, 24, person);
    keep(c, 25, offer);
    keep(c, 26, short_raw);
    keep(c, 27, offer_club);
    keep(c, 28, news_cell);
    keep(c, 29, found_cell);
    keep(c, 30, clubs_cell);
    // 0x103fb840's callees read r9 and r10 as this routine left them (the person's byte offset, the person table).
    c.set(8, c.u32(clubs_cell));
    c.set(9, c.u32(offer + 0xc) * person_size);
    c.set(10, c.u32(c.toc(toc::people)));
    const auto listed_raw = invoke(c, offer_people, 0x1040e328, {manager, offer, 0xf, frame.at(listed_words), listed_max});
    keep(c, 31, listed_raw);
    const auto listed = std::int8_t(listed_raw);
    if (listed == 0) return;
    NewsItem item(c, frame.at(item_at), 0xfb7, 0x1040e344);
    item.field(0, c.u32(person), 0x1040e354);
    item.field(1, name_value(c, person, 0x28), 0x1040e368);
    item.field(2, name_value(c, person, 0x2c), 0x1040e37c);
    item.field(3, name_value(c, person, 0x30), 0x1040e390);
    item.field(4, c.u32(offer + 8), 0x1040e3a0);
    keep(c, 25, 0);
    const auto lookup = contract_club(c, person, 0x1040e3b4);
    keep(c, 25, lookup.found);
    item.field(5, c.u32(lookup.club), 0x1040e40c);  // no guard: a person without a club reads address 0
    item.field(6, sign16(short_raw), 0x1040e420);
    const auto pointer = c.u32(offer_club + 0x30);
    item.field(7, pointer ? c.u32(pointer) : all_ones, 0x1040e444);
    keep(c, 26, std::uint32_t(std::int32_t(listed)));
    for (std::uint32_t i = 0; std::int8_t(i) < listed; ++i) {
        const auto entry_at = frame.at(listed_words) + i * 4;
        keep(c, 24, i);
        keep(c, 25, entry_at);
        item.post(c.u32(entry_at), 0x1040e468);
    }
}
}

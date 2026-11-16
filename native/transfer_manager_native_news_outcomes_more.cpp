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

void post_offer_news_fb9(Cpu& c, std::uint32_t manager, std::uint32_t offer) {
    Frame frame(c, 0x1040ea40, 0x1d0, {.first_gpr = 18});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), people_cell = c.toc(toc::people),
               contracts_cell = c.toc(toc::contracts), clubs_cell = c.toc(toc::clubs);
    const auto person = c.u32(people_cell) + c.u32(offer + 0xc) * person_size;
    keep(c, 18, 0);
    keep(c, 19, manager);
    keep(c, 20, offer);
    keep(c, 24, person);
    keep(c, 26, news_cell);
    keep(c, 27, people_cell);
    keep(c, 28, contracts_cell);
    keep(c, 29, found_cell);
    keep(c, 30, clubs_cell);
    const auto lookup = contract_club(c, person, 0x1040ea8c);
    keep(c, 18, lookup.found);
    keep(c, 25, lookup.club);
    const auto offer_club = club_at(c, c.u32(offer + 8));
    keep(c, 22, 0);
    keep(c, 21, 0);
    keep(c, 23, offer_club);
    const auto listed_raw = invoke(c, offer_people, 0x1040eb08, {manager, offer, 0xf, frame.at(listed_words), listed_max});
    keep(c, 31, listed_raw);
    const auto listed = std::int8_t(listed_raw);
    if (listed == 0) return;
    NewsItem item(c, frame.at(item_at), 0xfb9, 0x1040eb24);
    item.field(0, c.u32(person), 0x1040eb34);
    item.field(1, name_value(c, person, 0x28), 0x1040eb48);
    item.field(2, name_value(c, person, 0x2c), 0x1040eb5c);
    item.field(3, name_value(c, person, 0x30), 0x1040eb70);
    item.field(4, c.u32(offer + 8), 0x1040eb80);
    keep(c, 18, 0);
    const auto second = contract_club(c, person, 0x1040eb90);
    keep(c, 18, second.found);
    if (second.club != 0) {
        keep(c, 28, 0);
        const auto third = contract_club(c, person, 0x1040ebf4);
        keep(c, 28, third.found);
        item.field(5, c.u32(third.club), 0x1040ec50);  // read through when this lookup finds none
    } else {
        item.field(5, all_ones, 0x1040ec64);
    }
    const auto person_status_of = [&](std::uint32_t subject, std::uint32_t club, std::uint32_t back) {
        return std::int8_t(invoke(c, news_outcomes::person_status, back, {manager, subject, club, 0}));
    };
    item.field(6, std::uint32_t(std::int32_t(person_status_of(person, offer_club, 0x1040ec78))), 0x1040ec88);
    std::uint32_t first_person = 0, usable = 0;
    if (const auto list = c.u32(offer + 4); list != 0) {
        c.put32(list + 0xc, c.u32(list + 4));
        for (;;) {
            const auto entry = invoke(c, list_next, 0x1040ece4, {c.u32(offer + 4)});
            if (entry == 0) break;
            const auto listed_person = c.u32(people_cell) + c.u32(entry) * person_size;
            keep(c, 18, listed_person);
            if (person_status_of(listed_person, lookup.club, 0x1040ecc4) != 0) {
                if (first_person == 0) {
                    first_person = listed_person;
                    keep(c, 21, first_person);
                }
                keep(c, 22, ++usable);
            }
        }
    }
    item.field(7, std::uint32_t(std::int32_t(std::int8_t(usable))), 0x1040ecfc);
    if (first_person != 0) {
        item.field(8, c.u32(first_person), 0x1040ed14);
        item.field(9, name_value(c, first_person, 0x28), 0x1040ed28);
        item.field(10, name_value(c, first_person, 0x2c), 0x1040ed3c);
        item.field(11, name_value(c, first_person, 0x30), 0x1040ed50);
    }
    keep(c, 20, std::uint32_t(std::int32_t(listed)));
    for (std::uint32_t i = 0; std::int8_t(i) < listed; ++i) {
        const auto entry_at = frame.at(listed_words) + i * 4;
        keep(c, 18, i);
        keep(c, 19, entry_at);
        item.post(c.u32(entry_at), 0x1040ed70);
    }
}

void post_person_news_fba(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t word, std::uint32_t offer) {
    Frame frame(c, 0x1040f270, 0x1c0, {.first_gpr = 25});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), contracts_cell = c.toc(toc::contracts),
               clubs_cell = c.toc(toc::clubs);
    keep(c, 25, word);
    keep(c, 26, person);
    keep(c, 27, news_cell);
    keep(c, 28, contracts_cell);
    keep(c, 29, found_cell);
    keep(c, 30, clubs_cell);
    const auto listed_raw = invoke(c, offer_people, 0x1040f2ac, {manager, offer, 3, frame.at(listed_words), listed_max});
    keep(c, 31, listed_raw);
    const auto listed = std::int8_t(listed_raw);
    if (listed == 0) return;
    NewsItem item(c, frame.at(item_at), 0xfba, 0x1040f2c8);
    item.field(0, c.u32(person), 0x1040f2d8);
    item.field(1, name_value(c, person, 0x28), 0x1040f2ec);
    item.field(2, name_value(c, person, 0x2c), 0x1040f300);
    item.field(3, name_value(c, person, 0x30), 0x1040f314);
    item.field(4, c.u32(word), 0x1040f324);
    keep(c, 25, 0);
    const auto first = contract_club(c, person, 0x1040f334);
    keep(c, 25, first.found);
    std::uint32_t club_id = all_ones;
    if (first.club != 0) {
        keep(c, 26, 0);
        const auto second = contract_club(c, person, 0x1040f398);
        keep(c, 26, second.found);
        club_id = c.u32(second.club);  // read through when the second lookup finds none
    }
    item.field(5, club_id, 0x1040f3fc);
    keep(c, 28, std::uint32_t(std::int32_t(listed)));
    for (std::uint32_t i = 0; std::int8_t(i) < listed; ++i) {
        const auto entry_at = frame.at(listed_words) + i * 4;
        keep(c, 26, entry_at);
        keep(c, 25, i);
        item.post(c.u32(entry_at), 0x1040f41c);
    }
}

void post_offer_news_fbb(Cpu& c, std::uint32_t offer, std::uint32_t other_person, std::uint32_t value) {
    Frame frame(c, 0x1040f780, 0x140, {.first_gpr = 24});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), clubs_cell = c.toc(toc::clubs);
    const auto person = offer_person(c, offer);
    keep(c, 24, found_cell);
    keep(c, 25, clubs_cell);
    keep(c, 26, 0);
    keep(c, 27, offer);
    keep(c, 28, other_person);
    keep(c, 29, value);
    keep(c, 30, person);
    keep(c, 31, news_cell);
    const auto lookup = contract_club(c, person, 0x1040f7d0);
    keep(c, 26, lookup.found);
    keep(c, 24, lookup.club);
    const auto offer_club = club_at(c, c.u32(offer + 8));
    keep(c, 25, offer_club);
    if (std::int8_t(invoke(c, human_club, 0x1040f838, {c.u32(news_cell), offer_club})) == 0) {
        if (lookup.club == 0) return;
        if (std::int8_t(invoke(c, human_club, 0x1040f854, {c.u32(news_cell), lookup.club})) == 0) return;
    }
    NewsItem item(c, frame.at(0x38), 0xfbb, 0x1040f870);
    item.field(0, c.u32(lookup.club), 0x1040f880);  // no guard: a person without a club reads address 0
    item.field(1, c.u32(offer_club), 0x1040f890);
    item.field(2, c.u32(person), 0x1040f8a0);
    item.field(3, name_value(c, person, 0x28), 0x1040f8b4);
    item.field(4, name_value(c, person, 0x2c), 0x1040f8c8);
    item.field(5, name_value(c, person, 0x30), 0x1040f8dc);
    item.field(6, c.u32(other_person), 0x1040f8ec);
    item.field(7, name_value(c, other_person, 0x28), 0x1040f900);
    item.field(8, name_value(c, other_person, 0x2c), 0x1040f914);
    item.field(9, name_value(c, other_person, 0x30), 0x1040f928);
    item.field(10, value, 0x1040f938);
    item.field(11, c.u32(offer + 0x20), 0x1040f948);
    item.post(offer_club, 0x1040f958);
    if (lookup.club != 0) item.post(lookup.club, 0x1040f970);
}

void post_person_news_bcc(Cpu& c, std::uint32_t person, std::uint32_t record) {
    Frame frame(c, 0x10410500, 0x140, {.first_gpr = 26});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), contracts_cell = c.toc(toc::contracts),
               clubs_cell = c.toc(toc::clubs);
    keep(c, 26, person);
    keep(c, 27, record);
    keep(c, 28, news_cell);
    keep(c, 29, contracts_cell);
    keep(c, 30, found_cell);
    keep(c, 31, clubs_cell);
    NewsItem item(c, frame.at(0x38), 0xbcc, 0x10410538);
    item.field(0, c.u32(person), 0x10410548);
    item.field(1, name_value(c, person, 0x28), 0x1041055c);
    item.field(2, name_value(c, person, 0x2c), 0x10410570);
    item.field(3, name_value(c, person, 0x30), 0x10410584);
    item.field(4, c.u32(record + 4), 0x10410594);
    if (contract_club_any(c, person, 0x104105a0) != 0) item.post(contract_club_any(c, person, 0x104105d8), 0x10410608);
    if (contract_club_any(c, person, 0x10410614) != 0) {
        const auto club = contract_club_any(c, person, 0x1041064c);
        if (c.u32(record + 4) != c.u32(club))  // read through when the club is gone
            item.post(contract_club_any(c, person, 0x1041068c), 0x104106bc);
    }
}

void post_offer_news_fc0(Cpu& c, std::uint32_t other, std::uint32_t offer) {
    Frame frame(c, 0x104108c0, 0x140, {.first_gpr = 27});
    const auto found_cell = c.toc(toc::found_contract), clubs_cell = c.toc(toc::clubs);
    const auto person = offer_person(c, offer);
    keep(c, 27, found_cell);
    keep(c, 28, person);
    keep(c, 29, other);
    keep(c, 30, offer);
    keep(c, 31, clubs_cell);
    NewsItem item(c, frame.at(0x38), 0xfc0, 0x10410904);
    item.field(0, c.u32(person), 0x10410914);
    item.field(1, name_value(c, person, 0x28), 0x10410928);
    item.field(2, name_value(c, person, 0x2c), 0x1041093c);
    item.field(3, name_value(c, person, 0x30), 0x10410950);
    item.field(4, c.u32(other + 8), 0x10410960);
    keep(c, 28, 0);
    const auto lookup = contract_club(c, person, 0x10410974);
    keep(c, 28, lookup.found);
    item.field(5, c.u32(lookup.club), 0x104109d0);  // no guard: a person without a club reads address 0
    item.field(6, c.u32(offer + 0x28), 0x104109e0);
    item.field(7, c.u32(offer + 0x2c), 0x104109f0);
    item.field(8, c.u32(other + 0x28), 0x10410a00);
    item.field(9, c.u32(other + 0x2c), 0x10410a10);
    item.field(10, sign8(c, offer + 0x34), 0x10410a24);
    item.post(club_at(c, c.u32(offer + 8)), 0x10410a44);
}

void post_offer_news_fc3(Cpu& c, std::uint32_t offer) {
    Frame frame(c, 0x104113e0, 0x150, {.first_gpr = 20});
    const auto found_cell = c.toc(toc::found_contract), news_cell = c.toc(toc::news), contracts_cell = c.toc(toc::contracts),
               clubs_cell = c.toc(toc::clubs), table_cell = c.toc(-0x7b9c), finance_cell = c.toc(-0x7ab8);
    const auto person = offer_person(c, offer);
    keep(c, 21, offer);
    keep(c, 22, person);
    keep(c, 25, 0);
    keep(c, 26, table_cell);
    keep(c, 27, finance_cell);
    keep(c, 28, news_cell);
    keep(c, 29, contracts_cell);
    keep(c, 30, found_cell);
    keep(c, 31, clubs_cell);
    // Only players of a quality (the low byte of the halfword at +0x5c) of 23 or more.
    if (std::int8_t(c.u16(person + 0x5c)) < 0x17) return;
    NewsItem item(c, frame.at(0x38), 0xfc3, 0x10411444);
    constexpr std::uint32_t row_size = 100, entry_size = 8, entries = 5, entry_club = 0x14, entry_share = 0x18;
    const auto entry_at = [&](std::uint32_t i) { return c.u32(table_cell) + c.u32(person) * row_size + i * entry_size; };
    std::uint32_t sum = 0;
    for (std::uint32_t i = 0; std::int8_t(i) < std::int8_t(entries); ++i)
        if (c.u32(entry_at(i) + entry_club) != all_ones) {
            sum = sign16(sum + sign8(c, entry_at(i) + entry_share));
            keep(c, 25, sum);
        }
    // The offer's fee (+0x20) over twenty, truncated toward zero as the original's multiply-high does.
    const auto quotient = std::int32_t((std::int64_t(0x66666667) * std::int32_t(c.u32(offer + 0x20))) >> 32) >> 3;
    const auto per_twentieth = std::uint32_t(quotient) + (std::uint32_t(quotient) >> 31);
    keep(c, 24, per_twentieth);
    if (std::int16_t(sum) == 0) {
        sum = 1;
        keep(c, 25, sum);
    }
    item.field(0, c.u32(person), 0x104114cc);
    item.field(1, name_value(c, person, 0x28), 0x104114e0);
    item.field(2, name_value(c, person, 0x2c), 0x104114f4);
    item.field(3, name_value(c, person, 0x30), 0x10411508);
    item.field(4, c.u32(offer + 8), 0x10411518);
    keep(c, 23, 0);
    const auto first = contract_club(c, person, 0x10411528);
    keep(c, 23, first.found);
    std::uint32_t club_id = all_ones;
    if (first.club != 0) {
        keep(c, 23, 0);
        const auto second = contract_club(c, person, 0x1041158c);
        keep(c, 23, second.found);
        club_id = c.u32(second.club);  // read through when the second lookup finds none
    }
    item.field(7, club_id, 0x104115f0);
    keep(c, 30, 0);
    keep(c, 29, 0);
    for (std::uint32_t i = 0; std::int8_t(i) < std::int8_t(entries); ++i) {
        keep(c, 30, i);
        keep(c, 29, i * entry_size);
        const auto entry = entry_at(i);
        const auto club_index = c.u32(entry + entry_club);
        if (club_index == all_ones) continue;
        const auto share = std::int8_t(c.u8(entry + entry_share));
        const auto club = club_at(c, club_index);
        keep(c, 23, club);
        const std::uint32_t weight = share >= 1 ? std::uint32_t(std::int32_t(share)) : 1;
        // The divisor is the running share sum, never zero here (a zero sum was replaced by 1).
        const auto amount = std::uint32_t(std::int32_t(per_twentieth) / std::int32_t(std::int16_t(sum))) * weight;
        keep(c, 20, amount);
        const auto finance = invoke(c, 0x10103510, 0x10411660, {c.u32(finance_cell), club_at(c, c.u32(offer + 8))});
        invoke(c, 0x100f7050, 0x1041166c, {finance, std::uint32_t(std::int32_t(amount) >> 31), amount});
        if (std::int32_t(amount) > std::int32_t(c.i16(club + 0x1ec)) * 11 - 10000) {
            item.field(5, amount, 0x10411690);
            item.field(6, c.u32(club), 0x104116a0);
            item.post(club, 0x104116b0);
        }
    }
}
}

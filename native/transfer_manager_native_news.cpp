// TRANSFER_MANAGER.CPP news builders, native-first (0x103ffe80, 0x104019d0, 0x10402b90, 0x10406910, 0x10406e00).
#include "transfer_manager_native_news.hpp"
#include "guest_fpu.hpp"
#include "transfer_manager_native_news_common.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace news_common;
namespace {
constexpr std::uint32_t news_item_signing = 0xfa1, news_item_fa2 = 0xfa2, news_item_fa6 = 0xfa6, news_item_fa7 = 0xfa7,
                        news_item_fac = 0xfac;
constexpr std::uint32_t news_item_faf = 0xfaf, news_item_fa4 = 0xfa4, news_item_fa8 = 0xfa8;
constexpr std::uint32_t list_capacity = 0x1e;                 // the clubs a news item is sent to (0x103fb840's output)
constexpr std::uint32_t make_recipients = 0x103fb840, offer_text = 0x10416710, news_accepts = 0x102cdbf0, next_item = 0x10738e00;
constexpr std::uint32_t offer_person = 0xc, offer_from_club = 8, offer_to_club = 4, offer_kind = 0x34, offer_flag = 0x37, offer_state = 0x38, offer_type = 0x24;
constexpr std::uint32_t offer_list = 4, list_cursor = 0xc, list_head = 4, offer_value_a = 0x28, offer_value_b = 0x2c;
}

void post_signing_news(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t outcome) {
    Frame frame(c, 0x103ffe80, 0x1d0, {.first_gpr = 21});
    const auto news_cell = c.toc(toc::news), contracts = c.toc(toc::contracts), found = c.toc(toc::found_contract), clubs_cell = c.toc(toc::clubs);
    const auto list = frame.at(0x38), item_at = frame.at(0xb0);
    keep(c, 21, 0);
    keep(c, 22, offer);
    keep(c, 23, outcome);
    keep(c, 27, news_cell);
    keep(c, 28, contracts);
    keep(c, 29, found);
    keep(c, 30, clubs_cell);
    keep(c, 31, manager);
    const auto person = person_record(c, c.u32(offer + offer_person));
    const auto offer_club = club_record(c, c.u32(offer + offer_from_club));
    keep(c, 25, person);
    keep(c, 24, offer_club);
    // The player's own club, when the contract found is live.
    auto contract = lookup_contract(c, person, 0x103ffee0);
    const auto live = live_contract_club(c, contract);
    keep(c, 21, live_contract(c, contract));
    keep(c, 26, live);
    const auto kind = c.i8(offer + offer_kind);
    // Recipients: 0x2d for kind 2, 0x2e for any other.
    // The original also leaves the kind, sign-extended, in r8 (a register 0x103fb840 passes on unread).
    const auto recipients = invoke(c, make_recipients, 0x103fff5c, {manager, offer, kind == 2 ? 0x2du : 0x2eu, list, list_capacity, std::uint32_t(std::int32_t(kind))});
    keep(c, 31, recipients);
    const auto count = std::int8_t(recipients);
    if (count == 0) return;
    keep(c, 21, 0);
    contract = lookup_contract(c, person, 0x103fff78);
    auto club = live_contract_club(c, contract);
    keep(c, 21, live_contract(c, contract));
    keep(c, 28, club);
    contract = lookup_contract(c, person, 0x103fffd4);
    if (contract != 0 && contract_state_is(c, contract, 5)) club = 0;
    if (std::int16_t(outcome) == 6 && c.i8(offer + offer_flag) != 0) club = 0;
    keep(c, 28, club);

    const NewsItem item(c, item_at, news_item_signing, 0x1040003c);
    person_fields(c, item, person, {0x1040004c, 0x10400060, 0x10400074, 0x10400088});
    item.field(4, club ? c.u32(club) : none, 0x104000a8);
    item.field(5, c.u32(offer + offer_from_club), 0x104000b8);
    item.field(6, c.u32(offer + offer_to_club), 0x104000c8);
    item.field(7, sign8(c, offer + offer_kind), 0x104000dc);
    item.field(8, std::uint32_t(std::int32_t(std::int16_t(outcome))), 0x104000ec);
    item.field(9, sign8(c, offer + offer_type), 0x10400100);
    item.field(10, invoke(c, offer_text, 0x10400118, {offer, 0, 0, 0, 1}), 0x10400128);
    // Fields read through the club: its name's id, or none.
    if (club != 0 && c.u32(club + 0x30) != 0) item.field(11, c.u32(c.u32(club + 0x30)), 0x1040014c);
    else item.field(11, none, 0x10400160);
    keep(c, 23, std::uint32_t(std::int32_t(count)));
    for (std::uint32_t index = 0; std::int8_t(index) < count; ++index) {
        keep(c, 21, index);
        keep(c, 22, list + index * 4);
        const auto recipient = c.u32(list + index * 4);
        // The item says whether the recipient is the signing club or the player's own.
        item.field(12, recipient == offer_club || recipient == live ? 1 : 0, recipient == offer_club || recipient == live ? 0x10400194 : 0x104001a8);
        item.post(recipient, 0x104001b8);
    }
}

void post_news_fa2(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t other) {
    Frame frame(c, 0x104019d0, 0x1c0, {.first_gpr = 24});
    const auto news_cell = c.toc(toc::news), people_cell = c.toc(toc::people), found = c.toc(toc::found_contract);
    const auto list = frame.at(0x38), item_at = frame.at(0xb0);
    const auto person = person_record(c, c.u32(offer + offer_person));
    keep(c, 24, person);
    keep(c, 25, offer);
    keep(c, 26, other);
    keep(c, 27, 0);
    keep(c, 28, news_cell);
    keep(c, 29, people_cell);
    keep(c, 30, found);
    const auto recipients = invoke(c, make_recipients, 0x10401a1c, {manager, other, 1, list, list_capacity});
    keep(c, 31, recipients);
    const NewsItem item(c, item_at, news_item_fa2, 0x10401a30);
    person_fields(c, item, person, {0x10401a40, 0x10401a54, 0x10401a68, 0x10401a7c});
    item.field(4, c.u32(offer + offer_from_club), 0x10401a8c);
    keep(c, 24, 0);
    const auto contract = lookup_contract(c, person, 0x10401aa0);
    keep(c, 24, live_contract(c, contract));
    item.field(5, c.u32(live_contract_club(c, contract)), 0x10401b00);
    item.field(6, c.u32(other + offer_value_a), 0x10401b10);
    item.field(7, c.u32(other + offer_value_b), 0x10401b20);
    item.field(8, c.u32(offer + offer_value_a), 0x10401b30);
    item.field(9, c.u32(offer + offer_value_b), 0x10401b40);
    item.field(10, c.u32(offer + 0x20), 0x10401b50);
    item.field(11, sign8(c, other + offer_kind), 0x10401b64);
    item.field(12, invoke(c, offer_text, 0x10401b7c, {offer, 0, 1, 0, 1}), 0x10401b8c);
    // The people listed on the offer: at most three get fields, all are counted.
    std::uint32_t slot = 0xe, listed = 0;
    keep(c, 26, slot);
    if (c.u32(offer + offer_list) != 0) {
        c.put32(c.u32(offer + offer_list) + list_cursor, c.u32(c.u32(offer + offer_list) + list_head));
        while (const auto next = invoke(c, next_item, 0x10401c2c, {c.u32(offer + offer_list)})) {
            const auto member = person_record(c, c.u32(next));
            keep(c, 24, member);
            if (std::int8_t(listed) < 3) {
                keep(c, 26, slot + 1);
                item.field(slot++, c.u32(member), 0x10401bd8);
                keep(c, 26, slot + 1);
                item.field(slot++, c.u32(c.u32(member + person_name) + name_id), 0x10401bf0);
                keep(c, 26, slot + 1);
                item.field(slot++, c.u32(c.u32(member + person_club_name) + name_id), 0x10401c08);
                keep(c, 26, slot + 1);
                item.field(slot++, c.u32(c.u32(member + person_nation) + name_id), 0x10401c20);
            }
            keep(c, 27, ++listed);
        }
    }
    item.field(0xd, std::uint32_t(std::int32_t(std::int8_t(listed))), 0x10401c44);
    keep(c, 26, std::uint32_t(std::int32_t(std::int8_t(recipients))));
    for (std::uint32_t index = 0; std::int8_t(index) < std::int8_t(recipients); ++index) {
        keep(c, 25, list + index * 4);
        keep(c, 24, index);
        item.post(c.u32(list + index * 4), 0x10401c68);
    }
}

void post_news_fac(Cpu& c, std::uint32_t offer) {
    Frame frame(c, 0x10402b90, 0x220, {.first_gpr = 25});
    const auto news_cell = c.toc(toc::news), people_cell = c.toc(toc::people), found = c.toc(toc::found_contract);
    const auto person = person_record(c, c.u32(offer + offer_person));
    keep(c, 25, found);
    keep(c, 26, c.toc(toc::clubs));
    keep(c, 27, offer);
    keep(c, 28, person);
    keep(c, 29, 0);
    keep(c, 30, news_cell);
    keep(c, 31, people_cell);
    const auto club = live_contract_club(c, lookup_contract(c, person, 0x10402bd8));
    keep(c, 29, club);
    const auto offer_club = club_record(c, c.u32(offer + offer_from_club));
    keep(c, 25, offer_club);
    if (std::int8_t(invoke(c, news_accepts, 0x10402c40, {c.u32(news_cell), offer_club})) != 0) {
        const NewsItem item(c, frame.at(0x11c), news_item_fac, 0x10402c58);
        person_fields(c, item, person, {0x10402c68, 0x10402c7c, 0x10402c90, 0x10402ca4});
        item.field(4, club ? c.u32(club) : none, 0x10402cc4);
        item.field(5, c.u32(offer + offer_from_club), 0x10402cd4);
        item.field(7, c.u32(offer + offer_value_a), 0x10402ce4);
        item.field(8, c.u32(offer + offer_value_b), 0x10402cf4);
        item.field(9, 0, 0x10402d04);
        item.field(10, 0, 0x10402d14);
        item.post(offer_club, 0x10402d24);
    }
    if (club == 0 || std::int8_t(invoke(c, news_accepts, 0x10402d38, {c.u32(news_cell), club})) == 0) return;
    if (c.u32(offer + offer_list) == 0) return;
    c.put32(c.u32(offer + offer_list) + list_cursor, c.u32(c.u32(offer + offer_list) + list_head));
    // One item per person listed on the offer, to the player's club.
    while (const auto next = invoke(c, next_item, 0x10402e4c, {c.u32(offer + offer_list)})) {
        const auto member = person_record(c, c.u32(next));
        keep(c, 25, member);
        const NewsItem listed(c, frame.at(0x38), news_item_fac, 0x10402d78);
        person_fields(c, listed, member, {0x10402d88, 0x10402d9c, 0x10402db0, 0x10402dc4});
        listed.field(4, c.u32(offer + offer_from_club), 0x10402dd4);
        // Never none here: the loop is entered only with a club (0x10402d24 and 0x10402d28 leave when it is 0), so the
        // original's -1 arm at 0x10402dec is dead.
        listed.field(5, club ? c.u32(club) : none, 0x10402df4);
        listed.field(7, c.u32(offer + offer_value_a), 0x10402e04);
        listed.field(8, c.u32(offer + offer_value_b), 0x10402e14);
        listed.field(9, 0, 0x10402e24);
        listed.field(10, 1, 0x10402e34);
        listed.post(club, 0x10402e44);
    }
}

void post_news_fa7(Cpu& c, std::uint32_t offer, std::uint32_t word, std::uint32_t other, std::uint32_t flag) {
    Frame frame(c, 0x10406910, 0x150, {.first_gpr = 23});
    const auto contracts = c.toc(toc::contracts), found = c.toc(toc::found_contract), news_cell = c.toc(toc::news), clubs_cell = c.toc(toc::clubs);
    const auto person = person_record(c, c.u32(offer + offer_person));
    keep(c, 23, word);
    keep(c, 24, other);
    keep(c, 25, flag);
    keep(c, 26, person);
    keep(c, 27, contracts);
    keep(c, 28, found);
    keep(c, 29, offer);
    keep(c, 30, news_cell);
    keep(c, 31, clubs_cell);
    const NewsItem item(c, frame.at(0x38), news_item_fa7, 0x10406964);
    person_fields(c, item, person, {0x10406974, 0x10406988, 0x1040699c, 0x104069b0});
    item.field(4, sign8(c, offer), 0x104069c4);
    item.field(5, c.u32(offer + offer_from_club), 0x104069d4);
    item.field(6, c.u32(offer + offer_to_club), 0x104069e4);
    item.field(7, word, 0x104069f4);
    item.field(8, std::uint32_t(std::int32_t(std::int8_t(flag))), 0x10406a04);
    if (std::int8_t(flag) != 0) {
        item.field(9, c.u32(other + offer_from_club), 0x10406a1c);
    } else {
        // Without a contract at the player's club there is nothing to say.
        if (contract_club(c, lookup_contract(c, person, 0x10406a2c)) == 0) return;
        item.field(9, c.u32(contract_club(c, lookup_contract(c, person, 0x10406a64))), 0x10406a98);
    }
    item.post(club_record(c, c.u32(offer + offer_from_club)), 0x10406abc);
    item.post(club_record(c, c.u32(offer + offer_to_club)), 0x10406ad8);
}

void post_news_fa6(Cpu& c, std::uint32_t offer, std::uint32_t word, std::uint32_t flag, std::uint32_t done) {
    Frame frame(c, 0x10406e00, 0x140, {.first_gpr = 24});
    const auto contracts = c.toc(toc::contracts), found = c.toc(toc::found_contract), news_cell = c.toc(toc::news), clubs_cell = c.toc(toc::clubs);
    keep(c, 24, offer);
    keep(c, 25, word);
    keep(c, 26, flag);
    keep(c, 28, news_cell);
    keep(c, 29, contracts);
    keep(c, 30, found);
    keep(c, 31, clubs_cell);
    if (done != 0) return;
    const auto person = person_record(c, c.u32(offer));
    keep(c, 27, person);
    if (contract_club(c, lookup_contract(c, person, 0x10406e54)) == 0) return;
    const NewsItem item(c, frame.at(0x38), news_item_fa6, 0x10406e90);
    person_fields(c, item, person, {0x10406ea0, 0x10406eb4, 0x10406ec8, 0x10406edc});
    item.field(4, c.u32(offer + 4), 0x10406eec);
    item.field(5, word, 0x10406efc);
    item.field(6, c.u32(offer + 0xc), 0x10406f0c);
    item.field(7, std::uint32_t(std::int32_t(std::int8_t(flag))), 0x10406f1c);
    item.field(8, c.u32(contract_club(c, lookup_contract(c, person, 0x10406f28))), 0x10406f5c);
    item.post(club_record(c, c.u32(offer + 0xc)), 0x10406f78);
    item.post(contract_club(c, lookup_contract(c, person, 0x10406f84)), 0x10406fb4);
}

void post_news_faf(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t other, std::uint32_t word) {
    Frame frame(c, 0x10403890, 0x150, {.first_gpr = 23});
    const auto news_cell = c.toc(toc::news), found = c.toc(toc::found_contract), clubs_cell = c.toc(toc::clubs);
    const auto list = frame.at(0x38);
    const auto person = person_record(c, c.u32(offer + offer_person));
    keep(c, 23, found);
    keep(c, 24, 0);
    keep(c, 25, clubs_cell);
    keep(c, 26, manager);
    keep(c, 27, offer);
    keep(c, 28, other);
    keep(c, 29, word);
    keep(c, 30, person);
    keep(c, 31, news_cell);
    const auto club = live_contract_club(c, lookup_contract(c, person, 0x104038e4));
    const auto clubs = c.u32(clubs_cell);
    const auto offer_club_offset = c.u32(offer + offer_from_club) * club_size;
    keep(c, 23, club);
    keep(c, 24, clubs);
    keep(c, 25, offer_club_offset);
    const auto recipients = invoke(c, make_recipients, 0x10403954, {manager, offer, 3, list, 2});
    keep(c, 26, recipients);
    const auto count = std::int8_t(recipients);
    if (count == 0) return;
    const NewsItem item(c, frame.at(0x40), news_item_faf, 0x10403970);
    item.field(0, c.u32(club), 0x10403980);
    item.field(1, c.u32(clubs + offer_club_offset), 0x10403990);
    item.field(2, c.u32(person), 0x104039a0);
    item.field(3, c.u32(c.u32(person + person_name) + name_id), 0x104039b4);
    item.field(4, c.u32(c.u32(person + person_club_name) + name_id), 0x104039c8);
    item.field(5, c.u32(c.u32(person + person_nation) + name_id), 0x104039dc);
    identity_fields(c, item, other, 6, {0x104039ec, 0x10403a00, 0x10403a14, 0x10403a28});
    item.field(10, word, 0x10403a38);
    item.field(11, c.u32(offer + 0x20), 0x10403a48);
    keep(c, 27, std::uint32_t(std::int32_t(count)));
    keep(c, 26, list);
    keep(c, 23, 0);
    for (std::uint32_t index = 0; std::int8_t(index) < count; ++index) {
        keep(c, 26, list + index * 4);
        keep(c, 23, index);
        item.post(c.u32(list + index * 4), 0x10403a68);
    }
}

namespace {
constexpr std::uint32_t club_rating = 0x1ec, manager_record = 0x90, recipient_limit = 3;

// Four fields from `first` about the manager record at `holder`+0x90 (or four empty ones): its
// id and the identifiers of its three names. `found` and `missing` are the original's return addresses.
void manager_fields(Cpu& c, const NewsItem& item, std::uint32_t holder, bool holder_may_be_null, std::uint32_t first,
                    const std::array<std::uint32_t, 4>& found, const std::array<std::uint32_t, 4>& missing) {
    if ((holder != 0 || !holder_may_be_null) && c.u32(holder + manager_record) != 0) {
        item.field(first, c.u32(c.u32(holder + manager_record)), found[0]);
        item.field(first + 1, c.u32(c.u32(c.u32(holder + manager_record) + person_name) + name_id), found[1]);
        item.field(first + 2, c.u32(c.u32(c.u32(holder + manager_record) + person_club_name) + name_id), found[2]);
        item.field(first + 3, c.u32(c.u32(c.u32(holder + manager_record) + person_nation) + name_id), found[3]);
    } else {
        for (unsigned i = 0; i < 4; ++i) item.field(first + i, none, missing[i]);
    }
}
}

void post_news_fa4(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club, std::uint32_t other_club,
                   std::uint32_t outcome, std::uint32_t offer, std::uint32_t contract, std::uint32_t list) {
    Frame frame(c, 0x10403e20, 0x200, {.first_gpr = 17, .gpr_offset = -0x4c, .first_fpr = 31});
    guest::Fpu fpu(c);
    const auto news_cell = c.toc(toc::news), clubs_cell = c.toc(toc::clubs), scale_table = c.toc(-0x4ba0);
    const auto candidates = frame.at(0x3c), recipients_at = frame.at(0x48), item_at = frame.at(0xc0);
    // The ninth argument the caller left in its frame.
    const auto ninth = c.u32(frame.caller() + 0x38);
    const auto offer_person_record = person_record(c, c.u32(offer + offer_person));
    keep(c, 17, other_club);
    keep(c, 18, contract);
    keep(c, 19, ninth);
    keep(c, 20, offer_person_record);
    keep(c, 21, 0);
    keep(c, 22, 0x2f);
    keep(c, 23, scale_table);
    keep(c, 24, news_cell);
    keep(c, 25, clubs_cell);
    keep(c, 26, manager);
    keep(c, 27, person);
    keep(c, 28, club);
    keep(c, 29, outcome);
    keep(c, 30, offer);
    keep(c, 31, list);
    const auto kind = std::int8_t(outcome);
    std::uint32_t chosen = 0;
    // r8-r10 reach 0x103fb840 unread by it: the people table cell, the contract and the list, unless the
    // replacement scan below ran, which leaves its counter (3) and the lowest rating there.
    std::uint32_t hidden_r8 = c.toc(toc::people), hidden_r9 = contract;
    // The (up to three) clubs of the list with the highest rating at +0x1ec.
    if (kind == 2 && list != 0 && c.i16(list) != 0) {
        c.put32(list + list_cursor, c.u32(list + list_head));
        while (const auto next = invoke(c, next_item, 0x10403f64, {list})) {
            if (std::int8_t(chosen) < std::int32_t(recipient_limit)) {
                c.put32(candidates + std::uint32_t(std::int32_t(std::int8_t(chosen))) * 4, club_record(c, c.u32(next)));
                keep(c, 21, ++chosen);
            } else {
                std::int32_t lowest = -1, lowest_rating = 0x2711;
                for (std::uint32_t slot = 0; std::int8_t(slot) < std::int32_t(recipient_limit); ++slot) {
                    const std::int32_t rating = c.i16(c.u32(candidates + slot * 4) + club_rating);
                    if (rating < std::int32_t(std::int16_t(lowest_rating))) {
                        lowest = std::int32_t(slot);
                        lowest_rating = rating;
                    }
                }
                hidden_r8 = 3;
                hidden_r9 = std::uint32_t(lowest_rating);
                const auto challenger = club_record(c, c.u32(next));
                // Index -1 (every rating at the bound) writes the word before the slots.
                if (c.i16(challenger + club_rating) > std::int32_t(std::int16_t(lowest_rating)))
                    c.put32(candidates + (std::uint32_t(std::int32_t(std::int8_t(lowest))) << 2), challenger);
            }
        }
    }
    std::uint32_t recipients_kind = 0x2f;
    if (kind != 2 && contract != 0) recipients_kind = 0x2d;
    else if (kind == 3) recipients_kind = c.u32(club) == c.u32(offer + offer_from_club) ? 0x2d : 0x2e;
    keep(c, 22, recipients_kind);
    const auto result = invoke(c, make_recipients, 0x10403fc8, {manager, offer, recipients_kind, recipients_at, list_capacity, hidden_r8, hidden_r9, list});
    keep(c, 25, result);
    const auto count = std::int8_t(result);
    if (count == 0) return;
    const NewsItem item(c, item_at, news_item_fa4, 0x10403fe4);
    person_fields(c, item, person, {0x10403ff4, 0x10404008, 0x1040401c, 0x10404030});
    item.field(4, other_club ? c.u32(other_club) : none, 0x10404050);
    item.field(5, c.u32(club), 0x10404060);
    item.field(6, std::uint32_t(std::int32_t(kind)), 0x10404070);
    item.field(7, c.u32(offer + offer_value_a), 0x10404080);
    item.field(8, c.u32(offer + offer_value_b), 0x10404090);
    item.field(9, sign8(c, offer + offer_type), 0x104040a4);
    item.field(10, ninth, 0x104040b4);
    item.field(11, c.u32(offer + 0x20), 0x104040c4);
    identity_fields(c, item, offer_person_record, 12, {0x104040d4, 0x104040e8, 0x104040fc, 0x10404110});
    if (contract != 0) {
        item.field(0x10, c.u32(contract + 4), 0x10404128);
        item.field(0x11, std::uint32_t(std::int32_t(c.i8(contract + contract_state)) >> 2), 0x10404140);
    } else {
        item.field(0x10, none, 0x10404154);
        item.field(0x11, 0, 0x10404164);
    }
    manager_fields(c, item, other_club, true, 0x12, {0x10404188, 0x104041a0, 0x104041b8, 0x104041d0}, {0x104041e4, 0x104041f4, 0x10404204, 0x10404214});
    manager_fields(c, item, club, false, 0x16, {0x10404230, 0x10404248, 0x10404260, 0x10404278}, {0x1040428c, 0x1040429c, 0x104042ac, 0x104042bc});

    // The attraction the shortlist gives the player for this club, scaled; asked from the contract at hand or the one found.
    const auto attraction = [&](std::uint32_t with, std::uint32_t back_call, std::uint32_t back_first, std::uint32_t back_second) {
        const auto role = c.u8(with + 0x60) & 0xfu;
        invoke(c, 0x10369080, back_call, {c.u32(manager + 0x988), person, club, 0, c.i8(offer + offer_type) == 1 ? 1u : 0u, role, frame.at(0x38)});
        c.set_f(31, fpu.frsp(c.f(1)));
        item.field(0x1a, c.u32(frame.at(0x38)), back_first);
        const auto scaled = fpu.fmul(c.f64(scale_table + 0x1a0), c.f(31));
        const auto bits = fpu.fctiwz(0, scaled);
        // The original stores the converted double in the frame (inside the item's buffer) and reads its low word.
        c.put64(frame.at(0x1a4), bits);
        item.field(0x1b, std::uint32_t(bits), back_second);
    };
    if (contract != 0) {
        attraction(contract, 0x104042fc, 0x10404310, 0x10404330);
    } else {
        const auto found_contract_record = invoke(c, 0x1007f720, 0x10404348, {c.u32(c.toc(toc::contracts)), c.u32(person), c.u32(club)});
        if (found_contract_record != 0) {
            attraction(found_contract_record, 0x10404388, 0x1040439c, 0x104043bc);
        } else {
            item.field(0x1a, 0, 0x104043d0);
            item.field(0x1b, 0, 0x104043e0);
        }
    }
    item.field(0x1c, list != 0 ? std::uint32_t(std::int32_t(c.i16(list))) : 0, 0x10404400);

    keep(c, 18, std::uint32_t(std::int32_t(std::int8_t(chosen))));
    for (std::uint32_t index = 0; std::int8_t(index) < std::int8_t(chosen); ++index) {
        keep(c, 17, candidates + index * 4);
        keep(c, 19, index);
        item.field(std::uint32_t(std::int32_t(std::int8_t(index + 0x1d))), c.u32(c.u32(candidates + index * 4)), 0x10404428);
    }
    keep(c, 19, std::uint32_t(std::int32_t(kind)));
    keep(c, 18, std::uint32_t(std::int32_t(count)));
    for (std::uint32_t index = 0; std::int8_t(index) < count; ++index) {
        keep(c, 17, recipients_at + index * 4);
        keep(c, 20, index);
        const auto recipient = c.u32(recipients_at + index * 4);
        item.post(recipient, 0x10404460);
        if (kind == 2) {
            // A club that gets the item as a recipient is dropped from the candidates, which get it last.
            for (std::uint32_t slot = 0; std::int8_t(slot) < std::int8_t(chosen); ++slot) {
                if (recipient == c.u32(candidates + slot * 4)) {
                    c.put32(candidates + slot * 4, 0);
                    break;
                }
            }
        }
    }
    // The counter of the loop above stays where it ended.
    keep(c, 20, count > 0 ? std::uint32_t(count) : 0);
    keep(c, 18, std::uint32_t(std::int32_t(std::int8_t(chosen))));
    for (std::uint32_t index = 0; std::int8_t(index) < std::int8_t(chosen); ++index) {
        keep(c, 17, candidates + index * 4);
        keep(c, 19, index);
        if (const auto candidate = c.u32(candidates + index * 4); candidate != 0) item.post(candidate, 0x10404500);
    }
}

void post_news_fa8(Cpu& c, std::uint32_t manager, std::uint32_t offer) {
    Frame frame(c, 0x10407460, 0x1e0, {.first_gpr = 19});
    const auto people_cell = c.toc(toc::people), salt_cell = c.toc(-0x7b90), cursor_cell = c.toc(-0x7b84), news_cell = c.toc(toc::news),
               contracts = c.toc(toc::contracts), found = c.toc(toc::found_contract), clubs_cell = c.toc(toc::clubs);
    const auto list = frame.at(0x40), item_at = frame.at(0xb8);
    const auto person = person_record(c, c.u32(offer + offer_person));
    keep(c, 20, person);
    keep(c, 21, 0);
    keep(c, 22, salt_cell);
    keep(c, 23, cursor_cell);
    keep(c, 24, news_cell);
    keep(c, 25, people_cell);
    keep(c, 26, contracts);
    keep(c, 27, found);
    keep(c, 28, clubs_cell);
    keep(c, 31, offer);
    const auto recipients = invoke(c, make_recipients, 0x104074b4, {manager, offer, 0x5f, list, list_capacity, c.u32(people_cell)});
    keep(c, 29, recipients);
    const auto count = std::int8_t(recipients);
    if (count == 0) return;
    const NewsItem item(c, item_at, news_item_fa8, 0x104074d0);
    person_fields(c, item, person, {0x104074e0, 0x104074f4, 0x10407508, 0x1040751c});
    item.field(4, c.u32(offer + offer_from_club), 0x1040752c);

    // The player's club when he has a live contract; the flag (r30) is what the original keeps.
    const auto live_club = [&](std::uint32_t back) {
        keep(c, 30, 0);
        const auto contract = lookup_contract(c, person, back);
        keep(c, 30, live_contract(c, contract));
        return live_contract_club(c, contract);
    };
    if (live_club(0x1040753c) != 0) item.field(5, c.u32(live_club(0x104075a0)), 0x10407604);
    else item.field(5, none, 0x10407604);
    item.field(6, c.u32(offer + 0x20), 0x10407614);
    item.field(7, 0, 0x10407624);
    item.field(8, invoke(c, offer_text, 0x1040763c, {offer, 1, 1, 0, 1}), 0x1040764c);
    item.field(9, sign8(c, offer + offer_flag), 0x10407660);
    item.field(0xc, sign8(c, offer + 0x11), 0x10407674);
    item.field(0xd, invoke(c, 0x10416d30, 0x1040767c, {offer}), 0x1040768c);
    if (c.i8(offer + offer_flag) != 0 && live_club(0x104076a8) != 0 && (c.i8(offer + offer_state) >> 4) == 3) {
        const auto contract = lookup_contract(c, person, 0x10407720);
        keep(c, 30, contract);
        if (contract == 0) {
            item.field(10, 0, 0x104077e0);
            item.field(11, 0, 0x104077f0);
        } else {
            // True when the contract's two halfwords are ahead of the date's (low half, then high half).
            const auto date = c.u32(c.toc(-0x7b80));
            const std::uint32_t date_low = date & 0xffff, until = c.u16(contract + 0x32);
            bool ahead;
            if (date_low < until) ahead = true;
            else if (date_low > until) ahead = false;
            else {
                const std::int32_t high = std::int16_t(date >> 16), start = c.i16(contract + 0x30);
                const std::int32_t mixed = start ^ high;
                ahead = ((std::uint32_t(mixed >> 1) - std::uint32_t(mixed & start)) >> 31) != 0;
            }
            if (ahead) {
                item.field(10, std::uint32_t(std::int32_t(c.i16(contract + 0x30))), 0x10407798);
                item.field(11, c.u16(contract + 0x32), 0x104077a8);
            } else {
                item.field(10, 0, 0x104077bc);
                item.field(11, 0, 0x104077cc);
            }
        }
    }
    // The people listed on the offer: at most three get four fields each, all are counted.
    std::uint32_t slot = 0xf, listed = 0;
    keep(c, 30, slot);
    if (c.u32(offer + offer_list) != 0) {
        c.put32(c.u32(offer + offer_list) + list_cursor, c.u32(c.u32(offer + offer_list) + list_head));
        while (const auto next = invoke(c, next_item, 0x10407890, {c.u32(offer + offer_list)})) {
            const auto member = person_record(c, c.u32(next));
            keep(c, 19, member);
            if (std::int8_t(listed) < 3) {
                keep(c, 30, slot + 1);
                item.field(slot++, c.u32(member), 0x1040783c);
                keep(c, 30, slot + 1);
                item.field(slot++, c.u32(c.u32(member + person_name) + name_id), 0x10407854);
                keep(c, 30, slot + 1);
                item.field(slot++, c.u32(c.u32(member + person_club_name) + name_id), 0x1040786c);
                keep(c, 30, slot + 1);
                item.field(slot++, c.u32(c.u32(member + person_nation) + name_id), 0x10407884);
            }
            keep(c, 21, ++listed);
        }
    }
    item.field(0xe, std::uint32_t(std::int32_t(std::int8_t(listed))), 0x104078a8);
    // A draw from the news table: the cursor moves two entries, wrapping to a fresh salt.
    const auto cursor = c.toc(-0x7b84), salt = c.toc(-0x7b90);
    const auto limit = c.u32(c.toc(-0x7b88));
    const auto moved = c.u32(cursor) + 2;
    c.put32(cursor, moved);
    if (moved > limit) {
        c.put32(cursor, c.toc(-0x7b8c));
        c.put32(salt, invoke(c, 0x1077b770, 0x104078d0) & 0xffff);
    }
    const std::int32_t total = std::int32_t(c.u16(c.u32(cursor)) + c.u32(salt));
    const auto high = std::int32_t((std::int64_t(0x55555556) * total) >> 32);
    const auto quotient = high + std::int32_t(std::uint32_t(high) >> 31);
    keep(c, 30, slot + 1);
    item.field(slot++, std::uint32_t(total) - std::uint32_t(quotient) * 3, 0x10407914);
    if (c.u32(person + 0x14) == 0) {
        item.field(slot, 0, 0x104079d8);
    } else {
        keep(c, 30, slot + 1);
        item.field(slot, 1, 0x10407934);
        keep(c, 21, 0);
        const auto contract = lookup_contract(c, person, 0x10407944);
        keep(c, 21, live_contract(c, contract));
        const auto reference = invoke(c, 0x100d71a0, 0x10407998, {person, live_contract_club(c, contract)});
        keep(c, 30, slot + 2);
        item.field(slot + 1, reference, 0x104079ac);
        const auto area = invoke(c, 0x10117490, 0x104079b4, {c.u32(person + 0x14)});
        item.field(slot + 2, std::uint32_t(std::int32_t(std::int16_t(area))), 0x104079c4);
    }
    if (std::int8_t(invoke(c, news_accepts, 0x104079f0, {c.u32(news_cell), club_record(c, c.u32(offer + offer_from_club))})) != 0) {
        invoke(c, 0x102cc8e0, 0x10407a04, {c.u32(news_cell), item_at});
        return;
    }
    keep(c, 21, std::uint32_t(std::int32_t(count)));
    keep(c, 22, 0);
    for (std::uint32_t index = 0; std::int8_t(index) < count; ++index) {
        keep(c, 20, list + index * 4);
        keep(c, 22, index);
        item.post(c.u32(list + index * 4), 0x10407a28);
    }
}
}

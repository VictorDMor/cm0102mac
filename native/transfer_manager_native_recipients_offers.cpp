#include "transfer_manager_native_recipients.hpp"
#include "transfer_manager_native_recipients_held.hpp"
#include "transfer_manager_native_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t find_contract = 0x10082f20;  // (contract manager, person) -> contract or 0
constexpr std::uint32_t find_club_contract = 0x1007f720, set_club_contract = 0x1007f840, set_person_contract = 0x1007f930;
constexpr std::uint32_t make_item = 0x102bdf90, set_item_field = 0x102cb4b0, post_item = 0x102cc140, club_takes_news = 0x102cdbf0;
constexpr std::uint32_t iterator_begin = 0x10738c00, iterator_end = 0x10738c20, iterator_next = 0x10738e00, iterator_reset = 0x10738ca0;
constexpr std::uint32_t offer_person = 0xc, offer_club = 8, offer_status = 0x34, offer_list = 4, offer_amount = 0x20;
constexpr std::uint32_t person_name_words = 0x28;
constexpr std::uint32_t news_signed = 0xfb1;

bool contract_kind_is_5(const Cpu& c, std::uint32_t contract) {
    return std::int8_t(std::int32_t(std::uint32_t(c.u8(contract + 0x38)) << 24 & 0xfe000000u) >> 26) == 5;
}
}

void close_offers_for_person(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    constexpr std::uint32_t iterator_at = 0x38, collected_at = 0x344, recipients_at = 0x48;
    constexpr std::uint32_t item_listed_at = 0x260, item_rival_at = 0x17c, item_group_at = 0x98;
    Frame frame(c, 0x1040b470, 0x420, {.first_gpr = 20});
    RecipientsHeld r(c);
    r[27] = c.toc(toc::news);
    r[21] = manager;
    r[31] = c.toc(toc::people);
    r[22] = person;
    r[28] = c.toc(toc::contracts);
    r[29] = c.toc(toc::found_contract);
    r[30] = c.toc(toc::clubs);
    const auto call = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        r.publish(c);
        return invoke(c, entry, back, arguments);
    };
    const auto people = [&] { return c.u32(r[31]); };
    const auto clubs = [&] { return c.u32(r[30]); };
    const auto contracts = [&] { return c.u32(r[28]); };
    const auto offer_at = [&](std::uint32_t entry) { return c.u32(c.u32(manager) + c.u32(entry) * 4); };
    const auto lookup = [&](std::uint32_t who, std::uint32_t back) {
        const auto contract = call(find_contract, back, {contracts(), who});
        c.put32(r[29], contract);
        return contract;
    };
    // The club of the contract found for `who` (0 without one).
    const auto plain_club = [&](std::uint32_t who, std::uint32_t back) {
        const auto contract = lookup(who, back);
        return contract ? clubs() + c.u32(contract + 4) * club_size : 0;
    };
    // The same, 0 too for a contract of kind 5; the original's flag register `flag` is cleared before the call.
    const auto member_club = [&](std::uint32_t who, unsigned flag, std::uint32_t back) {
        r[flag] = 0;
        const auto contract = lookup(who, back);
        if (contract && !contract_kind_is_5(c, contract)) r[flag] = 1;
        return r[flag] ? clubs() + c.u32(contract + 4) * club_size : 0;
    };
    const auto start_item = [&](std::uint32_t item, std::uint32_t back) { call(make_item, back, {frame.at(item), news_signed, 0}); };
    const auto field = [&](std::uint32_t item, std::uint32_t slot, std::uint32_t value, std::uint32_t back) {
        call(set_item_field, back, {frame.at(item), slot, value});
    };
    const auto post = [&](std::uint32_t item, std::uint32_t club, std::uint32_t back) {
        call(post_item, back, {c.u32(c.toc(toc::news)), frame.at(item), club});
    };
    const auto name_fields = [&](std::uint32_t item, const std::uint32_t (&back)[4]) {
        field(item, 0, c.u32(person), back[0]);
        for (unsigned n = 0; n < 3; ++n) field(item, n + 1, c.u32(c.u32(person + person_name_words + 4 * n) + 0x20), back[n + 1]);
    };
    const auto collect = [&](std::uint32_t club) {
        c.put32(frame.at(collected_at) + (std::uint32_t(std::int32_t(std::int8_t(r[24]))) << 2), club);
        ++r[24];
    };
    const auto offer_club_at = [&](std::uint32_t offer) { return clubs() + c.u32(offer + offer_club) * club_size; };

    call(iterator_begin, 0x1040b4a4, {frame.at(iterator_at)});
    r[24] = 0;
    // Offers for the person that are being closed: each gets status 0x10 and its clubs' contract rows updated.
    if (call(0x103eaea0, 0x1040b4bc, {manager, person, frame.at(iterator_at), 0}) != 0) {
        c.put32(frame.at(0x44), c.u32(frame.at(0x3c)));
        for (;;) {
            const auto entry = call(iterator_next, 0x1040b7ec, {frame.at(iterator_at)});
            if (!entry) break;
            const auto offer = offer_at(entry);
            r[25] = offer;
            if (c.u8(offer + offer_status) == 0x13) call(0x103f3540, 0x1040b4f8, {manager, offer});
            c.put8(offer + offer_status, 0x10);
            call(0x103eb4a0, 0x1040b514, {manager, offer, 10, 0});
            r[23] = plain_club(people() + c.u32(offer + offer_person) * person_size, 0x1040b52c);
            if (const auto row = call(find_club_contract, 0x1040b560, {contracts(), c.u32(offer + offer_person), c.u32(offer + offer_club)}))
                c.put8(row + 0x64, 4);
            if (const auto list = c.u32(offer + offer_list)) {
                c.put32(list + 0xc, c.u32(list + 4));
                r[20] = 4;
                for (;;) {
                    const auto listed = call(iterator_next, 0x1040b5b0, {c.u32(offer + offer_list)});
                    if (!listed) break;
                    if (const auto row = call(find_club_contract, 0x1040b59c, {contracts(), c.u32(listed), c.u32(r[23])}))
                        c.put8(row + 0x64, r[20]);
                }
            }
            call(set_club_contract, 0x1040b5e0,
                 {contracts(), people() + c.u32(offer + offer_person) * person_size, clubs() + c.u32(offer + offer_club) * club_size});
            r[23] = 0;
            c.put8(offer + 0x10, 0);
            if (const auto list = c.u32(offer + offer_list)) {
                c.put32(list + 0xc, c.u32(list + 4));
                for (;;) {
                    const auto listed = call(iterator_next, 0x1040b628, {c.u32(offer + offer_list)});
                    r[20] = listed;
                    if (!listed) break;
                    call(set_person_contract, 0x1040b61c, {contracts(), people() + c.u32(listed) * person_size, c.u32(offer + 0x28)});
                    c.put8(listed + 4, r[23]);
                }
            }
            r.publish(c);
            if (std::int8_t(call(club_takes_news, 0x1040b648, {c.u32(c.toc(toc::news)), offer_club_at(offer)})) == 0) continue;
            start_item(item_listed_at, 0x1040b660);
            name_fields(item_listed_at, {0x1040b670, 0x1040b684, 0x1040b698, 0x1040b6ac});
            field(item_listed_at, 4, c.u32(offer + offer_club), 0x1040b6bc);
            std::uint32_t about = 0;
            if (member_club(person, 20, 0x1040b6cc)) {
                const auto club = member_club(person, 20, 0x1040b730);
                about = c.u32(club);
            }
            field(item_listed_at, 5, about, 0x1040b794);
            field(item_listed_at, 0xc, 0, 0x1040b7a4);
            post(item_listed_at, offer_club_at(offer), 0x1040b7c0);
            collect(offer_club_at(offer));
        }
    }
    call(iterator_reset, 0x1040b7fc, {frame.at(iterator_at)});
    // Offers the person's other listings made: told to the club that offered, then closed by 0x104170f0.
    if (call(0x103eb020, 0x1040b80c, {manager, person, frame.at(iterator_at)}) != 0) {
        c.put32(frame.at(0x44), c.u32(frame.at(0x3c)));
        for (;;) {
            const auto entry = call(iterator_next, 0x1040ba50, {frame.at(iterator_at)});
            if (!entry) break;
            r[20] = 0;
            const auto offer = offer_at(entry);
            r[25] = offer;
            r[26] = member_club(people() + c.u32(offer + offer_person) * person_size, 20, 0x1040b84c);
            if (std::int8_t(call(club_takes_news, 0x1040b8a4, {c.u32(c.toc(toc::news)), r[26]})) != 0) {
                r[23] = people() + c.u32(offer + offer_person) * person_size;
                start_item(item_rival_at, 0x1040b8cc);
                name_fields(item_rival_at, {0x1040b8dc, 0x1040b8f0, 0x1040b904, 0x1040b918});
                field(item_rival_at, 4, c.u32(r[26]), 0x1040b928);
                const auto club = member_club(person, 20, 0x1040b938);
                field(item_rival_at, 5, c.u32(club), 0x1040b994);
                field(item_rival_at, 6, c.u32(offer + offer_amount), 0x1040b9a4);
                field(item_rival_at, 7, c.u32(offer + offer_amount) + c.u32(person + 8), 0x1040b9bc);
                field(item_rival_at, 8, c.u32(r[23]), 0x1040b9cc);
                for (unsigned n = 0; n < 3; ++n)
                    field(item_rival_at, 9 + n, c.u32(c.u32(r[23] + person_name_words + 4 * n) + 0x20),
                          std::uint32_t(0x1040b9e0 + 0x14 * n));
                field(item_rival_at, 0xc, 1, 0x1040ba18);
                post(item_rival_at, r[26], 0x1040ba28);
                collect(r[26]);
            }
            call(0x104170f0, 0x1040ba48, {offer, person});
        }
    }
    const auto returned = call(0x103fc710, 0x1040ba70, {manager, person, 10, frame.at(recipients_at), 0x14});
    r[31] = returned;
    if (std::int8_t(returned) != 0) {
        start_item(item_group_at, 0x1040ba8c);
        name_fields(item_group_at, {0x1040ba9c, 0x1040bab0, 0x1040bac4, 0x1040bad8});
        const auto club = member_club(person, 21, 0x1040bae8);
        field(item_group_at, 5, c.u32(club), 0x1040bb44);
        field(item_group_at, 0xc, 2, 0x1040bb54);
        r[23] = std::uint32_t(std::int32_t(std::int8_t(r[24])));
        r[22] = std::uint32_t(std::int32_t(std::int8_t(returned)));
        r[21] = frame.at(recipients_at);
        r[24] = 0;
        r[20] = 0;
        for (; std::int8_t(r[24]) < std::int8_t(r[22]); ++r[24], r[20] += 4, r[21] += 4) {
            bool seen = false;
            for (std::int32_t i = 0; std::int8_t(i) < std::int32_t(r[23]); ++i)
                if (c.u32(r[21]) == c.u32(frame.at(collected_at) + 4 * std::uint32_t(i))) {
                    seen = true;
                    break;
                }
            if (!seen) post(item_group_at, c.u32(frame.at(recipients_at) + r[20]), 0x1040bbcc);
        }
    }
    call(iterator_end, 0x1040bbf0, {frame.at(iterator_at), 0xffffffff});
}
}

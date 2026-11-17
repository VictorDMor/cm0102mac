// Cases for transfer_manager_native_text_b*.cpp: the texts of transfer news messages 4001, 4002 and 4003.
#include "transfer_manager_native_checks.hpp"
#include <array>
#include <map>

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t toc_base = 0x108bba90;
constexpr std::uint32_t item_at = scratch_base + 0x4000, output_at = scratch_base + 0x6000, scratch_session = scratch_base + 0x8000;

// A news item in the scratch area: the payload words at +8 + 4 * slot.
std::vector<Write> item_words(const std::map<unsigned, std::uint32_t>& words, std::uint32_t at = item_at) {
    std::vector<Write> writes;
    for (const auto& [slot, value] : words) writes.push_back({4, at + 8 + slot * 4, value});
    return writes;
}
void add(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }
// The three name ids a person's record links (+0x28, +0x2c, +0x30; their word +0x20); 0 for an absent link.
std::array<std::uint32_t, 3> name_ids(const Scenario& scenario, std::uint32_t person) {
    std::array<std::uint32_t, 3> ids{};
    for (unsigned i = 0; i < 3; ++i)
        if (const auto link = scenario.r32(person + 0x28 + i * 4)) ids[i] = scenario.r32(link + 0x20);
    return ids;
}

// Real offers of the manager (an array of pointers at +0, its size at +4), spread over the array.
std::vector<std::uint32_t> real_offers(const Scenario& scenario, unsigned wanted) {
    std::vector<std::uint32_t> offers;
    const auto manager = scenario.transfer_manager();
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size == 0 || size > 100000) return offers;
    const auto step = size / wanted + 1;
    for (std::uint32_t i = 0; i < size; i += step)
        if (const auto offer = scenario.r32(array + i * 4)) offers.push_back(offer);
    return offers;
}
std::string label(const char* what, unsigned a, unsigned b = 0, unsigned c = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
Case make_case(std::uint32_t entry, std::uint32_t detail, std::vector<Write> memory, std::string text) {
    return {.entry = entry, .registers = {{2, toc_base}, {3, 0}, {4, item_at}, {5, detail}, {6, output_at}}, .memory = std::move(memory),
            .label = std::move(text)};
}
// The session's current slot record (+4) and its word at +0x24 pointing at a club id, built in the scratch area.
std::vector<Write> pending_club(const Scenario& scenario, std::int32_t club_id, bool present) {
    const auto session = scenario.toc(-0x7bb0);
    const auto slot = session + std::uint32_t(std::int32_t(std::int16_t(scenario.r32(session + 0x2c58) >> 16)) * 0x2c0);
    if (!present) return {{4, slot + 4, 0}};
    return {{4, slot + 4, scratch_session}, {4, scratch_session + 0x24, scratch_session + 0x100}, {4, scratch_session + 0x100, std::uint32_t(club_id)}};
}
}

TRANSFER_MANAGER_NATIVE_CASES("text_b") {
    if (scenario.match) return;
    const auto people = scenario.people();
    // 0x104026b0 (4003): fields 0 person, 1-3 name ids, 4 the club left (-1: none), 5 the club joined, 6 a flag, 9 a marker.
    unsigned n = 0;
    for (unsigned index = 3; index < people; index += people / 14 + 1, ++n) {
        const auto person = scenario.person(index);
        const auto ids = name_ids(scenario, person);
        for (const std::int32_t from : {-1, 4 + int(n), 9})
            for (const std::uint32_t marker : {0u, 1u, 2u})
                for (const std::uint32_t flag : {0u, 1u})
                    for (unsigned variant = 0; variant < 3; ++variant) {
                        const std::int32_t to = (variant == 2) ? from : 11 + int(n);
                        std::map<unsigned, std::uint32_t> words{{0, index}, {1, ids[0] + (variant == 1)}, {2, ids[1]}, {3, ids[2]},
                                                                {4, std::uint32_t(from)}, {5, std::uint32_t(to)}, {6, flag * (n % 3 ? 1u : 0x80000000u)}, {9, marker}};
                        for (const unsigned detail : {0u, 1u}) {
                            auto memory = item_words(words);
                            if (detail) add(memory, pending_club(scenario, variant == 0 ? to : to + 1, (variant + n) % 4 != 3));
                            cases.push_back(make_case(0x104026b0, detail, memory, label("4003", index, detail, from + marker * 3 + flag * 7 + variant)));
                        }
                    }
    }

    // 0x10401c90 (4002): 0 person, 1-3 name ids, 4 club left, 5 club joined, 6/7 the offer (index, id), 0xa wage, 0xb kind,
    // 0xc fee, 0xd count of extra people (4 slots each from 0xe: person and three name ids); the item's word +0xdc.
    {
        const auto offers = real_offers(scenario, 12);
        const auto manager = scenario.transfer_manager();
        const auto offer_count = scenario.r32(manager + 4);
        const auto array = scenario.r32(manager);
        unsigned k = 0;
        for (const auto offer : offers) {
            std::uint32_t index = 0;
            for (std::uint32_t i = 0; i < offer_count; ++i)
                if (scenario.r32(array + i * 4) == offer) { index = i; break; }
            const auto offer_person = scenario.r32(offer + 0xc), offer_club = scenario.r32(offer + 8), offer_id = scenario.r32(offer + 0x2c);
            if (offer_person >= people) continue;
            const auto person = scenario.person(offer_person);
            const auto ids = name_ids(scenario, person);
            for (unsigned variant = 0; variant < 7; ++variant)
                for (const std::uint32_t state : {0u, 1u, 3u, 4u, 0x82u}) {
                    if (variant >= 4 && state != 1u && state != 3u) continue;
                    const std::uint32_t kind = variant == 6 ? 2u : (variant == 5 ? 0x101u : 1u);
                    const std::uint32_t extras_count = (k + variant + state) % 6;
                    std::map<unsigned, std::uint32_t> words{{0, variant == 2 ? offer_person + 1 : offer_person}, {1, ids[0]}, {2, ids[1]}, {3, ids[2] + (variant == 3)},
                                                            {4, variant == 1 ? offer_club + 1 : offer_club}, {5, 7 + k}, {6, variant == 4 ? offer_count + 5 : index},
                                                            {7, variant == 0 ? offer_id : offer_id + 1}, {0xa, (k % 4 == 1) ? 0 : 100 + k * 50}, {0xb, kind},
                                                            {0xc, (k % 3 == 1) ? 90u : (k % 3 == 2 ? 0u : 4000u + k)}, {0xd, extras_count == 5 ? 0xffu : extras_count}};
                    for (unsigned extra = 0; extra < 3; ++extra) {
                        const auto who = (offer_person + 5 * (extra + 1)) % people;
                        const auto who_ids = name_ids(scenario, scenario.person(who));
                        words[0xe + extra * 4] = who;
                        words[0xf + extra * 4] = who_ids[0];
                        words[0x10 + extra * 4] = who_ids[1];
                        words[0x11 + extra * 4] = who_ids[2] + (extra == 1 && k % 2);
                    }
                    auto memory = item_words(words);
                    memory.push_back({4, item_at + 0xdc, 0x5151 + k});
                    memory.push_back({1, offer + 0x34, state});
                    if (variant == 0 && state == 1u) add(memory, pending_club(scenario, 0, false));
                    if (variant == 3 && state == 1u) memory.push_back({4, array + index * 4, 0});  // no offer at the index
                    for (const unsigned detail : {0u, 1u, 0x80u})
                        cases.push_back(make_case(0x10401c90, detail, memory, label("4002", offer_person, detail, variant * 8 + state + k * 64)));
                }
            ++k;
        }
        // The amounts: wage and fee in every order, with and without extra people.
        unsigned pair = 0;
        for (const auto offer : offers) {
            const auto offer_person = scenario.r32(offer + 0xc);
            if (offer_person >= people || pair >= 6) continue;
            const auto ids = name_ids(scenario, scenario.person(offer_person));
            for (const auto& [wage, fee] : {std::pair{0u, 0u}, {0u, 90u}, {100u, 0u}, {500u, 90u}, {0u, 5000u}, {0xfffffffdu, 0xfffffffcu}})
                for (const std::uint32_t extras : {0u, 1u, 7u}) {
                    auto memory = item_words({{0, offer_person}, {1, ids[0]}, {2, ids[1]}, {3, ids[2]}, {4, 9}, {5, 12 + pair}, {0xa, wage}, {0xb, 3},
                                              {0xc, fee}, {0xd, extras}, {0xe, 5}, {0xf, ids[0]}, {0x10, ids[1]}, {0x11, ids[2]}});
                    memory.push_back({4, item_at + 0xdc, 0x4141});
                    cases.push_back(make_case(0x10401c90, 1, memory, label("4002 amounts", offer_person, wage, fee * 8 + extras)));
                }
            ++pair;
        }
    }

    // 0x104001e0 (4001): 0 person, 1-3 name ids, 4 club left (-1: none), 5 club joined, 6 a flag, 7 kind, 8 reason (bit set for kind 2),
    // 9 marker, 0xa amount, 0xb competition (-1: none), 0xc extra flag.
    {
        const std::uint32_t reasons[] = {0, 5, 6, 10, 0x23, 3, 4, 0x16, 0x22, 2, 1, 0x200, 0x600, 0x300, 0x1800, 0x2000, 0x2100, 0x1000, 0x3b00,
                                         0x2400, 0x900, 0xa, 0x802, 0x1c00, 0x2200, 0x2600, 0x2800};
        const std::uint32_t kinds[] = {2, 3, 1, 0x102};
        const std::int32_t competitions[] = {0, 3, 17, -1};
        unsigned i = 0;
        for (const auto kind : kinds)
            for (const std::uint32_t flag : {0u, 1u})
                for (const auto reason : reasons)
                    for (const std::uint32_t amount : {0u, 1250000u})
                        for (const std::uint32_t extra : {0u, 1u})
                            for (const std::uint32_t marker : {0u, 1u, 2u}) {
                                ++i;
                                const auto index = (i * 7) % people;
                                const auto ids = name_ids(scenario, scenario.person(index));
                                const std::int32_t from = ((marker + i / 7) & 1) ? -1 : std::int32_t(5 + i % 40);
                                const std::int32_t to = std::int32_t(60 + i % 70);
                                std::map<unsigned, std::uint32_t> words{{0, index}, {1, ids[0]}, {2, ids[1]}, {3, ids[2] + (i % 11 == 0)},
                                                                        {4, std::uint32_t(from)}, {5, std::uint32_t(to)}, {6, flag * (i % 5 ? 1u : 0x80000000u)},
                                                                        {7, kind}, {8, reason | ((i % 13 == 0) ? 0x10000u : 0u)}, {9, marker},
                                                                        {0xa, amount}, {0xb, std::uint32_t(competitions[(i / 2) % 4])}, {0xc, extra}};
                                for (const unsigned detail : {0u, 1u})
                                    cases.push_back(make_case(0x104001e0, detail, item_words(words),
                                                              label("4001", index, detail, kind * 100000 + flag * 50000 + reason)));
                            }
    }
}
}

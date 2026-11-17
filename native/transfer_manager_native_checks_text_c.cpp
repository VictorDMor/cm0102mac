// Cases for transfer_manager_native_text_c*.cpp: the texts of transfer news messages 4006, 4007, 4008, 4012, 4014 and 4015.
#include "transfer_manager_native_checks.hpp"
#include <array>
#include <map>

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t toc_base = 0x108bba90;
constexpr std::uint32_t item_at = scratch_base + 0x4000, output_at = scratch_base + 0x6000;

// A news item in the scratch area: the payload words at +8 + 4 * slot, the item id at +0xdc.
std::vector<Write> item_words(const std::map<unsigned, std::uint32_t>& words, std::uint32_t id = 0x1234) {
    std::vector<Write> writes{{4, item_at + 0xdc, id}};
    for (const auto& [slot, value] : words) writes.push_back({4, item_at + 8 + slot * 4, value});
    return writes;
}
// The three name ids a person's record links (+0x28, +0x2c, +0x30; their word +0x20).
std::array<std::uint32_t, 3> name_ids(const Scenario& scenario, std::uint32_t person) {
    std::array<std::uint32_t, 3> ids{};
    for (unsigned i = 0; i < 3; ++i)
        if (const auto link = scenario.r32(person + 0x28 + i * 4)) ids[i] = scenario.r32(link + 0x20);
    return ids;
}
std::string label(const char* what, unsigned a, unsigned b = 0, unsigned c = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
Case make_case(std::uint32_t entry, std::uint32_t detail, std::vector<Write> memory, std::string text, std::uint32_t owner = 0) {
    return {.entry = entry, .registers = {{2, toc_base}, {3, owner}, {4, item_at}, {5, detail}, {6, output_at}}, .memory = std::move(memory),
            .label = std::move(text)};
}
// Real offers of the manager (an array of pointers at +0, its size at +4), spread over the array.
std::vector<std::pair<std::uint32_t, std::uint32_t>> real_offers(const Scenario& scenario, unsigned wanted) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> offers;
    const auto manager = scenario.transfer_manager();
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size == 0 || size > 100000) return offers;
    const auto step = size / wanted + 1;
    for (std::uint32_t i = 0; i < size; i += step)
        if (const auto offer = scenario.r32(array + i * 4)) offers.push_back({i, offer});
    return offers;
}
// The person fields (index, then the three name ids; `shift` makes the person's name unknown to the date link).
void person_fields(std::map<unsigned, std::uint32_t>& words, const Scenario& scenario, unsigned first_slot, unsigned index, bool known) {
    const auto ids = name_ids(scenario, scenario.person(index));
    words[first_slot] = index;
    words[first_slot + 1] = ids[0] + (known ? 0 : 1);
    words[first_slot + 2] = ids[1];
    words[first_slot + 3] = ids[2];
}
}

TRANSFER_MANAGER_NATIVE_CASES("text_c") {
    if (scenario.match) return;
    const auto people = scenario.people();
    const auto step = people / 9 + 1;
    constexpr std::uint32_t entry_4012 = 0x10402e70, entry_4014 = 0x10403460, entry_4015 = 0x10403a90, entry_4007 = 0x10406af0,
                            entry_4006 = 0x10406fd0, entry_4008 = 0x10407a50;
    unsigned n = 0;

    // 4012: fields 0 person, 1-3 name ids, 4 and 5 clubs, 7 and 8 an offer's slot and id, 9 the marker, 10 a flag.
    const auto offers = real_offers(scenario, 6);
    // A null record in the manager's list (slot 0 emptied), reached by its index and by an index past the end.
    const auto manager_array = scenario.r32(scenario.transfer_manager()), manager_size = scenario.r32(scenario.transfer_manager() + 4);
    if (manager_size > 1)
        for (const std::uint32_t slot : {0u, manager_size + 3}) {
            std::map<unsigned, std::uint32_t> words;
            person_fields(words, scenario, 0, 7, true);
            words[4] = 6;
            words[5] = 13;
            words[7] = slot;
            words[8] = 1;
            auto memory = item_words(words);
            memory.push_back({4, manager_array, 0});
            cases.push_back(make_case(entry_4012, 1, memory, label("4012 null record", slot)));
        }
    for (unsigned index = 3; index < people; index += step, ++n) {
        for (const std::uint32_t marker : {0u, 1u, 4u, 2u})
            for (const std::uint32_t flag : {0u, 1u})
                for (const std::int32_t from : {-1, 5 + int(n), 8})
                    for (const bool known : {true, false}) {
                        std::map<unsigned, std::uint32_t> words;
                        person_fields(words, scenario, 0, index, known);
                        words[4] = std::uint32_t(from);
                        words[5] = 12 + n;
                        words[9] = marker;
                        words[10] = flag;
                        for (const std::uint32_t detail : {0u, 1u}) {
                            if (marker == 0 && !offers.empty()) {
                                for (std::size_t k = 0; k < offers.size(); ++k) {
                                    auto with_offer = words;
                                    with_offer[0] = scenario.r32(offers[k].second + 0xc);
                                    with_offer[7] = offers[k].first + (k % 3 == 2 ? 100000 : 0);
                                    with_offer[8] = scenario.r32(offers[k].second + 0x2c) + (k % 4 == 3 ? 1 : 0);
                                    cases.push_back(make_case(entry_4012, detail, item_words(with_offer), label("4012 offer", index, unsigned(k), detail + 2 * flag)));
                                }
                            }
                            cases.push_back(make_case(entry_4012, detail, item_words(words), label("4012", index, marker, from + 4 + flag + detail * 11)));
                        }
                    }
    }

    // 4014: fields 0 person, 1-3 name ids, 4 the club left, 5 the club joined, 6 a marker, 7 the refusal flags, 8 and 9 their values.
    n = 0;
    for (unsigned index = 3; index < people; index += step, ++n)
        for (const std::int32_t from : {-1, 6 + int(n), 9})
            for (const std::uint32_t marker : {0u, 1u, 3u})
                for (const std::uint32_t flags : {0u, 0x80000000u, 1u, 8u, 0x400000u, 0x1000000u, 0x20000000u, 0x10000u, 0x8000u, 0x80000u})
                    for (const bool known : {true, false}) {
                        std::map<unsigned, std::uint32_t> words;
                        person_fields(words, scenario, 0, index, known);
                        words[4] = std::uint32_t(from);
                        words[5] = 10 + n;
                        words[6] = marker;
                        words[7] = flags;
                        words[8] = 3 + n;
                        words[9] = 7;
                        for (const std::uint32_t detail : {0u, 1u})
                            cases.push_back(make_case(entry_4014, detail, item_words(words), label("4014", index, from + 4 + marker * 3 + known, unsigned(flags % 977) + detail), scenario.transfer_manager()));
                    }

    // 4015: fields 0-1 clubs, 2-5 a first person and name ids, 6-9 a second, 10 an amount, 11 its threshold.
    n = 0;
    for (unsigned index = 3; index < people; index += step, ++n)
        for (const std::uint32_t amount : {0u, 40000u, 5000000u})
            for (const std::uint32_t threshold : {0u, 25000u, 6000000u})
                for (const bool known : {true, false}) {
                    std::map<unsigned, std::uint32_t> words;
                    words[0] = 5 + n;
                    words[1] = 14 + n;
                    person_fields(words, scenario, 2, index, known);
                    person_fields(words, scenario, 6, (index * 7 + 11) % people, !known);
                    words[10] = amount;
                    words[11] = threshold;
                    for (const std::uint32_t detail : {0u, 1u})
                        cases.push_back(make_case(entry_4015, detail, item_words(words), label("4015", index, amount / 1000 + threshold / 1000, detail + 2 * known)));
                }

    // 4007: fields 0 person, 1-3 name ids, 4 a sign byte, 5-6 clubs, 7 an amount, 9 a third club.
    n = 0;
    for (unsigned index = 3; index < people; index += step, ++n)
        for (const std::uint32_t sign : {0u, 1u, 0xffu})
            for (const std::uint32_t amount : {0u, 750000u, 0x7fffffffu})
                for (const bool known : {true, false}) {
                    std::map<unsigned, std::uint32_t> words;
                    person_fields(words, scenario, 0, index, known);
                    words[4] = sign;
                    words[5] = 3 + n;
                    words[6] = 20 + n;
                    words[7] = amount;
                    words[9] = 8 + n;
                    for (const std::uint32_t detail : {0u, 1u})
                        cases.push_back(make_case(entry_4007, detail, item_words(words), label("4007", index, sign + known, amount / 1000 + detail)));
                }

    // 4006: fields 0 person, 1-3 name ids, 4 an amount, 5 a selector, 6 and 8 clubs, 7 a kind.
    n = 0;
    for (unsigned index = 3; index < people; index += step, ++n)
        for (const std::uint32_t kind : {2u, 0u, 5u})
            for (const std::uint32_t selector : {1u, 0u, 3u})
                for (const std::uint32_t amount : {0u, 42000u})
                    for (const bool known : {true, false}) {
                        std::map<unsigned, std::uint32_t> words;
                        person_fields(words, scenario, 0, index, known);
                        words[4] = amount;
                        words[5] = selector;
                        words[6] = 4 + n;
                        words[7] = kind;
                        words[8] = 30 + n;
                        for (const std::uint32_t detail : {0u, 1u})
                            cases.push_back(make_case(entry_4006, detail, item_words(words), label("4006", index, kind * 4 + selector, amount / 1000 + detail + 2 * known)));
                    }

    // 4008: fields 0 subject, 1-3 name ids, 4 and 5 clubs, 6 an amount, 9 a marker, 10-11 an object, 12 a kind, 13 a second amount,
    // 14 the number of listed people (4 fields each from 0xf), then the selector, a role flag, role arguments; 0xf holds the flag
    // of the short message.
    n = 0;
    for (unsigned index = 3; index < people; index += step, ++n)
        for (const std::uint32_t marker : {1u, 0u})
            for (const std::uint32_t amount : {0u, 90000u, 5000000u})
                for (const std::uint32_t object_kind : {0u, 3u})
                    for (const std::int32_t club_b : {-1, 11 + int(n)})
                        for (const std::uint32_t kind : {0u, 2u, 7u, 11u})
                            for (const std::uint32_t count : {0u, 1u, 2u, 3u, 5u})
                                for (const std::uint32_t selector : {1u, 0u, 4u}) {
                                    std::map<unsigned, std::uint32_t> words;
                                    person_fields(words, scenario, 0, index, (index + count + selector) % 3 != 0);
                                    words[4] = 2 + n;
                                    words[5] = std::uint32_t(club_b);
                                    words[6] = amount;
                                    words[9] = marker;
                                    words[10] = 4;
                                    words[11] = object_kind;
                                    words[12] = kind;
                                    words[13] = (n % 3 == 1) ? 12000u : (n % 3 == 2) ? 1u : 0u;
                                    words[14] = count;
                                    const unsigned listed = count < 3 ? count : 3;
                                    for (unsigned k = 0; k < listed; ++k) person_fields(words, scenario, 0xf + 4 * k, (index + 5 * (k + 1)) % people, k != 1);
                                    const unsigned tail = 0xf + 4 * listed;
                                    words[tail] = selector;
                                    words[tail + 1] = (n + count) % 2;
                                    words[tail + 2] = 1;
                                    words[tail + 3] = (selector == 4) ? 2 : 3;
                                    for (const std::uint32_t detail : {1u})
                                        cases.push_back(make_case(entry_4008, detail, item_words(words), label("4008", index, marker * 8 + kind + object_kind, club_b + 2 + count * 7 + selector * 31 + amount / 1000)));
                                    if (count == 0 && selector == 1 && kind == 0 && marker == 1) {
                                        std::map<unsigned, std::uint32_t> brief;
                                        person_fields(brief, scenario, 0, index, true);
                                        brief[4] = 2 + n;
                                        for (const std::uint32_t flag : {0u, 1u, 2u, 3u, 0xffu})
                                            for (const std::uint32_t club : {2u + n, 3u + n}) {
                                                brief[4] = club;
                                                brief[15] = flag;
                                                cases.push_back(make_case(entry_4008, 0, item_words(brief), label("4008 short", index, flag, club)));
                                            }
                                    }
                                }
}
}

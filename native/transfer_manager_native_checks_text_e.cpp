// Cases for transfer_manager_native_text_e*.cpp: the texts of transfer news messages 3011, 4011, 4013, 4016 and 4017.
// The items are built in the scratch area from the career's own people and clubs: field i of an item is the word at
// +8+4*i (NEWS.CPP 0x102cb570), its id at +0xdc; the output is a cleared text object.
#include "transfer_manager_native_checks.hpp"
#include <array>
#include <map>

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t toc_base = 0x108bba90;
constexpr std::uint32_t item_at = scratch_base + 0x4000, output_at = scratch_base + 0x6000;

std::vector<Write> item_words(const std::map<unsigned, std::uint32_t>& words, std::uint32_t id = 0x1234) {
    std::vector<Write> writes{{4, item_at + 0xdc, id}, {4, output_at, 0}, {4, output_at + 4, 0}};
    for (const auto& [slot, value] : words) writes.push_back({4, item_at + 8 + slot * 4, value});
    return writes;
}
// The three name ids a person's record links (+0x28, +0x2c, +0x30; their word +0x20): the day, month and year the
// messages hand to the date text, which the person's name check (0x100cbde0) compares.
std::array<std::uint32_t, 3> name_ids(const Scenario& scenario, std::uint32_t person) {
    std::array<std::uint32_t, 3> ids{};
    for (unsigned i = 0; i < 3; ++i)
        if (const auto link = scenario.r32(person + 0x28 + i * 4)) ids[i] = scenario.r32(link + 0x20);
    return ids;
}
// The person's index, then the three ids; `known` false shifts the first id so the name is not known.
void person_fields(std::map<unsigned, std::uint32_t>& words, const Scenario& scenario, unsigned first_slot, unsigned index, bool known) {
    const auto ids = name_ids(scenario, scenario.person(index));
    words[first_slot] = index;
    words[first_slot + 1] = ids[0] + (known ? 0 : 1);
    words[first_slot + 2] = ids[1];
    words[first_slot + 3] = ids[2];
}
std::string label(const char* what, unsigned a, unsigned b = 0, unsigned c = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
Case make_case(std::uint32_t entry, std::uint32_t detail, std::vector<Write> memory, std::string text) {
    return {.entry = entry, .registers = {{2, toc_base}, {4, item_at}, {5, detail}, {6, output_at}}, .memory = std::move(memory), .label = std::move(text)};
}
}

TRANSFER_MANAGER_NATIVE_CASES("text_e") {
    if (scenario.match) return;
    const auto people = scenario.people();
    const auto step = people / 7 + 1;
    constexpr std::uint32_t entry_3011 = 0x1040fda0, entry_4011 = 0x10409410, entry_4013 = 0x10409bd0, entry_4016 = 0x1040a280,
                            entry_4017 = 0x1040bc10;
    unsigned n = 0;
    const auto signed_word = [](std::int32_t value) { return std::uint32_t(value); };

    // 3011: fields 0 person, 1-3 name ids, 4 a flag byte, 5 a staff member's index (0x128-byte records, name at +0xae).
    n = 0;
    for (unsigned index = 3; index < people; index += step, ++n)
        for (const std::uint32_t flag : {0u, 1u, 0xffu})
            for (const std::uint32_t member : {0u, 3u + n, 40u})
                for (const bool known : {true, false}) {
                    std::map<unsigned, std::uint32_t> words;
                    person_fields(words, scenario, 0, index, known);
                    words[4] = flag;
                    words[5] = member;
                    for (const std::uint32_t detail : {0u, 1u})
                        cases.push_back(make_case(entry_3011, detail, item_words(words), label("3011", index, flag * 4 + known + detail * 2, member)));
                }

    // 4011: fields 0 and 1 clubs, 2 person with ids at 3-5, 6 a marker, 7 and 8 flags, 10 a flags byte.
    n = 0;
    for (unsigned index = 3; index < people; index += step, ++n)
        for (const std::uint32_t marker : {0x11u, 0u, 5u})
            for (const std::uint32_t flags : {0u, 1u, 2u, 3u, 4u, 5u, 8u, 0x22u, 0x23u, 0x40u, 0xffu}) {
                std::map<unsigned, std::uint32_t> words;
                person_fields(words, scenario, 2, index, (index + flags) % 3 != 0);
                words[0] = 4 + n;
                words[1] = 15 + n;
                words[6] = marker;
                words[10] = flags;
                for (const std::uint32_t second : {0u, 1u})
                    for (const std::uint32_t third : {0u, 1u}) {
                        words[7] = second;
                        words[8] = third;
                        cases.push_back(make_case(entry_4011, 1, item_words(words), label("4011", index, marker * 4 + second * 2 + third, flags)));
                    }
                cases.push_back(make_case(entry_4011, 0, item_words(words), label("4011 short", index, marker, flags)));
            }

    // 4013: fields 0 and 1 clubs, 2 and 6 people with their ids after them, 10 an amount, 11 its threshold, 12 a kind.
    n = 0;
    for (unsigned index = 3; index < people; index += step, ++n)
        for (const std::uint32_t amount : {0u, 40000u, 5000000u, 0xfffff000u})
            for (const std::uint32_t threshold : {0u, 25000u, 6000000u})
                for (const std::uint32_t kind : {0x16u, 0u, 5u})
                    for (const bool known : {true, false}) {
                        std::map<unsigned, std::uint32_t> words;
                        words[0] = 5 + n;
                        words[1] = 14 + n;
                        person_fields(words, scenario, 2, index, known);
                        person_fields(words, scenario, 6, (index * 7 + 11) % people, !known);
                        words[10] = amount;
                        words[11] = threshold;
                        words[12] = kind;
                        for (const std::uint32_t detail : {0u, 1u})
                            cases.push_back(make_case(entry_4013, detail, item_words(words), label("4013", index, kind + amount / 1000 + threshold / 1000, detail + 2 * known)));
                    }

    // 4016: fields 0 a club left, 1 the club, 2 person with ids at 3-5, 6 a third club, 7 a kind, 8 a flag.
    n = 0;
    constexpr std::uint32_t kinds[] = {0xb, 0x24, 0x17, 0x18, 0x13, 0xe, 0x14, 0x15, 0xc, 0xd, 0xf, 0x10, 0x11, 3, 4, 0x22, 5, 0x23, 1, 0x8017};
    for (unsigned index = 3; index < people; index += step, ++n)
        for (const std::uint32_t kind : kinds)
            for (const std::int32_t from : {-1, 6 + int(n)})
                for (const std::uint32_t flag : {0u, 1u})
                    for (const bool same : {false, true}) {
                        std::map<unsigned, std::uint32_t> words;
                        person_fields(words, scenario, 2, index, (index + kind) % 4 != 0);
                        words[0] = signed_word(from);
                        words[1] = 11 + n;
                        words[6] = same ? 11 + n : 20 + n;
                        words[7] = kind;
                        words[8] = flag;
                        cases.push_back(make_case(entry_4016, 1, item_words(words), label("4016", index, kind, unsigned(from + 1) + flag * 100 + same * 1000)));
                    }
    // The short form: the wording comes from the club's and the person's remainders; a negative club is a remainder below zero.
    for (const std::int32_t club : {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13})
        for (unsigned index = 3; index < 3 + 12 * step; index += step)
            for (const std::uint32_t flag : {0u, 1u}) {
                std::map<unsigned, std::uint32_t> words;
                person_fields(words, scenario, 2, index % people, index % 2 == 0);
                words[0] = signed_word(-1);
                words[1] = signed_word(club);
                words[8] = flag;
                cases.push_back(make_case(entry_4016, 0, item_words(words), label("4016 short", index, unsigned(club + 1), flag)));
            }

    // 4017: fields 0 person with ids at 1-3, 4 and 5 clubs, 6 and 7 two figures, 8 a second person with ids at 9-11, 12 a kind.
    n = 0;
    for (unsigned index = 3; index < people; index += step, ++n)
        for (const std::uint32_t kind : {0u, 1u, 2u, 0xffu})
            for (const std::uint32_t first : {0u, 90000u})
                for (const std::uint32_t second : {0u, 90000u, 0x7fffffffu})
                    for (const bool known : {true, false}) {
                        std::map<unsigned, std::uint32_t> words;
                        person_fields(words, scenario, 0, index, known);
                        words[4] = 6 + n;
                        words[5] = 12 + n;
                        words[6] = first;
                        words[7] = second;
                        person_fields(words, scenario, 8, (index * 7 + 11) % people, !known);
                        words[12] = kind;
                        for (const std::uint32_t detail : {0u, 1u})
                            cases.push_back(make_case(entry_4017, detail, item_words(words), label("4017", index, kind, first / 1000 + second / 1000 + detail + 2 * known)));
                    }
}
}

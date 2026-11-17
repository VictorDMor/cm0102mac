// Cases for transfer_manager_native_text_f*.cpp: the texts of eight transfer news messages (0x1040c580, 0x1040d0d0,
// 0x1040d820, 0x1040df70, 0x1040e490, 0x1040eda0, 0x1040f450, 0x1040f990).
// The items are built in the scratch area from the career's own people, clubs, offers and competitions: field i of an
// item is the word at +8+4*i (NEWS.CPP 0x102cb570), its kind at +0 and its id at +0xdc; the output is a cleared text object.
#include "guest_abi.hpp"
#include "transfer_manager_native_checks.hpp"

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t person_names = 0x28, offer_serial = 0x2c, offer_status = 0x34, club_word = 0x90;
constexpr std::uint32_t output_at = scratch_base + 0x2000, item_at = scratch_base + 0x4000, item_step = 0x400;
constexpr std::uint32_t toc_base = 0x108bba90;

struct Item {
    std::uint32_t at;
    std::vector<Write> writes;
    Item(std::uint32_t slot, std::uint32_t kind) : at(item_at + (slot % 8) * item_step) {
        writes.push_back({4, at, kind});
        writes.push_back({4, at + 0xdc, 1000 + slot});
        for (unsigned i = 0; i < 50; ++i) writes.push_back({4, at + 8 + 4 * i, 0});
    }
    Item& set(unsigned field, std::uint32_t value) {
        writes.push_back({4, at + 8 + 4 * field, value});
        return *this;
    }
};
struct Names {
    std::uint32_t first, second, common;
};
Names names_of(const Scenario& scenario, unsigned index) {
    const auto person = scenario.person(index);
    const auto word = [&](std::uint32_t slot) { return scenario.r32(scenario.r32(person + slot) + 0x20); };
    return {word(person_names), word(person_names + 4), word(person_names + 8)};
}
std::uint32_t lcg(std::uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return state >> 8;
}
template <typename T>
T pick(std::uint32_t& state, std::initializer_list<T> values) {
    return *(values.begin() + lcg(state) % values.size());
}
std::string label(const char* what, unsigned a, unsigned b = 0) { return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b); }

Case message_case(std::uint32_t entry, const Scenario& scenario, const Item& item, std::uint32_t flag, std::string name,
                  std::vector<Write> extra = {}) {
    Case result;
    result.entry = entry;
    result.registers = {{2, toc_base}, {3, scenario.transfer_manager()}, {4, item.at}, {5, flag}, {6, output_at}};
    result.memory = item.writes;
    result.memory.insert(result.memory.end(), extra.begin(), extra.end());
    result.memory.push_back({4, output_at, 0});
    result.memory.push_back({4, output_at + 4, 0});
    result.label = std::move(name);
    return result;
}
std::vector<std::pair<std::uint32_t, std::uint32_t>> real_offers(const Scenario& scenario, unsigned wanted) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> offers;  // (slot, offer)
    const auto manager = scenario.transfer_manager();
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size == 0 || size > 100000) return offers;
    const auto step = size / wanted + 1;
    for (std::uint32_t i = 0; i < size; i += step)
        if (const auto offer = scenario.r32(array + i * 4)) offers.push_back({i, offer});
    return offers;
}
}

TRANSFER_MANAGER_NATIVE_CASES("text_f") {
    if (scenario.match) return;
    const auto people = scenario.people();
    const auto offers = real_offers(scenario, 24);
    std::uint32_t seed = 777;
    const auto person_index = [&] { return lcg(seed) % people; };
    const auto club = [&] { return pick<std::uint32_t>(seed, {0, 1, 5, 17, 40, 77, 120, 200, 300, 450}); };
    const auto club_or_none = [&] { return pick<std::uint32_t>(seed, {0xffffffffu, club(), club(), club()}); };
    const auto flag = [&](unsigned v) { return v % 3 == 0 ? 0u : pick<std::uint32_t>(seed, {1, 0x100, 0xff, 0x7f}); };
    unsigned slot = 0;
    const auto table = scenario.toc(-0x7bb0);
    const auto current_slot = std::int32_t(std::int16_t(scenario.r32(table + 0x2c58) >> 16));

    // The club record's stored word (+0x90): the news slot's word (the "same" case), another, or empty.
    const auto stored = [&](std::uint32_t club_index, unsigned mode) {
        std::vector<Write> writes;
        // The news slot's word is made non-zero so that "the same" can be told from "empty".
        writes.push_back({4, table + 4 + std::uint32_t(current_slot) * 0x2c0, 0x5555u});
        writes.push_back({4, scenario.club(club_index) + club_word, mode == 0 ? 0x5555u : mode == 1 ? 0u : 0x5559u});
        return writes;
    };
    const auto date_of = [&](Item& item, unsigned first_field, const Names& names, unsigned v) {
        item.set(first_field, v % 5 == 4 ? names.first + 1 : names.first).set(first_field + 1, names.second).set(first_field + 2, names.common);
    };
    const auto join = [](std::vector<Write> a, const std::vector<Write>& b) {
        a.insert(a.end(), b.begin(), b.end());
        return a;
    };

    // ---- 0x1040df70: a person, two clubs, a month and year. ----
    for (unsigned v = 0; v < 120; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4023);
        const auto first = club(), second = club_or_none() == 0xffffffffu ? club() : club();
        item.set(0, index);
        date_of(item, 1, names, v);
        item.set(4, first).set(5, second);
        item.set(8, pick<std::uint32_t>(seed, {1, 6, 12, 0xffff, 0x10001, 0}));
        item.set(9, pick<std::uint32_t>(seed, {2001, 1999, 0x10000, 0, 2002}));
        cases.push_back(message_case(0x1040df70, scenario, item, flag(v), label("df70", v), stored(second, (v / 2) % 3)));
    }

    // ---- 0x1040e490: the sentence code (field 6), the competition (field 7). ----
    for (unsigned v = 0; v < 240; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4024);
        const auto second = club();
        item.set(0, index);
        date_of(item, 1, names, v);
        item.set(4, club()).set(5, second);
        item.set(6, pick<std::uint32_t>(seed, {4, 9, 0x26, 0x1e, 0x25, 0x1d, 0x1f, 0, 3, 0x104, 0xff}));
        item.set(7, pick<std::uint32_t>(seed, {0, 1, 2, 5, 9, 20, 40}));
        cases.push_back(message_case(0x1040e490, scenario, item, flag(v), label("e490", v), stored(second, (v / 2) % 3)));
    }

    // ---- 0x1040eda0: a club or two, a kind with a second person. ----
    for (unsigned v = 0; v < 240; ++v) {
        const auto index = person_index(), other = person_index();
        const auto names = names_of(scenario, index), other_names = names_of(scenario, other);
        Item item(slot++, 4025);
        item.set(0, index);
        date_of(item, 1, names, v);
        item.set(4, club()).set(5, club_or_none());
        item.set(6, pick<std::uint32_t>(seed, {0, 1, 2, 0x100}));
        item.set(7, pick<std::uint32_t>(seed, {0, 1, 2, 0x100, 1}));
        item.set(8, other);
        date_of(item, 9, other_names, v / 2);
        cases.push_back(message_case(0x1040eda0, scenario, item, flag(v), label("eda0", v)));
    }

    // ---- 0x1040f450: one or two clubs and a name word of the first. ----
    for (unsigned v = 0; v < 120; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4026);
        item.set(0, index);
        date_of(item, 1, names, v);
        item.set(4, club()).set(5, club_or_none());
        cases.push_back(message_case(0x1040f450, scenario, item, flag(v), label("f450", v)));
    }

    // ---- 0x1040f990: two people, two clubs, a fee between two thresholds. ----
    for (unsigned v = 0; v < 240; ++v) {
        const auto index = person_index(), other = person_index();
        const auto names = names_of(scenario, index), other_names = names_of(scenario, other);
        Item item(slot++, 4027);
        item.set(0, club()).set(1, club()).set(2, index);
        date_of(item, 3, names, v);
        item.set(6, other);
        date_of(item, 7, other_names, v / 2);
        item.set(10, pick<std::uint32_t>(seed, {0, 1, 5, 250000, 7500000, 0x7fffffff, 0x80000000u, 0xffffffffu}));
        item.set(11, pick<std::uint32_t>(seed, {0, 1, 250000, 5, 0x80000000u, 0xffffffffu, 0x7fffffff}));
        cases.push_back(message_case(0x1040f990, scenario, item, flag(v), label("f990", v)));
    }

    // ---- 0x1040d0d0: the stage (field 10) against the offer named by fields 11 and 12. ----
    for (unsigned v = 0; v < 360; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4028);
        std::vector<Write> extra;
        item.set(0, index);
        date_of(item, 1, names, v);
        item.set(4, club()).set(5, club_or_none());
        item.set(9, pick<std::uint32_t>(seed, {0, 1, 2, 7, 500}));
        item.set(10, pick<std::uint32_t>(seed, {1, 1, 1, 0, 5, 0x101}));
        if (!offers.empty()) {
            const auto [offer_slot, offer] = offers[v % offers.size()];
            item.set(11, v % 11 == 10 ? 0x7fffffff : offer_slot);
            if (v % 13 == 12) extra.push_back({4, scenario.r32(scenario.transfer_manager()) + offer_slot * 4, 0});
            item.set(12, v % 5 == 4 ? scenario.r32(offer + offer_serial) + 1 : scenario.r32(offer + offer_serial));
            extra.push_back({1, offer + offer_status, pick<std::uint32_t>(seed, {0, 1, 1, 2, 3, 0x81, 4, 0xfe})});
            // The offer's person and club words against the item's: equal in some cases.
            if (v % 3 == 0) {
                item.set(0, scenario.r32(offer + 0xc));
                item.set(4, v % 6 == 0 ? scenario.r32(offer + 8) + 1 : scenario.r32(offer + 8));  // the club word differs in half
            }
        } else {
            item.set(11, 7).set(12, 9);
        }
        cases.push_back(message_case(0x1040d0d0, scenario, item, flag(v), label("d0d0", v), extra));
    }

    // ---- 0x1040d820: the stage word (field 9), the offer (fields 6 and 7), the contract text (fields 8, 10-12). ----
    for (unsigned v = 0; v < 480; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4029);
        std::vector<Write> extra;
        item.set(0, index);
        date_of(item, 1, names, v);
        const auto second = club();
        item.set(4, club()).set(5, second);
        item.set(8, pick<std::uint32_t>(seed, {0, 1, 3, 0xffffffffu, 0x80000000u}));
        item.set(9, v % 2 ? pick<std::uint32_t>(seed, {0, 0, 0, 1, 2, 8, 0xf, 3}) : 0);
        item.set(10, pick<std::uint32_t>(seed, {1, 6, 12, 0x10003, 0xffff}));
        item.set(11, pick<std::uint32_t>(seed, {2001, 1999, 0x10000, 0}));
        item.set(12, pick<std::uint32_t>(seed, {0, 1, 3, 0xffffffffu}));
        if (v % 3 == 0 || v % 7 == 1) item.set(8, 1).set(12, 1);
        if (!offers.empty()) {
            const auto [offer_slot, offer] = offers[v % offers.size()];
            item.set(6, v % 11 == 10 ? 0x7fffffff : offer_slot);
            if (v % 13 == 12) extra.push_back({4, scenario.r32(scenario.transfer_manager()) + offer_slot * 4, 0});
            item.set(7, v % 5 == 4 ? scenario.r32(offer + offer_serial) + 1 : scenario.r32(offer + offer_serial));
            extra.push_back({1, offer + offer_status, pick<std::uint32_t>(seed, {0, 1, 2, 8, 0xf, 3, 0x81, 0xfe})});
        } else {
            item.set(6, 7).set(7, 9);
        }
        extra = join(extra, stored(second, (v / 2) % 3));
        cases.push_back(message_case(0x1040d820, scenario, item, flag(v), label("d820", v), extra));
    }

    // ---- 0x1040c580: two amounts, a month and year, and up to three more people. ----
    for (unsigned v = 0; v < 320; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4030);
        item.set(0, index);
        date_of(item, 1, names, v);
        item.set(4, club()).set(5, club_or_none());
        item.set(6, pick<std::uint32_t>(seed, {0, 1, 100000, 250000, 7500000, 0x7fffffff, 0x80000000u, 0xffffffffu, 16777217, 0x40000000, 5}));
        item.set(8, pick<std::uint32_t>(seed, {0, 1, 100000, 250000, 7500000, 0x7fffffff, 0x80000000u, 0xffffffffu, 16777217, 0x3fffffff, 5}));
        item.set(10, pick<std::uint32_t>(seed, {1, 6, 12, 0x10003, 0xffff, 0x8000}));
        item.set(11, pick<std::uint32_t>(seed, {2001, 1999, 0x10000, 0, 0x8001}));
        const auto count = pick<std::uint32_t>(seed, {0, 0, 1, 2, 3, 3, 4, 0xffffffffu, 0x101, 0x7f, 0x80});
        item.set(12, count);
        for (unsigned k = 0; k < 3; ++k) {
            const auto other = person_index();
            const auto other_names = names_of(scenario, other);
            item.set(13 + 4 * k, other);
            date_of(item, 14 + 4 * k, other_names, v + k);
        }
        cases.push_back(message_case(0x1040c580, scenario, item, flag(v), label("c580", v)));
    }
    // The second amount no larger than the first, small first amounts: the texts the large amounts never reach.
    for (unsigned v = 0; v < 48; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4030);
        item.set(0, index);
        date_of(item, 1, names, v);
        item.set(4, club()).set(5, v % 3 == 0 ? 0xffffffffu : club());
        const std::uint32_t amounts[][2] = {{0x80000000u, 0x80000000u}, {0xfffffffbu, 0xfffffffbu}, {0xffffff00u, 0x80000000u}, {0, 0}};
        item.set(6, amounts[v % 4][0]).set(8, amounts[v % 4][1]);
        item.set(10, 6).set(11, 2001).set(12, v % 2);
        const auto other = person_index();
        const auto other_names = names_of(scenario, other);
        item.set(13, other);
        date_of(item, 14, other_names, 0);
        cases.push_back(message_case(0x1040c580, scenario, item, v % 6 == 5 ? 0 : 1, label("c580 low", v)));
    }
}
}

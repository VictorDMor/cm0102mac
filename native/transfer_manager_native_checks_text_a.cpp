// Cases for transfer_manager_native_text_a*.cpp: the texts of the transfer news messages 4000, 3020, 4032 and 4035.
// The items are built in the scratch area from the career's own people, clubs and offers: field i of an item is the
// word at +8+4*i (NEWS.CPP 0x102cb570), its kind at +0 and its id at +0xdc; the output is a cleared text object.
#include "transfer_manager_native_checks.hpp"
#include "guest_abi.hpp"

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t person_names = 0x28, offer_serial = 0x2c, offer_status = 0x34, club_competition_word = 0x90;
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
// A person's three name words (what the message's name text and the "may be named" test compare).
struct Names {
    std::uint32_t first, second, common;
};
Names names_of(const Scenario& scenario, unsigned index) {
    // The person's name words are records (pointers at +0x28, +0x2c, +0x30) whose word +0x20 is what the item carries.
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

// The registers every case sets: the item (r4), the flag byte (r5), the output (r6) and the table of contents.
Case message_case(std::uint32_t entry, const Item& item, std::uint32_t flag, std::string name, std::vector<Write> extra = {}) {
    Case result;
    result.entry = entry;
    result.registers = {{2, toc_base}, {4, item.at}, {5, flag}, {6, output_at}};
    result.memory = item.writes;
    result.memory.insert(result.memory.end(), extra.begin(), extra.end());
    result.memory.push_back({4, output_at, 0});
    result.memory.push_back({4, output_at + 4, 0});
    result.label = std::move(name);
    return result;
}
// Real offers of the manager (an array of pointers at +0, its size at +4).
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

TRANSFER_MANAGER_NATIVE_CASES("text_a") {
    if (scenario.match) return;
    const auto people = scenario.people();
    const auto offers = real_offers(scenario, 12);
    std::uint32_t seed = 12345;
    const auto person_index = [&] { return lcg(seed) % people; };
    const auto club = [&] { return pick<std::uint32_t>(seed, {0, 1, 5, 17, 40, 77, 120, 200, 300, 450}); };
    const auto flags = [&] { return pick<std::uint32_t>(seed, {0, 1, 0x100, 0xff, 0x7f, 2}); };
    // The club the news table holds for the current slot: the seller counts as "elsewhere" unless its word 0x90 is this.
    const auto table = scenario.toc(-0x7bb0);
    const auto own_club = scenario.r32(table + 4 + std::uint32_t(std::int32_t(std::int16_t(scenario.r32(table + 0x2c58) >> 16))) * 0x2c0);
    unsigned slot = 0;

    // ---- 0x104106d0 (kind 3020): a person, a date, a club; both texts, known and unknown names. ----
    for (unsigned v = 0; v < 24; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        const bool known = v % 3 != 2;
        Item item(slot++, 3020);
        item.set(0, index).set(1, known ? names.first : names.first + 1).set(2, names.second).set(3, names.common).set(4, club());
        cases.push_back(message_case(0x104106d0, item, v & 1 ? flags() : 0, label("3020", v)));
    }

    // ---- 0x104116e0 (kind 4035): a person, a fee and three clubs. ----
    for (unsigned v = 0; v < 24; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4035);
        item.set(0, index).set(1, names.first).set(2, names.second).set(3, v % 4 == 3 ? names.common + 1 : names.common)
            .set(4, club()).set(5, pick<std::uint32_t>(seed, {0, 1, 999, 250000, 7500000, 0x7fffffff, 0x80000000})).set(6, club()).set(7, club());
        cases.push_back(message_case(0x104116e0, item, flags() * (v & 1), label("4035", v)));
    }

    // ---- 0x10410a60 (kind 4032): the stored offer decides the status written back into the item. ----
    for (unsigned v = 0; v < 48; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4032);
        item.set(0, index).set(1, names.first).set(2, names.second).set(3, v % 5 == 4 ? names.common + 3 : names.common)
            .set(4, club()).set(5, club());
        std::vector<Write> extra;
        const auto status_choice = v % 6;
        item.set(10, status_choice < 3 ? 1 : pick<std::uint32_t>(seed, {0, 3, 7, 0xff}));
        if (!offers.empty()) {
            const auto [offer_slot, offer] = offers[v % offers.size()];
            item.set(6, offer_slot);
            const auto serial = scenario.r32(offer + offer_serial);
            item.set(7, v % 4 == 3 ? serial + 1 : serial);
            // The stored status byte: 0 and 1 (nothing to write), others (written back), 3 and 0 flavour the text.
            extra.push_back({1, offer + offer_status, pick<std::uint32_t>(seed, {0, 1, 3, 4, 0xfe})});
            if (status_choice == 1) item.set(0, scenario.r32(offer + 0xc)).set(4, scenario.r32(offer + 8));
            if (status_choice == 2) item.set(0, scenario.r32(offer + 0xc)).set(4, scenario.r32(offer + 8) + 1).set(7, serial + 1);
        } else {
            item.set(6, 7).set(7, 9);
        }
        if (v % 7 == 6) item.set(6, 0x7fffffff);  // out of the offer array: the first slot is read
        if (v % 9 == 8 && !offers.empty()) {      // an empty slot of the offer array
            item.set(6, offers[0].first);
            extra.push_back({4, scenario.r32(scenario.transfer_manager()) + offers[0].first * 4, 0});
        }
        cases.push_back(message_case(0x10410a60, item, v & 1 ? flags() : 0, label("4032", v), extra));
    }

    // ---- 0x103fe1a0 (kind 4000): the short text, the stage lookup, and the sentences of the long text. ----
    for (unsigned v = 0; v < 48; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4000);
        item.set(0, index).set(1, names.first).set(2, names.second).set(3, v % 5 == 4 ? names.common + 1 : names.common).set(4, club());
        std::vector<Write> extra;
        item.set(9, v % 3 == 0 ? 0 : pick<std::uint32_t>(seed, {1, 2, 5, 0x11, 3}));
        if (!offers.empty()) {
            const auto [offer_slot, offer] = offers[v % offers.size()];
            item.set(6, offer_slot).set(7, scenario.r32(offer + offer_serial));
            extra.push_back({1, offer + offer_status, pick<std::uint32_t>(seed, {0, 1, 5, 2, 0x11})});
            if (v % 12 == 3) item.set(6, 0x7fffffff);                                              // out of the array
            if (v % 12 == 9) extra.push_back({4, scenario.r32(scenario.transfer_manager()) + offer_slot * 4, 0});  // empty slot
        }
        cases.push_back(message_case(0x103fe1a0, item, 0, label("4000 short", v), extra));
    }
    unsigned variant = 0;
    for (unsigned v = 0; v < 900; ++v, ++variant) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4000);
        const auto buyer = club();
        const auto seller = pick<std::uint32_t>(seed, {0xffffffffu, club(), club(), buyer, club()});
        item.set(0, index).set(1, names.first).set(2, names.second).set(3, v % 7 == 6 ? names.common + 1 : names.common);
        item.set(4, buyer).set(5, seller);
        // Whether the seller counts as another club depends on word 0x90 of its record against the slot of the
        // current club in the news table: some cases set it to the table's own value, some to 0.
        std::vector<Write> extra;
        if (seller != 0xffffffffu && seller < 500) {
            const auto record = scenario.club(seller) + club_competition_word;
            extra.push_back({4, record, pick<std::uint32_t>(seed, {0, 0, own_club, 0x1234})});
        }
        const auto fee = pick<std::uint32_t>(seed, {0, 1, 250000, 500000, 600000, 4000000, 0xffffff00u});
        const auto limit = pick<std::uint32_t>(seed, {0, 100000, 300000, 700000, 5000000});
        item.set(8, fee).set(10, limit);
        item.set(9, pick<std::uint32_t>(seed, {0, 1, 2, 5, 0x11, 4}));
        item.set(0xb, v % 5 == 0 ? pick<std::uint32_t>(seed, {1, 3, 0x100}) : 0);
        item.set(0xc, pick<std::uint32_t>(seed, {0, 3, 11, 0xfff0}));
        item.set(0xd, pick<std::uint32_t>(seed, {0, 2001, 1999, 0x10000}));
        item.set(0xe, pick<std::uint32_t>(seed, {0, 1, 2, 3, 0x101}));
        item.set(0xf, pick<std::uint32_t>(seed, {0, 1, 2, 3, 0x203}));
        item.set(0x10, pick<std::uint32_t>(seed, {0, 1, 2, 3, 4, 5, 6, 7, 0x83}));
        item.set(0x11, pick<std::uint32_t>(seed, {0xffffffffu, club(), club()}));
        item.set(0x12, pick<std::uint32_t>(seed, {0, 3, 5, 9}));
        const auto count = pick<std::uint32_t>(seed, {0, 0, 1, 2, 3, 4, 0xffffffffu, 0x101});
        item.set(0x17, count);
        // The people named after the first one: index, three words.
        for (unsigned k = 0; k < 4; ++k) {
            const auto other = person_index();
            const auto other_names = names_of(scenario, other);
            item.set(0x18 + 4 * k, other).set(0x19 + 4 * k, other_names.first).set(0x1a + 4 * k, other_names.second)
                .set(0x1b + 4 * k, k == 1 ? other_names.common + 1 : other_names.common);
        }
        // The word the 100c6bf0 lookup is given sits after the named people.
        const auto named = std::int8_t(count) < 3 ? std::int8_t(count) : 3;
        const auto subject_word = 0x18 + 4 * (std::int8_t(count) > 0 ? named : 0);
        item.set(subject_word, pick<std::uint32_t>(seed, {0, 1, 2, 3, 5, 9, 0x11, 0x41}));
        cases.push_back(message_case(0x103fe1a0, item, v % 11 == 0 ? 0x100 : 1 + v % 2, label("4000 long", v), extra));
    }
    // The loan sentences (word 0xb set, no named people): the date and the agent word choose among them.
    for (unsigned v = 0; v < 240; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4000);
        const auto buyer = club();
        const auto seller = v % 2 == 0 ? club() : 0xffffffffu;
        item.set(0, index).set(1, names.first).set(2, names.second).set(3, names.common).set(4, buyer).set(5, seller);
        std::vector<Write> extra;
        if (seller != 0xffffffffu)
            extra.push_back({4, scenario.club(seller) + club_competition_word, v % 6 == 0 ? own_club : 0});
        item.set(8, v % 4 == 0 ? 0 : 500000).set(9, 1 + v % 5).set(0xb, 1);
        item.set(0xc, 3 + v % 9).set(0xd, v % 8 == 7 ? 0 : 2000 + v % 3).set(0xf, v % 4).set(0x12, (v / 4) % 3 == 0 ? 3 : (v / 4) % 3 == 1 ? 5 : 8);
        // The word after the (absent) people names the position the sentence is about; 11 is the one that is not listed.
        item.set(0x11, 0xffffffffu).set(0x17, 0).set(0x18, v % 7 == 3 ? 11 : v % 24);
        cases.push_back(message_case(0x103fe1a0, item, 1, label("4000 loan", v), extra));
    }
}
}

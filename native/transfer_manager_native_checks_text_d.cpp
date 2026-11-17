// Cases for transfer_manager_native_text_d*.cpp: the texts of the transfer news messages 4004, 4005, 4009 and 4031.
// The items are built in the scratch area from the career's own people, clubs, offers and competitions: field i of an
// item is the word at +8+4*i (NEWS.CPP 0x102cb570), its kind at +0 and its id at +0xdc; the output is a cleared text object.
#include "guest_abi.hpp"
#include <cstdio>
#include "transfer_manager_native_checks.hpp"

namespace cm::aot::transfer_manager_native_checks {
namespace {
constexpr std::uint32_t person_names = 0x28, offer_serial = 0x2c, offer_status = 0x34;
constexpr std::uint32_t output_at = scratch_base + 0x2000, item_at = scratch_base + 0x4000, item_step = 0x400;
constexpr std::uint32_t league_at = scratch_base + 0x10000, window_at = scratch_base + 0x10200;
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

// The registers every case sets: the manager (r3), the item (r4), the flag byte (r5), the output (r6) and the table of contents.
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

TRANSFER_MANAGER_NATIVE_CASES("text_d") {
    if (scenario.match) return;
    const auto people = scenario.people();
    const auto offers = real_offers(scenario, 16);
    std::uint32_t seed = 4242;
    const auto person_index = [&] { return lcg(seed) % people; };
    const auto club = [&] { return pick<std::uint32_t>(seed, {0, 1, 5, 17, 40, 77, 120, 200, 300, 450}); };
    const auto club_or_none = [&] { return pick<std::uint32_t>(seed, {0xffffffffu, club(), club(), club()}); };
    unsigned slot = 0;
    const auto table = scenario.toc(-0x7bb0);
    const auto current_slot = std::int32_t(std::int16_t(scenario.r32(table + 0x2c58) >> 16));
    const auto news_record = scenario.r32(table + 4 + std::uint32_t(current_slot) * 0x2c0);
    const auto league_word = scenario.r32(scenario.toc(-0x7a70));
    const auto clubs_table = scenario.r32(scenario.toc(-0x7be8));

    // A league window the second club's record can point at: its word 8 is the current season's word, or not.
    const auto league_for = [&](std::uint32_t club_index, unsigned mode) {
        std::vector<Write> writes;
        const auto record = scenario.club(club_index) + 0x2c;
        writes.push_back({4, record, mode == 3 ? 0 : league_at});
        writes.push_back({4, league_at + 0x74, mode == 2 ? 0 : window_at});
        writes.push_back({4, window_at + 8, mode == 0 ? league_word : league_word + 1});
        return writes;
    };

    // ---- 0x10408f80 (kind 4009): a person, a date, two clubs. ----
    for (unsigned v = 0; v < 24; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4009);
        item.set(0, club()).set(1, club()).set(2, index).set(3, v % 3 == 2 ? names.first + 1 : names.first).set(4, names.second).set(5, names.common);
        cases.push_back(message_case(0x10408f80, scenario, item, v & 1 ? pick<std::uint32_t>(seed, {1, 0x100, 0xff, 0x7f}) : 0, label("4009", v)));
    }

    // ---- 0x104101f0 (kind 4031): a person, a date, a club, a competition, the status and a fee. ----
    for (unsigned v = 0; v < 40; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4031);
        item.set(0, index).set(1, v % 3 == 2 ? names.first + 1 : names.first).set(2, names.second).set(3, names.common);
        item.set(4, club()).set(5, pick<std::uint32_t>(seed, {0, 1, 2, 5, 9, 20, 40, 100}));
        item.set(6, pick<std::uint32_t>(seed, {3, 3, 0, 1, 7, 0x103}));
        item.set(7, pick<std::uint32_t>(seed, {0, 1, 999, 250000, 7500000, 0x7fffffff, 0x80000000}));
        cases.push_back(message_case(0x104101f0, scenario, item, v & 1 ? pick<std::uint32_t>(seed, {1, 0x100, 0xff}) : 0, label("4031", v)));
    }

    // ---- 0x10405ca0 (kind 4005): the stage update (offers, the news slot), then the short or long text. ----
    for (unsigned v = 0; v < 400; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4005);
        item.set(0, index).set(1, v % 7 == 6 ? names.first + 1 : names.first).set(2, names.second).set(3, names.common);
        item.set(4, club()).set(5, club_or_none());
        item.set(8, pick<std::uint32_t>(seed, {0, 1, 300000, 0xffffff00u, 5}));
        item.set(10, pick<std::uint32_t>(seed, {0, 1, 400000, 100000, 0x7fffffff}));
        item.set(11, pick<std::uint32_t>(seed, {0, 1, 2, 5, 0x100}));
        item.set(12, pick<std::uint32_t>(seed, {0, 0, 1, 0x100}));
        item.set(13, pick<std::uint32_t>(seed, {0, 1, 2, 0x101}));
        item.set(14, pick<std::uint32_t>(seed, {0, 3, 11, 0xfff0, 0x10005}));
        item.set(15, pick<std::uint32_t>(seed, {0, 2001, 1999, 0x10000}));
        item.set(17, pick<std::uint32_t>(seed, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0x11, 0x12, 0x13, 0x14, 0x20, 0x7f, 0x80, 0xffffffffu}));
        std::vector<Write> extra;
        // The item's stage (word 9): some end the update at once, the rest look the offer up.
        item.set(9, pick<std::uint32_t>(seed, {0, 1, 4, 0xe, 3, 0x10, 2, 0x12, 0x11, 0xffffffffu, 0xd, 5, 0xc, 0x13}));
        if (!offers.empty()) {
            const auto [offer_slot, offer] = offers[v % offers.size()];
            item.set(6, v % 9 == 8 ? 0x7fffffff : offer_slot);
            // Some cases point at an offer slot that holds nothing.
            if (v % 13 == 12) extra.push_back({4, scenario.r32(scenario.transfer_manager()) + offer_slot * 4, 0});
            item.set(7, v % 5 == 4 ? scenario.r32(offer + offer_serial) + 1 : scenario.r32(offer + offer_serial));
            extra.push_back({1, offer + offer_status, pick<std::uint32_t>(seed, {0, 1, 2, 3, 0xc, 0xd, 0xe, 0x11, 0x12, 0x13, 0x4, 0xfe})});
            // The news slot's club against the offer's buyer: equal for half of the cases.
            if (v % 2 == 0 && news_record) extra.push_back({4, news_record + 0x24, clubs_table + scenario.r32(offer + 8) * 0x248});
        } else {
            item.set(6, 7).set(7, 9);
        }
        cases.push_back(message_case(0x10405ca0, scenario, item, v % 3 == 0 ? 0 : pick<std::uint32_t>(seed, {1, 0x100, 0xff}), label("4005", v), extra));
    }

    // ---- 0x10405ca0, directed: every stored stage against the news slot's club (same / other), both texts. ----
    if (!offers.empty()) {
        const auto [offer_slot, offer] = offers[0];
        for (const std::uint32_t code : {0u, 1u, 2u, 3u, 4u, 0xcu, 0xdu, 0xeu, 0x11u, 0x12u, 0x13u, 0xfeu})
            for (unsigned same = 0; same < 2; ++same)
                for (unsigned full = 0; full < 2; ++full) {
                    const auto index = person_index();
                    const auto names = names_of(scenario, index);
                    Item item(slot++, 4005);
                    item.set(0, index).set(1, names.first).set(2, names.second).set(3, names.common).set(4, club()).set(5, club());
                    item.set(6, offer_slot).set(7, scenario.r32(offer + offer_serial)).set(9, 0).set(10, 400000).set(8, 1).set(12, code & 1).set(17, 6);
                    std::vector<Write> extra{{1, offer + offer_status, code}};
                    if (news_record) extra.push_back({4, news_record + 0x24, same ? clubs_table + scenario.r32(offer + 8) * 0x248 : 0x1234u});
                    cases.push_back(message_case(0x10405ca0, scenario, item, full ? 1 : 0, label("4005 directed", code, same * 2 + full), extra));
                }
    }

    // ---- 0x10404530, directed: the sentence tree by kind, stage mark, club A, flags and league window. ----
    {
        const std::uint32_t sets[] = {0x10, 0x18, 0x4010, 0x4018, 0x200010, 0x40000000, 0x40000408, 0x40000400, 0x40000008, 0x40000040,
                                      0x40000004, 0x40000800, 0x40000200, 0x4000, 0x200000, 0x204000, 0x40, 0x100, 0x140, 0x4, 0x800, 0x20000, 0x20010, 0x200018, 0};
        unsigned n = 0;
        for (const std::uint32_t kind : {2u, 3u, 4u})
            for (const std::uint32_t mark : {0u, 1u, 2u})
                for (unsigned a_none = 0; a_none < 2; ++a_none)
                    for (const auto flagset : sets)
                        for (unsigned mode = 0; mode < ((flagset & 0x200000) ? 4u : 1u); ++mode, ++n) {
                            const auto index = person_index();
                            const auto names = names_of(scenario, index);
                            Item item(slot++, 4004);
                            const auto buyer = club();
                            item.set(0, index).set(1, names.first).set(2, names.second).set(3, names.common);
                            item.set(4, a_none ? 0xffffffffu : club()).set(5, buyer).set(6, kind).set(9, mark);
                            const auto fee = n % 3 == 0 ? 150000u : n % 3 == 1 ? 100001u : 7u;
                            item.set(10, fee).set(11, n % 4 == 0 ? fee : 200000u);
                            const auto other = person_index();
                            const auto other_names = names_of(scenario, other);
                            item.set(12, other).set(13, other_names.first).set(14, other_names.second).set(15, other_names.common);
                            item.set(16, n % 17 == 5 ? club() : 0xffffffffu).set(17, n % 2 ? 6u : 0u);
                            const auto second = person_index(), third = person_index();
                            const auto second_names = names_of(scenario, second), third_names = names_of(scenario, third);
                            item.set(18, n % 5 == 1 ? 0xffffffffu : second).set(19, second_names.first).set(20, second_names.second).set(21, second_names.common);
                            item.set(22, n % 7 == 1 ? 0xffffffffu : third).set(23, third_names.first).set(24, third_names.second).set(25, third_names.common);
                            item.set(26, flagset).set(27, pick<std::uint32_t>(seed, {0, 3, 30, 400, 3000, 100000})).set(28, n % 6);
                            for (unsigned k = 0; k < 4; ++k) item.set(29 + k, club());
                            std::vector<Write> extra;
                            if (buyer < 500) extra = league_for(buyer, mode);
                            cases.push_back(message_case(0x10404530, scenario, item, n % 5 == 4 ? 0 : 1, label("4004 directed", n), extra));
                        }
    }

    // ---- 0x10404530 (kind 4004): every sentence of the long text, the short text and the extra clubs. ----
    for (unsigned v = 0; v < 1500; ++v) {
        const auto index = person_index();
        const auto names = names_of(scenario, index);
        Item item(slot++, 4004);
        const auto buyer = club(), seller = club_or_none();
        item.set(0, index).set(1, names.first).set(2, names.second).set(3, v % 11 == 10 ? names.common + 1 : names.common);
        item.set(4, seller).set(5, buyer);
        item.set(6, pick<std::uint32_t>(seed, {2, 3, 4, 4, 4, 1, 0xff}));
        item.set(9, pick<std::uint32_t>(seed, {0, 1, 1, 2}));
        const auto fee = pick<std::uint32_t>(seed, {0, 250000, 150000, 99999, 100001});
        item.set(10, fee);
        item.set(11, v % 3 == 0 ? fee : pick<std::uint32_t>(seed, {0, 150000, 100001, 7}));
        const auto other = person_index();
        const auto other_names = names_of(scenario, other);
        item.set(12, other).set(13, v % 4 == 3 ? other_names.first + 1 : other_names.first).set(14, other_names.second).set(15, other_names.common);
        item.set(16, pick<std::uint32_t>(seed, {0xffffffffu, 0xffffffffu, club()}));
        item.set(17, pick<std::uint32_t>(seed, {0, 6, 6, 1}));
        const auto second = person_index(), third = person_index();
        const auto second_names = names_of(scenario, second), third_names = names_of(scenario, third);
        item.set(18, pick<std::uint32_t>(seed, {0xffffffffu, second, second}));
        item.set(19, second_names.first).set(20, second_names.second).set(21, second_names.common);
        item.set(22, pick<std::uint32_t>(seed, {0xffffffffu, third, third}));
        item.set(23, third_names.first).set(24, third_names.second).set(25, third_names.common);
        // The flag word: single bits and the combinations the sentence tree tests.
        std::uint32_t flags = 0;
        for (const auto bit : {0x4u, 0x8u, 0x10u, 0x40u, 0x100u, 0x400u, 0x800u, 0x4000u, 0x20000u, 0x200000u, 0x40000000u})
            if (lcg(seed) % 3 == 0) flags |= bit;
        item.set(26, v % 3 == 0 ? pick<std::uint32_t>(seed, {0x10, 0x18, 0x4010, 0x4018, 0x200010, 0x40000000, 0x40000408, 0x40000400, 0x40000008, 0x40000040,
                                                           0x40000004, 0x40000800, 0x40000200, 0x4000, 0x200000, 0x204000, 0x40, 0x100, 0x140, 0x4, 0x800, 0x20000, 0x20010, 0x200018})
                                    : flags);
        item.set(27, pick<std::uint32_t>(seed, {0, 1, 3, 5, 10, 30, 100, 400, 3000, 0xfffffff0u, 0x7fffffff}));
        const auto count = pick<std::uint32_t>(seed, {0, 0, 1, 2, 3, 4, 0xffffffffu, 0x101});
        item.set(28, count);
        for (unsigned k = 0; k < 4; ++k) item.set(29 + k, club());
        std::vector<Write> extra;
        // The second club's league window against the current season's word.
        if (buyer < 500 && v % 3 != 0) extra = league_for(buyer, (v / 3) % 4);
        const auto flag = v % 4 == 0 ? 0 : pick<std::uint32_t>(seed, {1, 0x100, 0xff});
        auto described = label("4004", v);
        const auto word = [&](unsigned i) { for (auto w = item.writes.rbegin(); w != item.writes.rend(); ++w) if (w->address == item.at + 8 + 4 * i) return w->value; return 0u; };
        for (const unsigned i : {4u, 5u, 6u, 9u, 10u, 11u, 16u, 17u, 18u, 22u, 26u, 27u, 28u}) {
            char buffer[32];
            std::snprintf(buffer, sizeof buffer, " f%u=%x", i, word(i));
            described += buffer;
        }
        described += " flag=" + std::to_string(flag);
        cases.push_back(message_case(0x10404530, scenario, item, flag, described, extra));
    }
}
}

// MANAGER_MANAGER.CPP checker cases for the manager texts block (manager_manager_texts.hpp): 0x101b76b0,
// 0x101b7960 and 0x101b5370, the message texts the dispatcher at 0x102c5900 builds.
//
// The career snapshot's real clubs, people, module object and id table; the message record (type word, then
// the fields from +8) and the text sink are built in the scratch area. A fixed seed draws the fields the
// routines branch on: the club ids around the limits the first routine tests, the flag and kind bytes, the
// halfword of flags, the id table the second copies and the count its callee answers, the people and their
// flag byte for the third, and the answers of the club and person tests (other modules' routines) in some
// cases so each outcome runs. A few forced cases follow for the outcomes the draw does not reach: a name
// found in the text, no second club for a flagged person, and level 11 with one club. Memory writes are
// undone after each case.
#include "manager_manager_checks.hpp"
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t post_text = 0x101b76b0, people_text = 0x101b7960, appointment = 0x101b5370;
constexpr std::int32_t manager_object = -0x79f0, club_total = -0x7b50, reserve_slot = -0x7b04, id_table_slot = -0x5404,
                       people_total_slot = -0x7be4;
constexpr std::uint32_t club_flag = 0x100c7d00, person_flag = 0x100cbde0, people_ids = 0x1019c590, name_in_text = 0x1077dda0;
enum class Force { none, name_found, no_second_club, level_eleven };
constexpr std::uint32_t message_area = scratch_base + 0x6000, sink_area = scratch_base + 0x7000;
struct Rng {
    std::uint64_t state;
    std::uint32_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return std::uint32_t(state >> 33);
    }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    bool chance(unsigned percent) { return below(100) < percent; }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(unsigned(values.size()))); }
};

struct Builder {
    const Scenario& sc;
    Rng& rng;
    std::uint32_t clubs, people;
    Case item;
    void put(unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); }
    // Fields of the message from +8, a text for the string fields, and a zero sink.
    void message(std::initializer_list<std::uint32_t> fields) {
        unsigned n = 0;
        for (const auto value : fields) put(4, message_area + 8 + 4 * n++, value);
        for (unsigned k = n; k < 16; ++k) put(4, message_area + 8 + 4 * k, 0x41424300 + k);  // a level can index up to 12
        put(4, sink_area, 0);
    }
    std::uint32_t club() { return rng.below(clubs); }
    std::uint32_t person() { return rng.below(people); }
    void registers(std::uint32_t flag) {
        item.registers = {{3, sc.global(manager_object)}, {4, message_area}, {5, flag}, {6, sink_area}};
    }
    std::uint32_t flag() { return rng.pick<std::uint32_t>({0, 0, 1, 0xff, 0x100, 0x80, 5}); }
    void answers(unsigned percent) {
        if (rng.chance(percent)) item.answers.push_back({club_flag, rng.below(2)});
    }
};

Case first_case(Rng& rng, const Scenario& sc, unsigned n) {
    Builder b{sc, rng, sc.global(club_total), sc.people(), {}};
    b.item.entry = post_text;
    const auto reserve = rng.pick<std::uint32_t>({0, 1, 3, 10, 0x40000000u, 0xc0000000u, 2});
    b.put(4, sc.toc(reserve_slot), reserve);
    const auto limit = b.clubs - (reserve << 1);
    const auto club = rng.chance(60) ? rng.pick<std::uint32_t>({limit - 1, limit, limit + 1, 0u, b.clubs - 1}) % b.clubs : b.club();
    const auto flags = rng.pick<std::uint32_t>({0x400, 0x2, 0x1, 0x4, 0x80, 0x10, 0x20, 0x100, 0x8, 0x200, 0, 0x8000, 0xffff, 0x7fff, 0x308, 0x300});
    b.message({club, flags});
    b.registers(n % 4 == 3 ? 1 : b.flag());
    b.item.label = "post_text " + std::to_string(n);
    return b.item;
}

Case second_case(Rng& rng, const Scenario& sc, unsigned n, Force force = Force::none) {
    Builder b{sc, rng, sc.global(club_total), sc.people(), {}};
    b.item.entry = people_text;
    b.message({b.club(), rng.next()});
    const auto table = sc.toc(id_table_slot);
    const bool found = force == Force::name_found;
    for (unsigned k = 0; k < 15; ++k) b.put(4, table + 4 * k, !found && rng.chance(25) ? 0xffffffffu : b.person());
    const auto flag = found || n % 3 ? 1u : b.flag();
    b.registers(flag);
    b.answers(50);
    if (found) {
        // Three people, and the name search finds each name in the text.
        b.item.answers.push_back({people_ids, 3});
        b.item.answers.push_back({name_in_text, 1});
    } else if (rng.chance(60)) {
        b.item.answers.push_back({people_ids, rng.pick<std::uint32_t>({0, 1, 2, 3, 7, 15, 0xff, 0x80, 0x100})});
    }
    b.item.label = "post_people_text " + std::to_string(n) + (found ? " name found" : "");
    return b.item;
}

Case third_case(Rng& rng, const Scenario& sc, unsigned n, Force force = Force::none) {
    Builder b{sc, rng, sc.global(club_total), sc.people(), {}};
    b.item.entry = appointment;
    const auto flag = force != Force::none || n % 2 ? 1u : b.flag();
    const bool pair = std::int8_t(flag) != 0;
    // The person is among the last sixteen (a text of its own) in a quarter of the cases.
    const auto person = rng.chance(25) ? b.people - 1 - rng.below(16) : rng.below(b.people - 16);
    const auto word = [&] { return rng.pick<std::uint32_t>({0, 1, 5, 100, 0xffffffffu, 1999, 0x1234}); };
    auto second = rng.chance(8) ? rng.pick<std::uint32_t>({0xffffffffu, b.clubs, b.clubs + 5}) : b.club();
    auto first = rng.chance(15) ? 0xffffffffu : b.club();
    if (rng.chance(25)) first = second;
    auto kind = pair ? rng.pick<std::uint32_t>({0xffffffffu, 0, 1, 2, 3, 4, 1, 2, 3, 2, 2}) : rng.pick<std::uint32_t>({3, 0, 1, 3, 0xff});
    auto level = rng.pick<std::uint32_t>({0, 0xb, 0xb, 1, 2, 5, 10, 12, 0x100});
    const auto text_index = rng.below(3);
    auto person_byte = rng.below(2);
    if (force == Force::no_second_club) {
        // A kind without a sentence, then the flagged person's post with no second club.
        first = b.club(), second = 0xffffffffu, kind = rng.pick<std::uint32_t>({0, 4});
    } else if (force == Force::level_eleven) {
        // Kind 2 for a person with the byte set, one club for both, level 11.
        first = second = b.club(), kind = 2, level = 0xb, person_byte = 1;
    }
    const bool with_level = force == Force::level_eleven || rng.chance(70);
    if (pair) b.message({person, word(), word(), word(), first, second, kind, with_level ? level : text_index});
    else b.message({person, word(), word(), word(), 0, second, kind, text_index});
    b.put(1, sc.global(-0x7be0) + person * 0x70 + 0x62, person_byte);
    b.registers(flag);
    b.answers(60);
    if (force == Force::no_second_club) b.item.answers.push_back({person_flag, 1});
    else if (rng.chance(50)) b.item.answers.push_back({person_flag, rng.below(2)});
    b.item.label = std::string("appointment ") + std::to_string(n) + (pair ? " pair" : " single") +
                   (force == Force::no_second_club ? " no second club" : force == Force::level_eleven ? " level 11" : "");
    return b.item;
}
}

MANAGER_MANAGER_CASES("texts") {
    if (scenario.match || !scenario.global(club_total) || scenario.global(club_total) < 4 || scenario.people() < 40 ||
        !scenario.toc(id_table_slot) || !scenario.toc(people_total_slot))
        return;
    Rng rng{0x7e575eed};
    for (unsigned n = 0; n < 160; ++n) cases.push_back(first_case(rng, scenario, n));
    for (unsigned n = 0; n < 160; ++n) cases.push_back(second_case(rng, scenario, n));
    for (unsigned n = 0; n < 240; ++n) cases.push_back(third_case(rng, scenario, n));
    for (unsigned n = 0; n < 8; ++n) cases.push_back(second_case(rng, scenario, n, Force::name_found));
    for (unsigned n = 0; n < 8; ++n) cases.push_back(third_case(rng, scenario, n, Force::no_second_club));
    for (unsigned n = 0; n < 8; ++n) cases.push_back(third_case(rng, scenario, n, Force::level_eleven));
}
}

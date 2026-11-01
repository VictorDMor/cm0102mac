// The person ratings (0x101f9470, 0x101fb580) in the fixtures the plain cases of match_eng_rating_checks.cpp don't reach:
// national sides with the cup-tier floors, the final-round and cup branches of a knockout fixture, the owner tests of a
// fixture without id, a contract record that pulls rating_a, and the clamps of the largest sheet values.
#include "match_eng_checks.hpp"

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("rating_ratings") {
    const auto match = scenario.match;
    const auto fixture = scenario.r32(match + 0x3118);
    const auto r2 = std::uint32_t(scenario.s.r<4>(8));
    const auto comp = scenario.r32(fixture + 0x14);
    const auto home = scenario.r32(fixture + 0x1c), away = scenario.r32(fixture + 0x20);
    const auto flag_address = scenario.r32(r2 - 0x7a64);  // the global word 0x101f94xx tests before asking for a listed competition
    // Competition ids the availability test (0x102aed80) lists.
    std::vector<std::uint32_t> listed;
    for (const std::int32_t offset : {-0x7bc0, -0x7a40, -0x6948, -0x6b00}) listed.push_back(scenario.r32(scenario.r32(r2 + std::uint32_t(offset))));

    struct Shape {
        bool national = false;
        unsigned listed = 0;     // 0: the fixture's own competition id, else 1 + the index of a listed id
        std::uint32_t kind = 10, tier = 0x12, strength = 0x2000, code32 = 0, code30 = 0, band = 0;
        bool own_by_competition = false;
        std::uint32_t sheet_value = 0, sheet_level = 0;
    };
    for (const unsigned side : {0u, 1u}) {
        const unsigned index = side ? 7 : 3;
        const auto player = scenario.player(side, index);
        const auto person = scenario.r32(player + 0x6c);
        if (!person) continue;
        const auto sheet = scenario.r32(person + 0x14);
        const auto own = side ? away : home, opposing = side ? home : away;
        const auto person_club = scenario.r32(person + 0xc);
        const auto shape_memory = [&](const Shape& shape) {
            std::vector<Write> memory = {{4, flag_address, 1}, {1, fixture + 0x3a, shape.kind}, {2, comp + 0x18, shape.tier},
                                         {2, opposing + 0x1ec, shape.strength}, {2, fixture + 0x32, shape.code32}, {2, fixture + 0x30, shape.code30}};
            if (shape.national) memory.push_back({4, own, 0x7fffffff});
            if (shape.listed) memory.push_back({4, comp, listed[shape.listed - 1]});
            memory.push_back({4, comp + 8, shape.own_by_competition ? person_club : person_club ^ 1});
            memory.push_back({4, own + 0x2c, person_club ^ 2});
            if (shape.band) memory.push_back({2, sheet + 0xc, shape.band});
            if (shape.sheet_value) for (const unsigned field : {8u, 0xau, 0xcu}) memory.push_back({2, sheet + field, shape.sheet_value});
            if (shape.sheet_level) memory.push_back({2, sheet + 4, shape.sheet_level});
            return memory;
        };
        const auto rate = [&](const Shape& shape, double weight, std::uint32_t flag, std::vector<Write> extra, std::string label,
                              std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            auto memory = shape_memory(shape);
            memory.insert(memory.end(), extra.begin(), extra.end());
            cases.push_back({.entry = 0x101f9470, .registers = {{3, player}, {4, person}, {5, own}, {6, opposing}, {7, fixture}, {9, flag}, {10, 1}},
                             .memory = std::move(memory), .f1 = weight, .label = std::move(label), .answers = std::move(answers)});
        };

        // A listed competition raises the level to the tier; the national branch needs a high tier, weight and opposing strength.
        const std::pair<std::uint32_t, std::uint32_t> national_codes[] = {{0x41f, 0}, {0x426, 0}, {0x427, 0}, {0, 0x8c}, {0, 0x14}, {0, 0x78}, {0, 0x82},
                                                                         {0, 0x96}, {0, 0x9b}};
        for (const bool national : {false, true}) for (const unsigned listed_id : {0u, 1u, 4u}) for (const std::uint32_t tier : {0x5u, 0x10u, 0x11u, 0x12u, 0x30u})
            for (const double weight : {3.0, 6.5, 7.0, 7.5, 10.0}) for (const std::uint32_t strength : {0x157cu, 0x157du, 0x3000u})
                for (const auto& [code32, code30] : national_codes) for (const bool own_by_competition : {false, true}) {
                    if (!national && !own_by_competition && strength == 0x157c) continue;
                    Shape shape = {.national = national, .listed = listed_id, .tier = tier, .strength = strength, .code32 = code32, .code30 = code30,
                                   .own_by_competition = own_by_competition, .band = (code30 & 1) ? 1000u : 9000u};
                    rate(shape, weight, 0, {}, "ratings, national fixture");
                }
        // A fixture of kind 8: the final-round codes, the cup codes and the codes of neither.
        const std::uint32_t final_codes[] = {0x43c, 0x43d, 0x440, 0x448, 0x449};
        const std::uint32_t cup_kinds[] = {0x78, 0x7c, 0x7d, 0x82, 0x87, 0x88, 0x8c, 0x96, 0x9b, 0x9c};
        for (const std::uint32_t tier : {0x5u, 0x20u, 0x7fu}) for (const double weight : {3.0, 7.0, 7.5, 7.9, 8.0, 10.0}) for (const std::uint32_t strength : {0x157cu, 0x157du, 0x3000u})
            for (const bool own_by_competition : {false, true}) for (const std::uint32_t band : {1000u, 9000u}) {
                for (const auto code32 : final_codes) {
                    Shape shape = {.kind = 8, .tier = tier, .strength = strength, .code32 = code32, .own_by_competition = own_by_competition, .band = band};
                    rate(shape, weight, 0, {}, "ratings, final round");
                }
                for (const auto code30 : cup_kinds) {
                    Shape shape = {.kind = 8, .tier = tier, .strength = strength, .code30 = code30, .own_by_competition = own_by_competition, .band = band};
                    rate(shape, weight, 1, {}, "ratings, cup round");
                }
            }
        // A fixture without id: the owner tests against the club's and the competition's words.
        for (const bool null_competition : {false, true}) for (const std::uint32_t club_word : {0u, 1u, 2u}) for (const bool by_competition : {false, true}) {
            Shape shape = {.kind = 0, .own_by_competition = by_competition};
            std::vector<Write> extra = {{4, fixture, 0xfffffffe}, {4, own + 0x2c, club_word == 0 ? 0 : club_word == 1 ? person_club : person_club ^ 2}};
            if (null_competition) extra.push_back({4, fixture + 0x14, 0});
            rate(shape, 3.0, 0, extra, "ratings, fixture without id");
        }
        // The contract record that pulls rating_a: every kind in its low nibble, the form halfword on both sides of its cap.
        for (const std::uint32_t tier : {11u, 0x30u}) for (const std::uint32_t nibble : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 15u})
            for (const std::uint32_t form : {0u, 5u, 19u, 20u, 0x7fu, 0x80u, 0xffu}) for (const std::uint32_t strength : {0x1000u, 0x7fffu})
                for (const std::uint32_t rating : {100u, 5000u}) {
                    Shape shape = {.kind = 0, .tier = tier, .strength = strength};
                    rate(shape, 3.0, 0, {{1, person + 0x60, nibble}, {2, person + 0x5c, form}, {2, own + 0x1ec, strength}, {2, sheet + 0xa, rating}},
                         "ratings, contract record", {{0x10082f90, person}});
                }
        // The largest sheet values: every rating held to 10000 from above.
        for (const std::uint32_t value : {0x2700u, 0x2711u, 0x5000u, 0x7000u}) for (const std::uint32_t level : {0u, 100u, 0x7000u}) for (const std::uint32_t tier : {5u, 0x14u})
            for (const double weight : {3.0, 20.0}) for (const std::uint32_t flag : {0u, 1u}) {
                Shape shape = {.kind = 0, .tier = tier, .sheet_value = value, .sheet_level = level};
                rate(shape, weight, flag, {}, "ratings, large sheet");
                shape.national = true;
                rate(shape, weight, flag, {}, "ratings, large sheet, national");
            }
    }

    // 0x101fb580 over the same shapes.
    for (const unsigned side : {0u, 1u}) {
        const auto record = match + 0x7aac + side * 0xd8;
        const auto own = side ? away : home, opposing = side ? home : away;
        const auto person = scenario.r32(record + 0x20);
        if (!person) continue;
        const auto sheet = scenario.r32(person + 0x1c);
        const auto person_club = scenario.r32(person + 0xc);
        const auto rate = [&](std::uint32_t kind, std::uint32_t tier, std::uint32_t code32, std::uint32_t code30, bool national, bool by_competition,
                              std::uint32_t sheet_value, std::uint32_t strength, double weight, std::uint32_t flag, std::vector<Write> extra, std::string label) {
            std::vector<Write> memory = {{1, fixture + 0x3a, kind}, {2, comp + 0x18, tier}, {2, fixture + 0x32, code32}, {2, fixture + 0x30, code30},
                                         {2, opposing + 0x1ec, strength}, {4, comp + 8, by_competition ? person_club : person_club ^ 1},
                                         {4, own + 0x2c, person_club ^ 2}};
            if (national) memory.push_back({4, own, 0x7fffffff});
            if (sheet_value) for (const unsigned field : {0x28u, 0x2au, 0x2cu}) memory.push_back({2, sheet + field, sheet_value});
            memory.insert(memory.end(), extra.begin(), extra.end());
            cases.push_back({.entry = 0x101fb580, .registers = {{3, record}, {4, own}, {5, opposing}, {6, fixture}, {8, flag}, {9, 1}},
                             .memory = std::move(memory), .f1 = weight, .label = std::move(label)});
        };
        const std::uint32_t final_codes[] = {0x41e, 0x41f, 0x423, 0x426, 0x427};
        const std::uint32_t cup_kinds[] = {0x78, 0x7c, 0x7d, 0x82, 0x87, 0x88, 0x96, 0x9b, 0x9c};
        for (const std::uint32_t tier : {5u, 0x20u, 0x7fu}) for (const bool by_competition : {false, true}) for (const double weight : {3.0, 10.0}) {
            for (const auto code : final_codes) rate(8, tier, code, 0, false, by_competition, 0, 0x2000, weight, 0, {}, "side ratings, final round");
            for (const auto code : cup_kinds) rate(8, tier, 0, code, false, by_competition, 0, 0x2000, weight, 1, {}, "side ratings, cup round");
            rate(8, tier, 0, 0, true, by_competition, 0, 0x2000, weight, 1, {}, "side ratings, national, kind 8");
        }
        for (const bool null_competition : {false, true}) for (const std::uint32_t club_word : {0u, 1u, 2u}) for (const bool by_competition : {false, true}) {
            std::vector<Write> extra = {{4, fixture, 0xfffffffe}, {4, own + 0x2c, club_word == 0 ? 0 : club_word == 1 ? person_club : person_club ^ 2}};
            if (null_competition) extra.push_back({4, fixture + 0x14, 0});
            rate(0, 5, 0, 0, false, by_competition, 0, 0x2000, 3.0, 0, extra, "side ratings, fixture without id");
        }
        // The ceilings of the level against the sheet values: small and large values, in cup rounds that replace the level by the tier.
        for (const std::uint32_t value : {0x100u, 0x2000u, 0x2711u, 0x7000u}) for (const std::uint32_t tier : {5u, 0x14u, 0x7fu}) for (const double weight : {3.0, 20.0})
            for (const std::uint32_t kind : {0u, 8u}) for (const bool national : {false, true}) {
                rate(kind, tier, 0, 0x82, national, false, value, 0x2000, weight, 1, {}, "side ratings, sheet values");
                rate(kind, tier, 0x420, 0, national, true, value, 0x4000, weight, 0, {}, "side ratings, sheet values, final round");
            }
    }
}

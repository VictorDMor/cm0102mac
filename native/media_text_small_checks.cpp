// The short press texts' cases: a story item in scratch memory (its values at +8, four bytes
// each, read through 0x102cb570), real clubs and persons from the career, and an output text
// slot that starts empty.
#include "media_checks.hpp"
#include <algorithm>

using namespace cm::aot::media_checks;

namespace {
constexpr std::uint32_t item_at = scratch_base + 0x1000, output_at = scratch_base + 0x800;
using Values = std::vector<std::uint32_t>;
std::vector<Write> item_values(const Values& values) {
    std::vector<Write> writes;
    for (std::size_t i = 0; i < values.size(); ++i) writes.push_back({4, item_at + 8 + 4 * std::uint32_t(i), values[i]});
    writes.push_back({4, output_at, 0});
    return writes;
}
Case story(std::uint32_t entry, std::uint32_t full, const Values& values, std::string label) {
    return {.entry = entry, .registers = {{3, 0x31313131}, {4, item_at}, {5, full}, {6, output_at}}, .memory = item_values(values),
            .f1 = {}, .label = std::move(label)};
}
// A person row (0x70 bytes, table -0x7be0) with a contract or club, and the ids of its three linked
// records: the key 0x100cbde0 matches the row against.
struct Known { std::uint32_t index, first, second, third; };
std::vector<Known> known_persons(const Scenario& s, unsigned count) {
    std::vector<Known> found;
    const auto base = s.global(-0x7be0);
    for (std::uint32_t i = 1; found.size() < count && i < 20000; ++i) {
        const auto row = base + i * 0x70;
        const auto link = [&](std::uint32_t at) { return s.r32(row + at) ? s.r32(s.r32(row + at) + 0x20) : 0xffffffffu; };
        if (s.r32(row + 0x14) && s.r32(row + 0x28) && s.r32(row + 0x2c) && s.r32(row + 0x30) && link(0x28) < 40000 && link(0x2c) < 40000)
            found.push_back({i, link(0x28), link(0x2c), link(0x30)});
    }
    return found;
}
// A person row that is neither under contract nor at a club (+0x14 and +0x1c empty).
std::uint32_t idle_person(const Scenario& s) {
    const auto base = s.global(-0x7be0);
    for (std::uint32_t i = 1; i < 20000; ++i)
        if (!s.r32(base + i * 0x70 + 0x14) && !s.r32(base + i * 0x70 + 0x1c)) return i;
    return 0;
}
}

MEDIA_CASES("text_small") {
    // 0x10688fd0: the club's figure either side of each threshold, in the long and the short form.
    for (const std::uint32_t full : {1u, 0x100u, 0u, 0x200u, 0xffu})
        for (const std::uint32_t figure : {0u, 0xcb2u, 0xcb3u, 0x1a5eu, 0x1a5fu, 0xffffffffu, 0x7fffffffu})
            for (const unsigned club : {0u, 7u})
                cases.push_back(story(0x10688fd0, full, {club, figure, 9}, "finance"));
}

MEDIA_CASES("text_small apology") {
    const auto people = known_persons(scenario, 3);
    const auto& who = people.at(0);
    const auto idle = idle_person(scenario);
    const auto make = [&](std::uint32_t player, std::uint32_t key, std::uint32_t club, std::uint32_t kind, std::uint32_t tone) {
        return Values{player, key, who.second, who.third, club, kind, 0x77, tone};
    };
    std::vector<std::uint32_t> kinds{0xffffffffu, 0x79, 0x80, 0x78};
    for (std::uint32_t k = 0; k <= 0x26; ++k) kinds.push_back(k);
    const std::uint32_t tones[] = {0, 1, 2, 3, 0xffffffffu};
    for (const auto kind : kinds) {
        const bool tone_matters = kind == 0 || (kind >= 2 && kind <= 5) || kind == 0xd || kind == 0xe;
        for (const auto tone : tone_matters ? std::vector<std::uint32_t>(std::begin(tones), std::end(tones)) : std::vector<std::uint32_t>{0})
            cases.push_back(story(0x10649ad0, 1, make(who.index, who.first, 5, kind, tone), "apology"));
    }
    cases.push_back(story(0x10649ad0, 0x100, make(who.index, who.first, 5, 0, 0), "apology, byte flag is zero"));
    cases.push_back(story(0x10649ad0, 1, make(who.index, who.first + 1, 5, 1, 0), "apology, key differs"));
    cases.push_back(story(0x10649ad0, 1, make(idle, who.first, 5, 1, 0), "apology, idle person"));
    for (const std::uint32_t full : {0u, 0x200u, 0x1ffu})
        for (const std::uint32_t kind : {0x25u, 0x24u, 0u, 0xffffffffu, 0x78u})
            for (const std::uint32_t tone : {0u, 1u, 2u, 3u, 0xffffffffu, 0xfffffffeu, 0x80000000u})
                cases.push_back(story(0x10649ad0, full == 0x1ffu ? 0x1ffu : full, make(who.index, who.first, 5, kind, tone), "apology short"));
    cases.push_back(story(0x10649ad0, 0, make(who.index, who.first + 1, 5, 0, 0), "apology short, key differs"));
    cases.push_back(story(0x10649ad0, 0, make(idle, who.first, 5, 0, 0), "apology short, idle person"));
}

MEDIA_CASES("text_small exchange") {
    const auto people = known_persons(scenario, 4);
    const auto& who = people.at(0);
    const auto& second = people.at(1);
    const auto& helper = people.at(2);
    const auto idle = idle_person(scenario);
    // player, key, club, kind, flag, tone, second person and key, the assistant's marker and person.
    const auto make = [&](std::uint32_t kind, std::uint32_t flag, std::uint32_t tone, std::uint32_t marker = 0x420e9) {
        return Values{who.index, who.first, who.second, who.third, 6, kind, flag, tone, second.index, second.first, second.second, second.third,
                      marker, helper.index, helper.first, helper.second, helper.third};
    };
    std::vector<std::uint32_t> kinds{0xffffffffu, 0x82, 0x83, 0x36, 0x100};
    for (std::uint32_t k = 0; k <= 0x37; ++k) kinds.push_back(k);
    const std::uint32_t tones[] = {0, 1, 2, 3, 4, 0xffffffffu};
    for (const auto kind : kinds)
        for (const std::uint32_t flag : {0u, 1u, 2u})
            for (const auto tone : tones) {
                if (flag == 2 && tone > 1) continue;
                cases.push_back(story(0x10655030, 1, make(kind, flag, tone), "exchange"));
            }
    cases.push_back(story(0x10655030, 0x100, make(4, 0, 0), "exchange, byte flag is zero"));
    for (const std::uint32_t kind : {0u, 6u}) for (const std::uint32_t tone : {0u, 1u})
        cases.push_back(story(0x10655030, 1, make(kind, 0, tone, 0x420e8), "exchange, no assistant"));
    cases.push_back(story(0x10655030, 1, make(0, 0, 0, 0x420ea), "exchange, marker above"));
    cases.push_back(story(0x10655030, 1, make(0x1e, 0, 0, 0), "exchange, marker zero"));
    for (const auto& keys : {Values{who.index, who.first + 1, who.second, who.third}, Values{who.index, who.first, who.second + 1, who.third}, Values{who.index, who.first, who.second, who.third + 1}}) {
        auto values = make(0x12, 0, 0);
        std::copy(keys.begin(), keys.end(), values.begin());
        cases.push_back(story(0x10655030, 1, values, "exchange, key differs"));
        auto others = make(0x12, 0, 0);
        others[8] = second.index; others[9] = keys[1]; others[10] = keys[2]; others[11] = keys[3];
        cases.push_back(story(0x10655030, 1, others, "exchange, second key differs"));
    }
    auto idle_first = make(0x12, 0, 0); idle_first[0] = idle;
    cases.push_back(story(0x10655030, 1, idle_first, "exchange, idle player"));
    auto idle_second = make(0x12, 0, 0); idle_second[8] = idle;
    cases.push_back(story(0x10655030, 1, idle_second, "exchange, idle second person"));
    const auto short_values = [&](std::uint32_t kind, std::uint32_t flag, std::uint32_t tone) {
        return Values{who.index, who.first, who.second, who.third, 6, kind, flag, tone};
    };
    for (std::uint32_t kind = 0; kind <= 0x28; ++kind)
        for (const std::uint32_t flag : {0u, 1u, 2u}) for (const std::uint32_t tone : {0u, 1u, 2u, 0xffffffffu}) {
            if (flag == 2 && tone > 1) continue;
            cases.push_back(story(0x10655030, 0, short_values(kind, flag, tone), "exchange short"));
        }
    for (const std::uint32_t kind : {0x82u, 0x83u, 0xffffffffu, 0x100u})
        cases.push_back(story(0x10655030, 0, short_values(kind, 0, 0), "exchange short"));
    cases.push_back(story(0x10655030, 0, Values{who.index, who.first + 1, who.second, who.third, 6, 0, 0, 0}, "exchange short, key differs"));
    cases.push_back(story(0x10655030, 0, Values{idle, who.first, who.second, who.third, 6, 0, 0, 0}, "exchange short, idle"));
}

MEDIA_CASES("text_small reply") {
    // 0x10657b30: every incident the jump table knows and some beyond it, each with the tone
    // either side of 1 and 2 (as a signed byte), and the level and output guards.
    constexpr std::uint32_t buffer = scratch_base + 0x4000;
    const auto reply = [&](std::uint32_t kind, std::uint32_t tone, std::uint32_t output, std::uint32_t level, std::string label) {
        cases.push_back({.entry = 0x10657b30, .registers = {{3, 0x31313131}, {4, kind}, {5, tone}, {6, output}, {7, level}},
                         .memory = {}, .f1 = {}, .label = std::move(label)});
    };
    std::vector<std::uint32_t> kinds{0, 1, 2, 0x83, 0x84, 0xffffffffu, 0xffff8000u, 0x7fff, 0x10003, 0x10082};
    for (std::uint32_t kind = 3; kind <= 0x82; ++kind) kinds.push_back(kind);
    for (const auto kind : kinds)
        for (const std::uint32_t tone : {0u, 1u, 2u, 3u, 0xffffffffu, 0x102u, 0x180u, 0x7fu})
            reply(kind, tone, buffer, 0xf, "reply");
    for (const std::uint32_t level : {0xeu, 0u, 0x8000u, 0x1000fu, 0x7fffu, 0xffffffffu})
        reply(0x25, 2, buffer, level, "reply level");
    reply(0x25, 2, 0, 0xf, "reply without output");
    reply(0x25, 2, 0, 0xe, "reply without output, low level");
}

MEDIA_CASES("text_small requests") {
    const auto people = known_persons(scenario, 3);
    const auto& who = people.at(0);
    const auto& second = people.at(1);
    const auto idle = idle_person(scenario);
    // 0x1067d2f0: player (0) with key (1-3), club (4), kind (5), tone (6), detail (7), mood (8), flag (9).
    for (const std::uint32_t kind : {2u, 1u, 0xffffffffu}) for (const std::uint32_t mood : {3u, 1u}) for (const std::uint32_t tone : {2u, 3u, 0u})
        for (const std::uint32_t detail : {0u, 2u, 4u, 5u}) for (const std::uint32_t flag : {0u, 1u})
            cases.push_back(story(0x1067d2f0, 1, {who.index, who.first, who.second, who.third, 5, kind, tone, detail, mood, flag}, "request"));
    cases.push_back(story(0x1067d2f0, 0x100, {who.index, who.first, who.second, who.third, 5, 2, 2, 2, 3, 1}, "request, byte flag is zero"));
    cases.push_back(story(0x1067d2f0, 1, {who.index, who.first + 1, who.second, who.third, 5, 2, 2, 2, 3, 1}, "request, key differs"));
    cases.push_back(story(0x1067d2f0, 1, {idle, who.first, who.second, who.third, 5, 2, 2, 2, 3, 1}, "request, idle person"));
    // Short form: kind (5), mood (6), detail (7).
    for (const std::uint32_t kind : {2u, 1u, 0u}) for (const std::uint32_t mood : {2u, 1u}) for (const std::uint32_t detail : {4u, 0u})
        cases.push_back(story(0x1067d2f0, 0, {who.index, who.first, who.second, who.third, 5, kind, mood, detail}, "request short"));
    cases.push_back(story(0x1067d2f0, 0, {who.index, who.first + 1, who.second, who.third, 5, 2, 2, 2, 3}, "request short, key differs"));
    cases.push_back(story(0x1067d2f0, 0, {idle, who.first, who.second, who.third, 5, 2, 2, 2, 3}, "request short, idle person"));
    // 0x1067dc10: the second person is values 9-12; kind (5) and tone (6).
    const auto quarrel = [&](std::uint32_t kind, std::uint32_t tone) {
        return Values{who.index, who.first, who.second, who.third, 6, kind, tone, 7, 8, second.index, second.first, second.second, second.third};
    };
    for (const std::uint32_t kind : {0xffffffffu, 0u, 1u, 2u, 3u, 4u, 5u}) for (const std::uint32_t tone : {0xffffffffu, 0u, 1u, 2u, 3u, 4u})
        cases.push_back(story(0x1067dc10, 1, quarrel(kind, tone), "quarrel"));
    cases.push_back(story(0x1067dc10, 0x100, quarrel(3, 2), "quarrel, byte flag is zero"));
    for (const auto& keys : {Values{who.first + 1, who.second, who.third}, Values{who.first, who.second + 1, who.third}}) {
        auto values = quarrel(3, 2); std::copy(keys.begin(), keys.end(), values.begin() + 1);
        cases.push_back(story(0x1067dc10, 1, values, "quarrel, key differs"));
        auto others = quarrel(3, 2); others[10] = keys[0]; others[11] = keys[1]; others[12] = keys[2];
        cases.push_back(story(0x1067dc10, 1, others, "quarrel, second key differs"));
    }
    auto idle_first = quarrel(3, 2); idle_first[0] = idle;
    cases.push_back(story(0x1067dc10, 1, idle_first, "quarrel, idle person"));
    auto idle_second = quarrel(3, 2); idle_second[9] = idle;
    cases.push_back(story(0x1067dc10, 1, idle_second, "quarrel, idle second person"));
    for (const std::uint32_t quoted : {0u, 1u}) for (const std::uint32_t kind : {2u, 3u, 4u, 5u})
        cases.push_back(story(0x1067dc10, 0, {who.index, who.first, who.second, who.third, 9, kind, 9, quoted}, "quote"));
    cases.push_back(story(0x1067dc10, 0, {who.index, who.first + 1, who.second, who.third, 9, 3, 9, 1}, "quote, key differs"));
    cases.push_back(story(0x1067dc10, 0, {idle, who.first, who.second, who.third, 9, 3, 9, 1}, "quote, idle person"));
}

MEDIA_CASES("text_small replay") {
    // 0x1067b920 reads the item `id` logged on a day out of the log queue (0x102ccdd0): a queue of
    // our own in scratch memory holds the rows, so every kind and its fields can be given.
    constexpr std::uint32_t queue_at = scratch_base + 0x20000, rows_at = scratch_base + 0x21000, table_at = scratch_base + 0x30000,
        date_at = scratch_base + 0x40000;
    constexpr std::uint32_t id = 0x1234, day_slot = 5;
    const auto day_base = scenario.global(-0x7be4);
    const auto count = scenario.global(-0x7b50);
    const auto queue_variable = scenario.toc(-0x7bf0);
    const auto setup = [&](std::uint32_t kind, const Values& values, std::uint32_t row_id, std::uint32_t first_row) {
        std::vector<Write> writes{{4, queue_variable, queue_at}, {4, queue_at, rows_at}, {4, queue_at + 4, 4}, {4, queue_at + 8, table_at},
                                  {4, date_at, day_base + day_slot}, {4, table_at + (day_slot + 0x10) * 0xc, first_row},
                                  {4, rows_at, kind}, {4, rows_at + 0xdc, row_id}};
        for (std::size_t i = 0; i < values.size(); ++i) writes.push_back({4, rows_at + 8 + 4 * std::uint32_t(i), values[i]});
        return writes;
    };
    const auto replay = [&](std::uint32_t kind, const Values& values, std::string label, std::uint32_t found = 1) {
        cases.push_back({.entry = 0x1067b920, .registers = {{3, date_at}, {4, id}, {22, 0x22222222}, {27, 0x27272727}}, .memory = setup(kind, values, id, 0),
                         .f1 = {}, .label = std::move(label), .answers = {{0x1073e4b0, found}}});
    };
    Values values;
    for (std::uint32_t i = 0; i < 0x14; ++i) values.push_back(0x10 + i);
    for (const std::uint32_t kind : {0xfbcu, 0xbddu, 0xbdfu, 0xbe6u, 0xbe8u, 0u, 0xbdeu, 0xbe0u, 0xbe7u, 0xbe9u, 0xfbbu, 0xfbdu, 0xffffffffu})
        replay(kind, values, "replay");
    // Kind 0xfbc: the extra party is only read when value 15 is set, and kept when it indexes the table.
    for (const std::uint32_t flag : {0u, 1u, 0xffffffffu}) for (const std::uint32_t party : {0xffffffffu, 0xffffff00u, 0u, 1u, count - 2, count - 1, count, count + 1, 0x7fffffffu, 0x80000000u}) {
        auto custom = values; custom[0xf] = flag; custom[0x10] = party;
        replay(0xfbc, custom, "replay party");
    }
    replay(0xbe6, values, "replay, record refused", 0);
    replay(0xbe6, values, "replay, record refused, byte clear", 0x100);
    replay(0xbe6, values, "replay, record accepted by byte", 0x1ff);
    cases.push_back({.entry = 0x1067b920, .registers = {{3, 0}, {4, id}}, .memory = setup(0xbe6, values, id, 0), .f1 = {}, .label = "replay without date"});
    cases.push_back({.entry = 0x1067b920, .registers = {{3, date_at}, {4, id}}, .memory = setup(0xbe6, values, id + 1, 0), .f1 = {}, .label = "replay, id not logged"});
    cases.push_back({.entry = 0x1067b920, .registers = {{3, date_at}, {4, id}}, .memory = setup(0xbe6, values, id, 0xffffffffu), .f1 = {}, .label = "replay, day empty"});
    cases.push_back({.entry = 0x1067b920, .registers = {{3, date_at}, {4, 0}}, .memory = setup(0xbe6, values, 0, 0), .f1 = {}, .label = "replay, id zero"});
}

MEDIA_CASES("text_small dialog") {
    // 0x1067bea0 builds the pane from the working record: its values 1-7 live in an array of 8-byte entries
    // that the window record points at (+0x10); ours is in scratch memory.
    constexpr std::uint32_t array_at = scratch_base + 0x50000;
    const auto world = scenario.toc(-0x7bb0);
    const auto window = world + std::uint32_t(std::int16_t(scenario.s.memory.read<2>(world + 0x2c58))) * 0x2c0;
    const auto people = known_persons(scenario, 1);
    const auto& who = people.at(0);
    // The toolbox-facing calls: the pane and buttons are not part of the snapshot, so both sides get the same answer.
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> ui_answers{
        {0x10713050, 0}, {0x102a1070, 0}, {0x100c7a50, 0}, {0x100caeb0, 0}, {0x10744250, 0}, {0x107442f0, 0}, {0x1076d470, 3},
        {0x107198c0, 0x7001}, {0x1074bcd0, 0}, {0x107444e0, 0x1a00}};
    const auto dialog = [&](std::uint32_t kind, std::uint32_t answer, std::uint32_t staff, std::string label, std::uint32_t club = 3) {
        std::vector<Write> writes{{4, window + 0x10, array_at}};
        const std::uint32_t values[8] = {0, club, kind, answer, staff, who.first, who.second, who.third};
        for (std::uint32_t i = 0; i < 8; ++i) { writes.push_back({4, array_at + 8 * i + 0x14, values[i]}); writes.push_back({1, array_at + 8 * i + 0x18, 0}); }
        cases.push_back({.entry = 0x1067bea0, .registers = {{3, 0x31313131}}, .memory = writes, .f1 = {}, .label = std::move(label), .answers = ui_answers});
    };
    std::vector<std::uint32_t> kinds{0xffffffffu, 0x83, 0x84, 0x1000, 0x80000000u};
    for (std::uint32_t k = 0; k <= 0x82; ++k) kinds.push_back(k);
    for (const auto kind : kinds) for (const std::uint32_t staff : {0xffffffffu, 0u, who.index}) dialog(kind, 1, staff, "dialog");
    for (const std::uint32_t answer : {0u, 1u, 2u, 3u, 4u, 0xffffffffu}) dialog(0x44, answer, who.index, "dialog answers");
    dialog(0x5c, 2, 0xffffffffu, "dialog, continent");
    // A zero divisor stops both sides alike (the scale routine answers 0).
    {
        auto zero = ui_answers; for (auto& answer : zero) if (answer.first == 0x1076d470) answer.second = 0;
        std::vector<Write> writes{{4, window + 0x10, array_at}};
        const std::uint32_t values[8] = {0, 3, 0x44, 1, 0xffffffffu, who.first, who.second, who.third};
        for (std::uint32_t i = 0; i < 8; ++i) { writes.push_back({4, array_at + 8 * i + 0x14, values[i]}); writes.push_back({1, array_at + 8 * i + 0x18, 0}); }
        cases.push_back({.entry = 0x1067bea0, .registers = {{3, 0}}, .memory = writes, .f1 = {}, .label = "dialog, zero scale", .answers = zero});
    }
}

MEDIA_CASES("text_small event") {
    // 0x1067ca30: the controls live in a pane of our own (scratch memory, 0x17c bytes each from +0xa686c); the
    // working record's values 0-9 in an array of 8-byte entries; the log queue as in the replay cases.
    constexpr std::uint32_t pane_at = scratch_base + 0x100, array_at = scratch_base + 0x50000, queue_at = scratch_base + 0x20000,
        rows_at = scratch_base + 0x21000, table_at = scratch_base + 0x30000, date_at = scratch_base + 0x40000;
    constexpr std::uint32_t item_id = 0x4321, day_slot = 5;
    const auto world = scenario.toc(-0x7bb0);
    const auto window = world + std::uint32_t(std::int16_t(scenario.s.memory.read<2>(world + 0x2c58))) * 0x2c0;
    const auto day_base = scenario.global(-0x7be4);
    const auto queue_variable = scenario.toc(-0x7bf0);
    const auto default_command = world + 0x260000 - 0x7e62;
    const auto people = known_persons(scenario, 1);
    const auto& who = people.at(0);
    const auto control_record = [&](std::uint32_t id) { return pane_at + 0xc0000 - 0x5794 + id * 0x17c; };
    struct Setup { std::uint32_t item_kind, club, kind, answer, staff, other_club, label; };
    const auto memory = [&](const Setup& s, const std::vector<std::pair<std::uint32_t, std::pair<std::uint32_t, std::uint32_t>>>& controls,
                            std::uint32_t default_value, std::uint32_t flag) {
        std::vector<Write> writes{{4, window + 0x10, array_at}, {4, window + 0x24, pane_at}, {4, window + 4, date_at}, {1, world + 0x2c5f, flag},
                                  {2, default_command, default_value}, {4, queue_variable, queue_at}, {4, queue_at, rows_at}, {4, queue_at + 4, 4},
                                  {4, queue_at + 8, table_at}, {4, date_at, day_base + day_slot}, {4, table_at + (day_slot + 0x10) * 0xc, 0},
                                  {4, rows_at, s.item_kind}, {4, rows_at + 0xdc, item_id}};
        const std::uint32_t values[10] = {item_id, s.club, s.kind, s.answer, s.staff, 0, 0, 0, s.label, s.other_club};
        for (std::uint32_t i = 0; i < 10; ++i) { writes.push_back({4, array_at + 8 * i + 0x14, values[i]}); writes.push_back({1, array_at + 8 * i + 0x18, 0}); }
        for (const auto& [id, command] : controls) {
            writes.push_back({4, control_record(id), command.first});
            writes.push_back({4, control_record(id) + 0x40, command.second});
        }
        return writes;
    };
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> ui{{0x10741a80, 0}, {0x10742ec0, 0}, {0x102cd0c0, 1}, {0x102a54b0, 0}, {0x102a83a0, 0}};
    const auto event = [&](std::uint32_t control, const Setup& s, std::uint32_t default_value, std::string label, std::uint32_t flag = 0,
                           std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}, std::vector<Write> overrides = {}) {
        auto all = ui;
        all.erase(std::remove_if(all.begin(), all.end(), [&](const auto& a) { return std::any_of(answers.begin(), answers.end(), [&](const auto& b) { return a.first == b.first; }); }), all.end());
        all.insert(all.end(), answers.begin(), answers.end());
        auto writes = memory(s, {
            {0, {0x1485, 1}}, {1, {0x1485, 2}}, {2, {0x1485, 0}}, {3, {0x109b, 0}}, {4, {0x109c, 0}}, {5, {0x7777, 0}}, {6, {0x1485, 5}},
            {7, {0x1485, 0xffffffffu}}, {8, {0x1485, 3}}, {9, {0x10000, 0}}, {10, {0x11485, 1}}}, default_value, flag);
        writes.insert(writes.end(), overrides.begin(), overrides.end());
        cases.push_back({.entry = 0x1067ca30, .registers = {{3, control}, {4, 0x44444444}}, .memory = std::move(writes), .f1 = {}, .label = std::move(label), .answers = all});
    };
    const Setup base{0xbdf, 3, 0x1f, 2, who.index, 4, 7};
    for (const std::uint32_t id : {0u, 1u, 2u, 6u, 7u, 8u, 10u}) event(id, base, 0x1485, "answer control");
    event(0xffff, base, 0x1485, "answer by default"); event(0xffff, base, 0x109b, "ok by default"); event(0xffff, base, 0x109c, "cancel by default");
    event(0xffff, base, 0x7777, "other by default");
    event(6, base, 0x109c, "answer with a zero command's default");
    for (const std::uint32_t id : {9u, 10u}) event(id, base, 0x109b, "command with only the high bits");
    event(0, base, 0x1485, "flag", 1); event(3, base, 0x109b, "flag, ok", 0x81);
    event(4, base, 0x1485, "cancel"); event(5, base, 0x1485, "other key", 0, {{0x102a54b0, 0}}); event(5, base, 0x1485, "other key handled", 0, {{0x102a54b0, 1}});
    event(0x10005, base, 0x1485, "other key, high control bits");
    for (const std::uint32_t item_kind : {0xbdfu, 0xbe6u, 0xbe8u, 0xfbcu, 0xbddu, 0xbdeu, 0u})
        for (const std::uint32_t kind : {0x1fu, 3u}) for (const std::uint32_t staff : {0xffffffffu, 0xfffffffeu, who.index}) for (const std::uint32_t other : {0xffffffffu, 4u})
            event(3, {item_kind, 3, kind, 2, staff, other, 7}, 0x1485, "ok");
    event(3, {0xbdf, 3, 0x1f, 0, who.index, 4, 7}, 0x1485, "ok without an answer");
    // The reaction tree over the item kind, past the kinds above: each side of 0xbdd, 0xbe6, 0xbe7, 0xbe9 and 0xfbc.
    for (const std::uint32_t item_kind : {0xbdcu, 0xbe5u, 0xbe7u, 0xbe9u, 0xbeau, 0xfbbu, 0xfbdu, 0x1000u, 0x7fffffffu, 0xffffffffu})
        for (const std::uint32_t kind : {0x1fu, 3u}) event(3, {item_kind, 3, kind, 2, who.index, 4, 7}, 0x1485, "ok, item kind");
    // A fetch that fails (the logged row is for another item): the item stays as the begin routine left it.
    event(3, {0xbdf, 3, 0x1f, 2, who.index, 4, 7}, 0x1485, "ok, item not logged", 0, {}, {{4, rows_at + 0xdc, item_id + 1}});
}

namespace {
// A club of the career whose competition link (+0x2c) is set, with the club record's other fields real.
std::uint32_t linked_club(const Scenario& s) {
    for (unsigned i = 1; i < 300; ++i) if (s.r32(s.club(i) + 0x2c) && s.r32(s.club(i) + 0x2c) < 0x20000000) return i;
    return 1;
}
}

MEDIA_CASES("text_small outlet") {
    // 0x10656450: the item's values (counted up by a stack byte: 16 and 17 name the outlet, or 29 and 30 after a
    // match's fields), real people, and a stand-in for the window's owner in scratch memory.
    const auto people = known_persons(scenario, 3);
    const auto& who = people.at(0);
    const auto& helper = people.at(1);
    const auto idle = idle_person(scenario);
    const auto club_number = linked_club(scenario);
    const auto club = scenario.club(club_number);
    const auto world = scenario.toc(-0x7bb0);
    const auto window = world + std::uint32_t(std::int16_t(scenario.s.memory.read<2>(world + 0x2c58))) * 0x2c0;
    const auto link = scenario.r32(club + 0x2c);
    const auto rival = linked_club(scenario) + 1;
    constexpr std::uint32_t owner_at = scratch_base + 0x600, pane_at = scratch_base + 0x700, rating_at = scratch_base + 0x800;
    struct Spec {
        std::uint32_t full = 1, kind = 3, tone = 0, captain = 0, assisted = 0, assistant = 0xffffffffu, reaction = 1, match = 0, home = 2, away = 1,
            outlet = 1, outlet_id = 0xffffffffu;
        bool local = false, agrees = false, known = true, idle_player = false, high = false, even = true, wrap = false, nobody = false, helper_known = true,
            unlinked = false, no_pane = false;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {};
    };
    const auto build = [&](const Spec& spec, std::string label) {
        Values v(31, 0);
        v[0] = spec.idle_player ? idle : who.index; v[1] = spec.known ? who.first : who.first + 1; v[2] = who.second; v[3] = who.third; v[4] = club_number;
        v[5] = spec.captain; v[6] = spec.kind; v[7] = spec.tone; v[8] = spec.assisted; v[10] = spec.reaction; v[11] = spec.assistant;
        v[12] = spec.helper_known ? helper.first : helper.first + 1; v[13] = helper.second; v[14] = helper.third; v[15] = spec.match;
        if (spec.match) {
            v[17] = 5; v[18] = 2001; v[23] = 7; v[24] = rival; v[25] = spec.home; v[26] = spec.away; v[27] = 11;
            v[29] = spec.outlet; v[30] = spec.outlet_id;
        } else { v[16] = spec.outlet; v[17] = spec.outlet_id; }
        if (spec.assistant < 0x100000 && spec.assistant != 0xffffffffu) v[11] = helper.index;
        auto memory = item_values(v);
        const std::uint32_t owner_value = (spec.local || spec.agrees || spec.no_pane) && !spec.nobody ? owner_at : 0;
        memory.push_back({4, window + 4, owner_value});
        memory.push_back({4, club + 0x90, spec.nobody ? 0u : spec.agrees ? owner_value : 0x77000u});
        if (spec.unlinked) memory.push_back({4, club + 0x2c, 0});
        if (spec.no_pane) {
            // The window's owner has no pane: it is local when his own +0xc is the club's +0x2c record.
            memory.push_back({4, owner_at + 0x24, 0});
            memory.push_back({4, owner_at + 0xc, spec.local ? link : link + 4});
        } else if (spec.local) {
            memory.push_back({4, owner_at + 0x24, pane_at});
            memory.push_back({4, pane_at + 0x2c, link});
        } else if (owner_value) {
            memory.push_back({4, owner_at + 0x24, pane_at});
            memory.push_back({4, pane_at + 0x2c, link + 4});
        }
        if (spec.kind == 0x52) {
            // The draw: entries and seed chosen for the parity wanted; the rating bound read from a row of our own.
            const auto table_at = scratch_base + 0x9000;
            const auto end = table_at + 0x100;
            memory.push_back({2, table_at + 2, spec.even ? 4u : 5u});
            memory.push_back({4, scenario.toc(-0x7b84), spec.wrap ? end : table_at});
            memory.push_back({4, scenario.toc(-0x7b88), end});
            memory.push_back({4, scenario.toc(-0x7b8c), table_at});
            memory.push_back({4, scenario.toc(-0x7b90), 0});
            if (spec.wrap) for (std::uint32_t i = 1; i <= 4; ++i) memory.push_back({2, table_at + 2 * i, 0x1111u * i});
            const auto row = scenario.global(-0x7be0) + who.index * 0x70;
            memory.push_back({4, row + 0xc, rating_at});
            memory.push_back({2, rating_at + 0xac, spec.high ? 0x2000u : 0x100u});
        }
        cases.push_back({.entry = 0x10656450, .registers = {{3, 0x31313131}, {4, item_at}, {5, spec.full}, {6, output_at}}, .memory = std::move(memory),
                         .f1 = {}, .label = std::move(label), .answers = spec.answers});
    };
    // The long form: who answers (nobody, the manager, an assistant), the tone, the captain flag, a match or not.
    for (const std::uint32_t assisted : {0u, 1u}) for (const std::uint32_t assistant : {0xffffffffu, 0u}) for (const std::uint32_t tone : {0u, 1u})
        for (const std::uint32_t captain : {0u, 1u}) for (const std::uint32_t match : {0u, 1u}) {
            if (!match) { build({.tone = tone, .captain = captain, .assisted = assisted, .assistant = assistant}, "outburst"); continue; }
            for (const auto [home, away] : {std::pair<std::uint32_t, std::uint32_t>{2, 1}, {1, 1}, {1, 2}, {0xffu, 1}})
                build({.tone = tone, .captain = captain, .assisted = assisted, .assistant = assistant, .match = 1, .home = home, .away = away}, "outburst, match");
        }
    for (const std::uint32_t reaction : {0u, 1u, 2u, 3u, 0xffffffffu}) for (const bool agrees : {false, true})
        build({.reaction = reaction, .agrees = agrees}, "outburst reaction");
    for (const std::uint32_t tone : {0u, 1u}) for (const std::uint32_t match : {0u, 1u}) for (const bool even : {true, false}) for (const bool high : {true, false})
        build({.kind = 0x52, .tone = tone, .match = match, .even = even, .high = high}, "outburst blast");
    build({.kind = 0x52, .wrap = true}, "outburst blast, table wraps"); build({.kind = 0x52, .wrap = true, .even = false, .high = true}, "outburst blast, table wraps");
    build({.kind = 0x52, .assisted = 1, .assistant = 0u}, "outburst blast, assisted");
    // The outlet: every kind, local to the manager or not, named by the outlet routine or by the text.
    for (const std::uint32_t outlet : {1u, 2u, 3u, 4u, 5u, 6u, 0u, 7u, 0xffffffffu}) for (const bool local : {false, true}) for (const std::uint32_t outlet_id : {0xffffffffu, 3u}) {
        build({.outlet = outlet, .outlet_id = outlet_id, .local = local}, "outlet");
        build({.full = 0, .outlet = outlet, .outlet_id = outlet_id, .local = local}, "outlet, short form");
    }
    // A window owner with no pane: local when his +0xc is the club's +0x2c, or not.
    for (const bool equal : {false, true}) for (const std::uint32_t outlet_id : {0xffffffffu, 3u}) for (const std::uint32_t full : {1u, 0u})
        build({.full = full, .outlet = 3, .outlet_id = outlet_id, .local = equal, .no_pane = true}, "outlet, owner without pane");
    build({.known = false}, "outburst, key differs"); build({.idle_player = true}, "outburst, idle player");
    build({.assisted = 1, .assistant = 0u, .agrees = true}, "outburst, assisted, agrees");
    // The short form: the player's remark alone.
    for (const std::uint32_t kind : {0x52u, 3u}) for (const std::uint32_t assisted : {0u, 1u}) for (const std::uint32_t assistant : {0xffffffffu, 0u})
        for (const std::uint32_t tone : {0u, 1u}) for (const std::uint32_t captain : {0u, 1u})
            build({.full = 0, .kind = kind, .tone = tone, .captain = captain, .assisted = assisted, .assistant = assistant}, "outburst short");
    for (const bool agrees : {false, true}) for (const std::uint32_t assisted : {0u, 1u})
        build({.full = 0, .assisted = assisted, .agrees = agrees}, "outburst short, fan");
    build({.full = 0, .known = false}, "outburst short, key differs"); build({.full = 0, .idle_player = true}, "outburst short, idle player");
    build({.full = 0x100}, "outburst, byte flag is zero");
    build({.nobody = true, .agrees = true}, "outburst, nobody"); build({.full = 0, .assisted = 1, .nobody = true, .agrees = true}, "outburst short, nobody");
    build({.assisted = 1, .assistant = 0u, .helper_known = false}, "outburst, assistant key differs");
    for (const std::uint32_t found : {0u, 1u, 0x100u})
        build({.match = 1, .answers = {{0x1029eb20, found}}}, "outburst, match link answered");
    build({.unlinked = true}, "outburst, club without link"); build({.full = 0, .unlinked = true}, "outburst short, club without link");
}

MEDIA_CASES("text_small outlet") {
    // The match's goals are the item's values 24 (home) and 25 (away), after the rival club's index (value 23); the cases
    // above leave the home goals at the club number plus one and vary only the away goals. Here the away goals exceed the
    // home goals, with and without an assistant answering, for both tones and the captain flag.
    const auto people = known_persons(scenario, 3);
    const auto& who = people.at(0);
    const auto& helper = people.at(1);
    const auto club_number = linked_club(scenario);
    const auto club = scenario.club(club_number);
    const auto world = scenario.toc(-0x7bb0);
    const auto window = world + std::uint32_t(std::int16_t(scenario.s.memory.read<2>(world + 0x2c58))) * 0x2c0;
    for (const std::uint32_t assisted : {0u, 1u}) for (const std::uint32_t assistant : {0xffffffffu, 0u}) for (const std::uint32_t tone : {0u, 1u})
        for (const std::uint32_t captain : {0u, 1u}) for (const std::uint32_t home_goals : {0u, 1u, 2u}) for (const std::uint32_t away_goals : {3u, 0x7fu}) {
            Values v(31, 0);
            v[0] = who.index; v[1] = who.first; v[2] = who.second; v[3] = who.third; v[4] = club_number;
            v[5] = captain; v[6] = 3; v[7] = tone; v[8] = assisted; v[10] = 1; v[11] = assistant == 0u ? helper.index : assistant;
            v[12] = helper.first; v[13] = helper.second; v[14] = helper.third; v[15] = 1;
            v[17] = 5; v[18] = 2001; v[23] = 7; v[24] = home_goals; v[25] = away_goals; v[27] = 11;
            v[29] = 1; v[30] = 0xffffffffu;
            auto memory = item_values(v);
            memory.push_back({4, window + 4, 0});
            memory.push_back({4, club + 0x90, 0x77000u});
            cases.push_back({.entry = 0x10656450, .registers = {{3, 0x31313131}, {4, item_at}, {5, 1}, {6, output_at}}, .memory = std::move(memory),
                             .f1 = {}, .label = "outburst, match lost at home", .answers = {}});
        }
}

// Direct calls of the setup block from a real match: argument variants, the
// early returns, the statistics fixture (id -2), a club that cannot field eleven
// players (the assertion report), the draws with unusual limits and the player
// assessment loop with varied records.
#include "match_eng_checks.hpp"
#include <algorithm>
#include <bit>
#include <random>

using namespace cm::aot::match_eng_checks;

MATCH_ENG_CASES("setup") {
    constexpr std::uint32_t toc = 0x108bba90;
    const auto match = scenario.match;
    const auto fixture = scenario.r32(match + 0x3118);
    const auto home = scenario.r32(fixture + 0x1c), away = scenario.r32(fixture + 0x20);
    const auto target = scenario.r32(match + 0x79f4);
    // A global: the variable behind the TOC entry.
    const auto global = [&](std::int32_t offset) { return scenario.r32(toc + std::uint32_t(offset)); };
    using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
    // The message object's own file work (0x10204d70, and with the silent flag 0x10746280 / 0x10742740) opens
    // files through Carbon imports a match snapshot has no host for. Both sides skip those calls.
    const Answers files = {{0x10204d70, 0}, {0x10746280, 0}, {0x10742740, 0}};
    // A scratch record in the match's own memory for what the world would hand out (a player slot the
    // setup loop never reaches).
    const auto object = scenario.player(1, 39), scratch = scenario.player(1, 38);
    // The manager answers: no ban from the club's side (0x10494d20), and the player's world object (0x10082f90).
    Answers world = files;
    world.insert(world.end(), {{0x10494d20, 0}, {0x10082f90, object}});
    const auto with = [](Answers base, Answers more) { base.insert(base.end(), more.begin(), more.end()); return base; };

    // The snapshot's fixture has no first-player flag (+0x40); the lineup is only judged when it has one.
    const auto flagged = [&](std::vector<Write> memory) {
        const bool given = std::any_of(memory.begin(), memory.end(), [&](const Write& w) { return w.address == fixture + 0x40; });
        if (!given) memory.push_back({1, fixture + 0x40, 1});
        return memory;
    };
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers,
                         std::vector<Write> memory, std::string label) {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {},
                         .label = std::move(label)});
    };
    const auto run = [&](std::uint32_t inactive, std::uint32_t message, std::uint32_t silent, std::vector<Write> memory,
                         std::string label, const Answers& answers) {
        cases.push_back({.entry = 0x101d1760, .registers = {{3, match}, {4, fixture}, {5, inactive}, {6, message}, {7, silent}},
                         .memory = flagged(std::move(memory)), .f1 = {},
                         .label = std::move(label) + (silent ? " silent" : "") + (inactive ? " inactive" : ""), .answers = answers});
    };
    const auto setup = [&](std::uint32_t inactive, std::uint32_t message, std::uint32_t silent, std::vector<Write> memory,
                           std::string label) { run(inactive, message, silent, std::move(memory), std::move(label), world); };

    for (const std::uint32_t inactive : {0u, 1u}) for (const std::uint32_t silent : {0u, 1u}) {
        setup(inactive, 0, silent, {}, "new message object");
        if (target) setup(inactive, target, silent, {}, "given message object");
    }
    // The world's own answers left to the game's code: the manager bans the club's players, or the
    // player has no world object, so the loop skips the player.
    run(0, 0, 0, {}, "manager as the snapshot has it", files);
    run(0, 0, 0, {}, "no world object", with(files, {{0x10494d20, 0}}));
    run(0, 0, 0, {{4, global(-0x7b54), 0}}, "no manager", with(files, {{0x10082f90, object}, {0x10494d20, 0}}));
    setup(0, 0, 0, {{1, match + 0x7930, 1}}, "played elsewhere");
    setup(0, 0, 0, {{4, fixture, 0xfffffffe}}, "statistics fixture");
    setup(0, 0, 0, {{4, fixture, 0xffffffff}}, "friendly id");
    setup(0, 0, 0, {{4, fixture, 7}}, "other id");
    for (const std::uint32_t options : {0x00u, 0xffu, 0x9au, 0xa9u}) setup(0, 0, 0, {{1, fixture + 0x4e, options}}, "option nibbles");
    for (const std::uint32_t mode : {0x00u, 0x04u, 0x08u, 0x0cu}) setup(0, 0, 0, {{1, fixture + 0x4f, mode}}, "mode bits");
    setup(0, 0, 0, {{1, fixture + 0x40, 0}}, "no first flag");
    // A fixture of the statistics kind has no competition record; any other kind needs one.
    setup(0, 0, 0, {{4, fixture, 0xfffffffe}, {4, fixture + 0x14, 0}}, "statistics fixture without competition");

    // The fixture's mode (0x10433da0, answered) and the kind of match (0x10434ad0): modes 2 and 3 draw the
    // byte at +0xe0b8 from a coin; the seed of the draws decides the coin.
    const auto seed = global(-0x7b90);
    for (const std::uint32_t mode : {0u, 1u, 2u, 3u}) for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u}) {
        if (mode < 2 && kind > 1) continue;
        for (const std::uint32_t draw_seed : {0u, 1u}) {
            if (mode < 2 && draw_seed) continue;
            setup(0, 0, 0, {{4, seed, draw_seed}}, "mode " + std::to_string(mode) + " kind " + std::to_string(kind) + " seed " + std::to_string(draw_seed));
            cases.back().answers = with(world, {{0x10433da0, mode}, {0x10434ad0, kind}});
        }
    }
    // The bearing table is built once per program run: the first time (flag clear) and when it is already built.
    setup(0, 0, 0, {{1, toc - 0x3912, 0}}, "first run builds the bearing table");
    setup(0, 0, 0, {{1, toc - 0x3912, 0}, {1, toc - 0x3911, 0}}, "table flags clear");
    setup(0, 0, 0, {{1, toc - 0x3911, 0}}, "bearing table built");
    setup(0, 0, 0, {{1, toc - 0x3911, 2}}, "bearing table flag 2");

    // The competition link (a global pointing at a word): with it, a fixture of a competition draws its
    // structure from the table by the fixture's index; the statistics fixture and a negative index attach none.
    const auto linked = global(-0x7428), table = global(-0x7ab4);
    for (const std::uint32_t id : {0xfffffffeu, 7u}) for (const std::uint32_t index : {0xffffu, 0u, 1u, 0x7fffu}) {
        for (const std::uint32_t weight : {0u, 100u, 500u}) {
            std::vector<Write> memory = {{4, linked, 1}, {4, table, scratch}, {4, fixture, id}, {2, fixture + 0x2e, index}};
            for (std::uint32_t entry = 0; entry < 2; ++entry) memory.push_back({2, scratch + entry * 0x2c + 0x16, weight});
            setup(0, 0, 0, memory, "linked competition " + std::to_string(id) + " index " + std::to_string(index));
        }
    }
    setup(0, 0, 0, {{4, linked, 0}}, "competition not linked");

    // The teams' ratings (+0x1ec) set the weights of the rating limits (0x104b0b90).
    const std::uint32_t ratings[] = {0, 1, 0x100, 0x7d0, 0xcb2, 0x1388, 0x2710, 0x4e20, 0x7fff};
    for (const auto a : ratings) for (const auto b : ratings) {
        if ((a + b) % 3 == 1) continue;
        setup(0, 0, 0, {{2, home + 0x1ec, a}, {2, away + 0x1ec, b}}, "ratings " + std::to_string(a) + " " + std::to_string(b));
    }
    // The competition's nation (fixture +0x18) against the home club's, and the two clubs' own (+0x3c, nation records).
    {
        const auto home_nation = scenario.r32(home + 0x3c), away_nation = scenario.r32(away + 0x3c);
        for (const auto home_value : {home_nation, away_nation}) for (const auto away_value : {home_nation, away_nation})
            for (const auto competition_value : {home_nation, away_nation})
                setup(0, 0, 0, {{4, home + 0x3c, home_value}, {4, away + 0x3c, away_value}, {4, fixture + 0x18, competition_value}}, "nations");
    }
    // The weights of the rating limits, as the world would give them (0x104b0b90 answered, the match keeps
    // what is stored): the first is the divisor, so it is never zero.
    for (const std::uint32_t first : {1u, 100u, 1000u, 0xfffffff0u}) for (const std::uint32_t second : {0u, 90u, 1000u, 0xffffffffu})
        for (const std::uint32_t third : {0u, 10u, 40000u, 60000u, 100000u, 0xfffe7960u}) {
            if ((first + second + third) % 3 == 1) continue;
            setup(0, 0, 0, {{4, match + 0x7c5c, first}, {4, match + 0x7c60, second}, {4, match + 0x7c64, third}}, "weights");
            cases.back().answers = with(world, {{0x104b0b90, 0}});
        }
    // The team records: an id out of range (0x100c7d00) for the home club (setup stops), for the away club
    // (its players are skipped), no squad pointer at +0x2c, and the world's answer 1 from the check.
    // 0x100c7d00 answers 1 for an id from G2 - 2 * G1 up (G1, G2 the game's counts at -0x7b04 and -0x7b50).
    const std::uint32_t limit_id = scenario.r32(global(-0x7b50)) - 2 * scenario.r32(global(-0x7b04));
    for (const std::uint32_t id : {limit_id, limit_id + 1, limit_id - 1}) {
        setup(0, 0, 0, {{4, home, id}}, "home id out of range");
        setup(0, 0, 0, {{4, away, id}}, "away id out of range");
    }
    setup(0, 0, 0, {{4, away + 0x2c, 0}}, "away without +0x2c");
    setup(0, 0, 0, {{4, home + 0x2c, 0}}, "home without +0x2c");
    // Clubs that cannot field eleven eligible players (every squad entry cleared).
    for (const auto club : {home, away}) {
        std::vector<Write> memory;
        for (std::uint32_t at = 0; at < 0x40; at += 4) memory.push_back({4, scenario.r32(club + 0x90) + at, 0});
        memory.push_back({4, club + 0x90, 0});
        setup(0, 0, 0, memory, "club without players");
        setup(0, 0, 1, memory, "club without players, silent");
    }
    // The computer picks too few: the shortage is reported through the assertion for each fixture id (the
    // competition's name comes from the statistics kinds -1 and -2 or the competition record). Fewer than
    // five picked in all sets the match inactive unless it is silent.
    for (const std::uint32_t picked : {0u, 2u, 3u, 10u}) for (const std::uint32_t silent : {0u, 1u})
        for (const std::uint32_t id : {0xfffffffeu, 0xffffffffu, 0xfffffffdu, 7u}) {
            run(0, 0, silent, {{4, fixture, id}}, "picked " + std::to_string(picked) + " id " + std::to_string(id),
                with(world, {{0x102065f0, picked}}));
        }
    // Only one side short: the other's count stays what the snapshot's squad gives.
    for (const std::uint32_t picked : {10u, 11u, 20u})
        run(0, 0, 0, {}, "picked " + std::to_string(picked), with(world, {{0x102065f0, picked}}));

    // The assessment of the players (one pass over twenty per side): random records around the
    // thresholds the code tests. The game's data is a mix of values; each case draws its own.
    std::mt19937 random(0x5e7a);
    const auto pick = [&](std::initializer_list<std::uint32_t> values) { return values.begin()[random() % values.size()]; };
    const auto profile = [&](std::vector<Write>& memory, bool eventful) {
        for (unsigned side = 0; side < 2; ++side) for (unsigned index = 0; index < 20; ++index) {
            const auto me = scenario.player(side, index);
            const auto record = scenario.r32(me + 0x6c);
            if (!record) continue;
            const auto attributes = scenario.r32(record + 0x14);
            const auto club = scenario.r32(me + 0x70);
            if (random() % 11 == 0) memory.push_back({1, me + 0x19, 0xff});
            memory.push_back({2, me + 0x1c2, pick({0, 1, 2, 3, 4, 0x80, 0x100, 0x400, 0x800, 0xfff, 0x2aa})});
            for (const unsigned at : {0x5eu, 0x62u, 0x63u, 0x66u, 0x67u, 0x69u})
                memory.push_back({1, record + at, pick({0, 1, 2, 5, 6, 7, 0xa, 0xc, 0xe, 0xf, 0x10, 0x14, 0x15, 0x19, 0x1a, 0x7f, 0x80, 0xff})});
            memory.push_back({2, record + 0x5c, pick({0, 1, 6, 0x10, 0x16, 0x17, 0x18, 0x19, 0x1e, 0x20, 0x21, 0x30, 0x7f, 0xff, 0x8000})});
            memory.push_back({4, record + 0x4c, random() % 7});
            memory.push_back({4, record + 0x50, random() % 7});
            if (attributes) {
                // Some players have no strong attribute (0x14, or 0xf and above), so the transfer flags are clear.
                const bool quiet = random() % 3 == 0;
                for (const unsigned at : {0xfu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u, 0x17u, 0x18u, 0x19u, 0x25u, 0x3cu})
                    memory.push_back({1, attributes + at, quiet ? pick({0, 1, 5, 6, 7, 0xa, 0xb, 0xe}) :
                                                                  pick({0, 1, 5, 6, 7, 0xa, 0xb, 0xf, 0x10, 0x14, 0x14, 0x14, 0x15, 0x19})});
                memory.push_back({2, attributes + 0xa, pick({0, 0x100, 0xcb2, 0xcb3, 0x186a, 0x186b, 0x1b57, 0x1b58, 0x1d4c, 0x1d4d, 0x2710, 0x7fff})});
            }
            if (eventful) {
                // A strong club with a poor player in form trouble: the form event's conditions.
                memory.push_back({1, record + 0x5e, 0});
                memory.push_back({2, record + 0x5c, pick({0x10, 0x14, 0x16, 0x21, 0x28, 0x30})});
                memory.push_back({1, record + 0x66, 0x14});
                memory.push_back({1, record + 0x67, pick({0xe, 0xc, 4})});
                if (attributes) memory.push_back({2, attributes + 0xa, 0x100});
            }
            if (side == 1 && index >= 17 && random() % 6 == 0) memory.push_back({4, record + 0x14, 0});
            if (club) {
                memory.push_back({1, club + 0x2f, pick({0, 1, 2, 5, 6, 8, 0xc, 0x14})});
                memory.push_back({2, club + 0xa, pick({0, 0x100, 0xe00, 0xea6, 0xea7, 0x1388, 0x2710, 0x7fff})});
            }
        }
        memory.push_back({4, object + 0x48, pick({0, 1, 0x40, 0x41, 0x1000, 0x8040})});
        memory.push_back({4, object + 0x50, pick({0, 1, 0x02000000, 0x2f7a0b, 0x100, 0x02000001})});
        for (const auto club : {home, away}) {
            if (eventful) { memory.push_back({2, club + 0x1ec, pick({0xcb3, 0x2710, 0x7fff})}); continue; }
            memory.push_back({2, club + 0x1ec, pick({0, 0x100, 0x7d0, 0xcb2, 0xcb3, 0x1388, 0x2710, 0x7fff})});
            if (random() % 4 == 0) memory.push_back({4, club + 0x90, 0});
        }
    };
    for (unsigned n = 0; n < 400; ++n) {
        std::vector<Write> memory;
        const bool eventful = n % 3 == 0;
        profile(memory, eventful);
        // The draws of this case: the seed and the cursor's table entry move every roll.
        memory.push_back({4, seed, random()});
        Answers answers = world;
        if (n % 5 == 0) answers.push_back({0x1037fa70, pick({0, 1, 0x40, 0x41})});
        // The world's verdicts on the player: a ban or a loan flag (-3, -5) and his value against the roll.
        if (n % 2) answers.push_back({0x100da0b0, random() % 2});
        if (n % 3) answers.push_back({0x100d9520, random() % 2});
        // The grade lookups (0x10303560) leave their float argument as it is.
        // The single the lookups start from (the pool's 6.5 at +0x14) is what they return: a range of grades.
        if (n % 4 == 1) {
            answers.push_back({0x10303560, 0});
            static constexpr float grades[] = {0.5f, 3.0f, 5.0f, 6.5f, 12.0f, 40.0f};
            memory.push_back({4, global(-0x52c8) + 0x14, std::bit_cast<std::uint32_t>(grades[(n / 4) % 6])});
        }
        if (n % 2 == 0) answers.push_back({0x10718130, pick({0, 100, 251, 1000, 0xffffffff})});
        cases.push_back({.entry = 0x101d1760, .registers = {{3, match}, {4, fixture}, {5, 0}, {6, 0}, {7, 0}},
                         .memory = flagged(std::move(memory)), .f1 = {}, .label = "players " + std::to_string(n), .answers = answers});
    }

    // The computer can't pick eleven.
    cases.push_back({.entry = 0x101d1760, .registers = {{3, match}, {4, fixture}, {5, 0}, {6, 0}, {7, 0}}, .memory = flagged({}), .f1 = {},
                     .label = "ten picked", .answers = with(world, {{0x102065f0, 10}})});
    cases.push_back({.entry = 0x101d1760, .registers = {{3, match}, {4, fixture}, {5, 0}, {6, 0}, {7, 1}}, .memory = flagged({}), .f1 = {},
                     .label = "ten picked, silent", .answers = with(world, {{0x102065f0, 10}})});
    // The draws: zero, small, large and negative limits.
    for (const std::uint32_t limit : {0u, 1u, 2u, 7u, 100u, 0xffffu, 0x10000u, 0x12345u, 1000000u, 0xffffffffu, 0xffff0001u,
                                      0xfffefffeu, 0x80000000u, 0x7fffffffu})
        add(0x101d3c90, {{3, limit}}, {}, "draw");
    // The first player of a side: which of the fixture's two flag bytes (+0x40, +0x41) selects the slot.
    for (const std::uint32_t side : {0u, 1u}) for (const std::uint32_t first : {0u, 1u, 0xffu}) for (const std::uint32_t second : {0u, 1u, 0x80u})
        add(0x101d3b60, {{3, match}, {4, side}}, {{1, fixture + 0x40, first}, {1, fixture + 0x41, second}}, "side player");
    // The destructor needs a match it may take apart: null, and the flag for freeing.
    add(0x101d3e50, {{3, 0}, {4, 1}}, {}, "null match");
    add(0x101d3e50, {{3, match}, {4, 0}}, {}, "destroy");
    add(0x101d3e50, {{3, match}, {4, 1}}, {}, "destroy and free");
    add(0x101d3e50, {{3, match}, {4, 0}}, {{1, match + 0x7913, 0}}, "no owned target");
    // An owned message object slot that is empty, and no message object at all.
    add(0x101d3e50, {{3, match}, {4, 0}}, {{1, match + 0x7913, 1}, {4, match + 0x79f4, 0}}, "owned but empty");
    add(0x101d3e50, {{3, match}, {4, 0}}, {{4, match + 0x3114, 0}}, "no message object");
    add(0x101d16a0, {{3, match}}, {}, "construct");
}

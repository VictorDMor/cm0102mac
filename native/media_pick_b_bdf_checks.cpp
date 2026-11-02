// Cases for the bdf item (0x106891b0): every story of the switch with draws steering each remainder,
// the club tiers, the manager's moods, the id sources, the player and match fields, and the posters.
#include "media_checks.hpp"

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
constexpr std::uint32_t entry_bdf = 0x106891b0;
constexpr std::uint32_t club_check = 0x100c7d00, club_gate = 0x1014c2e0, history_has = 0x1069e700, manager_effect = 0x106584d0,
    queue_id = 0x102b7450, queue_known = 0x102aed80, competition_club = 0x10463a90,
    competition_slots = 0x1049d330, match_text = 0x1030d4a0, match_fields = 0x1030dcd0;
constexpr std::uint32_t table_at = scratch_base + 0x8000, player_at = scratch_base + 0x1000, contract_at = scratch_base + 0x1100,
    link24_at = scratch_base + 0x1140, link20_at = scratch_base + 0x1150, ids_at = scratch_base + 0x1200,
    match_at = scratch_base + 0x1400, queue_word = scratch_base + 0x1500;
Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
void append(Writes& to, const Writes& more) { to.insert(to.end(), more.begin(), more.end()); }
// The random table: the cursor steps through uint16 entries, the seed is added to each.
Writes table(const Scenario& s, const std::vector<std::uint32_t>& entries, std::uint32_t seed = 0, bool refill = false) {
    Writes writes;
    for (std::size_t i = 0; i < entries.size(); ++i) writes.push_back(h16(table_at + 2 * std::uint32_t(i + 1), entries[i]));
    const auto end = table_at + 0x100;
    writes.push_back(w32(s.toc(-0x7b84), refill ? end : table_at));
    writes.push_back(w32(s.toc(-0x7b88), end));
    writes.push_back(w32(s.toc(-0x7b8c), table_at));
    writes.push_back(w32(s.toc(-0x7b90), seed));
    if (refill) for (std::uint32_t i = 1; i <= 4; ++i) writes.push_back(h16(table_at + 2 * i, 0x1111u * i));
    return writes;
}
std::vector<std::pair<unsigned, std::uint32_t>> arguments(std::uint32_t self, std::uint32_t club, std::uint32_t story, std::uint32_t player,
                                                          std::uint32_t match, std::uint32_t margin, std::uint32_t value) {
    return {{3, self}, {4, club}, {5, story}, {6, player}, {7, match}, {8, margin}, {9, value}, {10, 0xa1a1a1a1}};
}
struct Club { std::uint32_t at, owner, link, third, manager, manager_link; };
}

MEDIA_CASES("pick_b_bdf") {
    const auto self = scenario.global(-0x7b94);
    std::vector<Club> clubs;
    for (unsigned i = 0; clubs.size() < 3 && i < 400; ++i) {
        const auto at = scenario.club(i);
        const auto manager = scenario.r32(at + 0x90);
        const Club club{at, scenario.r32(at + 0x2c), scenario.r32(at + 0x30), scenario.r32(at + 0x3c), manager, manager ? scenario.r32(manager + 0x1c) : 0};
        if (club.owner && club.link && club.third && club.manager && club.manager_link) clubs.push_back(club);
    }
    if (clubs.empty()) return;
    const auto& club = clubs[0];
    const auto today = scenario.toc(-0x7b80), copy_date = scenario.toc(-0x7a9c);
    const auto year = scenario.r32(today) & 0xffff;
    const auto rival_a = scenario.global(-0x7a1c), rival_b = scenario.global(-0x7a10);

    // The fixture every case starts from: a player and match of our own in the scratch area, a date copy a year
    // older than today, and a club rating, owner and link that pass.
    const auto base = [&](const Club& c) {
        Writes out{h16(copy_date + 2, year - 1), h16(c.at + 0x1ec, 0x1400)};
        append(out, {w32(player_at + 0x14, contract_at), h16(contract_at + 0xa, 0x1000), w32(player_at, 77), w32(player_at + 0x24, link24_at),
                     w32(link24_at, 0x2468), w32(player_at + 0x20, link20_at), w32(link20_at, 0x1357), b8(player_at + 0x60, 3),
                     b8(player_at + 0x62, 4), b8(player_at + 0x5e, 1), w32(player_at + 0x28, ids_at), w32(player_at + 0x2c, ids_at + 0x40),
                     w32(player_at + 0x30, ids_at + 0x80), w32(ids_at + 0x20, 11), w32(ids_at + 0x60, 12), w32(ids_at + 0xa0, 13),
                     b8(match_at + 0x49, 2), b8(match_at + 0x4a, 1), w32(match_at + 0x1c, c.at), w32(match_at + 0x20, 0x5555),
                     w32(queue_word, 0x3131)});
        return out;
    };
    const Answers quiet{{match_text, 0}, {match_fields, 0}};
    const auto add = [&](const Club& c, std::uint32_t story, Writes memory, Answers answers, const char* label, std::uint32_t player = player_at,
                         std::uint32_t match = match_at, std::uint32_t margin = 5, std::uint32_t value = 0x99) {
        answers.insert(answers.end(), quiet.begin(), quiet.end());
        cases.push_back({.entry = entry_bdf, .registers = arguments(self, c.at, story, player, match, margin, value), .memory = std::move(memory),
                         .f1 = {}, .label = label, .answers = std::move(answers)});
    };
    const auto with = [&](const Club& c, std::initializer_list<Writes> more) {
        auto out = base(c);
        for (const auto& part : more) append(out, part);
        return out;
    };
    std::vector<std::uint32_t> stories;
    for (std::uint32_t s = 0x3c; s <= 0x63; ++s) stories.push_back(s);
    for (const std::uint32_t s : {0x3bu, 0x64u, 0u, 0x7fu, 0x80u, 0xffu, 0x1005au, 0x1ff03du}) stories.push_back(s);

    // The pick of every story, with draws steering each remainder, the club's tier and the posting.
    const std::vector<std::uint32_t> picks{0, 1, 2, 3, 5, 6};
    const std::vector<std::vector<std::uint32_t>> tails{
        {0, 0, 0, 0, 0, 0, 0}, {1499, 5, 1, 1, 1, 2999, 1}, {750, 20, 2, 2, 0, 5, 0}, {1499, 50, 0, 3, 2, 100, 2},
        {0, 80, 1, 0, 1, 2000, 1}, {1499, 0, 2, 1, 0, 1, 0}, {300, 7, 0, 1, 3, 2999, 2}, {1499, 8, 1, 0, 1, 0, 1},
        {200, 34, 2, 1, 2, 1, 0}, {1000, 35, 1, 1, 0, 2999, 1}, {1499, 59, 0, 0, 0, 50, 2}, {0, 60, 0, 1, 1, 2999, 0},
        {1499, 99, 1, 0, 2, 2000, 1}, {2, 150, 2, 1, 1, 3, 2}};
    for (const auto story : stories)
        for (const auto pick : picks)
            for (const auto& tail : tails) {
                std::vector<std::uint32_t> entries{pick};
                entries.insert(entries.end(), tail.begin(), tail.end());
                for (const std::uint32_t checked : {0u, 1u}) {
                    if (checked && story != 0x5a && story != 0x44 && (pick || &tail != &tails[1])) continue;
                    add(club, story, with(club, {table(scenario, entries)}), {{club_check, checked}, {club_gate, 0}, {queue_id, queue_word}, {queue_known, 1}},
                        "story");
                }
            }

    // The date check: a copy a year ahead, the same year with the day later, equal and earlier, and a year behind.
    for (const std::int32_t day_today : {100, -100, 0, 0x7fff, -0x8000})
        for (const std::int32_t day_copy : {100, -100, 99, 101, 0, -1, 0x7fff, -0x8000, 1})
            for (const std::int32_t delta : {-1, 0, 1})
                add(club, 0x3d, with(club, {{h16(today, std::uint32_t(day_today)), h16(copy_date, std::uint32_t(day_copy)), h16(copy_date + 2, year + std::uint32_t(delta))}, table(scenario, {1, 0, 0, 0})}),
                    {{club_check, 0}, {club_gate, 0}}, "date");

    // The refusals, one at a time, with the callee that decides answered both ways.
    const auto refuse = [&](const char* label, Writes memory, Answers answers, std::uint32_t story = 0x3d) {
        add(club, story, with(club, {memory, table(scenario, {1, 0, 0, 0, 0, 0})}), std::move(answers), label);
    };
    add(Club{0, 0, 0, 0, 0, 0}, 0x3d, base(club), {}, "no club");
    for (const std::uint32_t check : {0u, 1u}) {
        refuse("no link", {w32(club.at + 0x30, 0)}, {{club_check, check}, {club_gate, 0}});
        refuse("no owner", {w32(club.at + 0x2c, 0)}, {{club_check, check}, {club_gate, 0}});
        refuse("no third", {w32(club.at + 0x3c, 0)}, {{club_check, check}, {club_gate, 0}});
        refuse("third empty", {w32(club.third + 0x10, 0)}, {{club_check, check}, {club_gate, 0}});
        refuse("third empty, a tier-3 story", {w32(club.third + 0x10, 0)}, {{club_check, check}, {club_gate, 0}}, 0x5a);
        refuse("no manager", {w32(club.at + 0x90, 0)}, {{club_check, check}, {club_gate, 0}});
        refuse("manager without link", {w32(club.manager + 0x1c, 0)}, {{club_check, check}, {club_gate, 0}});
        for (const std::uint32_t story : {0x3du, 0x5au, 0x44u, 0x5cu}) refuse("checked story", {}, {{club_check, check}, {club_gate, 1}, {queue_id, queue_word}, {queue_known, 1}}, story);
    }
    for (const std::uint32_t known : {0u, 1u, 0x100u})
        refuse("history", {}, {{club_check, 0}, {club_gate, 0}, {history_has, known}});
    for (const std::uint32_t rating : {0xd00u, 0xdabu, 0xdacu, 0xdadu, 0x8000u, 0x7fffu})
        refuse("player standing", {h16(contract_at + 0xa, rating)}, {{club_check, 0}, {club_gate, 0}});
    refuse("player without contract", {w32(player_at + 0x14, 0)}, {{club_check, 0}, {club_gate, 0}});

    // The tier: the club's rating against a draw (both sides), the gate, then the roll over 100.
    const std::vector<std::uint32_t> rolls100{0, 5, 7, 8, 20, 34, 35, 59, 60, 99, 160, 0xfff0};
    for (const std::uint32_t rating : {0x1000u, 0x1193u, 0x1194u, 0x1400u, 0x176fu, 0x2000u, 0x8000u, 0x7fffu})
        for (const std::uint32_t rem : {0u, 1u, 750u, 1499u, 1500u, 0xffffu})
            for (const auto roll : rolls100)
                for (const std::uint32_t third : {0u, 1u, 2u})
                    for (const std::uint32_t checked : {0u, 1u}) {
                        if (checked && (third || roll != 5)) continue;
                        for (const std::uint32_t story : {0x3du, 0x5au}) {
                            if (checked && story == 0x3d) continue;
                            add(club, story, with(club, {{h16(club.at + 0x1ec, rating)}, table(scenario, {1, rem, roll, third, 1, 5})}),
                                {{club_check, checked}, {club_gate, 0}, {queue_id, queue_word}, {queue_known, 1}}, "tier");
                        }
                    }
    for (const std::uint32_t seed : {0u, 0x80000001u, 0xfffffff0u})
        for (const std::uint32_t entry : {0u, 1u, 7u, 0xffffu})
            add(club, 0x3d, with(club, {table(scenario, {entry, entry, entry, entry, entry, entry}, seed)}), {{club_check, 0}, {club_gate, 0}}, "seeded draws");
    add(club, 0x3d, with(club, {table(scenario, {7, 9, 4, 4, 4}, 0x1fff, true)}), {{club_check, 0}, {club_gate, 0}}, "wraps");

    // The manager's moods (two flags) and the effect on the pick.
    for (const std::uint32_t b63 : {0x10u, 0xfu, 5u, 0x80u})
        for (const std::uint32_t b69 : {5u, 6u, 0xeu, 0xfu, 0x20u, 0x80u})
            for (const std::uint32_t standing : {0x1676u, 0x1677u, 0x8000u})
                for (const std::uint32_t story : {0x3du, 0x3eu, 0x50u}) {
                    const auto writes = Writes{b8(club.manager + 0x63, b63), b8(club.manager + 0x69, b69), h16(club.manager_link + 0x2a, standing)};
                    add(club, story, with(club, {writes, table(scenario, {1, 0, 0, 0, 0, 0})}), {{club_check, 0}, {club_gate, 0}}, "moods");
                }
    for (const std::uint32_t effect : {0u, 1u, 0x7fu, 0x80u, 0x1ffu, 0xfffffff0u})
        for (const std::uint32_t gate : {0u, 1u})
            for (const std::uint32_t story : {0x3du, 0x3eu, 0x5eu, 0x50u, 0x51u, 0x52u, 0x5fu})
                add(club, story, with(club, {table(scenario, {1, 0, 0, 0, 0, 0})}), {{club_check, 0}, {club_gate, gate}, {manager_effect, effect}}, "effect");

    // The story id: the queue's (0x5a, 0x44), the club's own (0x5c) and the link's.
    for (const std::uint32_t story : {0x5au, 0x44u})
        for (const std::uint32_t known : {0u, 1u})
            add(club, story, with(club, {table(scenario, {1, 0, 0, 0, 0, 0}), {w32(queue_word, 1), w32(queue_word + 0x10, 2)}}),
                {{club_check, 1}, {club_gate, 0}, {queue_id, queue_word}, {queue_known, known}}, "queue id");
    for (const std::uint32_t id : {0xffffffffu, 0u, 3u, 7u})
        add(club, 0x5c, with(club, {table(scenario, {1, 0, 0, 0, 0, 0}), {w32(club.at + 0x14, id)}}), {{club_check, 0}, {club_gate, 0}}, "own id");
    for (const std::uint32_t id : {0u, 1u, 5u, 0xffffffffu})
        add(club, 0x3d, with(club, {table(scenario, {1, 0, 0, 0, 0, 0}), {w32(club.link, id)}}), {{club_check, 0}, {club_gate, 0}}, "link id");
    for (const auto& other : clubs)
        for (const std::uint32_t story : {0x3du, 0x41u, 0x5cu})
            add(other, story, with(other, {table(scenario, {1, 0, 0, 0, 0, 0})}), {{club_check, 0}, {club_gate, 0}}, "other clubs");

    // The competition: whether the id names the club, and the slots.
    for (const std::uint32_t home : {club.at, 0u, 0x4444u})
        for (const std::uint32_t slots : {0u, 1u})
            add(club, 0x3d, with(club, {table(scenario, {1, 0, 0, 0, 0, 0})}), {{club_check, 0}, {club_gate, 0}, {competition_club, home}, {competition_slots, slots}}, "competition");

    // The player: none, and each shape of its links, kinds and flag.
    add(club, 0x3d, with(club, {table(scenario, {1, 0, 0, 0, 0, 0})}), {{club_check, 0}, {club_gate, 0}}, "no player", 0);
    add(club, 0x3f, with(club, {table(scenario, {1, 1, 0, 0, 0, 0})}), {{club_check, 0}, {club_gate, 0}}, "no player, pick", 0);
    for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 0x80u, 0xffu, 0x7fu})
        for (const std::uint32_t flag : {0u, 1u, 0x80u})
            add(club, 0x3d, with(club, {table(scenario, {1, 0, 0, 0, 0, 0}), {b8(player_at + 0x62, kind), b8(player_at + 0x5e, flag)}}),
                {{club_check, 0}, {club_gate, 0}}, "player kinds");
    for (const std::uint32_t link24 : {0u, link24_at})
        for (const std::uint32_t link20 : {0u, link20_at})
            for (const std::uint32_t kind60 : {0u, 0xbu, 0x80u})
                for (const std::uint32_t flag : {0u, 1u}) {
                    add(club, 0x3d, with(club, {table(scenario, {1, 0, 0, 0, 0, 0}), {w32(player_at + 0x24, link24), w32(player_at + 0x20, link20), b8(player_at + 0x60, kind60), b8(player_at + 0x5e, flag)}}),
                        {{club_check, 0}, {club_gate, 0}}, "player links");
                }
    add(club, 0x3d, with(club, {table(scenario, {1, 0, 0, 0, 0, 0}), {w32(player_at + 0x28, 0)}}), {{club_check, 0}, {club_gate, 0}}, "player id link");

    // The match: none, unplayed, each side winning, drawn, with and without the club on that side.
    add(club, 0x3d, with(club, {table(scenario, {1, 0, 0, 0, 0, 0})}), {{club_check, 0}, {club_gate, 0}}, "no match", player_at, 0);
    for (const std::uint32_t home : {0xffu, 0u, 1u, 2u, 0x80u, 0x7fu})
        for (const std::uint32_t away : {0xffu, 0u, 1u, 2u, 0x80u, 0x7fu})
            for (const std::uint32_t side : {0u, 1u, 2u}) {
                const auto first = side == 1 ? club.at : 0x7777u, second = side == 2 ? club.at : 0x8888u;
                add(club, 0x3d, with(club, {table(scenario, {1, 0, 0, 0, 0, 0}), {b8(match_at + 0x49, home), b8(match_at + 0x4a, away), w32(match_at + 0x1c, first), w32(match_at + 0x20, second)}}),
                    {{club_check, 0}, {club_gate, 0}}, "match", player_at, match_at, 5, 0x1234);
            }
    for (const std::uint32_t margin : {0u, 0x14u, 0x15u, 0x80u, 0xffu, 0x1ffu, 0x7fu})
        add(club, 0x3d, with(club, {table(scenario, {1, 0, 0, 0, 0, 0})}), {{club_check, 0}, {club_gate, 0}}, "margins", player_at, match_at, margin, 0xffff8000u);

    // The posting: by gate and parity, the stand posters, the 3000 draw with each story, the owner and rival.
    const std::vector<std::uint32_t> post_stories{0x3c, 0x3d, 0x3e, 0x41, 0x50, 0x51, 0x5a, 0x5c, 0x5d, 0x44, 0x61, 0x62, 0x63, 0x64, 0x42};
    const std::vector<std::vector<std::uint32_t>> post_draws{{1, 0, 0, 0, 0, 0, 0, 0}, {1, 1, 20, 1, 1, 0, 0, 0}, {1, 1499, 99, 1, 0, 2999, 1, 2}, {0, 800, 5, 2, 0, 1500, 0, 1}};
    for (const auto story : post_stories)
        for (const std::uint32_t rating : {0x1000u, 0x1388u, 0x1770u, 0x1964u, 0x2328u, 0x2400u, 0x7fffu, 0xffff8000u})
            for (const std::uint32_t stand : {0x10u, 0xfu, 0u})
                for (const std::uint32_t margin : {0u, 0x10u, 0x15u, 0x80u})
                    for (const std::uint32_t gate : {0u, 1u})
                        for (const auto& draws : post_draws)
                            add(club, story, with(club, {{h16(club.at + 0x1ec, rating), h16(club.link + 0x18, stand)}, table(scenario, draws)}),
                                {{club_check, story == 0x5a || story == 0x44}, {club_gate, gate}, {queue_id, queue_word}, {queue_known, 1}}, "posters", player_at, match_at, margin);
    for (const std::uint32_t link_null : {0u, 1u})
        for (const std::uint32_t story : {0x3du, 0x3cu, 0x5cu, 0x41u, 0x5au})
            for (const std::uint32_t draw : {0u, 1u, 2u, 3u})
                add(club, story, with(club, {{w32(club.at + 0x30, link_null ? 0 : club.link)}, table(scenario, {1, 0, 0, 0, draw, draw, draw, draw})}),
                    {{club_check, story == 0x5a}, {club_gate, 0}, {queue_id, queue_word}, {queue_known, 1}}, "posters, link");
    for (const std::uint32_t rating : {0x1387u, 0x1388u, 0x176fu, 0x1770u, 0x1963u, 0x1964u, 0x2000u})
        for (const std::uint32_t owner_rival : {rival_a, rival_b, 3u})
            for (const std::uint32_t story : {0x3cu, 0x3eu})
                add(club, story, with(club, {{h16(club.at + 0x1ec, rating), h16(club.link + 0x18, 5), w32(club.owner + 0x68, owner_rival)}, table(scenario, {1, 0, 0, 0, 0, 0})}),
                    {{club_check, 0}, {club_gate, 0}}, "owner and rival");
    for (const std::uint32_t one : {0u, 1u, 2u, 7u})
        for (const std::uint32_t two : {0u, 1u, 2u, 7u})
            add(club, 0x3c, with(club, {{h16(club.at + 0x1ec, 0x2000), h16(club.link + 0x18, 5), w32(club.owner + 0x68, one), w32(scenario.toc(-0x7a1c), two), w32(scenario.toc(-0x7a10), one)}, table(scenario, {1, 0, 0, 0, 0, 0})}),
                {{club_check, 0}, {club_gate, 0}}, "rival slots");

    // The history and the follow-up.
    for (const std::uint32_t story : {0x3du, 0x3eu, 0x50u, 0x5eu, 0x5au, 0x60u})
        for (const std::uint32_t gate : {0u, 1u})
            for (const std::uint32_t effect : {0u, 1u, 0x80u})
                add(club, story, with(club, {table(scenario, {1, 0, 0, 0, 0, 0})}),
                    {{club_check, story == 0x5a}, {club_gate, gate}, {manager_effect, effect}, {queue_id, queue_word}, {queue_known, 1}}, "history");
}

// ---- Closing the last uncovered outcomes (central coverage run). The history already holds the story (0x1069e700 is a media
// routine, so its answer is ignored in the central run: a one-row history is laid out in the press object), a negative sample
// for the tier's third draw (a seed word below zero), and the margin below the lively mark for the stories 0x50 and 0x51.
MEDIA_CASES("pick_b_bdf_closure") {
    const auto self = scenario.global(-0x7b94);
    std::vector<Club> clubs;
    for (unsigned i = 0; clubs.size() < 3 && i < 400; ++i) {
        const auto at = scenario.club(i);
        const auto manager = scenario.r32(at + 0x90);
        const Club club{at, scenario.r32(at + 0x2c), scenario.r32(at + 0x30), scenario.r32(at + 0x3c), manager, manager ? scenario.r32(manager + 0x1c) : 0};
        if (club.owner && club.link && club.third && club.manager && club.manager_link) clubs.push_back(club);
    }
    if (clubs.empty()) return;
    const auto& club = clubs[0];
    const auto today = scenario.toc(-0x7b80), copy_date = scenario.toc(-0x7a9c);
    const auto year = scenario.r32(today) & 0xffff;
    const auto fixture = [&]() {
        Writes out{h16(copy_date + 2, year - 1), h16(club.at + 0x1ec, 0x1400)};
        append(out, {w32(player_at + 0x14, contract_at), h16(contract_at + 0xa, 0x1000), w32(player_at, 77), w32(player_at + 0x24, link24_at),
                     w32(link24_at, 0x2468), w32(player_at + 0x20, link20_at), w32(link20_at, 0x1357), b8(player_at + 0x60, 3),
                     b8(player_at + 0x62, 4), b8(player_at + 0x5e, 1), w32(player_at + 0x28, ids_at), w32(player_at + 0x2c, ids_at + 0x40),
                     w32(player_at + 0x30, ids_at + 0x80), w32(ids_at + 0x20, 11), w32(ids_at + 0x60, 12), w32(ids_at + 0xa0, 13),
                     b8(match_at + 0x49, 2), b8(match_at + 0x4a, 1), w32(match_at + 0x1c, club.at), w32(match_at + 0x20, 0x5555),
                     w32(queue_word, 0x3131)});
        return out;
    };
    const auto add = [&](std::uint32_t story, std::initializer_list<Writes> more, Answers answers, const char* label, std::uint32_t margin = 5) {
        auto memory = fixture();
        for (const auto& part : more) append(memory, part);
        answers.push_back({match_text, 0});
        answers.push_back({match_fields, 0});
        cases.push_back({.entry = entry_bdf, .registers = arguments(self, club.at, story, player_at, match_at, margin, 0x99), .memory = std::move(memory),
                         .f1 = {}, .label = label, .answers = std::move(answers)});
    };
    const Answers quiet_club{{club_check, 0}, {club_gate, 0}, {queue_id, queue_word}, {queue_known, 1}};

    // The history holds the story for this club: a row of story 0xbdf with the club's id and the story number as its grade (the scan
    // of a story without expiry ignores the person); a row of another story number, club or story does not stop the item.
    const auto club_id = scenario.r32(club.at);
    const std::uint32_t history_at = scratch_base + 0x30000;
    const auto row = [&](unsigned n, std::uint32_t row_club, std::uint32_t grade, std::uint32_t story) {
        const auto at = history_at + n * 0x18;
        return Writes{w32(at + 8, 77), w32(at + 0xc, row_club), b8(at + 0x10, 0), h16(at + 0x12, grade), w32(at + 0x14, story)};
    };
    const auto history = [&](std::initializer_list<Writes> rows) {
        Writes out{w32(self, history_at), w32(self + 4, 8), w32(self + 8, std::uint32_t(rows.size()))};
        for (const auto& r : rows) append(out, r);
        return out;
    };
    for (const std::uint32_t story : {0x3du, 0x41u, 0x60u}) {
        add(story, {table(scenario, {1, 0, 0, 0, 0, 0}), history({row(0, club_id, story, 0xbdf)})}, quiet_club, "history holds the story");
        add(story, {table(scenario, {1, 0, 0, 0, 0, 0}), history({row(0, club_id + 1, story, 0xbdf), row(1, club_id, story, 0xbdf)})}, quiet_club, "history, second row");
        add(story, {table(scenario, {1, 0, 0, 0, 0, 0}), history({row(0, club_id, story + 1, 0xbdf)})}, quiet_club, "history, other grade");
        add(story, {table(scenario, {1, 0, 0, 0, 0, 0}), history({row(0, club_id + 1, story, 0xbdf)})}, quiet_club, "history, other club");
        add(story, {table(scenario, {1, 0, 0, 0, 0, 0}), history({row(0, club_id, story, 0xbdd)})}, quiet_club, "history, other item");
    }

    // The tier's third draw below zero: the club's rating under the first draw's threshold (a negative remainder lowers it), the
    // gate closed, and a seed word below zero, so x % 3 is -2, -1 or 0 and the choice of the tier is not the plain one.
    for (const std::uint32_t seed : {0xfffffff0u, 0xfffffff1u, 0xfffffff2u, 0xffffffffu, 0xfffff000u})
        for (const std::uint32_t entry : {0u, 1u})
            add(0x3d, {{h16(club.at + 0x1ec, 0x1000)}, table(scenario, {entry, entry, entry, entry, entry, entry}, seed)}, quiet_club, "negative third draw");

    // The stories 0x50 and 0x51 posted with the club below the top bracket (rating under 0x2328, level over 15) and the margin at or
    // below 15: the lively test fails on the margin. The margin bonus applies from above 0x14 only.
    for (const std::uint32_t story : {0x50u, 0x51u, 0x3du})
        for (const std::uint32_t margin : {0u, 5u, 0xfu, 0x10u, 0x15u, 0x80u})
            for (const std::uint32_t rating : {0x2000u, 0x2327u})
                for (const std::uint32_t gate : {0u, 1u})
                    add(story, {{h16(club.at + 0x1ec, rating), h16(club.link + 0x18, 0x10)}, table(scenario, {1, 0, 0, 0, 0, 0, 0, 0})},
                        {{club_check, 0}, {club_gate, gate}, {queue_id, queue_word}, {queue_known, 1}}, "lively margin", margin);
}

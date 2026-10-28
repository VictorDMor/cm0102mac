#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

// Host-only selection policy. No guest writes, random draws or career fields.
namespace cm::aot::best_xi {
enum Attribute : unsigned {
    acceleration, aggression, agility, anticipation, balance, bravery,
    crossing, decisions, dribbling, finishing, flair, free_kicks, handling,
    heading, jumping, leadership, long_shots, marking, off_the_ball, pace,
    passing, positioning, reflexes, stamina, strength, tackling, teamwork,
    technique, creativity, work_rate, attribute_count
};
struct Player {
    std::uint32_t staff = 0;
    std::int32_t id = 0;
    // Comparable attribute values in hundredths, decoded by the original game.
    std::array<int, attribute_count> attributes{};
    // GK, SW, D, DM, M, AM, F, left, right, centre (0..20).
    std::array<int, 10> positions{};
    unsigned condition = 10000;
};
struct Role {
    unsigned without_ball = 0, with_ball = 0;
    std::uint64_t instructions = 0;
    unsigned team = 0;
};
using Weights = std::array<int, attribute_count>;
inline Weights role_weights(unsigned code) {
    Weights w{};
    const auto add = [&](std::initializer_list<std::pair<Attribute, int>> entries) {
        for (const auto [attribute, weight] : entries) w[attribute] += weight;
    };
    if (code & 1) {
        add({{handling,6}, {reflexes,6}, {agility,3},
             {positioning,3}, {anticipation,2}, {decisions,2}, {jumping,1}});
    } else if (code & 6) {
        if (code & 0x880)
            add({{tackling,4}, {marking,3}, {positioning,4}, {pace,4},
                 {acceleration,3}, {stamina,3}, {crossing,3}, {passing,2},
                 {teamwork,2}, {work_rate,3}});
        else
            add({{tackling,5}, {marking,5}, {positioning,5}, {heading,4},
                 {jumping,3}, {strength,3}, {anticipation,3}, {pace,2},
                 {decisions,2}, {passing,1}});
    } else if (code & 8) {
        add({{tackling,4}, {positioning,4}, {marking,3}, {anticipation,3},
             {decisions,4}, {passing,4}, {teamwork,3}, {stamina,3}, {strength,2}});
    } else if ((code & 0x880) && (code & 0x70)) {
        add({{dribbling,5}, {crossing,5}, {acceleration,4}, {pace,4},
             {off_the_ball,3}, {passing,2}, {technique,3}, {stamina,2}});
    } else if (code & 0x40) {
        add({{finishing,6}, {off_the_ball,5}, {anticipation,3}, {decisions,3},
             {pace,3}, {acceleration,3}, {heading,3}, {jumping,2},
             {strength,2}, {technique,2}});
    } else if (code & 0x20) {
        add({{passing,4}, {creativity,5}, {decisions,4}, {off_the_ball,4},
             {dribbling,3}, {finishing,3}, {technique,3}});
    } else {
        add({{passing,5}, {decisions,4}, {creativity,4}, {off_the_ball,3},
             {tackling,2}, {technique,3}, {stamina,3}, {teamwork,3}});
    }
    return w;
}
inline int position_penalty(const Player& player, unsigned code) {
    int line = 0;
    for (unsigned i = 0; i < 7; ++i)
        if (code & (1u << i)) line = std::max(line, player.positions[i]);
    int side = 20;
    if (!(code & 1) && (code & 0xf80)) {
        side = 0;
        if (code & 0x80) side = std::max(side, player.positions[8]);
        if (code & 0x100) side = std::max({side, player.positions[9], (player.positions[8] + 1) / 2});
        if (code & 0x200) side = std::max(side, player.positions[9]);
        if (code & 0x400) side = std::max({side, player.positions[9], (player.positions[7] + 1) / 2});
        if (code & 0x800) side = std::max(side, player.positions[7]);
    }
    int penalty = (20 - line) * 120 + (20 - side) * 70;
    // Keep specialist keepers and outfield players in their own jobs whenever
    // the pool permits it. Emergency assignments still fill a depleted squad.
    const auto outfield = *std::max_element(player.positions.begin() + 1, player.positions.begin() + 7);
    if (((code & 1) && player.positions[0] < 10) ||
        (!(code & 1) && player.positions[0] >= 15 && outfield < 10)) penalty += 5000;
    return penalty;
}
inline int score(const Player& player, const Role& role) {
    auto w = role_weights(role.without_ball);
    const auto attacking_code = role.with_ball & 0x7f ? role.with_ball : role.without_ball;
    const auto attacking = role_weights(attacking_code);
    for (unsigned i = 0; i < attribute_count; ++i) w[i] += attacking[i];
    if (!(role.without_ball & 1)) {
        const auto bits = role.instructions;
        if (bits & 0x800000) { w[off_the_ball] += 6; w[acceleration] += 3; w[stamina] += 3; }
        if (bits & 0x2000000) { w[dribbling] += 8; w[technique] += 3; w[balance] += 2; }
        if (bits & 0x8000000) { w[strength] += 6; w[balance] += 3; w[teamwork] += 2; }
        if (bits & 0x20000000) { w[long_shots] += 8; w[decisions] += 2; }
        if (bits & 0x100000000ULL) { w[passing] += 6; w[creativity] += 6; w[decisions] += 3; }
        if (bits & 0x400000000ULL) { w[crossing] += 10; w[technique] += 2; }
        if (bits & 0x200000) { w[creativity] += 4; w[off_the_ball] += 3; w[decisions] += 3; }
        // Pressing Yes, or Team with team pressing on (0x4000 is No).
        if ((bits & 0x8000) || ((bits & 0x2000) && (role.team & 0x100))) {
            w[stamina] += 5; w[work_rate] += 5; w[tackling] += 2;
        }
        if ((bits & 4) || ((bits & 1) && (role.team & 2))) {
            w[passing] += 4; w[technique] += 3; w[decisions] += 3;
        } else if ((bits & 0x18) || ((bits & 1) && (role.team & 0xc))) {
            w[passing] += 5; w[creativity] += 3;
        }
        if ((role.team & 0x400) && (role.without_ball & 6)) {
            w[positioning] += 5; w[anticipation] += 4; w[decisions] += 4; w[pace] += 2;
        }
        if (role.team & 0x1000) { w[pace] += 3; w[acceleration] += 3; w[off_the_ball] += 2; }
        if (role.team & 0x10000) { w[off_the_ball] += 3; w[dribbling] += 2; w[decisions] += 2; }
        if (role.team & 0x8000) { w[positioning] += 3; w[marking] += 2; w[tackling] += 2; }
        if (role.team & 0x4000) { w[positioning] += 3; w[teamwork] += 2; }
        if ((bits & 0x1000) || ((bits & 0x200) && (role.team & 0x40))) {
            w[tackling] += 3; w[strength] += 3; w[decisions] += 2;
        }
    }
    int total = 0, weight = 0;
    for (unsigned i = 0; i < attribute_count; ++i) {
        total += w[i] * player.attributes[i]; weight += w[i];
    }
    const auto attributes = total / weight;
    const auto fit = (position_penalty(player, role.without_ball) + position_penalty(player, attacking_code)) / 2;
    return attributes * int(std::min(player.condition, 10000u)) / 10000 - fit;
}

// Exact maximum-weight assignment for at most eleven roles and fifty players.
// Each transition consumes one player, so a versatile player cannot be reused.
// Cardinality wins first, then combined suitability; ties use stable staff IDs.
inline std::array<std::uint32_t, 11> select(std::span<const Player> players, std::span<const Role> roles) {
    if (roles.size() > 11 || players.size() > 50) throw std::invalid_argument("Invalid Best XI pool");
    std::vector<const Player*> ordered;
    for (const auto& player : players) ordered.push_back(&player);
    std::sort(ordered.begin(), ordered.end(), [](const auto* a, const auto* b) { return a->id < b->id; });
    const auto states = 1u << roles.size();
    constexpr auto absent = std::numeric_limits<int>::min() / 2;
    struct Path { int value = absent; std::array<std::uint32_t, 11> staff{}; };
    std::vector<Path> paths(states);
    paths[0].value = 0;
    for (const auto* player : ordered) {
        std::array<int, 11> scores{};
        for (unsigned role = 0; role < roles.size(); ++role) scores[role] = score(*player, roles[role]);
        for (unsigned mask = states - 1; mask > 0; --mask) {
            for (unsigned role = 0; role < roles.size(); ++role) {
                if (!(mask & (1u << role))) continue;
                const auto& previous = paths[mask ^ (1u << role)];
                if (previous.value == absent) continue;
                const auto value = previous.value + scores[role];
                if (value > paths[mask].value) {
                    paths[mask] = previous;
                    paths[mask].value = value;
                    paths[mask].staff[role] = player->staff;
                }
            }
        }
    }
    unsigned best = 0;
    for (unsigned mask = 1; mask < states; ++mask)
        if (std::popcount(mask) > std::popcount(best) ||
            (std::popcount(mask) == std::popcount(best) && paths[mask].value > paths[best].value))
            if (paths[mask].value != absent) best = mask;
    return paths[best].staff;
}
}

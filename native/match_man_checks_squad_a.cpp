// MATCH_MAN.CPP checker cases for the squad_a block (match_man_squad_a.hpp): 0x1020a290.
//
// The routine runs on a match side: the real match's two sides give the pointers (club, staff
// data, orders object, the 20 player records and their data). The branches hang on role words
// (0x10289560, 0x102895e0), attributes (0x100d1cb0 and its siblings), data bytes and the
// manager's bytes, so a case writes the bytes it reads (the checker undoes the writes) or has
// the dispatcher answer a callee (both sides) where only the callee's result matters.
#include "match_man_checks.hpp"
#include "match_man_squad_a_detail.hpp"
#include <cstdlib>
#include <cstring>

namespace cm::aot::match_man_checks {
namespace {
using namespace match_man;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
constexpr std::uint32_t entry = 0x1020a290;
constexpr std::uint32_t role = 0x10289560, role_more = 0x102895e0, attribute = 0x100d1cb0,
                        judge_attribute = 0x100d2a80, staff_attribute = 0x100d2f50, opponent = 0x101fd100,
                        registry_check = 0x10147e70, registry_act = 0x10149df0;

// The bytes of a player's data record the routine reads.
struct Bytes { std::uint8_t b12, b13, b14, b15, b17, b18, b19, b1a, b2e, b3c, b40, b44; };
constexpr Bytes profiles[] = {
    {0x14, 14, 14, 14, 10, 10, 0x14, 14, 17, 14, 4, 12},   // keeper-like, middling
    {0x13, 5, 12, 6, 8, 3, 0x14, 8, 12, 9, 9, 18},         // b14 above the others, byte 0x19 is 20
    {0x14, 12, 4, 13, 12, 11, 0x14, 18, 19, 16, 2, 5},     // b14 below
    {0x12, 20, 20, 20, 5, 20, 0x14, 20, 20, 20, 20, 20},
    {0x14, 0, 0, 0, 0, 0, 0x13, 0, 0, 0, 0, 0},
    {0x14, 0xff, 0xff, 0xff, 0xf0, 0x80, 0x14, 0xf5, 0xf0, 0xf2, 0xff, 0xff},  // negative as signed bytes
};

void write_bytes(std::vector<Write>& memory, std::uint32_t data, const Bytes& b) {
    const std::pair<std::uint32_t, std::uint8_t> items[] = {
        {squad_data::b12, b.b12}, {squad_data::b13, b.b13}, {squad_data::b14, b.b14}, {squad_data::b15, b.b15},
        {squad_data::b17, b.b17}, {squad_data::b18, b.b18}, {squad_data::b19, b.b19}, {squad_data::b1a, b.b1a},
        {squad_data::b2e, b.b2e}, {squad_data::b3c, b.b3c}, {squad_data::b40, b.b40}, {squad_data::b44, b.b44}};
    for (const auto& [offset, value] : items) memory.push_back({1, data + offset, value});
}
}

MATCH_MAN_CASES("squad_a") {
    if (!scenario.match) return;
    for (unsigned n = 0; n < 2; ++n) {
        const auto side = scenario.side(n);
        const auto name = "side " + std::to_string(n);
        const auto staff = scenario.r32(side + squad_side::staff);
        const auto club = scenario.r32(side + squad_side::club);
        cases.push_back({.entry = entry, .registers = {{3, side}}, .label = name});

        // The kind byte, the extras' type (1 and 9 pass), the random cursor at, just under and well
        // under its limit (an advance past it draws from 0x1077b770).
        const auto extras = scenario.r32(side + squad_side::extras);
        const auto cursor_slot = scenario.global(-0x7b84) ? scenario.toc(-0x7b84) : 0;
        const auto limit = scenario.global(-0x7b88);
        const auto cursor = scenario.global(-0x7b84);
        std::vector<std::uint32_t> cursors{cursor, limit, limit - 2, limit - 4};
        for (const std::uint32_t kind : {0u, 1u, 0x80u, 0x100u})
            for (const std::uint32_t type : {1u, 9u, 4u})
                for (const auto position : cursors)
                    for (const std::uint32_t own : {0u, 3000u, 0xf000u}) {
                        std::vector<Write> memory{{1, side + squad_side::kind, kind}, {1, extras + 0x3a, type},
                                                  {4, cursor_slot, position}, {2, club + squad_club::rating, own}};
                        cases.push_back({.entry = entry, .registers = {{3, side}}, .memory = memory,
                                         .label = name + " kind " + std::to_string(kind) + " type " + std::to_string(type) +
                                                  " cursor " + std::to_string(position) + " rating " + std::to_string(own)});
                    }
        // No staff data: leaves at 0x1020a4c8.
        cases.push_back({.entry = entry, .registers = {{3, side}}, .memory = {{4, side + squad_side::staff, 0}},
                         .label = name + " without staff data"});
        (void)staff;
        // No club person at all (the registry is not asked).
        cases.push_back({.entry = entry, .registers = {{3, side}},
                         .memory = {{4, club + squad_club::person, 0}, {1, side + squad_side::kind, 0}},
                         .label = name + " no club person"});
        // The club's person among the newest of the table, with the registry answering.
        const auto count = scenario.people();
        for (const std::uint32_t back : {1u, 16u, 17u})
            for (const std::uint32_t flag : {0u, 1u})
                for (const std::uint32_t checked : {0u, 1u})
                    for (const std::uint32_t acted : {0u, 1u}) {
                        const auto person = scenario.person(count - back);
                        cases.push_back({.entry = entry, .registers = {{3, side}},
                                         .memory = {{4, club + squad_club::person, person}, {1, person + 0x6b, flag},
                                                    {1, side + squad_side::kind, 0}},
                                         .label = name + " club person " + std::to_string(count - back) + " flag " +
                                                  std::to_string(flag) + " check " + std::to_string(checked) + " act " + std::to_string(acted),
                                         .answers = {{registry_check, checked}, {registry_act, acted}}});
                    }

        // Roles, attributes and data bytes: every combination of the role bits the tests use, each
        // with its own mix of the manager's bytes, orders flags, forms, club ratings and the player
        // flag; the mixes cycle independently so the bytes the rules compare meet many role words.
        std::vector<std::uint32_t> words{0};
        const std::uint32_t bits[] = {0x2, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800};
        for (const auto bit : bits) words.push_back(bit);
        for (unsigned a = 0; a < 10; ++a)
            for (unsigned b = a + 1; b < 10; ++b) words.push_back(bits[a] | bits[b]);
        for (const std::uint32_t word : {0x40u | 0x200 | 0x10, 0x10u | 0x8 | 0x80, 0x40u | 0x100 | 0x20, 0x8u | 0x800 | 0x40, 0x10u | 0x20 | 0x400,
                                         0x10u | 0x8 | 0x20, 0xffffu, 0x8000u, 0x12345u})
            words.push_back(word);
        struct Manager { std::uint8_t b33, b3f, b3c, b35, b2e; };
        constexpr Manager managers[] = {{9, 15, 14, 12, 17}, {14, 5, 20, 6, 0x11}, {16, 3, 9, 15, 12}, {20, 20, 0, 0, 20},
                                        {0, 0, 0xf0, 0xf0, 0x80}, {8, 14, 15, 13, 16}, {19, 17, 12, 20, 18}, {21, 6, 13, 9, 11}};
        constexpr std::pair<std::uint32_t, std::uint32_t> orders_flags[] = {{0, 0}, {0, 8}, {0x400000, 0}, {0x800000, 0x20},
                                                                             {0x1000000, 8}, {0, 0x20}, {0x8, 0x28}};
        constexpr std::uint32_t forms[] = {0, 0x3f800000, 0x40a00000, 0x41200000, 0x42c80000, 0xbf800000, 0x3f000000};
        constexpr std::uint32_t ratings[][2] = {{1000, 1000}, {3000, 1000}, {1000, 3000}, {0xf000, 100}, {2000, 2100}};
        constexpr std::uint32_t query_words[] = {0, 1, 0x200, 0x201, 0x3ff, 0x400};
        constexpr std::uint32_t fallbacks[] = {0x3f800000, 0x42c80000, 0xc2c80000, 0};
        constexpr std::uint32_t floors[] = {0xc47a0000, 0x3f800000, 0x47c35000};
        const auto orders = scenario.r32(side + squad_side::orders);
        std::uint32_t serial = 0;
        for (unsigned pass = 0; pass < 5; ++pass)
        for (const auto word : words) {
            for (const std::uint32_t more : {0u, 0x10u | 0x40, 0x20u}) {
                const auto profile = profiles[serial % std::size(profiles)];
                // Passes 3 and 4 push the attributes high and negative (as signed bytes).
                const auto attributes = std::uint32_t(serial * 7 % 29) + (pass == 3 ? 90 : pass == 4 ? 0xe0 : 0);
                const auto other = serial % 20;
                // Pass 0 leaves the match's own values; later passes mix them in at different strides.
                const auto manager = managers[serial * (pass + 4) / 3 % std::size(managers)];
                const auto flags = orders_flags[serial / (pass + 1) % std::size(orders_flags)];
                const auto form = forms[serial * (pass + 2) % std::size(forms)];
                const auto rating = ratings[serial / (pass + 2) % std::size(ratings)];
                const bool flag24 = serial % (pass + 2) == 0;
                ++serial;
                std::vector<Write> memory;
                for (unsigned index = 0; index < 20; ++index) {
                    const auto player = scenario.player(n, index);
                    const auto data = scenario.r32(player + squad_player::data);
                    if (data) write_bytes(memory, data, profile);
                    if (pass) {
                        memory.push_back({4, player + squad_player::form, form});
                        memory.push_back({1, player + squad_player::flag24, flag24});
                    }
                }
                if (pass >= 2) {
                    // Some players are one of the side's three named persons (the closing notice), the
                    // orders' per-slot words feed the query, and the singles decide the ratio test.
                    const auto player = scenario.player(n, (serial * 3) % 20);
                    memory.push_back({4, player + squad_player::person, scenario.r32(side + squad_side::others[serial % 3])});
                    for (unsigned slot = 0; slot < 24; ++slot)
                        memory.push_back({4, orders + 0x188c + slot * 8, query_words[(serial + slot) % std::size(query_words)]});
                    const auto singles = scenario.toc(-0x527c);
                    memory.push_back({4, singles + 0xc, fallbacks[serial % std::size(fallbacks)]});
                    memory.push_back({4, singles + 0x10, floors[serial / 2 % std::size(floors)]});
                }
                if (pass) {
                    for (const auto& [offset, value] : {std::pair{squad_staff::b33, manager.b33}, {squad_staff::b3f, manager.b3f},
                                                        {squad_staff::b3c, manager.b3c}, {squad_staff::b35, manager.b35},
                                                        {squad_staff::b2e, manager.b2e}})
                        memory.push_back({1, staff + offset, value});
                    memory.push_back({4, orders + 0x1880, flags.first});
                    memory.push_back({4, orders + 0x588, flags.second});
                    memory.push_back({2, club + squad_club::rating, rating[0]});
                    memory.push_back({2, scenario.r32(side + squad_side::rival_club) + squad_club::rating, rating[1]});
                }
                cases.push_back({.entry = entry, .registers = {{3, side}}, .memory = memory,
                                 .label = name + " role " + std::to_string(word) + "/" + std::to_string(more) + " attr " +
                                          std::to_string(attributes) + " pass " + std::to_string(pass) + " profile " + std::to_string((serial - 1) % std::size(profiles)),
                                 .answers = {{role, word}, {role_more, more}, {attribute, attributes}, {opponent, other},
                                             {judge_attribute, attributes + 3}, {staff_attribute, attributes}}});
            }
        }
    }
    // Targeted families for rules the mixes above reach rarely.
    for (unsigned n = 0; n < 2; ++n) {
        const auto side = scenario.side(n);
        const auto orders = scenario.r32(side + squad_side::orders);
        const auto singles = scenario.toc(-0x527c);
        const auto name = "side " + std::to_string(n);
        const auto players_with = [&](const Bytes& profile, std::vector<Write>& memory) {
            for (unsigned index = 0; index < 20; ++index) {
                const auto data = scenario.r32(scenario.player(n, index) + squad_player::data);
                if (data) write_bytes(memory, data, profile);
            }
        };
        // The duel scenario against a goalkeeper (data byte 0x12 or 0x13 is 20) and the three ratings.
        for (const std::uint32_t word : {0x230u, 0x330u, 0x630u, 0x238u})
            for (const std::uint32_t attributes : {13u, 15u, 16u, 20u, 28u})
                for (const unsigned profile : {0u, 2u, 5u, 3u})
                    for (const std::uint32_t fallback : {0x42c80000u, 0u})
                    for (const std::uint32_t rating : {8500u, 30000u}) {
                        std::vector<Write> memory;
                        players_with(profiles[profile], memory);
                        memory.push_back({4, singles + 0xc, fallback});
                        // The floor (single at +0x10) above any rating keeps both quotients at the fallback.
                        memory.push_back({4, singles + 0x10, 0x47c35000});
                        memory.push_back({2, scenario.r32(side + squad_side::club) + squad_club::rating, rating});
                        memory.push_back({2, scenario.r32(side + squad_side::rival_club) + squad_club::rating, rating});
                        cases.push_back({.entry = entry, .registers = {{3, side}}, .memory = memory,
                                         .label = name + " duel role " + std::to_string(word) + " attr " + std::to_string(attributes) +
                                                  " profile " + std::to_string(profile) + " fallback " + std::to_string(fallback) +
                                                  " rating " + std::to_string(rating),
                                         .answers = {{role, word}, {role_more, 0x20}, {attribute, attributes}, {opponent, 3},
                                                     {judge_attribute, 30}, {staff_attribute, attributes}}});
                    }
        // The finishing and cap rules: role 8 or 0x10, attributes around the scaled finishing bar, and the
        // query word with bit 0x200 under each setting of the orders' flag word at +0x588 (bit 0x20 picks
        // the setting). The manager's cap byte (+0x3f) widens the second bar.
        for (const std::uint32_t word : {0x8u, 0x10u, 0x20u, 0x18u})
            for (std::uint32_t attributes = 1; attributes < 40; attributes += (attributes < 8 ? 3 : 1))
                for (const std::uint32_t flags : {0u, 8u, 0x20u})
                    for (const std::uint32_t cap : {100u, 0x7fu, 5u}) {
                        if (word != 0x8 && (attributes % 3 || cap != 100)) continue;
                        std::vector<Write> memory;
                        players_with(profiles[0], memory);
                        // Kind 0 with another extras type makes the opening mark nothing (it sets bit 0x20 of
                        // the word at +0x588 otherwise, which the rule tests).
                        memory.push_back({1, side + squad_side::kind, 0});
                        memory.push_back({1, scenario.r32(side + squad_side::extras) + 0x3a, 4});
                        memory.push_back({4, orders + 0x588, flags});
                        for (unsigned slot = 0; slot < 24; ++slot) memory.push_back({4, orders + 0x188c + slot * 8, 0x200});
                        memory.push_back({1, scenario.r32(side + squad_side::staff) + squad_staff::b3f, cap});
                        memory.push_back({2, scenario.r32(side + squad_side::club) + squad_club::rating, 8500});
                        memory.push_back({2, scenario.r32(side + squad_side::rival_club) + squad_club::rating, 8500});
                        cases.push_back({.entry = entry, .registers = {{3, side}}, .memory = memory,
                                         .label = name + " finishing role " + std::to_string(word) + " attr " + std::to_string(attributes) +
                                                  " flags " + std::to_string(flags) + " cap " + std::to_string(cap),
                                         .answers = {{role, word}, {role_more, 0}, {attribute, attributes}, {opponent, 3},
                                                     {judge_attribute, attributes}, {staff_attribute, attributes}}});
                    }
    }
}
}

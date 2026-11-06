#include "match_debugger_factory.hpp"
#include "match_debugging.hpp"
#include "debug_localization.hpp"
#include "best_xi_selection.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
using namespace cm::aot;
using namespace cm::aot::match_debugger;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void check_assignment_policy() {
    using namespace best_xi;
    auto player = [](unsigned id) {
        Player p; p.id = int(id); p.staff = 100 + id;
        p.attributes.fill(1000); p.positions.fill(20); return p;
    };
    const Role fullback{0x804, 0x804, 0, 0};
    std::array<Player, 3> pool{player(1), player(2), player(3)};
    pool[1].attributes[tackling] = 1800;
    pool[2].attributes[finishing] = 2000;
    require(select(pool, std::array{fullback})[0] == pool[1].staff,
            "Best XI ignored the stronger fullback's relevant attributes");
    pool[0].attributes[handling] = pool[0].attributes[reflexes] = 2000;
    require(select(pool, std::array{Role{1,1,0,0}})[0] == pool[0].staff,
            "Keeper attributes did not decide the keeper contest");
    // Equal fullback profiles except defence versus attacking delivery. The
    // exact same candidates must trade places when crossing is requested.
    pool = {player(1), player(2), player(3)};
    pool[0].attributes[tackling] = pool[0].attributes[positioning] = 1500;
    pool[1].attributes[crossing] = 1800;
    require(select(pool, std::array{fullback})[0] == pool[0].staff,
            "Defensive fullback profile not preferred");
    auto crossing_role = fullback; crossing_role.instructions = 0x400000000ULL;
    require(select(pool, std::array{crossing_role})[0] == pool[1].staff,
            "Cross-ball instruction did not change the fullback comparison");
    auto no_pressing = fullback; no_pressing.instructions = 0x4000; no_pressing.team = 0x100;
    require(score(pool[0], no_pressing) == score(pool[0], fullback), "Pressing No inherited team pressing");
    // Greedy picking would spend the versatile player at left-back. Globally
    // the other player fills that role, saving the specialist's only rival.
    pool = {player(1), player(2), player(3)};
    pool[0].attributes.fill(1400);
    pool[1].attributes.fill(1300); pool[1].positions[8] = 1;
    pool[2].attributes.fill(100);
    const std::array roles{Role{0x804,0x804,0,0}, Role{0x84,0x84,0,0}};
    const auto selected = select(pool, roles);
    require(selected[0] == pool[1].staff && selected[1] == pool[0].staff,
            "Global assignment wasted a versatile player");
    int exhaustive = std::numeric_limits<int>::min();
    for (unsigned a = 0; a < pool.size(); ++a) for (unsigned b = 0; b < pool.size(); ++b)
        if (a != b) exhaustive = std::max(exhaustive, score(pool[a], roles[0]) + score(pool[b], roles[1]));
    require(exhaustive == score(pool[1], roles[0]) + score(pool[0], roles[1]),
            "Assignment disagreed with exhaustive alternatives");
    std::reverse(pool.begin(), pool.end());
    require(select(pool, roles) == selected, "Roster order changed the best assignment");
    pool = {player(1), player(2), player(3)};
    require(select(pool, std::array{fullback})[0] == pool[0].staff, "Equal scores did not prefer stable identity");
    std::reverse(pool.begin(), pool.end());
    require(select(pool, std::array{fullback})[0] == 101, "Tie depended on roster order");
    pool[0].attributes.fill(2000); pool[0].positions[3] = 0;
    require(select(pool, std::array{Role{0x208,0x208,0,0}})[0] != pool[0].staff,
            "Attributes overrode an entirely unfamiliar position");
    pool = {player(1), player(2), player(3)};
    pool[0].attributes.fill(1500); pool[0].condition = 4000;
    require(select(pool, std::array{fullback})[0] == pool[1].staff, "Current condition was ignored");
    require(select(std::span<const Player>{}, roles) == std::array<std::uint32_t,11>{}, "Empty pool invented players");
    const auto depleted = select(std::span(pool).first(1), roles);
    require(std::count(depleted.begin(), depleted.end(), pool[0].staff) == 1, "Depleted pool reused a player");
    // Enumerate every legal assignment independently of the DP on varied
    // overlapping roles, attributes and conditions, including negative scores.
    for (unsigned sample = 0; sample < 16; ++sample) {
        std::array<Player,5> varied{player(1),player(2),player(3),player(4),player(5)};
        for (unsigned i = 0; i < varied.size(); ++i) {
            for (unsigned j = 0; j < attribute_count; ++j)
                varied[i].attributes[j] = int((sample * 13 + i * 7 + j * (i + 1)) % 20 + 1) * 100;
            for (unsigned j = 0; j < 10; ++j) varied[i].positions[j] = int((sample + i * 3 + j * 7) % 21);
            varied[i].condition = 6000 + (i * 1000);
        }
        const std::array jobs{Role{1,1,0,0}, Role{0x804,0x810,0x400000000ULL,0}, Role{0x240,0x240,0,0}};
        int optimum = std::numeric_limits<int>::min();
        for (unsigned a = 0; a < varied.size(); ++a) for (unsigned b = 0; b < varied.size(); ++b)
            for (unsigned c = 0; c < varied.size(); ++c) if (a != b && a != c && b != c)
                optimum = std::max(optimum, score(varied[a],jobs[0]) + score(varied[b],jobs[1]) + score(varied[c],jobs[2]));
        const auto choice = select(varied, jobs);
        int actual = 0;
        for (unsigned job = 0; job < jobs.size(); ++job) {
            const auto chosen = std::find_if(varied.begin(), varied.end(), [&](const auto& p) { return p.staff == choice[job]; });
            require(chosen != varied.end(), "Assignment invented a player");
            actual += score(*chosen, jobs[job]);
        }
        require(actual == optimum, "Global selection is worse than an enumerated alternative");
    }
    std::cout << "Best XI policy: role attributes, instructions, global optimum, identity ties, positions and condition passed\n";
}
std::uint32_t invoke(State& s, std::uint32_t entry, std::initializer_list<std::uint32_t> args) {
    unsigned reg = 3;
    for (auto value : args) s.wr<4>(reg++ * 4, value);
    s.wr<4>(4128, return_sentinel); s.pc = entry;
    try { run_until(s, return_sentinel); }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + " entry=" + hex_address(entry) + " pc=" + hex_address(s.pc));
    }
    return s.r<4>(12);
}
std::array<std::uint32_t, 20> squad(State& s, std::uint32_t tactics, std::uint32_t club) {
    std::array<std::uint32_t, 20> result{};
    for (unsigned i = 0; i < result.size(); ++i) result[i] = invoke(s, 0x103af800, {tactics, club, i, 0});
    return result;
}
std::vector<std::uint8_t> rng_state(const State& s) {
    auto result = s.memory.copy_out(0x108b3f00, 16);
    for (auto global : {0x108b3f00u, 0x108b3f08u, 0x108b3f0cu}) {
        const auto bytes = s.memory.copy_out(s.memory.read<4>(global), 4);
        result.insert(result.end(), bytes.begin(), bytes.end());
    }
    const auto samples = s.memory.copy_out(s.memory.read<4>(0x108b3f04), 51000 * 2);
    result.insert(result.end(), samples.begin(), samples.end());
    return result;
}
void check_original_attribute_selection(State& s, std::uint32_t club, std::uint32_t tactics,
                                        std::uint32_t formation, std::uint32_t a, std::uint32_t b) {
    const auto roster = s.memory.copy_out(club + 0x98, 200);
    const auto pa = std::uint32_t(s.memory.read<4>(a + 0x14));
    const auto pb = std::uint32_t(s.memory.read<4>(b + 0x14));
    require(pa && pb && pa != pb, "Need two different actual player records");
    const auto original_a = s.memory.copy_out(pa, 0x48), original_b = s.memory.copy_out(pb, 0x48);
    const auto medical = s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7ba0));
    const auto conditions = s.memory.read<4>(medical);
    const auto ca = std::uint32_t(conditions + s.memory.read<4>(a) * 0x24);
    const auto cb = std::uint32_t(conditions + s.memory.read<4>(b) * 0x24);
    const auto original_ca = s.memory.copy_out(ca, 0x24), original_cb = s.memory.copy_out(cb, 0x24);
    const auto probe = invoke(s, 0x10773520, {0x18e0});
    s.memory.copy_in(probe, s.memory.copy_out(formation, 0x18e0));
    for (unsigned i = 0; i < 11; ++i) {
        s.memory.write<2>(probe + 0x12e + i * 2, i ? 1 : 0x804);
        s.memory.write<2>(probe + 0x150 + i * 2, i ? 1 : 0x804);
        s.memory.write<8>(probe + 0x1888 + i * 8, 0);
    }
    s.memory.write<4>(probe + 0x588, 0);
    for (unsigned i = 0; i < 50; ++i) s.memory.write<4>(club + 0x98 + i * 4, i == 0 ? a : i == 1 ? b : 0);
    for (const auto record : {pa, pb}) {
        s.memory.write<2>(record + 4, 100);
        for (unsigned offset = 0xf; offset <= 0x1a; ++offset) s.memory.write<1>(record + offset, 1);
        s.memory.write<1>(record + 0x11, 20); s.memory.write<1>(record + 0x17, 20);
        for (unsigned offset = 0x1b; offset <= 0x44; ++offset) s.memory.write<1>(record + offset, 10);
    }
    for (const auto condition : {ca, cb}) {
        s.memory.write<2>(condition + 8, 10000); s.memory.write<2>(condition + 10, 10000);
    }
    // The original getter must decode negative technical bytes and current
    // ability before comparing them with the physical attributes.
    s.memory.write<1>(pa + 0x3e, -50); s.memory.write<1>(pb + 0x3e, 50);
    require(invoke(s, 0x100d2330, {a, pa, 28}) < invoke(s, 0x100d2330, {b, pb, 28}),
            "Original technical decode did not distinguish the fixtures");
    auto random = rng_state(s);
    require(select_debug_best_xi(s, club, 0, probe, 0) && squad(s, tactics, club)[0] == b,
            "Actual selector ignored the better decoded tackling attribute");
    require(random == rng_state(s), "Attribute selection consumed game RNG");
    s.memory.write<1>(pa + 0x3e, 50); s.memory.write<1>(pb + 0x3e, -50);
    require(select_debug_best_xi(s, club, 0, probe, 0) && squad(s, tactics, club)[0] == a,
            "Swapping attributes did not swap the preferred actual player");
    s.memory.write<1>(pa + 0x3e, 10); s.memory.write<1>(pb + 0x3e, 10);
    s.memory.write<1>(pa + 0x3e, 80);
    s.memory.write<1>(pa + 0x23, 10); s.memory.write<1>(pb + 0x23, 45);
    require(select_debug_best_xi(s, club, 0, probe, 0) && squad(s, tactics, club)[0] == a,
            "Actual defensive fullback contest failed");
    s.memory.write<8>(probe + 0x1888, 0x400000000ULL);
    const auto attributes_a = s.memory.copy_out(pa, 0x48), attributes_b = s.memory.copy_out(pb, 0x48);
    require(select_debug_best_xi(s, club, 0, probe, 0) && squad(s, tactics, club)[0] == b,
            "Actual high-word crossing instruction did not change the selection");
    require(s.memory.copy_out(pa, 0x48) == attributes_a && s.memory.copy_out(pb, 0x48) == attributes_b,
            "Best XI changed a player's attributes");
    require(random == rng_state(s), "Repeated attribute selection changed the RNG pool/seed/cursor");
    s.memory.copy_in(pa, original_a); s.memory.copy_in(pb, original_b);
    s.memory.copy_in(ca, original_ca); s.memory.copy_in(cb, original_cb);
    s.memory.copy_in(club + 0x98, roster);
    require(select_debug_best_xi(s, club, 0, formation, 0), "Could not restore actual selection context");
    std::cout << "Original Mac attribute decode: competing fullbacks, swapped attributes, live crossing instruction and unchanged RNG passed\n";
}
void check_recovery(State& s, std::uint32_t club, std::uint32_t opponent,
                    std::uint32_t tactics, std::uint32_t formation) {
    const auto medical = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7ba0)));
    const auto discipline = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7aa4)));
    const auto records = std::uint32_t(s.memory.read<4>(medical));
    const auto indexes = std::uint32_t(s.memory.read<4>(discipline + 0x14));
    const auto scratch = invoke(s, 0x10773520, {128});
    std::vector<std::uint32_t> players;
    for (unsigned i = 0; i < 50; ++i) {
        const auto staff = std::uint32_t(s.memory.read<4>(club + 0x98 + i * 4));
        if (staff && s.memory.read<4>(staff + 0x14)) players.push_back(staff);
    }
    require(players.size() > 20, "Need both selected and unselected squad players");
    const auto outsider = std::uint32_t(s.memory.read<4>(opponent + 0x98));
    require(select_debug_best_xi(s, club, 0, formation, 0), "Need a selected XI and substitutes for recovery");
    const auto selected = squad(s, tactics, club);
    const auto unselected = std::find_if(players.begin(), players.end(), [&](auto staff) {
        return std::find(selected.begin(), selected.end(), staff) == selected.end();
    });
    require(selected[0] && selected[11] && unselected != players.end(), "Missing recovery selection groups");
    auto condition = [&](std::uint32_t staff) { return records + s.memory.read<4>(staff) * 0x24; };
    auto injury = [&](std::uint32_t staff, unsigned type) {
        s.memory.write<4>(s.r<4>(4) + 0x38, 0);
        require(invoke(s, 0x10188cc0, {medical, type, 0, staff, 0, 1, 0xffffffff, 0}) != 0xffffffff,
                "Original medical setup failed");
    };
    for (const auto staff : players) {
        const auto record = condition(staff);
        s.memory.write<2>(record + 8, 6200);
        s.memory.write<2>(record + 10, 7500);
        s.memory.write<2>(record + 12, 1200);
        s.memory.write<1>(record + 0x14, 11);
        s.memory.write<1>(record + 0x15, 3);
    }
    injury(selected[0], 5);
    injury(selected[11], 11);
    injury(*unselected, 77); // Recovering rather than actively injured.
    injury(outsider, 5);
    const auto outside_condition = s.memory.copy_out(condition(outsider), 0x24);
    const auto outside_injury = std::uint32_t(s.memory.read<4>(medical + 4)) +
        s.memory.read<4>(condition(outsider) + 0x10) * 0x38;
    const auto outside_medical = s.memory.copy_out(outside_injury, 0x38);
    // Queue real medical updates for an in-squad player and an opponent.
    s.memory.write<1>(scratch, 5); s.memory.write<1>(scratch + 1, 20);
    invoke(s, 0x10186cd0, {medical, players.back(), scratch, scratch + 1});
    invoke(s, 0x10186cd0, {medical, outsider, scratch, scratch + 1});
    // Register match-count and time-based bans using the real discipline store.
    std::vector<std::uint32_t> suspended;
    for (unsigned category = 0; category < 4; ++category) {
        unsigned group = 42;
        for (unsigned i = 0; i < 42; ++i) {
            const auto rules = s.memory.read<4>(s.memory.read<4>(discipline) + i * 4);
            if (rules && s.memory.read<1>(rules + 4) == category) { group = i; break; }
        }
        // This English world instantiates only two categories. Construct real
        // empty rule stores for the others so every category is exercised.
        if (group == 42) {
            for (unsigned i = 0; i < 42; ++i)
                if (!s.memory.read<4>(s.memory.read<4>(discipline) + i * 4)) { group = i; break; }
            require(group < 42, "No empty disciplinary rule slot");
            const auto rules = invoke(s, 0x10773520, {0x24});
            invoke(s, 0x105b7030, {rules, 0});
            s.memory.write<1>(rules + 4, category);
            s.memory.write<1>(rules + 5, group);
            s.memory.write<4>(s.memory.read<4>(discipline) + group * 4, rules);
        }
        const auto staff = players[category == 0 ? 0 : category == 1 ? 11 : players.size() - category];
        suspended.push_back(staff);
        const auto id = std::uint32_t(s.memory.read<4>(staff));
        s.memory.copy_in(scratch, std::vector<std::uint8_t>(24, 0xff));
        s.memory.write<1>(scratch, category % 2 ? 0xff : 3);
        s.memory.write<1>(scratch + 1, 0);
        s.memory.write<2>(scratch + 2, category % 2 ? 30 : 0xffff);
        s.memory.copy_in(scratch + 4, s.memory.copy_out(s.memory.read<4>(s.r<4>(8) - 0x7b80), 5));
        s.memory.write<1>(scratch + 0xc + category, group);
        s.memory.write<4>(scratch + 0x14, 0x40000000);
        invoke(s, 0x100dfb30, {discipline, id, scratch});
        const auto rules = std::uint32_t(s.memory.read<4>(s.memory.read<4>(discipline) + group * 4));
        const auto record = invoke(s, 0x105bc2b0, {rules, id, 0});
        require(record && signed_value<2>(s.memory.read<2>(record + 6)) >= 0, "Original ban setup failed");
        require(invoke(s, 0x105bc600, {rules, staff, 0, 0, 0}) != 0, "Original ban query missed setup");
    }
    require(suspended.size() == 4, "Need all four disciplinary categories");
    // Deferred bans use the original queue format and append operation.
    for (const auto staff : {players.back(), outsider}) {
        s.memory.copy_in(scratch, std::vector<std::uint8_t>(16, 0));
        s.memory.write<4>(scratch, s.memory.read<4>(staff));
        invoke(s, 0x10738d10, {discipline + 0x1c, scratch, 16});
        const auto index = indexes + s.memory.read<4>(staff) * 0x2c;
        s.memory.write<1>(index + 0x28, s.memory.read<1>(index + 0x28) + 1);
    }
    const auto outside_discipline = s.memory.copy_out(indexes + s.memory.read<4>(outsider) * 0x2c, 0x2c);
    const auto lineup = squad(s, tactics, club);
    const auto original_tactics = s.memory.copy_out(formation, 0x18e0);
    const auto roster = s.memory.copy_out(club + 0x98, 200);
    select_native_debugging(false);
    const auto before = s.memory.copy_out(condition(players.front()), 0x24);
    const auto executed = s.executed;
    require(!recover_debug_squad(s, club) && s.executed == executed &&
            before == s.memory.copy_out(condition(players.front()), 0x24), "Disabled recovery changed state");
    select_native_debugging(true);
    const auto registers = s.registers;
    const auto unique = s.unique;
    const auto pc = s.pc;
    require(recover_debug_squad(s, club), "Squad recovery rejected the actual club");
    require(s.registers == registers && s.unique == unique && s.pc == pc, "Recovery clobbered caller");
    for (const auto staff : players) {
        const auto record = condition(staff);
        require(s.memory.read<4>(record + 0x10) == 0xffffffff &&
                s.memory.read<1>(record + 0x14) == 0xff && !s.memory.read<1>(record + 0x15), "Medical risk/injury survived");
        require(invoke(s, 0x10187680, {medical, staff}) == 0 &&
                invoke(s, 0x101878d0, {medical, staff, 1}) == 100 &&
                invoke(s, 0x101877e0, {medical, staff, 1}) == 100, "Original readouts are not healthy and 100%");
        require(!invoke(s, 0x10186e10, {medical, staff}), "Pending medical update survived");
        for (unsigned category = 0; category < 4; ++category) {
            const auto index = indexes + s.memory.read<4>(staff) * 0x2c + category * 8;
            const auto group = signed_value<1>(s.memory.read<1>(index));
            if (group < 0) continue;
            const auto rules = std::uint32_t(s.memory.read<4>(s.memory.read<4>(discipline) + unsigned(group) * 4));
            require(!invoke(s, 0x105bc600, {rules, staff, 0, 0, 0}), "Active suspension survived");
        }
        require(!s.memory.read<1>(indexes + s.memory.read<4>(staff) * 0x2c + 0x28), "Deferred suspension survived");
    }
    require(invoke(s, 0x1018c1a0, {medical, selected[0]}), "Recovery lost completed injury history");
    require(invoke(s, 0x10186e10, {medical, outsider}), "Recovery removed opponent medical update");
    require(s.memory.copy_out(condition(outsider), 0x24) == outside_condition &&
            s.memory.copy_out(outside_injury, 0x38) == outside_medical &&
            s.memory.copy_out(indexes + s.memory.read<4>(outsider) * 0x2c, 0x2c) == outside_discipline,
            "Recovery changed the opponent");
    require(squad(s, tactics, club) == lineup && s.memory.copy_out(formation, 0x18e0) == original_tactics &&
            s.memory.copy_out(club + 0x98, 200) == roster, "Recovery changed squad selection or tactics");
    require(recover_debug_squad(s, club) && !recover_debug_squad(s, 0), "Repeat recovery/invalid club guard failed");
    std::cout << "Recovery: " << players.size() << " actual roster players, " << suspended.size()
              << " suspension categories, medical history, queues, 100% readouts, opponent isolation and gating passed\n";
}
}
int main(int argc, char** argv) {
    try {
        check_assignment_policy();
        if (argc != 4) throw std::runtime_error("Expected original program, read-only game and private root");
        require(debug_text(9, DebugText::best_xi) == "Melhor 11" && debug_text(0, DebugText::best_xi) == "Best XI",
                "Portuguese/English Best XI labels");
        require(debug_text(9, DebugText::add_goal) == "Adicionar Gol" && debug_text(0, DebugText::add_goal) == "Add Goal" &&
                debug_text(9, DebugText::goal) == "Gol" && debug_text(0, DebugText::goal) == "Goal", "Goal labels");
        for (unsigned language = 0; language < 11; ++language)
            for (auto key : {DebugText::best_xi, DebugText::add_goal, DebugText::goal, DebugText::on, DebugText::off, DebugText::recover_squad, DebugText::extend_contract, DebugText::cancel_sale, DebugText::remove_sale_clause})
                require(!debug_text(language, key).empty(), "Missing supported-language label");
        require(debug_text(255, DebugText::best_xi) == "Best XI", "Unknown language fallback");
        require(debug_text(9, DebugText::recover_squad) == "Recuperar" &&
                debug_text(0, DebugText::recover_squad) == "Recover" &&
                debug_text(255, DebugText::recover_squad) == "Recover", "Recovery localization/fallback");
        require(debug_text(9, DebugText::extend_contract) == "Renovar Contrato por 1 Ano" &&
                debug_text(0, DebugText::extend_contract) == "Extend Contract by 1 Year" &&
                debug_text(255, DebugText::extend_contract) == "Extend Contract by 1 Year", "Contract extension localization/fallback");
        require(debug_text(9, DebugText::cancel_sale) == "Cancelar Venda" &&
                debug_text(0, DebugText::cancel_sale) == "Cancel Sale" &&
                debug_text(255, DebugText::cancel_sale) == "Cancel Sale", "Sale cancellation localization/fallback");
        require(debug_text(9, DebugText::remove_sale_clause) == "Remover Cl\xe1usula de Venda" &&
                debug_text(0, DebugText::remove_sale_clause) == "Remove Release Clause" &&
                debug_text(255, DebugText::remove_sale_clause) == "Remove Release Clause", "Sale clause localization/fallback");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        auto live = create_match({argv[1], argv[2], root}, {677, 5677, 12345, 2001, 194});
        auto& s = *live.state;
        s.budget = s.executed + 500000000;
        // The standalone match world does not normally need the career's
        // transfer/availability services; construct the real empty services.
        for (const auto spec : std::array<std::array<unsigned, 3>, 5>{{
                 {0x7bf0, 0x20, 0x102cb930}, {0x7b40, 0xa7c, 0x103df0d0},
                 {0x7ab8, 4, 0x100f84e0}, {0x7b24, 0x10, 0x100031b0},
                 {0x7aa4, 0x2c, 0x100de410}}}) {
            const auto object = invoke(s, 0x10773520, {spec[1]});
            invoke(s, spec[2], {object, 0});
            s.memory.write<4>(s.memory.read<4>(s.r<4>(8) - spec[0]), object);
        }
        std::cout << "Original availability services ready\n" << std::flush;
        const auto club = std::uint32_t(s.memory.read<4>(live.match + 0x7aac + 0x34));
        const auto formation = std::uint32_t(s.memory.read<4>(live.match + 0x7aac + 0x30));
        const auto tactics = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7a6c)));
        require(club && formation && tactics, "Missing original selection context");
        const auto before = squad(s, tactics, club);
        initialize_native_debugging(false);
        const auto executed = s.executed;
        require(!select_debug_best_xi(s, club, 0, formation, 0) && s.executed == executed && squad(s, tactics, club) == before,
                "Disabled Best XI changed selection");
        select_native_debugging(true);
        std::cout << "Selecting actual squad\n" << std::flush;
        const auto original_formation = s.memory.copy_out(formation, 0x18e0);
        const auto registers = s.registers;
        const auto unique = s.unique;
        const auto pc = s.pc;
        require(select_debug_best_xi(s, club, 0, formation, 0), "Best XI failed on an actual club");
        require(s.registers == registers && s.unique == unique && s.pc == pc, "Best XI clobbered caller");
        require(s.memory.copy_out(formation, 0x18e0) == original_formation, "Best XI changed chosen tactics");
        const auto first = squad(s, tactics, club);
        std::set<std::uint32_t> seen;
        for (auto staff : first) {
            require(staff && seen.insert(staff).second, "Incomplete or duplicate actual squad");
            const auto status = invoke(s, 0x10060c00, {staff, club, 0});
            require(!((status >= 1 && status <= 6) || status == 8 || status == 21), "Unavailable player selected");
        }
        require(invoke(s, 0x10117490, {std::uint32_t(s.memory.read<4>(first[0] + 0x14))}) & 1, "Starting goalkeeper is not a keeper");
        check_original_attribute_selection(s, club, tactics, formation, first[1], first[2]);
        // A real fixture with five bench places, and no extra stale reserves.
        const auto fixture = invoke(s, 0x10773520, {0x54});
        s.memory.copy_in(fixture, s.memory.copy_out(s.memory.read<4>(live.match + 0x3118), 0x54));
        s.memory.write<1>(fixture + 0x4e, 0x53);
        require(select_debug_best_xi(s, club, 0, formation, fixture), "Fixture selection failed");
        const auto five = squad(s, tactics, club);
        for (unsigned i = 0; i < 20; ++i) require(bool(five[i]) == (i < 16), "Competition bench limit ignored");
        // A changed formation must be read at click time, not cached or replaced.
        const auto alternate = invoke(s, 0x10773520, {0x18e0});
        s.memory.copy_in(alternate, original_formation);
        for (unsigned i = 1; i < 11; ++i) s.memory.write<2>(alternate + 0x12e + i * 2, 0x240);
        require(select_debug_best_xi(s, club, 0, alternate, 0), "Alternate formation failed");
        require(squad(s, tactics, club) != first, "Formation did not affect selection");
        // Keep seven real eligible players: never duplicate one to invent an XI.
        const auto roster = s.memory.copy_out(club + 0x98, 200);
        for (unsigned i = 0; i < 50; ++i) s.memory.write<4>(club + 0x98 + i * 4, i < 7 ? first[i] : 0);
        require(select_debug_best_xi(s, club, 0, formation, 0), "Depleted club selection failed");
        seen.clear();
        for (auto staff : squad(s, tactics, club)) if (staff) require(seen.insert(staff).second, "Depleted squad duplicated a player");
        require(seen.size() == 7, "Depleted squad lost available players");
        for (unsigned i = 0; i < 50; ++i) s.memory.write<4>(club + 0x98 + i * 4, 0);
        require(select_debug_best_xi(s, club, 0, formation, 0), "Empty pool selection failed");
        require(squad(s, tactics, club) == std::array<std::uint32_t, 20>{}, "Empty pool retained unavailable players");
        s.memory.copy_in(club + 0x98, roster);
        const auto opponent = std::uint32_t(s.memory.read<4>(live.match + 0x7aac + 0xd8 + 0x34));
        check_recovery(s, club, opponent, tactics, formation);
        select_native_debugging(false);
        std::cout << "Best XI: live selection, availability, unique players, formation, bench limits, depleted pool, gating and 11 languages passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

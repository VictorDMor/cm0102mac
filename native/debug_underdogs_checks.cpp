#include "debug_underdogs.hpp"
#include "match_debugging.hpp"
#include "debug_localization.hpp"
#include "database_adapter.hpp"
#include "match_debugger_factory.hpp"
#include "match_debugger_checkpoint.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace cm::aot;
using namespace cm::aot::match_debugger;
void require(bool ok, const std::string& message) { if (!ok) throw std::runtime_error(message); }
std::uint32_t invoke(State& s, std::uint32_t entry, std::initializer_list<std::uint32_t> args) {
    unsigned reg = 3;
    for (auto value : args) s.wr<4>(reg++ * 4, value);
    s.wr<4>(4128, return_sentinel); s.pc = entry;
    s.budget = s.executed + fresh_match_step_limit;
    if (entry == 0x100d2330) database_run_original(s, entry);
    else run_until(s, return_sentinel);
    return std::uint32_t(s.r<4>(12));
}
struct Cleanup {
    std::filesystem::path root;
    ~Cleanup() { std::error_code ignored; std::filesystem::remove_all(root, ignored); }
};
using Records = std::map<std::uint32_t, std::vector<std::uint8_t>>;
Records records(const State& s, std::array<std::uint32_t, 2> clubs) {
    Records result;
    for (auto club : clubs) for (unsigned slot = 0; slot < 50; ++slot) {
        const auto staff = std::uint32_t(s.memory.read<4>(club + 0x98 + slot * 4));
        if (!staff) continue;
        const auto player = std::uint32_t(s.memory.read<4>(staff + 0x14));
        if (player) { result[player] = s.memory.copy_out(player, 0x48); result[staff + 0x64] = s.memory.copy_out(staff + 0x64, 1); }
    }
    return result;
}
void restored(const State& s, const Records& before) {
    for (const auto& [address, bytes] : before)
        require(s.memory.copy_out(address, bytes.size()) == bytes, "Attribute restoration failed at " + hex_address(address));
}
void profile(State& s, std::uint32_t staff, unsigned target) {
    const auto player = std::uint32_t(s.memory.read<4>(staff + 0x14));
    for (unsigned attribute = 1; attribute <= 31; ++attribute) {
        const auto value = invoke(s, 0x100d2330, {staff, player, attribute});
        require(value == target, "Original profile attribute " + std::to_string(attribute) + " is " +
            std::to_string(value) + ", expected " + std::to_string(target));
    }
}
void controls(State& s, std::uint32_t fixture) {
    const auto original_root = s.memory.read<4>(0x108b3ee0);
    const auto queue_slot = s.memory.read<4>(s.r<4>(8) - 0x7420);
    const auto original_queue = s.memory.read<4>(queue_slot);
    const auto root = allocate(s, 0x260000, true), page = allocate(s, 0x130000, true);
    const auto descriptor = allocate(s, 0x220, true), group = allocate(s, 0x18, true);
    const auto queue = allocate(s, 0x60, true), entries = allocate(s, 0x6c, true);
    s.memory.write<4>(0x108b3ee0, root);
    s.memory.write<4>(root + 0x24, page); s.memory.write<4>(root + 0x10, descriptor);
    s.memory.write<4>(descriptor + 0x1c, group);
    s.memory.write<4>(s.memory.read<4>(s.r<4>(8) - 0x7420), queue);
    s.memory.write<4>(queue + 0x1c, entries); s.memory.write<4>(entries, fixture);
    // The original Competition cell remains untouched when adding the menu.
    s.memory.write<2>(page + 0x129da6, 1);
    s.memory.write<4>(page + 0xba864 + 8, 2000);
    s.memory.write<4>(page + 0xba864 + 12, 2);
    const auto competition = s.memory.copy_out(page + 0xba864, 0x17c);
    initialize_native_debugging(false);
    append_fixture_debugging_controls(s);
    require(s.memory.read<2>(page + 0x129da6) == 1, "Off mode added fixture controls");
    select_native_debugging(true);
    const auto registers = s.registers;
    const auto unique = s.unique;
    const auto pc = s.pc;
    append_fixture_debugging_controls(s);
    require(s.registers == registers && s.unique == unique && s.pc == pc, "Builder corrupted UI caller");
    require(s.memory.copy_out(page + 0xba864, 0x17c) == competition, "Competition control changed");
    const auto count = s.memory.read<2>(page + 0x129da6);
    require(count == 4, "Expected Debugging button and two menu items");
    for (unsigned action : {30018u, 30019u}) {
        unsigned index = 0;
        for (; index < count; ++index)
            if (s.memory.read<4>(page + 0xba864 + index * 0x17c + 8) == action) break;
        require(index < count, "Missing underdog menu action");
        // Exercise the generated dispatch hook, including a stale click after Off.
        select_native_debugging(false);
        require(invoke(s, 0x1028bc90, {index}) == 0, "Off mode accepted a stale menu action");
        require(underdog_debugging(s).fixtures.empty(), "Off mode armed a fixture");
        select_native_debugging(true);
        require(invoke(s, 0x1028bc90, {index}) == std::uint32_t(-11), "Fixture action did not rebuild screen");
        require(underdog_debugging(s).fixtures.size() == 1, "Menu did not arm the displayed fixture");
        require(underdog_debugging(s).fixtures[0].mode == (action == 30018 ? UnderdogMode::normal : UnderdogMode::maximum),
            "Menu selected the wrong mode");
        finish_debug_underdogs(s, fixture);
    }
    require(debug_text(9, DebugText::underdogs) == "Zebras Vencem" &&
        debug_text(9, DebugText::underdogs_max) == "Zebras Vencem MAX", "Portuguese labels changed");
    s.memory.write<4>(0x108b3ee0, original_root);
    s.memory.write<4>(queue_slot, original_queue);
}
struct EngineReads final : MemoryObserver {
    State& s;
    unsigned reads = 0, attribute_reads = 0;
    std::map<std::uint32_t, std::uint8_t> expected;
    explicit EngineReads(State& state) : s(state) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void read(std::uint32_t address, const std::uint8_t* bytes, unsigned size) override {
        if (!underdog_debugging(static_cast<const State&>(s)).depth) return;
        ++reads;
        for (unsigned i = 0; i < size; ++i) if (const auto found = expected.find(address + i); found != expected.end()) {
            require(bytes[i] == found->second, "Engine read an incorrect temporary attribute");
            ++attribute_reads;
        }
    }
};
}
int main(int argc, char** argv) {
    try {
        require(argc == 4, "Expected original program, game and private output root");
        Cleanup cleanup{std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())};
        auto prepared = prepare_match({argv[1], argv[2], cleanup.root}, {2262, 5677, 12345, 2001, 194});
        auto& s = *prepared.state;
        const auto fixture = prepared.fixture;
        s.memory.write<1>(fixture + 0x43, 0xff);
        const std::array clubs{std::uint32_t(s.memory.read<4>(fixture + 0x1c)), std::uint32_t(s.memory.read<4>(fixture + 0x20))};
        const std::array fixtures{fixture};
        const auto home_reputation = s.memory.read<2>(clubs[0] + 0x1ec), away_reputation = s.memory.read<2>(clubs[1] + 0x1ec);
        auto before = records(s, clubs);
        initialize_native_debugging(false);
        require(!arm_debug_underdogs(s, fixtures, UnderdogMode::maximum), "Off mode armed debugging");
        restored(s, before);
        select_native_debugging(true);
        // Exhaust both profile scaling branches, all valid current abilities,
        // both modes and both locations of the underdog. Compare through the
        // game's original getter (not a duplicate of the encoding formula).
        for (unsigned weaker = 0; weaker < 2; ++weaker) for (auto mode : {UnderdogMode::normal, UnderdogMode::maximum}) {
            s.memory.write<2>(clubs[weaker] + 0x1ec, 1000); s.memory.write<2>(clubs[1-weaker] + 0x1ec, 9000);
            const auto a = prepared.rosters[weaker][0], b = prepared.rosters[1-weaker][0];
            for (unsigned goalkeeper : {0u, 14u, 15u, 20u}) for (unsigned ability = 1; ability <= 200; ++ability) {
                s.memory.write<1>(a.player + 0xf, goalkeeper); s.memory.write<2>(a.player + 4, ability);
                s.memory.write<1>(b.player + 0xf, goalkeeper); s.memory.write<2>(b.player + 4, ability);
                const auto original = records(s, clubs);
                require(arm_debug_underdogs(s, fixtures, mode), "Valid fixture rejected");
                {
                    DebugUnderdogScope scope(s, fixture);
                    profile(s, a.staff, 20);
                    if (mode == UnderdogMode::maximum) profile(s, b.staff, 5);
                    else {
                        require(s.memory.copy_out(b.player, 0x48) == original.at(b.player), "Normal mode changed the favorite");
                        require(s.memory.copy_out(b.staff + 0x64, 1) == original.at(b.staff + 0x64), "Normal mode changed favorite determination");
                    }
                }
                restored(s, original);
                finish_debug_underdogs(s, fixture);
            }
        }
        for (const auto& [address, bytes] : before) s.memory.copy_in(address, bytes);
        s.memory.write<2>(clubs[0] + 0x1ec, 1000); s.memory.write<2>(clubs[1] + 0x1ec, 1000);
        require(!arm_debug_underdogs(s, fixtures, UnderdogMode::maximum), "Equal reputations picked an underdog");
        s.memory.write<2>(clubs[1] + 0x1ec, 9000);
        controls(s, fixture);
        s.memory.write<1>(fixture + 0x43, 2);
        require(!arm_debug_underdogs(s, fixtures, UnderdogMode::normal), "Completed fixture accepted");
        s.memory.write<1>(fixture + 0x43, 0xff);
        // Duplicate/replacement clicks, copied queue core and exception rollback.
        require(arm_debug_underdogs(s, fixtures, UnderdogMode::maximum), "Could not arm MAX");
        require(arm_debug_underdogs(s, fixtures, UnderdogMode::maximum) && underdog_debugging(s).fixtures.size() == 1,
            "Repeated click duplicated the fixture");
        const auto copy = allocate(s, 0x54, true);
        s.memory.copy_in(copy, s.memory.copy_out(fixture, 0x54));
        s.memory.write<2>(copy + 0x28, s.memory.read<2>(copy + 0x28) + 1);
        { DebugUnderdogScope other_day(s, copy); restored(s, before); }
        s.memory.copy_in(copy, s.memory.copy_out(fixture, 0x54));
        try {
            DebugUnderdogScope scope(s, copy);
            DebugUnderdogScope nested(s, copy);
            for (const auto& player : prepared.rosters[0]) profile(s, player.staff, 20);
            for (const auto& player : prepared.rosters[1]) profile(s, player.staff, 5);
            throw std::runtime_error("injected interruption");
        } catch (const std::runtime_error& error) { require(std::string(error.what()) == "injected interruption", error.what()); }
        restored(s, before);
        finish_debug_underdogs(s, copy);
        require(underdog_debugging(s).fixtures.empty(), "Copied fixture completion did not disarm original");
        // Start an actual match through production with MAX, then run through
        // halftime to completion. Only this fixture is armed; player records
        // must be byte-exact after setup and after every engine return.
        require(arm_debug_underdogs(s, fixtures, UnderdogMode::maximum), "Could not arm production fixture");
        EngineReads observer(s);
        {
            const auto registers = s.registers;
            const auto unique = s.unique;
            const auto pc = s.pc;
            const auto random_before = s.memory.copy_out(0x108b3f00, 16);
            DebugUnderdogScope scope(s, fixture);
            for (const auto& [address, bytes] : before) for (unsigned i = 0; i < bytes.size(); ++i) {
                const auto value = std::uint8_t(s.memory.read<1>(address + i));
                if (value != bytes[i]) observer.expected.emplace(address + i, value);
            }
            require(s.registers == registers && s.unique == unique && s.pc == pc &&
                s.memory.copy_out(0x108b3f00, 16) == random_before, "Attribute scope altered registers or RNG pointers");
        }
        s.memory.date_observer = &observer;
        auto live = start_match(std::move(prepared.state), prepared.match, fixture, std::move(prepared.rosters));
        require(observer.attribute_reads > 0, "Production setup did not read temporary attributes");
        observer.attribute_reads = 0;
        s.memory.date_observer = nullptr;
        restored(s, before);
        // Factory binds its isolated -2 fixture to a detailed friendly after
        // setup; real career fixtures already have their final competition.
        finish_debug_underdogs(s, fixture);
        underdog_debugging(s).fixtures.clear();
        s.memory.write<1>(fixture + 0x43, 0xff);
        require(arm_debug_underdogs(s, fixtures, UnderdogMode::maximum), "Could not arm bound friendly");
        unsigned breaks = 0, steps = 0;
        for (; steps < 2000; ++steps) {
            observer.reads = 0; s.memory.date_observer = &observer;
            const auto result = invoke(s, 0x101d3f60, {live.match, 1});
            s.memory.date_observer = nullptr;
            require(observer.reads > 0, "Production advance bypassed temporary attributes");
            restored(s, before);
            if (result == 2) break;
            if (result == 4) {
                ++breaks;
                require(!underdog_debugging(s).fixtures.empty(), "Halftime disarmed the fixture");
            }
            require(result == 1 || result == 4, "Unexpected match result");
        }
        require(steps < 2000 && breaks && underdog_debugging(s).fixtures.empty(), "Match did not complete and disarm");
        require(observer.attribute_reads > 0, "Production match did not read temporary attributes");
        restored(s, before);
        s.memory.write<2>(clubs[0] + 0x1ec, home_reputation); s.memory.write<2>(clubs[1] + 0x1ec, away_reputation);
        std::cout << "PASS: original 31-attribute readouts for CA 1..200 and keeper branches; both modes/sides, full rosters, UI gating/actions, copied fixtures, exception rollback, production setup and completed match (" << steps + 1 << " advances)\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

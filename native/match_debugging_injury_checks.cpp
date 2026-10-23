#include "match_debugger_factory.hpp"
#include "match_debugger_checkpoint.hpp"
#include "match_debugging.hpp"
#include "direct_commands.hpp"
#include "ui_bridge.hpp"
#include "semantic_io_checks.hpp"
#include <chrono>

namespace {
using namespace cm::semantic_io_checks;
using namespace cm::aot::match_debugger;
std::uint32_t invoke(State& s, std::uint32_t entry, std::initializer_list<std::uint32_t> args) {
    unsigned reg = 3;
    for (auto value : args) s.wr<4>(reg++ * 4, value);
    s.wr<4>(4128, return_sentinel); s.pc = entry;
    s.budget = s.executed + fresh_match_step_limit;
    run_until(s, return_sentinel);
    return std::uint32_t(s.r<4>(12));
}
std::uint32_t actor(std::uint32_t match, unsigned player) { return match + 0x311c + player * 0x1c4; }
std::uint32_t controller(const State& s) { return s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7ba0)); }
std::uint32_t injury_record(const State& s, std::uint32_t match, unsigned player) {
    const auto manager = controller(s);
    const auto staff = s.memory.read<4>(actor(match, player) + 0x6c);
    const auto row = s.memory.read<4>(manager) + s.memory.read<4>(staff) * 0x24;
    const auto index = s.memory.read<4>(row + 0x10);
    require(index != 0xffffffff, "Player has no actual medical injury record");
    return std::uint32_t(s.memory.read<4>(manager + 4) + index * 0x38);
}
template<class Action> void rejects(State& s, Action action) {
    const Image before(s);
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Invalid injury was accepted");
    before.compare(s, "Rejected injury");
}
struct Cleanup {
    std::filesystem::path root;
    ~Cleanup() { std::error_code ignored; std::filesystem::remove_all(root, ignored); }
};
struct Screen {
    std::uint32_t root, page, descriptor, fixture, status, report;
    Screen(State& s, std::uint32_t match) {
        root = allocate(s, 0x260000, true); page = allocate(s, 0x130000, true);
        descriptor = allocate(s, 0x220, true); fixture = allocate(s, 0x80, true);
        status = allocate(s, 0x100, true); report = allocate(s, 0x1000, true);
        s.memory.write<4>(0x108b3ee0, root);
        s.memory.write<2>(root + 0x2c38, 1);
        s.memory.write<4>(root + 0x24, page); s.memory.write<4>(root + 0x10, descriptor);
        s.memory.write<4>(descriptor + 8, descriptor + 0x210);
        s.memory.write<4>(descriptor + 0x210, 0x1028dc80);
        s.memory.write<4>(descriptor + 0x14, fixture);
        s.memory.write<4>(fixture + 0x58, match); s.memory.write<4>(fixture + 0x5c, status);
        s.memory.write<4>(fixture + 0x60, report);
        s.memory.write<2>(page + 0x129da6, 3);
        for (unsigned i = 0; i < 3; ++i) {
            const auto control = page + 0xba864 + i * 0x17c;
            s.memory.write<4>(control + 8, i == 0 ? 2 : i == 1 ? 30002 : 30003);
            s.memory.write<4>(control + 12, 2);
            s.memory.write<4>(control + 0x48, 8);
        }
        idle(s);
    }
    void idle(State& s) const {
        s.memory.write<2>(page + 0x12a904, 0xffff);
        s.memory.write<1>(s.memory.read<4>(0x108b4644), 0);
        s.wr<4>(4128, 0x1076ff7c);
    }
    void dispatch(State& s, unsigned control = 0xffffffff) const {
        s.wr<4>(12, control); s.wr<4>(4128, return_sentinel);
        s.budget = s.executed + fresh_match_step_limit;
        require(dispatch_match_debugging_control(s), "Match handler did not consume shortcut");
        require(s.r<4>(12) == std::uint32_t(-11), "Accepted shortcut did not rebuild match screen");
        require(!s.memory.read<2>(root + 0x25819e), "Shortcut leaked into the next screen");
    }
};
}
int main(int argc, char** argv) {
    try {
        require(argc == 4, "Expected original program, game and private check root");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        Cleanup cleanup{root};
        auto engine = create_match({argv[1], argv[2], root}, {677, 5677, 12345, 2001, 194});
        auto& s = *engine.state; const auto match = engine.match;
        initialize_native_debugging(true);
        rejects(s, [&] { award_debug_injury(s, match, 8, 5); });
        invoke(s, 0x101d3f60, {match, 1});
        s.memory.write<2>(match + 0x7948, 137);
        select_native_debugging(false);
        rejects(s, [&] { award_debug_injury(s, match, 8, 5); });
        select_native_debugging(true);
        rejects(s, [&] { award_debug_injury(s, match, 11, 5); });
        rejects(s, [&] { award_debug_injury(s, match, 40, 5); });
        rejects(s, [&] { award_debug_injury(s, match, 8, 77); });
        for (unsigned player = 0; player < 40; ++player)
            require(debug_player_weight(s, match, player, DebugMatchEvent::injury) == (player % 20 < 11 ? 1u : 0u),
                "Injury selection must be uniform and include both keepers, excluding substitutes");
        {
            const Image before(s);
            for (unsigned draw = 0; draw < 1000; ++draw) {
                const auto player = random_debug_player(s, match, draw % 2, DebugMatchEvent::injury);
                require(player && *player / 20 == draw % 2 && *player % 20 < 11, "Invalid random injury player");
                require(random_debug_injury() < 77, "Random injury selected a fitness status");
            }
            before.compare(s, "Random selection consumed original RNG or mutated state");
        }
        const Checkpoint baseline(s);
        // Exercise every selectable medical type, with keepers and outfield
        // players on both sides; compare complete original registration records.
        for (unsigned type = 0; type < 77; ++type) {
            auto actual = baseline.restore(), expected = baseline.restore();
            const unsigned player = std::array{8u, 28u, 0u, 20u}[type % 4];
            const auto a = actor(match, player), staff = std::uint32_t(s.memory.read<4>(a + 0x6c));
            const auto fixture = std::uint32_t(actual->memory.read<4>(match + 0x3118));
            const auto fixture_before = actual->memory.copy_out(fixture, 0x58);
            const auto registers = actual->registers; const auto scratch = actual->unique; const auto pc = actual->pc;
            // Model original post-match registration with a finished fixture
            // copy, while leaving the actual in-progress fixture unchanged.
            const auto context = allocate(*expected, 0x58, false);
            expected->memory.copy_in(context, fixture_before);
            for (unsigned side = 0; side < 2; ++side) {
                const auto score = expected->memory.read<1>(match + (side ? 0xe073 : 0xe03d));
                expected->memory.write<1>(context + 0x43 + side, score);
                expected->memory.write<1>(context + 0x47 + side, score);
            }
            expected->wr<4>(4, expected->r<4>(4) - 0x400);
            invoke(*expected, 0x1018b9f0, {controller(*expected), staff, 0});
            expected->memory.write<4>(expected->r<4>(4) + 0x38, 0);
            invoke(*expected, 0x10188cc0, {controller(*expected), type, 0, staff, context, 1, 0xffffffff, 0});
            expected->memory.write<1>(a + 0x1c, 1); expected->memory.write<1>(a + 0x1d, 1);
            invoke(*expected, 0x101f6c60, {match});
            actual->budget = actual->executed + fresh_match_step_limit;
            award_debug_injury(*actual, match, player, type);
            require(actual->registers == registers && actual->unique == scratch && actual->pc == pc, "Injury corrupted UI caller");
            const auto record = injury_record(*actual, match, player);
            require(actual->memory.read<1>(record) == type && signed_value<2>(actual->memory.read<2>(record + 0x30)) > 0,
                "Injury type or medical duration missing");
            require(actual->memory.read<1>(record + 0x20) == actual->memory.read<1>(match + (player < 20 ? 0xe03d : 0xe073)),
                "Live-match injury was registered as a training injury");
            require(actual->memory.copy_out(fixture, 0x58) == fixture_before, "Injury altered the live fixture");
            require(actual->memory.copy_out(record, 0x38) == expected->memory.copy_out(injury_record(*expected, match, player), 0x38),
                "Medical injury record differs from original registration");
            require(actual->memory.copy_out(match, 0xe0bc) == expected->memory.copy_out(match, 0xe0bc), "Original injury match effects differ");
            require(!debug_player_weight(*actual, match, player, DebugMatchEvent::injury), "Injured player remains eligible");
            rejects(*actual, [&] { award_debug_injury(*actual, match, player, type); });
            if (type < 4) for (unsigned tick = 0; tick < 10; ++tick) {
                invoke(*actual, 0x101d3f60, {match, 1}); invoke(*expected, 0x101d3f60, {match, 1});
                require(actual->memory.copy_out(match, 0xe0bc) == expected->memory.copy_out(match, 0xe0bc), "Injury did not resume original play");
            }
            std::cout << "injury=" << type << " player=" << player << " original-record-and-match=PASS\n";
        }
        auto live = baseline.restore(); Screen ui(*live, match);
        for (unsigned key = 1; key <= 6; ++key) for (bool keypad : {false, true}) {
            auto state = Checkpoint(*live).restore();
            UIEvent event{UIEvent::Kind::key_down};
            event.message = (std::array{0u, 18u, 19u, 20u, 21u, 23u, 22u}[key] << 8) | ('0' + key);
            if (keypad) event.message = (std::array{0u, 83u, 84u, 85u, 86u, 87u, 88u}[key] << 8) | ('0' + key);
            event.modifiers = 0x81;
            require(deliver_match_shortcut(*state, original_command_context(*state, 0xffff), event), "Number row shortcut not delivered");
            ui.dispatch(*state);
            const unsigned side = key <= 2 ? key - 1 : key % 2;
            unsigned affected = 0;
            for (unsigned player = 0; player < 40; ++player) {
                const auto a = actor(match, player);
                const auto offset = key <= 2 ? 0xc : key <= 4 ? 0x23 : 0x1c;
                if (state->memory.read<1>(a + offset) == live->memory.read<1>(a + offset)) continue;
                require(player / 20 == side && player % 20 < 11 && (key > 2 || player % 20 != 0), "Shortcut affected the wrong player/side");
                ++affected;
                if (key >= 5) require(state->memory.read<1>(injury_record(*state, match, player)) < 77, "Shortcut did not assign an injury");
            }
            require(affected == 1, "Shortcut must affect exactly one eligible player");
        }
        for (unsigned modifiers : {0x100u, 0x800u, 0x1000u}) {
            UIEvent event{UIEvent::Kind::key_down}; event.message = '1'; event.modifiers = modifiers;
            const Image before(*live);
            require(!deliver_match_shortcut(*live, original_command_context(*live, 0xffff), event), "Modified shortcut accepted");
            before.compare(*live, "Modified shortcut");
        }
        // Random goals must leave the manually selected repeat scorer intact.
        ui.dispatch(*live, 1);
        ui.idle(*live);
        UIEvent away_goal{UIEvent::Kind::key_down}; away_goal.message = '2';
        require(deliver_match_shortcut(*live, original_command_context(*live, 0xffff), away_goal), "Random away goal unavailable");
        ui.dispatch(*live);
        const auto manual_goals = live->memory.read<1>(actor(match, 8) + 0xc);
        ui.dispatch(*live, 2);
        require(live->memory.read<1>(actor(match, 8) + 0xc) == manual_goals + 1,
            "Random goal replaced the repeat button's manually chosen scorer");
        ui.idle(*live);
        UIEvent event{UIEvent::Kind::auto_key}; event.message = '1';
        { const Image before(*live); require(deliver_match_shortcut(*live, original_command_context(*live, 0xffff), event), "Repeat not swallowed");
          before.compare(*live, "Held shortcut"); }
        select_native_debugging(false); event.kind = UIEvent::Kind::key_down;
        require(!deliver_match_shortcut(*live, original_command_context(*live, 0xffff), event), "Disabled debugging accepted shortcut");
        event.message = 'z';
        require(deliver_match_shortcut(*live, original_command_context(*live, 0xffff), event), "Z must work independently of debugging");
        require(live->memory.read<2>(ui.page + 0x12a904) == 0, "Z did not select original play/pause control");
        ui.idle(*live); live->memory.write<2>(ui.root + 0x2c3a, 1);
        require(!deliver_match_shortcut(*live, original_command_context(*live, 0xffff), event), "Shortcut escaped into modal dialog");
        live->memory.write<2>(ui.root + 0x2c3a, 0);
        live->memory.write<4>(ui.descriptor + 0x210, 0x100ee9d0);
        require(!deliver_match_shortcut(*live, original_command_context(*live, 0xffff), event), "Z escaped into a file dialog");
        std::cout << "PASS: 77 actual injury records, original withdrawal effects, 40 following steps, rejection, uniform player pool, both number rows, exact H/A mapping, Z original control and repeat/modifier/modal guards\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

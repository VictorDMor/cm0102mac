#include "aot_runtime.hpp"
#include "match_debugging.hpp"
#include "debug_localization.hpp"
#include "match_replay.hpp"
#include "database_adapter.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
using namespace cm::aot;
void require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
std::uint32_t invoke(State& s, unsigned entry, std::initializer_list<unsigned> args) {
    unsigned reg = 3;
    for (auto value : args) s.wr<4>(reg++ * 4, value);
    s.wr<4>(4128, return_sentinel); s.pc = entry; s.budget = s.executed + 100000000;
    database_run_original(s, entry);
    return std::uint32_t(s.r<4>(12));
}
std::vector<std::uint8_t> rng(const State& s) {
    auto result = s.memory.copy_out(0x108b3f00, 16);
    for (auto global : {0x108b3f00u, 0x108b3f08u, 0x108b3f0cu}) {
        const auto bytes = s.memory.copy_out(s.memory.read<4>(global), 4);
        result.insert(result.end(), bytes.begin(), bytes.end());
    }
    const auto samples = s.memory.copy_out(s.memory.read<4>(0x108b3f04), 102000);
    result.insert(result.end(), samples.begin(), samples.end());
    return result;
}
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected an existing career snapshot");
        auto state = std::make_unique<State>(); load_match_snapshot(*state, argv[1]);
        auto& s = *state;
        s.wr<4>(8, 0x108bba90);
        const auto global = [&](unsigned offset) { return std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - offset))); };
        const auto people = global(0x7be0), count = global(0x7be4);
        std::uint32_t staff = 0, neighbor = 0, nonplayer = 0;
        for (unsigned i = 0; i < count; ++i) {
            const auto p = people + i * 0x70;
            if (!s.memory.read<4>(p + 0x14)) nonplayer = p;
            else if (!staff) staff = p;
            else if (!neighbor) neighbor = p;
        }
        require(staff && neighbor && nonplayer, "Missing real player and non-player fixtures");
        const auto player = std::uint32_t(s.memory.read<4>(staff + 0x14));
        const auto other = std::uint32_t(s.memory.read<4>(neighbor + 0x14));
        const auto saved_staff = s.memory.copy_out(staff, 0x70), saved_player = s.memory.copy_out(player, 0x48);
        const auto saved_other = s.memory.copy_out(other, 0x48), saved_neighbor = s.memory.copy_out(neighbor, 0x70);
        const auto random = rng(s);
        initialize_native_debugging(false);
        require(!maximize_debug_player_attributes(s, staff), "Disabled action changed attributes");
        select_native_debugging_features(all_debug_features & ~(1u << unsigned(DebugFeature::maximize_attributes)));
        require(!maximize_debug_player_attributes(s, staff), "Another feature enabled maximization");
        require(s.memory.copy_out(staff, 0x70) == saved_staff && s.memory.copy_out(player, 0x48) == saved_player,
                "Disabled maximization wrote player records");
        select_native_debugging_features(1u << unsigned(DebugFeature::maximize_attributes));
        for (auto invalid : {0u, nonplayer, staff + 1, people + count * 0x70, 0xfffffff0u})
            require(!maximize_debug_player_attributes(s, invalid), "Invalid staff accepted");
        for (auto invalid : {0u, player + 1, global(0x7b38) + global(0x74d4) * 0x48, 0xfffffff0u}) {
            s.memory.write<4>(staff + 0x14, invalid);
            require(!maximize_debug_player_attributes(s, staff), "Invalid player accepted");
        }
        s.memory.write<4>(staff + 0x14, player);
        const auto registers = s.registers; const auto unique = s.unique; const auto pc = s.pc;
        require(maximize_debug_player_attributes(s, staff), "Valid player rejected");
        require(s.registers == registers && s.unique == unique && s.pc == pc && rng(s) == random,
                "Maximization changed caller state or game RNG");
        const auto maximized_staff = s.memory.copy_out(staff, 0x70), maximized_player = s.memory.copy_out(player, 0x48);
        // Exact write boundaries: CA/PA, positions, identity, club and job are untouched.
        for (unsigned i = 0; i < saved_staff.size(); ++i)
            require(maximized_staff[i] == ((i == 0x61 || (i >= 0x63 && i <= 0x69)) ? 20 : saved_staff[i]),
                    "Personality maximum or unrelated staff field differs");
        for (unsigned i = 0; i < saved_player.size(); ++i)
            if (i < 0x1b || i > 0x44) require(maximized_player[i] == saved_player[i], "Unrelated player field changed");
        require(maximized_player[0x25] == 1 && maximized_player[0x2d] == 1,
                "Dirtiness and injury proneness must be minimized");
        for (unsigned offset : {0x21u, 0x22u, 0x2cu, 0x30u, 0x34u, 0x3bu, 0x42u})
            require(maximized_player[offset] == 20, "Hidden positive attribute not maximized");
        for (unsigned offset : {0x35u, 0x38u, 0x41u})
            require(maximized_player[offset] == 100, "Hidden technical attribute not maximized");
        require(maximize_debug_player_attributes(s, staff) && s.memory.copy_out(staff, 0x70) == maximized_staff &&
                s.memory.copy_out(player, 0x48) == maximized_player, "Repeated action is not idempotent");
        for (unsigned ability : {0u, 1u, 100u, 200u}) for (unsigned goalkeeper : {0u, 15u, 20u}) {
            s.memory.write<2>(player + 4, ability);
            s.memory.write<1>(player + 0xf, goalkeeper);
            for (unsigned attribute = 1; attribute <= 31; ++attribute)
                require(invoke(s, 0x100d2330, {staff, player, attribute}) == 20,
                        "Original profile getter did not return 20");
        }
        // The original normalizer and ordinary stores must remain free to change attributes.
        invoke(s, 0x100c2cb0, {staff});
        require(s.memory.copy_out(player + 0x1b, 42) != std::vector<std::uint8_t>(maximized_player.begin() + 0x1b, maximized_player.begin() + 0x45),
                "Original normalization appears locked");
        s.memory.write<1>(player + 0x1b, 7); s.memory.write<1>(player + 0x2d, 9);
        s.memory.write<1>(staff + 0x64, 8);
        require(s.memory.read<1>(player + 0x1b) == 7 && s.memory.read<1>(player + 0x2d) == 9 &&
                s.memory.read<1>(staff + 0x64) == 8, "Ordinary attribute writes were intercepted");
        // No employment or primary contract is needed by the action.
        s.memory.write<4>(staff + 0x24, 0);
        require(maximize_debug_player_attributes(s, staff) && !s.memory.read<4>(staff + 0x24), "Unattached player rejected or employed");
        require(s.memory.copy_out(neighbor, 0x70) == saved_neighbor && s.memory.copy_out(other, 0x48) == saved_other,
                "Another player's records changed");
        for (unsigned language = 0; language < 11; ++language)
            require(!debug_text(language, DebugText::maximize_attributes).empty(), "Missing maximum label");
        require(debug_text(9, DebugText::maximize_attributes) == "Maximizar Atributos" &&
                debug_text(255, DebugText::maximize_attributes) == "Maximize Attributes", "Maximum labels differ");
        std::cout << "Maximize Attributes: 372 original profile readings, hidden/negative values, isolation, caller/RNG, toggles, invalid records and unlocked normalization passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

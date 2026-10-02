#include "match_replay.hpp"
#include "match_turbo.hpp"
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

int main(int argc, char** argv) {
    bool turbo = false;
    for (int index = 3; index < argc; ++index) {
        const std::string_view option(argv[index]);
        if (option == "--turbo" && !turbo) turbo = true;
        else {
            std::cerr << "Usage: cm-match-replay <private-snapshot> <instruction-limit> [--turbo]\n";
            return 2;
        }
    }
    if (argc < 3) {
        std::cerr << "Usage: cm-match-replay <private-snapshot> <instruction-limit> [--turbo]\n";
        return 2;
    }
    try {
        std::uint64_t limit = 0;
        const std::string_view input(argv[2]);
        const auto parsed = std::from_chars(input.data(), input.data() + input.size(), limit);
        if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size() ||
            limit == 0 || limit > 10000000)
            throw std::runtime_error("Instruction limit must be 1 through 10000000");
        if (turbo) {
            setenv("CM_AOT_MATCH_REPLAY_TURBO", "1", 1);
            cm::aot::set_match_turbo_enabled(true);
        }
        auto state = std::make_unique<cm::aot::State>();
        cm::aot::load_match_snapshot(*state, argv[1]);
        if (state->executed > UINT64_MAX - limit)
            throw std::runtime_error("Private match instruction count would overflow");
        state->budget = state->executed + limit - 1;
        std::string_view stop = "return";
        try {
            while (state->pc != cm::aot::return_sentinel) {
                if (cm::aot::import_name(state->pc)) { stop = "native-import"; break; }
                if (!cm::aot::dispatch_page(*state))
                    throw std::runtime_error("Uncompiled original address");
            }
        } catch (const std::runtime_error& error) {
            if (std::string_view(error.what()) != "Native instruction budget exhausted") throw;
            stop = "instruction-limit";
        }
        std::uint64_t register_hash = UINT64_C(14695981039346656037);
        for (const auto byte : state->registers) {
            register_hash ^= byte;
            register_hash *= UINT64_C(1099511628211);
        }
        const auto memory_hash = cm::aot::match_snapshot_memory_hash(*state);
        std::cout << "mode=" << (turbo ? "turbo" : "normal")
                  << " stop=" << stop << " pc=" << cm::aot::hex_address(state->pc)
                  << " instructions=" << state->executed
                  << " register-hash=" << std::hex << register_hash
                  << " memory-hash=" << memory_hash << std::dec << '\n';
        return stop == "native-import" ? 3 : 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

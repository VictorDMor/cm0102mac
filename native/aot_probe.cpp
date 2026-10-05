#include "calendar_adapter.hpp"
#include "date_subsystem_audit.hpp"
#include "rng_core_adapter.hpp"
#include "aot_runtime.hpp"
#include "platform.hpp"
#include "probe_host.hpp"
#include "application_exit.hpp"
#include <iostream>
#include <memory>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <charconv>

int cm::aot::run_probe(int argc, char** argv) {
    if (argc != 3 && argc != 5) {
        std::cerr << "Usage: cm-aot-probe <relocated-segment-directory> <original-entry-hex> [<game-directory> <writable-directory>]\n";
        return 2;
    }
    auto state = std::make_unique<cm::aot::State>();
    std::unique_ptr<cm::aot::MemoryObserver> rng_storage;
    try {
        if (const char* value = std::getenv("CM_AOT_INSTRUCTION_BUDGET")) {
            const std::string input(value);
            if (input == "unlimited") state->budget = UINT64_MAX;
            else {
                const auto parsed = std::from_chars(input.data(), input.data() + input.size(), state->budget);
                if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size() || !state->budget || state->budget > 10000000000ULL)
                    throw std::runtime_error("CM_AOT_INSTRUCTION_BUDGET must be unlimited or an integer from 1 through 10000000000");
            }
        }
        cm::aot::load_program(*state, argv[1]);
        // Private reference capture must never record the semantic candidate
        // as its own oracle. Normal apps retain the production defaults.
        state->calendar_original = std::getenv("CM_AOT_SEASON_REFERENCE") != nullptr || std::getenv("CM_AOT_DATE_ORIGINAL") != nullptr;
        state->rng_original = std::getenv("CM_AOT_SEASON_REFERENCE") != nullptr || std::getenv("CM_AOT_RNG_ORIGINAL") != nullptr;
        state->rng_trace = std::getenv("CM_AOT_VALIDATION_EVENT_POINTER") != nullptr;
        if (state->rng_trace && std::getenv("CM_AOT_RNG_STORAGE_AUDIT"))
            rng_storage = cm::aot::observe_rng_storage(*state);
        if(const auto* date_edges=std::getenv("CM_AOT_DATE_RECORD")) {
            if(!state->calendar_original || !state->rng_original)
                throw std::runtime_error("App Date recording requires explicit all-original Date and RNG selection");
            open_date_subsystem_audit(date_edges,true);
        }
        if (argc == 5) state->platform = std::make_shared<cm::aot::NativePlatform>(argv[3], argv[4]);
        state->pc = std::stoul(argv[2], nullptr, 0);
        cm::aot::run(*state);
        std::cout << "Original routine returned after " << state->executed
                  << " statically recompiled instructions; r3="
                  << cm::aot::hex_address(state->r<4>(12)) << '\n';
        return 0;
    } catch (const ApplicationExit&) {
        close_date_subsystem_audit(true);
        std::cout << calendar_call_summary(*state) << '\n' << date_subsystem_call_summary(*state);
        std::cout << rng_core_call_summary(*state);
        std::cout << "Original application requested a normal exit after " << state->executed
                  << " statically recompiled instructions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\nOriginal PC: " << cm::aot::hex_address(state->pc)
                  << "; executed: " << state->executed
                  << "; native calls: " << state->native_calls
                  << "; LR: " << cm::aot::hex_address(state->r<4>(4128)) << '\n';
        for (unsigned r = 0; r < 32; ++r) {
            std::cerr << "r" << r << "=" << cm::aot::hex_address(state->r<4>(r * 4));
            std::cerr << (r % 4 == 3 ? '\n' : ' ');
        }
        try {close_date_subsystem_audit();} catch (...) {}
        // Original PowerPC ABI back chains, read only after a failed run. Each
        // callee saves its return address in its caller's linkage area.
        auto frame = std::uint32_t(state->r<4>(4));
        for (unsigned depth = 0; frame && depth < 24; ++depth) {
            try {
                const auto caller = std::uint32_t(state->memory.read<4>(frame));
                if (!caller || caller <= frame) break;
                const auto return_address = state->memory.read<4>(std::uint64_t(caller) + 8);
                std::cerr << "Original stack: frame=" << cm::aot::hex_address(frame)
                          << " return=" << cm::aot::hex_address(return_address) << '\n';
                frame = caller;
            } catch (const std::exception&) {
                std::cerr << "Original stack: unreadable back chain\n";
                break;
            }
        }
        if (const char* directory = std::getenv("CM_AOT_CAPTURE_DIRECTORY"); directory && state->platform) {
            try {
                for (const auto& path : state->platform->save_window_images(directory))
                    std::cerr << "Saved native window image: " << path << '\n';
            } catch (const std::exception& capture_error) {
                std::cerr << "Native window capture failed: " << capture_error.what() << '\n';
            }
        }
        if (const char* addresses = std::getenv("CM_AOT_DUMP_ADDRESSES")) {
            std::istringstream input(addresses);
            std::string value;
            while (std::getline(input, value, ',')) {
                try {
                    const auto address = std::stoul(value, nullptr, 0);
                    const auto bytes = state->memory.copy_out(address, 256);
                    for (std::size_t offset = 0; offset < bytes.size(); offset += 16) {
                        std::cerr << cm::aot::hex_address(address + offset) << ": ";
                        for (unsigned i = 0; i < 16; ++i)
                            std::cerr << std::hex << std::setw(2) << std::setfill('0') << unsigned(bytes[offset + i]) << ' ';
                        std::cerr << std::dec << ' ';
                        for (unsigned i = 0; i < 16; ++i) {
                            const auto byte = bytes[offset + i];
                            std::cerr << char(byte >= 32 && byte < 127 ? byte : '.');
                        }
                        std::cerr << '\n';
                    }
                } catch (const std::exception&) { std::cerr << "Cannot inspect requested original data address\n"; }
            }
        }
        if (state->pc == 0x10bf7c40) {
            const auto address = state->r<4>(20);
            try {
                auto bytes = state->memory.copy_out(address + 1, state->memory.read<1>(address));
                std::cerr << "FSMakeFSSpec name bytes:";
                for (auto b : bytes) std::cerr << ' ' << std::hex << unsigned(b);
                std::cerr << std::dec << '\n';
            } catch (const std::exception&) { std::cerr << "Unreadable FSSpec name\n"; }
        }
        return 1;
    }
}

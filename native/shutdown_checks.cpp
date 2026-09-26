#include "aot_runtime.hpp"
#include "application_exit.hpp"
#include "callbacks.hpp"
#include "platform.hpp"
#include "probe_host.hpp"
#include <iostream>
#include <unistd.h>

using namespace cm::aot;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct TempDirectory {
    std::filesystem::path path;
    TempDirectory() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-shutdown-check-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create shutdown test directory");
        path = pattern;
    }
    ~TempDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
};
void prepare(State& s, std::uint32_t entry) {
    s.pc = entry;
    s.wr<4>(4, 0x700fff00); s.wr<4>(8, 0x108bba90); s.wr<4>(4128, return_sentinel);
    for (unsigned r = 14; r < 32; ++r) s.wr<4>(r * 4, 0xcafe0000u + r);
    s.budget = s.executed + 10000;
}
void original(State& s, std::uint32_t entry) {
    prepare(s, entry);
    run(s);
    require(s.pc == return_sentinel && s.r<4>(4) == 0x700fff00 && s.r<4>(8) == 0x108bba90,
            "Original cursor lifecycle changed stack, TOC, or return address");
    for (unsigned r = 14; r < 32; ++r)
        require(s.r<4>(r * 4) == 0xcafe0000u + r, "Original cursor lifecycle changed a nonvolatile register");
}
}
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "Usage: cm-shutdown-checks <original-segments> <game-directory>\n"; return 2; }
    auto state = std::make_unique<State>();
    auto& s = *state;
    try {
        TempDirectory temporary;
        load_program(s, argv[1]);
        s.platform = std::make_shared<NativePlatform>(argv[2], temporary.path, AudioMode::offline);
        unsigned colored = 0, monochrome = 0;
        for (unsigned repeat = 0; repeat < 3; ++repeat) {
            original(s, 0x1076e7c0); // Actual three-cursor initialization, including monochrome fallback.
            const auto table = s.memory.read<4>(0x108b7e9c);
            std::map<std::uint32_t, std::uint32_t> payloads;
            std::uint32_t color = 0;
            auto remember = [&](std::uint32_t handle) {
                require(handle != 0, "Original cursor handle was not allocated");
                payloads.emplace(handle, s.memory.read<4>(handle));
            };
            for (unsigned i = 0; i < 3; ++i) {
                const auto handle = std::uint32_t(s.memory.read<4>(table + i * 8));
                if (!handle) continue; // Missing cursor 128 uses the original arrow fallback.
                remember(handle);
                if (s.memory.read<1>(table + i * 8 + 4)) {
                    color = handle; ++colored;
                    const auto data = s.memory.read<4>(handle);
                    const auto map = s.memory.read<4>(data + 2);
                    remember(map); remember(s.memory.read<4>(data + 6));
                    remember(s.memory.read<4>(s.memory.read<4>(map) + 42));
                } else {
                    ++monochrome;
                    const auto allocations = s.allocated;
                    s.wr<4>(12, 128 + i);
                    require(bridge_import(s, "CarbonLib::GetCursor") && s.r<4>(12) == handle,
                            "Repeated GetCursor did not reuse its Resource Manager handle");
                    s.wr<4>(12, 0x43555253); s.wr<4>(16, 128 + i);
                    require(bridge_import(s, "CarbonLib::GetResource") && s.r<4>(12) == handle && s.allocated == allocations,
                            "GetCursor and GetResource do not share the same original resource ownership");
                }
            }
            require(color != 0, "Original color cursor path was not exercised");
            s.wr<4>(12, color);
            require(bridge_import(s, "CarbonLib::SetCCursor"), "Native color cursor selection missing");
            original(s, 0x1076e870); // Actual quit cleanup: InitCursor, ReleaseResource, DisposeCCursor.
            for (const auto& [handle, payload] : payloads)
                require(!s.memory.read<4>(handle) && !s.allocated.contains(payload),
                        "Original quit left a cursor, PixMap, pixel buffer, or palette allocated");
            const auto allocations = s.allocated;
            bool rejected = false;
            s.wr<4>(12, color);
            try { (void)bridge_import(s, "CarbonLib::DisposeCCursor"); }
            catch (const std::runtime_error& error) { rejected = std::string(error.what()) == "Invalid or double DisposeCCursor"; }
            require(rejected && s.allocated == allocations, "Duplicate cursor disposal changed unrelated allocations");
        }
        // ExitToShell must escape nested original callbacks, not return as if
        // only that callback had completed and resume the application afterward.
        s.memory.map(0x30000000, 8);
        s.memory.write<4>(0x30000000, 0x10bf7bf8);
        s.memory.write<4>(0x30000004, 0);
        prepare(s, 0x1234);
        bool exited = false;
        try { (void)call_original(s, 0x30000000, {}); }
        catch (const ApplicationExit&) { exited = true; }
        require(exited, "Original exit returned into its caller instead of unwinding to the host");
        std::array<std::string, 3> arguments{"cm-aot-probe", argv[1], "0x10bf7bf8"};
        std::array<char*, 3> probe_arguments{arguments[0].data(), arguments[1].data(), arguments[2].data()};
        require(run_probe(probe_arguments.size(), probe_arguments.data()) == 0,
                "Application host reported a normal original exit as a diagnostic stop");
        std::cout << "Original cursor initialization/disposal passed: " << colored << " color and " << monochrome
                  << " monochrome cursors; nested exit and successful host shutdown passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << " at " << hex_address(s.pc) << "; LR=" << hex_address(s.r<4>(4128)) << '\n';
        return 1;
    }
}

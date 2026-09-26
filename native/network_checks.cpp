#include "aot_runtime.hpp"
#include "platform.hpp"
#include <CoreServices/CoreServices.h>
#include <iostream>
#include <unistd.h>

using namespace cm::aot;
namespace {
constexpr std::uint32_t object = 0x30000000, result = 0x300d0000;
constexpr auto initialization = "Apple;Carbon;Networking::InitOpenTransportInContext";
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct TempDirectory {
    std::filesystem::path path;
    TempDirectory() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-network-check-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create network test directory");
        path = pattern;
    }
    ~TempDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
};
void prepare(State& s, std::uint32_t entry) {
    s.pc = entry;
    s.wr<4>(4, 0x700fff00);
    s.wr<4>(8, 0x108bba90);
    s.wr<4>(4128, return_sentinel);
    for (unsigned r = 14; r < 32; ++r) s.wr<4>(r * 4, 0xcafe0000u + r);
    s.budget = s.executed + 100000;
}
void preserved(const State& s) {
    require(s.pc == return_sentinel && s.r<4>(4) == 0x700fff00 && s.r<4>(8) == 0x108bba90,
            "Network failure changed the original return address, stack, or TOC");
    for (unsigned r = 14; r < 32; ++r)
        require(s.r<4>(r * 4) == 0xcafe0000u + r, "Network failure changed a nonvolatile register");
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-network-checks <original-segments>\n"; return 2; }
    auto state = std::make_unique<State>();
    auto& s = *state;
    try {
        TempDirectory temporary;
        std::filesystem::create_directory(temporary.path / "Game");
        load_program(s, argv[1]);
        s.memory.map(object, 0xe0000);
        s.platform = std::make_shared<NativePlatform>(temporary.path / "Game", temporary.path / "User", AudioMode::offline);
        // Both the application macro's null output and the game's four-byte
        // output must fail explicitly, without writing a native-sized pointer.
        for (const auto output : {0u, result + 4}) {
            prepare(s, 0x1234);
            s.memory.copy_in(result, std::vector<std::uint8_t>(12, 0xa5));
            s.wr<4>(12, 1); s.wr<4>(16, output);
            require(s.platform->dispatch(s, initialization), "Network initialization was not handled");
            require(std::int32_t(s.r<4>(12)) == kOTNotSupportedErr, "Unavailable networking reported success or an unrelated error");
            require(s.memory.read<4>(result) == 0xa5a5a5a5 && s.memory.read<4>(result + 8) == 0xa5a5a5a5 &&
                    s.memory.read<4>(result + 4) == (output ? 0 : 0xa5a5a5a5), "Network context write exceeded its original pointer boundary");
            preserved(s);
        }
        // Run the actual initializer twice: no active flag may survive failure.
        for (unsigned attempt = 0; attempt < 2; ++attempt) {
            prepare(s, 0x107450b0);
            s.memory.write<4>(object + 16, 0xa5a5a5a5);
            s.wr<4>(12, object);
            run(s);
            require(!s.r<4>(12) && !s.memory.read<4>(object + 16) && !s.memory.read<1>(object + 20),
                    "Original initializer did not propagate failure with an inactive client");
            preserved(s);
        }
        // The original menu constructor communicates failure through a byte.
        // Its original destructor must then clean up without entering network I/O.
        s.memory.copy_in(result, std::vector<std::uint8_t>(12, 0xa5));
        prepare(s, 0x10739f70);
        s.wr<4>(12, object); s.wr<4>(16, result + 4); s.wr<4>(20, 0); s.wr<4>(24, 0);
        run(s);
        require(s.r<4>(12) == object && s.memory.read<1>(result + 4) == 0 &&
                s.memory.read<4>(result) == 0xa5a5a5a5 && s.memory.read<4>(result + 5) == 0xa5a5a5a5,
                "Original network constructor did not preserve its byte-sized failure result");
        preserved(s);
        const auto allocation = s.memory.read<4>(object);
        require(allocation != 0, "Original network constructor did not allocate its local list");
        prepare(s, 0x1073a120);
        s.wr<4>(12, object); s.wr<4>(16, 0); // Fixture owns the outer object.
        run(s);
        preserved(s);
        require(!s.memory.read<1>(object + 20), "Cleanup activated an unavailable client");
        std::cout << "Unavailable-network ABI, original initialization failure, constructor, and cleanup checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << " at " << hex_address(s.pc) << "; LR=" << hex_address(s.r<4>(4128)) << '\n';
        return 1;
    }
}

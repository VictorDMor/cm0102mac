#include "native_service_adapter.hpp"
#include "semantic_io_checks.hpp"
#include "platform.hpp"
#include <unistd.h>

namespace {
using namespace cm::semantic_io_checks;
struct Temporary {
    std::filesystem::path path;
    Temporary() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-service-event-XXXXXX").string();
        require(mkdtemp(pattern.data()), "Private event fixture directory"); path = pattern;
        std::filesystem::create_directory(path / "Game");
    }
    ~Temporary() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
std::uint32_t api(State& s, const char* name, std::initializer_list<std::uint32_t> arguments) {
    unsigned r = 3; for (auto argument : arguments) s.wr<4>(4 * r++, argument);
    s.wr<4>(4128, return_sentinel);
    require(s.platform->dispatch(s, name) && s.pc == return_sentinel, "Actual native event API");
    return std::uint32_t(s.r<4>(12));
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2 || (argc == 3 && std::string_view(argv[2]) == "--original"), "Explicit event fixture input/mode");
        const bool original = argc == 3;
        auto state = std::make_unique<State>(); auto& s = *state; load_program(s, argv[1]);
        Temporary temporary; s.platform = std::make_shared<NativePlatform>(temporary.path / "Game", temporary.path / "User", AudioMode::offline);
        constexpr std::uint32_t launch_vector = 0x108bdaf4, launch_flag = 0x108b8564;
        require(api(s, "CarbonLib::AEInstallEventHandler", {0x61657674, 0x6f617070, launch_vector, 0, 0}) == 0, "Actual launch handler registration");
        const auto record = allocate(s, 16, true); s.memory.write<1>(launch_flag, 0);
        require(api(s, "CarbonLib::WaitNextEvent", {0x400, record, 0, 0}) == 1 && s.memory.read<2>(record) == 23 &&
            s.memory.read<4>(record + 2) == 0x61657674 && s.memory.read<4>(record + 10) == 0x6f617070 && !s.memory.read<1>(launch_flag),
            "Actual AppKit launch event is queued before the facade callback");
        s.native_service_original = original;
        const auto stack = s.r<4>(4), toc = s.r<4>(8);
        for (unsigned n = 0; n < 2; ++n) {
            s.wr<4>(12, record); s.wr<4>(4128, return_sentinel); s.pc = native_service::entries[0];
            const auto steps = s.executed;
            run_until(s, return_sentinel);
            require(s.r<4>(12) == (n ? std::uint32_t(-1708) : 0u) && s.memory.read<1>(launch_flag) == 1 &&
                s.executed == steps + (n ? 15 : 19) && s.r<4>(4) == stack && s.r<4>(8) == toc &&
                s.r<4>(4128) == return_sentinel, "Facade executes four-instruction launch callback once and restores its caller");
        }
        require(!s.native_service_original_body && !s.native_service_dispatch_escapes &&
            s.native_service_calls[0] == (original ? 0u : 2u) && s.native_service_returns[0] == (original ? 0u : 2u) &&
            s.native_service_original_calls[0] == (original ? 2u : 0u) && !s.native_service_faults[0] && !s.native_service_transfers[0],
            "Dedicated actual event-facade diagnostic evidence");
        std::cout << "Actual AppKit launch event: original callback once, consumed-event error, caller restored; "
            << (original ? "original" : "semantic") << " facade invoked twice\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

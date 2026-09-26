#include "callbacks.hpp"
#include "aot_runtime.hpp"
#include "platform.hpp"
#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include <unistd.h>

using namespace cm::aot;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
struct TempDirectory {
    std::filesystem::path path;
    TempDirectory() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-native-events-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create event test directory");
        path = pattern;
    }
    ~TempDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
};
std::uint32_t call(State& s, const char* name, std::initializer_list<std::uint32_t> arguments) {
    unsigned r = 3;
    for (auto value : arguments) s.wr<4>(r++ * 4, value);
    s.wr<4>(4128, return_sentinel);
    require(s.platform->dispatch(s, name), "Event API was not handled");
    require(s.pc == return_sentinel, "Event API did not return to caller");
    return s.r<4>(12);
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-callback-checks <relocated-segment-directory>\n"; return 2; }
    try {
        auto state = std::make_unique<State>();
        auto& s = *state;
        load_program(s, argv[1]);
        constexpr std::uint32_t launch_vector = 0x108bdaf4, launch_flag = 0x108b8564;
        s.memory.write<1>(launch_flag, 0);
        s.pc = 0x12345678;
        const auto registers = s.registers;
        s.unique.fill(0xa5);
        const auto steps = s.executed;
        require(call_original(s, launch_vector, {}) == 0, "Original launch callback result");
        require(s.memory.read<1>(launch_flag) == 1 && s.executed == steps + 4,
                "Original launch callback did not execute its four instructions");
        require(s.registers == registers && s.pc == 0x12345678,
                "Callback corrupted its native caller's saved ABI state");
        for (auto byte : s.unique) require(byte == 0xa5, "Callback corrupted caller temporaries");
        TempDirectory temporary;
        std::filesystem::create_directory(temporary.path / "Game");
        s.platform = std::make_shared<NativePlatform>(temporary.path / "Game", temporary.path / "User");
        require(call(s, "CarbonLib::AEInstallEventHandler", {0x61657674, 0x6f617070, launch_vector, 0, 0}) == 0,
                "Original application launch handler installation");
        s.memory.write<1>(launch_flag, 0);
        const auto record = allocate(s, 16, true);
        require(call(s, "CarbonLib::WaitNextEvent", {0x400, record, 0, 0}) == 1,
                "Real AppKit application launch notification did not reach the legacy event queue");
        require(s.memory.read<2>(record) == 23 && s.memory.read<4>(record + 2) == 0x61657674 &&
                s.memory.read<4>(record + 10) == 0x6f617070, "High-level EventRecord fields");
        require(s.memory.read<1>(launch_flag) == 0, "Launch callback ran before AEProcessAppleEvent");
        require(call(s, "CarbonLib::AEProcessAppleEvent", {record}) == 0 && s.memory.read<1>(launch_flag) == 1,
                "Native AppleEvent did not invoke the original ARM64-recompiled launch callback");
        require(call(s, "CarbonLib::AEProcessAppleEvent", {record}) == std::uint32_t(-1708),
                "Already-consumed AppleEvent invoked the callback again");
        const auto device = call(s, "CarbonLib::GetMainDevice", {});
        require(device != 0 && call(s, "CarbonLib::GetMainDevice", {}) == device,
                "Main graphics device handle was absent or unstable");
        const auto data = std::uint32_t(s.memory.read<4>(device));
        const auto pixels = std::uint32_t(s.memory.read<4>(s.memory.read<4>(data + 22)));
        const auto main_display = CGMainDisplayID();
        const auto bounds = CGDisplayBounds(main_display);
        const std::array<std::int16_t, 4> native_bounds = {
            std::int16_t(CGRectGetMinY(bounds)), std::int16_t(CGRectGetMinX(bounds)),
            std::int16_t(CGRectGetMaxY(bounds)), std::int16_t(CGRectGetMaxX(bounds))};
        for (unsigned i = 0; i < 4; ++i)
            require(s.memory.read<2>(data + 34 + i * 2) == std::uint16_t(native_bounds[i]) &&
                    s.memory.read<2>(pixels + 6 + i * 2) == std::uint16_t(native_bounds[i]),
                    "Original device/PixMap bounds differ from the connected native main screen");
        require(s.memory.read<2>(data + 4) == 2 &&
                s.memory.read<2>(data + 20) == (0x2801u | (CGDisplayIsActive(main_display) ? 0x8000u : 0)),
                "Original direct-color device type or native display status flags");
        require(s.memory.read<4>(pixels) == 0 && s.memory.read<2>(pixels + 30) == 16 &&
                s.memory.read<2>(pixels + 32) == 32 && s.memory.read<2>(pixels + 34) == 3 &&
                s.memory.read<2>(pixels + 36) == 8,
                "Presentation pixel metadata exposed a host pointer or wrong component format");
        const auto rectangle = allocate(s, 16, true);
        call(s, "CarbonLib::SetRect", {rectangle, 80, 60, 280, 160});
        require(call(s, "CarbonLib::CreateNewWindow", {13, 0x02200000, rectangle, rectangle + 8}) == 0,
                "Original plain window could not be created through AppKit");
        const auto window = std::uint32_t(s.memory.read<4>(rectangle + 8));
        call(s, "CarbonLib::GetWindowBounds", {window, 32, rectangle});
        require(s.memory.read<2>(rectangle) == 60 && s.memory.read<2>(rectangle + 2) == 80 &&
                s.memory.read<2>(rectangle + 4) == 160 && s.memory.read<2>(rectangle + 6) == 280,
                "Actual native window bounds differ from the original global rectangle");
        const auto window_port = call(s, "CarbonLib::GetWindowPort", {window});
        const auto window_pixels = call(s, "CarbonLib::GetPortPixMap", {window_port});
        const auto pixel_record = std::uint32_t(s.memory.read<4>(window_pixels));
        const auto storage = std::uint32_t(s.memory.read<4>(pixel_record));
        s.memory.copy_in(storage, std::vector<std::uint8_t>(200 * 100 * 4, 0xff));
        s.memory.write<4>(0x10bf71f8, window); // Original background-window global.
        s.memory.write<4>(rectangle, 0x77696e64); s.memory.write<4>(rectangle + 4, 1013);
        const auto target = call(s, "CarbonLib::GetWindowEventTarget", {window});
        require(call(s, "CarbonLib::InstallEventHandler", {target, 0x108bdabc, 1, rectangle, 0, 0}) == 0,
                "Original background paint callback registration");
        const auto before_paint = s.executed;
        call(s, "CarbonLib::ShowWindow", {window});
        require(s.executed > before_paint && s.memory.copy_out(storage, 200 * 100 * 4) == std::vector<std::uint8_t>(200 * 100 * 4),
                "Native window invalidation did not run the original paint callback over the whole window");
        call(s, "CarbonLib::GetPort", {rectangle});
        require(s.memory.read<4>(rectangle) == 0, "Native paint callback did not restore the previous current port");
        call(s, "CarbonLib::DisposeWindow", {window});
        require(!s.allocated.contains(storage), "Native window close retained original pixel storage");
        std::cout << "Original callback ABI preservation and real AppKit-to-original launch delivery passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

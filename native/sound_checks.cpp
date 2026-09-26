#include "aot_runtime.hpp"
#include "platform.hpp"
#include "callbacks.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <unistd.h>

using namespace cm::aot;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
struct TempDirectory {
    std::filesystem::path path;
    TempDirectory() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-native-sound-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create sound test directory");
        path = pattern;
    }
    ~TempDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
};
std::uint32_t call(State& s, const char* name, std::initializer_list<std::uint32_t> arguments) {
    unsigned r = 3;
    for (auto value : arguments) s.wr<4>(r++ * 4, value);
    s.wr<4>(4128, return_sentinel);
    require(s.platform->dispatch(s, name), "Sound API was not handled");
    require(s.pc == return_sentinel, "Sound API did not return to caller");
    return s.r<4>(12);
}
void pump(State& s) {
    // Offline rendering publishes completion before returning. The next native
    // call must deliver it to the original callback without a timing workaround.
    (void)call(s, "CarbonLib::TickCount", {});
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-sound-checks <relocated-segment-directory>\n"; return 2; }
    try {
        auto state = std::make_unique<State>();
        auto& s = *state;
        load_program(s, argv[1]);
        TempDirectory temporary;
        std::filesystem::create_directory(temporary.path / "Game");
        s.platform = std::make_shared<NativePlatform>(temporary.path / "Game", temporary.path / "User", AudioMode::offline);
        const auto cell = allocate(s, 16, true), header = allocate(s, 66, true), pcm = allocate(s, 2048, true);
        const auto user = allocate(s, 16, true), command = allocate(s, 8, true);
        s.memory.copy_in(cell, {0x40, 0x0d, 0xac, 0x44, 0, 0, 0, 0, 0, 0});
        s.wr<8>(16384, float_bits<8>(-1.25)); s.wr<8>(16416, float_bits<8>(3.5));
        call(s, "CarbonLib::x80tod", {cell});
        require(float_value<8>(s.r<8>(16400)) == 22050 && float_value<8>(s.r<8>(16384)) == -1.25 &&
                float_value<8>(s.r<8>(16416)) == 3.5, "Extended80 byte order or original f1 return ABI");
        require(call(s, "CarbonLib::X2Fix", {}) == 0x56220000, "Original f1 fixed-point conversion ABI");
        s.wr<8>(16400, float_bits<8>(-1.5));
        require(call(s, "CarbonLib::X2Fix", {}) == 0xfffe8000, "Signed fixed-point conversion");
        s.wr<8>(16400, float_bits<8>(44100));
        require(call(s, "CarbonLib::X2Fix", {}) == 0x7fffffff, "Native fixed-point saturation");
        s.wr<4>(8, 0x108bba90);
        const auto context = call(s, "CarbonLib::SetCurrentA5", {});
        require(context == 0x108bba90 && call(s, "CarbonLib::SetA5", {0x12345678}) == context &&
                call(s, "CarbonLib::SetCurrentA5", {}) == 0x12345678 &&
                call(s, "CarbonLib::SetA5", {context}) == context, "Original logical A5 context save/restore");
        constexpr auto callback = 0x108bd624u;
        require(call(s, "CarbonLib::NewSndCallBackUPP", {callback}) == callback, "Original sound callback transition vector");
        s.memory.write<4>(cell, 0);
        require(call(s, "CarbonLib::SndNewChannel", {cell, 5, 0x85, callback}) == 0, "Original sampled channel creation");
        auto channel = std::uint32_t(s.memory.read<4>(cell));
        require(s.allocated.at(channel) == 1060 && s.memory.read<4>(channel + 8) == callback &&
                s.memory.read<4>(channel + 12) == 0 && s.memory.read<2>(channel + 30) == 128,
                "Original SndChannel allocation and packed fields");
        s.memory.write<4>(channel + 12, user);
        s.memory.write<4>(user, header); s.memory.write<4>(user + 4, pcm);
        s.memory.write<4>(header, pcm); s.memory.write<4>(header + 4, 1);
        s.memory.write<4>(header + 8, 0x56220000); s.memory.write<1>(header + 20, 0xff);
        s.memory.write<4>(header + 22, 1024); s.memory.write<2>(header + 48, 16);
        for (unsigned i = 0; i < 1024; ++i) s.memory.write<2>(pcm + i * 2, 0x4000);
        auto issue = [&](unsigned code, std::uint32_t data, bool queued = false) {
            s.memory.write<2>(command, code); s.memory.write<2>(command + 2, 0); s.memory.write<4>(command + 4, data);
            return call(s, queued ? "CarbonLib::SndDoCommand" : "CarbonLib::SndDoImmediate", {channel, command, 1});
        };
        require(issue(46, 0x01000100) == 0 && issue(81, header) == 0 && issue(13, context, true) == 0, "Original buffer/callback commands");
        s.memory.clear(command, 8); // The queue must own a copy of the command.
        pump(s);
        require(s.memory.read<1>(user + 9) == 0, "Original callback ran before native audio rendered");
        auto rendered = s.platform->render_audio_offline(4096);
        for (unsigned i = 0; i < 2230; ++i) require(std::abs(rendered[i * 2] - 0.5f) < 0.0001f &&
            std::abs(rendered[i * 2 + 1] - 0.5f) < 0.0001f, "Extended header PCM playback at original sample rate");
        const auto before_callback = s.executed;
        pump(s);
        require(s.memory.read<1>(user + 9) == 1 && s.executed > before_callback,
                "Native completion did not run the actual recompiled original sound callback");
        const auto after_callback = s.executed;
        pump(s);
        require(s.executed == after_callback, "Original sound callback delivered more than once");

        s.memory.write<1>(user + 8, 1); s.memory.write<1>(user + 9, 0);
        require(issue(81, header) == 0 && issue(13, context, true) == 0, "Original looping sound start");
        (void)s.platform->render_audio_offline(4096);
        pump(s);
        require(s.executed > after_callback && s.memory.read<1>(user + 9) == 0, "Original loop callback did not resubmit audio");
        s.memory.write<1>(user + 8, 0);
        rendered = s.platform->render_audio_offline(4096);
        require(std::abs(rendered[0] - 0.5f) < 0.0001f, "Original looping callback failed to restart native audio");
        pump(s);
        require(s.memory.read<1>(user + 9) == 1, "Original loop shutdown did not mark sound done");

        s.memory.write<1>(user + 9, 0);
        require(issue(81, header) == 0 && issue(13, context, true) == 0 && issue(4, 0) == 0, "Original flush command");
        rendered = s.platform->render_audio_offline(4096);
        require(std::abs(rendered[0] - 0.5f) < 0.0001f, "flushCmd incorrectly stopped active sound");
        pump(s);
        require(s.memory.read<1>(user + 9) == 0, "Flushed callback was delivered");
        require(issue(81, header) == 0 && issue(13, context, true) == 0 && issue(3, 0) == 0, "Original quiet command");
        pump(s);
        require(s.memory.read<1>(user + 9) == 1, "quietCmd incorrectly flushed queued callbacks");

        s.memory.write<1>(user + 9, 0);
        require(issue(81, header) == 0 && issue(13, context, true) == 0, "Original cancellation setup");
        require(call(s, "CarbonLib::SndDisposeChannel", {channel, 1}) == 0 && !s.allocated.contains(channel), "Owned original channel disposal");
        pump(s);
        require(s.memory.read<1>(user + 9) == 0 && s.allocated.contains(user) && s.allocated.contains(pcm),
                "Channel disposal invoked a canceled callback or freed game-owned sound data");
        require(call(s, "CarbonLib::SndDisposeChannel", {channel, 1}) == std::uint32_t(-205), "Invalid channel result code");

        channel = allocate(s, 1060, true); s.memory.write<2>(channel + 30, 2); s.memory.write<4>(channel + 12, user);
        s.memory.write<4>(cell, channel);
        require(call(s, "CarbonLib::SndNewChannel", {cell, 5, 0x85, callback}) == 0 && s.memory.read<4>(channel + 12) == user, "Caller-owned sound channel");
        require(issue(81, header) == 0 && issue(13, context, true) == 0 && issue(0, 0, true) == 0 &&
                issue(13, context, true) == std::uint32_t(-203), "Original queue capacity and noWait error");
        require(call(s, "CarbonLib::SndDisposeChannel", {channel, 0}) == 0 && s.allocated.contains(channel) &&
                s.memory.read<1>(user + 9) == 1, "Deferred disposal failed to drain queued audio or freed caller-owned memory");

        // Execute the actual match-exit cleanup, including quiet -> DisposePtr
        // for the PCM/header -> FSClose -> user-data disposal -> channel disposal.
        // A callback delivered from inside FSClose would see freed game data.
        std::ofstream(temporary.path / "Game" / "teardown.pcm", std::ios::binary).put('\0');
        const auto spec = allocate(s, 70, true), ref_cell = allocate(s, 4, true);
        const auto owner = allocate(s, 48, true), vector = allocate(s, 8, true);
        s.memory.write<2>(spec, 0xffff); s.memory.write<4>(spec + 2, 2);
        const std::string leaf = "teardown.pcm";
        s.memory.write<1>(spec + 6, leaf.size());
        s.memory.copy_in(spec + 7, {leaf.begin(), leaf.end()});
        s.memory.write<1>(owner + 40, 1);
        s.memory.write<4>(vector, 0x10744700); s.memory.write<4>(vector + 4, 0x108bba90);
        std::uint64_t cleanup_instructions = 0;
        for (bool queued : {false, true}) {
            for (bool completed : {false, true}) {
                std::vector<std::uint32_t> released;
                std::vector<std::uint16_t> closed;
                for (unsigned index : {0u, 4u, 9u}) {
                    const auto info = allocate(s, 16, true), sound_header = allocate(s, 66, true);
                    const auto samples = allocate(s, 2048, true);
                    require(call(s, "CarbonLib::FSpOpenDF", {spec, 1, ref_cell}) == 0, "Match sound file open");
                    const auto ref = std::uint16_t(s.memory.read<2>(ref_cell));
                    closed.push_back(ref);
                    s.memory.write<4>(cell, 0);
                    require(call(s, "CarbonLib::SndNewChannel", {cell, 5, 0x85, callback}) == 0, "Match sound channel creation");
                    channel = std::uint32_t(s.memory.read<4>(cell));
                    s.memory.write<4>(owner + 4 * index, channel);
                    s.memory.write<4>(channel + 12, info);
                    s.memory.write<4>(info, sound_header); s.memory.write<4>(info + 4, samples);
                    s.memory.write<1>(info + 8, index == 4 ? 0 : 1); // One-shot and looping channels.
                    s.memory.write<2>(info + 12, ref);
                    s.memory.write<4>(sound_header, samples); s.memory.write<4>(sound_header + 4, 1);
                    s.memory.write<4>(sound_header + 8, 0x56220000); s.memory.write<1>(sound_header + 20, 0xff);
                    s.memory.write<4>(sound_header + 22, 1024); s.memory.write<2>(sound_header + 48, 16);
                    require(issue(81, sound_header) == 0 && (!queued || issue(13, context, true) == 0), "Match sound submission");
                    released.insert(released.end(), {channel, info, sound_header, samples});
                }
                if (completed) (void)s.platform->render_audio_offline(4096);
                const auto before = s.executed;
                const auto allocations = s.allocated.size();
                const auto registers = s.registers;
                (void)call_original(s, vector, std::array{owner});
                const auto instructions = s.executed - before;
                if (!cleanup_instructions) cleanup_instructions = instructions;
                require(instructions == cleanup_instructions, "Audio callback re-entered original match cleanup");
                require(s.registers == registers, "Match sound cleanup changed the suspended caller");
                require(s.allocated.size() + released.size() == allocations, "Match sound cleanup leaked or released unrelated allocations");
                for (auto address : released) require(!s.allocated.contains(address), "Original match sound payload not released");
                for (unsigned i = 0; i < 10; ++i) require(s.memory.read<4>(owner + i * 4) == 0, "Original match sound slot not cleared");
                for (auto ref : closed) require(call(s, "CarbonLib::GetEOF", {ref, ref_cell}) == std::uint32_t(-51), "Original match sound file was not closed");
                pump(s);
                require(s.executed == before + instructions, "Disposed match callback ran after the next timing poll");
            }
        }
        std::cout << "Original sound ABI, native offline playback, original one-shot/loop callbacks, flush/quiet, queue capacity, and disposal passed\n";
        std::cout << "Original match sound cleanup passed with playing/completed buffers and queued callbacks; " << cleanup_instructions << " instructions per cleanup\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

#include "platform.hpp"
#include "aot_runtime.hpp"
#include "filesystem_bridge.hpp"
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <sys/xattr.h>
#include <CoreServices/CoreServices.h>
#include <ctime>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

using namespace cm::aot;
namespace fs = std::filesystem;
namespace {
void require(bool result, const char* message) { if (!result) throw std::runtime_error(message); }
struct TempDirectory {
    fs::path path;
    TempDirectory() {
        auto pattern = (fs::temp_directory_path() / "cm-native-files-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create filesystem test directory");
        path = pattern;
    }
    ~TempDirectory() { std::error_code error; fs::remove_all(path, error); }
};
void string(State& s, std::uint32_t at, const std::string& text) {
    s.memory.write<1>(at, text.size());
    s.memory.copy_in(at + 1, {text.begin(), text.end()});
}
std::string string(State& s, std::uint32_t at) {
    const auto bytes = s.memory.copy_out(at + 1, s.memory.read<1>(at));
    return {bytes.begin(), bytes.end()};
}
std::int32_t call(State& s, const char* name, std::initializer_list<std::uint32_t> args) {
    unsigned n = 3;
    for (auto arg : args) s.wr<4>((n++) * 4, arg);
    s.wr<4>(4128, return_sentinel);
    s.pc = 0x1234;
    require(s.platform->dispatch(s, name), "Filesystem API was not handled");
    require(s.pc == return_sentinel, "Filesystem API did not return to caller");
    return std::int32_t(s.r<4>(12));
}
}
int main() {
    try {
        for (std::uint64_t capacity : {UINT64_C(0), UINT64_C(4096), UINT64_C(256) << 20,
                                      UINT64_C(4) << 30, UINT64_C(460) << 30, UINT64_C(8) << 40}) {
            for (const auto available : {UINT64_C(0), UINT64_C(1), capacity / 4, capacity}) {
                const auto geometry = legacy_volume_geometry({capacity, available, available});
                const auto total = std::uint64_t(geometry.blocks) * geometry.block_size;
                const auto free = std::uint64_t(geometry.free_blocks) * geometry.block_size;
                require(total <= capacity && total <= UINT32_MAX && free <= available && free <= total,
                        "Classic volume geometry overflowed or overstated host space");
                if (available >= (UINT64_C(4) << 30))
                    require((std::uint32_t(geometry.free_blocks) * geometry.block_size >> 20) >= 4095,
                            "Original 32-bit free-space calculation falsely reported a full modern disk");
            }
        }
        TempDirectory temporary;
        auto game = temporary.path / "Game";
        auto user = temporary.path / "User";
        fs::create_directories(game / "Data");
        std::ofstream(game / "Data" / "players.dat", std::ios::binary) << "original-data";
        std::ofstream(game / "Data" / "Caf\xc3\xa9.dat", std::ios::binary) << "accented";
        fs::create_directory_symlink(game / "Data", game / "Linked Data");
        auto state = std::make_unique<State>();
        auto& s = *state;
        s.memory.map(0x70000000, 0x10000);
        s.platform = std::make_shared<NativePlatform>(game, user);
        constexpr std::uint32_t name = 0x70000000, spec = 0x70000200, pb = 0x70000400;
        const std::vector<std::uint8_t> alphabet{'a', 'z', 0x8e, 0x9f, 0xa7, 0xde, 0xdf, 0}; // e acute, u umlaut, sharp-s, fi/fl.
        s.memory.write<4>(name, 0xa5a5a5a5); s.memory.write<4>(name + 12, 0xa5a5a5a5);
        s.memory.copy_in(name + 4, alphabet);
        call(s, "CarbonLib::UppercaseText", {name + 4, std::uint32_t(alphabet.size()), 0});
        require(s.memory.copy_out(name + 4, 8) == std::vector<std::uint8_t>({'A', 'Z', 0x83, 0x86, 0xa7, 0xde, 0xdf, 0}) &&
                s.memory.read<4>(name) == 0xa5a5a5a5 && s.memory.read<4>(name + 12) == 0xa5a5a5a5,
                "Native UppercaseText changed the original byte span or MacRoman accented mapping");
        s.memory.copy_in(name + 4, {'a', 'z'});
        call(s, "CarbonLib::UppercaseText", {name + 4, 1, std::uint32_t(-1)});
        require(s.memory.read<2>(name + 4) == 0x417a, "System-script uppercase length and -1 script ABI");
        // Compare the translated clock ABI against independent POSIX local time
        // and the host's real legacy APIs. Guard bytes catch native-width writes.
        const auto before = std::time(nullptr);
        call(s, "CarbonLib::GetDateTime", {name + 4});
        const auto after = std::time(nullptr);
        std::tm local_before{}, local_after{};
        require(localtime_r(&before, &local_before) && localtime_r(&after, &local_after), "Native local time query");
        const auto date = s.memory.read<4>(name + 4);
        const auto first = std::uint32_t(before + local_before.tm_gmtoff + 2082844800LL);
        const auto last = std::uint32_t(after + local_after.tm_gmtoff + 2082844800LL);
        require(std::uint32_t(date - first) <= std::uint32_t(last - first), "GetDateTime epoch, local time zone, or 32-bit encoding");
        const auto tick_before = ::TickCount();
        const auto tick = std::uint32_t(call(s, "CarbonLib::TickCount", {}));
        const auto tick_after = ::TickCount();
        require(std::uint32_t(tick - tick_before) <= std::uint32_t(tick_after - tick_before), "TickCount does not share the native system tick epoch");
        s.memory.copy_in(name, std::vector<std::uint8_t>(16, 0xa5));
        UnsignedWide micros_before{}, micros_after{};
        ::Microseconds(&micros_before);
        call(s, "CarbonLib::Microseconds", {name + 4});
        ::Microseconds(&micros_after);
        const auto micros = s.memory.read<8>(name + 4);
        require(micros >= ((std::uint64_t(micros_before.hi) << 32) | micros_before.lo) &&
                micros <= ((std::uint64_t(micros_after.hi) << 32) | micros_after.lo) &&
                s.memory.read<4>(name) == 0xa5a5a5a5 && s.memory.read<4>(name + 12) == 0xa5a5a5a5,
                "Microseconds changed the native timer epoch, PowerPC hi/lo order, or output boundary");
        s.memory.copy_in(name, std::vector<std::uint8_t>(20, 0xa5));
        MachineLocation location{};
        ::ReadLocation(&location);
        call(s, "CarbonLib::ReadLocation", {name + 4});
        require(s.memory.read<4>(name + 4) == std::uint32_t(location.latitude) &&
                s.memory.read<4>(name + 8) == std::uint32_t(location.longitude) &&
                s.memory.read<4>(name + 12) == std::uint32_t(location.u.gmtDelta) &&
                s.memory.read<4>(name) == 0xa5a5a5a5 && s.memory.read<4>(name + 16) == 0xa5a5a5a5,
                "ReadLocation changed the native GMT/DST value or the original 12-byte record boundary");
        call(s, "CarbonLib::SetRect", {name, std::uint32_t(-2), 3, 1, 5});
        require(std::uint32_t(call(s, "CarbonLib::PinRect", {name, 0x00050001})) == 0x00050001 &&
                std::uint32_t(call(s, "CarbonLib::PinRect", {name, 0x00060002})) == 0x00040000 &&
                std::uint32_t(call(s, "CarbonLib::PinRect", {name, 0xfffffffd})) == 0x0003fffe,
                "PinRect lost signed coordinates or the original right/bottom equality quirk");
        require(call(s, "CarbonLib::NewGWorld", {pb, 16, name, 0, 0, 0}) == 0, "Native RGB555 offscreen allocation");
        const auto world = std::uint32_t(s.memory.read<4>(pb));
        const auto bitmap = std::uint32_t(call(s, "CarbonLib::GetPortPixMap", {world}));
        const auto descriptor = std::uint32_t(s.memory.read<4>(bitmap));
        require(s.memory.copy_out(descriptor + 6, 8) == s.memory.copy_out(name, 8) &&
                s.memory.read<2>(descriptor + 4) == 0x8008 && s.memory.read<2>(descriptor + 32) == 16 &&
                s.memory.read<2>(descriptor + 36) == 5, "Original nonzero-origin RGB555 PixMap layout and padded row stride");
        require(call(s, "CarbonLib::LockPixels", {bitmap}) == 1, "Native offscreen pixels could not be locked");
        const auto pixels = std::uint32_t(call(s, "CarbonLib::GetPixBaseAddr", {bitmap}));
        require(s.memory.copy_out(pixels, 16) == std::vector<std::uint8_t>(16), "Offscreen pixels exposed stale allocation data");
        s.memory.write<2>(pixels, 0x7c00); s.memory.write<2>(pixels + 8, 0x03e0);
        require(s.memory.copy_out(pixels, 2) == std::vector<std::uint8_t>({0x7c, 0}) &&
                std::uint32_t(call(s, "CarbonLib::GetPixBaseAddr", {bitmap})) == pixels,
                "Original direct RGB555 pixel writes or locked address were not preserved");
        require(call(s, "CarbonLib::NewGWorld", {pb + 8, 32, name, 0, 0, 0}) == 0, "Native ARGB copy destination");
        const auto destination_world = std::uint32_t(s.memory.read<4>(pb + 8));
        const auto destination_map = std::uint32_t(call(s, "CarbonLib::GetPortPixMap", {destination_world}));
        const auto destination_record = std::uint32_t(s.memory.read<4>(destination_map));
        const auto destination_pixels = std::uint32_t(s.memory.read<4>(destination_record));
        call(s, "CarbonLib::CopyBits", {descriptor, destination_record, name, name, 0, 0});
        require(s.memory.read<4>(destination_pixels) == 0xff0000 &&
                s.memory.read<4>(destination_pixels + 12) == 0x00ff00,
                "RGB555 red and green rows were not converted to big-endian ARGB");
        s.memory.write<4>(destination_pixels + 4, 0x00ff00);
        s.memory.write<4>(destination_pixels + 8, 0x0000ff);
        call(s, "CarbonLib::SetRect", {name + 16, std::uint32_t(-2), 3, 0, 4});
        call(s, "CarbonLib::SetRect", {name + 24, std::uint32_t(-1), 3, 1, 4});
        call(s, "CarbonLib::CopyBits", {destination_record, destination_record, name + 16, name + 24, 0, 0});
        require(s.memory.read<4>(destination_pixels) == 0xff0000 &&
                s.memory.read<4>(destination_pixels + 4) == 0xff0000 && s.memory.read<4>(destination_pixels + 8) == 0x00ff00,
                "Overlapping CopyBits read pixels already overwritten by the same copy");
        call(s, "CarbonLib::DisposeGWorld", {destination_world});
        call(s, "CarbonLib::SetPort", {world}); call(s, "CarbonLib::GetPort", {pb});
        require(s.memory.read<4>(pb) == world, "Offscreen current-port selection");
        s.memory.write<2>(pixels + 6, 0x1234);
        call(s, "CarbonLib::ForeColor", {30}); call(s, "CarbonLib::PaintRect", {name});
        require(s.memory.read<2>(pixels) == 0x7fff && s.memory.read<2>(pixels + 4) == 0x7fff &&
                s.memory.read<2>(pixels + 6) == 0x1234, "RGB555 rectangle fill changed row padding or used the wrong white value");
        call(s, "CarbonLib::DisposeGWorld", {world});
        require(!s.allocated.contains(pixels), "Disposed GWorld retained its pixel allocation");
        call(s, "CarbonLib::GetPort", {pb});
        require(s.memory.read<4>(pb) == 0, "Disposed GWorld remained selected");
        s.memory.copy_in(name, {9, 8, 7, 6, 5});
        require(call(s, "CarbonLib::PtrToHand", {name, pb, 5}) == 0, "Original relocatable handle copy");
        const auto handle = std::uint32_t(s.memory.read<4>(pb));
        const auto payload = std::uint32_t(s.memory.read<4>(handle));
        s.memory.write<1>(name, 0);
        require(s.memory.copy_out(payload, 5) == std::vector<std::uint8_t>({9, 8, 7, 6, 5}), "Handle aliased source memory");
        require(call(s, "CarbonLib::GetHandleSize", {handle}) == 5, "Logical handle size");
        s.memory.copy_in(name, {1, 2, 3});
        require(call(s, "CarbonLib::Munger", {handle, 0, 0, 0, name, 3}) >= 0, "Native prefix insertion");
        require(s.memory.copy_out(payload, 8) == std::vector<std::uint8_t>({1, 2, 3, 9, 8, 7, 6, 5}),
                "Munger did not preserve the original payload suffix");
        require(call(s, "CarbonLib::GetHandleSize", {handle}) == 8, "Munger logical-size update");
        call(s, "CarbonLib::HLock", {handle});
        require(s.memory.read<4>(handle) == payload, "Lock changed payload address");
        call(s, "CarbonLib::DisposeHandle", {handle});
        require(call(s, "CarbonLib::GetHandleSize", {handle}) == 0 && s.memory_error == -109,
                "Released handle remained live");
        s.memory.copy_in(name, {4, 5, 6});
        require(call(s, "CarbonLib::PtrToHand", {name, pb, 3}) == 0, "Handle resize fixture");
        const auto grow = std::uint32_t(s.memory.read<4>(pb));
        const std::vector<std::uint8_t> larger(40, 0x5a);
        s.memory.copy_in(name, larger);
        require(call(s, "CarbonLib::Munger", {grow, 0, 0, 3, name, 40}) >= 0, "Native handle growth");
        require(call(s, "CarbonLib::GetHandleSize", {grow}) == 40, "Grown handle size");
        require(s.memory.copy_out(s.memory.read<4>(grow), 40) == larger, "Master pointer did not follow relocated data");
        call(s, "CarbonLib::DisposeHandle", {grow});
        string(s, name, "data");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0, 0, name, spec}) == 0, "Case-insensitive Data lookup");
        require(s.memory.read<2>(spec) == 0xffff && s.memory.read<4>(spec + 2) == 2,
                "Original 70-byte FSSpec fields");
        require(string(s, spec + 6) == "Data", "Actual native filename was not preserved");
        require(call(s, "CarbonLib::ResolveAliasFile", {spec, 1, name + 300, name + 301}) == 0,
                "Ordinary data directory resolution");
        require(s.memory.read<1>(name + 300) == 1 && s.memory.read<1>(name + 301) == 0,
                "Folder/alias flags");
        s.memory.clear(pb, 108);
        s.memory.write<4>(pb + 18, name);
        s.memory.write<4>(pb + 48, 2);
        require(call(s, "CarbonLib::PBGetCatInfoSync", {pb}) == 0, "Real directory catalog lookup");
        require(s.memory.read<1>(pb + 30) == 0x10 && s.memory.read<2>(pb + 52) == 2,
                "Directory attributes and real entry count");
        const auto data_id = std::uint32_t(s.memory.read<4>(pb + 48));
        string(s, name, "PLAYERS.DAT");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, data_id, name, spec}) == 0,
                "Lookup using assigned legacy directory ID");
        s.memory.clear(pb, 108);
        s.memory.write<4>(pb + 18, spec + 6);
        s.memory.write<2>(pb + 22, 0xffff);
        s.memory.write<4>(pb + 48, data_id);
        require(call(s, "CarbonLib::PBGetCatInfoSync", {pb}) == 0, "Real file catalog lookup");
        require(s.memory.read<4>(pb + 54) == 13 && s.memory.read<2>(pb + 16) == 0,
                "Original file length/ioResult fields");
        string(s, name, std::string("Caf") + char(0x8e) + ".dat");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, data_id, name, spec}) == 0,
                "MacRoman accented filename lookup");
        string(s, name, "missing.sav");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xfffffffe, 3, name, spec}) == -43,
                "Missing final file must return fnfErr");
        require(string(s, spec + 6) == "missing.sav" && s.memory.read<2>(spec) == 0xfffe,
                "Missing-file FSSpec must remain usable for later creation");
        string(s, name, "::outside");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0, 2, name, spec}) == -120,
                "Classic parent traversal escaped configured file roots");
        string(s, name, "Linked Data");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0, 0, name, spec}) == 0, "Link FSSpec creation");
        require(string(s, spec + 6) == "Linked Data", "FSSpec resolved alias prematurely");
        require(call(s, "CarbonLib::ResolveAliasFile", {spec, 1, name + 300, name + 301}) == 0,
                "Native symbolic link resolution");
        require(s.memory.read<1>(name + 301) == 1 && string(s, spec + 6) == "Data", "Resolved alias metadata");
        s.memory.clear(pb, 122);
        s.memory.write<4>(pb + 18, name);
        require(call(s, "CarbonLib::PBHGetVInfoSync", {pb}) == 0, "Native game volume query");
        require(s.memory.read<2>(pb + 22) == 0xffff && string(s, name) == "CM 01-02", "Volume identity");
        require(s.memory.read<4>(pb + 48) > 0 && s.memory.read<4>(pb + 82) == 2, "Volume allocation/file counts");
        auto volume_counts = [&] {
            s.memory.clear(pb, 122);
            s.memory.write<2>(pb + 22, 0xffff);
            require(call(s, "CarbonLib::PBHGetVInfoSync", {pb}) == 0, "Refresh volume counts");
            return std::pair{s.memory.read<4>(pb + 82), s.memory.read<4>(pb + 86)};
        };
        const auto initial_counts = volume_counts();
        const auto previous_time = fs::last_write_time(game / "Data");
        std::ofstream(game / "Data" / "external.dat") << "external";
        // Restoring mtime must not conceal a directory mutation: ctime also
        // participates in validation, and nested directories are checked.
        fs::last_write_time(game / "Data", previous_time);
        require(volume_counts().first == initial_counts.first + 1, "Cached volume hid a nested external addition");
        fs::rename(game / "Data" / "external.dat", game / "Data" / "renamed.dat");
        string(s, name, "CM 01-02:Data:renamed.dat");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0, 0, name, spec}) == 0, "Cached names hid an external rename");
        fs::remove(game / "Data" / "renamed.dat");
        fs::create_directory(game / "Data" / "renamed.dat");
        auto changed_counts = volume_counts();
        require(changed_counts.first == initial_counts.first && changed_counts.second == initial_counts.second + 1,
                "Cached volume retained a replaced entry's file type");
        fs::remove(game / "Data" / "renamed.dat");
        // Symlink targets can change without touching the link's directory.
        const auto target = user / "target";
        std::ofstream(target) << "target";
        fs::create_symlink(target, game / "changing-link");
        require(volume_counts().first == initial_counts.first + 1, "Symlink file count changed");
        fs::remove(target);
        fs::create_directory(target);
        changed_counts = volume_counts();
        require(changed_counts.first == initial_counts.first && changed_counts.second == initial_counts.second + 1,
                "Cached volume hid a symlink target type change");
        fs::remove(target);
        require(volume_counts() == initial_counts, "Dangling symlink changed volume counts");
        fs::remove(game / "changing-link");
        require(volume_counts() == initial_counts, "Cached volume hid external removal");
        s.memory.write<4>(pb + 18, name);
        string(s, name, "CM 01-02:Data:players.dat");
        s.memory.write<2>(pb + 22, 0);
        s.memory.write<2>(pb + 28, 0xffff);
        require(call(s, "CarbonLib::PBHGetVInfoSync", {pb}) == 0 && s.memory.read<2>(pb + 22) == 0xffff,
                "Volume lookup from an absolute Classic file path");
        for (const auto& relative : {"CM3 Config", ":Data:players.dat", ""}) {
            string(s, name, relative);
            s.memory.write<2>(pb + 22, 0);
            require(call(s, "CarbonLib::PBHGetVInfoSync", {pb}) == 0 && s.memory.read<2>(pb + 22) == 0xffff,
                    "Relative volume lookup did not use the current volume");
            string(s, name, relative);
            s.memory.write<2>(pb + 22, 0xfffe);
            require(call(s, "CarbonLib::PBHGetVInfoSync", {pb}) == 0 && s.memory.read<2>(pb + 22) == 0xfffe,
                    "Relative volume lookup ignored an explicit volume reference");
        }
        string(s, name, "Missing Volume:Data");
        require(call(s, "CarbonLib::PBHGetVInfoSync", {pb}) == -35,
                "Unknown absolute volume was accepted as a relative path");
        s.memory.write<2>(pb + 28, 2);
        require(call(s, "CarbonLib::PBHGetVInfoSync", {pb}) == 0 && s.memory.read<2>(pb + 22) == 0xfffe,
                "Writable volume enumeration");
        s.memory.write<2>(pb + 28, 3);
        require(call(s, "CarbonLib::PBHGetVInfoSync", {pb}) == -35 && s.memory.read<2>(pb + 16) == std::uint16_t(-35),
                "Volume enumeration did not terminate with nsvErr");
        require(call(s, "CarbonLib::FindFolder", {0xffff8000, 0x70726566, 1, pb, pb + 4}) == 0,
                "Native preferences directory lookup");
        require(s.memory.read<2>(pb) == 0xfffe && fs::is_directory(user / "Preferences"),
                "Preferences were not created on the private writable volume");
        string(s, name, "CM Preferences");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xfffffffe, std::uint32_t(s.memory.read<4>(pb + 4)), name, spec}) == -43,
                "Preference file creation specification");
        require(!fs::exists(game / "Preferences"), "Preferences modified the original media");
        s.memory.clear(pb, 64);
        s.memory.write<4>(pb + 32, name);
        s.memory.write<4>(pb + 36, 20);
        s.memory.write<1>(name + 20, 0xa5);
        require(call(s, "CarbonLib::PBHGetVolParmsSync", {pb}) == 0, "Native volume parameter query");
        require(s.memory.read<4>(pb + 40) == 20 && s.memory.read<2>(name) == 2,
                "32-bit volume parameter size/version");
        require(s.memory.read<4>(name + 6) == 0 && s.memory.read<1>(name + 20) == 0xa5,
                "Host pointer or excess bytes escaped into the volume parameter buffer");
        s.memory.write<4>(pb + 36, 3);
        s.memory.write<1>(name + 3, 0x5a);
        require(call(s, "CarbonLib::PBHGetVolParmsSync", {pb}) == 0 && s.memory.read<4>(pb + 40) == 3 &&
                s.memory.read<1>(name + 3) == 0x5a, "Partial volume parameter buffer overrun");
        string(s, name, "CM 01-02:Data:players.dat");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0, 0, name, spec}) == 0, "Read fixture specification");
        require(call(s, "CarbonLib::FSpOpenDF", {spec, 1, pb}) == 0, "Native data fork open");
        const auto file_ref = std::uint32_t(s.memory.read<2>(pb));
        require(call(s, "CarbonLib::GetEOF", {file_ref, pb + 4}) == 0 && s.memory.read<4>(pb + 4) == 13,
                "Native data fork size");
        s.memory.write<4>(pb + 4, 20);
        s.memory.clear(name, 32);
        s.memory.write<1>(name + 13, 0xa5);
        require(call(s, "CarbonLib::FSRead", {file_ref, pb + 4, name}) == -39 && s.memory.read<4>(pb + 4) == 13,
                "Partial read must preserve actual byte count and report eofErr");
        const auto read_bytes = s.memory.copy_out(name, 13);
        require(std::string(read_bytes.begin(), read_bytes.end()) == "original-data" && s.memory.read<1>(name + 13) == 0xa5,
                "Native read altered bytes or wrote beyond EOF");
        require(call(s, "CarbonLib::SetFPos", {file_ref, 1, 9}) == 0, "Native absolute seek");
        s.memory.write<4>(pb + 4, 4);
        require(call(s, "CarbonLib::FSRead", {file_ref, pb + 4, name}) == 0, "Read after seek");
        const auto suffix = s.memory.copy_out(name, 4);
        require(std::string(suffix.begin(), suffix.end()) == "data", "Native file mark was not preserved");
        s.memory.write<4>(pb + 4, 0);
        require(call(s, "CarbonLib::FSRead", {file_ref, pb + 4, name}) == 0 && s.memory.read<4>(pb + 4) == 0,
                "Zero-length read at EOF");
        s.memory.clear(pb, 64);
        s.memory.write<2>(pb + 24, file_ref);
        require(call(s, "CarbonLib::PBGetEOFSync", {pb}) == 0 && s.memory.read<4>(pb + 28) == 13,
                "Parameter-block EOF layout");
        s.memory.write<4>(pb + 32, name);
        s.memory.write<4>(pb + 36, 8);
        s.memory.write<2>(pb + 44, 1);
        s.memory.write<4>(pb + 46, 0);
        require(call(s, "CarbonLib::PBReadSync", {pb}) == 0 && s.memory.read<4>(pb + 40) == 8 &&
                s.memory.read<4>(pb + 46) == 8 && s.memory.read<2>(pb + 16) == 0,
                "Parameter-block read count, mark, and result");
        s.memory.write<2>(pb + 44, 3);
        s.memory.write<4>(pb + 46, 1);
        require(call(s, "CarbonLib::PBSetFPosSync", {pb}) == 0 && s.memory.read<4>(pb + 46) == 9,
                "Seek relative to the current native file mark");
        require(call(s, "CarbonLib::PBCloseSync", {pb}) == 0, "Native parameter-block close");
        require(call(s, "CarbonLib::FSClose", {file_ref}) == -51, "Closed file reference remained usable");
        const std::vector<std::uint8_t> large(1024 * 1024 + 19, 0xc5);
        {
            std::ofstream large_file(game / "large.dat", std::ios::binary);
            large_file.write(reinterpret_cast<const char*>(large.data()), large.size());
        }
        string(s, name, "large.dat");
        s.memory.clear(pb, 64);
        s.memory.write<4>(pb + 18, name);
        s.memory.write<1>(pb + 27, 1);
        s.memory.write<4>(pb + 48, 2);
        require(call(s, "CarbonLib::PBHOpenDFSync", {pb}) == 0, "Native hierarchical data fork open");
        const auto destination = allocate(s, large.size() + 1, true);
        s.memory.write<4>(pb + 32, destination);
        s.memory.write<4>(pb + 36, large.size() + 1);
        s.memory.write<2>(pb + 44, 0);
        require(call(s, "CarbonLib::PBReadSync", {pb}) == -39 && s.memory.read<4>(pb + 40) == large.size() &&
                s.memory.copy_out(destination, large.size()) == large,
                "Read across native chunk boundary lost data or EOF count");
        call(s, "CarbonLib::PBCloseSync", {pb});
        string(s, name, "games");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, 2, name, spec}) == -43,
                "New logical game directory specification");
        const auto original_spec = s.memory.copy_out(spec, 70);
        require(call(s, "CarbonLib::FSpDirCreate", {spec, 0xfffffffe, pb}) == 0,
                "Native private backing directory creation");
        const auto games_id = std::uint32_t(s.memory.read<4>(pb));
        require(fs::is_directory(user / "Files" / "games") && !fs::exists(game / "games"),
                "Game directory creation wrote to original media");
        require(s.memory.copy_out(spec, 70) == original_spec, "FSpDirCreate changed its const input specification");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, 2, name, spec}) == 0 &&
                s.memory.read<2>(spec) == 0xffff && s.memory.read<4>(spec + 2) == 2,
                "Private backing changed the original volume/parent identity");
        require(call(s, "CarbonLib::FSpDirCreate", {spec, 0xfffffffe, pb}) == -48,
                "Existing directory creation must report dupFNErr");
        string(s, name, "season");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, games_id, name, spec}) == -43,
                "Directory ID returned by FSpDirCreate is unusable with the original volume");
        require(call(s, "CarbonLib::FSpDirCreate", {spec, 0xfffffffe, pb}) == 0 &&
                fs::is_directory(user / "Files" / "games" / "season"), "Nested logical game directory creation");
        string(s, name, "CM 01-02:games:season");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0, 0, name, spec}) == 0,
                "Absolute Classic path did not resolve the private backing directory");
        string(s, name, "::Data:players.dat");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, games_id, name, spec}) == 0,
                "Traversal from private game directory could not reach original data");
        s.memory.clear(pb, 108);
        s.memory.write<2>(pb + 22, 0xffff);
        s.memory.write<2>(pb + 28, 0xffff);
        s.memory.write<4>(pb + 48, games_id);
        require(call(s, "CarbonLib::PBGetCatInfoSync", {pb}) == 0 && s.memory.read<4>(pb + 100) == 2 &&
                s.memory.read<2>(pb + 52) == 1, "Private directory catalog parent and child count");
        bool enumerated_games = false;
        for (unsigned index = 1; index <= 8; ++index) {
            s.memory.clear(pb, 108);
            s.memory.write<4>(pb + 18, name);
            s.memory.write<2>(pb + 22, 0xffff);
            s.memory.write<2>(pb + 28, index);
            s.memory.write<4>(pb + 48, 2);
            const auto result = call(s, "CarbonLib::PBGetCatInfoSync", {pb});
            if (result == -43) break;
            require(result == 0, "Merged game-directory enumeration failed");
            if (string(s, name) == "games") {
                enumerated_games = true;
                require(s.memory.read<4>(pb + 48) == games_id, "Enumerated directory ID changed");
            }
        }
        require(enumerated_games, "Private game directory is missing from the original root enumeration");
        string(s, name, "CM3 Hall Of Fame");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, 2, name, spec}) == -43, "New private data-file specification");
        require(call(s, "CarbonLib::FSpCreate", {spec, 0x3f3f3f3f, 0x46616d65, 0xffffffff}) == 0,
                "Native data-file creation");
        const auto saved = user / "Files" / "CM3 Hall Of Fame";
        require(fs::is_regular_file(saved) && !fs::exists(game / "CM3 Hall Of Fame"), "Data-file creation changed original media");
        std::array<std::uint8_t, 32> finder{};
        require(getxattr(saved.c_str(), "com.apple.FinderInfo", finder.data(), finder.size(), 0, 0) == 32 &&
                std::string(finder.begin(), finder.begin() + 8) == "Fame????", "Native created-file Finder metadata byte order");
        require(call(s, "CarbonLib::FSpCreate", {spec, 0, 0, 0xffffffff}) == -48, "Data-file creation overwrote an existing file");
        require(call(s, "CarbonLib::FSpOpenDF", {spec, 3, pb}) == 0, "Native read/write data-fork open");
        const auto write_ref = std::uint32_t(s.memory.read<2>(pb));
        std::vector<std::uint8_t> write_payload(large.size());
        for (std::size_t i = 0; i < write_payload.size(); ++i) write_payload[i] = (i * 17) ^ (i >> 10);
        s.memory.copy_in(destination, write_payload);
        s.memory.clear(pb, 64); s.memory.write<2>(pb + 24, write_ref);
        s.memory.write<4>(pb + 32, destination); s.memory.write<4>(pb + 36, write_payload.size());
        require(call(s, "CarbonLib::PBWriteSync", {pb}) == 0 && s.memory.read<4>(pb + 40) == write_payload.size() &&
                s.memory.read<4>(pb + 46) == write_payload.size() && s.memory.read<2>(pb + 16) == 0,
                "Chunked data write lost count, mark, or original ioResult fields");
        require(call(s, "CarbonLib::FSClose", {write_ref}) == 0, "Native written data-file close");
        {
            std::ifstream stored(saved, std::ios::binary);
            const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stored)), {});
            require(bytes == write_payload, "Native bytes on disk differ across the write chunk boundary");
        }
        require(call(s, "CarbonLib::FSpOpenDF", {spec, 3, pb}) == 0, "Written file reopen through its original specification");
        const auto resized_ref = std::uint32_t(s.memory.read<2>(pb));
        s.memory.clear(pb, 64); s.memory.write<2>(pb + 24, resized_ref); s.memory.write<4>(pb + 28, 27);
        require(call(s, "CarbonLib::PBSetEOFSync", {pb}) == 0 && fs::file_size(saved) == 27,
                "Native data-file truncation and original parameter-block layout");
        s.memory.write<2>(pb + 44, 2); s.memory.write<4>(pb + 46, 5);
        require(call(s, "CarbonLib::PBSetFPosSync", {pb}) == -39 && s.memory.read<4>(pb + 46) == 27,
                "Classic EOF seek must report eofErr and the clamped file mark");
        require(call(s, "CarbonLib::SetEOF", {resized_ref, 32}) == 0 &&
                call(s, "CarbonLib::SetFPos", {resized_ref, 2, 0}) == 0, "Native EOF extension and seek to the new end");
        s.memory.copy_in(name, {9, 8, 7}); s.memory.write<4>(pb, 3);
        require(call(s, "CarbonLib::FSWrite", {resized_ref, pb, name}) == 0 && s.memory.read<4>(pb) == 3,
                "Native write after an EOF-relative seek");
        call(s, "CarbonLib::FSClose", {resized_ref});
        {
            std::ifstream stored(saved, std::ios::binary);
            const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stored)), {});
            std::vector<std::uint8_t> expected(write_payload.begin(), write_payload.begin() + 27);
            expected.resize(32); expected.insert(expected.end(), {9, 8, 7});
            require(bytes == expected, "Native EOF extension failed to preserve data or zero the gap");
        }
        string(s, name, "CM 01-02:Data:players.dat");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0, 0, name, spec}) == 0, "Original copy-on-write specification");
        require(call(s, "CarbonLib::FSpOpenDF", {spec, 1, pb}) == 0, "Original read-only data-fork open");
        const auto read_only_ref = std::uint32_t(s.memory.read<2>(pb));
        s.memory.write<4>(pb, 1);
        require(call(s, "CarbonLib::FSWrite", {read_only_ref, pb, name}) != 0 && s.memory.read<4>(pb) == 0,
                "Read-only native handle accepted a write");
        call(s, "CarbonLib::FSClose", {read_only_ref});
        require(call(s, "CarbonLib::FSpOpenDF", {spec, 3, pb}) == 0, "Private copy-on-write open");
        const auto copied_ref = std::uint32_t(s.memory.read<2>(pb));
        s.memory.copy_in(name, {'L', 'O', 'C', 'A', 'L'}); s.memory.write<4>(pb, 5);
        require(call(s, "CarbonLib::FSWrite", {copied_ref, pb, name}) == 0, "Private copy-on-write update");
        call(s, "CarbonLib::FSClose", {copied_ref});
        {
            std::ifstream source(game / "Data" / "players.dat"), copied(user / "Files" / "Data" / "players.dat");
            const std::string source_bytes((std::istreambuf_iterator<char>(source)), {});
            const std::string copied_bytes((std::istreambuf_iterator<char>(copied)), {});
            require(source_bytes == "original-data" && copied_bytes == "LOCALnal-data", "Copy-on-write altered original bytes or lost the untouched suffix");
        }
        require(call(s, "CarbonLib::FSpOpenDF", {spec, 1, pb}) == 0, "Reopen did not resolve the private overlay");
        const auto overlay_ref = std::uint32_t(s.memory.read<2>(pb));
        s.memory.write<4>(pb, 5);
        require(call(s, "CarbonLib::FSRead", {overlay_ref, pb, name}) == 0 &&
                s.memory.copy_out(name, 5) == std::vector<std::uint8_t>({'L', 'O', 'C', 'A', 'L'}),
                "Original filename lookup ignored updated private file contents");
        call(s, "CarbonLib::FSClose", {overlay_ref});
        auto delete_named = [&](const std::string& filename) {
            string(s, name, filename);
            s.memory.clear(pb, 64);
            s.memory.write<4>(pb + 18, name);
            s.memory.write<2>(pb + 22, 0xffff);
            s.memory.write<4>(pb + 48, 2);
            s.memory.write<4>(pb + 52, 0xa5a5a5a5);
            const auto result = call(s, "CarbonLib::PBHDeleteSync", {pb});
            require(s.memory.read<2>(pb + 16) == std::uint16_t(result) &&
                    s.memory.read<4>(pb + 52) == 0xa5a5a5a5,
                    "Delete did not preserve the original parameter block and 16-bit result");
            return result;
        };
        const auto deleted = user / "Files" / "games" / "delete.dat";
        std::ofstream(deleted) << "private-data";
        const std::string resource_bytes = "private-resource";
        require(setxattr(deleted.c_str(), "com.apple.ResourceFork", resource_bytes.data(), resource_bytes.size(), 0, 0) == 0,
                "Delete resource-fork fixture");
        string(s, name, "CM 01-02:games:delete.dat");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0, 0, name, spec}) == 0 &&
                call(s, "CarbonLib::FSpOpenDF", {spec, 1, pb}) == 0, "Open deletion fixture");
        const auto delete_ref = std::uint32_t(s.memory.read<2>(pb));
        require(delete_named("CM 01-02:games:DELETE.DAT") == -47 && fs::is_regular_file(deleted),
                "Deletion of an open file did not return fBsyErr");
        call(s, "CarbonLib::FSClose", {delete_ref});
        require(delete_named("CM 01-02:games:DELETE.DAT") == 0 && !fs::exists(deleted) &&
                !fs::exists(deleted / "..namedfork/rsrc"), "Private deletion retained a data or resource fork");
        require(delete_named("CM 01-02:games:delete.dat") == -43, "Missing deletion did not return fnfErr");
        require(delete_named("CM 01-02:games") == -47 && fs::is_regular_file(saved),
                "Nonempty directory deletion removed private saves");
        fs::create_directory(user / "Files" / "empty");
        require(delete_named("CM 01-02:empty") == 0 && !fs::exists(user / "Files" / "empty"),
                "Empty private directory deletion");
        require(delete_named("CM 01-02:") == -47 && fs::is_directory(game), "Volume root deletion was accepted");
        for (const auto& protected_name : {"CM 01-02:Data:players.dat", "CM 01-02:Data:Caf\x8e.dat"}) {
            bool stopped = false;
            try { delete_named(protected_name); } catch (const std::runtime_error&) { stopped = true; }
            require(stopped, "Original/overridden media deletion did not stop before mutation");
        }
        require(fs::is_regular_file(game / "Data" / "players.dat") &&
                fs::is_regular_file(user / "Files" / "Data" / "players.dat"),
                "Unsupported deletion modified original media or revealed an old version");
        fs::create_directory(game / "Merged");
        fs::create_directory(user / "Files" / "Merged");
        std::ofstream(game / "Merged" / "Caf\xc3\xa9.dat") << "old";
        std::ofstream(game / "Merged" / "zeta.dat") << "last";
        std::ofstream(game / "Merged" / "alpha.dat") << "first";
        // CoreServices creates decomposed names. They must override an NFC
        // original with the same case-insensitive canonical spelling.
        std::ofstream(user / "Files" / "Merged" / "CAFE\xcc\x81.DAT") << "private replacement";
        std::ofstream(user / "Files" / "Merged" / "beta.dat") << "second";
        string(s, name, "Merged");
        s.memory.clear(pb, 108);
        s.memory.write<4>(pb + 18, name);
        s.memory.write<2>(pb + 22, 0xffff);
        s.memory.write<4>(pb + 48, 2);
        require(call(s, "CarbonLib::PBGetCatInfoSync", {pb}) == 0 && s.memory.read<2>(pb + 52) == 4,
                "Case/accent overlay produced a duplicate directory entry");
        const auto merged_id = std::uint32_t(s.memory.read<4>(pb + 48));
        const std::array<std::string, 4> expected_names{"CAF\x83.DAT", "alpha.dat", "beta.dat", "zeta.dat"};
        for (unsigned index = 1; index <= expected_names.size() + 1; ++index) {
            s.memory.clear(pb, 108);
            s.memory.write<4>(pb + 18, name);
            s.memory.write<2>(pb + 22, 0xffff);
            s.memory.write<2>(pb + 28, index);
            s.memory.write<4>(pb + 48, merged_id);
            const auto result = call(s, "CarbonLib::PBGetCatInfoSync", {pb});
            if (index > expected_names.size()) require(result == -43, "Merged enumeration contains extra entries");
            else {
                require(result == 0 && string(s, name) == expected_names[index - 1],
                        "Merged enumeration changed filename spelling or byte order");
                if (index == 1) require(s.memory.read<4>(pb + 54) == 19,
                                        "Case/accent overlay did not select the private file");
            }
        }
        std::ofstream(user / "Files" / "Merged" / "late.dat") << "new";
        s.memory.clear(pb, 108);
        s.memory.write<2>(pb + 22, 0xffff);
        s.memory.write<2>(pb + 28, 0xffff);
        s.memory.write<4>(pb + 48, merged_id);
        require(call(s, "CarbonLib::PBGetCatInfoSync", {pb}) == 0 && s.memory.read<2>(pb + 52) == 5,
                "Directory listing hid an external file added after the previous query");
        require(fs::file_size(game / "Merged" / "Caf\xc3\xa9.dat") == 3,
                "Merged listing modified the original file");
        // Reproduce the live save failure: the game's MacRoman leaf name is
        // composed, while FSCreateFileUnicode persists its decomposed form.
        const std::string cup_name = "Copa Am\xe9rica_407.tmp";
        string(s, name, cup_name);
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, 2, name, spec}) == -43,
                "New accented temporary file unexpectedly exists");
        require(call(s, "CarbonLib::FSpCreate", {spec, 0, 0, 0xffffffff}) == 0,
                "Native accented temporary-file creation");
        require(fs::is_regular_file(user / "Files" / "Copa AmE\xcc\x80rica_407.tmp"),
                "Accented temporary file was not created in private backing");
        string(s, name, cup_name);
        s.memory.clear(pb, 108);
        s.memory.write<4>(pb + 18, name);
        require(call(s, "CarbonLib::PBGetCatInfoSync", {pb}) == 0 && s.memory.read<4>(pb + 54) == 0,
                "A just-created decomposed temporary file is invisible to its MacRoman catalog lookup");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, 2, name, spec}) == 0 &&
                call(s, "CarbonLib::FSpOpenDF", {spec, 1, pb}) == 0,
                "Cannot reopen a decomposed filename through its original MacRoman name");
        require(call(s, "CarbonLib::FSClose", {std::uint32_t(s.memory.read<2>(pb))}) == 0,
                "Cannot close the accented temporary file");
        string(s, name, "Copa AmErica_407.tmp");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, 2, name, spec}) == -43,
                "Canonical comparison erased a meaningful accent");
        auto rename_named = [&](const std::string& old_name, const std::string& new_name) {
            string(s, name, old_name); string(s, name + 300, new_name);
            s.memory.copy_in(pb, std::vector<std::uint8_t>(64, 0xa5));
            s.memory.write<4>(pb + 18, name);
            s.memory.write<2>(pb + 22, 0xffff);
            s.memory.write<4>(pb + 28, name + 300);
            s.memory.write<4>(pb + 48, 2);
            const auto before = s.memory.copy_out(pb, 64);
            const auto result = call(s, "CarbonLib::PBHRenameSync", {pb});
            auto expected = before;
            write_be<2>(expected.data() + 16, std::uint16_t(result));
            require(s.memory.copy_out(pb, 64) == expected, "Rename changed input fields or missed ioResult");
            return result;
        };
        auto catalog_id = [&](const std::string& filename) {
            string(s, name, filename); s.memory.clear(pb, 108);
            s.memory.write<4>(pb + 18, name);
            require(call(s, "CarbonLib::PBGetCatInfoSync", {pb}) == 0, "Rename fixture catalog lookup");
            return s.memory.read<4>(pb + 48);
        };
        const auto old_save = user / "Files" / "games" / "rename-temp.sav";
        const auto new_save = user / "Files" / "games" / "Renamed.sav";
        std::ofstream(old_save) << "serialized-save";
        require(setxattr(old_save.c_str(), "com.apple.ResourceFork", resource_bytes.data(), resource_bytes.size(), 0, 0) == 0 &&
                setxattr(old_save.c_str(), "com.apple.FinderInfo", finder.data(), finder.size(), 0, 0) == 0,
                "Rename fork/metadata fixture");
        const auto old_id = catalog_id("CM 01-02:games:rename-temp.sav");
        string(s, name, "CM 01-02:games:rename-temp.sav");
        require(call(s, "CarbonLib::FSMakeFSSpec", {0, 0, name, spec}) == 0 &&
                call(s, "CarbonLib::FSpOpenDF", {spec, 1, pb}) == 0, "Open rename fixture");
        const auto renamed_ref = std::uint32_t(s.memory.read<2>(pb));
        require(rename_named("CM 01-02:games:rename-temp.sav", "Renamed.sav") == 0 &&
                !fs::exists(old_save) && fs::file_size(new_save) == 15, "Private save rename failed");
        require(catalog_id("CM 01-02:games:Renamed.sav") == old_id, "Rename changed the existing file ID");
        s.memory.write<4>(pb, 15);
        require(call(s, "CarbonLib::FSRead", {renamed_ref, pb, name}) == 0 &&
                s.memory.copy_out(name, 15) == std::vector<std::uint8_t>({'s','e','r','i','a','l','i','z','e','d','-','s','a','v','e'}),
                "Rename invalidated an open data fork");
        call(s, "CarbonLib::FSClose", {renamed_ref});
        std::array<std::uint8_t, 32> renamed_finder{};
        require(getxattr(new_save.c_str(), "com.apple.FinderInfo", renamed_finder.data(), renamed_finder.size(), 0, 0) == 32 &&
                renamed_finder == finder, "Rename changed Finder metadata");
        std::ifstream renamed_resource(new_save / "..namedfork/rsrc", std::ios::binary);
        require(std::string(std::istreambuf_iterator<char>(renamed_resource), {}) == resource_bytes,
                "Rename changed the resource fork");
        std::ofstream(user / "Files" / "games" / "occupied.sav") << "keep";
        require(rename_named("CM 01-02:games:Renamed.sav", "occupied.sav") == -48 &&
                fs::file_size(new_save) == 15 && fs::file_size(user / "Files" / "games" / "occupied.sav") == 4,
                "Rename overwrote an existing destination");
        require(rename_named("CM 01-02:games:missing.sav", "new.sav") == -43 &&
                rename_named("CM 01-02:games:Renamed.sav", ":outside") == -37,
                "Rename accepted a missing source or directory-changing name");
        const auto season_id = catalog_id("CM 01-02:games:season");
        std::ofstream(user / "Files" / "games" / "season" / "child.sav") << "child";
        const auto child_id = catalog_id("CM 01-02:games:season:child.sav");
        require(rename_named("CM 01-02:games:season", "season-new") == 0 &&
                catalog_id("CM 01-02:games:season-new") == season_id &&
                catalog_id("CM 01-02:games:season-new:child.sav") == child_id,
                "Directory rename changed existing directory or descendant IDs");
        bool protected_rename = false;
        try { rename_named("CM 01-02:Data:players.dat", "moved.dat"); }
        catch (const std::runtime_error&) { protected_rename = true; }
        require(protected_rename && fs::exists(game / "Data" / "players.dat") &&
                fs::exists(user / "Files" / "Data" / "players.dat"), "Rename changed original or overlaid media");
        bool rejected = false;
        try { NativePlatform invalid(game, game / "Unwanted"); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected && !fs::exists(game / "Unwanted"), "Invalid configuration modified game data");
        std::cout << "Native filesystem checks passed against actual files and directories\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

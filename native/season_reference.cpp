#include "season_reference.hpp"
#include "match_replay.hpp"
#include "callbacks.hpp"
#include "heap_changes.hpp"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <optional>
#include <map>
#include <set>
#include <string_view>

namespace cm::aot {
namespace {
bool semantic_calendar_replay = false;
bool live_pure_imports = false;
bool relaxed_import_inputs = false;
std::uint64_t relaxed_input_digest = 0xcbf29ce484222325;  // FNV-1a over what the imports observed
bool rng_checks = true;
constexpr std::uint32_t day_pc = 0x101195e8;
constexpr std::uint32_t date_global = 0x108b3f10, phase_global = 0x108b40a4;
// Version 1 stores every register, the generated temporaries and both heap
// tables at each import; version 2 stores the register bytes that changed
// since the previous record and the heap-table changes the import made
// (a week of the 26-country career went from 34 GB to a few).
constexpr std::array<char, 8> magic{'C','M','D','A','Y','0','0','1'}, magic_v2{'C','M','D','A','Y','0','0','2'};
unsigned transcript_version = 2;
using Registers = decltype(State::registers);
enum class Event : std::uint8_t { import, write, clear, map, chunk, callback, callback_end, import_end, boundary, rng };
struct Stream {
    gzFile file = nullptr;
    bool writing;
    std::filesystem::path path;
    int descriptor = -1;
    explicit Stream(const std::filesystem::path& name, bool output) : writing(output), path(name) {
        descriptor = output ? ::open(name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644) : ::open(name.c_str(), O_RDONLY);
        if (descriptor >= 0) file = gzdopen(descriptor, output ? "wb1" : "rb");
        if (!file) {
            if (descriptor >= 0) ::close(descriptor);
            throw std::runtime_error("Cannot open private season transcript");
        }
    }
    // A forked process shares the descriptor's file offset with its parent;
    // give the child its own, at the same position, so its reads leave the
    // parent's transcript where it was.
    void detach_after_fork() {
        const int own = ::open(path.c_str(), O_RDONLY);
        if (own < 0 || ::lseek(own, ::lseek(descriptor, 0, SEEK_CUR), SEEK_SET) < 0 || ::dup2(own, descriptor) < 0)
            throw std::runtime_error("Cannot detach the private season transcript");
        ::close(own);
    }
    ~Stream() { if (file) gzclose(file); }
    void bytes(void* data, std::size_t size) {
        auto* position = static_cast<std::uint8_t*>(data);
        while (size) {
            const auto count = unsigned(std::min<std::size_t>(size, 1 << 20));
            const auto result = writing ? gzwrite(file, position, count) : gzread(file, position, count);
            if (result != int(count)) throw std::runtime_error("Truncated or unwritable private season transcript");
            position += count; size -= count;
        }
    }
    template <typename T> void put(const T& value) { auto copy = value; bytes(&copy, sizeof(copy)); }
    template <typename T> T take() { T value{}; bytes(&value, sizeof(value)); return value; }
    void finish() {
        const auto result = gzclose(file); file = nullptr;
        if (result != Z_OK) throw std::runtime_error("Cannot finalize private season transcript");
    }
    bool at_end() {
        const int next = gzgetc(file);
        if (next == -1) return gzeof(file);
        gzungetc(next, file);
        return false;
    }
    void require_eof() {
        std::uint8_t byte{};
        if (gzread(file, &byte, 1) != 0 || !gzeof(file))
            throw std::runtime_error("Private season transcript has trailing data or a checksum error");
    }
};
std::uint32_t pointer(const State& s, std::uint32_t global) { return std::uint32_t(s.memory.read<4>(global)); }
std::array<std::uint16_t, 3> date(const State& s) {
    const auto address = pointer(s, date_global);
    return {std::uint16_t(s.memory.read<2>(address)), std::uint16_t(s.memory.read<2>(address + 2)),
            std::uint16_t(s.memory.read<1>(pointer(s, phase_global)))};
}
void take_table(Stream& stream, std::map<std::uint32_t, std::uint32_t>& table) {
    const auto count = stream.take<std::uint32_t>();
    if (count > 1000000) throw std::runtime_error("Invalid private season allocation count");
    table.clear();
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto address = stream.take<std::uint32_t>(), size = stream.take<std::uint32_t>();
        if (!table.emplace(address, size).second) throw std::runtime_error("Duplicate private season allocation");
    }
}
// Version 2 registers: runs of bytes that differ from the previous record's
// registers (`base`), each as offset, length and bytes; 0xffff ends the list.
void put_registers(Stream& stream, Registers& base, const Registers& now) {
    constexpr std::size_t block = 64;
    std::size_t at = 0;
    while (at < now.size()) {
        const auto length = std::min(block, now.size() - at);
        if (!std::memcmp(now.data() + at, base.data() + at, length)) { at += length; continue; }
        for (const auto end = at + length; at < end;) {
            if (now[at] == base[at]) { ++at; continue; }
            const auto start = at;
            while (at < now.size() && now[at] != base[at] && at - start < 0xfffe) ++at;
            stream.put(std::uint16_t(start)); stream.put(std::uint16_t(at - start));
            stream.bytes(const_cast<std::uint8_t*>(now.data() + start), at - start);
        }
    }
    stream.put(std::uint16_t(0xffff));
    base = now;
}
void take_registers(Stream& stream, Registers& base) {
    for (;;) {
        const auto start = stream.take<std::uint16_t>();
        if (start == 0xffff) return;
        const auto length = stream.take<std::uint16_t>();
        if (std::size_t(start) + length > base.size()) throw std::runtime_error("Invalid private season register run");
        stream.bytes(base.data() + start, length);
    }
}
void apply_heap_changes(std::map<std::uint32_t, std::uint32_t>& allocated, std::map<std::uint32_t, std::uint32_t>& available,
                        const std::vector<HeapChange>& changes) {
    for (const auto& change : changes) {
        auto& table = change.table ? available : allocated;
        if (!change.erase) table[change.address] = change.size;
        else if (!table.erase(change.address)) throw std::runtime_error("First divergence: recorded heap change on a missing block");
    }
}
// The heap state an import leaves behind: the second part of every chunk.
struct Allocator {
    std::map<std::uint32_t, std::uint32_t> allocated, available;
    std::uint32_t heap_end = 0;
    std::int16_t memory_error = 0;
    explicit Allocator(const State& s)
        : allocated(s.allocated), available(s.available), heap_end(s.heap_end), memory_error(s.memory_error) {}
    explicit Allocator(Stream& stream) {
        heap_end = stream.take<std::uint32_t>(); memory_error = stream.take<std::int16_t>();
        take_table(stream, allocated); take_table(stream, available);
    }
    void restore(State& s) && {
        s.allocated = std::move(allocated); s.available = std::move(available);
        s.heap_end = heap_end; s.memory_error = memory_error;
    }
    bool operator==(const Allocator&) const = default;
    // In place: copying the heap tables for every replayed import dominated
    // week-long replays.
    bool matches(const State& s) const {
        return s.heap_end == heap_end && s.memory_error == memory_error && s.allocated == allocated && s.available == available;
    }
};
// The first part of every chunk: the PC, registers and generated temporaries.
constexpr std::size_t chunk_registers_size = sizeof(State::pc) + sizeof(State::registers) + sizeof(State::unique);
// Registers of the last record read (an import's input or a chunk); version
// 2 records are deltas against it.
Registers record_registers{};
// A version 2 chunk: PC, register delta, a reserved flag (0), heap end,
// MemError and the import's heap-table changes.
struct Chunk {
    std::uint32_t pc = 0, heap_end = 0;
    std::int16_t memory_error = 0;
    std::vector<HeapChange> changes;
};
Chunk take_chunk_v2(Stream& stream) {
    Chunk chunk;
    chunk.pc = stream.take<std::uint32_t>();
    take_registers(stream, record_registers);
    if (stream.take<std::uint8_t>() != 0) throw std::runtime_error("Unsupported private season chunk flag");
    chunk.heap_end = stream.take<std::uint32_t>(); chunk.memory_error = stream.take<std::int16_t>();
    const auto count = stream.take<std::uint32_t>();
    if (count > 10000000) throw std::runtime_error("Invalid private season heap change count");
    chunk.changes.resize(count);
    for (auto& change : chunk.changes) {
        change.table = stream.take<std::uint8_t>(); change.erase = stream.take<std::uint8_t>();
        change.address = stream.take<std::uint32_t>(); change.size = stream.take<std::uint32_t>();
        if (change.table > 1 || change.erase > 1) throw std::runtime_error("Invalid private season heap change");
    }
    return chunk;
}
// Applies a version 2 chunk the replay does not compute itself (an OS
// import's effects). Its heap changes are relative to the recorded heap,
// which the live heap equals unless the replay has already diverged.
void apply_chunk(State& s, const Chunk& chunk) {
    s.pc = chunk.pc; s.registers = record_registers;
    apply_heap_changes(s.allocated, s.available, chunk.changes);
    s.heap_end = chunk.heap_end; s.memory_error = chunk.memory_error;
}
void take_chunk(Stream& stream, State& s) {
    if (transcript_version == 2) { apply_chunk(s, take_chunk_v2(stream)); return; }
    s.pc = stream.take<std::uint32_t>();
    s.registers = stream.take<decltype(s.registers)>();
    s.unique = stream.take<decltype(s.unique)>();
    Allocator(stream).restore(s);
}
void skip(Stream& stream, std::uint64_t size) {
    std::array<std::uint8_t, 65536> scratch;
    while (size) {
        const auto count = std::size_t(std::min<std::uint64_t>(size, scratch.size()));
        stream.bytes(scratch.data(), count); size -= count;
    }
}
// Heap state after the last recorded import. A live replay computes pure
// imports itself; the heap it reaches must match before each OS import.
std::optional<Allocator> recorded_allocator;
// Reads a chunk's allocator state and skips its registers. Version 2
// applies the chunk's heap changes to the recorded heap instead.
void skip_chunk(Stream& stream) {
    if (transcript_version == 2) {
        auto chunk = take_chunk_v2(stream);
        if (!recorded_allocator) return;  // nothing to check against yet
        apply_heap_changes(recorded_allocator->allocated, recorded_allocator->available, chunk.changes);
        recorded_allocator->heap_end = chunk.heap_end; recorded_allocator->memory_error = chunk.memory_error;
        return;
    }
    skip(stream, chunk_registers_size);
    recorded_allocator = Allocator(stream);
}
void skip_rng_write(Stream& stream) {
    if (transcript_version == 2) {
        stream.take<std::uint8_t>();
        const auto size = stream.take<std::uint8_t>();
        if (size > 8) throw std::runtime_error("Invalid original RNG state width");
        std::array<std::uint8_t, 8> skipped{}; stream.bytes(skipped.data(), size);
        return;
    }
    stream.take<std::uint32_t>();
    const auto size = stream.take<std::uint64_t>();
    if (size > 8) throw std::runtime_error("Invalid original RNG state width");
    std::array<std::uint8_t, 8> skipped{}; stream.bytes(skipped.data(), size);
}
// Timing replays never compare the heap: skip the tables unread. Each chunk
// holds the whole heap table, and rebuilding it as maps dominated replays.
void skip_unchecked_chunk(Stream& stream) {
    if (transcript_version == 2) {
        // Registers still advance the delta base; the heap changes are skipped unread.
        stream.take<std::uint32_t>();
        take_registers(stream, record_registers);
        if (stream.take<std::uint8_t>() != 0) throw std::runtime_error("Unsupported private season chunk flag");
        skip(stream, sizeof(std::uint32_t) + sizeof(std::int16_t));
        const auto count = stream.take<std::uint32_t>();
        if (count > 10000000) throw std::runtime_error("Invalid private season heap change count");
        skip(stream, std::uint64_t(count) * (2 + 4 + 4));
        return;
    }
    skip(stream, chunk_registers_size + sizeof(std::uint32_t) + sizeof(std::int16_t));
    for (unsigned table = 0; table < 2; ++table) {
        const auto count = stream.take<std::uint32_t>();
        if (count > 1000000) throw std::runtime_error("Invalid private season allocation count");
        skip(stream, std::uint64_t(count) * 8);
    }
}
std::string heap_difference(const Allocator& expected, const Allocator& actual) {
    if (expected.heap_end != actual.heap_end)
        return "heap end " + hex_address(expected.heap_end) + " expected, " + hex_address(actual.heap_end) + " found";
    if (expected.memory_error != actual.memory_error)
        return "MemError " + std::to_string(expected.memory_error) + " expected, " + std::to_string(actual.memory_error) + " found";
    for (const auto& [name, a, b] : {std::tuple{"allocation", &expected.allocated, &actual.allocated},
                                     std::tuple{"free block", &expected.available, &actual.available}}) {
        const auto [x, y] = std::mismatch(a->begin(), a->end(), b->begin(), b->end());
        if (x == a->end() && y == b->end()) continue;
        const auto describe = [](auto it, auto end) {
            return it == end ? std::string("none") : hex_address(it->first) + " size " + std::to_string(it->second);
        };
        return std::string("first differing ") + name + ": " + describe(x, a->end()) + " expected, " + describe(y, b->end()) + " found";
    }
    return "identical";
}
// Recorder side (always version 2): `base` holds the registers of the
// previous record written.
void put_input(Stream& stream, Registers& base, const State& s) {
    stream.put(s.pc); stream.put(s.executed); stream.put(s.native_calls); put_registers(stream, base, s.registers);
}
void put_chunk(Stream& stream, Registers& base, const State& s, std::vector<HeapChange>& changes) {
    stream.put(Event::chunk); stream.put(s.pc); put_registers(stream, base, s.registers);
    stream.put(std::uint8_t(0)); stream.put(s.heap_end); stream.put(s.memory_error);
    stream.put(std::uint32_t(changes.size()));
    for (const auto& change : changes) { stream.put(change.table); stream.put(change.erase); stream.put(change.address); stream.put(change.size); }
    changes.clear();
}
// What the bridges know of an import: the argument registers it reads (r3
// upward, f1 upward; a double also takes two integer slots, so num2dec reads
// r3, f1 and r6), and whether it is pure, computed from guest state alone, so
// an operation replay can run it live. Unlisted imports are replayed and
// checked on all eight integer and thirteen floating arguments.
struct ImportContract { unsigned integer, floating; bool pure; };
ImportContract import_contract(std::string_view name) {
    static const std::map<std::string_view, ImportContract> known{
        {"CarbonLib::pow", {0, 2, true}}, {"CarbonLib::atan2", {0, 2, true}}, {"CarbonLib::sin", {0, 1, true}},
        {"CarbonLib::cos", {0, 1, true}}, {"CarbonLib::atan", {0, 1, true}}, {"CarbonLib::sqrt", {0, 1, true}},
        {"CarbonLib::num2dec", {4, 1, true}}, {"CarbonLib::dec2num", {1, 0, true}}, {"CarbonLib::nan", {1, 0, true}},
        {"CarbonLib::NewPtr", {1, 0, true}}, {"CarbonLib::NewPtrClear", {1, 0, true}},
        {"CarbonLib::DisposePtr", {1, 0, true}}, {"CarbonLib::BlockMoveData", {3, 0, true}},
        {"CarbonLib::MemError", {0, 0, true}},
        {"CarbonLib::X2Fix", {0, 1, false}}, {"CarbonLib::x80tod", {1, 0, false}},
        {"CarbonLib::PBGetForkSizeSync", {1, 0, false}}, {"CarbonLib::PBSetForkPositionSync", {1, 0, false}},
        {"CarbonLib::PBWriteForkSync", {1, 0, false}}, {"CarbonLib::PBReadForkSync", {1, 0, false}},
        {"CarbonLib::FSCloseFork", {1, 0, false}}, {"CarbonLib::FSOpenFork", {5, 0, false}},
        {"CarbonLib::FSMakeFSSpec", {4, 0, false}}, {"CarbonLib::FSpMakeFSRef", {2, 0, false}},
        {"CarbonLib::FSMakeFSRefUnicode", {5, 0, false}}, {"CarbonLib::FSGetCatalogInfo", {6, 0, false}},
        {"CarbonLib::HGetVol", {3, 0, false}}, {"CarbonLib::ResolveAliasFile", {4, 0, false}},
        {"CarbonLib::UpgradeScriptInfoToTextEncoding", {5, 0, false}},
        {"CarbonLib::CreateTextToUnicodeInfoByEncoding", {2, 0, false}},
        {"CarbonLib::DisposeTextToUnicodeInfo", {1, 0, false}},
        {"CarbonLib::ConvertFromTextToUnicode", {8, 0, false}},
        // Carbon signatures (integer argument registers), so a rewrite's
        // leftover volatile registers are not mistaken for arguments.
        {"CarbonLib::SetCursor", {1, 0, false}},
        {"CarbonLib::SetCCursor", {1, 0, false}},
        {"CarbonLib::GetCursor", {1, 0, false}},
        {"CarbonLib::GetCCursor", {1, 0, false}},
        {"CarbonLib::DisposeCCursor", {1, 0, false}},
        {"CarbonLib::InitCursor", {0, 0, false}},
        {"CarbonLib::SetPort", {1, 0, false}},
        {"CarbonLib::GetPort", {1, 0, false}},
        {"CarbonLib::MoveTo", {2, 0, false}},
        {"CarbonLib::DrawText", {3, 0, false}},
        {"CarbonLib::TextFont", {1, 0, false}},
        {"CarbonLib::TextSize", {1, 0, false}},
        {"CarbonLib::TextWidth", {3, 0, false}},
        {"CarbonLib::ForeColor", {1, 0, false}},
        {"CarbonLib::PaintRect", {1, 0, false}},
        {"CarbonLib::InsetRect", {3, 0, false}},
        {"CarbonLib::SetRect", {5, 0, false}},
        {"CarbonLib::PtInRect", {2, 0, false}},
        {"CarbonLib::PtInRgn", {2, 0, false}},
        {"CarbonLib::PinRect", {2, 0, false}},
        {"CarbonLib::SetPt", {3, 0, false}},
        {"CarbonLib::LocalToGlobal", {1, 0, false}},
        {"CarbonLib::GetFontInfo", {1, 0, false}},
        {"CarbonLib::CopyBits", {6, 0, false}},
        {"CarbonLib::GetPortBitMapForCopyBits", {1, 0, false}},
        {"CarbonLib::GetPortPixMap", {1, 0, false}},
        {"CarbonLib::GetPixBaseAddr", {1, 0, false}},
        {"CarbonLib::LockPixels", {1, 0, false}},
        {"CarbonLib::NewGWorld", {6, 0, false}},
        {"CarbonLib::DisposeGWorld", {1, 0, false}},
        {"CarbonLib::QDFlushPortBuffer", {2, 0, false}},
        {"CarbonLib::GetWindowPort", {1, 0, false}},
        {"CarbonLib::GetWindowBounds", {3, 0, false}},
        {"CarbonLib::GetRegionBounds", {2, 0, false}},
        {"CarbonLib::NewRgn", {0, 0, false}},
        {"CarbonLib::DisposeRgn", {1, 0, false}},
        {"CarbonLib::GetGrayRgn", {0, 0, false}},
        {"CarbonLib::GetMainDevice", {0, 0, false}},
        {"CarbonLib::GetMaxDevice", {1, 0, false}},
        {"CarbonLib::SetFractEnable", {1, 0, false}},
        {"CarbonLib::FMGetFontFamilyFromName", {1, 0, false}},
        {"CarbonLib::SelectWindow", {1, 0, false}},
        {"CarbonLib::BringToFront", {1, 0, false}},
        {"CarbonLib::ShowWindow", {1, 0, false}},
        {"CarbonLib::DisposeWindow", {1, 0, false}},
        {"CarbonLib::DragWindow", {3, 0, false}},
        {"CarbonLib::TrackGoAway", {2, 0, false}},
        {"CarbonLib::FindWindow", {2, 0, false}},
        {"CarbonLib::Button", {0, 0, false}},
        {"CarbonLib::GetMouse", {1, 0, false}},
        {"CarbonLib::GetKeys", {1, 0, false}},
        {"CarbonLib::WaitNextEvent", {4, 0, false}},
        {"CarbonLib::FlushEvents", {2, 0, false}},
        {"CarbonLib::Microseconds", {1, 0, false}},
        {"CarbonLib::GetDateTime", {1, 0, false}},
        {"CarbonLib::MenuSelect", {1, 0, false}},
        {"CarbonLib::MenuKey", {1, 0, false}},
        {"CarbonLib::HiliteMenu", {1, 0, false}},
        {"CarbonLib::GetMenuHandle", {1, 0, false}},
        {"CarbonLib::GetMenu", {1, 0, false}},
        {"CarbonLib::InvalMenuBar", {0, 0, false}},
        {"CarbonLib::EnableMenuItem", {2, 0, false}},
        {"CarbonLib::DisableMenuItem", {2, 0, false}},
        {"CarbonLib::GetMenuItemText", {3, 0, false}},
        {"CarbonLib::SetMenuItemText", {3, 0, false}},
        {"CarbonLib::CountMenuItems", {1, 0, false}},
        {"CarbonLib::DeleteMenuItem", {2, 0, false}},
        {"CarbonLib::ModalDialog", {2, 0, false}},
        {"CarbonLib::GetNewDialog", {3, 0, false}},
        {"CarbonLib::DisposeDialog", {1, 0, false}},
        {"CarbonLib::GetDialogWindow", {1, 0, false}},
        {"CarbonLib::NewHandle", {1, 0, false}},
        {"CarbonLib::DisposeHandle", {1, 0, false}},
        {"CarbonLib::GetHandleSize", {1, 0, false}},
        {"CarbonLib::HLock", {1, 0, false}},
        {"CarbonLib::FreeMem", {0, 0, false}},
        {"CarbonLib::UppercaseText", {3, 0, false}},
        {"CarbonLib::EqualString", {4, 0, false}},
        {"CarbonLib::GetResource", {2, 0, false}},
        {"CarbonLib::Get1Resource", {2, 0, false}},
        {"CarbonLib::Get1IndResource", {2, 0, false}},
        {"CarbonLib::ReleaseResource", {1, 0, false}},
        {"CarbonLib::DetachResource", {1, 0, false}},
        {"CarbonLib::GetIndString", {3, 0, false}},
        {"CarbonLib::Count1Resources", {1, 0, false}},
        {"CarbonLib::UseResFile", {1, 0, false}},
        {"CarbonLib::CurResFile", {0, 0, false}},
        {"CarbonLib::SetResLoad", {1, 0, false}},
        {"CarbonLib::SndDoCommand", {3, 0, false}},
        {"CarbonLib::SndDoImmediate", {2, 0, false}},
        {"CarbonLib::PBCloseSync", {1, 0, false}},
        {"CarbonLib::PBGetCatInfoSync", {1, 0, false}},
        {"CarbonLib::PBGetEOFSync", {1, 0, false}},
        {"CarbonLib::PBHCreateSync", {1, 0, false}},
        {"CarbonLib::PBHDeleteSync", {1, 0, false}},
        {"CarbonLib::PBHGetFInfoSync", {1, 0, false}},
        {"CarbonLib::PBHGetVInfoSync", {1, 0, false}},
        {"CarbonLib::PBHGetVolParmsSync", {1, 0, false}},
        {"CarbonLib::PBHOpenDFSync", {1, 0, false}},
        {"CarbonLib::PBHRenameSync", {1, 0, false}},
        {"CarbonLib::PBReadSync", {1, 0, false}},
        {"CarbonLib::PBSetCatInfoSync", {1, 0, false}},
        {"CarbonLib::PBSetEOFSync", {1, 0, false}},
        {"CarbonLib::PBSetFPosSync", {1, 0, false}},
        {"CarbonLib::PBWriteSync", {1, 0, false}},
        {"CarbonLib::PBSetForkSizeSync", {1, 0, false}},
        // The Toolbox calls MAIN.CPP makes, by their Carbon signatures (a
        // Point passed by value takes one register).
        {"CarbonLib::AEGetParamDesc", {4, 0, false}}, {"CarbonLib::AEInstallEventHandler", {5, 0, false}},
        {"CarbonLib::AEProcessAppleEvent", {1, 0, false}}, {"CarbonLib::AppendResMenu", {2, 0, false}},
        {"CarbonLib::BlockMove", {3, 0, false}}, {"CarbonLib::DisposeSpeechChannel", {1, 0, false}},
        {"CarbonLib::FSpCreateResFile", {4, 0, false}}, {"CarbonLib::FSpDirCreate", {3, 0, false}}, {"CarbonLib::FSpOpenResFile", {2, 0, false}},
        {"CarbonLib::FindFolder", {5, 0, false}}, {"CarbonLib::Gestalt", {2, 0, false}}, {"CarbonLib::GetCurrentProcess", {1, 0, false}},
        {"CarbonLib::GetEventClass", {1, 0, false}}, {"CarbonLib::GetEventKind", {1, 0, false}}, {"CarbonLib::GetEventParameter", {7, 0, false}},
        {"CarbonLib::GetIndMenuItemWithCommandID", {5, 0, false}}, {"CarbonLib::GetNewMBar", {1, 0, false}},
        {"CarbonLib::GetProcessInformation", {2, 0, false}}, {"CarbonLib::GetWindowEventTarget", {1, 0, false}},
        {"CarbonLib::InstallEventHandler", {6, 0, false}}, {"CarbonLib::MoreMasters", {0, 0, false}},
        {"CarbonLib::NewAEEventHandlerUPP", {1, 0, false}}, {"CarbonLib::NewEventHandlerUPP", {1, 0, false}},
        {"CarbonLib::NewSpeechChannel", {2, 0, false}}, {"CarbonLib::SetMenuBar", {1, 0, false}}, {"CarbonLib::SpeakText", {3, 0, false}},
        {"CarbonLib::SpeechBusy", {0, 0, false}}};
    const auto found = known.find(name);
    return found == known.end() ? ImportContract{8, 13, false} : found->second;
}
bool pure_import(std::string_view name) { return import_contract(name).pure; }
// What a callee can observe at entry: its arguments, the stack, TOC and
// return address, FPSCR control bits, and the registers its caller keeps
// across the call (r13-r31, f14-f31, cr2-cr4). The replay restores every
// register from the transcript when an import returns, so a preserved
// register must be compared here or a difference in it would be erased.
// The remaining volatile registers are scratch the callee never reads; the
// single-rounding multiply-add and rewritten routines can leave them
// different (game-observable equivalence, decided 2026-10-20).
bool observed_by_import(std::size_t byte, ImportContract arguments) {
    if (byte < 128) {
        const auto r = byte / 4;
        if (r == 0 || r == 11 || r == 12) return false;
        return r < 3 || r > 10 || r - 3 < arguments.integer;
    }
    if (byte >= 16384 && byte < 16384 + 32 * 16) {
        const auto f = (byte - 16384) / 16;
        if ((byte - 16384) % 16 >= 8) return false; // VSX half; the PowerPC has none.
        if (f == 0) return false;
        return f > 13 || f - 1 < arguments.floating;
    }
    if (byte >= 1024 && byte < 1030) return false;  // XER bits
    if (byte >= 1280 && byte < 1304) return false;  // FPSCR status bits
    if (byte >= 2304 && byte < 2312) { const auto field = byte - 2304; return field >= 2 && field <= 4; }
    if (byte >= 2432 && byte < 2444) return false;  // Ghidra's tea and r2Save: P-code scratch, not the processor's
    if (byte >= 4100 && byte < 4104) return false;  // XER
    if (byte >= 4132 && byte < 4136) return false;  // CTR
    return true;
}
std::string register_name(std::size_t byte) {
    if (byte < 128) return "r" + std::to_string(byte / 4);
    if (byte >= 16384 && byte < 16384 + 32 * 16) return "f" + std::to_string((byte - 16384) / 16);
    if (byte >= 2304 && byte < 2312) return "cr" + std::to_string(byte - 2304);
    if (byte >= 1280 && byte < 1312) return "FPSCR bit " + std::to_string(byte - 1280);
    if (byte >= 4128 && byte < 4132) return "LR";
    return "register byte " + std::to_string(byte);
}
// An import's or boundary's recorded input; its registers are read into
// record_registers and checked before the next record is read.
struct Input {
    std::uint32_t pc;
    std::uint64_t executed, calls;
};
Input take_input(Stream& stream) {
    Input input;
    input.pc = stream.take<std::uint32_t>();
    input.executed = stream.take<std::uint64_t>(); input.calls = stream.take<std::uint64_t>();
    if (transcript_version == 2) take_registers(stream, record_registers);
    else record_registers = stream.take<Registers>();
    return input;
}
// Season replays compare every register. An operation replay compares what
// the import observes; a null import is a day boundary inside a routine,
// where every register can be live.
void check_input(const Input& input, const State& s, const char* import = nullptr) {
    if (input.pc != s.pc) throw std::runtime_error("First divergence: native import PC at " + hex_address(s.pc));
    const bool masked = live_pure_imports && import;
    const auto contract = masked ? import_contract(import) : ImportContract{};
    if (relaxed_import_inputs) {
        const auto mix = [](std::uint8_t byte) { relaxed_input_digest = (relaxed_input_digest ^ byte) * 0x100000001b3; };
        for (unsigned shift = 0; shift < 32; shift += 8) mix(std::uint8_t(s.pc >> shift));
        for (std::size_t byte = 0; byte < s.registers.size(); ++byte)
            if (!masked || observed_by_import(byte, contract)) mix(s.registers[byte]);
        return;
    }
    // An operation replay runs pure imports itself, so its call count differs.
    if (!live_pure_imports && ((!semantic_calendar_replay && input.executed != s.executed) || input.calls != s.native_calls))
        throw std::runtime_error("First divergence: execution or native-call order at " + hex_address(s.pc));
    const auto& registers = record_registers;
    if (registers == s.registers) return;
    for (std::size_t byte = 0; byte < s.registers.size(); ++byte) {
        if (registers[byte] == s.registers[byte] || (masked && !observed_by_import(byte, contract))) continue;
        throw std::runtime_error("First divergence: native input register byte " + std::to_string(byte) +
            " (" + register_name(byte) + ") at " + hex_address(s.pc) + (import ? std::string(" calling ") + import : ""));
    }
}
void check_input(Stream& stream, const State& s) { check_input(take_input(stream), s); }
struct Recorder final : MemoryObserver {
    State* owner;
    std::filesystem::path directory;
    Stream stream;
    std::array<std::uint16_t, 3> initial_date;
    unsigned days, completed = 0, native_depth = 0;
    std::uint64_t rng_writes = 0;
    std::array<std::uint32_t, 2> rng_fields;
    std::vector<unsigned> callback_depths;
    bool finished = false;
    Registers base{};
    std::vector<HeapChange> heap_log;
    // A session capture (between two automation requests) watches memory
    // from the date_observer slot, so the app's drawing observer keeps
    // memory.observer, which the drawing code requires.
    bool session = false;
    Recorder(State& s, const std::filesystem::path& path, unsigned count, bool session_capture = false)
        : owner(&s), directory(path), stream(path / "imports.gz", true), initial_date(date(s)), days(count),
          rng_fields{pointer(s, 0x108b3f00), pointer(s, 0x108b3f0c)}, session(session_capture) {
        stream.put(magic_v2);
        save_runtime_snapshot(s, path / "initial.snapshot");
        std::ofstream report(path / "capture.txt");
        report << "format=CM DAY 002\nstatus=recording\ndays=" << days << '\n' << (session ? "session=1\n" : "")
               << season_state_summary(s) << '\n';
        if (!report) throw std::runtime_error("Cannot write private season capture metadata");
    }
    void mapped(std::uint32_t start, std::uint64_t size) override {
        if (native_depth) { stream.put(Event::map); stream.put(start); stream.put(size); }
    }
    void written(std::uint32_t start, const std::uint8_t* bytes, std::size_t size) override {
        if (!size) return;
        const auto field = std::find(rng_fields.begin(), rng_fields.end(), start);
        if (!native_depth && field == rng_fields.end()) return;
        if (!native_depth) {
            // Version 2 RNG write: which of the two state words, width, bytes.
            if (size > 8) throw std::runtime_error("Invalid original RNG state width");
            ++rng_writes;
            stream.put(Event::rng); stream.put(std::uint8_t(field - rng_fields.begin())); stream.put(std::uint8_t(size));
            stream.bytes(const_cast<std::uint8_t*>(bytes), size);
            return;
        }
        stream.put(Event::write);
        stream.put(start); stream.put(std::uint64_t(size));
        stream.bytes(const_cast<std::uint8_t*>(bytes), size);
    }
    void cleared(std::uint32_t start, std::uint64_t size) override {
        if (!size) return;
        if (native_depth) { stream.put(Event::clear); stream.put(start); stream.put(size); }
    }
};
std::unique_ptr<Recorder> recorder;
// Session captures: requested by the automation channel, started at the end
// of the next import and finished at the beginning of the next OS import
// after the stop request, so a replay ends where its transcript does.
std::optional<std::filesystem::path> session_start;
bool session_stop = false;
bool session_replay = false;
std::unique_ptr<Stream> replay;
unsigned replay_depth = 0;
std::optional<Event> next_event(std::optional<Input>& input);
struct ReplayObserver final : MemoryObserver {
    std::array<std::uint32_t, 2> rng_fields;
    std::uint64_t rng_writes = 0;
    explicit ReplayObserver(const State& s) : rng_fields{pointer(s, 0x108b3f00), pointer(s, 0x108b3f0c)} {
        // Only the two RNG state words matter; other writes skip the callback.
        restrict_write_notifications(std::min(rng_fields[0], rng_fields[1]),
                                     std::uint64_t(std::max(rng_fields[0], rng_fields[1])) + 8);
    }
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t start, const std::uint8_t* bytes, std::size_t size) override {
        if (std::find(rng_fields.begin(), rng_fields.end(), start) == rng_fields.end()) return;
        std::optional<Input> unexpected;
        if (next_event(unexpected) != Event::rng)
            throw std::runtime_error("First divergence: original RNG consumption order");
        if (transcript_version == 2) {
            const auto field = replay->take<std::uint8_t>();
            if (field > 1 || rng_fields[field] != start || replay->take<std::uint8_t>() != size)
                throw std::runtime_error("First divergence: original RNG consumption order");
        } else if (replay->take<std::uint32_t>() != start || replay->take<std::uint64_t>() != size)
            throw std::runtime_error("First divergence: original RNG consumption order");
        if (size > 8) throw std::runtime_error("Invalid original RNG state width");
        std::array<std::uint8_t, 8> expected{}; replay->bytes(expected.data(), size);
        if (!std::equal(expected.begin(), expected.begin() + size, bytes))
            throw std::runtime_error("First divergence: original RNG state write " + std::to_string(rng_writes) + " at " + hex_address(start));
        ++rng_writes;
    }
};
std::unique_ptr<ReplayObserver> replay_observer;
std::optional<std::array<std::uint16_t, 3>> replay_previous_date;
bool enabled() {
    static const bool value = std::getenv("CM_AOT_SEASON_REFERENCE") != nullptr;
    return value;
}
std::string byte_difference(const std::string& field, std::uint64_t address, unsigned expected, unsigned actual) {
    return "First divergence: " + field + " at " + hex_address(std::uint32_t(address)) +
           " expected=" + std::to_string(expected) + " actual=" + std::to_string(actual);
}
}
void season_day_boundary(State& s, std::uint32_t pc) {
    if (session_replay || (recorder && recorder->session)) return;
    if (!replay && !enabled()) return;
    if (pc != day_pc) throw std::runtime_error("Invalid season reference boundary");
    if (date(s)[2] != 0) return; // Three original phases; capture morning to morning.
    s.pc = pc;
    if (replay) {
        if (replay_depth) return;
        if (!replay_observer && rng_checks) replay_observer = std::make_unique<ReplayObserver>(s);
        s.memory.observer = replay_observer.get();
        const auto now = date(s);
        if (replay_previous_date && now == *replay_previous_date) return;
        replay_previous_date = now;
        std::optional<Input> unexpected;
        if (next_event(unexpected) != Event::boundary) throw std::runtime_error("First divergence: missing day boundary");
        check_input(*replay, s);
        const auto final = replay->take<std::uint8_t>();
        if (final > 1) throw std::runtime_error("Invalid private day boundary flag");
        if (final) { s.memory.observer = nullptr; replay->require_eof(); throw SeasonReplayComplete{}; }
        return;
    }
    if (!recorder) {
        const std::filesystem::path directory(std::getenv("CM_AOT_SEASON_REFERENCE"));
        if (!directory.is_absolute() || !std::filesystem::is_directory(directory) ||
            !std::filesystem::is_empty(directory))
            throw std::runtime_error("Season reference requires a new empty absolute private directory");
        unsigned days = 1;
        if (const auto* value = std::getenv("CM_AOT_SEASON_REFERENCE_DAYS")) {
            const std::string input(value);
            if (input.size() != 1 || input[0] < '1' || input[0] > '7')
                throw std::runtime_error("Season reference interval must be 1 through 7 full days");
            days = unsigned(input[0] - '0');
        }
        recorder = std::make_unique<Recorder>(s, directory, days);
        s.memory.observer = recorder.get();
        recorder->stream.put(Event::boundary); put_input(recorder->stream, recorder->base, s); recorder->stream.put(std::uint8_t(0));
    } else if (!recorder->finished && recorder->owner == &s && !recorder->native_depth) {
        // Count observed calendar days, not repeated visits to a UI loop.
        const auto now = date(s);
        if (now[0] == recorder->initial_date[0] && now[1] == recorder->initial_date[1]) return;
        const auto previous = recorder->initial_date;
        if (!((now[1] == previous[1] && now[0] == previous[0] + 1) ||
              (now[1] == previous[1] + 1 && now[0] == 0 && (previous[0] == 364 || previous[0] == 365))))
            throw std::runtime_error("Season fixture skipped a complete calendar-day boundary");
        recorder->initial_date = now;
        ++recorder->completed;
        const bool final = recorder->completed == recorder->days;
        recorder->stream.put(Event::boundary); put_input(recorder->stream, recorder->base, s); recorder->stream.put(std::uint8_t(final));
        if (final) {
            s.memory.observer = nullptr;
            save_runtime_snapshot(s, recorder->directory / "expected.snapshot");
            recorder->stream.finish(); recorder->finished = true;
            std::ofstream report(recorder->directory / "capture.txt", std::ios::app);
            report << "status=complete\ncompleted-days=" << recorder->completed << "\nrng-state-writes=" << recorder->rng_writes << '\n' << season_state_summary(s) << '\n';
            if (!report) throw std::runtime_error("Cannot finalize private season capture metadata");
        }
    }
}
namespace {
// A session capture that cannot go on (a full disk, a request inside a modal
// callback) is dropped with a message; the game itself continues.
void abandon_session(State& s, const char* why) {
    std::fprintf(stderr, "Session capture abandoned: %s\n", why);
    if (s.memory.date_observer == recorder.get()) s.memory.date_observer = nullptr;
    recorder.reset();
    session_stop = false;
    heap_changes = nullptr;
}
}
void season_record_import_begin(State& s) {
    if (!recorder || recorder->finished || recorder->owner != &s) return;
    if (recorder->session && !recorder->native_depth && recorder->callback_depths.empty()) {
        if (const char* name = import_name(s.pc);
            name && !pure_import(name) && (session_stop || std::string_view(name) == "CarbonLib::ExitToShell")) {
            // The replay stops when it reaches this call with its transcript at an end.
            try {
                recorder->stream.finish(); recorder->finished = true;
                s.memory.date_observer = nullptr;
                save_runtime_snapshot(s, recorder->directory / "expected.snapshot");
                std::ofstream report(recorder->directory / "capture.txt", std::ios::app);
                report << "status=complete\nscope=session ending before " << name << '\n' << season_state_summary(s) << '\n';
                if (!report) throw std::runtime_error("Cannot finalize private session capture metadata");
                recorder.reset();
                session_stop = false;
            } catch (const std::exception& error) { abandon_session(s, error.what()); }
            return;
        }
    }
    recorder->stream.put(Event::import); put_input(recorder->stream, recorder->base, s);
    ++recorder->native_depth;
    if (!recorder->session) s.memory.observer = recorder.get();
    if (recorder->native_depth == 1 && !recorder->heap_log.empty())
        throw std::runtime_error("Season recorder lost the end of an earlier import");
    heap_changes = &recorder->heap_log; heap_changes_owner = &s;
}
void season_record_import_end(State& s, const char* name) {
    // A session starts where the automation request arrived: at the end of
    // the main loop's WaitNextEvent, outside any import or callback.
    if (session_start && (!recorder || recorder->finished) && !replay && name && std::string_view(name) == "CarbonLib::WaitNextEvent") {
        const auto directory = *session_start;
        session_start.reset();
        if (s.memory.date_observer) { std::fprintf(stderr, "Session capture refused: date diagnostics are attached\n"); return; }
        try { recorder = std::make_unique<Recorder>(s, directory, 0, true); }
        catch (const std::exception& error) { abandon_session(s, error.what()); return; }
        s.memory.date_observer = recorder.get();
        return;
    }
    if (!recorder || recorder->finished || recorder->owner != &s) return;
    put_chunk(recorder->stream, recorder->base, s, recorder->heap_log); recorder->stream.put(Event::import_end);
    if (!recorder->native_depth && recorder->session) { abandon_session(s, "it started inside an import"); return; }
    if (!recorder->native_depth) throw std::runtime_error("Invalid season import nesting");
    --recorder->native_depth;
    heap_changes = nullptr;
}
void season_record_callback_begin(State& s, std::uint32_t vector, std::span<const std::uint32_t> arguments) {
    if (!recorder || recorder->finished || recorder->owner != &s) return;
    recorder->callback_depths.push_back(recorder->native_depth);
    if (!recorder->native_depth) return;
    put_chunk(recorder->stream, recorder->base, s, recorder->heap_log);
    heap_changes = nullptr;  // the callback's own imports record their changes
    recorder->stream.put(Event::callback); recorder->stream.put(vector);
    recorder->stream.put(std::uint8_t(arguments.size()));
    for (const auto argument : arguments) recorder->stream.put(argument);
    recorder->native_depth = 0;
}
void season_record_callback_end(State& s, std::uint32_t result) {
    if (!recorder || recorder->finished || recorder->owner != &s) return;
    if (recorder->callback_depths.empty() && recorder->session) { abandon_session(s, "it started inside a callback"); return; }
    if (recorder->callback_depths.empty()) throw std::runtime_error("Invalid season callback nesting");
    const auto depth = recorder->callback_depths.back(); recorder->callback_depths.pop_back();
    if (!depth) return;
    recorder->stream.put(Event::callback_end); recorder->stream.put(result);
    recorder->native_depth = depth;
    if (!recorder->session) s.memory.observer = recorder.get();
    heap_changes = &recorder->heap_log; heap_changes_owner = &s;
}
void open_season_replay(const std::filesystem::path& directory, bool semantic_calendar) {
    if (replay) throw std::runtime_error("A private season replay is already active");
    semantic_calendar_replay = semantic_calendar;
    live_pure_imports = false;
    replay = std::make_unique<Stream>(directory / "imports.gz", false);
    const auto header = replay->take<std::array<char, 8>>();
    if (header != magic && header != magic_v2) { replay.reset(); throw std::runtime_error("Unrecognized private season transcript"); }
    transcript_version = header == magic_v2 ? 2 : 1;
    record_registers = {};
    replay_depth = 0;
    replay_previous_date.reset();
}
void open_operation_replay(const std::filesystem::path& directory) {
    open_season_replay(directory);
    live_pure_imports = true;
}
void seed_season_replay(const State& initial) {
    if (replay && transcript_version == 2 && live_pure_imports && rng_checks) recorded_allocator = Allocator(initial);
}
void open_timing_replay(const std::filesystem::path& directory) {
    open_operation_replay(directory);
    rng_checks = false;
}
std::string season_session_request(const std::optional<std::filesystem::path>& directory) {
    if (directory) {
        if ((recorder && !recorder->finished) || session_start) throw std::runtime_error("a capture is already recording");
        if (!directory->is_absolute()) throw std::runtime_error("a session capture needs an absolute directory");
        std::filesystem::create_directories(*directory);
        if (!std::filesystem::is_empty(*directory)) throw std::runtime_error("the session capture directory is not empty");
        session_start = *directory;
        return "recording from the end of this event call into " + directory->string();
    }
    if (!recorder || !recorder->session) throw std::runtime_error("no session capture is recording");
    session_stop = true;
    return "stopping at the next OS import";
}
unsigned import_integer_arguments(std::string_view name) { return import_contract(name).integer; }
void season_launch_capture(State& s, const std::filesystem::path& directory) {
    // Like a session start, a capture that cannot be taken is refused with a
    // message; the game itself starts.
    try {
        if (recorder || session_start) throw std::runtime_error("a capture is already recording");
        if (s.memory.date_observer) throw std::runtime_error("date diagnostics are attached");
        if (!directory.is_absolute()) throw std::runtime_error("a launch capture needs an absolute directory");
        std::filesystem::create_directories(directory);
        if (!std::filesystem::is_empty(directory)) throw std::runtime_error("the launch capture directory is not empty");
        recorder = std::make_unique<Recorder>(s, directory, 0, true);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Launch capture refused: %s\n", error.what());
        recorder.reset();
        return;
    }
    s.memory.date_observer = recorder.get();
}
void open_session_replay(const std::filesystem::path& directory, bool timing) {
    if (timing) open_timing_replay(directory); else open_operation_replay(directory);
    session_replay = true;
}
void begin_session_replay(State& s) {
    // Sessions have no day boundary, where season replays attach the RNG
    // check: attach it before the first instruction.
    if (!session_replay || !rng_checks) return;
    replay_observer = std::make_unique<ReplayObserver>(s);
    s.memory.observer = replay_observer.get();
}
void season_replay_after_fork() {
    if (replay) replay->detach_after_fork();
}
void season_replay_relax_import_inputs() { relaxed_import_inputs = true; }
std::uint64_t season_replay_import_digest() { return relaxed_input_digest; }
void close_season_replay() {
    session_replay = false;
    semantic_calendar_replay = live_pure_imports = relaxed_import_inputs = false; rng_checks = true; recorded_allocator.reset();
    replay.reset(); replay_observer.reset(); replay_depth = 0; replay_previous_date.reset();
}
namespace {
void skip_recorded_import() {
    for (;;) {
        const auto event = replay->take<Event>();
        if (event == Event::import_end) return;
        if (event == Event::chunk) {
            if (rng_checks) skip_chunk(*replay);
            else skip_unchecked_chunk(*replay);
        }
        else if (event == Event::map || event == Event::write || event == Event::clear) {
            replay->take<std::uint32_t>();
            const auto size = replay->take<std::uint64_t>();
            if (event == Event::write) skip(*replay, size);
        } else throw std::runtime_error("Invalid private season transcript event in a pure import");
    }
}
// The next recorded event this replay does not compute itself. A live replay
// skips recorded pure imports, reading an OS import's input ahead into
// `input`, and has no event at the end of its transcript.
std::optional<Event> next_event(std::optional<Input>& input) {
    for (;;) {
        if (live_pure_imports && replay->at_end()) return std::nullopt;
        const auto event = replay->take<Event>();
        if (!rng_checks && event == Event::rng) { skip_rng_write(*replay); continue; }
        if (!live_pure_imports || event != Event::import) return event;
        input = take_input(*replay);
        const char* name = import_name(input->pc);
        if (!name || !pure_import(name)) return event;
        skip_recorded_import();
        input.reset();
    }
}
std::optional<Input> next_recorded_import() {
    std::optional<Input> input;
    const auto event = next_event(input);
    if (!event) return std::nullopt;
    if (*event != Event::import) throw std::runtime_error("First divergence: native import order");
    return input ? input : take_input(*replay);
}
}
void require_season_replay_eof() {
    if (!replay) throw std::runtime_error("No private season replay is open");
    if (live_pure_imports)
        if (const auto input = next_recorded_import())
            throw std::runtime_error("First divergence: recorded native import at " + hex_address(input->pc) + " was not called");
    replay->require_eof();
}
bool season_replay_import(State& s, const char* name) {
    if (!replay) return false;
    if (live_pure_imports && pure_import(name)) return false;
    s.memory.observer = nullptr;
    const auto input = next_recorded_import();
    if (!input && session_replay) { s.memory.observer = nullptr; throw SeasonReplayComplete{}; }
    if (!input) throw std::runtime_error(std::string("First divergence: ") + name + " called after the transcript ends");
    check_input(*input, s, name);
    if (live_pure_imports && rng_checks && recorded_allocator && !recorded_allocator->matches(s))
        throw std::runtime_error(std::string("First divergence: heap state calling ") + name + ": " +
                                 heap_difference(*recorded_allocator, Allocator(s)));
    for (;;) {
        const auto event = replay->take<Event>();
        if (event == Event::chunk) {
            if (transcript_version == 2) {
                const auto chunk = take_chunk_v2(*replay);
                apply_chunk(s, chunk);
                if (live_pure_imports && rng_checks && recorded_allocator) {
                    apply_heap_changes(recorded_allocator->allocated, recorded_allocator->available, chunk.changes);
                    recorded_allocator->heap_end = chunk.heap_end; recorded_allocator->memory_error = chunk.memory_error;
                }
            } else {
                take_chunk(*replay, s);
                if (rng_checks) recorded_allocator = Allocator(s);
            }
        } else if (event == Event::map || event == Event::write || event == Event::clear) {
            const auto address = replay->take<std::uint32_t>();
            const auto size = replay->take<std::uint64_t>();
            if (!size || size > (UINT64_C(1) << 32) - address)
                throw std::runtime_error("Invalid private season memory operation");
            if (event == Event::map) s.memory.map(address, size);
            else if (event == Event::clear) s.memory.clear(address, size);
            else {
                if (size > 0x10000000) throw std::runtime_error("Private season write exceeds allocation limit");
                std::vector<std::uint8_t> bytes(size); replay->bytes(bytes.data(), bytes.size());
                s.memory.copy_in(address, bytes);
            }
        } else if (event == Event::callback) {
            const auto vector = replay->take<std::uint32_t>();
            const auto count = replay->take<std::uint8_t>();
            if (count > 8) throw std::runtime_error("Invalid private season callback argument count");
            std::vector<std::uint32_t> arguments;
            for (unsigned i = 0; i < count; ++i) arguments.push_back(replay->take<std::uint32_t>());
            ++replay_depth;
            s.memory.observer = replay_observer.get();
            const auto result = call_original(s, vector, arguments);
            --replay_depth;
            s.memory.observer = nullptr;
            // A timing replay has no observer: skip RNG writes the callback made.
            auto event = replay->take<Event>();
            while (!rng_checks && event == Event::rng) { skip_rng_write(*replay); event = replay->take<Event>(); }
            if (event != Event::callback_end || replay->take<std::uint32_t>() != result)
                throw std::runtime_error("First divergence: original native callback result");
        } else if (event == Event::import_end) { s.memory.observer = replay_observer.get(); return true; }
        else throw std::runtime_error("Invalid private season transcript event");
    }
}
std::string season_state_summary(const State& s) {
    const auto d = date(s);
    std::ostringstream out;
    out << "game.date.year=" << d[1] << " game.date.day-of-year=" << d[0] << " game.phase=" << d[2];
    for (const auto& [name, global] : std::vector<std::pair<const char*, std::uint32_t>>{
        {"rng.seed", 0x108b3f00}, {"rng.cursor", 0x108b3f0c}, {"rng.end", 0x108b3f08}})
        out << ' ' << name << '=' << hex_address(s.memory.read<4>(pointer(s, global)));
    out << " instructions=" << s.executed << " native-calls=" << s.native_calls;
    return out.str();
}
namespace {
enum class Comparison { strict, semantic_calendar, operation };
// load_program maps the guest stack here (tools/compile_recomp.py).
constexpr std::uint32_t stack_base = 0x70000000;
// A return address follows bl, bctrl or blrl with the link bit set, or is the
// sentinel a whole-routine run returns to.
bool ends_after_call(const State& s) {
    if (s.pc == return_sentinel) return true;
    try {
        const auto instruction = std::uint32_t(s.memory.read<4>(s.pc - 4));
        return (instruction & 0xfc000003) == 0x48000001 || instruction == 0x4e800421 || instruction == 0x4e800021;
    } catch (const std::exception&) { return false; }
}
std::string compare_states(const State& expected, const State& actual, Comparison mode) {
    try {
        for (const auto& [name, global, offset, size] : std::vector<std::tuple<const char*, std::uint32_t, unsigned, unsigned>>{
            {"game.date.day-of-year", date_global, 0, 2}, {"game.date.year", date_global, 2, 2},
            {"game.date.leap-year", date_global, 4, 1}, {"game.phase", phase_global, 0, 1},
            {"rng.seed", 0x108b3f00, 0, 4}, {"rng.cursor", 0x108b3f0c, 0, 4}, {"rng.end", 0x108b3f08, 0, 4}}) {
            const auto address = pointer(expected, global) + offset, other = pointer(actual, global) + offset;
            const auto a = expected.memory.copy_out(address, size), b = actual.memory.copy_out(other, size);
            if (a != b) { const auto pair = std::mismatch(a.begin(), a.end(), b.begin());
                const auto index = pair.first - a.begin(); return byte_difference(name, address + index, a[index], b[index]); }
        }
    } catch (const std::exception& error) { return "First divergence: named game field unavailable: " + std::string(error.what()); }
    if (expected.pc != actual.pc) return "First divergence: boundary PC";
    if ((mode == Comparison::strict && expected.executed != actual.executed) ||
        (mode != Comparison::operation && expected.native_calls != actual.native_calls))
        return "First divergence: instruction or native-call consumption";
    if (expected.allocated != actual.allocated || expected.available != actual.available ||
        expected.heap_end != actual.heap_end || expected.memory_error != actual.memory_error)
        return "First divergence: allocator state";
    // An operation that ends at a return address leaves its caller only r3,
    // r4, f1 and the preserved registers; its scratch registers, generated
    // temporaries and the stack below SP are dead. A mid-routine end is
    // compared whole.
    const bool returned = mode == Comparison::operation && ends_after_call(expected);
    for (std::size_t byte = 0; byte < expected.registers.size(); ++byte)
        if (expected.registers[byte] != actual.registers[byte] && (!returned || observed_by_import(byte, {2, 1, false})))
            return byte_difference("architectural register byte (" + register_name(byte) + ")", byte,
                                   expected.registers[byte], actual.registers[byte]);
    if (const auto pair = std::mismatch(expected.unique.begin(), expected.unique.end(), actual.unique.begin());
        !returned && pair.first != expected.unique.end()) {
        const auto index = pair.first - expected.unique.begin();
        return byte_difference("generated temporary byte", index, expected.unique[index], actual.unique[index]);
    }
    const auto live = [&](std::uint32_t address) {
        return !returned || address < stack_base || address >= std::uint32_t(expected.r<4>(4));
    };
    const auto ranges = expected.memory.mapped_ranges();
    if (ranges != actual.memory.mapped_ranges()) return "First divergence: mapped memory ranges";
    for (const auto& [start, size] : ranges) {
        for (std::uint64_t offset = 0; offset < size; offset += 65536) {
            const auto length = std::size_t(std::min<std::uint64_t>(size - offset, 65536));
            const auto address = start + std::uint32_t(offset);
            const auto a = expected.memory.copy_out(address, length), b = actual.memory.copy_out(address, length);
            for (auto pair = std::mismatch(a.begin(), a.end(), b.begin()); pair.first != a.end();
                 pair = std::mismatch(pair.first + 1, a.end(), pair.second + 1)) {
                const auto index = pair.first - a.begin(); const auto at = address + std::uint32_t(index);
                if (!live(at)) continue;
                auto allocation = expected.allocated.upper_bound(at);
                std::string field = "mapped game/runtime memory";
                if (allocation != expected.allocated.begin()) {
                    --allocation;
                    if (at - allocation->first < allocation->second)
                        field = "allocation " + hex_address(allocation->first) + " offset " + std::to_string(at - allocation->first);
                }
                return byte_difference(field, at, a[index], b[index]);
            }
        }
    }
    return {};
}
}
std::string compare_season_states(const State& expected, const State& actual, bool semantic_calendar) {
    return compare_states(expected, actual, semantic_calendar ? Comparison::semantic_calendar : Comparison::strict);
}
std::string compare_operation_states(const State& expected, const State& actual) {
    return compare_states(expected, actual, Comparison::operation);
}
}

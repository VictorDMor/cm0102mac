#include "season_reference.hpp"
#include "match_replay.hpp"
#include "callbacks.hpp"
#include <zlib.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <optional>

namespace cm::aot {
namespace {
bool semantic_calendar_replay = false;
constexpr std::uint32_t day_pc = 0x101195e8;
constexpr std::uint32_t date_global = 0x108b3f10, phase_global = 0x108b40a4;
constexpr std::array<char, 8> magic{'C','M','D','A','Y','0','0','1'};
enum class Event : std::uint8_t { import, write, clear, map, chunk, callback, callback_end, import_end, boundary, rng };
struct Stream {
    gzFile file = nullptr;
    bool writing;
    explicit Stream(const std::filesystem::path& path, bool output) : writing(output) {
        file = gzopen(path.c_str(), output ? "wb1" : "rb");
        if (!file) throw std::runtime_error("Cannot open private season transcript");
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
void put_table(Stream& stream, const std::map<std::uint32_t, std::uint32_t>& table) {
    stream.put(std::uint32_t(table.size()));
    for (const auto& [address, size] : table) { stream.put(address); stream.put(size); }
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
void put_chunk(Stream& stream, const State& s) {
    stream.put(Event::chunk); stream.put(s.pc); stream.put(s.registers); stream.put(s.unique);
    stream.put(s.heap_end); stream.put(s.memory_error);
    put_table(stream, s.allocated); put_table(stream, s.available);
}
void take_chunk(Stream& stream, State& s) {
    s.pc = stream.take<std::uint32_t>();
    s.registers = stream.take<decltype(s.registers)>();
    s.unique = stream.take<decltype(s.unique)>();
    s.heap_end = stream.take<std::uint32_t>(); s.memory_error = stream.take<std::int16_t>();
    take_table(stream, s.allocated); take_table(stream, s.available);
}
void put_input(Stream& stream, const State& s) {
    stream.put(s.pc); stream.put(s.executed); stream.put(s.native_calls); stream.put(s.registers);
}
void check_input(Stream& stream, const State& s) {
    const auto pc = stream.take<std::uint32_t>();
    const auto executed = stream.take<std::uint64_t>(), calls = stream.take<std::uint64_t>();
    const auto registers = stream.take<decltype(s.registers)>();
    if (pc != s.pc) throw std::runtime_error("First divergence: native import PC at " + hex_address(s.pc));
    if ((!semantic_calendar_replay && executed != s.executed) || calls != s.native_calls)
        throw std::runtime_error("First divergence: execution or native-call order at " + hex_address(s.pc));
    if (registers != s.registers) {
        const auto pair = std::mismatch(registers.begin(), registers.end(), s.registers.begin());
        throw std::runtime_error("First divergence: native input register byte " +
            std::to_string(pair.first - registers.begin()) + " at " + hex_address(s.pc));
    }
}
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
    Recorder(State& s, const std::filesystem::path& path, unsigned count)
        : owner(&s), directory(path), stream(path / "imports.gz", true), initial_date(date(s)), days(count),
          rng_fields{pointer(s, 0x108b3f00), pointer(s, 0x108b3f0c)} {
        stream.put(magic);
        save_runtime_snapshot(s, path / "initial.snapshot");
        std::ofstream report(path / "capture.txt");
        report << "format=CM DAY 001\nstatus=recording\ndays=" << days << '\n' << season_state_summary(s) << '\n';
        if (!report) throw std::runtime_error("Cannot write private season capture metadata");
    }
    void mapped(std::uint32_t start, std::uint64_t size) override {
        if (native_depth) { stream.put(Event::map); stream.put(start); stream.put(size); }
    }
    void written(std::uint32_t start, const std::uint8_t* bytes, std::size_t size) override {
        if (!size) return;
        if (!native_depth && std::find(rng_fields.begin(), rng_fields.end(), start) == rng_fields.end()) return;
        stream.put(native_depth ? Event::write : Event::rng);
        if (!native_depth) ++rng_writes;
        stream.put(start); stream.put(std::uint64_t(size));
        stream.bytes(const_cast<std::uint8_t*>(bytes), size);
    }
    void cleared(std::uint32_t start, std::uint64_t size) override {
        if (!size) return;
        if (native_depth) { stream.put(Event::clear); stream.put(start); stream.put(size); }
    }
};
std::unique_ptr<Recorder> recorder;
std::unique_ptr<Stream> replay;
unsigned replay_depth = 0;
struct ReplayObserver final : MemoryObserver {
    std::array<std::uint32_t, 2> rng_fields;
    std::uint64_t rng_writes = 0;
    explicit ReplayObserver(const State& s) : rng_fields{pointer(s, 0x108b3f00), pointer(s, 0x108b3f0c)} {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t start, const std::uint8_t* bytes, std::size_t size) override {
        if (std::find(rng_fields.begin(), rng_fields.end(), start) == rng_fields.end()) return;
        if (replay->take<Event>() != Event::rng || replay->take<std::uint32_t>() != start ||
            replay->take<std::uint64_t>() != size)
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
    if (!replay && !enabled()) return;
    if (pc != day_pc) throw std::runtime_error("Invalid season reference boundary");
    if (date(s)[2] != 0) return; // Three original phases; capture morning to morning.
    s.pc = pc;
    if (replay) {
        if (replay_depth) return;
        if (!replay_observer) replay_observer = std::make_unique<ReplayObserver>(s);
        s.memory.observer = replay_observer.get();
        const auto now = date(s);
        if (replay_previous_date && now == *replay_previous_date) return;
        replay_previous_date = now;
        if (replay->take<Event>() != Event::boundary) throw std::runtime_error("First divergence: missing day boundary");
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
        recorder->stream.put(Event::boundary); put_input(recorder->stream, s); recorder->stream.put(std::uint8_t(0));
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
        recorder->stream.put(Event::boundary); put_input(recorder->stream, s); recorder->stream.put(std::uint8_t(final));
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
void season_record_import_begin(State& s) {
    if (!recorder || recorder->finished || recorder->owner != &s) return;
    recorder->stream.put(Event::import); put_input(recorder->stream, s);
    ++recorder->native_depth; s.memory.observer = recorder.get();
}
void season_record_import_end(State& s) {
    if (!recorder || recorder->finished || recorder->owner != &s) return;
    put_chunk(recorder->stream, s); recorder->stream.put(Event::import_end);
    if (!recorder->native_depth) throw std::runtime_error("Invalid season import nesting");
    --recorder->native_depth;
}
void season_record_callback_begin(State& s, std::uint32_t vector, std::span<const std::uint32_t> arguments) {
    if (!recorder || recorder->finished || recorder->owner != &s) return;
    recorder->callback_depths.push_back(recorder->native_depth);
    if (!recorder->native_depth) return;
    put_chunk(recorder->stream, s);
    recorder->stream.put(Event::callback); recorder->stream.put(vector);
    recorder->stream.put(std::uint8_t(arguments.size()));
    for (const auto argument : arguments) recorder->stream.put(argument);
    recorder->native_depth = 0;
}
void season_record_callback_end(State& s, std::uint32_t result) {
    if (!recorder || recorder->finished || recorder->owner != &s) return;
    if (recorder->callback_depths.empty()) throw std::runtime_error("Invalid season callback nesting");
    const auto depth = recorder->callback_depths.back(); recorder->callback_depths.pop_back();
    if (!depth) return;
    recorder->stream.put(Event::callback_end); recorder->stream.put(result);
    recorder->native_depth = depth; s.memory.observer = recorder.get();
}
void open_season_replay(const std::filesystem::path& directory, bool semantic_calendar) {
    if (replay) throw std::runtime_error("A private season replay is already active");
    semantic_calendar_replay = semantic_calendar;
    replay = std::make_unique<Stream>(directory / "imports.gz", false);
    if (replay->take<std::array<char, 8>>() != magic) { replay.reset(); throw std::runtime_error("Unrecognized private season transcript"); }
    replay_depth = 0;
    replay_previous_date.reset();
}
void close_season_replay() { semantic_calendar_replay = false; replay.reset(); replay_observer.reset(); replay_depth = 0; replay_previous_date.reset(); }
bool season_replay_import(State& s, const char* name) {
    if (!replay) return false;
    s.memory.observer = nullptr;
    (void)name; // Exact import PC is checked; names are fixed by the original program.
    if (replay->take<Event>() != Event::import) throw std::runtime_error("First divergence: native import order");
    check_input(*replay, s);
    for (;;) {
        const auto event = replay->take<Event>();
        if (event == Event::chunk) take_chunk(*replay, s);
        else if (event == Event::map || event == Event::write || event == Event::clear) {
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
            if (replay->take<Event>() != Event::callback_end || replay->take<std::uint32_t>() != result)
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
std::string compare_season_states(const State& expected, const State& actual, bool semantic_calendar) {
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
    if ((!semantic_calendar && expected.executed != actual.executed) || expected.native_calls != actual.native_calls)
        return "First divergence: instruction or native-call consumption";
    if (expected.allocated != actual.allocated || expected.available != actual.available ||
        expected.heap_end != actual.heap_end || expected.memory_error != actual.memory_error)
        return "First divergence: allocator state";
    for (const auto& [name, a, b] : std::vector<std::tuple<const char*, std::span<const std::uint8_t>, std::span<const std::uint8_t>>>{
        {"architectural register byte", expected.registers, actual.registers}, {"generated temporary byte", expected.unique, actual.unique}}) {
        const auto pair = std::mismatch(a.begin(), a.end(), b.begin());
        if (pair.first != a.end()) { const auto index = pair.first - a.begin(); return byte_difference(name, index, a[index], b[index]); }
    }
    const auto ranges = expected.memory.mapped_ranges();
    if (ranges != actual.memory.mapped_ranges()) return "First divergence: mapped memory ranges";
    for (const auto& [start, size] : ranges) {
        for (std::uint64_t offset = 0; offset < size; offset += 65536) {
            const auto length = std::size_t(std::min<std::uint64_t>(size - offset, 65536));
            const auto address = start + std::uint32_t(offset);
            const auto a = expected.memory.copy_out(address, length), b = actual.memory.copy_out(address, length);
            const auto pair = std::mismatch(a.begin(), a.end(), b.begin());
            if (pair.first != a.end()) {
                const auto index = pair.first - a.begin(); const auto at = address + std::uint32_t(index);
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

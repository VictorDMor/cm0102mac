#include "routine_check.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "season_reference.hpp"
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <bit>
#include <optional>
#include <cstdio>
#include <iomanip>
#include <cstring>
#include <iostream>
#include <map>
#include <fstream>
#include <set>
#include <sstream>
#include <vector>


namespace cm::aot {
RoutineChecker* routine_checker = nullptr;
bool (*dispatch_probe)(State&) = nullptr;
namespace {
bool in_child = false;
constexpr unsigned page_shift = 12;
constexpr std::uint32_t page_size = 1u << page_shift;

// Marks every 4 KiB page a run writes, clears or maps, and passes each event
// on to the observer it replaced.
struct DirtyPages final : MemoryObserver {
    MemoryObserver* previous;
    std::vector<std::uint64_t> bits = std::vector<std::uint64_t>((std::uint64_t(1) << (32 - page_shift)) / 64, 0);
    std::vector<std::pair<std::uint32_t, std::uint64_t>> maps;
    explicit DirtyPages(MemoryObserver* replaced) : previous(replaced) {}
    void mark(std::uint64_t start, std::uint64_t size) {
        if (!size) return;
        for (auto page = start >> page_shift; page <= (start + size - 1) >> page_shift; ++page)
            bits[page / 64] |= std::uint64_t(1) << (page % 64);
    }
    void mapped(std::uint32_t start, std::uint64_t size) override {
        maps.emplace_back(start, size); mark(start, size);
        if (previous) previous->mapped(start, size);
    }
    void written(std::uint32_t start, const std::uint8_t* bytes, std::size_t size) override {
        mark(start, size);
        if (previous && previous->observes_write(start, unsigned(size))) previous->written(start, bytes, size);
    }
    void cleared(std::uint32_t start, std::uint64_t size) override {
        mark(start, size);
        if (previous) previous->cleared(start, size);
    }
    void read(std::uint32_t start, const std::uint8_t* bytes, unsigned size) override {
        if (previous) previous->read(start, bytes, size);
    }
    std::vector<std::uint32_t> pages() const {
        std::vector<std::uint32_t> result;
        for (std::size_t word = 0; word < bits.size(); ++word)
            for (auto value = bits[word]; value; value &= value - 1)
                result.push_back(std::uint32_t(word * 64 + unsigned(std::countr_zero(value))));
        return result;
    }
};

// ended: the session capture ends inside the call (SeasonReplayComplete);
// both runs then stop at the same import, and their states there compare.
enum class Outcome : std::uint8_t { returned, transfer, exit, fault, ended };
struct Report {
    Outcome outcome = Outcome::fault;
    std::string message;
    std::uint32_t pc = 0;
    decltype(State::registers) registers{};
    std::uint32_t heap_end = 0;
    std::int16_t memory_error = 0;
    std::map<std::uint32_t, std::uint32_t> allocated, available;
    std::vector<std::pair<std::uint32_t, std::uint64_t>> maps;
    std::map<std::uint32_t, std::vector<std::uint8_t>> pages;  // page number -> contents
    std::uint64_t imports = 0;  // what the imports observed (synthetic calls)
};

class Writer {
public:
    explicit Writer(int fd) : fd_(fd) {}
    ~Writer() { flush(); }
    void bytes(const void* data, std::size_t size) {
        const auto* p = static_cast<const std::uint8_t*>(data);
        buffer_.insert(buffer_.end(), p, p + size);
        if (buffer_.size() > (1u << 20)) flush();
    }
    template <typename T> void put(const T& value) { bytes(&value, sizeof value); }
    void flush() {
        std::size_t done = 0;
        while (done < buffer_.size()) {
            const auto n = ::write(fd_, buffer_.data() + done, buffer_.size() - done);
            if (n <= 0) _exit(3);
            done += std::size_t(n);
        }
        buffer_.clear();
    }
private:
    int fd_;
    std::vector<std::uint8_t> buffer_;
};
class Reader {
public:
    explicit Reader(int fd) : fd_(fd) {}
    void bytes(void* data, std::size_t size) {
        auto* p = static_cast<std::uint8_t*>(data);
        while (size) {
            const auto n = ::read(fd_, p, size);
            if (n <= 0) throw std::runtime_error("Routine check child ended without a complete report");
            p += n; size -= std::size_t(n);
        }
    }
    template <typename T> T take() { T value{}; bytes(&value, sizeof value); return value; }
private:
    int fd_;
};

void put_table(Writer& w, const std::map<std::uint32_t, std::uint32_t>& table) {
    w.put(std::uint32_t(table.size()));
    for (const auto& [address, size] : table) { w.put(address); w.put(size); }
}
std::map<std::uint32_t, std::uint32_t> take_table(Reader& r) {
    std::map<std::uint32_t, std::uint32_t> table;
    for (auto count = r.take<std::uint32_t>(); count; --count) {
        const auto address = r.take<std::uint32_t>();
        table[address] = r.take<std::uint32_t>();
    }
    return table;
}

// The bytes of one page; parts outside every mapping read as zero.
std::vector<std::uint8_t> page_bytes(const Memory& memory, std::uint32_t page) {
    const auto start = std::uint64_t(page) << page_shift, end = start + page_size;
    try { return memory.copy_out(std::uint32_t(start), page_size); } catch (const std::runtime_error&) {}
    std::vector<std::uint8_t> bytes(page_size, 0);
    for (const auto& [low, size] : memory.mapped_ranges()) {
        const auto a = std::max<std::uint64_t>(low, start), b = std::min<std::uint64_t>(std::uint64_t(low) + size, end);
        if (a >= b) continue;
        const auto part = memory.copy_out(std::uint32_t(a), std::size_t(b - a));
        std::copy(part.begin(), part.end(), bytes.begin() + std::ptrdiff_t(a - start));
    }
    return bytes;
}

// Child: run, then report through `fd`, and leave without running any of the
// parent's exit handlers or flushing its buffers. Writes are tracked in the
// date_observer slot: the replay swaps memory.observer at every import.
[[noreturn]] void run_child(State& s, std::uint32_t entry, RoutineRunner runner, int fd, bool corrupt, bool synthetic) {
    in_child = true;
    Outcome outcome = Outcome::fault;
    std::string message;
    DirtyPages dirty(s.memory.date_observer);
    std::vector<std::uint32_t> pages;
    try {
        season_replay_after_fork();
        if (synthetic) season_replay_relax_import_inputs();
        s.memory.date_observer = &dirty;
        runner(s, entry);
        outcome = Outcome::returned;
    } catch (const GuestContextTransfer&) { outcome = Outcome::transfer; }
    catch (const SeasonReplayComplete&) { outcome = Outcome::ended; }
    catch (const ApplicationExit&) { outcome = Outcome::exit; }
    catch (const std::exception& e) { message = e.what(); }
    catch (...) { message = "unknown exception"; }
    s.memory.date_observer = dirty.previous;
    try {
        pages = dirty.pages();
        if (corrupt) {
            // Spoil the result without telling any observer: a byte the run
            // wrote, or r31 (always compared) when it wrote nothing.
            // A written page may be mapped only in part: flip its first mapped byte
            // from the middle on.
            std::optional<std::uint32_t> spoiled;
            for (const auto page : pages) {
                const auto start = std::uint64_t(page) << page_shift, middle = start + page_size / 2;
                for (const auto& [low, size] : s.memory.mapped_ranges()) {
                    const auto at = std::max<std::uint64_t>(low, middle);
                    if (at < std::min<std::uint64_t>(std::uint64_t(low) + size, start + page_size)) { spoiled = std::uint32_t(at); break; }
                }
                if (spoiled) break;
            }
            if (!spoiled) s.wr<4>(31 * 4, s.r<4>(31 * 4) ^ 0x5a5a5a5a);
            else {
                const auto saved = s.memory.observer; s.memory.observer = nullptr;
                s.memory.write<1>(*spoiled, s.memory.read<1>(*spoiled) ^ 0x5a);
                s.memory.observer = saved;
            }
        }
        Writer w(fd);
        w.put(outcome);
        w.put(std::uint32_t(message.size())); w.bytes(message.data(), message.size());
        w.put(s.pc);
        w.bytes(s.registers.data(), s.registers.size());
        w.put(s.heap_end); w.put(s.memory_error);
        put_table(w, s.allocated); put_table(w, s.available);
        w.put(std::uint32_t(dirty.maps.size()));
        for (const auto& [start, size] : dirty.maps) { w.put(start); w.put(size); }
        w.put(std::uint32_t(pages.size()));
        for (const auto page : pages) {
            w.put(page);
            const auto bytes = page_bytes(s.memory, page);
            w.bytes(bytes.data(), bytes.size());
        }
        w.put(season_replay_import_digest());
    } catch (...) { _exit(4); }
    ::close(fd);
    _exit(0);
}

Report read_report(int fd) {
    Reader r(fd);
    Report report;
    report.outcome = r.take<Outcome>();
    report.message.resize(r.take<std::uint32_t>());
    r.bytes(report.message.data(), report.message.size());
    report.pc = r.take<std::uint32_t>();
    r.bytes(report.registers.data(), report.registers.size());
    report.heap_end = r.take<std::uint32_t>();
    report.memory_error = r.take<std::int16_t>();
    report.allocated = take_table(r);
    report.available = take_table(r);
    for (auto count = r.take<std::uint32_t>(); count; --count) {
        const auto start = r.take<std::uint32_t>();
        report.maps.emplace_back(start, r.take<std::uint64_t>());
    }
    for (auto count = r.take<std::uint32_t>(); count; --count) {
        const auto page = r.take<std::uint32_t>();
        std::vector<std::uint8_t> bytes(page_size);
        r.bytes(bytes.data(), bytes.size());
        report.pages.emplace(page, std::move(bytes));
    }
    report.imports = r.take<std::uint64_t>();
    return report;
}

Report fork_run(State& s, std::uint32_t entry, RoutineRunner runner, bool corrupt, bool synthetic = false) {
    int fds[2];
    if (::pipe(fds) != 0) throw std::runtime_error("Routine check cannot create a pipe");
    std::cout.flush(); std::cerr.flush(); std::fflush(nullptr);
    const auto pid = ::fork();
    if (pid < 0) throw std::runtime_error("Routine check cannot fork");
    if (pid == 0) { ::close(fds[0]); run_child(s, entry, runner, fds[1], corrupt, synthetic); }
    ::close(fds[1]);
    Report report;
    std::string failure;
    try { report = read_report(fds[0]); } catch (const std::exception& e) { failure = e.what(); }
    ::close(fds[0]);
    int status = 0;
    ::waitpid(pid, &status, 0);
    if (!failure.empty()) throw std::runtime_error(failure + " (status " + std::to_string(status) + ")");
    return report;
}

std::string hex(std::uint64_t value, int width = 8) {
    std::ostringstream out; out << std::hex << std::setfill('0') << std::setw(width) << value; return out.str();
}
}

namespace {
// What differs between the original's and the rewrite's run of one call,
// from the parent's state before it (stack addresses shown relative to sp).
// Whether both runs stopped at the same point with the same message: a
// synthetic call into a path the recording never took stops at the first
// import the recording has no output for (an error alert, an exit), or at
// the same fault. Import stops can compare state; internal faults are
// matched only for diagnostics and remain failed equivalence checks.
// At an import both must stop at the same stub; a fault inside the routine
// is matched by its message (the access and address), since the rewrite's pc
// is where it last met the original (its entry or last call), not the
// faulting instruction.
bool same_stop(const Report& expected, const Report& actual) {
    if (expected.outcome != Outcome::fault || actual.outcome != Outcome::fault || expected.message != actual.message)
        return false;
    return expected.pc == actual.pc || (!import_name(expected.pc) && !import_name(actual.pc));
}

std::vector<std::string> compare_reports(const State& s, const Report& expected, const Report& actual, unsigned results,
                                         std::uint32_t stack, bool strict_volatile, bool stop_compared = false) {
    using R = RoutineChecker;
    [[maybe_unused]] constexpr unsigned result_r3 = R::result_r3, result_r4 = R::result_r4, result_f1 = R::result_f1;
    std::vector<std::string> found;
    // A replay never faults, so a stopped child is a failure of the rewrite or
    // of the harness, even when both stopped the same way; except for
    // synthetic calls stopping at the same import (stop_compared), whose
    // states there are compared like returns, without the condition register
    // (the rewrites reproduce it at returns only).
    const bool stopped = stop_compared && same_stop(expected, actual);
    // Internal faults are not comparison boundaries: matching messages do
    // not prove that the writes, imports or results agree. In particular,
    // do not turn an invalid synthetic input into an equivalence pass.
    if (stopped && !import_name(expected.pc))
        return {"both runs faulted internally; equivalence was not checked: " + expected.message};
    // At an import, r3 (its first argument) and f1 count too.
    if (stopped) results |= R::result_r3 | R::result_f1;
    if (expected.outcome == Outcome::fault && !stopped) found.push_back("the original stopped: " + expected.message);
    if (actual.outcome == Outcome::fault && !stopped) found.push_back("the rewrite stopped: " + actual.message);
    if (expected.outcome != actual.outcome)
        found.push_back("outcome " + std::to_string(int(expected.outcome)) + " vs " + std::to_string(int(actual.outcome)));
    if (expected.imports != actual.imports) found.push_back("the imports observed different inputs");
    if (expected.outcome == actual.outcome &&
        (expected.outcome == Outcome::returned || expected.outcome == Outcome::ended || stopped)) {
        if (expected.pc != actual.pc) found.push_back("pc " + hex(expected.pc) + " vs " + hex(actual.pc));
        const auto word = [](const Report& r, std::size_t offset) { return read_be<4>(r.registers.data() + offset); };
        // r4-r10 too: a caller's next varargs call (sprintf) stores r3-r10
        // into the stack image, so leftovers there reach memory.
        std::vector<unsigned> gprs{1, 2, 4, 5, 6, 7, 8, 9, 10};
        if (results & result_r3) gprs.push_back(3);
        for (unsigned n = 13; n < 32; ++n) gprs.push_back(n);
        for (const auto n : gprs)
            if (word(expected, n * 4) != word(actual, n * 4))
                found.push_back("r" + std::to_string(n) + " " + hex(word(expected, n * 4)) + " vs " + hex(word(actual, n * 4)));
        for (unsigned n = (results & result_f1) ? 1 : 14; n < 32; n = n == 1 ? 14 : n + 1) {
            const auto offset = 0x4000 + n * 16;
            if (read_be<8>(expected.registers.data() + offset) != read_be<8>(actual.registers.data() + offset))
                found.push_back("f" + std::to_string(n));
        }
        // Every CR field: a later callee that saves cr2-cr4 stores the
        // whole CR (mfcr), volatile fields included.
        for (unsigned field = 0; field < 8 && !stopped; ++field)
            if (expected.registers[2304 + field] != actual.registers[2304 + field])
                found.push_back("cr" + std::to_string(field) + " " + std::to_string(expected.registers[2304 + field]) +
                                " vs " + std::to_string(actual.registers[2304 + field]));
        // The rest of the volatile state rarely reaches memory; compared
        // only with strict_volatile, to find such a leak.
        if (strict_volatile) {
            for (unsigned n : {0u, 3u, 11u, 12u})
                if ((n != 3 || !(results & result_r3)) && word(expected, n * 4) != word(actual, n * 4))
                    found.push_back("volatile r" + std::to_string(n) + " " + hex(word(expected, n * 4)) + " vs " + hex(word(actual, n * 4)));
            for (std::size_t byte : {1024u, 1025u, 1026u, 1027u, 1028u, 1029u, 4100u, 4101u, 4102u, 4103u})
                if (expected.registers[byte] != actual.registers[byte]) found.push_back("volatile XER byte " + std::to_string(byte));
            if (read_be<4>(expected.registers.data() + 4132) != read_be<4>(actual.registers.data() + 4132)) found.push_back("volatile CTR");
        }
        // FPSCR (one byte per field, 1280-1311): setjmp stores it into memory.
        for (unsigned byte = 1280; byte < 1312; ++byte)
            if (expected.registers[byte] != actual.registers[byte])
                found.push_back("FPSCR byte " + std::to_string(byte - 1280));
    }
    if (expected.heap_end != actual.heap_end || expected.memory_error != actual.memory_error ||
        expected.allocated != actual.allocated || expected.available != actual.available)
        found.push_back("heap tables");
    if (expected.maps != actual.maps) found.push_back("new mappings");
    std::set<std::uint32_t> pages;
    for (const auto& [page, _] : expected.pages) pages.insert(page);
    for (const auto& [page, _] : actual.pages) pages.insert(page);
    unsigned listed = 0, differing = 0;
    for (const auto page : pages) {
        const auto address = std::uint64_t(page) << page_shift;
        const auto before = page_bytes(s.memory, page);
        const auto e = expected.pages.find(page), a = actual.pages.find(page);
        const auto& want = e != expected.pages.end() ? e->second : before;
        const auto& got = a != actual.pages.end() ? a->second : before;
        if (want == got) continue;
        for (std::uint32_t i = 0; i < page_size; ++i) {
            if (want[i] == got[i]) continue;
            ++differing;
            if (listed++ >= 24) continue;
            const auto at = std::uint32_t(address + i);
            std::string where;
            if (at < stack + 0x100 && at + 0x10000 > stack) {
                const auto offset = std::int64_t(at) - std::int64_t(stack);
                where = " (sp" + std::string(offset < 0 ? "-" : "+") + hex(std::uint64_t(offset < 0 ? -offset : offset), 1) + ")";
            }
            found.push_back("byte " + hex(at) + where + " original " + hex(want[i], 2) + " rewrite " + hex(got[i], 2) +
                            " before " + hex(before[i], 2));
        }
    }
    if (differing > listed) found.push_back(std::to_string(differing) + " differing bytes in all");
    return found;
}
}

bool routine_check_child() { return in_child; }

RoutineChecker::RoutineChecker(std::function<bool(std::uint32_t)> wanted, std::uint64_t all, std::uint64_t every)
    : wanted_(std::move(wanted)), all_(all), every_(every) {}

std::uint64_t RoutineChecker::differences() const {
    std::uint64_t total = 0;
    for (const auto& [_, t] : totals_) total += t.differences;
    return total;
}

bool RoutineChecker::check(State& s, std::uint32_t entry, RoutineRunner original, RoutineRunner rewrite, unsigned results) {
    if (in_child || !wanted_(entry)) return false;
    auto& totals = totals_[entry];
    const auto call = ++totals.calls;
    if (call > all_ && (!every_ || call % every_)) return false;
    const auto stack = std::uint32_t(s.r<4>(4)), caller = std::uint32_t(s.r<4>(4128));
    const bool corrupt = corrupt_first && !totals.checked && !differences();
    const auto expected = fork_run(s, entry, original, false);
    const auto actual = fork_run(s, entry, rewrite, corrupt);
    ++totals.checked;
    corrupt_first = corrupt_first && !corrupt;

    auto found = compare_reports(s, expected, actual, compare_f1 ? results | result_f1 : results, stack, strict_volatile);
    if (found.empty()) return true;
    ++totals.differences;
    std::cerr << "Routine " << hex(entry) << " call " << call << " (sp " << hex(stack) << ", return " << hex(caller)
              << ") differs from the original:\n";
    for (const auto& line : found) std::cerr << "  " << line << '\n';
    throw std::runtime_error("Routine " + hex(entry) + " differs from the original");
}

void run_original_body(State& s, std::uint32_t entry, bool& declining) {
    const auto stack = std::uint32_t(s.r<4>(4)), caller = std::uint32_t(s.r<4>(4128));
    struct Restore { bool& flag; bool value; ~Restore() { flag = value; } } restore{declining, declining};
    declining = true;
    s.pc = entry;
    if (!dispatch_page(s)) throw std::runtime_error("Missing original body at " + hex_address(entry));
    for (;;) {
        if (s.pc != caller) run_until(s, caller);
        if (std::uint32_t(s.r<4>(4)) == stack) return;
        // A nested activation of the same code returned here; keep going.
        if (!dispatch_page(s)) throw std::runtime_error("Missing original code at " + hex_address(s.pc));
    }
}
}

namespace cm::aot {
std::vector<SyntheticCall> read_synthetic_calls(const std::filesystem::path& file) {
    // One call per line: trigger=0xENTRY[#n] call=0xENTRY [results=r3|r3r4|f1|none] r3=VALUE ...
    std::ifstream input(file);
    if (!input) throw std::runtime_error("Cannot read synthetic calls " + file.string());
    std::vector<SyntheticCall> calls;
    for (std::string line; std::getline(input, line);) {
        if (line.empty() || line[0] == '#') continue;
        SyntheticCall call;
        call.line = line;
        std::istringstream words(line);
        for (std::string word; words >> word;) {
            const auto equal = word.find('=');
            if (equal == std::string::npos) throw std::runtime_error("Bad synthetic call: " + line);
            const auto key = word.substr(0, equal), value = word.substr(equal + 1);
            if (key == "trigger") {
                const auto hash = value.find('#');
                call.trigger = std::uint32_t(std::stoul(value.substr(0, hash), nullptr, 16));
                if (hash != std::string::npos) call.nth = std::uint32_t(std::stoul(value.substr(hash + 1)));
            } else if (key == "call") call.target = std::uint32_t(std::stoul(value, nullptr, 16));
            else if (key == "from") call.from = std::uint32_t(std::stoul(value, nullptr, 16));
            else if (key == "results") {
                call.results = value == "none" ? 0u : value == "r3r4" ? RoutineChecker::result_r3 | RoutineChecker::result_r4
                             : value == "f1" ? RoutineChecker::result_f1 : RoutineChecker::result_r3;
            } else if (key == "m8" || key == "m16" || key == "m32" ||
                       key == "expect8" || key == "expect16" || key == "expect32") {
                const auto colon = value.rfind(':');
                if (colon == std::string::npos) throw std::runtime_error("Bad synthetic memory value: " + line);
                const bool expectation = key.starts_with("expect");
                auto& values = expectation ? call.expected_memory : call.writes;
                values.push_back({unsigned(std::stoul(key.substr(expectation ? 6 : 1))) / 8,
                                  value.substr(0, colon), value.substr(colon + 1)});
            } else if (key.size() >= 2 && key[0] == 'r') call.registers.emplace_back(unsigned(std::stoul(key.substr(1))), value);
            else throw std::runtime_error("Bad synthetic call: " + line);
        }
        if (!call.trigger || !call.target) throw std::runtime_error("A synthetic call needs trigger= and call=: " + line);
        calls.push_back(std::move(call));
    }
    return calls;
}

std::vector<std::string> compare_routine_call(State& s, std::uint32_t entry, RoutineRunner original, RoutineRunner rewrite,
                                              unsigned results, bool corrupt, bool strict_volatile) {
    const auto stack = std::uint32_t(s.r<4>(4));
    const auto expected = fork_run(s, entry, original, false);
    const auto actual = fork_run(s, entry, rewrite, corrupt);
    return compare_reports(s, expected, actual, results, stack, strict_volatile);
}

namespace {
std::uint64_t synthetic_made = 0;

// A synthetic call's value: a number, @rN (register N at the trigger) or
// [EXPR] (the word at EXPR), followed by any number of +off / -off.
class SyntheticValue {
public:
    SyntheticValue(const Memory& memory, const decltype(State::registers)& registers)
        : memory_(memory), registers_(registers) {}
    std::uint32_t operator()(const std::string& text) const {
        std::size_t at = 0;
        const auto result = expression(text, at);
        if (at != text.size()) throw std::runtime_error("Bad synthetic value: " + text);
        return result;
    }

private:
    std::uint32_t number(const std::string& text, std::size_t& at) const {
        std::size_t used = 0;
        const auto value = std::uint32_t(std::stoul(text.substr(at), &used, 0));
        at += used;
        return value;
    }
    std::uint32_t expression(const std::string& text, std::size_t& at) const {
        auto result = term(text, at);
        while (at < text.size() && (text[at] == '+' || text[at] == '-')) {
            const char sign = text[at++];
            const auto offset = number(text, at);
            result = sign == '+' ? result + offset : result - offset;
        }
        return result;
    }
    std::uint32_t term(const std::string& text, std::size_t& at) const {
        if (at >= text.size()) throw std::runtime_error("Bad synthetic value: " + text);
        if (text[at] == '[') {
            ++at;
            const auto address = expression(text, at);
            if (at >= text.size() || text[at] != ']') throw std::runtime_error("Bad synthetic value: " + text);
            ++at;
            return std::uint32_t(memory_.read<4>(address));
        }
        if (text[at] == '@') {
            if (at + 1 >= text.size() || text[at + 1] != 'r') throw std::runtime_error("Bad synthetic register: " + text);
            at += 2;
            const auto n = number(text, at);
            if (n > 31) throw std::runtime_error("Bad synthetic register: " + text);
            return std::uint32_t(read_be<4>(registers_.data() + n * 4));
        }
        return number(text, at);
    }
    const Memory& memory_;
    const decltype(State::registers)& registers_;
};
}
std::uint64_t synthetic_calls_made() { return synthetic_made; }

unsigned run_synthetic_calls(State& s, std::vector<SyntheticCall>& calls, RoutineRunner original, RoutineRunner rewrite,
                             bool strict_volatile) {
    if (in_child) return 0;
    unsigned differing = 0;
    // Dispatches of each trigger, and of each (trigger, return address) for
    // triggers given with from=.
    static std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> seen;
    const auto link = std::uint32_t(s.r<4>(4128));
    std::set<std::pair<std::uint32_t, std::uint32_t>> counted;
    for (auto& call : calls) {
        if (call.trigger != s.pc || (call.from && call.from != link)) continue;
        const std::pair key{s.pc, call.from};
        if (counted.insert(key).second) ++seen[key];
        if (seen[key] != call.nth) continue;
        ++synthetic_made;
        call.made = true;
        const auto saved = s.registers;
        const auto saved_pc = s.pc;
        const SyntheticValue value(s.memory, saved);
        // Memory writes for the call only (state the recording never had),
        // made without observers and undone however the call ends; a bad
        // line is reported, never left behind in the replay's state.
        std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> undo;
        const auto observer = s.memory.observer, date_observer = s.memory.date_observer;
        std::vector<std::string> found;
        std::string stop;
        try {
            s.memory.observer = nullptr; s.memory.date_observer = nullptr;
            for (const auto& write : call.writes) {
                const auto address = value(write.address), v = value(write.value);
                undo.emplace_back(address, s.memory.copy_out(address, write.size));
                if (write.size == 1) s.memory.write<1>(address, v & 0xff);
                else if (write.size == 2) s.memory.write<2>(address, v & 0xffff);
                else s.memory.write<4>(address, v);
            }
            s.memory.observer = observer; s.memory.date_observer = date_observer;
            // A fresh frame below the trigger's, returning to the sentinel.
            const auto sp = (std::uint32_t(s.r<4>(4)) - 0x400) & ~0xfu;
            std::vector<std::pair<unsigned, std::uint32_t>> registers;
            for (const auto& [n, text] : call.registers) registers.emplace_back(n, value(text));
            s.wr<4>(4, sp);
            for (const auto& [n, v] : registers) s.wr<4>(n * 4, v);
            s.wr<4>(4128, return_sentinel);
            s.pc = call.target;
            const auto expected = fork_run(s, call.target, original, false, true);
            const auto actual = fork_run(s, call.target, rewrite, false, true);
            found = compare_reports(s, expected, actual, call.results, sp, strict_volatile, true);
            if (!call.expected_memory.empty()) {
                const auto check_memory = [&](const Report& report, const std::string& side) {
                    if (report.outcome != Outcome::returned || report.pc != return_sentinel) {
                        found.push_back(side + " final memory expectations require a completed return");
                        return;
                    }
                    for (const auto& check : call.expected_memory) {
                        const auto address = value(check.address);
                        const auto mask = check.size == 4 ? 0xffffffffu : (1u << (check.size * 8)) - 1;
                        const auto wanted = value(check.value) & mask;
                        const auto input_maps = s.memory.mapped_ranges();
                        const auto mapped = [&](std::uint32_t at) {
                            const auto contains = [at](const auto& range) {
                                return at >= range.first && std::uint64_t(at) - range.first < range.second;
                            };
                            return std::any_of(input_maps.begin(), input_maps.end(), contains) ||
                                   std::any_of(report.maps.begin(), report.maps.end(), contains);
                        };
                        if (std::uint64_t(address) + check.size > (std::uint64_t(1) << 32)) {
                            found.push_back(side + " memory expectation wraps the guest address space");
                            continue;
                        }
                        bool valid = true;
                        std::uint32_t got = 0;
                        for (unsigned byte = 0; byte < check.size; ++byte) {
                            const auto at = address + byte, page = at >> page_shift;
                            if (!mapped(at)) { valid = false; break; }
                            const auto changed = report.pages.find(page);
                            const auto contents = changed == report.pages.end() ? page_bytes(s.memory, page) : changed->second;
                            got = (got << 8) | contents[at & (page_size - 1)];
                        }
                        if (!valid) found.push_back(side + " memory expectation is unmapped at " + hex(address));
                        else if (got != wanted)
                            found.push_back(side + " memory expectation " + hex(address) + ": " + hex(got) + " != " + hex(wanted));
                    }
                };
                check_memory(expected, "original");
                check_memory(actual, "rewrite");
            }
            if (same_stop(expected, actual)) stop = " (both stopped at " + hex(expected.pc) + ": " + expected.message + ")";
        } catch (const std::exception& error) { found.push_back(error.what()); }
        s.memory.observer = nullptr; s.memory.date_observer = nullptr;
        for (auto it = undo.rbegin(); it != undo.rend(); ++it) s.memory.copy_in(it->first, it->second);
        s.memory.observer = observer; s.memory.date_observer = date_observer;
        s.registers = saved;
        s.pc = saved_pc;
        std::cerr << (found.empty() ? "synthetic ok" + stop + ": " : "synthetic DIFFERS" + stop + ": ") << call.line << '\n';
        for (const auto& line : found) std::cerr << "  " << line << '\n';
        if (found.empty() && !call.expected_memory.empty())
            std::cerr << "  verified " << call.expected_memory.size() << " final memory expectations in both completed runs\n";
        if (!found.empty()) ++differing;
    }
    return differing;
}
}

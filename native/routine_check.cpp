#include "routine_check.hpp"
#include "application_exit.hpp"
#include "guest_context_transfer.hpp"
#include "season_reference.hpp"
#include <sys/wait.h>
#include <unistd.h>
#include <bit>
#include <cstdio>
#include <iomanip>
#include <cstring>
#include <iostream>
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

enum class Outcome : std::uint8_t { returned, transfer, exit, fault };
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
[[noreturn]] void run_child(State& s, std::uint32_t entry, RoutineRunner runner, int fd, bool corrupt) {
    in_child = true;
    Outcome outcome = Outcome::fault;
    std::string message;
    DirtyPages dirty(s.memory.date_observer);
    std::vector<std::uint32_t> pages;
    try {
        season_replay_after_fork();
        s.memory.date_observer = &dirty;
        runner(s, entry);
        outcome = Outcome::returned;
    } catch (const GuestContextTransfer&) { outcome = Outcome::transfer; }
    catch (const ApplicationExit&) { outcome = Outcome::exit; }
    catch (const std::exception& e) { message = e.what(); }
    catch (...) { message = "unknown exception"; }
    s.memory.date_observer = dirty.previous;
    try {
        pages = dirty.pages();
        if (corrupt) {
            // Spoil the result without telling any observer: a byte the run
            // wrote, or r31 (always compared) when it wrote nothing.
            if (pages.empty()) s.wr<4>(31 * 4, s.r<4>(31 * 4) ^ 0x5a5a5a5a);
            else {
                auto bytes = page_bytes(s.memory, pages.front());
                bytes[page_size / 2] ^= 0x5a;
                const auto saved = s.memory.observer; s.memory.observer = nullptr;
                s.memory.copy_in(pages.front() << page_shift, bytes);
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
    return report;
}

Report fork_run(State& s, std::uint32_t entry, RoutineRunner runner, bool corrupt) {
    int fds[2];
    if (::pipe(fds) != 0) throw std::runtime_error("Routine check cannot create a pipe");
    std::cout.flush(); std::cerr.flush(); std::fflush(nullptr);
    const auto pid = ::fork();
    if (pid < 0) throw std::runtime_error("Routine check cannot fork");
    if (pid == 0) { ::close(fds[0]); run_child(s, entry, runner, fds[1], corrupt); }
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

    std::vector<std::string> found;
    // A replay never faults, so a stopped child is a failure of the rewrite or
    // of the harness, even when both stopped the same way.
    if (expected.outcome == Outcome::fault) found.push_back("the original stopped: " + expected.message);
    if (actual.outcome == Outcome::fault) found.push_back("the rewrite stopped: " + actual.message);
    if (expected.outcome != actual.outcome)
        found.push_back("outcome " + std::to_string(int(expected.outcome)) + " vs " + std::to_string(int(actual.outcome)));
    if (expected.outcome == Outcome::returned && actual.outcome == Outcome::returned) {
        if (expected.pc != actual.pc) found.push_back("pc " + hex(expected.pc) + " vs " + hex(actual.pc));
        const auto word = [](const Report& r, std::size_t offset) { return read_be<4>(r.registers.data() + offset); };
        std::vector<unsigned> gprs{1, 2};
        if (results & result_r3) gprs.push_back(3);
        if (results & result_r4) gprs.push_back(4);
        for (unsigned n = 13; n < 32; ++n) gprs.push_back(n);
        for (const auto n : gprs)
            if (word(expected, n * 4) != word(actual, n * 4))
                found.push_back("r" + std::to_string(n) + " " + hex(word(expected, n * 4)) + " vs " + hex(word(actual, n * 4)));
        for (unsigned n = (results & result_f1) ? 1 : 14; n < 32; n = n == 1 ? 14 : n + 1) {
            const auto offset = 0x4000 + n * 16;
            if (read_be<8>(expected.registers.data() + offset) != read_be<8>(actual.registers.data() + offset))
                found.push_back("f" + std::to_string(n));
        }
        for (unsigned field = 2; field <= 4; ++field)
            if (expected.registers[2304 + field] != actual.registers[2304 + field])
                found.push_back("cr" + std::to_string(field));
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

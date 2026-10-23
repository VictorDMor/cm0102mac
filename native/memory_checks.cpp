#include "aot_runtime.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>

using namespace cm::aot;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool accepts(Memory& memory, std::uint32_t address, std::size_t size) {
    try { (void)memory.copy_out(address, size); return true; }
    catch (const std::runtime_error& error) {
        require(std::string(error.what()) == "Unmapped data access at " + hex_address(address), "Unexpected memory diagnostic");
        return false;
    }
}
template <unsigned N> bool accepts_scalar(Memory& memory, std::uint64_t address) {
    const auto access = [&](bool write) {
        try {
            if (write) memory.write<N>(address, 0);
            else (void)memory.read<N>(address);
            return true;
        } catch (const std::runtime_error& error) {
            require(std::string(error.what()) == "Unmapped data access at " + hex_address(std::uint32_t(address)),
                    "Unexpected scalar access diagnostic");
            return false;
        }
    };
    const bool readable = access(false);
    require(access(true) == readable, "Read and write bounds disagree");
    return readable;
}
void rejected_wide_read(Memory& memory, std::uint64_t address) {
    bool rejected = false;
    try { (void)memory.read<8>(address); }
    catch (const std::runtime_error& error) {
        rejected = std::string(error.what()) == "Unmapped data access at " + hex_address(std::uint32_t(address));
    }
    require(rejected, "Wide or wrapping read was not rejected");
}
struct WriteObserver final : MemoryObserver {
    unsigned calls = 0;
    std::vector<std::pair<std::uint32_t, std::size_t>> writes;
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t address, const std::uint8_t*, std::size_t size) override {
        ++calls;
        writes.emplace_back(address, size);
    }
    void interval(std::uint64_t begin, std::uint64_t end) { restrict_write_notifications(begin, end); }
};

struct ReadObserver final : MemoryObserver {
    Memory& memory;
    std::vector<std::pair<std::uint32_t, unsigned>>* trace = nullptr;
    unsigned calls = 0;
    bool mutate = false, fail = false;
    explicit ReadObserver(Memory& value) : memory(value) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void read(std::uint32_t address, const std::uint8_t*, unsigned size) override {
        ++calls;
        if (trace) require(!trace->empty() && trace->back() == std::pair(address, size),
                           "Read observer ran before its trace entry");
        if (mutate) memory.write<1>(address, 0x9a);
        if (fail) throw std::runtime_error("Read observer fault");
    }
};
void read_notification_checks() {
    Memory memory;
    constexpr std::uint32_t base = 0x36000000;
    memory.map(base, 128);
    ReadObserver observer(memory);
    std::vector<std::pair<std::uint32_t, unsigned>> trace;
    const auto verify = [&]<unsigned N>() {
        constexpr auto value = UINT64_C(0x0123456789abcdef) & mask<N>();
        memory.write<N>(base + 1, value);
        memory.read_trace = &trace;
        memory.date_observer = nullptr;
        const auto before = trace.size();
        require(memory.read<N>(base + 1) == value && trace.size() == before + 1,
                "Trace-only scalar read changed");
        memory.date_observer = &observer;
        observer.trace = &trace;
        observer.mutate = true;
        const auto changed = (value & ~(UINT64_C(255) << ((N - 1) * 8))) |
                             (UINT64_C(0x9a) << ((N - 1) * 8));
        require(memory.read<N>(base + 1) == changed,
                "Scalar value was loaded before the observer's write");
        memory.read_trace = observer.trace = nullptr;
        observer.mutate = false;
        require(memory.read<N>(base + 1) == changed, "Observer-only scalar read changed");
    };
    verify.template operator()<1>(); verify.template operator()<2>();
    verify.template operator()<4>(); verify.template operator()<8>();
    memory.read_trace = &trace;
    observer.trace = &trace;
    observer.mutate = observer.fail = true;
    bool failed = false;
    try { (void)memory.read<4>(base + 1); }
    catch (const std::runtime_error& error) { failed = std::string(error.what()) == "Read observer fault"; }
    require(failed, "Read observer exception was lost");
    const auto calls = observer.calls;
    const auto entries = trace.size();
    rejected_wide_read(memory, base + 125);
    require(observer.calls == calls && trace.size() == entries,
            "Invalid scalar read reached diagnostics");
    memory.read_trace = nullptr; memory.date_observer = nullptr;
    require(memory.read<1>(base + 1) == 0x9a, "Faulting observer's completed write was lost");
    memory.observer = &observer;
    (void)memory.read<4>(base + 1);
    require(observer.calls == calls, "Write observer unexpectedly received a read");
    memory.observer = nullptr;
    std::cout << "Scalar read trace/callback order, callback mutation and faults passed\n";
}

void write_notification_checks() {
    Memory memory;
    constexpr std::uint32_t base = 0x35000000;
    memory.map(base, 128);
    std::array<bool, 128> interested{};
    for (unsigned n = 32; n < 48; ++n) interested[n] = true;
    WriteObserver filtered, all_writes;
    filtered.interval(base + 32, base + 48);
    memory.observer = &filtered;
    memory.date_observer = &all_writes;
    const auto verify = [&]<unsigned N>(unsigned offset) {
        const auto before = filtered.calls, all_before = all_writes.calls;
        memory.write<N>(base + offset, 0x0123456789abcdef);
        const bool expected = std::any_of(interested.begin() + offset,
            interested.begin() + offset + N, [](bool value) { return value; });
        require(filtered.calls == before + expected, "Scalar write notification missed byte interest");
        require(all_writes.calls == all_before + 1, "General diagnostics lost a scalar write");
        if (expected) require(filtered.writes.back() == all_writes.writes.back(), "Filtered write extent changed");
    };
    for (unsigned offset = 0; offset < 64; ++offset) {
        verify.template operator()<1>(offset);
        verify.template operator()<2>(offset);
        verify.template operator()<4>(offset);
        verify.template operator()<8>(offset);
    }
    filtered.interval(0, 0);
    const auto before_empty = filtered.calls;
    memory.write<8>(base + 32, 0);
    require(filtered.calls == before_empty, "Empty write interest received a scalar callback");
    // Bulk notifications retain their complete existing callback contract.
    memory.copy_in(base, {1, 2, 3});
    require(filtered.calls == before_empty + 1 && filtered.writes.back().second == 3,
            "Bulk write notification changed");
    memory.observer = memory.date_observer = nullptr;

    Memory high;
    high.map(0xfffffff0, 16);
    WriteObserver top;
    top.interval(0xffffffff, UINT64_C(0x100000000));
    high.observer = &top;
    high.write<8>(0xfffffff8, 1);
    require(top.calls == 1 && top.writes.back().first == 0xfffffff8,
            "Last address-space byte lost its overlapping wide write");
    const auto before_fault = top.calls;
    rejected_wide_read(high, 0xfffffff9);
    bool faulted = false;
    try { high.write<8>(0xfffffff9, 0); }
    catch (const std::runtime_error&) { faulted = true; }
    require(faulted && top.calls == before_fault, "Invalid write reached the observer");
    high.observer = nullptr;
    std::cout << "256 scalar write-interest byte-oracle cases, general/bulk observer preservation and top-boundary notifications passed\n";
}
void checks() {
    read_notification_checks();
    write_notification_checks();
    constexpr std::uint32_t base = 0x32000000;
    constexpr std::size_t span = 2048;
    Memory memory;
    std::array<bool, span> mapped{};
    require(!accepts(memory, base, 0) && !accepts(memory, base, 1), "Empty memory accepted a read");
    unsigned cases = 2;
    std::mt19937 random(0x01022001);
    const auto verify = [&](unsigned offset, unsigned size) {
        bool expected = false;
        if (size == 0) {
            // Existing API permits empty operations at either endpoint.
            expected = (offset < span && mapped[offset]) || (offset && offset <= span && mapped[offset - 1]);
        } else if (offset < span && size <= span - offset) {
            expected = std::all_of(mapped.begin() + offset, mapped.begin() + offset + size, [](bool value) { return value; });
        }
        require(accepts(memory, base + offset, size) == expected, "Mapped-byte oracle and range check disagree");
        if (size == 1) require(accepts_scalar<1>(memory, base + offset) == expected, "Byte bounds disagree");
        if (size == 2) require(accepts_scalar<2>(memory, base + offset) == expected, "Word bounds disagree");
        if (size == 4) require(accepts_scalar<4>(memory, base + offset) == expected, "Long bounds disagree");
        if (size == 8) require(accepts_scalar<8>(memory, base + offset) == expected, "Double bounds disagree");
        ++cases;
    };
    // Map out of order, overlap, and join intervals while leaving holes inside
    // the same OS page. OS page permissions cannot substitute for byte bounds.
    for (unsigned round = 0; round < 24; ++round) {
        unsigned offset = random() % (span - 64), count = 1 + random() % 64;
        memory.map(base + offset, count);
        std::fill(mapped.begin() + offset, mapped.begin() + offset + count, true);
        for (unsigned sample = 0; sample < 128; ++sample)
            verify(random() % (span + 32), random() % 33);
        for (unsigned size : {0u, 1u, 2u, 4u, 8u, 16u, 32u}) {
            verify(offset, size);
            verify(offset + count, size);
            if (offset) verify(offset - 1, size);
        }
    }
    for (unsigned offset = 0; offset <= span; ++offset)
        for (unsigned size : {0u, 1u, 4u, 8u, 32u}) verify(offset, size);
    memory.map(base, span);
    memory.write<8>(base + span - 8, UINT64_C(0x0123456789abcdef));
    require(memory.read<8>(base + span - 8) == UINT64_C(0x0123456789abcdef), "Big-endian payload changed");
    require(accepts(memory, base + span - 8, 8) && !accepts(memory, base + span - 7, 8), "End boundary changed");
    Memory high;
    high.map(0xfffffff0, 16);
    high.write<8>(0xfffffff8, UINT64_C(0xfedcba9876543210));
    require(high.read<8>(0xfffffff8) == UINT64_C(0xfedcba9876543210), "Last 32-bit bytes inaccessible");
    for (auto address : {UINT64_C(0xfffffff9), UINT64_C(0x100000000), UINT64_C(0xfffffffffffffffc)})
        rejected_wide_read(high, address);
    require(!accepts(high, 0xfffffff0, std::size_t(-1)), "Wrapping span accepted");
    for (auto address : {UINT64_C(0x100000000), UINT64_C(0xfffffffffffffffc)})
        require(!accepts_scalar<8>(high, address), "Wide scalar access accepted");
    // Exercise certified prefixes, partial starts, exact ends, cross-page
    // accesses, disjoint intervals on one page, and subsequent merges.
    Memory pages;
    constexpr std::uint32_t page_base = 0x34000000;
    std::vector<bool> page_bytes(4 * 65536 + 32);
    for (auto [offset, count] : std::array<std::pair<unsigned, unsigned>, 7>{{
            {65536 + 3, 65536 + 17}, {0, 65536 - 3}, {3 * 65536, 19},
            {65536 - 3, 6}, {2 * 65536 + 20, 65536 - 20},
            {3 * 65536 + 23, 65536 + 9}, {3 * 65536 + 19, 4}}}) {
        pages.map(page_base + offset, count);
        std::fill(page_bytes.begin() + offset, page_bytes.begin() + offset + count, true);
        const auto verify_scalar = [&]<unsigned N>(unsigned at) {
            bool expected = at + N <= page_bytes.size() &&
                std::all_of(page_bytes.begin() + at, page_bytes.begin() + at + N, [](bool b) { return b; });
            require(accepts_scalar<N>(pages, page_base + at) == expected, "Page boundary byte oracle disagrees");
            ++cases;
        };
        for (unsigned boundary : {0u, offset, offset + count, 65536u, 2 * 65536u, 3 * 65536u, 4 * 65536u}) {
            for (unsigned at = boundary > 8 ? boundary - 8 : 0; at <= boundary + 8; ++at) {
                verify_scalar.template operator()<1>(at);
                verify_scalar.template operator()<2>(at);
                verify_scalar.template operator()<4>(at);
                verify_scalar.template operator()<8>(at);
            }
        }
    }
    // The game's layout: code ends and data starts 12 bytes later in one
    // 64 KiB block; both sides and the gap keep exact bounds.
    {
        Memory layout;
        layout.map(0x10000000, 0x8b3a84);
        layout.map(0x108b3a90, 0x34be84);
        for (std::uint32_t address = 0x108b3a78; address <= 0x108b3a98; ++address) {
            const bool code = address + 4 <= 0x108b3a84, data = address >= 0x108b3a90;
            require(accepts_scalar<4>(layout, address) == (code || data), "Code/data block bounds");
            require(accepts_scalar<1>(layout, address) == (address < 0x108b3a84 || address >= 0x108b3a90), "Gap byte bounds");
            ++cases;
        }
        require(accepts_scalar<4>(layout, 0x108bfffc) && accepts_scalar<8>(layout, 0x108bfff8), "Data to the block end");
        require(!accepts_scalar<4>(layout, 0x10bff912) && accepts_scalar<4>(layout, 0x10bff910), "Data end");
    }
    high.map(0xffff0000, 65536);
    require(accepts_scalar<8>(high, 0xfffffff8), "Indexed top boundary inaccessible");
    require(!accepts_scalar<8>(high, 0xfffffff9), "Indexed top boundary overflow accepted");
    bool invalid_map = false;
    try { high.map(16, UINT64_MAX); }
    catch (const std::runtime_error& error) { invalid_map = std::string(error.what()) == "Invalid data mapping"; }
    require(invalid_map, "Wrapping map accepted");
    State state;
    state.budget = 1;
    state.step(0x10000000);
    require(state.executed == 1 && state.pc == 0x10000000, "Successful instruction accounting changed");
    bool exhausted = false;
    try { state.step(0x10000004); }
    catch (const std::runtime_error& error) { exhausted = std::string(error.what()) == "Native instruction budget exhausted"; }
    require(exhausted && state.executed == 2 && state.pc == 0x10000004, "Budget failure state changed");
    std::cout << cases << " mapped-byte oracle cases plus payload and 32-bit boundary checks passed\n";
}
void benchmark() {
    Memory memory;
    // Mapping shapes captured from the original saved-game load. Access mixes
    // are deliberately synthetic; they are not a full-game timing claim.
    const std::array<std::pair<std::uint32_t, unsigned>, 6> ranges{{
        {0x10000000, 9124484}, {0x108b3a90, 3424132}, {0x10bf7a20, 1036},
        {0x20000000, 35140288}, {0x60000000, 2072}, {0x70000000, 1048576}}};
    for (auto [start, size] : ranges) memory.map(start, size);
    std::mt19937 random(102);
    std::uint64_t checksum = 0;
    for (const auto& mix : std::array<std::vector<unsigned>, 6>{{
            {0}, {1}, {3}, {5}, {0, 1, 3, 5}, {0, 1, 3, 3, 3, 3, 5, 5}}}) {
        std::vector<std::uint32_t> addresses;
        for (unsigned i = 0; i < 4096; ++i) {
            const auto [start, size] = ranges[mix[i % mix.size()]];
            const auto address = start + (random() % ((size - 8) / 8)) * 8;
            addresses.push_back(address);
            memory.write<8>(address, i);
        }
        std::vector<double> samples;
        for (unsigned repeat = 0; repeat < 9; ++repeat) {
            const auto start = std::chrono::steady_clock::now();
            for (unsigned round = 0; round < 512; ++round)
                for (auto address : addresses) checksum += memory.read<8>(address);
            samples.push_back(std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / (512 * addresses.size()));
        }
        std::sort(samples.begin(), samples.end());
        std::cout << "mix=";
        for (auto index : mix) std::cout << index;
        std::cout << " median_ns_per_read=" << samples[4] << " checksum=" << checksum << '\n';
    }
}
}
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--benchmark") benchmark();
        else if (argc == 1) checks();
        else throw std::runtime_error("Usage: cm-memory-checks [--benchmark]");
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

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
void rejected_wide_read(Memory& memory, std::uint64_t address) {
    bool rejected = false;
    try { (void)memory.read<8>(address); }
    catch (const std::runtime_error& error) {
        rejected = std::string(error.what()) == "Unmapped data access at " + hex_address(std::uint32_t(address));
    }
    require(rejected, "Wide or wrapping read was not rejected");
}
void checks() {
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

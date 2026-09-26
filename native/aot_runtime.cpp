#include "aot_runtime.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>

namespace cm::aot {
std::string hex_address(std::uint32_t value) {
    std::ostringstream result;
    result << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
    return result.str();
}
Memory::Memory() {
    // Only data is stored here. Never grant execute permission to these pages.
    void* p = mmap(nullptr, UINT64_C(1) << 32, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (p == MAP_FAILED) throw std::runtime_error("Cannot reserve legacy data address space");
    data_ = static_cast<std::uint8_t*>(p);
}
Memory::~Memory() { munmap(data_, UINT64_C(1) << 32); }
void Memory::map(std::uint32_t start, std::uint64_t size) {
    if (!size || std::uint64_t(start) + size > (UINT64_C(1) << 32))
        throw std::runtime_error("Invalid data mapping");
    const auto page = std::uint64_t(sysconf(_SC_PAGESIZE));
    const auto begin = start & ~(page - 1);
    const auto end = (start + size + page - 1) & ~(page - 1);
    if (mprotect(data_ + begin, end - begin, PROT_READ | PROT_WRITE) != 0)
        throw std::runtime_error("Cannot map legacy data pages");
    ranges_.push_back({start, std::uint64_t(start) + size});
    std::sort(ranges_.begin(), ranges_.end(), [](auto a, auto b) { return a.start < b.start; });
    std::vector<Range> merged;
    for (auto range : ranges_) {
        if (!merged.empty() && range.start <= merged.back().end)
            merged.back().end = std::max(merged.back().end, range.end);
        else merged.push_back(range);
    }
    ranges_ = std::move(merged);
}
namespace {
// Keep exception formatting and its register spills off the valid-access path.
[[noreturn, gnu::cold, gnu::noinline]] void unmapped_access(std::uint64_t address) {
    throw std::runtime_error("Unmapped data access at " + hex_address(std::uint32_t(address)));
}
}
void Memory::check(std::uint64_t address, std::uint64_t size) const {
    if (address < (UINT64_C(1) << 32) && size <= (UINT64_C(1) << 32) - address) {
        for (const auto& range : ranges_)
            if (address >= range.start && address + size <= range.end) return;
    }
    unmapped_access(address);
}
void Memory::load(std::uint32_t start, const std::filesystem::path& file, std::uint64_t size) {
    if (std::filesystem::file_size(file) != size) throw std::runtime_error("Segment size mismatch");
    map(start, size);
    std::ifstream stream(file, std::ios::binary);
    if (!stream.read(reinterpret_cast<char*>(data_ + start), std::streamsize(size)))
        throw std::runtime_error("Cannot read program segment");
}
void Memory::copy_in(std::uint32_t start, const std::vector<std::uint8_t>& bytes) {
    check(start, bytes.size());
    std::memcpy(data_ + start, bytes.data(), bytes.size());
}
void Memory::clear(std::uint32_t start, std::uint64_t size) {
    check(start, size);
    std::memset(data_ + start, 0, size);
}
std::vector<std::uint8_t> Memory::copy_out(std::uint32_t start, std::size_t size) const {
    check(start, size);
    return {data_ + start, data_ + start + size};
}
void run(State& s) {
    const bool trace = std::getenv("CM_AOT_TRACE_IMPORTS") != nullptr;
    while (s.pc != return_sentinel) {
        if (const char* name = import_name(s.pc)) {
            if (trace) {
                std::fprintf(stderr, "%s pc=%08x lr=%08x", name, s.pc, unsigned(s.r<4>(4128)));
                for (unsigned r = 3; r <= 10; ++r) std::fprintf(stderr, " r%u=%08x", r, unsigned(s.r<4>(r * 4)));
                std::fprintf(stderr, "\n");
            }
            if (bridge_import(s, name)) {
                if (trace) std::fprintf(stderr, "%s returned r3=%08x\n", name, unsigned(s.r<4>(12)));
                ++s.native_calls;
                continue;
            }
            throw std::runtime_error(std::string("Unimplemented native OS bridge: ") + name +
                " (r3=" + hex_address(s.r<4>(12)) + ", r4=" + hex_address(s.r<4>(16)) + ")");
        }
        if (!dispatch_page(s))
            throw std::runtime_error("Uncompiled original address: " + hex_address(s.pc));
    }
}
}

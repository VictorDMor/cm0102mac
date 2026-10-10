#include "aot_runtime.hpp"
#include "allocator_adapter.hpp"
#include "stream_buffer_adapter.hpp"
#include "match_turbo.hpp"
#include "season_reference.hpp"
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
    if (!size || size > (UINT64_C(1) << 32) - start)
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
    // Mappings only grow: refresh the merged interval containing this addition.
    // A later partial interval in the same page cannot replace a valid prefix.
    for (const auto& range : ranges_) {
        if (start < range.start || start >= range.end) continue;
        const auto first = (range.start + 65535) >> 16;
        const auto last = (range.end - 1) >> 16;
        for (auto page_index = first; page_index <= last; ++page_index)
            (*page_ends_)[page_index] = range.end;
        break;
    }
    if (observer) observer->mapped(start, size);
    if (date_observer) date_observer->mapped(start, size);
}
void instruction_budget_exhausted() {
    throw std::runtime_error("Native instruction budget exhausted");
}
namespace {
struct MatchPcProfile {
    static constexpr std::uint64_t stride = 1048583;
    static constexpr std::uint64_t after(std::uint64_t executed) {
        return executed > UINT64_MAX - stride ? UINT64_MAX : executed + stride;
    }
    std::uint64_t next = 0;
    std::map<std::uint32_t, std::uint64_t> counts;
    ~MatchPcProfile() {
        if (counts.empty()) return;
        std::vector<std::pair<std::uint32_t, std::uint64_t>> ranked(counts.begin(), counts.end());
        std::sort(ranked.begin(), ranked.end(), [](auto a, auto b) { return a.second > b.second; });
        std::fprintf(stderr, "Match original-PC samples: %zu distinct\n", ranked.size());
        for (std::size_t i = 0; i < std::min<std::size_t>(ranked.size(), 40); ++i)
            std::fprintf(stderr, "  %08x %llu\n", ranked[i].first,
                         static_cast<unsigned long long>(ranked[i].second));
    }
};
MatchPcProfile& match_pc_profile() {
    static MatchPcProfile profile;
    return profile;
}
}
bool match_pc_profile_enabled() {
    static const bool enabled = std::getenv("CM_AOT_MATCH_PC_PROFILE") != nullptr;
    return enabled;
}
bool match_replay_turbo_enabled() {
    return std::getenv("CM_AOT_MATCH_REPLAY_TURBO") != nullptr;
}
std::uint64_t match_pc_sample_threshold(std::uint64_t executed) {
    auto& profile = match_pc_profile();
    // Uninstrumented pages also advance the global instruction count. Rebase
    // an overdue threshold on reentry instead of emitting catch-up samples on
    // consecutive instructions. These remain periodic selected-page visits,
    // not unbiased CPU percentages or a count of work in unsampled pages.
    if (!profile.next || executed >= profile.next)
        profile.next = MatchPcProfile::after(executed);
    return profile.next;
}
std::uint64_t record_match_pc_sample(std::uint32_t address) {
    auto& profile = match_pc_profile();
    ++profile.counts[address];
    profile.next = MatchPcProfile::after(profile.next);
    return profile.next;
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
    if (observer) observer->written(start, data_ + start, bytes.size());
    if (date_observer) date_observer->written(start, data_ + start, bytes.size());
}
void Memory::clear(std::uint32_t start, std::uint64_t size) {
    check(start, size);
    std::memset(data_ + start, 0, size);
    if (observer) observer->cleared(start, size);
    if (date_observer) date_observer->cleared(start, size);
}
std::vector<std::uint8_t> Memory::copy_out(std::uint32_t start, std::size_t size) const {
    check(start, size);
    return {data_ + start, data_ + start + size};
}
std::vector<std::pair<std::uint32_t, std::uint64_t>> Memory::mapped_ranges() const {
    std::vector<std::pair<std::uint32_t, std::uint64_t>> result;
    result.reserve(ranges_.size());
    for (const auto& range : ranges_)
        result.emplace_back(std::uint32_t(range.start), range.end - range.start);
    return result;
}
void run_until(State& s, std::uint32_t return_pc) {
    const bool trace = std::getenv("CM_AOT_TRACE_IMPORTS") != nullptr;
    while (s.pc != return_pc) {
        if (const char* name = import_name(s.pc)) {
            if (season_replay_import(s, name)) { ++s.native_calls; continue; }
            season_record_import_begin(s);
            if (s.allocator_observer) s.allocator_observer->import_enter(s, name);
            if (s.stream_buffer_observer) s.stream_buffer_observer->import_enter(s, name);
            if (trace) {
                std::fprintf(stderr, "%s pc=%08x lr=%08x", name, s.pc, unsigned(s.r<4>(4128)));
                for (unsigned r = 3; r <= 10; ++r) std::fprintf(stderr, " r%u=%08x", r, unsigned(s.r<4>(r * 4)));
                std::fprintf(stderr, "\n");
            }
            if (bridge_import(s, name)) {
                season_record_import_end(s);
                if (trace) std::fprintf(stderr, "%s returned r3=%08x\n", name, unsigned(s.r<4>(12)));
                ++s.native_calls;
                if (s.allocator_observer) s.allocator_observer->import_leave(s, name);
                if (s.stream_buffer_observer) s.stream_buffer_observer->import_leave(s, name);
                continue;
            }
            throw std::runtime_error(std::string("Unimplemented native OS bridge: ") + name +
                " (r3=" + hex_address(s.r<4>(12)) + ", r4=" + hex_address(s.r<4>(16)) + ")");
        }
        if (!dispatch_page(s))
            throw std::runtime_error("Uncompiled original address: " + hex_address(s.pc));
    }
}
void run(State& s) {run_until(s,return_sentinel);}
}

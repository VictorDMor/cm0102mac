#include "aot_runtime.hpp"
#include "allocator_adapter.hpp"
#include "import_bridge.hpp"
#include "stream_buffer_adapter.hpp"
#include "stream_io_adapter.hpp"
#include "file_controller_adapter.hpp"
#include "file_path_adapter.hpp"
#include "runtime_lifecycle_adapter.hpp"
#include "code_fragment_adapter.hpp"
#include "cpp_object_storage_adapter.hpp"
#include "exception_runtime_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "quick_save_runtime.hpp"
#include "match_turbo.hpp"
#include "season_reference.hpp"
#include "routine_check.hpp"
#include "owner_in_place.hpp"
#include <algorithm>
#include <sys/ucontext.h>
#include <csignal>
#include <atomic>
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
namespace {
// The 4 GiB guest space, then a PROT_NONE guard as large again: any sum of two
// 32-bit values used as an address faults inside the reservation in hardware
// mode instead of reaching host memory.
constexpr std::uint64_t reservation_size = (UINT64_C(1) << 33) + 0x10000;
std::atomic<Memory*> hardware_memory{nullptr};
std::atomic<std::uint8_t*> hardware_base{nullptr};
HardwareFaultReporter fault_reporter = nullptr;
void* fault_reporter_context = nullptr;
struct sigaction previous_segv{}, previous_bus{};

[[noreturn]] void hardware_fault_stop(std::uint64_t address, std::uint64_t host_pc) {
    if (auto* memory = hardware_memory.load()) memory->leave_hardware_bounds();
    // The guest PC in State is exact only at run starts and may predate
    // unpublished page counts; the host PC names the faulting code (atos).
    // The address is the first byte the host reported, which for an access
    // that crosses into an unmapped page is that page's first byte.
    char message[160];
    if (address < (UINT64_C(1) << 32))
        std::snprintf(message, sizeof message, "Unmapped data access at 0x%08x (hardware bounds; host pc 0x%llx)",
                      unsigned(address), static_cast<unsigned long long>(host_pc));
    else
        std::snprintf(message, sizeof message,
                      "Unmapped data access at 0x%llx, past the 4 GiB guest space (hardware bounds; host pc 0x%llx)",
                      static_cast<unsigned long long>(address), static_cast<unsigned long long>(host_pc));
    if (fault_reporter) fault_reporter(address, message, fault_reporter_context);
    else std::fprintf(stderr, "%s\n", message);
    std::fflush(stdout);
    std::fflush(stderr);
    std::_Exit(70);
}

void hardware_fault_signal(int signal, siginfo_t* info, void* raw) {
    const auto base = reinterpret_cast<std::uintptr_t>(hardware_base.load(std::memory_order_relaxed));
    const auto address = reinterpret_cast<std::uintptr_t>(info->si_addr);
    if (base && hardware_memory.load(std::memory_order_relaxed) && address - base < reservation_size) {
#if defined(__arm64__)
        // Continue in hardware_fault_stop on this thread, below the red zone,
        // with the faulting instruction as its return address for backtraces.
        auto& thread = static_cast<ucontext_t*>(raw)->uc_mcontext->__ss;
        const auto pc = __darwin_arm_thread_state64_get_pc(thread);
        const auto sp = (__darwin_arm_thread_state64_get_sp(thread) - 1024) & ~std::uint64_t(15);
        __darwin_arm_thread_state64_set_sp(thread, sp);
        __darwin_arm_thread_state64_set_lr_fptr(thread, reinterpret_cast<void*>(pc));
        thread.__x[0] = address - base;
        thread.__x[1] = pc;
        __darwin_arm_thread_state64_set_pc_fptr(thread, reinterpret_cast<void*>(&hardware_fault_stop));
        return;
#endif
    }
    // Not a guest access: hand it to the previous disposition, keeping this
    // handler for later guest faults. Default or ignored: fault again with
    // the default action (ignoring would retry the instruction forever).
    const auto& previous = signal == SIGBUS ? previous_bus : previous_segv;
    if (previous.sa_flags & SA_SIGINFO) {
        if (previous.sa_sigaction) return previous.sa_sigaction(signal, info, raw);
    } else if (previous.sa_handler != SIG_DFL && previous.sa_handler != SIG_IGN && previous.sa_handler) {
        return previous.sa_handler(signal);
    }
    std::signal(signal, SIG_DFL);
}
}
void set_hardware_fault_reporter(HardwareFaultReporter reporter, void* context) {
    fault_reporter = reporter;
    fault_reporter_context = context;
}
Memory::Memory() {
    // Only data is stored here. Never grant execute permission to these pages.
    void* p = mmap(nullptr, reservation_size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (p == MAP_FAILED) throw std::runtime_error("Cannot reserve legacy data address space");
    data_ = static_cast<std::uint8_t*>(p);
}
Memory::~Memory() {
    leave_hardware_bounds();
    if (!shared_) munmap(data_, reservation_size);
}
void Memory::share(const Memory& main) {
    if (hardware_ || &main == this) throw std::logic_error("A shared guest memory view needs another, plain memory");
    if (!shared_) munmap(data_, reservation_size);
    shared_ = true;
    data_ = main.data_;
    ranges_ = main.ranges_;
    page_ends_ = main.page_ends_;
    partial_starts_ = main.partial_starts_;
    // Hardware bounds belong to the reservation, so they hold for this view too.
    exact_bounds_ = main.hardware_ ? 0 : 1;
}
void Memory::use_hardware_bounds() {
#if !defined(__arm64__)
    throw std::runtime_error("Hardware bounds need an ARM64 host");
#else
    if (hardware_) return;
    Memory* expected = nullptr;
    if (!hardware_memory.compare_exchange_strong(expected, this))
        throw std::runtime_error("Another guest memory already uses hardware bounds");
    // Install the handler unless it is already the current one; a handler
    // installed since then becomes the disposition for faults elsewhere.
    const auto install = [](int signal, struct sigaction& previous) {
        struct sigaction current{};
        if (sigaction(signal, nullptr, &current) != 0) return false;
        if ((current.sa_flags & SA_SIGINFO) && current.sa_sigaction == hardware_fault_signal) return true;
        struct sigaction action{};
        action.sa_sigaction = hardware_fault_signal;
        action.sa_flags = SA_SIGINFO;
        sigemptyset(&action.sa_mask);
        return sigaction(signal, &action, &previous) == 0;
    };
    if (!install(SIGSEGV, previous_segv) || !install(SIGBUS, previous_bus)) {
        hardware_memory.store(nullptr);
        throw std::runtime_error("Cannot install the hardware bounds fault handler");
    }
    hardware_base.store(data_);
    hardware_ = true;
    --exact_bounds_;
#endif
}
void Memory::leave_hardware_bounds() {
    if (!hardware_) return;
    hardware_ = false;
    ++exact_bounds_;
    Memory* expected = this;
    if (hardware_memory.compare_exchange_strong(expected, nullptr)) hardware_base.store(nullptr);
}
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
        if (range.start & 0xffff) (*partial_starts_)[range.start >> 16] = range;
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
bool Memory::try_copy_unobserved(std::uint32_t address, std::uint8_t* destination, std::size_t size) const {
    if (read_trace || date_observer) return false;
    const auto end = (*page_ends_)[address >> 16];
    if (address >= end || size > end - address) return false;
    std::memcpy(destination, data_ + address, size);
    return true;
}
void Memory::observe_scalar_read(std::uint64_t address, unsigned size) const {
    if (read_trace) read_trace->emplace_back(std::uint32_t(address), size);
    if (date_observer) date_observer->read(std::uint32_t(address), data_ + address, size);
}
void Memory::report_write(std::uint64_t address, unsigned size) {
    const auto start = std::uint32_t(address);
    if (observer && observer->observes_write(address, size)) observer->written(start, data_ + address, size);
    if (date_observer) date_observer->written(start, data_ + address, size);
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
// Generated with import_name: the range of the import stubs.
extern const std::uint32_t import_stubs_begin, import_stubs_end;
namespace {
// The dispatcher loop. Check tools (dispatch_probe) get a copy that asks the
// probe before each page; testing the probe on every iteration cost the
// day replay 2-4%. With `owner_first`, the first instruction's page is not
// dispatched when its entry has a C++ owner that runs in place
// (run_owner_in_place); the page would only ask that owner's hook.
template <bool Probe> void dispatch_until(State& s, std::uint32_t return_pc, bool quick_save_boundary = false,
                                          bool owner_first = false) {
    GuestDispatchScope dispatch(s, return_pc);
    struct QuickSaveBoundary {
        bool enabled;
        GuestDispatchScope* previous = nullptr;
        QuickSaveBoundary(GuestDispatchScope& scope, bool active) : enabled(active) {
            if (enabled) previous = set_quick_save_dispatch_boundary(&scope);
        }
        ~QuickSaveBoundary() { if (enabled) set_quick_save_dispatch_boundary(previous); }
    } boundary(dispatch, quick_save_boundary);
    // Launch-time diagnostic policy, shared by nested dispatch and every State.
    // Semantic dependency calls can enter here millions of times per match;
    // looking up the process environment on every call dominates that path.
    static const bool trace = std::getenv("CM_AOT_TRACE_IMPORTS") != nullptr;
    for (;;) {
    try {
    if (owner_first) {
        owner_first = false;
        // A decline (or no such owner) leaves the state untouched: the loop
        // then dispatches the page, whose hook is asked again.
        if (s.pc != return_pc) run_owner_in_place(s, s.pc);
    }
    while (s.pc != return_pc) {
        if (const char* name = s.pc - import_stubs_begin < import_stubs_end - import_stubs_begin ? import_name(s.pc) : nullptr) {
            if (season_replay_import(s, name)) { ++s.native_calls; continue; }
            season_record_import_begin(s);
            if (s.allocator_observer) s.allocator_observer->import_enter(s, name);
            if (s.stream_buffer_observer) s.stream_buffer_observer->import_enter(s, name);
            if (s.stream_io_observer) s.stream_io_observer->import_enter(s, name);
            if (s.file_controller_observer) s.file_controller_observer->import_enter(s, name);
            if (s.file_path_observer) s.file_path_observer->import_enter(s, name);
            if (s.runtime_lifecycle_observer) s.runtime_lifecycle_observer->import_enter(s, name);
            if (s.code_fragment_observer) s.code_fragment_observer->import_enter(s, name);
            if (s.cpp_object_storage_observer) s.cpp_object_storage_observer->import_enter(s, name);
            if (s.exception_runtime_observer) s.exception_runtime_observer->import_enter(s, name);
            if (trace) {
                std::fprintf(stderr, "%s pc=%08x lr=%08x", name, s.pc, unsigned(s.r<4>(4128)));
                for (unsigned r = 3; r <= 10; ++r) std::fprintf(stderr, " r%u=%08x", r, unsigned(s.r<4>(r * 4)));
                std::fprintf(stderr, "\n");
            }
            if ((s.file_controller_observer && s.file_controller_observer->import_fixture(s, name)) ||
                (s.file_path_observer && s.file_path_observer->import_fixture(s, name)) ||
                (s.runtime_lifecycle_observer && s.runtime_lifecycle_observer->import_fixture(s, name)) ||
                (s.code_fragment_observer && s.code_fragment_observer->import_fixture(s, name)) ||
                (s.cpp_object_storage_observer && s.cpp_object_storage_observer->import_fixture(s, name)) ||
                (s.exception_runtime_observer && s.exception_runtime_observer->import_fixture(s, name)) || bridge_import_name(s, name)) {
                season_record_import_end(s, name);
                if (trace) std::fprintf(stderr, "%s returned r3=%08x\n", name, unsigned(s.r<4>(12)));
                ++s.native_calls;
                if (s.allocator_observer) s.allocator_observer->import_leave(s, name);
                if (s.stream_buffer_observer) s.stream_buffer_observer->import_leave(s, name);
                if (s.stream_io_observer) s.stream_io_observer->import_leave(s, name);
                if (s.file_controller_observer) s.file_controller_observer->import_leave(s, name);
                if (s.file_path_observer) s.file_path_observer->import_leave(s, name);
                if (s.runtime_lifecycle_observer) s.runtime_lifecycle_observer->import_leave(s, name);
                if (s.code_fragment_observer) s.code_fragment_observer->import_leave(s, name);
                if (s.cpp_object_storage_observer) s.cpp_object_storage_observer->import_leave(s, name);
                if (s.exception_runtime_observer) s.exception_runtime_observer->import_leave(s, name);
                continue;
            }
            throw std::runtime_error(std::string("Unimplemented native OS bridge: ") + name +
                " (r3=" + hex_address(s.r<4>(12)) + ", r4=" + hex_address(s.r<4>(16)) + ")");
        }
        if constexpr (Probe) { if (dispatch_probe && dispatch_probe(s)) continue; }
        if (!dispatch_page(s))
            throw std::runtime_error("Uncompiled original address: " + hex_address(s.pc));
    }
    return;
    } catch (const GuestContextTransfer& transfer) {
        if (!dispatch.resumes(transfer)) throw;
    }
    }
}
// A call into a leaf routine (getters and other short straight-line bodies)
// needs no dispatcher scope: it cannot call or transfer.
[[gnu::always_inline]] inline void run_until_from(State& s, std::uint32_t return_pc, bool owner_first) {
    if (s.pc != return_pc && run_leaf(s)) {
        if (s.pc == return_pc) return;
        owner_first = false;  // the call's entry has run; s.pc is elsewhere
    }
    if (dispatch_probe) [[unlikely]] dispatch_until<true>(s, return_pc);
    else dispatch_until<false>(s, return_pc, false, owner_first);
}
}
void run_until(State& s, std::uint32_t return_pc) { run_until_from(s, return_pc, false); }
// Check tools (a probe or a routine checker) keep the page dispatch they observe.
void run_owner_until(State& s, std::uint32_t return_pc) { run_until_from(s, return_pc, !routine_checker); }
void run_quick_save_until(State& s, std::uint32_t return_pc) {
    if (dispatch_probe) [[unlikely]] dispatch_until<true>(s, return_pc, true);
    else dispatch_until<false>(s, return_pc, true);
}
void run(State& s) {run_until(s,return_sentinel);}
}

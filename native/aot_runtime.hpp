#pragma once

// Support for C++ produced at build time from the original PowerPC program.
// No instruction decoder, interpreter, JIT, executable guest mapping, or CPU
// emulator is linked into the resulting ARM64 program.
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace cm::aot {
class NativePlatform;
struct RngBoundaryObserver;
struct BackgroundBoundaryObserver;
struct CollectionBoundaryObserver;
struct StringBoundaryObserver;
struct MemoryPrimitivesBoundaryObserver;
struct IntegerParsingBoundaryObserver;
struct FormattedInputBoundaryObserver;
struct FormattedOutputBoundaryObserver;
struct CharacterConversionBoundaryObserver;
struct FloatingInputBoundaryObserver;
struct AllocatorBoundaryObserver;
struct StreamBufferBoundaryObserver;
struct StreamIoBoundaryObserver;
struct FileControllerBoundaryObserver;
struct FilePathBoundaryObserver;
struct RuntimeLifecycleBoundaryObserver;
struct CodeFragmentBoundaryObserver;
struct CppObjectStorageBoundaryObserver;
struct ExceptionRuntimeBoundaryObserver;
struct CompilerArithmeticBoundaryObserver;
struct RegisterContextBoundaryObserver;
struct StdlibCoreBoundaryObserver;
struct TimeRuntimeBoundaryObserver;
struct ImportDescriptorBoundaryObserver;
struct NativeServiceBoundaryObserver;
constexpr std::uint32_t return_sentinel = 0xfffffff0;
using Vector128 = std::array<std::uint8_t, 16>;

template <unsigned N> constexpr std::uint64_t mask() {
    static_assert(N > 0 && N <= 8);
    if constexpr (N == 8) return UINT64_MAX;
    else return (UINT64_C(1) << (N * 8)) - 1;
}
template <unsigned N> std::uint64_t read_be(const std::uint8_t* p) {
    static_assert(N > 0 && N <= 8);
    std::uint64_t value = 0;
    for (unsigned i = 0; i < N; ++i) value = (value << 8) | p[i];
    return value;
}
template <unsigned N> void write_be(std::uint8_t* p, std::uint64_t value) {
    static_assert(N > 0 && N <= 8);
    for (unsigned i = N; i > 0; --i) { p[i - 1] = value & 255; value >>= 8; }
}
template <unsigned N> std::int64_t signed_value(std::uint64_t value) {
    value &= mask<N>();
    if constexpr (N < 8) {
        if (value & (UINT64_C(1) << (8 * N - 1))) value |= ~mask<N>();
    }
    return std::bit_cast<std::int64_t>(value);
}
template <unsigned N> std::uint64_t shift_left(std::uint64_t a, std::uint64_t b) {
    return b >= N * 8 ? 0 : (a << b) & mask<N>();
}
template <unsigned N> std::uint64_t shift_right(std::uint64_t a, std::uint64_t b) {
    return b >= N * 8 ? 0 : (a & mask<N>()) >> b;
}
template <unsigned N> std::uint64_t shift_signed(std::uint64_t a, std::uint64_t b) {
    auto v = signed_value<N>(a);
    return std::uint64_t(b >= N * 8 ? (v < 0 ? -1 : 0) : v >> b) & mask<N>();
}
template <unsigned N> std::uint64_t carry(std::uint64_t a, std::uint64_t b) {
    return (a & mask<N>()) > mask<N>() - (b & mask<N>());
}
template <unsigned N> std::uint64_t leading_zeros(std::uint64_t a) {
    return std::countl_zero(a & mask<N>()) - (64 - N * 8);
}
inline std::uint64_t divide_unsigned(std::uint64_t a, std::uint64_t b) {
    if (!b) throw std::runtime_error("Integer division by zero in translated code");
    return a / b;
}
template <unsigned N> std::uint64_t divide_signed(std::uint64_t a, std::uint64_t b) {
    auto x = signed_value<N>(a), y = signed_value<N>(b);
    if (!y) throw std::runtime_error("Signed division by zero in translated code");
    if (x == std::numeric_limits<std::int64_t>::min() && y == -1)
        throw std::runtime_error("Signed division overflow in translated code");
    return std::uint64_t(x / y) & mask<N>();
}
template <unsigned N> auto float_value(std::uint64_t bits) {
    static_assert(N == 4 || N == 8);
    if constexpr (N == 4) return std::bit_cast<float>(std::uint32_t(bits));
    else return std::bit_cast<double>(bits);
}
template <unsigned N, typename T> std::uint64_t float_bits(T value) {
    static_assert(N == 4 || N == 8);
    if constexpr (N == 4) return std::bit_cast<std::uint32_t>(float(value));
    else return std::bit_cast<std::uint64_t>(double(value));
}
template <unsigned N, typename T> std::uint64_t float_truncate(T value) {
    // Exceptional conversions need a verified PowerPC policy, not host UB.
    const auto rounded = std::trunc(static_cast<long double>(value));
    const auto bound = std::ldexp(1.0L, N * 8 - 1);
    if (!std::isfinite(rounded) || rounded < -bound || rounded >= bound)
        throw std::runtime_error("Unimplemented exceptional float-to-int conversion");
    return std::uint64_t(std::int64_t(rounded)) & mask<N>();
}

// Optional private fixture observer for native-service effects and original
// RNG state writes. Original instructions and callbacks still execute normally.
struct MemoryObserver {
    virtual ~MemoryObserver() = default;
    virtual void mapped(std::uint32_t start, std::uint64_t size) = 0;
    virtual void written(std::uint32_t start, const std::uint8_t* bytes, std::size_t size) = 0;
    virtual void cleared(std::uint32_t start, std::uint64_t size) = 0;
    virtual void read(std::uint32_t, const std::uint8_t*, unsigned) {}
};
class Memory {
public:
    MemoryObserver* observer = nullptr;
    // Separate Date diagnostics survive import transcript observer switches.
    MemoryObserver* date_observer = nullptr;
    // Optional direct boundary diagnostics, independent of write transcripts.
    std::vector<std::pair<std::uint32_t, unsigned>>* read_trace = nullptr;
    Memory();
    ~Memory();
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;
    void map(std::uint32_t start, std::uint64_t size);
    void load(std::uint32_t start, const std::filesystem::path& file, std::uint64_t size);
    void copy_in(std::uint32_t start, const std::vector<std::uint8_t>& bytes);
    void clear(std::uint32_t start, std::uint64_t size);
    std::vector<std::uint8_t> copy_out(std::uint32_t start, std::size_t size) const;
    std::vector<std::pair<std::uint32_t, std::uint64_t>> mapped_ranges() const;
    Vector128 read_vector(std::uint64_t address) const;
    void write_vector(std::uint64_t address, const Vector128& value);
    template <unsigned N> std::uint64_t read(std::uint64_t address) const {
        check_scalar<N>(address);
        if (read_trace) read_trace->emplace_back(std::uint32_t(address), N);
        if (date_observer) date_observer->read(std::uint32_t(address), data_ + address, N);
        return read_be<N>(data_ + address);
    }
    template <unsigned N> void write(std::uint64_t address, std::uint64_t value) {
        check_scalar<N>(address);
        write_be<N>(data_ + address, value);
        if (observer) observer->written(std::uint32_t(address), data_ + address, N);
        if (date_observer) date_observer->written(std::uint32_t(address), data_ + address, N);
    }
private:
    template <unsigned N> void check_scalar(std::uint64_t address) const {
        static_assert(N > 0 && N <= 8);
        // An entry certifies the continuous interval from its 64 KiB boundary
        // to the exact mapped end. Partial starts and holes use the range oracle.
        if (address < (UINT64_C(1) << 32) &&
            address + N <= (*page_ends_)[address >> 16]) return;
        check(address, N);
    }
    void check(std::uint64_t address, std::uint64_t size) const;
    std::uint8_t* data_;
    struct Range { std::uint64_t start, end; };
    std::vector<Range> ranges_;
    std::unique_ptr<std::array<std::uint64_t, 65536>> page_ends_ =
        std::make_unique<std::array<std::uint64_t, 65536>>();
};

[[noreturn, gnu::cold, gnu::noinline]] void instruction_budget_exhausted();

struct State {
    Memory memory;
    // Ghidra's v0..v31 occupy 0x4200..0x43ff. The extension at 0x4400
    // holds VSCR, which this language exposes only through user operations.
    std::array<std::uint8_t, 0x4410> registers{};
    std::array<std::uint8_t, 0x45000> unique{};
    std::uint32_t pc = 0;
    std::uint64_t executed = 0, budget = 10000000;
    std::uint64_t native_calls = 0;
    // The original ABI stores pointers in 32-bit data fields. Allocations live
    // in reserved, non-executable data pages, independently of ARM64 pointers.
    std::map<std::uint32_t, std::uint32_t> allocated;
    std::map<std::uint32_t, std::uint32_t> available;
    std::uint32_t heap_end = 0x20000000;
    std::int16_t memory_error = 0;
    // Keep platform implementation details out of all generated units. Future
    // filesystem, window, event, and sound changes do not alter this ABI again.
    std::shared_ptr<NativePlatform> platform;
    // Generated register offsets are constants. Inlining lets the compiler
    // fold the bounds check and access them directly, retaining checks for
    // dynamic callers without an out-of-line call per register access.
    template <unsigned N> [[gnu::always_inline]] std::uint64_t r(std::size_t offset) const {
        if (offset + N > registers.size()) throw std::out_of_range("Register offset");
        return read_be<N>(registers.data() + offset);
    }
    template <unsigned N> [[gnu::always_inline]] void wr(std::size_t offset, std::uint64_t value) {
        if (offset + N > registers.size()) throw std::out_of_range("Register offset");
        write_be<N>(registers.data() + offset, value);
    }
    template <unsigned N> std::uint64_t u(std::size_t offset) const {
        if (offset + N > unique.size()) throw std::out_of_range("Temporary offset");
        return read_be<N>(unique.data() + offset);
    }
    template <unsigned N> void wu(std::size_t offset, std::uint64_t value) {
        if (offset + N > unique.size()) throw std::out_of_range("Temporary offset");
        write_be<N>(unique.data() + offset, value);
    }
    Vector128 read_vector_register(std::size_t offset) const;
    void write_vector_register(std::size_t offset, const Vector128& value);
    Vector128 read_vector_temporary(std::size_t offset) const;
    void write_vector_temporary(std::size_t offset, const Vector128& value);
    bool calendar_original = false;
    std::array<std::uint64_t, 2> calendar_calls{};
    std::uint64_t calendar_exceptional_calls = 0;
    // Final Date diagnostic layout. These are never serialized in saves or
    // private runtime snapshots; normal production has no read observer.
    bool date_subsystem_original = false;
    std::array<bool,26> date_entry_original{};
    std::array<std::uint64_t,26> date_subsystem_calls{},date_original_calls{};
    std::uint32_t date_original_resume = 0, calendar_original_resume = 0;
    // Private baseline selections; production uses all 34 semantic entries.
    bool date_difference_original = false;
    std::uint64_t date_difference_calls = 0;
    bool date_ordering_original = false;
    std::uint64_t date_ordering_calls = 0;
    bool date_before_original = false;
    std::uint64_t date_before_calls = 0;
    bool date_month_original = false;
    std::uint64_t date_month_calls = 0;
    bool date_day_of_month_original = false;
    std::uint64_t date_day_of_month_calls = 0;
    bool date_weekday_original = false;
    std::uint64_t date_weekday_calls = 0;
    // RNG diagnostics are process-local, never serialized in game saves or
    // machine snapshots. Keep the complete layout together for one rebuild.
    bool rng_original = false, rng_trace = false;
    std::array<bool, 3> rng_entry_original{};
    std::array<std::uint64_t, 3> rng_calls{}, rng_original_calls{};
    std::uint32_t rng_original_resume = 0;
    std::uint64_t rng_ordered_digest = UINT64_C(14695981039346656037);
    RngBoundaryObserver* rng_observer = nullptr;
    // Background selection diagnostics are optional, process-local, and never
    // serialized. They do not replace a memory or drawing observer.
    bool background_original = false;
    std::array<bool, 4> background_entry_original{};
    std::array<std::uint64_t, 4> background_calls{}, background_original_calls{};
    std::uint32_t background_original_resume = 0;
    BackgroundBoundaryObserver* background_observer = nullptr;
    // Collection lookup diagnostics are private, process-local, and never
    // serialized. Observing them does not replace memory or drawing observers.
    bool collection_original = false;
    std::array<bool, 5> collection_entry_original{};
    std::array<std::uint64_t, 5> collection_calls{}, collection_original_calls{};
    std::uint32_t collection_original_resume = 0;
    CollectionBoundaryObserver* collection_observer = nullptr;
    // Optional string-core diagnostics are process-local, never serialized.
    bool string_original = false;
    std::array<bool, 14> string_entry_original{};
    std::array<std::uint64_t, 14> string_calls{}, string_original_calls{};
    std::uint32_t string_original_resume = 0;
    StringBoundaryObserver* string_observer = nullptr;
    // Optional memory-primitive diagnostics are process-local, never serialized.
    bool memory_primitives_original = false;
    std::array<bool, 6> memory_primitives_entry_original{};
    std::array<std::uint64_t, 6> memory_primitives_calls{}, memory_primitives_original_calls{};
    std::uint32_t memory_primitives_original_resume = 0;
    MemoryPrimitivesBoundaryObserver* memory_primitives_observer = nullptr;
    // Optional integer-parser diagnostics are process-local, never serialized.
    bool integer_parsing_original = false;
    std::array<bool, 5> integer_parsing_entry_original{};
    std::array<std::uint64_t, 5> integer_parsing_calls{}, integer_parsing_original_calls{};
    std::array<std::uint64_t, 2> integer_parsing_phase_escapes{};
    std::uint32_t integer_parsing_original_resume = 0;
    IntegerParsingBoundaryObserver* integer_parsing_observer = nullptr;
    // Private formatted-input diagnostics; never serialized or bundled as settings.
    bool formatted_input_original = false;
    std::array<bool, 7> formatted_input_entry_original{};
    std::array<std::uint64_t, 7> formatted_input_calls{}, formatted_input_original_calls{};
    std::uint64_t formatted_input_dispatch_escapes = 0;
    std::uint32_t formatted_input_original_resume = 0;
    FormattedInputBoundaryObserver* formatted_input_observer = nullptr;
    // Optional diagnostics for the three related text/numeric chains. Never serialized.
    bool formatted_output_original = false;
    std::array<bool, 15> formatted_output_entry_original{};
    std::array<std::uint64_t, 15> formatted_output_calls{}, formatted_output_original_calls{};
    std::uint64_t formatted_output_dispatch_escapes = 0, formatted_output_shared_tail_calls = 0;
    std::uint32_t formatted_output_original_resume = 0;
    FormattedOutputBoundaryObserver* formatted_output_observer = nullptr;
    bool character_conversion_original = false;
    std::array<bool, 6> character_conversion_entry_original{};
    std::array<std::uint64_t, 6> character_conversion_calls{}, character_conversion_original_calls{};
    std::uint32_t character_conversion_original_resume = 0;
    CharacterConversionBoundaryObserver* character_conversion_observer = nullptr;
    bool floating_input_original = false;
    std::array<std::uint64_t, 1> floating_input_calls{}, floating_input_original_calls{};
    std::uint64_t floating_input_dispatch_escapes = 0;
    std::uint32_t floating_input_original_resume = 0;
    FloatingInputBoundaryObserver* floating_input_observer = nullptr;
    // Allocator selection, counters and effects are private, never serialized.
    bool allocator_original = false;
    std::array<bool,27> allocator_entry_original{};
    std::array<std::uint64_t,27> allocator_calls{}, allocator_original_calls{};
    std::uint32_t allocator_original_resume = 0;
    std::uint64_t allocator_dispatch_escapes = 0;
    AllocatorBoundaryObserver* allocator_observer = nullptr;
    // Stream storage diagnostics are process-local and never serialized.
    bool stream_buffer_original = false;
    std::array<bool,13> stream_buffer_entry_original{};
    std::array<std::uint64_t,13> stream_buffer_calls{}, stream_buffer_original_calls{};
    std::uint32_t stream_buffer_original_resume = 0;
    std::uint64_t stream_buffer_dispatch_escapes = 0;
    StreamBufferBoundaryObserver* stream_buffer_observer = nullptr;
    // Buffered item I/O diagnostics remain private and process-local.
    bool stream_io_original = false;
    std::array<bool,9> stream_io_entry_original{};
    std::array<std::uint64_t,9> stream_io_calls{}, stream_io_original_calls{};
    std::uint32_t stream_io_original_resume = 0;
    std::uint64_t stream_io_dispatch_escapes = 0;
    StreamIoBoundaryObserver* stream_io_observer = nullptr;
    // File-controller selection and fixtures are process-local, never saved.
    bool file_path_original = false;
    std::array<bool, 7> file_path_entry_original{};
    std::array<std::uint64_t, 7> file_path_calls{}, file_path_original_calls{};
    std::uint32_t file_path_original_resume = 0;
    std::uint64_t file_path_dispatch_escapes = 0;
    FilePathBoundaryObserver* file_path_observer = nullptr;
    // Lifecycle diagnostics describe both returns and successful termination;
    // they remain process-local and are never saved into the guest program.
    bool runtime_lifecycle_original = false;
    std::array<bool, 6> runtime_lifecycle_entry_original{};
    std::array<std::uint64_t, 6> runtime_lifecycle_calls{}, runtime_lifecycle_original_calls{};
    std::array<std::uint64_t, 6> runtime_lifecycle_returns{}, runtime_lifecycle_terminations{};
    std::uint32_t runtime_lifecycle_original_resume = 0;
    std::uint64_t runtime_lifecycle_dispatch_escapes = 0;
    RuntimeLifecycleBoundaryObserver* runtime_lifecycle_observer = nullptr;
    // Code-fragment registry diagnostics remain private and unpersisted.
    bool code_fragment_original = false;
    std::array<bool, 5> code_fragment_entry_original{};
    std::array<std::uint64_t, 5> code_fragment_calls{}, code_fragment_original_calls{};
    std::uint32_t code_fragment_original_resume = 0;
    std::uint64_t code_fragment_dispatch_escapes = 0;
    CodeFragmentBoundaryObserver* code_fragment_observer = nullptr;
    // C++ object-storage diagnostics remain private and unpersisted.
    bool cpp_object_storage_original = false;
    std::array<bool, 12> cpp_object_storage_entry_original{};
    std::array<std::uint64_t, 12> cpp_object_storage_calls{}, cpp_object_storage_original_calls{};
    std::array<std::uint64_t, 12> cpp_object_storage_returns{}, cpp_object_storage_terminations{};
    std::uint32_t cpp_object_storage_original_resume = 0;
    std::uint64_t cpp_object_storage_dispatch_escapes = 0;
    CppObjectStorageBoundaryObserver* cpp_object_storage_observer = nullptr;
    // Exception diagnostics are process-local, separate from guest contexts.
    bool exception_runtime_original = false;
    std::array<bool, 21> exception_runtime_entry_original{};
    std::array<std::uint64_t, 21> exception_runtime_calls{}, exception_runtime_original_calls{};
    std::array<std::uint64_t, 21> exception_runtime_returns{}, exception_runtime_transfers{}, exception_runtime_terminations{};
    std::uint32_t exception_runtime_original_resume = 0;
    std::uint64_t exception_runtime_dispatch_escapes = 0, exception_runtime_unknown_transfers = 0;
    std::uint64_t exception_runtime_landings = 0, exception_runtime_original_landings = 0;
    std::uint64_t exception_runtime_landing_returns = 0, exception_runtime_landing_transfers = 0, exception_runtime_landing_terminations = 0;
    ExceptionRuntimeBoundaryObserver* exception_runtime_observer = nullptr;
    bool file_controller_original = false;
    std::array<bool, 26> file_controller_entry_original{};
    std::array<std::uint64_t, 26> file_controller_calls{}, file_controller_original_calls{};
    std::uint32_t file_controller_original_resume = 0;
    std::uint64_t file_controller_dispatch_escapes = 0;
    FileControllerBoundaryObserver* file_controller_observer = nullptr;
    // Compiler arithmetic diagnostics are process-local and unpersisted.
    bool compiler_arithmetic_original = false;
    std::array<bool, 8> compiler_arithmetic_entry_original{};
    std::array<std::uint64_t, 8> compiler_arithmetic_calls{}, compiler_arithmetic_original_calls{};
    std::array<std::uint64_t, 8> compiler_arithmetic_returns{}, compiler_arithmetic_faults{};
    std::array<std::uint64_t, 8> compiler_arithmetic_budget_original_calls{}, compiler_arithmetic_profile_original_calls{};
    std::uint32_t compiler_arithmetic_original_resume = 0;
    std::uint64_t compiler_arithmetic_dispatch_escapes = 0;
    CompilerArithmeticBoundaryObserver* compiler_arithmetic_observer = nullptr;
    // Register-context diagnostics never enter guest buffers or save files.
    bool register_context_original = false;
    std::array<bool, 30> register_context_entry_original{};
    std::array<std::uint64_t, 30> register_context_calls{}, register_context_original_calls{};
    std::array<std::uint64_t, 30> register_context_returns{}, register_context_transfers{}, register_context_faults{};
    std::array<std::uint64_t, 30> register_context_budget_original_calls{}, register_context_profile_original_calls{};
    std::uint32_t register_context_original_body = 0;
    std::uint64_t register_context_dispatch_escapes = 0, register_context_unknown_transfers = 0;
    RegisterContextBoundaryObserver* register_context_observer = nullptr;
    // Word utilities and comparator-search diagnostics remain process-local.
    bool stdlib_core_original = false;
    std::array<bool, 4> stdlib_core_entry_original{};
    std::array<std::uint64_t, 4> stdlib_core_calls{}, stdlib_core_original_calls{};
    std::array<std::uint64_t, 4> stdlib_core_returns{}, stdlib_core_transfers{}, stdlib_core_faults{};
    std::array<std::uint64_t, 4> stdlib_core_budget_original_calls{}, stdlib_core_profile_original_calls{};
    std::array<std::uint64_t, 4> stdlib_core_budget_resumes{}, stdlib_core_budget_returns{};
    std::uint32_t stdlib_core_original_body = 0;
    std::uint64_t stdlib_core_dispatch_escapes = 0;
    StdlibCoreBoundaryObserver* stdlib_core_observer = nullptr;
    // Mac clock/calendar diagnostics are process-local and never persisted.
    bool time_runtime_original = false;
    std::array<bool, 8> time_runtime_entry_original{};
    std::array<std::uint64_t, 8> time_runtime_calls{}, time_runtime_original_calls{};
    std::array<std::uint64_t, 8> time_runtime_returns{}, time_runtime_transfers{}, time_runtime_faults{};
    std::array<std::uint64_t, 8> time_runtime_budget_original_calls{}, time_runtime_profile_original_calls{};
    std::array<std::uint64_t, 8> time_runtime_budget_resumes{}, time_runtime_budget_returns{};
    std::uint32_t time_runtime_original_body = 0;
    std::uint64_t time_runtime_dispatch_escapes = 0;
    TimeRuntimeBoundaryObserver* time_runtime_observer = nullptr;
    // Live import and CFM descriptor diagnostics are process-local and unsaved.
    bool import_descriptor_original = false;
    std::array<bool, 260> import_descriptor_entry_original{};
    std::array<std::uint64_t, 260> import_descriptor_calls{}, import_descriptor_original_calls{};
    std::array<std::uint64_t, 260> import_descriptor_tails{}, import_descriptor_faults{};
    std::array<std::uint64_t, 260> import_descriptor_budget_stops{}, import_descriptor_transfers{};
    std::array<std::uint64_t, 260> import_descriptor_budget_original_calls{}, import_descriptor_profile_original_calls{};
    std::uint32_t import_descriptor_original_body = 0;
    std::uint64_t import_descriptor_dispatch_escapes = 0;
    ImportDescriptorBoundaryObserver* import_descriptor_observer = nullptr;
    // Framed native-service diagnostics are process-local and never persisted.
    bool native_service_original = false;
    std::array<bool, 3> native_service_entry_original{};
    std::array<std::uint64_t, 3> native_service_calls{}, native_service_original_calls{};
    std::array<std::uint64_t, 3> native_service_returns{}, native_service_faults{};
    std::array<std::uint64_t, 3> native_service_budget_stops{}, native_service_transfers{};
    std::array<std::uint64_t, 3> native_service_profile_original_calls{};
    std::uint32_t native_service_original_body = 0;
    std::uint64_t native_service_dispatch_escapes = 0;
    NativeServiceBoundaryObserver* native_service_observer = nullptr;
    void step(std::uint32_t address) {
        pc = address;
        if (executed++ >= budget) instruction_budget_exhausted();
    }
};

// Definitions are emitted by the build-time compiler for selected original code.
bool dispatch_page(State& state);
const char* import_name(std::uint32_t address);
void load_program(State& state, const std::filesystem::path& directory);
void run(State& state);
// Execute a direct dependency call without callback argument marshalling or
// restoration. Imports, callbacks and transcript boundaries use the same path.
void run_until(State& state, std::uint32_t return_pc);
bool bridge_import(State& state, const std::string& name);
std::uint32_t allocate(State& state, std::uint32_t size, bool clear);
void deallocate(State& state, std::uint32_t address);
std::string hex_address(std::uint32_t value);
}

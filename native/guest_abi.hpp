#pragma once
// The original program's calling convention, for routines rewritten as plain
// C++ (CodeWarrior PowerPC, CFM). A rewrite computes with C++ values, but
// leaves what the game can observe exactly as the original does:
// - guest memory, through the checked big-endian accessors, so memory
//   observers see every write;
// - the stack image: the original's prologue stores (saved registers, LR,
//   back chain) and its stack locals, because the game later reads stack it
//   never wrote (docs/aot-port.md, "Uninitialised stack reads");
// - the registers its callers may read: r1, r2, r13-r31, f14-f31, cr2-cr4,
//   and the return registers r3 (r4 for 64-bit results) and f1.
// Calls into routines that still run generated code pass the arguments and
// the non-volatile registers the original holds at that call, because the
// callee's prologue saves those into the stack image too.
#include "aot_runtime.hpp"
#include "guest_context_transfer.hpp"
#include "guest_register_frame.hpp"
#include "owner_in_place.hpp"
#include <atomic>
#include <bit>
#include <cstdio>
#include <cstring>
#include <cxxabi.h>
#include <utility>
#include <exception>
#include <initializer_list>
#include <stdexcept>

// libc++abi's own declaration (cxa_exception.h, and <cxxabi.h> in newer
// libc++abi releases); the macOS SDK's <cxxabi.h> does not carry it. The
// record stays opaque here, as there, so either declaration can come first.
namespace __cxxabiv1 {
struct __cxa_eh_globals;
extern "C" __cxa_eh_globals* __cxa_get_globals();
}

namespace cm::aot::guest {
// Ghidra's PowerPC register file (program.json "registers").
namespace reg {
constexpr std::size_t gpr(unsigned n) { return n * 4; }
constexpr std::size_t fpr(unsigned n) { return 0x4000 + n * 16; }
constexpr std::size_t cr(unsigned field) { return 2304 + field; }  // one 4-bit field per byte
constexpr std::size_t xer_so = 1024, xer_ov = 1025, xer_ca = 1027;
constexpr std::size_t r2_save = 2440;  // Ghidra's TOC save at each bl (not memory)
constexpr std::size_t lr = 4128, ctr = 4132;
}

class Cpu {
public:
    explicit Cpu(State& state) : s(state) {}
    State& s;

    // Registers.
    std::uint32_t r(unsigned n) const { return std::uint32_t(s.r<4>(reg::gpr(n))); }
    void set(unsigned n, std::uint32_t value) { s.wr<4>(reg::gpr(n), value); }
    void set(std::initializer_list<std::pair<unsigned, std::uint32_t>> values) {
        for (const auto& [n, value] : values) set(n, value);
    }
    double f(unsigned n) const { return std::bit_cast<double>(s.r<8>(reg::fpr(n))); }
    void set_f(unsigned n, double value) { s.wr<8>(reg::fpr(n), std::bit_cast<std::uint64_t>(value)); }
    std::uint32_t sp() const { return r(1); }
    std::uint32_t lr() const { return std::uint32_t(s.r<4>(reg::lr)); }
    void set_lr(std::uint32_t value) { s.wr<4>(reg::lr, value); }
    std::uint8_t cr(unsigned field) const { return std::uint8_t(s.r<1>(reg::cr(field))); }
    void set_cr(unsigned field, std::uint8_t value) { s.wr<1>(reg::cr(field), value); }
    // A compare into a CR field, as cmpw/cmpwi (signed) or cmplw/cmplwi
    // (logical) leave it: LT 8, GT 4, EQ 2, plus XER[SO]. Record forms
    // (`extsb.`, `rlwinm.`, ...) compare their result with 0 into cr0.
    void compare(unsigned field, std::int32_t a, std::int32_t b) {
        set_cr(field, std::uint8_t((a < b ? 8 : a > b ? 4 : 2) | (s.r<1>(reg::xer_so) & 1)));
    }
    void compare_logical(unsigned field, std::uint32_t a, std::uint32_t b) {
        set_cr(field, std::uint8_t((a < b ? 8 : a > b ? 4 : 2) | (s.r<1>(reg::xer_so) & 1)));
    }
    // A word of the table of contents: `lwz rX,offset(r2)`.
    std::uint32_t toc(std::int32_t offset) const { return u32(r(2) + std::uint32_t(offset)); }

    // Guest memory (big-endian).
    std::uint8_t u8(std::uint32_t a) const { return std::uint8_t(s.memory.read<1>(a)); }
    std::int8_t i8(std::uint32_t a) const { return std::int8_t(u8(a)); }
    std::uint16_t u16(std::uint32_t a) const { return std::uint16_t(s.memory.read<2>(a)); }
    std::int16_t i16(std::uint32_t a) const { return std::int16_t(u16(a)); }
    std::uint32_t u32(std::uint32_t a) const { return std::uint32_t(s.memory.read<4>(a)); }
    std::int32_t i32(std::uint32_t a) const { return std::int32_t(u32(a)); }
    std::uint64_t u64(std::uint32_t a) const { return s.memory.read<8>(a); }
    float f32(std::uint32_t a) const { return std::bit_cast<float>(u32(a)); }
    double f64(std::uint32_t a) const { return std::bit_cast<double>(u64(a)); }
    void put8(std::uint32_t a, std::uint32_t v) { s.memory.write<1>(a, v & 0xff); }
    void put16(std::uint32_t a, std::uint32_t v) { s.memory.write<2>(a, v & 0xffff); }
    void put32(std::uint32_t a, std::uint32_t v) { s.memory.write<4>(a, v); }
    void put64(std::uint32_t a, std::uint64_t v) { s.memory.write<8>(a, v); }
    void put_f32(std::uint32_t a, float v) { put32(a, std::bit_cast<std::uint32_t>(v)); }
    void put_f64(std::uint32_t a, double v) { put64(a, std::bit_cast<std::uint64_t>(v)); }

    // `bl target` at the original call site whose next instruction is
    // `return_address`: runs the callee (generated code or another owner) and
    // returns when it does. The caller sets the arguments and the
    // non-volatile registers first.
    void call(std::uint32_t target, std::uint32_t return_address) { guest_call(target, return_address, run_until); }
    // The same, except that another module's C++ owner at `target` runs in
    // place of its page (owner_in_place.hpp); for callers without check hooks.
    void call_in_place(std::uint32_t target, std::uint32_t return_address) {
        guest_call(target, return_address, run_owner_until);
    }
    // `bl` to a routine rewritten in C++, called directly: only the link
    // register, which the callee's prologue saves.
    void link(std::uint32_t return_address) {
        s.wr<4>(reg::r2_save, r(2));
        set_lr(return_address);
    }
    // `blr`: the hook's caller continues at the link register.
    void finish() { s.pc = lr(); }

private:
    void guest_call(std::uint32_t target, std::uint32_t return_address, void (*run)(State&, std::uint32_t)) {
        s.wr<4>(reg::r2_save, r(2));
        set_lr(return_address);
        s.pc = target;
        const auto stack = sp();
        run(s, return_address);
        // A rewrite never declines, so its original body (where the return
        // address lies) runs nowhere else; a mismatch is a wiring error.
        if (sp() != stack) throw std::logic_error("Guest call returned on a different stack: " + hex_address(target));
    }
};

// The stack image of a CodeWarrior prologue and the state its epilogue
// restores. The prologue stores, relative to the caller's stack pointer:
// GPRs first..31 at gpr_offset (stmw layout), FPRs first..31 at fpr_offset,
// LR at +8 and CR at +4 when the routine saves them, and the back chain at
// the new stack pointer. The epilogue reloads the saved registers from those
// slots (whatever they hold by then), pops the frame and restores LR.
struct Saves {
    unsigned first_gpr = 32;          // 32: none
    std::int32_t gpr_offset = 0;      // default: -(32 - first_gpr) * 4 - 8 * fprs
    unsigned first_fpr = 32;
    std::int32_t fpr_offset = 0;      // default: -(32 - first_fpr) * 8
    bool lr = true, cr = false;
};
namespace detail {
// A register span of `words` words (1-32). A variable-size memcpy is a libc
// call; each case is a constant-size copy the compiler emits inline.
template <std::size_t... N>
inline void copy_register_words(std::uint8_t* to, const std::uint8_t* from, unsigned words, std::index_sequence<N...>) {
    (void)((words == N + 1 ? (std::memcpy(to, from, (N + 1) * 4), true) : false) || ...);
}
inline void copy_register_words(std::uint8_t* to, const std::uint8_t* from, unsigned words) {
    copy_register_words(to, from, words, std::make_index_sequence<32>{});
}
// The count std::uncaught_exceptions() returns, read in place. libc++abi
// keeps it in the thread's __cxa_eh_globals {caughtExceptions,
// uncaughtExceptions}; __cxa_get_globals allocates that record on the
// thread's first use and frees it only when the thread exits, so its address
// is looked up once per thread instead of through three library calls per
// read. The layout is libc++abi's, not a published ABI: the first lookup in
// the process proves that the field tracks std::uncaught_exceptions(), and if
// it does not, every frame asks std::uncaught_exceptions() as before.
struct ItaniumExceptionGlobals {
    void* caught_exceptions;
    unsigned int uncaught_exceptions;
};
inline const unsigned int* uncaught_exceptions_field() {
    return &reinterpret_cast<const ItaniumExceptionGlobals*>(__cxxabiv1::__cxa_get_globals())->uncaught_exceptions;
}
// Whether `read()` equals std::uncaught_exceptions() at the current depth d,
// and at d + 1 inside a destructor that a thrown exception runs, and at d
// again once it is caught. One throw and catch.
template <class Read> bool uncaught_count_tracks(Read read) noexcept {
    struct During {
        Read& read;
        bool& tracks;
        int depth;
        ~During() { tracks = tracks && std::uncaught_exceptions() == depth + 1 && int(read()) == depth + 1; }
    };
    const int depth = std::uncaught_exceptions();
    bool tracks = int(read()) == depth;
    try {
        During during{read, tracks, depth};
        throw 0;
    } catch (int) {}
    return tracks && int(read()) == depth;
}
inline bool uncaught_exceptions_field_tracks() noexcept {
    const auto* field = uncaught_exceptions_field();
    return uncaught_count_tracks([field] { return *field; });
}
enum class UncaughtCount : unsigned char { unchecked, in_place, standard };
// Decided once per process; a checker may set `standard`, or reset it with
// another `uncaught_count_check`, to exercise the fallback.
inline constinit std::atomic<UncaughtCount> uncaught_count_mode{UncaughtCount::unchecked};
inline constinit bool (*uncaught_count_check)() noexcept = uncaught_exceptions_field_tracks;
inline thread_local constinit const unsigned int* uncaught_exception_count = nullptr;
[[gnu::cold, gnu::noinline]] inline const unsigned int* find_uncaught_exception_count() noexcept {
    auto mode = uncaught_count_mode.load(std::memory_order_acquire);
    if (mode == UncaughtCount::unchecked) {
        const auto decided = uncaught_count_check() ? UncaughtCount::in_place : UncaughtCount::standard;
        if (uncaught_count_mode.compare_exchange_strong(mode, decided, std::memory_order_acq_rel)) {
            mode = decided;
            if (decided == UncaughtCount::standard)
                std::fputs("cm: the C++ runtime's uncaught-exception count is not where expected; "
                           "guest frames call std::uncaught_exceptions()\n", stderr);
        }
    }
    if (mode != UncaughtCount::in_place) return nullptr;
    return uncaught_exception_count = uncaught_exceptions_field();
}
// The thread's count, or null where frames must call std::uncaught_exceptions().
inline const unsigned int* uncaught_exceptions_counter() noexcept {
    const auto* count = uncaught_exception_count;
    return count ? count : find_uncaught_exception_count();
}
inline unsigned int uncaught_exceptions(const unsigned int* counter) noexcept {
    return counter ? *counter : unsigned(std::uncaught_exceptions());
}
}
class Frame {
public:
    // Inline at each of its ~2,100 sites, where the save layout is a constant:
    // the tests of it and the copy sizes fold away. The scalar fallbacks stay
    // out of line.
    [[gnu::always_inline]] Frame(Cpu& cpu, std::uint32_t entry, std::uint32_t size, Saves saves)
        : c(cpu), caller_sp(cpu.sp()), saves_(normalized(saves)), scope(cpu.s, entry) {
        // Register file and stack hold the same big-endian bytes. Without an
        // interested write observer or Date diagnostics, a certified save
        // area is copied directly: GPRs in one span, FPRs one at a time
        // (16-byte slots in the register file, 8 on the stack). Otherwise the
        // scalar stores keep callbacks, their order and partial faults.
        // The layout is read from the argument: the member's address has
        // escaped into the native scope chain, so only the argument folds.
        const auto layout = normalized(saves);
        const auto caller = caller_sp;
        if (layout.first_fpr < 32) {
            const auto start = caller + std::uint32_t(layout.fpr_offset);
            const auto count = 32 - layout.first_fpr;
            if (auto* guest = GuestRegisterFrame::writable_span(c.s.memory, start, count * 8)) {
                for (unsigned i = 0; i < count; ++i)
                    std::memcpy(guest + i * 8, c.s.registers.data() + reg::fpr(layout.first_fpr + i), 8);
            } else {
                save_fprs(start, layout.first_fpr);
            }
        }
        if (layout.first_gpr < 32) {
            const auto start = caller + std::uint32_t(layout.gpr_offset);
            const auto words = 32 - layout.first_gpr;
            if (auto* guest = GuestRegisterFrame::writable_span(c.s.memory, start, words * 4)) {
                detail::copy_register_words(guest, c.s.registers.data() + reg::gpr(layout.first_gpr), words);
            } else {
                save_gprs(start, layout.first_gpr);
            }
        }
        if (layout.cr) c.put32(caller + 4, condition_register());
        if (layout.lr) c.put32(caller + 8, c.lr());
        base_ = caller - size;
        c.put32(base_, caller);
        c.set(1, base_);
        scope.frame(base_);
    }
    // Returning runs the epilogue; unwinding (a guest context transfer, a
    // fault) leaves the guest state as it is.
    // Unwinding is told apart as std::uncaught_exceptions() does, from the
    // thread's count (detail::uncaught_exceptions_counter). The pointer is
    // this thread's and outlives the frame: a frame is a stack object of the
    // thread that built it, and no thread-exit destructor in this code base
    // runs guest code (every thread_local here is a plain pointer).
    ~Frame() { if (!left_ && detail::uncaught_exceptions(uncaught_) == exceptions_) leave(); }
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    // The new stack pointer, and the address of a local at sp+offset.
    std::uint32_t base() const { return base_; }
    std::uint32_t at(std::int32_t offset) const { return base_ + std::uint32_t(offset); }
    std::uint32_t caller() const { return caller_sp; }
    // The epilogue. Nothing happens on unwinding: a guest context transfer
    // abandons this invocation with the guest state the transfer left.
    void leave() {
        left_ = true;
        if (saves_.first_gpr < 32) {
            const auto start = caller_sp + std::uint32_t(saves_.gpr_offset);
            // The stack save area and register file have the same byte order.
            // Without read diagnostics a certified range is copied; otherwise
            // the scalar reads keep their callbacks, including writes to
            // registers restored before a fault.
            const auto words = 32 - saves_.first_gpr;
            if (const auto* guest = GuestRegisterFrame::readable_span(c.s.memory, start, words * 4)) {
                detail::copy_register_words(c.s.registers.data() + reg::gpr(saves_.first_gpr), guest, words);
            } else {
                for (unsigned n = saves_.first_gpr; n < 32; ++n)
                    c.set(n, c.u32(start + (n - saves_.first_gpr) * 4));
            }
        }
        if (saves_.first_fpr < 32) {
            const auto start = caller_sp + std::uint32_t(saves_.fpr_offset);
            const auto count = 32 - saves_.first_fpr;
            if (const auto* guest = GuestRegisterFrame::readable_span(c.s.memory, start, count * 8)) {
                for (unsigned i = 0; i < count; ++i)
                    std::memcpy(c.s.registers.data() + reg::fpr(saves_.first_fpr + i), guest + i * 8, 8);
            } else {
                for (unsigned n = saves_.first_fpr; n < 32; ++n)
                    c.s.wr<8>(reg::fpr(n), c.u64(start + (n - saves_.first_fpr) * 8));
            }
        }
        if (saves_.cr) {
            // mtcrf 0x38: only cr2-cr4 come back.
            const auto word = c.u32(caller_sp + 4);
            for (unsigned field = 2; field <= 4; ++field) c.set_cr(field, (word >> (28 - 4 * field)) & 0xf);
        }
        // `lwz r0,size+8(r1); mtlr r0`: r0 is left holding the return address.
        if (saves_.lr) { c.set(0, c.u32(caller_sp + 8)); c.set_lr(c.r(0)); }
        c.set(1, caller_sp);
    }

private:
    [[gnu::noinline, gnu::cold]] void save_fprs(std::uint32_t start, unsigned first) {
        for (unsigned n = first; n < 32; ++n) c.put64(start + (n - first) * 8, c.s.r<8>(reg::fpr(n)));
    }
    [[gnu::noinline, gnu::cold]] void save_gprs(std::uint32_t start, unsigned first) {
        for (unsigned n = first; n < 32; ++n) c.put32(start + (n - first) * 4, c.r(n));
    }
    static Saves normalized(Saves saves) {
        const auto fprs = std::int32_t(32 - saves.first_fpr);
        if (saves.first_fpr < 32 && !saves.fpr_offset) saves.fpr_offset = -fprs * 8;
        if (saves.first_gpr < 32 && !saves.gpr_offset) saves.gpr_offset = -std::int32_t(32 - saves.first_gpr) * 4 - fprs * 8;
        return saves;
    }
    std::uint32_t condition_register() const {
        std::uint32_t word = 0;
        for (unsigned field = 0; field < 8; ++field) word |= std::uint32_t(c.cr(field) & 0xf) << (28 - 4 * field);
        return word;
    }
    Cpu& c;
    std::uint32_t caller_sp, base_ = 0;
    Saves saves_;
    GuestNativeScope scope;
    const unsigned int* uncaught_ = detail::uncaught_exceptions_counter();
    unsigned int exceptions_ = detail::uncaught_exceptions(uncaught_);
    bool left_ = false;
};

// The original's assertion report, as CodeWarrior expanded it at every site
// of the game's assert macro: split the source file's name into two stack
// buffers (0x10739890), take the version text (0x10768810), format
// "v%s %s.%s %d" into the shared message buffer (0x1077b720), show it
// (0x10769b60), and clear the shared success flag. `at` is the address of
// the first call; the others follow at fixed distances. The caller sets the
// non-volatile registers the original holds there first. `call(entry,
// back)` makes each call once its registers are set (default: Cpu::call).
struct Assertion {
    std::uint32_t at;
    std::uint32_t file, format, title;  // strings in the module's pool
    std::uint32_t name, extension;      // the stack buffers
    std::uint32_t line;
};
template <class Call>
void report_assertion(Cpu& c, const Assertion& a, Call&& call) {
    const auto buffer = c.toc(-0x7ba8);
    c.set({{3, a.file}, {4, 0}, {5, 0}, {6, a.name}, {7, a.extension}});
    call(0x10739890, a.at + 4);
    call(0x10768810, a.at + 8);
    c.set({{5, c.r(3)}, {3, buffer}, {4, a.format}, {6, a.name}, {7, a.extension}, {8, a.line}});
    call(0x1077b720, a.at + 0x24);
    c.set({{4, buffer}, {3, a.title}, {5, 0}});
    call(0x10769b60, a.at + 0x34);
    c.put8(c.toc(-0x7bac), 0);
}
inline void report_assertion(Cpu& c, const Assertion& a) {
    report_assertion(c, a, [&](std::uint32_t entry, std::uint32_t back) { c.call(entry, back); });
}
}

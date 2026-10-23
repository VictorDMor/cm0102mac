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
#include <bit>
#include <exception>
#include <initializer_list>
#include <stdexcept>

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
    void call(std::uint32_t target, std::uint32_t return_address) {
        s.wr<4>(reg::r2_save, r(2));
        set_lr(return_address);
        s.pc = target;
        const auto stack = sp();
        run_until(s, return_address);
        // A rewrite never declines, so its original body (where the return
        // address lies) runs nowhere else; a mismatch is a wiring error.
        if (sp() != stack) throw std::logic_error("Guest call returned on a different stack: " + hex_address(target));
    }
    // `bl` to a routine rewritten in C++, called directly: only the link
    // register, which the callee's prologue saves.
    void link(std::uint32_t return_address) {
        s.wr<4>(reg::r2_save, r(2));
        set_lr(return_address);
    }
    // `blr`: the hook's caller continues at the link register.
    void finish() { s.pc = lr(); }
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
class Frame {
public:
    Frame(Cpu& cpu, std::uint32_t entry, std::uint32_t size, Saves saves)
        : c(cpu), caller_sp(cpu.sp()), saves_(normalized(saves)), scope(cpu.s, entry) {
        for (unsigned n = saves_.first_fpr; n < 32; ++n)
            c.put64(caller_sp + std::uint32_t(saves_.fpr_offset) + (n - saves_.first_fpr) * 8, c.s.r<8>(reg::fpr(n)));
        for (unsigned n = saves_.first_gpr; n < 32; ++n)
            c.put32(caller_sp + std::uint32_t(saves_.gpr_offset) + (n - saves_.first_gpr) * 4, c.r(n));
        if (saves_.cr) c.put32(caller_sp + 4, condition_register());
        if (saves_.lr) c.put32(caller_sp + 8, c.lr());
        base_ = caller_sp - size;
        c.put32(base_, caller_sp);
        c.set(1, base_);
        scope.frame(base_);
    }
    // Returning runs the epilogue; unwinding (a guest context transfer, a
    // fault) leaves the guest state as it is.
    ~Frame() { if (!left_ && std::uncaught_exceptions() == exceptions_) leave(); }
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
        for (unsigned n = saves_.first_gpr; n < 32; ++n)
            c.set(n, c.u32(caller_sp + std::uint32_t(saves_.gpr_offset) + (n - saves_.first_gpr) * 4));
        for (unsigned n = saves_.first_fpr; n < 32; ++n)
            c.s.wr<8>(reg::fpr(n), c.u64(caller_sp + std::uint32_t(saves_.fpr_offset) + (n - saves_.first_fpr) * 8));
        if (saves_.cr) {
            // mtcrf 0x38: only cr2-cr4 come back.
            const auto word = c.u32(caller_sp + 4);
            for (unsigned field = 2; field <= 4; ++field) c.set_cr(field, (word >> (28 - 4 * field)) & 0xf);
        }
        if (saves_.lr) c.set_lr(c.u32(caller_sp + 8));
        c.set(1, caller_sp);
    }

private:
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
    int exceptions_ = std::uncaught_exceptions();
    bool left_ = false;
};

// The original's assertion report, as CodeWarrior expanded it at every site
// of the game's assert macro: split the source file's name into two stack
// buffers (0x10739890), take the version text (0x10768810), format
// "v%s %s.%s %d" into the shared message buffer (0x1077b720), show it
// (0x10769b60), and clear the shared success flag. `at` is the address of
// the first call; the others follow at fixed distances. The caller sets the
// non-volatile registers the original holds there first.
struct Assertion {
    std::uint32_t at;
    std::uint32_t file, format, title;  // strings in the module's pool
    std::uint32_t name, extension;      // the stack buffers
    std::uint32_t line;
};
inline void report_assertion(Cpu& c, const Assertion& a) {
    const auto buffer = c.toc(-0x7ba8);
    c.set({{3, a.file}, {4, 0}, {5, 0}, {6, a.name}, {7, a.extension}});
    c.call(0x10739890, a.at + 4);
    c.call(0x10768810, a.at + 8);
    c.set({{5, c.r(3)}, {3, buffer}, {4, a.format}, {6, a.name}, {7, a.extension}, {8, a.line}});
    c.call(0x1077b720, a.at + 0x24);
    c.set({{4, buffer}, {3, a.title}, {5, 0}});
    c.call(0x10769b60, a.at + 0x34);
    c.put8(c.toc(-0x7bac), 0);
}
}

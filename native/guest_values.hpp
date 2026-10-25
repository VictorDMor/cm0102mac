#pragma once
// Typed values for routines rewritten as plain C++ (MAIN.CPP, PLAYER_STATS.CPP):
// views of guest memory, the original's compares, and the register state the
// game can observe, named. The routines compute with C++ values and leave
// what the game can observe as the original does (guest_abi.hpp): memory, the
// stack image, and the registers its callers and callees can read (r1, r2,
// r13-r31, the results, r4-r10 and the condition register). They keep, in
// r13-r31, the values the original keeps there across its calls: each
// callee's prologue saves them into the stack image.
#include "guest_abi.hpp"

namespace cm::aot::guest {
// A value in guest memory (big-endian), read and written as T.
template <class T> class Field {
public:
    Field(Cpu& c, std::uint32_t address) : c_(&c), address_(address) {}
    T get() const { return *this; }
    operator T() const {
        if constexpr (sizeof(T) == 1) return T(c_->u8(address_));
        else if constexpr (sizeof(T) == 2) return T(c_->u16(address_));
        else return T(c_->u32(address_));
    }
    // Writes the value (a view writes through even when const, like a reference).
    const Field& operator=(T value) const {
        if constexpr (sizeof(T) == 1) c_->put8(address_, std::uint32_t(value));
        else if constexpr (sizeof(T) == 2) c_->put16(address_, std::uint32_t(value));
        else c_->put32(address_, std::uint32_t(value));
        return *this;
    }
    // A view is not reassigned: writing a value takes a T.
    Field& operator=(const Field&) = delete;
    Field(const Field&) = default;
    std::uint32_t address() const { return address_; }

private:
    Cpu* c_;
    std::uint32_t address_;
};

// The original's compares into cr0 (cmpwi/cmpw, cmplwi/cmplw, and the record
// forms, which compare their result with 0). A callee that saves the
// condition register stores it whole, so each compare is made here too.
inline std::int32_t compare(Cpu& c, std::int32_t a, std::int32_t b = 0) {
    c.compare(0, a, b);
    return a < b ? -1 : a > b ? 1 : 0;
}
inline std::int32_t compare_logical(Cpu& c, std::uint32_t a, std::uint32_t b = 0) {
    c.compare_logical(0, a, b);
    return a < b ? -1 : a > b ? 1 : 0;
}
// A 16-bit value as the original holds it in a register (sign-extended).
inline std::uint32_t extended(std::int16_t value) { return std::uint32_t(std::int32_t(value)); }
// A local the original keeps in non-volatile register R (r13-r31) while it
// calls other code. The routines it calls save R in their stack frames, so
// the value is part of the stack image the game may read later (exact stack
// images, CLAUDE.md). It lives in R: assigning writes R and reading reads R,
// as the original does after each call (whose epilogue reloads R from its
// save slot). Otherwise a plain value.
template <unsigned R>
class Kept {
    static_assert(R >= 13 && R <= 31, "Kept: not a non-volatile register");

public:
    explicit Kept(Cpu& c, std::uint32_t value = 0) : c(c) { c.set(R, value); }
    Kept(const Kept&) = delete;
    Kept& operator=(std::uint32_t value) {
        c.set(R, value);
        return *this;
    }
    Kept& operator=(const Kept& other) { return *this = std::uint32_t(other); }
    operator std::uint32_t() const { return c.r(R); }

private:
    Cpu& c;
};
// A register number checked when the program is compiled: r13-r31 for
// keep(), r3-r10 for leave().
template <unsigned Low, unsigned High>
struct RegisterIn {
    unsigned n;
    consteval RegisterIn(unsigned n) : n(n) {
        if (n < Low || n > High) throw "register out of range";
    }
};
struct RegisterValue {
    RegisterIn<13, 31> n;
    std::uint32_t value;
};
struct LeftValue {
    RegisterIn<3, 10> n;
    std::uint32_t value;
};
// Values the original keeps in non-volatile registers (as Kept) that the
// C++ needs no name for: typically addresses of fields it also names.
inline void keep(Cpu& c, std::initializer_list<RegisterValue> values) {
    for (const auto& [n, value] : values) c.set(n.n, value);
}
// The original's `bl` into another rewritten routine of the same module: its
// argument registers (r3 on) and the return address. The C++ call follows;
// the registers are what the generated body reads when the hook declines
// (the routine checker).
inline void enter(Cpu& c, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto value : arguments) c.set(n++, value);
    c.link(back);
}
// What the original leaves in volatile registers when it returns: a
// caller's next varargs call stores r3-r10 into its stack image.
inline void leave(Cpu& c, std::initializer_list<LeftValue> values) {
    for (const auto& [n, value] : values) c.set(n.n, value);
}
// A value a C++ routine returns, left in r3 as the original's callee leaves it.
inline std::uint32_t result(Cpu& c, std::uint32_t value) {
    c.set(3, value);
    return value;
}
// A result tested for zero as the original does (cmpwi/cmplwi, or a record form).
inline bool nonzero(Cpu& c, std::int32_t value) { return compare(c, value) != 0; }
inline bool nonzero_logical(Cpu& c, std::uint32_t value) { return compare_logical(c, value) != 0; }
}

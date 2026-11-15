#pragma once
// FINANCE.CPP, block board: the pieces the four routines share (the non-volatile registers the original
// holds at each call, the sample-table draws, the draw below a limit, the income booking).
#include "finance_detail.hpp"
#include "finance_ledger.hpp"
#include "guest_fpu.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <utility>

namespace cm::aot::finance::board {
// Offsets in the double pool (toc::doubles).
namespace pool {
inline constexpr std::uint32_t scale = 0x08,        // 65535.0
                               zero = 0x10;
}
inline std::uint32_t u(std::int32_t value) { return std::uint32_t(value); }
inline std::int32_t i(std::uint32_t value) { return std::int32_t(value); }

// The non-volatile registers (r13-r31, f14-f31) as the original holds them, mirrored in native variables
// and written to the Cpu before each call: the callees save them in their frames, and code that runs
// later reads those frames as stale stack.
class Held {
public:
    explicit Held(Cpu& cpu) : c(cpu) {
        for (unsigned n = 13; n < 32; ++n) r_[n] = c.r(n);
        for (unsigned n = 14; n < 32; ++n) f_[n] = c.f(n);
    }
    void set(unsigned n, std::uint32_t value) { r_[n] = value; }
    void set_f(unsigned n, double value) { f_[n] = value; dirty_f_ |= 1u << n; }
    std::uint32_t get(unsigned n) const { return r_[n]; }
    void sync() {
        for (unsigned n = 13; n < 32; ++n) c.set(n, r_[n]);
        for (unsigned n = 14; n < 32; ++n)
            if (dirty_f_ >> n & 1u) c.set_f(n, f_[n]);
    }
    Cpu& c;

private:
    std::array<std::uint32_t, 32> r_{};
    std::array<double, 32> f_{};
    std::uint32_t dirty_f_ = 0;
};

// int to double through the magic word, as the original does with `xoris` and the stack.
inline double to_double(guest::Fpu& fpu, Cpu& c, std::int32_t value) {
    const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(value) ^ 0x80000000u);
    return fpu.fsub(std::bit_cast<double>(bits), c.f64(c.toc(toc::doubles) + doubles::int_magic));
}
inline double pool_double(Cpu& c, std::uint32_t offset) { return c.f64(c.toc(toc::doubles) + offset); }

// The next sample of the shared table (see finance_detail.hpp `draw`), with the held registers written
// before the redraw's call. `back` is the address after that call.
std::uint32_t sample(Held& h, std::uint32_t back);
// A draw whose only use is "the sample is a multiple of `modulus`" (the compiler's signed remainder).
bool multiple_of(Held& h, std::uint32_t back, std::int32_t modulus);

// The draw below a limit, as the unit writes it inline: one sample, then the sample's remainder with the
// limit's sign for a limit within +-0xffff, or a scaled place plus a tenth of a draw below a tenth of the
// limit (0x100f2100) for a larger one. The result is left in the held register `dest`, which the original
// also loads with the scaled place before the call of 0x100f2100. `redraw_back` is the address after the
// redraw's call, `helper_back` the one after the call of 0x100f2100 (0: the site's limit cannot reach it).
struct Below { unsigned dest; std::uint32_t redraw_back, helper_back; };
std::int32_t draw_below(Held& h, const Below& site, std::int32_t limit);
// The same with the limit a double (held in f31), converted by fctiwz at each place the original reads it.
std::int32_t draw_below_real(Held& h, guest::Fpu& fpu, const Below& site, double limit);

// Income on the donations line (+0xb4, both copies) and the balance, as the original adds them.
void book_donation(Cpu& c, std::uint32_t account, std::uint32_t amount);
// The finance news of a board's action (0x10101ff0): `manager`, club, amount, the second result word of the
// account refresh, and the kind.
void announce(Held& h, std::uint32_t manager, std::uint32_t club, std::uint32_t amount, std::uint32_t refreshed,
              std::uint32_t kind, std::uint32_t back);
// The account refresh of 0x100f5770 after a booking: its second result word (r4).
std::uint32_t refresh_after_booking(Held& h, std::uint32_t account, std::uint32_t back);
}

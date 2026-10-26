#pragma once
// What shortlist_manager_*.cpp share: the module's TOC slots, string pool and
// constants, the game's tables it reads, the assertion report and the calls
// into the rest of the program. The game records' views are in
// shortlist_manager_records.hpp; views only one group needs live with it.
//
// The routines compute with C++ values and leave what the game can observe as
// the original does (guest_values.hpp); floating point through guest::Fpu
// (guest_fpu.hpp), which leaves FPSCR as the original. r0, r11, r12, CTR, XER
// and f0/f2-f13 are not reproduced: nothing around the module reads them
// before writing them (tools/shortlist_manager_audit.py --volatiles, a
// ctest). The condition register is reproduced at returns.
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "shortlist_manager.hpp"
#include "shortlist_manager_records.hpp"
#include <initializer_list>

namespace cm::aot {
// Generated (semantic_guest_call.hpp): a divw whose quotient cannot matter
// when its divisor is zero.
bool quotient_free_division(std::uint32_t address);
}

namespace cm::aot::shortlist_manager::detail {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;
using guest::Field;
using guest::Kept;
using guest::compare;
using guest::compare_logical;
using guest::enter;
using guest::extended;
using guest::keep;
using guest::leave;
using guest::nonzero;
using guest::nonzero_logical;
using guest::result;
// The game records the routines read and write, one view each.
using namespace records;

// TOC slots SHORTLIST_MANAGER.CPP reads (lwz rX,slot(r2): the address of the
// object or table; `Cpu::toc(slot)`). Names from use; unnamed game slots are
// listed by offset in the group that reads them.
namespace slot {
// The module's own.
constexpr std::int32_t pool = -0x4dd8;       // the module's strings (text:: offsets), 0x1080b64c
constexpr std::int32_t constants = -0x4ddc;  // double constants (k::), 0x1080b8d8
constexpr std::int32_t singles = -0x4de8;    // single constants (k::), 0x1080b8ac
// Jump tables (case labels, one word each).
constexpr std::int32_t cases_1036f7e0 = -0x4e14;
constexpr std::int32_t cases_10369080_a = -0x4e10, cases_10369080_b = -0x4e0c, cases_10369080_c = -0x4e08;
constexpr std::int32_t cases_10366470_a = -0x4e04, cases_10366470_b = -0x4e00, cases_10366470_c = -0x4dfc,
                       cases_10366470_d = -0x4df8;
constexpr std::int32_t cases_10365d10 = -0x4df4;
constexpr std::int32_t cases_10361490_a = -0x4df0, cases_10361490_b = -0x4dec;
constexpr std::int32_t cases_10359dc0_a = -0x4de4, cases_10359dc0_b = -0x4de0;
// Transition vectors of the module's callbacks (qsort/bsearch comparators,
// the array element constructor).
constexpr std::int32_t compare_10366410_tv = -0x62a8, compare_10358ff0_tv = -0x62a4,
                       compare_10358f80_tv = -0x62a0, construct_10358af0_tv = -0x629c,
                       compare_103663b0_tv = -0x6298, compare_10358f50_tv = -0x6294,
                       compare_10358f20_tv = -0x6290;
// The game's (names shared with player_stats_detail.hpp).
constexpr std::int32_t clubs = -0x7be8;         // club records (0x248 bytes each)
constexpr std::int32_t person_count = -0x7be4;  // int32: number of person records
constexpr std::int32_t persons = -0x7be0;       // person records (0x70 bytes each)
constexpr std::int32_t game = -0x7bb0;          // the game object
constexpr std::int32_t success = -0x7bac;       // int8: cleared by every assertion report
constexpr std::int32_t message = -0x7ba8;       // the shared message buffer
constexpr std::int32_t today = -0x7b80;         // the game date: int16 day of year, uint16 year
constexpr std::int32_t contracts = -0x7b7c;     // the contracts object (0x10082f20/0x10082f90's first argument)
constexpr std::int32_t save_version = -0x7b74;  // int32*: the loaded save's version
constexpr std::int32_t competitions = -0x7b5c;  // Competition* per id
constexpr std::int32_t club_count = -0x7b50;    // int32: entries of slot::clubs
constexpr std::int32_t calendars = -0x7b44;     // Calendar[] (the TOC word is the array)
constexpr std::int32_t player_stats = -0x7b3c;  // the player-stats object (PLAYER_STATS.CPP)
constexpr std::int32_t start_date = -0x7a9c;    // Date*: the game's start date
// The game's random-number table: a cursor stepping two bytes through
// uint16 values up to random_end, back at random_table with a new salt
// (rand() & 0xffff) when it passes the end; a draw is the value plus the salt.
constexpr std::int32_t random_cursor = -0x7b84, random_end = -0x7b88, random_table = -0x7b8c, random_salt = -0x7b90;
// Ids of four nations whose regions count as groups of their own (as in
// player_stats_relations.cpp).
constexpr std::int32_t regional_nation_1 = -0x7514, regional_nation_2 = -0x7504, regional_nation_3 = -0x750c,
                       regional_nation_4 = -0x7510;
}

// The module's strings (offsets into its pool, slot::pool).
namespace text {
constexpr std::uint32_t file_name = 0, assert_format = 0x16, error_title = 0x23, assessing_squads = 0x29,
                        creating_shortlists = 0x3f, two_strings = 0x53, one_string = 0x58, goalkeeper = 0x5b,
                        defender_left = 0x68, defender_center = 0x78, defender_right = 0x8a,
                        midfielder_left = 0x9b, midfielder_centre = 0xad, midfielder_right = 0xc1, forward = 0xd4,
                        backroom_staff = 0xde, dario_gradi = 0xef, pat_nevin = 0xfb, kevin_keegan = 0x105,
                        indispensable = 0x112, important = 0x13b, rotation = 0x169, backup = 0x198,
                        hot_prospect = 0x1c3, decent_young = 0x1f0, not_needed = 0x215, on_trial = 0x23b,
                        uncertain = 0x253;
}

// The pool's constants (slot::constants, doubles, lfd at +offset; slot::singles,
// lfs at +offset). The values are the pool's; `.local/shortlist/constants.txt`
// lists the whole pool.
namespace k {
constexpr double u16_max = 65535.0;                 // constants +0x00
constexpr double signed_bias = 4503601774854144.0;  // constants +0x08: 2^52 + 2^31, the int-to-double idiom's
constexpr double hundred_s = 100.0f, half_s = 0.5f, one_s = 1.0f, zero_s = 0.0f, two_s = 2.0f;  // singles +0, +4, +0xc, +0x10, +0x24
}

// Sign extension as the original's extsb, extsh and lha do it: the low
// byte or halfword of a register as a signed value (byte_of, half_of), and a
// byte as the register holds it after extsb (extended8; guest::extended is
// the halfword's).
inline std::int32_t byte_of(std::uint32_t value) { return std::int8_t(value); }
inline std::int32_t half_of(std::uint32_t value) { return std::int16_t(value); }
inline std::uint32_t extended8(std::uint32_t value) { return std::uint32_t(byte_of(value)); }

// The compiler's int-to-double idiom: the words 0x43300000 and the integer
// (signed: signed_word, the value xor 0x80000000) in the stack temporary at
// `at`, read back as a double; the original then subtracts the pool's bias
// (fsub or fsubs). The temporary is part of the stack image.
inline double stacked_integer(Cpu& c, std::uint32_t at, std::uint32_t low) {
    c.put32(at, 0x43300000);
    c.put32(at + 4, low);
    return c.f64(at);
}
inline std::uint32_t signed_word(std::int32_t value) { return std::uint32_t(value) ^ 0x80000000u; }

// mulhw: the high word of the signed 64-bit product.
inline std::int32_t multiply_high(std::int32_t a, std::int32_t b) {
    return std::int32_t((std::int64_t(a) * std::int64_t(b)) >> 32);
}

// divw at `site` (the instruction's address) by a divisor the program does
// not fix, as the generated code computes it: aot_runtime.hpp divide_signed
// (a zero divisor stops the run with an error; INT_MIN / -1 gives INT_MIN),
// except at the divisions tools/compile_recomp.py proves quotient-free
// (quotient_free_division: the quotient only feeds a mullw by the same
// divisor), where a zero divisor gives quotient 0, so the remainder idiom
// (divw, mullw, subf) leaves the dividend. Division by a constant needs
// none of this (the compiler multiplies by a magic number).
inline std::int32_t divide(std::uint32_t site, std::int32_t a, std::int32_t b) {
    if (b == 0 && quotient_free_division(site)) return 0;
    return std::int32_t(std::uint32_t(divide_signed<4>(std::uint32_t(a), std::uint32_t(b))));
}
// The remainder idiom after the divw at `site`: a - (a / b) * b, wrapping.
inline std::int32_t modulo(std::uint32_t site, std::int32_t a, std::int32_t b) {
    return std::int32_t(std::uint32_t(a) - std::uint32_t(divide(site, a, b)) * std::uint32_t(b));
}

// The rest of the original program that SHORTLIST_MANAGER.CPP calls: the
// game, the C library and the Toolbox glues. Each call sets the argument
// registers the original's call site sets and returns r3; `back` is the
// address after the original `bl`. The caller sets the non-volatile
// registers the original holds at the call first (callees save them into
// the stack image). Calls through a Toolbox glue reload r2 from sp+0x14
// afterwards, as the original's next instruction does (toolbox()). Groups
// name the game's routines they call next to their own code.
// A call into another routine of SHORTLIST_MANAGER.CPP runs its C++ directly
// (shortlist_manager_adapter.cpp): the `bl`'s link register, then the
// routine's hook, as the dispatcher would run it, without the round trip.
// False (the caller makes the guest call) when its C++ does not run here.
bool call_rewritten(Cpu& c, std::uint32_t entry, std::uint32_t back);

class Program {
public:
    explicit Program(Cpu& c) : c(c) {}
    std::uint32_t call(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
        unsigned n = 3;
        for (const auto value : arguments) c.set(n++, value);
        if (!call_rewritten(c, entry, back)) c.call(entry, back);
        return c.r(3);
    }
    std::uint32_t toolbox(std::uint32_t glue, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
        const auto result = call(glue, back, arguments);
        c.set(2, c.u32(c.sp() + 0x14));
        return result;
    }
    // The C library.
    std::uint32_t malloc(std::uint32_t size, std::uint32_t back) { return call(0x10775130, back, {size}); }
    void free(std::uint32_t pointer, std::uint32_t back) { call(0x10775170, back, {pointer}); }
    std::uint32_t realloc(std::uint32_t pointer, std::uint32_t size, std::uint32_t back) {
        return call(0x107751b0, back, {pointer, size});
    }
    void memset(std::uint32_t to, std::uint32_t value, std::uint32_t size, std::uint32_t back) {
        call(0x10778300, back, {to, value, size});
    }
    // sprintf (0x1077b720) is varargs: its callee stores r3-r10 into the
    // caller's frame, so set every one the original holds there.
    std::uint32_t sprintf(std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        return call(0x1077b720, back, arguments);
    }
    void qsort(std::uint32_t base, std::uint32_t count, std::uint32_t size, std::uint32_t compare, std::uint32_t back) {
        call(0x10768300, back, {base, count, size, compare});
    }
    std::uint32_t bsearch(std::uint32_t key, std::uint32_t base, std::uint32_t count, std::uint32_t size,
                          std::uint32_t compare, std::uint32_t back) {
        return call(0x10775600, back, {key, base, count, size, compare});
    }
    std::uint32_t operator_new(std::uint32_t size, std::uint32_t back) { return call(0x10773520, back, {size}); }
    void operator_delete(std::uint32_t pointer, std::uint32_t back) { call(0x107735a0, back, {pointer}); }

private:
    Cpu& c;
};

// The original's assertion report (guest::report_assertion) with this
// module's pool texts: `at` is the address of its first call, `name` and
// `extension` the two stack buffers, `line` the source line it prints.
// Most sites clear the success flag through r4 (`lwz r4,-0x7bac(r2); li
// r0,0; stb r0,0(r4)`), which then holds the flag's address; sites that keep
// that address in a non-volatile register pass flag_in_r4 = false.
inline void report(Cpu& c, std::uint32_t pool, std::uint32_t at, std::uint32_t name, std::uint32_t extension,
                   std::uint32_t line, bool flag_in_r4 = true) {
    guest::report_assertion(c, {at, pool + text::file_name, pool + text::assert_format, pool + text::error_title,
                                name, extension, line});
    if (flag_in_r4) c.set(4, c.toc(slot::success));
}
}

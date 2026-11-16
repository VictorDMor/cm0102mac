#pragma once
// TRANSFER_MANAGER.CPP block "offers": what its files share.
#include "guest_fpu.hpp"
#include "transfer_manager_native_detail.hpp"
#include <bit>

namespace cm::aot::transfer_manager_native::offers {
using guest::Cpu;
using guest::Frame;
// Words of the table of contents the block reads besides detail's.
namespace cell {
inline constexpr std::int32_t sample_base = -0x7b90,   // the cell the draws add to each halfword sample
                              sample_first = -0x7b8c,  // where the cursor restarts when the table runs out
                              sample_last = -0x7b88,   // the cell holding the last cursor position
                              sample_cursor = -0x7b84, // the cell holding the cursor (a halfword pointer)
                              floats = -0x4ba0,        // the double constants of the module
                              person_count = -0x7be4,  // a count the clubs' first word is compared with (0x103e9ac0)
                              awol = -0x7b24;          // the object 0x10003dc0 / 0x10006900 are asked of
}
// Bytes of an offer (the record behind a transfer slot).
namespace offer {
inline constexpr std::uint32_t list = 0x04, club = 0x08, person = 0x0c, side = 0x11, stage = 0x34, flagged = 0x35,
                               answers = 0x36, protected_from_withdrawal = 0x37;
}
inline constexpr std::uint32_t shortlists = 0x988, status_rows = 0x9a0, status_row_size = 10;
inline constexpr std::uint32_t refill_sample = 0x1077b770, next_in_list = 0x10738e00;

// The sample table: a halfword per draw, each added to a base word that the routine at 0x1077b770 refills.
struct Samples {
    Cpu& c;
    std::uint32_t base_cell, first, last_cell, cursor_cell;
    explicit Samples(Cpu& cpu)
        : c(cpu), base_cell(c.toc(cell::sample_base)), first(c.toc(cell::sample_first)),
          last_cell(c.toc(cell::sample_last)), cursor_cell(c.toc(cell::sample_cursor)) {}
    // The next draw: moves the cursor on by one halfword (refilling past the last), and returns the halfword
    // plus the base. `hold()` sets the registers the original has at the refill call, which only that call sees.
    template <class Hold>
    std::uint32_t next(std::uint32_t refill_back, Hold&& hold) const {
        const auto moved = c.u32(cursor_cell) + 2;
        const auto last = c.u32(last_cell);
        c.put32(cursor_cell, moved);
        if (moved > last) {
            c.put32(cursor_cell, first);
            hold();
            invoke(c, refill_sample, refill_back);
            c.put32(base_cell, c.r(3) & 0xffff);
        }
        return std::uint32_t(c.u16(c.u32(cursor_cell))) + c.u32(base_cell);
    }
};
// A sample as a signed remainder by a nonzero range of at most 0xffff in size (divw and mullw as the
// generated code does them: the sample is the base word plus a halfword, so it may be any word).
inline std::int32_t remainder_of(std::uint32_t sample, std::int32_t range) {
    const auto quotient = std::int32_t(std::int64_t(std::int32_t(sample)) / range);
    const auto rest = std::int32_t(sample - std::uint32_t(quotient) * std::uint32_t(range));
    return range > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
}
// The original's int-to-double: the magic high word over the biased integer, minus the bias constant.
inline double magic(std::int32_t x) {
    return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u));
}
inline double to_double(guest::Fpu& fpu, Cpu& c, std::uint32_t pool, std::int32_t x) {
    return fpu.fsub(magic(x), c.f64(pool + 8));
}

// The person and club records of an offer, from the tables.
inline std::uint32_t person_of(Cpu& c, std::uint32_t offer_record) {
    return c.u32(c.toc(toc::people)) + c.u32(offer_record + offer::person) * person_size;
}
inline std::uint32_t club_of(Cpu& c, std::uint32_t offer_record) {
    return c.u32(c.toc(toc::clubs)) + c.u32(offer_record + offer::club) * club_size;
}
// The shortlist notice every stage change ends with: 0x1036d240 looks the person up for the club, and a
// hit is reported back (0x10365590). `pass_side` hands the hit's own side on (0x103e9910, 0x103e4130,
// 0x103e44b0) or not (0x103e4390).
inline void tell_shortlists(Cpu& c, std::uint32_t manager, std::uint32_t offer_record, std::uint32_t lookup_back,
                            std::uint32_t update_back, bool pass_side) {
    const auto differs_from_11 = [](std::int8_t side) { return side != 11 ? 1u : 0u; };
    const auto hit = invoke(c, 0x1036d240, lookup_back,
                            {c.u32(manager + shortlists), club_of(c, offer_record), person_of(c, offer_record),
                             differs_from_11(c.i8(offer_record + offer::side)), 0});
    if (!hit) return;
    invoke(c, 0x10365590, update_back,
           {c.u32(manager + shortlists), person_of(c, offer_record), club_of(c, offer_record), 1,
            pass_side ? differs_from_11(c.i8(hit)) : 0u});
}
}

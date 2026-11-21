#pragma once
// PLAYER_REGEN.CPP: the records, globals and shared pieces its routines read (player_regen.hpp has the routines).
// Field meanings come from how this module and its callers use them; "(inferred)" marks a name no reader has
// confirmed. The person and club records are the game database's (database_detail.hpp,
// database_init_detail.hpp); only the fields this module reads are named here.
#include "database_detail.hpp"
#include "guest_abi.hpp"
#include "guest_fpu.hpp"
#include "player_regen.hpp"
#include <bit>
#include <cstdint>

namespace cm::aot::player_regen {
using guest::Fpu;
using guest::Frame;

// TOC slots (`lwz rX,slot(r2)`, `c.toc(slot)`): a slot holds the address of the variable, or a table of constants.
namespace toc {
inline constexpr std::int32_t
    constants = -0x50a8,        // the unit's doubles (constant:: below), 0x107fa5f0
    pool = -0x50a0,             // the unit's strings (not used by the pilots)
    people = -0x7be0,           // cell: the 0x70-byte person records (database_init_detail.hpp person)
    person_count = -0x7be4,     // cell: how many people there are; the last 16 are not candidates
    kind_table = -0x7b9c,       // cell: 100-byte records indexed by a person's first word; the class bits live in byte +0xc (the same cell is contract_index in contract_manager_detail.hpp: the per-person daily record)
    year_date = -0x7b80,        // cell: the game's current date (halfword day +0, halfword year +2)
    rng_salt = -0x7b90,         // cell: the pool's salt word (the last refill's value)
    rng_start = -0x7b8c,        // the pool's first sample (the slot holds the address itself, not a cell)
    rng_end = -0x7b88,          // cell: the address of the pool's last sample
    rng_cursor = -0x7b84;       // cell: the address of the sample the next draw reads
}

// The unit's doubles, read from guest memory (never host literals). +0 is the bias of a signed integer converted
// through the stack (high word 0x43300000, the integer's sign bit flipped: 0x4330000080000000).
namespace constant {
inline constexpr std::uint32_t bias = 0x00, word_range = 0x08 /* 65535.0 */, cube_exponent = 0x10 /* 3.0 */,
                               square_exponent = 0x20 /* 2.0 */, zero = 0x78 /* 0.0 */;
}
// The high word of a converted integer's double.
inline constexpr std::uint32_t conversion_high = 0x43300000;

// The regen object's lists (constructor 0x102d8230: list A 250 entries of 0x1c bytes, B of 0xc, C and D of 0x10).
// A list is read as {+4 base of the entries, +8 count}.
namespace list {
inline constexpr std::uint32_t entries = 0x04, count = 0x08;
}
// The regen object itself: the word at +0x20 (the non-player staff pool's, inferred: nonzero when the pool holds entries).
namespace regen_object { inline constexpr std::uint32_t staff_pool_word = 0x20; }
// A list A entry (retirement entry, 0x1c bytes): its first word is the person's id.
namespace retirement_entry {
inline constexpr std::uint32_t size = 0x1c, id = 0x00,
                               kind = 0x18,        // byte: 8 marks a pending injury retirement
                               announced = 0x19;   // signed byte: nonzero once announced (inferred)
inline constexpr std::int32_t injury_kind = 8;
// The two dates (a halfword day and an unsigned halfword year each): the retirement is due at the first, and the second
// is the one an entry whose first date has passed is ordered by (inferred).
inline constexpr std::uint32_t first_date = 0x08, second_date = 0x10;
}
// A list B entry (regen queue, 0xc bytes): id, a signed halfword at +4 and an unsigned halfword at +6.
namespace queue_entry {
inline constexpr std::uint32_t id = 0x00, rank_low = 0x04 /* signed halfword */, rank_high = 0x06 /* unsigned halfword */;
}
// The game's date as the TOC cell year_date points at: a signed halfword day and an unsigned halfword year.
namespace date {
inline constexpr std::uint32_t day = 0x00, year = 0x02;
}

// A person record (0x70 bytes). database_init_detail.hpp names its first fields (database::init::person); the
// rest are the offsets the candidate score reads.
namespace person {
inline constexpr std::uint32_t size = 0x70,
    id = 0x00,            // word: also the index into the kind table (100-byte records)
    nation = 0x0c,        // pointer: a record whose word +0x68 is compared with the caller's key
    player = 0x14,        // pointer: the player record (database::person::player)
    staff = 0x1c,         // word: nonzero for a person who is staff (inferred: not a candidate)
    club = 0x24,          // pointer: the person's club, 0 when unattached
    age = 0x5c,           // halfword; the low byte (+0x5d) is the age the score reads, signed
    role_flag = 0x5e,     // byte (its square is added for an unattached person)
    rating_61 = 0x61, rating_63 = 0x63, rating_64 = 0x64, rating_65 = 0x65, rating_66 = 0x66, rating_67 = 0x67,
    rating_69 = 0x69;     // signed bytes: attribute ratings
inline constexpr std::uint32_t nation_key = 0x68;  // the nation record's word compared with the caller's key (inferred)
}
// The player record: two halfwords (+0xa, +0xc) the score compares with 7500 (0x1d4c) and a signed byte (+0x2f).
namespace player_record {
inline constexpr std::uint32_t value_a = 0x0a, value_c = 0x0c, potential_byte = 0x2f;
}
// The club row (0x248 bytes): the reputation halfword and the word +0x90.
namespace club_row {
inline constexpr std::uint32_t reputation = 0x1ec, word_90 = 0x90;
}
// The kind table's class byte: bits 2..3 are the class; a person whose class is not 0 cannot be a candidate.
namespace kind_record { inline constexpr std::uint32_t size = 100, class_byte = 0x0c; }

namespace callee {
inline constexpr std::uint32_t reseed = 0x1077b770;  // the pooled generator's refill
}
// The addresses after the original's `bl reseed` at each inline draw this module's pilots make (the callee saves
// the return address, and the call trace pairs the calls by it).
namespace refill_site {
inline constexpr std::uint32_t random_first = 0x102da684, random_second = 0x102da770,
                               age_term = 0x102dc4c8, nation_term = 0x102dc6b0, unattached_term = 0x102dc960,
                               final_term = 0x102dc9dc;
}

// The prologue's `stw r0,8(r1)`: the saved link register in the caller's frame. A routine that can call a dependency (the
// pooled generator's refill) writes it first, as the original does, so that the production proof (a run with no
// generated instruction budget) sees the C++ routine entered and framed when the dependency stops. Its bytes are the
// callee's part of the caller's frame, free under the native contract; no reader reads them (the stack audit).
inline void save_link(Cpu& c) { c.put32(c.sp() + 8, c.lr()); }

// The signed value of a byte or halfword (the original's extsb / extsh).
inline std::int32_t sext8(std::uint32_t v) { return std::int32_t(std::int8_t(v & 0xff)); }
inline std::int32_t sext16(std::uint32_t v) { return std::int32_t(std::int16_t(v & 0xffff)); }
// divw as the generated code has it: signed, truncating; the quotient of a zero divisor is 0 (it only ever feeds a
// multiply by the same divisor: tools/compile_recomp.py zero_divisor_quotient_unused).
inline std::int32_t divide_signed(std::int32_t a, std::int32_t b) {
    return b == 0 ? 0 : std::int32_t(std::uint32_t(std::int64_t(a) / std::int64_t(b)));
}
// mulhw: the high word of the signed 64-bit product.
inline std::int32_t mulhw(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * std::int64_t(b)) >> 32); }

// The high word beside an integer's sign-flipped bits: the double the original builds on its frame and subtracts the
// bias from (exactly the integer).
inline double biased(std::int32_t value) {
    return std::bit_cast<double>((std::uint64_t(conversion_high) << 32) | (std::uint32_t(value) ^ 0x80000000u));
}

// The registers a scaled draw leaves besides r3 (r4..r9; r10 is never written): callers' next varargs call spills them.
struct ScaledLeftovers {
    std::uint32_t sign_of_tenth = 0, scaled_product = 0, sign_of_quotient = 0, sum = 0, sample = 0, high_word = 0;
};

// The pooled generator the module's inline draws share: the cells' addresses and the pool's first sample.
class Pool {
public:
    explicit Pool(Cpu& cpu)
        : c(cpu), salt(cpu.toc(toc::rng_salt)), end(cpu.toc(toc::rng_end)), cursor(cpu.toc(toc::rng_cursor)), start(cpu.toc(toc::rng_start)) {}
    // Moves to the next sample; past the end, back to the start with a new salt. `back` is the address after the original's
    // call of the refill (the callee saves it).
    void advance(std::uint32_t back) const;
    // The sample under the cursor plus the salt.
    std::int32_t sum() const { return std::int32_t(std::uint32_t(c.u16(c.u32(cursor))) + c.u32(salt)); }
    // One draw below `bound` (0x102da630's result, or one of its inline copies): `back` is the first refill's return
    // address, `second_back` the second draw's (when the bound is scaled and its second bound is not 0). The second draw of
    // an inline copy is a call of 0x102da630, whose own first refill site is `refill_site::random_first`.
    // `converted`: the double an inline copy converted before the draw (the nation term's pow result): the original
    // converts it again at the start of the scaled form and after the second draw, which leaves FPSCR bits.
    std::int32_t draw(std::int32_t bound, std::uint32_t back, std::uint32_t second_back, const double* converted = nullptr,
                      ScaledLeftovers* leftovers = nullptr) const;
    // The same after the cursor has moved (the nation term advances before it knows its bound; a bound of 0 then draws
    // nothing more but still has moved the cursor, and divides by zero: the quotient is taken as 0).
    std::int32_t draw_advanced(std::int32_t bound, std::uint32_t second_back, const double* converted = nullptr,
                               ScaledLeftovers* leftovers = nullptr) const;
private:
    Cpu& c;
    std::uint32_t salt, end, cursor, start;
};
}

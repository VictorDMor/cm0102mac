#pragma once
// DATABASE.CPP's init block: the state of one run of "Player Initialisation" (0x100b8cf0) and the
// compiler idioms its phases share. The routine is one 9,738-instruction loop over the person table
// in which CodeWarrior inlined the game's bounded random draw and a clamp macro everywhere; the
// phases below keep the original's order of draws, stack temporaries and calls.
#include "database_generation_detail.hpp"
#include "guest_abi.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include <array>
#include <cstdint>

namespace cm::aot::database::init {
using guest::Cpu;

constexpr std::uint32_t entry = 0x100b8cf0;

// Records (inferred meanings from how the routine and its callers use them).
namespace person {  // 0x70 bytes, from the table the TOC word at -0x7be0 points at
constexpr std::uint32_t id = 0x0, nation = 0xc, club_a = 0x1c /*unused here*/, player = 0x14, staff = 0x1c,
                        birth_club = 0x20, club = 0x24, first_name = 0x28, second_name = 0x2c, common_name = 0x30,
                        contract_date = 0x34, born = 0x4c, date_a = 0x54, date_b = 0x44, age = 0x5c, role_flag = 0x5e,
                        role_byte = 0x60, rating_61 = 0x61, role = 0x62;
}
// The compiler's byte slots 0x61-0x69 of a person: rated 1 to 20, generated when out of range.
constexpr std::uint32_t rating_slot(unsigned index) { return 0x61 + index; }

// The inline sample pool: the draws read 16-bit values from a table, one more each time and add a
// 32-bit salt that is refreshed (rng_core) when the pointer passes the table's end.
constexpr std::uint32_t rng_core = 0x1077b770;

// A 16-bit value as the original holds it in a register after extsh.
inline std::int32_t s8(std::uint32_t v) { return std::int32_t(std::int8_t(v & 0xff)); }
inline std::int32_t s16(std::uint32_t v) { return std::int32_t(std::int16_t(v & 0xffff)); }
inline std::uint32_t u32(std::int32_t v) { return std::uint32_t(v); }
inline std::uint32_t ext16(std::int32_t v) { return u32(s16(u32(v))); }
// divw as the generated code has it: a zero divisor answers 0.
inline std::int32_t div_s(std::int32_t a, std::int32_t b) {
    return b == 0 ? 0 : std::int32_t(std::uint32_t(std::int64_t(a) / std::int64_t(b)));
}
inline std::int32_t rem_s(std::int32_t a, std::int32_t b) {
    return std::int32_t(std::uint32_t(a) - std::uint32_t(div_s(a, b)) * std::uint32_t(b));
}
inline bool in_rating_range(std::uint32_t v, std::int32_t low, std::int32_t high) {
    return s16(v) >= low && s16(v) <= high;
}

// Where the compiler's wide-bound branch of an inline draw keeps its three stack temporaries.
struct WideSlots {
    std::uint32_t first, second, result;  // offsets from the frame base
};

class Gen {
public:
    Gen(Cpu& cpu, guest::Frame& frame, std::uint32_t date_in);

    Cpu& c;
    guest::Frame& f;
    guest::Fpu fpu;
    // r13-r31 as the original holds them whenever it calls: g[n] is r<n>. Each call syncs them first
    // (the callee's prologue saves them into the stack image); the epilogue restores the entry values.
    std::array<std::uint32_t, 32> g{};

    // r14, r17-r21 (loaded once by the prologue: the sample pool and the constant pool) and r22 (the date argument).
    generation::Pool pool;
    std::uint32_t count_word, date;

    std::uint32_t at(std::int32_t offset) const { return f.at(offset); }
    double constant(std::uint32_t offset) const { return c.f64(pool.constants + offset); }

    void sync();
    // A call out of DATABASE.CPP (generated code or another module's owner): non-volatile registers first.
    void call(std::uint32_t target, std::uint32_t back);
    // A call to a rewritten routine of this module: r3.. and the link register. Registers first.
    void enter(std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {});

    // The inline draw: advance the pool pointer by 2 (refilling when past the end) and read the sample.
    std::int32_t next();
    // The bounded draw, narrow branch: 0 for a bound of 0 (no draw); else |sample % bound| (sign of bound).
    std::int32_t below(std::int32_t bound);
    // The same where the bound is a 32-bit word: beyond +-65535 the compiler's wide branch scales in floating point.
    std::int32_t below_wide(std::int32_t bound, const WideSlots& slots, std::uint32_t back);

    // The compiler's int-to-double: the words 0x43300000 and value ^ 0x80000000 at `slot`.
    double converted(std::uint32_t slot_offset, std::int32_t value);
    // x -> truncated(k * x) as the scaling sites compute it: stores at `in`, result bits at `out`.
    std::int32_t scaled(std::int32_t value, std::uint32_t k_offset, std::uint32_t in, std::uint32_t out,
                        bool constant_in_f1 = false);

    // Phases (database_init_*.cpp). `p` is the person record, `player` its player record.
    void person_names(std::uint32_t p);
    void person_dates(std::uint32_t p);
    void person_progress();
    void person_ratings(std::uint32_t p);
    void person_nation(std::uint32_t p);
    void player_abilities(std::uint32_t p, std::uint32_t player);
    void fix_contract_date(std::uint32_t p, std::int32_t year_field, bool use_field, std::uint32_t fixed_year,
                           std::uint32_t back_field, std::uint32_t back_fixed);
    void player_age_and_dates(std::uint32_t p, std::uint32_t player);
    void player_attributes(std::uint32_t p, std::uint32_t player);
    void raise_special_slots();
    void final_player_byte(std::uint32_t player);
    void staff_only(std::uint32_t p);
    void staff_record(std::uint32_t p);
    void staff_slots(std::uint32_t p);
    void staff_traits(std::uint32_t p);
    void staff_names_and_role(std::uint32_t p);
    void loop();

    std::uint32_t person_table_slot() const { return c.toc(-0x7be0); }
};

// Records of the 0x1b0 slot array the attribute phases use: 4-byte slots whose first two bytes hold
// the 16-bit value, terminated by -0x7f (a slot's byte +2 is a flag: 1 or 2).
constexpr std::uint32_t slot_array = 0x1b0, slot_array_b = 0xb4;
constexpr std::int16_t slot_end = -0x7f;
}

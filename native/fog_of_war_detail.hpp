#pragma once
// fog_of_war.cpp's records, globals and shared pieces (fog_of_war.hpp has the routines).
#include "fog_of_war.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::fog_of_war {
using guest::Frame;

// TOC slots. A "cell" holds a pointer the game sets; the others are the addresses themselves.
namespace toc {
inline constexpr std::int32_t
    people_count = -0x7be4,   // cell: the number of people
    people = -0x7be0,         // cell: the 0x70-byte person rows
    clubs = -0x7be8,          // cell: the 0x248-byte club rows
    enabled = -0x797c,        // byte: whether the game keeps the fog of war (the preference)
    tables = -0x7978,         // cell: the game's table of sixteen slots
    today = -0x7b80,          // the current date (5 bytes)
    success = -0x7bac,        // byte: the shared success flag
    message = -0x7ba8,        // the shared message buffer
    progress = -0x7bb0,       // the progress gauge the screen manager draws
    contracts = -0x7b7c,      // cell: the contract table
    last_contract = -0x7b78,  // cell: the last post a contract lookup found (the lookup's callers set it)
    stats = -0x7b3c,          // cell: the player history object
    version = -0x7b74,        // cell: the saved game's version
    pool = -0x4058,           // the unit's strings
    doubles = -0x405c;        // the unit's doubles
}
// Offsets in the unit's string pool.
namespace text {
inline constexpr std::uint32_t source_file = 0x00,  // "fog_of_war.cpp"
                               report_format = 0x0f, // "v%s %s.%s %d"
                               error = 0x1c,         // "Error"
                               file_name = 0x22,     // "fog.dat"
                               initialising = 0x2a;  // "Initialising game data"
}
// Offsets in the unit's double pool.
namespace doubles {
inline constexpr std::uint32_t fame = 0x00,         // 8.5
                               unsigned_magic = 0x08,  // 2^52
                               signed_magic = 0x10,    // 2^52 + 2^31
                               sample_span = 0x18;     // 65535
}
// The 0x70-byte person row (database_init_detail.hpp's person; meanings inferred from use here).
namespace person {
inline constexpr std::uint32_t id = 0x00, nation = 0x0c, player = 0x14, staff = 0x1c,
                               national_side = 0x20,  // a club row, as `club`: compared between people, its nation read
                               club = 0x24, age = 0x5c /* a halfword read as its low byte */, caps = 0x5e,
                               human = 0x6b;  // 1 for a human manager
inline constexpr std::uint32_t size = 0x70;
}
// The two reputations of a player record (+0x14) and of a staff record (+0x1c) (names inferred).
namespace reputation {
inline constexpr std::uint32_t player_home = 0x0a, player_world = 0x0c, staff_home = 0x2a, staff_world = 0x2c;
inline constexpr std::int32_t home_threshold = 0x109a, world_threshold = 0x1482;
}
namespace club {
inline constexpr std::uint32_t nation = 0x2c, division = 0x30, human_manager = 0x90, size = 0x248;
}
namespace nation {
inline constexpr std::uint32_t rank = 0x6c, reputation = 0xac;
}
// A knowledge object (8 bytes) and its entries, one per person.
namespace knowledge {
inline constexpr std::uint32_t entries = 0x0, manager = 0x4, size = 8;
}
namespace entry {
inline constexpr std::uint32_t level = 0x0, seen = 0x4, size = 0xc;
inline constexpr std::int32_t full = 3;
}
inline constexpr int slots = 16;
// Save versions below this one have no fog of war: every human starts afresh.
inline constexpr std::int32_t first_saved_version = 0x3e036;
// The days without a sighting after which a level falls by one.
inline constexpr std::int32_t memory_days = 0x21c;

namespace callee {
inline constexpr std::uint32_t
    open = 0x107475b0, read = 0x10747a80, write = 0x10747cc0, close = 0x10747900,  // ZIPDIR.CPP's file layer
    version_magnitude = 0x10775550,  // labs
    allocate_object = 0x10773520, free_object = 0x107735a0,                     // new, delete
    allocate_block = 0x10775130, free_block = 0x10775170,                       // new[], delete[]
    copy_text = 0x10727890,           // (destination, text): the localized text
    progress = 0x10743120,            // (gauge, text, done, of, -1)
    days_between = 0x10718130,        // (date, other's two words)
    human_runs_club = 0x1014c2e0,     // (club, manager, flag, 0, 0)
    primary_contract = 0x10082f20,    // (contracts, person): a post (word +4: the club) or 0
    club_of_code = 0x100ce320,        // (club, out byte, flag)
    attribute_rating = 0x100d2f50,    // (staff record, byte)
    seasons_since_club = 0x10309cc0,  // (history, person, club): 0 there now, -1 never
    reseed = 0x1077b770;              // the sample table's next base
}

// The non-volatile registers r13-r31 at the calls that can reach an import (the file layer, allocation, the
// history file, drawing): a capture's replay checks every one of them at each import, and the file layer
// leaves most of them as its caller had them. `hold` records what the original's register holds from that
// point on, `apply` writes them before a call. Algorithms keep their own named locals; Frame's epilogue gives
// the caller its registers back.
class CallRegisters {
public:
    explicit CallRegisters(Cpu& cpu) : c(cpu) { for (unsigned n = 13; n < 32; ++n) value[n] = c.r(n); }
    void hold(std::initializer_list<std::pair<unsigned, std::uint32_t>> held) { for (const auto& [n, v] : held) value[n] = v; }
    void apply() const { for (unsigned n = 13; n < 32; ++n) c.set(n, value[n]); }
private:
    Cpu& c;
    std::uint32_t value[32]{};
};

// `bl entry` with the arguments in r3 onwards; the callee's r3.
inline std::uint32_t invoke(Cpu& c, std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    c.call(entry, back);
    return c.r(3);
}

// People and their rows.
inline std::int32_t people_count(Cpu& c) { return c.i32(c.toc(toc::people_count)); }
inline std::uint32_t person_row(Cpu& c, std::uint32_t index) { return c.u32(c.toc(toc::people)) + index * person::size; }
inline bool fog_enabled(Cpu& c) { return c.i8(c.toc(toc::enabled)) != 0; }
// A manager's slot in the table: the human managers are the last sixteen people.
inline std::uint32_t slot_of(Cpu& c, std::uint32_t table, std::uint32_t manager) {
    return table + (std::uint32_t(people_count(c)) - 1 - c.u32(manager + person::id)) * 4;
}
inline std::uint32_t entry_of(Cpu& c, std::uint32_t knowledge, std::uint32_t index) {
    return c.u32(knowledge + knowledge::entries) + index * entry::size;
}
// The five bytes of today's date into an entry's `seen`.
inline void stamp_today(Cpu& c, std::uint32_t at) {
    const auto today = c.toc(toc::today);
    c.put16(at + 2, c.u16(today + 2));
    c.put16(at, c.u16(today));
    c.put8(at + 4, c.u8(today + 4));
}
// The club a person's primary contract names, or 0 (with the lookup's answer left in the shared cell).
inline std::uint32_t contracted_club(Cpu& c, std::uint32_t person_at, std::uint32_t back) {
    const auto post = invoke(c, callee::primary_contract, back, {c.u32(c.toc(toc::contracts)), person_at});
    c.put32(c.toc(toc::last_contract), post);
    return post ? c.u32(c.toc(toc::clubs)) + c.u32(post + 4) * club::size : 0;
}
// A person's age byte: the halfword at +0x5c, its low byte signed (lha, extsb).
inline std::int32_t age_of(Cpu& c, std::uint32_t person_at) { return std::int8_t(c.u16(person_at + person::age) & 0xff); }
// Whether a person's home reputation passes 4250 (the player record first, else the staff record).
inline bool home_reputation_high(Cpu& c, std::uint32_t person_at) {
    if (const auto player = c.u32(person_at + person::player))
        return c.i16(player + reputation::player_home) > reputation::home_threshold;
    const auto staff = c.u32(person_at + person::staff);
    return staff && c.i16(staff + reputation::staff_home) > reputation::home_threshold;
}
// A signed word divided by 10 as the compiler's multiply-high does it (truncating).
inline std::int32_t tenth(std::int32_t value) { return value / 10; }

// The game's shared sample table, stepped inline at each draw: the cursor cell advances by one halfword;
// past the last entry it returns to the first and the base is drawn again (`back` follows that `bl`).
namespace sample_table {
inline constexpr std::int32_t base = -0x7b90, first = -0x7b8c, last = -0x7b88, cursor = -0x7b84;
}
inline std::uint32_t draw_sample(Cpu& c, std::uint32_t back) {
    const auto cursor_cell = c.toc(sample_table::cursor), base_cell = c.toc(sample_table::base);
    const auto next = c.u32(cursor_cell) + 2;
    c.put32(cursor_cell, next);
    if (next > c.u32(c.toc(sample_table::last))) {
        c.put32(cursor_cell, c.toc(sample_table::first));
        c.call(callee::reseed, back);
        c.put32(base_cell, c.r(3) & 0xffff);
    }
    return std::uint32_t(c.u16(c.u32(cursor_cell))) + c.u32(base_cell);
}
// The sample as a remainder of `limit` (divw, mullw, subf, then the sign of the limit), limit nonzero.
inline std::int32_t remainder_in(std::uint32_t sample, std::int32_t limit) {
    const auto rest = std::int32_t(std::int64_t(std::int32_t(sample)) % limit);
    return limit > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
}
// The word an int-to-double leaves on the stack: 0x43300000 over the value (biased for a signed one).
inline double as_double_bits(std::uint32_t low) { return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | low); }

// Whether caps times a tenth of the nation's reputation pass 8.5, in the original's single precision
// (fsubs, fsubs, fmuls, fcmpo; FPSCR follows). The person and their nation are not null.
inline bool capped_for_reputable_nation(Cpu& c, std::uint32_t person_at, std::uint32_t nation_at) {
    guest::Fpu fpu(c);
    const auto pool = c.toc(toc::doubles);
    const auto tenths = tenth(c.i16(nation_at + nation::reputation));
    const double caps = fpu.fsubs(as_double_bits(c.u8(person_at + person::caps)), c.f64(pool + doubles::unsigned_magic));
    const double weight = fpu.fsubs(as_double_bits(std::uint32_t(tenths) ^ 0x80000000u), c.f64(pool + doubles::signed_magic));
    const double product = fpu.fmuls(caps, weight);
    fpu.fcmp(0, product, c.f64(pool + doubles::fame));
    return product > c.f64(pool + doubles::fame);
}
// A person's world reputation passes 5250 (the player record first, else the staff record).
inline bool world_reputation_high(Cpu& c, std::uint32_t person_at) {
    if (const auto player = c.u32(person_at + person::player))
        return c.i16(player + reputation::player_world) > reputation::world_threshold;
    const auto staff = c.u32(person_at + person::staff);
    return staff && c.i16(staff + reputation::staff_world) > reputation::world_threshold;
}

// The assertion report of a failed check: `at` is the address of the report's first call, `name` and
// `extension` the frame offsets of its two buffers, `line` the source line.
struct CheckSite { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
inline void report(Cpu& c, const Frame& frame, const CheckSite& site) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {site.at, pool + text::source_file, pool + text::report_format, pool + text::error,
                                frame.at(site.name), frame.at(site.extension), site.line});
}

}

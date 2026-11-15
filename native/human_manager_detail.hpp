#pragma once
// HUMAN_MANAGER.CPP: the layouts, globals and helpers more than one of the unit's files uses.
#include "guest_abi.hpp"
#include "human_manager_adapter.hpp"
#include <initializer_list>
#include <optional>

namespace cm::aot::human_manager {
using guest::Cpu;
using guest::Frame;

// The human-manager table, the object most routines of the unit get in r3 (global: the word in
// the cell at TOC humans): one record per human, indexed by the staff id.
namespace table {
inline constexpr std::uint32_t records = 0x00,  // the records, `count` of them
                               count = 0x04;
}
// A human's record (0xa65c bytes). Names marked (inferred) come from how the unit and its callers
// use the field, not from the original's source.
namespace record {
inline constexpr std::uint32_t size = 0xa65c;
inline constexpr std::uint32_t tactics = 0x0000;       // five tactic slots, `tactic_size` each
inline constexpr std::uint32_t tactic_size = 0x1eb4, tactic_slots = 5;
inline constexpr std::uint32_t away = 0x9984,          // byte: the human's clubs are left to the game (inferred)
                               runs_club = 0x9985,     // byte: the human picks the club's team himself (inferred)
                               runs_nation = 0x9986,   // byte: the same for a national side (inferred)
                               contracts = 0x998c,     // STAFF_CONTRACTS.CPP object (0x103721f0 builds it)
                               first_offer = 0x99f0,   // word: a TRANSFER_MANAGER.CPP object, or null
                               second_offer = 0x99f4,  // word: the same
                               setting = 0x99fc,       // word, read and written by 0x10148bf0 / 0x10148d50
                               current_tactic = 0x9a00,  // signed byte: the tactic slot in use
                               first_name = 0x9a01,    // text, 0x32 bytes (inferred)
                               second_name = 0x9a33,   // text, 0x32 bytes (inferred)
                               holiday = 0x9a65,       // byte (inferred)
                               short_setting = 0x9a66, // halfword, read and written by 0x10147bc0 / 0x10147d20
                               flags = 0x9ab8,         // bytes, indexed by 0x10149df0
                               list = 0x9ac0,          // the area 0x1014a1a0 returns
                               screens = 0xa640,       // CLUB_SCREENS.CPP object (0x10064030 builds it)
                               entries = 0xa650,       // word: the 0x20-byte sorted entries, or null
                               entry_capacity = 0xa654,  // word: how many entries the allocation holds
                               entry_count = 0xa658;   // word
}
// Fields of a tactic slot, at the slot's start (the current slot: record + current_tactic * tactic_size).
namespace tactic {
inline constexpr std::uint32_t field_18e0 = 0x18e0, field_18e4 = 0x18e4, field_18e8 = 0x18e8, field_18e9 = 0x18e9,
                               field_18ea = 0x18ea;
}
// The game record (TOC game): the humans in play, 0x2c0 bytes each, and whose turn it is.
namespace game {
inline constexpr std::uint32_t human_size = 0x2c0, human_staff = 0x04,  // word: the human's staff record
                               current = 0x2c58;                           // signed halfword: the human in play
}
// Human managers hold the last 16 staff ids: a record's index is the staff id less the staff
// count, plus 16.
inline constexpr std::int32_t human_slots = 16;
namespace staff {
inline constexpr std::uint32_t id = 0x00, job = 0x20;
}

// Words of the table of contents (Cpu::toc).
namespace toc {
inline constexpr std::int32_t pool = -0x54a0,           // the unit's strings
                              folder_buffer = -0x54a4,  // the buffer 0x10147150 writes a folder path into
                              humans = -0x7a54,         // the cell holding the human-manager table
                              message = -0x7ba8,        // the shared message buffer
                              success = -0x7bac,        // the shared success flag
                              game = -0x7bb0,           // the game record
                              staff_count = -0x7be4,    // the cell holding how many staff there are
                              people = -0x7be0,         // the cell holding the staff array (0x70 bytes each)
                              tactics_templates = -0x7a6c,  // the cell holding TACTICS.CPP's template list
                              record_constructor = -0x73dc, // transition vector of 0x10145e10
                              record_destructor = -0x73e0;  // transition vector of 0x10145d70
}
// Runtime routines more than one block calls.
namespace runtime {
inline constexpr std::uint32_t allocate = 0x107735f0,         // (bytes)
                               construct_array = 0x10773150,  // (block, constructor, destructor, size, count)
                               destroy_array = 0x107734a0,    // (array, destructor)
                               fill = 0x10778300;             // (block, byte, bytes)
}
// Offsets in the unit's string pool.
namespace text {
inline constexpr std::uint32_t source_file = 0x37,    // "HUMAN_MANAGER.CPP"
                               report_format = 0x49,  // "v%s %s.%s %d"
                               error = 0x56,          // "Error"
                               tactics = 0x5c, search = 0x64, training = 0x6b, print = 0x74;  // folder names
}

// The assertion report of a failed check: `at` is the address of the report's first call,
// `name` and `extension` the frame offsets of its two buffers, `line` the source line.
struct Site { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
inline void report(Cpu& c, const Frame& frame, const Site& site) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {site.at, pool + text::source_file, pool + text::report_format, pool + text::error,
                                frame.at(site.name), frame.at(site.extension), site.line});
}

// The staff a per-human routine works for: the one given, or (null) the human whose turn it is.
inline std::uint32_t staff_or_current(Cpu& c, std::uint32_t staff) {
    if (staff) return staff;
    const auto record = c.toc(toc::game);
    const auto current = std::uint32_t(std::int32_t(c.i16(record + game::current)));
    return c.u32(record + current * game::human_size + game::human_staff);
}
// The staff's raw index: the id less the staff count plus 16 (wrapping, as the original's subf/addic).
inline std::uint32_t raw_index(Cpu& c, std::uint32_t staff_record) {
    return c.u32(staff_record + staff::id) - c.u32(c.toc(toc::staff_count)) + std::uint32_t(human_slots);
}
// Whether a raw index lies inside 0..count-1 (signed compares, count - 1 wrapping).
inline bool inside(Cpu& c, std::uint32_t humans, std::uint32_t raw) {
    return std::int32_t(raw) >= 0 && std::int32_t(raw) <= std::int32_t(c.u32(humans + table::count) - 1);
}
// The staff's index in the table, or nothing when it lies outside.
inline std::optional<std::uint32_t> index_of(Cpu& c, std::uint32_t humans, std::uint32_t staff_record) {
    const auto raw = raw_index(c, staff_record);
    return inside(c, humans, raw) ? std::optional(raw) : std::nullopt;
}
inline std::uint32_t record_at(Cpu& c, std::uint32_t humans, std::uint32_t index) {
    return c.u32(humans + table::records) + index * record::size;
}
// The tactic slot a record uses (its current_tactic byte, sign-extended).
inline std::uint32_t current_tactic(Cpu& c, std::uint32_t human) {
    return human + std::uint32_t(std::int32_t(c.i8(human + record::current_tactic))) * record::tactic_size;
}

// The opening most per-human routines share: a frame (0xa0 bytes unless given) saving r29-r31, which
// the prologue loads with the success flag, the message buffer and the pool, in one of two orders.
struct Opening : Frame {
    enum class Order { success_first, message_first };
    Opening(Cpu& c, std::uint32_t entry, Order order = Order::success_first, std::uint32_t size = 0xa0)
        : Frame(c, entry, size, {.first_gpr = 29}) {
        const auto success = c.toc(toc::success), message = c.toc(toc::message), pool = c.toc(toc::pool);
        if (order == Order::success_first) c.set({{29, success}, {30, message}, {31, pool}});
        else c.set({{29, message}, {30, pool}, {31, success}});
    }
};
// The two checks at the head of the per-human routines: a staff record (given or the current human's),
// and one inside the table. Each failure reports at its site with the frame buffers at 0x68/0x40 and
// 0x48/0x38 (the 0xa0-byte frames).
struct Checks { std::uint32_t no_human_at, no_human_line, outside_at, outside_line; };
inline std::optional<std::uint32_t> index_or_report(Cpu& c, const Frame& frame, std::uint32_t humans,
                                                    std::uint32_t& staff, const Checks& checks) {
    staff = staff_or_current(c, staff);
    if (!staff) {
        report(c, frame, {checks.no_human_at, 0x68, 0x40, checks.no_human_line});
        return std::nullopt;
    }
    const auto index = index_of(c, humans, staff);
    if (!index) report(c, frame, {checks.outside_at, 0x48, 0x38, checks.outside_line});
    return index;
}
inline std::optional<std::uint32_t> record_or_report(Cpu& c, const Frame& frame, std::uint32_t humans,
                                                     std::uint32_t staff, const Checks& checks) {
    const auto index = index_or_report(c, frame, humans, staff, checks);
    return index ? std::optional(record_at(c, humans, *index)) : std::nullopt;
}

// The non-volatile registers r13-r31 at a routine's dispatched calls, for routines that track many of them
// (the file routines): callees save them into frames that later code reads as stale stack, so each call
// sees the values the original holds there. The algorithm keeps its own named locals; `hold` records
// what the original's register holds from that point on, `apply` writes them before a call.
class CallRegisters {
public:
    explicit CallRegisters(Cpu& cpu) : c(cpu) { for (unsigned n = 13; n < 32; ++n) value[n] = c.r(n); }
    void hold(std::initializer_list<std::pair<unsigned, std::uint32_t>> held) { for (const auto& [n, v] : held) value[n] = v; }
    void apply() const { for (unsigned n = 13; n < 32; ++n) c.set(n, value[n]); }
private:
    Cpu& c;
    std::uint32_t value[32]{};
};

// `bl entry` with the arguments in r3 onwards; the callee's r3. For routines outside the unit.
inline std::uint32_t invoke(Cpu& c, std::uint32_t entry, std::uint32_t back,
                            std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    c.call(entry, back);
    return c.r(3);
}
// The same for a routine of this unit that lives in another of its files: runs its C++ in place.
inline std::uint32_t invoke_unit(Cpu& c, std::uint32_t entry, std::uint32_t back,
                                 std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    call(c, entry, back);
    return c.r(3);
}
}

#include "formation_record.hpp"
#include <array>
#include <span>

namespace cm::aot::formation {
using guest::Cpu;
using guest::Frame;
namespace {
// FORMATION.CPP's string pool (a TOC slot), the assert strings and the shared
// success flag.
constexpr std::int32_t pool_slot = -0x550c, message_slot = -0x7ba8, success_slot = -0x7bac;
constexpr std::int32_t clubs_slot = -0x7be8;  // -> the club records
constexpr std::uint32_t club_size = 0x248, club_name = 0x221;
constexpr std::uint32_t club_name_form = 0x23b;  // int8 the text formatter takes with the name
// Routines still called through the dispatcher.
constexpr std::uint32_t clear_memory = 0x10778300, copy_string = 0x1077dbf0, string_length = 0x1077dbd0;
constexpr std::uint32_t format_text = 0x10727890;  // translated texts: format, then arguments
constexpr std::uint32_t print = 0x1077b720;        // sprintf
constexpr std::uint32_t set_position = 0x10111310, update_zone_grid = 0x10116560;

// Texts in the pool.
namespace text {
constexpr std::uint32_t uninitialised = 0x0, formation_vs = 0x2f, empty = 0x74c, name = 0x778, abbreviation = 0x8b7;
constexpr std::uint32_t goalkeeper = 0x6d2, goalkeeper_short = 0x794, free_role = 0x8bc;
}

guest::Assertion assertion(Cpu& c, std::uint32_t at, const Frame& frame, std::int32_t name, std::int32_t extension,
                           std::uint32_t line) {
    const auto pool = c.toc(pool_slot);
    return {at, pool + 0xe, pool + 0x1c, pool + 0x29, frame.at(name), frame.at(extension), line};
}
bool valid(std::int8_t player) { return player >= 0 && player < players; }
std::uint32_t extend(std::int16_t value) { return std::uint32_t(std::int32_t(value)); }
// cr0 as a signed compare or a record-form result leaves it (LT 8, GT 4,
// EQ 2, with XER[SO] copied).
void set_cr0(Cpu& c, std::int64_t a, std::int64_t b) {
    c.set_cr(0, std::uint8_t((a < b ? 8 : a > b ? 4 : 2) | (c.s.r<1>(guest::reg::xer_so) & 1)));
}

// Instructions 0xa:0x15552221, role bits 0x12 and no player in every slot.
constexpr std::uint64_t default_instructions = 0x0000000a'15552221;
void reset_players(Cpu& c, std::uint32_t formation) {
    for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
        c.put64(formation + field::instructions + player * 8, default_instructions);
        c.put8(formation + field::role + player, 0x12);
        c.put32(formation + field::player + player * 4, 0xffffffffu);
    }
}
// Builds the zone grids, then marks the record saved and unchanged.
void finish_record(Cpu& c, std::uint32_t formation, std::uint32_t return_address) {
    c.set(3, formation);
    c.call(update_zone_grid, return_address);
    c.put8(formation + field::saved, 1);
    c.put8(formation + field::changed, 0);
    c.put8(formation + field::flag_1886, 0);
}

// The default 4-4-2, slot by slot.
constexpr std::array<std::uint16_t, 11> default_positions{0x001, 0x804, 0x084, 0x104, 0x404, 0x110,
                                                          0x810, 0x410, 0x140, 0x440, 0x090};

// The bits of the team options word an option belongs to; 0 for none.
std::uint32_t team_option_field(std::int32_t option) {
    switch (option) {
    case 0x1: case 0x2: case 0x4: case 0x8: return 0xf;
    case 0x10: case 0x20: case 0x40: return 0x70;
    case 0x80: case 0x100: return 0x180;
    case 0x200: case 0x400: return 0x600;
    case 0x800: case 0x1000: return 0x1800;
    case 0x2000: case 0x4000: return 0x6000;
    case 0x8000: case 0x10000: case 0x20000: return 0x38000;
    default: return 0;
    }
}
// The value the original's binary search (cmpwi) last compares a valid
// option with: the result is left in cr0.
std::int32_t team_option_last_compare(std::int32_t option) {
    if (option == 0x200) return 0x200;
    if (option < 0x200) {
        if (option == 0x20) return 0x20;
        if (option < 0x20) {
            if (option == 8) return 8;
            if (option < 8) return option > 3 ? 5 : 1;
            return 0x10;
        }
        if (option == 0x80) return 0x80;
        return option > 0x80 ? 0x100 : 0x40;
    }
    if (option == 0x4000) return 0x4000;
    if (option < 0x4000) {
        if (option == 0x1000) return 0x1000;
        if (option > 0x1000) return 0x2000;
        return option == 0x800 ? 0x800 : 0x400;
    }
    if (option == 0x10000) return 0x10000;
    return option > 0x10000 ? 0x20000 : 0x8000;
}
// The bits of a player's instruction word an option belongs to; 0 for none.
// The groups are the original's: 0x40000000 shares the field of 0x10000 and
// 0x20000, and 0x80000000 that of 0x40000 and 0x80000, whose field also
// takes the whole high word (setting one of those clears it).
std::uint64_t instruction_field(std::uint64_t option) {
    switch (option) {
    case 0x1: case 0x2: case 0x4: case 0x8: case 0x10: return 0x1f;
    case 0x20: case 0x40: case 0x80: case 0x100: return 0x1e0;
    case 0x200: case 0x400: case 0x800: case 0x1000: return 0x1e00;
    case 0x2000: case 0x4000: case 0x8000: return 0xe000;
    case 0x10000: case 0x20000: case 0x40000000: return 0x40030000;
    case 0x40000: case 0x80000: case 0x80000000: return 0xffffffff'800c0000;
    case 0x100000: case 0x200000: return 0x300000;
    case 0x400000: case 0x800000: return 0xc00000;
    case 0x1000000: case 0x2000000: return 0x3000000;
    case 0x4000000: case 0x8000000: return 0xc000000;
    case 0x10000000: case 0x20000000: return 0x30000000;
    case 0x1'00000000: case 0x2'00000000: return 0x3'00000000;
    case 0x4'00000000: case 0x8'00000000: return 0xc'00000000;
    default: return 0;
    }
}
// r9 as the original's binary search over the option leaves it when the
// option belongs to no field (`r9` on entry otherwise); the assertion's
// sprintf spills it into the stack image.
std::uint32_t search_r9(std::int64_t option, std::uint32_t low, std::uint32_t r9) {
    if (option > 0x40000) {
        if (option > 0x8000000) {
            r9 = option < 0x1'00000000 ? low ^ (option < 0x40000000 ? 0x20000000 : 0x40000000) : low ^ 0x8000000;
        } else if (option > 0x800000) {
            r9 = low ^ 0x2000000;
        } else {
            r9 = low ^ (option < 0x200000 ? 0x100000 : 0x200000);
        }
    } else if (option > 0x4000 && option < 0x40000) {
        r9 = 0x10000;
    }
    return r9;
}

// One text of a name: the position bit it names, its format in the pool and
// the address after the original's call that formats it.
struct Part {
    std::uint16_t bit, text;
    std::uint32_t return_address;
};
const Part* first_set(std::span<const Part> parts, std::uint16_t position) {
    for (const auto& part : parts)
        if (position & part.bit) return &part;
    return nullptr;
}
// The formatter, into `out`; r5-r10 stay as the caller has them.
void format(Cpu& c, std::uint32_t out, std::uint32_t pattern, std::uint32_t return_address) {
    c.set({{3, out}, {4, pattern}});
    c.call(format_text, return_address);
}
// A 100-byte text buffer initialised from static data, as the original copies
// it (twelve 8-byte steps, then two halfwords).
void copy_buffer(Cpu& c, std::uint32_t to, std::uint32_t from) {
    for (std::uint32_t offset = 0; offset < 96; offset += 4) c.put32(to + offset, c.u32(from + offset));
    c.put16(to + 96, c.u16(from + 96));
    c.put16(to + 98, c.u16(from + 98));
}
// The forward line's format takes one (empty) argument.
constexpr std::uint16_t forward = 0x40;
}

std::uint32_t initialise(Cpu& c, std::uint32_t formation) {
    Frame frame(c, 0x10110c80, 0x40, {.first_gpr = 31});
    c.set({{31, formation}, {3, formation}, {4, 0}, {5, record_size}});
    c.call(clear_memory, 0x10110ca0);
    c.set({{3, formation}, {4, c.toc(pool_slot) + text::uninitialised}});
    c.call(copy_string, 0x10110cac);
    c.put8(formation + field::password, 0);
    c.put8(formation + field::creator, 0);
    c.set({{3, formation + field::position_set}, {4, 0}, {5, std::uint32_t(players)}});
    c.call(clear_memory, 0x10110cc8);
    for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
        c.set({{3, formation}, {4, player}, {5, default_positions[player]}, {6, 0}});
        c.call(set_position, 0x10110cdc + player * 0x14);
    }
    c.put32(formation + field::team_options, 0x22a91);
    reset_players(c, formation);
    finish_record(c, formation, 0x10110f0c);
    return formation;
}

std::uint32_t initialise_from_tactics(Cpu& c, std::uint32_t formation, std::uint32_t source, std::uint32_t side) {
    Frame frame(c, 0x10110f40, 0x80, {.first_gpr = 25});
    const auto pool = c.toc(pool_slot);
    c.set({{27, c.toc(message_slot)}, {28, pool}, {29, formation}, {30, source}, {31, side}});
    if (!source) {
        guest::report_assertion(c, assertion(c, 0x10110f80, frame, 0x40, 0x38, 0xae));
        c.set(4, c.toc(success_slot));
        return formation;
    }
    // Named after the other side's club.
    const bool second = std::int8_t(side) != 0;
    const auto clubs = c.u32(c.toc(clubs_slot));
    const auto club = clubs + c.u32(source + (second ? 0 : 4)) * club_size;
    const auto form = std::uint32_t(std::int32_t(c.i8(club + club_name_form)));
    c.set({{3, formation}, {4, pool + text::formation_vs}, {5, form}, {6, form}, {7, club + club_name}, {8, clubs}});
    c.call(format_text, second ? 0x1011102c : 0x10110ffc);
    c.put8(formation + field::password, 0);
    c.put8(formation + field::creator, 0);
    c.set({{3, formation + field::position_set}, {4, 0}, {5, std::uint32_t(players)}});
    c.call(clear_memory, 0x10111048);

    // The side's positions without (+0x156) and with the ball (+0x16c).
    const auto tactics = std::uint32_t(std::int32_t(std::int8_t(side))) * 0x664;
    c.set(28, tactics);
    for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
        const auto positions = source + tactics + player * 2;
        c.set({{25, player}, {26, positions}, {27, player * 2}});
        c.set({{3, formation}, {4, player}, {5, extend(c.i16(positions + 0x156))}, {6, 0}});
        c.call(set_position, 0x10111074);
        c.set({{3, formation}, {4, player}, {5, extend(c.i16(positions + 0x16c))}, {6, 1}});
        c.call(set_position, 0x10111088);
    }
    c.set({{25, std::uint32_t(players)}, {27, std::uint32_t(players) * 2}});
    c.put32(formation + field::team_options, c.u32(source + tactics + 0x184));
    reset_players(c, formation);
    finish_record(c, formation, 0x10111208);
    return formation;
}

std::uint32_t set_team_option(Cpu& c, std::uint32_t formation, std::int32_t option) {
    Frame frame(c, 0x10113b60, 0x70, {.first_gpr = 30});
    const auto mask = team_option_field(option);
    if (!mask) {
        c.set({{31, c.toc(pool_slot)}, {30, c.toc(message_slot)}});
        guest::report_assertion(c, assertion(c, 0x10113db4, frame, 0x40, 0x38, 0x676));
        c.set(4, c.toc(success_slot));
        return 0;
    }
    const auto word = formation + field::team_options;
    const auto old = c.u32(word), cleared = old & ~mask, value = cleared | std::uint32_t(option);
    c.put32(word, value);
    c.put8(formation + field::saved, 0);
    c.set({{4, value}, {5, cleared}, {6, old}});  // as the original leaves them (see set_instruction)
    set_cr0(c, option, team_option_last_compare(option));
    return 1;
}

std::uint32_t set_instruction(Cpu& c, std::uint32_t formation, std::int8_t player, std::uint64_t option) {
    Frame frame(c, 0x10113e30, 0xa0, {.first_gpr = 29});
    const auto fail = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
        c.set({{31, c.toc(pool_slot)}, {30, c.toc(message_slot)}, {29, c.toc(success_slot)}});
        guest::report_assertion(c, assertion(c, at, frame, name, extension, line));
        return 0u;
    };
    if (!valid(player)) return fail(0x10113e7c, 0x68, 0x40, 0x69d);
    const auto mask = instruction_field(option);
    if (!mask) {
        c.set(9, search_r9(std::int64_t(option), std::uint32_t(option), c.r(9)));
        return fail(0x1011498c, 0x48, 0x38, 0x70a);
    }
    const auto base = formation + std::uint32_t(player) * 8, word = base + field::instructions;
    const auto old = c.u64(word), value = (old & ~mask) | option;
    c.put64(word, value);
    c.put8(formation + field::saved, 0);
    // Scratch registers as the original leaves them: the caller's next
    // varargs call spills r4-r10 into the stack image.
    const auto old_high = std::uint32_t(old >> 32), old_low = std::uint32_t(old);
    if (mask >> 32 == 0xffffffff) c.set({{4, 0}, {7, 0}, {8, old_low}, {9, base}});
    else c.set({{4, std::uint32_t(value >> 32)}, {7, std::uint32_t((old & ~mask) >> 32)}, {8, old_high}, {9, old_low}, {10, base}});
    // cr0: the search's last record-form result, zero (a match) except for
    // 4, reached when the range test against 5 fails (1).
    set_cr0(c, option == 4 ? 1 : 0, 0);
    return 1;
}

std::uint32_t set_role(Cpu& c, std::uint32_t formation, std::int8_t slot, std::uint32_t role) {
    Frame frame(c, 0x10114a00, 0x70, {.first_gpr = 30});
    std::uint8_t keep;
    switch (std::int8_t(role)) {
    case 0x1: case 0x2: case 0x4: keep = 0xf8; break;
    case 0x8: case 0x10: keep = 0xe7; break;
    default:
        c.set({{31, c.toc(pool_slot)}, {30, c.toc(message_slot)}});
        guest::report_assertion(c, assertion(c, 0x10114ab0, frame, 0x40, 0x38, 0x7b8));
        c.set(4, c.toc(success_slot));
        return 0;
    }
    const auto slot_base = formation + std::uint32_t(std::int32_t(slot)), bits = slot_base + field::role;
    const auto cleared = std::uint32_t(c.u8(bits) & keep), value = cleared | role;
    c.put8(bits, value);
    c.put8(formation + field::saved, 0);
    c.set({{4, value}, {6, slot_base}});  // as the original leaves them (see set_instruction)
    // cr0: `andi.` of the cleared byte for 8 and 0x10, else `cmplwi role, 16`.
    if (keep == 0xe7) set_cr0(c, cleared, 0);
    else set_cr0(c, -1, 0);
    return 1;
}

std::uint32_t position_name(Cpu& c, std::int16_t position, std::uint32_t buffer, std::int32_t size,
                            std::uint32_t strict) {
    static constexpr std::array<Part, 6> lines{{{0x2, 0x6dd, 0x10116aa8}, {0x4, 0x6e5, 0x10116ac0},
                                                {0x8, 0x6ee, 0x10116ad8}, {0x10, 0x703, 0x10116af0},
                                                {0x20, 0x70e, 0x10116b08}, {forward, 0x723, 0x10116b24}}};
    static constexpr std::array<Part, 5> sides{{{0x80, 0x74d, 0x10116b9c}, {0x100, 0x752, 0x10116bb4},
                                                {0x200, 0x75e, 0x10116bcc}, {0x400, 0x765, 0x10116be4},
                                                {0x800, 0x772, 0x10116bfc}}};
    Frame frame(c, 0x10116970, 0x190, {.first_gpr = 26});
    const auto pool = c.toc(pool_slot);
    c.set({{26, buffer}, {27, strict}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, pool}});
    // The line and side texts start as copies of two static buffers; the
    // copy loops leave r4 and r6-r9 as below, which the calls pass on.
    const auto line_text = frame.at(0x114), side_text = frame.at(0xb0);
    const auto line_initial = c.toc(-0x5518), side_initial = c.toc(-0x551c);
    copy_buffer(c, line_text, line_initial);
    copy_buffer(c, side_text, side_initial);
    c.set({{4, c.u16(side_initial + 0x60)}, {6, side_initial + 0x5c}, {7, frame.at(0x10c)},
           {8, c.u16(line_initial + 0x62)}, {9, frame.at(0x170)}});
    const auto fail = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
        guest::report_assertion(c, assertion(c, at, frame, name, extension, line));
        return 0u;
    };
    if (!buffer || size < 100) return fail(0x10116a28, 0x90, 0x48, 0xc56);
    c.set(28, extend(position));
    c.put8(buffer, 0);
    const auto code = std::uint16_t(position);
    if (code & 0x1) {
        format(c, buffer, pool + text::goalkeeper, 0x10116a8c);
        return 1;
    }
    if (const auto* line = first_set(lines, code)) {
        if (line->bit == forward) c.set(5, pool + text::empty);
        format(c, line_text, pool + line->text, line->return_address);
    } else if (std::int8_t(strict)) {
        return fail(0x10116b44, 0x70, 0x40, 0xc93);
    }
    if (const auto* side = first_set(sides, code)) format(c, side_text, pool + side->text, side->return_address);
    else if (std::int8_t(strict)) return fail(0x10116c1c, 0x50, 0x38, 0xcc0);
    c.set({{3, buffer}, {4, pool + text::name}, {5, line_text}, {6, side_text}});
    c.call(format_text, 0x10116c74);
    return 1;
}

std::uint32_t position_abbreviation(Cpu& c, std::int16_t position, std::uint32_t buffer, std::int32_t size,
                                    std::uint32_t line_alone) {
    static constexpr std::array<Part, 6> lines{{{0x2, 0x7b2, 0x10116d5c}, {0x4, 0x7cd, 0x10116d78},
                                                {0x8, 0x7e8, 0x10116d94}, {0x10, 0x810, 0x10116db0},
                                                {0x20, 0x82d, 0x10116dcc}, {0x40, 0x855, 0x10116de8}}};
    // The three central sides share "C".
    static constexpr std::array<Part, 5> sides{{{0x80, 0x86f, 0x10116e88}, {0x100, 0x886, 0x10116ea4},
                                                {0x200, 0x886, 0x10116ec0}, {0x400, 0x886, 0x10116edc},
                                                {0x800, 0x89f, 0x10116ef8}}};
    Frame frame(c, 0x10116c90, 0xe0, {.first_gpr = 26});
    const auto pool = c.toc(pool_slot);
    c.set({{26, line_alone}, {28, buffer}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, pool}});
    const auto fail = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
        guest::report_assertion(c, assertion(c, at, frame, name, extension, line));
        return 0u;
    };
    if (!buffer || size < 10) return fail(0x10116cd4, 0xa0, 0x48, 0xcde);
    c.set(27, extend(position));
    c.put8(buffer, 0);
    const auto code = std::uint16_t(position);
    // Every format takes one (empty) argument.
    if (code & 0x1) {
        c.set(5, pool + text::empty);
        format(c, buffer, pool + text::goalkeeper_short, 0x10116d3c);
        return 1;
    }
    const auto line_text = frame.at(0x58), side_text = frame.at(0x50);
    const auto* line = first_set(lines, code);
    if (!line) return fail(0x10116e00, 0x80, 0x40, 0xd19);
    c.set(5, pool + text::empty);
    format(c, line_text, pool + line->text, line->return_address);
    if (std::int8_t(line_alone)) {
        c.set(3, line_text);
        c.call(string_length, 0x10116e54);
        if (c.r(3) > 1) {
            // The line text is printed as the format.
            c.set({{3, buffer}, {4, line_text}});
            c.call(print, 0x10116e68);
            return 1;
        }
    }
    const auto* side = first_set(sides, code);
    if (!side) return fail(0x10116f10, 0x60, 0x38, 0xd4b);
    c.set(5, pool + text::empty);
    format(c, side_text, pool + side->text, side->return_address);
    c.set({{3, buffer}, {4, pool + text::abbreviation}, {5, line_text}, {6, side_text}});
    c.call(print, 0x10116f68);
    return 1;
}

std::uint32_t position_part_name(Cpu& c, std::int16_t part, std::uint32_t buffer, std::int32_t size) {
    static constexpr std::array<Part, 13> names{{
        {0x1, text::goalkeeper, 0x10117028}, {0x2, 0x6dd, 0x10117044}, {0x4, 0x6e5, 0x10117060},
        {0x8, 0x6ee, 0x1011707c}, {0x10, 0x703, 0x10117098}, {0x20, 0x70e, 0x101170b4},
        {forward, 0x723, 0x101170d4}, {0x80, 0x74d, 0x101170f0}, {0x100, 0x752, 0x1011710c},
        {0x200, 0x75e, 0x10117128}, {0x400, 0x765, 0x10117144}, {0x800, 0x772, 0x10117160},
        {0x1000, text::free_role, 0x1011717c}}};
    Frame frame(c, 0x10116f80, 0x70, {.first_gpr = 30});
    const auto pool = c.toc(pool_slot);
    c.set({{31, pool}, {30, c.toc(message_slot)}});
    if (!buffer || size < 100) {
        guest::report_assertion(c, assertion(c, 0x10116fc0, frame, 0x40, 0x38, 0xd63));
        c.set(4, c.toc(success_slot));
        return 0;
    }
    c.put8(buffer, 0);
    for (const auto& name : names) {
        if (part != std::int16_t(name.bit)) continue;
        if (name.bit == forward) c.set(5, pool + text::empty);
        format(c, buffer, pool + name.text, name.return_address);
        return 1;
    }
    c.set({{3, buffer}, {4, pool + text::empty}});
    c.call(copy_string, 0x10117190);
    return 1;
}

namespace {
// The wide checks' cr0: the line bit test (`rlwinm.`, positive) that found
// one, or the loop's last compare (11 with 11).
std::uint32_t wide_found(Cpu& c) { set_cr0(c, 1, 0); return 0; }
std::uint32_t none_wide(Cpu& c) { set_cr0(c, 0, 0); return 1; }
}

std::uint32_t no_wide_midfield_or_attack(Cpu& c, std::uint32_t formation) {
    Frame frame(c, 0x101182a0, 0x280, {.first_gpr = 26});
    const Record record(c, formation);
    for (std::int8_t player = 0; player < players; ++player) {
        const auto without = std::uint16_t(record.position(player)), with = std::uint16_t(record.position(player, true));
        // The original checks the defensive midfield line (0x8) without the
        // ball only.
        if (((without & 0x880) && (without & (0x40 | 0x20 | 0x10 | 0x8))) ||
            ((with & 0x880) && (with & (0x40 | 0x20 | 0x10))))
            return wide_found(c);
    }
    return none_wide(c);
}

std::uint32_t no_wide_defence(Cpu& c, std::uint32_t formation) {
    Frame frame(c, 0x10118a40, 0x190, {.first_gpr = 26});
    const Record record(c, formation);
    for (std::int8_t player = 0; player < players; ++player)
        for (const bool with_ball : {false, true}) {
            const auto code = std::uint16_t(record.position(player, with_ball));
            if ((code & 0x880) && (code & (0x4 | 0x8))) return wide_found(c);
        }
    return none_wide(c);
}
}

// DISPLAY.CPP 0x1071de80: commands 9-26 (the display table's control records and the text buffers). See
// display_interpreter.hpp. Each command is written from the listing between its address and the common
// exit 0x10723638 (r3 = -1); the stream reads are the original's bounds-checked reads (network.h).
#include "display_interpreter.hpp"
#include <bit>
#include <optional>

namespace cm::aot::display {
namespace {
namespace field {  // the stream: {bytes, position, length}
inline constexpr std::uint32_t position = 0x4, length = 0x8;
}
// Slots of a display table, displacements from table + 0x130000 (the generated `addis rX,table,0x13`).
namespace slot {
inline constexpr std::int32_t flag_c = -0x56c0,        // byte
                              mark_clear = -0x5700,    // byte
                              value_a = -0x56fc,       // halfword set by command 19
                              value_b = -0x56fa;       // halfword set by command 20
}
std::uint32_t high(const Interpreter& s) { return s.table + table::high; }
std::uint32_t at(const Interpreter& s, std::int32_t displacement) { return high(s) + std::uint32_t(displacement); }

constexpr Site site(std::uint32_t where, std::int32_t name, std::int32_t extension, std::uint32_t line) {
    return {where, name, extension, line, text::network_header};
}
std::uint32_t call(Interpreter& s, std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
    s.regs.apply();
    return invoke(s.c, entry, back, arguments);
}
std::uint32_t call_unit(Interpreter& s, std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
    s.regs.apply();
    return invoke_unit(s.c, entry, back, arguments);
}
}

// 0x10722774: with the table's flag set, only when its counts agree with the table's; marks the table and,
// when the third flag is set, calls 0x10718fc0 with every argument -1.
std::uint32_t command_9(Interpreter& s) {
    Cpu& c = s.c;
    const auto flag = c.i16(s.table + table::announced_area_count);
    if (flag != 0) {
        if (c.i16(at(s, table::area_count - table::high)) != flag) return common_exit_result;
        if (c.i16(at(s, table::guio_count - table::high)) != c.i16(s.table + table::announced_guio_count)) return common_exit_result;
    }
    c.put8(s.table + table::unsent, 1);
    c.put8(at(s, slot::mark_clear), 0);
    if (c.i8(at(s, slot::flag_c)) != 0)
        call_unit(s, 0x10718fc0, 0x107227d8, {s.table, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu});
    return common_exit_result;
}

// 0x107227dc: a text message: the engine call with {word, byte, byte, text} is replayed into the output
// buffer (TOC -0x5c04), then handed to 0x1073a4c0.
std::uint32_t command_10(Interpreter& s) {
    Cpu& c = s.c;
    const auto& frame = s.frame;
    auto& regs = s.regs;
    if (const auto v = s.source.read32(site(0x10722804, 0x894, 0x19c, 0x93))) c.put32(frame.at(0x1818), *v);
    if (const auto v = s.source.read8(site(0x10722890, 0x874, 0x194, 0x8f))) regs.hold({{20, *v}});
    if (const auto v = s.source.read8(site(0x10722904, 0x854, 0x18c, 0x8f))) regs.hold({{24, *v}});
    if (const auto v = s.source.read16_signed(site(0x10722978, 0x514, 0xbc, 0x90))) regs.hold({{13, *v}});
    const auto text = frame.at(0x1714);
    // 0x10739ef0 copies r13 bytes (no bounds check in the original) from the stream into the frame.
    call(s, 0x10739ef0, 0x107229e8,
         {text, s.source.at_position(), regs[13]});
    s.source.advance(regs[13]);
    regs.hold({{17, 0xffffffffu}});
    if (c.u32(regs[19]) != 0) {
        call_unit(s, 0x10723650, 0x10722a10, {text, 0});
        const auto result = call(s, 0x10744860, 0x10722a24, {c.u32(regs[19]), regs[20], text, regs[24]});
        regs.hold({{17, result}});
        call(s, 0x10744ca0, 0x10722a3c, {c.u32(regs[19]), result, 0x64});
    }
    const auto out_address = c.toc(-0x5c04);
    c.put32(out_address + field::position, 0);
    regs.hold({{13, out_address + field::position}, {14, out_address + field::length}});
    CheckedStream out(c, frame, out_address, regs);
    // Each store is checked on its own: a failed one reports and the next still runs.
    out.write8(site(0x10722a78, 0x834, 0x184, 0x85), 0xd);
    out.write32(site(0x10722afc, 0x814, 0x17c, 0x89), c.u32(frame.at(0x1818)));  // stwbrx: byte-reversed
    out.write8(site(0x10722b80, 0x7f4, 0x174, 0x85), regs[17]);
    const auto saved_half = sign16(c.u16(frame.at(0x18d2)));
    c.put16(regs[18], saved_half);
    const auto done = call(s, 0x1073a4c0, 0x10722bf8,
                           {c.u32(frame.at(0x18cc)), out.bytes(), out.position(), saved_half});
    if (std::int8_t(done) != 0) return common_exit_result;
    // A failed hand-over: the assertion text with the source file, formatted as the report's but with a
    // different tail (ODD site 0x10722c14: no success-flag clear, two string appends instead).
    const auto pool = regs[30];
    regs.apply();
    c.set({{3, pool}, {4, 0}, {5, 0}, {6, frame.at(0x15b4)}, {7, frame.at(0x4e4)}});
    c.call(0x10739890, 0x10722c18);
    c.call(0x10768810, 0x10722c1c);
    c.set({{5, c.r(3)}, {3, regs[29]}, {4, pool + text::report_format}, {6, frame.at(0x15b4)}, {7, frame.at(0x4e4)}, {8, 0xb28}});
    c.call(0x1077b720, 0x10722c38);
    c.set({{3, c.toc(-0x79dc)}, {4, pool + 0x95}});
    c.call(0x10727890, 0x10722c44);
    c.set({{3, regs[26]}, {5, regs[29]}, {4, pool + 0xa3}});
    c.call(0x10727890, 0x10722c54);
    c.set({{3, c.toc(-0x79dc)}, {4, regs[26]}, {5, 0}});
    c.call(0x10769b60, 0x10722c64);
    return common_exit_result;
}

// 0x10722c68: byte r17; with the TOC -0x7468 object, -1 calls 0x10744700, anything else 0x10744770(object, byte).
std::uint32_t command_11(Interpreter& s) {
    Cpu& c = s.c;
    if (const auto v = s.source.read8(site(0x10722c90, 0x7d4, 0x16c, 0x8f))) s.regs.hold({{17, *v}});
    const auto object = c.u32(s.regs[19]);
    if (object == 0) return common_exit_result;
    if (std::int8_t(s.regs[17]) == -1) call(s, 0x10744700, 0x10722cf8, {object});
    else call(s, 0x10744770, 0x10722d04, {object, s.regs[17]});
    return common_exit_result;
}

// 0x10722d08: byte r17 and a halfword (frame +0x1828); with the TOC -0x7468 object calls 0x10744ca0.
std::uint32_t command_12(Interpreter& s) {
    Cpu& c = s.c;
    if (const auto v = s.source.read8(site(0x10722d30, 0x7b4, 0x164, 0x8f))) s.regs.hold({{17, *v}});
    if (const auto v = s.source.read16_signed(site(0x10722da4, 0x794, 0x15c, 0x90))) c.put16(s.frame.at(0x1828), *v);
    const auto object = c.u32(s.regs[19]);
    if (object == 0) return common_exit_result;
    call(s, 0x10744ca0, 0x10722e18, {object, s.regs[17], sign16(c.u16(s.frame.at(0x1828)))});
    return common_exit_result;
}

// 0x10722e1c: a word (frame +0x1818) and a byte r17, then 0x10742e60(game object, byte, word).
std::uint32_t command_13(Interpreter& s) {
    Cpu& c = s.c;
    if (const auto v = s.source.read32(site(0x10722e44, 0x774, 0x154, 0x93))) c.put32(s.frame.at(0x1818), *v);
    if (const auto v = s.source.read8(site(0x10722ed0, 0x754, 0x14c, 0x8f))) s.regs.hold({{17, *v}});
    call(s, 0x10742e60, 0x10722f2c, {s.regs[14], s.regs[17], c.u32(s.frame.at(0x1818))});
    return common_exit_result;
}

// 0x10722f30: a byte r17, then 0x10742e90(game object, byte).
std::uint32_t command_14(Interpreter& s) {
    if (const auto v = s.source.read8(site(0x10722f58, 0x734, 0x144, 0x8f))) s.regs.hold({{17, *v}});
    call(s, 0x10742e90, 0x10722fb0, {s.regs[14], s.regs[17]});
    return common_exit_result;
}

// 0x10722fb4: a halfword (frame +0x72) and a word r17 stored into the current human's row (+0x1c, +0x20).
std::uint32_t command_15(Interpreter& s) {
    Cpu& c = s.c;
    if (const auto v = s.source.read16_signed(site(0x10722fdc, 0x714, 0x13c, 0x90))) c.put16(s.frame.at(0x72), *v);
    if (const auto v = s.source.read32(site(0x1072306c, 0x6f4, 0x134, 0x92))) s.regs.hold({{17, *v}});
    const auto game_object = s.regs[14];
    const auto row = [&] {
        return game_object + std::uint32_t(std::int32_t(c.i16(game_object + game::current_human))) * game::human_size;
    };
    c.put16(row() + 0x1c, c.u16(s.frame.at(0x72)));
    c.put32(row() + 0x20, s.regs[17]);
    return common_exit_result;
}

// 0x107230f4: a halfword (frame +0x72, the area index) and a word r17, then 0x107145f0(area, word).
std::uint32_t command_16(Interpreter& s) {
    Cpu& c = s.c;
    if (const auto v = s.source.read16_signed(site(0x1072311c, 0x6d4, 0x12c, 0x90))) c.put16(s.frame.at(0x72), *v);
    if (const auto v = s.source.read32(site(0x107231ac, 0x6b4, 0x124, 0x92))) s.regs.hold({{17, *v}});
    const auto area = s.table + std::uint32_t(std::int32_t(c.i16(s.frame.at(0x72)))) * table::area_size + table::areas;
    call(s, 0x107145f0, 0x10723224, {area, s.regs[17]});
    return common_exit_result;
}

// 0x10723228 and 0x10723238: clear the byte the TOC word -0x744c / -0x5c00 points to.
std::uint32_t command_17(Interpreter& s) {
    s.c.put8(s.c.toc(-0x744c), 0);
    return common_exit_result;
}
std::uint32_t command_18(Interpreter& s) {
    s.c.put8(s.c.toc(-0x5c00), 0);
    return common_exit_result;
}

// 0x10723248 and 0x107232e0: a halfword into the table's slot (a failed read leaves it and exits).
std::uint32_t command_19(Interpreter& s) {
    if (const auto v = s.source.read16_signed(site(0x10723270, 0x694, 0x11c, 0x90))) s.c.put16(at(s, slot::value_a), *v);
    return common_exit_result;
}
std::uint32_t command_20(Interpreter& s) {
    if (const auto v = s.source.read16_signed(site(0x10723308, 0x674, 0x114, 0x90))) s.c.put16(at(s, slot::value_b), *v);
    return common_exit_result;
}

// 0x10723378: 0x1074cd90(TOC -0x7408 object, 0, 0, 0).
std::uint32_t command_21(Interpreter& s) {
    call(s, 0x1074cd90, 0x1072338c, {s.regs[25], 0, 0, 0});
    return common_exit_result;
}

// 0x10723390: with the table's list, 0x1074cd90(TOC -0x7408 object, array, count, capacity halfword).
std::uint32_t command_22(Interpreter& s) {
    Cpu& c = s.c;
    const auto list = c.u32(s.table + table::string_list);
    if (list == 0) return common_exit_result;
    call(s, 0x1074cd90, 0x107233b4, {s.regs[25], c.u32(list), c.u32(list + 4), sign16(c.u16(list + 8))});
    return common_exit_result;
}

// 0x107233b8: allocates the table's list (12 bytes through 0x10775200(1, 12)) and reads its capacity halfword.
std::uint32_t command_23(Interpreter& s) {
    Cpu& c = s.c;
    const auto list = call(s, 0x10775200, 0x107233c4, {1, 0xc});
    c.put32(s.table + table::string_list, list);
    if (list == 0) return common_exit_result;
    if (const auto v = s.source.read16_signed(site(0x10723400, 0x654, 0x10c, 0x90))) c.put16(list + 8, *v);
    return common_exit_result;
}

// 0x1072346c: frees the table's list: each element through 0x10775170, the array, and clears the slot.
// The original's loop keeps r13 (table + 0x130000), r14 (offset) and r16 (index) while it calls.
std::uint32_t command_24(Interpreter& s) {
    Cpu& c = s.c;
    const auto base = high(s);
    s.regs.hold({{13, base}});
    if (c.u32(s.table + table::string_list) == 0) return common_exit_result;
    s.regs.hold({{16, 0}, {14, 0}});
    for (;;) {
        const auto list = c.u32(s.table + table::string_list);
        if (!(std::int32_t(std::int16_t(s.regs[16])) < std::int32_t(c.u32(list + 4)))) break;  // cmpw
        call(s, 0x10775170, 0x10723494, {c.u32(c.u32(list) + s.regs[14])});
        s.regs.hold({{14, s.regs[14] + 4}, {16, s.regs[16] + 1}});
    }
    call(s, 0x10775170, 0x107234b8, {c.u32(c.u32(s.table + table::string_list))});
    c.put32(s.table + table::string_list, 0);
    return common_exit_result;
}

// 0x107234c8, also index 26: appends an element to the table's list. Command 25 first reads a length-
// prefixed text into the frame (+0x1714) and stores a copy; command 26 appends a null.
std::uint32_t command_25(Interpreter& s) {
    Cpu& c = s.c;
    const auto text_command = s.command == 0x19;
    const auto text = s.frame.at(0x1714);
    if (text_command) {
        if (const auto v = s.source.read16_signed(site(0x107234fc, 0x4f4, 0xb4, 0x90))) s.regs.hold({{13, *v}});
        const auto position = s.source.position();
        call(s, 0x10739ef0, 0x1072356c, {text, s.source.bytes() + position, s.regs[13]});
        s.source.advance(s.regs[13]);
    }
    auto list = c.u32(s.table + table::string_list);
    if (list == 0) return common_exit_result;
    const auto grown = call(s, 0x107751b0, 0x1072359c, {c.u32(list), (c.u32(list + 4) + 1) << 2});
    if (grown == 0) return common_exit_result;
    list = c.u32(s.table + table::string_list);
    c.put32(list, grown);
    if (text_command) {
        const auto length = call(s, 0x1077dbd0, 0x107235c4, {text});
        const auto copy = call(s, 0x10775130, 0x107235cc, {length + 1});
        list = c.u32(s.table + table::string_list);
        c.put32(c.u32(list) + (c.u32(list + 4) << 2), copy);
        list = c.u32(s.table + table::string_list);
        const auto index = c.u32(list + 4);
        const auto array = c.u32(list);
        if (c.u32(array + (index << 2)) == 0) return common_exit_result;
        c.put32(list + 4, index + 1);
        call(s, 0x1077dbf0, 0x10723614, {c.u32(array + (index << 2)), text});
    } else {
        list = c.u32(s.table + table::string_list);
        const auto index = c.u32(list + 4);
        const auto array = c.u32(list);
        c.put32(list + 4, index + 1);
        c.put32(array + (index << 2), 0);
    }
    return common_exit_result;
}
}

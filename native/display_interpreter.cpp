// DISPLAY.CPP 0x1071de80: the screen-command interpreter's dispatcher, and commands 0-5.
// See display_interpreter.hpp for how the routine is split.
#include "display_interpreter.hpp"
#include <array>
#include <bit>
#include <optional>

namespace cm::aot::display {
namespace {
// The stream the commands come from: three words.
namespace stream {
inline constexpr std::uint32_t bytes = 0x0, position = 0x4, length = 0x8;
}
using Command = std::uint32_t (*)(Interpreter&);
constexpr std::array<Command, 27> commands{
    command_0, command_1, command_2, command_3, command_4, command_5, command_6, command_7, command_8,
    command_9, command_10, command_11, command_12, command_13, command_14, command_15, command_16,
    command_17, command_18, command_19, command_20, command_21, command_22, command_23, command_24,
    command_25, command_25};
}

std::uint32_t interpret(Cpu& c, std::uint32_t table, std::uint32_t r4, std::uint32_t r5, std::uint32_t source) {
    // The caller's r21 is the command when the read fails (0x1071df2c skips the load): read it first.
    const auto caller_r21 = c.r(21);
    const Frame frame(c, 0x1071de80, 0x18b0, {.first_gpr = 13});
    CallRegisters regs(c);
    regs.hold({{15, table}, {25, c.toc(-0x7408)}, {19, c.toc(-0x7468)}, {16, source}, {18, c.toc(-0x5c24)},
               {26, c.toc(-0x79e0)}, {14, c.toc(toc::game)}, {28, c.toc(toc::success)}, {29, c.toc(toc::message)},
               {30, c.toc(toc::pool)}});
    // The two arguments the original keeps in its caller's parameter area (entry sp +0x1c, +0x22).
    c.put32(source + stream::position, 0);
    const auto position = c.u32(source + stream::position);
    c.put32(frame.at(0x18cc), r4);
    const auto length = c.u32(source + stream::length);
    c.put16(frame.at(0x18d2), r5);
    std::uint32_t command_byte = caller_r21;
    if (position + 1 > length) {  // cmplw: unsigned
        regs.apply();
        report(c, frame, {0x1071def0, 0x1594, 0x4dc, 0x8f, text::network_header});
    } else {
        command_byte = c.u8(c.u32(source + stream::bytes) + position);
        c.put32(source + stream::position, position + 1);
        c.set(4, c.u32(source + stream::bytes));
    }
    regs.hold({{21, command_byte}});
    const auto command = std::int8_t(command_byte);
    // `cmplwi r0,0x1a` on the sign-extended byte: a negative command is above 26 too.
    if (std::uint32_t(std::int32_t(command)) > 26) {
        regs.apply();
        return common_exit_result;
    }
    Interpreter state{c, frame, regs, table, source, command, c.r(4), CheckedStream(c, frame, source, regs)};
    const auto result = commands[std::size_t(command)](state);
    regs.apply();
    return result;
}

namespace {
// Field offsets of a display table (from table + table::high) that the commands set from the stream.
namespace field {
inline constexpr std::int32_t guio_count = -0x625a,      // signed halfword: the table's GUI object count
                              overflow = -0x56bf;        // byte: command 1 sets it when the table is full
}
// Frame offsets the commands use.
namespace slot {
inline constexpr std::int32_t r4_word = 0x18cc, r5_half = 0x18d2;
}

// A read into a register: a failed read leaves the register as it was.
std::uint32_t into(Interpreter& in, unsigned reg, const std::optional<std::uint32_t>& value) {
    if (value) in.regs.hold({{reg, *value}});
    return in.regs[reg];
}

// The end of commands 0-2: with `reply_wanted` set, the answer through 0x10723850 (`mode` is its r6);
// otherwise two bytes (5 and `code`) appended to the global output stream (TOC -0x5c04, same layout as the
// source's: {bytes, position, length}). The first append's failed check falls through to the second; the
// second's failed check ends the command.
std::uint32_t finish(Interpreter& in, std::uint32_t reply_back, std::uint32_t mode, std::uint8_t code,
                     const Site& first, const Site& second) {
    auto& c = in.c;
    if (c.u8(in.table + table::reply_wanted) != 0) {
        in.regs.apply();
        invoke_unit(c, 0x10723850, reply_back,
                    {in.table, c.u32(in.frame.at(slot::r4_word)), sign16(c.u16(in.frame.at(slot::r5_half))), mode});
        return common_exit_result;
    }
    const auto out_address = c.toc(-0x5c04);
    c.put32(out_address + stream::position, 0);
    in.regs.hold({{13, out_address + stream::position}, {14, out_address + stream::length}});
    CheckedStream out(c, in.frame, out_address, in.regs);
    out.write8(first, 5);  // a failed check reports and falls through to the second write
    out.write8(second, code);
    return common_exit_result;
}
}

// Command 0 (0x1071df5c): the screen's header.
std::uint32_t command_0(Interpreter& in) {
    auto& c = in.c;
    auto& source = in.source;
    in.regs.apply();
    invoke_unit(c, 0x10719560, 0x1071df68, {in.table, 1});
    c.put16(in.regs[18], c.u16(in.frame.at(slot::r5_half)));
    const auto length = into(in, 13, source.read16({0x1071df98, 0x634, 0x104, 0x90}));
    in.regs.apply();
    invoke(c, 0x10739ef0, 0x1071e008, {in.frame.at(0x1614), source.at_position(), length});
    source.advance(length);
    const auto mode = into(in, 24, source.read8({0x1071e03c, 0x1574, 0x4d4, 0x8f}));
    if (const auto v = source.read16({0x1071e0b0, 0x1554, 0x4cc, 0x90})) c.put16(in.table + table::announced_area_count, *v);
    if (const auto v = source.read16({0x1071e144, 0x1534, 0x4c4, 0x90})) c.put16(in.table + table::announced_guio_count, *v);
    if (const auto v = source.read8({0x1071e1d8, 0x1514, 0x4bc, 0x8f})) c.put8(in.table + table::width_flag, *v);
    if (const auto v = source.read16({0x1071e25c, 0x14f4, 0x4b4, 0x90})) c.put16(in.table + table::height_flag, *v);
    c.put8(in.table + table::mode, mode);
    in.regs.apply();
    invoke_unit(c, 0x10723650, 0x1071e2dc, {in.frame.at(0x1614), 1});
    invoke(c, 0x1073ede0, 0x1071e2e8, {in.regs[14], in.frame.at(0x1614)});
    return finish(in, 0x1071e30c, 2, 0, {0x1071e34c, 0x14d4, 0x4ac, 0x85}, {0x1071e3d0, 0x14b4, 0x4a4, 0x85});
}

// Command 1 (0x1071ed10): GUI objects, each a flag word and the fields its bits select.
std::uint32_t command_1(Interpreter& in) {
    auto& c = in.c;
    auto& regs = in.regs;
    const auto& frame = in.frame;
    const auto table_address = in.table;
    const auto high = table_address + table::high;
    const auto guio_base = table_address + table::guios;
    regs.hold({{31, high}});
    if (c.i8(in.table + table::screen_set) != 0) return common_exit_result;
    c.put16(frame.at(0x182a), 0);
    c.put16(regs[18], c.u16(frame.at(slot::r5_half)));
    c.put32(frame.at(0x1834), 0);
    auto& source = in.source;
    struct Item { std::uint32_t mask; unsigned size; std::int32_t slot; Site site; };
    // The bits of the flag word in the original's order (the string, bit 0x4000, and the record copy,
    // bit 0x80000, are written out below). Stored as the original does, into the frame slot.
    static constexpr Item first_items[] = {
        {0x1, 4, 0xb0, {0x1071ef28, 0x12d4, 0x42c, 0x92}},    {0x2, 4, 0xac, {0x1071efcc, 0x12b4, 0x424, 0x92}},
        {0x4, 4, 0x1818, {0x1071f070, 0x1294, 0x41c, 0x93}},  {0x8, 2, 0x1828, {0x1071f118, 0x1274, 0x414, 0x90}},
        {0x10, 2, 0x1826, {0x1071f1b8, 0x1254, 0x40c, 0x90}}, {0x20, 2, 0x1824, {0x1071f258, 0x1234, 0x404, 0x90}},
        {0x40, 2, 0x1822, {0x1071f2f8, 0x1214, 0x3fc, 0x90}}, {0x80, 1, 0x181d, {0x1071f398, 0x11f4, 0x3f4, 0x8f}},
        {0x100, 2, 0x1820, {0x1071f42c, 0x11d4, 0x3ec, 0x90}}, {0x200, 2, 0x78, {0x1071f4d0, 0x11b4, 0x3e4, 0x95}},
        {0x400, 2, 0x76, {0x1071f57c, 0x1194, 0x3dc, 0x95}},  {0x800, 4, 0xa8, {0x1071f624, 0x1174, 0x3d4, 0x92}},
        {0x1000, 1, 0x181c, {0x1071f6c8, 0x1154, 0x3cc, 0x8f}}, {0x2000, 2, 0x74, {0x1071f75c, 0x1134, 0x3c4, 0x95}},
    };
    static constexpr Item second_items[] = {
        {0x8000, 2, 0x181e, {0x1071f8e4, 0x1114, 0x3bc, 0x90}}, {0x10000, 4, 0xa4, {0x1071f984, 0x10f4, 0x3b4, 0x92}},
        {0x20000, 4, 0xa0, {0x1071fa28, 0x10d4, 0x3ac, 0x93}},  {0x40000, 2, 0x72, {0x1071facc, 0x10b4, 0x3a4, 0x90}},
    };
    const auto store = [&](const Item& item, std::uint32_t value) {
        if (item.size == 4) c.put32(frame.at(item.slot), value);
        else if (item.size == 2) c.put16(frame.at(item.slot), value);
        else c.put8(frame.at(item.slot), value);
    };
    // A bit's field: read when the bit is set, zero when it is clear and the 0x100000 block did not
    // already fill it.
    const auto field_of = [&](const Item& item, std::uint32_t flags, std::uint32_t block) {
        if (flags & item.mask) {
            const auto value = item.size == 4 ? source.read32(item.site) : item.size == 2 ? source.read16(item.site) : source.read8(item.site);
            if (value) store(item, *value);
        } else if (block == 0) {
            store(item, 0);
        }
    };
    while (c.i16(frame.at(0x182a)) < c.i16(table_address + table::announced_guio_count)) {
        into(in, 17, source.read32({0x1071ed68, 0x12f4, 0x434, 0x92}));
        const auto flags = regs[17];
        const auto block = flags & 0x100000;
        regs.hold({{13, block}});
        if (block) {
            // 0x1071edd0: fourteen out-parameters in the frame, filled by 0x10725dc0.
            const auto p = [&](std::int32_t offset) { return frame.at(offset); };
            c.put32(frame.at(0x38), p(0xac));
            c.put32(frame.at(0x3c), p(0x78));
            c.put32(frame.at(0x40), p(0x76));
            c.put32(frame.at(0x44), p(0xa8));
            c.put32(frame.at(0x48), p(0x70));
            c.put32(frame.at(0x4c), p(0x74));
            c.put32(frame.at(0x50), p(0x7c));
            c.put32(frame.at(0x54), p(0x84));
            c.put32(frame.at(0x58), p(0xa4));
            c.put32(frame.at(0x5c), p(0xa0));
            c.put32(frame.at(0x60), p(0x72));
            c.put32(frame.at(0x64), p(0x80));
            regs.hold({{18, p(0x7c)}, {14, p(0x84)}});
            regs.apply();
            invoke(c, 0x10725dc0, 0x1071ee60,
                   {c.u32(c.toc(-0x5bfc)) + c.u32(frame.at(0x1834)), p(0xb0), p(0x9c), p(0x98), p(0x94), p(0x90), p(0x8c), p(0x88)});
            c.put16(frame.at(0x1828), c.u32(frame.at(0x9c)));
            c.put16(frame.at(0x1826), c.u32(frame.at(0x98)));
            c.put16(frame.at(0x1824), c.u32(frame.at(0x94)));
            c.put16(frame.at(0x1822), c.u32(frame.at(0x90)));
            const auto text_source = c.u32(frame.at(0x7c));
            c.put8(frame.at(0x181d), c.u32(frame.at(0x8c)));
            c.put16(frame.at(0x1820), c.u32(frame.at(0x88)));
            c.put32(frame.at(0x1818), c.u32(c.u32(c.toc(-0x5bfc)) + c.u32(frame.at(0x1834)) + 0x58));
            c.put8(frame.at(0x181c), c.u16(frame.at(0x70)));
            c.put16(frame.at(0x181e), c.u32(frame.at(0x84)));
            if (text_source != 0) invoke(c, 0x1077dbf0, 0x1071eeec, {frame.at(0x1714), text_source});
            else c.put8(frame.at(0x1714), 0);
        }
        for (const auto& item : first_items) {
            if (item.mask == 0x200) c.put32(frame.at(0x1830), flags & 0x200);
            if (item.mask == 0x400) c.put32(frame.at(0x182c), flags & 0x400);
            if (item.mask == 0x2000) regs.hold({{18, flags & 0x2000}});
            field_of(item, flags, block);
        }
        if (flags & 0x4000) {  // a string: halfword length, then that many bytes
            if (const auto v = source.read16({0x1071f804, 0x5d4, 0xec, 0x90})) c.put16(frame.at(0x1854), *v);
            const auto length = sign16(c.u16(frame.at(0x1854)));
            regs.apply();
            invoke(c, 0x10739ef0, 0x1071f878, {frame.at(0x1714), source.at_position(), length});
            source.advance(sign16(c.u16(frame.at(0x1854))));
            if (c.u32(frame.at(0xb0)) & 0x400) {
                regs.apply();
                invoke_unit(c, 0x10723650, 0x1071f8a0, {frame.at(0x1714), 1});
            }
        } else if (block == 0) {
            c.put8(frame.at(0x1714), 0);
        }
        for (const auto& item : second_items) field_of(item, flags, block);
        regs.hold({{19, flags & 0x80000}});
        if (flags & 0x80000) {  // copy another GUI object's values
            if (const auto v = source.read16({0x1071fb78, 0x1094, 0x39c, 0x90})) c.put16(frame.at(0x1814), *v);
            const auto record = table_address + std::uint32_t(std::int32_t(c.i16(frame.at(0x1814))) * 0x17c) + 0xc0000;
            c.put32(frame.at(0xb0), c.u32(record - 0x5790));
            c.put32(frame.at(0xac), c.u32(record - 0x5764));
            c.put16(frame.at(0x78), c.u16(record - 0x5732));
            c.put16(frame.at(0x76), c.u16(record - 0x5730));
            c.put32(frame.at(0xa8), c.u32(record - 0x5760));
            c.put8(frame.at(0x181c), c.u8(record - 0x572d));
            c.put16(frame.at(0x74), c.u16(record - 0x572c));
        }
        // The values for the new GUI object: registers and the call's stack arguments.
        regs.hold({{25, sign8(c.u8(frame.at(0x181c)))}, {23, sign16(c.u16(frame.at(0x181e)))},
                   {20, sign16(c.u16(frame.at(0x72)))}, {21, c.u32(frame.at(0xa0))}, {22, c.u32(frame.at(0xa4))},
                   {24, c.u16(frame.at(0x74))}, {26, c.u32(frame.at(0xa8))}, {27, c.u16(frame.at(0x76))},
                   {14, c.u16(frame.at(0x78))}, {13, c.u32(frame.at(0xac))}});
        c.put32(frame.at(0x1838), sign16(c.u16(frame.at(0x1820))));
        c.put32(frame.at(0x183c), sign8(c.u8(frame.at(0x181d))));
        c.put32(frame.at(0x1840), sign16(c.u16(frame.at(0x1822))));
        c.put32(frame.at(0x1844), sign16(c.u16(frame.at(0x1824))));
        c.put32(frame.at(0x1848), sign16(c.u16(frame.at(0x1826))));
        c.put32(frame.at(0x184c), sign16(c.u16(frame.at(0x1828))));
        c.put32(frame.at(0x1850), c.u32(frame.at(0xb0)));
        const auto count = c.i16(high + field::guio_count);
        if (count < 0x4af) {
            c.put16(high + field::guio_count, std::uint32_t(count + 1));
            regs.apply();
            invoke(c, 0x10778300, 0x1071fcc8, {guio_at(table_address, count), 0, 0x17c});
            const auto index = std::uint32_t(c.i16(high + field::guio_count)) - 1;
            const auto guio = guio_at(table_address, std::int32_t(index));
            c.put32(frame.at(0x38), c.u32(frame.at(0x1840)));
            c.put32(frame.at(0x3c), c.u32(frame.at(0x183c)));
            c.put32(frame.at(0x40), c.u32(frame.at(0x1838)));
            c.put32(frame.at(0x44), regs[13]);
            c.put32(frame.at(0x48), regs[14]);
            c.put32(frame.at(0x4c), regs[27]);
            c.put32(frame.at(0x50), regs[26]);
            c.put32(frame.at(0x54), regs[25]);
            c.put32(frame.at(0x58), regs[24]);
            c.put32(frame.at(0x5c), frame.at(0x1714));
            c.put32(frame.at(0x60), regs[23]);
            c.put32(frame.at(0x64), regs[22]);
            c.put32(frame.at(0x68), regs[21]);
            c.put32(frame.at(0x6c), regs[20]);
            regs.apply();
            invoke(c, 0x10725a00, 0x1071fd50,
                   {guio, table_address + 4, guio_base, sign16(index), c.u32(frame.at(0x1850)), c.u32(frame.at(0x184c)),
                    c.u32(frame.at(0x1848)), c.u32(frame.at(0x1844))});
            if (regs[20] != 0xffffffffu) {
                const auto last = std::uint32_t(c.i16(high + field::guio_count)) - 1;
                regs.apply();
                invoke(c, 0x107144c0, 0x1071fd78,
                       {table_address + 4 + regs[20] * table::area_size, sign16(last)});
            } else {
                const auto last = sign16(std::uint32_t(c.i16(high + field::guio_count)) - 1);
                if (last != 0xffffffffu) {
                    const auto limit = c.i16(table_address + table::guio_limit);
                    if (limit >= 0x4b0) {
                        regs.apply();
                        report(c, frame, {0x1071fdb4, 0x5b4, 0xe4, 0x2e5});
                    } else {
                        c.put16(table_address + table::guio_limit, std::uint32_t(limit + 1));
                        c.put16(table_address + (std::uint32_t(std::int32_t(limit)) << 1) + table::index_list, last);
                    }
                }
            }
            regs.hold({{13, sign16(std::uint32_t(c.i16(high + field::guio_count)) - 1)}});
        } else {
            regs.hold({{13, 0xffffffffu}});
            c.put8(high + field::overflow, 1);
        }
        const auto current_guio = [&] { return guio_at(table_address, half(regs[13])); };
        if ((regs[19] == 0) && c.u32(frame.at(0x1830)) != 0) {
            regs.apply();
            invoke(c, 0x10726ae0, 0x1071fe54, {current_guio(), c.u8(table_address + table::mode), 1});
        }
        if ((regs[19] == 0) && c.u32(frame.at(0x182c)) != 0) {
            regs.apply();
            invoke(c, 0x10726ae0, 0x1071fe8c, {current_guio(), c.u8(table_address + table::mode), 2});
        }
        if ((regs[19] == 0) && regs[18] != 0) {
            regs.apply();
            invoke(c, 0x10726ae0, 0x1071fec0, {current_guio(), c.u8(table_address + table::mode), 4});
        }
        const auto record = table_address + std::uint32_t(std::int32_t(std::int16_t(regs[13])) * 0x17c) + 0xc0000;
        c.put32(record - 0x5748, 0);
        c.put32(record - 0x5744, c.u32(frame.at(0x1818)));
        c.put32(frame.at(0x1834), c.u32(frame.at(0x1834)) + 0x17c);
        c.put16(frame.at(0x182a), std::uint32_t(c.i16(frame.at(0x182a)) + 1));
    }
    const auto copy_to = c.u32(c.toc(-0x5bfc));
    const auto count = c.i16(high + field::guio_count);
    c.put16(copy_to + 0x70000 - 0xac0, std::uint32_t(count));
    regs.apply();
    invoke(c, 0x10739ef0, 0x1071ff38, {copy_to, guio_base, std::uint32_t(std::int32_t(count)) * 0x17c});
    return finish(in, 0x1071ff5c, 3, 1, {0x1071ff9c, 0x1074, 0x394, 0x85}, {0x10720020, 0x1054, 0x38c, 0x85});
}

// Command 2 (0x1071e430): areas, each with two strings, read until the count command 0 stored.
std::uint32_t command_2(Interpreter& in) {
    auto& c = in.c;
    auto& regs = in.regs;
    const auto& frame = in.frame;
    const auto table_address = in.table;
    const auto high = table_address + table::high;
    regs.hold({{19, high}});
    if (c.i8(table_address + table::screen_set) != 0) return common_exit_result;
    c.put16(regs[18], c.u16(frame.at(slot::r5_half)));
    regs.hold({{14, frame.at(0x15d4)}, {21, 0}, {18, 0}});
    auto& source = in.source;
    // A string: with the flag byte set, a halfword length then the bytes; otherwise a fill (0x10778300).
    const auto text = [&](std::uint32_t flag, const Site& length_site, std::uint32_t length_register, std::uint32_t buffer,
                          std::uint32_t copy_back, std::uint32_t fill_back, unsigned fill_register) {
        if (sign8(flag) != 0) {
            const auto length = into(in, length_register, source.read16(length_site));
            regs.apply();
            invoke(c, 0x10739ef0, copy_back, {buffer, source.at_position(), length});
            source.advance(length);
        } else {
            regs.apply();
            invoke(c, 0x10778300, fill_back, {buffer, 1, regs[fill_register]});
        }
    };
    while (std::int32_t(sign16(regs[21])) < std::int32_t(c.i16(table_address + table::announced_area_count))) {
        if (const auto v = source.read16({0x1071e484, 0x1494, 0x49c, 0x90})) c.put16(frame.at(0x1828), *v);
        if (const auto v = source.read16({0x1071e508, 0x1474, 0x494, 0x90})) c.put16(frame.at(0x1826), *v);
        if (const auto v = source.read16({0x1071e58c, 0x1454, 0x48c, 0x90})) c.put16(frame.at(0x1824), *v);
        if (const auto v = source.read16({0x1071e610, 0x1434, 0x484, 0x90})) c.put16(frame.at(0x1822), *v);
        into(in, 22, source.read8({0x1071e694, 0x1414, 0x47c, 0x94}));
        into(in, 24, source.read8({0x1071e708, 0x13f4, 0x474, 0x8f}));
        text(regs[24], {0x1071e784, 0x614, 0xfc, 0x90}, 17, frame.at(0x15f4), 0x1071e7f4, 0x1071e814, 22);
        into(in, 23, source.read8({0x1071e83c, 0x13d4, 0x46c, 0x94}));
        into(in, 24, source.read8({0x1071e8b0, 0x13b4, 0x464, 0x8f}));
        text(regs[24], {0x1071e92c, 0x5f4, 0xf4, 0x90}, 20, frame.at(0x15d4), 0x1071e99c, 0x1071e9bc, 23);
        if (const auto v = source.read32({0x1071e9e4, 0x1394, 0x45c, 0x92})) c.put32(frame.at(0xac), *v);
        if (const auto v = source.read16({0x1071ea6c, 0x1374, 0x454, 0x95})) c.put16(frame.at(0x78), *v);
        into(in, 13, source.read16({0x1071eaf8, 0x1354, 0x44c, 0x90}));
        regs.apply();
        invoke(c, 0x10714440, 0x1071eb5c, {table_address + regs[18] + 4});
        c.put32(frame.at(0x38), regs[14]);
        c.put32(frame.at(0x3c), c.u32(frame.at(0xac)));
        c.put32(frame.at(0x40), c.u16(frame.at(0x78)));
        c.put32(frame.at(0x44), regs[13]);
        regs.apply();
        const auto area = invoke_unit(c, 0x107198c0, 0x1071eb98,
                                      {table_address, sign16(c.u16(frame.at(0x1828))), sign16(c.u16(frame.at(0x1826))),
                                       sign16(c.u16(frame.at(0x1824))), sign16(c.u16(frame.at(0x1822))), regs[22],
                                       frame.at(0x15f4), regs[23]});
        c.put16(frame.at(0x72), area);
        regs.apply();
        invoke(c, 0x10715650, 0x1071ebb4,
               {table_address + std::uint32_t(c.i16(frame.at(0x72))) * table::area_size + 4, c.u8(table_address + table::mode)});
        regs.hold({{18, regs[18] + table::area_size}, {21, regs[21] + 1}});
    }
    return finish(in, 0x1071ebec, 1, 2, {0x1071ec2c, 0x1334, 0x444, 0x85}, {0x1071ecb0, 0x1314, 0x43c, 0x85});
}

// Command 3 (0x10720080): once, ask 0x10718fc0 for the screen with all four arguments -1.
std::uint32_t command_3(Interpreter& in) {
    auto& c = in.c;
    if (c.i8(in.table + table::screen_set) != 0) return common_exit_result;
    c.put8(in.table + table::unsent, 1);
    in.regs.apply();
    invoke_unit(c, 0x10718fc0, 0x107200b0, {in.table, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu});
    return common_exit_result;
}

// Command 4 (0x107200b4): answers -2 (leaves through 0x1072363c, not the common exit).
std::uint32_t command_4(Interpreter&) { return 0xfffffffeu; }

// Command 5 (0x107200bc): one byte, passed on to 0x1071c2d0.
std::uint32_t command_5(Interpreter& in) {
    auto& c = in.c;
    auto& source = in.source;
    const auto value = into(in, 21, source.read8({0x107200e4, 0x1034, 0x384, 0x8f}));
    in.regs.apply();
    invoke_unit(c, 0x1071c2d0, 0x10720144,
                {in.table, c.u32(in.frame.at(slot::r4_word)), sign16(c.u16(in.frame.at(slot::r5_half))), value});
    return common_exit_result;
}
}

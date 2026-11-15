// DISPLAY.CPP 0x10723a30 and 0x10723cb0: the network screen transport. Both append to the global output
// stream (TOC -0x5c04: {bytes, position, length} words, as the interpreter's source) and hand it to
// NETWORK 0x1073a4c0, with the "Network Error" dialog when that fails.
//   0x10723a30  send_ready       with the table's reply flag (reply_wanted) clear: set the table's screen_set byte,
//                                 append the two bytes 5 and 0; with it set: send what the stream holds.
//   0x10723cb0  send_gui_object  append command 6 (a GUI object: its 0x10725dc0 fields, its string, and, when
//                                 its link is not -1, the linked object's fields) and send the stream.
// The original is called from SCRMAN only, in network games; no capture reaches it.
#include "display_network.hpp"
#include "display_detail.hpp"
#include <bit>

namespace cm::aot::display {
namespace {
using u32 = std::uint32_t;
namespace stream {
inline constexpr u32 position = 0x4, length = 0x8;
}
namespace gui {
inline constexpr u32 position = 0x60;  // the 8 bytes copied to the frame when the object is linked
}

// The report of a failed append check, then the "Network Error" dialog 0x10769b60 shows after a failed send
// (ODD in the sites file: the title and text come from LANGLIB, the file name is DISPLAY.CPP itself).
void send_failed_dialog(Cpu& c, const Frame& frame, CallRegisters& regs, const Site& site, u32 text_string, u32 title_string) {
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message);
    regs.apply();
    invoke(c, 0x10739890, site.at + 4, {pool, 0, 0, frame.at(site.name), frame.at(site.extension)});
    const auto id = invoke(c, 0x10768810, site.at + 8);
    invoke(c, 0x1077b720, site.at + 0x24, {message, pool + text::report_format, id, frame.at(site.name), frame.at(site.extension), site.line});
    invoke(c, 0x10727890, site.at + 0x30, {title_string, pool + 0x95});
    c.set(3, text_string); c.set(4, pool + 0xa3); c.set(5, message);
    c.call(0x10727890, site.at + 0x40);
    invoke(c, 0x10769b60, site.at + 0x50, {title_string, text_string, 0});
}

}

// 0x10723a30.
void send_ready(Cpu& c, u32 table, u32 destination) {
    const Frame frame(c, 0x10723a30, 0x100, {.first_gpr = 22});
    CallRegisters regs(c);
    const auto out = c.toc(-0x5c04), position_cell = out + stream::position;
    const u32 text_string = c.toc(-0x79e0), title_string = c.toc(-0x79dc);  // the strings the error dialog shows
    regs.hold({{22, destination}, {26, text_string}, {27, title_string}, {28, out}, {29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    const auto dialog = [&](const Site& site) { send_failed_dialog(c, frame, regs, site, text_string, title_string); };
    if (c.u8(table + table::reply_wanted) == 0) {
        c.put8(table + table::screen_set, 1);
        regs.hold({{24, position_cell}, {23, out + stream::length}});
        c.put32(position_cell, 0);
        const auto count = sign16(c.u16(c.toc(-0x5c24)));  // lha r25,0(r6): TOC -0x5c24 is the address of a halfword
        regs.hold({{25, count}});
        CheckedStream sink(c, frame, out, regs);
        sink.write({0x10723ab4, 0x78, 0x40, 0x85}, 1, 5);
        sink.write({0x10723b34, 0x58, 0x38, 0x85}, 1, 0);
        regs.apply();
        const auto sent = invoke(c, 0x1073a4c0, 0x10723ba0, {destination, c.u32(out), c.u32(position_cell), count});
        if (std::int8_t(sent) == 0) dialog({0x10723bbc, 0x98, 0x48, 0xc4a});
    } else {
        regs.apply();
        const auto count = sign16(c.u16(c.toc(-0x5c24)));
        const auto sent = invoke(c, 0x1073a4c0, 0x10723c24, {destination, c.u32(out), c.u32(position_cell), count});
        if (std::int8_t(sent) == 0) dialog({0x10723c40, 0xb8, 0x50, 0xc69});
    }
}

namespace {
// 0x10725dc0's twelve out-parameters, set up as the original does (frame words 0x38-0x64 are the stack
// arguments, the six others are registers r4-r10); the fields come back through these frame slots.
namespace slot {
inline constexpr std::int32_t w_a8 = 0xa8, w_94 = 0x94, w_90 = 0x90, w_8c = 0x8c, w_88 = 0x88, w_84 = 0x84, w_80 = 0x80,
                              w_a4 = 0xa4, w_70 = 0x70, w_6e = 0x6e, w_a0 = 0xa0, w_68 = 0x68, w_6c = 0x6c, w_74 = 0x74,
                              w_7c = 0x7c, w_98 = 0x98, w_9c = 0x9c, w_6a = 0x6a, w_78 = 0x78, text = 0x80c;
}
void describe(Cpu& c, const Frame& frame, CallRegisters& regs, u32 object, u32 back) {
    const auto p = [&](std::int32_t offset) { return frame.at(offset); };
    const std::int32_t stacked[] = {slot::w_a4, slot::w_70, slot::w_6e, slot::w_a0, slot::w_68, slot::w_6c,
                                    slot::w_74, slot::w_7c, slot::w_98, slot::w_9c, slot::w_6a, slot::w_78};
    for (unsigned n = 0; n < 12; ++n) c.put32(frame.at(0x38 + 4 * std::int32_t(n)), p(stacked[n]));
    regs.apply();
    invoke(c, 0x10725dc0, back, {object, p(slot::w_a8), p(slot::w_94), p(slot::w_90), p(slot::w_8c), p(slot::w_88), p(slot::w_84), p(slot::w_80)});
}
}

// 0x10723cb0.
bool send_gui_object(Cpu& c, u32 table, u32 destination, u32 index_word) {
    const Frame frame(c, 0x10723cb0, 0x960, {.first_gpr = 13});
    CallRegisters regs(c);
    const auto pool = c.toc(toc::pool);
    const auto out = c.toc(-0x5c04);
    regs.hold({{15, table}, {16, index_word}, {26, out + stream::length}, {27, out + stream::position}, {28, out},
               {29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {31, pool}});
    c.put32(out + stream::position, 0);
    c.put32(frame.at(0x97c), destination);  // entry sp +0x1c: the caller's parameter area
    CheckedStream sink(c, frame, out, regs);
    // The 47 sites of the sites file in order; [46] is the final dialog.
    static constexpr Site sites[] = {
        {0x10723d14, 0x7cc, 0x214, 0x85}, {0x10723d94, 0x7ac, 0x20c, 0x86}, {0x10723f20, 0x78c, 0x204, 0x88}, {0x10723f9c, 0x76c, 0x1fc, 0x86},
        {0x10724020, 0x74c, 0x1f4, 0x86}, {0x107240a4, 0x72c, 0x1ec, 0x86}, {0x10724128, 0x70c, 0x1e4, 0x86}, {0x107241ac, 0x6ec, 0x1dc, 0x85},
        {0x10724228, 0x6cc, 0x1d4, 0x86}, {0x107242b0, 0x6ac, 0x1cc, 0x88}, {0x10724330, 0x68c, 0x1c4, 0x8b}, {0x107243b0, 0x66c, 0x1bc, 0x8b},
        {0x10724430, 0x64c, 0x1b4, 0x88}, {0x107244ac, 0x62c, 0x1ac, 0x85}, {0x1072452c, 0x60c, 0x1a4, 0x8b}, {0x107245b8, 0x24c, 0xb4, 0x86},
        {0x10724660, 0x5ec, 0x19c, 0x86}, {0x107246e8, 0x5cc, 0x194, 0x88}, {0x10724768, 0x5ac, 0x18c, 0x89}, {0x107247e8, 0x58c, 0x184, 0x86},
        {0x1072486c, 0x56c, 0x17c, 0x89}, {0x107248f4, 0x54c, 0x174, 0x86}, {0x10724994, 0x52c, 0x16c, 0x86}, {0x10724a18, 0x50c, 0x164, 0x88},
        {0x10724a98, 0x4ec, 0x15c, 0x88}, {0x10724b18, 0x4cc, 0x154, 0x88}, {0x10724b98, 0x4ac, 0x14c, 0x88}, {0x10724d20, 0x48c, 0x144, 0x88},
        {0x10724d9c, 0x46c, 0x13c, 0x86}, {0x10724e20, 0x44c, 0x134, 0x86}, {0x10724ea4, 0x42c, 0x12c, 0x86}, {0x10724f28, 0x40c, 0x124, 0x86},
        {0x10724fac, 0x3ec, 0x11c, 0x85}, {0x10725028, 0x3cc, 0x114, 0x86}, {0x107250b0, 0x3ac, 0x10c, 0x88}, {0x10725130, 0x38c, 0x104, 0x8b},
        {0x107251b0, 0x36c, 0xfc, 0x8b}, {0x10725230, 0x34c, 0xf4, 0x88}, {0x107252ac, 0x32c, 0xec, 0x85}, {0x1072532c, 0x30c, 0xe4, 0x8b},
        {0x107253b8, 0x22c, 0xac, 0x86}, {0x10725460, 0x2ec, 0xdc, 0x86}, {0x107254e8, 0x2cc, 0xd4, 0x88}, {0x10725568, 0x2ac, 0xcc, 0x89},
        {0x107255e8, 0x28c, 0xc4, 0x86}, {0x1072566c, 0x26c, 0xbc, 0x89}, {0x107256f4, 0x7ec, 0x21c, 0xcf2}};
    // The width of each site's write (the listing's stb/sth/stw); [46] is the dialog, no write.
    static constexpr u32 widths[] = {1, 2, 4, 2, 2, 2, 2, 1, 2, 4, 2, 2, 4, 1, 2, 2, 2, 4, 4, 2, 4, 2, 2, 4, 4, 4, 4, 4, 2, 2, 2, 2, 1, 2, 4, 2, 2, 4, 1, 2, 2, 2, 4, 4, 2, 4, 0};
    const auto word = [&](std::int32_t offset) { return c.u32(frame.at(offset)); };
    const auto half = [&](std::int32_t offset) { return u32(c.u16(frame.at(offset))); };
    const auto half_signed = [&](std::int32_t offset) { return sign16(c.u16(frame.at(offset))); };
    // The string: copied to frame +0x80c from the pointer 0x10725dc0 returned in +0x74 (or empty).
    const auto copy_string = [&](u32 back) {
        if (word(slot::w_74) != 0) {
            regs.apply();
            invoke(c, 0x1077dbf0, back, {frame.at(slot::text), word(slot::w_74)});
        } else {
            c.put8(frame.at(slot::text), 0);
        }
    };
    // The string with its length (+1 as a signed halfword), then the 0x10739ef0 copy into the stream.
    const auto string_out = [&](u32 length_back, u32 copy_back, const Site& length_site) {
        regs.apply();
        const auto length = invoke(c, 0x1077dbd0, length_back, {frame.at(slot::text)});
        const auto count = sign16(length + 1);
        regs.hold({{13, count}});
        sink.write16(length_site, count);  // the string length: a halfword (sites 15 and 40, line 0x86)
        regs.apply();
        invoke(c, 0x10739ef0, copy_back, {sink.at_position(), frame.at(slot::text), count});
        sink.advance(count);
        return count;
    };

    sink.write(sites[0], widths[0], 6);
    sink.write(sites[1], widths[1], index_word);
    const auto index = sign16(index_word);
    const auto offset = index * 0x17c;  // r25 (wrapping 32-bit product of the sign-extended halfword)
    const auto object = table + offset + 0xba864;
    regs.hold({{25, offset}, {24, object}});
    describe(c, frame, regs, object, 0x10723e88);
    regs.hold({{23, sign16(word(slot::w_94))}, {22, sign16(word(slot::w_90))}, {21, sign16(word(slot::w_8c))},
               {20, sign16(word(slot::w_88))}, {13, sign8(word(slot::w_84))}, {19, sign16(word(slot::w_80))},
               {18, sign16(word(slot::w_7c))}, {14, sign8(half(slot::w_68))},
               {17, c.u32(table + offset + 0xba8bc)}});
    copy_string(0x10723ee8);
    sink.write(sites[2], widths[2], word(slot::w_a8));
    sink.write(sites[3], widths[3], regs[23]);
    sink.write(sites[4], widths[4], regs[22]);
    sink.write(sites[5], widths[5], regs[21]);
    sink.write(sites[6], widths[6], regs[20]);
    sink.write(sites[7], widths[7], regs[13]);
    sink.write(sites[8], widths[8], regs[19]);
    sink.write(sites[9], widths[9], word(slot::w_a4));
    sink.write(sites[10], widths[10], half(slot::w_70));
    sink.write(sites[11], widths[11], half(slot::w_6e));
    sink.write(sites[12], widths[12], word(slot::w_a0));
    sink.write(sites[13], widths[13], regs[14]);
    sink.write(sites[14], widths[14], half(slot::w_6c));
    string_out(0x10724588, 0x1072462c, sites[15]);
    sink.write(sites[16], widths[16], regs[18]);
    sink.write(sites[17], widths[17], word(slot::w_98));
    sink.write(sites[18], widths[18], word(slot::w_9c));
    sink.write(sites[19], widths[19], half_signed(slot::w_6a));
    sink.write(sites[20], widths[20], regs[17]);
    const auto link = sign16(c.u16(table + offset + 0xba8da));  // lha r13: the linked object's index
    regs.hold({{13, link}});
    sink.write(sites[21], widths[21], link);
    if (link != 0xffffffffu) {
        regs.apply();
        invoke(c, 0x1077fc28, 0x10724968, {object + gui::position, frame.at(0x224), 8});
        c.set(2, c.u32(frame.at(0x14)));  // the import glue leaves r2 in the frame; the original reloads it
        sink.write(sites[22], widths[22], std::byteswap(std::uint16_t(0x1000)));  // 0x10724994 stores the constant 0x1000 unswapped
        sink.write(sites[23], widths[23], half_signed(0x226));
        sink.write(sites[24], widths[24], half_signed(0x224));
        sink.write(sites[25], widths[25], half_signed(0x22a));
        sink.write(sites[26], widths[26], half_signed(0x228));
        if (link != sign16(index_word)) {
            const auto other_offset = link * 0x17c;
            regs.hold({{13, other_offset}, {14, frame.at(0x98)}});
            describe(c, frame, regs, table + other_offset + 0xba864, 0x10724c88);
            regs.hold({{13, sign16(word(slot::w_94))}, {14, sign16(word(slot::w_90))}, {16, sign16(word(slot::w_8c))},
                       {17, sign16(word(slot::w_88))}, {18, sign8(word(slot::w_84))}, {19, sign16(word(slot::w_80))},
                       {20, sign8(half(slot::w_68))}, {21, sign16(word(slot::w_7c))},
                       {22, c.u32(table + other_offset + 0xba8bc)}});
            copy_string(0x10724ce8);
            sink.write(sites[27], widths[27], word(slot::w_a8));
            sink.write(sites[28], widths[28], regs[13]);
            sink.write(sites[29], widths[29], regs[14]);
            sink.write(sites[30], widths[30], regs[16]);
            sink.write(sites[31], widths[31], regs[17]);
            sink.write(sites[32], widths[32], regs[18]);
            sink.write(sites[33], widths[33], regs[19]);
            sink.write(sites[34], widths[34], word(slot::w_a4));
            sink.write(sites[35], widths[35], half(slot::w_70));
            sink.write(sites[36], widths[36], half(slot::w_6e));
            sink.write(sites[37], widths[37], word(slot::w_a0));
            sink.write(sites[38], widths[38], regs[20]);
            sink.write(sites[39], widths[39], half(slot::w_6c));
            string_out(0x10725388, 0x1072542c, sites[40]);
            sink.write(sites[41], widths[41], regs[21]);
            sink.write(sites[42], widths[42], word(slot::w_98));
            sink.write(sites[43], widths[43], word(slot::w_9c));
            sink.write(sites[44], widths[44], half_signed(slot::w_6a));
            sink.write(sites[45], widths[45], regs[22]);
        }
    }
    regs.apply();
    const auto sent = invoke(c, 0x1073a4c0, 0x107256d8,
                             {c.u32(frame.at(0x97c)), c.u32(out), c.u32(out + stream::position), sign16(c.u16(c.toc(-0x5c24)))});
    if (std::int8_t(sent) != 0) {
        c.put8(table + table::unsent, 0);
        return true;
    }
    send_failed_dialog(c, frame, regs, sites[46], c.toc(-0x79e0), c.toc(-0x79dc));
    return false;
}
}

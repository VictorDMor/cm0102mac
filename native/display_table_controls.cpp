// DISPLAY.CPP block "table": the two "add a control" routines (0x10719740, 0x107198c0).
#include "display_table.hpp"
#include <bit>
#include <initializer_list>
#include <optional>

namespace cm::aot::display {
namespace {
using u32 = std::uint32_t;

// A site of network.h's checked write.
constexpr Site net(u32 at, std::int32_t name, std::int32_t extension, u32 line) { return {at, name, extension, line, text::network_header}; }
// The routine's own outgoing parameter area: the words the callee reads at sp +0x38...
void store_parameters(Cpu& c, const Frame& frame, std::initializer_list<u32> words) {
    u32 offset = 0x38;
    for (const auto word : words) { c.put32(frame.at(std::int32_t(offset)), word); offset += 4; }
}
}

// 0x10719740. Arguments r3-r10 plus the caller's parameter area; the original keeps every one of them in
// r13-r31 across its calls, so each is held there for the callees' prologues.
std::uint32_t add_control(Cpu& c, u32 table, u32 r4, u32 r5, u32 r6, u32 r7, u32 r8, u32 r9, u32 r10) {
    const Frame frame(c, 0x10719740, 0xc0, {.first_gpr = 13});
    CallRegisters regs(c);
    const auto parameters = frame.caller();
    const u32 p38 = c.u32(parameters + 0x38), p3e = c.u16(parameters + 0x3e), p42 = c.u16(parameters + 0x42),
              p44 = c.u32(parameters + 0x44), p4a = u32(c.i16(parameters + 0x4a)), p4e = c.u16(parameters + 0x4e),
              p50 = c.u32(parameters + 0x50), p54 = c.u32(parameters + 0x54), p58 = c.u32(parameters + 0x58),
              p5c = c.u32(parameters + 0x5c), area = u32(c.i16(parameters + 0x62));  // r13 .. r14
    const auto count = c.i16(field(table, -0x625a));
    if (count >= 0x4af) {
        c.put8(field(table, -0x56bf), 1);
        c.set(3, 0xffffffffu);
        return 0xffffffffu;
    }
    regs.hold({{15, table}, {16, r4}, {17, r5}, {18, r6}, {19, r7}, {20, r8}, {21, r9}, {22, r10}, {23, p38}, {24, p3e},
               {25, p42}, {26, p44}, {27, p4a}, {28, p4e}, {29, p50}, {30, p54}, {31, p58}, {13, p5c}, {14, area}});
    c.put16(field(table, -0x625a), u32(count + 1));
    regs.apply();
    invoke(c, 0x10778300, 0x107197e0, {guio_at(table, count), 0, table::guio_size});
    const u32 last = u32(c.i16(field(table, -0x625a))) - 1;  // the new object's index
    store_parameters(c, frame, {r8, r9, r10, p38, p3e, p42, p44, p4a, p4e, p50, p54, p58, p5c, area});
    regs.apply();
    invoke(c, 0x10725a00, 0x10719858,
           {guio_at(table, std::int32_t(last)), table + table::areas, guio_at(table, 0), u32(half(last)), r4, r5, r6, r7});
    if (half(area) != -1) {
        const u32 owner = table + table::areas + area * table::area_size;
        regs.apply();
        invoke(c, 0x107144c0, 0x10719880, {owner, u32(half(u32(c.i16(field(table, -0x625a))) - 1))});
    } else {
        // The index list's routine takes the table and the new index; its frame saves only r30 and r31.
        c.link(0x1071989c);
        regs.apply();
        c.set(3, table);
        add_control_index(c, table, u32(half(u32(c.i16(field(table, -0x625a))) - 1)));
    }
    const u32 result = u32(half(u32(c.i16(field(table, -0x625a))) - 1));
    c.set(3, result);
    return result;
}

// 0x107198c0: the grid control. The original describes it to the network stream field by field (each
// write checked against the stream's capacity: a failure reports at that site and goes on), then takes
// the next area record and fills it through 0x10713d10.
std::uint32_t add_grid_control(Cpu& c, u32 table, u32 r4, u32 r5, u32 r6, u32 r7, u32 r8, u32 r9, u32 r10) {
    const Frame frame(c, 0x107198c0, 0x2b0, {.first_gpr = 13});
    CallRegisters regs(c);
    const auto stream = c.toc(-0x5c08), message = c.toc(toc::message), pool = c.toc(toc::pool), success = c.toc(toc::success);
    const auto parameters = frame.caller();
    const u32 p38 = c.u32(parameters + 0x38), p3c = c.u32(parameters + 0x3c), p42 = c.u16(parameters + 0x42),
              p46 = u32(c.i16(parameters + 0x46));
    // r13..r31 as the original holds them: r15 r4, r14 r5, r13 r6, r17 r7, r20 r8, r24 r9, r21 r10,
    // r23 and r22 the parameter words, r18 and r19 their halfwords; r16 stays at its entry value until
    // a string is sent.
    regs.hold({{26, stream}, {27, table}, {15, r4}, {25, success}, {28, message}, {31, stream + 4}, {30, pool},
               {29, stream + 8}, {14, r5}, {13, r6}, {17, r7}, {20, r8}, {24, r9}, {21, r10}, {23, p38}, {22, p3c},
               {18, p42}, {19, p46}});
    CheckedStream out(c, frame, stream, regs);  // a checked write reports and goes on; the stream is little-endian
    // A text: its length (the low byte of `length`) as a halfword, then the bytes copied by 0x10739ef0
    // without a capacity check (a quirk). r16 holds the length byte, then its sign extension.
    const auto put_text = [&](u32 length, u32 source, const Site& site, u32 back) {
        u32 r16 = length & 0xff;
        regs.hold({{16, r16}});
        out.write16(site, r16);
        r16 = u32(half(r16));
        regs.hold({{16, r16}});
        regs.apply();
        invoke(c, 0x10739ef0, back, {out.at_position(), source, r16});
        out.advance(r16);
    };
    out.write16(net(0x10719944, 0x240, 0xb8, 0x86), r4);
    out.write16(net(0x107199d0, 0x220, 0xb0, 0x86), r5);
    out.write16(net(0x10719a58, 0x200, 0xa8, 0x86), r6);
    out.write16(net(0x10719ae0, 0x1e0, 0xa0, 0x86), r7);
    out.write8(net(0x10719b64, 0x1c0, 0x98, 0x8a), r8);
    out.write8(net(0x10719be0, 0x1a0, 0x90, 0x85), r9 != 0);
    if (r9 != 0) put_text(r8, r9, net(0x10719c74, 0xe0, 0x60, 0x86), 0x10719ce4);
    out.write8(net(0x10719d18, 0x180, 0x88, 0x8a), r10);
    out.write8(net(0x10719d94, 0x160, 0x80, 0x85), p38 != 0);
    if (p38 != 0) put_text(r10, p38, net(0x10719e28, 0xc0, 0x58, 0x86), 0x10719e98);
    out.write32(net(0x10719ecc, 0x140, 0x78, 0x88), p3c);
    out.write16(net(0x10719f48, 0x120, 0x70, 0x8b), p42);
    out.write16(net(0x10719fc4, 0x100, 0x68, 0x86), p46);

    const auto count = c.i16(field(table, -0x625c));
    if (count >= 0xf9) {
        c.put8(field(table, -0x56be), 1);
        c.set(3, 0xffffffffu);
        return 0xffffffffu;
    }
    c.put16(field(table, -0x625c), u32(count + 1));
    regs.apply();
    invoke(c, 0x10778300, 0x1071a060, {table + table::areas + u32(count) * table::area_size, 0, table::area_size});
    const u32 last = u32(c.i16(field(table, -0x625c))) - 1;  // the new area's index
    store_parameters(c, frame, {r8, r9, r10, p38, p3c, p42, p46});
    invoke(c, 0x10713d10, 0x1071a0b8,
           {table + table::areas + last * table::area_size, table + table::areas, guio_at(table, 0), u32(half(last)), r4, r5, r6, r7});
    c.link(0x1071a0d0);
    regs.apply();
    c.set(3, table);
    add_area_index(c, table, u32(half(u32(c.i16(field(table, -0x625c))) - 1)));
    const u32 result = u32(half(u32(c.i16(field(table, -0x625c))) - 1));
    c.set(3, result);
    return result;
}
}

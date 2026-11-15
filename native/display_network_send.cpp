// DISPLAY.CPP block "network", part 1: the display table's network form, sending side.
//
// The original builds each message in the global stream (TOC -0x5c04: {bytes, position, length}) and
// hands it to 0x1073a4c0 with the peer (r4) and context (r5) it was given. Every write is checked
// against the stream's length (network.h: a failure reports at that site and goes on); the stream is
// little-endian. A failed send raises "Network Error / Unable to send data." through the ODD sequence
// at the send's own source line (DISPLAY.CPP).
//   0x1071c2d0  one part of the screen: header (0), GUI objects (1), the commands' stream (2), areas (3)
//   0x1071c1d0  the whole screen, with a log line of the byte counts
//   0x10723650  a resource path rewritten to the game's folders (pictures, sounds, data)
//   0x10723850  the reply {5, byte}
#include "display_network.hpp"
#include "display_detail.hpp"
#include <array>
#include <bit>

namespace cm::aot::display {
namespace {
using u32 = std::uint32_t;
using Regs = std::array<u32, 32>;
constexpr Site net(u32 at, std::int32_t name, std::int32_t extension, u32 line) {
    return {at, name, extension, line, text::network_header};
}
constexpr u32 half_bits(u32 value) { return value & 0xffff; }

// The global stream and the pool strings of the failure dialog.
namespace stream_field {
inline constexpr u32 position = 4;
}
namespace text_offset {
inline constexpr u32 network_error = 0x95, unable_to_send = 0xa3, screen_sent = 0x49;
}
// Display table fields (displacements from table + 0x130000).
namespace f {
inline constexpr std::int32_t area_count = -0x625c, guio_count = -0x625a, mirror_flag = -0x56c1;
}
// A GUI object record (0x17c bytes) as the game's getter (0x10725dc0) reads it, and the copies of the
// last send the game keeps per human (0x6f544 bytes each: the records, their count at +0x6f540).
namespace copy {
inline constexpr u32 size = 0x70000 - 0xabc, count = 0x6f540;
}
namespace human {
inline constexpr std::uint32_t row = 0x2c58;  // signed halfword of the main object
}
// Frame slots 0x1071c2d0 hands to the GUI object getter, and their positions in the previous object's
// set: the getter writes the 19 values of one object through these.
struct Slots { std::int32_t current, previous; };
namespace slot {
inline constexpr Slots a{0xe8, 0xb0}, b{0xe4, 0xac}, c{0xb8, 0x80}, d{0xdc, 0xa4}, e{0xd8, 0xa0}, f{0xd4, 0x9c},
                       g{0xd0, 0x98}, h{0xcc, 0x94}, i{0xc8, 0x90}, h76{0x76, 0x6c}, h74{0x74, 0x6a}, j{0xe0, 0xa8},
                       s7a{0x7a, 0x70}, h72{0x72, 0x68}, text{0xbc, 0x84}, k{0xc4, 0x8c}, l{0xc0, 0x88}, m{0xb4, 0x7c},
                       s78{0x78, 0x6e};
}

// What one call of 0x1071c2d0 holds: the frame, the registers r13-r31 as the original has them at every
// dispatched call (`r` mirrors them; `flush` hands them over), and the stream.
class Part {
public:
    Part(Cpu& cpu, const Frame& frame_, CallRegisters& regs_, u32 table_, u32 peer, u32 context, u32 mode)
        : c(cpu), frame(frame_), regs(regs_), table(table_), stream(c.toc(-0x5c04)), pool(c.toc(toc::pool)),
          message(c.toc(toc::message)), out(cpu, frame_, stream, [this] { flush(); }) {
        for (unsigned n = 13; n < 32; ++n) r[n] = regs[n];
        r[27] = stream, r[16] = peer, r[15] = table, r[18] = c.toc(-0x5c08), r[14] = c.toc(toc::game),
        r[28] = c.toc(toc::success), r[29] = message, r[17] = context, r[30] = pool, r[20] = mode,
        r[13] = c.toc(-0x5bfc), r[25] = stream + 4, r[24] = stream + 8;
    }
    void flush() {
        regs.hold({{13, r[13]}, {14, r[14]}, {15, r[15]}, {16, r[16]}, {17, r[17]}, {18, r[18]}, {19, r[19]}, {20, r[20]},
                   {21, r[21]}, {22, r[22]}, {23, r[23]}, {24, r[24]}, {25, r[25]}, {26, r[26]}, {27, r[27]}, {28, r[28]},
                   {29, r[29]}, {30, r[30]}, {31, r[31]}});
        regs.apply();
    }
    u32 position() const { return out.position(); }
    // A checked write of `size` bytes (little-endian): a failed check flushes the registers and reports.
    void put(unsigned size, u32 value, const Site& site) { out.write(site, size, value); }
    // 0x1073a4c0 on `buffer`/`length`; true when the low byte of its result is nonzero (extsb.).
    bool send(u32 buffer, u32 length, u32 back) {
        flush();
        return std::int8_t(invoke(c, 0x1073a4c0, back, {r[16], buffer, length, r[17]})) != 0;
    }
    bool send_stream(u32 back) { return send(out.bytes(), position(), back); }
    // The failure sequence of a send (the ODD sites): the assertion text, then the dialog "Network
    // Error / Unable to send data." (unlike the checked writes it leaves the success flag alone).
    void network_failure(const Site& site) {
        flush();
        invoke(c, 0x10739890, site.at + 4, {pool + text::source_file, 0, 0, frame.at(site.name), frame.at(site.extension)});
        const u32 number = invoke(c, 0x10768810, site.at + 8);
        invoke(c, 0x1077b720, site.at + 0x24,
               {message, pool + text::report_format, number, frame.at(site.name), frame.at(site.extension), site.line});
        invoke(c, 0x10727890, site.at + 0x30, {c.toc(-0x79dc), pool + text_offset::network_error});
        invoke(c, 0x10727890, site.at + 0x40, {c.toc(-0x79e0), pool + text_offset::unable_to_send, message});
        invoke(c, 0x10769b60, site.at + 0x50, {c.toc(-0x79dc), c.toc(-0x79e0), 0});
    }
    // The stream's bytes copied by 0x10739ef0 at the position, without a check (a quirk of the original).
    void copy_to_stream(u32 source, u32 length, u32 back) {
        flush();
        invoke(c, 0x10739ef0, back, {out.at_position(), source, length});
        out.advance(length);
    }

    Cpu& c;
    const Frame& frame;
    CallRegisters& regs;
    Regs r{};
    u32 table, stream, pool, message;
    CheckedStream out;
};

// Mode 0: the header (the game's title, the counts and two display flags).
u32 send_header(Part& p) {
    Cpu& c = p.c;
    const u32 table = p.table;
    p.flush();
    invoke(c, 0x1073edb0, 0x1071c3a8, {p.r[14]});  // the title; the next call measures it (r3 stays)
    const u32 size = invoke(c, 0x1077dbd0, 0x1071c3ac);
    const u32 game = p.r[14];
    p.r[14] = sign16(size + 1);
    p.flush();
    p.r[13] = invoke(c, 0x1073edb0, 0x1071c3bc, {game});
    p.put(2, half_bits(p.r[14]), net(0x1071c3e8, 0x234, 0xf4, 0x86));
    p.copy_to_stream(p.r[13], p.r[14], 0x1071c45c);
    p.put(1, 0, net(0x1071c490, 0x5d4, 0x1dc, 0x85));
    p.put(2, half_bits(u32(c.i16(field(table, f::area_count)))), net(0x1071c518, 0x5b4, 0x1d4, 0x86));
    p.put(2, half_bits(u32(c.i16(field(table, f::guio_count)))), net(0x1071c5a4, 0x594, 0x1cc, 0x86));
    p.put(1, c.u8(table + table::width_flag), net(0x1071c630, 0x574, 0x1c4, 0x85));
    p.put(2, half_bits(u32(c.i16(table + table::height_flag))), net(0x1071c6b4, 0x554, 0x1bc, 0x86));
    if (!p.send_stream(0x1071c724)) {
        p.network_failure({0x1071c740, 0x694, 0x20c, 0x81e});
        return 0;
    }
    return p.position();
}

// Mode 2: the stream the screen commands built (TOC -0x5c08), sent as it is.
u32 send_command_stream(Part& p) {
    Cpu& c = p.c;
    const u32 other = p.r[18];
    p.r[13] = other + 4;
    if (!p.send(c.u32(other), c.u32(other + 4), 0x1071c7c4)) {
        p.network_failure({0x1071c7e0, 0x674, 0x204, 0x82d});
        return 0;
    }
    return c.u32(p.r[13]);
}

// Mode 3: each area that has an id (the word at +0x200 of its record), as its own message {0x10, index, id};
// then the message {3}.
u32 send_areas(Part& p) {
    Cpu& c = p.c;
    const u32 table = p.table;
    const u32 index_slot = p.frame.at(0x78);
    p.r[14] = table + table::high;
    c.put16(index_slot, 0);
    p.r[19] = 0;
    while (c.i16(index_slot) < c.i16(p.r[14] + u32(f::area_count))) {
        const u32 id = c.u32(table + u32(c.i16(index_slot)) * table::area_size + 0x200);
        p.r[13] = id;
        if (id != 0xffffffffu) {
            c.put32(p.stream + stream_field::position, 0);
            p.put(1, 0x10, net(0x1071db48, 0x2b4, 0x114, 0x85));
            p.put(2, half_bits(u32(c.i16(index_slot))), net(0x1071dbcc, 0x294, 0x10c, 0x86));
            p.put(4, id, net(0x1071dc50, 0x274, 0x104, 0x88));
            if (!p.send_stream(0x1071dcb8)) {
                p.network_failure({0x1071dcd4, 0x634, 0x1f4, 0x8e7});
                return 0;
            }
            p.r[19] += p.position();
        }
        c.put16(index_slot, u32(c.i16(index_slot)) + 1);
    }
    c.put32(p.stream + stream_field::position, 0);
    p.put(1, p.r[20], net(0x1071dd84, 0x254, 0xfc, 0x85));
    if (!p.send_stream(0x1071ddec)) {
        p.network_failure({0x1071de08, 0x614, 0x1ec, 0x8f8});
        return 0;
    }
    return p.r[19] + p.position();
}

// One member of a GUI object's description: the flag bit that announces it, its place in the getter's
// slots, how it is compared and measured, and how it is written.
struct Member {
    u32 bit;
    Slots slots;
    bool half;    // a halfword slot (compared as 16 bits); otherwise a word
    unsigned compared;  // bytes it adds to a difference/full description
    unsigned written;   // 1, 2 or 4 bytes
    Site site;
};
constexpr Member members[] = {
    {0x1, slot::a, false, 4, 4, net(0x1071cec0, 0x514, 0x1ac, 0x88)},
    {0x2, slot::b, false, 4, 4, net(0x1071cf48, 0x4f4, 0x1a4, 0x88)},
    {0x4, slot::c, false, 4, 4, net(0x1071cfd0, 0x4d4, 0x19c, 0x89)},
    {0x8, slot::d, false, 2, 2, net(0x1071d05c, 0x4b4, 0x194, 0x86)},
    {0x10, slot::e, false, 2, 2, net(0x1071d0f4, 0x494, 0x18c, 0x86)},
    {0x20, slot::f, false, 2, 2, net(0x1071d18c, 0x474, 0x184, 0x86)},
    {0x40, slot::g, false, 2, 2, net(0x1071d224, 0x454, 0x17c, 0x86)},
    {0x80, slot::h, false, 1, 1, net(0x1071d2bc, 0x434, 0x174, 0x85)},
    {0x100, slot::i, false, 2, 2, net(0x1071d348, 0x414, 0x16c, 0x86)},
    {0x200, slot::h76, true, 2, 2, net(0x1071d3dc, 0x3f4, 0x164, 0x8b)},
    {0x400, slot::h74, true, 2, 2, net(0x1071d464, 0x3d4, 0x15c, 0x8b)},
    {0x800, slot::j, false, 4, 4, net(0x1071d4ec, 0x3b4, 0x154, 0x88)},
    {0x1000, slot::s7a, true, 1, 1, net(0x1071d578, 0x394, 0x14c, 0x85)},
    {0x2000, slot::h72, true, 2, 2, net(0x1071d600, 0x374, 0x144, 0x8b)},
    {0x4000, slot::text, false, 0, 0, net(0x1071d698, 0x214, 0xec, 0x86)},  // the text: handled apart
    {0x8000, slot::k, false, 2, 2, net(0x1071d750, 0x354, 0x13c, 0x86)},
    {0x10000, slot::l, false, 4, 4, net(0x1071d7e4, 0x334, 0x134, 0x88)},
    {0x20000, slot::m, false, 4, 4, net(0x1071d86c, 0x314, 0x12c, 0x89)},
    {0x40000, slot::s78, true, 2, 2, net(0x1071d8f4, 0x2f4, 0x124, 0x86)},
};
constexpr Site flags_site = net(0x1071ce38, 0x534, 0x1b4, 0x88);
constexpr Site duplicate_site = net(0x1071d980, 0x2d4, 0x11c, 0x86);
constexpr u32 duplicate_bit = 0x80000, text_bit = 0x4000;

// The getter 0x10725dc0 writes the 19 values of one GUI object through seven pointers (r4-r10) and
// twelve more in the parameter area at sp +0x38: this routine's frame slots, one set for the object and
// one for its copy from the last send.
u32 call_getter(Part& p, u32 object, bool previous, u32 back) {
    Cpu& c = p.c;
    const auto at = [&](Slots s) { return p.frame.at(previous ? s.previous : s.current); };
    const std::array<Slots, 12> stacked{slot::b, slot::h76, slot::h74, slot::j, slot::s7a, slot::h72,
                                        slot::text, slot::k, slot::l, slot::m, slot::s78, slot::c};
    u32 offset = 0x38;
    for (const auto s : stacked) {
        c.put32(p.frame.at(std::int32_t(offset)), at(s));
        offset += 4;
    }
    p.flush();
    return invoke(c, 0x10725dc0, back,
                  {object, at(slot::a), at(slot::d), at(slot::e), at(slot::f), at(slot::g), at(slot::h), at(slot::i)});
}

// Mode 1: every GUI object of the table. Each is described by a flag word and the members it names:
// all its nonzero members, or, when the table is mirrored and the previous send had this object, the
// members that differ from the copy (the shorter of the two), or a back-reference to an identical
// object among the ten before it (flag 0x80000 and its index).
u32 send_gui_objects(Part& p) {
    Cpu& c = p.c;
    const u32 table = p.table, base = table + table::high;
    const Frame& frame = p.frame;
    const auto value = [&](const Member& m, bool previous) {
        const u32 at = frame.at(previous ? m.slots.previous : m.slots.current);
        return m.half ? u32(c.u16(at)) : c.u32(at);
    };
    const auto text_slot = [&](bool previous) { return c.u32(frame.at(previous ? slot::text.previous : slot::text.current)); };
    p.r[31] = base, p.r[23] = 0, p.r[26] = 0;
    while (std::int16_t(p.r[23]) < c.i16(base + u32(f::guio_count))) {
        call_getter(p, table + table::guios + p.r[26], false, 0x1071c8e4);
        p.r[21] = 0, p.r[22] = 0, p.r[19] = 0, p.r[20] = 0;
        const u32 shared = c.u32(p.r[13]);
        if (c.i8(base + u32(f::mirror_flag)) != 0 && shared != 0) {
            const u32 copy_base = shared + u32(c.i16(p.r[14] + human::row)) * copy::size;
            if (std::int16_t(p.r[23]) < c.i16(copy_base + copy::count)) {
                p.r[18] = frame.at(0x7c), p.r[22] = frame.at(0x6e);
                call_getter(p, copy_base + p.r[26], true, 0x1071c9b8);
                p.r[22] = p.r[21] | 0x100000;
                p.r[20] = 0;
                for (const auto& m : members) {
                    if (m.bit == text_bit) continue;
                    if (value(m, false) != value(m, true)) p.r[22] |= m.bit, p.r[20] += m.compared;
                }
                // The text differs when both exist and the string compare says so; its length is added.
                const u32 now = text_slot(false), before = text_slot(true);
                if (now != 0 && before != 0) {
                    p.flush();
                    if (invoke(c, 0x1077dc90, 0x1071cb88, {now, before}) != 0) {
                        p.flush();
                        p.r[20] += invoke(c, 0x1077dbd0, 0x1071cb98, {now});
                        p.r[22] |= text_bit;
                    }
                }
            }
        }
        // The full description: nonzero members, the text (when not empty) among them in the flag order.
        for (const auto& m : members) {
            if (m.bit == text_bit) {
                const u32 now = text_slot(false);
                if (now != 0 && c.i8(now) != 0) {
                    p.flush();
                    p.r[19] += invoke(c, 0x1077dbd0, 0x1071ccd4, {now});
                    p.r[21] |= text_bit;
                }
            } else if (value(m, false) != 0) {
                p.r[21] |= m.bit, p.r[19] += m.compared;
            }
        }
        // An identical object among the ten before this one: its index replaces the members.
        p.r[18] = u32(-1);
        const std::int32_t here = std::int16_t(p.r[23]);
        u32 candidate = sign16(u32(here - 1)), offset = candidate * 0x17c;
        const std::int32_t lowest = here - 10;
        while (std::int16_t(candidate) >= 0 && std::int16_t(p.r[18]) == -1 && std::int16_t(candidate) > lowest) {
            const u32 record = table + table::guios + offset;
            const auto same = [&](const Member& m, u32 at, bool half) {
                return value(m, false) == (half ? u32(c.u16(record + at)) : c.u32(record + at));
            };
            if (same(members[0], 0xc, false) && same(members[1], 0x38, false) && same(members[9], 0x6a, true) &&
                same(members[10], 0x6c, true) && same(members[11], 0x3c, false) && same(members[12], 0x6e, true) &&
                same(members[13], 0x70, true)) {
                p.r[19] -= 0x11;
                p.r[21] = (p.r[21] | duplicate_bit) & 0xffffc1fc;
                p.r[18] = candidate;
            }
            offset -= 0x17c;
            --candidate;
        }
        if (p.r[22] != 0 && std::int32_t(p.r[20]) < std::int32_t(p.r[19])) p.r[21] = p.r[22];
        p.put(4, p.r[21], flags_site);
        for (const auto& m : members) {
            if ((p.r[21] & m.bit) == 0) continue;
            if (m.bit == text_bit) {
                const u32 string = text_slot(false);
                p.flush();
                const u32 length = invoke(c, 0x1077dbd0, 0x1071d664, {string});
                p.r[20] = sign16(length + 1);
                p.r[19] = string;
                p.put(2, half_bits(p.r[20]), m.site);
                p.copy_to_stream(p.r[19], p.r[20], 0x1071d70c);
            } else {
                p.put(m.written, value(m, false), m.site);
            }
        }
        if ((p.r[21] & duplicate_bit) != 0) p.put(2, half_bits(p.r[18]), duplicate_site);
        p.r[26] += 0x17c;
        p.r[23] += 1;
    }
    if (!p.send_stream(0x1071da08)) {
        p.network_failure({0x1071da24, 0x654, 0x1fc, 0x8c4});
        return 0;
    }
    // The copy the next send is compared with.
    if (c.i8(base + u32(f::mirror_flag)) != 0 && c.u32(p.r[13]) != 0) {
        const u32 shared = c.u32(p.r[13]);
        c.put16(shared + u32(c.i16(p.r[14] + human::row)) * copy::size + copy::count, u32(c.i16(base + u32(f::guio_count))));
        const u32 count = u32(c.i16(base + u32(f::guio_count))), row = u32(c.i16(p.r[14] + human::row));
        p.flush();
        invoke(c, 0x10739ef0, 0x1071dadc, {c.u32(p.r[13]) + row * copy::size, table + table::guios, count * 0x17c});
    }
    return p.position();
}
}

u32 send_screen_part(Cpu& c, u32 table, u32 peer, u32 context, u32 part) {
    const Frame frame(c, 0x1071c2d0, 0x710, {.first_gpr = 13});
    CallRegisters regs(c);
    Part p(c, frame, regs, table, peer, context, part);
    c.put32(p.stream + stream_field::position, 0);
    p.put(1, part, net(0x1071c344, 0x5f4, 0x1e4, 0x85));
    // r19 is the caller's when no part matches (the original returns its entry value).
    u32 result = p.r[19];
    switch (std::int8_t(part)) {
        case 0: result = send_header(p); break;
        case 2: result = send_command_stream(p); break;
        case 1: result = send_gui_objects(p); break;
        case 3: result = send_areas(p); break;
        default: break;
    }
    c.set(3, result);
    return result;
}

void send_screen(Cpu& c, u32 table, u32 peer, u32 context) {
    const Frame frame(c, 0x1071c1d0, 0x60, {.first_gpr = 24});
    CallRegisters regs(c);
    regs.hold({{24, table}, {25, peer}, {26, context}, {27, 0}, {28, 0}, {29, 0}});
    if (peer == 0 || c.i8(peer + 0xc35b8) == 0) return;
    const auto part = [&](u32 mode, u32 back) {
        regs.apply();
        c.link(back);
        return send_screen_part(c, table, peer, context, mode);
    };
    const u32 header = part(0, 0x1071c214);
    u32 area_bytes = 0, gui_object_bytes = 0, command_bytes = 0, total = header;
    regs.hold({{30, header}, {31, header}});
    if (c.u8(table + table::reply_wanted) == 0) {
        command_bytes = part(2, 0x1071c240);
        regs.hold({{29, command_bytes}, {31, total + command_bytes}});
        total += command_bytes;
        gui_object_bytes = part(1, 0x1071c260);
        regs.hold({{28, gui_object_bytes}, {31, total + gui_object_bytes}});
        total += gui_object_bytes;
        area_bytes = part(3, 0x1071c280);
        regs.hold({{27, area_bytes}});
        total += area_bytes;
        regs.hold({{31, total}});
    }
    c.put32(frame.at(0x38), area_bytes);
    regs.apply();
    invoke(c, 0x1077b720, 0x1071c2b8,
           {c.toc(-0x7bb8), c.toc(toc::pool) + text_offset::screen_sent, total, header, u32(c.i16(field(table, f::area_count))), command_bytes,
            u32(c.i16(field(table, f::guio_count))), gui_object_bytes});
}

void localize_resource_path(Cpu& c, u32 path, u32 pictures_first) {
    const Frame frame(c, 0x10723650, 0x150, {.first_gpr = 29});
    CallRegisters regs(c);
    const u32 pool = c.toc(toc::pool);
    regs.hold({{31, path}, {30, pool}, {29, pictures_first}});
    regs.apply();
    if (invoke(c, 0x1077dbd0, 0x10723678, {path}) == 0) return;
    if (std::int8_t(invoke(c, 0x10739d60, 0x10723688, {path})) != 0) return;
    invoke(c, 0x10739640, 0x10723698, {path});
    const u32 buffer = frame.at(0x38);
    // The last occurrence of `folder` in the path; true (and the replaced name sent through the data
    // path routine to the path) when one is found. Backslashes after it become colons.
    const auto find = [&](u32 folder, u32 skip, u32 name, u32 first_back, u32 data_back, u32 copy_back) {
        u32 found = path;
        regs.hold({{29, found}});
        for (;;) {
            regs.apply();
            const u32 next = invoke(c, 0x1077dda0, first_back, {found, pool + folder});
            if (next == 0) break;
            found = next + skip;
            regs.hold({{29, found}});
        }
        if (found == path) return false;
        u32 source = found + 1, target = buffer;
        for (;;) {
            const u32 ch = c.u8(source++);
            c.put8(target++, ch);
            if (std::int8_t(ch) == 0) break;
            if (std::int8_t(ch) == 0x5c) c.put8(target - 1, 0x3a);
        }
        regs.hold({{29, source}});
        regs.apply();
        const u32 resolved = invoke(c, 0x10746280, data_back, {pool + name, buffer});
        invoke(c, 0x1077dbf0, copy_back, {path, resolved});
        return true;
    };
    if (std::int8_t(pictures_first) != 0) {
        if (find(0x107, 8, 0x110, 0x107236bc, 0x10723714, 0x10723720)) return;
    } else {
        if (find(0x11d, 6, 0x124, 0x10723740, 0x10723798, 0x107237a4)) return;
    }
    find(0x12f, 4, 0x134, 0x107237c4, 0x1072381c, 0x10723828);
}

void send_reply(Cpu& c, u32 peer, u32 context, u32 byte) {
    const Frame frame(c, 0x10723850, 0xe0, {.first_gpr = 21});
    CallRegisters regs(c);
    const u32 stream = c.toc(-0x5c04), pool = c.toc(toc::pool), message = c.toc(toc::message);
    regs.hold({{28, stream}, {21, peer}, {26, c.toc(-0x79e0)}, {22, context}, {27, c.toc(-0x79dc)}, {25, stream + 4},
               {29, c.toc(toc::success)}, {24, stream + 8}, {30, message}, {23, byte}, {31, pool}});
    c.put32(stream + stream_field::position, 0);
    CheckedStream out(c, frame, stream, regs);
    out.write8(net(0x107238bc, 0x70, 0x40, 0x85), 5);
    out.write8(net(0x1072393c, 0x50, 0x38, 0x85), byte);
    regs.apply();
    const u32 length = c.u32(stream + stream_field::position);
    if (std::int8_t(invoke(c, 0x1073a4c0, 0x107239a4, {peer, c.u32(stream), length, context})) != 0) return;
    invoke(c, 0x10739890, 0x107239c4, {pool, 0, 0, frame.at(0x90), frame.at(0x48)});
    const u32 number = invoke(c, 0x10768810, 0x107239c8);
    invoke(c, 0x1077b720, 0x107239e4, {message, pool + text::report_format, number, frame.at(0x90), frame.at(0x48), 0xc4a});
    invoke(c, 0x10727890, 0x107239f0, {c.toc(-0x79dc), pool + text_offset::network_error});
    invoke(c, 0x10727890, 0x10723a00, {c.toc(-0x79e0), pool + text_offset::unable_to_send, message});
    invoke(c, 0x10769b60, 0x10723a10, {c.toc(-0x79dc), c.toc(-0x79e0), 0});
}
}

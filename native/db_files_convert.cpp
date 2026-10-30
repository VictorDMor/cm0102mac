#include "db_files_detail.hpp"
#include <bit>

namespace cm::aot::db_files {
namespace {
using namespace guest;

// One record's conversion: fields little-endian on disk become big-endian;
// bytes and text are copied. Fields are written in the original's order, and
// fields it leaves out stay as the destination holds them. This module only moves
// a table's fields and never interprets them, so offsets are their names here.
struct Record {
    Cpu& c;
    std::uint32_t out, in;
    std::uint32_t word(std::uint32_t to, std::uint32_t from) const {
        const auto raw = c.u32(in + from);
        c.put32(out + to, std::byteswap(raw));
        return raw;
    }
    std::uint32_t half(std::uint32_t to, std::uint32_t from) const {
        const auto raw = c.i16(in + from);
        c.put16(out + to, std::byteswap(std::uint16_t(raw)));
        return extended(raw);
    }
    std::uint32_t byte(std::uint32_t to, std::uint32_t from) const {
        const auto value = c.u8(in + from);
        c.put8(out + to, value);
        return value;
    }
    // A date (day, year, flag) copied, then its two fields swapped in place.
    void date(std::uint32_t to, std::uint32_t from) const {
        c.put16(out + to + 2, c.u16(in + from + 2));
        c.put16(out + to, c.u16(in + from));
        c.put8(out + to + 4, c.u8(in + from + 4));
        c.put16(out + to, std::byteswap(c.u16(out + to)));
        c.put16(out + to + 2, std::byteswap(c.u16(out + to + 2)));
    }
    // BlockMoveData through the transition vector. It saves the TOC at sp+20;
    // r3-r5 stay as the call leaves them.
    void text(std::uint32_t to, std::uint32_t from, std::uint32_t size, std::uint32_t back) const {
        c.set({{3, in + from}, {4, out + to}, {5, size}});
        c.call(block_move, back);
        c.set(2, c.u32(c.sp() + 20));
    }
    // A double swapped through two stack slots, the unswapped bytes in `image`
    // and the result in `swapped`, both left behind in the frame. The original
    // holds them in f0 on the way. Returns the unswapped high word.
    std::uint32_t real(std::uint32_t to, std::uint32_t from, std::uint32_t image, std::uint32_t swapped) const {
        const auto raw = c.u64(in + from);
        const auto value = std::byteswap(raw);
        c.put64(image, raw);
        c.put64(swapped, value);
        c.s.wr<8>(reg::fpr(0), value);
        c.put64(out + to, value);
        return std::uint32_t(raw >> 32);
    }
};

// The routines that swap doubles build their masks from these before the
// loop, so an empty table still leaves them.
void leave_swap_masks(Cpu& c) { leave(c, {{6, 0xff000000}, {7, 0x00ff0000}, {8, 0x00010000}}); }

// The loops that keep no frame: `mtctr; cmpwi; beqlr`, and r3, r4, r5 advance.
template <class Convert>
void leaf_loop(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count,
               std::uint32_t out_size, std::uint32_t in_size, Convert&& convert) {
    if (compare(c, std::int32_t(count)) == 0) return;
    for (auto left = count; left != 0; --left, out += out_size, in += in_size) {
        const Record record{c, out, in};
        convert(record);
        c.set(3, out + out_size);
        c.set(4, in + in_size);
    }
}
// The loops in a frame: `cmpwi r,0; subi r,r,1; bne`, leaving cr0 equal.
template <class Convert>
void framed_loop(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count,
                 std::uint32_t out_size, std::uint32_t in_size, Convert&& convert) {
    for (auto left = count; left != 0; --left, out += out_size, in += in_size)
        convert(Record{c, out, in});
    c.compare(0, 0, 0);
}
}

void convert_continents(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    Frame frame(c, 0x105d7530, 0x80, {.first_gpr = 20});
    leave_swap_masks(c);
    framed_loop(c, out, in, count, 0xc8, 0xc6, [&](const Record& r) {
        const auto high = r.real(0, 0xbe, frame.at(0x38), frame.at(0x40));
        c.set({{6, r.word(8, 0)}, {7, (high >> 8) & 0xff00}, {8, 0}, {9, high >> 24}, {10, 0}});
        r.text(0xc, 0x1f, 4, 0x105d7648);
        r.text(0x10, 4, 0x1a, 0x105d765c);
        r.text(0x2a, 0x23, 0x1a, 0x105d7670);
        r.text(0x44, 0x3d, 0x65, 0x105d7684);
        r.text(0xa9, 0xa3, 0x1a, 0x105d7698);
        r.byte(0xc3, 0x1e);
        r.byte(0xc4, 0xa2);
        r.byte(0xc5, 0xbd);
    });
}

void convert_nations(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    Frame frame(c, 0x105d7a30, 0x130, {.first_gpr = 23});
    leave_swap_masks(c);
    framed_loop(c, out, in, count, 0x128, 0x122, [&](const Record& r) {
        std::uint32_t high = 0;
        for (std::uint32_t i = 0; i < 13; ++i)
            high = r.real(i * 8, 0xa8 + i * 8, frame.at(0xf8 - i * 0x10), frame.at(0x100 - i * 0x10));
        c.set({{7, (high >> 8) & 0xff00}, {8, high >> 24}, {9, 0}, {10, 0}});
        r.word(0x68, 0);
        r.word(0x6c, 0x88);
        r.word(0x70, 0x11d);
        r.word(0x74, 0x71);
        r.word(0x78, 0x7a);
        r.word(0x7c, 0x80);
        for (std::uint32_t i = 0; i < 6; ++i) r.word(0x80 + i * 4, 0x90 + i * 4);
        r.word(0x98, 0x110);
        r.word(0x9c, 0x114);
        r.word(0xa0, 0x118);
        r.text(0xa4, 0x53, 4, 0x105d8460);
        r.half(0xa8, 0x86);
        r.half(0xaa, 0x8c);
        c.set(6, r.half(0xac, 0x8e));
        r.text(0xae, 4, 0x33, 0x105d84a4);
        r.text(0xe1, 0x38, 0x1a, 0x105d84b8);
        r.text(0xfb, 0x57, 0x1a, 0x105d84cc);
        constexpr std::uint8_t bytes[][2]{{0x15, 0x37}, {0x16, 0x52}, {0x17, 0x75}, {0x18, 0x76}, {0x19, 0x77},
            {0x1a, 0x78}, {0x1b, 0x79}, {0x1c, 0x7e}, {0x1d, 0x7f}, {0x1e, 0x84}, {0x1f, 0x85}};
        for (const auto& [to, from] : bytes) r.byte(0x100u + to, from);
        r.byte(0x120, 0x11c);
        r.byte(0x121, 0x121);
    });
}

void convert_cities(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    Frame frame(c, 0x105d88b0, 0x80, {.first_gpr = 23});
    leave_swap_masks(c);
    framed_loop(c, out, in, count, 0x38, 0x38, [&](const Record& r) {
        r.real(0, 0x23, frame.at(0x48), frame.at(0x50));
        const auto high = r.real(8, 0x2b, frame.at(0x38), frame.at(0x40));
        // What the swap's masks leave: pieces of the second double.
        c.set({{7, 0xff}, {8, 0}, {9, (high >> 8) & 0xff00}, {10, (high << 8) & 0xff0000}});
        r.word(0x10, 0);
        r.word(0x14, 0x34);
        c.set(6, r.word(0x18, 0x1f));
        r.text(0x1c, 4, 0x1a, 0x105d8a94);
        r.byte(0x36, 0x1e);
        r.byte(0x37, 0x33);
    });
}

void convert_stadiums(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    Frame frame(c, 0x105d8e20, 0x50, {.first_gpr = 29});
    framed_loop(c, out, in, count, 0x50, 0x4e, [&](const Record& r) {
        r.word(0, 0);
        r.word(4, 0x3c);
        r.word(8, 0x40);
        r.word(0xc, 0x44);
        r.word(0x10, 0x38);
        c.set(6, r.word(0x14, 0x48));
        r.text(0x18, 4, 0x33, 0x105d8e9c);
        r.byte(0x4b, 0x37);
        r.byte(0x4c, 0x4c);
        r.byte(0x4d, 0x4d);
    });
}

void convert_clubs(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    Frame frame(c, 0x105d9290, 0x50, {.first_gpr = 29});
    framed_loop(c, out, in, count, 0x248, 0x245, [&](const Record& r) {
        constexpr std::uint16_t words[][2]{{0, 0}, {4, 0x65}, {8, 0x73}, {0xc, 0x77}, {0x10, 0x7b}, {0x14, 0x1db},
            {0x18, 0x230}, {0x1c, 0x234}, {0x20, 0x238}, {0x24, 0x23c}, {0x28, 0x240}, {0x2c, 0x53}, {0x30, 0x57},
            {0x34, 0x5b}, {0x38, 0x60}, {0x3c, 0x69}, {0x40, 0x6e}};
        for (const auto& [to, from] : words) r.word(to, from);
        for (std::uint32_t i = 0; i < 21; ++i) r.word(0x44 + i * 4, 0x83 + i * 4);
        // The original's unrolled copy leaves the 47th raw word in r7.
        for (std::uint32_t i = 0; i < 50; ++i) {
            const auto raw = r.word(0x98 + i * 4, 0xd7 + i * 4);
            if (i == 46) c.set(7, raw);
        }
        for (std::uint32_t i = 0; i < 15; ++i) r.word(0x160 + i * 4, 0x19f + i * 4);
        for (std::uint32_t i = 0; i < 20; ++i) r.word(0x19c + i * 4, 0x1e0 + i * 4);
        c.set(6, r.half(0x1ec, 0x80));
        r.text(0x1ee, 4, 0x33, 0x105d966c);
        r.text(0x221, 0x38, 0x1a, 0x105d9680);
        r.byte(0x23b, 0x52);
        r.byte(0x23c, 0x5f);
        r.byte(0x23d, 0x64);
        r.byte(0x23e, 0x6d);
        r.byte(0x23f, 0x72);
        r.byte(0x240, 0x7f);
        r.byte(0x241, 0x82);
        r.byte(0x242, 0x37);
        r.byte(0x243, 0x1df);
        r.byte(0x244, 0x244);
    });
}

void convert_staff(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    leaf_loop(c, out, in, count, 0x70, 0x6e, [&](const Record& r) {
        constexpr std::uint8_t words[][2]{{0, 0}, {4, 0x4e}, {8, 0x52}, {0xc, 0x1a}, {0x10, 0x1e}, {0x14, 0x61},
            {0x18, 0x65}, {0x1c, 0x69}, {0x20, 0x24}, {0x24, 0x39}, {0x28, 4}, {0x2c, 8}, {0x30, 0xc}};
        for (const auto& [to, from] : words) r.word(to, from);
        r.date(0x34, 0x10);
        r.date(0x3c, 0x29);
        r.date(0x44, 0x31);
        r.date(0x4c, 0x3e);
        r.date(0x54, 0x46);
        c.set(6, r.half(0x5c, 0x18));
        constexpr std::uint8_t bytes[][2]{{0x5e, 0x22}, {0x5f, 0x23}, {0x60, 0x28}, {0x61, 0x56}, {0x62, 0x3d},
            {0x63, 0x57}, {0x64, 0x58}, {0x65, 0x59}, {0x66, 0x5a}, {0x67, 0x5b}, {0x68, 0x5c}, {0x69, 0x5d},
            {0x6a, 0x5e}, {0x6b, 0x5f}, {0x6c, 0x60}};
        for (const auto& [to, from] : bytes) r.byte(to, from);
        c.set(5, r.byte(0x6d, 0x6d));
        // The original copies this byte and then clears it.
        c.put8(r.out + 0x6d, 0);
    });
}

void convert_oldstaff(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    leaf_loop(c, out, in, count, 0x9d, 0x9d, [&](const Record& r) {
        for (std::uint32_t i = 0; i < 4; ++i) r.word(i * 4, i * 4);
        r.date(0x10, 0x10);
        r.half(0x18, 0x18);
        r.word(0x1a, 0x1a);
        r.word(0x1e, 0x1e);
        r.byte(0x22, 0x22);
        r.byte(0x23, 0x23);
        r.word(0x24, 0x24);
        r.byte(0x28, 0x28);
        r.date(0x29, 0x29);
        r.date(0x31, 0x31);
        r.word(0x39, 0x39);
        r.byte(0x3d, 0x3d);
        r.date(0x3e, 0x3e);
        r.date(0x46, 0x46);
        r.word(0x4e, 0x4e);
        r.word(0x52, 0x52);
        for (std::uint32_t at = 0x56; at <= 0x60; ++at) r.byte(at, at);
        std::uint32_t last = 0;
        for (std::uint32_t at = 0x61; at <= 0x99; at += 4) last = r.word(at, at);
        c.set(5, last);
    });
}

void convert_non_players(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    leaf_loop(c, out, in, count, 0x44, 0x44, [&](const Record& r) {
        r.word(0, 0);
        for (std::uint32_t i = 0; i < 8; ++i) r.word(4 + i * 4, 0x23 + i * 4);
        std::uint32_t last = 0;
        for (std::uint32_t i = 0; i < 5; ++i) last = r.half(0x24 + i * 2, 4 + i * 2);
        for (std::uint32_t i = 0; i < 0x15; ++i) r.byte(0x2e + i, 0xe + i);
        r.byte(0x43, 0x43);
        c.set(5, last);
    });
}

void convert_players(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    leaf_loop(c, out, in, count, 0x48, 0x46, [&](const Record& r) {
        r.word(0, 0);
        std::uint32_t last = 0;
        for (std::uint32_t i = 0; i < 5; ++i) last = r.half(4 + i * 2, 5 + i * 2);
        r.byte(0xe, 4);
        for (std::uint32_t at = 0xf; at <= 0x45; ++at) r.byte(at, at);
        c.set(5, last);
    });
}

void convert_staff_preferences(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    leaf_loop(c, out, in, count, 0x34, 0x34, [&](const Record& r) {
        std::uint32_t last = 0;
        for (std::uint32_t at = 0; at <= 0x30; at += 4) last = r.word(at, at);
        c.set(5, last);
    });
}

void convert_staff_history(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    leaf_loop(c, out, in, count, 0x14, 0x11, [&](const Record& r) {
        r.word(0, 0);
        r.word(4, 4);
        r.word(8, 0xa);
        c.set(5, r.half(0xc, 8));
        r.byte(0xe, 0xe);
        r.byte(0xf, 0xf);
        r.byte(0x10, 0x10);
    });
}

void convert_club_comps(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    Frame frame(c, 0x105de450, 0x50, {.first_gpr = 29});
    framed_loop(c, out, in, count, 0x6c, 0x6b, [&](const Record& r) {
        r.word(0, 0);
        r.word(4, 0x59);
        r.word(8, 0x5d);
        r.word(0xc, 0x61);
        r.word(0x10, 0x65);
        r.text(0x14, 0x53, 4, 0x105de4c0);
        c.set(6, r.half(0x18, 0x69));
        r.text(0x1a, 4, 0x33, 0x105de4e4);
        r.text(0x4d, 0x38, 0x1a, 0x105de4f8);
        r.byte(0x67, 0x52);
        r.byte(0x68, 0x57);
        r.byte(0x69, 0x58);
        r.byte(0x6a, 0x37);
    });
}

void convert_club_comp_history(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    leaf_loop(c, out, in, count, 0x1c, 0x1a, [&](const Record& r) {
        r.word(0, 0);
        r.word(4, 4);
        r.word(8, 0xa);
        r.word(0xc, 0xe);
        r.word(0x10, 0x12);
        r.word(0x14, 0x16);
        c.set(5, r.half(0x18, 8));
    });
}

void convert_staff_comps(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    Frame frame(c, 0x105df0c0, 0x50, {.first_gpr = 29});
    framed_loop(c, out, in, count, 0x68, 0x65, [&](const Record& r) {
        r.word(0, 0);
        r.word(4, 0x53);
        r.word(8, 0x57);
        r.word(0xc, 0x5b);
        r.word(0x10, 0x5f);
        c.set(6, r.half(0x14, 0x63));
        r.text(0x16, 0x38, 0x1a, 0x105df140);
        r.text(0x30, 4, 0x33, 0x105df154);
        r.byte(0x63, 0x52);
        r.byte(0x64, 0x37);
    });
}

void convert_staff_comp_history(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    leaf_loop(c, out, in, count, 0x3c, 0x3a, [&](const Record& r) {
        constexpr std::uint8_t words[][2]{{0, 0}, {4, 0x16}, {8, 0x26}, {0xc, 0x36}, {0x10, 4}, {0x14, 0x12},
            {0x18, 0x22}, {0x1c, 0x32}, {0x20, 0xa}, {0x24, 0xe}, {0x28, 0x1a}, {0x2c, 0x1e}, {0x30, 0x2a}, {0x34, 0x2e}};
        for (const auto& [to, from] : words) r.word(to, from);
        c.set(5, r.half(0x38, 8));
    });
}

void convert_officials(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    leaf_loop(c, out, in, count, 0x2c, 0x2b, [&](const Record& r) {
        r.word(0, 0);
        r.word(4, 4);
        r.word(8, 8);
        r.word(0xc, 0x16);
        r.word(0x10, 0x1a);
        r.half(0x14, 0x14);
        r.half(0x16, 0x1e);
        r.half(0x18, 0x20);
        r.half(0x1a, 0x22);
        r.date(0x1c, 0xc);
        c.set(5, c.u16(r.in + 0xe));
        for (std::uint32_t at = 0x24; at <= 0x2a; ++at) r.byte(at, at);
    });
}

void convert_colours(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    Frame frame(c, 0x105dfd60, 0x50, {.first_gpr = 29});
    framed_loop(c, out, in, count, 0x3c, 0x3a, [&](const Record& r) {
        c.set(6, r.word(0, 0));
        r.text(4, 4, 0x33, 0x105dfda0);
        r.byte(0x37, 0x37);
        r.byte(0x38, 0x38);
        r.byte(0x39, 0x39);
    });
}

void convert_names(Cpu& c, std::uint32_t out, std::uint32_t in, std::uint32_t count) {
    Frame frame(c, 0x105e0140, 0x50, {.first_gpr = 29});
    framed_loop(c, out, in, count, 0x2c, 0x3c, [&](const Record& r) {
        r.text(0, 0, 0x20, 0x105e0178);
        r.word(0x20, 0x33);
        c.set(3, r.word(0x24, 0x37));
        r.byte(0x28, 0x3b);
    });
}
}

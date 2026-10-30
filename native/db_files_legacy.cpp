#include "db_files_detail.hpp"

namespace cm::aot::db_files {
namespace {
using namespace guest;

// Version 1 files, read field by field. A read's two call sites (packed stream
// or plain) sit at fixed distances from its `extsb.`, depending on whether
// computing the address takes one instruction or two.
struct FieldReader {
    Cpu& c;
    std::uint32_t file, packed;
    bool read(std::uint32_t site, std::uint32_t address, std::uint32_t size, unsigned address_instructions = 1) const {
        const auto extra = (address_instructions - 1) * 4;
        const auto is_packed = compare(c, std::int8_t(packed)) != 0;
        c.set({{6, file}, {3, address}, {4, size}, {5, 1}});
        if (is_packed) c.call(packed_read, site + 0x1c + extra);
        else c.call(stream_read, site + 0x34 + extra * 2);
        if (compare_logical(c, c.r(3), 1) >= 0) return true;
        c.set(3, 0);
        return false;
    }
    // A halfword stored on disk as one byte, read through a stack slot. The
    // original leaves the byte in r3 while it stores the field.
    void store_byte_as_half(std::uint32_t slot, std::uint32_t field) const {
        c.set(3, c.u8(slot));
        c.put16(field, c.r(3));
    }
};

// `extsb. r0,packed; beq; li r0,1` or the end marker's test, then the assertion.
void check_end_marker(Cpu& c, const Frame& frame, std::uint32_t file, std::uint32_t packed,
                      std::uint32_t marker_back, std::uint32_t assertion_at, std::uint32_t line) {
    bool mac = compare(c, std::int8_t(packed)) != 0;
    if (!mac) {
        c.set(3, file);
        c.call(read_end_marker, marker_back);
        mac = c.r(3) == mac_end_marker;
        c.set(4, 0xf2000000);
    }
    if (compare(c, mac) == 0)
        report_assertion(c, assertion(c, assertion_at, frame.at(0x44), frame.at(0x3c), line));
}
}

std::uint32_t read_old_clubs(Cpu& c, std::uint32_t file, std::uint32_t packed, std::uint32_t records, std::uint32_t count) {
    Frame frame(c, 0x105d9710, 0xa0, {.first_gpr = 21});
    keep(c, {{23, c.toc(-0x7ba8)}, {24, c.toc(pool_slot)}, {30, file}, {29, packed}, {27, records}, {25, count}});
    const FieldReader in{c, file, packed};
    const auto byte_slot = frame.at(0x38);
    std::uint32_t index = 0, offset = 0;
    keep(c, {{26, index}, {28, offset}});
    for (; compare_logical(c, index, count) < 0; ++index, offset += 0x248, keep(c, {{26, index}, {28, offset}})) {
        const auto record = records + offset;
        if (!in.read(0x105d9744, record, 4)) return 0;
        struct Field { std::uint32_t site; std::uint16_t offset, size; };
        constexpr Field first[]{{0x105d9788, 0x1ee, 0x33}, {0x105d97d4, 0x221, 0x1a}, {0x105d9820, 0x2c, 4},
            {0x105d986c, 0x30, 4}, {0x105d98b8, 0x34, 4}, {0x105d9904, 0x23c, 1}, {0x105d9950, 0x38, 4},
            {0x105d999c, 0x23d, 1}, {0x105d99e8, 4, 4}, {0x105d9a34, 0x3c, 4}, {0x105d9a80, 0x23e, 1},
            {0x105d9acc, 0x40, 4}, {0x105d9b18, 0x23f, 1}, {0x105d9b64, 8, 4}, {0x105d9bb0, 0xc, 4},
            {0x105d9bfc, 0x10, 4}, {0x105d9c48, 0x240, 1}};
        for (const auto& field : first)
            if (!in.read(field.site, record + field.offset, field.size, 2)) return 0;
        if (!in.read(0x105d9c94, byte_slot, 1)) return 0;
        // A byte on disk: zero means 1, then the field holds 500 times it.
        keep(c, {{31, record}});
        std::int16_t multiplier = std::int16_t(c.u8(byte_slot));
        c.put16(record + 0x1ec, std::uint16_t(multiplier));
        if (compare(c, multiplier) == 0) {
            multiplier = 1;
            c.put16(record + 0x1ec, 1);
        }
        c.set(3, extended(multiplier));
        c.put16(record + 0x1ec, std::uint32_t(multiplier) * 0x1f4);
        if (!in.read(0x105d9d04, record + 0x241, 1)) return 0;
        for (std::uint32_t i = 0; i < 16; ++i)
            if (!in.read(0x105d9d48 + i * 0x44, record + 0x44 + i * 4, 4)) return 0;
        // The arrays keep their index and byte offset in r21 and r22.
        const auto array = [&](std::uint32_t site, std::uint32_t at, std::uint32_t items) {
            keep(c, {{21, 0}, {22, 0}});
            for (std::uint32_t i = 0; i < items; ++i) {
                if (!in.read(site, record + at + i * 4, 4, 2)) return false;
                keep(c, {{21, i + 1}, {22, (i + 1) * 4}});
            }
            return true;
        };
        if (!array(0x105da190, 0x84, 3)) return 0;
        if (!in.read(0x105da1f0, record + 0x90, 4)) return 0;
        if (!in.read(0x105da234, record + 0x94, 4)) return 0;
        if (!array(0x105da280, 0x98, 50)) return 0;
        if (!array(0x105da2e8, 0x160, 5)) return 0;
        if (!array(0x105da350, 0x174, 7)) return 0;
        if (!array(0x105da3b8, 0x190, 3)) return 0;
        if (!in.read(0x105da418, record + 0x14, 4)) return 0;
        if (!in.read(0x105da45c, record + 0x243, 1)) return 0;
        if (!array(0x105da4a8, 0x19c, 20)) return 0;
        if (!array(0x105da510, 0x18, 4)) return 0;
        if (!in.read(0x105da570, record + 0x28, 4)) return 0;
    }
    check_end_marker(c, frame, file, packed, 0x105da5dc, 0x105da60c, 0x316);
    return result(c, 1);
}

std::uint32_t read_old_non_players(Cpu& c, std::uint32_t file, std::uint32_t packed, std::uint32_t records, std::uint32_t count) {
    Frame frame(c, 0x105db6c0, 0x60, {.first_gpr = 25});
    keep(c, {{30, file}, {29, packed}, {27, records}, {25, count}});
    const FieldReader in{c, file, packed};
    const auto byte_slot = frame.at(0x38);
    std::uint32_t index = 0, offset = 0;
    keep(c, {{26, index}, {28, offset}});
    for (; compare_logical(c, index, count) < 0; ++index, offset += 0x44, keep(c, {{26, index}, {28, offset}})) {
        const auto record = records + offset;
        if (!in.read(0x105db6f0, record, 4)) return 0;
        if (!in.read(0x105db738, record + 0x24, 2, 2)) return 0;
        if (!in.read(0x105db788, record + 0x26, 2, 2)) return 0;
        if (!in.read(0x105db7d8, byte_slot, 1)) return 0;
        keep(c, {{31, record}});
        in.store_byte_as_half(byte_slot, record + 0x28);
        if (!in.read(0x105db82c, byte_slot, 1)) return 0;
        in.store_byte_as_half(byte_slot, record + 0x2a);
        if (!in.read(0x105db878, byte_slot, 1)) return 0;
        in.store_byte_as_half(byte_slot, record + 0x2c);
        if (!in.read(0x105db8c8, record + 0x2e, 1)) return 0;
        for (std::uint32_t i = 0; i < 20; ++i)
            if (!in.read(0x105db910 + i * 0x48, record + 0x2f + i, 1)) return 0;
        for (std::uint32_t i = 0; i < 8; ++i)
            if (!in.read(0x105dbeb0 + i * 0x48, record + 4 + i * 4, 4)) return 0;
        if (!in.read(0x105dc0f0, record + 0x43, 1)) return 0;
    }
    return result(c, 1);
}

std::uint32_t read_old_players(Cpu& c, std::uint32_t file, std::uint32_t packed, std::uint32_t records, std::uint32_t count) {
    Frame frame(c, 0x105dc710, 0x60, {.first_gpr = 25});
    keep(c, {{30, file}, {29, packed}, {27, records}, {25, count}});
    const FieldReader in{c, file, packed};
    const auto byte_slot = frame.at(0x38);
    std::uint32_t index = 0, offset = 0;
    keep(c, {{26, index}, {28, offset}});
    for (; compare_logical(c, index, count) < 0; ++index, offset += 0x48, keep(c, {{26, index}, {28, offset}})) {
        const auto record = records + offset;
        if (!in.read(0x105dc740, record, 4)) return 0;
        if (!in.read(0x105dc788, record + 0xe, 1, 2)) return 0;
        if (!in.read(0x105dc7d8, record + 4, 2, 2)) return 0;
        if (!in.read(0x105dc828, record + 6, 2, 2)) return 0;
        if (!in.read(0x105dc878, byte_slot, 1)) return 0;
        keep(c, {{31, record}});
        in.store_byte_as_half(byte_slot, record + 8);
        if (!in.read(0x105dc8cc, byte_slot, 1)) return 0;
        in.store_byte_as_half(byte_slot, record + 0xa);
        if (!in.read(0x105dc918, byte_slot, 1)) return 0;
        in.store_byte_as_half(byte_slot, record + 0xc);
        if (!in.read(0x105dc968, record + 0xf, 1)) return 0;
        for (std::uint32_t i = 0; i < 54; ++i)
            if (!in.read(0x105dc9b0 + i * 0x48, record + 0x10 + i, 1)) return 0;
    }
    return result(c, 1);
}

std::uint32_t read_old_club_comps(Cpu& c, std::uint32_t file, std::uint32_t packed, std::uint32_t records, std::uint32_t count) {
    Frame frame(c, 0x105de550, 0x90, {.first_gpr = 24});
    keep(c, {{24, c.toc(-0x7ba8)}, {25, c.toc(pool_slot)}, {30, file}, {28, packed}, {29, records}, {26, count}});
    const FieldReader in{c, file, packed};
    const auto byte_slot = frame.at(0x38);
    std::uint32_t index = 0, offset = 0;
    keep(c, {{27, index}, {31, offset}});
    for (; compare_logical(c, index, count) < 0; ++index, offset += 0x6c, keep(c, {{27, index}, {31, offset}})) {
        const auto record = records + offset;
        if (!in.read(0x105de588, record, 4)) return 0;
        struct Field { std::uint32_t site; std::uint16_t offset, size; };
        constexpr Field fields[]{{0x105de5d0, 0x1a, 0x33}, {0x105de620, 0x4d, 0x1a}, {0x105de670, 0x14, 4},
            {0x105de6c0, 0x68, 1}, {0x105de710, 0x69, 1}, {0x105de760, 4, 4}, {0x105de7b0, 8, 4},
            {0x105de800, 0xc, 4}, {0x105de850, 0x10, 4}};
        for (const auto& field : fields)
            if (!in.read(field.site, record + field.offset, field.size, 2)) return 0;
        if (!in.read(0x105de8a0, byte_slot, 1)) return 0;
        in.store_byte_as_half(byte_slot, record + 0x18);
    }
    check_end_marker(c, frame, file, packed, 0x105de91c, 0x105de94c, 0x5e3);
    return result(c, 1);
}
}

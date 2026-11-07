// DATABASE.CPP's generation block, part 3: the copies between a person's attribute bytes and
// an array of slots (database_generation.hpp). A slot is four bytes: a 16-bit value (the
// attribute byte, sign-extended) and, in 0x100c5ab0, a flag byte at +2. The routines are leaves
// the compiler scheduled in registers; the registers they leave at return are the last values
// each held (derived from the instruction stream), set here after the copy.
//
// Precondition (the callers' use): the person record, the slot array and the optional output bytes do not
// overlap. The original interleaves its loads and stores; the copies here read each record byte before writing
// the slot or after, so an overlapping buffer would copy differently.
#include "database_generation.hpp"
#include "database_generation_detail.hpp"
#include <array>

namespace cm::aot::database {
using namespace generation;
namespace {
constexpr std::uint32_t slot_size = 4;
// The first two groups of attribute bytes (0x100c6150, 0x100c63c0): 13 then 8 offsets of the person record.
constexpr std::array<std::uint8_t, 13> first_group{0x2e, 0x2f, 0x32, 0x33, 0x34, 0x35, 0x36, 0x3a, 0x3c, 0x3d, 0x3f, 0x40, 0x42};
constexpr std::array<std::uint8_t, 8> second_group{0x31, 0x30, 0x37, 0x38, 0x39, 0x3b, 0x3e, 0x41};
// All 54 attribute bytes in slot order (0x100c5ab0, 0x100c65d0); slot 54 is the byte at +0x64 of another record.
constexpr std::array<std::uint8_t, 54> all_attributes{
    0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x1a, 0x17, 0x18, 0x19, 0x30, 0x3b, 0x1e, 0x23, 0x24, 0x26,
    0x27, 0x2a, 0x2b, 0x31, 0x32, 0x33, 0x35, 0x37, 0x38, 0x39, 0x3a, 0x3e, 0x41, 0x43, 0x28, 0x22, 0x29, 0x40,
    0x3d, 0x3c, 0x36, 0x2e, 0x2f, 0x25, 0x1f, 0x20, 0x21, 0x1b, 0x1c, 0x1d, 0x2c, 0x2d, 0x42, 0x3f, 0x44, 0x34};
constexpr std::uint32_t extra_slot = 54, extra_offset = 0x64;
// 0x100c5ab0 writes a flag byte before these slots' values: 2, and 1 for slot 19 (offset 0x2a).
constexpr std::array<std::uint8_t, 55> flags = [] {
    std::array<std::uint8_t, 55> f{};
    for (const unsigned i : {15, 17, 18, 20, 21, 22, 23, 25, 26, 29, 31, 32, 33, 35, 45, 51, 52}) f[i] = 2;
    f[19] = 1;
    return f;
}();

std::uint32_t slot_at(std::uint32_t array, unsigned i) { return array + i * slot_size; }
// What the lhax / lbz+extsb loads leave in a register.
std::uint32_t extsh(std::uint16_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }

// The first 0x100c63c0 instructions' stores below r1: the saved non-volatile registers of a leaf
// that has no frame (`stw r31,-4(r1)`), reloaded before the return.
void save_registers(Cpu& c, unsigned first) {
    for (unsigned r = first; r <= 31; ++r) c.put32(c.sp() - 4 * (32 - r), c.r(r));
}
void restore_registers(Cpu& c, unsigned first) {
    for (unsigned r = first; r <= 31; ++r) c.set(r, c.u32(c.sp() - 4 * (32 - r)));
}
}

// 0x100c6150: attribute bytes of a person into slots 0-20 (first group, then second group);
// the number of the first group (13) is stored in `count_out` between them when it is not null.
void copy_attributes_to_slots(Cpu& c, std::uint32_t person, std::uint32_t array, std::uint32_t count_out) {
    save_registers(c, 31);
    compare_logical(c, count_out);
    unsigned i = 0;
    for (const auto offset : first_group) c.put16(slot_at(array, i++), extsb(c.u8(person + offset)));
    if (count_out != 0) c.put8(count_out, 13);
    for (const auto offset : second_group) c.put16(slot_at(array, i++), extsb(c.u8(person + offset)));
    // The last values of the volatile registers (each is a different schedule slot of the original).
    c.set({{0, 0x50}, {4, 0x4c}, {7, 0x48}, {8, 0x44}});
    c.set(3, extsb(c.u8(person + 0x41)));
    c.set(6, extsb(c.u8(person + 0x3e)));
    c.set(9, c.u8(person + 0x39));
    c.set(10, extsb(c.u8(person + 0x38)));
    c.set(11, extsb(c.u8(person + 0x3c)));
    c.set(12, extsb(c.u8(person + 0x3a)));
    restore_registers(c, 31);
}

// 0x100c63c0: the same attribute bytes loaded back from the slots' values (the low byte of each).
void copy_attributes_from_slots(Cpu& c, std::uint32_t person, std::uint32_t array) {
    save_registers(c, 31);
    unsigned i = 0;
    for (const auto offset : first_group) c.put8(person + offset, c.u16(slot_at(array, i++)));
    for (const auto offset : second_group) c.put8(person + offset, c.u16(slot_at(array, i++)));
    c.set(0, extsh(c.u16(array + 80)));
    c.set(4, extsh(c.u16(array + 76)));
    c.set(6, extsh(c.u16(array + 72)));
    c.set(7, 0x48);
    c.set(8, extsh(c.u16(array + 68)));
    c.set(9, extsh(c.u16(array + 64)));
    c.set(10, extsh(c.u16(array + 36)));
    c.set(11, extsh(c.u16(array + 32)));
    c.set(12, extsh(c.u16(array + 28)));
    restore_registers(c, 31);
}

// 0x100c65d0: all 54 attribute bytes of a person, and byte +0x64 of `other`, from the slots.
void copy_all_attributes_from_slots(Cpu& c, std::uint32_t person, std::uint32_t other, std::uint32_t array) {
    save_registers(c, 30);
    unsigned i = 0;
    for (const auto offset : all_attributes) c.put8(person + offset, c.u16(slot_at(array, i++)));
    c.put8(other + extra_offset, c.u16(slot_at(array, extra_slot)));
    c.set(0, extsh(c.u16(array + 216)));
    c.set(6, extsh(c.u16(array + 212)));
    c.set({{7, 0xd4}, {8, 0xd0}, {9, 0xcc}, {10, 0xc8}});
    c.set(11, extsh(c.u16(array + 196)));
    c.set(12, extsh(c.u16(array + 152)));
    restore_registers(c, 30);
}

// 0x100c5ab0: all 54 attribute bytes of a person, and byte +0x64 of `other`, into the slots, with
// flag bytes at +2 of some (every slot's flag byte is cleared first, 63 of them). Three optional
// counts store the number of the slots written so far: 9 (`after_nine`), 12 (`after_twelve`) and 32
// (`after_thirty_two`).
void copy_all_attributes_to_slots(Cpu& c, std::uint32_t person, std::uint32_t other, std::uint32_t array,
                                  std::uint32_t after_twelve, std::uint32_t after_nine,
                                  std::uint32_t after_thirty_two) {
    save_registers(c, 28);
    for (unsigned i = 0; i < 63; ++i) c.put8(slot_at(array, i) + 2, 0);
    const auto write = [&](unsigned i, std::uint32_t source) {
        if (flags[i]) c.put8(slot_at(array, i) + 2, flags[i]);
        c.put16(slot_at(array, i), extsb(c.u8(source)));
    };
    for (unsigned i = 0; i < all_attributes.size(); ++i) {
        write(i, person + all_attributes[i]);
        if (i == 8 && after_nine != 0) c.put8(after_nine, 9);
        if (i == 11 && after_twelve != 0) c.put8(after_twelve, 12);
        if (i == 31 && after_thirty_two != 0) c.put8(after_thirty_two, 32);
    }
    write(extra_slot, other + extra_offset);
    compare_logical(c, after_thirty_two);
    c.set({{0, 0xd8}, {6, 0xd4}, {7, 0xd0}, {10, 2}, {11, 0xcc}});
    c.set(3, extsb(c.u8(other + extra_offset)));
    c.set(8, extsb(c.u8(person + 0x44)));
    c.set(9, extsb(c.u8(person + 0x3f)));
    c.set(12, array + 204);
    restore_registers(c, 28);
}
}

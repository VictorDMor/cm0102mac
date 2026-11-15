// DISPLAY.CPP block "table": see display_table.hpp.
#include "display_table.hpp"
#include <bit>

namespace cm::aot::display {
namespace {
using u32 = std::uint32_t;

namespace f {
inline constexpr std::int32_t buffer = -0x56dc, buffer2 = -0x56d8;
}
// The tail of the shared setup buffer: 0x70000 - 0xabc bytes per record.
inline constexpr u32 setup_record = 0x70000 - 0xabc;

// Destroys the table's areas and GUI objects (the original's two loops: r25 the counter, r26 the byte
// offset, r27 the field base). The counts are re-read at every test, as the original does.
void destroy_children(Cpu& c, CallRegisters& regs, u32 table, u32 area_back, u32 guio_back) {
    u32 i = 0, offset = 0;
    regs.hold({{25, i}, {26, offset}, {27, field(table, 0)}});
    while (half(i) < c.i16(field(table, -0x625c))) {
        regs.hold({{25, i}, {26, offset}});
        regs.apply();
        invoke(c, 0x10714440, area_back, {table + (offset + 4)});
        offset += table::area_size;
        ++i;
    }
    regs.hold({{25, i}, {26, offset}});
    i = 0, offset = 0;
    regs.hold({{25, i}, {26, offset}});
    while (half(i) < c.i16(field(table, -0x625a))) {
        regs.hold({{25, i}, {26, offset}});
        regs.apply();
        invoke(c, 0x10726a50, guio_back, {table + (table::guios + offset)});
        offset += table::guio_size;
        ++i;
    }
    regs.hold({{25, i}, {26, offset}});
}
}

void reset_table(Cpu& c, u32 table, u32 flag) {
    c.put32(field(table, -0x56d8), 0);
    c.put32(field(table, -0x56dc), 0);
    for (const auto address : {table::reply_wanted, table::screen_set, table::high - 0x56c2, table::high - 0x56c8, table::high - 0x56c0}) c.put8(table + address, 0);
    c.put8(field(table, -0x56c1), flag);
    c.put8(table, 0);
    c.put8(field(table, -0x56ba), 1);
    c.put32(table + table::string_list, 0);
    for (const auto address : {table::area_count, table::guio_count, table::announced_guio_count, table::announced_area_count}) c.put16(table + address, 0);
    c.put16(table + table::height_flag, 0xffff);
    c.put8(table + table::width_flag, 0);
    c.put16(field(table, -0x56fc), 0xffff);
    c.put16(field(table, -0x56fa), 0xffff);
}

std::uint32_t destroy_table(Cpu& c, u32 table, u32 flag) {
    const Frame frame(c, 0x10718ba0, 0x60, {.first_gpr = 25});
    CallRegisters regs(c);
    const auto count_variable = c.toc(-0x5bf0), pointer_variable = c.toc(-0x5bec);
    regs.hold({{28, table}, {29, flag}, {30, count_variable}, {31, pointer_variable}});
    if (table != 0) {
        regs.hold({{27, field(table, 0)}});
        if (c.u32(table + table::string_list) != 0) {
            u32 i = 0, offset = 0, list;
            regs.hold({{25, i}, {26, offset}});
            for (;;) {
                list = c.u32(table + table::string_list);
                if (half(i) >= c.i32(list + 4)) break;
                regs.hold({{25, i}, {26, offset}});
                regs.apply();
                invoke(c, 0x10775170, 0x10718bec, {c.u32(c.u32(list) + offset)});
                offset += 4;
                ++i;
            }
            regs.hold({{25, i}, {26, offset}});
            regs.apply();
            invoke(c, 0x10775170, 0x10718c10, {c.u32(list)});
            c.put32(table + table::string_list, 0);
        }
        regs.apply();
        c.set(3, table);
        c.link(0x10718c24);
        free_setup_buffer(c);
        destroy_children(c, regs, table, 0x10718c40, 0x10718c78);
        c.put16(field(table, -0x625c), 0);
        c.put16(field(table, -0x625a), 0);
        // The original frees -0x56dc twice, re-reading the field after clearing it (a callee may
        // have stored a new pointer there).
        if (c.u32(field(table, f::buffer)) != 0) {
            regs.apply();
            invoke(c, 0x1076b7e0, 0x10718cac, {c.u32(field(table, f::buffer))});
            c.put32(field(table, f::buffer), 0);
        }
        if (c.u32(field(table, f::buffer)) != 0) {
            regs.apply();
            invoke(c, 0x1076b7e0, 0x10718ccc, {c.u32(field(table, f::buffer))});
            c.put32(field(table, f::buffer), 0);
        }
        if (c.u32(field(table, f::buffer2)) != 0) {
            regs.apply();
            invoke(c, 0x1076b7e0, 0x10718cec, {c.u32(field(table, f::buffer2))});
            c.put32(field(table, f::buffer2), 0);
        }
        if (const auto pending = c.u32(pointer_variable); pending != 0 && c.i16(count_variable) <= 0) {
            regs.apply();
            invoke(c, 0x1076b7e0, 0x10718d14, {pending});
            c.put32(pointer_variable, 0);
            c.put16(count_variable, 0);
        }
        regs.apply();
        invoke(c, 0x10773420, 0x10718d38, {guio_at(table, 0), c.toc(-0x5bf4), table::guio_size, 0x4b0});
        invoke(c, 0x10773420, 0x10718d4c, {table + table::areas, c.toc(-0x5bf8), table::area_size, 0xfa});
        if (half(flag) > 0) invoke(c, 0x107735a0, 0x10718d5c, {table});
    }
    c.set(3, table);
    return table;
}

std::uint32_t destroy_area(Cpu& c, u32 object, u32 flag) {
    const Frame frame(c, 0x10718d80, 0x40, {.first_gpr = 30});
    c.set({{30, object}, {31, flag}});
    if (object != 0) {
        invoke(c, 0x10714440, 0x10718da4, {object});
        if (half(flag) > 0) invoke(c, 0x107735a0, 0x10718db4, {object});
    }
    c.set(3, object);
    return object;
}

std::uint32_t destroy_gui_object(Cpu& c, u32 object, u32 flag) {
    const Frame frame(c, 0x10718dd0, 0x40, {.first_gpr = 30});
    c.set({{30, object}, {31, flag}});
    if (object != 0) {
        invoke(c, 0x10726a50, 0x10718df4, {object});
        if (half(flag) > 0) invoke(c, 0x107735a0, 0x10718e04, {object});
    }
    c.set(3, object);
    return object;
}

void allocate_setup_buffer(Cpu& c, u32 count) {
    const Frame frame(c, 0x10718e20, 0x70, {.first_gpr = 29});
    const auto variable = c.toc(-0x5bfc), pool = c.toc(toc::pool), message = c.toc(toc::message);
    c.set({{29, message}, {30, pool}, {31, variable}});
    if (c.u32(variable) == 0) {
        const auto block = invoke(c, 0x10775200, 0x10718e60, {setup_record, u32(half(count))});
        c.put32(variable, block);
        if (block == 0) {
            report(c, frame, {0x10718e80, 0x40, 0x38, 0xc0});
            c.set(4, c.toc(toc::success));
            invoke(c, 0x1076f7d0, 0x10718ec8, {0xffffffffu});
        }
    }
}

void free_setup_buffer(Cpu& c) {
    const Frame frame(c, 0x10718ef0, 0x40, {.first_gpr = 31});
    const auto variable = c.toc(-0x5bfc);
    c.set(31, variable);
    if (const auto block = c.u32(variable); block != 0) {
        invoke(c, 0x10775170, 0x10718f14, {block});
        c.put32(variable, 0);
    }
}

// Callers read r3-r10 after these leaves and the two index appends (tools/register_contract.py --after):
// each leaves r3/r5/r6 as the original does.
void clear_setup_record(Cpu& c, u32 index) {
    const auto cell = c.toc(-0x5bfc), base = c.u32(cell);
    c.set({{3, cell}, {5, base}});
    if (base == 0) return;
    const auto record = base + 0x70000 + index * setup_record;
    c.put16(record - 0xac0, 0);
    c.set({{3, record}, {5, base + 0x70000}, {6, 0}});
}

void corner_anchor(Cpu& c, u32 corner, u32 value) {
    const auto bounds = c.u32(c.toc(-0x5c2c));
    if (bounds == 0) return;
    const auto output_word = c.toc(-0x5c20), anchor_y = c.toc(-0x5c14), anchor_x = c.toc(-0x5c10), success = c.toc(toc::success);
    c.put8(success, 1);
    c.put32(output_word, value);
    // The midpoint of the bounds' +0 and +8 words, rounded toward zero (srawi after adding the sign bit).
    const auto midpoint = [&] {
        const u32 sum = c.u32(bounds + 8) + c.u32(bounds);
        return u32(std::int32_t((sum >> 31) + sum) >> 1);
    };
    switch (corner & 0xff) {
    case 0: c.put32(anchor_x, midpoint()); c.put32(anchor_y, c.u32(bounds + 0x14) + 2); break;
    case 1: c.put32(anchor_x, midpoint()); c.put32(anchor_y, c.u32(bounds + 4) + 1); break;
    case 2: c.put32(anchor_x, midpoint()); c.put32(anchor_y, c.u32(bounds + 0x28) + 2); break;
    case 3: c.put32(anchor_x, midpoint()); c.put32(anchor_y, c.u32(bounds + 0x10) + 1); break;
    default: c.put8(success, 0); c.put32(output_word, 0); break;
    }
}

void initialize_unit(Cpu& c) {
    const Frame frame(c, 0x107259a0, 0x40, {.first_gpr = 31});
    const auto handler = c.toc(-0x5c28);
    c.set(31, handler);
    auto object = invoke(c, 0x1073ae10, 0x107259c4, {c.toc(-0x5c04), 0xc350});
    invoke(c, 0x10772df0, 0x107259d0, {object, handler, c.toc(-0x3f98)});
    object = invoke(c, 0x1073ae10, 0x107259dc, {c.toc(-0x5c08), 0xc00});
    invoke(c, 0x10772df0, 0x107259e8, {object, handler, c.toc(-0x3f9c)});
}

void clear_table(Cpu& c, u32 table, u32 destroy_children_flag) {
    const Frame frame(c, 0x10719560, 0x80, {.first_gpr = 25});
    CallRegisters regs(c);
    const auto stream = c.toc(-0x5c08), message = c.toc(toc::message), pool = c.toc(toc::pool);
    regs.hold({{28, table}, {26, 0}, {27, field(table, 0)}, {29, stream}, {30, message}, {31, pool}});
    c.put16(table + table::height_flag, 0xffff);
    c.put8(table + table::width_flag, 0);
    if (c.i8(table + table::screen_set) == 0 && std::int8_t(destroy_children_flag) != 0) destroy_children(c, regs, table, 0x107195bc, 0x107195f4);
    // Rewind the network stream and write the command byte 2 (an overflowing write reports).
    c.put32(stream + 4, 0);
    const auto other = c.toc(-0x5c04);
    const auto position = c.u32(stream + 4), capacity = c.u32(stream + 8);
    c.put32(other + 4, 0);
    if (position + 1 > capacity) {
        regs.apply();
        report(c, frame, {0x10719648, 0x40, 0x38, 0x85, text::network_header});
    } else {
        c.put8(c.u32(stream) + position, 2);
        c.put32(stream + 4, c.u32(stream + 4) + 1);
        c.set({{5, 0}, {6, stream + 4}, {7, position}});  // what the original leaves there (0x10719610..0x1071961c)
    }
    c.set(4, 0);
    c.put8(table, 0);
    for (const auto address : {table::high - 0x5704, table::guio_limit, table::guio_count, table::area_count}) c.put16(table + address, 0);
    for (const auto displacement : {-0x56d0, -0x56ce, -0x56cc}) c.put16(field(table, displacement), 0xffff);
    c.put8(field(table, -0x56c0), 0);
    for (const auto displacement : {-0x56f4, -0x56f0, -0x56ec, -0x56e8, -0x56e4, -0x56e0}) c.put32(field(table, displacement), 0);
    c.put8(field(table, -0x56bf), 0);
    c.put8(field(table, -0x56be), 0);
    c.put16(field(table, -0x56ca), 0xffff);
    for (const auto address : {table::unsent, table::screen_set, table::high - 0x5700}) c.put8(table + address, 0);
    if (const auto buffer = c.u32(field(table, f::buffer)); buffer != 0) {
        regs.apply();
        invoke(c, 0x1076b7e0, 0x10719714, {buffer});
        c.put32(field(table, f::buffer), 0);
    }
    c.put8(field(table, -0x56c0), 0);
    c.set(3, table + table::high);
}

void add_control_index(Cpu& c, u32 table, u32 index) {
    const Frame frame(c, 0x1071a100, 0x70, {.first_gpr = 30});
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message);
    c.set({{30, message}, {31, pool}});
    if (half(index) == -1) return;
    const auto count = c.i16(table + table::guio_limit);
    c.set({{5, u32(count)}, {6, table + table::high}});
    if (count >= 0x4b0) {
        report(c, frame, {0x1071a14c, 0x40, 0x38, 0x2e5});
        c.set(3, c.toc(toc::success));
    } else {
        c.put16(table + table::guio_limit, u32(count + 1));
        c.put16(table + table::index_list + u32(count) * 2, index);
        c.set(3, table + table::high + u32(count) * 2);
    }
}

void add_area_index(Cpu& c, u32 table, u32 index) {
    const Frame frame(c, 0x1071a1c0, 0x70, {.first_gpr = 30});
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message);
    c.set({{30, message}, {31, pool}});
    if (half(index) == -1) return;
    const auto count = c.i16(field(table, -0x5704));
    c.set({{5, u32(count)}, {6, table + table::high}});
    if (count >= 0xfa) {
        report(c, frame, {0x1071a20c, 0x40, 0x38, 0x2fd});
        c.set(3, c.toc(toc::success));
    } else {
        c.put16(field(table, -0x5704), u32(count + 1));
        c.put16(field(table, -0x6258) + u32(count) * 2, index);
        c.set(3, table + table::high + u32(count) * 2);
    }
}
}

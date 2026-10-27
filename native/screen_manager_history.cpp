#include "screen_manager_detail.hpp"

namespace cm::aot::screen_manager {
using namespace detail;

std::uint32_t broadcast_history(Cpu& c, std::uint32_t address, std::uint32_t filter,
    std::uint32_t key, std::uint32_t value, std::uint32_t active,
    std::uint32_t callback, std::uint32_t context) {
    Frame frame(c, 0x1073e350, 0x60, {.first_gpr = 22});
    Kept<23> manager_address(c, address);
    Kept<24> selected(c, filter);
    Kept<25> history_key(c, key);
    Kept<26> history_value(c, value);
    Kept<27> history_active(c, active);
    Kept<28> history_callback(c, callback);
    Kept<29> history_context(c, context);
    Manager manager{c, address};
    Kept<30> previous_group(c, extended(manager.current()));
    manager.current() = 0;
    auto append = [&](std::uint32_t back) {
        enter(c, back, {manager_address, history_key, history_value, history_active, history_callback, history_context});
        result(c, append_history(c, manager_address, history_key, history_value, history_active, history_callback, history_context));
    };
    while (true) {
        c.set(4, extended(manager.current()));
        if (compare(c, c.r(4), 16) >= 0) break;
        const auto count = manager.count(std::int32_t(c.r(4))).get();
        if (compare(c, count) != 0) {
            const auto matches = [&] {
                if (compare_logical(c, selected) == 0) return true;
                return compare_logical(c, selected, c.u32(manager.group() + 4)) == 0;
            };
            if (matches()) append(0x1073e3d8);
            Kept<31> row(c, 1);
            Kept<22> offset(c, 0x2c);
            for (;;) {
                const auto group = extended(manager.current());
                c.set({{5, group}, {4, extended(std::int16_t(std::uint32_t(row)))}, {3, group * 2}});
                const auto rows = manager.count(std::int32_t(group)).get();
                if (compare(c, c.r(4), rows) >= 0) {
                    if (compare(c, rows, 1) > 0)
                        Program{c}.call(0x10745e00, 0x1073e474, {manager.group(), manager.group() + 0x2c, 0x2c});
                    break;
                }
                Program{c}.call(0x10745e00, 0x1073e3f8, {manager.group(), manager.group() + offset, 0x2c});
                if (matches()) append(0x1073e434);
                offset = offset + 0x2c;
                row = row + 1;
            }
        }
        manager.current() = std::int16_t(manager.current().get() + 1);
    }
    manager.current() = std::int16_t(std::uint32_t(previous_group));
    return 1;
}

std::uint32_t append_history(Cpu& c, std::uint32_t address, std::uint32_t key,
    std::uint32_t value, std::uint32_t active, std::uint32_t callback, std::uint32_t context) {
    Frame frame(c, 0x1073e4b0, 0x90, {.first_gpr = 21});
    Kept<23> history_key(c, key);
    Kept<22> manager_address(c, address);
    keep(c, {{29, c.toc(slot::message)}, {30, c.toc(slot::pool)}});
    Kept<24> history_value(c, value);
    Kept<25> history_active(c, active);
    Kept<26> history_callback(c, callback);
    Kept<27> history_context(c, context);
    Kept<28> defaults_used(c, 0);
    Manager manager{c, address};
    Program program{c};
    auto group = manager.group();
    const auto offset = group - address;
    if (compare_logical(c, c.u32(group + 4)) == 0 && compare_logical(c, c.u32(group + 8)) == 0 &&
        compare_logical(c, c.u32(address + 0x2c00)) != 0) {
        history_value = c.u32(address + 0x2c04);
        history_key = c.u32(address + 0x2c00);
        history_active = c.u8(address + 0x2c08);
        defaults_used = 1;
        history_callback = c.u32(address + 0x2c0c);
        history_context = c.u32(address + 0x2c10);
    }
    if (compare_logical(c, history_key) == 0 || compare_logical(c, history_value) == 0) {
        assertion(c, frame, 0x1073e550, 0x40, 0x38, 0x4eb);
        c.set(4, c.toc(slot::success));
        return 0;
    }
    c.set({{4, offset}, {3, group}, {5, c.u32(group + 0x10)}});
    if (compare_logical(c, c.r(5)) != 0 && byte_nonzero(c, history_active)) {
        c.set(6, c.u32(group + 8));
        while (compare_logical(c, c.r(6)) != 0) {
            const auto node = c.r(6);
            if (byte_nonzero(c, c.u8(node + 0x10)) && compare_logical(c, c.u32(node), history_key) == 0) {
                while (compare_logical(c, c.r(6)) != 0) {
                    if (byte_nonzero(c, c.u8(c.r(6) + 0x10))) manager.history() = c.r(6);
                    c.set(6, c.u32(c.r(6) + 0x1f8));
                }
                c.set({{4, manager.group()}, {3, manager.history()}});
                if (compare_logical(c, c.u32(c.r(3)), history_key) != 0) manager.history() = c.r(5);
                return 0;
            }
            c.set(6, c.u32(node + 0x1f8));
        }
    }
    group = address + c.r(4);
    c.set(3, group);
    if (compare_logical(c, c.r(5), c.u32(group + 0xc)) != 0) {
        c.put32(group + 0xc, c.r(5));
        c.set(3, manager.group());
        c.set(4, c.u32(c.r(3) + 0x10));
        c.set(3, c.u32(c.r(3) + 0xc));
        Kept<31> node(c, c.u32(c.r(4) + 0x1f8));
        c.put32(c.r(3) + 0x1f8, 0);
        while (compare_logical(c, node) != 0) {
            Kept<29> index(c, 0);
            c.set(4, c.u32(node + 0x1f8));
            Kept<21> field_offset(c, 0);
            manager.history() = c.r(4);
            while (compare(c, std::int16_t(std::uint32_t(index)), 60) < 0) {
                c.set(4, node + field_offset);
                c.set(3, c.u32(c.r(4) + 0x14));
                if (compare_logical(c, c.r(3)) != 0 && byte_nonzero(c, c.u8(c.r(4) + 0x18)))
                    program.call(0x10773610, 0x1073e6b0);
                field_offset = field_offset + 8;
                index = index + 1;
            }
            program.call(0x107735a0, 0x1073e6cc, {node});
            const auto count_address = manager.group() + 0x16;
            c.set({{4, count_address - address}, {3, extended(c.i16(count_address))}});
            c.put16(count_address, c.r(3) - 1);
            node = manager.history();
        }
        c.set(3, manager.group());
        manager.history() = c.u32(c.r(3) + 0xc);
    }
    c.set(3, manager.group());
    if (compare(c, c.i16(c.r(3) + 0x16), 100) > 0) {
        Kept<31> node(c, c.u32(c.r(3) + 8));
        if (byte_nonzero(c, c.u8(node + 0x10))) return 0;
        Kept<29> index(c, 0);
        Kept<21> field_offset(c, 0);
        c.put32(c.r(3) + 8, c.u32(node + 0x1f8));
        c.set(3, c.u32(manager.group() + 8));
        c.put32(c.r(3) + 0x1f4, 0);
        while (compare(c, std::int16_t(std::uint32_t(index)), 60) < 0) {
            c.set(4, node + field_offset);
            c.set(3, c.u32(c.r(4) + 0x14));
            if (compare_logical(c, c.r(3)) != 0 && byte_nonzero(c, c.u8(c.r(4) + 0x18)))
                program.call(0x10773610, 0x1073e788);
            field_offset = field_offset + 8;
            index = index + 1;
        }
        program.call(0x107735a0, 0x1073e7a4, {node});
        c.set(4, manager.group());
        c.set(3, extended(c.i16(c.r(4) + 0x16)));
        c.put16(c.r(4) + 0x16, c.r(3) - 1);
    }
    c.set(3, manager.history());
    if (compare_logical(c, c.r(3)) != 0) {
        const auto target = c.u32(c.r(3) + 4);
        if (compare_logical(c, target) != 0) program.callback(target, 0x1073e7e4);
    }
    Kept<31> node(c, program.call(0x10773520, 0x1073e7f0, {0x2fc}));
    program.call(0x10778300, 0x1073e800, {node, 0, 0x2fc});
    c.put8(node + 0x10, history_active);
    c.put32(node, history_key);
    c.put32(node + 4, history_callback);
    c.put32(node + 0xc, history_context);
    c.put32(node + 8, history_value);
    program.call(0x1077dbf0, 0x1073e820, {node + 0x1fc, c.r(30) + 0xb4});
    if (byte_nonzero(c, history_active)) {
        const auto count_address = manager.group() + 0x14;
        c.put16(count_address, c.i16(count_address) + 1);
    }
    c.set(3, c.u32(manager.group() + 0xc));
    if (compare_logical(c, c.r(3)) != 0) {
        c.put32(c.r(3) + 0x1f8, node);
        c.put32(node + 0x1f4, c.u32(manager.group() + 0xc));
    }
    c.put32(manager.group() + 0xc, node);
    c.set(3, manager.group());
    if (compare_logical(c, c.u32(c.r(3) + 8)) == 0) c.put32(c.r(3) + 8, node);
    c.set({{5, 0}, {4, missing}});
    const bool is_active = byte_nonzero(c, history_active);
    manager.history() = node;
    c.set(6, manager.group());
    c.set(3, extended(c.i16(c.r(6) + 0x16)));
    c.put16(c.r(6) + 0x16, c.r(3) + 1);
    c.put8(manager.group() + 0x19, c.r(5));
    c.put16(manager.group() + 0x1c, c.r(4));
    c.set(3, manager.group());
    c.put32(c.r(3) + 0x20, c.r(5));
    if (is_active) c.put8(manager.group() + 0x1a, c.r(5));
    c.set(3, extended(manager.current()));
    const bool used_defaults = byte_nonzero(c, defaults_used);
    c.set(4, 1);
    c.set(3, manager.group());
    c.put8(c.r(3) + 0x18, c.r(4));
    if (used_defaults) {
        Kept<23> index(c, 0);
        Kept<21> field_offset(c, 0);
        for (;;) {
            c.set(3, extended(std::int16_t(std::uint32_t(index))));
            if (compare(c, c.r(3), c.i16(address + 0x2c18)) >= 0) break;
            const auto values = c.u32(address + 0x2c14);
            enter(c, 0x1073e954, {manager_address, extended(std::int16_t(std::uint32_t(index))), c.u32(values + field_offset), 0});
            result(c, set_history_value(c, manager_address, c.r(4), c.r(5), 0));
            field_offset = field_offset + 4;
            index = index + 1;
        }
    }
    return 1;
}
}

namespace cm::aot::screen_manager {
std::uint32_t history_title(Cpu& c, std::uint32_t address) {
    const auto node = Manager{c, address}.history().get();
    c.set(3, node);
    return compare_logical(c, node) == 0 ? c.toc(slot::pool) + 0xb6 : node + 0x1fc;
}
std::uint32_t set_history_title(Cpu& c, std::uint32_t address, std::uint32_t title) {
    Frame frame(c, 0x1073ede0, 0x40, {});
    c.set(3, Manager{c, address}.history());
    if (compare_logical(c, c.r(3)) != 0) {
        if (compare_logical(c, title) != 0)
            Program{c}.call(0x1077dbf0, 0x1073ee14, {c.r(3) + 0x1fc, title});
        else c.put8(c.r(3) + 0x1fc, 0);
    }
    return c.r(3);
}
std::uint32_t clear_history(Cpu& c, std::uint32_t address) {
    Frame frame(c, 0x1073ee30, 0x50, {.first_gpr = 28});
    Kept<31> manager_address(c, address);
    Manager manager{c, address};
    c.set(3, manager.group());
    Kept<28> node(c, c.u32(c.r(3) + 8));
    while (compare_logical(c, node) != 0) {
        Kept<29> index(c, 0);
        c.set(4, c.u32(node + 0x1f8));
        Kept<30> offset(c, 0);
        manager.history() = c.r(4);
        while (compare(c, std::int16_t(std::uint32_t(index)), 60) < 0) {
            c.set(4, node + offset);
            c.set(3, c.u32(c.r(4) + 0x14));
            if (compare_logical(c, c.r(3)) != 0 && byte_nonzero(c, c.u8(c.r(4) + 0x18)))
                Program{c}.call(0x10773610, 0x1073eea8);
            offset = offset + 8;
            index = index + 1;
        }
        Program{c}.call(0x107735a0, 0x1073eec4, {node});
        const auto count = manager.group() + 0x16;
        c.set({{4, count - manager_address}, {3, extended(c.i16(count))}});
        c.put16(count, c.r(3) - 1);
        node = manager.history();
    }
    c.set(4, 0);
    for (unsigned offset : {8, 12, 16, 22, 20}) {
        c.set(3, manager.group());
        if (offset >= 20) c.put16(c.r(3) + offset, 0);
        else c.put32(c.r(3) + offset, 0);
    }
    return c.r(3);
}
std::uint32_t history_value(Cpu& c, std::uint32_t address, std::uint32_t index) {
    Frame frame(c, 0x1073ef70, 0xa0, {.first_gpr = 27});
    keep(c, {{27, address}, {29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {28, index}, {31, c.toc(slot::pool)}});
    Manager manager{c, address};
    if (compare_logical(c, manager.history()) == 0) {
        assertion(c, frame, 0x1073efc0, 0x68, 0x40, 0x687);
        Program{c}.call(0x1076f7d0, 0x1073f004, {missing});
    }
    if (compare(c, index) < 0 || compare(c, index, 59) > 0) {
        assertion(c, frame, 0x1073f028, 0x48, 0x38, 0x68d);
        return 0;
    }
    return c.u32(manager.history() + index * 8 + 0x14);
}
std::uint32_t set_history_value(Cpu& c, std::uint32_t address, std::uint32_t index,
    std::uint32_t value, std::uint32_t copy_string) {
    Frame frame(c, 0x1073f1e0, 0xd0, {.first_gpr = 25});
    Kept<25> manager_address(c, address);
    keep(c, {{29, c.toc(slot::success)}, {30, c.toc(slot::message)}});
    Kept<28> offset(c, index);
    keep(c, {{31, c.toc(slot::pool)}});
    Kept<26> source(c, value);
    Kept<27> length(c, copy_string);
    Manager manager{c, address};
    Program program{c};
    if (compare_logical(c, manager.history()) == 0) {
        assertion(c, frame, 0x1073f238, 0x90, 0x48, 0x6d8);
        program.call(0x1076f7d0, 0x1073f27c, {missing});
    }
    if (compare(c, offset) < 0 || compare(c, offset, 59) > 0) {
        assertion(c, frame, 0x1073f2a0, 0x70, 0x40, 0x6de);
        return c.r(3);
    }
    offset = offset * 8;
    auto field = [&] { return manager.history().get() + offset; };
    c.set(3, field());
    if (byte_nonzero(c, c.u8(c.r(3) + 0x18))) {
        program.call(0x10773610, 0x1073f30c, {c.u32(c.r(3) + 0x14)});
        c.set({{4, 0}, {3, field()}});
        c.put8(c.r(3) + 0x18, 0);
    }
    if (compare_logical(c, source) != 0 && byte_nonzero(c, length)) {
        length = program.call(0x1077dbd0, 0x1073f340, {source}) + 1;
        const auto allocation = program.call(0x107735f0, 0x1073f34c, {length});
        c.set(4, field());
        c.put32(c.r(4) + 0x14, allocation);
        c.set(3, c.u32(field() + 0x14));
        if (compare_logical(c, c.r(3)) == 0) {
            assertion(c, frame, 0x1073f398, 0x50, 0x38, 0x6f5);
            return c.r(3);
        }
        program.call(0x10739ef0, 0x1073f3e4, {c.r(3), source, length});
        c.set({{4, 1}, {3, field()}});
        c.put8(c.r(3) + 0x18, 1);
    } else {
        c.set({{4, 0}, {3, field()}});
        c.put32(c.r(3) + 0x14, source);
        c.set(3, field());
        c.put8(c.r(3) + 0x18, 0);
    }
    return c.r(3);
}
std::uint32_t broadcast_history_value(Cpu& c, std::uint32_t address, std::uint32_t filter,
    std::uint32_t index, std::uint32_t value, std::uint32_t copy_string) {
    Frame frame(c, 0x1073f0a0, 0x60, {.first_gpr = 24});
    Kept<26> manager_address(c, address);
    Kept<27> selected(c, filter);
    Kept<28> field(c, index);
    Kept<29> source(c, value);
    Kept<30> copy(c, copy_string);
    Manager manager{c, address};
    Kept<31> previous_group(c, extended(manager.current()));
    manager.current() = 0;
    auto set_value = [&](std::uint32_t back) {
        enter(c, back, {manager_address, field, source, copy});
        result(c, set_history_value(c, manager_address, field, source, copy));
    };
    for (;;) {
        c.set(4, extended(manager.current()));
        if (compare(c, c.r(4), 16) >= 0) break;
        if (compare(c, manager.count(std::int32_t(c.r(4)))) != 0) {
            const auto matches = [&] {
                return compare_logical(c, selected) == 0 || compare_logical(c, selected, c.u32(manager.group() + 4)) == 0;
            };
            if (matches()) set_value(0x1073f118);
            Kept<24> row(c, 1);
            Kept<25> offset(c, 0x2c);
            for (;;) {
                const auto group = extended(manager.current());
                c.set({{5, group}, {4, extended(std::int16_t(std::uint32_t(row)))}, {3, group * 2}});
                const auto count = manager.count(std::int32_t(group)).get();
                if (compare(c, c.r(4), count) >= 0) {
                    if (compare(c, count, 1) > 0)
                        Program{c}.call(0x10745e00, 0x1073f1ac, {manager.group(), manager.group() + 0x2c, 0x2c});
                    break;
                }
                Program{c}.call(0x10745e00, 0x1073f138, {manager.group(), manager.group() + offset, 0x2c});
                if (matches()) set_value(0x1073f16c);
                offset = offset + 0x2c;
                row = row + 1;
            }
        }
        c.set(3, extended(manager.current()));
        manager.current() = std::int16_t(c.r(3) + 1);
    }
    manager.current() = std::int16_t(std::uint32_t(previous_group));
    return c.r(3);
}
}

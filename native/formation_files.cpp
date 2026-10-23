#include "formation_files.hpp"

namespace cm::aot::formation {
using guest::Cpu;
using guest::Frame;
namespace {
// FORMATION.CPP's string pool and the globals its asserts use.
constexpr std::int32_t pool_slot = -0x550c, message_slot = -0x7ba8, success_slot = -0x7bac;
constexpr std::int32_t humans_slot = -0x7a54;  // -> human manager table
// Strings in the pool.
constexpr std::uint32_t read_mode = 0x75, write_mode = 0x3fe;

// The C library and the routines the files go through (generated code).
constexpr std::uint32_t open_file = 0x10739de0, read_file = 0x10775fe0, write_file = 0x10776360,
                        close_file = 0x107766f0, file_platform = 0x107464d0, set_file_platform = 0x107465a0,
                        string_length = 0x1077dbd0, compare_strings = 0x1077dc90, format = 0x1077b720,
                        split_path = 0x10739890, clear = 0x10778300, swap16 = 0x1074d5a0, swap32 = 0x1074d610,
                        swap64 = 0x1074d680, manager_name = 0x10148eb0, player_spot = 0x101124c0,
                        set_player_spot = 0x10111e60, rebuild_zones = 0x10116560;

constexpr std::uint32_t mac_platform = 0xf1fffff1;  // platform tag of a big-endian (Mac) file
constexpr std::uint32_t preset_key = 123456789;     // added to a preset file's identifier
// Format versions: the oldest identifier that is a formation, the oldest
// readable one, and when the shape, display order, team instruction bit
// 0x20000 and 64-bit player instructions arrived.
constexpr std::int32_t first_version = 0x98ec54, readable_version = 0x98ec59, shape_version = 0x98ec5a,
                       order_version = 0x98ec5c, flag_version = 0x98ec5d, current_version = 0x98ec5e;

// A compare into cr0 (cmpw, cmplw, a record form against 0): less, greater,
// equal and XER's summary overflow.
void compare(Cpu& c, std::int32_t a, std::int32_t b) {
    c.set_cr(0, std::uint8_t((a < b) << 3 | (a > b) << 2 | (a == b) << 1 | (c.s.r<1>(guest::reg::xer_so) & 1)));
}
void compare_logical(Cpu& c, std::uint32_t a, std::uint32_t b) {
    c.set_cr(0, std::uint8_t((a < b) << 3 | (a > b) << 2 | (a == b) << 1 | (c.s.r<1>(guest::reg::xer_so) & 1)));
}

std::uint32_t byte_swapped(std::uint32_t value) {
    return value << 24 | (value & 0xff00) << 8 | (value >> 8 & 0xff00) | value >> 24;
}
// fread/fwrite(buffer, size, count, file) at the original `bl` before `back`.
std::uint32_t transfer(Cpu& c, std::uint32_t routine, std::uint32_t buffer, std::uint32_t size, std::uint32_t count,
                       std::uint32_t file, std::uint32_t back) {
    c.set({{3, buffer}, {4, size}, {5, count}, {6, file}});
    c.call(routine, back);
    return c.r(3);
}
void close(Cpu& c, std::uint32_t file, std::uint32_t back) {
    c.set(3, file);
    c.call(close_file, back);
}
// Inverts the lock's bytes (but 0xff) up to its length, which the original
// asks again before each byte; `counter` (and `cursor`, the byte's address,
// when not 0) are the non-volatile registers it counts in.
void invert_lock(Cpu& c, std::uint32_t lock, unsigned counter, unsigned cursor, std::uint32_t back) {
    for (std::uint32_t index = 0;; ++index) {
        c.set(counter, index);
        if (cursor) c.set(cursor, lock + index);
        c.set(3, lock);
        c.call(string_length, back);
        if (index >= c.r(3)) return;
        const auto byte = c.u8(lock + index);
        c.put8(lock + index, byte == 0xff ? 0xff : byte ^ 0xff);
    }
}
// The record's flags after a load or save; `lock` set when the lock is. The
// original leaves the lock's first character (sign-extended) in r4, 1 in r5.
void mark_on_disk(Cpu& c, std::uint32_t formation) {
    c.set({{4, std::uint32_t(std::int32_t(c.i8(formation + disk::lock)))}, {5, 1}});
    c.put8(formation + field::saved, 1);
    c.put8(formation + disk::locked, c.i8(formation + disk::lock) != 0);
    c.put8(formation + disk::on_disk, 1);
}

// An error site: the message formatted with the path into a stack buffer
// (unused afterwards), then the assertion report at `at` (its first call).
struct Site {
    std::uint32_t at;
    std::uint32_t message;            // pool offset, 0: no message
    std::int32_t name, extension;     // the assertion's stack buffers
    std::uint32_t line;
};
class Reporter {
public:
    Reporter(Cpu& cpu, const Frame& frame, std::uint32_t path, std::uint32_t quiet, std::int32_t message_buffer)
        : c(cpu), frame_(frame), pool(cpu.toc(pool_slot)), path_(path), quiet_(quiet), buffer(message_buffer) {}
    void operator()(const Site& site) const {
        compare(c, std::int8_t(quiet_), 0);  // extsb. on the quiet flag
        if (std::int8_t(quiet_)) return;
        if (site.message) {
            c.set({{3, frame_.at(buffer)}, {4, pool + site.message}, {5, path_}});
            c.call(format, site.at - 0x14);
        }
        guest::report_assertion(
            c, {site.at, pool + 0xe, pool + 0x1c, pool + 0x29, frame_.at(site.name), frame_.at(site.extension), site.line});
    }

private:
    Cpu& c;
    const Frame& frame_;
    std::uint32_t pool, path_, quiet_;
    std::int32_t buffer;
};
}

std::uint32_t load_from_disk(Cpu& c, std::uint32_t formation, std::uint32_t path, std::uint32_t quiet,
                             std::uint32_t preset) {
    Frame frame(c, 0x10115000, 0x510, {.first_gpr = 23});
    const auto pool = c.toc(pool_slot);
    c.set({{23, preset}, {25, c.toc(success_slot)}, {26, c.toc(message_slot)}, {27, quiet}, {28, formation},
           {29, path}, {31, pool}});
    const Reporter report(c, frame, path, quiet, 0x318);
    if (!path) {
        report({0x1011505c, 0x51, 0x2cc, 0xc4, 0x8d1});
        return 0;
    }
    c.set({{3, path}, {4, pool + read_mode}});
    c.call(open_file, 0x101150ac);
    const auto file = c.r(3);
    c.set(30, file);
    if (!file) {
        report({0x101150e0, 0x78, 0x2ac, 0xbc, 0x8d9});
        return 0;
    }
    // A failed read: report, close the file, fail.
    const auto fail = [&](const Site& site, std::uint32_t back) {
        report(site);
        close(c, file, back);
        return 0u;
    };
    const auto read = [&](std::uint32_t buffer, std::uint32_t size, std::uint32_t back) {
        return transfer(c, read_file, buffer, size, 1, file, back) == 1;
    };
    const auto swap = [&](std::uint32_t routine, std::uint32_t buffer, std::uint32_t count, std::uint32_t back) {
        c.set({{3, buffer}, {4, count}});
        c.call(routine, back);
    };
    const auto swap_word = [&](std::uint32_t address) { c.put32(address, byte_swapped(c.u32(address))); };

    c.call(file_platform, 0x10115128);
    const bool native = c.r(3) == mac_platform;
    c.set(24, native);
    const auto identifier = frame.at(0x40);
    if (!read(identifier, 4, 0x10115154)) return fail({0x10115188, 0xa8, 0x28c, 0xb4, 0x8e3}, 0x101151cc);
    if (!native) swap_word(identifier);
    if (std::int8_t(preset)) c.put32(identifier, c.u32(identifier) - preset_key);
    const auto version = [&] { return c.i32(identifier); };
    c.set(4, c.u32(identifier));  // the original compares it in r4
    if (version() < first_version || version() > current_version)
        return fail({0x1011524c, 0xe1, 0x26c, 0xac, 0x8f0}, 0x10115290);
    if (version() < readable_version) return fail({0x101152d0, 0x115, 0x24c, 0xa4, 0x8f9}, 0x10115314);

    const auto lock = formation + disk::lock;
    if (!read(lock, disk::lock_size, 0x10115330)) return fail({0x10115364, 0x154, 0x22c, 0x9c, 0x902}, 0x101153a8);
    invert_lock(c, lock, 23, 0, 0x101153e4);
    if (!read(formation + disk::creator, disk::creator_size, 0x10115400))
        return fail({0x10115434, 0x18b, 0x20c, 0x94, 0x90e}, 0x10115478);

    const auto shape = formation + field::shape;
    if (version() >= shape_version) {
        if (!read(shape, 4, 0x101154a8)) return fail({0x101154dc, 0x1c1, 0x1ec, 0x8c, 0x919}, 0x10115520);
        if (!native) swap_word(shape);
    } else {
        c.put32(shape, 0);
    }
    if (!read(formation + field::position, 0x16, 0x1011555c))
        return fail({0x10115590, 0x1ff, 0x1cc, 0x84, 0x929}, 0x101155d4);
    if (!native) swap(swap16, formation + field::position, 11, 0x101155f0);
    c.set(3, formation);
    c.link(0x101155f8);
    update_shape(c, formation);

    const auto order = formation + disk::display_order;
    if (version() >= order_version) {
        if (!read(order, 11, 0x10115620)) return fail({0x10115654, 0x23f, 0x1ac, 0x7c, 0x939}, 0x10115698);
    } else {
        c.set({{3, order}, {4, 0}, {5, 11}});
        c.call(clear, 0x101156b0);
    }
    if (!read(formation + field::position_with_ball, 0x16, 0x101156c4))
        return fail({0x101156f8, 0x27b, 0x18c, 0x74, 0x948}, 0x1011573c);
    if (!native) swap(swap16, formation + field::position_with_ball, 11, 0x10115758);
    if (!read(formation + disk::spots, 0x420, 0x1011576c))
        return fail({0x101157a0, 0x2c1, 0x16c, 0x6c, 0x952}, 0x101157e4);
    if (!native) swap(swap16, formation + disk::spots, 0x210, 0x10115800);
    // Two versions also stored which positions were changed; read and dropped.
    if (c.u32(identifier) == 0x98ec59 || c.u32(identifier) == 0x98ec5a) {
        const auto changed = frame.at(0x3e0);
        if (!read(changed, 0x108, 0x1011582c)) return fail({0x10115860, 0x300, 0x14c, 0x64, 0x961}, 0x101158a4);
        if (!native) swap(swap16, changed, 0x84, 0x101158c0);
    }

    // From here a failed read is reported but the load goes on.
    const auto team = formation + disk::team_instructions;
    if (!read(team, 4, 0x101158d4)) report({0x10115908, 0x340, 0x12c, 0x5c, 0x96a});
    else if (!native) swap_word(team);
    const auto instructions = formation + field::instructions;
    if (version() < current_version) {
        // 32-bit instructions: the high word becomes 0xa.
        const auto old = frame.at(0x2ec);
        if (!read(old, 0x2c, 0x10115984)) report({0x101159b8, 0x380, 0x10c, 0x54, 0x974});
        else if (!native) swap(swap32, old, 11, 0x10115a0c);
        for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
            c.put32(instructions + player * 8, 0xa);
            c.put32(instructions + player * 8 + 4, c.u32(old + player * 4));
        }
    } else {
        if (!read(instructions, 0x58, 0x10115adc)) report({0x10115b10, 0x380, 0xec, 0x4c, 0x981});
        else if (!native) swap(swap64, instructions, 11, 0x10115b64);
    }
    if (!read(formation + field::role, 11, 0x10115b78)) report({0x10115bac, 0x3c2, 0xcc, 0x44, 0x988});
    if (!(c.u32(team) & 0x4000)) c.put32(team, c.u32(team) | 0x2000);
    if (version() < flag_version) c.put32(team, c.u32(team) | 0x20000);

    // Each player's spot for the first cell without the ball, set again by
    // hand (the cell travels by value: two zero halfwords at sp+0x38).
    c.set({{24, 0}, {23, 0}});
    for (std::uint32_t player = 0; player < std::uint32_t(players); ++player) {
        c.set(25, player);
        c.put16(frame.at(0x38), 0);
        c.put16(frame.at(0x3a), 0);
        c.set({{3, formation}, {4, player}, {5, 0}, {6, c.u32(frame.at(0x38))}, {7, frame.at(0x3c)}});
        c.call(player_spot, 0x10115c48);
        c.set({{3, formation}, {4, player}, {5, 0}, {6, c.u32(frame.at(0x38))}, {7, c.u32(frame.at(0x3c))}});
        c.call(set_player_spot, 0x10115c60);
    }
    c.set(25, players);
    c.set({{3, path}, {4, 0}, {5, 0}, {6, formation + disk::name}, {7, 0}});
    c.call(split_path, 0x10115c84);
    c.set(3, formation);
    c.call(rebuild_zones, 0x10115c8c);
    mark_on_disk(c, formation);
    close(c, file, 0x10115cb8);
    return 1;
}

std::uint32_t save_to_disk(Cpu& c, std::uint32_t formation, std::uint32_t path, std::uint32_t quiet,
                           std::uint32_t preset) {
    Frame frame(c, 0x10115cd0, 0x330, {.first_gpr = 24});
    const auto pool = c.toc(pool_slot);
    c.set({{24, preset}, {25, formation}, {26, path}, {27, quiet}, {29, c.toc(success_slot)},
           {30, c.toc(message_slot)}, {31, pool}});
    const auto identifier = frame.at(0x38);
    c.put32(identifier, current_version);
    c.set(4, 0x990000);  // left over from building the identifier
    const Reporter report(c, frame, path, quiet, 0x244);
    if (!path) {
        report({0x10115d28, 0, 0x224, 0x9c, 0x9c3});
        return 0;
    }
    c.set({{3, path}, {4, pool + write_mode}});
    c.call(open_file, 0x10115d78);
    const auto file = c.r(3);
    c.set(28, file);
    if (!file) {
        report({0x10115dac, 0x401, 0x204, 0x94, 0x9cf});
        return 0;
    }
    const auto fail = [&](const Site& site, std::uint32_t back) {
        report(site);
        close(c, file, back);
        return 0u;
    };
    const auto write = [&](std::uint32_t buffer, std::uint32_t size, std::uint32_t back) {
        return transfer(c, write_file, buffer, size, 1, file, back) == 1;
    };
    if (std::int8_t(preset)) c.put32(identifier, c.u32(identifier) + preset_key);
    if (!write(identifier, 4, 0x10115e1c)) return fail({0x10115e50, 0x42f, 0x1e4, 0x8c, 0x9de}, 0x10115e94);
    // The lock goes out inverted and is restored afterwards.
    const auto lock = formation + disk::lock;
    invert_lock(c, lock, 24, 0, 0x10115ed0);
    if (!write(lock, disk::lock_size, 0x10115eec)) {
        invert_lock(c, lock, 24, 0, 0x10115f28);
        return fail({0x10115f5c, 0x468, 0x1c4, 0x84, 0x9f1}, 0x10115fa0);
    }
    invert_lock(c, lock, 24, 0, 0x10115fdc);
    if (!write(formation + disk::creator, disk::creator_size, 0x10115ff8))
        return fail({0x1011602c, 0x49f, 0x1a4, 0x7c, 0xa01}, 0x10116070);
    if (!write(formation + field::shape, 4, 0x1011608c)) return fail({0x101160c0, 0x4d5, 0x184, 0x74, 0xa0e}, 0x10116104);
    if (!write(formation + field::position, 0x16, 0x10116120))
        return fail({0x10116154, 0x513, 0x164, 0x6c, 0xa1b}, 0x10116198);
    if (!write(formation + disk::display_order, 11, 0x101161b4))
        return fail({0x101161e8, 0x553, 0x144, 0x64, 0xa28}, 0x1011622c);
    if (!write(formation + field::position_with_ball, 0x16, 0x10116248))
        return fail({0x1011627c, 0x58f, 0x124, 0x5c, 0xa35}, 0x101162c0);
    if (!write(formation + disk::spots, 0x420, 0x101162dc))
        return fail({0x10116310, 0x5d5, 0x104, 0x54, 0xa42}, 0x10116354);
    // From here a failed write is only reported.
    if (!write(formation + disk::team_instructions, 4, 0x10116370)) report({0x101163a4, 0x614, 0xe4, 0x4c, 0xa4f});
    if (!write(formation + field::instructions, 0x58, 0x101163f4)) report({0x10116428, 0x654, 0xc4, 0x44, 0xa5a});
    if (!write(formation + field::role, 11, 0x10116478)) report({0x101164ac, 0x696, 0xa4, 0x3c, 0xa65});
    c.set({{3, file}, {4, mac_platform}});
    c.call(set_file_platform, 0x101164f8);
    close(c, file, 0x10116500);
    c.set({{3, path}, {4, 0}, {5, 0}, {6, formation + disk::name}, {7, 0}});
    c.call(split_path, 0x10116518);
    mark_on_disk(c, formation);
    return 1;
}

std::uint32_t read_lock(Cpu& c, std::uint32_t path, std::uint32_t lock, std::uint32_t creator, std::uint32_t preset) {
    Frame frame(c, 0x10117fa0, 0x80, {.first_gpr = 27});
    const auto pool = c.toc(pool_slot);
    c.set({{27, c.toc(message_slot)}, {28, lock}, {29, creator}, {30, preset}, {31, pool}});
    if (!path || !lock || !creator) {
        compare_logical(c, 0, 0);  // the null argument's cmplwi
        guest::report_assertion(c, {0x10117ff0, pool + 0xe, pool + 0x1c, pool + 0x29, frame.at(0x44), frame.at(0x3c), 0xfe0});
        c.set(4, c.toc(success_slot));  // where it cleared the success flag
        return 0;
    }
    c.put8(lock, 0);
    c.put8(creator, 0);
    c.set({{3, path}, {4, pool + read_mode}});
    c.call(open_file, 0x1011804c);
    const auto file = c.r(3);
    c.set(31, file);
    if (!file) {
        compare(c, 0, 0);  // or. on the file
        return 0;
    }
    c.call(file_platform, 0x10118058);
    const bool native = c.r(3) == mac_platform;
    c.set(27, native);
    const auto fail = [&] {
        close(c, file, 0x10118160);
        return 0u;
    };
    const auto identifier = frame.at(0x38);
    const auto read = [&](std::uint32_t buffer, std::uint32_t size, std::uint32_t back) {
        const auto items = transfer(c, read_file, buffer, size, 1, file, back);
        compare_logical(c, items, 1);
        return items == 1;
    };
    if (!read(identifier, 4, 0x10118084)) return fail();
    if (!native) c.put32(identifier, byte_swapped(c.u32(identifier)));
    if (std::int8_t(preset)) c.put32(identifier, c.u32(identifier) - preset_key);
    c.set(4, c.u32(identifier));
    compare(c, c.i32(identifier), readable_version);
    if (c.i32(identifier) < readable_version) return fail();
    if (!read(lock, disk::lock_size, 0x101180e0)) return fail();
    invert_lock(c, lock, 27, 30, 0x10118124);
    if (!read(creator, disk::creator_size, 0x10118140)) return fail();
    close(c, file, 0x10118150);
    return 1;
}

std::uint32_t lock_state(Cpu& c, std::uint32_t entry, std::uint32_t path, std::uint32_t preset) {
    Frame frame(c, entry, 0x140, {.first_gpr = 31});
    const auto result = [&](std::uint32_t state) {
        c.set(31, state);
        return state;
    };
    result(2);
    const auto lock = frame.at(0x38), creator = frame.at(0x6c);
    c.set({{3, path}, {4, lock}, {5, creator}, {6, preset}});
    c.link(entry + 0x24);
    c.set(3, read_lock(c, path, lock, creator, preset));
    compare(c, std::int8_t(c.r(3)), 0);
    if (!std::int8_t(c.r(3))) return 2;
    c.set(3, lock);
    c.call(string_length, entry + 0x34);
    compare_logical(c, c.r(3), 0);
    if (!c.r(3)) return result(1);
    c.set({{3, c.u32(c.toc(humans_slot))}, {4, 0}});
    c.call(manager_name, entry + 0x4c);  // the current human manager's name
    c.set(4, lock);
    c.call(compare_strings, entry + 0x54);
    compare(c, std::int32_t(c.r(3)), 0);
    return result(c.r(3) == 0 ? 9 : 5);
}
}

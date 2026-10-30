#include "db_files_detail.hpp"
#include <array>

namespace cm::aot::db_files {
namespace {
using namespace guest;
using Converter = void (*)(Cpu&, std::uint32_t, std::uint32_t, std::uint32_t);
using OldReader = std::uint32_t (*)(Cpu&, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

// Where the loader's callees find its values: they save these registers into the
// stack image. The C++ computes with its own values and only mirrors them here.
// Every table's loader uses the first assignment but oldstaff's.
struct Registers {
    unsigned packed, success, message, header, pool, file, records, size, count, version, mac, buffer, source;
};
constexpr Registers usual{19, 26, 27, 31, 28, 29, 30, 23, 25, 24, 22, 21, 20},
    oldstaff_registers{20, 28, 29, 22, 30, 31, 21, 26, 27, 0, 25, 24, 23};

// Distances from the entry that differ with the version test's length.
struct Layout {
    std::uint32_t read_at, version_error_at, old_call_back;
};
constexpr Layout one_version{0x184, 0x278, 0}, two_versions{0x18c, 0x280, 0},
    old_reader_layout{0x190, 0x29c, 0x284}, clubs_layout{0x190, 0x2cc, 0x2b4};
// Which versions a file may have: only the current one, 1 and 2 alike (cities),
// or the current one and 1 through an older reader.
enum class Versions { current, one_or_two, current_or_old };
enum class Variant { usual, repairs_clubs, oldstaff };
struct Spec {
    std::uint32_t entry, name;
    std::uint32_t memory_size, disk_size;
    std::int32_t current;
    std::uint32_t allocation_line, version_line;
    Converter convert;
    Layout layout = one_version;
    Versions versions = Versions::current;
    OldReader old_reader = nullptr;
    Variant variant = Variant::usual;
};
constexpr std::array specs{
    Spec{0x105d7250, 0xa, 0xc8, 0xc6, 1, 0x123, 0x12c, convert_continents},
    Spec{0x105d7750, 0x49, 0x128, 0x122, 2, 0x15a, 0x163, convert_nations},
    Spec{0x105d85d0, 0x55, 0x38, 0x38, 2, 0x1fd, 0x207, convert_cities, two_versions, Versions::one_or_two},
    Spec{0x105d8b40, 0x60, 0x50, 0x4e, 1, 0x234, 0x23d, convert_stadiums},
    Spec{0x105d8f60, 0x6d, 0x248, 0x245, 2, 0x26d, 0x285, convert_clubs, clubs_layout, Versions::current_or_old, read_old_clubs, Variant::repairs_clubs},
    Spec{0x105da6e0, 0x77, 0x70, 0x6e, 2, 0x331, 0x33a, convert_staff},
    Spec{0x105dac80, 0x81, 0x9d, 0x9d, 1, 0x385, 0x38e, convert_oldstaff, one_version, Versions::current, nullptr, Variant::oldstaff},
    Spec{0x105db230, 0x8e, 0x44, 0x44, 2, 0x3e3, 0x3ef, convert_non_players, old_reader_layout, Versions::current_or_old, read_old_non_players},
    Spec{0x105dc1d0, 0x9e, 0x48, 0x46, 2, 0x46e, 0x47a, convert_players, old_reader_layout, Versions::current_or_old, read_old_players},
    Spec{0x105dd980, 0xaa, 0x34, 0x34, 1, 0x530, 0x539, convert_staff_preferences},
    Spec{0x105ddd90, 0xc0, 0x14, 0x11, 1, 0x569, 0x572, convert_staff_history},
    Spec{0x105de150, 0xd2, 0x6c, 0x6b, 2, 0x59d, 0x5a9, convert_club_comps, old_reader_layout, Versions::current_or_old, read_old_club_comps},
    Spec{0x105dea20, 0xe1, 0x1c, 0x1a, 1, 0x5fc, 0x605, convert_club_comp_history},
    Spec{0x105dede0, 0xf7, 0x68, 0x65, 2, 0x62f, 0x638, convert_staff_comps},
    Spec{0x105df210, 0x107, 0x3c, 0x3a, 1, 0x684, 0x68d, convert_staff_comp_history},
    Spec{0x105df630, 0x11e, 0x2c, 0x2b, 1, 0x6c1, 0x6ca, convert_officials},
    Spec{0x105dfa80, 0x12c, 0x3c, 0x3a, 1, 0x700, 0x709, convert_colours},
    Spec{0x105dfe60, 0x138, 0x2c, 0x3c, 1, 0x731, 0x73a, convert_names},
};

// Reports the assertion, clears the success flag and calls exit. The original
// continues past exit if it returns, so the callers do too.
void fatal(Cpu& c, const Assertion& a) {
    report_assertion(c, a);
    c.set(3, 0xffffffff);
    c.call(exit_program, a.at + 0x44);
}
}

std::uint32_t open_for_reading(Cpu& c, std::uint32_t directory, std::uint32_t name) {
    Frame frame(c, 0x105d71b0, 0x40, {.first_gpr = 31});
    const auto path = c.toc(-0x7bb8);
    keep(c, {{31, path}});
    c.set({{6, name}, {5, directory}, {4, 0}, {3, path}, {7, 0}});
    c.call(merge_path, 0x105d71dc);
    c.set({{4, c.toc(pool_slot) + pool_rb}, {3, path}});
    c.call(open_stream, 0x105d71e8);
    return c.r(3);
}

std::uint32_t open_for_writing(Cpu& c, std::uint32_t directory, std::uint32_t name) {
    Frame frame(c, 0x105d7200, 0x40, {.first_gpr = 31});
    const auto path = c.toc(-0x7bb8);
    keep(c, {{31, path}});
    c.set({{6, name}, {5, directory}, {4, 0}, {3, path}, {7, 0}});
    c.call(merge_path, 0x105d722c);
    c.set({{4, c.toc(pool_slot) + pool_wb}, {3, path}});
    c.call(open_stream, 0x105d723c);
    return c.r(3);
}

std::uint32_t save(Cpu& c, std::uint32_t entry, std::uint32_t record_size, std::uint32_t file,
                   std::uint32_t packed, std::uint32_t records, std::uint32_t count) {
    Frame frame(c, entry, 0x40, {.first_gpr = 31});
    keep(c, {{31, count}});
    const bool is_packed = compare(c, std::int8_t(packed)) != 0;
    c.set({{3, records}, {5, count}, {6, file}, {4, record_size}});
    if (is_packed) c.call(packed_write, entry + 0x34);
    else c.call(stream_write, entry + 0x4c);
    return result(c, c.r(3) == count);
}

std::uint32_t load(Cpu& c, Table table, std::uint32_t file, std::uint32_t packed,
                   std::uint32_t records, std::uint32_t header) {
    const auto& spec = specs[std::size_t(table)];
    const bool late_version = spec.variant == Variant::oldstaff;
    const auto& r = late_version ? oldstaff_registers : usual;
    const auto entry = spec.entry;
    // The oldstaff loader reads the version only when it tests it, so its
    // earlier calls sit one instruction nearer the entry.
    const std::uint32_t shift = late_version ? 4 : 0;
    Frame frame(c, entry, 0x3c0, {.first_gpr = r.packed});
    const auto pool = c.toc(pool_slot);
    const auto sp = frame.base();
    const auto path = sp + 0x188, base_name = sp + 0x88, cached_name = sp + 0x288;
    c.set(r.packed, packed);
    c.set(r.success, c.toc(-0x7bac));
    c.set(r.message, c.toc(-0x7ba8));
    c.set(r.header, header);
    c.set(r.pool, pool);
    c.set(r.file, file);
    c.set(r.records, records);
    c.set(r.size, spec.memory_size);
    const auto count = c.u32(header + 0x38);
    c.set(r.count, count);
    auto version = late_version ? 0 : std::int32_t(c.u32(header + 0x40));
    if (!late_version) c.set(r.version, std::uint32_t(version));

    bool mac = compare(c, std::int8_t(packed)) != 0;
    if (!mac) {
        c.call(read_end_marker, entry + 0x4c - shift);
        mac = c.r(3) == mac_end_marker;
        c.set(4, 0xf2000000);
    }
    compare(c, mac);
    c.set(r.mac, mac);
    auto buffer = records, source = file;
    c.set(r.buffer, buffer);
    c.set(r.source, source);
    auto size = spec.memory_size;
    if (!mac) {
        // A Windows file: use the converted copy in the data folder if there
        // is one, otherwise read and convert this one and write that copy.
        c.set({{3, path}, {4, pool + spec.name}});
        c.call(copy_string, entry + 0x7c - shift);
        c.set({{3, path}, {4, pool + pool_space}});
        c.call(append_string, entry + 0x88 - shift);
        c.set({{4, header}, {3, path}});
        c.call(append_string, entry + 0x94 - shift);
        c.set({{3, path}, {5, pool + pool_dam}, {4, path}});
        c.call(replace_extension, entry + 0xa4 - shift);
        c.set({{3, pool + pool_data_folder}, {4, path}});
        c.call(data_folder_path, entry + 0xb0 - shift);
        c.set({{4, c.r(3)}, {3, path}});
        c.call(copy_string, entry + 0xbc - shift);
        c.set({{3, path}, {6, base_name}, {4, 0}, {5, 0}, {7, 0}});
        c.call(split_path, entry + 0xd4 - shift);
        c.set({{3, cached_name}, {6, base_name}, {7, pool + pool_dam}, {4, 0}, {5, 0}});
        c.call(merge_path, entry + 0xec - shift);
        c.set({{4, pool}, {3, path}});
        c.call(open_stream, entry + 0xf8 - shift);
        source = c.r(3);
        c.set(r.source, source);
        if (compare(c, std::int32_t(source)) != 0) {
            mac = true;
            c.set(r.mac, 1);
        } else {
            c.set(3, count * spec.disk_size);
            source = file;
            c.set(r.source, source);
            size = spec.disk_size;
            c.set(r.size, size);
            c.call(allocate, entry + 0x118 - shift);
            buffer = c.r(3);
            c.set(r.buffer, buffer);
            if (compare(c, std::int32_t(buffer)) == 0)
                fatal(c, assertion(c, entry + 0x134 - shift, sp + 0x68, sp + 0x40, spec.allocation_line));
        }
    }

    if (late_version) version = std::int32_t(c.u32(header + 0x40));
    enum class Route { current, old, error } route = Route::error;
    switch (spec.versions) {
    case Versions::current_or_old:
        // cmpwi 2; beq current; bge error; cmpwi 1; bge old.
        if (compare(c, version, spec.current) == 0) route = Route::current;
        else if (version < spec.current && compare(c, version, 1) >= 0) route = Route::old;
        break;
    case Versions::one_or_two:
        // cmpwi 3; bge error; cmpwi 1; bge current.
        if (compare(c, version, 3) < 0 && compare(c, version, 1) >= 0) route = Route::current;
        break;
    case Versions::current:
        if (compare(c, version, spec.current) == 0) route = Route::current;
        break;
    }
    if (route == Route::error) {
        fatal(c, assertion(c, entry + spec.layout.version_error_at, sp + 0x48, sp + 0x38, spec.version_line));
        return result(c, 0);
    }
    if (route == Route::old) {
        enter(c, entry + spec.layout.old_call_back, {file, packed, records, count});
        return spec.old_reader(c, file, packed, records, count);
    }

    const auto block = entry + spec.layout.read_at;
    c.set({{3, buffer}, {4, size}, {5, count}, {6, source}});
    if (compare(c, std::int8_t(packed)) != 0) c.call(packed_read, block + 0x1c);
    else c.call(stream_read, block + 0x38);
    const auto read = c.r(3);
    c.set(r.packed, read);
    if (compare_logical(c, source, file) != 0) {
        c.set(3, source);
        c.call(stream_close, block + 0x4c);
    }
    if (compare_logical(c, read, count) != 0) return result(c, 0);
    if (compare(c, std::int8_t(mac)) == 0) {
        c.set({{3, header}, {4, cached_name}, {5, 0x32}});
        c.call(copy_string_limited, block + 0x74);
        c.put8(header + 0x32, 0);
        c.put32(header + 0x3c, 0);
        enter(c, block + 0x90, {records, buffer, count});
        spec.convert(c, records, buffer, count);
        c.set({{3, path}, {4, pool + pool_wb}});
        c.call(open_stream, block + 0x9c);
        const auto copy = c.r(3);
        c.set(r.packed, copy);
        if (compare(c, std::int32_t(copy)) != 0) {
            c.set({{3, records}, {5, count}, {6, copy}, {4, spec.memory_size}});
            c.call(stream_write, block + 0xb8);
            c.set({{4, mac_end_marker}, {3, copy}});
            c.call(write_end_marker, block + 0xc8);
            c.set(3, copy);
            c.call(stream_close, block + 0xd0);
        }
        c.set(3, buffer);
        c.call(release, block + 0xd8);
    }
    if (spec.variant == Variant::repairs_clubs) {
        // A zero at +0x1ec becomes 1, as the version 1 reader also does.
        c.set(4, 0);
        if (compare_logical(c, count) != 0) {
            std::uint32_t offset = 0;
            for (auto left = count; left != 0; --left, offset += 0x248) {
                c.set(5, offset + 0x1ec);
                const auto value = c.i16(records + offset + 0x1ec);
                c.set(3, extended(value));
                if (compare(c, value) == 0) c.put16(records + offset + 0x1ec, 1);
            }
            c.set(4, offset);
        }
    }
    return result(c, 1);
}
}

// The press object's construction and its file, media.dat: a count and the
// event rows, a count and the history rows. Loading and saving report a
// failed step through the game's assertion and return 0, leaving the file
// open (the original never closes it on those paths).
#include "media_store.hpp"
#include "media_store_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::store;
namespace {
// Where one assertion site sits: its first call, the frame's two name buffers and the line.
struct Site {
    std::uint32_t at;
    unsigned name, extension, line;
};
std::uint32_t fail(Cpu& c, const Frame& frame, const Site& site) {
    assertion(c, frame, site.at, site.name, site.extension, site.line);
    return result(c, 0);
}
// The open file's handle lives in the frame at +0x38.
struct DataFile {
    Cpu& c;
    const Frame& frame;
    std::uint32_t handle() const { return c.u32(frame.at(0x38)); }
    std::uint32_t transfer(std::uint32_t entry, std::uint32_t back, std::uint32_t buffer, std::uint32_t size,
                           std::uint32_t count) const {
        return Program{c}.call(entry, back, {buffer, size, count, handle()});
    }
};
}

// The empty press; a path loads media.dat into it.
std::uint32_t construct_press(Cpu& c, std::uint32_t self, std::uint32_t path) {
    Frame frame(c, 0x10646d90, 0x40, {.first_gpr = 31});
    keep(c, {{31, self}});
    const Press press{c, self};
    press.event_count() = 0; press.event_capacity() = 0; press.events() = 0;
    press.history() = 0; press.history_capacity() = 0; press.history_count() = 0;
    leave(c, {{5, 0}});
    c.set(3, 0);
    if (compare_logical(c, path) != 0) {
        enter(c, 0x10646de4, {self, path, 0});
        load_press(c, self, path);
    }
    return result(c, self);
}

// Releases both tables; a positive flag also deletes the object itself.
std::uint32_t destroy_press(Cpu& c, std::uint32_t self, std::uint32_t flags) {
    Frame frame(c, 0x10646e00, 0x40, {.first_gpr = 30});
    keep(c, {{31, flags}, {30, self}});
    const Press press{c, self};
    Program program{c};
    if (compare(c, std::int32_t(self)) != 0) {
        if (compare_logical(c, press.events()) != 0) {
            program.call(library::release, 0x10646e34, {press.events()});
            press.events() = 0;
        }
        if (compare_logical(c, press.history()) != 0) {
            program.call(library::release, 0x10646e50, {press.history()});
            press.history() = 0;
        }
        if (compare(c, std::int32_t(std::int16_t(flags))) > 0) program.call(0x107735a0, 0x10646e68, {self});
    }
    return result(c, self);
}

std::uint32_t load_press(Cpu& c, std::uint32_t self, std::uint32_t path) {
    Frame frame(c, 0x10646e90, 0x1a0, {.first_gpr = 27});
    keep(c, {{28, c.toc(slot::flag)}, {29, self}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    const Press press{c, self};
    Program program{c};
    const DataFile file{c, frame};
    if (compare_logical(c, path) == 0) return result(c, 0);
    c.put32(frame.at(0x38), program.call(library::open_file, 0x10646ed8, {path, c.toc(slot::pool), 0, 0x16, 4}));
    if (compare_logical(c, file.handle()) == 0) return result(c, 0);

    if (compare(c, file.transfer(library::read_file, 0x10646f04, self + 0x14, 4, 1), 1) != 0)
        return fail(c, frame, {0x10646f20, 0x15c, 0x74, 0xa5});
    press.event_capacity() = press.event_count().get();
    press.events() = program.call(library::allocate, 0x10646f78, {press.event_capacity() * event_size});
    if (compare_logical(c, press.events()) == 0 && compare(c, press.event_capacity()) != 0)
        return fail(c, frame, {0x10646fa8, 0x13c, 0x6c, 0xb1});
    const auto events_read = file.transfer(library::read_file, 0x10647000, press.events(), event_size,
                                           std::uint32_t(press.event_count()));
    if (compare(c, press.event_count(), std::int32_t(events_read)) != 0)
        return fail(c, frame, {0x10647020, 0x11c, 0x64, 0xb9});
    program.call(library::sort, 0x10647078, {press.events(), std::uint32_t(press.event_count()), event_size, c.toc(slots::event_order)});

    if (compare(c, file.transfer(library::read_file, 0x1064708c, self + 8, 4, 1), 1) != 0)
        return fail(c, frame, {0x106470a8, 0xfc, 0x5c, 0xc4});
    press.history_capacity() = press.history_count().get();
    press.history() = program.call(library::allocate, 0x10647100, {press.history_capacity() * history_size});
    if (compare_logical(c, press.history()) == 0 && compare(c, press.history_capacity()) != 0)
        return fail(c, frame, {0x10647130, 0xdc, 0x54, 0xcf});

    const auto version = program.call(library::save_version, 0x10647180, {c.u32(c.toc(slots::version))});
    if (compare(c, std::int32_t(version), 0x3e037) >= 0) {
        const auto rows_read = file.transfer(library::read_file, 0x10647380, press.history(), history_size,
                                             std::uint32_t(press.history_count()));
        if (compare(c, press.history_count(), std::int32_t(rows_read)) != 0)
            return fail(c, frame, {0x106473a0, 0x7c, 0x3c, 0xf5});
    } else {
        // Older files: rows come in a staging buffer and are copied field by field.
        Kept<27> staging(c, program.call(library::allocate, 0x1064719c, {press.history_capacity() * history_size}));
        if (compare(c, std::int32_t(std::uint32_t(staging))) == 0 && compare(c, press.history_capacity()) != 0)
            return fail(c, frame, {0x106471c4, 0xbc, 0x4c, 0xd8});
        const auto rows_read = file.transfer(library::read_file, 0x1064721c, staging, history_size,
                                             std::uint32_t(press.history_count()));
        if (compare(c, press.history_count(), std::int32_t(rows_read)) != 0)
            return fail(c, frame, {0x1064723c, 0x9c, 0x44, 0xdf});
        for (std::int32_t index = 0; compare(c, index, press.history_capacity()) < 0; ++index) {
            const auto from = std::uint32_t(staging) + std::uint32_t(index) * history_size;
            const auto to = press.history_row(std::uint32_t(index));
            to.club() = c.u32(from + 0xc);
            to.date().assign(from);
            // The old row's byte at +0x11 becomes the new grade, sign-extended.
            to.grade() = std::int16_t(c.i8(from + 0x11));
            to.story() = c.u32(from + 0x14);
            to.mark() = c.u8(from + 0x10);
            to.person() = c.u32(from + 8);
        }
        program.call(library::release, 0x10647368, {staging});
    }
    program.call(library::sort, 0x106473f8, {press.history(), std::uint32_t(press.history_count()), history_size, c.toc(slots::history_order)});
    program.call(library::close_file, 0x10647400, {frame.at(0x38)});
    return result(c, 1);
}

std::uint32_t save_press(Cpu& c, std::uint32_t self, std::uint32_t path) {
    Frame frame(c, 0x10647420, 0x140, {.first_gpr = 28});
    keep(c, {{31, c.toc(slot::message)}, {30, c.toc(slot::flag)}, {29, c.toc(slot::pool)}, {28, self}});
    const Press press{c, self};
    Program program{c};
    const DataFile file{c, frame};
    if (compare_logical(c, path) == 0) return fail(c, frame, {0x10647468, 0x10c, 0x64, 0x112});
    c.put32(frame.at(0x38), program.call(library::open_file, 0x106474c4, {path, c.toc(slot::pool), 1, 0x16, 4}));
    if (compare_logical(c, file.handle()) == 0) return fail(c, frame, {0x106474e8, 0xec, 0x5c, 0x11a});
    if (compare(c, file.transfer(library::write_file, 0x10647540, self + 0x14, 4, 1), 1) != 0)
        return fail(c, frame, {0x1064755c, 0xcc, 0x54, 0x121});
    const auto events_written = file.transfer(library::write_file, 0x106475b4, press.events(), event_size,
                                              std::uint32_t(press.event_count()));
    if (compare(c, press.event_count(), std::int32_t(events_written)) != 0)
        return fail(c, frame, {0x106475d4, 0xac, 0x4c, 0x129});
    if (compare(c, file.transfer(library::write_file, 0x1064762c, self + 8, 4, 1), 1) != 0)
        return fail(c, frame, {0x10647648, 0x8c, 0x44, 0x130});
    const auto rows_written = file.transfer(library::write_file, 0x106476a0, press.history(), history_size,
                                            std::uint32_t(press.history_count()));
    if (compare(c, press.history_count(), std::int32_t(rows_written)) != 0)
        return fail(c, frame, {0x106476c0, 0x6c, 0x3c, 0x138});
    program.call(library::close_file, 0x1064770c, {frame.at(0x38)});
    return result(c, 1);
}
}

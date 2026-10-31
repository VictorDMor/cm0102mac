#include "awol_detail.hpp"
#include <array>

namespace cm::aot::awol {
using namespace detail;
namespace {
// The flag buffer (one byte per staff member, cleared) and the three empty
// lists. r31 holds each new list across its constructor.
void create_storage(Cpu& c, std::uint32_t records, std::uint32_t back) {
    c.put32(records + flags_buffer, call(c, 0x10775200, back, {c.u32(c.r(31)), 1}));
    unsigned offset = 0xc;
    for (const auto member : {primary_list, secondary_list, pending_list}) {
        const auto list = call(c, 0x10773520, back + offset, {0x10});
        c.set(31, list);
        if (compare(c, std::int32_t(list)) != 0) c.call(list_construct, back + offset + 0xc);
        c.put32(records + member, c.r(31));
        offset += 0x18;
    }
}
void free_flags(Cpu& c, std::uint32_t records, std::uint32_t back) {
    c.set(3, c.u32(records + flags_buffer));
    if (compare_logical(c, c.r(3)) != 0) {
        c.call(0x10775170, back);
        c.put32(records + flags_buffer, 0);
    }
}
void destroy_list(Cpu& c, std::uint32_t records, std::uint32_t member, std::uint32_t back) {
    c.set(3, c.u32(records + member));
    if (compare_logical(c, c.r(3)) != 0) {
        call(c, list_destroy, back, {c.r(3), 1});
        c.put32(records + member, 0);
    }
}
}

std::uint32_t construct(Cpu& c, std::uint32_t records, std::uint32_t file) {
    Frame frame(c, 0x100031b0, 0x40, {.first_gpr = 30});
    const bool from_file = compare_logical(c, file) != 0;
    keep(c, {{31, c.toc(-0x7be4)}, {30, records}});
    for (const auto member : {flags_buffer, primary_list, secondary_list, pending_list}) c.put32(records + member, 0);
    if (from_file) {
        enter(c, 0x100031ec);
        c.set(3, load(c, records, file));
        if (compare(c, std::int8_t(c.r(3))) != 0) return result(c, records);
        // A failed load frees only the flags and the first list; the original
        // leaves any second or third list it had read in place.
        free_flags(c, records, 0x10003204);
        destroy_list(c, records, primary_list, 0x10003220);
        create_storage(c, records, 0x10003234);
    } else {
        create_storage(c, records, 0x10003290);
    }
    return result(c, records);
}

std::uint32_t destroy(Cpu& c, std::uint32_t records, std::uint32_t deleting) {
    Frame frame(c, 0x10003300, 0x40, {.first_gpr = 30});
    keep(c, {{31, deleting}, {30, records}});
    if (compare(c, std::int32_t(records)) != 0) {
        free_flags(c, records, 0x10003330);
        destroy_list(c, records, primary_list, 0x1000334c);
        destroy_list(c, records, secondary_list, 0x10003368);
        destroy_list(c, records, pending_list, 0x10003384);
        if (compare(c, std::int16_t(deleting)) > 0) call(c, 0x107735a0, 0x1000339c, {records});
    }
    return result(c, records);
}

std::uint32_t load(Cpu& c, std::uint32_t records, std::uint32_t file) {
    Frame frame(c, 0x100033c0, 0x80, {.first_gpr = 30});
    keep(c, {{31, records}, {30, file}});
    const auto stream = frame.at(0x40), size = frame.at(0x3c), count = frame.at(0x38);
    const auto close = [&](std::uint32_t back, std::uint32_t value) {
        call(c, 0x10747900, back, {stream});
        return result(c, value);
    };
    const auto read = [&](std::uint32_t into, std::uint32_t unit, std::uint32_t items, std::uint32_t back) {
        return call(c, 0x10747a80, back, {into, unit, items, c.u32(stream)});
    };
    call(c, date_construct, 0x100033e4, {frame.at(0x58)});
    call(c, date_construct, 0x100033ec, {frame.at(0x48)});
    call(c, date_construct, 0x100033f4, {frame.at(0x68)});
    if (compare_logical(c, c.r(30)) == 0) return result(c, 0);
    const auto opened = call(c, 0x107475b0, 0x1000341c, {c.r(30), c.toc(-0x5a08), 0, 0x16, 4});
    const bool have_stream = compare_logical(c, opened) != 0;
    c.put32(stream, opened);
    if (!have_stream) return result(c, 0);

    if (compare(c, std::int32_t(call(c, 0x10747a80, 0x10003444, {size, 4, 1, opened})), 1) != 0) return close(0x10003454, 0);
    c.set(3, c.u32(size));
    if (compare(c, std::int32_t(c.r(3))) == 0) return close(0x10003470, 0);
    c.call(0x10775130, 0x1000347c);
    c.put32(records + flags_buffer, c.r(3));
    c.set(3, c.u32(records + flags_buffer));
    if (compare_logical(c, c.r(3)) == 0) return close(0x10003494, 0);
    {
        const auto bytes = call(c, 0x10747a80, 0x100034ac, {c.r(3), 1, c.u32(size), c.u32(stream)});
        if (compare(c, c.i32(size), std::int32_t(bytes)) != 0) return close(0x100034c0, 0);
    }

    // Each list: its count, then fixed-size records appended one by one. The
    // record buffers overlap the dates constructed above, as in the original.
    struct ListFile { std::uint32_t member, record, bytes; std::array<std::uint32_t, 7> back; };
    static constexpr std::array<ListFile, 2> first_lists{{
        {primary_list, 0x54, 0xc, {0x100034dc, 0x100034ec, 0x100034fc, 0x10003508, 0x10003528, 0x10003538, 0x10003554}},
        {secondary_list, 0x44, 0x10, {0x10003578, 0x10003588, 0x10003598, 0x100035a4, 0x100035c4, 0x100035d4, 0x100035f0}},
    }};
    for (const auto& list : first_lists) {
        if (compare(c, std::int32_t(read(count, 4, 1, list.back[0])), 1) != 0) return close(list.back[1], 0);
        c.set(30, call(c, 0x10773520, list.back[2], {0x10}));
        if (compare(c, std::int32_t(c.r(30))) != 0) c.call(list_construct, list.back[3]);
        c.put32(records + list.member, c.r(30));
        c.set(30, 0);
        while (compare(c, std::int32_t(c.r(30)), c.i32(count)) < 0) {
            if (compare(c, std::int32_t(read(frame.at(list.record), list.bytes, 1, list.back[4])), 1) != 0) return close(list.back[5], 0);
            call(c, list_append, list.back[6], {c.u32(records + list.member), frame.at(list.record), list.bytes});
            c.set(30, c.r(30) + 1);
        }
    }

    c.set(30, call(c, 0x10773520, 0x10003608, {0x10}));
    if (compare(c, std::int32_t(c.r(30))) != 0) c.call(list_construct, 0x10003614);
    c.put32(records + pending_list, c.r(30));
    const auto version = call(c, 0x10775550, 0x10003624, {c.u32(c.toc(-0x7b74))});
    c.set(4, 0x40000);
    // Files before version 0x3e006 end after the second list.
    if (compare(c, std::int32_t(version), 0x3e005) > 0) {
        if (compare(c, std::int32_t(read(count, 4, 1, 0x10003648)), 1) != 0) return close(0x10003658, 0);
        c.set(30, 0);
        while (compare(c, std::int32_t(c.r(30)), c.i32(count)) < 0) {
            if (compare(c, std::int32_t(read(frame.at(0x60), 0x14, 1, 0x1000367c)), 1) != 0) return close(0x1000368c, 0);
            call(c, list_append, 0x100036a8, {c.u32(records + pending_list), frame.at(0x60), 0x14});
            c.set(30, c.r(30) + 1);
        }
    }
    return close(0x100036c0, 1);
}

std::uint32_t save(Cpu& c, std::uint32_t records, std::uint32_t file) {
    Frame frame(c, 0x100036e0, 0x260, {.first_gpr = 27});
    keep(c, {{29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {28, records}, {31, c.toc(-0x5a08)}, {27, c.toc(-0x7be4)}});
    const auto stream = frame.at(0x3c), count = frame.at(0x38);
    // The assert macro's expansion, on the pool and buffer the routine cached.
    const auto report = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
        const auto pool = c.r(31), buffer = c.r(30);
        call(c, 0x10739890, at + 4, {pool + 9, 0, 0, frame.at(name), frame.at(extension)});
        c.call(0x10768810, at + 8);
        c.set({{5, c.r(3)}, {3, buffer}, {4, pool + 0x12}, {6, frame.at(name)}, {7, frame.at(extension)}, {8, line}});
        c.call(0x1077b720, at + 0x24);
        c.set({{4, buffer}, {3, pool + 0x1f}, {5, 0}});
        c.call(0x10769b60, at + 0x34);
        c.put8(c.r(29), 0);
    };
    const auto fail_closing = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
        report(at, name, extension, line);
        call(c, 0x10747900, at + 0x44, {stream});
        return result(c, 0);
    };
    if (compare_logical(c, file) == 0) {
        report(0x10003720, 0x228, 0xa0, 0x132);
        return result(c, 0);
    }
    const auto opened = call(c, 0x107475b0, 0x1000377c, {file, c.r(31), 1, 0x16, 4});
    c.set(6, opened);
    c.put32(stream, opened);
    if (compare(c, std::int32_t(opened)) == 0) {
        report(0x1000379c, 0x208, 0x98, 0x13a);
        return result(c, 0);
    }
    if (compare(c, std::int32_t(call(c, 0x10747cc0, 0x100037f0, {c.r(27), 4, 1})), 1) != 0)
        return fail_closing(0x1000380c, 0x1e8, 0x90, 0x141);
    {
        const auto written = call(c, 0x10747cc0, 0x1000386c, {c.u32(records + flags_buffer), 1, c.u32(c.r(27)), c.u32(stream)});
        if (compare(c, c.i32(c.r(27)), std::int32_t(written)) != 0) return fail_closing(0x1000388c, 0x1c8, 0x88, 0x148);
    }
    struct Site { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
    struct ListFile { std::uint32_t member, bytes, count_back, next_back, write_back; Site count_failed, no_record, write_failed; };
    static constexpr std::array<ListFile, 3> lists{{
        {primary_list, 0xc, 0x100038f8, 0x10003978, 0x100039f4,
         {0x10003914, 0x1a8, 0x80, 0x151}, {0x10003994, 0x188, 0x78, 0x15a}, {0x10003a10, 0x168, 0x70, 0x160}},
        {secondary_list, 0x10, 0x10003a90, 0x10003b10, 0x10003b8c,
         {0x10003aac, 0x148, 0x68, 0x16a}, {0x10003b2c, 0x128, 0x60, 0x173}, {0x10003ba8, 0x108, 0x58, 0x179}},
        {pending_list, 0x14, 0x10003c28, 0x10003ca8, 0x10003d24,
         {0x10003c44, 0xe8, 0x50, 0x183}, {0x10003cc4, 0xc8, 0x48, 0x18c}, {0x10003d40, 0xa8, 0x40, 0x192}},
    }};
    for (const auto& list : lists) {
        const auto fail = [&](const Site& site) { return fail_closing(site.at, site.name, site.extension, site.line); };
        c.put32(count, std::uint32_t(std::int32_t(list_count(c, c.u32(records + list.member)))));
        if (compare(c, std::int32_t(call(c, 0x10747cc0, list.count_back, {count, 4, 1, c.u32(stream)})), 1) != 0) return fail(list.count_failed);
        c.set(3, c.u32(records + list.member));
        c.set(27, 0);
        rewind(c, c.r(3));
        for (;;) {
            c.set(3, c.u32(records + list.member));
            if (compare(c, std::int32_t(c.r(27)), list_count(c, c.r(3))) >= 0) break;
            c.call(list_next, list.next_back);
            if (compare_logical(c, c.r(3)) == 0) return fail(list.no_record);
            if (compare(c, std::int32_t(call(c, 0x10747cc0, list.write_back, {c.r(3), list.bytes, 1, c.u32(stream)})), 1) != 0) return fail(list.write_failed);
            c.set(27, c.r(27) + 1);
        }
    }
    call(c, 0x10747900, 0x10003da8, {stream});
    return result(c, 1);
}
}

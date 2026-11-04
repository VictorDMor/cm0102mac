#include "transfer_manager_construct.hpp"
#include "guest_values.hpp"
#include "transfer_manager_adapter.hpp"
#include "transfer_manager_approval_bootstrap.hpp"
#include "transfer_manager_load.hpp"
#include "transfer_manager_nations.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr std::uint32_t nation_initializer = 0x103fd420, load_section = 0x103df790, staff_initializer = 0x103ec6d0;

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

void media_name(Cpu& c, const Frame& frame, std::uint32_t left, std::uint32_t right,
                Kept<23>& left_cursor, Kept<25>& ready) {
    const auto left_copy = frame.at(0x58), right_copy = frame.at(0xc8), name = frame.at(0x124);
    const auto first = c.u32(right), second = c.u32(right + 4), third = c.u32(right + 8), fourth = c.u32(right + 12);
    const auto tail = c.u8(right + 16);
    const auto left_first = c.u32(left), left_second = c.u32(left + 4), left_third = c.u32(left + 8),
               left_fourth = c.u32(left + 12);
    auto source = right_copy + 1, destination = name + 1;
    c.set({{0, left_fourth}, {3, left_third}, {4, left_second}, {5, left_first}, {6, tail}, {7, fourth},
           {8, third}, {9, second}, {10, first}, {11, destination}, {12, source}});
    left_cursor = left_copy + 1;
    ready = 0;
    c.put32(right_copy, first);
    c.put32(right_copy + 4, second);
    c.put32(right_copy + 8, third);
    c.put32(right_copy + 12, fourth);
    c.put8(right_copy + 16, tail);
    c.put32(left_copy, left_first);
    c.put32(left_copy + 4, left_second);
    c.put32(left_copy + 8, left_third);
    c.put32(left_copy + 12, left_fourth);
    for (;;) {
        const auto ch = c.i8(source);
        c.set(0, std::uint32_t(ch));
        if (!nonzero(c, ch)) break;
        const auto first_byte = c.u8(source++);
        c.set({{3, first_byte}, {12, source}});
        const auto other = c.u8(left_cursor);
        left_cursor = left_cursor + 1;
        c.set(0, other);
        c.put8(destination, c.r(3));
        c.put8(destination + 1, other);
        destination += 2;
        c.set(11, destination);
    }
    const auto right_size = c.u8(right_copy);
    const auto total = right_size + c.u8(left_copy);
    c.set({{0, std::uint32_t(total)}, {7, right_size}});
    c.put8(name, total);
}

void confirm_media(Cpu& c, const Frame& frame, std::uint32_t left, std::uint32_t right, std::uint32_t pool) {
    Kept<23> left_cursor(c, c.r(23));
    Kept<25> ready(c, c.r(25));
    do {
        media_name(c, frame, left, right, left_cursor, ready);
        const auto error = call(c, 0x1077fee0, 0x103df1ec, {0, 0, frame.at(0x124), frame.at(0xdc)});
        c.set(2, c.u32(frame.at(0x14)));
        const auto status = std::int16_t(error);
        c.set(0, std::uint32_t(status));
        if (!nonzero(c, status)) ready = 1;
        if (!nonzero(c, ready)) {
            call(c, 0x10727890, 0x103df210, {frame.at(0x2ec), pool});
            call(c, 0x10727890, 0x103df21c, {frame.at(0x224), pool + 10});
            call(c, 0x10769b60, 0x103df22c, {frame.at(0x2ec), frame.at(0x224), 1});
        }
    } while (!nonzero(c, ready));
}

struct AllocationError {
    std::uint32_t back, name, extension, line, reason;
};

struct Construction {
    Cpu& c;
    const Frame& frame;
    const Field<std::uint32_t> manager;
    std::uint32_t pool, message, title, body;

    void report(const AllocationError& site) {
        const auto name = frame.at(site.name), extension = frame.at(site.extension), back = site.back;
        call(c, 0x10739890, back, {pool + 0x37, 0, 0, name, extension});
        const auto version = call(c, 0x10768810, back + 4);
        call(c, 0x1077b720, back + 0x24, {message, pool + 0x4c, version, name, extension, site.line, pool + site.reason});
        call(c, 0x10727890, back + 0x30, {title, pool + 0xa6});
        call(c, 0x10727890, back + 0x40, {body, pool + 0xbe, message});
        call(c, 0x10769b60, back + 0x50, {title, body, 0});
    }

    bool make_status_table(std::int32_t count_slot, std::uint32_t size, std::uint32_t field,
                           std::uint32_t back, const AllocationError& error) {
        const auto storage = call(c, 0x10775200, back, {c.u32(c.toc(count_slot)), size});
        c.set(4, manager);
        c.put32(manager + field, storage);
        c.set(3, manager);
        c.set(0, c.u32(manager + field));
        if (nonzero_logical(c, c.r(0))) return true;
        report(error);
        return false;
    }
};

using Initializer = void (*)(Cpu&, std::uint32_t);
void initialize(Cpu& c, std::uint32_t manager, std::uint32_t entry, std::uint32_t back, Initializer body) {
    c.set(3, manager);
    if (!transfer_manager_direct_calls_enabled()) {
        c.call(entry, back);
        return;
    }
    c.link(back);
    c.s.pc = entry;
    body(c, manager);
    c.finish();
}

bool load_existing(Cpu& c, std::uint32_t manager, std::uint32_t archive) {
    constexpr auto back = 0x103df49c;
    c.set({{3, manager}, {4, archive}});
    if (transfer_manager_direct_calls_enabled()) {
        c.link(back);
        c.s.pc = load_section;
        load_transfer_manager(c, manager, archive);
        c.finish();
    } else {
        c.call(load_section, back);
    }
    const auto value = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(value));
    return nonzero(c, value);
}
}

std::uint32_t construct_transfer_manager(Cpu& c, std::uint32_t manager, std::uint32_t archive) {
    Frame frame(c, 0x103df0d0, 0x3e0, {.first_gpr = 23});
    c.set(0, c.lr());
    const Kept<24> request(c, archive);
    const Kept<26> body(c, c.toc(-0x79e0));
    const Kept<27> title(c, c.toc(-0x79dc));
    const Kept<28> message(c, c.toc(-0x7ba8));
    const Kept<29> pool(c, c.toc(-0x4b9c));
    const Kept<30> left(c, c.toc(-0x4b98));
    const Kept<31> right(c, c.toc(-0x4b94));
    Construction construction{c, frame, {c, frame.caller() + 0x18}, pool, message, title, body};
    const auto& transfers = construction.manager;
    transfers = manager;
    call(c, 0x10773320, 0x103df11c, {transfers + 0x18, c.toc(-0x6214), c.toc(-0x6314), 16, 150});
    call(c, 0x10715880, 0x103df128, {transfers + 0x97c});
    call(c, 0x10738c00, 0x103df134, {transfers + 0x9a4});
    call(c, 0x10738c00, 0x103df140, {transfers + 0xa5c});
    confirm_media(c, frame, left, right, pool);

    constexpr std::uint32_t zero_fields[]{
        0, 8, 4, 0xc, 0x14, 0x10, 0x994, 0x99c, 0x998, 0x9b4, 0x9bc, 0x9b8,
        0x9c0, 0x9c8, 0x9c4, 0x9cc, 0x9d4, 0x9d0, 0xa50, 0xa58, 0xa54,
        0xa6c, 0xa70, 0xa74, 0xa78,
    };
    c.set(0, 0);
    for (const auto field : zero_fields) c.put32(transfers + field, 0);
    c.put8(transfers + 0x978, 0);
    c.set(6, transfers);
    const auto date = frame.at(0x38);
    call(c, 0x107156b0, 0x103df318, {date, 0, 0});
    c.set(0, 0);
    c.put16(transfers + 0x97e, c.u16(date + 2));
    c.put16(transfers + 0x97c, c.u16(date));
    const auto leap = c.u8(date + 4);
    c.set(4, leap);
    c.put8(transfers + 0x980, leap);
    c.put8(transfers + 0x984, 0);
    initialize(c, transfers, nation_initializer, 0x103df350, initialize_nation_transfer_records);

    if (!construction.make_status_table(-0x7be4, 10, 0x9a0, 0x103df360, {0x103df390, 0xa8, 0x50, 0xa3, 0x5e}) ||
        !construction.make_status_table(-0x7b50, 16, 0xa4c, 0x103df3f8, {0x103df428, 0x88, 0x48, 0xab, 0x1ce}))
        return result(c, transfers);
    c.set(0, 0xffffffff);
    const bool has_archive = nonzero_logical(c, request);
    c.put32(transfers + 0x990, 0xffffffff);
    if (has_archive && load_existing(c, transfers, request)) return result(c, transfers);

    const Kept<23> shortlist(c, call(c, 0x10773520, 0x103df4b4, {0xe0}));
    if (nonzero(c, shortlist)) call(c, 0x10356410, 0x103df4c4, {shortlist, 0});
    c.set(3, transfers);
    c.put32(transfers + 0x988, shortlist);
    c.set(3, transfers);
    c.set(0, c.u32(transfers + 0x988));
    if (nonzero_logical(c, c.r(0))) {
        initialize(c, transfers, staff_initializer, 0x103df550, initialize_staff_approvals);
    } else {
        construction.report({0x103df4f4, 0x68, 0x40, 0xc0, 0x215});
    }
    return result(c, transfers);
}
}

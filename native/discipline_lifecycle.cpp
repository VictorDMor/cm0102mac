// DISCIPLINE.CPP lifecycle block (discipline_lifecycle.hpp), native-first: the manager's constructor and
// destructor and the creation of its 42 ruling bodies. The load and save are in discipline_lifecycle_file.cpp.
#include "discipline_lifecycle.hpp"
#include "discipline_lifecycle_detail.hpp"
#include "discipline_adapter.hpp"

namespace cm::aot::discipline {
using guest::Cpu;
using guest::Frame;
namespace callee = lifecycle_callee;
namespace field = lifecycle_field;

namespace {
constexpr std::uint32_t folder_name_at = 0xcc, folder_spec_at = 0x84, dialog_text_at = 0x294, dialog_title_at = 0x1cc;
constexpr CheckSite history_missing{0x100de5c4, 0x50, 0x38, 0x7b};

// The Pascal name of the temp folder, built in the frame as the original builds it: the length byte is the sum
// of both strings' length bytes and the characters of the first string alternate with those of the second.
void build_folder_name(Cpu& c, const Frame& frame) {
    const auto odd = c.toc(lifecycle_toc::folder_odd), even = c.toc(lifecycle_toc::folder_even);  // the strings' addresses
    auto at = frame.at(std::int32_t(folder_name_at) + 1);
    for (std::uint32_t i = 1; c.i8(odd + i) != 0; ++i, at += 2) {
        c.put8(at, c.u8(odd + i));
        c.put8(at + 1, c.u8(even + i));
    }
    c.put8(frame.at(std::int32_t(folder_name_at)), c.u8(odd) + c.u8(even));
}

}

void construct_manager(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x100de410, 0x380, {.first_gpr = 25});
    c.put32(frame.caller() + 0x18, manager);  // the original keeps its argument in the caller's parameter area
    c.set(3, manager + manager::pending);
    c.call(callee::list_construct, 0x100de444);
    for (;;) {
        build_folder_name(c, frame);
        c.set({{3, 0}, {4, 0}, {5, frame.at(std::int32_t(folder_name_at))}, {6, frame.at(std::int32_t(folder_spec_at))}});
        c.call(callee::make_spec, 0x100de4f0);
        c.set(2, c.u32(frame.at(0x14)));
        if (std::int16_t(c.r(3)) == 0) break;
        // The folder is missing: ask for the CD, then look again.
        const auto pool = c.toc(lifecycle_toc::pool);
        c.set({{3, frame.at(std::int32_t(dialog_text_at))}, {4, pool}});
        c.call(callee::pascal_string, 0x100de514);
        c.set({{3, frame.at(std::int32_t(dialog_title_at))}, {4, pool + 10}});
        c.call(callee::pascal_string, 0x100de520);
        c.set({{3, frame.at(std::int32_t(dialog_text_at))}, {4, frame.at(std::int32_t(dialog_title_at))}, {5, 1}});
        c.call(callee::dialog, 0x100de530);
    }
    c.put32(manager + field::records, 0);
    c.put16(manager + field::record_capacity, 0);
    c.put16(manager + field::record_count, 0);
    c.put32(manager + field::index, 0);
    c.put16(manager + field::index_capacity, 0);
    c.put16(manager + field::index_count, 0);
    const auto pool = c.toc(lifecycle_toc::pool);
    c.set(3, 8);
    c.call(callee::object_allocate, 0x100de574);
    const auto history = c.r(3);
    if (history != 0) {
        c.set({{3, pool + 0x37}, {4, pool + 0x40}});
        c.call(callee::history_name, 0x100de588);
        c.set({{4, c.r(3)}, {3, history}, {5, 0x18}});
        c.call(callee::history_construct, 0x100de598);
    }
    c.put32(manager + manager::history, history);
    if (c.u32(manager + manager::history) == 0) return report_check(c, frame, history_missing);
    if (file != 0) {
        c.link(0x100de61c);
        if (std::int8_t(load_manager(c, manager, file)) != 0) return;
    }
    c.set(3, manager);
    call(c, callee::table_allocate, 0x100de634);
    c.link(0x100de640);
    create_bodies(c, manager, 0);
}

void destroy_manager(Cpu& c, std::uint32_t manager, std::int16_t deleting) {
    Frame frame(c, 0x100de660, 0x50, {.first_gpr = 28});
    if (manager == 0) return;
    const auto release = [&](std::uint32_t at, std::uint32_t back) {
        if (c.u32(at) == 0) return;
        c.set(3, c.u32(at));
        c.call(callee::free, back);
    };
    release(manager + field::records, 0x100de698);
    release(manager + field::index, 0x100de6a8);
    release(manager + manager::people, 0x100de6b8);
    for (std::int8_t index = 0; index < manager::body_count; ++index) {
        const auto body = body_object(c, manager, std::uint32_t(index));
        if (body == 0) continue;
        c.set({{3, body}, {4, 1}});
        c.call(callee::body_destroy, 0x100de6dc);
    }
    release(manager + manager::bodies, 0x100de700);
    if (c.u32(manager + manager::history) != 0) {
        c.set({{3, c.u32(manager + manager::history)}, {4, 1}});
        c.call(callee::history_destroy, 0x100de714);
    }
    c.set({{3, manager + manager::pending}, {4, 0xffffffffu}});
    c.call(callee::list_destroy, 0x100de720);
    if (deleting > 0) {
        c.set(3, manager);
        c.call(callee::object_delete, 0x100de730);
    }
}
}

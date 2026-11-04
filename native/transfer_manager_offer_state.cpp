#include "transfer_manager_offer_state.hpp"
#include "guest_values.hpp"
#include "transfer_manager_adapter.hpp"
#include "transfer_manager_collections.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr auto filter_entry = 0x103fb390, list_next = 0x10738e00;

struct OfferIndexDiagnostic {
    std::uint32_t back, line;
    bool negative;
};

void report_invalid_offer_index(Cpu& c, const Frame& frame, const OfferIndexDiagnostic& error) {
    const auto pool = c.r(31), message = c.r(30), valid = c.r(29);
    const auto name = frame.at(error.negative ? 0x68 : 0x48), extension = frame.at(error.negative ? 0x40 : 0x38);
    c.set({{3, pool + 0x37}, {4, 0}, {5, 0}, {6, name}, {7, extension}});
    c.call(0x10739890, error.back);
    c.call(0x10768810, error.back + 4);
    const auto version = c.r(3);
    c.set({{3, message}, {4, pool + 0x6ff}, {5, version}, {6, name}, {7, extension}, {8, error.line}});
    c.call(0x1077b720, error.back + 0x20);
    c.set({{3, pool + 0x70c}, {4, message}, {5, 0}});
    c.call(0x10769b60, error.back + 0x30);
    c.set(0, 0);
    c.put8(valid, 0);
}

bool invalid_offer_index(Cpu& c, const Frame& frame, std::uint32_t manager, std::int32_t index,
                         std::uint32_t split_return, std::uint32_t line) {
    if (compare(c, index) < 0) {
        report_invalid_offer_index(c, frame, {split_return, line, true});
        return true;
    }
    const auto count = c.i32(manager + 4);
    c.set(0, std::uint32_t(count));
    if (compare(c, index, count) < 0) return false;
    report_invalid_offer_index(c, frame, {split_return + 0x60, line + 5, false});
    return true;
}

std::uint32_t indexed_offer(Cpu& c, std::uint32_t manager, std::uint32_t index) {
    const auto array = c.u32(manager);
    const auto offset = index * 4;
    c.set({{0, offset}, {3, array}});
    return c.u32(array + offset);
}

void set_response_kind(Cpu& c, std::uint32_t offer, std::uint32_t other_response, std::uint32_t response) {
    const auto kind = c.u8(offer + 0x34);
    c.set(0, kind);
    const auto updated = compare(c, kind, other_response) == 0 ? 8 : response;
    c.set(0, updated);
    c.put8(offer + 0x34, updated);
}

void reset_cursor(Cpu& c, std::uint32_t list) {
    const auto head = c.u32(list + 4);
    c.set({{0, head}, {3, list}});
    c.put32(list + 0xc, head);
}

std::uint32_t next_row(Cpu& c, std::uint32_t list, std::uint32_t back) {
    c.set(3, list);
    c.call(list_next, back);
    const auto row = c.r(3);
    compare_logical(c, row);
    return row;
}

bool is_inactive(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t back) {
    c.set({{3, manager}, {4, offer}});
    if (transfer_manager_direct_calls_enabled()) {
        c.link(back);
        c.s.pc = filter_entry;
        offer_is_filtered(c, offer);
        c.finish();
    } else {
        c.call(filter_entry, back);
    }
    const auto inactive = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(inactive));
    return nonzero(c, inactive);
}
}

void mark_offer_club_response(Cpu& c, std::uint32_t manager, std::int32_t index) {
    Frame frame(c, 0x103e6e30, 0xa0, {.first_gpr = 29});
    c.set(0, c.lr());
    const Kept<29> valid(c, c.toc(-0x7bac));
    const Kept<30> message(c, c.toc(-0x7ba8));
    const Kept<31> pool(c, c.toc(-0x4b9c));
    if (invalid_offer_index(c, frame, manager, index, 0x103e6e74, 0xd91)) return;
    const auto offer = result(c, indexed_offer(c, manager, index));
    if (!nonzero_logical(c, offer)) return;
    const auto closed = c.u8(offer + 0x24);
    c.set(0, closed);
    if (compare(c, closed, 1) != 0) set_response_kind(c, offer, 6, 7);
}

void mark_offer_staff_response(Cpu& c, std::uint32_t manager, std::int32_t index, std::int32_t staff) {
    Frame frame(c, 0x103e6f70, 0xa0, {.first_gpr = 27});
    c.set(0, c.lr());
    const Kept<27> staff_id(c, std::uint32_t(staff));
    Kept<28> no_responses(c, 1);
    Kept<29> offer(c, c.toc(-0x7bac));
    const Kept<30> message(c, c.toc(-0x7ba8));
    Kept<31> pool(c, c.toc(-0x4b9c));
    if (invalid_offer_index(c, frame, manager, index, 0x103e6fb4, 0xdc3)) return;
    offer = indexed_offer(c, manager, index);
    if (!nonzero_logical(c, offer)) return;
    pool = 1;
    reset_cursor(c, c.u32(offer + 4));
    while (const auto row = next_row(c, c.u32(offer + 4), 0x103e70a0)) {
        const auto id = c.i32(row);
        c.set(0, std::uint32_t(id));
        if (compare(c, id, staff) == 0) c.put8(row + 4, pool);
        const auto response = c.i8(row + 4);
        c.set(0, std::uint32_t(response));
        if (nonzero(c, response)) no_responses = 0;
    }
    c.set(0, no_responses);
    if (nonzero(c, no_responses)) set_response_kind(c, offer, 7, 6);
}

void clear_inactive_offer_flag(Cpu& c, std::uint32_t manager, std::uint32_t offer) {
    Frame frame(c, 0x103f6190, 0x40, {.first_gpr = 30});
    c.set(0, c.lr());
    const Kept<30> transfers(c, manager);
    const Kept<31> current(c, offer);
    if (!is_inactive(c, transfers, current, 0x103f61b0)) return;
    const auto list = result(c, c.u32(current));
    if (nonzero_logical(c, list)) {
        reset_cursor(c, list);
        while (const auto row = next_row(c, c.u32(current), 0x103f61fc)) {
            const auto linked = indexed_offer(c, transfers, c.u32(row));
            if (!is_inactive(c, transfers, linked, 0x103f61ec)) return;
        }
    }
    const auto staff = c.u32(current + 0xc);
    const auto statuses = c.u32(transfers + 0x9a0);
    const auto offset = staff * 10;
    const auto status = c.u8(statuses + offset);
    c.set({{0, std::uint32_t(status & 1)}, {3, status}, {4, offset}, {5, statuses}});
    if (nonzero(c, status & 1)) {
        c.set(0, status & 0xfe);
        c.put8(statuses + offset, status & 0xfe);
    }
}
}

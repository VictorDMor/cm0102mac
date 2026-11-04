#include "transfer_manager_callbacks.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

struct DateRecord {
    Cpu& cpu;
    std::uint32_t address;

    std::uint16_t year() const { return cpu.u16(address + 2); }
    std::int16_t day() const { return cpu.i16(address); }
    std::uint32_t day_and_year() const { return cpu.u32(address); }
    std::uint32_t tail() const { return cpu.u32(address + 4); }
};

// The equal-year comparison forms half the signed day difference without
// overflowing. Its shifted xor can survive in an argument register.
std::int32_t day_difference(Cpu& c, std::int16_t first, std::int16_t second, unsigned scratch) {
    const auto difference_bits = std::int32_t(first) ^ std::int32_t(second);
    const auto shifted = difference_bits >> 1;
    c.set(scratch, std::uint32_t(shifted));
    c.s.wr<1>(reg::xer_ca, difference_bits < 0 && (difference_bits & 1));
    return shifted - (difference_bits & first);
}

struct DateCopy {
    Cpu& cpu;
    std::uint32_t first, spare, second;

    struct First {
        std::uint32_t head;
        std::uint16_t left_year, right_year;
    };

    First retain_first(const DateRecord& left, const DateRecord& right) const {
        const auto head = right.day_and_year();
        const auto tail = right.tail();
        cpu.put32(first, head);
        const auto left_year = left.year();
        const auto right_year = DateRecord{cpu, first}.year();
        cpu.put32(spare, head);
        compare_logical(cpu, right_year, left_year);
        cpu.put32(spare + 4, tail);
        cpu.put32(first + 4, tail);
        return {head, left_year, right_year};
    }

    DateRecord copy_again() const {
        const DateRecord saved{cpu, spare};
        const auto head = saved.day_and_year();
        const auto tail = saved.tail();
        cpu.put32(second, head);
        cpu.put32(second + 4, tail);
        return {cpu, second};
    }
};
}

std::uint32_t compare_priority_and_index(Cpu& c, std::uint32_t left, std::uint32_t right) {
    const auto right_priority = c.u32(right + 0xc);
    const auto difference = c.u32(left + 0xc) - right_priority;
    c.set({{5, right_priority}, {0, difference}});
    if (nonzero(c, std::int32_t(difference))) return result(c, difference);
    return compare_index(c, left + 4, right + 4);
}

std::uint32_t compare_index(Cpu& c, std::uint32_t left, std::uint32_t right) {
    const auto right_index = c.u32(right);
    const auto left_index = c.u32(left);
    c.set({{4, right_index}, {0, left_index}});
    return result(c, left_index - right_index);
}

std::uint32_t compare_ranked_transfers(Cpu& c, std::uint32_t left, std::uint32_t right) {
    const auto left_rank = c.i32(left + 0x18), right_rank = c.i32(right + 0x18);
    c.set({{5, std::uint32_t(left_rank)}, {0, std::uint32_t(right_rank)}});
    const auto order = compare(c, left_rank, right_rank);
    if (order != 0) return result(c, std::uint32_t(-order));
    const auto left_kind = c.i8(left + 0x1c), right_kind = c.i8(right + 0x1c);
    c.set({{0, std::uint32_t(left_kind)}, {4, std::uint32_t(right_kind)}});
    if (compare(c, left_kind, right_kind) == 0) return result(c, 0);
    if (compare(c, left_kind, 2) == 0) return result(c, 1);
    // A type-2 offer sorts after the other kinds for tied ranks. Distinct
    // other kinds compare equal, and keep the comparator's volatile residue.
    const auto right_type = std::uint32_t(right_kind);
    c.set(0, ~((right_type - 2) | (2 - right_type)));
    c.s.wr<1>(reg::xer_ca, right_kind == 2);
    return result(c, right_kind == 2 ? 0xffffffff : 0);
}

std::uint32_t compare_linked_dates(Cpu& c, std::uint32_t left_slot, std::uint32_t right_slot) {
    const auto right_record = c.u32(right_slot);
    const auto left_record = c.u32(left_slot);
    const DateRecord left{c, left_record + 0x20}, right{c, right_record + 0x20};
    const DateCopy copy{c, c.sp() - 8, c.sp() - 0x18, c.sp() - 0x10};
    const auto [right_head, left_year, right_year] = copy.retain_first(left, right);
    c.set({{4, left_year}, {5, left_record}, {6, right_year}});
    auto order = (right_year > left_year) - (right_year < left_year);
    if (order == 0) {
        const auto right_day = DateRecord{c, copy.first}.day();
        c.set(4, std::uint32_t(right_day));
        order = -day_difference(c, right_day, left.day(), 3);
    }
    if (nonzero(c, order > 0)) {
        c.set(0, 1);
        return result(c, 0xffffffff);
    }

    const auto again = copy.copy_again();
    const auto next_left_year = left.year(), next_right_year = again.year();
    c.set(4, next_right_year);
    order = compare_logical(c, next_right_year, next_left_year);
    if (order == 0) {
        const auto left_day = left.day();
        c.set(4, std::uint32_t(left_day));
        order = day_difference(c, left_day, again.day(), 3);
    }
    c.set(0, order < 0 ? 0xffffffff : 0);
    return result(c, order < 0);
}

std::uint32_t compare_dated_entries(Cpu& c, std::uint32_t left_address, std::uint32_t right_address) {
    const DateRecord left{c, left_address}, right{c, right_address};
    const DateCopy copy{c, c.sp() - 8, c.sp() - 0x18, c.sp() - 0x10};
    const auto [right_head, left_year, right_year] = copy.retain_first(left, right);
    c.set({{4, right_year}, {5, right_head}, {6, left_year}});
    auto order = (right_year > left_year) - (right_year < left_year);
    if (order == 0) {
        const auto right_day = DateRecord{c, copy.first}.day();
        c.set(5, std::uint32_t(right_day));
        order = -day_difference(c, right_day, left.day(), 4);
    }
    if (nonzero(c, order > 0)) {
        c.set(0, 1);
        return result(c, 0xffffffff);
    }

    const auto again = copy.copy_again();
    const auto next_right_year = again.year();
    c.set(4, next_right_year);
    order = compare_logical(c, next_right_year, left_year);
    if (order == 0) {
        const auto left_day = left.day();
        c.set(4, std::uint32_t(left_day));
        order = day_difference(c, left_day, again.day(), 3);
    }
    c.set(0, order < 0 ? 0xffffffff : 0);
    return result(c, order < 0);
}

std::uint32_t compare_linked_priorities(Cpu& c, std::uint32_t left_slot, std::uint32_t right_slot) {
    const auto left = c.u32(left_slot), right = c.u32(right_slot);
    const auto left_priority = c.i8(left + 4), right_priority = c.i8(right + 4);
    c.set({{4, right}, {5, left}, {0, std::uint32_t(right_priority)}});
    if (compare(c, left_priority, right_priority) != 0)
        return result(c, std::uint32_t(left_priority - right_priority));
    const auto right_kind = c.i8(right + 3), left_kind = c.i8(left + 3);
    c.set(0, std::uint32_t(left_kind));
    return result(c, std::uint32_t(left_kind - right_kind));
}

std::uint32_t compare_optional_index(Cpu& c, std::uint32_t left, std::uint32_t right) {
    const auto left_index = c.i32(left);
    c.set(5, std::uint32_t(left_index));
    if (compare(c, left_index, -1) == 0) {
        const auto right_index = c.i32(right);
        c.set(0, std::uint32_t(right_index));
        if (compare(c, right_index, -1) == 0) return result(c, 0);
        compare(c, left_index, -1);
        return result(c, 1);
    }
    const auto right_index = c.i32(right);
    c.set(4, std::uint32_t(right_index));
    if (compare(c, right_index, -1) == 0) return result(c, 0xffffffff);
    const auto left_value = c.u32(left);
    c.set(0, left_value);
    return result(c, left_value - std::uint32_t(right_index));
}

std::uint32_t compare_ranked_staff(Cpu& c, std::uint32_t left_address, std::uint32_t right_address) {
    Frame frame(c, 0x10411cd0, 0x60, {});
    const auto right_rank = c.i8(right_address + 0x19), left_rank = c.i8(left_address + 0x19);
    const auto rank_order = left_rank - right_rank;
    c.set(5, std::uint32_t(right_rank));
    if (nonzero(c, rank_order)) return result(c, std::uint32_t(rank_order));

    const DateRecord left{c, left_address}, right{c, right_address};
    const DateCopy copy{c, frame.at(0x50), frame.at(0x40), frame.at(0x48)};
    const auto [right_head, left_year, right_year] = copy.retain_first(left, right);
    c.set({{5, right_head}, {6, right_year}, {7, left_year}});
    auto order = (right_year > left_year) - (right_year < left_year);
    if (order == 0) {
        const auto right_day = DateRecord{c, copy.first}.day();
        c.set(6, std::uint32_t(right_day));
        order = -day_difference(c, right_day, left.day(), 5);
    }
    if (nonzero(c, order > 0)) return result(c, 0xffffffff);

    const auto again = copy.copy_again();
    const auto next_right_year = again.year();
    c.set(5, next_right_year);
    order = compare_logical(c, next_right_year, left_year);
    if (order == 0) {
        const auto left_day = left.day();
        c.set(6, std::uint32_t(left_day));
        order = day_difference(c, left_day, again.day(), 5);
    }
    if (nonzero(c, order < 0)) return result(c, 1);

    constexpr std::int32_t staff_table_slot = -0x7be0;
    constexpr std::uint32_t staff_size = 0x70, compare_staff_names = 0x100cd6b0;
    const auto left_index = c.u32(left_address + 8);
    const auto staff_table = c.u32(c.toc(staff_table_slot));
    c.put32(frame.at(0x3c), staff_table + left_index * staff_size);
    const auto right_staff = staff_table + c.u32(right_address + 8) * staff_size;
    c.put32(frame.at(0x38), right_staff);
    c.set({{0, right_staff}, {3, frame.at(0x3c)},
           {4, frame.at(0x38)}, {5, staff_table}});
    c.call(compare_staff_names, 0x10411dfc);
    return c.r(3);
}

void initialize_record_sentinels(Cpu& c, std::uint32_t destination) {
    Frame frame(c, 0x103f9780, 0x40, {});
    c.set({{3, destination}, {4, 0xffffffff}, {5, 0x2c}});
    c.call(0x10778300, 0x103f979c);
}
}

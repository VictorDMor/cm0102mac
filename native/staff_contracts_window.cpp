#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using guest::Cpu;
using guest::compare;
using guest::compare_logical;

constexpr unsigned frame_size = 0x70;
constexpr std::uint32_t initialize_date = 0x10715880, date_interval = 0x10717dd0, staff_age = 0x100c6a10;
constexpr std::int32_t reference_date = -0x79e8, today_slot = -0x7b80, enabled_slot = -0x7a68;
constexpr std::int32_t staff_array_slot = -0x7be0;
constexpr unsigned staff_record_size = 0x70;

bool reference_after_start(Cpu& c, std::uint32_t contract, std::uint32_t frame) {
    block(c, 0x10380f70, 9);
    const auto reference = c.toc(reference_date);
    c.set(4, reference);
    const auto start_year = c.u16(contract + ContractRecord::start_date + 2);
    c.set(5, start_year);
    const auto date_word = c.u32(reference);
    c.set(3, date_word);
    const auto date_tail = c.u32(reference + 4);
    c.set(0, date_tail);
    c.put32(frame + 0x38, date_word);
    const auto year = c.u16(frame + 0x3a);
    c.set(3, year);
    c.put32(frame + 0x3c, date_tail);
    compare_logical(c, year, start_year);
    bool after;
    if (year > start_year) {
        block(c, 0x10380f94, 2);
        after = true;
    } else {
        block(c, 0x10380f9c, 1);
        if (year < start_year) {
            block(c, 0x10380fa0, 2);
            after = false;
        } else {
            block(c, 0x10380fa8, 8);
            const auto day = c.i16(frame + 0x38);
            c.set(4, guest::extended(day));
            const auto start_day = c.i16(contract + ContractRecord::start_date);
            const std::int32_t different = day ^ start_day;
            c.set(3, std::uint32_t(different >> 1));
            c.s.wr<1>(guest::reg::xer_ca, different < 0 && (different & 1));
            after = day > start_day;
        }
    }
    c.set(0, after);
    block(c, 0x10380fc8, 2);
    const auto overflow = c.s.r<1>(guest::reg::xer_so) & 1;
    c.set_cr(0, std::uint8_t((c.cr(0) & 0xf0) | (after ? 4 : 2) | overflow));
    return after;
}

void measure_interval(Cpu& c, std::uint32_t contract, std::uint32_t today, std::uint32_t output,
                      std::uint32_t pc, std::uint32_t back) {
    block(c, pc, 5);
    c.set(5, c.u32(contract + ContractRecord::start_date));
    c.set(4, today);
    c.set(6, c.u32(contract + ContractRecord::start_date + 4));
    c.set(3, output);
    c.call(date_interval, back);
}

void update_age_threshold(Cpu& c, std::uint32_t contract, std::uint32_t frame,
                          const guest::Kept<30>& output) {
    block(c, 0x10380fec, 5);
    const auto flags = c.u8(frame + 0x4c);
    c.set({{0, flags}, {4, contract + ContractRecord::start_date}});
    const auto years = c.u16(frame + 0x4a);
    c.set(3, years);
    const auto days = c.i16(frame + 0x48);
    c.set(5, guest::extended(days));
    c.put16(frame + 0x52, years);

    block(c, 0x10381000, 8);
    const auto array_slot = c.toc(staff_array_slot);
    c.set(3, array_slot);
    c.put16(frame + 0x50, guest::extended(days));
    const auto staff = c.u32(array_slot);
    c.set(3, staff);
    c.put8(frame + 0x54, flags);
    const auto displacement = c.u32(contract) * staff_record_size;
    c.set({{0, displacement}, {3, staff + displacement}});
    c.call(staff_age, 0x10381020);

    block(c, 0x10381020, 9);
    const auto age = std::int8_t(c.r(3));
    const bool younger = age < 28;
    c.set(4, std::uint32_t(age));
    compare_logical(c, output, 0);
    c.set({{0, std::uint32_t(age) + 0x80000000u}, {3, younger ? 0u : UINT32_MAX}, {27, younger ? 3u : 2u}});
    c.s.wr<1>(guest::reg::xer_ca, younger);
    if (output) {
        block(c, 0x10381044, 1);
        c.put8(output, c.r(27));
    }
}

bool interval_in_window(Cpu& c, std::uint32_t frame, const guest::Kept<29>& boundary) {
    block(c, 0x1038105c, 10);
    const auto interval_years = c.u16(frame + 0x42);
    c.set(3, interval_years);
    const auto threshold = std::int8_t(c.r(27));
    c.set(5, std::uint32_t(threshold));
    const auto flags = c.u8(frame + 0x44);
    c.set(0, flags);
    c.put16(frame + 0x52, interval_years);
    const auto elapsed_days = c.i16(frame + 0x40);
    c.set(3, guest::extended(elapsed_days));
    const auto stored_years = c.u16(frame + 0x52);
    c.set(4, stored_years);
    c.put16(frame + 0x50, guest::extended(elapsed_days));
    const bool in_window = compare(c, stored_years, threshold) < 0;
    c.put8(frame + 0x54, flags);
    if (in_window) {
        block(c, 0x10381084, 2);
        c.set(3, 1);
        return true;
    }

    block(c, 0x1038108c, 2);
    if (compare_logical(c, boundary, 0) != 0) {
        block(c, 0x10381094, 2);
        if (compare(c, threshold, stored_years) == 0) {
            block(c, 0x1038109c, 3);
            const auto days = c.i16(frame + 0x50);
            c.set(0, guest::extended(days));
            if (compare(c, days) == 0) {
                block(c, 0x103810a8, 2);
                if (compare_logical(c, boundary, 0) != 0) {
                    block(c, 0x103810b0, 2);
                    c.set(0, 1);
                    c.put8(boundary, 1);
                }
            }
        }
    }
    block(c, 0x103810b8, 1);
    c.set(3, 0);
    return false;
}
}

bool query_start_date_window(Cpu& c, std::uint32_t contract, std::uint32_t exact_boundary,
                             std::uint32_t age_threshold) {
    block(c, 0x10380f20, 10);
    const auto stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    detail::transfer_saved_registers<27>(c, stack);
    const guest::Kept<31> today(c, c.toc(today_slot));
    const guest::Kept<28> row(c, contract);
    const guest::Kept<29> boundary(c, exact_boundary);
    const guest::Kept<30> threshold_output(c, age_threshold);
    c.put32(stack + 8, link);
    const auto frame = stack - frame_size;
    c.put32(frame, stack);
    c.set({{1, frame}, {3, frame + 0x50}});
    c.call(initialize_date, 0x10380f48);

    block(c, 0x10380f48, 2);
    if (compare_logical(c, boundary, 0) != 0) {
        block(c, 0x10380f50, 2);
        c.set(0, 0);
        c.put8(boundary, 0);
    }

    block(c, 0x10380f58, 4);
    const auto flag = c.toc(enabled_slot);
    c.set(3, flag);
    const auto enabled = c.u8(flag);
    c.set(0, enabled);
    bool in_window = true;
    if (compare(c, enabled, 1) != 0) {
        block(c, 0x10380f68, 2);
        c.set(3, 1);
    } else if (reference_after_start(c, row, frame)) {
        block(c, 0x10380fd0, 2);
        c.set(3, 1);
    } else {
        measure_interval(c, row, today, frame + 0x48, 0x10380fd8, 0x10380fec);
        update_age_threshold(c, row, frame, threshold_output);
        measure_interval(c, row, today, frame + 0x40, 0x10381048, 0x1038105c);
        in_window = interval_in_window(c, frame, boundary);
    }

    block(c, 0x103810bc, 5);
    const auto restored_stack = c.sp() + frame_size;
    const auto restored_link = c.u32(restored_stack + 8);
    c.set({{0, restored_link}, {1, restored_stack}});
    c.set_lr(restored_link);
    detail::transfer_saved_registers<27, true>(c, restored_stack);
    return in_window;
}
}

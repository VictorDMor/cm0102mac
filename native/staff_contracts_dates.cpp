#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using guest::Cpu;

constexpr std::uint32_t construct_date = 0x10715880;
constexpr std::uint32_t date_difference = 0x10718130;
constexpr std::int32_t today_slot = -0x7b80;
constexpr unsigned date_frame_size = 0x40;

void leave_date_frame(Cpu& c) {
    const auto caller_stack = c.sp() + date_frame_size;
    const auto link = c.u32(caller_stack + 8);
    c.set({{0, link}, {1, caller_stack}});
    c.set_lr(link);
}
}

void initialize_draft(Cpu& c, std::uint32_t contract) {
    block(c, 0x103721f0, 7);
    const auto caller_stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    c.put32(caller_stack - 4, c.r(31));
    const guest::Kept<31> draft(c, contract);
    c.put32(caller_stack + 8, link);
    c.set(3, draft + ContractRecord::start_date);
    c.put32(caller_stack - date_frame_size, caller_stack);
    c.set(1, caller_stack - date_frame_size);
    c.call(construct_date, 0x1037220c);

    block(c, 0x1037220c, 2);
    c.set(3, draft + ContractRecord::end_date);
    c.call(construct_date, 0x10372214);

    // The constructor initializes selected fields. It leaves bonus slots,
    // the primary staff ID and the gaps between the date fields untouched.
    block(c, 0x10372214, 16);
    c.set({{4, UINT32_MAX}, {0, 0}});
    c.put32(draft + ContractRecord::club_id, UINT32_MAX);
    c.set(3, draft);
    c.put8(draft + ContractRecord::terms_flag, 0);
    c.put32(draft + ContractRecord::terms_marker, UINT32_MAX);
    c.put16(draft + ContractRecord::review_marker, 0);
    c.put8(draft + ContractRecord::review_flags, 0);
    c.put32(draft + ContractRecord::status, 0);
    c.put32(draft + ContractRecord::review_conditions, 0);
    c.put32(draft + ContractRecord::draft_auxiliary, 0);
    leave_date_frame(c);
    c.set(31, c.u32(c.sp() - 4));
}

std::int16_t end_date_delta(Cpu& c, std::uint32_t contract) {
    block(c, 0x10374d10, 8);
    const auto caller_stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    const auto today = c.toc(today_slot);
    c.set(5, today);
    c.put32(caller_stack + 8, link);
    c.set(3, contract + ContractRecord::end_date);
    c.put32(caller_stack - date_frame_size, caller_stack);
    c.set(1, caller_stack - date_frame_size);
    c.set(4, c.u32(today));
    c.set(5, c.u32(today + 4));
    c.call(date_difference, 0x10374d30);

    block(c, 0x10374d30, 5);
    const auto days = std::int16_t(c.r(3));
    c.set(3, guest::extended(days));
    leave_date_frame(c);
    return days;
}
}

#include "staff_contracts.hpp"
#include "staff_contracts_adapter.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {

namespace {
using detail::block;
using detail::ContractRecord;
using guest::Cpu;

constexpr std::uint32_t reset_entry = 0x1037f050, clear_one_entry = 0x1037f7d0;
constexpr std::uint32_t clear_all_entry = 0x1037f830, refresh_entry = 0x1037f590;
constexpr std::uint32_t reset_status_entry = 0x1037fa60;
constexpr std::int32_t staff_table_slot = -0x7be0;
constexpr unsigned staff_record_size = 0x70;
constexpr unsigned clearance_counter = 0x4c, review_grade = 0x60;
constexpr unsigned league_context = 0x2c, reputation_category = 0x119;

void restore_review_frame(Cpu& c, unsigned size, bool saves_r29 = false) {
    const auto stack = c.sp() + size;
    const auto link = c.u32(stack + 8);
    c.set({{0, link}, {1, stack}});
    c.set_lr(link);
    c.set(31, c.u32(stack - 4));
    c.set(30, c.u32(stack - 8));
    if (saves_r29) c.set(29, c.u32(stack - 12));
}

void call_clear_review_conditions(Cpu& c, std::uint32_t contract) {
    constexpr auto back = 0x1037f088;
    if (!staff_contracts_calls_directly()) {
        c.call(clear_all_entry, back);
        return;
    }
    c.link(back);
    clear_review_conditions(c, contract, 0);
    c.finish();
    if (c.s.pc != back) run_until(c.s, back);
}

void call_reset_status(Cpu& c, std::uint32_t contract) {
    constexpr auto back = 0x1037f090;
    c.set(3, contract);
    if (!staff_contracts_calls_directly()) {
        c.call(reset_status_entry, back);
        return;
    }
    c.link(back);
    reset_status_bits(c, contract);
    c.finish();
    if (c.s.pc != back) run_until(c.s, back);
}
}

void reset_review_state(Cpu& c, std::uint32_t contract, std::uint32_t staff, std::uint32_t club) {
    block(c, reset_entry, 14);
    GuestNativeScope scope(c.s, reset_entry);
    const auto stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    c.put32(stack - 4, c.r(31));
    const guest::Kept<31> employer(c, club);
    c.put32(stack - 8, c.r(30));
    const guest::Kept<30> person(c, staff);
    c.set(4, 0);
    c.put32(stack - 12, c.r(29));
    const guest::Kept<29> record(c, contract);
    c.put32(stack + 8, link);
    c.set(0, 0);
    c.put32(stack - 0x50, stack);
    c.set(1, stack - 0x50);
    scope.frame(c.sp());

    c.put8(contract + clearance_counter, 0);
    c.put8(contract + ContractRecord::review_flags, 0);
    call_clear_review_conditions(c, contract);
    block(c, 0x1037f088, 2);
    call_reset_status(c, record);
    block(c, 0x1037f090, 7);
    c.set({{3, 4}, {0, 0}});
    c.put8(record + ContractRecord::review_flags, 4);
    const bool has_employer = guest::nonzero_logical(c, employer);
    c.put8(record + 0x46, 0);
    c.put8(record + 0x45, 0);
    if (has_employer) {
        block(c, 0x1037f0ac, 3);
        const auto league = c.u32(employer + league_context);
        c.set(5, league);
        if (guest::nonzero_logical(c, league)) {
            block(c, 0x1037f0b8, 7);
            const auto service_slot = c.toc(-0x7b3c);
            c.set({{3, service_slot}, {4, person}});
            c.set(6, c.u8(league + reputation_category));
            c.set(5, employer);
            c.set(3, c.u32(service_slot));
            c.set(7, 0);
            c.call(0x103092f0, 0x1037f0d4);
            block(c, 0x1037f0d4, 4);
            const auto grade = std::int8_t(c.r(3));
            c.set({{0, std::uint32_t(grade)}, {3, 10}});
            if (guest::compare(c, grade, 10) <= 0) {
                block(c, 0x1037f0e4, 1);
                c.set(3, std::uint32_t(grade));
            }
            block(c, 0x1037f0e8, 3);
            const auto packed = (c.u8(record + review_grade) & 15) | ((c.r(3) & 15) << 4);
            c.set(0, packed);
            c.put8(record + review_grade, packed);
        }
    }
    block(c, 0x1037f0f4, 7);
    restore_review_frame(c, 0x50, true);
}

void clear_review_condition(Cpu& c, std::uint32_t contract, std::uint32_t mask) {
    block(c, clear_one_entry, 11);
    GuestNativeScope scope(c.s, clear_one_entry);
    const auto stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    c.put32(stack - 4, c.r(31));
    const guest::Kept<31> removed(c, mask);
    c.put32(stack - 8, c.r(30));
    const guest::Kept<30> record(c, contract);
    c.put32(stack + 8, link);
    c.put32(stack - 0x40, stack);
    c.set(1, stack - 0x40);
    scope.frame(c.sp());

    const auto flags = c.u32(contract + ContractRecord::review_conditions);
    const auto remaining = flags & ~mask;
    c.set({{4, flags}, {0, remaining}});
    c.put32(contract + ContractRecord::review_conditions, remaining);
    c.call(refresh_entry, 0x1037f7fc);

    block(c, 0x1037f7fc, 7);
    const auto index = c.u32(record);
    c.set({{0, index}, {4, removed}});
    const auto staff_slot = c.toc(staff_table_slot);
    c.set({{3, staff_slot}, {0, index * staff_record_size}});
    const auto staff_table = c.u32(staff_slot);
    c.set(3, staff_table + index * staff_record_size);
    c.call(0x100de090, 0x1037f818);
    block(c, 0x1037f818, 6);
    restore_review_frame(c, 0x40);
}

void clear_review_conditions(Cpu& c, std::uint32_t contract, std::uint32_t refresh) {
    block(c, clear_all_entry, 15);
    GuestNativeScope scope(c.s, clear_all_entry);
    const auto stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    c.put32(stack - 4, c.r(31));
    const guest::Kept<31> refresh_flag(c, refresh);
    c.put32(stack - 8, c.r(30));
    const guest::Kept<30> record(c, contract);
    c.put32(stack + 8, link);
    c.set(0, 0);
    c.put32(stack - 0x40, stack);
    c.set(1, stack - 0x40);
    scope.frame(c.sp());

    c.put32(contract + ContractRecord::review_conditions, 0);
    const auto staff_slot = c.toc(staff_table_slot);
    c.set(3, staff_slot);
    const auto index = c.u32(record);
    c.set(0, index);
    const auto staff_table = c.u32(staff_slot);
    c.set({{0, index * staff_record_size}, {3, staff_table + index * staff_record_size}});
    c.call(0x100ddf30, 0x1037f86c);

    block(c, 0x1037f86c, 2);
    const auto flag = std::int8_t(std::uint32_t(refresh_flag));
    c.set(0, std::uint32_t(flag));
    const auto cr0 = c.cr(0) & 0xf0;
    guest::compare(c, flag);
    c.set_cr(0, std::uint8_t(c.cr(0) | cr0));
    if (flag) {
        block(c, 0x1037f874, 3);
        c.set({{3, record}, {4, 0}});
        c.call(refresh_entry, 0x1037f880);
    }
    block(c, 0x1037f880, 6);
    restore_review_frame(c, 0x40);
}
}

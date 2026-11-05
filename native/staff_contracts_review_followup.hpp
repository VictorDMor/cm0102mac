#pragma once

#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts::detail {

// Both callers run the same notification policy from this point. Their
// guest frames keep the draft and table slots in different registers.
template<std::uint32_t Start, unsigned Draft, unsigned Staff, unsigned Policy>
void notify_review_followup(Cpu& c, const guest::Kept<Draft>& draft, const guest::Kept<Staff>& staff_slot,
                            const guest::Kept<Policy>& policy_slot) {
    constexpr auto staff_stride = 0x70u, staff_contract = 0x14u;
    block(c, Start, 2);
    c.set(3, draft);
    detail::call_owned(c, 0x1037f110, Start + 0x8, has_review_trigger, std::uint32_t(draft));
    block(c, Start + 0x8, 2);
    if (!record_nonzero(c, std::int8_t(c.r(3)))) return;

    block(c, Start + 0x10, 7);
    auto index = c.u32(draft);
    c.set({{0, index}, {5, 0}});
    auto staff_table = c.u32(staff_slot);
    c.set({{4, staff_table}, {0, index * staff_stride}});
    c.set(3, c.u32(policy_slot));
    c.set(4, staff_table + index * staff_stride);
    c.call(0x103f43b0, Start + 0x2c);
    block(c, Start + 0x2c, 2);
    if (record_nonzero(c, std::int8_t(c.r(3)))) return;

    block(c, Start + 0x34, 4);
    const auto review = c.i8(draft + ContractRecord::review_flags);
    c.set(3, std::uint32_t(review));
    if (record_nonzero(c, review & 8)) return;
    block(c, Start + 0x44, 2);
    if (record_nonzero(c, review & 1)) return;

    block(c, Start + 0x4c, 6);
    index = c.u32(draft);
    c.set(0, index);
    staff_table = c.u32(staff_slot);
    c.set({{4, staff_table}, {0, index * staff_stride}});
    c.set(3, c.u32(policy_slot));
    c.set(4, staff_table + index * staff_stride);
    c.call(0x103e6470, Start + 0x64);
    block(c, Start + 0x64, 2);
    if (record_nonzero(c, std::int8_t(c.r(3)))) return;

    block(c, Start + 0x6c, 8);
    index = c.u32(draft);
    c.set(0, index);
    staff_table = c.u32(staff_slot);
    c.set({{4, staff_table}, {3, index * staff_stride}, {0, index * staff_stride + 0x5c}});
    const auto age = std::int8_t(c.u16(staff_table + index * staff_stride + 0x5c));
    c.set(0, std::uint32_t(age));
    if (guest::compare(c, age, 20) <= 0) {
        block(c, Start + 0x8c, 5);
        const auto person = staff_table + index * staff_stride;
        c.set(3, person);
        const auto primary = c.u32(person + staff_contract);
        c.set(3, primary);
        const auto service = c.i16(primary + 10);
        c.set(0, guest::extended(service));
        if (guest::compare(c, service, 4250) <= 0) return;
    }

    block(c, Start + 0xa0, 4);
    const auto manager = c.toc(-0x7b7c);
    c.set({{3, manager}, {4, draft}});
    c.set(3, c.u32(manager));
    c.call(0x1008e350, Start + 0xb0);
}
}

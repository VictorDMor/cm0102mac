#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::record_nonzero;
using detail::ContractRecord;
using guest::Cpu;

constexpr auto refresh_entry = 0x1037f590u, clear_entry = 0x1037f7d0u;
constexpr auto staff_stride = 0x70u, club_stride = 0x248u;
constexpr auto staff_contract = 0x14u, review_grade = 0x60u;
constexpr auto review_counter = 0x4cu, review_minimum = 0x44u, review_reason = 0x45u;

void clear_present_conditions(Cpu& c, const guest::Kept<28>& draft) {
    constexpr std::array masks{0x10u, 1u, 4u, 8u, 0x100u, 0x80u, 0x10000u};
    auto site = 0x1037f704u;
    for (const auto mask : masks) {
        block(c, site, 3);
        if (record_nonzero(c, c.u32(draft + ContractRecord::review_conditions) & mask)) {
            const bool wide_mask = mask == 0x10000;
            const auto after = site + (wide_mask ? 28 : 24);
            block(c, site + 12, wide_mask ? 4 : 3);
            c.set({{3, draft}, {4, mask}});
            detail::call_owned(c, clear_entry, after, clear_review_condition, std::uint32_t(draft), mask);
        }
        site += 24;
    }
}

bool eligible_for_clearance(Cpu& c, const guest::Kept<28>& draft, const guest::Kept<30>& staff,
                            std::uint32_t employer) {
    block(c, 0x1037f638, 3);
    const auto current = c.u32(staff + staff_contract);
    c.set(0, current);
    if (!guest::nonzero_logical(c, current)) return false;

    block(c, 0x1037f644, 4);
    const auto draft_club = c.u32(draft + ContractRecord::club_id);
    c.set(3, draft_club);
    const auto primary_club = c.u32(employer);
    c.set(0, primary_club);
    if (guest::compare(c, std::int32_t(draft_club), std::int32_t(primary_club)) != 0) return false;

    block(c, 0x1037f654, 3);
    if (!record_nonzero(c, c.u8(draft + ContractRecord::review_flags) & 8)) return false;

    block(c, 0x1037f660, 3);
    if (record_nonzero(c, c.i8(draft + ContractRecord::terms_flag))) return false;

    block(c, 0x1037f66c, 2);
    c.set(3, draft);
    detail::call_owned(c, 0x1037f110, 0x1037f674, has_review_trigger, std::uint32_t(draft));
    block(c, 0x1037f674, 2);
    if (record_nonzero(c, std::int8_t(c.r(3)))) return false;

    block(c, 0x1037f67c, 4);
    const auto role = c.u8(draft + review_grade) & 15u;
    c.set(0, role);
    if (guest::compare_logical(c, role, 7) == 0) return false;

    block(c, 0x1037f68c, 5);
    const auto policy_slot = c.toc(-0x7b40);
    c.set({{3, policy_slot}, {4, staff}, {5, 0}});
    c.set(3, c.u32(policy_slot));
    c.call(0x103ea290, 0x1037f6a0);
    block(c, 0x1037f6a0, 2);
    return !record_nonzero(c, std::int8_t(c.r(3)));
}

void complete_clearance(Cpu& c, const guest::Kept<28>& draft, const guest::Kept<30>& staff,
                        const guest::Kept<31>& manager_slot, const guest::Kept<29>& context) {
    block(c, 0x1037f6a8, 4);
    c.set({{3, c.u32(manager_slot)}, {4, draft}, {5, context}});
    c.call(0x1008f620, 0x1037f6b8);

    block(c, 0x1037f6b8, 11);
    const auto review = c.u8(draft + ContractRecord::review_flags) & ~8u;
    c.set(0, review);
    c.put8(draft + ContractRecord::review_flags, review);
    const auto availability = c.u8(staff + 0x65);
    c.set(3, availability);
    const auto response = c.u8(staff + 0x67);
    c.set({{3, availability + response}, {0, availability + response + 5}});
    c.put8(draft + review_counter, availability + response + 5);
    const auto reason = c.u8(draft + review_reason);
    c.set(0, reason);
    if (guest::nonzero_logical(c, reason)) {
        block(c, 0x1037f6e4, 2);
        c.set(0, 0);
        c.put8(draft + review_reason, 0);
    }
    block(c, 0x1037f6ec, 4);
    const auto minimum = c.i8(draft + review_minimum);
    c.set(0, std::uint32_t(minimum));
    if (guest::compare(c, minimum, 15) < 0) {
        block(c, 0x1037f6fc, 2);
        c.set(0, 15);
        c.put8(draft + review_minimum, 15);
    }
    clear_present_conditions(c, draft);
}
}

void refresh_review_conditions(Cpu& c, std::uint32_t contract, std::uint32_t review_context) {
    block(c, refresh_entry, 16);
    GuestNativeScope scope(c.s, refresh_entry);
    const auto stack = c.sp(), link = c.lr();
    c.set(0, link);
    detail::transfer_saved_registers<26>(c, stack);
    guest::Kept<30> staff(c, c.toc(-0x7be0));
    const guest::Kept<29> context(c, review_context);
    const guest::Kept<31> manager_slot(c, c.toc(-0x7b7c));
    const guest::Kept<28> draft(c, contract);
    const guest::Kept<26> primary_scratch(c, c.toc(-0x7b78));
    c.set(27, 0);
    c.put32(stack + 8, link);
    const auto frame = stack - 0x50;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);

    auto index = c.u32(contract);
    c.set(0, index);
    const auto table = c.u32(staff);
    c.set({{4, table}, {0, index * staff_stride}});
    c.set(3, c.u32(manager_slot));
    c.set(4, table + index * staff_stride);
    c.call(0x10082f20, 0x1037f5d0);

    block(c, 0x1037f5d0, 3);
    const auto primary = c.r(3);
    const auto found = guest::nonzero_logical(c, primary);
    c.put32(primary_scratch, primary);
    if (found) {
        block(c, 0x1037f5dc, 6);
        const auto kind_byte = c.u8(primary + ContractRecord::packed_kind);
        const auto kind = detail::unpack_kind(kind_byte);
        c.s.wr<1>(guest::reg::xer_ca, (kind_byte & 0x82) == 0x82);
        c.set(0, std::uint32_t(kind));
        if (guest::compare(c, kind, 5) != 0) {
            block(c, 0x1037f5f4, 1);
            c.set(27, 1);
        }
    }

    block(c, 0x1037f5f8, 2);
    std::uint32_t employer = 0;
    if (guest::nonzero(c, std::int32_t(c.r(27)))) {
        block(c, 0x1037f600, 7);
        const auto resolved = c.u32(primary_scratch);
        c.set(3, resolved);
        const auto clubs_slot = c.toc(-0x7be8);
        c.set(4, clubs_slot);
        const auto club_id = c.u32(resolved + ContractRecord::club_id);
        c.set(0, club_id);
        const auto clubs = c.u32(clubs_slot);
        c.set({{3, clubs}, {0, club_id * club_stride}});
        employer = clubs + club_id * club_stride;
    } else block(c, 0x1037f61c, 1);
    c.set(4, employer);

    block(c, 0x1037f620, 6);
    index = c.u32(draft);
    c.set(0, index);
    const auto has_employer = guest::nonzero_logical(c, employer);
    const auto current_table = c.u32(staff);
    c.set({{3, current_table}, {0, index * staff_stride}});
    staff = current_table + index * staff_stride;
    if (has_employer && eligible_for_clearance(c, draft, staff, employer))
        complete_clearance(c, draft, staff, manager_slot, context);

    block(c, 0x1037f7b0, 5);
    const auto restored = c.sp() + 0x50;
    const auto caller = c.u32(restored + 8);
    c.set({{0, caller}, {1, restored}});
    c.set_lr(caller);
    detail::transfer_saved_registers<26, true>(c, restored);
}
}

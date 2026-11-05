#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::record_nonzero;
using detail::ContractRecord;
using guest::Cpu;

constexpr auto status_entry = 0x1037f8a0u, staff_stride = 0x70u;
constexpr auto review_counter = 0x4cu, staff_contract = 0x14u;

void report_status_mask(Cpu& c, const guest::Kept<28>& source, const guest::Kept<27>& diagnostic) {
    const auto frame = c.sp();
    block(c, 0x1037f8d0, 6);
    c.set({{3, source}, {6, frame + 0x40}, {7, frame + 0x38}, {4, 0}, {5, 0}});
    c.call(0x10739890, 0x1037f8e8);
    block(c, 0x1037f8e8, 1);
    c.link(0x1037f8ec);
    c.set(3, c.toc(-0x5b20));
    c.s.charge(2);
    block(c, 0x1037f8ec, 7);
    c.set({{5, c.r(3)}, {3, diagnostic}, {4, source + 0x14}, {6, frame + 0x40}, {7, frame + 0x38},
           {8, 0x1e9b}});
    c.call(0x1077b720, 0x1037f908);
    block(c, 0x1037f908, 4);
    c.set({{4, diagnostic}, {3, source + 0x21}, {5, 0}});
    c.call(0x10769b60, 0x1037f918);
    block(c, 0x1037f918, 3);
    const auto active = c.toc(-0x7bac);
    c.set({{3, active}, {0, 0}});
    c.put8(active, 0);
}

bool needs_followup(Cpu& c, const guest::Kept<29>& draft, const guest::Kept<30>& staff_slot,
                    const guest::Kept<31>& policy_slot) {
    auto index = c.u32(draft);
    c.set(0, index);
    auto staff_table = c.u32(staff_slot);
    c.set({{0, index * staff_stride}, {3, staff_table + index * staff_stride}});
    const auto staff_status = c.i8(staff_table + index * staff_stride + 0x62);
    c.set(0, std::uint32_t(staff_status));
    if (guest::compare(c, staff_status, 12) == 0) return false;
    block(c, 0x1037f950, 2);
    if (guest::compare(c, staff_status, 13) == 0) return false;
    block(c, 0x1037f958, 2);
    if (guest::compare(c, staff_status, 15) == 0) return false;

    block(c, 0x1037f960, 3);
    const auto counter = c.u8(draft + review_counter);
    c.set(3, counter);
    if (!record_nonzero(c, std::int8_t(counter))) return false;
    block(c, 0x1037f96c, 3);
    if (record_nonzero(c, std::int8_t(counter) < 0 ? 0x10000 : 0)) return false;
    block(c, 0x1037f978, 3);
    if (record_nonzero(c, c.i8(draft + ContractRecord::terms_flag))) return false;

    block(c, 0x1037f984, 2);
    c.set(3, draft);
    detail::call_owned(c, 0x1037f110, 0x1037f98c, has_review_trigger, std::uint32_t(draft));
    block(c, 0x1037f98c, 2);
    if (!record_nonzero(c, std::int8_t(c.r(3)))) return false;

    block(c, 0x1037f994, 7);
    index = c.u32(draft);
    c.set({{0, index}, {5, 0}});
    staff_table = c.u32(staff_slot);
    c.set({{4, staff_table}, {0, index * staff_stride}});
    c.set(3, c.u32(policy_slot));
    c.set(4, staff_table + index * staff_stride);
    c.call(0x103f43b0, 0x1037f9b0);
    block(c, 0x1037f9b0, 2);
    if (record_nonzero(c, std::int8_t(c.r(3)))) return false;

    block(c, 0x1037f9b8, 4);
    const auto review = c.i8(draft + ContractRecord::review_flags);
    c.set(3, std::uint32_t(review));
    if (record_nonzero(c, review & 8)) return false;
    block(c, 0x1037f9c8, 2);
    if (record_nonzero(c, review & 1)) return false;

    block(c, 0x1037f9d0, 6);
    index = c.u32(draft);
    c.set(0, index);
    staff_table = c.u32(staff_slot);
    c.set({{4, staff_table}, {0, index * staff_stride}});
    c.set(3, c.u32(policy_slot));
    c.set(4, staff_table + index * staff_stride);
    c.call(0x103e6470, 0x1037f9e8);
    block(c, 0x1037f9e8, 2);
    if (record_nonzero(c, std::int8_t(c.r(3)))) return false;

    block(c, 0x1037f9f0, 8);
    index = c.u32(draft);
    c.set(0, index);
    staff_table = c.u32(staff_slot);
    c.set({{4, staff_table}, {3, index * staff_stride}, {0, index * staff_stride + 0x5c}});
    const auto age = std::int8_t(c.u16(staff_table + index * staff_stride + 0x5c));
    c.set(0, std::uint32_t(age));
    if (guest::compare(c, age, 20) > 0) return true;

    block(c, 0x1037fa10, 5);
    const auto person = staff_table + index * staff_stride;
    c.set(3, person);
    const auto primary = c.u32(person + staff_contract);
    c.set(3, primary);
    const auto value = c.i16(primary + 10);
    c.set(0, guest::extended(value));
    return guest::compare(c, value, 4250) > 0;
}
}

void add_review_status_bits(Cpu& c, std::uint32_t contract, std::uint32_t bits) {
    block(c, status_entry, 12);
    GuestNativeScope scope(c.s, status_entry);
    const auto stack = c.sp(), link = c.lr();
    c.set(0, link);
    detail::transfer_saved_registers<26>(c, stack);
    const guest::Kept<26> mask(c, bits);
    const guest::Kept<30> staff_slot(c, c.toc(-0x7be0));
    const auto warn = guest::compare(c, std::int32_t(bits), 0x1000) > 0;
    const guest::Kept<31> policy_slot(c, c.toc(-0x7b40));
    const guest::Kept<27> diagnostic(c, c.toc(-0x7ba8));
    const guest::Kept<29> draft(c, contract);
    const guest::Kept<28> source(c, c.toc(-0x4da4));
    c.put32(stack + 8, link);
    const auto frame = stack - 0x80;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    if (warn) report_status_mask(c, source, diagnostic);

    block(c, 0x1037f924, 11);
    const auto combined = c.u32(draft + ContractRecord::status) | mask;
    c.set(0, combined);
    c.put32(draft + ContractRecord::status, combined);
    if (needs_followup(c, draft, staff_slot, policy_slot)) {
        block(c, 0x1037fa24, 4);
        const auto manager = c.toc(-0x7b7c);
        c.set({{3, manager}, {4, draft}});
        c.set(3, c.u32(manager));
        c.call(0x1008e350, 0x1037fa34);
    }
    block(c, 0x1037fa34, 5);
    const auto restored = c.sp() + 0x80;
    const auto caller = c.u32(restored + 8);
    c.set({{0, caller}, {1, restored}});
    c.set_lr(caller);
    detail::transfer_saved_registers<26, true>(c, restored);
}
}

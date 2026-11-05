#include "staff_contracts.hpp"
#include "staff_contracts_review_followup.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::record_nonzero;
using detail::ContractRecord;
using guest::Cpu;

constexpr auto status_entry = 0x1037f8a0u, staff_stride = 0x70u;
constexpr auto review_counter = 0x4cu;

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

bool needs_followup(Cpu& c, const guest::Kept<29>& draft, const guest::Kept<30>& staff_slot) {
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

    return true;
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
    if (needs_followup(c, draft, staff_slot))
        detail::notify_review_followup<0x1037f984>(c, draft, staff_slot, policy_slot);
    block(c, 0x1037fa34, 5);
    const auto restored = c.sp() + 0x80;
    const auto caller = c.u32(restored + 8);
    c.set({{0, caller}, {1, restored}});
    c.set_lr(caller);
    detail::transfer_saved_registers<26, true>(c, restored);
}
}

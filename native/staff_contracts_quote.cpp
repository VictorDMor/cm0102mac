#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"
#include "floating_point.hpp"

namespace cm::aot::staff_contracts {

void apply_country_wage_quote(guest::Cpu& c, std::uint32_t contract, std::uint32_t player) {
    constexpr auto entry = 0x10378d30u;
    detail::block(c, entry, 11);
    GuestNativeScope scope(c.s, entry);
    const auto stack = c.sp(), link = c.lr();
    c.set(0, link);
    c.put32(stack - 4, c.r(31));
    const guest::Kept<31> draft(c, contract);
    c.set(6, 1);
    c.put32(stack + 8, link);
    c.set({{7, UINT32_MAX}, {8, 11}, {9, 0}});
    const auto frame = stack - 0x50;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(5, c.u32(player + 0x24));
    c.call(0x10378da0, 0x10378d5c);

    detail::block(c, 0x10378d5c, 7);
    // An enabled invalid conversion can stop before writing its result.
    constexpr auto converted = guest::reg::fpr(0);
    original_fctiwz(c.s, converted, guest::reg::fpr(1));
    const auto scratch = c.sp() + 0x38;
    c.put64(scratch, c.s.r<8>(converted));
    const auto wage = c.u32(scratch + 4);
    c.set(0, wage);
    c.put32(draft + 12, wage);
    const auto stored_wage = c.i32(draft + 12);
    c.set(0, std::uint32_t(stored_wage));
    if (guest::compare(c, stored_wage, 55) < 0) {
        detail::block(c, 0x10378d78, 2);
        c.set(0, 55);
        c.put32(draft + 12, 55);
    }
    detail::block(c, 0x10378d80, 5);
    const auto restored = c.sp() + 0x50;
    const auto caller = c.u32(restored + 8);
    c.set({{0, caller}, {1, restored}});
    c.set_lr(caller);
    c.set(31, c.u32(restored - 4));
}
}

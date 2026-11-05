#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using guest::Cpu;

constexpr unsigned frame_size = 0x50, adjustment_slot = 0x38;
constexpr unsigned terms_value = 0x0c;
constexpr std::uint32_t scale_by_52 = 0x800000, scale_by_four = 0x400000;

bool uses_scale(Cpu& c, std::uint32_t flags, std::uint32_t scale) {
    const auto selected = flags & scale;
    c.set(0, selected);
    // The record-form mask updates the four condition bits in place.
    const auto overflow = c.s.r<1>(guest::reg::xer_so) & 1;
    c.set_cr(0, std::uint8_t((c.cr(0) & 0xf0) | (selected ? 4 : 2) | overflow));
    return selected != 0;
}

void adjust_value(Cpu& c, std::uint32_t contract, std::uint32_t entry, std::uint32_t adjustment) {
    block(c, entry, 12);
    const auto caller_stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    c.put32(caller_stack - 4, c.r(31));
    const auto preferences_slot = c.toc(-0x7ac8);
    c.set(4, preferences_slot);
    const guest::Kept<31> row(c, contract);
    c.put32(caller_stack + 8, link);
    c.set(0, 0);
    const auto frame = caller_stack - frame_size;
    c.put32(frame, caller_stack);
    c.set(1, frame);
    const auto amount = frame + adjustment_slot;
    c.put32(amount, 0);

    const auto preferences = c.u32(preferences_slot);
    c.set(3, preferences);
    const auto flags = c.u32(preferences + 4);
    c.set(3, flags);
    if (uses_scale(c, flags, scale_by_52)) {
        block(c, entry + 0x30, 6);
        const auto value = c.u32(row + terms_value);
        const auto annual_value = value * 52u;
        c.set({{3, amount}, {4, 4}, {0, annual_value}});
        c.put32(amount, annual_value);
        c.call(adjustment, entry + 0x48);

        block(c, entry + 0x48, 9);
        c.set(3, 0x4ec50000);
        const auto adjusted = c.i32(amount);
        // Only the shift carry survives the compiler's reciprocal division.
        const auto high_product = (INT64_C(0x4ec4ec4f) * adjusted) >> 32;
        c.s.wr<1>(guest::reg::xer_ca, high_product < 0 && (high_product & 15));
        const auto value_per_period = adjusted / 52;
        c.set({{3, std::uint32_t(high_product < 0)}, {0, std::uint32_t(value_per_period)}});
        c.put32(row + terms_value, std::uint32_t(value_per_period));
    } else {
        block(c, entry + 0x6c, 2);
        if (uses_scale(c, flags, scale_by_four)) {
            block(c, entry + 0x74, 6);
            const auto value = c.u32(row + terms_value);
            const auto four_period_value = value * 4u;
            c.set({{3, amount}, {4, 8}, {0, four_period_value}});
            c.put32(amount, four_period_value);
            c.call(adjustment, entry + 0x8c);

            block(c, entry + 0x8c, 5);
            const auto adjusted = c.i32(amount);
            const auto value_per_period = adjusted / 4;
            c.s.wr<1>(guest::reg::xer_ca, adjusted < 0 && adjusted > -4);
            c.set(0, std::uint32_t(value_per_period));
            c.put32(row + terms_value, std::uint32_t(value_per_period));
        } else {
            block(c, entry + 0xa0, 5);
            const auto value = c.u32(row + terms_value);
            c.set({{0, value}, {3, amount}, {4, 8}});
            c.put32(amount, value);
            c.call(adjustment, entry + 0xb4);

            block(c, entry + 0xb4, 2);
            const auto adjusted = c.u32(amount);
            c.set(0, adjusted);
            c.put32(row + terms_value, adjusted);
        }
    }

    block(c, entry + 0xbc, 5);
    const auto restored_stack = c.sp() + frame_size;
    const auto restored_link = c.u32(restored_stack + 8);
    c.set({{0, restored_link}, {1, restored_stack}});
    c.set_lr(restored_link);
    c.set(31, c.u32(restored_stack - 4));
}
}

void adjust_primary_value(Cpu& c, std::uint32_t contract) {
    adjust_value(c, contract, 0x10380560, 0x1000a060);
}

void adjust_alternate_value(Cpu& c, std::uint32_t contract) {
    adjust_value(c, contract, 0x10380630, 0x1000a170);
}
}

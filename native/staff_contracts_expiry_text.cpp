#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using guest::Cpu;

constexpr auto expiry_text_entry = 0x10374d80u, first_month = 0x10374df8u;
constexpr auto ordinary_text = 0x10374eb8u, restore_text_frame = 0x10374f28u, expiry_text_end = 0x10374f44u;
constexpr auto localized_format = 0x10727890u, format_date = 0x10715e00u;

std::int32_t read_expiry_kind(Cpu& c, std::uint32_t record) {
    const auto packed = c.u8(record + ContractRecord::packed_kind);
    c.s.wr<1>(guest::reg::xer_ca, (packed & 0x82) == 0x82);
    const auto kind = detail::unpack_kind(packed);
    c.set(0, std::uint32_t(kind));
    return kind;
}

void emit_month(Cpu& c, std::uint32_t target, const guest::Kept<30>& output, const guest::Kept<31>& strings) {
    const auto displacement = target - first_month;
    const auto month = displacement / 16;
    const auto part = displacement % 16;
    if (part <= 8) {
        block(c, target, 3 - part / 4);
        if (part == 0) c.set(3, output);
        if (part <= 4) c.set(4, strings + 0x11e + month * 0x11);
        c.call(localized_format, first_month + month * 16 + 12);
    }
    block(c, first_month + month * 16 + 12, 1);
}

void emit_expiry(Cpu& c, const guest::Kept<29>& draft, const guest::Kept<30>& output,
                 const guest::Kept<31>& strings) {
    block(c, ordinary_text, 6);
    const auto kind = read_expiry_kind(c, draft);
    if (guest::compare(c, kind, 3) == 0) {
        block(c, 0x10374ed0, 3);
        c.set({{3, output}, {4, strings + 0xb9}});
        c.call(localized_format, 0x10374edc);
        block(c, 0x10374edc, 1);
        return;
    }
    block(c, 0x10374ee0, 2);
    if (guest::compare(c, kind, 4) == 0) {
        block(c, 0x10374ee8, 4);
        c.set({{3, output}, {4, strings + 0xca}, {5, strings + 0xf2}});
        c.call(localized_format, 0x10374ef8);
        block(c, 0x10374ef8, 1);
        return;
    }
    block(c, 0x10374efc, 2);
    if (guest::compare(c, kind, 1) == 0) {
        block(c, 0x10374f04, 3);
        c.set({{3, output}, {4, strings + 0xf3}});
        c.call(localized_format, 0x10374f10);
        block(c, 0x10374f10, 1);
        return;
    }
    block(c, 0x10374f14, 5);
    c.set({{4, output}, {3, draft + ContractRecord::end_date}, {5, 200}, {6, 255}});
    c.call(format_date, restore_text_frame);
}

void finish_expiry_text(Cpu& c, std::uint32_t target) {
    block(c, target, (expiry_text_end - target) / 4);
    if (target <= restore_text_frame) c.set(0, c.u32(c.sp() + 0x58));
    if (target <= restore_text_frame + 4) c.set(1, c.sp() + 0x50);
    if (target <= restore_text_frame + 8) c.set_lr(c.r(0));
    if (target <= restore_text_frame + 12) c.set(31, c.u32(c.sp() - 4));
    if (target <= restore_text_frame + 16) c.set(30, c.u32(c.sp() - 8));
    if (target <= restore_text_frame + 20) c.set(29, c.u32(c.sp() - 12));
    c.finish();
}
}

void describe_expiry_text(Cpu& c, std::uint32_t contract, std::uint32_t output, std::int8_t style) {
    block(c, expiry_text_entry, 12);
    GuestNativeScope scope(c.s, expiry_text_entry);
    const auto caller = c.lr(), stack = c.sp();
    c.set(0, caller);
    c.put32(stack - 4, c.r(31));
    const guest::Kept<31> strings(c, c.toc(-0x4da4));
    c.put32(stack - 8, c.r(30));
    const guest::Kept<30> destination(c, output);
    c.put32(stack - 12, c.r(29));
    const guest::Kept<29> draft(c, contract);
    c.put32(stack + 8, caller);
    c.set(0, std::uint32_t(style));
    const auto month_style = guest::compare(c, style, 1) == 0;
    c.put32(stack - 0x50, stack);
    c.set(1, stack - 0x50);
    scope.frame(c.sp());

    if (month_style) {
        block(c, 0x10374db0, 6);
        if (guest::compare(c, read_expiry_kind(c, draft), 2) == 0) {
            block(c, 0x10374dc8, 2);
            c.set(3, draft + ContractRecord::end_date);
            c.call(0x10718360, 0x10374dd0);
            block(c, 0x10374dd0, 5);
            const auto month = std::uint32_t(std::int8_t(c.r(3)));
            c.set(4, month);
            const auto year = c.i16(draft + ContractRecord::end_date + 2);
            c.set(0, std::uint16_t(year));
            const bool has_month = guest::compare_logical(c, month, 11) <= 0;
            c.set(5, guest::extended(year));
            auto target = restore_text_frame;
            if (has_month) {
                block(c, 0x10374de4, 5);
                const auto table = c.toc(-0x4db0);
                c.set({{3, table}, {0, month * 4}});
                target = c.u32(table + month * 4);
                c.set(3, target);
                c.s.wr<4>(guest::reg::ctr, target);
                if (target >= first_month && target < ordinary_text && target % 4 == 0) {
                    emit_month(c, target, destination, strings);
                    target = restore_text_frame;
                } else if (target == ordinary_text) {
                    emit_expiry(c, draft, destination, strings);
                    target = restore_text_frame;
                }
            }
            if (target >= restore_text_frame && target < expiry_text_end && target % 4 == 0) {
                finish_expiry_text(c, target);
            } else {
                // The audited month table only enters its label cases.
                if (target >= expiry_text_entry && target < expiry_text_end)
                    throw std::logic_error("Unclassified staff-contract expiry text continuation");
                c.s.pc = target;
                run_until(c.s, caller);
            }
            return;
        }
    }
    emit_expiry(c, draft, destination, strings);
    finish_expiry_text(c, restore_text_frame);
}
}

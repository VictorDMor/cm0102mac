#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using guest::Cpu;

constexpr std::int32_t text_pool_slot = -0x4da4, preferences_slot = -0x7ac8;
constexpr unsigned wage_field = 0x0c;
constexpr unsigned wage_frame = 0x120, date_frame = 0x40;
constexpr std::uint32_t read_money = 0x10009f20, format_money = 0x1000b410;
constexpr std::uint32_t localize = 0x10727890, format = 0x1077b720, format_date = 0x10715e00;
constexpr std::uint32_t weekly = 0x200000, monthly = 0x400000, yearly = 0x800000;

void record_signed(Cpu& c, std::int32_t value) {
    const auto condition = value < 0 ? 8u : value > 0 ? 4u : 2u;
    c.set_cr(0, std::uint8_t((c.cr(0) & 0xf0) | condition | (c.s.r<1>(guest::reg::xer_so) & 1)));
}

bool tests_period(Cpu& c, std::uint32_t preferences, std::uint32_t period) {
    const auto enabled = preferences & period;
    c.set(0, enabled);
    record_signed(c, std::int32_t(enabled));
    return enabled != 0;
}

std::uint32_t restore_link(Cpu& c, unsigned size) {
    const auto stack = c.sp() + size;
    const auto link = c.u32(stack + 8);
    c.set({{0, link}, {1, stack}});
    c.set_lr(link);
    return stack;
}

unsigned select_wage_period(Cpu& c, std::uint32_t contract, std::uint32_t preferences, std::uint32_t frame) {
    const auto initial_wage = c.u32(contract + wage_field);
    c.set(0, initial_wage);
    const auto settings = c.u32(preferences);
    c.set(4, settings);
    c.put32(frame + 0x38, initial_wage);
    const auto flags = c.u32(settings + 4);
    c.set(4, flags);

    unsigned scale, mode;
    if (tests_period(c, flags, weekly)) {
        block(c, 0x103725c8, 4);
        scale = 1;
        mode = 8;
    } else {
        block(c, 0x103725d8, 2);
        if (tests_period(c, flags, monthly)) {
            block(c, 0x103725e0, 5);
            scale = 4;
            mode = 8;
        } else {
            block(c, 0x103725f4, 4);
            scale = 52;
            mode = 4;
        }
    }

    const auto wage = c.u32(contract + wage_field) * scale;
    c.set({{0, wage}, {3, mode}});
    c.put32(frame + 0x38, wage);
    return mode;
}

void emit_wage_label(Cpu& c, std::uint32_t destination, std::uint32_t text,
                     std::uint32_t amount, std::uint32_t pc, std::uint32_t formatter) {
    block(c, pc, 4);
    c.set({{3, destination}, {4, text}, {5, amount}});
    c.call(formatter, pc + 16);
    if (pc + 16 != 0x10372710) block(c, pc + 16, 1);
}

void append_wage_period(Cpu& c, std::uint32_t destination, std::uint32_t pool,
                        std::uint32_t preferences, std::uint32_t amount, bool abbreviated) {
    const auto start = abbreviated ? 0x1037265cu : 0x103726b8u;
    block(c, start, 4);
    const auto settings = c.u32(preferences);
    c.set(3, settings);
    const auto flags = c.u32(settings + 4);
    c.set(3, flags);
    if (tests_period(c, flags, weekly)) {
        emit_wage_label(c, destination, pool + (abbreviated ? 0x2a : 0x69), amount, start + 0x10, localize);
        return;
    }
    block(c, start + 0x24, 2);
    if (tests_period(c, flags, monthly)) {
        emit_wage_label(c, destination, pool + (abbreviated ? 0x3f : 0x83), amount, start + 0x2c, localize);
        return;
    }
    block(c, start + 0x40, 2);
    if (tests_period(c, flags, yearly))
        emit_wage_label(c, destination, pool + (abbreviated ? 0x54 : 0x9e), amount, start + 0x48, localize);
}
}

void format_wage_text(Cpu& c, std::uint32_t contract, std::uint32_t destination,
                      std::uint32_t style, std::int8_t format_flags) {
    block(c, 0x10372590, 14);
    GuestNativeScope scope(c.s, 0x10372590);
    const auto stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    detail::transfer_saved_registers<27>(c, stack);
    const guest::Kept<30> preferences(c, c.toc(preferences_slot));
    const guest::Kept<28> output(c, destination);
    const guest::Kept<31> pool(c, c.toc(text_pool_slot));
    const guest::Kept<29> form(c, style);
    c.put32(stack + 8, link);
    const auto frame = stack - wage_frame;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    const auto mode = select_wage_period(c, contract, preferences, frame);

    block(c, 0x10372604, 6);
    const auto flags = std::uint32_t(format_flags);
    c.set(0, flags);
    const guest::Kept<27> display_mode(c, flags | mode);
    c.set({{3, frame + 0x38}, {4, std::uint32_t(std::int8_t(std::uint32_t(display_mode)))}});
    c.call(read_money, 0x1037261c);

    block(c, 0x1037261c, 6);
    const auto formatted = frame + 0x3c;
    c.set({{5, c.r(3)}, {6, c.r(4)}, {3, formatted}, {4, 200},
           {7, std::uint32_t(std::int8_t(std::uint32_t(display_mode)))}});
    c.call(format_money, 0x10372634);

    block(c, 0x10372634, 2);
    const auto selected_form = std::int8_t(std::uint32_t(form));
    c.set(0, std::uint32_t(selected_form));
    record_signed(c, selected_form);
    if (!selected_form) {
        emit_wage_label(c, output, pool + 0x27, formatted, 0x1037263c, format);
    } else {
        block(c, 0x10372650, 3);
        const bool abbreviated = guest::compare(c, selected_form, 2) == 0;
        append_wage_period(c, output, pool, preferences, formatted, abbreviated);
    }

    block(c, 0x10372710, 5);
    const auto restored_stack = restore_link(c, wage_frame);
    detail::transfer_saved_registers<27, true>(c, restored_stack);
}

void format_contract_end(Cpu& c, std::uint32_t contract, std::uint32_t destination) {
    block(c, 0x10372730, 10);
    GuestNativeScope scope(c.s, 0x10372730);
    const auto stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    const auto pool = c.toc(text_pool_slot);
    c.set(5, pool);
    c.put32(stack + 8, link);
    c.put32(stack - date_frame, stack);
    c.set(1, stack - date_frame);
    scope.frame(stack - date_frame);
    const auto packed = c.u8(contract + ContractRecord::packed_kind);
    const auto kind = detail::unpack_kind(packed);
    c.set(0, std::uint32_t(kind));
    c.s.wr<1>(guest::reg::xer_ca, (packed & 0x82) == 0x82);
    if (guest::compare(c, kind, 3) == 0) {
        block(c, 0x10372758, 3);
        c.set({{3, destination}, {4, pool + 0xb9}});
        c.call(localize, 0x10372764);
        block(c, 0x10372764, 1);
    } else {
        block(c, 0x10372768, 2);
        if (guest::compare(c, kind, 4) == 0) {
            block(c, 0x10372770, 4);
            c.set({{3, destination}, {4, pool + 0xca}, {5, pool + 0xf2}});
            c.call(localize, 0x10372780);
            block(c, 0x10372780, 1);
        } else {
            block(c, 0x10372784, 2);
            if (guest::compare(c, kind, 1) == 0) {
                block(c, 0x1037278c, 3);
                c.set({{3, destination}, {4, pool + 0xf3}});
                c.call(localize, 0x10372798);
                block(c, 0x10372798, 1);
            } else {
                block(c, 0x1037279c, 4);
                c.set({{3, contract + ContractRecord::end_date}, {5, 200}, {6, 255}});
                c.call(format_date, 0x103727ac);
            }
        }
    }
    block(c, 0x103727ac, 4);
    restore_link(c, date_frame);
}
}

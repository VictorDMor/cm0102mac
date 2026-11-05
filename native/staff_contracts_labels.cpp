#include "staff_contracts.hpp"
#include "staff_contracts_adapter.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using guest::Cpu;

constexpr std::uint32_t bonus_name_entry = 0x10380270, option_name_entry = 0x10380300;
constexpr std::uint32_t format_text = 0x10727890, copy_text = 0x1077dbf0;
constexpr std::uint32_t read_money = 0x10009f20, format_money = 0x1000b410;
constexpr std::int32_t text_pool = -0x4da4;
constexpr unsigned label_frame = 0x40;

struct Label {
    std::uint32_t pc;
    unsigned offset;
};

void open_frame(Cpu& c, std::uint32_t caller_stack, unsigned size) {
    const auto stack = caller_stack - size;
    c.put32(stack, caller_stack);
    c.set(1, stack);
}

void restore_link(Cpu& c, unsigned size) {
    const auto stack = c.sp() + size;
    const auto link = c.u32(stack + 8);
    c.set({{0, link}, {1, stack}});
    c.set_lr(link);
}

Label bonus_label(Cpu& c, std::int8_t index) {
    if (index == 1) return {0x103802b8, 0x294};
    block(c, 0x1038028c, 1);
    if (index < 1) {
        block(c, 0x10380290, 2);
        if (guest::compare(c, index) >= 0) return {0x103802a8, 0x289};
        block(c, 0x10380298, 1);
    } else {
        block(c, 0x1038029c, 2);
        if (guest::compare(c, index, 3) < 0) {
            block(c, 0x103802a4, 1);
            return {0x103802c8, 0x2a1};
        }
    }
    return {0x103802d8, 0x2b3};
}

Label option_label(Cpu& c, std::int8_t index) {
    if (index == 2) return {0x10380360, 0x2fe};
    block(c, 0x1038031c, 1);
    if (index < 2) {
        block(c, 0x10380320, 2);
        if (guest::compare(c, index) == 0) return {0x10380340, 0x2c6};
        block(c, 0x10380328, 1);
        if (index > 0) return {0x10380350, 0x2e3};
        block(c, 0x1038032c, 1);
    } else {
        block(c, 0x10380330, 2);
        if (guest::compare(c, index, 4) == 0) return {0x10380380, 0x333};
        block(c, 0x10380338, 1);
        if (index < 4) {
            block(c, 0x1038033c, 1);
            return {0x10380370, 0x319};
        }
    }
    return {0x10380390, 0x34e};
}

void write_name(Cpu& c, std::int8_t index, std::uint32_t destination, bool bonus) {
    block(c, bonus ? bonus_name_entry : option_name_entry, 7);
    const auto stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    const auto pool = c.toc(text_pool);
    c.set(5, pool);
    c.put32(stack + 8, link);
    c.set(0, std::uint32_t(index));
    guest::compare(c, index, bonus ? 1 : 2);
    open_frame(c, stack, label_frame);
    const auto label = bonus ? bonus_label(c, index) : option_label(c, index);
    block(c, label.pc, 3);
    c.set({{3, destination}, {4, pool + label.offset}});
    const auto resume = label.pc + 12;
    c.call(format_text, resume);

    const auto epilogue = bonus ? 0x103802e4u : 0x1038039cu;
    if (resume != epilogue) block(c, resume, 1);
    block(c, epilogue, 4);
    restore_link(c, label_frame);
}

void call_name(Cpu& c, std::uint32_t index, std::uint32_t destination, bool bonus, std::uint32_t back) {
    c.set({{3, index}, {4, destination}});
    if (!staff_contracts_calls_directly()) {
        c.call(bonus ? bonus_name_entry : option_name_entry, back);
        return;
    }
    c.link(back);
    write_name(c, std::int8_t(index), destination, bonus);
    c.finish();
    if (c.s.pc != back) run_until(c.s, back);
}
}

void name_bonus(Cpu& c, std::int8_t index, std::uint32_t destination) {
    write_name(c, index, destination, true);
}

void name_option(Cpu& c, std::int8_t index, std::uint32_t destination) {
    write_name(c, index, destination, false);
}

void describe_option(Cpu& c, std::uint32_t index, std::uint32_t destination) {
    block(c, 0x10374cb0, 6);
    const auto stack = c.sp();
    const auto link = c.lr();
    c.set({{0, link}, {3, index}});
    c.put32(stack + 8, link);
    c.set(4, destination);
    open_frame(c, stack, label_frame);
    call_name(c, index, destination, false, 0x10374cc8);
    block(c, 0x10374cc8, 4);
    restore_link(c, label_frame);
}

void describe_bonus(Cpu& c, std::uint32_t contract, std::uint32_t index, std::uint32_t destination,
                    std::uint32_t name_only) {
    constexpr unsigned frame_size = 0x1e0;
    block(c, 0x10374be0, 14);
    const auto caller = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    c.put32(caller - 4, c.r(31));
    const guest::Kept<31> short_form(c, name_only);
    c.put32(caller - 8, c.r(30));
    const guest::Kept<30> text(c, destination);
    c.put32(caller - 12, c.r(29));
    const guest::Kept<29> selector(c, index);
    c.put32(caller - 16, c.r(28));
    const guest::Kept<28> record(c, contract);
    c.set(3, selector);
    c.put32(caller + 8, link);
    open_frame(c, caller, frame_size);
    const auto frame = c.sp();
    const auto label = frame + 0x3c;
    call_name(c, selector, label, true, 0x10374c18);

    block(c, 0x10374c18, 3);
    const auto style = std::int8_t(std::uint32_t(short_form));
    c.set(0, std::uint32_t(style));
    if (guest::compare(c, style, 1) == 0) {
        block(c, 0x10374c24, 3);
        c.set({{3, text}, {4, label}});
        c.call(copy_text, 0x10374c30);
        block(c, 0x10374c30, 1);
    } else {
        block(c, 0x10374c34, 8);
        const auto displacement = std::uint32_t(std::int8_t(std::uint32_t(selector)) * 4);
        const auto selected = record + displacement;
        const auto amount = frame + 0x38;
        c.set({{0, displacement}, {3, amount}, {4, 10}, {5, selected}});
        const auto value = c.u32(selected + ContractRecord::bonus);
        c.set(0, value);
        c.put32(amount, value);
        c.call(read_money, 0x10374c54);

        block(c, 0x10374c54, 6);
        const auto formatted_amount = frame + 0x104;
        c.set({{5, c.r(3)}, {6, c.r(4)}, {3, formatted_amount}, {4, 200}, {7, 10}});
        c.call(format_money, 0x10374c6c);

        block(c, 0x10374c6c, 6);
        const auto pool = c.toc(text_pool);
        c.set({{3, text}, {4, pool + 0xfb}, {5, formatted_amount}, {6, label}});
        c.call(format_text, 0x10374c84);
    }
    block(c, 0x10374c84, 8);
    restore_link(c, frame_size);
    c.set(31, c.u32(c.sp() - 4));
    c.set(30, c.u32(c.sp() - 8));
    c.set(29, c.u32(c.sp() - 12));
    c.set(28, c.u32(c.sp() - 16));
}
}

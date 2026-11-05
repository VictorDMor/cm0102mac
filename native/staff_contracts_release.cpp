#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using guest::compare;
using guest::Cpu;

constexpr unsigned frame_size = 0x70;
constexpr unsigned non_promotion = 0x1c, minimum_fee = 0x1d, relegation = 0x1f, manager_job = 0x20;
constexpr std::uint32_t localize = 0x10727890, plain_format = 0x1077b720;
constexpr std::int32_t source_strings = -0x4da4;

std::int8_t record_signed_byte(Cpu& c, std::uint8_t raw) {
    const auto value = std::int8_t(raw);
    const auto condition = value < 0 ? 8u : value > 0 ? 4u : 2u;
    c.set(0, std::uint32_t(value));
    c.set_cr(0, std::uint8_t((c.cr(0) & 0xf0) | condition | (c.s.r<1>(guest::reg::xer_so) & 1)));
    return value;
}

void report_short_buffer(Cpu& c, std::uint32_t frame, const guest::Kept<31>& source,
                         const guest::Kept<30>& diagnostic, std::uint32_t site, unsigned line) {
    block(c, site, 6);
    c.set({{3, source}, {6, frame + 0x40}, {7, frame + 0x38}, {4, 0}, {5, 0}});
    c.call(0x10739890, site + 24);

    // The original caller inlines the diagnostic context getter.
    block(c, site + 24, 1);
    c.link(site + 28);
    c.set(3, c.toc(-0x5b20));
    c.s.charge(2);

    block(c, site + 28, 7);
    c.set({{5, c.r(3)}, {3, diagnostic}, {4, source + 0x14}, {6, frame + 0x40},
           {7, frame + 0x38}, {8, line}});
    c.call(plain_format, site + 56);

    block(c, site + 56, 4);
    c.set({{4, diagnostic}, {3, source + 0x21}, {5, 0}});
    c.call(0x10769b60, site + 72);

    block(c, site + 72, 5);
    const auto active = c.toc(-0x7bac);
    c.set({{4, active}, {0, 0}, {3, 0}});
    c.put8(active, 0);
}

void emit(Cpu& c, std::uint32_t pool, std::uint32_t pc, unsigned text_offset) {
    block(c, pc, 4);
    c.set({{3, c.r(4)}, {4, pool + text_offset}, {5, pool + 0xf2}});
    c.call(localize, pc + 16);
    block(c, pc + 16, 2);
}

std::uint8_t read_clause(Cpu& c, std::uint32_t record, unsigned field, unsigned reg) {
    const auto value = c.u8(record + field);
    c.set(reg, value);
    return value;
}

void describe_clauses(Cpu& c, const guest::Kept<29>& record, const guest::Kept<31>& pool) {
    block(c, 0x10380dd8, 3);
    const auto buyout = record_signed_byte(c, read_clause(c, record, minimum_fee, 3));
    if (buyout) {
        block(c, 0x10380de4, 3);
        c.set(0, std::uint32_t(buyout));
        if (compare(c, buyout, 1) != 0) {
            block(c, 0x10380df0, 2);
            c.set(0, 1);
            c.put8(record + minimum_fee, 1);
        }
        emit(c, pool, 0x10380df8, 0x460);
        read_clause(c, record, minimum_fee, 3);
        return;
    }

    block(c, 0x10380e10, 3);
    const auto promotion_clause = record_signed_byte(c, read_clause(c, record, non_promotion, 3));
    if (promotion_clause) {
        block(c, 0x10380e1c, 3);
        const auto relegation_clause = record_signed_byte(c, read_clause(c, record, relegation, 5));
        if (relegation_clause) {
            block(c, 0x10380e28, 3);
            c.set(0, std::uint32_t(promotion_clause));
            if (compare(c, promotion_clause, 2) == 0) {
                emit(c, pool, 0x10380e34, 0x4a4);
                c.set(3, 2);
                return;
            }
            block(c, 0x10380e4c, 3);
            c.set(0, std::uint32_t(relegation_clause));
            if (compare(c, relegation_clause, 2) == 0) {
                emit(c, pool, 0x10380e58, 0x4eb);
                c.set(3, 2);
                return;
            }
            emit(c, pool, 0x10380e70, 0x52b);
            c.set(3, 1);
            return;
        }
    }

    block(c, 0x10380e88, 2);
    if (record_signed_byte(c, std::uint8_t(c.r(3)))) {
        emit(c, pool, 0x10380e90, 0x4a4);
        read_clause(c, record, non_promotion, 3);
        return;
    }

    block(c, 0x10380ea8, 3);
    if (record_signed_byte(c, read_clause(c, record, relegation, 0))) {
        emit(c, pool, 0x10380eb4, 0x4eb);
        read_clause(c, record, relegation, 3);
        return;
    }

    block(c, 0x10380ecc, 3);
    if (record_signed_byte(c, read_clause(c, record, manager_job, 0))) {
        emit(c, pool, 0x10380ed8, 0x582);
        read_clause(c, record, manager_job, 3);
        return;
    }

    block(c, 0x10380ef0, 3);
    c.set({{3, c.r(4)}, {4, pool + 0x5c3}});
    c.call(plain_format, 0x10380efc);
    block(c, 0x10380efc, 1);
    c.set(3, 0);
}

constexpr std::uint32_t squad_entry = 0x10380c10;
constexpr std::uint32_t squad_labels_begin = 0x10380cb4, squad_value = 0x10380d30;
constexpr std::uint32_t squad_epilogue = 0x10380d34, squad_end = 0x10380d4c;

void emit_squad_label(Cpu& c, std::uint32_t target, std::uint32_t pool) {
    constexpr std::array offsets{0x419, 0x423, 0x427, 0x432, 0x43b, 0x442, 0x44b, 0x455};
    const auto index = (target - squad_labels_begin) / 16;
    const auto part = (target - squad_labels_begin) % 16;
    if (part <= 8) {
        block(c, target, 3 - part / 4);
        if (part == 0) c.set(3, c.r(4));
        if (part <= 4) c.set(4, pool + offsets[index]);
        c.call(localize, squad_labels_begin + index * 16 + 12);
    }
    if (index < offsets.size() - 1) block(c, squad_labels_begin + index * 16 + 12, 1);
}

void return_squad(Cpu& c, std::uint32_t target) {
    block(c, target, (squad_end - target) / 4);
    if (target <= squad_epilogue) c.set(0, c.u32(c.sp() + frame_size + 8));
    if (target <= squad_epilogue + 4) c.set(1, c.sp() + frame_size);
    if (target <= squad_epilogue + 8) c.set_lr(c.r(0));
    if (target <= squad_epilogue + 12) c.set(31, c.u32(c.sp() - 4));
    if (target <= squad_epilogue + 16) c.set(30, c.u32(c.sp() - 8));
    c.finish();
}
}

void describe_squad_status(Cpu& c, std::uint32_t contract, std::int32_t capacity) {
    block(c, squad_entry, 9);
    GuestNativeScope scope(c.s, squad_entry);
    const auto stack = c.sp();
    const auto caller = c.lr();
    c.set(0, caller);
    c.put32(stack - 4, c.r(31));
    const bool fits = compare(c, capacity, 20) >= 0;
    const guest::Kept<31> pool(c, c.toc(source_strings));
    c.put32(stack - 8, c.r(30));
    const guest::Kept<30> diagnostic(c, c.toc(-0x7ba8));
    c.put32(stack + 8, caller);
    const auto frame = stack - frame_size;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    if (!fits) {
        report_short_buffer(c, frame, pool, diagnostic, 0x10380c34, 0x20ec);
        return_squad(c, squad_epilogue);
        return;
    }

    block(c, 0x10380c90, 4);
    const auto status = c.u8(contract + 0x60) & 0x0fu;
    c.set(0, status);
    std::uint32_t target = 0x10380d24;
    if (guest::compare_logical(c, status, 7) <= 0) {
        block(c, 0x10380ca0, 5);
        const auto table = c.toc(-0x4dc0);
        c.set(3, table);
        c.set(0, status * 4);
        target = c.u32(table + status * 4);
        c.set(3, target);
        c.s.wr<4>(guest::reg::ctr, target);
    }

    // The eight table cases can also enter at their formatting continuations.
    // The audited table has no entries before its first label. A retargeted
    // table may select another label, an epilogue, or an external continuation.
    if (target >= squad_labels_begin && target < squad_value && target % 4 == 0) {
        emit_squad_label(c, target, pool);
        target = squad_value;
    }
    if (target == squad_value) {
        block(c, squad_value, 1);
        c.set(3, 1);
        target = squad_epilogue;
    }
    if (target >= squad_epilogue && target < squad_end && target % 4 == 0) {
        return_squad(c, target);
        return;
    }
    if (target >= squad_entry && target < squad_end)
        throw std::logic_error("Unclassified staff-contract squad label continuation");
    c.s.pc = target;
    run_until(c.s, caller);
}

void describe_release_summary(Cpu& c, std::uint32_t contract, std::int32_t capacity) {
    block(c, 0x10380d50, 11);
    GuestNativeScope scope(c.s, 0x10380d50);
    const auto stack = c.sp();
    const auto link = c.lr();
    c.set(0, link);
    c.put32(stack - 4, c.r(31));
    const bool fits = compare(c, capacity, 20) >= 0;
    const guest::Kept<31> pool(c, c.toc(source_strings));
    c.put32(stack - 8, c.r(30));
    const guest::Kept<30> diagnostic(c, c.toc(-0x7ba8));
    c.put32(stack - 12, c.r(29));
    const guest::Kept<29> record(c, contract);
    c.put32(stack + 8, link);
    const auto frame = stack - frame_size;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    if (fits) describe_clauses(c, record, pool);
    else report_short_buffer(c, frame, pool, diagnostic, 0x10380d7c, 0x2125);

    block(c, 0x10380f00, 7);
    const auto restored_stack = c.sp() + frame_size;
    const auto restored_link = c.u32(restored_stack + 8);
    c.set({{0, restored_link}, {1, restored_stack}});
    c.set_lr(restored_link);
    c.set(31, c.u32(restored_stack - 4));
    c.set(30, c.u32(restored_stack - 8));
    c.set(29, c.u32(restored_stack - 12));
}
}

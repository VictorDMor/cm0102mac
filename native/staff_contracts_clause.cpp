#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using guest::compare;
using guest::compare_logical;
using guest::Cpu;

constexpr unsigned frame_size = 0x40;
constexpr std::uint32_t cfm_call = 0x10781320;
constexpr unsigned competition_index = 0x30, staff_age = 0x5c;
constexpr std::int32_t competition_table_slot = -0x7b5c;

bool competition_allows(Cpu& c, std::uint32_t competition, std::uint32_t pc, unsigned method) {
    block(c, pc, 2);
    bool allowed = false;
    if (compare_logical(c, competition, 0) != 0) {
        block(c, pc + 8, 4);
        c.set(3, competition);
        const auto vtable = c.u32(competition);
        c.set(12, vtable);
        c.set(12, c.u32(vtable + method));
        c.call(cfm_call, pc + 24);

        block(c, pc + 24, 3);
        c.set(2, c.u32(c.sp() + 0x14));
        const auto answer = std::int8_t(c.r(3));
        c.set(0, std::uint32_t(answer));
        const auto condition = answer < 0 ? 8u : answer > 0 ? 4u : 2u;
        c.set_cr(0, std::uint8_t((c.cr(0) & 0xf0) | condition | (c.s.r<1>(guest::reg::xer_so) & 1)));
        allowed = answer != 0;
    }
    block(c, pc + (allowed ? 36 : 44), 2);
    c.set(3, allowed);
    return allowed;
}

bool manager_clause_age(Cpu& c, std::uint32_t staff) {
    block(c, 0x103804e4, 10);
    const auto age = std::int8_t(c.i16(staff + staff_age));
    const std::int32_t different = age ^ 31;
    const bool allowed = age > 31;
    c.set({{4, std::uint32_t(age)}, {0, allowed}, {3, allowed}});
    c.s.wr<1>(guest::reg::xer_ca, different < 0 && (different & 1));
    return allowed;
}

bool clause_eligibility(Cpu& c, std::uint32_t staff, std::uint32_t competition, std::int8_t option) {
    block(c, 0x1038043c, 3);
    c.set(0, std::uint32_t(option));
    if (compare(c, option, 2) == 0) {
        block(c, 0x103804a8, 2);
        c.set(3, 0);
        return false;
    }

    block(c, 0x10380448, 1);
    if (option < 2) {
        block(c, 0x1038044c, 2);
        if (compare(c, option) == 0) return competition_allows(c, competition, 0x1038046c, 0xa0);
        block(c, 0x10380454, 1);
        if (option == 1) {
            block(c, 0x103804a0, 2);
            c.set(3, 1);
            return true;
        }
        block(c, 0x10380458, 1);
    } else {
        block(c, 0x1038045c, 2);
        if (compare(c, option, 4) == 0) return manager_clause_age(c, staff);
        block(c, 0x10380464, 1);
        if (option == 3) {
            block(c, 0x10380468, 1);
            return competition_allows(c, competition, 0x103804b0, 0xa4);
        }
    }

    block(c, 0x1038050c, 1);
    c.set(3, 0);
    return false;
}
}

bool supports_option(Cpu& c, std::uint32_t staff, std::uint32_t club, std::int8_t option) {
    block(c, 0x10380400, 7);
    GuestNativeScope scope(c.s, 0x10380400);
    const auto stack = c.sp();
    const auto link = c.lr();
    c.set({{0, link}, {7, 0}});
    c.put32(stack + 8, link);
    const auto frame = stack - frame_size;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);

    const auto index = c.u32(club + competition_index);
    c.set(6, index);
    std::uint32_t competition = 0;
    if (compare_logical(c, index, 0) != 0) {
        block(c, 0x1038041c, 7);
        const auto table_slot = c.toc(competition_table_slot);
        c.set(4, table_slot);
        const auto number = c.u32(index);
        c.set(0, number);
        const auto table = c.u32(table_slot);
        c.set(4, table);
        const auto displacement = number * 4;
        c.set(0, displacement);
        competition = c.u32(table + displacement);
        c.set(0, competition);
        if (compare_logical(c, competition, 0) != 0) {
            block(c, 0x10380438, 1);
            c.set(7, competition);
        }
    }

    const auto allowed = clause_eligibility(c, staff, competition, option);
    block(c, 0x10380510, 4);
    const auto restored_stack = c.sp() + frame_size;
    const auto restored_link = c.u32(restored_stack + 8);
    c.set({{0, restored_link}, {1, restored_stack}});
    c.set_lr(restored_link);
    return allowed;
}
}

#include "hall_of_fame_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"

#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
bool declining = std::getenv("CM_HALL_OF_FAME_ORIGINAL") != nullptr;
constexpr auto compare_rank_entry = 0x10142100u, initialize_record_entry = 0x101407a0u;

struct RankingRecord {
    Cpu& cpu;
    std::uint32_t address;
    std::uint32_t primary_rank() const { return cpu.u32(address + 0x68); }
    std::int32_t secondary_rank() const { return cpu.i32(address + 0x64); }
};

void block(Cpu& c, std::uint32_t pc, unsigned count) {
    c.s.step(pc);
    c.s.charge(count - 1);
}

void finish(Cpu& c, std::uint32_t pc, int result) {
    block(c, pc, 2);
    c.set(3, std::uint32_t(result));
    c.finish();
}

void initialize_record(Cpu& c) {
    GuestNativeScope scope(c.s, initialize_record_entry);
    block(c, initialize_record_entry, 7);
    const auto address = c.r(3), caller = c.sp(), frame = caller - 0x40;
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set(31, address);
    c.put32(caller + 8, c.r(0));
    c.set(3, address + 0x70);
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    c.call(0x10715880, 0x101407bc);
    block(c, 0x101407bc, 6);
    c.set(3, address);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.finish();
}

void compare_rank(Cpu& c) {
    const RankingRecord left{c, c.r(3)}, right{c, c.r(4)};
    block(c, compare_rank_entry, 2);
    c.compare_logical(0, left.address, 0);
    if (!left.address) return finish(c, 0x10142108, 1);
    block(c, 0x10142110, 2);
    c.compare_logical(0, right.address, 0);
    if (!right.address) return finish(c, 0x10142118, -1);

    block(c, 0x10142120, 4);
    const auto right_primary = right.primary_rank();
    c.set(0, right_primary);
    const auto left_primary = left.primary_rank();
    c.set(5, left_primary);
    c.compare_logical(0, right_primary, left_primary);
    if (right_primary > left_primary) return finish(c, 0x10142130, 1);
    block(c, 0x10142138, 2);
    c.compare_logical(0, left_primary, right_primary);
    if (left_primary > right_primary) return finish(c, 0x10142140, -1);

    block(c, 0x10142148, 4);
    const auto right_secondary = right.secondary_rank();
    c.set(4, std::uint32_t(right_secondary));
    const auto left_secondary = left.secondary_rank();
    c.set(5, std::uint32_t(left_secondary));
    c.compare(0, right_secondary, left_secondary);
    if (left_secondary > right_secondary) return finish(c, 0x10142158, 1);

    block(c, 0x10142160, 5);
    // The original subtracts sign bits with the carry from the unsigned
    // words. Keep the resulting carry separately from the ranking decision.
    const auto left_sign = unsigned(left_secondary < 0);
    const auto right_sign = unsigned(right_secondary < 0);
    const auto borrow = unsigned(std::uint32_t(left_secondary) < std::uint32_t(right_secondary));
    c.set(0, left_sign);
    c.s.wr<1>(guest::reg::xer_ca, right_sign >= left_sign + borrow);
    c.set(3, left_secondary == right_secondary ? 0 : UINT32_MAX);
    c.finish();
}
}

void hall_of_fame_run_original(State& state, std::uint32_t entry) {
    run_original_body(state, entry, declining);
}

void hall_of_fame_run_rewrite(State& state, std::uint32_t entry) {
    Cpu c(state);
    switch (entry) {
    case initialize_record_entry: initialize_record(c); break;
    case compare_rank_entry: compare_rank(c); break;
    default: throw std::logic_error("Unknown Hall of Fame entry " + hex_address(entry));
    }
}

bool try_modern_hall_of_fame(State& state, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(state, entry, hall_of_fame_run_original, hall_of_fame_run_rewrite,
                               RoutineChecker::result_r3);
    hall_of_fame_run_rewrite(state, entry);
    return true;
}
}

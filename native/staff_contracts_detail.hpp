#pragma once

#include "guest_values.hpp"
#include "staff_contracts_adapter.hpp"

namespace cm::aot::staff_contracts::detail {
using guest::Cpu;

// Keep the original straight-line budget boundaries for a stop inside a
// partially completed operation. The algorithm between them uses C++ values.
inline void block(Cpu& c, std::uint32_t pc, unsigned instructions) {
    c.s.step(pc);
    c.s.charge(instructions - 1);
}

template<class Operation, class... Args>
void call_owned(Cpu& c, std::uint32_t entry, std::uint32_t resume, Operation operation, Args... args) {
    if (!staff_contracts_calls_directly()) {
        c.call(entry, resume);
        return;
    }
    c.link(resume);
    operation(c, args...);
    c.finish();
    if (c.s.pc != resume) run_until(c.s, resume);
}

inline void call_owned_continuation(Cpu& c, std::uint32_t entry, std::uint32_t resume, void (*operation)(Cpu&)) {
    if (!staff_contracts_calls_directly()) {
        c.call(entry, resume);
        return;
    }
    c.link(resume);
    operation(c);
    if (c.s.pc != resume) run_until(c.s, resume);
}

// Record forms replace only CR0's low nibble.
inline bool record_nonzero(Cpu& c, std::int32_t value, unsigned destination = 0) {
    c.set(destination, std::uint32_t(value));
    const auto upper = c.cr(0) & 0xf0;
    const auto nonzero = guest::nonzero(c, value);
    c.set_cr(0, std::uint8_t(upper | c.cr(0)));
    return nonzero;
}

// The exported multiword moves leave the transfer address live in TEA,
// including the address of the first word that faults.
template<unsigned First, bool Restore = false>
void transfer_saved_registers(Cpu& c, std::uint32_t caller_stack) {
    constexpr std::size_t tea = 2432;
    auto address = caller_stack - (32 - First) * 4;
    c.s.wr<4>(tea, address);
    for (unsigned r = First; r < 32; ++r) {
        if constexpr (Restore) c.set(r, c.u32(address));
        else c.put32(address, c.r(r));
        address += 4;
        c.s.wr<4>(tea, address);
    }
}

inline bool qualifies_for_veteran_option(Cpu& c, std::uint32_t person,
                                         std::uint32_t pc, unsigned reputation_reg) {
    block(c, pc, 4);
    const auto age = std::int8_t(c.i16(person + 0x5c));
    c.set(0, std::uint32_t(age));
    if (guest::compare(c, age, 32) <= 0) return false;
    block(c, pc + 0x10, 3);
    const auto reputation = std::int16_t(c.r(reputation_reg));
    c.set(3, guest::extended(reputation));
    if (guest::compare(c, reputation, 3250) <= 0) return false;
    block(c, pc + 0x1c, 9);
    c.set(4, 0x55550000);
    const auto id = c.i32(person);
    const auto third = (INT64_C(0x55555556) * id) >> 32;
    c.set({{3, std::uint32_t(id)}, {4, 0x55555556}, {5, std::uint32_t(third)}});
    if (record_nonzero(c, id % 3, 4)) return false;

    block(c, pc + 0x40, 21);
    c.set(4, 0x66660000);
    const auto first = c.u8(person + 0x64);
    c.set(6, first);
    c.set(5, 0x66666667);
    const auto second = c.u8(person + 0x67);
    c.set(4, second);
    const auto high = (INT64_C(0x66666667) * id) >> 32;
    c.set(7, std::uint32_t(high));
    const auto third_byte = c.i8(person + 0x63);
    const auto combined = std::int8_t(first) + std::int8_t(second);
    const auto tenth = id / 10;
    const auto previous = std::int32_t(c.r(0));
    const auto metric = combined + id % 10 + previous;
    const auto final_metric = metric - 28 + third_byte;
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 3));
    c.set({{6, std::uint32_t(tenth * 10)}, {4, std::uint32_t(combined)}, {5, std::uint32_t(third_byte)},
           {7, high < 0 ? 1u : 0u}, {3, std::uint32_t(metric)}, {0, std::uint32_t(final_metric)}});
    return guest::compare(c, final_metric, 45) > 0;
}

struct ContractRecord {
    static constexpr unsigned club_id = 4;
    static constexpr unsigned draft_auxiliary = 8;
    static constexpr unsigned bonus = 0x10;
    static constexpr unsigned option = 0x1c;
    static constexpr unsigned release_amount = 0x24;
    static constexpr unsigned start_date = 0x28;
    static constexpr unsigned end_date = 0x30;
    static constexpr unsigned packed_kind = 0x38;
    static constexpr unsigned review_marker = 0x42;
    static constexpr unsigned status = 0x48;
    static constexpr unsigned review_conditions = 0x50;
    static constexpr unsigned terms_flag = 0x54;
    static constexpr unsigned terms_marker = 0x58;
    static constexpr unsigned review_flags = 0x5c;
};

inline std::int32_t unpack_kind(std::uint8_t packed) {
    return std::int8_t(packed & 0xfc) / 4;
}
}

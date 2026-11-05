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

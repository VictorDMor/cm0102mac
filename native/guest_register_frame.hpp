#pragma once

#include "aot_runtime.hpp"

namespace cm::aot {

// Guest registers and guest memory use the same byte order. Transfer the
// entire stmw/lmw range after proving that each constituent access is mapped.
// The generated scalar body handles observers, locks and partial faults.
struct GuestRegisterFrame {
    // The page index can certify a contiguous span without a range scan.
    static bool mapped_span(const Memory& memory, std::uint32_t address, unsigned size) {
        const auto page = address >> 16;
        const auto end = std::uint64_t(address) + size;
        if (end <= (*memory.page_ends_)[page]) return true;
        const auto& partial = (*memory.partial_starts_)[page];
        return address >= partial.start && end <= partial.end;
    }

    // Borrow a mapped scratch range for an audited call-free sequence.
    static std::uint8_t* unobserved_scratch(Memory& memory, std::uint32_t address, unsigned size) {
        if (memory.date_observer || memory.read_trace ||
            (memory.observer && memory.observer->observes_write(address, size)) ||
            !mapped_span(memory, address, size)) return nullptr;
        return memory.data_ + address;
    }

    template <unsigned First, bool Restore>
    static bool try_transfer(State& s, std::uint32_t address) {
        static_assert(First < 32);
        auto& memory = s.memory;
        constexpr auto size = (32u - First) * 4;
        if constexpr (Restore) {
            if (memory.date_observer || memory.read_trace) return false;
        } else {
            if (memory.date_observer ||
                (memory.observer && memory.observer->observes_write(address, size))) return false;
        }

        if (!mapped_span(memory, address, size)) return false;

        auto* const guest = memory.data_ + address;
        auto* const registers = s.registers.data() + First * 4;
        if constexpr (Restore) std::memcpy(registers, guest, size);
        else std::memcpy(guest, registers, size);
        s.wr<4>(2432, address + size);
        return true;
    }
};

}

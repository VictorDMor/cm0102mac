#pragma once

#include "guest_values.hpp"

namespace cm::aot::contract_screens::detail {
using namespace guest;

struct Controls {
    Cpu& cpu;
    std::uint32_t screens, layout;
    std::int16_t control;

    std::uint32_t address() const { return layout + 0xc0000 + std::uint32_t(control) * 0x17c; }

    std::int16_t action(unsigned scratch) const {
        std::int16_t selected = 0;
        if (compare(cpu, control, -1) != 0) {
            cpu.set(scratch, address());
            selected = std::int16_t(cpu.u32(address() - 0x5794));
        }
        if (compare(cpu, selected) == 0) {
            cpu.set(scratch, screens + 0x260000);
            selected = cpu.i16(screens + 0x260000 - 0x7e62);
        }
        cpu.set(0, std::uint32_t(selected));
        return selected;
    }

    std::uint32_t value(unsigned scratch = 3) const {
        cpu.set(0, std::uint32_t(control));
        if (compare(cpu, control, -1) == 0) return 0;
        cpu.set(0, std::uint32_t(control) * 0x17c);
        cpu.set(scratch, address());
        return cpu.u32(address() - 0x5754);
    }
};

inline bool offer_changed(Cpu& c, std::uint32_t offer, std::uint32_t revision) {
    const auto saved_revision = c.u32(offer + 0x2c);
    c.set(0, saved_revision);
    if (compare(c, saved_revision, revision) != 0) return true;
    const auto status = c.i8(offer + 0x34);
    c.set(0, std::uint32_t(status));
    return compare(c, status, 3) == 0 || compare(c, status, 2) == 0;
}

inline bool needs_refresh(Cpu& c, std::uint32_t offer, std::uint32_t revision) {
    return compare_logical(c, offer) != 0 && offer_changed(c, offer, revision);
}
}

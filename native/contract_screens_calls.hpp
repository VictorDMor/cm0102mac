#pragma once

#include "contract_screens_adapter.hpp"
#include "guest_abi.hpp"

namespace cm::aot::contract_screens::detail {
template<class... Args>
void call_owned(guest::Cpu& cpu, std::uint32_t entry, std::uint32_t back,
                void (*body)(guest::Cpu&, Args...), Args... arguments) {
    unsigned reg = 3;
    (cpu.set(reg++, arguments), ...);
    if (contract_screens_can_call_directly(entry)) {
        cpu.link(back);
        body(cpu, arguments...);
        cpu.s.pc = back;
    } else {
        cpu.call(entry, back);
    }
}
}

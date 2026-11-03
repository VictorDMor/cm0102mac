#pragma once

#include "contract_screens_adapter.hpp"
#include "guest_abi.hpp"
#include <type_traits>

namespace cm::aot::contract_screens::detail {
template<class Result, class... Args>
Result call_owned(guest::Cpu& cpu, std::uint32_t entry, std::uint32_t back,
                Result (*body)(guest::Cpu&, Args...), Args... arguments) {
    static_assert(std::is_void_v<Result> || std::is_same_v<Result, std::uint32_t>);
    unsigned reg = 3;
    (cpu.set(reg++, arguments), ...);
    if (contract_screens_can_call_directly(entry)) {
        cpu.link(back);
        if constexpr (std::is_void_v<Result>) body(cpu, arguments...);
        else cpu.set(3, body(cpu, arguments...));
        cpu.s.pc = back;
    } else {
        cpu.call(entry, back);
    }
    if constexpr (!std::is_void_v<Result>) return cpu.r(3);
}
}

#pragma once
#include "guest_abi.hpp"

namespace cm::aot::cash {
using guest::Cpu;
std::uint32_t symbol_length(Cpu&);
std::uint64_t display_amount(Cpu&, std::uint32_t amount, std::uint32_t flags);
void store_amount(Cpu&, std::uint32_t amount, std::uint64_t displayed);
std::uint32_t increase(Cpu&, std::uint32_t amount, std::uint32_t flags);
std::uint32_t decrease(Cpu&, std::uint32_t amount, std::uint32_t flags);
std::uint32_t standard(Cpu&, std::uint32_t amount, std::uint32_t index);
std::uint32_t preset(Cpu&, std::uint32_t amount, std::uint32_t index,
                     std::uint32_t table, std::uint32_t count);
std::uint32_t configure(Cpu&, std::uint32_t currency);
std::uint32_t setting(Cpu&);
std::uint32_t label(Cpu&, std::uint32_t output, std::uint32_t capacity, std::uint32_t currency);
std::uint32_t format(Cpu&, std::uint32_t output, std::uint32_t capacity,
                     std::uint64_t amount, std::uint32_t flags);
std::uint64_t quantum(Cpu&, std::uint64_t amount, std::uint32_t flags);
}

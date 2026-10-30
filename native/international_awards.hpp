#pragma once
#include "guest_abi.hpp"

namespace cm::aot::international_awards {
using guest::Cpu;
std::uint32_t construct(Cpu&, std::uint32_t manager, std::uint32_t file);
void initialize(Cpu&, std::uint32_t manager, std::uint32_t file);
void publish_due(Cpu&, std::uint32_t manager, std::uint32_t date);
}

#pragma once
#include "guest_abi.hpp"

namespace cm::aot::siprint {
using guest::Cpu;
std::uint32_t destroy(Cpu&, std::uint32_t printer, std::int16_t deleting);
void initialize(Cpu&, std::uint32_t printer);
void idle(Cpu&);
void load_format(Cpu&, std::uint32_t printer);
void save_format(Cpu&, std::uint32_t printer);
std::int32_t available(Cpu&, std::uint32_t printer);
void page_setup(Cpu&, std::uint32_t printer);
void print_lines(Cpu&, std::uint32_t printer, std::uint32_t lines,
                 std::int32_t count, std::uint32_t columns);
void initialize_global(Cpu&);
}

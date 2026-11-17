#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_a": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// The texts of four transfer news messages: `item` is the news item, `full` the original's flag byte
// (non-zero: the long text), `output` the NEWS.CPP message the text goes to.
// 0x104106d0: kind 3020.
void news_text_3020(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x10410a60: kind 4032 (may write word 10 or 11 of the stored item).
void news_text_4032(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x104116e0: kind 4035.
void news_text_4035(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x103fe1a0: kind 4000 (may write word 9 of the stored item).
void news_text_4000(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
}

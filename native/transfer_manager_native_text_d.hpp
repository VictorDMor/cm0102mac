#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_d": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// The texts of transfer news messages: `item` is the news item, `full` the original's flag byte
// (non-zero: the long text), `output` the NEWS.CPP message the text goes to.
// 0x10408f80: kind 4009.
void news_text_4009(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x10404530: kind 4004; `manager` is r3 (read by the refusal texts it calls).
void news_text_4004(guest::Cpu& c, std::uint32_t manager, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x10405ca0: kind 4005 (may write word 9 of the stored item).
void news_text_4005(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x104101f0: kind 4031.
void news_text_4031(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
}

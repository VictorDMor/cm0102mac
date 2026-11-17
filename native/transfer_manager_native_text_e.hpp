#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_e": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// News message 3011 (0x1040fda0): a person dated by a day, and a member of the staff table named with him.
void news_text_3011(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
// 4011 (0x10409410): two clubs and a person, in the wordings of the marker (field 6) and the flags byte (field 10).
void news_text_4011(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
// 4013 (0x10409bd0): two clubs and two people on two dates, with an amount as cash text when the second figure is the larger.
void news_text_4013(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
// 4016 (0x1040a280): a club, a person and a second club in about forty wordings picked by the kind (field 7).
void news_text_4016(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
// 4017 (0x1040bc10): a person and one or two clubs (or a second person), with the person's name text from DATABASE.CPP.
void news_text_4017(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
}

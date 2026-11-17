#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_f": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// The texts of transfer news messages: `item` is the news item, `full` the original's flag byte
// (non-zero: the long text), `output` the NEWS.CPP message the text goes to.
// 0x1040df70: a person and two clubs, the long text with a month and year.
void news_text_1040df70(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x1040e490: a person and two clubs; a code (field 6) selects the sentence.
void news_text_1040e490(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x1040eda0: a person and one or two clubs, optionally a second person.
void news_text_1040eda0(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x1040f450: a person, a club and a second club (-1 for none), with a name word of the first club.
void news_text_1040f450(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x1040f990: two people and two clubs, with a fee when field 10 is above field 11.
void news_text_1040f990(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x1040d0d0: a person and one or two clubs; word 10 of the stored item follows the offer's stage.
void news_text_1040d0d0(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x1040d820: a person and two clubs; word 9 of the stored item follows the offer's stage.
void news_text_1040d820(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
// 0x1040c580: a person, two clubs, two amounts and up to three more people.
void news_text_1040c580(guest::Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output);
}

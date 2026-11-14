#pragma once
// INJURY.CPP: what the game says about injuries: the names and descriptions of injury types, the
// texts of screens and the news a recovery or an injury makes (block "text").
//
// Entries that a caller reaches in the original take their arguments in r3 onwards; the routines
// below that call each other (injury_name_text, language_flag, form_text) expect the arguments of
// the original call in r3-r7 as well, because the varargs formatter they call reads r3-r10.
#include "guest_abi.hpp"

namespace cm::aot::injury_unit {
// 0x1018c1a0: the reading of the clock object (manager +0xc) for the two words at +0x18/+0x1c of the
// person's status row; 0 without a clock. A null person is reported (line 0xd71) and gives 0.
std::uint32_t row_date_reading(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person);
// 0x1018c270: the degree of a status row's byte +0x15 (at least 1), 0 for a person that is null, past
// the people count, without a detail record (person +0x14) or whose kind byte (+0x14) is negative.
std::int32_t status_degree_of(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person);
// 0x1018c2f0: the status row's kind byte (+0x14), -1 for a person that is null, past the people count
// or without a detail record.
std::uint32_t status_kind_of(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person);
// 0x1018c450: the injury name of a person's current injury into `buffer` (style as injury_name_text),
// "None" without one. A null person is reported (line 0xe4c).
void injury_name_of_person(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t buffer,
                           std::uint32_t size, std::uint32_t style);
// 0x1018c560: the text of an injury type into `buffer`, which must hold at least 30 bytes (else
// reported, line 0xe76). Style 0 gives the plain names, style 1 the descriptions, except in language 3
// where both give the plain names; another style writes nothing. A type past 81 is reported.
void injury_name_text(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t type, std::uint32_t buffer,
                      std::int32_t size, std::uint32_t style);
// 0x1018d140: what the language and the injury type give (0, 1 or 2); 1 for a type past 76 in languages
// 9, 3 and 5, 0 in any other language.
std::int32_t language_flag(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t type);
// 0x1018d950: the time the person's injury still has to run, as text: the days of the record's first
// length (`which` non-zero) or second one, less the days since it started, formatted by 0x1030e880 into
// `out`. 0 (and an empty text) without an injury; a null person is reported (line 0x1056).
std::uint32_t time_left_text(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t out,
                             std::uint32_t first, std::uint32_t which, std::uint32_t second);
// 0x1018dab0: the text of one of 13 forms into `buffer` (at least 20 bytes, else reported, line 0x10b9) and
// 2 in language 3, else 0. A form past 12 writes nothing and is reported (line 0x10d0).
std::uint32_t form_text(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t form, std::uint32_t buffer,
                        std::int32_t size);
// 0x1018f340: 1 when the person's current injury is of type 81, else 0. A null person is reported.
std::uint32_t has_injury_81(guest::Cpu& c, std::uint32_t person);
// 0x1018f470: the news of a recovery (the record `news`, `style` 0 light training, else full training) into
// the text sink, with the person's name attached when 0x100cbde0 allows it.
void recovery_news(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t news, std::uint32_t style,
                   std::uint32_t sink);
// 0x1018fbe0: the line a match result adds to the news of an injury: the score, the clubs and the competition.
void match_result_news(guest::Cpu& c, std::uint32_t news, std::uint32_t sink);
// 0x1018fdf0: copies the clock object's reading into the status row's two words (+0x18/+0x1c). A null
// person is reported twice (lines 0x1462, 0x1466), each followed by 0x1076f7d0(-1).
void record_row_date(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person);
}

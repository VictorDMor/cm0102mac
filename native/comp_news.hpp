#pragma once
// COMP.CPP, block `news`: the news items a competition files (comp_news.cpp) and the texts that render them and a
// club's fate in a league (comp_texts*.cpp). Included at the end of comp.hpp.
#include "comp.hpp"

namespace cm::aot::comp {
// A news item of kind 0x1a: a club's prize money in the competition. Filed (0x102cc140, under the club) when the news
// manager has news on and the 64-bit amount (high:low) is at least 50 times the club's halfword +0x1ec. `place`, `round`
// and `flag` are the raw r7, r8 and r10 words (stored as signed byte, signed halfword, signed byte); `other` the
// competition object whose two sides' flags the item records (0: none).
void news_prize_money(Cpu&, std::uint32_t competition, std::uint32_t club, std::uint32_t amount_high, std::uint32_t amount_low,
                      std::uint32_t place, std::uint32_t round, std::uint32_t other, std::uint32_t flag);  // 0x10452010
// A news item of kind 0x1f: the club, the competition, a value and the competition's year (filed under the club).
void news_competition_year(Cpu&, std::uint32_t competition, std::uint32_t club, std::uint32_t value);       // 0x104527c0
// A news item of kind 0x23: a promotion or relegation of club `first` (non-null, else the assertion); `second` and
// `third` are optional (null: -1); `kind` a signed byte. Filed under the competition and, when `third` is not null, under it.
void news_movement(Cpu&, std::uint32_t competition, std::uint32_t third, std::uint32_t first, std::uint32_t second,
                   std::uint32_t kind);                                                                       // 0x10452a60

// The texts. `output` is the text object (0x1074b830 sets its text, 0x1074bab0 appends a link row).
// A club's fate text in a league (r5 `kind` 0..3 and 5 pick the wording, `plain` the form without the competition's
// name); `mode` != 0 hands over to 0x10469710 (`extra` zero) or 0x10464d00.
void league_fate_text(Cpu&, std::uint32_t competition, std::uint32_t club, std::uint32_t kind, std::uint32_t mode, std::uint32_t pass,
                      std::uint32_t stack_byte, std::uint32_t form, std::uint32_t output);                    // 0x10450030
// A stage's position text ("Winners", ...) into `text` (r4); `position` < 30 asserts.
void position_text(Cpu&, std::uint32_t text, std::uint32_t position, std::uint32_t code_word, std::uint32_t second_word,
                   std::uint32_t kind, std::uint32_t club_record);                                             // 0x10450580
// A club's place text into `text` (empty first); returns `text`.
std::uint32_t place_text(Cpu&, std::uint32_t competition, std::uint32_t text, std::uint32_t club, std::uint32_t stage);  // 0x10451ac0
void prize_news_text(Cpu&, std::uint32_t competition, std::uint32_t item, std::uint32_t form, std::uint32_t output);       // 0x10452290
void money_news_text(Cpu&, std::uint32_t competition, std::uint32_t item, std::uint32_t form, std::uint32_t output);       // 0x10452880
void movement_news_text(Cpu&, std::uint32_t competition, std::uint32_t item, std::uint32_t form, std::uint32_t output);    // 0x10452bb0
}

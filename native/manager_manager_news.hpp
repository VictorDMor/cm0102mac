#pragma once
// MANAGER_MANAGER.CPP manager news text block.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101b9b50: the news item of a club's manager after a match. `object` is the module object, `club` the manager's
// club, `opponent` the other club, `competition` the fixture's competition (0 prints only the opponent's text into
// the news line and answers 0), `code` the event, and `phrase` a string the text embeds. The caller's stack
// carries eight more arguments from its sp + 0x38: the pointer to a triple of person ids (word 0x38) and the
// mode (word 0x3c) for the comment, and five bytes (the low bytes of the words 0x44..0x54): `kind`, `expected`,
// `actual`, `limit` and `commented`, which pick a second paragraph on the result against the expectation.
// The club's own text goes into the frame, the event's text into the frame too (a code without a rule, the
// fallback, formats it into the news line and answers 0), and the posted item (0x1077b720) holds the text and,
// when there is one, the comment. Answers 1 once it posted.
std::uint32_t compose(guest::Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t opponent,
                      std::uint32_t competition, std::uint32_t code, std::uint32_t phrase);
// 0x101bfd10: the comment naming people, called by compose when the paragraph is empty and the code has bit
// 0x800 or 0x1000. `people` is a triple of person ids (-1 for none; null returns at once), `named` a byte, `text`
// the buffer, `mode` a word (0, 1, 2 or other) that picks the wording. With `named` set a present id formats the
// first form into `text`, which the original then empties again (its second form needs the flag clear), so the
// text always ends empty; with it clear a present id formats the second form, an absent one leaves `text`
// empty. The name routine (0x100caeb0) gets r3 (flag set) or r28 (clear) as the person, so a person found in a
// later slot, or none in the first, leaves the caller's r3 or r28 there (reproduced).
void comment(guest::Cpu& c, std::uint32_t people, std::uint32_t named, std::uint32_t text, std::uint32_t mode);
}

#pragma once
// FINANCE.CPP, block news: the board and stadium stories the finance code posts to the news desk, and the
// text of each (NEWS.CPP calls the text routines, once each). A story is a news item of a kind (0x1779..0x1783)
// built in the poster's frame: numbered words, then it is handed to the desk with its subject.
//
// Which story is which, from the texts (the ledger names in finance_ledger.hpp are guesses; none of these
// routines touches the ledger):
//   0x1779  the board's money events, by event 1..7: transfer funds made available, a new board takes control,
//           a cash injection (3, 4), a share issue, a sponsorship deal, the club becomes a PLC.
//   0x177a  a stand or terrace closed by the Health and Safety Inspector.
//   0x177b  a financial crisis (0) or the receivers called in (other).
//   0x177c  shareholders' dividends.
//   0x177d  stadium works by kind 1..8: more seats, more standing room, seats installed, stadium bought, sold,
//           undersoil heating installed or failed, training facilities.
//   0x1780  the club plans to enlarge (1) or redevelop (0) the stadium to enter a competition.
//   0x1783  receivers appeased.
//   0x1788  posted by post_expansion_news; no text of this block is for it.
//   0x177f  posted by post_stadium_link_news; no text of this block is for it either. The texts of
//           expansion_text (kinds 0x177e?) and relocation_text (0x1781/0x1782?) have no poster in this block: the kinds
//           those two read are not established, the texts only read the words of the item they are given.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::finance {
using guest::Cpu;

// The posters. `club` is the club record the story is about (its index is the record's word at +0).
// Raw words are the registers the original keeps in its callee-saved registers; a signed byte or halfword
// is the low part of the register, sign-extended where the original does.
void post_board_event_news(Cpu&, std::uint32_t club, std::uint32_t injection, std::uint32_t budget, std::uint32_t event);  // 0x10101ff0
void post_stand_closed_news(Cpu&, std::uint32_t club, std::uint32_t seats, std::uint32_t terracing);                   // 0x10102590
void post_expansion_news(Cpu&, std::uint32_t club, std::uint32_t first, std::uint32_t second);                        // 0x10102820
void post_financial_crisis_news(Cpu&, std::uint32_t club);                                                           // 0x10102a70
void post_dividend_news(Cpu&, std::uint32_t club, std::uint32_t amount);                                             // 0x10102c90
void post_stadium_works_news(Cpu&, std::uint32_t club, std::uint32_t quantity, std::uint32_t kind);                  // 0x10102e00
void post_stadium_link_news(Cpu&, std::uint32_t club, std::uint32_t record);                                         // 0x101030c0
void post_receivers_appeased_news(Cpu&, std::uint32_t club_index);                                                   // 0x10103270
void post_competition_stadium_news(Cpu&, std::uint32_t club_index, std::uint32_t competition, std::uint32_t redevelop);  // 0x10103380

// The texts: `item` is the news item, `body` a signed byte (the headline when 0), `out` the text object. The
// item's words are read with 0x102cb570 (the word at +8+4n).
void board_event_text(Cpu&, std::uint32_t item, std::uint32_t body, std::uint32_t out);        // 0x101020c0
void stand_closed_text(Cpu&, std::uint32_t item, std::uint32_t body, std::uint32_t out);       // 0x10102660
void expansion_text(Cpu&, std::uint32_t item, std::uint32_t body, std::uint32_t out);          // 0x10102910
void financial_crisis_text(Cpu&, std::uint32_t item, std::uint32_t body, std::uint32_t out);   // 0x10102b50
void dividend_text(Cpu&, std::uint32_t item, std::uint32_t body, std::uint32_t out);           // 0x10102d20
void stadium_works_text(Cpu&, std::uint32_t item, std::uint32_t body, std::uint32_t out);      // 0x10102ea0
void relocation_text(Cpu&, std::uint32_t item, std::uint32_t body, std::uint32_t out);         // 0x10103180
void receivers_appeased_text(Cpu&, std::uint32_t item, std::uint32_t body, std::uint32_t out); // 0x101032e0
void competition_stadium_text(Cpu&, std::uint32_t item, std::uint32_t body, std::uint32_t out);  // 0x10103420
}

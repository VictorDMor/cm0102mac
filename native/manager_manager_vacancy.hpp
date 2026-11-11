#pragma once
// MANAGER_MANAGER.CPP vacancy block, written native-first: a manager's application for a club's vacancy (the board's
// answer, the news it files, the candidates the club weighs) and the texts of three of those news messages.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101bec80 (r4 = a move record, r5 = the manager's index, r6 = a byte): files the news message 0x29 (the two
// clubs' ids (+0x1c, +0x20), the record at +0x14's id, the index, that record's +8 record's +0x68 and the byte) to
// both clubs' news desks, unless the global date copy (TOC -0x7a9c) is after today's. Nothing for a null move.
void file_vacancy_news(guest::Cpu& c, std::uint32_t move, std::uint32_t manager_index, std::int8_t kind);
// 0x101bef90 (r4 = club record, r5, r6, r7 = three bytes): files the news message 0x1789 (the club's id, two
// bytes, the id of the record at +0x2c (+0x68) and a third byte) to the club's desk; nothing for a null club or one
// without the +0x2c record.
void file_answer_news(guest::Cpu& c, std::uint32_t club, std::int8_t first, std::int8_t second, std::int8_t third);
// 0x101bf3a0 (r4 = pointer to a word, r5 = record, r6, r7): files the news message 0x2b with the record's id (+0x68),
// two words and the pointed word, through 0x102cc400; nothing for a null record.
void file_offer_news(guest::Cpu& c, std::uint32_t id_pointer, std::uint32_t record, std::uint32_t first, std::uint32_t second);
}

namespace cm::aot::manager_manager {
// The texts of three news messages (called from another module through the news table): r4 = the message record,
// r5 = flag, r6 = the output object. With the flag set the full body is built, with the flag clear only the headline.
// 0x101bede0: message 0x29 (file_vacancy_news); 0x101bf070: message 0x1789 (file_answer_news) and the
// board's reply of message 6025; 0x101bf460: message 0x2b (file_offer_news).
void vacancy_news_text(guest::Cpu& c, std::uint32_t message, std::uint32_t flag, std::uint32_t out);
void answer_news_text(guest::Cpu& c, std::uint32_t message, std::uint32_t flag, std::uint32_t out);
void offer_news_text(guest::Cpu& c, std::uint32_t message, std::uint32_t flag, std::uint32_t out);
}

namespace cm::aot::manager_manager {
// 0x101bdf80 (r3 = the world's table of 0x4c-byte club-news rows, r4 = the club record, r5 = the kind
// byte): the board's answer to a manager's request. It counts the request in the row's first byte, then, against the
// limit its reputation gives (2, or 3 above 0xcb2): kind 1 searches the manager's pending moves and, for a club
// interested in him, rewrites that club's record (draws), kind 2 weighs the club's pending move against its squad
// and the date (draw) and either refuses (news 0x1789) or accepts it and files the move's news (0x101bec80). Files
// the answer through 0x101bef90 and 0x101bf3a0; some paths adjust the row's reputation halfword (+6) and call the
// board's review 0x101a8ad0.
void answer_request(guest::Cpu& c, std::uint32_t table, std::uint32_t person, std::uint32_t kind);
}

namespace cm::aot::manager_manager {
// 0x101c00d0 (r4 = a candidate record, r5 = a key): the standing of the candidate in the 0x1029ec40 roster of the
// key (two squads of 20 slots): 0 when he is in neither, otherwise -1 when his slot's rating byte is below 7, 1 when
// above, 0 at 7, for a slot among the first 11 of its squad (a later slot is not counted).
std::int8_t roster_standing(guest::Cpu& c, std::uint32_t candidate, std::uint32_t key);

// 0x101bf5c0: the candidates a club (r5) takes from its staff list for the move r4 (r3 = the news-row table, r6 =
// where to write up to three of their ids, -1 filled first, r7 = the key, r8 = a pointer to the mode byte: 0 looks
// for the weaker candidates, otherwise for the stronger). Draws twice (the club's willingness), reports through the
// byte pointers r9, r10 and the three stack words (the caller's sp+0x38, +0x3c, +0x40), adjusts the club's news row
// reputation by +-10 per grade. Returns how many it took.
struct Candidates {
    std::uint32_t table, move, club, ids, key, mode, out_a, out_b, out_c, out_d, out_e;
};
std::int8_t gather_candidates(guest::Cpu& c, const Candidates& a);
Candidates read_candidates(const guest::Cpu& c);

// 0x101b9130 (r3 = the news-row table, r4 = a person pointer, r5 = a second pointer, r6 = a move record, r7 = a
// halfword, r8, r9, r10 = three numbers, and a seventh at the caller's sp+0x38): files the news message 0x138c
// to r4's desk: its numbers, the candidates gathered by gather_candidates (the mode is set when r8 > r9), a
// draw modulo 4 and the six bytes the gatherer reported.
void file_vacancy_event(guest::Cpu& c, std::uint32_t table, std::uint32_t person, std::uint32_t second, std::uint32_t move,
                        std::int16_t half, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t extra);
}

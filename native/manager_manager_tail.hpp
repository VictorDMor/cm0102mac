#pragma once
// MANAGER_MANAGER.CPP tail block, written native-first: the manager-move bookkeeping at the end of the module
// (the application a club weighs, the queue of moves that fall due, the news they file) and the texts of three
// of those news messages.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101c42c0 (r4 = record, r5 = a byte): 0 when the byte is 2 and the record's standing byte (+0x240) is 0x14,
// otherwise 1. Called from another module with r3 = 1 or 0 as the only result read.
std::uint32_t standing_allows(guest::Cpu& c, std::uint32_t record, std::int8_t kind);

// 0x101c1180: files the news message 0x178a about the manager record `person` taking a job: the person's index and
// three record ids, three byte/halfword values and the club's id; nothing happens without the person's club
// (+0x24) or the club's league (+0x2c).
void post_move_news(guest::Cpu& c, std::uint32_t person, std::int8_t first, std::int8_t second, std::int16_t third,
                    std::int8_t fourth);
// 0x101c0fc0 (r3 = the list object): walks the queue of moves in list[5] and, for each node whose date is not after
// today's, hands the person over (up to four effects through 0x100e0410), posts the news (post_move_news) and
// removes the node.
void process_due_moves(guest::Cpu& c, std::uint32_t list);
// 0x101c1830 (r3 = list, r4 = person record): finds in the search 0x1048b180 the person's pending move to another
// club and files the news message 0x178d about it (post_rival_news), then queues the person's index in list[4].
// Returns 1 when it found one, otherwise 0.
std::uint32_t announce_move(guest::Cpu& c, std::uint32_t list, std::uint32_t person);
// 0x101c1a10 (r4 = person, r5 and r6 = the two other participants' records): files the news message 0x178d.
void post_rival_news(guest::Cpu& c, std::uint32_t person, std::uint32_t other_a, std::uint32_t other_b);

// 0x101c0230: a club weighs the application of the manager record `person` (r3 = list, r4 = person, r5 = flag,
// r6..r10 = five bytes, then four words the caller keeps in its frame at +0x38.. : a byte, a halfword, a byte and
// a word). Appends a 0x20-byte decision record to list[5], draws for the club's interest and, where it is
// interested, files the club's call through 0x106a15f0. Returns the outcome byte at +4 of the record (0 refused,
// 1 and 2 accepted).
struct Application {
    std::uint32_t byte_a, byte_b, byte_c, byte_d, byte_e;  // r6..r10 (stored as bytes)
    std::uint32_t stage;                                    // the byte at the caller's sp+0x3b
    std::int16_t number;                                    // the halfword at sp+0x3e
    std::uint32_t kind;                                     // the byte at sp+0x43
    std::int32_t slot;                                      // the word at sp+0x44
};
// The arguments of an entry at the caller's stack pointer (call before the frame is built).
Application read_application(const guest::Cpu& c);
std::uint32_t weigh_application(guest::Cpu& c, std::uint32_t list, std::uint32_t person, std::int8_t flag,
                                const Application& a);

// The texts of the news messages (called from another module through a table): r4 = the message record, r5 = flag,
// r6 = the output object. With the flag set the full body is built, with the flag clear only the headline.
// 0x101c12a0: message 0x178a (post_move_news); 0x101c1c20: message 0x178d (post_rival_news); 0x101c3b10: the
// assessment message 0x178f (manager_manager_assess).
void move_news_text(guest::Cpu& c, std::uint32_t message, std::uint32_t flag, std::uint32_t out);
void rival_news_text(guest::Cpu& c, std::uint32_t message, std::uint32_t flag, std::uint32_t out);
void assessment_news_text(guest::Cpu& c, std::uint32_t message, std::uint32_t flag, std::uint32_t out);
}

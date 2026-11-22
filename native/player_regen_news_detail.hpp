#pragma once
// PLAYER_REGEN.CPP, blocks `news_posting` and `news_texts`: what the news routines share. A news item is built on the posting routine's
// frame (0x102bdf90 starts it, 0x102cb4b0 sets its words); the text builders read the words back (0x102cb570) and format localized
// sentences into the text buffer (0x10727890), which the news item appenders (0x1074b830, 0x1074bab0) copy into the output item. Every
// callee runs through the dispatcher (invoke) at the original's return address, with the original's frame size under it, so the
// arguments, stack words and leftover registers each callee sees are the original's.
#include "player_regen_detail.hpp"

namespace cm::aot::player_regen::news {
namespace entry {
inline constexpr std::uint32_t begin_item = 0x102bdf90, set_word = 0x102cb4b0, word = 0x102cb570, format = 0x10727890,
                               begin_text = 0x1074b830, append_text = 0x1074bab0, begin_line = 0x1074b960, person_name = 0x100cb590,
                               name_is_new = 0x100cbde0, position_name = 0x100c6bf0, job_title = 0x100c6af0;
}
namespace cells {
inline constexpr std::int32_t news_manager = -0x7bf0,      // cell: the news manager the posting routines hand the item to
    contract_result = -0x7b78,                              // cell: the contract manager's last answer (the posting routines store it)
    contract_manager = -0x7b7c, managers = -0x7b40,         // cells: the contract manager and the human managers' object
    clubs = -0x7be8, nations = -0x7b98, people = -0x7be0,   // cells: the club rows (0x248), nation rows (0x128), person records (0x70)
    text_buffer = -0x7bb8, pool = -0x50a0;                  // the sentence buffer (the slot holds the buffer) and the unit's strings
}
// The record fields these routines read.
namespace row {
inline constexpr std::uint32_t club_size = 0x248, nation_size = 0x128, person_size = 0x70;
inline constexpr std::uint32_t club_id = 0x00, club_name = 0x1ee /* the name's text, the short form */, club_gender = 0x242,
                               club_long_name = 0x221 /* a second name text (the staff texts) */, club_long_gender = 0x23b;
inline constexpr std::uint32_t nation_name = 0xae, nation_gender = 0x115;
}
// The buffer the name of the person is built into, and its room.
inline constexpr std::uint32_t name_room = 400;

// `bl 0x10727890` with the arguments of one sentence: the buffer (r3), the format text (r4), then the values in r5...r10 and,
// beyond six, in the stack words the caller stores from sp+0x38. A site with fewer values leaves r6...r10 as the last callee left them.
inline std::uint32_t say(Cpu& c, const Frame& frame, std::uint32_t buffer, std::uint32_t text, std::uint32_t back,
                         std::initializer_list<std::uint32_t> values) {
    unsigned n = 5;
    auto stack = frame.at(0x38);
    for (const auto value : values) {
        if (n <= 10) { c.set(n++, value); continue; }
        c.put32(stack, value);
        stack += 4;
    }
    return invoke(c, entry::format, back, {buffer, text});
}
}

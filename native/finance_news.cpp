// FINANCE.CPP news block (finance_news.hpp): the posters of the board and stadium stories. Each builds a news
// item in its frame (0x102bdf90 starts it with its kind, 0x102cb4b0 sets its numbered words) and hands it to the
// news desk (0x102cc140; 0x102cc400 for the board stories whose club has a second recipient).
#include "finance_news.hpp"
#include "finance_detail.hpp"
#include <cstdio>

namespace cm::aot::finance {
using guest::Frame;

namespace {
namespace news_call {
inline constexpr std::uint32_t start_item = 0x102bdf90, set_word = 0x102cb4b0, post = 0x102cc140, post_to = 0x102cc400;
}
namespace slot {
inline constexpr std::int32_t desk = -0x7bf0, clubs = -0x7be8;
}
namespace kind {
inline constexpr std::uint32_t board_event = 0x1779, stand_closed = 0x177a, financial_crisis = 0x177b, dividend = 0x177c,
                               stadium_works = 0x177d, stadium_link = 0x177f, competition_stadium = 0x1780,
                               receivers_appeased = 0x1783, expansion = 0x1788;
}
inline constexpr std::uint32_t item_at = 0x38;  // the item's place in every poster's frame (0xec bytes)

// The news item of a poster's frame. Every call site is the address of its `bl` in the original (the call
// returns to the next instruction).
struct Item {
    Cpu& c;
    std::uint32_t at;
    void start(std::uint32_t item_kind, std::uint32_t site) const { invoke(c, news_call::start_item, site + 4, {at, item_kind, 0}); }
    void word(unsigned index, std::uint32_t value, std::uint32_t site) const { invoke(c, news_call::set_word, site + 4, {at, index, value}); }
    std::uint32_t desk() const { return c.u32(c.toc(slot::desk)); }
    void post(std::uint32_t subject, std::uint32_t site) const { invoke(c, news_call::post, site + 4, {desk(), at, subject}); }
};

std::uint32_t sign_extend(std::int32_t narrow) { return std::uint32_t(narrow); }

// The sign of an odd draw: -1, 0 or 1 (the sample's remainder by two, signed), from the inline sample-table draw
// of the sites 0x10102878 and 0x10102ad8 (draw() in finance_detail.hpp; `redraw` is the address after its `bl`).
std::int32_t draw_trit(Cpu& c, std::uint32_t redraw) { return std::int32_t(draw(c, redraw)) % 2; }
}

// 0x10101ff0. The club's index, the two amounts and the event (see board_event_text); a PLC or new board
// event of a club that has a second recipient (its word at +0x2c) goes to that recipient.
void post_board_event_news(Cpu& c, std::uint32_t club, std::uint32_t injection, std::uint32_t budget, std::uint32_t event) {
    Frame frame(c, 0x10101ff0, 0x140, {.first_gpr = 27});
    const Item item{c, frame.at(item_at)};
    c.set({{27, club}, {28, injection}, {29, budget}, {30, event}, {31, c.toc(slot::desk)}});
    item.start(kind::board_event, 0x10102020);
    item.word(0, c.u32(club), 0x10102030);
    item.word(1, injection, 0x10102040);
    item.word(2, budget, 0x10102050);
    item.word(3, sign_extend(std::int8_t(event)), 0x10102060);
    const auto what = std::int8_t(event);
    if ((what == 2 || what == 7) && c.u32(club + 0x2c) != 0)
        invoke(c, news_call::post_to, 0x10102090, {item.desk(), item.at, c.u32(club + 0x2c)});
    else
        item.post(club, 0x101020a0);
}

// 0x10102590. The stadium record (the club's word at +0x3c) names the stand; -1 when the club has none.
void post_stand_closed_news(Cpu& c, std::uint32_t club, std::uint32_t seats, std::uint32_t terracing) {
    Frame frame(c, 0x10102590, 0x130, {.first_gpr = 29});
    const Item item{c, frame.at(item_at)};
    c.set({{29, club}, {30, seats}, {31, terracing}});
    item.start(kind::stand_closed, 0x101025c0);
    item.word(0, c.u32(club), 0x101025d0);
    if (const auto stadium = c.u32(club + 0x3c); stadium != 0) item.word(1, c.u32(stadium), 0x101025ec);
    else item.word(1, 0xffffffffu, 0x10102600);
    item.word(2, sign_extend(std::int16_t(seats)), 0x10102610);
    item.word(3, sign_extend(std::int8_t(terracing)), 0x10102620);
    item.post(club, 0x10102634);
}

// 0x10102820. Nothing is posted for no club. The second word is 2, or a draw of -1, 0 or 1.
void post_expansion_news(Cpu& c, std::uint32_t club, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x10102820, 0x140, {.first_gpr = 27});
    const Item item{c, frame.at(item_at)};
    c.set({{27, first}, {28, second}, {29, c.toc(sample_table::base)}, {30, c.toc(sample_table::cursor)}, {31, club}});
    if (club == 0) return;
    item.start(kind::expansion, 0x10102854);
    item.word(1, sign_extend(std::int16_t(first)), 0x10102864);
    const std::int32_t value = std::int16_t(second) == 2 ? 2 : draw_trit(c, 0x101028a4);
    item.word(2, sign_extend(std::int16_t(value)), 0x101028dc);
    item.post(club, 0x101028f0);
}

// 0x10102a70. The second word is a draw of -1, 0 or 1: 0 is the financial crisis, any other the receivers.
void post_financial_crisis_news(Cpu& c, std::uint32_t club) {
    Frame frame(c, 0x10102a70, 0x130, {.first_gpr = 29});
    const Item item{c, frame.at(item_at)};
    c.set({{29, c.toc(sample_table::base)}, {30, c.toc(sample_table::cursor)}, {31, club}});
    item.start(kind::financial_crisis, 0x10102aa0);
    item.word(0, c.u32(club), 0x10102ab0);
    const std::int32_t value = draw_trit(c, 0x10102adc);
    item.word(1, sign_extend(std::int16_t(value)), 0x10102b10);
    item.post(club, 0x10102b24);
}

// 0x10102c90. The dividend total and a signed byte of the club record (+0x241), which the text only tests.
void post_dividend_news(Cpu& c, std::uint32_t club, std::uint32_t amount) {
    Frame frame(c, 0x10102c90, 0x130, {.first_gpr = 30});
    const Item item{c, frame.at(item_at)};
    c.set({{30, club}, {31, amount}});
    item.start(kind::dividend, 0x10102cb8);
    item.word(0, c.u32(club), 0x10102cc8);
    item.word(1, amount, 0x10102cd8);
    item.word(2, sign_extend(c.i8(club + 0x241)), 0x10102cec);
    item.post(club, 0x10102d00);
}

// 0x10102e00. Works of a kind: a count of seats or people (kinds 1 to 3) or a stadium index (kinds 4 to 6).
void post_stadium_works_news(Cpu& c, std::uint32_t club, std::uint32_t quantity, std::uint32_t kind_word) {
    Frame frame(c, 0x10102e00, 0x130, {.first_gpr = 29});
    const Item item{c, frame.at(item_at)};
    c.set({{29, club}, {30, quantity}, {31, kind_word}});
    item.start(kind::stadium_works, 0x10102e30);
    item.word(0, c.u32(club), 0x10102e40);
    item.word(1, quantity, 0x10102e50);
    item.word(2, sign_extend(std::int8_t(kind_word)), 0x10102e60);
    item.post(club, 0x10102e74);
}

// 0x101030c0. Nothing is posted for no club or no record. The second word is read through the club's ground
// row (+0x3c) and that row's word at +0x10. Either can send the original to address 0x10, which the original
// machine could read and this one cannot: a null row, where the test is inverted (0x10103118 `bne` skips the
// check for a non-null row and a null one falls into 0x1010311c `lwz r0,0x10(r3)`), and a row whose +0x10 is
// null (0x10103134 `lwz r5,0x10(r5)`; 138 of the 7,106 rows of the career snapshot). The two callers, in
// 0x100fa6e0, store the row just before the call and the two rows they use have the word, so neither happens
// with the game's own data. There is no item then: what low memory held on the original is not known. Said
// once per run. The generated body is not guarded and still stops there.
void post_stadium_link_news(Cpu& c, std::uint32_t club, std::uint32_t record) {
    Frame frame(c, 0x101030c0, 0x130, {.first_gpr = 30});
    const Item item{c, frame.at(item_at)};
    c.set({{30, club}, {31, record}});
    item.start(kind::stadium_link, 0x101030e8);
    if (club == 0 || record == 0) return;
    item.word(0, c.u32(club), 0x1010310c);
    const auto stadium = c.u32(club + 0x3c);
    const auto named = stadium != 0 ? c.u32(stadium + 0x10) : 0;
    if (named == 0) {
        static bool said = false;
        if (!said) {
            said = true;
            std::fprintf(stderr, "Stadium news for a club without a named ground (club %u, %s): no item\n", unsigned(c.u32(club)),
                         stadium == 0 ? "no ground row" : "the row names none");
        }
        return;
    }
    item.word(1, c.u32(named + 0x10), 0x10103138);
    item.word(2, c.u32(record), 0x10103148);
    item.post(club, 0x1010315c);
}

// 0x10103270. The subject is the club the index names.
void post_receivers_appeased_news(Cpu& c, std::uint32_t club_index) {
    Frame frame(c, 0x10103270, 0x130, {.first_gpr = 31});
    const Item item{c, frame.at(item_at)};
    c.set(31, club_index);
    item.start(kind::receivers_appeased, 0x10103290);
    item.word(0, club_index, 0x101032a0);
    item.post(c.u32(c.toc(slot::clubs)) + club_index * club_size, 0x101032c0);
}

// 0x10103380. A competition index and whether the stadium is redeveloped (1 reads "enlarge" in the text).
void post_competition_stadium_news(Cpu& c, std::uint32_t club_index, std::uint32_t competition, std::uint32_t redevelop) {
    Frame frame(c, 0x10103380, 0x130, {.first_gpr = 29});
    const Item item{c, frame.at(item_at)};
    c.set({{29, club_index}, {30, competition}, {31, redevelop}});
    item.start(kind::competition_stadium, 0x101033b0);
    item.word(0, club_index, 0x101033c0);
    item.word(1, competition, 0x101033d0);
    item.word(2, sign_extend(std::int8_t(redevelop)), 0x101033e0);
    item.post(c.u32(c.toc(slot::clubs)) + club_index * club_size, 0x10103400);
}
}

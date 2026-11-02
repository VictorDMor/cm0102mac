#pragma once
// The press story writer: one story record in, the story text (body, the manager's quote) and its links
// out. The record is read field by field (0x102cb570); the stories are chosen by the story code (field 9),
// the result kind (field 1) and numbers that pick between wordings.
#include "media_detail.hpp"
#include "media_text_mid_detail.hpp"
#include <array>
#include <bit>
#include <initializer_list>
#include <span>
#include <utility>

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;
using namespace detail::text;

// The quote for a story code: the phrase and the return address of its formatter call. Some codes have a
// second wording for when a field of the record is zero (`test`), and some name the club's nation.
enum class Test : std::uint8_t { none, field5, field15 };
struct QuoteEntry {
    std::uint32_t code, phrase, back;
    Test test = Test::none;
    std::uint32_t zero_phrase = 0, zero_back = 0;
    bool nation = false;
};
// The headline for a story code: the words are the club (its marker twice and its short name), plus the
// club's row for the first, or only the staff member's short name.
enum class Words : std::uint8_t { club, club_and_row, subject };
struct HeadlineEntry { std::uint32_t code, phrase, back; Words words = Words::club; };

struct Story {
    Story(Cpu& cpu, const Frame& f, std::uint32_t record_, std::uint32_t full_, std::uint32_t output_);
    Cpu& c;
    const Frame& frame;
    std::uint32_t record, full, output;
    std::uint32_t pool, clubs_slot;  // the source pool, the slot of the club table

    // The record's fields, by their index. kind: how the story went for the club (0 and 1 are the two
    // named results); club_id: the club it is about; code: which story to write; the people are a staff
    // member (staff_*) and another one (other_*), each with the three parts of the name.
    std::uint32_t kind = 0, outlet_kind = 0, field5 = 0, field6 = 0, club_id = 0, competition_id = 0, code = 0, mood = 0;
    std::uint32_t other_staff = 0, other_first = 0, other_middle = 0, other_last = 0;
    std::uint32_t staff_id = 0, staff_first = 0, staff_middle = 0, staff_last = 0;
    std::uint32_t has_match = 0, own_club = 0;
    // The numbers that follow the match result (or field 0x14 on, when the record holds none), read with a
    // counter: field14 lives in r14, field1b in r13 (a signed byte), field1c in r17. The stories use them as
    // people (field16, field17: persons; field19: a club; field1a: a player's slot), an amount (field18) and
    // counts or flags; the names say where in the record they are, not what they mean.
    std::uint32_t field15 = 0, field16 = 0, field17 = 0, field18 = 0, field19 = 0, field1a = 0, field1d = 0;

    // Places in the frame.
    std::uint32_t at(std::uint32_t offset) const { return frame.at(std::int32_t(offset)); }
    std::uint32_t body = frame.at(0x1b3c), quote = frame.at(0x3cc), joined = frame.at(0x5c0), outlet_text = frame.at(0xf0),
                  score_text = frame.at(0x8c), nationality = frame.at(0x124), subject_full = frame.at(0x28c),
                  subject_short = frame.at(0x214), other_full = frame.at(0x19c), stage_name = frame.at(0x304),
                  amount_text = frame.at(0x98);

    // Memory and arithmetic, as 32-bit register values.
    std::uint32_t u8(std::uint32_t a) const { return c.u8(a); }
    std::uint32_t u16(std::uint32_t a) const { return c.u16(a); }
    std::uint32_t u32(std::uint32_t a) const { return c.u32(a); }
    std::uint32_t i16(std::uint32_t a) const { return extended(c.i16(a)); }
    std::uint32_t toc(std::int32_t offset) const { return c.toc(offset); }
    static std::uint32_t sb(std::uint32_t v) { return extended(std::int16_t(std::int8_t(v))); }
    static std::uint32_t sh(std::uint32_t v) { return extended(std::int16_t(v)); }
    static std::uint32_t clz(std::uint32_t v) { return std::uint32_t(std::countl_zero(v)); }
    static std::uint32_t rem2(std::uint32_t v) { return std::uint32_t(std::int32_t(v) % 2); }
    static std::uint32_t rem3(std::uint32_t v) { return std::uint32_t(std::int32_t(v) % 3); }
    static std::uint32_t rem4(std::uint32_t v) { return std::uint32_t(std::int32_t(v) % 4); }
    int cmp(std::uint32_t a, std::uint32_t b) const { return compare(c, std::int32_t(a), std::int32_t(b)); }
    int cmpl(std::uint32_t a, std::uint32_t b) const { return compare_logical(c, a, b); }
    void put8(std::uint32_t a, std::uint32_t v) const { c.put8(a, v); }
    void put32(std::uint32_t a, std::uint32_t v) const { c.put32(a, v); }
    // The formatter's arguments past the sixth are the frame's outgoing words, from sp+0x38: word 0 is the seventh argument.
    void outgoing(unsigned word, std::uint32_t v) const { c.put32(c.sp() + 0x38 + 4 * word, v); }
    std::uint32_t phrase(std::uint32_t offset) const { return pool + offset; }

    // The world's tables and the rows this story is about.
    std::uint32_t clubs() const { return c.u32(clubs_slot); }
    std::uint32_t competitions() const { return c.u32(c.toc(slot::competitions)); }
    std::uint32_t persons() const { return c.u32(c.toc(slot::persons)); }
    std::uint32_t home() const { return clubs() + club_id * club_size; }
    std::uint32_t rival_id() const { return c.u32(at(0xe4)); }
    std::uint32_t rival() const { return clubs() + rival_id() * club_size; }
    std::uint32_t club_row(std::uint32_t id) const { return clubs() + id * club_size; }
    std::uint32_t competition() const { return competitions() + competition_id * competition_size; }
    std::uint32_t staff_row() const { return table(c, staff_slot) + staff_id * staff_size; }
    std::uint32_t other_row() const { return table(c, staff_slot) + other_staff * staff_size; }
    std::uint32_t nation() const { return c.u32(home() + 0x2c); }
    // The two clubs' names with the byte next to each, the competition's, the nation's, and the match's goals.
    std::uint32_t club_marker(std::uint32_t row) const { return sb(c.u8(row + 0x23b)); }
    std::uint32_t home_marker() const { return club_marker(home()); }
    std::uint32_t rival_marker() const { return club_marker(rival()); }
    std::uint32_t rival_long_marker() const { return sb(c.u8(rival() + 0x242)); }
    std::uint32_t home_short() const { return home() + 0x221; }
    std::uint32_t home_long() const { return home() + 0x1ee; }
    std::uint32_t rival_short() const { return rival() + 0x221; }
    std::uint32_t rival_long() const { return rival() + 0x1ee; }
    std::uint32_t competition_name() const { return competition() + 0x4d; }
    std::uint32_t competition_marker() const { return sb(c.u8(competition() + 0x67)); }
    std::uint32_t competition_long_name() const { return competition() + 0x1a; }
    std::uint32_t competition_long_marker() const { return sb(c.u8(competition() + 0x6a)); }
    std::uint32_t nation_marker() const { return sb(c.u8(nation() + 0x116)); }
    std::uint32_t home_goals() const { return sb(c.u8(at(0xe8))); }
    std::uint32_t away_goals() const { return sb(c.u8(at(0xe9))); }
    std::uint32_t outlet_flag() const { return c.u8(at(0x87)); }

    // Writing: a phrase through the formatter; any other routine with its register arguments; the score
    // as text.
    void say(std::uint32_t destination, std::uint32_t text, std::uint32_t back, std::initializer_list<std::uint32_t> args) const;
    void call(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers) const;
    void score(std::uint32_t back) const;
    // The closing line of a level match: one of four by the result in r17, appended to the body.
    void level_ending(std::uint32_t first_back) const;

    // The parts of the writer, in order.
    void read_record();
    void choose_outlet();
    void outlet_name();
    void run();
    void headline();
    bool say_quote(std::span<const QuoteEntry> table, bool with_other);
    bool say_headline(std::span<const HeadlineEntry> table);
    void quote_text();
    void links();
    void stories();
    void story_50(); void story_51(); void story_3f(); void story_5f(); void story_60(); void story_42(); void story_43(); void story_44();
    void story_5a(); void story_5c(); void story_5b(); void story_52(); void story_3e(); void story_5e(); void story_3c(); void story_3d();
    void story_63(); void story_61(); void story_5d(); void story_62(); void story_40(); void story_41();
};
}

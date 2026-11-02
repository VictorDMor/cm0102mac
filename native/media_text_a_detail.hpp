#pragma once
// What 0x1064cf80 (a press story about a player, with the manager's answer) is made of: the story
// item's values, the phrases with their arguments as data (`Line`), and the context (`Story`) whose
// methods choose and write them. A story is a column of text about a player (or his manager), then,
// in the long form, the manager's reply; both end with the links that make the names clickable.
#include "media_text_shared.hpp"
#include "media_text_small_detail.hpp"
#include <array>
#include <bit>
#include <span>

namespace cm::aot::media::detail::column {
using namespace guest;
using namespace detail;
using namespace detail::small;

namespace entry {
constexpr std::uint32_t decode_match = 0x1030dee0, today = 0x10715880, adjective = 0x10168a00, sprintf = 0x1077b720,
    age_text = 0x100db410, match_text = 0x100d6300, position_text = 0x100d6af0,
            nation_text = 0x100c76c0,
    fixture = 0x1029eb20, set_text = 0x1074b830;
}
constexpr std::uint32_t club_article_offset = small::club_article, club_name_offset = small::club_name,
    club_short_name_offset = small::club_short_name, club_short_article_offset = small::club_short_article;
constexpr int teams_slot = -0x7538, user_slot = -0x7bb0, calendar_slot = -0x7b4c;

// What a phrase can name. Each is read when the phrase is written, from the story and the world.
enum class Arg : std::uint8_t {
    plain,  // 0x64: the mark the phrases' empty placeholders take
    player_name, player_short, manager_name, manager_short, second_name, third_name,
    adjective_text, outlet_name, score_text, position_text, date_text,
    club_article, club_name, club_short, clubs_base, club_short_article,
    nation, nation_article, nation_name,
    rival_short_article, rival_short, other_article, other_name,
    person_article, person_text,
    team_article, team_name, teams_base,
    position_kind, outlet_article, age_flag_arg,
};
enum class Dest : std::uint16_t { story_text = 0x1e3c, reply_text = 0x6cc, outlet_name = 0x94 };
enum class Of : std::uint8_t { nation, person_record };

// One phrase of the pool with the address after the call that writes it, up to sixteen arguments, where it
// goes and what goes with it: a nationality adjective written first.
struct Line {
    std::uint32_t phrase, back;
    std::array<Arg, 16> args;
    std::uint8_t count;
    Dest dest = Dest::story_text;
    bool sprintf_form = false;
    std::uint8_t adjective_kind = 0;
    Of adjective_of = Of::nation;
    std::uint32_t adjective_back = 0;
    constexpr Line to(Dest d) const { Line l = *this; l.dest = d; return l; }
    constexpr Line plain() const { Line l = *this; l.sprintf_form = true; return l; }
    constexpr Line adjective(std::uint8_t kind, std::uint32_t at, Of of) const {
        Line l = *this; l.adjective_kind = kind; l.adjective_back = at; l.adjective_of = of; return l;
    }
};
template <class... A>
constexpr Line L(std::uint32_t phrase, std::uint32_t back, A... args) {
    static_assert(sizeof...(A) <= 16);
    Line line{phrase, back, {}, std::uint8_t(sizeof...(A))};
    unsigned i = 0;
    ((line.args[i++] = Arg(args)), ...);
    return line;
}
// The same for a line of the manager's reply.
template <class... A>
constexpr Line R(std::uint32_t phrase, std::uint32_t back,
        A... args) { return L(phrase, back, args...).to(Dest::reply_text); }
inline constexpr Arg plain = Arg::plain, player_name = Arg::player_name, player_short = Arg::player_short,
    manager_name = Arg::manager_name, manager_short = Arg::manager_short, second_name = Arg::second_name,
    third_name = Arg::third_name, adjective_text = Arg::adjective_text, outlet_name = Arg::outlet_name,
    score_text = Arg::score_text, position_text = Arg::position_text, date_text = Arg::date_text,
    club_article = Arg::club_article, club_name = Arg::club_name, club_short = Arg::club_short,
            clubs_base = Arg::clubs_base,
    club_short_article = Arg::club_short_article, nation = Arg::nation, nation_article = Arg::nation_article,
    nation_name = Arg::nation_name, rival_short_article = Arg::rival_short_article, rival_short = Arg::rival_short,
    other_article = Arg::other_article, other_name = Arg::other_name, person_article = Arg::person_article,
    person_text = Arg::person_text, team_article = Arg::team_article,
    team_name = Arg::team_name, teams_base = Arg::teams_base, position_kind = Arg::position_kind,
    outlet_article = Arg::outlet_article, age_flag_arg = Arg::age_flag_arg;
inline constexpr Dest story_text = Dest::story_text, reply_text = Dest::reply_text;
inline constexpr Of of_nation = Of::nation, of_person = Of::person_record;

// The story being written. The item's values live in the registers the original keeps them in while it
// calls the formatter (each callee saves them into the stack image), so an assignment sets the register too.
struct Story {
    Story(Cpu& cpu, const Frame& f, std::uint32_t item_, std::uint32_t output_)
        : c(cpu), frame(f), item(item_), output(output_) {}
    Cpu& c;
    const Frame& frame;
    std::uint32_t item, output;
    // The item's values: the player (a person row), his club, the story's kind, tone and numbers.
    std::int32_t player = 0, club = 0, tone = 0, medium = 0, person = 0, kind = 0, answer = 0, level = 0, traits = 0,
                 manager = 0, rival = 0, result = 0, third = 0, figure = 0, second = 0;
    std::int32_t domestic = 0;  // the story is told in the club's own country
    std::uint32_t status = 0;   // the player's position text, or 0 for a player-manager

    std::uint32_t at(unsigned offset) const { return frame.at(std::int32_t(offset)); }
    int cmp(std::int32_t a, std::int32_t b) const { return compare(c, a, b); }
    int cmpl(std::uint32_t a, std::uint32_t b) const { return compare_logical(c, a, b); }
    std::int32_t byte_at(unsigned offset) const { return std::int8_t(c.u8(at(offset))); }
    std::int32_t word_at(unsigned offset) const { return std::int32_t(c.u32(at(offset))); }
    // The stack bytes and words the stories test.
    std::int32_t article() const { return byte_at(0x61); }
    std::int32_t goals_home() const { return byte_at(0x8c); }
    std::int32_t goals_away() const { return byte_at(0x8d); }
    std::int32_t has_match() const { return byte_at(0x31e0); }
    std::int32_t byte_31d1() const { return byte_at(0x31d1); }
    std::int32_t byte_31d0() const { return byte_at(0x31d0); }
    std::int32_t age_flag() const { return byte_at(0x3218); }
    std::int32_t variant() const { return word_at(0x3204); }
    std::int32_t second_person() const { return word_at(0x3200); }
    std::int32_t word_31f0() const { return word_at(0x31f0); }
    std::int32_t word_31d4() const { return word_at(0x31d4); }
    std::int32_t half_31d8() const { return std::int16_t(c.u16(at(0x31d8))); }
    std::uint32_t rival_club() const { return c.u32(at(0x88)); }
    std::uint32_t team() const { return c.u32(at(0x3208)); }

    std::uint32_t clubs() const { return c.u32(c.toc(slot::clubs)); }
    std::uint32_t club_of(std::int32_t index) const { return clubs() + std::uint32_t(index) * club_size; }
    std::uint32_t person_at(std::int32_t index) const {
        return c.u32(c.toc(slot::persons)) + std::uint32_t(index) * 0x128;
    }
    std::uint32_t team_at() const { return c.u32(c.toc(teams_slot)) + team() * 0x38; }
    std::uint32_t value(Arg arg) const;
    // Writes the line (after its adjective).
    void say(const Line& line);
    void clobber_register(unsigned n) { c.set(n, clubs()); }
    void reseed_tone();
    std::int32_t pick_three() const;
    void score(std::uint32_t back);
    void position(std::uint32_t back);
    // A link from the finished text to a record (0x1074bab0); `clubs_in_r7` is the stray value the
    // original leaves in r7 at the club links.
    void link(std::uint32_t text, std::uint32_t kind, std::uint32_t record, std::uint32_t back,
            bool clubs_in_r7 = false);

    // The long form: the outlet, the column, the manager's reply and the links.
    void long_form();
    void short_form();
    void outlet();
    void reply();
    void level_ending(std::uint32_t first_back);
    void finish_long();
    // The story column, one function per way of choosing its wording (media_text_a_story.cpp).
    void write_column_by_kind();
    void write_column_by_tone_then_traits();
    void append_ending_if_level();
    void headline();  // the short form's one line (media_text_a_headline.cpp)

    // The manager's reply (media_text_a_reply.cpp): who answers, then the kind of story or the player's traits.
    void user_told();
    void user_admitted();
    void other_told();
    void other_said();
    bool reply_goes_by_traits();
    void user_told_by_kind();
    void user_told_by_traits();
    void user_admitted_by_kind();
    void user_admitted_by_traits();
    void other_told_by_kind();
    void other_told_by_traits();
    void other_said_by_kind();
    void other_said_by_traits();
    // A line out of a set, chosen by the answer given, the kind of story, or the level and tone.
    void say_by_answer_of_eleven(std::span<const Line> lines);
    void say_by_answer_of_ten(std::span<const Line> lines);
    void say_by_kind_of_eight(std::span<const Line> lines);
    void say_by_level_and_tone(std::span<const Line> lines);

    // The player's traits as the original tests them. The signed form compares `traits & mask` with the mask;
    // the unsigned form compares `(traits & mask) - base` with `mask - base`, so the condition register comes
    // out as the original leaves it.
    bool has_all_traits(std::int32_t mask) const { return cmp(traits & mask, mask) == 0; }
    bool has_all_traits_unsigned(std::uint32_t mask, std::uint32_t base) const {
        return cmpl((std::uint32_t(traits) & mask) - base, mask - base) == 0;
    }
    bool has_any_trait(std::int32_t mask) const { return cmp(traits & mask, 0) != 0; }
    // The traits that give the column its own wording, and the reply with it.
    bool has_column_trait() const {
        return has_all_traits_unsigned(0x400200, 0x400000) || has_all_traits_unsigned(0x102200, 0x100000) ||
               has_all_traits_unsigned(0x104200, 0x100000) || has_all_traits_unsigned(0x100200, 0x100000) ||
               has_all_traits(0x480) || has_all_traits(0x500) || has_all_traits_unsigned(0x20820, 0x20000) ||
               has_all_traits(0x1020) || has_all_traits_unsigned(0x20810, 0x20000) ||
               has_all_traits_unsigned(0x10001, 0x10000) || has_any_trait(0x1);
    }
    // A second tier of traits (the third also needs the story to be told at home).
    bool has_secondary_trait() const {
        return has_all_traits(0xa) || has_all_traits(0x8) || (has_all_traits(0x2) && cmp(domestic, 0) != 0);
    }
};
}

#pragma once
// The press story writer: one story record in, the story text (body, the manager's quote) and its links
// out. The record is read field by field (0x102cb570); the stories are chosen by the story code (field 9),
// the result kind (field 1) and numbers that pick between wordings.
#include "media_detail.hpp"
#include "media_records.hpp"
#include "media_text_c_detail.hpp"
#include "media_text_mid_detail.hpp"
#include <array>
#include <bit>
#include <initializer_list>
#include <optional>
#include <span>
#include <utility>

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;
using namespace detail::text;
using report::Var;
using report::NationView;
using report::CompetitionView;

// A nation record is 0x128 bytes (person_size in media_detail.hpp is the same size: it is the nation table's).
constexpr std::uint32_t nation_size = person_size;
// The marker byte the original passes for text that has none of its own, such as the nationality adjective
// (the phrases' {} take a marker twice).
constexpr std::uint32_t unmarked = 0x64;
// Routines the writer calls (0x1077b720 is sprintf; the others are named in the stories where they are used).
constexpr std::uint32_t sprintf_entry = 0x1077b720, copy_text = 0x1077dbf0, append_entry = 0x1077dc60, compare_entry = 0x1077dc90,
    club_flag = 0x100c7d00, stage_label = 0x104769d0, stage_ordinal = 0x10476ec0;
// The kind of the link that makes a match score clickable (the others are in link_kind).
constexpr std::uint32_t link_score = 0x7d3;

// The quote for a story code: the phrase and the return address of its formatter call. Some codes have a
// second wording for when a field of the record is zero (`test`), and some name the club's nation.
enum class Test : std::uint8_t { none, field5, title_holder };
struct QuoteEntry {
    std::uint32_t code, phrase, back;
    Test test = Test::none;
    std::uint32_t zero_phrase = 0, zero_back = 0;
    bool nation = false;
};
// The headline for a story code: the words are the club (its marker twice and its short name), plus the
// club's row for the first, or only the subject's short name.
enum class Words : std::uint8_t { club, club_and_row, subject };
struct HeadlineEntry { std::uint32_t code, phrase, back; Words words = Words::club; };

// A phrase of the pool and the return address of the formatter call that writes it.
struct Wording { std::uint32_t phrase, back; };
// A wording, or two: the second for a club that holds the title (the record's `title_holder` is set).
struct Pick {
    Wording fresh;
    std::optional<Wording> holding;
    Pick(Wording w) : fresh(w) {}
    Pick(std::uint32_t phrase, std::uint32_t back) : fresh{phrase, back} {}
    Pick(Wording w, Wording h) : fresh(w), holding(h) {}
};
// A competition has two names (+0x4d and the longer +0x1a), each with its marker byte.
enum class Form : std::uint8_t { plain, long_form };
// The call that spells the subject's job (0x100c6bf0): its return address and the frame byte (an offset) where it
// leaves the job's article marker.
struct Job { std::uint32_t back, marker; };
// The call that spells the club's nationality adjective into the frame: its return address and which of the
// adjective's forms (0x1b, 0x1c, ... : the form the phrase wants) it asks for.
struct Adjective { std::uint32_t back, form; };
// How a nationality adjective and the club's and competition's words are laid out in a phrase (the compiler
// arranged the formatter's arguments differently for each phrase).
enum class Order : std::uint8_t { club_competition_nation, club_nation_competition, nation_club_competition };

// The wordings of one tournament-tip leaf, for a club of the host nation / of one other host / of two other
// hosts: what the media say, what a former player says, and what the manager of a club says (with his job).
struct TipLeaf {
    Wording generic, former, claim;
    Form generic_form, former_form, claim_form;
    Job job;
};
struct HostTips { TipLeaf own, one, two; };

struct Story {
    Story(Cpu& cpu, const Frame& f, std::uint32_t record_, std::uint32_t full_, std::uint32_t output_);
    Cpu& c;
    const Frame& frame;
    std::uint32_t record, full, output;
    std::uint32_t pool, clubs_slot;  // the source pool, the slot of the club table

    // The record's fields, by their index. kind: how the story went for the club (0 and 1 are the two
    // named results); club_id: the club it is about; code: which story to write. The manager is the club's
    // manager (manager_*); the subject is the person the story names (subject_*): a former player or a
    // manager who is quoted, or a player or manager the story is about.
    std::uint32_t kind = 0, outlet_kind = 0, club_id = 0, competition_id = 0, code = 0, mood = 0;
    // Selectors of the manager's quote (field 5 and 6): the quote is bolder when field 5 is set. What the
    // record means by them is not established.
    std::uint32_t field5 = 0, field6 = 0;
    std::uint32_t manager_id = 0, manager_first = 0, manager_middle = 0, manager_last = 0;
    std::uint32_t subject_id = 0, subject_first = 0, subject_middle = 0, subject_last = 0;
    std::uint32_t has_match = 0;
    std::uint32_t users_club = 0;  // whether the manager of the club is the user's own manager
    // The numbers that follow the match result (or field 0x14 on, when the record holds none), read with a
    // counter. What the stories do with them: title_holder (field 0x15) is set when the club holds the title
    // the story is about (the wordings speak of keeping and of currently holding it); host_one and host_two (0x16, 0x17) are
    // the nations that host a tournament (-1: none, so one host only); spent (0x18) is the sum a club spent
    // on players; staff_club and staff_job (0x19, 0x1a) are the club and job of the subject, when he is quoted
    // as a club's manager or coach (-1: unknown); outlet_region (0x1d) is the region of the outlet that
    // runs the story (as text_c's `outlet_region`, -1: none).
    std::uint32_t title_holder = 0, host_one = 0, host_two = 0, spent = 0, staff_club = 0, staff_job = 0, outlet_region = 0;
    // Record words the original keeps in non-volatile registers (the routines it calls save them into their
    // frames). Var reads and writes the register without writing it when made.
    Var<14> run_length{c};      // field 0x14: games in the unbeaten / winless run the story reports
    Var<13> former_player{c};   // field 0x1b (a signed byte): the subject is a former player
    Var<17> outcome{c};         // field 0x1c: how the match ended for text_c's endings (1-4: won, lost, won, lost)
    Var<16> domestic{c};        // the record pointer until choose_outlet makes it 1 for a manager from the club's country

    // Places in the frame.
    std::uint32_t at(std::uint32_t offset) const { return frame.at(std::int32_t(offset)); }
    std::uint32_t body = frame.at(0x1b3c), quote = frame.at(0x3cc), joined = frame.at(0x5c0), outlet_text = frame.at(0xf0),
                  score_text = frame.at(0x8c), nationality = frame.at(0x124), subject_full = frame.at(0x28c),
                  subject_short = frame.at(0x214), manager_full = frame.at(0x19c), stage_name = frame.at(0x304),
                  amount_text = frame.at(0x98);

    // Memory and arithmetic, as 32-bit register values. Sums and products of record words wrap in 32 bits
    // (uint32 arithmetic) and the remainders are signed, as the original computes them.
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
    // The compiler used the record's registers as temporaries for the last arguments of a long phrase. The
    // formatter's prologue saves them, so what they hold at the call is part of the stack image: set the
    // register and give back the value.
    template <unsigned R>
    std::uint32_t scratch(std::uint32_t value) const {
        c.set(R, value);
        return value;
    }

    // The world's tables and the rows this story is about.
    std::uint32_t clubs() const { return c.u32(clubs_slot); }
    std::uint32_t competitions() const { return c.u32(c.toc(slot::competitions)); }
    std::uint32_t nation_table() const { return c.u32(c.toc(slot::persons)); }  // (the slot is named for the 0x128-byte rows)
    std::uint32_t persons() const { return table(c, staff_slot); }
    std::uint32_t club_row(std::uint32_t id) const { return clubs() + id * club_size; }
    std::uint32_t home() const { return club_row(club_id); }
    std::uint32_t rival_id() const { return c.u32(at(0xe4)); }
    std::uint32_t rival() const { return club_row(rival_id()); }
    std::uint32_t competition() const { return competitions() + competition_id * competition_size; }
    std::uint32_t subject_row() const { return persons() + subject_id * staff_size; }
    std::uint32_t manager_row() const { return persons() + manager_id * staff_size; }
    std::uint32_t nation() const { return records::Club{c, home()}.nation(); }

    // Views of the rows: names with the byte next to each, as the phrases' {} markers read them.
    Club club_view(std::uint32_t row) const { return {c, row}; }
    Club home_club() const { return {c, home()}; }
    Club rival_club() const { return {c, rival()}; }
    Club staff_club_view() const { return {c, club_row(staff_club)}; }
    NationView nation_view(std::uint32_t row) const { return {c, row}; }
    NationView home_nation() const { return {c, nation()}; }
    NationView host_nation(std::uint32_t id) const { return {c, nation_table() + id * nation_size}; }
    CompetitionView competition_view() const { return {c, competition()}; }
    std::uint32_t nation_id() const { return records::Nation{c, nation()}.id().get(); }

    std::uint32_t home_marker() const { return home_club().short_marker(); }
    std::uint32_t rival_marker() const { return rival_club().short_marker(); }
    std::uint32_t rival_long_marker() const { return rival_club().long_marker(); }
    std::uint32_t home_short() const { return home_club().short_name(); }
    std::uint32_t home_long() const { return home_club().long_name(); }
    std::uint32_t rival_short() const { return rival_club().short_name(); }
    std::uint32_t rival_long() const { return rival_club().long_name(); }
    // text_c_detail names the competition's +0x4d name `name` and its +0x1a name `short_name`; the +0x1a
    // field is the longer one (0x33 bytes against 0x1a), so here it is the long form.
    std::uint32_t competition_name(Form f = Form::plain) const {
        return f == Form::plain ? competition_view().name() : competition_view().short_name();
    }
    std::uint32_t competition_marker(Form f = Form::plain) const {
        return f == Form::plain ? competition_view().marker() : competition_view().short_marker();
    }
    std::uint32_t competition_long_name() const { return competition_name(Form::long_form); }
    std::uint32_t competition_long_marker() const { return competition_marker(Form::long_form); }
    std::uint32_t nation_marker() const { return home_nation().marker(); }
    std::uint32_t nation_name() const { return home_nation().name(); }
    // The nation's continent record (+0x74): its text at +0x10 and its marker byte at +0xc3.
    std::uint32_t continent() const { return u32(nation() + 0x74); }
    std::uint32_t continent_name() const { return continent() + 0x10; }
    std::uint32_t continent_marker() const { return sb(u8(continent() + 0xc3)); }
    // The two goals of the match, signed bytes of the decoded result, and what the stories ask of them.
    std::int32_t home_goals() const { return std::int32_t(sb(c.u8(at(0xe8)))); }
    std::int32_t away_goals() const { return std::int32_t(sb(c.u8(at(0xe9)))); }
    std::int32_t goal_margin() const { return home_goals() - away_goals(); }
    std::int32_t goal_total() const { return home_goals() + away_goals(); }
    std::uint32_t outlet_flag() const { return c.u8(at(0x87)); }

    // The user's record (rows of 0x2c0 bytes at the selected slot) and the manager it holds.
    std::uint32_t user_offset() const;
    std::uint32_t users_manager() const;

    // The conditions that recur. Each makes the compares the original makes, in its order.
    bool at_home() const { return cmp(sb(domestic), 0) != 0; }
    bool subject_has_club_and_job() const { return cmp(subject_id, -1) > 0 && cmp(staff_club, -1) > 0 && cmp(staff_job, -1) > 0; }
    bool subject_is_former_player() const { return cmp(subject_id, -1) > 0 && cmp(sb(former_player), 0) != 0; }
    bool club_in_host_nation() const { return cmp(host_one, nation_id()) == 0 || cmp(host_two, nation_id()) == 0; }

    // Writing: a phrase through the formatter; any other routine with its register arguments; the score
    // as text.
    void say(std::uint32_t destination, std::uint32_t text, std::uint32_t back, std::initializer_list<std::uint32_t> args) const;
    void call(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers) const;
    void score(std::uint32_t back) const;
    // The closing line of a level match: one of four by the outcome, appended to the body.
    void level_ending(std::uint32_t first_back) const;
    // A wording into the body.
    void tell(Wording w, std::initializer_list<std::uint32_t> args) const { say(body, phrase(w.phrase), w.back, args); }
    // The subject's job; returns the routine's result (r3, the job's text) and leaves the marker in the frame.
    std::uint32_t job_of_subject(Job job) const;
    std::uint32_t job_marker(Job job) const { return sb(u8(at(job.marker))); }
    // Which of three wordings, by the margin of the match: a heavy one (over 3 goals), a low-scoring one (5 goals
    // or fewer in all), or the rest.
    Wording by_margin(const std::array<Wording, 3>& wordings) const;
    Wording pick(const Pick& p) const;
    // No match to report: the shared text buffer is emptied.
    void no_match() const { put8(toc(slot::text), 0); }
    // The club's nationality adjective, spelled into `nationality`.
    void adjective_of_nation(Adjective adjective) const;

    // The leaves the stories share.
    // The media on the club and the competition.
    void media_on_competition(const Pick& w, Form f) const;
    // A former player's words on the club and the competition.
    void former_player_on_competition(const Pick& w, Form f) const;
    // A club's manager or coach: his club, job and name first.
    void staff_on_competition(Job job, const Pick& w, Form f) const;
    // The same without the competition.
    void staff_remark(Job job, Wording w) const;
    // The media of the club's country, named by their adjective.
    void foreign_media_on_competition(Adjective adjective, const Pick& w, Order order, Form f) const;
    // The media in the club's country, by the nation's name.
    void media_in_nation_on_competition(const Pick& w, Form f) const;
    void media_in_nation(const Pick& w) const;
    // The club alone, for the media or a pundit.
    void media_on_club(const Pick& w) const;
    // The media on a club whose nation is not a host: with one other host, or with two.
    void one_host_media(Wording w, Form f) const;
    void two_hosts_media(Wording w, Form f) const;

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

    // Tournament tips: the media on the host nations (the stories for a tournament, a trophy and its hosts).
    void host_tips(const HostTips& tips) const;
    void own_host_tip(const TipLeaf& leaf) const;
    void one_host_tip(const TipLeaf& leaf) const;
    void two_hosts_tip(const TipLeaf& leaf) const;

    // The stories, by what they write (the story code in the comment at each).
    void unbeaten_run_ends();         // 0x50
    void winless_run_ends();          // 0x51
    void relegation_battle();         // 0x3f
    void promotion_tipped();          // 0x5f
    void relegation_escape_tipped();  // 0x60
    void title_written_off();         // 0x42
    void midtable_written_off();      // 0x43
    void tournament_written_off();    // 0x44
    void tournament_favourites();     // 0x5a
    void cup_favourites();            // 0x5c
    void big_spenders();              // 0x5b
    void starved_striker();           // 0x52
    void relegation_mire();           // 0x3e
    void winning_form_praised();      // 0x5e
    void fans_losing_patience();      // 0x3c
    void fans_protest_match();        // 0x3d
    void title_challenge_asked();     // 0x63
    void title_challenge_doubted();   // 0x61
    void title_favourites();          // 0x5d
    void title_run_unstoppable();     // 0x62
    void promotion_bid_stalling();    // 0x40
    void title_pressure();            // 0x41
};
}

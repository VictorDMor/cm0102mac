#pragma once
// What the press story shares (media.cpp 0x1067fed0): the record's variables, held where the original
// holds them (a story variable lives in its non-volatile register, because the routines it calls save
// that register into the stack image), the tables of clubs, nations, people and competitions, and the
// pieces every wording is made from.
#include "media_detail.hpp"
#include "media_text_mid_detail.hpp"
#include <algorithm>
#include <array>
#include <bit>

namespace cm::aot::media::detail::report {
using namespace guest;
using namespace detail;
using namespace detail::text;

// The routines the story calls.
constexpr std::uint32_t sprintf_entry = 0x1077b720, copy_entry = 0x1077dbf0, append_entry = 0x1077dc60, compare_entry = 0x1077dc90,
    amount_phrase = 0x100d6af0, amount_text_entry = 0x100d6300, job_title = 0x100c6bf0, competition_title = 0x104769d0,
    competition_article_of = 0x10476ec0, date_of = 0x107156b0, date_text = 0x10717270, nationality_of = 0x10168a00,
    match_known = 0x1029eb20, nation_team = 0x100c76c0, split_name = 0x10739890, version_text = 0x10768810, show_error = 0x10769b60;

// The high word of the signed product (mulhw): the compiler's division by a constant leaves it in a register.
inline std::uint32_t mulhw(std::uint32_t a, std::uint32_t b) {
    return std::uint32_t(std::uint64_t(std::int64_t(std::int32_t(a)) * std::int32_t(b)) >> 32);
}

// A story variable the original keeps in register R: reading and writing go to the register, which the
// callees' prologues save. (Unlike Kept, it does not write R when it is made.)
template <unsigned R>
struct Var {
    Cpu& c;
    operator std::uint32_t() const { return c.r(R); }
    Var& operator=(std::uint32_t value) { c.set(R, value); return *this; }
};

// A formatter argument, or the same word twice (the phrases' {} markers take the byte and its twin).
struct Words {
    std::array<std::uint32_t, 2> v{};
    unsigned n = 1;
    template <class T> Words(const T& word) : v{std::uint32_t(word), 0} {}
};
inline Words twice(std::uint32_t word) {
    Words w(word);
    w.v[1] = word;
    w.n = 2;
    return w;
}

// Records of the world's tables, as the story reads them.
struct NationView {
    Cpu& c;
    std::uint32_t at;
    std::uint32_t name() const { return at + 0xe1; }
    std::uint32_t short_name() const { return at + 0xae; }
    std::uint32_t marker() const { return signed_byte(c.u8(at + 0x116)); }
    std::uint32_t short_marker() const { return signed_byte(c.u8(at + 0x115)); }
    std::uint32_t id() const { return c.u32(at + 0x68); }
    operator std::uint32_t() const { return at; }
};
struct ClubView {
    Cpu& c;
    std::uint32_t at;
    std::uint32_t short_name() const { return at + 0x221; }
    std::uint32_t long_name() const { return at + 0x1ee; }
    std::uint32_t short_marker() const { return signed_byte(c.u8(at + 0x23b)); }
    std::uint32_t long_marker() const { return signed_byte(c.u8(at + 0x242)); }
    NationView nation() const { return {c, c.u32(at + 0x2c)}; }
    operator std::uint32_t() const { return at; }
};
struct PersonView {
    Cpu& c;
    std::uint32_t at;
    NationView nation() const { return {c, c.u32(at + 0xc)}; }
    operator std::uint32_t() const { return at; }
};
struct CompetitionView {
    Cpu& c;
    std::uint32_t at;
    std::uint32_t name() const { return at + 0x4d; }
    std::uint32_t marker() const { return signed_byte(c.u8(at + 0x67)); }
    std::uint32_t short_name() const { return at + 0x1a; }
    std::uint32_t short_marker() const { return signed_byte(c.u8(at + 0x6a)); }
    operator std::uint32_t() const { return at; }
};
// A table: the TOC slot holds the address of the word that points to the rows.
template <class View>
struct Rows {
    Cpu& c;
    int slot;
    std::uint32_t size;
    std::uint32_t base() const { return c.u32(c.toc(slot)); }
    View operator[](std::uint32_t index) const { return {c, base() + index * size}; }
};

// The story being written.
struct Report {
    Report(Cpu& cpu, const Frame& f, std::uint32_t output_object) : c(cpu), frame(f), out{cpu, output_object} {}
    Cpu& c;
    const Frame& frame;
    text::Output out;
    Rows<ClubView> clubs{c, -0x7be8, club_size};
    Rows<NationView> nations{c, -0x7b98, 0x128};
    Rows<PersonView> people{c, -0x7be0, staff_size};
    Rows<CompetitionView> competitions{c, -0x7bbc, 0x6c};

    // The record's values the original keeps in registers. Their roles are inferred from how the wordings use them.
    Var<26> subject{c};         // the staff member the story is about
    Var<25> club{c};            // his club
    Var<24> tone{c};            // 0-4: how the wording leans
    Var<23> flags{c};           // bits that choose the wording
    Var<22> amount{c};          // a sum the story names
    Var<21> competition{c};     // a competition
    Var<20> suitor{c};          // a club that wants him (-1: none)
    Var<19> legend{c};          // a famous person quoted (-1: none)
    Var<18> nation_a{c};        // two nations the story mentions (-1: none)
    Var<17> nation_b{c};
    Var<16> previous_club{c};   // a club he came from (-1: none)
    Var<14> domestic{c};        // whether the club's nation is the user's (the outlet is the national kind then)
    Var<13> outcome{c};         // how a match story ends (1-4)
    Var<27> amount_flag{c};     // the marker byte the amount's wording leaves
    Var<14>& competition_article = domestic;  // (the match report reuses r14 for the competition's article)
    Var<15> output_object{c};


    // The record's values and buffers in the frame.
    Field<std::uint8_t> full{c, frame.at(0x1f43)};
    Field<std::uint32_t> subject_first{c, frame.at(0x1ec8)};
    Field<std::uint32_t> subject_middle{c, frame.at(0x1ec4)};
    Field<std::uint32_t> subject_last{c, frame.at(0x1ec0)};
    Field<std::uint32_t> kind{c, frame.at(0x1ebc)};
    Field<std::uint32_t> nation{c, frame.at(0x1eb8)};
    Field<std::uint32_t> other{c, frame.at(0x1eb4)};
    Field<std::uint32_t> other_first{c, frame.at(0x1eb0)};
    Field<std::uint32_t> other_middle{c, frame.at(0x1eac)};
    Field<std::uint32_t> other_last{c, frame.at(0x1ea8)};
    Field<std::uint32_t> closing{c, frame.at(0x1ea4)};
    Field<std::uint8_t> with_match{c, frame.at(0x1ea0)};
    Field<std::uint32_t> transfer_agreed{c, frame.at(0x1e9c)};
    Field<std::int16_t> due_day{c, frame.at(0x1e98)};
    Field<std::uint16_t> due_year{c, frame.at(0x1e96)};
    Field<std::uint8_t> transfer_rumour{c, frame.at(0x1e94)};
    Field<std::uint32_t> legend_first{c, frame.at(0x1e90)};
    Field<std::uint32_t> legend_middle{c, frame.at(0x1e8c)};
    Field<std::uint32_t> legend_last{c, frame.at(0x1e88)};
    Field<std::uint32_t> legend_job{c, frame.at(0x1e84)};
    Field<std::uint32_t> outlet_kind{c, frame.at(0x1e80)};
    Field<std::uint32_t> outlet_region{c, frame.at(0x1e7c)};
    Field<std::uint32_t> position{c, frame.at(0x1e78)};
    Field<std::uint32_t> second_division{c, frame.at(0x1e74)};
    Field<std::uint32_t> other_row{c, frame.at(0x1e70)};
    Field<std::uint32_t> users_manager{c, frame.at(0x1e6c)};
    Field<std::uint32_t> match_id{c, frame.at(0xfc)};
    Field<std::uint8_t> second_goals{c, frame.at(0xf9)};
    Field<std::uint8_t> first_goals{c, frame.at(0xf8)};
    Field<std::uint32_t> second_club{c, frame.at(0xf4)};
    Field<std::uint32_t> first_club{c, frame.at(0xf0)};
    Field<std::int16_t> match_year{c, frame.at(0xea)};
    Field<std::int16_t> match_day{c, frame.at(0xe8)};
    Field<std::uint32_t> match_competition{c, frame.at(0xe4)};
    Field<std::uint16_t> today_year{c, frame.at(0xde)};
    Field<std::int16_t> today_day{c, frame.at(0xdc)};
    Field<std::uint8_t> article_marker{c, frame.at(0x8c)};

    const std::uint32_t story = frame.at(0xae4);
    const std::uint32_t paragraph = frame.at(0x6fc);
    const std::uint32_t amount_text = frame.at(0x634);
    const std::uint32_t competition_name = frame.at(0x56c);
    const std::uint32_t subject_full = frame.at(0x4f4);
    const std::uint32_t subject_short = frame.at(0x47c);
    const std::uint32_t other_full = frame.at(0x404);
    const std::uint32_t other_short = frame.at(0x38c);
    const std::uint32_t nationality = frame.at(0x314);
    const std::uint32_t legend_name = frame.at(0x29c);
    const std::uint32_t due_date = frame.at(0x224);
    const std::uint32_t due_date_short = frame.at(0x1ac);
    const std::uint32_t outlet = frame.at(0x100);
    const std::uint32_t score = frame.at(0xb0);
    const std::uint32_t date_long = frame.at(0xa0);
    const std::uint32_t date_short = frame.at(0x98);

    std::uint32_t pool(unsigned offset) const { return c.toc(slot::pool) + offset; }
    Field<std::uint8_t> byte_at(unsigned offset) const { return {c, frame.at(offset)}; }
    int cmp(std::int32_t a, std::int32_t b) const { return compare(c, a, b); }
    int cmpl(std::uint32_t a, std::uint32_t b) const { return compare_logical(c, a, b); }
    std::uint32_t call(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> args = {}) const {
        return Program{c}.call(entry, back, args);
    }
    // One phrase of the pool formatted into `destination`: the arguments go to r5-r10 and the frame's outgoing words;
    // `leftovers` are the registers the compiler left holding dead variables there.
    void say(std::uint32_t destination, unsigned phrase, std::uint32_t back, std::initializer_list<Words> args,
             std::initializer_list<RegisterValue> leftovers = {}) const;

    void read_record(std::uint32_t record);
    void outlet_text();
    void legend_text();
    void long_story();
    void links();
    void outcome_story(unsigned place);
    void kind_29();
    void kind_2a();
    void kind_2b();
    void kind_2c();
    void kind_2d();
    void kind_2e();
    void kind_2f();
    void kind_30();
    void kind_31();
    void kind_32();
    void kind_33();
    void kind_33_after();
    void kind_34();
    void match_story();
    void match_story_other();
    void paragraph_two();
    void short_story();
};
}

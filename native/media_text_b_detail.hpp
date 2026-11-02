#pragma once
// What the board and fans stories share: the story's variables (read field by field from the
// record), the phrases with their arguments as data (`Line`), and the choosing of one of them.
#include "media_detail.hpp"
#include "media_text_mid_detail.hpp"
#include <array>

namespace cm::aot::media::detail::press {
using Output = detail::text::Output;

// An argument of a phrase: a number (the plain marker 0x64...), or something the story holds.
struct Arg {
    constexpr Arg() = default;
    constexpr Arg(std::uint32_t value) : v(value) {}
    std::uint32_t v = 0;
};
// Things a phrase names: the three people's names (each in full and by surname), the club and the pool's
// own strings.
inline constexpr std::uint32_t slot_base = 0x80000000, pool_base = 0x40000000;
inline constexpr Arg subject_full{slot_base + 1}, subject_short{slot_base + 2}, chairman_full{slot_base + 3},
    chairman_short{slot_base + 4}, manager_full{slot_base + 5}, manager_short{slot_base + 6}, club_short{slot_base + 7},
    club_marker{slot_base + 8}, poll_percent{slot_base + 9};
constexpr Arg pool_text(std::uint32_t offset) { return Arg{pool_base + offset}; }

// A phrase of the pool, the address after the call that formats it and up to ten arguments.
struct Line {
    std::uint32_t phrase, back;
    std::array<Arg, 10> args;
    unsigned count;
};
template <class... A>
constexpr Line L(std::uint32_t phrase, std::uint32_t back, A... args) {
    static_assert(sizeof...(A) <= 10);
    Line line{phrase, back, {}, unsigned(sizeof...(A))};
    unsigned i = 0;
    ((line.args[i++] = Arg(args)), ...);
    return line;
}

// The paragraph a flag of the record writes: three phrases (by the remainder of the count) and the return
// address of the append that follows.
struct FlagParagraph {
    std::int32_t bit;
    Line lines[3];
    std::uint32_t append_back;
};

// A story being written: the record's variables and the places in the frame.
struct Press {
    Press(Cpu& cpu, const Frame& f, std::uint32_t owner_object, std::uint32_t output)
        : c(cpu), frame(f), owner(owner_object), text(cpu.toc(slot::text)), out{cpu, output} {}
    Cpu& c;
    const Frame& frame;
    std::uint32_t owner;
    std::uint32_t text;  // the shared text buffer
    Output out;
    // The record: which kind of story, code, tone, the club, the people involved...
    std::int32_t about_board = 0, code = 0, tone = 0, seed = 0, kind = 0, count = 0, flags = 0, last = 0;
    std::int32_t extra_a = 0, extra_b = 0, chair = 0, own_club = 0;
    std::uint32_t club = 0, manager = 0, subject = 0, chairman = 0;

    std::uint32_t frame_at(std::uint32_t offset) const { return frame.at(std::int32_t(offset)); }
    std::uint32_t note_at(std::uint32_t offset) const { return c.u32(frame_at(offset)); }
    std::int32_t note_a() const { return std::int32_t(note_at(0x320)); }
    std::int32_t note_b() const { return std::int32_t(note_at(0x31c)); }
    std::int32_t note_c() const { return std::int32_t(note_at(0x318)); }
    int cmp(std::int32_t a, std::int32_t b) const { return compare(c, a, b); }
    int cmpl(std::uint32_t a, std::uint32_t b) const { return compare_logical(c, a, b); }
    // The compare chain every three-way pick makes on a remainder: 1, then more than 1, then 0.
    // Gives the index of the phrase: 0 for 0, 1 for 1 and 2 for anything else.
    unsigned three_ways(std::int32_t rest) const {
        if (cmp(rest, 1) == 0) return 1;
        if (cmp(rest, 1) > 0) return 2;
        return cmp(rest, 0) >= 0 ? 0 : 2;
    }
    // The story-kind tests of the leaves module, called with the caller's object and the story code.
    bool transfer(std::uint32_t back) const;
    bool ordinary(std::uint32_t back) const;
    bool grade_band(std::uint32_t back) const;
    std::uint32_t value(Arg arg) const;
    void say(const Line& line) const;
    // Leaves chosen by whether a chairman is named (chair is 0: none), and with it by whether the club is
    // the user's own (own_club is 0: it is not) or the subject is ordinary (the odd-or-ordinary test at `back`).
    void by_chair(const Line& none, const Line& named) const { say(cmp(chair, 0) == 0 ? none : named); }
    void by_owner_and_chair(const Line& away_none, const Line& away_named, const Line& own_none, const Line& own_named) const {
        if (cmp(own_club, 0) == 0) return by_chair(away_none, away_named);
        by_chair(own_none, own_named);
    }
    void by_ordinary_and_chair(std::uint32_t back, const Line& odd_none, const Line& odd_named, const Line& ordinary_none,
                               const Line& ordinary_named) const {
        if (!ordinary(back)) return by_chair(odd_none, odd_named);
        by_chair(ordinary_none, ordinary_named);
    }

    // The stories, by who they are about and how long.
    void short_board();
    void short_fans();
    void long_board();
    void long_fans();
    void paragraph();
    bool nation_is_foreign() const;
    void flag_paragraph(const FlagParagraph& entry) const;
};
}

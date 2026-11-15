// FINANCE.CPP news block (finance_news.hpp): the headline and body text of the board and stadium stories. Each
// routine reads the item's words (0x102cb570), picks one of the unit's pool texts, formats it into a buffer of its
// frame (0x10727890; the money amounts first through 0x10009f20 and 0x1000b410) and gives the buffer to the text
// object (0x1074b830), with a link row (0x1074bab0) for a club that the text names.
#include "finance_news.hpp"
#include "finance_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>
#include <stdexcept>

namespace cm::aot::finance {
using guest::Frame;

namespace {
namespace text_call {
inline constexpr std::uint32_t read_word = 0x102cb570, format = 0x10727890, set_text = 0x1074b830, link = 0x1074bab0,
                               amount_parts = 0x10009f20, amount_text = 0x1000b410;
}
namespace slot {
inline constexpr std::int32_t clubs = -0x7be8, competitions = -0x7bbc, stadiums = -0x7548;
}
namespace row {  // fields of the records the texts name
inline constexpr std::uint32_t club_gender = 0x242, club_name = 0x1ee, club_headline_gender = 0x23b,
                               club_headline_name = 0x221, club_reputation = 0x1ec, stadium_size = 0x50,
                               stadium_seats = 4, stadium_standing = 8, stadium_name = 0x18, stadium_gender = 0x4b,
                               competition_size = 0x6c, competition_gender = 0x6a, competition_name = 0x1a;
}
inline constexpr std::uint32_t club_link = 0x7d5;           // the link row of a club
inline constexpr std::uint32_t big_amount = 50000;          // "a lot of money available", in pounds
inline constexpr std::uint32_t reputation_scale = 0x318;    // in the double pool: the reputation's squared scale

std::uint32_t wide(std::int32_t narrow) { return std::uint32_t(narrow); }

// One text of a routine's frame: `buffer` is where the formats are written. Every call site is the address of
// its `bl` in the original; the call returns to the next instruction.
class Story {
public:
    Story(Cpu& c, const Frame& frame, std::uint32_t buffer_offset, std::uint32_t out)
        : c(c), frame(frame), buffer(frame.at(std::int32_t(buffer_offset))), out(out), pool(c.toc(toc::pool)) {}

    std::uint32_t read(std::uint32_t item, unsigned index, std::uint32_t site) const {
        return invoke(c, text_call::read_word, site + 4, {item, index});
    }
    // The format at `offset` of the pool with the values of its replacements (r5 onwards).
    void format(std::uint32_t offset, std::uint32_t site, std::initializer_list<std::uint32_t> values = {}) const {
        unsigned n = 5;
        c.set({{3, buffer}, {4, pool + offset}});
        for (const auto value : values) c.set(n++, value);
        c.call(text_call::format, site + 4);
    }
    // A club's name and gender replacements: the gender twice (the text names it for the two sides of its
    // markup), then the name.
    void format_club(std::uint32_t offset, std::uint32_t site, std::uint32_t club) const {
        const auto gender = wide(c.i8(club + row::club_gender));
        format(offset, site, {gender, gender, club + row::club_name});
    }
    // The money amount held at `slot` in the frame: its parts (0x10009f20) and its text at `text`.
    void amount(std::uint32_t slot, std::uint32_t text, std::uint32_t parts_site, std::uint32_t text_site) const {
        invoke(c, text_call::amount_parts, parts_site + 4, {frame.at(std::int32_t(slot)), 4});
        const auto high = c.r(3), low = c.r(4);
        invoke(c, text_call::amount_text, text_site + 4, {frame.at(std::int32_t(text)), 0x3e8, high, low, 4});
    }
    void link(std::uint32_t club, std::uint32_t site) const {
        invoke(c, text_call::link, site + 4, {out, club + row::club_name, club_link, club});
    }
    void finish(std::uint32_t site) const { invoke(c, text_call::set_text, site + 4, {out, buffer}); }
    std::uint32_t frame_at(std::uint32_t offset) const { return frame.at(std::int32_t(offset)); }

    Cpu& c;
    const Frame& frame;
    std::uint32_t buffer, out, pool;
};

std::uint32_t club_row(Cpu& c, std::uint32_t index) { return c.u32(c.toc(slot::clubs)) + index * club_size; }
std::uint32_t stadium_row(Cpu& c, std::uint32_t index) { return c.u32(c.toc(slot::stadiums)) + index * row::stadium_size; }

// The money the club's reputation stands for: its squared value times a scale of the double pool, truncated.
// The texts compare an amount against it to tell "enough to buy players" from "very little".
std::int32_t reputation_limit(Cpu& c, std::uint32_t club) {
    guest::Fpu fpu{c};
    const auto pool = c.toc(toc::doubles);
    const auto to_double = [&](std::int32_t value) {
        const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(value) ^ 0x80000000u);
        return fpu.fsub(std::bit_cast<double>(bits), c.f64(pool + doubles::int_magic));
    };
    const auto reputation = std::int32_t(c.i16(club + row::club_reputation));
    const auto first = to_double(reputation), second = to_double(reputation);
    const auto square = fpu.fmul(first, second);
    const auto scaled = fpu.fmul(c.f64(pool + reputation_scale), square);
    return std::int32_t(std::uint32_t(fpu.fctiwz(0, scaled)));
}
}

// 0x101020c0. Word 0 the club, 1 the injection, 2 the budget, 3 the event (board_event kinds in finance_news.hpp).
void board_event_text(Cpu& c, std::uint32_t item, std::uint32_t body, std::uint32_t out) {
    Frame frame(c, 0x101020c0, 0x1be0, {.first_gpr = 23});
    const Story story(c, frame, 0x814, out);
    c.set({{25, c.toc(slot::clubs)}, {23, item}, {26, c.toc(toc::doubles)}, {31, out}, {27, c.toc(toc::pool)}});
    if (std::int8_t(body) == 0) {
        const auto club_index = story.read(item, 0, 0x101020f4);
        c.set(26, club_index);
        const auto injection = story.read(item, 1, 0x10102104);
        c.set(24, injection);
        const auto event = std::int32_t(story.read(item, 3, 0x10102114));
        const auto headline_club = [&](std::uint32_t offset, std::uint32_t format_site, std::uint32_t link_site) {
            c.set(24, club_index * club_size);
            const auto club = club_row(c, club_index);
            const auto gender = wide(c.i8(club + row::club_headline_gender));
            story.format(offset, format_site, {gender, gender, club + row::club_headline_name});
            invoke(c, text_call::link, link_site + 4, {out, club + row::club_headline_name, club_link, club});
        };
        switch (event) {
        case 1: story.format(0x1a9, 0x10102170); break;
        case 2: headline_club(0x1c7, 0x101021a0, 0x101021b8); break;
        case 3:
        case 4:
            c.put32(story.frame_at(0x40), injection);
            story.amount(0x40, 0x42c, 0x10102214, 0x1010222c);
            story.format(0x22a, 0x1010223c, {story.frame_at(0x42c)});
            break;
        case 5: story.format(0x193, 0x10102160); break;
        case 6: story.format(0x174, 0x10102150); break;
        case 7: headline_club(0x1ff, 0x101021e8, 0x10102200); break;
        default: break;
        }
    } else {
        const auto club_index = story.read(item, 0, 0x1010224c);
        c.set(28, club_index);
        const auto injection = story.read(item, 1, 0x1010225c);
        c.set(29, injection);
        const auto budget = story.read(item, 2, 0x1010226c);
        c.set(30, budget);
        const auto event = story.read(item, 3, 0x1010227c);
        c.set(24, event);
        c.put32(story.frame_at(0x3c), injection);
        c.put32(story.frame_at(0x38), budget);
        story.amount(0x3c, 0x42c, 0x10102294, 0x101022ac);
        story.amount(0x38, 0x44, 0x101022b8, 0x101022d0);
        const auto club = club_row(c, club_index);
        const auto named = [&](std::uint32_t offset, std::uint32_t format_site, std::initializer_list<std::uint32_t> more = {}) {
            // Gender, gender, the club's name, then the amounts the text carries.
            const auto gender = wide(c.i8(club + row::club_gender));
            unsigned n = 8;
            c.set({{3, story.buffer}, {4, story.pool + offset}, {5, gender}, {6, gender}, {7, club + row::club_name}});
            for (const auto value : more) c.set(n++, value);
            c.call(text_call::format, format_site + 4);
        };
        const auto amounts = story.frame_at(0x42c), budget_text = story.frame_at(0x44);
        const auto rich = [&](std::uint32_t amount) { return std::int32_t(amount) > std::int32_t(big_amount); };
        switch (event) {  // an unsigned compare: only 0..7 are stories
        case 1:
            if (std::int32_t(budget) > reputation_limit(c, club)) {
                if (rich(budget)) story.format(0x42a, 0x101023bc, {amounts, budget_text});
                else story.format(0x4af, 0x101023d0, {amounts});
            } else if (std::int32_t(budget) >= 0) {
                story.format(0x5da, 0x10102400, {amounts});
            } else {
                story.format(0x542, 0x101023ec, {amounts});
            }
            break;
        case 2:
            // The budget is tested against the limit only when it is large; the text then names it, else the
            // injection decides between two wordings.
            if (rich(budget) && std::int32_t(budget) > reputation_limit(c, club)) named(0x689, 0x10102494, {budget_text});
            else if (rich(injection)) named(0x74e, 0x101024d8, {amounts});
            else named(0x818, 0x10102508);
            story.link(club, 0x10102524);
            break;
        case 3: story.format(0x351, 0x10102324, {amounts}); break;
        case 4: story.format(0x3c8, 0x10102338, {amounts}); break;
        case 5: story.format(0x2df, 0x10102310, {amounts}); break;
        case 6: story.format(0x25d, 0x101022fc, {amounts}); break;
        case 7:
            c.set(24, club_index * club_size);
            named(0x8d1, 0x10102554);
            story.link(club, 0x1010256c);
            break;
        default: break;
        }
    }
    story.finish(0x10102578);
}
// 0x10102660. Word 1 is the stadium (-1: none), 2 the seats or capacity involved, 3 whether it was a stand (the
// terracing otherwise).
void stand_closed_text(Cpu& c, std::uint32_t item, std::uint32_t body, std::uint32_t out) {
    Frame frame(c, 0x10102660, 0x13e0, {.first_gpr = 27});
    const Story story(c, frame, 0x38, out);
    c.set({{28, c.toc(slot::stadiums)}, {27, item}, {29, c.toc(toc::pool)}, {31, out}});
    if (std::int8_t(body) == 0) {
        if (story.read(item, 3, 0x10102690) != 0) story.format(0x979, 0x101026a4);
        else story.format(0x986, 0x101026b4);
    } else {
        const auto stadium_index = story.read(item, 1, 0x101026c4);
        c.set(30, stadium_index);
        const auto size = story.read(item, 2, 0x101026d4);
        c.set(27, size);
        const auto flag_word = story.read(item, 3, 0x101026e8);
        const bool stand = std::int8_t(flag_word) != 0;
        const auto stadium = stadium_row(c, stadium_index);
        if (std::int32_t(stadium_index) == -1) {
            // No stadium: the row is read one row before the table's start. The seating text prints the flag
            // (r5 still holds it from 0x101026f4) where its capacity would be; the last branch tests a row
            // pointer that was just set to zero, so 0x101027d0..0x101027e8 never run.
            if (stand) {
                if (c.u32(stadium + row::stadium_standing) != 0) story.format(0xb27, 0x101027b4, {wide(std::int8_t(flag_word))});
                else story.format(0xbad, 0x101027c4);
            } else {
                story.format(0xc17, 0x101027f4);
            }
        } else if (stand) {
            if (c.u32(stadium + row::stadium_standing) != 0) story.format(0x9b3, 0x10102744, {size});
            else story.format(0xa30, 0x10102754);
        } else if (stadium != 0) {
            story.format(0xaa4, 0x10102774, {c.u32(stadium + row::stadium_seats) - size});
        } else {
            story.format(0xa30, 0x10102784);
        }
    }
    story.finish(0x10102800);
}

// 0x10102910. Word 1 is the kind of works (1 to 4 and 6 are stadium plans, the others the training ground), 2 why
// they stop (0 no money, 1 cancelled, anything else the council).
void expansion_text(Cpu& c, std::uint32_t item, std::uint32_t body, std::uint32_t out) {
    Frame frame(c, 0x10102910, 0x13e0, {.first_gpr = 27});
    const Story story(c, frame, 0x38, out);
    c.set({{27, item}, {28, body}, {29, out}, {30, c.toc(toc::pool)}});
    const auto kind = std::int32_t(story.read(item, 1, 0x10102938));
    c.set(31, kind);
    const auto reason = std::int32_t(story.read(item, 2, 0x10102948));
    const bool stadium_plan = (kind >= 1 && kind <= 4) || kind == 6;
    if (std::int8_t(body) == 0) {
        if (!stadium_plan) story.format(0xcef, 0x101029d4);
        else if (reason == 0) story.format(0xc8a, 0x101029a4);
        else if (reason == 1) story.format(0xcaa, 0x101029b4);
        else story.format(0xcc6, 0x101029c4);
    } else {
        if (!stadium_plan) story.format(0xe01, 0x10102a48);
        else if (reason == 0 || reason == 1) story.format(0xd16, 0x10102a28);
        else story.format(0xd81, 0x10102a38);
    }
    story.finish(0x10102a54);
}

// 0x10102b50. Word 0 is the club, 1 whether the receivers are in (0: the crisis).
void financial_crisis_text(Cpu& c, std::uint32_t item, std::uint32_t body, std::uint32_t out) {
    Frame frame(c, 0x10102b50, 0x13e0, {.first_gpr = 27});
    const Story story(c, frame, 0x38, out);
    c.set({{29, c.toc(slot::clubs)}, {27, item}, {30, c.toc(toc::pool)}, {28, out}});
    if (std::int8_t(body) == 0) {
        if (std::int16_t(story.read(item, 1, 0x10102b80)) == 0) story.format(0xe5f, 0x10102ba8);
        else story.format(0xe78, 0x10102bb8);
    } else {
        const auto club_index = story.read(item, 0, 0x10102bc8);
        c.set(31, club_index);
        const auto state = std::int16_t(story.read(item, 1, 0x10102bd8));
        const auto club = club_row(c, club_index);
        if (state == 0) story.format_club(0xe8c, 0x10102c20, club);
        else if (state == 1) story.format_club(0xf63, 0x10102c50, club);
        story.link(club, 0x10102c6c);
    }
    story.finish(0x10102c78);
}

// 0x10102d20. Word 1 is the dividend total; word 2 is read and tested, but both of its outcomes format the same text.
void dividend_text(Cpu& c, std::uint32_t item, std::uint32_t body, std::uint32_t out) {
    Frame frame(c, 0x10102d20, 0x17c0, {.first_gpr = 28});
    const Story story(c, frame, 0x424, out);
    c.set({{30, c.toc(toc::pool)}, {29, out}, {28, item}});
    if (std::int8_t(body) == 0) {
        story.format(0x1027, 0x10102d58);
    } else {
        const auto total = story.read(item, 1, 0x10102d68);
        c.set(31, total);
        const auto flag = story.read(item, 2, 0x10102d78);
        c.put32(story.frame_at(0x38), total);
        c.set(31, wide(std::int8_t(flag)));
        story.amount(0x38, 0x3c, 0x10102d8c, 0x10102da4);
        if (std::int8_t(flag) != 0) story.format(0x1044, 0x10102dbc, {story.frame_at(0x3c)});
        else story.format(0x1044, 0x10102dd0, {story.frame_at(0x3c)});
    }
    story.finish(0x10102ddc);
}

// 0x10102ea0. Word 1 is a count or a stadium index, word 2 the kind of works (see post_stadium_works_news).
void stadium_works_text(Cpu& c, std::uint32_t item, std::uint32_t body, std::uint32_t out) {
    Frame frame(c, 0x10102ea0, 0x13d0, {.first_gpr = 28});
    const Story story(c, frame, 0x38, out);
    c.set({{31, out}, {30, c.toc(toc::pool)}, {29, c.toc(slot::stadiums)}, {28, item}});
    if (std::int8_t(body) == 0) {
        switch (std::int8_t(story.read(item, 2, 0x10102edc))) {
        case 4: story.format(0x10e4, 0x10102f34); break;
        case 5: story.format(0x10c7, 0x10102f24); break;
        case 6: story.format(0x10ff, 0x10102f44); break;
        case 7: story.format(0x10ac, 0x10102f14); break;
        case 8: story.format(0x111b, 0x10102f54); break;
        default: story.format(0x1136, 0x10102f64); break;
        }
    } else {
        const auto quantity = story.read(item, 1, 0x10102f74);
        c.set(28, quantity);
        const auto works = std::uint32_t(std::int32_t(std::int8_t(story.read(item, 2, 0x10102f88))));
        // The works of a stadium (4 to 6) name it: its gender byte twice, then its name.
        const auto stadium_text = [&](std::uint32_t offset, std::uint32_t site) {
            const auto stadium = stadium_row(c, quantity);
            const auto gender = wide(c.i8(stadium + row::stadium_gender));
            story.format(offset, site, {gender, gender, stadium + row::stadium_name});
        };
        switch (works > 8 ? 0 : works) {  // an unsigned compare: negative bytes and 9 or more say nothing
        case 1: story.format(0x13c6, 0x10103058, {quantity}); break;
        case 2: story.format(0x1415, 0x1010306c, {quantity}); break;
        case 3: story.format(0x148f, 0x10103080, {quantity}); break;
        case 4: stadium_text(0x133b, 0x10103044); break;
        case 5: stadium_text(0x1294, 0x10103014); break;
        case 6: stadium_text(0x11cf, 0x10102fe4); break;
        case 7: story.format(0x1158, 0x10102fb4); break;
        case 8: story.format(0x14e2, 0x10103090); break;
        default: break;
        }
    }
    story.finish(0x1010309c);
}

// 0x10103180. Word 0 is the club, 2 its new stadium (word 1 is read and not used).
void relocation_text(Cpu& c, std::uint32_t item, std::uint32_t body, std::uint32_t out) {
    Frame frame(c, 0x10103180, 0x13d0, {.first_gpr = 28});
    const Story story(c, frame, 0x38, out);
    c.set({{30, c.toc(toc::pool)}, {29, out}, {28, item}});
    if (std::int8_t(body) == 0) {
        story.format(0x154b, 0x101031b8);
    } else {
        const auto club_index = story.read(item, 0, 0x101031c8);
        c.set(31, club_index);
        story.read(item, 1, 0x101031d8);
        const auto stadium_index = story.read(item, 2, 0x101031e4);
        const auto stadium = stadium_row(c, stadium_index), club = club_row(c, club_index);
        const auto club_gender = wide(c.i8(club + row::club_gender)), stadium_gender = wide(c.i8(stadium + row::stadium_gender));
        story.format(0x156b, 0x10103238,
                     {club_gender, club_gender, stadium_gender, stadium_gender, club + row::club_name, stadium + row::stadium_name});
    }
    story.finish(0x10103244);
}

// 0x101032e0. Word 0 is the club.
void receivers_appeased_text(Cpu& c, std::uint32_t item, std::uint32_t body, std::uint32_t out) {
    Frame frame(c, 0x101032e0, 0x13d0, {.first_gpr = 30});
    const Story story(c, frame, 0x38, out);
    c.set({{31, c.toc(toc::pool)}, {30, out}});
    if (std::int8_t(body) == 0) {
        story.format(0x1664, 0x1010330c);
    } else {
        const auto club_index = story.read(item, 0, 0x1010331c);
        story.format_club(0x1677, 0x1010334c, club_row(c, club_index));
    }
    story.finish(0x10103358);
}

// 0x10103420. Word 1 is the competition, 2 whether the stadium is enlarged (1) or redeveloped (0).
void competition_stadium_text(Cpu& c, std::uint32_t item, std::uint32_t body, std::uint32_t out) {
    Frame frame(c, 0x10103420, 0x13e0, {.first_gpr = 27});
    const Story story(c, frame, 0x38, out);
    c.set({{29, c.toc(slot::competitions)}, {27, item}, {30, c.toc(toc::pool)}, {28, out}});
    if (std::int8_t(body) == 0) {
        story.format(0x1136, 0x10103450);
    } else {
        const auto competition_index = story.read(item, 1, 0x10103460);
        c.set(31, competition_index);
        const auto enlarge = std::int8_t(story.read(item, 2, 0x10103470));
        if (enlarge == 0 || enlarge == 1) {
            const auto competition = c.u32(c.toc(slot::competitions)) + competition_index * row::competition_size;
            const auto gender = wide(c.i8(competition + row::competition_gender));
            story.format(enlarge == 1 ? 0x1704 : 0x1781, enlarge == 1 ? 0x101034b8 : 0x101034e8,
                         {gender, gender, competition + row::competition_name});
        }
    }
    story.finish(0x101034f4);
}
}

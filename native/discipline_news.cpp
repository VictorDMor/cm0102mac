// DISCIPLINE.CPP news block (discipline_news.hpp): the ban story of news message type 2002, native-first.
//
// The routine reads the message's fields, looks up the fixture, the clubs and the competition, and picks one of ~150
// localized formats of the module's pool. The choice is plain control flow below; each choice names a row of the
// tables (format offsets, and the original return address of each call, which the call trace compares) and the
// shape of the values the format reads (the clubs' gender bytes and names, the competition's, the texts built
// before it).
#include "discipline_news.hpp"
#include "discipline_texts.hpp"
#include <array>
#include <initializer_list>

namespace cm::aot::discipline {
namespace {
namespace callee {
inline constexpr std::uint32_t init_text = 0x10715880, read_field = 0x102cb570, decode_stats = 0x1030e2a0,
                               decode_fixture = 0x1030dee0, competition_hosted = 0x104abf40, competition_cup = 0x104646d0,
                               name_matches = 0x100cbde0, fixture_link = 0x1029eb20, name_text = 0x100cb590,
                               date_pieces = 0x107156b0, date_text = 0x10717620, day_text = 0x1030e880, format = 0x10727890,
                               plain_format = 0x1077b720, set_text = 0x1074b830, append_text = 0x1074b960,
                               link = 0x1074bab0, concat = 0x1077dc60;
}
namespace toc {
inline constexpr std::int32_t pool = -0x5608, buffer = -0x7bb8, competitions = -0x7bbc, clubs = -0x7be8, people = -0x7be0,
                              competition_kinds = -0x7b34, fixtures = -0x7b4c;
}
namespace at {  // the original's frame (size 0x3970): offsets of its locals
inline constexpr std::int32_t counter = 0x60, bodies = 0x64, end_parts = 0x68, score_text = 0x70, day_text = 0x7c,
                              person = 0x9c, person_index = 0xa8, name_object = 0xac, fixture = 0xb4,
                              end_date_text = 0xd8, bodies_text = 0x13c, name_text = 0x1a0, extra_flag = 0x3918,
                              fixture_link = 0x3919;
inline constexpr std::int32_t main_text = 0x1208, suffix_text = 0xe20;
// The remark texts of the ban, one buffer for each place that formats it (main text, then the three kinds that
// concatenate it to the main text).
inline constexpr std::int32_t remark[4] = {main_text, 0xa38, 0x650, 0x268};
// Inside the fixture object (at::fixture): its two halfwords, competition, two clubs and their scores, record.
inline constexpr std::int32_t fixture_day = fixture, fixture_year = fixture + 2, fixture_competition = fixture + 8,
                              fixture_kind = fixture + 0xc, fixture_group = fixture + 0xe, club_one = fixture + 0x14,
                              club_two = fixture + 0x18, score_one = fixture + 0x1c, score_two = fixture + 0x1d,
                              fixture_record = fixture + 0x20;
}
namespace row {  // record sizes and the offsets read in them
inline constexpr std::uint32_t club_size = 0x248, club_name = 0x221, club_gender = 0x23b, competition_size = 0x6c,
                               competition_gender = 0x6a, competition_other_gender = 0x67, competition_name = 0x4d,
                               competition_short = 0x1a, person_size = 0x70;
}
namespace link_tag {  // the kind of link a text object row carries
inline constexpr std::uint32_t club = 0x7d5, fixture = 0x7d3, person = 0x7d9;
}
inline constexpr std::uint32_t module_page = 0x100e0000;  // the return addresses in the tables are offsets in it

// What the values of a format are made of (see ban_values).
enum class Names { None, Clubs, Competition };
enum class Extra { None, Length, Date };
// The three texts of one case of the story: with the message's flag set, without an end date, with one. Each has
// its format (offset in the pool) and the original return address (offset in the module's page) of its call.
struct Wording {
    Names names;
    Extra extra;
    std::uint16_t format[3];
    std::uint16_t back[3];
    bool second_hundred = false;  // the one text with an end date that takes the width twice
};
namespace wordings {
// By the ban's kind; [0] where it lasts one match, [1] where it lasts several (a length of 0 or 2 and more).
constexpr Wording kind_zero[2] = {{Names::None, Extra::None, {0x6a8, 0x73e, 0x7ce}, {0x148c, 0x14b8, 0x14ec}, true},
                                  {Names::None, Extra::Length, {0x897, 0x944, 0x9e7}, {0x151c, 0x154c, 0x1580}}};
constexpr Wording kind_one_competition[2] = {{Names::Competition, Extra::None, {0xac1, 0xbc8, 0xcc9}, {0x1644, 0x16e8, 0x1790}},
                                             {Names::Competition, Extra::Length, {0xe01, 0xf1f, 0x1033}, {0x1838, 0x18e0, 0x198c}}};
// By the clubs' scores: [0] the first club ahead, [1] behind, [2] level.
constexpr Wording kind_one_clubs[3][2] = {
    {{Names::Clubs, Extra::None, {0x117e, 0x126b, 0x1352}, {0x1ce0, 0x1d60, 0x1de4}},
     {Names::Clubs, Extra::Length, {0x1470, 0x1574, 0x166e}, {0x1e68, 0x1eec, 0x1f74}}},
    {{Names::Clubs, Extra::None, {0x179f, 0x188d, 0x1975}, {0x2000, 0x2080, 0x2104}},
     {Names::Clubs, Extra::Length, {0x1a94, 0x1b99, 0x1c94}, {0x2188, 0x220c, 0x2294}}},
    {{Names::Clubs, Extra::None, {0x1dc6, 0x1eb4, 0x1f9c}, {0x231c, 0x239c, 0x2420}},
     {Names::Clubs, Extra::Length, {0x20bb, 0x21c0, 0x22bb}, {0x24a4, 0x2528, 0x25b0}}}};
constexpr Wording kind_one_alone[2] = {{Names::None, Extra::None, {0x23ed, 0x2489, 0x251f}, {0x25e4, 0x2610, 0x263c}},
                                       {Names::None, Extra::Length, {0x25ec, 0x269f, 0x2748}, {0x266c, 0x269c, 0x26d0}}};
constexpr Wording kind_four[2] = {{Names::None, Extra::None, {0x2828, 0x28da, 0x2986}, {0x270c, 0x2738, 0x2764}},
                                  {Names::None, Extra::Length, {0x2a69, 0x2b32, 0x2bf1}, {0x2794, 0x27c4, 0x27f8}}};
constexpr Wording kind_two_competition[2] = {{Names::Competition, Extra::None, {0x2ce7, 0x2de4, 0x2edb}, {0x28b4, 0x2958, 0x2a00}},
                                             {Names::Competition, Extra::Length, {0x3009, 0x311d, 0x3227}, {0x2aa8, 0x2b50, 0x2bfc}}};
constexpr Wording kind_two_clubs[3][2] = {
    {{Names::Clubs, Extra::None, {0x3368, 0x344b, 0x3528}, {0x2f50, 0x2fd0, 0x3054}},
     {Names::Clubs, Extra::Length, {0x363c, 0x3736, 0x3826}, {0x30d8, 0x315c, 0x31e4}}},
    {{Names::Clubs, Extra::None, {0x394d, 0x3a31, 0x3b0f}, {0x3270, 0x32f0, 0x3374}},
     {Names::Clubs, Extra::Length, {0x3c24, 0x3d1f, 0x3e10}, {0x33f8, 0x347c, 0x3504}}},
    {{Names::Clubs, Extra::None, {0x3f38, 0x401c, 0x40fa}, {0x358c, 0x360c, 0x3690}},
     {Names::Clubs, Extra::Length, {0x420f, 0x430a, 0x43fb}, {0x3714, 0x3798, 0x3820}}}};
constexpr Wording kind_three_competition[2] = {{Names::Competition, Extra::None, {0x4523, 0x4620, 0x4717}, {0x38dc, 0x3980, 0x3a28}},
                                               {Names::Competition, Extra::Length, {0x4845, 0x4959, 0x4a63}, {0x3ad0, 0x3b78, 0x3c24}}};
constexpr Wording kind_three_clubs[3][2] = {
    {{Names::Clubs, Extra::None, {0x4ba4, 0x4c87, 0x4d64}, {0x3f78, 0x3ff8, 0x407c}},
     {Names::Clubs, Extra::Length, {0x4e78, 0x4f72, 0x5062}, {0x4100, 0x4184, 0x420c}}},
    {{Names::Clubs, Extra::None, {0x5189, 0x526d, 0x534b}, {0x4298, 0x4318, 0x439c}},
     {Names::Clubs, Extra::Length, {0x5460, 0x555b, 0x564c}, {0x4420, 0x44a4, 0x452c}}},
    {{Names::Clubs, Extra::None, {0x5774, 0x5858, 0x5936}, {0x45b4, 0x4634, 0x46b8}},
     {Names::Clubs, Extra::Length, {0x5a4b, 0x5b46, 0x5c37}, {0x473c, 0x47c0, 0x4848}}}};
// A ban by a date (a length below 0): the end date is the date text. Kind 1 with a count of one names the clubs.
constexpr Wording dated_zero = {Names::None, Extra::Date, {0x5d5f, 0x5e17, 0x5ec5}, {0x489c, 0x48cc, 0x4900}};
constexpr Wording dated_one_clubs[3] = {{Names::Clubs, Extra::Date, {0x5faa, 0x60b9, 0x61be}, {0x49ac, 0x4a30, 0x4ab8}},
                                        {Names::Clubs, Extra::Date, {0x62fa, 0x640a, 0x6510}, {0x4b40, 0x4bc4, 0x4c4c}},
                                        {Names::Clubs, Extra::Date, {0x664d, 0x675d, 0x6863}, {0x4cd0, 0x4d54, 0x4ddc}}};
constexpr Wording dated_one_alone = {Names::None, Extra::Date, {0x69a0, 0x6a5e, 0x6b12}, {0x4e0c, 0x4e3c, 0x4e70}};
constexpr Wording dated_four = {Names::None, Extra::Date, {0x6bfd, 0x6cd1, 0x6d9b}, {0x4ea8, 0x4ed8, 0x4f0c}};
constexpr Wording dated_two[3] = {{Names::Clubs, Extra::Date, {0x6e9c, 0x6fa8, 0x70aa}, {0x4fb0, 0x5034, 0x50bc}},
                                  {Names::Clubs, Extra::Date, {0x71e3, 0x72f0, 0x73f3}, {0x5144, 0x51c8, 0x5250}},
                                  {Names::Clubs, Extra::Date, {0x752d, 0x763a, 0x773d}, {0x52d4, 0x5358, 0x53e0}}};
constexpr Wording dated_three[3] = {{Names::Clubs, Extra::Date, {0x7877, 0x797c, 0x7a77}, {0x5484, 0x5508, 0x5590}},
                                    {Names::Clubs, Extra::Date, {0x7ba9, 0x7caf, 0x7dab}, {0x5618, 0x569c, 0x5724}},
                                    {Names::Clubs, Extra::Date, {0x7ede, 0x7fe4, 0x80e0}, {0x57a8, 0x582c, 0x58b8}}};
}

// The remark on the result of the fixture (a text of the pool by the clubs' scores), formatted into one of the four
// buffers at::remark: the format, and the return address of the call for each buffer (offsets in the module's page).
struct Remark {
    bool names_first_club;
    std::uint16_t format;
    std::uint16_t back[4];
};
constexpr Remark remarks[11] = {
    {true, 0x33e, {0x111c, 0x19e4, 0x2c54, 0x3c7c}},  // the first club leads by 6 or more
    {true, 0x394, {0x115c, 0x1a24, 0x2c94, 0x3cbc}},  // by 4 or 5
    {true, 0x3e7, {0x11a8, 0x1a70, 0x2ce0, 0x3d08}},  // ahead with more than 5 goals scored in all
    {true, 0x43a, {0x11f0, 0x1ab8, 0x2d28, 0x3d50}},  // ahead
    {false, 0x486, {0x1220, 0x1ae8, 0x2d58, 0x3d80}}, // level with more than 5 scored in all
    {false, 0x4c6, {0x124c, 0x1b14, 0x2d84, 0x3dac}}, // level, scoring
    {false, 0x4fc, {0x1270, 0x1b38, 0x2da8, 0x3dd0}}, // level, no score
    {true, 0x537, {0x12b0, 0x1b78, 0x2de8, 0x3e10}},  // behind by 6 or more
    {true, 0x5af, {0x12f0, 0x1bb8, 0x2e28, 0x3e50}},  // by 4 or 5
    {true, 0x600, {0x1334, 0x1bfc, 0x2e6c, 0x3e94}},  // behind, more than 5 goals scored in all
    {true, 0x65e, {0x136c, 0x1c34, 0x2ea4, 0x3ecc}},  // behind
};
// Which remark fits the scores `a` (the first club's) and `b` (the second's), signed bytes read as ints.
const Remark& remark_for(int a, int b) {
    const int lead = a - b, total = a + b;
    if (lead > 5) return remarks[0];
    if (lead > 3) return remarks[1];
    if (b < a) return total > 5 ? remarks[2] : remarks[3];
    if (a != 0 && a == b) return total > 5 ? remarks[4] : remarks[5];
    if (a == b) return remarks[6];
    if (lead < -5) return remarks[7];
    if (lead < -3) return remarks[8];
    return total < 6 ? remarks[10] : remarks[9];
}

// The values of a varargs format: the first six go in r5-r10, the rest on the stack from sp+0x38.
class Arguments {
public:
    Arguments(std::initializer_list<std::uint32_t> list = {}) { add(list); }
    Arguments& add(std::initializer_list<std::uint32_t> list) {
        for (const auto value : list) values_[count_++] = value;
        return *this;
    }
    unsigned size() const { return count_; }
    std::uint32_t operator[](unsigned at) const { return values_[at]; }

private:
    std::array<std::uint32_t, 16> values_{};
    unsigned count_ = 0;
};

std::uint32_t widen(std::int32_t byte_or_half) { return std::uint32_t(byte_or_half); }

class BanStory {
public:
    BanStory(guest::Cpu& c, const guest::Frame& frame, std::uint32_t owner, std::uint32_t message, std::uint32_t text,
             std::uint32_t flag_word)
        : c(c), frame(frame), owner(owner), message(message), out(text), flag_word(flag_word) {
        for (unsigned n = 13; n < 32; ++n) held[n] = c.r(n);
    }

    void tell(bool body) {
        read_message();
        if (body) body_text();
        else headline();
    }

private:
    guest::Cpu& c;
    const guest::Frame& frame;
    std::uint32_t owner, message, out, flag_word;
    // The message's fields (the order of the original's reads).
    std::int32_t length = 0, date = 0, kind = 0, count = 0, refuse_four = 0, flagged = 0;
    std::uint16_t end_day = 0, end_year = 0;
    bool has_fixture = false;
    bool names_competition = false;  // the fixture's competition is named in the text
    std::uint32_t matched = 0;       // 0x100cbde0's result: the person's name is a link

    // ---- small readers ----
    std::uint32_t pool(std::uint32_t offset) const { return c.toc(toc::pool) + offset; }
    std::uint32_t text_buffer() const { return c.toc(toc::buffer); }
    std::uint32_t clubs() const { return c.u32(c.toc(toc::clubs)); }
    std::uint32_t club_at(std::uint32_t index) const { return clubs() + index * row::club_size; }
    std::uint32_t club_gender(std::uint32_t index) const { return widen(c.i8(club_at(index) + row::club_gender)); }
    std::uint32_t club_name(std::uint32_t index) const { return club_at(index) + row::club_name; }
    std::uint32_t competition_at() const {
        return c.u32(c.toc(toc::competitions)) + c.u32(frame.at(at::fixture_competition)) * row::competition_size;
    }
    std::uint32_t person_at() const {
        return c.u32(c.toc(toc::people)) + c.u32(frame.at(at::person_index)) * row::person_size;
    }
    std::uint32_t club_one() const { return c.u32(frame.at(at::club_one)); }
    std::uint32_t club_two() const { return c.u32(frame.at(at::club_two)); }
    std::int32_t score_one() const { return c.i8(frame.at(at::score_one)); }
    std::int32_t score_two() const { return c.i8(frame.at(at::score_two)); }
    std::uint32_t bodies_text() const { return frame.at(at::bodies_text); }
    std::uint32_t name_text() const { return frame.at(at::name_text); }
    std::uint32_t score_text() const { return frame.at(at::score_text); }

    // The registers the original holds at each call it makes. Every callee saves r13-r31 (or the upper ones) in its frame,
    // and a link row of 0x1074bab0 (and the junk words of a varargs formatter) copies those stack bytes into the text
    // object, so the registers are the original's at each call, as they come into being: `hold` records what the original
    // loads or computes into one, and `registers` writes them all before a call (the reader the brief names; see
    // manager_manager_standing.cpp). The long formats' argument temporaries (r13, r14, r15, r17) are held the same way.
    std::array<std::uint32_t, 32> held{};
    void hold(unsigned n, std::uint32_t value) { held[n] = value; }
    void registers() {
        for (unsigned n = 13; n < 32; ++n) c.set(n, held[n]);
    }
    void call(std::uint32_t target, std::uint32_t back) {
        registers();
        c.call(target, back);
    }
    // The temporaries of a main text's call: by the competition rows (the competition's name pointer arguments are
    // addressed from r13, r14, r15) and by the long formats of the two clubs with an end date.
    void leaf_registers(const Wording& wording, int arm) {
        const auto competitions = c.u32(c.toc(toc::competitions));
        if (wording.names == Names::Competition && arm != 2) {
            hold(13, frame.at(at::bodies_text));
            hold(14, c.u32(frame.at(at::fixture_competition)) * row::competition_size);
            hold(15, competitions);
        } else if (wording.names == Names::Competition) {
            hold(13, frame.at(at::end_date_text));
            hold(14, frame.at(at::bodies_text));
            hold(15, club_gender(club_two()));
            hold(17, competitions);
        } else if (wording.names == Names::Clubs && wording.extra != Extra::None && arm == 2) {
            hold(13, frame.at(at::end_date_text));
        }
    }

    // ---- calls ----
    // The next integer of the message: the running index is a byte of the frame, advanced before the call.
    std::uint32_t next_field(std::uint32_t back) {
        const auto index = c.u8(frame.at(at::counter));
        c.put8(frame.at(at::counter), index + 1u);
        c.set({{3, message}, {4, index}});
        call(callee::read_field, back);
        return c.r(3);
    }
    void format(std::uint32_t buffer, std::uint32_t format_offset, const Arguments& values, std::uint32_t back,
                std::uint32_t target = callee::format) {
        c.set({{3, buffer}, {4, pool(format_offset)}});
        unsigned reg = 5, slot = 0x38;
        for (unsigned n = 0; n < values.size(); ++n) {
            if (reg <= 10) c.set(reg++, values[n]);
            else c.put32(frame.at(std::int32_t(slot)), values[n]), slot += 4;
        }
        call(target, back);
    }
    // 0x100e7490's text of the bodies into the frame buffer (the texts block's C++, which sets the generated routine's own
    // registers itself). Its frame saves this routine's registers as its caller's, and the original calls it with r3-r8.
    std::uint32_t describe_bodies(std::uint32_t argument) {
        registers();
        c.set({{3, owner}, {4, frame.at(at::bodies)}, {5, bodies_text()}, {6, 100}, {7, widen(refuse_four)}, {8, argument}});
        return describe_ban_bodies(c, owner, frame.at(at::bodies), bodies_text(), 100, std::int8_t(refuse_four), argument);
    }
    void text_call(std::uint32_t target, std::uint32_t first, std::uint32_t second, std::uint32_t back) {
        c.set({{3, first}, {4, second}});
        call(target, back);
    }
    void set_text(std::uint32_t source, std::uint32_t back) { text_call(callee::set_text, out, source, back); }
    // A row of the story's links: the name (or text) with its tag and the record it opens.
    void link(std::uint32_t name, std::uint32_t tag, std::uint32_t record, std::uint32_t back) {
        c.set({{3, out}, {4, name}, {5, tag}, {6, record}});
        call(callee::link, back);
    }
    void link_clubs(std::uint32_t one_back, std::uint32_t two_back) {
        link(club_name(club_one()), link_tag::club, club_at(club_one()), one_back);
        link(club_name(club_two()), link_tag::club, club_at(club_two()), two_back);
    }
    void format_person_name(std::uint32_t style, std::uint32_t back) {
        c.set({{3, c.u32(frame.at(at::person))}, {4, c.u32(frame.at(at::person + 4))}, {5, c.u32(frame.at(at::person + 8))},
               {6, name_text()}, {7, 200}, {8, style}, {9, c.u32(frame.at(at::person_index))}});
        call(callee::name_text, back);
    }

    // ---- the message ----
    // `hold` marks where the original loads a variable into its register: the callee-saved registers then hold what
    // the original's hold at each later call.
    void read_message() {
        hold(26, text_buffer());
        hold(27, c.toc(toc::competitions));
        hold(28, c.toc(toc::clubs));
        hold(29, c.toc(toc::pool));
        hold(31, owner);
        hold(30, message);
        hold(15, flag_word);
        hold(16, out);
        hold(13, frame.at(at::name_object));
        c.put8(frame.at(at::counter), 0);
        c.set(3, frame.at(at::name_object));
        call(callee::init_text, 0x100e0bb4);
        c.set(3, frame.at(at::fixture));
        call(callee::init_text, 0x100e0bbc);
        hold(20, 0);
        hold(18, 0);
        c.put8(frame.at(at::extra_flag), next_field(0x100e0bd8));
        next_field(0x100e0bf4);
        length = std::int8_t(next_field(0x100e0c08));
        hold(25, widen(length));
        date = std::int16_t(next_field(0x100e0c20));
        hold(24, widen(date));
        end_day = std::uint16_t(next_field(0x100e0c38));
        hold(14, end_day);
        end_year = std::uint16_t(next_field(0x100e0c50));
        hold(23, end_year);
        constexpr std::uint32_t body_backs[4] = {0x100e0c68, 0x100e0c80, 0x100e0c98, 0x100e0cb0};
        for (unsigned k = 0; k < 4; ++k) c.put8(frame.at(at::bodies + std::int32_t(k)), next_field(body_backs[k]));
        kind = std::int8_t(next_field(0x100e0cc8));
        hold(22, widen(kind));
        count = std::int8_t(next_field(0x100e0ce0));
        hold(21, widen(count));
        refuse_four = std::int8_t(next_field(0x100e0cf8));
        hold(17, widen(refuse_four));
        next_field(0x100e0d10);
        flagged = std::int8_t(next_field(0x100e0d24));
        hold(19, widen(flagged));
        // The person (three words and the table index) and, when the message has one, the fixture.
        c.put8(frame.at(at::counter), c.u8(frame.at(at::counter)) + 2u);
        c.set({{3, message}, {4, frame.at(at::counter)}, {5, frame.at(at::person)}});
        call(callee::decode_stats, 0x100e0d44);
        has_fixture = next_field(0x100e0d58) != 0;
        if (has_fixture) {
            hold(20, 1);
            c.set({{3, message}, {4, frame.at(at::counter)}, {5, frame.at(at::fixture)}});
            call(callee::decode_fixture, 0x100e0d74);
            const auto competition = c.u32(frame.at(at::fixture_competition));
            c.set({{3, c.u32(c.toc(toc::competition_kinds))}, {4, c.u32(c.toc(toc::competitions)) + competition * row::competition_size}});
            call(callee::competition_hosted, 0x100e0d90);
            names_competition = std::int8_t(c.r(3)) == 0;
            hold(18, names_competition ? 1 : 0);
            c.set(3, competition);
            call(callee::competition_cup, 0x100e0da4);
            // A cup competition's own kinds, or one with a group, are never named.
            if (std::int8_t(c.r(3)) != 0 && (c.i16(frame.at(at::fixture_kind)) == 0x418 || c.i16(frame.at(at::fixture_kind)) == 0x419 ||
                                             c.i16(frame.at(at::fixture_kind)) == 0xa0 || c.i16(frame.at(at::fixture_group)) != 0)) {
                names_competition = false;
                hold(18, 0);
            }
        }
        c.set({{3, person_at()}, {4, c.u32(frame.at(at::person))}, {5, c.u32(frame.at(at::person + 4))},
               {6, c.u32(frame.at(at::person + 8))}, {7, frame.at(at::name_object)}});
        call(callee::name_matches, 0x100e0e00);
        matched = c.r(3);
        hold(30, matched);
        if (has_fixture) {
            c.set({{3, c.u32(c.toc(toc::fixtures))}, {4, c.u32(frame.at(at::fixture_record))}, {5, club_one()},
                   {6, club_two()}, {7, c.u16(frame.at(at::fixture_day))}, {8, c.u16(frame.at(at::fixture_year))}});
            call(callee::fixture_link, 0x100e0e30);
            c.put8(frame.at(at::fixture_link), c.r(3));
        }
    }

    // ---- the headline ----
    void headline() {
        format_person_name(5, 0x100e0e5c);
        if (length > -1) {
            if (length == 1) format(text_buffer(), 0x81, {name_text()}, 0x100e0e7c);
            else format(text_buffer(), 0xba, {name_text(), widen(length)}, 0x100e0e94);
        } else {
            c.set({{3, widen(date)}, {4, frame.at(at::day_text)}, {5, 0x1e}, {6, 0}});
            call(callee::day_text, 0x100e0eac);
            format(text_buffer(), 0x106, {name_text(), frame.at(at::day_text)}, 0x100e0ec0);
        }
        set_text(text_buffer(), 0x100e0ecc);
        if (std::int8_t(matched) != 0) link(name_text(), link_tag::person, person_at(), 0x100e0ef8);
    }

    // ---- the body ----
    void body_text() {
        format_person_name(1, 0x100e0f1c);
        if (end_year != 0) {
            c.set({{3, frame.at(at::end_parts)}, {4, widen(std::int16_t(end_day))}, {5, end_year}});
            call(callee::date_pieces, 0x100e0f34);
            c.set({{3, frame.at(at::end_parts)}, {4, frame.at(at::end_date_text)}, {5, 100}, {6, 0xff}});
            call(callee::date_text, 0x100e0f48);
        }
        if (has_fixture) {
            // The result as "winner's goals, loser's goals" (0x150), the first club's goals first on a draw.
            const auto a = score_one(), b = score_two();
            format(score_text(), 0x150, {widen(std::max(a, b)), widen(std::min(a, b))}, a < b ? 0x100e0f90 : 0x100e0f78);
        }
        // The bodies' text (0x100e7490, the C++ block of this module).
        describe_bodies(4);
        if (c.i8(frame.at(at::bodies + 3)) == 0 && refuse_four != 0) two_clubs_story();
        else ban_story();
    }

    // ---- a ban with a named club pair and no fourth body: the main text names both clubs ----
    void two_clubs_story() {
        const auto one = club_one(), two = club_two();
        Arguments values;
        if (names_competition) {
            const auto competition = competition_at();
            const auto other = widen(c.i8(competition + row::competition_other_gender));
            values.add({other, other});
        }
        values.add({club_gender(one), club_gender(one), club_gender(two), club_gender(two), name_text(), widen(date)});
        if (names_competition) values.add({competition_at() + row::competition_name});
        values.add({club_name(one), club_name(two)});
        if (names_competition) {
            hold(13, clubs() + row::club_gender);
            hold(14, c.u32(c.toc(toc::competitions)));
        }
        format(frame.at(at::main_text), names_competition ? 0x17c : 0x26f, values, names_competition ? 0x100e1050 : 0x100e10b0);
        set_text(frame.at(at::main_text), 0x100e10bc);
        if (has_fixture && score_one() > -1) {
            score_remark(0);
            text_call(callee::append_text, out, frame.at(at::main_text), 0x100e1378);
            if (c.i8(frame.at(at::fixture_link)) != 0) link(score_text(), link_tag::fixture, c.u32(frame.at(at::fixture_record)), 0x100e1398);
        }
        if (std::int8_t(matched) != 0) link(name_text(), link_tag::person, person_at(), 0x100e13c4);
        link_clubs(0x100e13e4, 0x100e1404);
    }

    // ---- every other ban: the class of its first body, then the text of the case ----
    struct Choice {
        const Wording* wording = nullptr;
        int remark_slot = 0;  // 1-3: a competition row, followed by the remark of that kind concatenated to it
    };
    // The case of the story: by the ban's length (a date, one match, several), its kind, the count, whether the
    // competition is named and which club leads. The kinds 0-4 are all it knows: others format nothing.
    Choice choose() const {
        const auto a = score_one(), b = score_two();
        const unsigned relation = b < a ? 0 : a < b ? 1 : 2;
        using namespace wordings;
        if (length < 0) {
            switch (kind) {
                case 0: return {&dated_zero};
                case 1: return {count == 1 ? &dated_one_clubs[relation] : &dated_one_alone};
                case 4: return {&dated_four};
                case 2: return {&dated_two[relation]};
                case 3: return {&dated_three[relation]};
                default: return {};
            }
        }
        const unsigned several = length == 1 ? 0 : 1;
        switch (kind) {
            case 0: return {&kind_zero[several]};
            case 1:
                if (count != 1) return {&kind_one_alone[several]};
                return names_competition ? Choice{&kind_one_competition[several], 1} : Choice{&kind_one_clubs[relation][several]};
            case 4: return {&kind_four[several]};
            case 2: return names_competition ? Choice{&kind_two_competition[several], 2} : Choice{&kind_two_clubs[relation][several]};
            case 3: return names_competition ? Choice{&kind_three_competition[several], 3} : Choice{&kind_three_clubs[relation][several]};
            default: return {};
        }
    }
    // The values a case's formats read (see Wording): `bodies` is the class 0x100e7490 gave the first body.
    Arguments ban_values(const Wording& wording, int arm, std::uint32_t bodies) const {
        const auto one = club_one(), two = club_two();
        Arguments values{bodies, bodies};
        if (arm == 2) values.add({100});
        if (arm == 2 && wording.second_hundred) values.add({100});
        if (wording.names == Names::Competition) {
            const auto gender = widen(c.i8(competition_at() + row::competition_gender));
            values.add({gender, gender});
        }
        if (wording.names != Names::None)
            values.add({club_gender(one), club_gender(one), club_gender(two), club_gender(two)});
        values.add({name_text()});
        if (wording.extra == Extra::Length) values.add({widen(length)});
        values.add({bodies_text()});
        if (wording.extra == Extra::Date) values.add({widen(date)});
        if (arm == 2) values.add({frame.at(at::end_date_text)});
        switch (wording.names) {
            case Names::None: values.add({widen(count)}); break;
            case Names::Clubs: values.add({club_name(one), score_text(), club_name(two)}); break;
            case Names::Competition: values.add({competition_at() + row::competition_short, club_name(one), club_name(two)}); break;
        }
        return values;
    }
    void ban_story() {
        // The class of the first body, from the second call of the bodies' text: a ban of one match refuses a
        // fourth body differently from the others (5, else 6).
        const auto bodies = widen(std::int8_t(describe_bodies(length == 1 ? 5 : 6)));
        // A kind the story does not know leaves the main text as the frame had it (never written).
        if (const auto [wording, remark_slot] = choose(); wording) {
            const auto arm = flagged != 0 ? 0 : end_year == 0 ? 1 : 2;
            leaf_registers(*wording, arm);
            format(frame.at(at::main_text), wording->format[arm], ban_values(*wording, arm, bodies), module_page + wording->back[arm]);
            if (remark_slot != 0 && has_fixture) {
                score_remark(remark_slot);
                text_call(callee::concat, frame.at(at::main_text), frame.at(at::remark[remark_slot]),
                          remark_slot == 1 ? 0x100e1c40 : remark_slot == 2 ? 0x100e2eb0 : 0x100e3ed8);
            }
        }
        if (c.i8(frame.at(at::extra_flag)) == 0) {
            format(text_buffer(), 0x82b5, {frame.at(at::main_text)}, 0x100e5904, callee::plain_format);
        } else {
            format(frame.at(at::suffix_text), 0x8213, {}, 0x100e58d4);
            format(text_buffer(), 0x82ae, {frame.at(at::main_text), frame.at(at::suffix_text)}, 0x100e58ec, callee::plain_format);
        }
        set_text(text_buffer(), 0x100e5910);
        if (std::int8_t(matched) != 0) link(name_text(), link_tag::person, person_at(), 0x100e593c);
        link_clubs(0x100e595c, 0x100e597c);
        // The flag byte is only written when the message has a fixture: without one, the original reads the leftover
        // byte of its frame here.
        if (c.i8(frame.at(at::fixture_link)) != 0) link(score_text(), link_tag::fixture, c.u32(frame.at(at::fixture_record)), 0x100e599c);
    }

    // The remark on the fixture's result into one of the remark buffers.
    void score_remark(int slot) {
        const auto& remark = remark_for(score_one(), score_two());
        Arguments values;
        if (remark.names_first_club) values.add({club_gender(club_one()), club_gender(club_one()), club_name(club_one())});
        values.add({score_text()});
        // The registers the compiler leaves around the call, which the formatter saves: the named remarks have the clubs
        // table in r9, the others the scores of the compare chain in r6-r9 (the main text's chain, by raw byte and
        // sign-extended, then the other chains' order).
        const auto a_raw = c.u8(frame.at(at::score_one)), b_raw = c.u8(frame.at(at::score_two));
        const auto a = widen(score_one()), b = widen(score_two());
        if (remark.names_first_club) values.add({clubs()});
        else if (slot == 0) values.add({a_raw, a - b, b_raw, b});
        else values.add({b_raw, a_raw, b, a});
        format(frame.at(at::remark[slot]), remark.format, values, module_page + remark.back[slot]);
    }
};
}

void ban_news_text(guest::Cpu& c, std::uint32_t owner, std::uint32_t message, std::int8_t body, std::uint32_t text) {
    const auto flag_word = c.r(5);
    guest::Frame frame(c, 0x100e0b70, 0x3970, {.first_gpr = 13});
    BanStory(c, frame, owner, message, text, flag_word).tell(body != 0);
}
}

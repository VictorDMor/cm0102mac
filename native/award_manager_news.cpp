#include "award_manager_detail.hpp"

#include <array>

namespace cm::aot::award_manager {
using namespace detail;
namespace {
constexpr auto format_text = 0x10727890u;
constexpr auto read_parameter = 0x102cb570u;
constexpr auto put_parameter = 0x102cd0c0u;

struct AwardNews {
    Cpu& c;
    const Frame& frame;
    std::uint32_t message, output, text, source, people, clubs;
    std::uint32_t parameter(unsigned index, std::uint32_t back) {
        return call(c, read_parameter, back, {message, index});
    }
    void unavailable(std::uint32_t label, std::uint32_t first, std::uint32_t second) {
        c.set(6, label);
        call(c, 0x1074bab0, first, {output, label + 0x16, 2004});
        c.set(6, label);
        call(c, 0x1074bab0, second, {output, label + 0x30, 2004});
    }
    std::uint32_t player(std::uint32_t id) const { return c.u32(people) + id * 0x70; }
};

bool find_nominee(AwardNews& news, std::uint32_t award, std::uint32_t shortlist,
                  std::uint32_t day, std::uint32_t year, std::uint32_t& club_id) {
    auto& c = news.c;
    c.set(22, shortlist);
    if (compare_logical(c, shortlist) == 0) {
        news.unavailable(c.u32(award + 8), 0x104375bc, 0x104375d0);
        return false;
    }
    const auto date = call(c, 0x107156b0, 0x10437434, {news.frame.at(0x40), extended(std::int16_t(day)), year});
    const auto copy = news.frame.at(0x38);
    c.put32(copy, c.u32(date));
    c.set(5, c.u32(date + 4));
    c.put32(copy + 4, c.r(5));
    c.set(3, c.u16(copy + 2));
    bool due = compare_logical(c, c.r(3), c.u16(shortlist + 0x42)) == 0;
    if (due) {
        c.set(3, extended(c.i16(copy)));
        due = compare(c, c.r(3), c.i16(shortlist + 0x40)) == 0;
    }
    c.set(4, due);
    if (compare(c, due) == 0) {
        news.unavailable(c.u32(award + 8), 0x104375bc, 0x104375d0);
        return false;
    }
    const auto managers = c.r(26), user_index = managers + 0x2c58, users = managers + 4;
    keep(c, {{21, user_index}, {26, users}});
    const auto voter_slot = users + std::uint32_t(c.i16(user_index)) * 0x2c0;
    const auto ballot = std::int16_t(call(c, 0x10439c10, 0x10437490, {shortlist, c.u32(voter_slot)}));
    if (compare(c, ballot) < 0) {
        news.unavailable(c.u32(award + 8), 0x10437590, 0x104375a4);
        return false;
    }
    const auto candidates = c.u32(shortlist);
    const auto nominee = candidates + std::uint32_t(ballot) * 16;
    const auto player = news.player(c.u32(nominee));
    club_id = c.u32(nominee + 4);
    c.set(19, club_id);
    c.set(5, candidates);
    call(c, 0x1030d6e0, 0x104374c4, {player, news.frame.at(0x48)});
    const auto voter = c.u32(voter_slot);
    c.set(20, voter);
    constexpr std::array returns{0x104374ecu, 0x10437508u, 0x10437524u, 0x10437540u, 0x1043755cu, 0x10437578u};
    constexpr std::array fields{0u, 0u, 0x54u, 0x48u, 0x4cu, 0x50u};
    for (unsigned i = 0; i < returns.size(); ++i) {
        const auto value = i == 0 ? 1 : i == 1 ? club_id : c.u32(news.frame.at(fields[i]));
        call(c, put_parameter, returns[i], {c.u32(c.r(24)), voter, c.u32(news.message + 0xdc), i + 4, value, 0});
    }
    return true;
}
}

void format_award_news(Cpu& c, std::uint32_t manager, std::uint32_t message,
                       std::uint32_t output, std::uint32_t show_detail) {
    Frame frame(c, 0x10437310, 0x1d0, {.first_gpr = 18});
    AwardNews news{c, frame, message, output, c.toc(-0x7bb8), c.toc(-0x4a84),
                   c.toc(-0x7be0), c.toc(-0x7be8)};
    keep(c, {{18, manager}, {23, news.clubs}, {24, c.toc(-0x7bf0)}, {25, news.people},
             {26, c.toc(-0x7bb0)}, {27, news.text}, {28, news.source},
             {29, message}, {30, output}, {31, show_detail}});
    call(c, 0x10715880, 0x10437350, {frame.at(0x58)});
    const auto group_index = news.parameter(0, 0x1043735c);
    c.set(22, group_index);
    const auto award_index = news.parameter(1, 0x1043736c);
    c.set(20, award_index);
    const auto day = std::uint16_t(news.parameter(2, 0x1043737c));
    c.set(21, day);
    const auto year = std::uint16_t(news.parameter(3, 0x1043738c));
    c.set(19, year);
    const auto published = std::int8_t(news.parameter(4, 0x1043739c));
    const auto groups = Manager{c, manager}.groups();
    const auto group = c.u32(groups + 4 * group_index);
    const auto awards = c.u32(group + 4);
    const auto award = c.u32(awards + 4 * award_index);
    const bool detailed = compare(c, std::int8_t(show_detail)) != 0;
    c.set(22, std::uint32_t(published));
    c.set(20, award);
    const auto label = c.u32(award + 8);
    const auto length = std::uint32_t(c.i8(label + (detailed ? 0x64 : 0x63)));
    call(c, format_text, detailed ? 0x10437404 : 0x104373e0,
         {news.text, news.source + (detailed ? 0xda : 0x9a), length, length, label + (detailed ? 0x30 : 0x16)});
    call(c, 0x1074b830, 0x10437410, {output, news.text});

    std::uint32_t club_id;
    if (compare(c, published) == 0) {
        if (!find_nominee(news, award, c.u32(award + 0x24), day, year, club_id)) return;
    } else {
        club_id = news.parameter(5, 0x104375e0);
        c.set(19, club_id);
        constexpr std::array fields{0x54u, 0x48u, 0x4cu, 0x50u};
        constexpr std::array returns{0x104375f4u, 0x10437604u, 0x10437614u, 0x10437624u};
        for (unsigned i = 0; i < fields.size(); ++i)
            c.put32(frame.at(fields[i]), news.parameter(i + 6, returns[i]));
    }
    if (compare(c, std::int8_t(show_detail)) == 0) return;
    const auto name = frame.at(0x60);
    const auto identity = c.u32(frame.at(0x54));
    const auto surname = c.u32(frame.at(0x48)), given = c.u32(frame.at(0x4c)), nationality = c.u32(frame.at(0x50));
    call(c, 0x100cb590, 0x10437650, {surname, given, nationality, name, 300, 1, identity});
    if (compare(c, club_id, no_id) == 0) {
        call(c, format_text, 0x104376a0, {news.text, news.source + 0x1a4, name});
    } else {
        const auto club = c.u32(news.clubs) + club_id * 0x248;
        const auto width = std::uint32_t(c.i8(club + 0x23b));
        call(c, format_text, 0x1043768c, {news.text, news.source + 0x147, width, width, 100, club + 0x221, name});
    }
    call(c, 0x1074b960, 0x104376ac, {output, news.text});
    const auto player = news.player(c.u32(frame.at(0x54)));
    const auto valid = call(c, 0x100cbde0, 0x104376d0, {player, c.u32(frame.at(0x48)),
        c.u32(frame.at(0x4c)), c.u32(frame.at(0x50)), 0});
    if (compare(c, std::int8_t(valid)) != 0)
        call(c, 0x1074bab0, 0x104376f8, {output, name, 2007, news.player(c.u32(frame.at(0x54)))});
    if (compare(c, club_id, no_id) != 0) {
        const auto clubs = c.u32(news.clubs);
        const auto club = clubs + club_id * 0x248;
        c.set(6, club);
        call(c, 0x1074bab0, 0x1043771c, {output, club + 0x221, 2005});
    }
}
}

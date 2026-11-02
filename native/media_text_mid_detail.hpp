#pragma once
// What the press's text generators share: the log record they read (field by field through
// 0x102cb570), clubs and staff in the world's tables, and the pieces every story ends with
// (the text object that takes the finished text, and the links that make names clickable).
#include "media_detail.hpp"
#include "media_text_shared.hpp"
#include <optional>
#include <span>

namespace cm::aot::media::detail::text {
namespace entry {
constexpr std::uint32_t person_name = 0x100cb590, staff_named = 0x100cbde0,
    player_name = 0x10168a00, to_amount = 0x10009f20, format_amount = 0x1000b410, set_text = 0x1074b830,
    append_text = 0x1074b960, add_link = 0x1074bab0;
}
constexpr int staff_slot = -0x7be0;
// Link kinds of 0x1074bab0.
namespace link_kind {
constexpr std::uint32_t club = 0x7d5, competition = 0x7d0, staff = 0x7d7;
}
constexpr std::uint32_t staff_size = 0x70;
constexpr std::uint32_t decode_match = 0x1030dee0, today = 0x10715880;
constexpr std::uint32_t any = 0xffffffff;

inline std::uint32_t table(Cpu& c, int slot) { return c.u32(c.toc(slot)); }

// A club record: the long and short names with the byte next to each (what the phrase's {} markers read).
struct Club {
    Cpu& c;
    std::uint32_t at;
    std::uint32_t long_name() const { return at + 0x1ee; }
    std::uint32_t short_name() const { return at + 0x221; }
    std::uint32_t long_marker() const { return signed_byte(c.u8(at + 0x242)); }
    std::uint32_t short_marker() const { return signed_byte(c.u8(at + 0x23b)); }
};
inline Club club_at(Cpu& c, std::uint32_t id) { return {c, table(c, -0x7be8) + id * club_size}; }
// A competition record: its name, with the byte next to it.
struct Competition {
    Cpu& c;
    std::uint32_t at;
    std::uint32_t name() const { return at + 0x4d; }
    std::uint32_t marker() const { return signed_byte(c.u8(at + 0x67)); }
};
inline Competition competition_at(Cpu& c, std::uint32_t id) { return {c, table(c, -0x7bbc) + id * competition_size}; }
inline std::uint32_t staff_at(Cpu& c, std::uint32_t id) { return c.u32(c.toc(staff_slot)) + id * staff_size; }

// The formatter's arguments past the sixth go to the frame's outgoing words. Some routines store
// them before they load the register arguments, so a wild pointer in those loads faults after the
// stores: write them as each is evaluated (`<<` evaluates left to right), then call `format`.
struct StackWords {
    Cpu& c;
    unsigned next = 0;
    StackWords& operator<<(std::uint32_t word) {
        c.put32(c.sp() + 0x38 + 4 * next++, word);
        return *this;
    }
};

// The compare chain every story switch makes on its variant (0-5), as the original's compares leave
// the condition register: 3, then 5 or 1 and 0. -1 is a variant outside 0-5.
inline int variant_of(Cpu& c, std::int32_t variant) {
    const auto against_three = compare(c, variant, 3);
    if (against_three == 0) return 3;
    if (against_three > 0) {
        const auto against_five = compare(c, variant, 5);
        if (against_five == 0) return 5;
        return against_five > 0 ? -1 : 4;
    }
    const auto against_one = compare(c, variant, 1);
    if (against_one == 0) return 1;
    if (against_one > 0) return 2;
    return compare(c, variant, 0) >= 0 ? 0 : -1;
}

// The match 1030dee0 decodes into the frame.
struct DecodedMatch {
    Cpu& c;
    std::uint32_t at;
    std::int16_t day() const { return c.i16(at); }
    std::uint16_t year() const { return c.u16(at + 2); }
    std::uint32_t competition() const { return c.u32(at + 8); }
    std::uint32_t first_club() const { return c.u32(at + 0x14); }
    std::uint32_t second_club() const { return c.u32(at + 0x18); }
    std::int8_t first_goals() const { return std::int8_t(c.u8(at + 0x1c)); }
    std::int8_t second_goals() const { return std::int8_t(c.u8(at + 0x1d)); }
    std::uint32_t id() const { return c.u32(at + 0x20); }
};

// The compare chain on a reply code: 2, then 4 or 1. 1, 2 and 3 are replies; any other code is none (0).
inline int reply_of(Cpu& c, std::int32_t code) {
    const auto against_two = compare(c, code, 2);
    if (against_two == 0) return 2;
    if (against_two > 0) return compare(c, code, 4) >= 0 ? 0 : 3;
    return compare(c, code, 1) >= 0 ? 1 : 0;
}

// The outlet of a story whose club and kind are known (see text_shared.hpp). `special_back` is the address
// after the call to the region's outlet routine; the wordings' code follows it.
struct StoryOutlet {
    std::uint32_t special_back, text, nationality, marker;
    bool clears_marker = false;  // some routines zero the marker after the region's outlet call (the rest of their calls sit 8 bytes later)
    bool compact = false;        // a routine that keeps the club in a register has shorter blocks
};
constexpr int user_slot = -0x7bb0;
// Whether the user's club and this club are in the same nation: the user's manager record (at +4 of his
// row in the table at the user slot) has a club whose nation matches, or else the club's own nation is
// the one the manager is in. Most routines leave r5 holding the row's offset when a manager exists.
inline bool shares_nation(Cpu& c, std::uint32_t club, bool leaves_r5) {
    const auto base = c.toc(user_slot);
    const auto row = base + std::uint32_t(std::int32_t(c.i16(base + 0x2c58))) * 0x2c0;
    const auto manager = c.u32(row + 4);
    if (manager == 0) return false;
    if (leaves_r5) c.set(5, std::uint32_t(std::int32_t(c.i16(base + 0x2c58))) * 0x2c0);
    if (const auto manager_club = c.u32(manager + 0x24); manager_club != 0) return c.u32(manager_club + 0x2c) == c.u32(club + 0x2c);
    return c.u32(club + 0x2c) == c.u32(manager + 0xc);
}
inline void describe_outlet(Cpu& c, const StoryOutlet& site, std::uint32_t kind, std::uint32_t club_id, std::uint32_t region) {
    const auto club = table(c, -0x7be8) + club_id * club_size;
    const bool same_nation = shares_nation(c, club, !site.compact);
    // A region and a kind both known: the outlet comes from the region's own table.
    if (compare(c, std::int32_t(club_id), -1) != 0 && compare(c, std::int32_t(kind), -1) != 0 && compare(c, std::int32_t(region), -1) != 0 &&
        compare_logical(c, c.u32(club + 0x2c)) != 0 && compare(c, std::int8_t(same_nation)) != 0) {
        outlet_from_region(c, kind, region, site.text, site.marker, site.special_back);
        if (site.clears_marker) c.put8(site.marker, 0);
        return;
    }
    write_outlet(c, {site.special_back + (site.clears_marker ? 8 : 0), site.text, site.nationality, site.marker, site.compact}, std::int32_t(kind), same_nation, club);
}

// Two more compare chains on small codes. Three-way: 1, then 3 or 0 (0, 1, 2 are the stories; -1 none).
inline int story_of(Cpu& c, std::int32_t code) {
    const auto against_one = compare(c, code, 1);
    if (against_one == 0) return 1;
    if (against_one > 0) return compare(c, code, 3) >= 0 ? -1 : 2;
    return compare(c, code, 0) >= 0 ? 0 : -1;
}
// 1, then 0: the code is 0, 1, or anything else (2).
inline int zero_one_or_other(Cpu& c, std::int32_t code) {
    const auto against_one = compare(c, code, 1);
    if (against_one == 0) return 1;
    if (against_one > 0) return 2;
    return compare(c, code, 0) >= 0 ? 0 : 2;
}

// What a story ends with: the text goes to the output object, then links for the names in it.
struct Output {
    Cpu& c;
    std::uint32_t object;
    void set(std::uint32_t text, std::uint32_t back) const { Program{c}.call(entry::set_text, back, {object, text}); }
    void append(std::uint32_t text, std::uint32_t back) const { Program{c}.call(entry::append_text, back, {object, text}); }
    // `leftover` is what the original's address arithmetic leaves in r7 (the table's base), when it does.
    void link(std::uint32_t token, std::uint32_t kind, std::uint32_t target, std::uint32_t back,
              std::optional<std::uint32_t> leftover = {}) const {
        if (leftover) c.set(7, *leftover);
        Program{c}.call(entry::add_link, back, {object, token, kind, target});
    }
    // Whether the three name parts are the staff member's own.
    bool named(std::uint32_t member, std::uint32_t first, std::uint32_t middle, std::uint32_t last, std::uint32_t back) const {
        return compare(c, std::int8_t(Program{c}.call(entry::staff_named, back, {member, first, middle, last, 0}))) != 0;
    }
    // The staff member behind a three-part name, when there is one, gets a link too.
    void staff_link(std::uint32_t member, std::uint32_t first, std::uint32_t middle, std::uint32_t last, std::uint32_t token,
                    std::uint32_t named_back, std::uint32_t link_back, std::uint32_t kind = link_kind::staff) const {
        if (named(member, first, middle, last, named_back)) link(token, kind, member, link_back);
    }
};
}

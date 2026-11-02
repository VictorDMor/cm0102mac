#pragma once
// What the press's text routines have in common (the blocks text_mid, text_a, text_b, text_c and text_d):
// reading a story record, naming the outlet that carries a story, and the four closing sentences a level
// match story can end with. Each routine keeps its own frame offsets and return addresses; they are
// arguments here.
#include "media_detail.hpp"
#include <span>

namespace cm::aot::media::detail::text {
namespace entry {
constexpr std::uint32_t read_field = 0x102cb570, append_string = 0x1077dc60;
}
inline std::uint32_t signed_byte(std::uint32_t value) { return extended(std::int16_t(std::int8_t(value))); }

// The story record being read, field after field through 0x102cb570. The routines count the fields in
// different ways, and each way leaves its own trace in the registers, so a reader has four ways to read:
//  - `read(index)`: the field by number, the call sites a fixed `step` apart from `back` on;
//  - `read.counted(back, advance)`: the position is what register `cursor` holds, which moves to the next
//    field (or stays, when `advance` is false: the read only peeks);
//  - `read.next()` / `read.at(back, advance)`: the reader counts itself and sets register `cursor` to the
//    position after the field, which is a step ahead of the field being read; the call sites are `backs`
//    in turn, or given.
struct RecordReader {
    Cpu& c;
    std::uint32_t record;
    unsigned cursor = 0;
    std::uint32_t back = 0, step = 0x14;
    std::span<const std::uint32_t> backs = {};
    std::uint32_t position = 0;

    std::uint32_t field(std::uint32_t index, std::uint32_t at) const {
        return Program{c}.call(entry::read_field, at, {record, index});
    }
    std::uint32_t operator()(std::uint32_t index) { return (*this)(index, step); }
    std::uint32_t operator()(std::uint32_t index, std::uint32_t distance) {
        const auto value = field(index, back);
        back += distance;
        return value;
    }
    std::uint32_t counted(std::uint32_t at, bool advance = true) const {
        const auto index = c.r(cursor);
        if (advance) c.set(cursor, index + 1);
        return field(index, at);
    }
    std::uint32_t at(std::uint32_t where, bool advance = true) {
        c.set(cursor, advance ? position + 1 : position);
        return field(position++, where);
    }
    std::uint32_t next(bool advance = true) { return at(backs[position], advance); }
};

// The four sentences that close a match story ended level or by outcome 1-4 (the compare ladder is
// 1, 2, 3, 4 in turn). The sentence is formatted into `destination` and appended to `target`; the call
// sites of the four sentences sit 0x5c apart from `first_back`, the append 0xc after its sentence.
// The words are worked out only for the outcome that matches. Returns whether one did.
inline constexpr std::uint32_t match_endings[4]{0x3118, 0x316c, 0x31b6, 0x3202};
template <class Words>
bool match_ending(Cpu& c, std::int32_t outcome, std::uint32_t first_back, std::uint32_t destination, std::uint32_t target, Words&& words) {
    for (unsigned n = 0; n < 4; ++n) {
        if (compare(c, outcome, std::int32_t(n + 1)) != 0) continue;
        const auto back = first_back + 0x5c * n;
        const auto w = words();
        format(c, destination, match_endings[n], back, {w[0], w[1], w[2], w[3]});
        Program{c}.call(entry::append_string, back + 0xc, {target, destination});
        return true;
    }
    return false;
}

// The press outlet that carries a story (a national or local newspaper or radio station, a magazine, a
// sports paper or a website), written to a text buffer with a marker byte for the article that goes before
// it. Every story that names an outlet makes this choice the same way: when the user's region has an
// outlet of its own for the kind it speaks (`outlet_from_region`); else the kind's wording, plain for a
// story told at home or with the club's nationality adjective in front.
constexpr std::uint32_t outlet_for_region = 0x106a9f10, game_day_kind = 0x10727880, nationality_adjective = 0x10168a00;
constexpr int owner_slot = -0x7570;
struct OutletWording { std::uint32_t plain, with_nation, nation_code; };
// By the position in the compare chain on the kind: kinds 1-6 in order, then any other kind.
inline constexpr OutletWording outlet_wordings[7]{
    {0xcd1, 0xce4, 0x16}, {0xd19, 0xd29, 0x17}, {0xd5b, 0xd72, 0x18}, {0xda2, 0xdb6, 0x18},
    {0xdec, 0xdf5, 0x19}, {0xe20, 0xe2d, 0x1a}, {0xe5c, 0xe64, 0x1a}};
constexpr unsigned magazine = 4;

// The return addresses of a site, at fixed distances from the start of its outlet code. A routine that
// keeps the club in a register has shorter blocks (`compact`).
struct OutletSite {
    std::uint32_t base;         // where the wordings' blocks count from
    std::uint32_t text, nationality, marker;  // the outlet's text, the adjective's buffer, the article marker byte
    bool compact = false;
};
struct OutletBlocks { std::uint32_t plain[7], adjective_after, format_after, game_day; };
inline constexpr OutletBlocks standard_blocks{{0x4c, 0xa4, 0xfc, 0x154, 0x1ac, 0x21c, 0x274}, 0x24, 0x38, 0x1e8};
inline constexpr OutletBlocks compact_blocks{{0x4c, 0x98, 0xe4, 0x130, 0x17c, 0x1e0, 0x22c}, 0x18, 0x2c, 0x1ac};

// The call to the region's outlet routine.
inline void outlet_from_region(Cpu& c, std::uint32_t kind, std::uint32_t region, std::uint32_t text, std::uint32_t marker, std::uint32_t back) {
    c.set({{3, c.u32(c.toc(owner_slot))}, {4, signed_byte(kind)}, {5, extended(std::int16_t(region))}, {6, text}, {7, marker}});
    media::call(c, outlet_for_region, back);
}

// The position of a kind in the wordings: the compare chain 4, then 6 or 2 and 1.
inline unsigned outlet_wording_of(Cpu& c, std::int32_t kind) {
    if (const auto against_four = compare(c, kind, 4); against_four == 0) return 3;
    else if (against_four > 0) {
        const auto against_six = compare(c, kind, 6);
        return against_six == 0 ? 5 : against_six > 0 ? 6 : 4;
    }
    if (const auto against_two = compare(c, kind, 2); against_two == 0) return 1;
    else if (against_two > 0) return 2;
    return compare(c, kind, 1) >= 0 ? 0 : 6;
}

// The outlet's wording for `kind`: plain when `at_home` (a byte), else in the nationality of `club`. A
// magazine takes the vowel article when the day's kind (0x10727880) is 8. Returns the wording's position.
inline unsigned write_outlet(Cpu& c, const OutletSite& site, std::int32_t kind, std::uint32_t at_home, std::uint32_t club) {
    const auto& blocks = site.compact ? compact_blocks : standard_blocks;
    const auto which = outlet_wording_of(c, kind);
    const auto& wording = outlet_wordings[which];
    const auto plain_back = site.base + blocks.plain[which];
    if (compare(c, std::int8_t(at_home)) != 0) {
        format(c, site.text, wording.plain, plain_back);
    } else {
        Program{c}.call(nationality_adjective, plain_back + blocks.adjective_after, {c.u32(club + 0x2c), site.nationality, 0x78, wording.nation_code});
        format(c, site.text, wording.with_nation, plain_back + blocks.format_after, {0x64, site.nationality});
    }
    if (which == magazine) {
        c.call(game_day_kind, site.base + blocks.game_day);
        const auto answer = signed_byte(c.r(3));
        c.set(3, answer);
        c.put8(site.marker, answer == 8 ? 1 : 0);
    } else c.put8(site.marker, 0);
    return which;
}
}

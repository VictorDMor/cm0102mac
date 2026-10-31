#include "awol_detail.hpp"
#include <array>

namespace cm::aot::awol {
using namespace detail;
namespace {
// The game's message and text layers.
inline constexpr std::uint32_t read_field = 0x102cb570, format_name = 0x100cb590, format_text = 0x10727890,
                               append_text = 0x1074b830, name_matches = 0x100cbde0, append_link = 0x1074bab0;
inline constexpr std::uint32_t name_length = 0x190;
// Offsets inside a nation record (0x128 bytes) and a club record.
inline constexpr std::uint32_t nation_size = 0x128, nation_name = 0xe1, nation_gender = 0x116;
inline constexpr std::uint32_t club_gender = 0x23b, club_name = 0x221, club_link_name = 0x1ee;
// The kinds of link the text carries.
inline constexpr std::uint32_t staff_link = 0x7da, club_link = 0x7d5;
// Style codes of the name formatter.
inline constexpr std::uint32_t full_style = 1, brief_style = 5;

// A staff member's id and the three parts of the name.
struct Person {
    std::uint32_t id, first, middle, last;
};

// What the three renderers share: the text they build, where it goes, and
// the tables the original keeps in non-volatile registers.
struct Page {
    Cpu& c;
    std::uint32_t text, output, staffs, clubs, pool;

    std::uint32_t club(std::uint32_t id) const { return c.u32(clubs) + id * club_size; }
    std::uint32_t gender_of(std::uint32_t record, std::uint32_t offset) const { return signed_byte(c.u8(record + offset)); }
    // sprintf(text, pool + token, arguments...): r5 on follow the order given.
    void say(std::uint32_t back, std::uint32_t token, std::initializer_list<std::uint32_t> arguments) const { say_into(text, back, token, arguments); }
    void say_into(std::uint32_t destination, std::uint32_t back, std::uint32_t token, std::initializer_list<std::uint32_t> arguments) const {
        c.set(3, destination);
        c.set(4, pool + token);
        unsigned n = 5;
        for (const auto value : arguments) c.set(n++, value);
        c.call(format_text, back);
    }
    void write_name(std::uint32_t back, const Person& who, std::uint32_t buffer, std::uint32_t style) const {
        call(c, format_name, back, {who.first, who.middle, who.last, buffer, name_length, style, who.id});
    }
    void append(std::uint32_t back) const { call(c, append_text, back, {output, text}); }
    // Adds the staff member's link when the name matches. The original keeps
    // the table offset in a non-volatile register of its own.
    void link_staff(unsigned offset_register, const Person& who, std::uint32_t buffer, std::uint32_t match_back, std::uint32_t link_back) const {
        const auto offset = who.id * staff_size;
        c.set(offset_register, offset);
        const auto found = call(c, name_matches, match_back, {c.u32(staffs) + offset, who.first, who.middle, who.last, 0});
        if (compare(c, std::int8_t(found)) == 0) return;
        call(c, append_link, link_back, {output, buffer, staff_link, c.u32(staffs) + offset});
    }
    void link_club(std::uint32_t back, std::uint32_t id, std::uint32_t name_offset) const {
        const auto record = club(id);
        call(c, append_link, back, {output, record + name_offset, club_link, record});
    }
};

// One field of the message, kept in the non-volatile register `into` (0:
// the original drops it).
std::uint32_t read(Cpu& c, std::uint32_t message, std::uint32_t field, std::uint32_t back, unsigned into) {
    const auto value = call(c, read_field, back, {message, field});
    if (into) c.set(into, value);
    return value;
}

// Whether the club is the one the session's current desk manages: the desk
// is chosen by a signed halfword, and a missing desk or club answers no.
bool manages_club(Cpu& c, std::uint32_t sessions, std::uint32_t club) {
    const auto desk = sessions + std::uint32_t(c.i16(sessions + 0x2c58)) * 0x2c0;
    c.set(3, c.u32(desk + 4));
    bool own = false;
    if (compare_logical(c, c.r(3)) != 0) {
        c.set(3, c.u32(c.r(3) + 0x24));
        if (compare_logical(c, c.r(3)) != 0) own = c.u32(c.r(3)) == club;
    }
    compare(c, std::int8_t(own));
    return own;
}

// The kinds the primary and brief texts distinguish; everything else adds
// no sentence. Both trees test 2, then 4 above it, 0 and 1 below it.
enum class Kind { zero, one, two, four, other };
Kind classify(Cpu& c, std::int32_t kind) {
    const auto order = compare(c, kind, 2);
    if (order == 0) return Kind::two;
    if (order > 0) return compare(c, kind, 4) == 0 ? Kind::four : Kind::other;
    const auto low = compare(c, kind, 0);
    return low == 0 ? Kind::zero : low > 0 ? Kind::one : Kind::other;
}

// The sentence about the club only: gender twice, the club's name, the
// staff member's.
void club_sentence(const Page& page, std::uint32_t back, std::uint32_t token, std::uint32_t club, std::uint32_t name) {
    const auto record = page.club(club);
    const auto gender = page.gender_of(record, club_gender);
    page.say(back, token, {gender, gender, record + club_name, name});
}
}

void describe_primary(Cpu& c, std::uint32_t message, std::uint32_t brief, std::uint32_t output) {
    Frame frame(c, 0x100072e0, 0x210, {.first_gpr = 16});
    const auto text = c.toc(-0x7bb8), nations = c.toc(-0x7b98), sessions = c.toc(-0x7bb0), staffs = c.toc(-0x7be0),
               clubs = c.toc(-0x7be8), pool = c.toc(-0x5a08);
    keep(c, {{31, text}, {16, nations}, {20, message}, {29, output}, {17, sessions}, {22, staffs}, {28, clubs}, {21, pool}});
    const bool full = compare(c, std::int8_t(brief)) != 0;
    const Page page{c, text, output, staffs, clubs, pool};
    const auto name = frame.at(0x40);
    // The sentences with a ninth argument leave it in the stack's parameter area.
    const auto spill = frame.at(0x38);
    const auto nation_of = [&](std::uint32_t id) { return c.u32(nations) + id * nation_size; };

    if (full) {
        const Person who{read(c, message, 0, 0x10007324, 23), read(c, message, 1, 0x10007338, 24),
                         read(c, message, 2, 0x1000734c, 25), read(c, message, 3, 0x10007360, 26)};
        const auto previous = read(c, message, 4, 0x10007374, 27);
        const auto club = read(c, message, 5, 0x10007384, 30);
        const auto kind = read(c, message, 6, 0x10007394, 18);
        const auto selector = read(c, message, 7, 0x100073a4, 19);
        const auto nation = read(c, message, 8, 0x100073b4, 20);
        page.write_name(0x100073d8, who, name, full_style);
        // The text is told differently for the club the player manages.
        if (!manages_club(c, sessions, club)) {
            switch (classify(c, std::int32_t(kind))) {
            case Kind::two: {
                const auto from = page.club(previous), to = page.club(club);
                const auto from_gender = page.gender_of(from, club_gender), to_gender = page.gender_of(to, club_gender);
                c.put32(spill, from + club_name);
                page.say(0x10007588, 0x3b5, {to_gender, to_gender, from_gender, from_gender, to + club_name, name});
                break;
            }
            case Kind::zero: club_sentence(page, 0x100075f0, 0x583, club, name); break;
            case Kind::one: club_sentence(page, 0x100075bc, 0x4aa, club, name); break;
            case Kind::four: {
                const auto with_nation = [&](std::uint32_t back, std::uint32_t token) {
                    const auto record = page.club(club), where = nation_of(nation);
                    const auto gender = page.gender_of(record, club_gender), where_gender = page.gender_of(where, nation_gender);
                    c.put32(spill, where + nation_name);
                    page.say(back, token, {gender, gender, where_gender, where_gender, record + club_name, name});
                };
                if (compare(c, std::int32_t(selector), 7) < 0) with_nation(0x100074a4, 0x25);
                // The nation -1 means none: the sentence names the club only.
                else if (compare(c, std::int32_t(nation), -1) == 0) club_sentence(page, 0x10007538, 0x2a3, club, name);
                else with_nation(0x10007504, 0x176);
                break;
            }
            case Kind::other: break;
            }
        } else {
            switch (classify(c, std::int32_t(kind))) {
            case Kind::two: {
                const auto from = page.club(previous), to = page.club(club);
                const auto from_gender = page.gender_of(from, club_gender), to_gender = page.gender_of(to, club_gender);
                c.put32(spill, to + club_name);
                page.say(0x10007718, 0x968, {from_gender, from_gender, to_gender, to_gender, name, from + club_name});
                break;
            }
            case Kind::zero: page.say(0x10007760, 0xb1f, {name}); break;
            case Kind::one: {
                const auto record = page.club(club);
                const auto gender = page.gender_of(record, club_gender);
                page.say(0x1000774c, 0xa51, {gender, gender, name, record + club_name});
                break;
            }
            case Kind::four: {
                if (compare(c, std::int32_t(selector), 7) < 0) {
                    const auto record = page.club(club), where = nation_of(nation);
                    const auto gender = page.gender_of(record, club_gender), where_gender = page.gender_of(where, nation_gender);
                    c.put32(spill, record + club_name);
                    page.say(0x10007678, 0x678, {where_gender, where_gender, gender, gender, name, where + nation_name});
                } else if (compare(c, std::int32_t(nation), -1) == 0) {
                    page.say(0x100076c8, 0x890, {name});
                } else {
                    const auto where = nation_of(nation);
                    const auto where_gender = page.gender_of(where, nation_gender);
                    page.say(0x100076b4, 0x79c, {where_gender, where_gender, name, where + nation_name});
                }
                break;
            }
            case Kind::other: break;
            }
        }
        page.append(0x1000776c);
        page.link_staff(16, who, name, 0x1000778c, 0x100077ac);
        if (compare(c, std::int32_t(previous), -1) > 0) page.link_club(0x100077d0, previous, club_link_name);
        if (compare(c, std::int32_t(club), -1) > 0) page.link_club(0x100077f4, club, club_link_name);
        return;
    }

    // The brief view reads field 4 and then field 6 twice, dropping both
    // first results.
    const Person who{read(c, message, 0, 0x10007804, 16), read(c, message, 1, 0x10007818, 17),
                     read(c, message, 2, 0x1000782c, 18), read(c, message, 3, 0x10007840, 19)};
    read(c, message, 4, 0x10007854, 0);
    const auto club = read(c, message, 5, 0x10007860, 23);
    read(c, message, 6, 0x10007870, 0);
    const auto kind = read(c, message, 6, 0x1000787c, 20);
    page.write_name(0x100078a0, who, name, brief_style);
    // The transfer and missing texts follow the staff id modulo 3, truncated
    // toward zero.
    const auto remainder = std::int32_t(who.id) % 3;
    switch (classify(c, std::int32_t(kind))) {
    case Kind::two:
        if (compare(c, remainder, 1) == 0) page.say(0x10007934, 0xc57, {name});
        else if (remainder > 0) page.say(0x10007948, 0xc8a, {name});
        else if (compare(c, remainder, 0) >= 0) page.say(0x10007920, 0xc24, {name});
        else page.say(0x10007948, 0xc8a, {name});
        break;
    case Kind::one:
        if (compare(c, remainder) == 0) page.say(0x10007980, 0xc57, {name});
        else page.say(0x10007994, 0xc8a, {name});
        break;
    case Kind::zero: {
        const auto record = page.club(club);
        const auto gender = page.gender_of(record, club_gender);
        page.say(0x100079c8, 0xcb7, {gender, gender, name, record + club_name});
        break;
    }
    case Kind::four: page.say(0x100078d8, 0xbe6, {name}); break;
    case Kind::other: break;
    }
    page.append(0x100079d4);
    page.link_staff(16, who, name, 0x100079f4, 0x10007a14);
    if (compare(c, std::int32_t(club), -1) > 0) page.link_club(0x10007a38, club, club_link_name);
}

void describe_training(Cpu& c, std::uint32_t message, std::uint32_t brief, std::uint32_t output) {
    Frame frame(c, 0x10008b60, 0x3a0, {.first_gpr = 16});
    const auto text = c.toc(-0x7bb8), sessions = c.toc(-0x7bb0), staffs = c.toc(-0x7be0), clubs = c.toc(-0x7be8),
               pool = c.toc(-0x5a08);
    keep(c, {{29, text}, {21, message}, {16, sessions}, {20, output}, {30, staffs}, {31, clubs}, {17, pool}});
    const bool full = compare(c, std::int8_t(brief)) != 0;
    const Page page{c, text, output, staffs, clubs, pool};
    const auto name = frame.at(0x1c8), reporter_name = frame.at(0x38);

    if (full) {
        const Person who{read(c, message, 0, 0x10008ba0, 28), read(c, message, 1, 0x10008bb4, 27),
                         read(c, message, 2, 0x10008bc8, 26), read(c, message, 3, 0x10008bdc, 25)};
        const auto club = read(c, message, 4, 0x10008bf0, 18);
        const auto selector = read(c, message, 5, 0x10008c00, 19);
        read(c, message, 6, 0x10008c10, 0);
        const Person reporter{read(c, message, 7, 0x10008c1c, 24), read(c, message, 8, 0x10008c30, 23),
                              read(c, message, 9, 0x10008c44, 22), read(c, message, 10, 0x10008c58, 21)};
        page.write_name(0x10008c80, who, name, full_style);
        // Exactly -1 means no reporter; any other value is taken as a staff id.
        const auto reported = [&] { return compare(c, std::int32_t(reporter.id), -1) != 0; };
        if (reported()) page.write_name(0x10008ca8, reporter, reporter_name, full_style);
        else c.put8(reporter_name, 0);
        if (manages_club(c, sessions, club)) {
            // The selector 0 and 1 have texts of their own, any other shares one.
            struct Texts { std::uint32_t alone, alone_back, with, with_back; };
            const auto order = compare(c, std::int32_t(selector), 1);
            const Texts texts = order == 0 ? Texts{0xdf7, 0x10008d60, 0xe60, 0x10008d78}
                              : order < 0 && compare(c, std::int32_t(selector), 0) >= 0 ? Texts{0xd09, 0x10008d2c, 0xd78, 0x10008d44}
                                                                                          : Texts{0xedc, 0x10008d94, 0xf4a, 0x10008dac};
            if (reported()) page.say(texts.with_back, texts.with, {reporter_name, name});
            else page.say(texts.alone_back, texts.alone, {name});
        } else {
            const auto record = page.club(club);
            const auto gender = page.gender_of(record, club_gender);
            const auto club_text = record + club_name;
            if (!reported()) {
                page.say(0x10008eb0, 0x1228, {gender, gender, 0x64, club_text, name});
            } else if (const auto order = compare(c, std::int32_t(selector), 1); order == 0) {
                page.say(0x10008e40, 0x108c, {gender, gender, club_text, reporter_name, name});
            } else if (order < 0 && compare(c, std::int32_t(selector), 0) >= 0) {
                page.say(0x10008e08, 0xfcb, {gender, gender, 0x64, club_text, reporter_name, name});
            } else {
                page.say(0x10008e78, 0x115a, {gender, gender, club_text, reporter_name, name});
            }
        }
        page.append(0x10008ebc);
        page.link_staff(16, who, name, 0x10008edc, 0x10008efc);
        if (reported()) page.link_staff(16, reporter, reporter_name, 0x10008f24, 0x10008f44);
        // The club link has no guard on the club.
        page.link_club(0x10008f60, club, club_link_name);
        return;
    }

    const Person who{read(c, message, 0, 0x10008f70, 19), read(c, message, 1, 0x10008f84, 22),
                     read(c, message, 2, 0x10008f98, 23), read(c, message, 3, 0x10008fac, 24)};
    const auto flag = read(c, message, 5, 0x10008fc0, 18);
    const auto kind = read(c, message, 6, 0x10008fd0, 16);
    page.write_name(0x10008ff4, who, name, brief_style);
    if (compare(c, std::int32_t(kind)) != 0) page.say(0x1000900c, 0x12c4, {name});
    else if (compare(c, std::int32_t(flag)) == 0) page.say(0x10009028, 0x12f9, {name});
    else page.say(0x1000903c, 0x133a, {name});
    page.append(0x10009048);
    page.link_staff(16, who, name, 0x10009068, 0x10009088);
}

void describe_fitness(Cpu& c, std::uint32_t message, std::uint32_t brief, std::uint32_t output) {
    Frame frame(c, 0x10009b60, 0x1bd0, {.first_gpr = 18});
    const auto staffs = c.toc(-0x7be0), clubs = c.toc(-0x7be8), pool = c.toc(-0x5a08);
    keep(c, {{23, message}, {30, staffs}, {31, clubs}, {22, brief}, {18, pool}, {21, output}});
    const auto text = frame.at(0x808), name = frame.at(0x420), reporter_name = frame.at(0x38);
    const Page page{c, text, output, staffs, clubs, pool};
    const auto id = read(c, message, 0, 0x10009b94, 29);
    const auto club = read(c, message, 1, 0x10009ba8, 19);
    const auto flag = read(c, message, 2, 0x10009bb8, 20);
    const auto first = read(c, message, 3, 0x10009bc8, 28), middle = read(c, message, 4, 0x10009bdc, 27),
               last = read(c, message, 5, 0x10009bf0, 26);
    const auto reporter_id = read(c, message, 6, 0x10009c04, 25);
    const auto reporter_first = read(c, message, 7, 0x10009c18, 24), reporter_middle = read(c, message, 8, 0x10009c2c, 23);
    const auto reporter_last = read(c, message, 9, 0x10009c40, 22);
    const bool full = compare(c, std::int8_t(brief)) != 0;
    const Person who{id, first, middle, last}, reporter{reporter_id, reporter_first, reporter_middle, reporter_last};

    if (!full) {
        page.write_name(0x10009c6c, who, name, brief_style);
        if (compare(c, std::int32_t(flag)) != 0) page.say(0x10009c88, 0x136e, {0x64, name});
        else page.say(0x10009c9c, 0x13a1, {name});
        page.append(0x10009ca8);
        page.link_staff(18, who, name, 0x10009cc8, 0x10009ce8);
        return;
    }

    page.write_name(0x10009d0c, who, name, full_style);
    // Any reporter id above -1 counts (signed), unlike the training text.
    const bool reported = compare(c, std::int32_t(reporter_id), -1) > 0;
    if (reported) page.write_name(0x10009d34, reporter, reporter_name, full_style);
    const auto record = page.club(club);
    const auto gender = page.gender_of(record, club_gender);
    if (reported) {
        if (compare(c, std::int32_t(flag)) != 0) page.say(0x10009d74, 0x13d4, {gender, gender, 0x64, record + club_name, reporter_name, name});
        else page.say(0x10009dac, 0x14a6, {gender, gender, record + club_name, reporter_name, name});
    } else if (compare(c, std::int32_t(flag)) != 0) {
        page.say(0x10009dec, 0x157f, {gender, gender, 0x64, record + club_name, name});
    } else {
        page.say(0x10009e24, 0x164d, {0x64, gender, gender, name, record + club_name});
    }
    page.append(0x10009e30);
    page.link_staff(18, who, name, 0x10009e50, 0x10009e70);
    // The club link has no guard on the club and names the club by its full name.
    page.link_club(0x10009e8c, club, club_name);
    if (compare(c, std::int32_t(reporter_id), -1) > 0) page.link_staff(18, reporter, reporter_name, 0x10009eb4, 0x10009ed4);
}
}

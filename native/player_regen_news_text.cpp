// PLAYER_REGEN.CPP, block `news_texts`: the callbacks that build the text of the module's three news messages (0xbce a youth promotion,
// 0xbba and 0xbbb retirements). Each reads the words of the message (0x102cb570), builds the person's name into a buffer on its frame
// (0x100cb590), formats one localized sentence into the shared text buffer (0x10727890: a sentence per case of the reason, the club, the
// nation and the person's job; the values handed over are the genders and names of the club and the nation, the person's name and a
// position's or job's title, the same values repeated in the orders the languages need) and then has the output item begin its text
// (0x1074b830) and append the person's name (0x1074bab0) when 0x100cbde0 allows it.
//
// Each sentence is one `say` line: the pool offset of the format, the address of the original's `bl` (the call trace pairs the calls by
// the return address) and the values in the order of the original's argument registers r5-r10 and stack words from sp+0x38. A site with
// fewer than six values leaves r6-r10 as the last callee left them, which the formatter's frame copies. Where the original reads
// back the byte a title routine (0x100c6bf0, 0x100c6af0) writes at a frame slot, the rewrite reads the same frame address, so a byte the
// routine does not write is the same leftover. Every routine runs in a frame of the original's size (the callees see the same addresses:
// the name buffer, the byte slots, the stack words) with r13-r31 set as the original holds them at each call.
//
// Unexercised: a message whose club or nation word is -1 where the sentence reads the record anyway (the original reads the row of index -1
// or the address 0x23b of a null club: both runs read the same address); the languages beyond what the fixtures set.
#include "player_regen_news_detail.hpp"
#include <array>

namespace cm::aot::player_regen {
namespace {
using namespace news;

// The record a sentence reads a club's or nation's gender and name from.
struct Rows {
    Cpu& c;
    std::uint32_t clubs_cell, nations_cell;
    std::uint32_t club_at(std::uint32_t index) const { return c.u32(clubs_cell) + index * row::club_size; }
    std::uint32_t nation_at(std::uint32_t index) const { return c.u32(nations_cell) + index * row::nation_size; }
};
std::uint32_t signed_byte(Cpu& c, std::uint32_t at) { return std::uint32_t(sext8(c.u8(at))); }

// The job byte's four groups the sentences distinguish for a staff member with a job: 12, 13, 15 and every other.
enum class JobGroup { twelve, thirteen, fifteen, other };
JobGroup group_of(std::int32_t job) {
    if (job == 0xe) return JobGroup::other;
    if (job < 0xe) return job == 0xc ? JobGroup::twelve : job > 0xb ? JobGroup::thirteen : JobGroup::other;
    return job < 0x10 ? JobGroup::fifteen : JobGroup::other;
}
}

void youth_promotion_text(Cpu& c, std::uint32_t item, std::uint32_t kind, std::uint32_t out) {
    Frame frame(c, 0x102dfbc0, 0x3b0, {.first_gpr = 13});
    const auto clubs_cell = c.toc(cells::clubs), buffer = c.toc(cells::text_buffer), people_cell = c.toc(cells::people), pool = c.toc(cells::pool);
    c.set({{15, item}, {25, clubs_cell}, {26, buffer}, {13, kind}, {16, people_cell}, {31, out}, {27, pool}});
    // The words 0..13 (word 5 is read and not used), kept in the registers the original keeps them in.
    constexpr std::uint32_t backs[14] = {0x102dfbf8, 0x102dfc0c, 0x102dfc20, 0x102dfc34, 0x102dfc48, 0x102dfc58, 0x102dfc64,
                                         0x102dfc74, 0x102dfc84, 0x102dfc98, 0x102dfcac, 0x102dfcc0, 0x102dfcd4, 0x102dfce4};
    constexpr unsigned registers[14] = {24, 23, 22, 21, 28, 0, 14, 29, 20, 17, 19, 18, 30, 13};
    std::uint32_t w[14];
    for (std::uint32_t n = 0; n < 14; ++n) {
        w[n] = invoke(c, entry::word, backs[n], {item, n});
        if (registers[n] != 0) c.set(registers[n], w[n]);
    }
    const auto person = w[0], first = w[1], surname = w[2], common = w[3], club = w[4], prospect_flag = w[6], is_assistant = w[7], discoverer = w[8],
               d_first = w[9], d_surname = w[10], d_common = w[11], great_or_good = w[12], second_grade = w[13];
    const auto name_at = frame.at(0x1cc), coach_name_at = frame.at(0x3c), scratch = frame.at(0x38);
    const auto say_at = [&](std::uint32_t text, std::uint32_t bl, std::initializer_list<std::uint32_t> values) {
        say(c, frame, buffer, pool + text, bl + 4, values);
    };
    const auto people = [&] { return c.u32(people_cell); };
    const auto name_of = [&](std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t at, std::uint32_t mode, std::uint32_t id, std::uint32_t back) {
        invoke(c, entry::person_name, back, {a, b, d, at, name_room, mode, id});
    };
    const auto append_name = [&](std::uint32_t at, std::uint32_t row, std::uint32_t bl) { invoke(c, entry::append_text, bl + 4, {out, at, 0x7d7, row}); };

    if (sext8(kind) == 0) {  // the headline
        name_of(first, surname, common, name_at, 5, person, 0x102dfd10);
        if (prospect_flag != 0) say_at(0xf8, 0x102dfd24, {name_at});
        else say_at(0x134, 0x102dfd38, {name_at});
        invoke(c, entry::begin_text, 0x102dfd48, {out, buffer});
        c.set(13, person * row::person_size);
        const auto row_of_person = people() + person * row::person_size;
        if (sext8(invoke(c, entry::name_is_new, 0x102dfd68, {row_of_person, first, surname, common, 0})) != 0)
            append_name(name_at, people() + person * row::person_size, 0x102dfd84);
        return;
    }
    // The body.
    name_of(first, surname, common, name_at, 1, person, 0x102dfdac);
    c.set(14, club * row::club_size);
    const auto club_row = [&] { return c.u32(clubs_cell) + club * row::club_size; };
    const auto club_gender = [&] { return signed_byte(c, club_row() + row::club_long_gender); };
    const auto in_setup = invoke(c, 0x100ce320, prospect_flag != 0 ? 0x102dfdcc : 0x102dfe4c, {club_row(), scratch, 1}) != 0;
    // Four sentences: the flag (word 6) and the answer of 0x100ce320 for the club.
    if (prospect_flag != 0) {
        if (in_setup) say_at(0x16e, 0x102dfdfc, {club_gender(), club_gender(), name_at, club_row() + row::club_long_name});
        else say_at(0x1fd, 0x102dfe2c, {club_gender(), club_gender(), name_at, club_row() + row::club_long_name});
    } else {
        if (in_setup) say_at(0x293, 0x102dfe7c, {club_gender(), club_gender(), name_at, club_row() + row::club_long_name});
        else say_at(0x320, 0x102dfeac, {club_gender(), club_gender(), name_at, club_row() + row::club_long_name});
    }
    invoke(c, entry::begin_text, 0x102dfebc, {out, buffer});
    if (std::int32_t(discoverer) > -1) {
        // The discoverer (a coach or the assistant manager): his name, a sentence by his grade (word 12: 2 great, 1 good; word 13) and kind.
        name_of(d_first, d_surname, d_common, coach_name_at, 1, discoverer, 0x102dfee4);
        if (great_or_good == 2) {
            if (is_assistant != 0) say_at(0x3b4, 0x102dff00, {coach_name_at});
            else say_at(0x457, 0x102dff14, {coach_name_at});
        } else if (great_or_good == 1 && second_grade == 2) {
            if (is_assistant != 0) say_at(0x4e9, 0x102dff40, {coach_name_at});
            else say_at(0x592, 0x102dff54, {coach_name_at});
        } else if (great_or_good == 1) {
            if (is_assistant != 0) say_at(0x62a, 0x102dff78, {coach_name_at});
            else say_at(0x6ce, 0x102dff8c, {coach_name_at});
        } else {
            if (is_assistant != 0) say_at(0x761, 0x102dffa8, {coach_name_at});
            else say_at(0x7d7, 0x102dffbc, {coach_name_at});
        }
        invoke(c, entry::begin_line, 0x102dffcc, {out, buffer});
        c.set(13, discoverer * row::person_size);
        const auto discoverer_row = people() + discoverer * row::person_size;
        if (sext8(invoke(c, entry::name_is_new, 0x102dffec, {discoverer_row, d_first, d_surname, d_common, 0})) != 0)
            invoke(c, entry::append_text, 0x102e000c, {out, coach_name_at, 0x7d7, people() + discoverer * row::person_size});
    }
    if (std::int32_t(club) > -1) invoke(c, entry::append_text, 0x102e0030, {out, club_row() + row::club_name, 0x7d5, club_row()});
    c.set(13, person * row::person_size);
    const auto row_of_person = people() + person * row::person_size;
    if (sext8(invoke(c, entry::name_is_new, 0x102e0050, {row_of_person, first, surname, common, 0})) != 0)
        invoke(c, entry::append_text, 0x102e0070, {out, name_at, 0x7d7, people() + person * row::person_size});
}

void retirement_text(Cpu& c, std::uint32_t item, std::uint32_t kind, std::uint32_t out) {
    Frame frame(c, 0x102e0510, 0x250, {.first_gpr = 13});
    const auto people_cell = c.toc(cells::people), clubs_cell = c.toc(cells::clubs), nations_cell = c.toc(cells::nations),
               buffer = c.toc(cells::text_buffer), pool = c.toc(cells::pool);
    c.set({{23, nations_cell}, {31, item}, {24, clubs_cell}, {15, out}, {14, c.toc(toc::rng_cursor)}, {25, buffer}, {26, people_cell}, {27, pool}});
    const Rows rows{c, clubs_cell, nations_cell};
    const auto name_at = frame.at(0x64);
    const auto say_at = [&](std::uint32_t text, std::uint32_t bl, std::initializer_list<std::uint32_t> values) {
        say(c, frame, buffer, pool + text, bl + 4, values);
    };
    const auto people = [&] { return c.u32(people_cell); };

    if (sext8(kind) == 0) {  // the headline: words 0..5 (the person's name ids, the reason, a flag)
        constexpr std::uint32_t backs[6] = {0x102e0554, 0x102e0568, 0x102e057c, 0x102e0590, 0x102e05a4, 0x102e05b4};
        std::uint32_t w[6];
        for (std::uint32_t n = 0; n < 6; ++n) {
            w[n] = invoke(c, entry::word, backs[n], {item, n});
            if (n < 4) c.set(n == 0 ? 13 : n == 1 ? 14 : n == 2 ? 16 : 17, w[n]);
            else c.set(n == 4 ? 18 : 19, std::uint32_t(sext8(w[n])));
        }
        const auto person = w[0], first = w[1], surname = w[2], common = w[3];
        const auto reason = sext8(w[4]), flag = sext8(w[5]);
        invoke(c, entry::person_name, 0x102e05d8, {first, surname, common, name_at, name_room, 5, person});
        if (reason == 8) say_at(0x83c, 0x102e05f0, {name_at});
        else if (flag == 0) say_at(0x89a, 0x102e0620, {name_at});
        else say_at(0x872, 0x102e060c, {name_at});
        invoke(c, entry::begin_text, 0x102e0630, {out, buffer});
        c.set(13, person * row::person_size);
        if (sext8(invoke(c, entry::name_is_new, 0x102e0650, {people() + person * row::person_size, first, surname, common, 0})) != 0)
            invoke(c, entry::append_text, 0x102e0670, {out, name_at, 0x7d7, people() + person * row::person_size});
        return;
    }
    // The body: words 0..9 and 11..14 (the reason, a flag, the club and nation indices (-1: none), the job, ...).
    constexpr std::uint32_t backs[14] = {0x102e0680, 0x102e0694, 0x102e06a8, 0x102e06bc, 0x102e06d0, 0x102e06e0, 0x102e06f0,
                                         0x102e0700, 0x102e0710, 0x102e0720, 0x102e0730, 0x102e0740, 0x102e0750, 0x102e0760};
    constexpr std::uint32_t indices[14] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xb, 0xc, 0xd, 0xe};
    std::uint32_t w[14];
    for (unsigned n = 0; n < 14; ++n) {
        w[n] = invoke(c, entry::word, backs[n], {item, indices[n]});
        switch (n) {
            case 0: c.set(22, w[n]); break;
            case 3: c.set(21, w[n]); break;
            case 4: c.set(20, std::uint32_t(sext8(w[n]))); break;
            case 5: c.set(19, std::uint32_t(sext8(w[n]))); break;
            case 6: c.set(28, w[n]); break;
            case 7: c.set(29, w[n]); break;
            case 8: c.set(18, std::uint32_t(sext8(w[n]))); break;
            case 9: c.set(17, w[n] & 0xff); break;
            case 10: c.set(13, w[n]); break;
            case 11: c.set(16, std::uint32_t(sext8(w[n]))); break;
            case 12: c.set(30, w[n]); break;
            case 13: c.set(31, w[n]); break;
            default: break;
        }
    }
    const auto person = w[0], first = w[1], surname = w[2], common = w[3];
    const auto reason = sext8(w[4]), flag = sext8(w[5]);
    const auto club = w[6], nation = w[7];
    const auto job = std::uint32_t(sext8(w[8]));
    const auto has_club_word = w[9] & 0xff;
    const auto word_b = w[10];
    const auto age = sext8(w[11]);
    const auto word_d = w[12], word_e = w[13];
    invoke(c, entry::person_name, 0x102e0784, {first, surname, common, name_at, name_room, 1, person});

    const auto club_gender = [&] { return signed_byte(c, rows.club_at(club) + row::club_gender); };
    const auto club_name = [&] { return rows.club_at(club) + row::club_name; };
    const auto nation_gender = [&] { return signed_byte(c, rows.nation_at(nation) + row::nation_gender); };
    const auto nation_name = [&] { return rows.nation_at(nation) + row::nation_name; };
    const auto person_job = [&] { return std::int32_t(sext8(c.u8(people() + person * row::person_size + 0x62))); };
    // The byte a title routine writes at sp+slot, and its answer: both read back by the sentence.
    struct Title { std::uint32_t text, byte; };
    const auto position = [&](std::uint32_t slot, std::uint32_t bl) {
        const auto text = invoke(c, entry::position_name, bl + 4, {job, 0, 0, frame.at(slot)});
        return Title{text, signed_byte(c, frame.at(slot))};
    };
    const auto title = [&](std::uint32_t slot, std::uint32_t bl) {
        const auto text = invoke(c, entry::job_title, bl + 4, {job, frame.at(slot)});
        return Title{text, signed_byte(c, frame.at(slot))};
    };
    const auto none = [&](std::int32_t index) { return index == -1; };
    const auto club_none = none(std::int32_t(club)), nation_none = none(std::int32_t(nation));
    const bool free_job = job == 0 || job == 0x10 || job == 0xb;  // no job, retired from football, or ...

    if (reason == 8) {
        if (has_club_word == 0) {
            if (c.u32(people() + person * row::person_size + 0x24) == 0) say_at(0xaf5, 0x102e095c, {name_at});
            else say_at(0xc4f, 0x102e0948, {club_gender(), club_gender(), club_name(), name_at});
        } else if (nation_none) {
            say_at(0xb85, 0x102e08fc, {nation_gender(), nation_gender(), nation_name(), name_at});
        } else if (club == 1) {
            say_at(0xaf5, 0x102e08c8, {name_at});
        } else {
            const auto current = person_job();
            if (current == 0 || current == 0x10 || current == 0xb) {
                say_at(0xa1b, 0x102e08b4, {nation_gender(), nation_gender(), club_gender(), club_gender(), nation_name(), club_name(), name_at});
            } else {
                const auto t = position(0x63, 0x102e07e4);
                say_at(0x8ce, 0x102e085c, {club_gender(), club_gender(), nation_gender(), nation_gender(), t.byte, t.byte, club_gender(), club_gender(),
                                           club_name(), nation_name(), name_at, t.text, club_name()});
            }
        }
    } else {
        say_at(0xd07, 0x102e0970, {name_at});
        if (flag == 0) {
            if (reason == 0x10) {
                if (age < 0x1b) say_at(0x163e, 0x102e0edc, {name_at});
                else say_at(0x16d9, 0x102e0ef0, {name_at});
            } else if (reason == 0x20 && (job == 6 || ((job - 8) & 0xff) <= 2 || ((job - 1) & 0xff) <= 2 || job == 4)) {
                const auto t = position(0x5a, 0x102e0f40);
                if (club_none) say_at(0x184f, 0x102e0f9c, {name_at});
                else say_at(0x1777, 0x102e0f88, {club_gender(), club_gender(), t.byte, t.byte, name_at, club_name(), t.text});
            } else if (word_d == 0 || free_job) {
                if (has_club_word == 0) say_at(0x22a4, 0x102e135c, {name_at});
                else if (nation_none) say_at(0x20a1, 0x102e1348, {name_at});
                else say_at(0x1b7e, 0x102e1334, {nation_gender(), nation_gender(), nation_name(), name_at});
            } else if (has_club_word == 0) {
                if (club_none) {
                    const auto t = position(0x58, 0x102e12d0);
                    say_at(0x21ef, 0x102e12f0, {t.byte, t.byte, name_at, t.text});
                } else {
                    const auto t = position(0x59, 0x102e1278);
                    say_at(0x2128, 0x102e12b8, {club_gender(), club_gender(), t.byte, t.byte, name_at, club_name(), t.text});
                }
            } else if (nation_none) {
                switch (group_of(person_job())) {
                    case JobGroup::twelve: say_at(0x1f23, 0x102e1230, {name_at}); break;
                    case JobGroup::thirteen: say_at(0x1fdd, 0x102e1244, {name_at}); break;
                    case JobGroup::fifteen: say_at(0x1e6b, 0x102e121c, {name_at}); break;
                    case JobGroup::other: say_at(0x20a1, 0x102e1258, {name_at}); break;
                }
            } else if (club_none) {
                switch (group_of(person_job())) {
                    case JobGroup::twelve: say_at(0x1cd3, 0x102e11a8, {name_at}); break;
                    case JobGroup::thirteen: say_at(0x1cd3, 0x102e11bc, {name_at}); break;
                    case JobGroup::fifteen: say_at(0x1c1d, 0x102e1194, {name_at}); break;
                    case JobGroup::other: say_at(0x1d97, 0x102e11d0, {name_at}); break;
                }
            } else {
                const auto with_club = [&](std::uint32_t text, std::uint32_t bl) {
                    say_at(text, bl, {nation_gender(), nation_gender(), club_gender(), club_gender(), nation_name(), name_at, club_name()});
                };
                switch (group_of(person_job())) {
                    case JobGroup::twelve: with_club(0x19a1, 0x102e10bc); break;
                    case JobGroup::thirteen: with_club(0x1a8a, 0x102e1114); break;
                    case JobGroup::fifteen: with_club(0x18b9, 0x102e1064); break;
                    case JobGroup::other: say_at(0x1b7e, 0x102e1148, {nation_gender(), nation_gender(), nation_name(), name_at}); break;
                }
            }
        } else if (word_d == 0 || word_e != 0) {
            if (word_b == 0) {
                if (has_club_word == 0) say_at(0xd07, 0x102e0eb8, {name_at});
                else if (nation_none) say_at(0xd07, 0x102e0ea4, {name_at});
                else if (club_none) say_at(0x15b1, 0x102e0e90, {nation_gender(), nation_gender(), nation_name(), name_at});
                else say_at(0x1505, 0x102e0e5c, {club_gender(), club_gender(), nation_gender(), nation_gender(), club_name(), nation_name(), name_at});
            } else if (has_club_word == 0) {
                const auto current = person_job();
                if (current == 0 || current == 0x10) say_at(0x1479, 0x102e0dec, {name_at});
                else {
                    const auto t = title(0x5b, 0x102e0db8);
                    say_at(0x13e3, 0x102e0dd8, {t.byte, t.byte, name_at, t.text});
                }
            } else if (nation_none) {
                if (free_job) say_at(0x133f, 0x102e0d84, {name_at});
                else {
                    const auto t = title(0x5c, 0x102e0d50);
                    say_at(0x129e, 0x102e0d70, {t.byte, t.byte, name_at, t.text});
                }
            } else if (club_none) {
                if (free_job || job == 0xc || job == 0xf || job == 0xd) {
                    say_at(0x11de, 0x102e0d28, {nation_gender(), nation_gender(), nation_name(), name_at});
                } else {
                    const auto t = title(0x5d, 0x102e0cb4);
                    say_at(0x111f, 0x102e0cf4, {nation_gender(), nation_gender(), t.byte, t.byte, nation_name(), name_at, t.text});
                }
            } else if (free_job) {
                // Without a job worth naming: one of two sentences by the parity of a pooled draw (2).
                Pool draw(c);
                draw.advance(0x102e0be0);
                if ((draw.sum() & 1) == 0) say_at(0x105d, 0x102e0c74, {nation_gender(), nation_gender(), nation_name(), name_at});
                else say_at(0xfa1, 0x102e0c40, {nation_gender(), nation_gender(), nation_name(), name_at});
            } else {
                const auto t = title(0x5e, 0x102e0b44);
                say_at(0xeba, 0x102e0bb0, {nation_gender(), nation_gender(), t.byte, t.byte, club_gender(), club_gender(), nation_name(), name_at, t.text,
                                           club_name()});
            }
        } else if (!club_none) {
            const auto t = position(0x62, 0x102e09a4);
            say_at(0xd5a, 0x102e0a04, {club_gender(), club_gender(), t.byte, t.byte, club_gender(), club_gender(), t.byte, t.byte, club_name(), t.text,
                                       name_at, club_name(), t.text});
        } else if (!nation_none) {
            const auto t = position(0x61, 0x102e0a24);
            say_at(0xd5a, 0x102e0a84, {nation_gender(), nation_gender(), t.byte, t.byte, nation_gender(), nation_gender(), t.byte, t.byte, nation_name(),
                                       t.text, name_at, nation_name(), t.text});
        } else if (job == 0) {
            const auto t = position(0x5f, 0x102e0adc);
            say_at(0xe6e, 0x102e0afc, {t.byte, t.byte, t.text, name_at});
        } else {
            const auto t = position(0x60, 0x102e0aa4);
            say_at(0xe20, 0x102e0ac4, {t.byte, t.byte, t.text, name_at});
        }
    }
    invoke(c, entry::begin_text, 0x102e136c, {out, buffer});
    c.set(13, person * row::person_size);
    if (sext8(invoke(c, entry::name_is_new, 0x102e138c, {people() + person * row::person_size, first, surname, common, 0})) != 0)
        invoke(c, entry::append_text, 0x102e13ac, {out, name_at, 0x7d7, people() + person * row::person_size});
}

void staff_retirement_text(Cpu& c, std::uint32_t item, std::uint32_t kind, std::uint32_t out) {
    Frame frame(c, 0x102e15b0, 0x210, {.first_gpr = 19});
    const auto people_cell = c.toc(cells::people), clubs_cell = c.toc(cells::clubs), buffer = c.toc(cells::text_buffer), pool = c.toc(cells::pool),
               humans = c.toc(-0x7bb0);  // the human managers' object (the slot holds the object)
    c.set({{21, humans}, {22, item}, {29, buffer}, {26, out}, {27, people_cell}, {30, pool}});
    const auto name_at = frame.at(0x44);
    const auto say_at = [&](std::uint32_t text, std::uint32_t bl, std::initializer_list<std::uint32_t> values) {
        say(c, frame, buffer, pool + text, bl + 4, values);
    };
    const auto people = [&] { return c.u32(people_cell); };

    if (sext8(kind) == 0) {  // the headline
        constexpr std::uint32_t backs[4] = {0x102e15ec, 0x102e1600, 0x102e1614, 0x102e1628};
        constexpr unsigned registers[4] = {19, 20, 21, 22};
        std::uint32_t w[4];
        for (std::uint32_t n = 0; n < 4; ++n) {
            w[n] = invoke(c, entry::word, backs[n], {item, n});
            c.set(registers[n], w[n]);
        }
        invoke(c, entry::person_name, 0x102e1650, {w[1], w[2], w[3], name_at, name_room, 5, w[0]});
        say_at(0x2316, 0x102e165c, {name_at});
        invoke(c, entry::begin_text, 0x102e166c, {out, buffer});
        c.set(19, w[0] * row::person_size);
        if (sext8(invoke(c, entry::name_is_new, 0x102e168c, {people() + w[0] * row::person_size, w[1], w[2], w[3], 0})) != 0)
            invoke(c, entry::append_text, 0x102e16ac, {out, name_at, 0x7d7, people() + w[0] * row::person_size});
        return;
    }
    constexpr std::uint32_t backs[7] = {0x102e16bc, 0x102e16d0, 0x102e16e4, 0x102e16f8, 0x102e170c, 0x102e171c, 0x102e172c};
    constexpr unsigned registers[7] = {28, 23, 24, 25, 20, 19, 22};
    std::uint32_t w[7];
    for (std::uint32_t n = 0; n < 7; ++n) {
        w[n] = invoke(c, entry::word, backs[n], {item, n});
        c.set(registers[n], registers[n] == 20 ? std::uint32_t(sext8(w[n])) : w[n]);
    }
    const auto person = w[0], first = w[1], surname = w[2], common = w[3], club_index = w[5];
    const auto reason = sext8(w[4]);
    const auto age_limit = std::int32_t(w[6]);
    const auto club = club_index != 0xffffffffu ? c.u32(clubs_cell) + club_index * row::club_size : 0;
    c.set(31, club);
    invoke(c, entry::person_name, 0x102e176c, {first, surname, common, name_at, name_room, 1, person});

    const auto record = [&] { return people() + person * row::person_size; };
    const auto club_gender = [&] { return signed_byte(c, club + row::club_long_gender); };
    const auto club_name = [&] { return club + row::club_long_name; };
    const auto staff = [&] { return c.u32(record() + 0x1c) != 0; };
    const auto job = [&] { return std::int32_t(sext8(c.u8(record() + 0x62))); };
    // The title of a position written at sp+slot, read back by the sentence.
    struct Title { std::uint32_t text, byte; };
    const auto position = [&](std::uint32_t raw_job, std::uint32_t slot, std::uint32_t bl) {
        const auto text = invoke(c, entry::position_name, bl + 4, {raw_job, 0, 0, frame.at(slot)});
        return Title{text, signed_byte(c, frame.at(slot))};
    };
    // The national team's side of a staff member (job byte +0x60 and link +0x20): the sentence with its team's gender and name.
    const auto national_sentence = [&](std::uint32_t slot, std::uint32_t position_bl, std::uint32_t text, std::uint32_t say_bl, std::uint32_t none_text,
                                       std::uint32_t none_bl) {
        const auto raw = c.u8(record() + 0x60);
        if (sext8(raw) == 0 || sext8(raw) == 0x10 || c.u32(record() + 0x20) == 0) {
            say_at(none_text, none_bl, {name_at});
        } else {
            const auto t = position(raw, slot, position_bl);
            const auto team = c.u32(record() + 0x20);
            say_at(text, say_bl, {signed_byte(c, team + row::club_gender), signed_byte(c, team + row::club_gender), t.byte, t.byte, name_at,
                                  team + row::club_name, t.text});
        }
    };
    // The sentence of a job group: 12, 13, 15 and every other job (group_of's order), as {pool offset, address of the original's `bl`}.
    using Sites = std::array<std::array<std::uint32_t, 2>, 4>;
    const auto by_group = [&](std::int32_t j, const Sites& sites) {
        const auto& site = sites[std::size_t(group_of(j))];
        say_at(site[0], site[1], {name_at});
    };
    const auto by_group_with_club = [&](std::int32_t j, const Sites& sites) {
        const auto& site = sites[std::size_t(group_of(j))];
        say_at(site[0], site[1], {club_gender(), club_gender(), club_name(), name_at});
    };
    const auto human_club = [&] {
        if (club == 0 || c.u32(club + 0x90) == 0) return false;
        const auto manager_row = humans + std::uint32_t(std::int32_t(c.i16(humans + 0x2c58)) * 0x2c0);
        return c.u32(club + 0x90) == c.u32(manager_row + 4);
    };
    if (human_club()) {
        if (reason == 0x10) {
            if (!staff()) {
                if (sext8(c.u16(record() + 0x5c)) >= 0x1b) say_at(0x28e6, 0x102e188c, {name_at});
                else say_at(0x283e, 0x102e1878, {name_at});
            } else if (job() == 0 || job() == 0x10) {
                say_at(0x273d, 0x102e1850, {name_at});
            } else {
                by_group(job(), {{{0x244f, 0x102e1814}, {0x254a, 0x102e1828}, {0x2353, 0x102e1800}, {0x264f, 0x102e183c}}});
            }
        } else if (reason == 1) {
            if (!staff()) say_at(0x2f2b, 0x102e1984, {name_at});
            else if (job() == 0 || job() == 0x10 || job() == 0xb) say_at(0x2e2b, 0x102e1970, {name_at});
            else {
                const Sites sites{{{0x2a95, 0x102e1918}, {0x2b9b, 0x102e192c}, {0x2991, 0x102e1904},
                                   age_limit <= 0x1b ? std::array<std::uint32_t, 2>{0x2d6c, 0x102e195c} : std::array<std::uint32_t, 2>{0x2cab, 0x102e1948}}};
                by_group(job(), sites);
            }
        } else if (reason == 0x20) {
            c.set(21, person * row::person_size);
            const auto raw = c.u8(record() + 0x62);
            if (sext8(raw) != 0 && sext8(raw) != 0x10 && c.u32(record() + 0x24) != 0) {
                const auto t = position(raw, 0x43, 0x102e19d0);
                say_at(0x2fd5, 0x102e19f0, {t.byte, t.byte, name_at, t.text});
            } else {
                national_sentence(0x42, 0x102e1a2c, 0x30a0, 0x102e1a68, 0x3182, 0x102e1a7c);
            }
        } else if (reason == 2) {
            say_at(0x3214, 0x102e1a98, {name_at});
        }
    } else if (reason == 0x10) {
        if (!staff()) {
            if (sext8(c.u16(record() + 0x5c)) >= 0x1b) say_at(0x398a, 0x102e1c04, {club_gender(), club_gender(), club_name(), name_at});
            else say_at(0x38a6, 0x102e1be0, {club_gender(), club_gender(), club_name(), name_at});
        } else if (job() == 0 || job() == 0x10) {
            say_at(0x376c, 0x102e1ba8, {club_gender(), club_gender(), club_name(), name_at});
        } else {
            by_group_with_club(job(), {{{0x33bd, 0x102e1b3c}, {0x34f4, 0x102e1b60}, {0x3288, 0x102e1b18}, {0x3635, 0x102e1b84}}});
        }
    } else if (reason == 1) {
        if (!staff()) {
            if (club == 0) say_at(0x2f2b, 0x102e1d54, {name_at});
            else say_at(0x3ee2, 0x102e1d40, {club_gender(), club_gender(), club_name(), name_at});
        } else if (job() == 0 || job() == 0x10 || job() == 0xb || club == 0) {
            say_at(0x2e2b, 0x102e1d14, {name_at});
        } else {
            by_group_with_club(job(), {{{0x3b99, 0x102e1cb8}, {0x3cc5, 0x102e1cdc}, {0x3a6f, 0x102e1c94}, {0x3dfb, 0x102e1d00}}});
        }
    } else if (reason == 0x20) {
        const auto raw = c.u8(record() + 0x62);
        if (club != 0 && sext8(raw) != 0 && sext8(raw) != 0x10 && c.u32(record() + 0x24) != 0) {
            const auto t = position(raw, 0x41, 0x102e1da8);
            say_at(0x3fb2, 0x102e1dd8, {club_gender(), club_gender(), t.byte, t.byte, club_name(), name_at, t.text});
        } else {
            c.set(19, person * row::person_size);
            national_sentence(0x40, 0x102e1e1c, 0x30a0, 0x102e1e58, 0x40a3, 0x102e1e6c);
        }
    } else if (reason == 2) {
        if (club == 0) say_at(0x3214, 0x102e1eb4, {name_at});
        else say_at(0x414c, 0x102e1ea0, {club_gender(), club_gender(), club_name(), name_at});
    }
    invoke(c, entry::begin_text, 0x102e1ec4, {out, buffer});
    c.set(19, person * row::person_size);
    if (sext8(invoke(c, entry::name_is_new, 0x102e1ee4, {record(), first, surname, common, 0})) != 0)
        invoke(c, entry::append_text, 0x102e1f04, {out, name_at, 0x7d7, record()});
}
}

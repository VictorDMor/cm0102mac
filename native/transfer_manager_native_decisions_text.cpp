// TRANSFER_MANAGER.CPP, block "decisions": the reasons a transfer is refused, as text (0x10410e50).
#include "guest_fpu.hpp"
#include "transfer_manager_native_decisions.hpp"
#include "transfer_manager_native_detail.hpp"
#include <array>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t format_text = 0x10727890, append_text = 0x1077b720;
constexpr std::int32_t pool_cell = -0x4ba0, text_cell = -0x4b9c, competitions_cell = -0x7bbc, league_table_cell = -0x7984;
constexpr std::uint32_t buffer_at = 0x38, buffer_size = 100, text_page = 0x10000;
// Flag bits of the caller's word and the message each one asks for (offsets from the text base + 0x10000).
constexpr std::uint32_t flag_refused = 0x80000000, flag_fee_low = 1, flag_league = 0x400000, flag_no_rival = 0x80000,
                        flag_rank = 0x8000, flag_stadium = 0x1000000, flag_coach = 0x2000000, flag_other = 0x10000,
                        flag_cap = 0x20000000;
constexpr std::uint32_t text_refused_only = 0xb08, text_fee_low = 0xcc0, text_league = 0xcfc, text_no_rival = 0xd5f,
                        text_rank_better = 0xd96, text_rank_else = 0xdf1, text_stadium = 0xe35, text_coach = 0xe79,
                        text_other = 0xeba, text_cap = 0xef6;
constexpr std::uint32_t text_summary_one = 0x4c19, text_summary_two = 0x4c1c;  // from the text base itself
// Records: club (name at +0x221, its length byte at +0x23b), competition (name +0x1a, length +0x6a).
constexpr std::uint32_t club_name = 0x221, club_name_length = 0x23b, club_competition = 0x14, club_stadium = 0x2c, club_standing = 0x30,
                        competition_size = 0x6c, competition_name = 0x1a, competition_name_length = 0x6a;

struct Refusal {
    Cpu& c;
    Frame& frame;
    guest::Fpu fpu;
    std::int32_t club, other;
    std::uint32_t flags, out, text_base = c.toc(text_cell), clubs_cell = c.toc(toc::clubs), same_stadium = 0;
    std::int32_t count = 0;  // the original's r28: how many reasons are written

    std::uint32_t clubs() const { return c.u32(clubs_cell); }
    std::uint32_t club_at(std::int32_t index) const { return clubs() + std::uint32_t(index) * club_size; }
    std::uint32_t competitions() const { return c.u32(c.toc(competitions_cell)); }
    std::uint32_t buffer(std::int32_t index) const { return frame.at(buffer_at + std::uint32_t(index) * buffer_size); }
    bool room() const { return std::int8_t(count) < 2; }
    void counted() { keep_signed(c, 28, count); }

    // The formatter takes the destination, the format and up to three values.
    void say(std::uint32_t back, std::uint32_t destination, std::uint32_t format, std::initializer_list<std::uint32_t> values = {}) {
        c.set(3, destination);
        c.set(4, format);
        unsigned n = 5;
        for (const auto value : values) c.set(n++, value);
        c.call(format_text, back);
    }
    // A name and its length byte: the formatter is given the length twice and the name's address.
    void say_named(std::uint32_t back, std::uint32_t destination, std::uint32_t format, std::uint32_t length_at, std::uint32_t name_at) {
        const auto length = std::uint32_t(std::int32_t(c.i8(length_at)));
        say(back, destination, text_base + text_page + format, {length, length, name_at});
    }
    // Adds a reason with no values at the next free buffer.
    void add(std::uint32_t back, std::uint32_t format) {
        say(back, buffer(std::int8_t(count)), text_base + text_page + format);
        ++count;
        counted();
    }
    void add_competition(std::uint32_t back, std::int32_t index) {
        const auto table = competitions();
        const auto record = table + std::uint32_t(index) * competition_size;
        c.set(8, table);  // the original loads the table into r8 and the formatter is handed it
        say_named(back, buffer(std::int8_t(count)), text_league, record + competition_name_length, record + competition_name);
        ++count;
        counted();
    }
    // The club's own refusal text (one of the first five bits): at the first buffer, ends the chain.
    bool first_reason(std::uint32_t flag, std::uint32_t format, std::uint32_t back) {
        if (!(flags & flag) || club <= -1) return false;
        const auto record = club_at(club);
        say_named(back, buffer(0), format, record + club_name_length, record + club_name);
        count = 1;
        counted();
        return true;
    }
};
}

std::uint32_t describe_refusal(Cpu& c, double fee, std::int32_t club, std::int32_t other, std::uint32_t flags) {
    Frame frame(c, 0x10410e50, 0x270, {.first_gpr = 23, .first_fpr = 31, .gpr_offset = -0x34});
    const auto out = c.u32(frame.at(0x2a8));  // the ninth argument, in the caller's parameter area
    Refusal r{c, frame, guest::Fpu(c), club, other, flags, out};
    keep(c, 23, std::uint32_t(club));
    keep(c, 24, std::uint32_t(other));
    keep(c, 25, flags);
    keep(c, 26, out);
    keep(c, 28, 0);
    keep(c, 29, c.toc(competitions_cell));
    keep(c, 30, r.clubs_cell);
    keep(c, 31, r.text_base);
    c.set_f(31, fee);
    const auto pool = c.toc(pool_cell);

    // Both clubs share one stadium record.
    if (other > -1 && club > -1) {
        const auto base = r.clubs() + club_stadium;
        const auto mine = c.u32(base + std::uint32_t(club) * club_size);
        if (mine == c.u32(base + std::uint32_t(other) * club_size)) r.same_stadium = 1;
    }
    keep(c, 27, r.same_stadium);
    const auto same = r.same_stadium != 0;
    if (flags & flag_refused) {
        r.say(0x10410ed8, out, r.text_base + text_page + text_refused_only);
        return 1;
    }

    // One of the first five reasons stops the chain at once.
    (void)(r.first_reason(0x8, 0xb55, 0x10410f20) || r.first_reason(0x20, 0xbac, 0x10410f68) ||
           r.first_reason(0x400, 0xbf6, 0x10410fb0) || r.first_reason(0x1000, 0xc31, 0x10410ff8) ||
           r.first_reason(0x100000, 0xc7a, 0x10411040));
    if (std::int8_t(r.count) == 0 && (flags & flag_fee_low)) {
        const auto limit = c.f64(pool + 0x1b0);
        r.fpu.fcmp(0, fee, limit);
        if (fee > limit || fee == limit) r.add(0x10411084, text_fee_low);
    }
    c.set(6, std::uint32_t(std::int32_t(std::int8_t(r.count))));  // 0x10411088: extsb r6,r28
    if (r.room() && (flags & flag_league) && club > -1) {
        const auto league = c.i32(r.club_at(club) + club_competition);
        if (league > -1) {
            r.add_competition(0x104110f4, league);
        } else {
            const auto entry = c.i32(c.u32(c.toc(league_table_cell)) + std::uint32_t(club) * 8);
            if (entry > -1) r.add_competition(0x1041114c, entry);
        }
    }
    if (!(!r.room() || same || !(flags & flag_no_rival))) {
        r.add(0x10411184, text_no_rival);
    } else if (r.room() && (flags & flag_rank)) {
        bool lower_rank = false;
        if (other > -1 && club > -1) {
            const auto mine = c.u32(r.clubs() + club_standing + std::uint32_t(club) * club_size);
            c.set(5, mine);  // 0x111bc and 0x111cc leave the two standing records in r5 and r6
            if (mine != 0) {
                const auto theirs = c.u32(r.clubs() + club_standing + std::uint32_t(other) * club_size);
                c.set(6, theirs);
                if (theirs != 0) {
                    const auto theirs_group = c.u32(theirs + 8);
                    if (theirs_group == c.u32(mine + 8)) {
                        const auto mine_rank = c.i16(mine + 0x18);
                        lower_rank = mine_rank > c.i16(theirs + 0x18);
                    }
                }
            }
        }
        if (lower_rank) r.add(0x10411210, text_rank_better);
        else r.add(0x10411234, text_rank_else);
    }
    bool stadium_done = false;
    if (r.room() && !same && (flags & flag_stadium) && other > -1) {
        const auto record = c.u32(r.club_at(other) + club_stadium);
        c.set(6, record);  // 0x11268: lwz r6 before the null test
        if (record != 0) {
            r.say_named(0x1041129c, r.buffer(std::int8_t(r.count)), text_stadium, record + 0x115, record + 0xae);
            ++r.count;
            r.counted();
            stadium_done = true;
        }
    }
    if (!stadium_done) {
        bool coach_done = false;
        if (r.room() && (flags & flag_coach) && other > -1) {
            if (const auto stadium_record = c.u32(r.club_at(other) + club_stadium); stadium_record != 0) {
                const auto record = c.u32(stadium_record + 0x74);
                c.set(6, record);  // 0x112d8: lwz r6 before the null test
                if (record != 0) {
                    r.say_named(0x1041130c, r.buffer(std::int8_t(r.count)), text_coach, record + 0xc3, record + 0x10);
                    ++r.count;
                    r.counted();
                    coach_done = true;
                }
            }
        }
        if (!coach_done && r.room() && !same && (flags & flag_other)) r.add(0x10411348, text_other);
    }
    if (r.room() && (flags & flag_cap)) r.add(0x10411378, text_cap);

    if (std::int8_t(r.count) == 2) {
        r.say(0x104113c0, out, r.text_base + text_summary_two, {r.buffer(0), r.buffer(1)});
    } else if (std::int8_t(r.count) == 1) {
        invoke(c, append_text, 0x104113a8, {out, r.text_base + text_summary_one, r.buffer(0)});
    }
    return std::uint32_t(r.count);
}
}

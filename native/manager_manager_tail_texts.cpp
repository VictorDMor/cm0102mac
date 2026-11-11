// MANAGER_MANAGER.CPP tail block (manager_manager_tail.hpp), native-first: the texts of three news messages
// (0x178a manager moves, 0x178d rivals, 0x178f assessments). Each reads the message record's fields through 0x102cb570,
// formats a text into the shared buffer and hands it to the output object (0x1074b830), and with the flag set also
// appends the people and clubs it names (0x1074bab0). Text only: no draw, no record of the game written.
#include "manager_manager_tail.hpp"
#include "manager_manager_detail.hpp"
#include <initializer_list>
#include <utility>

namespace cm::aot::manager_manager {
using guest::Cpu;
namespace {
namespace toc_cell {
constexpr std::int32_t text_buffer = -0x7bb8, pool = -0x5368, people = -0x7be0, clubs = -0x7be8, rows = -0x7b98,
                       competitions = -0x7bbc;
}
namespace callee {
constexpr std::uint32_t formatter = 0x10727890, to_object = 0x1074b830, name_to_object = 0x1074bab0, field = 0x102cb570,
                        person_name = 0x100caeb0, person_shown = 0x100cbde0, row_name = 0x10168a00;
}
constexpr std::uint32_t text_pool_base = 0x20000;  // the formats are at pool + 0x20000 + offset (addis r4,rP,2; addi)
constexpr std::uint32_t person_stride = 0x70, club_stride = 0x248;
namespace club_at { constexpr std::uint32_t name = 0x1ee, standing = 0x242, region = 0x23b, nickname = 0x221; }
constexpr std::uint32_t name_tag = 0x7d5, person_tag = 0x7da;

// What the three texts share: the message record's fields, the formatter, and the original's registers.
class News {
public:
    News(Cpu& cpu, std::uint32_t message, std::uint32_t out)
        : c(cpu), message(message), out(out), pool(cpu.toc(toc_cell::pool)), text(cpu.toc(toc_cell::text_buffer)) {}

    std::uint32_t field(std::uint32_t index, std::uint32_t back) {
        c.set({{3, message}, {4, index}});
        c.call(callee::field, back);
        return c.r(3);
    }
    std::uint32_t person_record(std::uint32_t index) const { return c.u32(c.toc(toc_cell::people)) + index * person_stride; }
    std::uint32_t club_record(std::uint32_t index) const { return c.u32(c.toc(toc_cell::clubs)) + index * club_stride; }

    // The formatter 0x10727890 with the text at `offset`: `bl` is the call's address in the original; the
    // arguments are the registers the original sets for it (r3 and r4 always).
    void say(std::uint32_t bl, std::uint32_t offset, std::initializer_list<std::pair<unsigned, std::uint32_t>> args = {}) {
        c.set({{3, text}, {4, pool + text_pool_base + offset}});
        c.set(args);
        c.call(callee::formatter, bl + 4);
    }
    void to_object(std::uint32_t bl) {
        c.set({{3, out}, {4, text}});
        c.call(callee::to_object, bl + 4);
    }
    // Whether 0x100cbde0 says the person is to be shown (its byte result), then the person's name with the tag.
    bool person_shown(std::uint32_t person, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t bl) {
        c.set({{3, person}, {4, a}, {5, b}, {6, d}, {7, 0}});
        c.call(callee::person_shown, bl + 4);
        return std::int8_t(c.r(3)) != 0;
    }
    void name_to_object(std::uint32_t object_text, std::uint32_t tag, std::uint32_t owner, std::uint32_t bl) {
        c.set({{3, out}, {4, object_text}, {5, tag}, {6, owner}});
        c.call(callee::name_to_object, bl + 4);
    }
    void person_name(std::uint32_t person, std::uint32_t buffer, std::uint32_t bl) {
        c.set({{3, person}, {4, buffer}, {5, 500}, {6, 1}});
        c.call(callee::person_name, bl + 4);
    }

    Cpu& c;
    std::uint32_t message, out, pool, text;
};
}

// ---- 0x101c12a0: message 0x178a, a manager takes a job ----
void move_news_text(Cpu& c, std::uint32_t message, std::uint32_t flag_word, std::uint32_t out) {
    guest::Frame frame(c, 0x101c12a0, 0x2e0, {.first_gpr = 18});
    constexpr std::uint32_t row_buffer = 0x38, name_buffer = 0xb0, row_stride = 0x128, row_standing = 0x115;
    News n(c, message, out);
    constexpr std::uint32_t getter_back[9] = {0x101c12d4, 0x101c12e4, 0x101c12f8, 0x101c130c, 0x101c1320, 0x101c1330,
                                              0x101c1340, 0x101c1354, 0x101c1364};
    std::uint32_t g[9];
    for (unsigned i = 0; i < 9; ++i) g[i] = n.field(i, getter_back[i]);
    const auto person = n.person_record(g[0]);
    const auto row = c.u32(c.toc(toc_cell::rows)) + g[5] * row_stride;
    const std::int8_t has_club = std::int8_t(g[4]);
    const std::int32_t six = std::int32_t(g[6]), seven = std::int32_t(g[7]);
    // The registers the original holds from here on (0x1074bab0's row copies the save area of the callees below it).
    c.set({{18, row}, {19, message}, {20, flag_word}, {21, g[6]}, {22, sext8(has_club)}, {23, g[3]}, {24, g[2]},
           {25, g[1]}, {26, n.text}, {27, c.toc(toc_cell::people)}, {28, n.pool}, {29, g[0] * person_stride},
           {30, g[7]}, {31, out}});
    n.person_name(person, frame.at(name_buffer), 0x101c138c);
    std::uint32_t back_object, back_shown, back_name;
    if (std::int8_t(flag_word) != 0) {
        c.set({{3, row}, {4, frame.at(row_buffer)}, {5, 0x78}, {6, 9}});
        c.call(callee::row_name, 0x101c13ac);
        const auto standing = sext8(c.i8(row + row_standing));
        auto say = [&](std::uint32_t bl, std::uint32_t offset) {
            n.say(bl, offset, {{5, standing}, {6, standing}, {7, 100}, {8, frame.at(row_buffer)}, {9, frame.at(name_buffer)}});
        };
        // The two texts that take the job's number set r10 to it.
        auto say_number = [&](std::uint32_t bl, std::uint32_t offset) {
            c.set(10, g[6]);
            say(bl, offset);
        };
        if (has_club == 0) say(0x101c14dc, 0x1090);
        else if (six == -1) {
            if (seven == -1) say(0x101c14b0, 0xfbc);
            else if (seven == 1) say(0x101c1454, 0xdcb);
            else say_number(0x101c1484, 0xe9c);
        } else if (six == 1) say(0x101c13e8, 0xbef);
        else say_number(0x101c1418, 0xccf);
        back_object = 0x101c14e8; back_shown = 0x101c1504; back_name = 0x101c1524;
    } else {
        if (has_club == 0) n.say(0x101c1588, 0x11bb);
        else if (six == -1 && seven == -1) n.say(0x101c1574, 0x118c, {{5, 100}, {6, frame.at(name_buffer)}});
        else n.say(0x101c1558, 0x115d, {{5, 100}, {6, frame.at(name_buffer)}});
        back_object = 0x101c1594; back_shown = 0x101c15b0; back_name = 0x101c15d0;
    }
    n.to_object(back_object);
    if (n.person_shown(person, g[1], g[2], g[3], back_shown)) n.name_to_object(frame.at(name_buffer), person_tag, person, back_name);
}

// ---- 0x101c1c20: message 0x178d, a club's rival moves in ----
void rival_news_text(Cpu& c, std::uint32_t message, std::uint32_t flag_word, std::uint32_t out) {
    guest::Frame frame(c, 0x101c1c20, 0x270, {.first_gpr = 20});
    constexpr std::uint32_t name_buffer = 0x48, competition_stride = 0x6c;
    News n(c, message, out);
    constexpr std::uint32_t getter_back[8] = {0x101c1c50, 0x101c1c60, 0x101c1c74, 0x101c1c88, 0x101c1c9c, 0x101c1cac,
                                              0x101c1cbc, 0x101c1ccc};
    std::uint32_t g[8];
    for (unsigned i = 0; i < 8; ++i) g[i] = n.field(i, getter_back[i]);
    const auto person = n.person_record(g[0]);
    const auto club_a = n.club_record(g[4]), club_b = n.club_record(g[5]);
    const auto competition_at = c.u32(c.toc(toc_cell::competitions)) + g[6] * competition_stride;
    c.set({{20, message}, {21, competition_at}, {22, flag_word}, {23, out}, {24, club_a}, {25, person}, {26, g[3]},
           {27, g[2]}, {28, g[1]}, {29, n.text}, {30, n.pool}, {31, club_b}});
    // The arguments the original leaves in r7..r9 at the call: 0x100caeb0 spills them into its frame.
    c.set({{7, c.u32(c.toc(toc_cell::competitions))}, {8, g[5] * club_stride}, {9, c.u32(c.toc(toc_cell::clubs))}});
    n.person_name(person, frame.at(name_buffer), 0x101c1d14);
    if (std::int8_t(flag_word) != 0) {
        const auto standing_a = sext8(c.i8(club_a + club_at::standing)), standing_b = sext8(c.i8(club_b + club_at::standing));
        const auto competition_byte = sext8(c.i8(competition_at + 0x67));
        c.put32(frame.at(0x38), club_a + club_at::name);
        c.put32(frame.at(0x3c), frame.at(name_buffer));
        c.put32(frame.at(0x40), club_b + club_at::name);
        c.put32(frame.at(0x44), competition_at + 0x4d);
        n.say(0x101c1d70, 0x11d1, {{5, standing_a}, {6, standing_a}, {7, standing_b}, {8, standing_b}, {9, competition_byte},
                                   {10, competition_byte}});
        n.to_object(0x101c1d7c);
        if (n.person_shown(person, g[1], g[2], g[3], 0x101c1d94)) n.name_to_object(frame.at(name_buffer), person_tag, person, 0x101c1db0);
        n.name_to_object(club_a + club_at::name, name_tag, club_a, 0x101c1dc4);
        n.name_to_object(club_b + club_at::name, name_tag, club_b, 0x101c1dd8);
    } else {
        const auto region = sext8(c.i8(club_a + club_at::region));
        n.say(0x101c1dfc, 0x131a, {{5, region}, {6, region}, {7, club_a + club_at::nickname}});
        n.to_object(0x101c1e08);
    }
}

// ---- 0x101c3b10: the assessment message 0x178f (manager_manager_assess.cpp files it) ----
void assessment_news_text(Cpu& c, std::uint32_t message, std::uint32_t flag_word, std::uint32_t out) {
    guest::Frame frame(c, 0x101c3b10, 0x280, {.first_gpr = 18});
    constexpr std::uint32_t name_buffer = 0x48, extra_args = 0x38;
    News n(c, message, out);
    constexpr std::uint32_t getter_back[11] = {0x101c3b40, 0x101c3b50, 0x101c3b64, 0x101c3b78, 0x101c3b8c, 0x101c3b9c,
                                               0x101c3bac, 0x101c3bbc, 0x101c3bcc, 0x101c3bd8, 0x101c3be8};
    std::uint32_t g[11];
    for (unsigned i = 0; i < 11; ++i) g[i] = n.field(i, getter_back[i]);
    const auto person = n.person_record(g[0]);
    const auto club = n.club_record(g[4]);
    const std::int32_t sector = std::int32_t(g[5]), kind = std::int32_t(g[6]), stage = std::int32_t(g[7]);
    c.set({{18, flag_word}, {19, g[5]}, {20, g[6]}, {21, g[7]}, {22, g[9]}, {23, g[10]}, {24, g[1]}, {25, g[2]},
           {26, g[3]}, {27, person}, {28, out}, {29, n.pool}, {30, n.text}, {31, club}});
    c.set(7, c.u32(c.toc(toc_cell::clubs)));  // left in r7 by the original; 0x100caeb0 spills it into its frame
    n.person_name(person, frame.at(name_buffer), 0x101c3c1c);
    const auto standing = sext8(c.i8(club + club_at::standing));
    const auto name = club + club_at::name;
    const auto buffer = frame.at(name_buffer);
    // The common shape of the texts of the body: the club's standing twice, 100 twice, the club's name and the
    // person's name text (the last also as the first stack word).
    auto body = [&](std::uint32_t bl, std::uint32_t offset) {
        c.put32(frame.at(extra_args), buffer);
        n.say(bl, offset, {{5, standing}, {6, standing}, {7, 100}, {8, 100}, {9, name}, {10, buffer}});
    };
    // The shorter shape: the standing twice and the club's name.
    auto short_body = [&](std::uint32_t bl, std::uint32_t offset) {
        n.say(bl, offset, {{5, standing}, {6, standing}, {7, name}});
    };
    // The three-way choice on the sector the original makes with signed compares: 1, 2, 3 and anything else.
    auto by_sector = [&](std::uint32_t one, std::uint32_t two, std::uint32_t three, std::uint32_t other) {
        return sector == 3 ? three : sector == 1 ? one : sector == 2 ? two : other;
    };
    if (std::int8_t(flag_word) != 0) {
        if (kind == 1) {
            if (stage == 2) {
                // sector 1, 2, 3, other: 0x135f 0x14e3 0x16d2 0x1888 (calls at 3c98, 3ccc, 3d00, 3d34)
                struct Pick { std::uint32_t bl, offset; };
                const Pick picks[4] = {{0x101c3c98, 0x135f}, {0x101c3ccc, 0x14e3}, {0x101c3d00, 0x16d2}, {0x101c3d34, 0x1888}};
                const auto pick = picks[sector == 1 ? 0 : sector == 2 ? 1 : sector == 3 ? 2 : 3];
                body(pick.bl, pick.offset);
            } else if (sector == 3) body(0x101c3e88, 0x2130);
            else if (sector == 1) {
                if (g[9] == 0) {
                    // The board's byte at +0x4b, and four stack words (the club's name, the name text twice, the board +0x18).
                    const auto board = c.u32(club + 0x3c);
                    const auto board_byte = sext8(c.i8(board + 0x4b));
                    c.put32(frame.at(0x38), name);
                    c.put32(frame.at(0x3c), buffer);
                    c.put32(frame.at(0x40), buffer);
                    c.put32(frame.at(0x44), board + 0x18);
                    n.say(0x101c3de4, 0x1bed, {{5, standing}, {6, standing}, {7, 100}, {8, 100}, {9, board_byte}, {10, board_byte}});
                } else body(0x101c3d90, 0x1a50);
            } else if (sector == 2) body(g[10] == 0 ? 0x101c3e20 : 0x101c3e54, g[10] == 0 ? 0x1d95 : 0x1faa);
            else body(0x101c3ebc, 0x231a);
        } else if (stage == 4) short_body(0x101c3ee8, 0x24c5);
        else if (stage == 3) {
            const std::uint32_t bl[4] = {0x101c3f34, 0x101c3f58, 0x101c3f7c, 0x101c3fa0};
            const std::uint32_t at[4] = {0x25bc, 0x270d, 0x2835, 0x294c};
            const auto k = sector == 1 ? 0 : sector == 2 ? 1 : sector == 3 ? 2 : 3;
            short_body(bl[k], at[k]);
        } else if (stage == 2) {
            const std::uint32_t bl[4] = {0x101c3fec, 0x101c4010, 0x101c4034, 0x101c4058};
            const std::uint32_t at[4] = {0x2a77, 0x2bce, 0x2ccb, 0x2dae};
            const auto k = sector == 1 ? 0 : sector == 2 ? 1 : sector == 3 ? 2 : 3;
            short_body(bl[k], at[k]);
        } else if (stage == 1) {
            const std::uint32_t bl[4] = {0x101c40a4, 0x101c40c8, 0x101c40ec, 0x101c4110};
            const std::uint32_t at[4] = {0x2f02, 0x2fbe, 0x3091, 0x3173};
            const auto k = sector == 1 ? 0 : sector == 2 ? 1 : sector == 3 ? 2 : 3;
            short_body(bl[k], at[k]);
        }
        (void)by_sector;
        n.to_object(0x101c411c);
        if (n.person_shown(person, g[1], g[2], g[3], 0x101c4134)) n.name_to_object(buffer, person_tag, person, 0x101c4150);
        n.name_to_object(name, name_tag, club, 0x101c4164);
    } else {
        if (kind == 1) {
            n.say(0x101c41a4, 0x3244, {{5, sext8(c.i8(club + club_at::region))}, {6, sext8(c.i8(club + club_at::region))},
                                       {7, club + club_at::nickname}});
        } else if (stage == 2 || stage == 3 || stage == 4) {
            const std::uint32_t bl[4] = {0x101c41ec, 0x101c4200, 0x101c4214, 0x101c4228};
            const std::uint32_t at[4] = {0x3284, 0x32a7, 0x32d7, 0x32d7};
            n.say(bl[sector == 1 ? 0 : sector == 2 ? 1 : sector == 3 ? 2 : 3], at[sector == 1 ? 0 : sector == 2 ? 1 : sector == 3 ? 2 : 3]);
        } else {
            const std::uint32_t bl[4] = {0x101c425c, 0x101c4270, 0x101c4284, 0x101c4298};
            const std::uint32_t at[4] = {0x330f, 0x3331, 0x3360, 0x3360};
            n.say(bl[sector == 1 ? 0 : sector == 2 ? 1 : sector == 3 ? 2 : 3], at[sector == 1 ? 0 : sector == 2 ? 1 : sector == 3 ? 2 : 3]);
        }
        n.to_object(0x101c42a4);
    }
}
}

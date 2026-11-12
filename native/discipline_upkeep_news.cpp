// DISCIPLINE.CPP upkeep block (discipline_upkeep.hpp): the news record of a ban, native-first. It calls the news
// and text routines of other modules with the arguments the original passes, in its order, and keeps in its frame
// what they read by address: the record's text-parameter list (0xf8), the running parameter index (0x38, a byte
// the formatting calls read and advance), the two text objects (0x44, 0x5c) and the recipient array (0x80).
#include "discipline_upkeep.hpp"
#include "discipline_upkeep_detail.hpp"

namespace cm::aot::discipline {
using guest::Cpu;
using guest::Frame;
using namespace upkeep;

namespace {
namespace news_callee {
inline constexpr std::uint32_t recipients = 0x103fc710, init_list = 0x102bdf90, init_text = 0x10715880,
                               person_text = 0x1030d6e0, add_parameter = 0x102cb4b0, finish_text = 0x1030e110,
                               fixture_side = 0x100c7d00, fixture_text = 0x1030d4a0, add_fixture_text = 0x1030dcd0,
                               can_post = 0x102cdbf0, post = 0x102cc140, post_to_all = 0x102cc8e0;
}
// The word the news manager's global holds, read at each use.
std::uint32_t news_object(const Cpu& c) { return c.u32(c.toc(toc::news)); }
struct Date {
    std::int16_t high;
    std::uint16_t low;
};
// `today` is before `end` (as in the bans block: the original's sign test is end.high > today.high).
bool before(Date today, Date end) {
    if (today.low != end.low) return today.low < end.low;
    return end.high > today.high;
}
}

void post_ban_news(Cpu& c, std::uint32_t person, std::uint32_t ban, std::uint32_t fixture) {
    Frame frame(c, 0x100e0700, 0x200, {.first_gpr = 26});
    const auto counter = frame.at(0x38), list = frame.at(0xf8), recipients = frame.at(0x80);
    const auto today = c.toc(toc::today);
    // r26-r31 hold what the original keeps in them: the today cell, the recipients call's result (0 without
    // one), the news manager cell, the person, the fixture and the ban. The generated callees save them in
    // their frames, and a ruling body later copies a ban struct it never fully wrote from that stack into a
    // ban record (bytes +0xd..+0xf, part of +0x14; the BlockMoveData reader 0x10bf7bbc that
    // tools/stack_contract.py lists for 0x100e0700). The recipients loop at the end reloads r26 and r29.
    const auto set_registers = [&](std::uint32_t result) {
        c.set({{26, today}, {27, result}, {28, c.toc(toc::news)}, {29, person}, {30, fixture}, {31, ban}});
    };
    set_registers(0);
    c.put8(counter, 0);
    std::int8_t count = 0;
    std::uint32_t recipients_result = 0;
    if (const auto source = c.u32(c.toc(toc::news_source)); source != 0) {
        c.set({{3, source}, {4, person}, {5, 0xda}, {6, recipients}, {7, 0x1e}});
        c.call(news_callee::recipients, 0x100e074c);
        recipients_result = c.r(3);
        count = std::int8_t(recipients_result);
        set_registers(recipients_result);
    }
    c.set({{3, list}, {4, 0x7d2}, {5, 0}});
    c.call(news_callee::init_list, 0x100e0760);
    c.set(3, frame.at(0x54));
    c.call(news_callee::init_text, 0x100e0768);
    c.set({{3, person}, {4, frame.at(0x44)}});
    c.call(news_callee::person_text, 0x100e0774);
    // The next text parameter: the running index is a byte in the frame, advanced before the value is read.
    const auto parameter = [&](std::uint32_t value, std::uint32_t back) {
        const auto index = c.u8(counter);
        c.put8(counter, index + 1u);
        c.set({{3, list}, {4, index}, {5, value}});
        c.call(news_callee::add_parameter, back);
    };
    const auto half = [&](std::uint32_t at) { return std::uint32_t(std::int32_t(c.i16(at))); };
    const auto byte = [&](std::uint32_t at) { return std::uint32_t(std::int32_t(c.i8(at))); };
    parameter(0, 0x100e078c);
    if (fixture != 0) parameter(half(fixture + 0x2e), 0x100e07ac);
    else parameter(0xffffffffu, 0x100e07c8);
    parameter(byte(ban), 0x100e07e4);
    parameter(half(ban + 2), 0x100e07fc);
    const auto now = c.u32(today);
    if (before({std::int16_t(now >> 16), std::uint16_t(now)}, {c.i16(ban + 4), c.u16(ban + 6)})) {
        parameter(half(ban + 4), 0x100e0870);
        parameter(c.u16(ban + 6), 0x100e0888);
    } else {
        parameter(0, 0x100e08a4);
        parameter(0, 0x100e08bc);
    }
    parameter(byte(ban + 0xc), 0x100e08d8);
    parameter(byte(ban + 0xd), 0x100e08f4);
    parameter(byte(ban + 0xe), 0x100e0910);
    parameter(byte(ban + 0xf), 0x100e092c);
    parameter(byte(ban + 0x10), 0x100e0948);
    parameter(half(ban + 0x12), 0x100e0960);
    parameter(c.u8(ban + 0x14) >> 6 & 1u, 0x100e097c);
    parameter(c.u8(ban + 0x14) >> 5 & 1u, 0x100e0998);
    parameter(c.u8(ban + 0x14) >> 7, 0x100e09b4);
    parameter(half(today), 0x100e09cc);
    parameter(c.u16(today + 2), 0x100e09e4);
    c.set({{3, frame.at(0x44)}, {4, list}, {5, counter}});
    c.call(news_callee::finish_text, 0x100e09f4);

    if (fixture == 0) {
        parameter(0, 0x100e0aac);
    } else {
        parameter(1, 0x100e0a14);
        c.set(3, frame.at(0x5c));
        c.call(news_callee::init_text, 0x100e0a1c);
        c.set(3, c.u32(fixture + 0x1c));
        c.call(news_callee::fixture_side, 0x100e0a24);
        const bool home = std::int8_t(c.r(3)) != 0;
        const auto side = c.u32(person + (home ? 0x20 : 0x24));  // the person's club
        if (side != c.u32(fixture + 0x1c) && side != c.u32(fixture + 0x20)) return;
        c.set({{3, fixture}, {4, side}, {5, frame.at(0x5c)}});
        c.call(news_callee::fixture_text, home ? 0x100e0a54 : 0x100e0a80);
        c.set({{3, frame.at(0x5c)}, {4, list}, {5, counter}});
        c.call(news_callee::add_fixture_text, 0x100e0a90);
    }
    if (c.i16(ban + 2) <= 0x32) {
        const auto human = c.u32(person + 0x24);
        if (human == 0) return;
        c.set({{3, news_object(c)}, {4, human}});
        c.call(news_callee::can_post, 0x100e0b40);
        if (std::int8_t(c.r(3)) == 0) return;
        c.set({{3, news_object(c)}, {4, list}, {5, c.u32(person + 0x24)}});
        c.call(news_callee::post, 0x100e0b58);
        return;
    }
    if (const auto manager_record = c.u32(person + 0x14); manager_record != 0 && c.i16(manager_record + 0xc) > 0x1f40) {
        c.set({{3, news_object(c)}, {4, list}});
        c.call(news_callee::post_to_all, 0x100e0adc);
        return;
    }
    c.set({{26, std::uint32_t(std::int32_t(count))}, {29, recipients}});
    for (std::int8_t at = 0; at < count; ++at) {
        c.set({{3, news_object(c)}, {4, list}, {5, c.u32(recipients + std::uint32_t(std::int32_t(at)) * 4)}});
        c.call(news_callee::post, 0x100e0b0c);
    }
}
}

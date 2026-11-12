// DISCIPLINE.CPP texts block cases (discipline_checks.hpp): the ban line, the bodies text and the status text,
// from the career snapshot's real ruling bodies and ban records; Case::memory builds the records a case
// hands over in the scratch area and moves the values the rules read. The output buffer is scratch memory
// filled with a pattern, so the comparison covers every byte the texts write (and the ones they leave).
#include "discipline_checks.hpp"
#include <cstdint>
#include <string>

namespace {
using namespace cm::aot::discipline_checks;
constexpr std::uint32_t ban_line = 0x100e7210, describe_bodies = 0x100e7490, status_text = 0x100e7780;
constexpr std::uint32_t output = scratch_base + 0x8000, bytes_at = scratch_base + 0x9000, record_at = scratch_base + 0xa000;

std::vector<Write> filled(std::uint32_t at, unsigned size) {
    std::vector<Write> writes;
    for (unsigned offset = 0; offset < size; offset += 4) writes.push_back({4, at + offset, 0x5a5a5a00u + offset / 4});
    return writes;
}
void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }

// A ban record the way a caller builds it (0x18 bytes) in scratch, with a date text source after it.
struct Ban {
    std::int8_t length = 2;
    std::int16_t date = 9;
    std::uint16_t early = 3, late = 4;
    std::int8_t bodies[4] = {-1, -1, -1, -1};
    std::uint8_t flag_byte = 0;
};
std::vector<Write> ban_writes(std::uint32_t at, const Ban& b) {
    std::vector<Write> w{{1, at, std::uint8_t(b.length)}, {1, at + 1, 0}, {2, at + 2, std::uint16_t(b.date)}, {2, at + 4, b.early},
                         {2, at + 6, b.late}, {1, at + 0x14, b.flag_byte}};
    for (unsigned k = 0; k < 4; ++k) w.push_back({1, at + 0xc + k, std::uint8_t(b.bodies[k])});
    return w;
}
// The ban record's date is formatted by 0x10715e00 only for a ban that starts after today. A day halfword outside a
// calendar day (negative or above 400) never occurs in a record, and the date formatter then reads stale stack:
// such a ban is only built where today's date is not before it (the compare of the halfwords is what it checks).
bool ahead(std::uint16_t today_early, std::uint16_t today_late, std::uint16_t early, std::uint16_t late) {
    if (today_late != late) return today_late < late;
    const auto a = std::int32_t(std::int16_t(today_early)), b = std::int32_t(std::int16_t(early));
    const auto differ = std::uint32_t(b ^ a);
    return ((std::uint32_t(std::int32_t(differ) >> 1) - (differ & std::uint32_t(b))) >> 31) != 0;
}
bool calendar_day(std::uint16_t day) { return day <= 400; }
}

DISCIPLINE_CASES("texts") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7aa4);
    const auto bodies = scenario.r32(manager);
    const auto date_at = scenario.toc(-0x7b80);
    const auto today = scenario.r32(date_at);  // the early halfword, then the late one
    const auto body_object = [&](unsigned index) { return scenario.r32(bodies + index * 4); };
    const auto i8 = [&](std::uint32_t at) { return std::int8_t(scenario.s.memory.read<1>(at)); };

    // ---- 0x100e7490: which bodies the ban names (all 16 patterns of present bytes), with and without the flag.
    // Pattern 0 (no body named) is left out: the original then reads the word before the ruling-body array, and both sides fault.
    for (unsigned pattern = 1; pattern < 16; ++pattern)
        for (std::uint32_t flag : {0u, 1u}) {
            std::vector<Write> memory = filled(output, 0x100);
            for (unsigned k = 0; k < 4; ++k) memory.push_back({1, bytes_at + k, (pattern >> k & 1) ? 3 + 7 * k : 0xffu});
            cases.push_back({.entry = describe_bodies, .registers = {{3, manager}, {4, bytes_at}, {5, output}, {6, 100}, {7, flag}, {8, 4}},
                             .label = "texts: bodies pattern " + std::to_string(pattern) + (flag ? " refusing a fourth" : ""), .memory = memory});
        }
    // The flag is a signed byte: bits above it do not count.
    {
        std::vector<Write> memory = filled(output, 0x100);
        for (unsigned k = 0; k < 4; ++k) memory.push_back({1, bytes_at + k, 3 + 7 * k});
        cases.push_back({.entry = describe_bodies, .registers = {{3, manager}, {4, bytes_at}, {5, output}, {6, 100}, {7, 0x100}, {8, 4}},
                         .label = "texts: bodies flag word with only high bits", .memory = memory});
        cases.push_back({.entry = describe_bodies, .registers = {{3, manager}, {4, bytes_at}, {5, output}, {6, 100}, {7, 0x80}, {8, 4}},
                         .label = "texts: bodies flag byte 0x80", .memory = memory});
    }
    // The size check (signed, below 0x46 reports and returns -1) and the argument handed to the body's method.
    for (std::uint32_t size : {0x45u, 0x46u, 0xffffffffu, 100u}) {
        std::vector<Write> memory = filled(output, 0x100);
        memory.push_back({1, bytes_at, 5});
        for (unsigned k = 1; k < 4; ++k) memory.push_back({1, bytes_at + k, 0xff});
        cases.push_back({.entry = describe_bodies, .registers = {{3, manager}, {4, bytes_at}, {5, output}, {6, size}, {7, 0}, {8, 4}},
                         .label = "texts: bodies size " + std::to_string(std::int32_t(size)), .memory = memory});
    }
    // The first body named is the one that describes itself and classifies the result: real bodies, then a real
    // body's kind byte (+4), type byte (+5) and byte at +0x21 moved over every outcome of the classification.
    for (unsigned index = 0; index < 42; ++index) {
        std::vector<Write> memory = filled(output, 0x100);
        memory.push_back({1, bytes_at, 0xff});
        memory.push_back({1, bytes_at + 1, std::uint32_t(index)});
        for (unsigned k = 2; k < 4; ++k) memory.push_back({1, bytes_at + k, 0xff});
        cases.push_back({.entry = describe_bodies, .registers = {{3, manager}, {4, bytes_at}, {5, output}, {6, 100}, {7, 0}, {8, 4}},
                         .label = "texts: body " + std::to_string(index) + " describes itself", .memory = memory});
    }
    {
        const auto object = body_object(1);
        struct Kind { int kind, type, extra; const char* what; };
        const Kind kinds[] = {{1, 0, 0, "kind 1"},          {0, 0, -1, "kind 0, byte -1"}, {0, 0, 0, "kind 0, byte 0"}, {0, 0, 7, "kind 0, byte 7"},
                              {3, 0, 0, "kind 3"},          {2, 1, 0, "type 1"},           {2, 2, 0, "type 2"},         {2, 0x23, 0, "type 0x23"},
                              {2, 0x29, 0, "type 0x29"},    {2, 0, 0, "type 0"},           {2, 3, 0, "type 3"},         {2, -1, 0, "type -1"},
                              {2, 0x22, 0, "type 0x22"},    {2, 0x24, 0, "type 0x24"},     {2, 0x7f, 0, "type 0x7f"},   {-1, 1, 0, "kind -1"},
                              {4, 1, 0, "kind 4"}};
        for (const auto& k : kinds) {
            std::vector<Write> memory = filled(output, 0x100);
            memory.push_back({1, bytes_at, 1});
            for (unsigned at = 1; at < 4; ++at) memory.push_back({1, bytes_at + at, 0xff});
            memory.push_back({1, object + 4, std::uint32_t(k.kind) & 0xff});
            memory.push_back({1, object + 5, std::uint32_t(k.type) & 0xff});
            memory.push_back({1, object + 0x21, std::uint32_t(k.extra) & 0xff});
            cases.push_back({.entry = describe_bodies, .registers = {{3, manager}, {4, bytes_at}, {5, output}, {6, 100}, {7, 0}, {8, 4}},
                             .label = std::string("texts: body classification, ") + k.what, .memory = memory});
        }
    }

    // ---- 0x100e7210: the real ban records of the snapshot, and records built in scratch for every length,
    // pattern of bodies, flag and date relation.
    {
        const auto records = scenario.r32(manager + 4);
        const auto count = std::int16_t(scenario.r32(manager + 8));
        unsigned taken = 0;
        for (std::int16_t at = 0; at < count && taken < 12; ++at) {
            const auto record = records + std::uint32_t(at) * 0x18;
            if (i8(record + 0xc) == -1) continue;
            ++taken;
            cases.push_back({.entry = ban_line, .registers = {{3, manager}, {4, record}, {5, output}, {6, 100}},
                             .label = "texts: line of real ban record " + std::to_string(at), .memory = filled(output, 0x100)});
        }
    }
    const auto line = [&](const Ban& b, std::uint32_t size, const std::string& label, std::vector<Write> extra = {}) {
        std::vector<Write> memory = filled(output, 0x100);
        append(memory, ban_writes(record_at, b));
        append(memory, extra);
        cases.push_back({.entry = ban_line, .registers = {{3, manager}, {4, record_at}, {5, output}, {6, size}}, .label = "texts: line " + label,
                         .memory = memory});
    };
    const std::uint16_t today_early = std::uint16_t(today >> 16), today_late = std::uint16_t(today);
    // Date relations: the later halfword ahead, behind, equal with the earlier one above, below, equal and of mixed signs.
    struct Relation { std::uint16_t early, late; const char* what; };
    const Relation relations[] = {{today_early, std::uint16_t(today_late + 1), "starts later (late halfword above)"},
                                  {today_early, std::uint16_t(today_late - 1), "started earlier (late halfword below)"},
                                  {today_early, today_late, "same day"},
                                  {std::uint16_t(today_early + 1), today_late, "same late halfword, early above"},
                                  {std::uint16_t(today_early - 1), today_late, "same late halfword, early below"},
                                  {0x8000, today_late, "same late halfword, early negative"},
                                  {0x7fff, today_late, "same late halfword, early at the maximum"},
                                  {0xffff, today_late, "same late halfword, early -1"},
                                  {0, today_late, "same late halfword, early 0"},
                                  {0x8000, std::uint16_t(today_late + 1), "late above, early negative"}};
    for (const auto& r : relations)
        if (!ahead(today_early, today_late, r.early, r.late) || calendar_day(r.early))
        for (int length : {0, -3, 1, 2, 5})
            for (std::uint8_t flag : {0, 0x40})
                line({.length = std::int8_t(length), .date = 9, .early = r.early, .late = r.late, .bodies = {3, 5, -1, -1}, .flag_byte = flag}, 100,
                     std::string(r.what) + ", length " + std::to_string(length) + (flag ? ", flag" : ""));
    // The relation with a different today: today's early halfword moved over the signed range.
    for (std::uint16_t early : {std::uint16_t(0), std::uint16_t(0x7fff), std::uint16_t(0x8000), std::uint16_t(0xffff), std::uint16_t(0x4000)})
        for (std::uint16_t ban_early : {std::uint16_t(0), std::uint16_t(1), std::uint16_t(0x7fff), std::uint16_t(0x8000), std::uint16_t(0xffff), std::uint16_t(0xc000)})
            if (!ahead(early, today_late, ban_early, today_late) || calendar_day(ban_early))
            line({.length = 3, .early = ban_early, .late = today_late, .bodies = {3, 5, -1, -1}}, 100,
                 "today's early " + std::to_string(early) + " against " + std::to_string(ban_early), {{2, date_at, early}});
    for (unsigned pattern = 1; pattern < 16; pattern += 2)  // bodies patterns where the first is named
        for (int length : {0, 1, 4})
            for (std::uint8_t flag : {0, 0x40}) {
                Ban b{.length = std::int8_t(length), .early = today_early, .late = std::uint16_t(today_late + 1), .flag_byte = flag};
                for (unsigned k = 0; k < 4; ++k) b.bodies[k] = (pattern >> k & 1) ? std::int8_t(3 + 7 * k) : std::int8_t(-1);
                line(b, 100, "bodies pattern " + std::to_string(pattern) + ", length " + std::to_string(length) + (flag ? ", flag" : ""));
            }
    line({.length = 2, .bodies = {3, 5, -1, -1}}, 0x31, "size 0x31 (assertion)");
    line({.length = 2, .bodies = {3, 5, -1, -1}}, 0x32, "size 0x32");
    line({.length = 2, .bodies = {3, 5, -1, -1}}, 0xffffffffu, "size -1 (assertion)");
    line({.length = -128, .date = -5, .bodies = {3, 5, 8, -1}, .flag_byte = 0xff}, 100, "length -128, negative date, all flag bits");
    line({.length = 127, .bodies = {3, 5, 8, 9}, .flag_byte = 0x40}, 100, "length 127, four bodies, flag (refused)");
    line({.length = 127, .early = today_early, .late = std::uint16_t(today_late + 1), .bodies = {3, 5, 8, 9}, .flag_byte = 0x40}, 100,
         "ahead, four bodies, flag (refused)");

    // ---- 0x100e7780: a status record (kind at +0x10, count at +0x12) in scratch.
    for (int kind : {5, 6, 0, 1, 4, 2, 3, 7, -1, 8, 0x7f})
        for (int count : {1, 2, 0, -3, 0x7fff, -32768}) {
            std::vector<Write> memory = filled(output, 0x80);
            memory.push_back({1, record_at + 0x10, std::uint32_t(kind) & 0xff});
            memory.push_back({2, record_at + 0x12, std::uint32_t(count) & 0xffff});
            cases.push_back({.entry = status_text, .registers = {{3, manager}, {4, record_at}, {5, output}, {6, 100}},
                             .label = "texts: status kind " + std::to_string(kind) + ", count " + std::to_string(count), .memory = memory});
        }
    for (std::uint32_t size : {0xeu, 0xfu, 0xffffffffu}) {
        std::vector<Write> memory = filled(output, 0x80);
        memory.push_back({1, record_at + 0x10, 3});
        cases.push_back({.entry = status_text, .registers = {{3, manager}, {4, record_at}, {5, output}, {6, size}},
                         .label = "texts: status size " + std::to_string(std::int32_t(size)), .memory = memory});
    }
}

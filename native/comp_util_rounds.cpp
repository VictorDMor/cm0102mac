// COMP_UTIL.CPP, blocks `round_codes` and the first routine of `round_text`: what the game knows of a round code.
// A round code (a halfword: 0..44 a group of rounds, 50..94 and 100..155 the next stretches, 160..374, then
// 1000..1155 the single rounds, and gaps between them) has a name text in each language (0x10469bb0) and, for
// the languages that inflect their words, a small class of that name (0x1046f1f0: inferred, the name's grammatical
// gender: the texts pass it twice to the localized format beside the name). Both are tables the original compiled
// as 74 and 27 jump tables; native/comp_util_rounds_tables.inc holds their data, drafted from the instruction
// database by tools/comp_util_tables.py (the tool classifies every arm by shape and fails on any other, and
// checks that its emulation of the range checks reaches every table entry). This file is the typed function
// around them: the range checks, the lookup and the calls in the original's order.
#include "comp_util_detail.hpp"
#include <span>
#include <string_view>

namespace cm::aot::comp_util {
namespace {
constexpr std::uint32_t sprintf_entry = 0x1077b720, format_entry = 0x10727890, copy_entry = 0x1077dbf0;
// The unit's string pool (TOC -0x47e4): the empty text and the error text the routines report out of range.
constexpr std::uint32_t empty_text = 0x5380, error_text = 0x5565;
// TOC slots: the preferences record (its +8 is the language), the shared message buffers of the game.
constexpr std::int32_t preferences_slot = -0x7ac8, error_buffer_slot = -0x7bb8, success_flag_slot = -0x7bac, assert_buffer_slot = -0x7ba8;
constexpr std::uint32_t language_offset = 0x08;  // the preferences record's language byte (GAME_CONFIG.CPP)

// ---- the data (comp_util_rounds_tables.inc) -------------------------------------------------------------------
// 0x1046f1f0: a stretch of codes (first..last) is a table (a digit per code, '-' for the table's error arm) or
// a gap; `error_site` is the address after the `bl` of the error report (0: none, the original returns 0).
struct NumberSpan { std::int32_t first, last; std::string_view numbers; std::uint32_t error_site; };
struct NumberVariant { std::int32_t language; bool kind_three; std::span<const NumberSpan> spans; };
// 0x10469bb0: each arm of the original ends in one call.
enum class Call : std::uint8_t {
    format,             // the localized format of the pool string at `key`, the code as its first value
    format_with_empty,  // the same with the empty text as the first value
    copy,               // sprintf of the empty text
    error,              // sprintf of the error text into the game's shared buffer, the result untouched
};
struct TextArm { std::uint32_t site; std::uint16_t key; Call call; };
struct TextSpan { std::int32_t first, last; std::span<const std::uint16_t> arms; std::uint32_t error_site; };
struct TextVariant { std::int32_t kind; std::span<const TextSpan> spans; };
#include "comp_util_rounds_tables.inc"

template <class Span>
const Span* span_of(std::span<const Span> spans, std::int32_t code) {
    for (const auto& span : spans)
        if (code >= span.first && code <= span.last) return &span;
    return nullptr;  // the spans cover every halfword: not reached
}
}

std::int32_t round_gender(Cpu& c, std::uint32_t code_register, std::uint32_t kind_register) {
    Frame frame(c, 0x1046f1f0, 0x50, {.first_gpr = 27});
    // r27 the preferences cell, r28 the code and r29 the kind as passed, r30 the pool, r31 the error buffer.
    c.set({{27, c.toc(preferences_slot)}, {28, code_register}, {29, kind_register}, {30, c.toc(-0x47e4)}, {31, c.toc(error_buffer_slot)}});
    const auto language = std::int32_t(c.u8(c.u32(c.r(27)) + language_offset));
    const bool kind_three = sext8(std::uint8_t(kind_register)) == 3;
    const NumberVariant* variant = nullptr;
    for (const auto& candidate : number_variants)
        if (candidate.language == language && candidate.kind_three == kind_three) variant = &candidate;
    // The other languages' branch: nothing is tested and 0 answered, r5 untouched.
    if (!variant) {
        c.set(3, 0);
        return 0;
    }
    const auto code = std::int32_t(sext16(std::uint16_t(code_register)));
    c.set(5, std::uint32_t(code));
    const auto* span = span_of(variant->spans, code);
    std::int32_t value = 0;
    std::uint32_t error_site = span->error_site;
    if (!span->numbers.empty()) {
        const char digit = span->numbers[std::size_t(code - span->first)];
        if (digit != '-') value = digit - '0', error_site = 0;
    }
    // An error: the report text goes to the shared buffer and the answer is 0. Some languages' gaps (the first
    // one's codes from 1156 on for kind 3) answer 0 without a report.
    if (error_site != 0) {
        c.set({{3, c.r(31)}, {4, c.r(30) + error_text}});
        c.call(sprintf_entry, error_site);
        value = 0;
    }
    c.set(3, std::uint32_t(value));
    return value;
}

std::uint32_t competition_gender(Cpu& c, std::uint32_t record, std::uint32_t second_code_register, std::uint32_t code_register, std::uint32_t kind_register) {
    Frame frame(c, 0x10476ec0, 0x40, {});
    // A round code (the first, else the second) decides; without one the record's own gender byte: the long
    // name's for kind 1, the short name's otherwise, 1 for no record.
    std::uint32_t result;
    if (sext16(std::uint16_t(code_register)) != 0) {
        c.set({{3, code_register}, {4, 0}});
        c.link(0x10476ee0);
        result = std::uint32_t(round_gender(c, code_register, 0));
    } else if (sext16(std::uint16_t(second_code_register)) != 0) {
        c.set({{3, second_code_register}, {4, 0}});
        c.link(0x10476ef8);
        result = std::uint32_t(round_gender(c, second_code_register, 0));
    } else if (sext8(std::uint8_t(kind_register)) == 1 && record != 0) {
        result = c.u8(record + comp::record::name_gender);
    } else if (record != 0) {
        result = c.u8(record + comp::record::short_name_gender);
    } else {
        result = 1;
    }
    c.set(3, result);
    return result;
}

std::uint32_t round_text(Cpu& c, std::uint32_t buffer, std::uint32_t size_register, std::uint32_t code_register, std::uint32_t kind_register) {
    Frame frame(c, 0x10469bb0, 0xb0, {.first_gpr = 24});
    const auto pool = c.toc(-0x47e4);
    c.set({{24, size_register}, {25, code_register}, {26, kind_register}, {27, c.toc(success_flag_slot)}, {28, c.toc(assert_buffer_slot)},
           {29, c.toc(error_buffer_slot)}, {30, pool}, {31, buffer}});
    // The buffer starts as the empty text, whatever follows.
    c.set({{3, buffer}, {4, pool + empty_text}});
    c.call(copy_entry, 0x10469be8);
    if (std::int32_t(size_register) < 30) {
        guest::report_assertion(c, {0x10469c04, pool, pool + 0xe, pool + 0x1b, frame.at(0x68), frame.at(0x40), 0x110f});
        c.set(3, buffer);
        return buffer;
    }
    const auto kind = std::int32_t(sext8(std::uint8_t(kind_register)));
    const TextVariant* variant = nullptr;
    for (const auto& candidate : text_variants)
        if (candidate.kind == kind) variant = &candidate;
    if (!variant) {
        guest::report_assertion(c, {0x1046f198, pool, pool + 0xe, pool + 0x1b, frame.at(0x48), frame.at(0x38), 0x15e2});
        c.set(3, buffer);
        return buffer;
    }
    const auto code = std::int32_t(sext16(std::uint16_t(code_register)));
    c.set(5, std::uint32_t(code));
    const auto* span = span_of(variant->spans, code);
    TextArm arm{span->error_site, 0, Call::error};
    if (!span->arms.empty()) arm = text_arms[span->arms[std::size_t(code - span->first)]];
    switch (arm.call) {
    case Call::format:
        c.set({{3, buffer}, {4, pool + arm.key}});
        c.call(format_entry, arm.site);
        break;
    case Call::format_with_empty:
        c.set({{3, buffer}, {4, pool + arm.key}, {5, pool + empty_text}});
        c.call(format_entry, arm.site);
        break;
    case Call::copy:
        c.set({{3, buffer}, {4, pool + arm.key}});
        c.call(sprintf_entry, arm.site);
        break;
    case Call::error:
        // The error text goes to the game's shared buffer; the caller's buffer keeps the empty text.
        c.set({{3, c.r(29)}, {4, pool + error_text}});
        c.call(sprintf_entry, arm.site);
        break;
    }
    c.set(3, buffer);
    return buffer;
}
}

#include "calendar_adapter.hpp"
#include "calendar.hpp"
#include <sstream>
#include <algorithm>
#include <zlib.h>
#include <cstdio>
#include <cstdlib>

namespace cm::aot {
namespace {
gzFile audit = nullptr;
bool audit_record = false, audit_original_running = false, audit_perturb = false;
bool audit_perturb_difference = false;
bool audit_perturb_ordering = false, audit_perturb_before = false, audit_include_before = false;
std::uint64_t audit_index = 0;
gzFile month_audit = nullptr;
bool month_record = false, month_perturb = false;
std::uint64_t month_index = 0;
using cm::calendar::Date;
using cm::calendar::Direction;
struct Boundary {
    std::uint32_t entry, scratch_slot, initialization, delegated_return;
    Direction direction;
};
constexpr Boundary addition{0x10717890, 0x108b7ae4, 0x108b84d5, 0x10717918, Direction::forward};
constexpr Boundary subtraction{0x10717b20, 0x108b7adc, 0x108b84d3, 0x10717ba8, Direction::backward};
constexpr std::uint32_t difference_entry = 0x10718130;
constexpr std::uint32_t difference_scratch = 0x108b7ad0, difference_flag = 0x108b84d0;
constexpr std::uint32_t ordering_entry = 0x10331860, before_entry = 0x103317f0;
constexpr std::uint32_t month_entry = 0x10718360;
void execute_month(State& s) {
    const auto date = std::uint32_t(s.r<4>(12));
    const auto flag = std::uint8_t(s.memory.read<1>(date + 4));
    const auto signed_flag = std::bit_cast<std::int8_t>(flag);
    s.wr<4>(0, std::uint32_t(int(signed_flag)));
    s.wr<1>(2304, (signed_flag < 0 ? 8 : signed_flag > 0 ? 4 : 2) | (s.r<1>(1024) & 1));
    const auto slot = std::uint32_t(s.r<4>(8)) - (flag ? 0x3fa4u : 0x3fa8u);
    s.wr<4>(16, s.memory.read<4>(slot));
    s.wr<4>(0, 2); s.wr<4>(16, s.r<4>(16) + 22);
    s.wr<4>(28, 11); s.wr<4>(24, 22); s.wr<4>(4132, 2);
    for (unsigned block = 0; block < 2; ++block) {
        for (unsigned index = 0; index < 6; ++index) {
            const auto threshold = cm::calendar::narrow_day(int(s.memory.read<2>(
                std::uint32_t(s.r<4>(16)) - index * 2)));
            s.wr<4>(20, std::uint32_t(int(threshold)));
            // Each group's first threshold precedes a fresh day load. The
            // remaining five probes reuse it and decrement the month register.
            if (!index)
                s.wr<4>(0, std::uint32_t(int(cm::calendar::narrow_day(int(s.memory.read<2>(date))))));
            else if (index) s.wr<4>(28, s.r<4>(28) - 1);
            const auto day = cm::calendar::narrow_day(int(s.r<2>(2)));
            s.wr<1>(2304, (threshold < day ? 8 : threshold > day ? 4 : 2) | (s.r<1>(1024) & 1));
            if (cm::calendar::recognizes_month(day, threshold)) {
                s.wr<4>(12, s.r<4>(28));
                s.pc = std::uint32_t(s.r<4>(4128)); return;
            }
        }
        s.wr<4>(24, s.r<4>(24) - 10); s.wr<4>(16, s.r<4>(16) - 12);
        s.wr<4>(28, s.r<4>(28) - 1); s.wr<4>(4132, s.r<4>(4132) - 1);
    }
    s.wr<4>(12, 0xffffffffu); s.pc = std::uint32_t(s.r<4>(4128));
}
// r4 and XER carry are observable clobbers of the original leap helper. Its
// unsigned 16-bit year makes both signed shifts nonnegative. Preserve these
// effects even though the arithmetic module uses ordinary Gregorian divisibility.
void leap_clobbers(State& s, std::uint16_t year) {
    s.wr<4>(2440, s.r<4>(8)); // Ghidra call-context TOC register.
    s.wr<4>(20, year);
    if (year % 4 == 0) {
        s.wr<4>(16, (UINT64_C(0x51eb851f) * year) >> 32);
        s.wr<1>(1027, 0);
    }
    s.wr<4>(12, cm::calendar::is_leap(year));
    s.wr<1>(2304, (cm::calendar::is_leap(year) ? 4 : 2) | (s.r<1>(1024) & 1));
}
Date read_date(const State& s, std::uint32_t address) {
    return {cm::calendar::narrow_day(int(s.memory.read<2>(address))),
            std::uint16_t(s.memory.read<2>(address + 2))};
}
void copy_date(State& s, std::uint32_t output, std::uint32_t input) {
    // Ordered loads/stores retain partial alias effects; no memcpy or prefetch.
    s.memory.write<2>(output, s.memory.read<2>(input));
    s.memory.write<2>(output + 2, s.memory.read<2>(input + 2));
    s.memory.write<1>(output + 4, s.memory.read<1>(input + 4));
}
void execute_ordering(State& s, bool before = false) {
    const auto stack = std::uint32_t(s.r<4>(4));
    const auto first = std::uint32_t(s.r<4>(12));
    // No frame or scratch. Spills precede reads, including aliased first dates.
    s.memory.write<4>(stack + 28, s.r<4>(16));
    s.memory.write<4>(stack + 32, s.r<4>(20));
    auto second_year = std::uint16_t(s.memory.read<2>(stack + 30));
    auto first_year = std::uint16_t(s.memory.read<2>(first + 2));
    if (before ? second_year >= first_year : second_year <= first_year) {
        // The reference repeats these loads on >= (before) or <= (after).
        second_year = std::uint16_t(s.memory.read<2>(stack + 30));
        first_year = std::uint16_t(s.memory.read<2>(first + 2));
    }
    s.wr<4>(0, first_year); s.wr<4>(16, second_year);
    const unsigned comparison = second_year > first_year ? 4 : second_year < first_year ? 8 : 2;
    s.wr<1>(2304, comparison | (s.r<1>(1024) & 1));
    const auto predicate = before ? cm::calendar::is_before : cm::calendar::is_after;
    bool result = predicate({0, second_year}, {0, first_year});
    if (second_year == first_year) {
        const auto second_day = cm::calendar::narrow_day(int(s.memory.read<2>(stack + 28)));
        const auto first_day = cm::calendar::narrow_day(int(s.memory.read<2>(first)));
        result = predicate({second_day, second_year}, {first_day, first_year});
        const auto bits = std::uint32_t(int(second_day)) ^ std::uint32_t(int(first_day));
        // Preserve xor/shift clobbers separately from the semantic predicate.
        // Unsigned shift avoids undefined behavior for negative input bits.
        s.wr<4>(16, (bits >> 1) | (bits & 0x80000000u));
        s.wr<4>(20, std::uint32_t(int(before ? first_day : second_day)));
        s.wr<1>(1027, (bits >> 31) && (bits & 1));
        s.wr<4>(0, result);
    }
    s.wr<4>(12, result);
    s.pc = std::uint32_t(s.r<4>(4128));
}
void execute_difference(State& s) {
    const auto stack = std::uint32_t(s.r<4>(4));
    const auto link = std::uint32_t(s.r<4>(4128));
    const auto first = std::uint32_t(s.r<4>(12));
    s.memory.write<4>(stack - 4, s.r<4>(124));
    const auto scratch = std::uint32_t(s.memory.read<4>(difference_scratch));
    s.memory.write<4>(stack - 8, s.r<4>(120));
    s.memory.write<4>(stack - 12, s.r<4>(116));
    s.memory.write<4>(stack - 16, s.r<4>(112));
    s.memory.write<4>(stack + 8, link);
    s.memory.write<4>(stack - 80, stack);
    s.wr<4>(4, stack - 80);
    // Eight bytes by value: the five-byte date plus all three padding bytes.
    s.memory.write<4>(stack + 28, s.r<4>(16));
    s.memory.write<4>(stack + 32, s.r<4>(20));
    s.wr<4>(24, scratch); s.wr<4>(120, first);
    if (!s.memory.read<1>(difference_flag)) {
        s.memory.write<2>(scratch, 40); s.memory.write<2>(scratch + 2, 1972);
        s.memory.write<1>(scratch + 4, 1); s.memory.write<1>(difference_flag, 1);
    }
    s.memory.write<4>(stack - 24, s.memory.read<4>(stack + 28));
    const auto first_year = std::uint16_t(s.memory.read<2>(first + 2));
    const auto second_year = std::uint16_t(s.memory.read<2>(stack - 22));
    s.memory.write<4>(stack - 20, s.memory.read<4>(stack + 32));
    s.wr<4>(20, first_year);
    bool later = first_year < second_year;
    if (first_year == second_year) {
        const auto second_day = cm::calendar::narrow_day(int(s.memory.read<2>(stack - 24)));
        const auto first_day = cm::calendar::narrow_day(int(s.memory.read<2>(first)));
        later = cm::calendar::difference_second_later({first_day, first_year}, {second_day, second_year});
        s.wr<4>(16, std::uint32_t(int(second_day)));
        const auto bits = std::uint32_t(int(first_day)) ^ std::uint32_t(int(second_day));
        s.wr<1>(1027, (bits >> 31) && (bits & 1));
    }
    std::int32_t total = 0;
    if (first_year != second_year) {
        // Do not preload the day/flag: the year store may alias the first date.
        s.memory.write<2>(scratch + 2, first_year);
        s.memory.write<2>(scratch, s.memory.read<2>(first));
        s.memory.write<1>(scratch + 4, s.memory.read<1>(first + 4));
        auto current = std::uint16_t(s.memory.read<2>(scratch + 2));
        while (later ? current < second_year : current > second_year) {
            const auto leap_year = std::uint16_t(current - 1);
            leap_clobbers(s, leap_year);
            total += cm::calendar::difference_year_step(current, later);
            current = std::uint16_t(int(current) + (later ? 1 : -1));
            s.memory.write<2>(scratch + 2, current);
            current = std::uint16_t(s.memory.read<2>(scratch + 2));
        }
    }
    // The original always reloads both days after the ordered scratch writes.
    const auto final_second_day = cm::calendar::narrow_day(int(s.memory.read<2>(stack + 28)));
    const auto final_first_day = cm::calendar::narrow_day(int(s.memory.read<2>(first)));
    total += int(final_first_day) - int(final_second_day);
    s.wr<4>(12, std::uint32_t(total));
    s.wr<1>(2304, 2 | (s.r<1>(1024) & 1)); // Final unsigned year comparison.
    s.wr<4>(0, s.memory.read<4>(stack + 8)); s.wr<4>(4128, s.r<4>(0));
    s.wr<4>(4, stack);
    for (unsigned reg : {124u, 120u, 116u, 112u})
        s.wr<4>(reg, s.memory.read<4>(stack - (128 - reg)));
    s.pc = std::uint32_t(s.r<4>(4128));
}
void execute(State& s, const Boundary& boundary) {
    const auto stack = std::uint32_t(s.r<4>(4));
    const auto link = std::uint32_t(s.r<4>(4128));
    const auto destination = std::uint32_t(s.r<4>(12));
    const auto source = std::uint32_t(s.r<4>(16));
    const auto offset = cm::calendar::narrow_day(int(s.r<2>(22)));
    // Retain the exact legacy frame bytes and nonvolatile-register convention.
    s.memory.write<4>(stack - 4, s.r<4>(124));
    const auto scratch = std::uint32_t(s.memory.read<4>(boundary.scratch_slot));
    s.memory.write<4>(stack - 8, s.r<4>(120));
    s.memory.write<4>(stack - 12, s.r<4>(116));
    s.memory.write<4>(stack - 16, s.r<4>(112));
    s.memory.write<4>(stack + 8, link);
    s.memory.write<4>(stack - 80, stack);
    s.wr<4>(4, stack - 80);
    s.wr<4>(124, scratch); s.wr<4>(120, destination);
    if (!s.memory.read<1>(boundary.initialization)) {
        s.memory.write<2>(scratch, 40); s.memory.write<2>(scratch + 2, 1972);
        s.memory.write<1>(scratch + 4, 1); s.memory.write<1>(boundary.initialization, 1);
    }
    if (offset < 0) {
        s.memory.write<2>(scratch + 2, s.memory.read<2>(source + 2));
        s.wr<4>(116, scratch + 2); s.wr<4>(112, scratch + 4);
        s.memory.write<2>(scratch, s.memory.read<2>(source));
        s.memory.write<1>(scratch + 4, s.memory.read<1>(source + 4));
        s.wr<4>(12, stack - 24); s.wr<4>(16, scratch);
        s.wr<4>(20, std::uint32_t(-int(offset)));
        s.wr<4>(4128, boundary.delegated_return);
        execute(s, boundary.direction == Direction::forward ? subtraction : addition);
        s.memory.write<2>(scratch + 2, s.memory.read<2>(stack - 22));
        const auto leap = s.memory.read<1>(stack - 20);
        const auto day = s.memory.read<2>(stack - 24);
        s.wr<4>(12, std::uint32_t(cm::calendar::narrow_day(int(day))));
        s.memory.write<2>(scratch, day); s.memory.write<1>(scratch + 4, leap);
    } else {
        s.wr<4>(112, scratch + 2);
        s.memory.write<2>(scratch + 2, s.memory.read<2>(source + 2));
        auto date = cm::calendar::shift_within_record(read_date(s, source),
            std::uint16_t(offset), boundary.direction);
        // The year was loaded before the day, and can overlap the source day.
        date.year = std::uint16_t(s.memory.read<2>(scratch + 2));
        s.memory.write<2>(scratch, std::uint16_t(date.day));
        for (;;) {
            date = read_date(s, scratch);
            leap_clobbers(s, boundary.direction == Direction::forward ? date.year : std::uint16_t(date.year - 1));
            if (!cm::calendar::crosses_year(date, boundary.direction)) break;
            date = cm::calendar::cross_year(date, boundary.direction);
            s.memory.write<2>(scratch, std::uint16_t(date.day));
            s.memory.write<2>(scratch + 2, date.year);
        }
        leap_clobbers(s, std::uint16_t(s.memory.read<2>(scratch + 2)));
        s.memory.write<1>(scratch + 4, s.r<4>(12));
    }
    copy_date(s, destination, scratch);
    s.wr<4>(0, s.memory.read<4>(stack + 8)); s.wr<4>(4128, s.r<4>(0));
    s.wr<4>(4, stack);
    s.wr<4>(124, s.memory.read<4>(stack - 4));
    s.wr<4>(120, s.memory.read<4>(stack - 8));
    s.wr<4>(116, s.memory.read<4>(stack - 12));
    s.wr<4>(112, s.memory.read<4>(stack - 16));
    s.pc = std::uint32_t(s.r<4>(4128));
}
}
bool try_modern_calendar(State& s, std::uint32_t entry) {
    if (entry == month_entry) {
        const bool original = s.calendar_original || s.date_month_original;
        if (audit_original_running || (original && !month_audit)) return false;
        const auto date = std::uint32_t(s.r<4>(12)), link = std::uint32_t(s.r<4>(4128));
        std::vector<std::uint8_t> edge;
        auto append = [&](std::uint64_t value, unsigned width) {
            for (unsigned i = width; i; --i) edge.push_back(std::uint8_t(value >> ((i - 1) * 8)));
        };
        const bool sample = !original && s.date_month_calls < 32 && std::getenv("CM_AOT_VALIDATION_EVENT_POINTER");
        const auto input = month_audit || sample ? s.memory.copy_out(date, 5) : std::vector<std::uint8_t>{};
        const auto table = month_audit || sample ? std::uint32_t(s.memory.read<4>(
            std::uint32_t(s.r<4>(8)) - (input[4] ? 0x3fa4u : 0x3fa8u))) : 0;
        if (month_audit) {
            append(entry, 4); append(date, 4); append(link, 4);
            edge.insert(edge.end(), input.begin(), input.end()); append(table, 4);
            const auto thresholds = s.memory.copy_out(table, 24);
            edge.insert(edge.end(), thresholds.begin(), thresholds.end());
        }
        if (original) {
            audit_original_running = true; s.pc = entry;
            try {
                do { if (!dispatch_page(s)) throw std::runtime_error("Uncompiled original month reference"); }
                while (s.pc != link);
            } catch (...) { audit_original_running = false; throw; }
            audit_original_running = false;
        } else {
            s.step(entry); ++s.date_month_calls; execute_month(s);
            if (month_perturb && s.date_month_calls == 1) s.wr<4>(12, s.r<4>(12) + 1);
            if (sample)
                std::fprintf(stderr, "date-month.modern entry=%08x sample=%llu date=%u/%d flag=%u table=%08x result=%d return=%08x\n",
                    entry, static_cast<unsigned long long>(s.date_month_calls), unsigned(read_be<2>(input.data()+2)),
                    int(cm::calendar::narrow_day(int(read_be<2>(input.data())))), unsigned(input[4]), table,
                    int(std::bit_cast<std::int32_t>(std::uint32_t(s.r<4>(12)))), link);
        }
        if (month_audit) {
            append(s.r<4>(12), 4);
            edge.insert(edge.end(), s.registers.begin(), s.registers.end()); append(s.pc, 4);
            if (month_record) {
                if (gzwrite(month_audit, edge.data(), unsigned(edge.size())) != int(edge.size()))
                    throw std::runtime_error("Cannot record private month edges");
            } else {
                std::vector<std::uint8_t> expected(edge.size());
                if (gzread(month_audit, expected.data(), unsigned(expected.size())) != int(expected.size()))
                    throw std::runtime_error("First divergence: month call count/truncated edge reference");
                const auto mismatch = std::mismatch(expected.begin(), expected.end(), edge.begin());
                if (mismatch.first != expected.end()) {
                    const auto byte = mismatch.first - expected.begin();
                    throw std::runtime_error("First divergence: month edge " + std::to_string(month_index) +
                        " at " + hex_address(entry) + " field=" + (byte >= 45 && byte < 49 ? "month return" : "month input/table/ABI/PC") +
                        " byte=" + std::to_string(byte) + " expected=" + std::to_string(*mismatch.first) +
                        " actual=" + std::to_string(*mismatch.second));
                }
            }
            ++month_index;
        }
        return true;
    }
    const bool difference = entry == difference_entry;
    const bool before = entry == before_entry;
    const bool ordering = entry == ordering_entry || before;
    const bool audited = audit && (!before || audit_include_before);
    const bool original = s.calendar_original || (difference && s.date_difference_original) ||
                          (before ? s.date_before_original : ordering && s.date_ordering_original);
    if (audit_original_running || (original && !audited)) return false;
    const auto return_pc = std::uint32_t(s.r<4>(4128));
    const auto destination = std::uint32_t(s.r<4>(12));
    std::vector<std::uint8_t> edge;
    auto append = [&](std::uint64_t value, unsigned width) {
        for (unsigned i = width; i; --i) edge.push_back(std::uint8_t(value >> ((i - 1) * 8)));
    };
    if (audited) {
        append(entry, 4);
        for (unsigned r : {12u, 16u, 20u, 4u, 8u, 4128u}) append(s.r<4>(r), 4);
        const auto input = s.memory.copy_out(std::uint32_t(s.r<4>(difference || ordering ? 12 : 16)), 5);
        edge.insert(edge.end(), input.begin(), input.end());
    }
    if (!difference && !ordering && cm::calendar::narrow_day(int(s.r<2>(22))) == -32768) {
        // Negating SHRT_MIN delegates indefinitely in the reference. Preserve
        // its bounded stack/budget failure instead of inventing a date result.
        ++s.calendar_exceptional_calls; return false;
    }
    if (original) {
        audit_original_running = true; s.pc = entry;
        try {
            do { if (!dispatch_page(s)) throw std::runtime_error("Uncompiled original calendar reference"); }
            while (s.pc != return_pc);
        } catch (...) { audit_original_running = false; throw; }
        audit_original_running = false;
    } else {
        s.step(entry); // Actual semantic dispatch, not fabricated original accounting.
        if (ordering) {
            auto& calls = before ? s.date_before_calls : s.date_ordering_calls;
            const bool sample = calls < 32 && std::getenv("CM_AOT_VALIDATION_EVENT_POINTER");
            const auto packed = sample ? s.r<4>(20) : 0;
            const auto stack = std::uint32_t(s.r<4>(4));
            ++calls;
            execute_ordering(s, before);
            const auto first = sample ? read_date(s, destination) : Date{};
            const auto second = sample ? read_date(s, stack + 28) : Date{};
            if (((before && audit_perturb_before) || (!before && audit_perturb_ordering)) && calls == 1)
                s.wr<4>(12, s.r<4>(12) ^ 1);
            if (sample)
                std::fprintf(stderr, "%s.modern entry=%08x sample=%llu first=%u/%d second=%u/%d second-tail=%08x result=%u return=%08x\n",
                    before ? "date-before" : "date-ordering", entry, static_cast<unsigned long long>(calls), first.year, first.day, second.year, second.day,
                    unsigned(packed), unsigned(s.r<4>(12)), return_pc);
        } else if (difference) {
            const bool sample = s.date_difference_calls < 32 && std::getenv("CM_AOT_VALIDATION_EVENT_POINTER");
            const auto first = sample ? read_date(s, destination) : Date{};
            const auto second = sample ? Date{cm::calendar::narrow_day(int(s.r<2>(16))), std::uint16_t(s.r<2>(18))} : Date{};
            const auto packed = sample ? s.r<4>(20) : 0;
            ++s.date_difference_calls;
            execute_difference(s);
            if (audit_perturb_difference && s.date_difference_calls == 1)
                s.wr<4>(12, s.r<4>(12) + 1);
            // Bounded private samples from real application calls; no UI option.
            if (sample)
                std::fprintf(stderr, "date-difference.modern sample=%llu first=%u/%d second=%u/%d second-tail=%08x result=%d return=%08x\n",
                    static_cast<unsigned long long>(s.date_difference_calls), first.year, first.day, second.year, second.day,
                    unsigned(packed), int(std::bit_cast<std::int32_t>(std::uint32_t(s.r<4>(12)))), return_pc);
        } else {
            ++s.calendar_calls[entry == addition.entry ? 0 : 1];
            execute(s, entry == addition.entry ? addition : subtraction);
        }
        if (!difference && !ordering && audit_perturb && s.calendar_calls[0] + s.calendar_calls[1] == 1)
            s.memory.write<2>(destination, s.memory.read<2>(destination) + 1);
    }
    if (audited) {
        if (ordering) {
            append(s.r<4>(12), 4);
            // Complete registers and eight spill bytes. Direct checks also
            // compare all temporaries and every mapped memory byte.
            edge.insert(edge.end(), s.registers.begin(), s.registers.end());
            const auto spills = s.memory.copy_out(std::uint32_t(s.r<4>(4)) + 28, 8);
            edge.insert(edge.end(), spills.begin(), spills.end());
        } else if (difference) {
            append(s.r<4>(12), 4);
            const auto scratch = s.memory.copy_out(std::uint32_t(s.memory.read<4>(difference_scratch)), 5);
            edge.insert(edge.end(), scratch.begin(), scratch.end());
            append(s.memory.read<1>(difference_flag), 1);
        } else {
            for (const auto address : {destination, std::uint32_t(s.memory.read<4>(addition.scratch_slot)),
                                  std::uint32_t(s.memory.read<4>(subtraction.scratch_slot))}) {
                const auto bytes = s.memory.copy_out(address, 5); edge.insert(edge.end(), bytes.begin(), bytes.end());
            }
            append(s.memory.read<1>(addition.initialization), 1);
            append(s.memory.read<1>(subtraction.initialization), 1);
        }
        for (unsigned r : {0u, 12u, 16u, 20u, 4u, 8u, 112u, 116u, 120u, 124u, 4128u}) append(s.r<4>(r), 4);
        if (difference) { append(s.r<4>(24), 4); append(s.r<4>(2440), 4); }
        for (unsigned r : {2304u, 1024u, 1027u}) append(s.r<1>(r), 1);
        if (difference) {
            const auto stack = s.memory.copy_out(std::uint32_t(s.r<4>(4)) - 80, 116);
            edge.insert(edge.end(), stack.begin(), stack.end());
        }
        if (audit_record) {
            if (gzwrite(audit, edge.data(), unsigned(edge.size())) != int(edge.size()))
                throw std::runtime_error("Cannot record private calendar edges");
        } else {
            std::vector<std::uint8_t> expected(edge.size());
            if (gzread(audit, expected.data(), unsigned(expected.size())) != int(expected.size()))
                throw std::runtime_error("First divergence: calendar call count/truncated edge reference");
            const auto mismatch = std::mismatch(expected.begin(), expected.end(), edge.begin());
            if (mismatch.first != expected.end()) {
                const auto index = mismatch.first - expected.begin();
                const auto field = before ? (index < 28 ? "before entry/by-value argument" : index < 33 ? "first date" :
                    index < 37 ? "before return" : "before ABI/spills") : ordering ? (index < 28 ? "ordering entry/by-value argument" : index < 33 ? "first date" :
                    index < 37 ? "ordering return" : "ordering ABI/spills") : difference ? (index < 28 ? "difference entry/by-value argument" : index < 33 ? "first date" :
                    index < 37 ? "difference return" : index < 43 ? "difference scratch/flag" : "difference ABI/stack") :
                    index < 28 ? "entry/effective argument" : index < 33 ? "input date" :
                    index < 35 ? "output day-of-year" : index < 37 ? "output year" : index == 37 ? "output leap flag" :
                    index < 48 ? "scratch date" : index < 50 ? "initialization flag" : "direct ABI register";
                throw std::runtime_error("First divergence: calendar edge " + std::to_string(audit_index) +
                    " at " + hex_address(entry) + " field=" + field + " byte=" + std::to_string(index) +
                    " expected=" + std::to_string(*mismatch.first) + " actual=" + std::to_string(*mismatch.second));
            }
        }
        ++audit_index;
    }
    return true;
}
void open_calendar_audit(const std::filesystem::path& path, bool record, bool perturb, bool perturb_difference, bool perturb_ordering, bool include_before, bool perturb_before) {
    if (audit) throw std::runtime_error("Calendar audit already active");
    audit = gzopen(path.c_str(), record ? "wb1" : "rb");
    if (!audit) throw std::runtime_error("Cannot open private calendar edge reference");
    if (include_before) {
        constexpr std::array<char,8> magic{'C','M','C','A','L','0','0','2'};
        auto header = magic;
        const auto count = record ? gzwrite(audit, header.data(), header.size()) : gzread(audit, header.data(), header.size());
        if (count != int(header.size()) || header != magic) {
            gzclose(audit); audit = nullptr;
            throw std::runtime_error("Unrecognized private calendar v2 stream");
        }
    }
    audit_record = record; audit_index = 0; audit_perturb = perturb;
    audit_perturb_difference = perturb_difference;
    audit_perturb_ordering = perturb_ordering;
    audit_include_before = include_before; audit_perturb_before = perturb_before;
}
void close_calendar_audit(bool require_end) {
    if (!audit) return;
    bool extra = false;
    if (require_end && !audit_record) { std::uint8_t byte; extra = gzread(audit, &byte, 1) != 0 || !gzeof(audit); }
    const auto result = gzclose(audit); audit = nullptr; audit_perturb = false; audit_perturb_difference = false;
    audit_perturb_ordering = false; audit_perturb_before = false; audit_include_before = false;
    if (extra || result != Z_OK) throw std::runtime_error("First divergence: calendar call count or edge checksum");
}
std::string calendar_call_summary(const State& s) {
    std::ostringstream out;
    out << "calendar.mode=" << (s.calendar_original ? "original" : "modern")
        << " modern-add=" << s.calendar_calls[0] << " modern-subtract=" << s.calendar_calls[1]
        << " exceptional-original=" << s.calendar_exceptional_calls;
    out << " difference-mode=" << (s.calendar_original || s.date_difference_original ? "original" : "modern")
        << " modern-difference=" << s.date_difference_calls;
    out << " ordering-mode=" << (s.calendar_original || s.date_ordering_original ? "original" : "modern")
        << " modern-ordering=" << s.date_ordering_calls;
    out << " before-mode=" << (s.calendar_original || s.date_before_original ? "original" : "modern")
        << " modern-before=" << s.date_before_calls;
    out << " month-mode=" << (s.calendar_original || s.date_month_original ? "original" : "modern")
        << " modern-month=" << s.date_month_calls;
    return out.str();
}
void open_month_audit(const std::filesystem::path& path, bool record, bool perturb) {
    if (month_audit || (perturb && record)) throw std::runtime_error("Invalid private month audit mode");
    month_audit = gzopen(path.c_str(), record ? "wb1" : "rb");
    if (!month_audit) throw std::runtime_error("Cannot open private month edge reference");
    constexpr std::array<char,8> magic{'C','M','M','O','N','0','0','1'};
    auto header = magic;
    const auto count = record ? gzwrite(month_audit, header.data(), header.size()) : gzread(month_audit, header.data(), header.size());
    if (count != int(header.size()) || header != magic) {
        gzclose(month_audit); month_audit = nullptr;
        throw std::runtime_error("Unrecognized private month stream");
    }
    month_record = record; month_perturb = perturb; month_index = 0;
}
void close_month_audit(bool require_end) {
    if (!month_audit) return;
    bool extra = false;
    if (require_end && !month_record) { std::uint8_t byte; extra = gzread(month_audit, &byte, 1) != 0 || !gzeof(month_audit); }
    const auto result = gzclose(month_audit); month_audit = nullptr; month_perturb = false;
    if (extra || result != Z_OK) throw std::runtime_error("First divergence: month call count or edge checksum");
}
}

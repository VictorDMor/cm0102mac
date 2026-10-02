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
bool audit_perturb_ordering = false;
std::uint64_t audit_index = 0;
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
constexpr std::uint32_t ordering_entry = 0x10331860;
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
void execute_ordering(State& s) {
    const auto stack = std::uint32_t(s.r<4>(4));
    const auto first = std::uint32_t(s.r<4>(12));
    // No frame or scratch. Spills precede reads, including aliased first dates.
    s.memory.write<4>(stack + 28, s.r<4>(16));
    s.memory.write<4>(stack + 32, s.r<4>(20));
    auto second_year = std::uint16_t(s.memory.read<2>(stack + 30));
    auto first_year = std::uint16_t(s.memory.read<2>(first + 2));
    if (second_year <= first_year) {
        // The reference repeats these ordered loads on its <= branch.
        second_year = std::uint16_t(s.memory.read<2>(stack + 30));
        first_year = std::uint16_t(s.memory.read<2>(first + 2));
    }
    s.wr<4>(0, first_year); s.wr<4>(16, second_year);
    const unsigned comparison = second_year > first_year ? 4 : second_year < first_year ? 8 : 2;
    s.wr<1>(2304, comparison | (s.r<1>(1024) & 1));
    bool later = cm::calendar::is_after({0, second_year}, {0, first_year});
    if (second_year == first_year) {
        const auto second_day = cm::calendar::narrow_day(int(s.memory.read<2>(stack + 28)));
        const auto first_day = cm::calendar::narrow_day(int(s.memory.read<2>(first)));
        later = cm::calendar::is_after({second_day, second_year}, {first_day, first_year});
        const auto bits = std::uint32_t(int(second_day)) ^ std::uint32_t(int(first_day));
        // Preserve xor/shift clobbers separately from the semantic predicate.
        // Unsigned shift avoids undefined behavior for negative input bits.
        s.wr<4>(16, (bits >> 1) | (bits & 0x80000000u));
        s.wr<4>(20, std::uint32_t(int(second_day)));
        s.wr<1>(1027, (bits >> 31) && (bits & 1));
        s.wr<4>(0, later);
    }
    s.wr<4>(12, later);
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
    const bool difference = entry == difference_entry;
    const bool ordering = entry == ordering_entry;
    const bool original = s.calendar_original || (difference && s.date_difference_original) ||
                          (ordering && s.date_ordering_original);
    if (audit_original_running || (original && !audit)) return false;
    const auto return_pc = std::uint32_t(s.r<4>(4128));
    const auto destination = std::uint32_t(s.r<4>(12));
    std::vector<std::uint8_t> edge;
    auto append = [&](std::uint64_t value, unsigned width) {
        for (unsigned i = width; i; --i) edge.push_back(std::uint8_t(value >> ((i - 1) * 8)));
    };
    if (audit) {
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
            const bool sample = s.date_ordering_calls < 32 && std::getenv("CM_AOT_VALIDATION_EVENT_POINTER");
            const auto first = sample ? read_date(s, destination) : Date{};
            const auto second = sample ? Date{cm::calendar::narrow_day(int(s.r<2>(16))), std::uint16_t(s.r<2>(18))} : Date{};
            const auto packed = sample ? s.r<4>(20) : 0;
            ++s.date_ordering_calls;
            execute_ordering(s);
            if (audit_perturb_ordering && s.date_ordering_calls == 1)
                s.wr<4>(12, s.r<4>(12) ^ 1);
            if (sample)
                std::fprintf(stderr, "date-ordering.modern sample=%llu first=%u/%d second=%u/%d second-tail=%08x result=%u return=%08x\n",
                    static_cast<unsigned long long>(s.date_ordering_calls), first.year, first.day, second.year, second.day,
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
    if (audit) {
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
                const auto field = ordering ? (index < 28 ? "ordering entry/by-value argument" : index < 33 ? "first date" :
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
void open_calendar_audit(const std::filesystem::path& path, bool record, bool perturb, bool perturb_difference, bool perturb_ordering) {
    if (audit) throw std::runtime_error("Calendar audit already active");
    audit = gzopen(path.c_str(), record ? "wb1" : "rb");
    if (!audit) throw std::runtime_error("Cannot open private calendar edge reference");
    audit_record = record; audit_index = 0; audit_perturb = perturb;
    audit_perturb_difference = perturb_difference;
    audit_perturb_ordering = perturb_ordering;
}
void close_calendar_audit(bool require_end) {
    if (!audit) return;
    bool extra = false;
    if (require_end && !audit_record) { std::uint8_t byte; extra = gzread(audit, &byte, 1) != 0 || !gzeof(audit); }
    const auto result = gzclose(audit); audit = nullptr; audit_perturb = false; audit_perturb_difference = false;
    audit_perturb_ordering = false;
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
    return out.str();
}
}

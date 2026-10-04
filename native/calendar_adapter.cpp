#include "calendar_adapter.hpp"
#include "calendar.hpp"
#include "date_subsystem_adapter.hpp"
#include <sstream>
#include <algorithm>
#include <zlib.h>
#include <cstdio>
#include <cstdlib>

namespace cm::aot {
namespace {
gzFile audit = nullptr;
bool audit_record = false, audit_perturb = false;
unsigned history_depth = 0;
struct HistoryScope {HistoryScope(){++history_depth;}~HistoryScope(){--history_depth;}};
struct DatePause {
    State& state;MemoryObserver* observer;
    explicit DatePause(State& s):state(s),observer(s.memory.date_observer){state.memory.date_observer=nullptr;}
    ~DatePause(){state.memory.date_observer=observer;}
    void body(){state.memory.date_observer=observer;}
    void diagnostics(){state.memory.date_observer=nullptr;}
};
bool audit_perturb_difference = false;
bool audit_perturb_ordering = false, audit_perturb_before = false, audit_include_before = false;
std::uint64_t audit_index = 0;
gzFile month_audit = nullptr;
bool month_record = false, month_perturb = false;
std::uint64_t month_index = 0;
gzFile day_audit = nullptr;
bool day_record = false, day_perturb = false;
std::uint64_t day_index = 0;
gzFile weekday_audit = nullptr;
bool weekday_record = false, weekday_perturb = false;
std::uint64_t weekday_index = 0;
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
constexpr std::uint32_t day_entry = 0x107182f0;
constexpr std::uint32_t weekday_entry = 0x10715b20;
void execute_weekday(State& s) {
    const auto pointer = std::uint32_t(s.r<4>(12));
    // Three ordered loads only: unsigned year, byte flag, signed day.
    const auto year = std::uint16_t(s.memory.read<2>(pointer + 2));
    const auto flag = std::uint8_t(s.memory.read<1>(pointer + 4));
    const auto day = cm::calendar::narrow_day(int(s.memory.read<2>(pointer)));
    const Date date{day, year};
    const auto years = std::int32_t(year) - 1600;
    const auto total = cm::calendar::weekday_days(date, flag);
    const auto remainder = total % 7;
    // ABI residue of the signed multiply-high divisions. int64 products fit;
    // arithmetic shifts are defined in C++23. These do not select the result.
    const auto century_high = (INT64_C(0x51eb851f) * years) >> 32;
    const auto century_floor = century_high >> 5;
    const auto four_century_floor = century_high >> 7;
    const auto seven_intermediate = ((-INT64_C(1840700269) * total) >> 32) + total;
    s.wr<4>(0, std::uint32_t(remainder));
    s.wr<4>(12, std::uint32_t(int(cm::calendar::weekday(date, flag))));
    s.wr<4>(16, std::uint32_t(four_century_floor + (four_century_floor < 0)));
    s.wr<4>(20, century_floor < 0);
    s.wr<4>(24, std::uint32_t(four_century_floor));
    s.wr<4>(28, four_century_floor < 0);
    s.wr<4>(32, std::uint32_t(total));
    // Positive exits keep r0's remainder; zero/negative exits sign-extend it
    // again. Both record CR0 against the remainder, not the returned weekday.
    s.wr<1>(2304, (s.r<1>(2304) & 0xf0) |
        (remainder < 0 ? 8 : remainder > 0 ? 4 : 2) | (s.r<1>(1024) & 1));
    s.wr<1>(1027, seven_intermediate < 0 && (std::uint64_t(seven_intermediate) & 3));
    s.pc = std::uint32_t(s.r<4>(4128));
}
void execute_day_of_month(State& s) {
    const auto date = std::uint32_t(s.r<4>(12));
    const auto flag = std::uint8_t(s.memory.read<1>(date + 4));
    const auto signed_flag = std::bit_cast<std::int8_t>(flag);
    s.wr<4>(0, std::uint32_t(int(signed_flag)));
    s.wr<1>(2304, (signed_flag < 0 ? 8 : signed_flag > 0 ? 4 : 2) | (s.r<1>(1024) & 1));
    const auto slot = std::uint32_t(s.r<4>(8)) - (flag ? 0x3fa4u : 0x3fa8u);
    s.wr<4>(20, s.memory.read<4>(slot));
    s.wr<4>(0, 12); s.wr<4>(16, s.r<4>(20) + 22);
    s.wr<4>(28, 11); s.wr<4>(4132, 12);
    for (int month = 11; month >= 0; --month) {
        const auto threshold = cm::calendar::narrow_day(int(s.memory.read<2>(s.r<4>(16))));
        s.wr<4>(0, std::uint32_t(int(threshold)));
        const auto day = cm::calendar::narrow_day(int(s.memory.read<2>(date)));
        s.wr<4>(24, std::uint32_t(int(day)));
        s.wr<1>(2304, (threshold < day ? 8 : threshold > day ? 4 : 2) | (s.r<1>(1024) & 1));
        if (cm::calendar::recognizes_month(day, threshold)) {
            s.wr<4>(0, unsigned(month) * 2);
            // The selected threshold is read again after recognition. Do not
            // substitute the probe value or freeze the mutable table.
            const auto selected = cm::calendar::narrow_day(int(s.memory.read<2>(
                std::uint32_t(s.r<4>(20) + s.r<4>(0)))));
            s.wr<4>(0, std::uint32_t(int(selected)));
            s.wr<4>(0, std::uint32_t(int(day) - int(selected)));
            s.wr<4>(12, std::uint32_t(int(cm::calendar::day_from_threshold(day, selected))));
            s.pc = std::uint32_t(s.r<4>(4128)); return;
        }
        s.wr<4>(16, s.r<4>(16) - 2); s.wr<4>(28, s.r<4>(28) - 1);
        s.wr<4>(4132, s.r<4>(4132) - 1);
    }
    s.wr<4>(12, 0xffffffffu); s.pc = std::uint32_t(s.r<4>(4128));
}
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
Date read_date(const State& s, std::uint32_t address) {
    return {cm::calendar::narrow_day(int(s.memory.read<2>(address))),
            std::uint16_t(s.memory.read<2>(address + 2))};
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

}
bool try_modern_calendar(State& s, std::uint32_t entry) {
    if(s.calendar_original_resume==entry){s.calendar_original_resume=0;return false;}
    DatePause observation(s);
    if (entry == weekday_entry) {
        const bool original = s.calendar_original || s.date_weekday_original;
        const auto active_weekday_audit=history_depth?nullptr:weekday_audit;
        if (original && !active_weekday_audit) return false;
        HistoryScope history;
        if (active_weekday_audit && weekday_record && !original)
            throw std::runtime_error("Weekday reference recording requires original execution");
        const auto pointer = std::uint32_t(s.r<4>(12)), link = std::uint32_t(s.r<4>(4128));
        const bool sample = !original && s.date_weekday_calls < 32 && std::getenv("CM_AOT_VALIDATION_EVENT_POINTER");
        const auto input = active_weekday_audit || sample ? s.memory.copy_out(pointer, 5) : std::vector<std::uint8_t>{};
        std::vector<std::uint8_t> edge;
        auto append = [&](std::uint64_t value, unsigned width) {
            for (unsigned i = width; i; --i) edge.push_back(std::uint8_t(value >> ((i - 1) * 8)));
        };
        if (active_weekday_audit) {
            append(entry, 4); append(pointer, 4); append(link, 4);
            edge.insert(edge.end(), input.begin(), input.end());
        }
        if (original) {
            observation.body(); s.calendar_original_resume=entry; s.pc = entry;
            try { run_until(s,link); } catch (...) {s.calendar_original_resume=0;throw;}
            observation.diagnostics();
        } else {
            observation.body(); s.step(entry); ++s.date_weekday_calls; execute_weekday(s); observation.diagnostics();
            if (weekday_perturb && s.date_weekday_calls == 1) s.wr<4>(12, s.r<4>(12) + 1);
            if (sample)
                std::fprintf(stderr, "date-weekday.modern entry=%08x sample=%llu pointer=%08x date=%u/%d flag=%u result=%d return=%08x\n",
                    entry, static_cast<unsigned long long>(s.date_weekday_calls), pointer,
                    unsigned(read_be<2>(input.data()+2)), int(cm::calendar::narrow_day(int(read_be<2>(input.data())))),
                    unsigned(input[4]), int(std::bit_cast<std::int32_t>(std::uint32_t(s.r<4>(12)))), link);
        }
        if (active_weekday_audit) {
            append(s.r<4>(12), 4);
            edge.insert(edge.end(), s.registers.begin(), s.registers.end()); append(s.pc, 4);
            if (weekday_record) {
                if (gzwrite(active_weekday_audit, edge.data(), unsigned(edge.size())) != int(edge.size()))
                    throw std::runtime_error("Cannot record private weekday edges");
            } else {
                std::vector<std::uint8_t> expected(edge.size());
                if (gzread(active_weekday_audit, expected.data(), unsigned(expected.size())) != int(expected.size()))
                    throw std::runtime_error("First divergence: weekday call count/truncated edge reference");
                const auto mismatch = std::mismatch(expected.begin(), expected.end(), edge.begin());
                if (mismatch.first != expected.end()) {
                    const auto byte = mismatch.first - expected.begin();
                    throw std::runtime_error("First divergence: weekday edge " + std::to_string(weekday_index) +
                        " at " + hex_address(entry) + " field=" + (byte >= 17 && byte < 21 ? "weekday return" : "weekday input/ABI/PC") +
                        " byte=" + std::to_string(byte) + " expected=" + std::to_string(*mismatch.first) +
                        " actual=" + std::to_string(*mismatch.second));
                }
            }
            ++weekday_index;
        }
        return true;
    }
    if (entry == month_entry || entry == day_entry) {
        const bool is_day = entry == day_entry;
        auto stream = history_depth ? nullptr : (is_day ? day_audit : month_audit);
        const bool record = is_day ? day_record : month_record;
        const bool perturb = is_day ? day_perturb : month_perturb;
        auto& index = is_day ? day_index : month_index;
        auto& calls = is_day ? s.date_day_of_month_calls : s.date_month_calls;
        const std::string name = is_day ? "day-of-month" : "month";
        const bool original = s.calendar_original || (is_day ? s.date_day_of_month_original : s.date_month_original);
        if (original && !stream) return false;
        HistoryScope history;
        const auto date = std::uint32_t(s.r<4>(12)), link = std::uint32_t(s.r<4>(4128));
        std::vector<std::uint8_t> edge;
        auto append = [&](std::uint64_t value, unsigned width) {
            for (unsigned i = width; i; --i) edge.push_back(std::uint8_t(value >> ((i - 1) * 8)));
        };
        const bool sample = !original && calls < 32 && std::getenv("CM_AOT_VALIDATION_EVENT_POINTER");
        const auto input = stream || sample ? s.memory.copy_out(date, 5) : std::vector<std::uint8_t>{};
        const auto table = stream || sample ? std::uint32_t(s.memory.read<4>(
            std::uint32_t(s.r<4>(8)) - (input[4] ? 0x3fa4u : 0x3fa8u))) : 0;
        if (stream) {
            append(entry, 4); append(date, 4); append(link, 4);
            edge.insert(edge.end(), input.begin(), input.end()); append(table, 4);
            const auto thresholds = s.memory.copy_out(table, 24);
            edge.insert(edge.end(), thresholds.begin(), thresholds.end());
        }
        if (original) {
            observation.body(); s.calendar_original_resume=entry;s.pc=entry;
            try {run_until(s,link);} catch (...) {s.calendar_original_resume=0;throw;}
            observation.diagnostics();
        } else {
            observation.body(); s.step(entry); ++calls;
            if (is_day) execute_day_of_month(s); else execute_month(s);
            observation.diagnostics();
            if (perturb && calls == 1) s.wr<4>(12, s.r<4>(12) + 1);
            if (sample) {
                if (is_day)
                    std::fprintf(stderr, "date-day-of-month.modern entry=%08x sample=%llu pointer=%08x date=%u/%d flag=%u table=%08x result=%d return=%08x\n",
                        entry, static_cast<unsigned long long>(calls), date, unsigned(read_be<2>(input.data()+2)),
                        int(cm::calendar::narrow_day(int(read_be<2>(input.data())))), unsigned(input[4]), table,
                        int(std::bit_cast<std::int32_t>(std::uint32_t(s.r<4>(12)))), link);
                else
                    std::fprintf(stderr, "date-month.modern entry=%08x sample=%llu date=%u/%d flag=%u table=%08x result=%d return=%08x\n",
                        entry, static_cast<unsigned long long>(calls), unsigned(read_be<2>(input.data()+2)),
                        int(cm::calendar::narrow_day(int(read_be<2>(input.data())))), unsigned(input[4]), table,
                        int(std::bit_cast<std::int32_t>(std::uint32_t(s.r<4>(12)))), link);
            }
        }
        if (stream) {
            append(s.r<4>(12), 4);
            edge.insert(edge.end(), s.registers.begin(), s.registers.end()); append(s.pc, 4);
            if (record) {
                if (gzwrite(stream, edge.data(), unsigned(edge.size())) != int(edge.size()))
                    throw std::runtime_error("Cannot record private " + name + " edges");
            } else {
                std::vector<std::uint8_t> expected(edge.size());
                if (gzread(stream, expected.data(), unsigned(expected.size())) != int(expected.size()))
                    throw std::runtime_error("First divergence: " + name + " call count/truncated edge reference");
                const auto mismatch = std::mismatch(expected.begin(), expected.end(), edge.begin());
                if (mismatch.first != expected.end()) {
                    const auto byte = mismatch.first - expected.begin();
                    throw std::runtime_error("First divergence: " + name + " edge " + std::to_string(index) +
                        " at " + hex_address(entry) + " field=" + (byte >= 45 && byte < 49 ? name + " return" : name + " input/table/ABI/PC") +
                        " byte=" + std::to_string(byte) + " expected=" + std::to_string(*mismatch.first) +
                        " actual=" + std::to_string(*mismatch.second));
                }
            }
            ++index;
        }
        return true;
    }
    const bool difference = entry == difference_entry;
    const bool before = entry == before_entry;
    const bool ordering = entry == ordering_entry || before;
    const bool audited = !history_depth && audit && (!before || audit_include_before);
    const bool original = s.calendar_original || (difference && s.date_difference_original) ||
                          (before ? s.date_before_original : ordering && s.date_ordering_original);
    if (original && !audited) return false;
    HistoryScope history;
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
        observation.body();s.calendar_original_resume=entry;s.pc=entry;
        try {run_until(s,return_pc);} catch (...) {s.calendar_original_resume=0;throw;}
        observation.diagnostics();
    } else {
        s.step(entry); // Actual semantic dispatch, not fabricated original accounting.
        if (ordering) {
            auto& calls = before ? s.date_before_calls : s.date_ordering_calls;
            const bool sample = calls < 32 && std::getenv("CM_AOT_VALIDATION_EVENT_POINTER");
            const auto packed = sample ? s.r<4>(20) : 0;
            const auto stack = std::uint32_t(s.r<4>(4));
            ++calls;
            observation.body();execute_ordering(s, before);observation.diagnostics();
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
            observation.body();execute_semantic_calendar_boundary(s, difference_entry);observation.diagnostics();
            if (audit_perturb_difference && s.date_difference_calls == 1)
                s.wr<4>(12, s.r<4>(12) + 1);
            // Bounded private samples from real application calls; no UI option.
            if (sample)
                std::fprintf(stderr, "date-difference.modern sample=%llu first=%u/%d second=%u/%d second-tail=%08x result=%d return=%08x\n",
                    static_cast<unsigned long long>(s.date_difference_calls), first.year, first.day, second.year, second.day,
                    unsigned(packed), int(std::bit_cast<std::int32_t>(std::uint32_t(s.r<4>(12)))), return_pc);
        } else {
            ++s.calendar_calls[entry == addition.entry ? 0 : 1];
            observation.body();execute_semantic_calendar_boundary(s, entry);observation.diagnostics();
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
    out << " day-of-month-mode=" << (s.calendar_original || s.date_day_of_month_original ? "original" : "modern")
        << " modern-day-of-month=" << s.date_day_of_month_calls;
    out << " weekday-mode=" << (s.calendar_original || s.date_weekday_original ? "original" : "modern")
        << " modern-weekday=" << s.date_weekday_calls;
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
void open_day_of_month_audit(const std::filesystem::path& path, bool record, bool perturb) {
    if (day_audit || (perturb && record)) throw std::runtime_error("Invalid private day-of-month audit mode");
    day_audit = gzopen(path.c_str(), record ? "wb1" : "rb");
    if (!day_audit) throw std::runtime_error("Cannot open private day-of-month edge reference");
    constexpr std::array<char,8> magic{'C','M','D','O','M','0','0','1'};
    auto header = magic;
    const auto count = record ? gzwrite(day_audit, header.data(), header.size()) : gzread(day_audit, header.data(), header.size());
    if (count != int(header.size()) || header != magic) {
        gzclose(day_audit); day_audit = nullptr;
        throw std::runtime_error("Unrecognized private day-of-month stream");
    }
    day_record = record; day_perturb = perturb; day_index = 0;
}
void close_day_of_month_audit(bool require_end) {
    if (!day_audit) return;
    bool extra = false;
    if (require_end && !day_record) { std::uint8_t byte; extra = gzread(day_audit, &byte, 1) != 0 || !gzeof(day_audit); }
    const auto result = gzclose(day_audit); day_audit = nullptr; day_perturb = false;
    if (extra || result != Z_OK) throw std::runtime_error("First divergence: day-of-month call count or edge checksum");
}
void open_weekday_audit(const std::filesystem::path& path, bool record, bool perturb) {
    if (weekday_audit || (perturb && record)) throw std::runtime_error("Invalid private weekday audit mode");
    weekday_audit = gzopen(path.c_str(), record ? "wb1" : "rb");
    if (!weekday_audit) throw std::runtime_error("Cannot open private weekday edge reference");
    constexpr std::array<char,8> magic{'C','M','W','K','D','0','0','1'};
    auto header = magic;
    const auto count = record ? gzwrite(weekday_audit, header.data(), header.size()) : gzread(weekday_audit, header.data(), header.size());
    if (count != int(header.size()) || header != magic) {
        gzclose(weekday_audit); weekday_audit = nullptr;
        throw std::runtime_error("Unrecognized private weekday stream");
    }
    weekday_record = record; weekday_perturb = perturb; weekday_index = 0;
}
void close_weekday_audit(bool require_end) {
    if (!weekday_audit) return;
    bool extra = false;
    if (require_end && !weekday_record) { std::uint8_t byte; extra = gzread(weekday_audit, &byte, 1) != 0 || !gzeof(weekday_audit); }
    const auto result = gzclose(weekday_audit); weekday_audit = nullptr; weekday_perturb = false;
    if (extra || result != Z_OK) throw std::runtime_error("First divergence: weekday call count or edge checksum");
}
}

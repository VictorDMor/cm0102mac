#include "calendar_adapter.hpp"
#include "calendar.hpp"
#include <sstream>
#include <algorithm>
#include <zlib.h>

namespace cm::aot {
namespace {
gzFile audit = nullptr;
bool audit_record = false, audit_original_running = false, audit_perturb = false;
std::uint64_t audit_index = 0;
using cm::calendar::Date;
using cm::calendar::Direction;
struct Boundary {
    std::uint32_t entry, scratch_slot, initialization, delegated_return;
    Direction direction;
};
constexpr Boundary addition{0x10717890, 0x108b7ae4, 0x108b84d5, 0x10717918, Direction::forward};
constexpr Boundary subtraction{0x10717b20, 0x108b7adc, 0x108b84d3, 0x10717ba8, Direction::backward};
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
    if (audit_original_running || (s.calendar_original && !audit)) return false;
    const auto return_pc = std::uint32_t(s.r<4>(4128));
    const auto destination = std::uint32_t(s.r<4>(12));
    std::vector<std::uint8_t> edge;
    auto append = [&](std::uint64_t value, unsigned width) {
        for (unsigned i = width; i; --i) edge.push_back(std::uint8_t(value >> ((i - 1) * 8)));
    };
    if (audit) {
        append(entry, 4);
        for (unsigned r : {12u, 16u, 20u, 4u, 8u, 4128u}) append(s.r<4>(r), 4);
        const auto input = s.memory.copy_out(std::uint32_t(s.r<4>(16)), 5);
        edge.insert(edge.end(), input.begin(), input.end());
    }
    if (cm::calendar::narrow_day(int(s.r<2>(22))) == -32768) {
        // Negating SHRT_MIN delegates indefinitely in the reference. Preserve
        // its bounded stack/budget failure instead of inventing a date result.
        ++s.calendar_exceptional_calls; return false;
    }
    if (s.calendar_original) {
        audit_original_running = true; s.pc = entry;
        try {
            do { if (!dispatch_page(s)) throw std::runtime_error("Uncompiled original calendar reference"); }
            while (s.pc != return_pc);
        } catch (...) { audit_original_running = false; throw; }
        audit_original_running = false;
    } else {
        s.step(entry); // Actual semantic dispatch, not fabricated original accounting.
        ++s.calendar_calls[entry == addition.entry ? 0 : 1];
        execute(s, entry == addition.entry ? addition : subtraction);
        if (audit_perturb && audit_index == 0)
            s.memory.write<2>(destination, s.memory.read<2>(destination) + 1);
    }
    if (audit) {
        for (const auto address : {destination, std::uint32_t(s.memory.read<4>(addition.scratch_slot)),
                                  std::uint32_t(s.memory.read<4>(subtraction.scratch_slot))}) {
            const auto bytes = s.memory.copy_out(address, 5); edge.insert(edge.end(), bytes.begin(), bytes.end());
        }
        append(s.memory.read<1>(addition.initialization), 1);
        append(s.memory.read<1>(subtraction.initialization), 1);
        for (unsigned r : {0u, 12u, 16u, 20u, 4u, 8u, 112u, 116u, 120u, 124u, 4128u}) append(s.r<4>(r), 4);
        for (unsigned r : {2304u, 1024u, 1027u}) append(s.r<1>(r), 1);
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
                const auto field = index < 28 ? "entry/effective argument" : index < 33 ? "input date" :
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
void open_calendar_audit(const std::filesystem::path& path, bool record, bool perturb) {
    if (audit) throw std::runtime_error("Calendar audit already active");
    audit = gzopen(path.c_str(), record ? "wb1" : "rb");
    if (!audit) throw std::runtime_error("Cannot open private calendar edge reference");
    audit_record = record; audit_index = 0; audit_perturb = perturb;
}
void close_calendar_audit(bool require_end) {
    if (!audit) return;
    bool extra = false;
    if (require_end && !audit_record) { std::uint8_t byte; extra = gzread(audit, &byte, 1) != 0 || !gzeof(audit); }
    const auto result = gzclose(audit); audit = nullptr; audit_perturb = false;
    if (extra || result != Z_OK) throw std::runtime_error("First divergence: calendar call count or edge checksum");
}
std::string calendar_call_summary(const State& s) {
    std::ostringstream out;
    out << "calendar.mode=" << (s.calendar_original ? "original" : "modern")
        << " modern-add=" << s.calendar_calls[0] << " modern-subtract=" << s.calendar_calls[1]
        << " exceptional-original=" << s.calendar_exceptional_calls;
    return out.str();
}
}

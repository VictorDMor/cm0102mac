#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using guest::Cpu;
using guest::compare;
using guest::compare_logical;

struct ContractEnd {
    Cpu& cpu;
    std::uint32_t address;

    std::int16_t day() const { return cpu.i16(address + ContractRecord::end_date); }
    std::uint16_t year() const { return cpu.u16(address + ContractRecord::end_date + 2); }
    std::uint8_t packed_kind() const { return cpu.u8(address + ContractRecord::packed_kind); }
};

struct ReferenceDate {
    std::uint32_t pc;
    std::int32_t toc_slot;
    std::int32_t scratch;
    std::int32_t cache;
};

// The offsets name live, two-word date objects in the TOC. The scratch and
// cache copies overlap neither each other nor the normal caller frame.
constexpr ReferenceDate long_window{0x103807ac, -0x79b4, -0x10, -0x78};
constexpr ReferenceDate middle_window{0x10380818, -0x79a8, -0x18, -0x90};
constexpr ReferenceDate upper_middle{0x1038087c, -0x79ac, -0x20, -0x88};
constexpr ReferenceDate upper_window{0x103808e0, -0x79b0, -0x28, -0x80};
constexpr ReferenceDate lower_window{0x10380a1c, -0x79a0, -0x48, -0xa0};
constexpr ReferenceDate lower_middle{0x10380a80, -0x79a4, -0x50, -0x98};

bool before_end(const ContractEnd& end, std::uint16_t end_year, std::uint16_t year,
                std::uint32_t scratch, std::uint32_t after_year_test) {
    auto& c = end.cpu;
    bool earlier;
    if (year < end_year) {
        block(c, after_year_test, 2);
        earlier = true;
        c.set(4, 1);
    } else {
        block(c, after_year_test + 8, 1);
        if (year > end_year) {
            block(c, after_year_test + 12, 2);
            earlier = false;
            c.set(4, 0);
        } else {
            block(c, after_year_test + 20, 8);
            const auto day = c.i16(scratch);
            c.set(4, guest::extended(day));
            const auto end_day = end.day();
            c.set(6, guest::extended(end_day));
            const std::int32_t differing_bits = day ^ end_day;
            c.set(5, std::uint32_t(differing_bits >> 1));
            c.s.wr<1>(guest::reg::xer_ca, differing_bits < 0 && (differing_bits & 1));
            earlier = day < end_day;
            c.set(4, earlier);
        }
    }
    block(c, after_year_test + 52, 2);
    compare(c, earlier);
    return earlier;
}

bool reference_precedes_end(const ContractEnd& end, std::uint16_t end_year, ReferenceDate reference) {
    auto& c = end.cpu;
    const auto scratch = c.sp() + std::uint32_t(reference.scratch);
    const auto cache = c.sp() + std::uint32_t(reference.cache);
    block(c, reference.pc, 10);
    const auto address = c.toc(reference.toc_slot);
    c.set(4, address);
    const auto day_and_year = c.u32(address);
    c.set(5, day_and_year);
    const auto date_tail = c.u32(address + 4);
    c.set(4, date_tail);
    c.put32(scratch, day_and_year);
    const auto year = c.u16(scratch + 2);
    c.set(6, year);
    c.put32(cache, day_and_year);
    compare_logical(c, year, end_year);
    c.put32(cache + 4, date_tail);
    c.put32(scratch + 4, date_tail);
    return before_end(end, end_year, year, scratch, reference.pc + 40);
}

std::uint8_t cached_band(const ContractEnd& end, std::uint16_t end_year, ReferenceDate reference,
                         std::uint32_t pc, std::int32_t spill, std::uint8_t band) {
    auto& c = end.cpu;
    const auto source = c.sp() + std::uint32_t(reference.cache);
    const auto scratch = c.sp() + std::uint32_t(spill);
    block(c, pc, 8);
    const auto day_and_year = c.u32(source);
    c.set(4, day_and_year);
    c.set(5, 0);
    const auto date_tail = c.u32(source + 4);
    c.set(6, date_tail);
    c.put32(scratch, day_and_year);
    const auto year = c.u16(scratch + 2);
    c.set(4, year);
    c.put32(scratch + 4, date_tail);
    bool equal = false;
    if (compare_logical(c, year, end_year) == 0) {
        block(c, pc + 32, 4);
        const auto day = c.i16(scratch);
        c.set(4, guest::extended(day));
        const auto end_day = end.day();
        c.set(0, guest::extended(end_day));
        equal = compare(c, day, end_day) == 0;
        if (equal) {
            block(c, pc + 48, 1);
            c.set(5, 1);
        }
    }
    block(c, pc + 52, 5);
    c.set(0, equal ? UINT32_MAX : 0);
    c.s.wr<1>(guest::reg::xer_ca, equal);
    return band - equal;
}

std::uint8_t lowest_band(const ContractEnd& end, std::uint16_t end_year) {
    auto& c = end.cpu;
    const auto scratch = c.sp() - 0x68;
    block(c, 0x10380b74, 8);
    const auto day_and_year = c.u32(c.sp() - 0xa0);
    c.set(4, day_and_year);
    c.set(6, 0);
    const auto date_tail = c.u32(c.sp() - 0x9c);
    c.set(5, date_tail);
    c.put32(scratch, day_and_year);
    const auto year = c.u16(scratch + 2);
    c.set(4, year);
    c.put32(scratch + 4, date_tail);
    bool equal = false;
    if (compare_logical(c, year, end_year) == 0) {
        block(c, 0x10380b94, 4);
        const auto day = c.i16(scratch);
        c.set(5, guest::extended(day));
        const auto end_day = end.day();
        c.set(4, guest::extended(end_day));
        equal = compare(c, day, end_day) == 0;
        if (equal) {
            block(c, 0x10380ba4, 1);
            c.set(6, 1);
        }
    }
    block(c, 0x10380ba8, 2);
    compare(c, equal);
    if (equal) {
        block(c, 0x10380bb0, 2);
        return 11;
    }

    const auto final_scratch = c.sp() - 0x70;
    block(c, 0x10380bb8, 9);
    const auto final_date = c.toc(-0x799c);
    c.set(6, final_date);
    c.set(5, 0);
    const auto final_day_and_year = c.u32(final_date);
    c.set(4, final_day_and_year);
    const auto final_tail = c.u32(final_date + 4);
    c.set(6, final_tail);
    c.put32(final_scratch, final_day_and_year);
    const auto final_year = c.u16(final_scratch + 2);
    c.set(4, final_year);
    c.put32(final_scratch + 4, final_tail);
    if (compare_logical(c, final_year, end_year) == 0) {
        block(c, 0x10380bdc, 4);
        const auto day = c.i16(final_scratch);
        c.set(4, guest::extended(day));
        const auto end_day = end.day();
        c.set(0, guest::extended(end_day));
        equal = compare(c, day, end_day) == 0;
        if (equal) {
            block(c, 0x10380bec, 1);
            c.set(5, 1);
        }
    }
    block(c, 0x10380bf0, 5);
    c.set(0, equal ? UINT32_MAX : 0);
    return 12 + equal;
}

}

std::uint8_t classify_expiry(Cpu& c, std::uint32_t contract) {
    const ContractEnd end{c, contract};
    block(c, expiry_entry, 6);
    const auto packed = end.packed_kind();
    const auto kind = detail::unpack_kind(packed);
    c.s.wr<1>(guest::reg::xer_ca, (packed & 0x82) == 0x82);
    c.set({{0, std::uint32_t(kind)}, {7, std::uint32_t(kind)}});
    if (compare(c, kind, 1) == 0) {
        block(c, 0x10380728, 2);
        return 14;
    }

    block(c, 0x10380730, 9);
    const auto today = c.toc(-0x7b80);
    c.set(4, today);
    const auto end_year = end.year();
    c.set(0, end_year);
    const auto day_and_year = c.u32(today);
    c.set(5, day_and_year);
    const auto date_tail = c.u32(today + 4);
    c.set(4, date_tail);
    const auto scratch = c.sp() - 8;
    c.put32(scratch, day_and_year);
    const auto year = c.u16(scratch + 2);
    c.set(5, year);
    c.put32(scratch + 4, date_tail);
    compare_logical(c, year, end_year);
    if (!before_end(end, end_year, year, scratch, 0x10380754)) {
        block(c, 0x10380790, 2);
        return 14;
    }

    block(c, 0x10380798, 3);
    c.set(4, std::uint32_t(kind));
    if (compare(c, kind, 2) != 0) {
        block(c, 0x103807a4, 2);
        return 0;
    }

    if (reference_precedes_end(end, end_year, long_window)) {
        block(c, 0x10380810, 2);
        return 0;
    }
    if (reference_precedes_end(end, end_year, middle_window)) {
        if (!reference_precedes_end(end, end_year, upper_middle))
            return cached_band(end, end_year, upper_middle, 0x103809d4, -0x40, 6);
        if (!reference_precedes_end(end, end_year, upper_window))
            return cached_band(end, end_year, upper_window, 0x1038098c, -0x38, 4);
        return cached_band(end, end_year, long_window, 0x10380944, -0x30, 2);
    }
    if (!reference_precedes_end(end, end_year, lower_window))
        return lowest_band(end, end_year);
    if (reference_precedes_end(end, end_year, lower_middle))
        return cached_band(end, end_year, middle_window, 0x10380ae4, -0x58, 8);
    return cached_band(end, end_year, lower_middle, 0x10380b2c, -0x60, 10);
}
}

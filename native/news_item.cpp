#include "news_detail.hpp"
#include <array>

namespace cm::aot::news {
using namespace detail;

std::uint32_t construct_item(Cpu& c, std::uint32_t address, std::uint32_t type,
                              std::uint32_t priority) {
    Frame frame(c, 0x102bdf90, 0x50, {.first_gpr = 28});
    Kept<28> item(c, address);
    keep(c, {{29, type}, {30, priority}, {31, c.toc(slot::today)}});
    Program program{c};
    program.call(0x10715880, 0x102bdfc4, {item + 0xd0});
    Item{c, item}.type() = std::int32_t(type);
    Item{c, item}.priority() = priority;
    program.call(0x10778300, 0x102bdfdc, {Item{c, item}.payload(), 0, Item::payload_size});
    const auto date = c.r(31);
    c.set({{3, c.u16(date + 2)}, {4, 0}, {5, c.toc(slot::language)}});
    c.put16(item + 0xd2, c.r(3));
    c.set(3, item); c.set(6, extended(c.i16(date))); c.put16(item + 0xd0, c.r(6));
    c.set(6, c.u8(date + 4)); c.put8(item + 0xd4, c.r(6));
    c.set(5, c.u8(c.r(5))); c.put8(item + 0xd8, c.r(5));
    Item{c, item}.id() = 0; c.put8(item + 0xe0, 0);
    return item;
}

std::uint32_t category(Cpu& c, std::uint32_t address) {
    Frame frame(c, 0x102cb340, 0x70, {.first_gpr = 30});
    keep(c, {{30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    const auto type = Item{c, address}.type().get();
    struct Range { std::int32_t first, limit; unsigned category; };
    constexpr std::array<Range, 7> ranges{{
        {1,1000,2}, {1000,2000,7}, {2000,3000,3}, {3000,4000,4},
        {4000,5000,5}, {5000,6000,1}, {6000,6033,6}}};
    for (const auto& range : ranges) {
        if (compare(c, type, range.first) < 0) continue;
        // The last category includes 6032, unlike the half-open thousands.
        if ((range.first == 6000 ? compare(c, type, 6032) <= 0
                                 : compare(c, type, range.limit) < 0)) return range.category;
    }
    assertion(c, frame, 0x102cb450, 0x40, 0x38, 0x1e4b);
    c.set(4, c.toc(slot::success));
    return 0;
}

std::uint32_t set_word(Cpu& c, std::uint32_t address, std::uint32_t index,
                       std::uint32_t value) {
    Frame frame(c, 0x102cb4b0, 0x70, {.first_gpr = 30});
    keep(c, {{30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    c.set(6, address);
    if (!valid_slot(c, index)) {
        assertion(c, frame, 0x102cb4f8, 0x40, 0x38, 0x1e5d);
        c.set(4, c.toc(slot::success)); return 0;
    }
    c.set(4, address + std::uint32_t(std::int8_t(index)) * 4);
    c.put32(c.r(4) + 8, value);
    return 1;
}

std::uint32_t word(Cpu& c, std::uint32_t address, std::uint32_t index) {
    Frame frame(c, 0x102cb570, 0x70, {.first_gpr = 30});
    keep(c, {{30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    if (!valid_slot(c, index)) {
        assertion(c, frame, 0x102cb5b4, 0x40, 0x38, 0x1e75);
        c.set(4, c.toc(slot::success)); return 0;
    }
    return c.u32(Item{c, address}.payload(std::uint32_t(std::int8_t(index))));
}

std::uint32_t set_text(Cpu& c, std::uint32_t address, std::uint32_t index,
                       std::uint32_t source, std::uint32_t next_slot) {
    Frame frame(c, 0x102cb630, 0xb0, {.first_gpr = 25});
    Kept<28> item(c, address); Kept<29> position(c, index);
    Kept<30> input(c, source); Kept<31> next(c, next_slot);
    keep(c, {{25, c.toc(slot::success)}, {26, c.toc(slot::message)}, {27, c.toc(slot::pool)}});
    if (!valid_slot(c, position)) {
        assertion(c, frame, 0x102cb684, 0x68, 0x40, 0x1e8d); return 0;
    }
    if (compare_logical(c, input) == 0) {
        assertion(c, frame, 0x102cb6e4, 0x48, 0x38, 0x1e95); return 0;
    }
    const auto offset = std::uint32_t(std::int8_t(position)) * 4;
    Program program{c};
    program.call(0x1077dc10, 0x102cb74c, {item + 8 + offset, input, Item::payload_size - offset});
    c.set({{3, offset + 8}, {4, 0}});
    // Even a truncated string always terminates at the end of the payload.
    c.put8(item + 0xcf, 0);
    if (compare_logical(c, next) != 0) {
        const auto length = program.call(0x1077dbd0, 0x102cb780, {input});
        const auto aligned = (length + 4) & ~3u;
        c.set({{4, aligned}, {3, std::uint32_t(std::int8_t(position))}});
        compare(c, 0);
        c.put8(next, std::uint32_t(std::int8_t(position)) + std::uint32_t(std::int32_t(aligned) / 4));
        // The original narrows before comparing: an overflowing next index
        // can become negative and is intentionally not clamped in that case.
        if (compare(c, c.i8(next), Item::payload_words) > 0) c.put8(next, Item::payload_words);
    }
    return 1;
}

std::uint32_t text(Cpu& c, std::uint32_t address, std::uint32_t index,
                   std::uint32_t output, std::uint32_t capacity) {
    Frame frame(c, 0x102cb7f0, 0xa0, {.first_gpr = 27});
    Kept<27> destination(c, output); Kept<28> count(c, capacity);
    keep(c, {{29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    c.set(7, address);
    if (!valid_slot(c, index)) {
        assertion(c, frame, 0x102cb840, 0x68, 0x40, 0x1ebf); return 0;
    }
    if (compare_logical(c, destination) == 0 || compare(c, count, 1) < 0) {
        assertion(c, frame, 0x102cb8a8, 0x48, 0x38, 0x1ec7); return 0;
    }
    Program{c}.call(0x1077dc10, 0x102cb908,
                    {destination, Item{c, address}.payload(std::uint32_t(std::int8_t(index))), count});
    c.put8(std::uint32_t(destination) + std::uint32_t(count) - 1, 0);
    return 1;
}
}

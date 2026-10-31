#include "utils_detail.hpp"
#include <array>

namespace cm::aot::utils {
using namespace detail;
namespace {
// Pool offsets of "zero".."twenty" and "Zero".."Twenty".
constexpr std::array<std::uint16_t, 21> lowercase_numbers{
    0x142, 0x147, 0x14b, 0x14f, 0x155, 0x15a, 0x15f, 0x163, 0x169, 0x16f, 0x174,
    0x178, 0x17f, 0x186, 0x18f, 0x198, 0x1a0, 0x1a8, 0x1b2, 0x1bb, 0x1c4};
constexpr std::array<std::uint16_t, 21> capitalized_numbers{
    0x1cb, 0x1d0, 0x1d4, 0x1d8, 0x1de, 0x1e3, 0x1e8, 0x1ec, 0x1f2, 0x1f8, 0x1fd,
    0x201, 0x208, 0x20f, 0x218, 0x221, 0x229, 0x231, 0x23b, 0x244, 0x24d};
// Pool offsets of the ordinal formats.
constexpr std::uint32_t french_first = 0xf0, plain_number = 0xf4, french_first_feminine = 0xf7, french_nth = 0xfc,
                        suffix_th = 0x102, suffix_st = 0x112, suffix_nd = 0x122, suffix_rd = 0x132;
// Values of the game's language byte that the text routines test.
constexpr std::int32_t french = 2, language_4 = 4, language_7 = 7, language_9 = 9, language_10 = 10;
constexpr unsigned plain_column = 0, upper_column = 1, lower_column = 2;

std::int32_t language(Cpu& c, std::uint32_t back) {
    return std::int8_t(call(c, current_language, back));
}
// The character at `row` of the table: rows are three bytes wide.
std::uint32_t mapped(Cpu& c, std::uint32_t table, std::uint32_t row, unsigned column) {
    return c.u8(table + row * 3 + column);
}
std::uint32_t case_mapped(Cpu& c, std::uint32_t character, unsigned column) {
    const auto row = character & 0xff;
    if (compare_logical(c, row, 0x20) < 0) return character;
    return result(c, c.u8(c.toc(characters_slot) + row * 3 + column));
}
}

void swap_memory(Cpu& c, std::uint32_t first, std::uint32_t second, std::uint32_t size) {
    Frame frame(c, 0x10745e00, 0xa0, {.first_gpr = 29});
    keep(c, {{31, c.toc(strings_slot)}, {30, c.toc(message_slot)}, {29, c.toc(-0x7bac)}});
    if (!nonzero_logical(c, first)) { report(c, 0x10745e40, frame, 0x68, 0x40, 0x167); return; }
    if (!nonzero_logical(c, second)) { report(c, 0x10745e9c, frame, 0x48, 0x38, 0x16e); return; }
    if (!nonzero_logical(c, size)) return;
    // One byte at a time, as the original, because the ranges may overlap.
    std::uint32_t last = 0;
    const auto swap = [&](std::uint32_t count) {
        for (; count != 0; --count, ++first, ++second) {
            last = c.u8(first);
            c.put8(first, c.u8(second));
            c.put8(second, last);
        }
    };
    if (nonzero(c, std::int32_t(size >> 3))) {
        swap(size & ~7u);
        size &= 7;
        c.set(5, size);
        compare(c, std::int32_t(size));
    }
    swap(size);
    leave(c, {{3, first}, {4, second}, {6, last}});
}

void trim(Cpu& c, std::uint32_t text) {
    Frame frame(c, 0x10745fc0, 0x70, {.first_gpr = 29});
    keep(c, {{31, c.toc(strings_slot)}, {30, c.toc(message_slot)}, {29, text}});
    if (!nonzero(c, std::int32_t(text))) {
        report(c, 0x10745ffc, frame, 0x40, 0x38, 0x18b);
        c.set(3, c.toc(-0x7bac));
        return;
    }
    Kept<30> length(c, call(c, string_length, 0x10746044));
    // Controls below the space: the leading ones shift the text down one by one.
    while (nonzero(c, std::int32_t(length)) && compare_logical(c, c.u8(text), 0x20) < 0) {
        call(c, string_copy, 0x10746058, {text, text + 1});
        length = length - 1;
    }
    c.set(3, 0);
    while (nonzero(c, std::int32_t(length))) {
        const std::uint32_t last = length - 1;
        c.set(4, last);
        if (compare_logical(c, c.u8(text + last), 0x20) >= 0) break;
        c.put8(text + last, 0);
        length = length - 1;
    }
}

std::int32_t compare_text(Cpu& c, std::uint32_t first, std::uint32_t second, std::uint32_t exact_case, std::uint32_t limit) {
    Frame frame(c, 0x107465f0, 0x60, {.first_gpr = 25});
    const auto table = c.toc(characters_slot);
    keep(c, {{31, table}, {25, first}, {26, second}, {27, exact_case}, {28, limit}, {30, 0}, {29, 1}});
    const auto strips_accents = compare(c, language(c, 0x10746620), language_10) != 0;
    if (!nonzero_logical(c, first) && !nonzero_logical(c, second)) return std::int32_t(result(c, 0));
    if (!nonzero_logical(c, first)) return std::int32_t(result(c, std::uint32_t(-1)));
    if (!nonzero_logical(c, second)) return std::int32_t(result(c, 1));
    // r5-r7 at return: the bytes last compared, r6 and r7 as they were when
    // a string is empty.
    auto left = first;
    std::uint32_t mapped_right = c.r(6), mapped_left = c.r(7), byte = 0;
    const auto done = [&](std::uint32_t at, std::int32_t value) {
        leave(c, {{4, at}, {5, byte}, {6, mapped_right}, {7, mapped_left}});
        return std::int32_t(result(c, std::uint32_t(value)));
    };
    for (std::uint32_t index = 0;; ++index, ++left) {
        byte = c.u8(left);
        if (byte == 0) break;
        const auto other = c.u8(second + index);
        if (other == 0) { mapped_right = 0; break; }
        mapped_left = strips_accents ? mapped(c, table, byte, plain_column) : byte;
        mapped_right = strips_accents ? mapped(c, table, other, plain_column) : other;
        if (std::int8_t(exact_case) == 0) {
            mapped_left = mapped(c, table, mapped_left, lower_column);
            mapped_right = mapped(c, table, mapped_right, lower_column);
        }
        byte = mapped_left;
        if (const auto order = compare_logical(c, mapped_left, mapped_right); order != 0) return done(left, order);
        if (limit != 0 && compare(c, std::int32_t(index), std::int32_t(limit - 1)) == 0) return done(left, 0);
    }
    if (byte == 0) {
        // The first string ended: reread the second at the same index.
        const auto index = left - first;
        if (compare(c, c.u8(second + index)) == 0) return done(left, 0);
        compare(c, 0);
        return done(left, -1);
    }
    compare(c, std::int32_t(byte));
    return done(left, 1);
}

std::uint32_t to_upper(Cpu& c, std::uint32_t character) { return case_mapped(c, character, upper_column); }
std::uint32_t to_lower(Cpu& c, std::uint32_t character) { return case_mapped(c, character, lower_column); }

std::uint32_t ordinal_text(Cpu& c, std::uint32_t number, std::uint32_t gender) {
    Frame frame(c, 0x107467a0, 0x80, {.first_gpr = 27});
    const auto buffer = c.toc(ordinal_slot), strings = c.toc(strings_slot);
    keep(c, {{29, number}, {30, buffer}, {28, c.toc(message_slot)}, {31, strings}, {27, gender}});
    if (compare_logical(c, number, 1) < 0) {
        report(c, 0x107467e0, frame, 0x40, 0x38, 0x3da);
        c.set(5, c.toc(-0x7bac));
        call(c, string_copy, 0x1074682c, {buffer, strings + no_text});
        return result(c, buffer);
    }
    const auto fixed = [&](std::uint32_t format, std::uint32_t back) {
        c.set({{3, buffer}, {4, strings + format}});
        c.call(format_text, back);
        return result(c, buffer);
    };
    const auto printed = [&](std::uint32_t format, std::uint32_t back) {
        c.set(5, number);
        return fixed(format, back);
    };
    const auto localized = [&](std::uint32_t format, std::uint32_t back) {
        c.set({{3, buffer}, {5, number}, {4, strings + format}});
        c.call(localized_text, back);
        return result(c, buffer);
    };
    const auto kind = std::int32_t(std::int8_t(gender));
    if (compare(c, language(c, 0x10746838), french) == 0) {
        if (compare(c, kind, 1) == 0)
            return compare_logical(c, number, 1) == 0 ? fixed(french_first, 0x10746864) : printed(plain_number, 0x10746878);
        if (compare(c, kind, 2) == 0)
            return compare_logical(c, number, 1) == 0 ? fixed(french_first_feminine, 0x10746898) : printed(french_nth, 0x107468ac);
        return compare_logical(c, number, 1) == 0 ? fixed(french_first, 0x107468c4) : printed(french_nth, 0x107468d8);
    }
    if ((compare(c, language(c, 0x107468e0), language_4) == 0 || compare(c, language(c, 0x107468f0), language_9) == 0 ||
         compare(c, language(c, 0x10746900), language_7) == 0) && compare(c, kind, 1) == 0)
        return printed(plain_number, 0x10746928);
    if (compare(c, language(c, 0x10746930), language_10) == 0 && compare(c, kind, 3) == 0)
        return printed(plain_number, 0x10746958);
    const auto hundreds = number % 100, tens = number % 10;
    if (compare_logical(c, hundreds, 9) > 0 && compare_logical(c, hundreds, 0x15) < 0) return localized(suffix_th, 0x107469ac);
    if (compare_logical(c, tens, 1) == 0) return localized(suffix_st, 0x107469c8);
    if (compare_logical(c, tens, 2) == 0) return localized(suffix_nd, 0x107469e4);
    if (compare_logical(c, tens, 3) == 0) return localized(suffix_rd, 0x10746a00);
    return localized(suffix_th, 0x10746a14);
}

std::uint32_t number_text(Cpu& c, std::uint32_t number, std::uint32_t text, std::uint32_t capacity, std::uint32_t lowercase) {
    Frame frame(c, 0x10746a70, 0xa0, {.first_gpr = 29});
    const auto strings = c.toc(strings_slot);
    keep(c, {{31, strings}, {30, c.toc(message_slot)}, {29, c.toc(-0x7bac)}});
    c.set(7, number);
    if (!nonzero_logical(c, text)) { report(c, 0x10746ab4, frame, 0x68, 0x40, 0x46f); return result(c, 0); }
    if (compare(c, std::int16_t(capacity), 0x32) < 0) { report(c, 0x10746b18, frame, 0x48, 0x38, 0x476); return result(c, 0); }
    const bool lower = nonzero(c, std::int8_t(lowercase));
    if (compare_logical(c, number, 20) > 0) {
        c.set({{3, text}, {5, number}, {4, strings + plain_number}});
        c.call(format_text, lower ? 0x10746ce0 : 0x10746e60);
    } else {
        // The original's two jump tables, one label per word.
        const auto word = lower ? lowercase_numbers[number] : capitalized_numbers[number];
        c.set({{3, text}, {4, strings + word}});
        c.call(localized_text, (lower ? 0x10746b8c : 0x10746d0c) + number * 0x10);
    }
    return result(c, 1);
}

std::uint32_t break_line(Cpu& c, std::uint32_t text, std::uint32_t before_middle, std::uint32_t after_middle) {
    Frame frame(c, 0x10746e80, 0x50, {.first_gpr = 29});
    keep(c, {{31, after_middle}, {30, before_middle}, {29, text}});
    if (!nonzero(c, std::int32_t(text))) return result(c, 0);
    if (!nonzero_logical(c, call(c, string_length, 0x10746eac))) return result(c, 0);
    const auto length = call(c, string_length, 0x10746ebc, {text});
    const auto middle = std::int32_t(length + (length >> 31)) >> 1;
    // From the middle outwards, the earlier side first.
    auto earlier = middle, later = middle;
    auto earlier_at = text + std::uint32_t(middle), later_at = earlier_at;
    const auto broken = [&](std::uint32_t at) {
        leave(c, {{4, earlier_at}, {5, later_at}, {6, std::uint32_t(earlier)}, {7, std::uint32_t(later)}});
        c.put8(at, '\n');
        return result(c, 1);
    };
    while (compare(c, earlier) > 0 || compare(c, later, std::int32_t(length)) < 0) {
        if (nonzero(c, std::int8_t(before_middle)) && compare(c, earlier) > 0 && compare(c, c.u8(earlier_at), ' ') == 0)
            return broken(text + std::uint32_t(earlier));
        if (nonzero(c, std::int8_t(after_middle)) && compare(c, later, std::int32_t(length)) < 0 && compare(c, c.u8(later_at), ' ') == 0)
            return broken(text + std::uint32_t(later));
        --earlier; --earlier_at; ++later; ++later_at;
    }
    leave(c, {{4, earlier_at}, {5, later_at}, {6, std::uint32_t(earlier)}, {7, std::uint32_t(later)}});
    return result(c, 0);
}

void short_name(Cpu& c, std::uint32_t name, std::uint32_t output) {
    Frame frame(c, 0x107473c0, 0x40, {.first_gpr = 30});
    keep(c, {{31, output}, {30, name}});
    const auto length = call(c, string_length, 0x107473e0);
    if (compare(c, std::int32_t(length), 0x1f) < 0) { call(c, string_copy, 0x107473f4, {output, name}); return; }
    // Too long for a Mac file name: drop the spaces, and if that is not
    // enough keep 22 characters and the name's last eight.
    auto from = name, to = output;
    std::int32_t count = 0;
    bool overflow = false;
    while (true) {
        const auto character = std::int32_t(std::int8_t(c.u8(from)));
        ++from;
        if (compare(c, character) == 0) break;
        if (compare(c, character, ' ') == 0) continue;
        if (compare(c, count, 0x1e) == 0) { c.put8(to, 0); overflow = true; break; }
        c.put8(to, std::uint32_t(character));
        ++to; ++count;
    }
    leave(c, {{4, from}, {5, to}, {6, std::uint32_t(count)}, {7, overflow}});
    if (nonzero(c, overflow)) {
        call(c, string_copy_limited, 0x10747468, {output + 0x16, name + (length - 8), 8});
        c.put8(output + 0x1e, 0);
    } else {
        c.put8(output + std::uint32_t(count), 0);
    }
}
}

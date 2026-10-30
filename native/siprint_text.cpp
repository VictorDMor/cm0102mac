#include "siprint.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"

namespace cm::aot::siprint {
namespace {
using namespace guest;
struct Calls {
    Cpu& c;
    std::uint32_t operator()(std::uint32_t target, std::uint32_t back,
                              std::initializer_list<std::uint32_t> args = {}) const {
        unsigned reg = 3;
        for (auto value : args) c.set(reg++, value);
        c.call(target, back);
        if (target >= 0x1077fad8 && target < 0x10781320) c.set(2, c.u32(c.sp() + 20));
        return c.r(3);
    }
};

// The compiler unrolls the decimal ruler eight bytes at a time. Its final
// argument/preserved registers are visible to the following event-pump call;
// describe that boundary explicitly while generating the text with one loop.
void fill_ruler(Cpu& c, std::uint32_t buffer, std::int16_t columns) {
    c.set(9, extended(columns));
    if (compare(c, columns) <= 0) return;
    for (int i = 0; i < columns; ++i) c.put8(buffer + unsigned(i), unsigned(i % 10 + '0'));
    const auto filled = unsigned((columns - 1) / 8) * 8;
    if (filled != 0) {
        const auto last = filled - 8;
        keep(c, {{16, (last + 3) / 10}, {17, last + 4}, {18, last + 5}, {19, last + 6}});
        leave(c, {{6, (last + 4) % 10}, {7, (last + 3) % 10},
                  {8, 0x66666667}, {10, buffer + last}});
    }
    leave(c, {{3, buffer + unsigned(columns)}, {4, unsigned(columns - 1) % 10}, {5, 0x66666667}});
    compare(c, filled, columns);
}
}

void print_lines(Cpu& c, std::uint32_t printer, std::uint32_t lines,
                 std::int32_t count, std::uint32_t columns) {
    Frame frame(c, 0x1074cd90, 0xf0, {.first_gpr = 14});
    Calls call{c}; Fpu fp(c);
    const auto message = c.toc(-0x7ba8), pool = c.toc(-0x3e14);
    keep(c, {{14, message}, {31, pool}, {20, printer}, {21, lines}, {22, std::uint32_t(count)}, {23, columns},
             {30, 0}, {29, 0}, {28, 0}, {27, 128}, {24, 0}});
    c.put32(frame.at(0x3c), 0);
    if (compare_logical(c, c.u8(printer)) == 0 || compare(c, count) == 0 ||
        compare(c, std::int16_t(columns)) == 0) return;
    call(0x107803f0, 0x1074cdfc, {printer + 4});
    call(0x10780378, 0x1074ce04); c.put16(printer + 2, c.r(3));
    const auto session = [&] { return c.u32(printer + 8); };
    const auto format = [&] { return c.u32(printer + 12); };
    bool session_created = false, document_started = false;
    std::uint32_t ruler = 0;
    const auto print_document = [&] {
        if (compare(c, call(0x10780408, 0x1074ce14, {printer + 8})) != 0) return;
        session_created = true; keep(c, {{30, 1}});
        if (compare(c, call(0x10780a68, 0x1074ce2c, {frame.at(0x3c)})) != 0) return;
        if (compare(c, call(0x10780a80, 0x1074ce44, {session(), c.u32(frame.at(0x3c))})) != 0) return;
        if (compare(c, call(0x10780a98, 0x1074ce64,
            {session(), c.u32(frame.at(0x3c)), format(), frame.at(0x38)})) != 0) return;
        if (compare_logical(c, c.u8(frame.at(0x38))) == 0) return;
        if (compare(c, call(0x10780ab0, 0x1074ce8c, {session(), c.u32(frame.at(0x3c)), format()})) != 0) return;
        document_started = true; keep(c, {{29, 1}});
        call(0x10780ac8, 0x1074cea4, {pool + 15});
        call(0x10780ae0, 0x1074ceac);
        call(0x10780af8, 0x1074ceb8, {1});
        call(0x10780b10, 0x1074cec8, {format(), frame.at(0x70)});
        c.set_f(3, c.f64(frame.at(0x88))); c.set_f(2, c.f64(frame.at(0x78)));
        c.set_f(1, c.f64(frame.at(0x80))); c.set_f(0, c.f64(frame.at(0x70)));
        c.set(3, extended(std::int16_t(columns)));
        c.set_f(2, fp.fsub(c.f(3), c.f(2))); c.set_f(0, fp.fsub(c.f(1), c.f(0)));
        const auto width_bits = fp.fctiwz(1, c.f(2));
        const auto height_bits = fp.fctiwz(0, c.f(0));
        c.put64(frame.at(0xa0), width_bits); c.put64(frame.at(0x98), height_bits);
        const auto width = std::uint32_t(width_bits), height = std::uint32_t(height_bits);
        keep(c, {{26, width}, {25, height}});
        ruler = call(0x10775130, 0x1074cf04);
        keep(c, {{24, ruler}});
        if (compare(c, ruler) == 0) return;
        fill_ruler(c, ruler, std::int16_t(columns));
        const auto page_width = std::int16_t(width);
        keep(c, {{15, extended(page_width)}});
        std::int16_t lower = 0, upper = 128, size = 0;
        for (;;) {
            call(0x1076fe80, 0x1074d0b8);
            size = std::int16_t((int(lower) + int(upper)) / 2);
            keep(c, {{16, extended(size)}});
            call(0x10780b28, 0x1074d0dc, {extended(size)});
            call(0x10780b40, 0x1074d0f0, {ruler, 0, columns});
            if (compare(c, std::int16_t(c.r(3)), page_width) < 0) {
                if (compare(c, size, lower) == 0) break;
                lower = size; keep(c, {{28, extended(lower)}});
            } else {
                if (compare(c, size, upper) == 0) break;
                upper = size; keep(c, {{27, extended(upper)}});
            }
        }
        call(0x10775170, 0x1074d130, {ruler}); ruler = 0; keep(c, {{24, 0}});
        call(0x10780b58, 0x1074d13c, {frame.at(0x48)});
        // FontInfo: ascent + descent + leading; widMax at +4 is unused.
        const auto leading = extended(c.i16(frame.at(0x4e)));
        const auto signed_height = extended(std::int16_t(height));
        const auto ascent = extended(c.i16(frame.at(0x48)));
        const auto spacing = std::int16_t(c.i16(frame.at(0x4a)) + leading + ascent);
        const auto lines_per_page = std::int16_t(divide_signed<4>(signed_height, extended(spacing)));
        leave(c, {{4, leading}, {5, signed_height}, {3, ascent}});
        keep(c, {{18, lines}, {17, 0}, {15, extended(spacing)}, {16, extended(lines_per_page)}});
        for (std::int32_t index = 0; compare(c, index, count) < 0; ++index) {
            // At 0x1074d174 the quotient is used only after multiplication by
            // its divisor. The generator's audited zero-divisor lowering
            // therefore leaves remainder=index for a zero-height page too.
            const auto quotient = lines_per_page == 0 ? 0u :
                std::uint32_t(divide_signed<4>(std::uint32_t(index), extended(lines_per_page)));
            const auto remainder = std::uint32_t(index) - quotient * extended(lines_per_page);
            if (compare(c, remainder) == 0) {
                if (compare(c, index) != 0 && compare(c, call(0x10780b70, 0x1074d194, {session()})) != 0) return;
                if (compare(c, call(0x10780b88, 0x1074d1b0, {session(), format(), frame.at(0x70)})) != 0) return;
            }
            const auto text = c.u32(lines + std::uint32_t(index) * 4);
            if (compare_logical(c, text) != 0) {
                const auto advance = std::uint32_t(index) * extended(spacing);
                c.set(3, c.toc(-0x3e18));
                c.set_f(0, c.f64(frame.at(0x78)));
                c.put32(frame.at(0xa0), 0x43300000);
                const auto x_bits = fp.fctiwz(3, c.f(0));
                c.set_f(1, c.f64(c.r(3)));
                c.put32(frame.at(0xa4), advance ^ 0x80000000);
                c.set_f(2, c.f64(frame.at(0x70))); c.set_f(0, c.f64(frame.at(0xa0)));
                c.put64(frame.at(0x98), x_bits);
                c.set_f(0, fp.fsub(c.f(0), c.f(1)));
                c.set(3, std::uint32_t(x_bits));
                c.set_f(0, fp.fadd(c.f(2), c.f(0)));
                const auto y_bits = fp.fctiwz(0, c.f(0)); c.put64(frame.at(0x90), y_bits);
                call(0x10780ba0, 0x1074d214, {std::uint32_t(x_bits), std::uint32_t(y_bits)});
                call(0x1077dbd0, 0x1074d220, {c.u32(lines + std::uint32_t(index) * 4)});
                call(0x10780bb8, 0x1074d234,
                    {c.u32(lines + std::uint32_t(index) * 4), 0, extended(std::int16_t(c.r(3)))});
            }
            keep(c, {{18, lines + (std::uint32_t(index) + 1) * 4}, {17, std::uint32_t(index) + 1}});
        }
        call(0x10780b70, 0x1074d250, {session()});
    };
    print_document();
    if (compare(c, document_started ? 1 : 0) != 0) {
        if (compare(c, call(0x10780bd0, 0x1074d264, {session()})) != 0) {
            report_assertion(c, {0x1074d284, pool + 23, pool + 35, pool + 48,
                                frame.at(0x50), frame.at(0x40), 0x176},
                [&](std::uint32_t target, std::uint32_t back) { call(target, back); });
        }
    }
    c.set(3, c.u32(frame.at(0x3c)));
    if (compare_logical(c, c.r(3)) != 0)
        call(0x107804e0, 0x1074d2d4);
    if (compare(c, session_created ? 1 : 0) != 0 && compare_logical(c, session()) != 0) {
        call(0x107804e0, 0x1074d2f4, {printer + 8}); c.put32(printer + 8, 0);
        call(0x107803c0, 0x1074d308, {extended(c.i16(printer + 2))});
        call(0x107804f8, 0x1074d314, {c.u32(printer + 4)});
    }
    if (compare_logical(c, ruler) != 0) call(0x10775170, 0x1074d328, {ruler});
}
}

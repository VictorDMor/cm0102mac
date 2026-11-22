// The font engine, block `t2ktt_head` (T2KTT.C and the start of TRUETYPE.C, 0x107630e0..0x1076460c): the input stream's constructors and
// their delete, the hint font the TrueType interpreter keeps (its construction, its size, its glyph set-up, its release), the three
// scale callbacks the interpreter calls by pointer, the ratio of two sizes and the two cmap lookups.
//
// A routine with a frame builds it at the original's size, so what it calls (the allocator and the stack leaver 0x10767bb0, the reporter,
// the stream readers) runs at the original's stack pointer. The calls into routines that are still generated go through the dispatcher at
// the original's return addresses; the stream readers and the fixed-point helpers are called directly.
#include "font_engine_detail.hpp"

namespace cm::aot::font_engine {
namespace {
using guest::Frame;
constexpr std::uint32_t units_per_em_entry = 0x10766cc0, hint_interpreter = 0x10756310, hint_installer = 0x1075a970;

// An API entry's setjmp on the memory object's jump buffer (+0x10): `*error_out = setjmp(...)`, then the body. A longjmp that lands at the
// setjmp's return is taken here (the transfer runtime would resume the generated body there): the code goes to *error_out again and the
// caller unwinds with it. A transfer for anyone else propagates.
template <class Body>
std::uint32_t protect(Cpu& c, const Frame& frame, std::uint32_t mem, std::uint32_t error_out, std::uint32_t back, Body&& body) {
    c.set(3, mem + 0x10);
    c.call(dispatched::setjmp_entry, back);
    c.put32(error_out, c.r(3));
    if (c.r(3) != 0) return c.r(3);
    try {
        body();
    } catch (const GuestContextTransfer& transfer) {
        if (transfer.state != &c.s || c.s.pc != back || c.sp() != frame.base()) throw;
        const auto code = c.r(3);
        c.put32(error_out, code);
        return code;
    }
    return 0;
}

// sraw / slw: the shift amount is the register's low six bits; 32..63 leave the sign (sraw) or nothing (slw).
constexpr std::int32_t shift_right(std::int32_t value, std::uint32_t amount) {
    amount &= 63;
    return amount >= 32 ? (value < 0 ? -1 : 0) : value >> amount;
}
constexpr std::uint32_t shift_left(std::uint32_t value, std::uint32_t amount) {
    amount &= 63;
    return amount >= 32 ? 0 : value << amount;
}
std::uint32_t big_endian_16(const Cpu& c, std::uint32_t at) { return std::uint32_t(c.u8(at)) << 8 | c.u8(at + 1); }
}

std::int32_t stream_remaining(Cpu& c, std::uint32_t stream_at) {
    const auto origin = c.u32(stream_at + stream::origin), length = c.u32(stream_at + stream::length);
    c.set(4, origin);
    return wrap(length - origin);
}

std::uint32_t new_substream(Cpu& c, std::uint32_t mem, std::uint32_t parent, std::uint32_t offset, std::uint32_t length, std::uint32_t pooled,
                            std::uint32_t error_out) {
    Frame frame(c, 0x107630f0, 0x50, {.first_gpr = 26});
    // The registers the original holds at its setjmp (the jump buffer keeps r13-r31).
    c.set({{26, mem}, {27, parent}, {28, offset}, {29, length}, {30, pooled}, {31, error_out}});
    // cr0: `or. r31,r8,r8` (what the jump buffer keeps); without an error pointer the branch is taken and the pooled test (`cmpwi r30,0`) is the last compare.
    c.compare(0, wrap(error_out != 0 ? error_out : pooled), 0);
    std::uint32_t made = 0;
    const auto build = [&] {
        // The pooled form takes the allocator's scratch buffer of slot 2 (0x107675c0), the plain one a block.
        const auto block = pooled ? dispatched::scratch(c, mem, stream::size, 2, 0x10763148) : dispatched::allocate(c, mem, stream::size, 0x10763158);
        c.put32(block + stream::mem, mem);
        const auto base = c.u32(parent + stream::base);
        c.put32(block + stream::base, base);
        c.put32(block + stream::position, offset);
        c.put32(block + stream::origin, offset);
        c.put32(block + stream::length, offset + length);
        c.put32(block + stream::reserved_18, 0);
        c.put32(block + stream::reserved_1c, 0);
        c.put8(block + stream::kind, stream::view);
        made = block;
    };
    if (error_out == 0) {
        build();
    } else if (protect(c, frame, mem, error_out, 0x10763124, build) != 0) {
        dispatched::free_blocks(c, mem, 0x107631a0);
        made = 0;
    }
    return made;
}

std::uint32_t new_input_stream(Cpu& c, std::uint32_t mem, std::uint32_t data, std::uint32_t length, std::uint32_t error_out) {
    Frame frame(c, 0x107631c0, 0x50, {.first_gpr = 28});
    c.set({{28, mem}, {29, data}, {30, length}, {31, error_out}});
    c.compare(0, wrap(error_out), 0);  // `or. r31,r6,r6`
    std::uint32_t made = 0;
    const auto build = [&] {
        const auto block = dispatched::allocate(c, mem, stream::size, 0x10763210);
        c.put32(block + stream::mem, mem);
        c.put32(block + stream::base, data);
        c.put32(block + stream::position, 0);
        c.put32(block + stream::origin, 0);
        c.put32(block + stream::length, length);
        c.put32(block + stream::reserved_18, 0);
        c.put32(block + stream::reserved_1c, 0);
        c.put8(block + stream::kind, stream::over_caller);
        made = block;
    };
    if (error_out == 0) {
        build();
    } else if (protect(c, frame, mem, error_out, 0x107631f8, build) != 0) {
        dispatched::free_blocks(c, mem, 0x10763258);
        made = 0;
    }
    return made;
}

void stream_rewind(Cpu& c, std::uint32_t stream_at) { c.put32(stream_at + stream::position, c.u32(stream_at + stream::origin)); }

void stream_seek(Cpu& c, std::uint32_t stream_at, std::uint32_t offset) {
    c.put32(stream_at + stream::position, c.u32(stream_at + stream::origin) + offset);
}

void delete_input_stream(Cpu& c, std::uint32_t stream_at, std::uint32_t error_out) {
    Frame frame(c, 0x107632a0, 0x40, {.first_gpr = 30});
    c.compare(0, wrap(stream_at), 0);  // `or. r30,r3,r3`
    if (stream_at == 0) return;
    // The stack leaver 0x10767bb0 saves r28-r31 where the game's printf reads them: r30 the stream, r31 the error pointer.
    c.set({{30, stream_at}, {31, error_out}});
    c.compare_logical(0, error_out, 0);  // `cmplwi r31,0`
    const auto release_stream = [&] {
        if (c.u8(stream_at + stream::kind) == stream::owns_data) {
            dispatched::release(c, c.u32(stream_at + stream::mem), c.u32(stream_at + stream::base), 0x107632f8);
            dispatched::release(c, c.u32(stream_at + stream::mem), stream_at, 0x10763304);
        } else {
            const auto mem = c.u32(stream_at + stream::mem);
            // The memory object keeps one stream of its own (+0x130) that it releases itself: a flag at +0x160 says it is free.
            if (stream_at == c.u32(mem + 0x130)) c.put32(mem + 0x160, 1);
            else dispatched::release(c, mem, stream_at, 0x1076332c);
        }
    };
    if (error_out == 0) {
        release_stream();
    } else if (protect(c, frame, c.u32(stream_at + stream::mem), error_out, 0x107632d4, release_stream) != 0) {
        dispatched::free_blocks(c, c.u32(stream_at + stream::mem), 0x10763338);
    }
}

std::uint32_t read_packed(Cpu& c, std::uint32_t stream_at) {
    std::uint32_t value = 0, shift = 0;
    for (;;) {
        const auto position = c.u32(stream_at + stream::position), base = c.u32(stream_at + stream::base);
        c.put32(stream_at + stream::position, position + 1);
        const auto byte = c.u8(base + position);
        value |= shift_left(byte & 0x7f, shift);
        shift += 7;
        if (!(byte & 0x80)) return value;
    }
}

// The three scale callbacks (FNT's graphic state points at one of them for the cvt scaling; called by pointer with the state and a value).
// 0x10763820: (value * mult + add) >> shift.
std::int32_t scale_shifted(Cpu& c, std::uint32_t state, std::int32_t value) {
    const auto mult = c.u32(state + graphic_state::scale_mult), add = c.u32(state + graphic_state::scale_add);
    const auto product = std::uint32_t(value) * mult + add;
    const auto result = shift_right(wrap(product), std::uint32_t(c.i16(state + graphic_state::scale_shift)));
    c.set({{4, std::uint32_t(result)}, {5, add}});
    return result;
}
// 0x10763840: (value * mult + add) / div, rounded toward zero by dividing the magnitude of a negative value.
std::int32_t scale_divided(Cpu& c, std::uint32_t state, std::int32_t value) {
    const auto mult = c.u32(state + graphic_state::scale_mult), add = c.u32(state + graphic_state::scale_add);
    if (value > -1) {
        const auto product = std::uint32_t(value) * mult;
        c.set({{4, product}, {5, add}});
        return wrap(std::uint32_t(divide_signed<4>(add + product, c.u32(state + graphic_state::scale_div))));
    }
    const auto product = negate(std::uint32_t(value)) * mult;
    c.set({{4, product}, {5, add}});
    const auto quotient = std::uint32_t(divide_signed<4>(add + product, c.u32(state + graphic_state::scale_div)));
    return wrap(negate(quotient));
}
// 0x10763890: the 16.16 product of the value and the fixed ratio.
std::int32_t scale_fixed(Cpu& c, std::uint32_t state, std::int32_t value) {
    Frame frame(c, 0x10763890, 0x40, {});
    Leftovers left{c.u32(state + graphic_state::round_unit), c.r(5), c.r(6), c.r(7), c.r(8)};
    const auto result = fix_multiply(value, wrap(left.r4), left);
    c.set({{4, left.r4}, {5, left.r5}, {6, left.r6}, {7, left.r7}, {8, left.r8}});
    return result;
}

void scale_ratio(Cpu& c, std::uint32_t size_a, std::uint32_t size_b, std::uint32_t out) {
    Frame frame(c, 0x107643d0, 0x50, {.first_gpr = 27});
    const auto pool = c.toc(toc::truetype_pool);
    auto numerator = wrap(size_a << 6), denominator = wrap(size_b);
    while (((numerator | denominator) & 1) == 0) {
        numerator >>= 1;
        denominator >>= 1;
    }
    c.put16(out + 0x10, 2);
    const auto half = denominator >> 1;
    Leftovers left{};
    c.put32(out + 0xc, std::uint32_t(fix_divide(numerator, denominator, left)));
    if (numerator <= -0x8000 || numerator >= 0x8000) return;
    c.put16(out + 0x0, std::uint32_t(numerator));
    std::uint32_t shift = 0;
    while ((shift_right(denominator, std::uint32_t(std::int16_t(shift))) & 1) == 0) ++shift;
    if (shift_right(denominator, std::uint32_t(std::int16_t(shift))) == 1) {
        c.put16(out + 0x10, 0);
        if (shift_left(1, std::uint32_t(std::int16_t(shift))) != std::uint32_t(denominator)) dispatched::assertion(c, pool, 0, 0x16, 0x49, 0x10764494);
    } else {
        c.put16(out + 0x10, 1);
    }
    c.put16(out + 0x2, shift);
    c.put32(out + 0x4, std::uint32_t(denominator));
    c.put32(out + 0x8, std::uint32_t(half));
}

void delete_hint_font(Cpu& c, std::uint32_t hint) {
    Frame frame(c, 0x107642e0, 0x50, {.first_gpr = 28});
    if (hint == 0) return;
    const auto state = hint + hint_font::state;
    const auto mem = [&] { return c.u32(hint + hint_font::mem); };
    // r28 the hint font, r29 its state; r30 and r31 count the program pointers below (the stack leaver saves all four).
    c.set({{28, hint}, {29, state}});
    dispatched::release(c, mem(), c.u32(hint + hint_font::state + graphic_state::scratch), 0x10764314);
    dispatched::release(c, mem(), c.u32(state + graphic_state::stack), 0x10764320);
    dispatched::release(c, mem(), c.u32(state + graphic_state::storage), 0x1076432c);
    dispatched::release(c, mem(), c.u32(state + graphic_state::cvt), 0x10764338);
    dispatched::release(c, mem(), c.u32(state + graphic_state::function_defs), 0x10764344);
    dispatched::release(c, mem(), c.u32(state + graphic_state::instruction_defs), 0x10764350);
    for (std::uint32_t k = 0; k < 2; ++k) {
        c.set({{30, k}, {31, 4 * k}});
        dispatched::release(c, mem(), c.u32(state + graphic_state::prep + 4 * k), 0x10764368);
    }
    c.set({{30, 2}, {31, 8}});
    dispatched::release(c, mem(), c.u32(hint + hint_font::zone_block), 0x10764384);
    dispatched::release(c, mem(), c.u32(hint + hint_font::cvt_words), 0x10764390);
    dispatched::release(c, mem(), c.u32(hint + hint_font::zones), 0x1076439c);
    dispatched::release(c, mem(), hint, 0x107643a8);
}

std::uint32_t cmap4_lookup(Cpu& c, std::uint32_t table, std::uint32_t code) {
    code &= 0xffff;
    const auto record = c.u32(c.u32(table + cmap::encodings) + std::uint32_t(c.i16(table + cmap::chosen)) * 4);
    const auto subtable = c.u32(table + cmap::data) + c.u32(record + cmap_encoding::offset);
    const auto segment_bytes = big_endian_16(c, subtable + 6);
    auto at = subtable + 14;
    std::uint32_t end;
    do {
        end = big_endian_16(c, at);
        at += 2;
    } while (code > end);
    at += segment_bytes;
    const auto start = big_endian_16(c, at);
    if (code < start) return 0;
    at += segment_bytes;
    const auto delta = big_endian_16(c, at);
    at += segment_bytes;
    const auto range_offset = big_endian_16(c, at);
    const auto within = (code - start) & 0xffff;
    if (range_offset == 0) return (code + delta) & 0xffff;
    return (big_endian_16(c, range_offset + within + at + within) + delta) & 0xffff;
}

std::uint32_t cmap6_lookup(Cpu& c, std::uint32_t table, std::uint32_t code) {
    const auto record = c.u32(c.u32(table + cmap::encodings) + std::uint32_t(c.i16(table + cmap::chosen)) * 4);
    auto at = c.u32(table + cmap::data) + c.u32(record + cmap_encoding::offset);
    const auto first = big_endian_16(c, at + 6), count = big_endian_16(c, at + 8);
    at += 10;
    const auto index = (code - first) & 0xffff;
    std::uint32_t glyph = 0;
    if (index < count) {
        at += index + index;
        glyph = big_endian_16(c, at);
    }
    c.set(9, at);
    return glyph;
}

std::uint32_t new_hint_font(Cpu& c, std::uint32_t mem, std::uint32_t input, std::uint32_t font) {
    Frame frame(c, 0x10763390, 0x70, {.first_gpr = 21});
    const auto pool = c.toc(toc::t2ktt_pool);
    const auto hint = dispatched::allocate(c, mem, hint_font::size, 0x107633b8);
    const auto maxp_table = c.u32(font + sfnt::maxp);
    const auto state = hint + hint_font::state;
    c.put32(hint + hint_font::mem, mem);
    c.set(3, font);
    c.call(units_per_em_entry, 0x107633d4);
    c.put16(hint + hint_font::units_per_em, c.r(3));
    if (maxp_table == 0) dispatched::assertion(c, pool, 0, 0xd, 0x44, 0x107633f0);
    c.put32(hint + hint_font::ppem_x, 0xffffffff);
    c.put32(hint + hint_font::ppem_y, 0x7fffa55a);
    c.put32(state + graphic_state::scratch, dispatched::allocate(c, mem, 0x4c8, 0x10763410));
    c.set(3, state);
    c.call(hint_installer, 0x1076341c);
    c.put32(state + graphic_state::stack, dispatched::allocate(c, mem, std::uint32_t(c.i16(maxp_table + maxp::max_stack_elements)) << 2, 0x1076342c));
    c.put32(state + graphic_state::storage, dispatched::allocate(c, mem, std::uint32_t(c.i16(maxp_table + maxp::max_storage)) << 2, 0x10763440));
    const auto storage_count = c.i16(maxp_table + maxp::max_storage);
    for (std::int32_t n = 0; n < storage_count; ++n) c.put32(c.u32(state + graphic_state::storage) + 4 * std::uint32_t(n), 0);
    // The original zeroes eight words an iteration when there are more than eight, reloading the storage pointer into r5..r11 each time: r9 and
    // r10 keep it (nothing after writes them).
    if (storage_count > 8) c.set({{9, c.u32(state + graphic_state::storage)}, {10, c.u32(state + graphic_state::storage)}});
    auto points = std::int32_t(c.i16(maxp_table + maxp::max_points));
    if (c.i16(maxp_table + maxp::max_composite_points) > points) points = c.i16(maxp_table + maxp::max_composite_points);
    const auto upoints = std::uint32_t(points);
    c.put32(hint + hint_font::zone_block, dispatched::allocate(c, c.u32(hint + hint_font::mem), upoints + ((upoints + 4) << 3) + 4, 0x10763538));
    c.put32(state + graphic_state::cvt, 0);
    c.put32(hint + hint_font::cvt_words, 0);
    c.put32(hint + hint_font::cvt_count, 0);
    if (const auto entry = find_table_entry(c, font, tag::cvt)) {
        const auto sub = new_substream(c, c.u32(hint + hint_font::mem), input, c.u32(entry + table_entry::offset), c.u32(entry + table_entry::length), 0, 0);
        const auto length = c.u32(entry + table_entry::length);
        auto count = length >> 1;
        c.put32(state + graphic_state::cvt, dispatched::allocate(c, mem, (length << 1) & 0xfffffffcu, 0x10763598));
        auto words = dispatched::allocate(c, mem, count << 1, 0x107635a8);
        c.put32(hint + hint_font::cvt_words, words);
        c.put32(hint + hint_font::cvt_count, count);
        while (wrap(count) > 0) {
            c.put16(words, std::uint32_t(read_signed_16(c, sub)));
            words += 2;
            --count;
        }
        // The delete frees through the stack leaver 0x10767bb0, which saves r28/r29 as this routine holds them: the maxp and the sfnt.
        c.set({{28, maxp_table}, {29, font}});
        delete_input_stream(c, sub, 0);
    }
    c.put32(state + graphic_state::function_defs, dispatched::allocate(c, mem, std::uint32_t(c.i16(maxp_table + maxp::max_function_defs)) << 3, 0x107635f0));
    c.put32(state + graphic_state::instruction_defs, dispatched::allocate(c, mem, std::uint32_t(c.i16(maxp_table + maxp::max_instruction_defs)) << 3, 0x10763604));
    for (std::uint32_t k = 0; k < 2; ++k) {
        c.put32(state + graphic_state::prep + 4 * k, 0);
        const auto entry = find_table_entry(c, font, k == 0 ? tag::prep : tag::fpgm);
        if (entry == 0) continue;
        stream_seek(c, input, c.u32(entry + table_entry::offset));
        c.put32(hint + hint_font::prep_length + 4 * k, c.u32(entry + table_entry::length));
        c.put32(state + graphic_state::prep + 4 * k, dispatched::allocate(c, mem, c.u32(hint + hint_font::prep_length + 4 * k), 0x1076366c));
        read_bytes(c, input, c.u32(state + graphic_state::prep + 4 * k), std::int32_t(c.u32(hint + hint_font::prep_length + 4 * k)));
    }
    c.put32(state + graphic_state::defaults + 0x18, c.toc(toc::default_state_word));
    c.put32(state + graphic_state::defaults + 0x14, 0x40);
    c.put32(state + graphic_state::defaults, 0x44);
    c.put32(state + graphic_state::defaults + 4, 0);
    c.put16(state + 0x54, 0);
    c.put8(state + 0x56, 1);
    c.put16(state + 0x4e, 9);
    c.put16(state + 0x50, 3);
    c.put16(state + 0x52, 0x80);
    c.put32(state + 0x30, 0);
    c.put32(state + 0x34, 0);
    const auto twilight = std::uint32_t(c.i16(maxp_table + maxp::max_twilight_points));
    c.put16(hint + hint_font::twilight_count, twilight);
    const auto zones = dispatched::allocate(c, mem, 22 * twilight, 0x10763700);
    c.put32(hint + hint_font::zones, zones);
    c.put32(hint + 0xf8, zones + 4 * twilight);
    c.put32(hint + 0xfc, zones + 8 * twilight);
    c.put32(hint + 0x100, zones + 12 * twilight);
    c.put32(hint + 0x104, zones + 16 * twilight);
    c.put32(hint + 0x108, zones + 18 * twilight);
    c.put32(hint + 0x10c, zones + 20 * twilight);
    c.put32(hint + 0x11c, zones + 21 * twilight);
    c.put16(hint + 0xf0, 0);
    c.put16(hint + 0xf2, 0);
    c.put16(hint + 0x110, 1);
    c.put16(hint + 0x112, 0);
    c.put32(hint + 0x114, hint + 0xf0);
    c.put32(hint + 0x118, hint + 0xf2);
    if (c.u32(state + graphic_state::fpgm) != 0) {
        c.put8(state + graphic_state::size_flag_c9, 1);
        for (std::uint32_t at = 0; at < 0x30; at += 4) c.put32(state + graphic_state::saved + at, c.u32(state + graphic_state::defaults + at));
        c.put16(state + graphic_state::saved + 0x30, c.u16(state + 0x54));
        c.put16(state + graphic_state::saved + 0x32, c.u16(state + 0x56));
        c.put8(state + graphic_state::size_flag_c8, 1);
        const auto program = c.u32(state + graphic_state::fpgm);
        c.set({{3, hint + hint_font::zones}, {4, program}, {5, program + c.u32(hint + hint_font::fpgm_length)}, {6, state}, {7, 0}});
        c.call(hint_interpreter, 0x10763804);
    }
    return hint;
}

void set_hint_size(Cpu& c, std::uint32_t hint, std::int32_t x, std::int32_t y) {
    if (hint == 0) return;
    if (c.i32(hint + hint_font::ppem_x) == x && c.i32(hint + hint_font::ppem_y) == y) return;
    Frame frame(c, 0x107638c0, 0x90, {.first_gpr = 16});
    const auto state = hint + hint_font::state;
    c.put32(hint + hint_font::ppem_x, std::uint32_t(x));
    c.put32(hint + hint_font::ppem_y, std::uint32_t(y));
    c.put32(hint + hint_font::x_scale, 0x10000);
    c.put32(hint + hint_font::y_scale, 0x10000);
    Leftovers left{};
    if (x == y) {
        c.put8(state + graphic_state::mode, 1);
        c.put16(state + 0xc, std::uint32_t(x));
    } else if (x > y) {
        c.put8(state + graphic_state::mode, 0);
        c.put16(state + 0xc, std::uint32_t(x));
        c.put32(state + graphic_state::ratio_up, std::uint32_t(fix_divide(y, x, left)));
    } else {
        c.put8(state + graphic_state::mode, 0);
        c.put16(state + 0xc, std::uint32_t(y));
        c.put32(state + graphic_state::ratio_down, std::uint32_t(fix_divide(x, y, left)));
    }
    c.put32(state + 0x10, c.u16(state + 0xc));
    c.put32(state + 0x10, c.u32(state + 0x10) << 16);
    c.put16(state + 0xe, c.u16(state + 0xc));
    for (std::uint32_t at = 0x14; at <= 0x20; at += 4) c.put32(state + at, 0);
    // The record the ratio is returned in lives in this frame (+0x38); what it does not fill is the stack's old content, which the
    // rest of the routine reads as the original does.
    const auto record = frame.at(0x38);
    scale_ratio(c, c.u16(state + 0xc), c.u16(hint + hint_font::units_per_em), record);
    const auto kind = c.i16(record + 0x10);
    const auto mult = std::int32_t(c.i16(record)), shift = std::int32_t(c.i16(record + 2));
    const auto denominator = c.u32(record + 4), round = c.u32(record + 8), fixed = c.u32(record + 0xc);
    const auto count = c.i32(hint + hint_font::cvt_count);
    const auto cvt_in = [&](std::int32_t n) { return std::int32_t(c.i16(c.u32(hint + hint_font::cvt_words) + 2 * std::uint32_t(n))); };
    const auto cvt_out = [&](std::int32_t n, std::uint32_t value) { c.put32(c.u32(state + graphic_state::cvt) + 4 * std::uint32_t(n), value); };
    if (kind == 1) {
        c.put32(state + graphic_state::scale_callback, c.toc(toc::scale_divide_callback));
        for (std::int32_t n = 0; n < count; ++n) {
            const auto value = cvt_in(n);
            if (value > -1) cvt_out(n, std::uint32_t(divide_signed<4>(round + std::uint32_t(value) * std::uint32_t(mult), denominator)));
            else cvt_out(n, negate(std::uint32_t(divide_signed<4>(round + negate(std::uint32_t(value)) * std::uint32_t(mult), denominator))));
        }
    } else if (kind == 0) {
        c.put32(state + graphic_state::scale_callback, c.toc(toc::scale_shift_callback));
        const auto scaled = [&](std::int32_t n) { return std::uint32_t(shift_right(wrap(std::uint32_t(cvt_in(n)) * std::uint32_t(mult) + round), std::uint32_t(shift))); };
        for (std::int32_t n = 0; n < count; ++n) cvt_out(n, scaled(n));
        // Eight values an iteration above eight: the last iteration leaves r10 the cvt pointer and r9 its seventh value.
        if (count > 8) c.set({{9, scaled(8 * ((count - 8 + 7) >> 3) - 8 + 6)}, {10, c.u32(hint + hint_font::cvt_words)}});
    } else if (kind == 2) {
        c.put32(state + graphic_state::scale_callback, c.toc(toc::scale_fixed_callback));
        for (std::int32_t n = 0; n < count; ++n) cvt_out(n, std::uint32_t(fix_multiply(cvt_in(n), wrap(fixed), left)));
    }
    c.put32(state + graphic_state::scale_mult, std::uint32_t(mult));
    c.put16(state + graphic_state::scale_shift, std::uint32_t(shift));
    c.put32(state + graphic_state::scale_div, denominator);
    c.put32(state + graphic_state::scale_add, round);
    c.put32(state + graphic_state::round_unit, fixed);
    const auto twilight = c.i16(hint + hint_font::twilight_count);
    for (std::int32_t n = 0; n < twilight; ++n) {
        const auto word = 4 * std::uint32_t(n), half = 2 * std::uint32_t(n);
        c.put32(c.u32(hint + 0xf4) + word, 0);
        c.put32(c.u32(hint + 0xf8) + word, 0);
        c.put32(c.u32(hint + 0xfc) + word, 0);
        c.put32(c.u32(hint + 0x100) + word, 0);
        c.put16(c.u32(hint + 0x104) + half, 0);
        c.put16(c.u32(hint + 0x108) + half, 0);
        c.put8(c.u32(hint + 0x10c) + std::uint32_t(n), 0);
        c.put8(c.u32(hint + 0x11c) + std::uint32_t(n), 0);
    }
    const auto prep = c.u32(state + graphic_state::prep);
    if (prep == 0) return;
    c.put8(state + graphic_state::size_flag_c9, 0);
    for (std::uint32_t at = 0; at < 0x30; at += 4) c.put32(state + graphic_state::saved + at, c.u32(state + graphic_state::defaults + at));
    c.put16(state + graphic_state::saved + 0x30, c.u16(state + 0x54));
    c.put16(state + graphic_state::saved + 0x32, c.u16(state + 0x56));
    c.put8(state + graphic_state::size_flag_c8, 1);
    c.put16(state + 0xca, 0);
    c.set({{3, hint + hint_font::zones}, {4, prep}, {5, prep + c.u32(hint + hint_font::prep_length)}, {6, state}, {7, 0}});
    c.call(hint_interpreter, 0x10763e20);
    for (std::uint32_t at = 0; at < 0x30; at += 4) c.put32(state + graphic_state::defaults + at, c.u32(state + graphic_state::saved + at));
    c.put16(state + 0x54, c.u16(state + graphic_state::saved + 0x30));
    c.put16(state + 0x56, c.u16(state + graphic_state::saved + 0x32));
}

void load_glyph_program(Cpu& c, std::uint32_t hint, std::uint32_t glyph) {
    if (hint == 0) return;
    if (c.u32(glyph + glyph_record::instructions) == 0) return;
    if (c.i32(glyph + glyph_record::instruction_length) <= 0) return;
    Frame frame(c, 0x10763eb0, 0x50, {.first_gpr = 26});
    // The glyph's points and the four phantom points after them.
    const auto points = std::uint32_t(c.i16(glyph + glyph_record::outline_points)) + 4;
    const auto count = wrap(points);
    const auto original_xs = c.u32(glyph + glyph_record::original_xs), original_ys = c.u32(glyph + glyph_record::original_ys);
    c.put32(hint + hint_font::glyph_original_xs, original_xs);
    c.put32(hint + hint_font::glyph_original_ys, original_ys);
    const auto zone = c.u32(hint + hint_font::zone_block);
    c.put32(hint + hint_font::zone_xs, zone);
    c.put32(hint + hint_font::zone_ys, zone + points * 4);
    c.put32(hint + hint_font::glyph_xs, c.u32(glyph + glyph_record::xs));
    c.put32(hint + hint_font::glyph_ys, c.u32(glyph + glyph_record::ys));
    c.put32(hint + hint_font::glyph_on_curve, c.u32(glyph + glyph_record::on_curve));
    c.put16(hint + hint_font::glyph_contours, c.u16(glyph + glyph_record::contours));
    c.put16(hint + hint_font::glyph_contour_flag, 0);
    c.put32(hint + hint_font::glyph_contour_starts, c.u32(glyph + glyph_record::contour_starts));
    c.put32(hint + hint_font::glyph_contour_ends, c.u32(glyph + glyph_record::contour_ends));
    c.put32(hint + hint_font::zone_touched, zone + points * 8);
    for (std::int32_t n = 0; n < count; ++n) {
        c.put32(zone + 4 * std::uint32_t(n), c.u32(original_xs + 4 * std::uint32_t(n)));
        c.put8(zone + points * 8 + std::uint32_t(n), 0);
    }
    for (std::int32_t n = 0; n < count; ++n) c.put32(zone + points * 4 + 4 * std::uint32_t(n), c.u32(original_ys + 4 * std::uint32_t(n)));
    // The glyph's own coordinates scaled by the font's ratios, y then x, in 16.16.
    Leftovers left{};
    for (const auto [scale_at, array_at] : {std::pair{hint_font::y_scale, hint_font::glyph_ys}, std::pair{hint_font::x_scale, hint_font::glyph_xs}}) {
        const auto scale = c.u32(hint + scale_at);
        if (scale == 0x10000) continue;
        auto at = c.u32(hint + array_at);
        for (std::int32_t n = 0; n < count; ++n, at += 2) c.put16(at, std::uint32_t(fix_multiply(c.i16(at), wrap(scale), left)));
    }
    // The phantom points after the outline's, rounded to 1/64 pixel: the first moved by half a unit and masked, the second placed after
    // it by the width rounded the same way (x), then the same for the next pair (y).
    const auto phantom = std::uint32_t(c.i16(glyph + glyph_record::outline_points)) * 4;
    auto row = original_xs + phantom;
    auto before = c.u32(row), after = c.u32(row + 4);
    c.put32(row, before + 0x20);
    auto width = (after - before + 0x20) & 0xffffffc0u;
    c.put32(row, c.u32(row) & 0xffffffc0u);
    c.put32(row + 4, width + c.u32(row));
    row = original_ys + phantom;
    before = c.u32(row + 8);
    after = c.u32(row + 0xc);
    c.put32(row + 8, before + 0x20);
    width = (after - before + 0x20) & 0xffffffc0u;
    c.put32(row + 8, c.u32(row + 8) & 0xffffffc0u);
    c.put32(row + 0xc, width + c.u32(row + 8));
    c.set({{9, row}, {10, original_ys}});  // the registers the last loads leave: the y row and the y array
    c.put8(hint + hint_font::flag_e1, 0);
    for (std::uint32_t at = 0; at < 0x30; at += 4) c.put32(hint + hint_font::saved_defaults + at, c.u32(hint + hint_font::state + graphic_state::defaults + at));
    c.put16(hint + hint_font::saved_flag_a0, c.u16(hint + hint_font::state + 0x54));
    c.put16(hint + hint_font::saved_flag_a0 + 2, c.u16(hint + hint_font::state + 0x56));
    c.put8(hint + hint_font::flag_e0, 0);
    const auto program = c.u32(glyph + glyph_record::instructions);
    c.set({{3, hint + hint_font::zones}, {4, program}, {5, program + c.u32(glyph + glyph_record::instruction_length)}, {6, hint + hint_font::state}, {7, 0}});
    c.call(hint_interpreter, 0x107642c0);
}

}

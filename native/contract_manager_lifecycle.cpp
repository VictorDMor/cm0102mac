#include "contract_manager_detail.hpp"
#include <array>
#include <utility>

namespace cm::aot::contract_manager {
using namespace detail;

std::uint32_t construct(Cpu& c, std::uint32_t manager_address, std::uint32_t file_address) {
    Frame frame(c, 0x10077ee0, 0x410, {.first_gpr = 13});
    // The date globals the reference keeps across its calls; callee frames
    // save these values as part of the observable stack image.
    Kept<17> deadline_15(c, c.toc(-0x79d8));
    Kept<30> manager(c, manager_address);
    Kept<18> deadline_14(c, c.toc(-0x79d4));
    Kept<31> file(c, file_address);
    Kept<19> deadline_13(c, c.toc(-0x79d0));
    Kept<20> deadline_12(c, c.toc(-0x79cc));
    Kept<21> deadline_11(c, c.toc(-0x79c8));
    Kept<22> deadline_10(c, c.toc(-0x79c4));
    Kept<23> deadline_9(c, c.toc(-0x79c0));
    Kept<24> deadline_8(c, c.toc(-0x79bc));
    Kept<25> deadline_7(c, c.toc(-0x79b8));
    Kept<14> deadline_6(c, c.toc(-0x79b4));
    Kept<13> deadline_5(c, c.toc(-0x79b0));
    Kept<26> today(c, c.toc(-0x7b80));
    Kept<27> pool(c, c.toc(slot::pool));
    Kept<28> encoded_first(c, c.toc(-0x5714));
    Kept<29> encoded_second(c, c.toc(-0x5710));
    Kept<15> second_prefix(c, c.r(15));
    Kept<16> medium_found(c, 0);
    Program program(c);

    // Build the original Pascal filename from its two interleaved pieces,
    // then ask FSMakeFSSpec for it. Failed checks display the same prompt
    // and retry; no result or filesystem behaviour is bypassed.
    do {
        std::array<std::uint32_t, 4> second_words{}, first_words{};
        for (unsigned i = 0; i < 4; ++i) second_words[i] = c.u32(encoded_second + i * 4);
        second_prefix = second_words[0];
        c.set(12, second_words[1]); c.set(11, second_words[2]); c.set(10, second_words[3]);
        medium_found = 0;
        const auto second_tail = c.u8(encoded_second + 16);
        c.set(9, second_tail);
        for (unsigned i = 0; i < 4; ++i) first_words[i] = c.u32(encoded_first + i * 4);
        c.set(8, first_words[0]); c.set(7, first_words[1]); c.set(6, first_words[2]); c.set(0, first_words[3]);
        for (unsigned i = 0; i < 4; ++i) c.put32(frame.at(0xc8 + i * 4), second_words[i]);
        c.put8(frame.at(0xd8), second_tail);
        for (unsigned i = 0; i < 4; ++i) c.put32(frame.at(0xb8 + i * 4), first_words[i]);
        auto output = frame.at(0x125), second_cursor = frame.at(0xc9), first_cursor = frame.at(0xb9);
        c.set(3, output); c.set(4, second_cursor); c.set(5, first_cursor);
        for (;;) {
            const auto next = c.i8(second_cursor);
            c.set(0, std::uint32_t(std::int32_t(next)));
            if (!nonzero(c, next)) break;
            const auto second_byte = c.u8(second_cursor++), first_byte = c.u8(first_cursor++);
            c.set(6, second_byte); c.set(0, first_byte);
            c.put8(output, second_byte); c.put8(output + 1, first_byte);
            output += 2;
            c.set(3, output); c.set(4, second_cursor); c.set(5, first_cursor);
        }
        const auto second_length = c.u8(frame.at(0xc8)), first_length = c.u8(frame.at(0xb8));
        c.set(7, second_length);
        c.set(0, std::uint32_t(second_length) + first_length);
        c.put8(frame.at(0x124), std::uint32_t(second_length) + first_length);
        const auto status = program.call(0x1077fee0, 0x10077fdc, {0, 0, frame.at(0x124), frame.at(0xdc)});
        c.set(2, c.u32(c.sp() + 0x14));
        const auto error = std::int16_t(status);
        c.set(0, extended(error));
        if (!nonzero(c, error)) medium_found = 1;
        if (!nonzero(c, medium_found)) {
            program.call(0x10727890, 0x10078000, {frame.at(0x2ec), pool});
            program.call(0x10727890, 0x1007800c, {frame.at(0x224), pool + 10});
            program.call(0x10769b60, 0x1007801c, {frame.at(0x2ec), frame.at(0x224), 1});
        }
    } while (!nonzero(c, medium_found));

    c.set(0, 0);
    for (const auto offset : {0u, 8u, 4u, 0x20u, 0x24u, 0x28u, 0x2cu, 0x30u,
                              0xcu, 0x14u, 0x10u, 0x18u, 0x1cu, 0x34u, 0x38u})
        c.put32(manager + offset, 0);
    constexpr std::array<unsigned, 16> intervals{7, 30, 60, 91, 121, 152, 182, 365, 550, 1095, 7, 30, 42, 60, 70, 91};
    auto kept_deadline = [&](unsigned index) -> std::uint32_t {
        switch (index) {
        case 5: return deadline_5; case 6: return deadline_6; case 7: return deadline_7;
        case 8: return deadline_8; case 9: return deadline_9; case 10: return deadline_10;
        case 11: return deadline_11; case 12: return deadline_12; case 13: return deadline_13;
        case 14: return deadline_14; default: return deadline_15;
        }
    };
    bool has_file = false;
    for (unsigned index = 0; index < intervals.size(); ++index) {
        const auto local = 0xb0 - index * 8;
        const auto back = index < 6 ? 0x10078074 + index * 0x30 : 0x1007818c + (index - 6) * 0x28;
        program.call(0x10717890, back, {frame.at(local), today, intervals[index]});
        const auto year = c.u16(frame.at(local + 2));
        if (index < 5) {
            c.set(5, year); c.set(4, today);
            const auto destination = c.toc(-0x799c - std::int32_t(index) * 4);
            c.set(3, destination);
            const auto day = c.i16(frame.at(local));
            c.set(7, extended(day));
            c.put16(destination + 2, year);
            c.set(3, frame.at(local - 8));
            const auto again = c.toc(-0x799c - std::int32_t(index) * 4);
            c.set(6, again); c.set(5, intervals[index + 1]);
            const auto flag = c.u8(frame.at(local + 4));
            c.set(0, flag);
            c.put16(again, extended(day)); c.put8(again + 4, flag);
        } else if (index < 15) {
            c.set(3, year); c.set(4, today);
            const auto day = c.i16(frame.at(local));
            c.set(6, extended(day)); c.set(5, intervals[index + 1]);
            const auto destination = kept_deadline(index);
            c.put16(destination + 2, year);
            c.set(3, frame.at(local - 8));
            const auto flag = c.u8(frame.at(local + 4));
            c.set(0, flag);
            c.put16(destination, extended(day)); c.put8(destination + 4, flag);
        } else {
            c.set(3, year);
            has_file = nonzero_logical(c, file);
            const auto flag = c.u8(frame.at(local + 4));
            c.set(0, flag);
            const auto destination = kept_deadline(index);
            c.put16(destination + 2, year);
            const auto day = c.i16(frame.at(local));
            c.set(3, extended(day));
            c.put16(destination, extended(day)); c.put8(destination + 4, flag);
        }
    }
    if (has_file) {
        const auto loaded = std::int8_t(program.call(0x10091bf0, 0x10078320, {manager, file}));
        c.set(0, std::uint32_t(std::int32_t(loaded)));
        nonzero(c, loaded);
    }
    return result(c, manager);
}

std::uint32_t destroy(Cpu& c, std::uint32_t manager_address, std::uint32_t deleting) {
    Frame frame(c, 0x10078350, 0x40, {.first_gpr = 30});
    Kept<31> flag(c, deleting);
    Kept<30> manager(c, manager_address);
    Program program(c);
    if (nonzero(c, std::int32_t(manager_address))) {
        for (const auto [offset, back] : {std::pair{0u, 0x10078380u}, {0xcu, 0x10078390u}, {0x28u, 0x100783a0u}}) {
            const auto buffer = c.u32(manager + offset);
            c.set(3, buffer);
            if (nonzero_logical(c, buffer)) program.call(0x10775170, back);
        }
        for (const auto [offset, back] : {std::pair{0x34u, 0x100783b4u}, {0x38u, 0x100783c8u}}) {
            const auto list = c.u32(manager + offset);
            c.set(3, list);
            if (nonzero_logical(c, list)) program.call(0x10738c20, back, {list, 1});
        }
        const auto mode = std::int16_t(std::uint32_t(flag));
        c.set(0, extended(mode));
        if (compare(c, mode) > 0) program.call(0x107735a0, 0x100783d8, {manager});
    }
    return result(c, manager);
}
}

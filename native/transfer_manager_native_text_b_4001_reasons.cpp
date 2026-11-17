// TRANSFER_MANAGER.CPP, block "text_b": the reasons of a kind-2 signing (0x104001e0, 0x10400350-0x104004e4).
#include "transfer_manager_native_text_b.hpp"
#include "transfer_manager_native_text_b_4001.hpp"

namespace cm::aot::transfer_manager_native::text_b {
namespace {
constexpr std::uint32_t reason_text_size = 100, competition_cells = -0x7bbc, competition_size = 0x6c, competition_name_length = 0x67,
                        competition_name = 0x4d;
// Reason bits of the item's field 8 and the text each one asks for (offsets from the text base).
constexpr std::uint32_t bit_a = 0x200, bit_b = 0x400, bit_c = 0x100, bit_d = 0x800, bit_competition = 0x1000, bit_e = 0x2000,
                        skip_bits = 0xa;
constexpr std::uint32_t text_a = 0x4af8, text_b = 0x4b2a, text_c = 0x4b56, text_d = 0x4b77, text_competition = 0x4b9f,
                        text_e = 0x4be8, text_two = 0x4c1c, text_one = 0x4c19;
}

void build_reasons(Signing& s) {
    auto& c = s.c;
    const auto reason = std::uint32_t(s.reason);
    const auto first = s.frame.at(Signing::reason_texts_at);
    const auto summary = s.reasons();
    // The count of reasons so far is the original's r16 (it held the item's kind until here).
    std::int8_t count = 0;
    c.put8(summary, 0);
    keep(c, 16, 0);
    if ((reason & skip_bits) == 0) {
        const auto room = [&] { return count < 2; };
        const auto slot = [&] { return first + std::uint32_t(count) * reason_text_size; };
        const auto added = [&] {
            ++count;
            keep(c, 16, std::uint32_t(std::int32_t(count)));
        };
        const auto has_a = reason & bit_a;
        keep(c, 24, has_a);
        if (has_a) {
            s.say_to(first, 0x1040037c, text_a, {});
            count = 1;
            keep(c, 16, 1);
        }
        // room() is always true here: only the first reason has been added, so the count is at most 1.
        if (room() && (reason & bit_b) && has_a == 0) {
            s.say_to(slot(), 0x104003b0, text_b, {});
            added();
        }
        // room() is always true here: the first two reasons need bit_a and no bit_a, so at most one was added.
        if (room() && (reason & bit_c)) {
            s.say_to(slot(), 0x104003dc, text_c, {});
            added();
        }
        if (room() && (reason & bit_d)) {
            s.say_to(slot(), 0x10400408, text_d, {});
            added();
        }
        c.set(6, std::uint32_t(std::int32_t(count)));  // 0x10400410: extsb r6,r16; the later calls read r6 as it stays
        if (room() && (reason & bit_competition) && !(reason & bit_d)) {
            // r8 holds the competition table the original loaded; the formatter is handed it.
            const auto table = c.u32(c.toc(competition_cells));
            const auto record = table + std::uint32_t(s.competition) * competition_size;
            const auto length = std::uint32_t(std::int32_t(c.i8(record + competition_name_length)));
            s.say_to(slot(), 0x10400460, text_competition, {length, length, record + competition_name, table});
            added();
        }
        if (room() && (reason & bit_e) && has_a == 0 && !(reason & bit_b)) {
            s.say_to(slot(), 0x1040049c, text_e, {});
            added();
        }
    }
    if (count == 2) {
        s.say_to(summary, 0x104004e4, text_two, {first, first + reason_text_size});
    } else if (count >= 1 && count < 2) {  // count < 2 always holds here: each reason is added only while count < 2, so count <= 2
        invoke(c, 0x1077b720, 0x104004cc, {summary, s.text_base + text_one, first});
    }
}
}

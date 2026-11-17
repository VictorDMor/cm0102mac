#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_b": the signing message (0x104001e0) and its parts (private to the block).
#include "guest_abi.hpp"
#include "transfer_manager_native_text_b_detail.hpp"
#include <initializer_list>

namespace cm::aot::transfer_manager_native::text_b {
// What the signing message reads of its news item, and the frame buffers it writes into.
struct Signing {
    guest::Cpu& c;
    guest::Frame& frame;
    std::uint32_t text, text_base, output;
    std::int32_t person, from, to;
    bool flagged;
    std::int32_t kind, reason, marker, amount, competition, extra;

    static constexpr std::uint32_t date_at = 0x498, cash_at = 0x4c, reasons_at = 0x308, reason_texts_at = 0x114;
    std::uint32_t date() const { return frame.at(date_at); }
    std::uint32_t cash() const { return frame.at(cash_at); }
    std::uint32_t reasons() const { return frame.at(reasons_at); }
    std::uint32_t len(std::int32_t club) const { return std::uint32_t(club_name_len(c, club_at(c, club))); }
    std::uint32_t name(std::int32_t club) const { return club_at(c, club) + club_name; }
    // The formatter into `destination`: values go to r5-r10, the rest to the stack words from sp+0x38.
    void say_to(std::uint32_t destination, std::uint32_t back, std::uint32_t format, std::initializer_list<std::uint32_t> values) const {
        c.set(3, destination);
        c.set(4, text_base + format);
        unsigned n = 5;
        std::uint32_t stack = 0x38;
        for (const auto value : values) {
            if (n <= 10) c.set(n++, value);
            else {
                c.put32(frame.at(stack), value);
                stack += 4;
            }
        }
        c.call(format_fn, back);
    }
    void say(std::uint32_t back, std::uint32_t format, std::initializer_list<std::uint32_t> values) const { say_to(text, back, format, values); }
};
// The one or two reasons written for a kind-2 signing (0x104003xx-0x104004xx): text for each into the frame, then the summary.
void build_reasons(Signing& s);
// The sentence for the kind, reason and amount (0x10400500-0x1040171c).
void write_signing(Signing& s);
}

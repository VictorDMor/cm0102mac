#include "transfer_manager_native_recipients.hpp"
#include "transfer_manager_native_recipients_held.hpp"
#include "transfer_manager_native_detail.hpp"
#include "transfer_manager_native_status.hpp"
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t find_contract = 0x10082f20;  // (contract manager, person) -> contract or 0
constexpr std::uint32_t club_takes_news = 0x102cdbf0;
constexpr std::uint32_t iterator_begin = 0x10738c00, iterator_end = 0x10738c20, iterator_next = 0x10738e00, iterator_reset = 0x10738ca0;
constexpr std::uint32_t pending_moves = 0xa5c;  // the manager's list of people whose move is due: item +0 the person, +4 a flag byte, +8 a word
}

void complete_due_moves(Cpu& c, std::uint32_t manager) {
    constexpr std::uint32_t club_status_link = 0x90, person_form = 0x67;
    constexpr std::uint32_t list_at = 0x48, spare_at = 0x38;
    Frame frame(c, 0x103e64e0, 0xc0, {.first_gpr = 14});
    RecipientsHeld r(c);
    const HalfwordDraw draw(c);
    r[20] = c.toc(-0x7b48);
    r[31] = manager;
    r[21] = c.toc(-0x79f0);
    r[22] = draw.seed;
    r[23] = draw.start;
    r[24] = draw.limit;
    r[25] = draw.cursor;
    r[26] = c.toc(-0x4ba0);
    r[27] = c.toc(toc::news);
    r[28] = c.toc(toc::people);
    r[17] = c.toc(toc::contracts);
    r[16] = c.toc(toc::found_contract);
    r[29] = c.toc(toc::clubs);
    const auto call = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        r.publish(c);
        return invoke(c, entry, back, arguments);
    };
    const auto call_native = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        r.publish(c);
        return invoke_native(c, entry, back, arguments);
    };
    const auto advance = [&](std::uint32_t back) {
        r.publish(c);
        draw.advance(back);
    };

    call(iterator_begin, 0x103e652c, {frame.at(list_at)});
    call(iterator_begin, 0x103e6534, {frame.at(spare_at)});
    // The iterator over the pending list starts as a copy of the list's header (its word +4 repeated at +0x10).
    c.put16(frame.at(list_at), c.u16(manager + pending_moves));
    const auto head = c.u32(manager + pending_moves + 4);
    c.put32(frame.at(list_at + 4), head);
    c.put32(frame.at(list_at + 8), c.u32(manager + pending_moves + 8));
    c.put32(frame.at(list_at + 0xc), c.u32(manager + pending_moves + 0xc));
    c.put32(frame.at(list_at + 0xc), head);

    for (;;) {
        const auto entry = call(iterator_next, 0x103e685c, {frame.at(list_at)});
        r[30] = entry;
        if (!entry) break;
        const auto flag = [&] { return std::int8_t(c.u8(entry + 4)); };
        r[18] = c.u32(r[28]) + c.u32(entry) * person_size;
        const auto person = r[18];
        const auto contract = call(find_contract, 0x103e6578, {c.u32(r[17]), person});
        c.put32(r[16], contract);
        r[19] = contract ? c.u32(r[29]) + c.u32(contract + 4) * club_size : 0;
        const auto club = r[19];
        if (!club) continue;
        if (std::int8_t(call(club_takes_news, 0x103e65b0, {c.u32(r[27]), club})) != 0 && flag() == 0) {
            const auto verdict = std::int16_t(call(0x101a0c20, 0x103e65d4, {c.u32(r[21]), person, club}));
            if (verdict != 0) {
                // 0x101b24d0 also reads r6, which the original's `extsh.` left holding the verdict.
                call(0x101b24d0, 0x103e65ec, {c.u32(r[21]), person, club, std::uint32_t(std::int32_t(verdict))});
                continue;
            }
            if (flag() == 0) {
                call(0x100da320, 0x103e660c, {person, club, 0});
                if (c.u32(club + club_status_link)) call(0x100d9b80, 0x103e6624, {person, c.u32(club + club_status_link), 1});
            }
            call(0x101a27d0, 0x103e6634, {c.u32(r[21]), person, club});
        }
        if (flag() == 0) {
            r[15] = std::uint32_t(std::int32_t(std::int8_t(c.u8(person + person_form))));
            const auto form = std::int32_t(r[15]);
            if (form == 0) {
                r[14] = 0;
            } else {
                advance(0x103e6674);
                // |form| < 0x10000 always (a signed byte): the original's floating-point branch (0x103e669c-0x103e676c,
                // taken above 0xffff or below -0xffff) cannot run.
                if (form > 0xffff || form < -0xffff) throw std::logic_error("0x103e669c is unreachable");
                const auto drawn = std::int32_t(draw.value());
                const auto remainder = drawn - drawn / form * form;
                r[14] = std::uint32_t(form > 0 ? remainder : -remainder);
            }
            advance(0x103e67bc);
            const auto roll = std::int32_t(draw.value()) % 20;
            if (std::int32_t(r[14]) < roll || std::int8_t(call(0x100da0b0, 0x103e6808, {person, club})) != 0)
                call(0x100d9b80, 0x103e6820, {person, c.u32(club + club_status_link), 1});
        }
        call(0x10386360, 0x103e682c, {c.u32(r[20]), person});
        call_native(0x1040b470, 0x103e6838, {manager, person});
        call_native(0x103e68c0, 0x103e6854, {manager, person, club, c.u8(entry + 4), c.u32(entry + 8), 0});
    }
    c.put16(frame.at(list_at), c.u16(frame.at(spare_at)));
    c.put32(frame.at(list_at + 4), c.u32(frame.at(spare_at + 4)));
    c.put32(frame.at(list_at + 8), c.u32(frame.at(spare_at + 8)));
    c.put32(frame.at(list_at + 0xc), c.u32(frame.at(spare_at + 0xc)));
    call(iterator_reset, 0x103e688c, {manager + pending_moves});
    call(iterator_end, 0x103e6898, {frame.at(spare_at), 0xffffffff});
    call(iterator_end, 0x103e68a4, {frame.at(list_at), 0xffffffff});
}
}

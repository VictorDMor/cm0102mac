#include "transfer_manager_native_offers.hpp"
#include "transfer_manager_native_offers_common.hpp"

namespace cm::aot::transfer_manager_native {
using namespace offers;

bool review_pending_offer(Cpu& c, std::uint32_t manager, std::uint32_t link) {
    Frame frame(c, 0x103e8bb0, 0xb0, {.first_gpr = 26});
    const auto contracts_cell = c.toc(toc::contracts), found_cell = c.toc(toc::found_contract);
    const auto person = c.u32(c.toc(toc::people)) + c.u32(link) * person_size;
    const auto club = c.u32(c.toc(toc::clubs)) + c.u32(link + 4) * club_size;
    keep(c, 26, contracts_cell);
    keep(c, 27, found_cell);
    keep(c, 28, manager);
    keep(c, 29, link);
    keep(c, 30, club);
    keep(c, 31, person);
    if (std::int8_t(invoke(c, 0x103ecef0, 0x103e8c08, {manager, person, club, 1})) &&
        // The original also loads the club's word at +0x2c into r5 here; the callee does not read it.
        !std::int8_t(invoke_native(c, 0x103ed520, 0x103e8c2c, {manager, person, c.u32(club + 0x2c), link, 0, 0}))) {
        const auto contract = invoke(c, 0x10082f20, 0x103e8c40, {c.u32(contracts_cell), person});
        c.put32(found_cell, contract);
        // The contract's kind is the high six bits of the byte at +0x38; 5 is the one that goes to a news item.
        if (contract && (c.i8(contract + 0x38) >> 2) == 5) {
            const auto item = frame.at(0x38);
            invoke(c, 0x10411e60, 0x103e8c80, {item});
            c.put32(frame.at(0x40), c.u32(club));
            c.put32(frame.at(0x44), c.u32(person));
            c.put8(frame.at(0x5c), 0);
            invoke_native(c, 0x1040a050, 0x103e8cac, {manager, item, club, 0x13});
            invoke(c, 0x10411f10, 0x103e8cb8, {item, 0xffffffff});
        } else {
            invoke_native(c, 0x10410500, 0x103e8ccc, {manager, person, link});
        }
        return false;
    }
    if (!std::int8_t(invoke(c, 0x10083180, 0x103e8ce4, {c.u32(contracts_cell), link, c.u8(link + 0x78)}))) return false;
    const auto finance = invoke(c, 0x10103510, 0x103e8d04, {c.u32(c.toc(-0x7ab8)), club});
    invoke(c, 0x100f7130, 0x103e8d0c, {finance, c.u32(link + 8)});
    return true;
}

namespace {
// The draw 0x103e8500 makes in line: a remainder of the sample over twice the answers so far (0..510, so
// the original's branch for a range past 0xffff and its calls of 0x103e3860 (0x103e86a0.., 0x103e8884..,
// 0x103e8a54..) never run: that range is at most 2 * 255).
std::int32_t answer_draw(const Samples& samples, std::uint32_t range, std::uint32_t refill_back, auto&& hold) {
    // Only 0x103e882c can see range 0 (the answer count wrapped past 255); the other two sites test the count > 1 first (0x103e8640, 0x103e89f4).
    if (range == 0) return 0;
    return remainder_of(samples.next(refill_back, hold), std::int32_t(range));
}
}

void answer_offer(Cpu& c, std::uint32_t manager, std::uint32_t offer_record, std::uint32_t club_slot, std::uint32_t reason) {
    Frame frame(c, 0x103e8500, 0x80, {.first_gpr = 22});
    const Samples samples(c);
    const auto found_cell = c.toc(toc::found_contract), clubs_cell = c.toc(toc::clubs);
    keep(c, 22, samples.base_cell);
    keep(c, 23, samples.first);
    keep(c, 24, samples.last_cell);
    keep(c, 25, samples.cursor_cell);
    keep(c, 26, c.toc(cell::floats));
    keep(c, 27, found_cell);
    keep(c, 28, clubs_cell);
    keep(c, 30, manager);
    keep(c, 31, reason);
    // The offer is also kept in the caller's frame (sp+0x1c), and handed on by that address.
    const auto slot = frame.at(0x9c);
    c.put32(slot, offer_record);
    const auto offer_now = [&] { return c.u32(slot); };
    const auto stage = c.i8(offer_now() + offer::stage);
    if (stage == 0x11 || stage == 0x12) return;
    c.put8(offer_now() + offer::answers, std::uint8_t(c.u8(offer_now() + offer::answers) + 1));
    const bool reason_8 = std::int16_t(reason) == 8;
    const auto answer_news = [&] { invoke_native(c, 0x10409200, 0x103e8b98, {manager, offer_now(), reason}); };
    const auto refuse = [&](std::uint32_t back) { invoke(c, 0x103eb4a0, back, {manager, offer_now(), 7, 0}); };
    const auto exempt_club = [&](std::uint32_t club, std::uint32_t back) {
        return std::int8_t(invoke(c, 0x1014c2e0, back, {club, 0, 1, 0, 0})) != 0;
    };
    // r22 holds the draw once made (the sample base cell before), r27 the range once set.
    std::uint32_t drawn = samples.base_cell, draw_range = found_cell;
    const auto set_draw = [&](std::int32_t value) { drawn = std::uint32_t(value); keep(c, 22, drawn); };
    if (c.u32(club_slot) == c.u32(offer_now() + offer::club)) {
        c.put8(offer_now() + offer::stage, 0x12);
        if (reason_8) {
            refuse(0x103e859c);
            return;
        }
        std::uint32_t special = 0;
        keep(c, 29, 0);
        const auto contract = invoke(c, 0x10082f20, 0x103e85c8, {c.u32(c.toc(toc::contracts)), person_of(c, offer_now())});
        c.put32(found_cell, contract);
        if (contract && (c.i8(contract + 0x38) >> 2) != 5) special = 1;
        keep(c, 29, special);
        const auto contract_club = special ? c.u32(clubs_cell) + c.u32(c.u32(found_cell) + 4) * club_size : 0;
        bool counter = false;
        if (!contract_club || !exempt_club(contract_club, 0x103e8630)) {
            const auto answers = c.u8(offer_now() + offer::answers);
            if (answers > 1) {
                draw_range = answers * 2;
                keep(c, 27, draw_range);
                set_draw(answer_draw(samples, draw_range, 0x103e8678, [] {}));
                counter = drawn != 0;
            }
        }
        if (counter) {
            invoke_native(c, 0x103e4130, 0x103e87bc, {manager, slot, 0, 6});
            return;
        }
        refuse(0x103e87d4);
        answer_news();
        return;
    }
    c.put8(offer_now() + offer::stage, 0x11);
    invoke_native(c, 0x103e5b70, 0x103e87e8, {manager, offer_now()});
    const auto offer_club = [&] { return c.u32(clubs_cell) + c.u32(offer_now() + offer::club) * club_size; };
    if (reason_8) {
        if (!exempt_club(offer_club(), 0x103e881c)) {
            const auto answers = c.u8(offer_now() + offer::answers);
            draw_range = answers * 2;
            keep(c, 27, draw_range);
            set_draw(answer_draw(samples, draw_range, 0x103e885c, [] {}));
            if (drawn != 0) {
                invoke_native(c, 0x103e44b0, 0x103e89a0, {manager, slot, 0, 6});
                return;
            }
        }
        refuse(0x103e89b8);
        return;
    }
    if (!exempt_club(offer_club(), 0x103e89e4)) {
        const auto answers = c.u8(offer_now() + offer::answers);
        if (answers > 1) {
            draw_range = answers * 2;
            keep(c, 27, draw_range);
            set_draw(answer_draw(samples, draw_range, 0x103e8a2c, [] {}));
            if (drawn != 0) {
                invoke_native(c, 0x103e44b0, 0x103e8b70, {manager, slot, 0, 6});
                return;
            }
        }
    }
    refuse(0x103e8b88);
    answer_news();
}
}

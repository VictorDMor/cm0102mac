#include "transfer_manager_native_flow.hpp"
#include "transfer_manager_native_flow_common.hpp"
#include "debug_force_purchase.hpp"
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using namespace flow;

namespace {
constexpr std::uint32_t date_at = 0x98, date_flags_at = 0xa4, rating_at = 0x38, kinds_at = 0x3c, candidates_at = 0x4c;
constexpr std::uint32_t shortlist_at = 0x7c, source_at = 0x7e, spare_at = 0x40, name_at = 0x5c, extension_at = 0x44;
constexpr std::uint32_t stage_closed = 0x10, stage_sent = 2, state_first_round = 1, kind_candidate = 0x3c, kind_terms = 0x38;
constexpr std::uint32_t pool_scale_a = 0xa0, pool_scale_b = 0xa8, pool_scale_c = 0xb0, pool_ratio = 0x18;

// The first source flag of a person's category word (0x10117490's answer), as the original's chain of tests
// picks it (1, 2, 4, 0x10, 8, 0x20, else 0x40).
std::uint16_t category_source(std::uint32_t category) {
    if (category & 1) return 1;
    if (category & 2) return 2;
    if (category & 4) return 4;
    if (category & 0x10) return 0x10;
    if (category & 8) return 8;
    if (category & 0x20) return 0x20;
    return 0x40;
}
}

void review_offer_slot(Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t flag_word) {
    Frame frame(c, 0x103ee1b0, 0x110, {.first_gpr = 13});
    guest::Fpu fpu(c);
    const auto pool = c.toc(-0x4ba0);
    const auto bidders_cell = c.toc(-0x7a54), found_cell = c.toc(toc::found_contract);
    const auto flag = [&] { return std::int8_t(flag_word) != 0; };
    keep(c, 25, bidders_cell);
    keep(c, 15, manager);
    keep(c, 14, c.toc(toc::manager));
    keep(c, 16, slot);
    keep(c, 29, pool);
    keep(c, 17, flag_word);
    keep(c, 30, c.toc(toc::people));
    keep(c, 26, 0xffffffff);
    keep(c, 23, c.toc(toc::contracts));
    keep(c, 24, 0);
    keep(c, 18, found_cell);
    keep(c, 13, c.toc(toc::clubs));
    const auto contracts = [&] { return c.u32(c.toc(toc::contracts)); };
    const auto shortlist = [&] { return c.u32(manager + manager_shortlist); };
    invoke(c, 0x10715880, 0x103ee1f8, {frame.at(date_at)});
    keep(c, 19, 0);
    // The date's flag byte is read as the call left it (the call never writes it): bit 7 is cleared and bits
    // 3-6 set to 2, the low three bits kept.
    c.put8(frame.at(date_flags_at), (c.u8(frame.at(date_flags_at)) & 0x07) | 0x10);
    c.put32(frame.at(0x88), 10000);
    const auto offer = c.u32(slot);
    keep(c, 22, offer);
    const auto person = person_of(c, c.u32(offer + offer_at::person));
    keep(c, 21, person);
    const auto club = club_of(c, c.u32(offer + offer_at::club));
    keep(c, 20, club);
    const auto contract = find_contract(c, person, 0x103ee250);
    keep(c, 19, contract && (std::int8_t(c.u8(contract + contract_status)) >> 2) != 5);
    const auto partner = contract_club(c, contract);
    keep(c, 28, partner);
    if (!c.u32(person + 0x14)) {
        c.put8(offer + offer_at::stage, stage_closed);
        invoke(c, 0x103eb4a0, 0x103ee2c4, {manager, offer, 1, 0});
        return;
    }
    keep(c, 19, 0);
    c.put16(frame.at(rating_at), 0);
    if (c.u8(offer + offer_at::state) == state_first_round) {
        if (flag()) return;
        // A first-round offer closes: the shortlist entries of its person and its listed people are updated,
        // the contract manager is told, and the offer is settled.
        const auto current = c.u32(slot);
        keep(c, 14, current);
        c.put8(current + offer_at::stage, stage_sent);
        const auto seller = person_of(c, c.u32(current + offer_at::person));
        keep(c, 16, seller);
        const auto seller_contract = invoke(c, 0x10082f20, 0x103ee310, {contracts(), seller});
        c.put32(found_cell, seller_contract);
        const auto seller_club = seller_contract ? club_of(c, c.u32(seller_contract + 4)) : 0;
        keep(c, 17, seller_club);
        invoke(c, 0x1036f100, 0x103ee350, {shortlist(), club_of(c, c.u32(current + offer_at::club)), seller});
        if (const auto list = c.u32(current + offer_at::listed)) {
            c.put32(list + 0xc, c.u32(list + 4));
            while (const auto entry = invoke(c, 0x10738e00, 0x103ee38c, {c.u32(current + offer_at::listed)}))
                invoke(c, 0x1036f100, 0x103ee384, {shortlist(), seller_club, person_of(c, c.u32(entry))});
        }
        invoke_native(c, 0x103ffe80, 0x103ee3a4, {manager, current, 0});
        invoke(c, 0x103eb4a0, 0x103ee3b8, {manager, current, 10, 0});
        invoke(c, 0x103f6190, 0x103ee3c4, {manager, current});
        invoke(c, 0x1007f840, 0x103ee3ec,
               {contracts(), person_of(c, c.u32(current + offer_at::person)), club_of(c, c.u32(current + offer_at::club))});
        keep(c, 17, 0);
        c.put8(current + offer_at::flag10, 0);
        if (const auto list = c.u32(current + offer_at::listed)) {
            c.put32(list + 0xc, c.u32(list + 4));
            for (;;) {
                const auto entry = invoke(c, 0x10738e00, 0x103ee434, {c.u32(current + offer_at::listed)});
                keep(c, 16, entry);
                if (!entry) break;
                invoke(c, 0x1007f930, 0x103ee428,
                       {contracts(), person_of(c, c.u32(entry)), c.u32(current + offer_at::own_slot)});
                c.put8(entry + 4, 0);
            }
        }
        const auto record = invoke(c, 0x1036d240, 0x103ee484,
                                   {shortlist(), club_of(c, c.u32(current + offer_at::club)),
                                    person_of(c, c.u32(current + offer_at::person)), not_eleven(c.u8(current + offer_at::kind)), 0});
        if (record)
            invoke(c, 0x10365590, 0x103ee4bc,
                   {shortlist(), person_of(c, c.u32(current + offer_at::person)), club_of(c, c.u32(current + offer_at::club)), 1, 0});
        return;
    }
    // A later round.
    // Match the generated 0x103ee4c0 override, including its jump past the
    // flag-only review guard. Only the specifically marked bid is advanced.
    if (forced_purchase_for_seller(c.s, offer, partner)) {
        invoke_native(c, 0x103e4740, 0x103ee4f4, {manager, slot, 0});
        return;
    }
    if (std::int8_t(invoke_native(c, 0x103f4d10, 0x103ee4d4, {manager, c.u32(slot), 0, 0}))) {
        if (flag()) return;
        invoke_native(c, 0x103e4740, 0x103ee4f4, {manager, slot, 0});
        return;
    }
    // A partner club that is in the player's hands and a person it will not release go to 0x103e4130.
    if (partner && std::int8_t(invoke(c, 0x1014c2e0, 0x103ee518, {partner, 1, 1, 0, 0}))) {
        const auto cell = c.u32(bidders_cell);
        const auto source = [&](std::uint32_t back) { return invoke(c, 0x100c96e0, back, {partner}); };
        if (std::int8_t(invoke(c, 0x10147e70, 0x103ee534, {cell, source(0x103ee528)}))) {
            const auto first = std::int8_t(invoke(c, 0x10149df0, 0x103ee554, {cell, 0, source(0x103ee544)}));
            if (first && c.u32(offer + offer_at::listed)) {
                invoke_native(c, 0x103e4130, 0x103ee57c, {manager, slot, 0, 0});
                return;
            }
            if (std::int8_t(invoke(c, 0x10149df0, 0x103ee598, {cell, 0, source(0x103ee588)})) == 1) {
                invoke_native(c, 0x103e4130, 0x103ee5b8, {manager, slot, 0, 0});
                return;
            }
        }
    }
    auto bid = std::int32_t(invoke(c, 0x10085860, 0x103ee5dc, {contracts(), partner, club, person, frame.at(rating_at), 1, 1}));
    keep(c, 18, word(bid));
    // The other offers for the same person: the best bid among those that are still open wins.
    if (const auto bidders = c.u32(offer + offer_at::bidders); bidders && !flag()) {
        c.put32(bidders + 0xc, c.u32(bidders + 4));
        while (const auto entry = invoke(c, 0x10738e00, 0x103ee668, {c.u32(offer + offer_at::bidders)})) {
            const auto index = c.u32(entry);
            const auto other = c.u32(c.u32(manager) + index * 4);
            keep(c, 13, other);
            if (std::int32_t(index) == c.i32(offer + offer_at::own_slot)) continue;
            if (std::int8_t(invoke(c, 0x103fb390, 0x103ee628, {manager, other}))) continue;
            const auto rival = std::int32_t(invoke(c, 0x10416840, 0x103ee64c, {other, 0, 0, 1, 0, 1}));
            if (rival > bid) {
                c.put16(frame.at(rating_at), 0);
                bid = rival;
                keep(c, 18, word(bid));
            }
        }
    }
    const auto asking = invoke(c, 0x10416710, 0x103ee688, {offer, 1, 0, 0, 1});
    keep(c, 31, asking);
    const auto floor_value = std::int32_t(invoke(c, 0x10416840, 0x103ee6a8, {offer, 0, 0, 1, 1, 1}));
    auto ceiling = std::int32_t(invoke(c, 0x10416840, 0x103ee6c8, {offer, 0, 1, 1, 1, 1}));
    keep(c, 25, word(ceiling));
    bool unlisted_left = false;
    if (const auto listed = c.u32(offer + offer_at::listed)) {
        const auto headroom = std::int32_t(word(bid) - word(ceiling));
        keep(c, 27, word(headroom));
        c.put32(listed + 0xc, c.u32(listed + 4));
        while (const auto entry = invoke(c, 0x10738e00, 0x103ee7e8, {c.u32(offer + offer_at::listed)})) {
            const auto listed_person = person_of(c, c.u32(entry));
            keep(c, 23, listed_person);
            const auto category = invoke(c, 0x10117490, 0x103ee700, {c.u32(listed_person + 0x14)});
            c.put16(frame.at(source_at), category_source(word(s16(category))));
            const auto value = std::int32_t(invoke(c, 0x10361490, 0x103ee7a8,
                                                   {c.u32(c.u32(c.toc(toc::manager)) + manager_shortlist), partner, listed_person,
                                                    frame.at(shortlist_at), frame.at(spare_at), 0xffffffff, 0}));
            const auto limited = value > headroom ? headroom : value;
            if (!flag() && !std::int8_t(invoke(c, 0x10364d00, 0x103ee7d4, {shortlist(), listed_person, partner, word(limited), 1})))
                unlisted_left = true, keep(c, 24, 1);
        }
    }
    if (ceiling > bid) {
        bid = ceiling;
        keep(c, 18, word(bid));
    }
    // The candidate offers: up to three kinds are built, each a pointer in the frame with its kind byte.
    std::int32_t count = 0;
    if (std::int8_t(invoke_native(c, 0x103eec00, 0x103ee818, {manager, club, person, frame.at(candidates_at), frame.at(kinds_at), word(bid)})))
        count = 1, keep(c, 19, 1);
    if (std::int8_t(invoke_native(c, 0x103eed00, 0x103ee854,
                                  {manager, club, person, offer, frame.at(candidates_at) + count * 4u, frame.at(kinds_at) + count, word(bid)}))) {
        count = std::int8_t(count + 1);
        keep(c, 19, word(count));
    }
    if (std::int8_t(invoke_native(c, 0x103ef580, 0x103ee890,
                                  {manager, club, person, frame.at(candidates_at) + count * 4u, frame.at(kinds_at) + count, word(bid)}))) {
        count = std::int8_t(count + 1);
        keep(c, 19, word(count));
    }
    // The best candidate kind (the first of the highest) is kept; a candidate worth more than the ratio of the
    // best proposal goes straight to the answer step.
    std::uint32_t best = 0;
    std::uint32_t best_kind = 0xffffffff;
    std::uint32_t found_better = 0;
    std::int32_t position = 0;
    keep(c, 27, 0);
    keep(c, 30, frame.at(kinds_at));
    keep(c, 18, word(count));
    keep(c, 25, 0);
    keep(c, 20, word(floor_value) ^ 0x80000000u);
    keep(c, 14, frame.at(candidates_at));
    keep(c, 13, 0);
    keep(c, 28, 0);
    keep(c, 23, 0x43300000);
    const double magic = c.f64(pool + 8);
    const auto bits = [&](std::uint32_t value) {
        return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (value ^ 0x80000000u));
    };
    for (; std::int8_t(position) < count; ++position) {
        const auto kind = c.u8(frame.at(kinds_at) + word(std::int8_t(position)));
        if (s8(kind) > s8(best_kind)) {
            best_kind = kind;
            keep(c, 26, kind);
            best = c.u32(frame.at(candidates_at) + word(position) * 4);
            keep(c, 27, best);
        }
        const auto worth = std::int32_t(invoke(c, 0x10416840, 0x103ee90c,
                                               {c.u32(frame.at(candidates_at) + word(position) * 4), 0, 0, 1, 1, 1}));
        const double scaled = fpu.fmul(c.f64(pool + pool_scale_a), fpu.fsubs(bits(word(worth)), magic));
        const double reference = fpu.fsub(bits(word(floor_value)), magic);
        fpu.fcmp(0, scaled, reference);
        if (scaled < reference && !unlisted_left) {
            if (!flag()) invoke_native(c, 0x103e4740, 0x103ee964, {manager, slot, 0});
            found_better = 1;
            keep(c, 13, 1);
            break;
        }
        keep(c, 25, word(position + 1) * 4);
        keep(c, 14, frame.at(candidates_at) + word(position + 1) * 4);
        keep(c, 28, word(position + 1));
    }
    const auto cleanup = [&] {
        keep(c, 14, word(s8(word(count))));
        keep(c, 13, frame.at(candidates_at));
        for (std::int32_t i = 0; i < count; ++i) {
            keep(c, 15, word(i));
            invoke(c, 0x10411f10, 0x103eebd0, {c.u32(frame.at(candidates_at) + word(i) * 4), 1});
            keep(c, 13, frame.at(candidates_at) + word(i + 1) * 4);
        }
    };
    // 0x103ee988: 0x103eec00 answers 1 at both of its exits, so at least one candidate was built.
    if (std::int8_t(count) == 0) throw std::logic_error("0x103ee988: no candidate built");
    if (!best) {
        report_failure(c, frame.at(name_at), frame.at(extension_at), 0x1eed, {0x103ee9cc, 0x103ee9d0, 0x103ee9f0, 0x103eea04});
        c.put8(c.toc(-0x7bac), 0);
        invoke_native(c, 0x103e4130, 0x103eea24, {manager, slot, 1, word(s16(c.u16(frame.at(rating_at))))});
        return;
    }
    if (!std::int8_t(found_better)) {
        const auto best_worth = std::int32_t(invoke(c, 0x10416840, 0x103eea4c, {best, 0, 0, 0, 0, 1}));
        const double scaled = fpu.fmul(c.f64(pool + pool_ratio), fpu.fsubs(bits(word(best_worth)), magic));
        const double base = fpu.fsub(bits(word(floor_value)), magic);
        fpu.fcmp(0, base, scaled);
        const auto high_nibble = c.u8(offer + kind_candidate) >> 4;
        if (base > scaled && high_nibble != 1 && high_nibble != 2 && !flag()) {
            c.put8(best + kind_terms, (c.u8(best + kind_terms) & 0x0f) | (c.u8(offer + kind_terms) & 0xf0));
            invoke_native(c, 0x103e2db0, 0x103eeadc, {manager, offer, best, 0});
            return cleanup();
        }
        const auto value = std::int32_t(c.u32(person + 8));
        const auto ask = std::int32_t(asking);
        // A proposal far under what the person is worth closes the offer (the first test, 2 million short, or
        // the second, 100000 short and not in the first round).
        bool close_offer = false;
        const double ratio_a = fpu.fmul(c.f64(pool + pool_scale_b), fpu.fsubs(bits(word(value)), magic));
        const double ask_a = fpu.fsub(bits(word(ask)), magic);
        fpu.fcmp(0, ask_a, ratio_a);
        if (ask_a < ratio_a && std::int32_t(word(value) - word(ask)) > 2000000) close_offer = true;
        if (!close_offer) {
            const double ratio_b = fpu.fmul(c.f64(pool + pool_scale_c), fpu.fsubs(bits(word(value)), magic));
            const double ask_b = fpu.fsub(bits(word(ask)), magic);
            fpu.fcmp(0, ask_b, ratio_b);
            close_offer = ask_b < ratio_b && std::int32_t(word(value) - word(ask)) > 100000 && !flag();
        }
        if (close_offer) c.put16(frame.at(rating_at), 10);
        invoke_native(c, 0x103e4130, 0x103eebb0, {manager, slot, 1, word(s16(c.u16(frame.at(rating_at))))});
    }
    cleanup();
}
}

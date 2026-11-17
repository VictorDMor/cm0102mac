#include "transfer_manager_native_flow.hpp"
#include "transfer_manager_native_flow_common.hpp"
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using namespace flow;

namespace {
constexpr std::uint32_t object_at = 0x3c, object_list = 0x40, object_kind_word = 0x78, rating_at = 0x38;
constexpr std::uint32_t contract_flags = 0x50, flag_free_agent = 0x800000;
// The working copy of an offer the original keeps in its frame (0x54 bytes built by 0x10411e60): the fields
// 0x10416840 and the others read, copied store by store (size by size) as the original does, the first word
// cleared and the +0x30 word set to -1.
void copy_offer(Cpu& c, const Frame& frame, std::uint32_t from) {
    const auto at = [&](std::uint32_t field) { return frame.at(std::int32_t(object_at + field)); };
    const auto words = {0x00u, 0x04u, 0x08u, 0x0cu, 0x20u, 0x28u, 0x2cu};
    for (const auto field : words) c.put32(at(field), c.u32(from + field));
    c.put8(at(0x10), c.u8(from + 0x10));
    c.put8(at(0x11), c.u8(from + 0x11));
    c.put16(at(0x16), c.u16(from + 0x16));
    c.put16(at(0x14), c.u16(from + 0x14));
    c.put8(at(0x18), c.u8(from + 0x18));
    c.put8(at(0x1c), c.u8(from + 0x1c));
    c.put8(at(0x24), c.u8(from + 0x24));
    c.put32(at(0x30), c.u32(from + 0x30));
    for (const auto field : {0x34u, 0x35u, 0x36u, 0x37u, 0x38u}) c.put8(at(field), c.u8(from + field));
    c.put32(at(0x3c), c.u32(from + 0x3c));
    c.put32(at(0x40), c.u32(from + 0x40));
    c.put8(at(0x44), c.u8(from + 0x44));
    c.put32(at(0x48), c.u32(from + 0x48));
    for (const auto field : {0x4cu, 0x50u, 0x51u}) c.put8(at(field), c.u8(from + field));
}
}

void evaluate_listed_offer(Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t final_flag) {
    Frame frame(c, 0x103e4d70, 0x100, {.first_gpr = 13});
    guest::Fpu fpu(c);
    const auto object = frame.at(object_at);
    const auto pool_float = c.toc(-0x4ba4), pool = c.toc(-0x4ba0), contracts_cell = c.toc(toc::contracts);
    const auto draw_seed = c.toc(-0x7b90), draw_cursor = c.toc(-0x7b84);
    keep(c, 27, pool_float);
    keep(c, 30, manager);
    keep(c, 13, draw_seed);
    keep(c, 31, slot);
    keep(c, 14, draw_cursor);
    keep(c, 15, final_flag);
    keep(c, 28, pool);
    keep(c, 18, c.toc(toc::people));
    keep(c, 29, contracts_cell);
    keep(c, 23, c.toc(toc::found_contract));
    keep(c, 20, c.toc(toc::clubs));
    const auto shortlist = [&] { return c.u32(manager + manager_shortlist); };
    const auto contracts = [&] { return c.u32(contracts_cell); };
    const auto finish = [&] { invoke(c, 0x10411f10, 0x103e57f8, {object, 0xffffffff}); };
    invoke(c, 0x10411e60, 0x103e4db4, {object});
    std::uint32_t paid_over = 0;  // what the people taken off the list cost together
    keep(c, 26, paid_over);
    std::uint32_t source;  // the manager slot whose offer is copied
    if (std::int8_t(final_flag)) {
        source = slot;
    } else {
        const auto first = c.u32(slot);
        if (c.i32(first + 0x30) == -1) {
            c.put8(first + offer_at::stage, 0x10);
            invoke(c, 0x103eb4a0, 0x103e4df4, {manager, c.u32(slot), 1, 0});
            return finish();
        }
        source = invoke(c, 0x103eb470, 0x103e4e0c, {manager, c.u32(first + 0x30)});
    }
    keep(c, 16, source);
    const auto offer = c.u32(source);
    const auto had_list = c.u32(offer + offer_at::listed) != 0;
    copy_offer(c, frame, offer);
    c.put32(object, 0);
    c.put32(frame.at(0x6c), 0xffffffff);
    if (had_list) {
        const auto list = invoke(c, 0x10773520, 0x103e4f0c, {0x10});
        keep(c, 17, list);
        if (list) invoke(c, 0x10738c00, 0x103e4f18, {list});
        c.put32(frame.at(object_list), list);
        invoke(c, 0x10739010, 0x103e4f2c, {list, c.u32(c.u32(source) + offer_at::listed)});
    }
    const auto current = c.u32(source);
    const auto person = person_of(c, c.u32(current + offer_at::person));
    const auto club = club_of(c, c.u32(current + offer_at::club));
    keep(c, 25, person);
    keep(c, 24, club);
    const auto record = invoke(c, 0x1036d240, 0x103e4f84,
                               {shortlist(), club, person, not_eleven(c.u8(c.u32(slot) + offer_at::kind)), 0});
    keep(c, 21, record);
    if (!record) {
        invoke_native(c, 0x103e44b0, 0x103e4fa0, {manager, slot, 0, 2});
        return finish();
    }
    const auto listed_shortlist = c.u32(c.u32(c.toc(toc::manager)) + manager_shortlist);
    const auto limit = std::int32_t(invoke(c, 0x10361490, 0x103e4fd8,
                                           {listed_shortlist, club, person, record, frame.at(rating_at), 0xffffffff, 1}));
    keep(c, 19, word(limit));
    auto offered = std::int32_t(invoke(c, 0x10416840, 0x103e4ff8, {object, 0, 1, 1, 1, 1}));
    keep(c, 22, word(offered));
    // The listed people: each one the club would pay for goes on the offer's price, the rest drop off the list.
    if (const auto list = c.u32(frame.at(object_list))) {
        c.put32(list + 0xc, c.u32(list + 4));
        for (;;) {
            const auto entry = invoke(c, 0x10738e00, 0x103e509c, {c.u32(frame.at(object_list))});
            keep(c, 16, entry);
            if (!entry) break;
            const auto listed_person = person_of(c, c.u32(entry));
            keep(c, 17, listed_person);
            const auto contract = find_contract(c, listed_person, 0x103e5030);
            const auto contract_club_at = contract ? club_of(c, c.u32(contract + 4)) : 0;
            const auto price = std::int32_t(invoke(c, 0x10085860, 0x103e5070,
                                                   {contracts(), club, contract_club_at, listed_person, 0, 1, 1}));
            if (std::int32_t(word(price) + word(offered)) > limit) {
                paid_over += word(price);
                keep(c, 26, paid_over);
                invoke(c, 0x10738e60, 0x103e508c, {c.u32(frame.at(object_list)), entry});
            } else {
                offered = std::int32_t(word(offered) + word(price));
                keep(c, 22, word(offered));
            }
        }
        const auto rest = c.u32(frame.at(object_list));
        if (s16(c.u16(rest)) == 0) {
            invoke(c, 0x10738c20, 0x103e50bc, {rest, 1});
            c.put32(frame.at(object_list), 0);
        }
    }
    c.put8(frame.at(object_kind_word), (c.u8(frame.at(object_kind_word)) & 0x0f) | 0x20);
    const auto shortlist_status = [&](std::uint32_t back) {
        invoke(c, 0x10365590, back, {shortlist(), person, club, 1, not_eleven(c.u8(record))});
    };
    if (offered <= limit) {
        // The club can afford everything: the shortfall is added to the amount and the offer is put to the
        // finance manager, or the whole deal is put forward.
        auto extra = std::int32_t(word(limit) - word(offered));
        if (std::int32_t(paid_over) < extra) extra = std::int32_t(paid_over);
        if (extra < 0) extra = 0;
        c.put32(frame.at(0x5c), c.u32(frame.at(0x5c)) + word(extra));
        const auto total = invoke(c, 0x10416710, 0x103e5740, {object, 0, 1, 0, 1});
        keep(c, 13, total);
        const auto account = invoke(c, 0x10103510, 0x103e5754, {c.u32(c.toc(-0x7ab8)), club});
        invoke(c, 0x100f5770, 0x103e576c, {account, person, 0, 0, 1, 0});
        const auto balance = (std::int64_t(std::int32_t(c.r(3))) << 32) | c.r(4);
        if (balance < std::int64_t(std::int32_t(total))) {
            shortlist_status(0x103e57bc);
            invoke_native(c, 0x103e44b0, 0x103e57d0, {manager, slot, 0, 2});
        } else if (!std::int8_t(final_flag)) {
            invoke_native(c, 0x103e25d0, 0x103e57ec, {manager, object, 0});
        }
        return finish();
    }
    // Too many people to keep: sign the player off the shortlist and close the offer.
    shortlist_status(0x103e510c);
    invoke_native(c, 0x103e44b0, 0x103e5120, {manager, slot, 0, 2});
    keep(c, 15, 0);
    std::uint32_t bound = 0;
    const auto partner_contract = find_contract(c, person, 0x103e5130);
    bound = partner_contract && (std::int8_t(c.u8(partner_contract + contract_status)) >> 2) != 5;
    keep(c, 15, bound);
    const auto partner = contract_club(c, partner_contract);
    keep(c, 16, partner);
    std::uint32_t partner_contract_at = partner ? invoke(c, 0x10082f90, 0x103e5194, {contracts(), person, partner}) : 0;
    keep(c, 15, partner_contract_at);
    keep(c, 17, partner_contract_at);
    // Not worth pursuing unless the partner club has a contract and the person is still signed at one.
    // !partner follows from !partner_contract_at (0x103e5178 leaves r15 = 0 without a partner); !club needs a club
    // address of 0, which no table index gives (0x103e4f70 adds a nonzero table base).
    if (!partner_contract_at || !partner || !club) return finish();
    const auto days = std::int32_t(invoke(c, 0x10718130, 0x103e51cc, {c.toc(-0x7b80), c.u32(person + 0x4c), c.u32(person + 0x50)}));
    if (days <= s8(c.u8(person + 0x65)) * 5 + s8(c.u8(person + 0x67)) + 0xd2) return finish();
    bool proceed = s16(c.u16(partner + 0x1ec)) + 0x4e2 < s16(c.u16(club + 0x1ec));
    if (!proceed) proceed = std::int8_t(invoke(c, 0x100d3300, 0x103e5210, {person, 0})) != 0;
    if (!proceed) proceed = (invoke(c, 0x1037fa70, 0x103e5220, {partner_contract_at}) & 8) != 0;
    if (!proceed) proceed = (c.u32(partner_contract_at + contract_flags) & 0x14) != 0;
    if (!proceed) return finish();
    const bool partner_in_player_state = std::int8_t(invoke(c, 0x1014c2e0, 0x103e524c, {partner, 0, 1, 0, 0})) != 0;
    auto price = std::int32_t(partner_in_player_state
                                  ? invoke(c, 0x10084be0, 0x103e5270, {contracts(), person, partner, 0, 0xffffffff, 1})
                                  : invoke(c, 0x10084be0, 0x103e5294, {contracts(), person, partner, 0, 0xffffffff, 0}));
    keep(c, 15, word(price));
    // How willing the partner club is to part: points for the contract's clauses, the person's age and ratings.
    const auto clause = c.u32(partner_contract_at + contract_flags);
    std::int32_t score = 0;
    if (clause) score = 5;
    if (clause & 0x101) score = s8(word(score + 5));
    if (s8(c.u8(person + 0x63)) == 0x14) score += 2;
    else if (s8(c.u8(person + 0x63)) > 0xf) score += 1;
    const auto experience = s8(c.u8(person + 0x65));
    if (experience == 1) score += 3;
    else if (experience < 5) score += 1;
    else if (experience == 0x14) score -= 3;
    else if (experience > 0xf) score -= 1;
    keep(c, 18, word(score));
    if (std::int8_t(invoke(c, 0x100da0b0, 0x103e5330, {person, partner}))) {
        score -= 2;
    } else if (std::int8_t(invoke(c, 0x100da050, 0x103e534c, {person, partner}))) {
        score += 2;
    }
    const auto form = s8(c.u16(person + 0x5c));
    if (form < 0x14) score -= 4;
    else if (form < 0x17) score -= 2;
    keep(c, 18, word(score));
    const auto grade = s8(word(score));
    const std::uint32_t grade_cell = grade > 10 ? 0 : grade > 8 ? 1 : grade > 5 ? 2 : grade > 3 ? 3 : grade > 1 ? 4
                                   : std::int8_t(score) == 0 ? 5 : grade < -2 ? 6 : std::int8_t(score) < 0 ? 7 : 8;
    const double factor = c.f32(pool_float + grade_cell * 4);
    const auto bits = [&](std::uint32_t value) {
        return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (value ^ 0x80000000u));
    };
    const double magic = c.f64(pool + 8);
    std::uint32_t rounded;
    if (price < 500000) {
        const double scaled = fpu.fmul(c.f64(pool + 0x20), fpu.fsubs(bits(word(price)), magic));
        rounded = std::uint32_t(fpu.fctiwz(0, scaled));
    } else if (price < 2500000) {
        const double above = fpu.fsubs(fpu.fsubs(bits(word(price)), magic), c.f32(pool_float + 0x24));
        const double result = fpu.multiply_add(0x103e5484, c.f64(pool + 0x30), above, c.f64(pool + 0x28));
        rounded = std::uint32_t(fpu.fctiwz(0, result));
    } else {
        const double above = fpu.fsubs(fpu.fsubs(bits(word(price)), magic), c.f32(pool_float + 0x28));
        const double result = fpu.multiply_add(0x103e54c4, c.f64(pool + 0x40), above, c.f64(pool + 0x38));
        rounded = std::uint32_t(fpu.fctiwz(0, result));
    }
    const double scaled_price = fpu.fmul(fpu.fsub(bits(rounded), magic), factor);
    const auto expected = std::int32_t(std::uint32_t(fpu.fctiwz(0, scaled_price)));
    bool refuse = offered > 0x10b0760;
    if (!refuse) {
        const double need = fpu.fmul(c.f64(pool), fpu.fsub(bits(word(expected)), magic));
        const double have = fpu.fsub(bits(word(offered)), magic);
        fpu.fcmp(0, have, need);
        refuse = have > need;
    }
    if (refuse) {
        invoke(c, 0x1037fa80, 0x103e555c, {partner_contract_at, flag_free_agent, 0});
        return finish();
    }
    if (offered <= 250000 || offered <= expected) return finish();
    const auto draw_range = std::int32_t(s8(c.u8(person + 0x69)) + 1);
    keep(c, 15, word(draw_range));
    std::int32_t chosen;
    if (draw_range == 0) {
        chosen = 0;
    } else {
        // |range| is at most 128, so the original's branch for ranges past 0xffff (0x103e55e0-0x103e56b0) is dead.
        if (draw_range > 0xffff || draw_range < -0xffff) throw std::logic_error("0x103e55e0: draw range past 16 bits");
        const HalfwordDraw draw(c);
        draw.advance(0x103e55b8);
        const auto sample = std::int32_t(draw.value());
        const auto remainder = sample - div_s(sample, draw_range) * draw_range;
        chosen = draw_range > 0 ? remainder : -remainder;
    }
    keep(c, 13, word(chosen));
    if (chosen == 0) invoke(c, 0x1037fa80, 0x103e56fc, {partner_contract_at, flag_free_agent, 0});
    return finish();
}
}

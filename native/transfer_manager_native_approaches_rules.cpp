// TRANSFER_MANAGER.CPP native-first block "approaches": the first status of an offer (0x103f3bf0) and the news
// about it (0x1040d5c0).
#include "transfer_manager_native_approaches.hpp"
#include "transfer_manager_native_approaches_detail.hpp"
#include "transfer_manager_native_status.hpp"
#include "guest_fpu.hpp"
#include <bit>
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace approaches;

// ---- 0x103f3bf0 ----
void choose_first_status(Cpu& c, std::uint32_t manager, std::uint32_t offer_at, std::uint32_t first, std::uint32_t second) {
    constexpr std::uint32_t find_contract = 0x10082f20, find_row = 0x10082f90, club_level = 0x103f8380,
                            level_of = 0x105c5030, is_human_kind = 0x100c7a20, shortlist_value = 0x10369080;
    Frame frame(c, 0x103f3bf0, 0xc0, {.first_gpr = 14});
    const auto buffer = c.toc(-0x7ba8), found_cell = c.toc(-0x7b78), clubs_cell = c.toc(-0x7be8), pool2 = c.toc(-0x4b9c),
               contracts_cell = c.toc(-0x7b7c), people_cell = c.toc(-0x7be0), pool = c.toc(-0x4ba0);
    const HalfwordDraw draw(c);
    keep(c, 14, buffer);
    keep(c, 15, found_cell);
    keep(c, 16, clubs_cell);
    keep(c, 17, 0x16d);
    keep(c, 18, pool2);
    keep(c, 19, contracts_cell);
    keep(c, 20, manager);
    keep(c, 21, offer_at);
    keep(c, 22, first);
    keep(c, 23, second);
    keep(c, 24, people_cell);
    keep(c, 26, 0);
    keep(c, 27, draw.seed);
    keep(c, 28, draw.start);
    keep(c, 29, draw.limit);
    keep(c, 30, draw.cursor);
    keep(c, 31, pool);

    const auto offering_club = c.u32(clubs_cell) + c.u32(offer_at + offer::club) * club_size;
    keep(c, 25, offering_club);
    const auto contract = invoke(c, find_contract, 0x103f3c6c,
                                 {c.u32(contracts_cell), c.u32(people_cell) + c.u32(offer_at + offer::person) * person_size});
    c.put32(found_cell, contract);
    const bool counts = counts_for_club(c, contract);
    if (counts) keep(c, 26, 1);
    const auto contract_club = counts ? club_of(c, clubs_cell, c.u32(found_cell)) : 0;
    keep(c, 26, contract_club);
    const auto person = c.u32(people_cell) + c.u32(offer_at + offer::person) * person_size;
    keep(c, 24, person);
    const auto row = invoke(c, find_row, 0x103f3cd8, {c.u32(contracts_cell), person, contract_club});
    keep(c, 19, row);

    // The level of each side: the offering club's, lowered to the contract club's when that is lower and the
    // offer has a list.
    std::uint32_t level = 0x16d;
    if (const auto side = invoke(c, club_level, 0x103f3ce8, {manager, offering_club}); side != 0) {
        level = invoke(c, level_of, 0x103f3cf4, {side});
        keep(c, 17, level);
    }
    if (c.u32(offer_at + offer::list) != 0) {
        const auto other = invoke(c, club_level, 0x103f3d10, {manager, contract_club});
        keep(c, 15, other);
        if (other != 0) {
            const auto other_level = invoke(c, level_of, 0x103f3d1c, {other});
            std::uint32_t chosen;
            if ((level & 0xffff) < (other_level & 0xffff)) chosen = level;
            else chosen = invoke(c, level_of, 0x103f3d3c, {other});
            level = chosen & 0xffff;
            keep(c, 17, level);
        }
    }
    const auto draw_remainder_three = [&](std::uint32_t back) {
        draw.advance(back);
        return signed_of(draw.value()) % 3 + 1;
    };
    const auto kind = level & 0xffff;
    if (kind < 3) {
        c.put8(first, 0);
        c.put8(second, 1);
        return;
    }
    if (kind < 6) {
        c.put8(first, 1);
        c.put8(second, std::uint32_t(draw_remainder_three(0x103f3d90)));
        return;
    }
    if (kind < 0xb) {
        c.put8(first, 2);
        c.put8(second, std::uint32_t(draw_remainder_three(0x103f3e00)));
        return;
    }

    // Level 11 and up: the contract's preference (+0x44) gives a ceiling or a drawn value.
    const std::uint32_t preference = row != 0 ? c.u8(row + 0x44) : 0xffffff9c;
    keep(c, 15, preference);
    // The person's data word is read only once the kind test passed (0x103f3e64), as the original does.
    if (std::int8_t(invoke(c, is_human_kind, 0x103f3e5c, {c.u8(offer_at + 0x11)})) != 0 && c.u32(person + 0x14) == 0)
        report_assertion(c, {0x103f3e84, pool2 + 0x37, pool2 + 0x6ff, pool2 + 0x70c, frame.at(0x40), frame.at(0x38), 0x29c2});

    guest::Fpu fpu(c);
    bool ceiling = false;
    if (contract_club != 0 && std::int8_t(invoke(c, is_human_kind, 0x103f3ed4, {c.u8(offer_at + 0x11)})) != 0) {
        const std::int32_t strength = c.i16(c.u32(person + 0x14) + 6);
        if (strength > 0x82 && strength > std::int32_t(c.i16(contract_club + 0x1ec)) / 60 &&
            std::int8_t(c.u16(person + 0x5c)) < 0x1a &&
            ((c.u8(c.u32(c.toc(-0x7b9c)) + c.u32(person) * 100 + 0xc) >> 4) & 0xf) < 3 &&
            std::int8_t(preference) > 0) {
            invoke(c, shortlist_value, 0x103f3f70, {c.u32(manager + manager::shortlist), person, offering_club, 0, 0, 1, 0});
            fpu.fcmp(0, c.f(1), c.f64(pool + 0xc0));
            ceiling = (c.cr(0) & 4) != 0;
        }
    }
    std::int32_t result;
    if (ceiling) {
        const std::uint32_t half = level & 0xffff;
        c.put32(frame.at(0x74), half);
        c.put32(frame.at(0x70), 0x43300000);
        auto scaled = fpu.fsub(std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | half), c.f64(pool + 0x128));
        scaled = fpu.fmul(c.f64(pool + 0x148), scaled);
        double cap = c.f64(pool + 0x140);
        fpu.fcmp(0, cap, scaled);
        if ((c.cr(0) & 8) == 0) cap = scaled;
        const auto bits = fpu.fctiwz(0, cap);
        c.put64(frame.at(0x68), bits);
        result = signed_of(c.u32(frame.at(0x6c)));
    } else {
        std::int32_t quotient = std::int32_t(std::int8_t(preference)) / 10;
        keep(c, 15, unsigned_of(quotient));
        std::int32_t base = quotient + 0xc;
        keep(c, 14, unsigned_of(base));
        if (base != 0) {
            draw.advance(0x103f4010);
            // Quotients lie in -12..12, so base is 0..24: the original's tests for base beyond 16 bits (the
            // double-precision scaling at 0x103f4038..0x103f4108) and for base <= 0 (the negated remainder at
            // 0x103f412c) cannot be reached.
            if (base > 0xffff || base < -0xffff) throw std::logic_error("0x103f4038: base outside -12..24");
            if (base <= 0) throw std::logic_error("0x103f412c: base <= 0 after the zero test");
            base = signed_of(draw.value()) % base;
        }
        std::int8_t limit = std::int8_t(base);
        keep(c, 14, std::uint32_t(std::int32_t(limit)));
        const auto roll = draw_remainder_three(0x103f415c);
        if (roll <= std::int32_t(limit)) {
            result = limit;
        } else {
            result = draw_remainder_three(0x103f41b8);
        }
    }
    const std::int32_t picked = std::int8_t(result);
    c.put8(first, std::uint32_t(picked / 3));
    c.put8(second, std::uint32_t(picked % 3));
}

// ---- 0x1040d5c0 ----
void post_offer_news(Cpu& c, std::uint32_t manager, std::uint32_t offer_at, std::uint32_t terms) {
    constexpr std::uint32_t news_item = 0xd8, clubs_list = 0x60;
    Frame frame(c, 0x1040d5c0, 0x1f0, {.first_gpr = 22});
    const auto people_cell = c.toc(-0x7be0), news_cell = c.toc(-0x7bf0), contracts_cell = c.toc(toc::contracts),
               found_cell = c.toc(-0x7b78), buffer = c.toc(-0x7ba8), pool = c.toc(-0x4b9c);
    const auto person = c.u32(people_cell) + c.u32(offer_at + offer::person) * person_size;
    keep(c, 23, offer_at);
    keep(c, 24, terms);
    keep(c, 26, news_cell);
    keep(c, 27, contracts_cell);
    keep(c, 28, found_cell);
    keep(c, 29, buffer);
    keep(c, 30, pool);
    keep(c, 25, person);
    // 0x103fb840 lists the clubs to tell (up to 0x1e of them, returned in a frame array) and how many.
    const auto count = invoke(c, 0x103fb840, 0x1040d610, {manager, offer_at, 0x2e, frame.at(clubs_list), 0x1e});
    keep(c, 31, count);
    if (std::int8_t(count) == 0) return;
    NewsItem item(c, frame.at(news_item), 0xfb5, 0x1040d62c);
    item.field(0, c.u32(person), 0x1040d63c);
    item.field(1, c.u32(c.u32(person + 0x28) + 0x20), 0x1040d650);
    item.field(2, c.u32(c.u32(person + 0x2c) + 0x20), 0x1040d664);
    item.field(3, c.u32(c.u32(person + 0x30) + 0x20), 0x1040d678);
    item.field(4, c.u32(offer_at + offer::club), 0x1040d688);
    keep(c, 22, 0);
    const auto contract = invoke(c, 0x10082f20, 0x1040d698, {c.u32(contracts_cell), person});
    c.put32(found_cell, contract);
    const bool counts = counts_for_club(c, contract);
    keep(c, 22, counts ? 1 : 0);
    const auto club = counts ? club_of(c, c.toc(-0x7be8), c.u32(found_cell)) : 0;
    item.field(5, c.u32(club), 0x1040d6f8);
    item.field(6, c.u32(offer_at + offer::slot), 0x1040d708);
    item.field(7, c.u32(offer_at + offer::serial), 0x1040d718);
    item.field(8, c.u32(terms + 0xc), 0x1040d728);
    item.field(9, byte_signed(c.u8(offer_at + offer::state)), 0x1040d73c);
    item.field(0xa, std::uint32_t(std::int32_t(c.i16(terms + 0x30))), 0x1040d74c);
    item.field(0xb, c.u16(terms + 0x32), 0x1040d75c);
    const auto again = invoke(c, 0x10082f20, 0x1040d768, {c.u32(contracts_cell), person});
    if (again == 0) {
        report_assertion(c, {0x1040d784, pool + 0x37, pool + 0x6ff, pool + 0x70c, frame.at(0x40), frame.at(0x38), 0x5e42});
        return;
    }
    item.field(0xc, c.u32(again + 0xc), 0x1040d7d8);
    keep(c, 24, byte_signed(std::uint8_t(count)));
    for (std::int32_t index = 0; std::int8_t(index) < std::int8_t(count); ++index) {
        keep(c, 23, frame.at(clubs_list) + unsigned_of(index) * 4);
        keep(c, 22, unsigned_of(index));
        item.post(c.u32(frame.at(clubs_list) + unsigned_of(index) * 4), 0x1040d7f8);
    }
}
}

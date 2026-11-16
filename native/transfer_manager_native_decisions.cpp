// TRANSFER_MANAGER.CPP, block "decisions": starting, removing and announcing offers.
#include "transfer_manager_native_decisions.hpp"
#include "guest_fpu.hpp"
#include "transfer_manager_native_detail.hpp"
#include <bit>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t find_contract = 0x10082f20;  // (contract manager, person) -> contract or 0
constexpr std::uint32_t list_next = 0x10738e00;      // (list) -> next element or 0
constexpr std::uint32_t list_remove = 0x10738e60;    // (list, element)
constexpr std::uint32_t release = 0x10411f10;        // (object, 1): destroys an offer or list object
constexpr std::uint32_t construct_offer = 0x10411e60, fill_offer = 0x104164f0, allocate = 0x10773520;
constexpr std::int32_t pool_cell = -0x4ba0;

void rewind(Cpu& c, std::uint32_t list) { c.put32(list + 0xc, c.u32(list + 4)); }

// The club of a contract found by 0x10082f20, or 0 when none was found.
std::uint32_t club_of_found(Cpu& c, std::uint32_t found) {
    return found ? c.u32(c.toc(toc::clubs)) + c.u32(found + 4) * club_size : 0;
}
// trunc(scale * (double(value) - magic)): the original's int-to-double through the magic pair, then fctiwz.
std::int32_t scaled(Cpu& c, guest::Fpu& fpu, std::uint32_t pool, std::uint32_t scale_at, std::uint32_t value) {
    const auto bits = (std::uint64_t(0x43300000) << 32) | (value ^ 0x80000000u);
    const auto difference = fpu.fsub(std::bit_cast<double>(bits), c.f64(pool + 8));
    return std::int32_t(fpu.fctiwz(0, fpu.fmul(c.f64(pool + scale_at), difference)));
}
}

// ---- 0x103ef580 ----
std::uint32_t start_reaction_offer(Cpu& c, std::uint32_t club, std::uint32_t person, std::uint32_t out_offer,
                                   std::uint32_t out_reaction, std::uint32_t fourth) {
    constexpr std::uint32_t reaction_plain = 0x32, reaction_eager = 0x50, reaction_reluctant = 0x3c, reaction_firm = 0x46;
    constexpr std::uint32_t scale_fee = 0x70, scale_extra = 0xd0;
    Frame frame(c, 0x103ef580, 0x80, {.first_gpr = 25});
    const auto pool = c.toc(pool_cell);
    keep(c, 25, c.toc(toc::found_contract));
    keep(c, 26, club);
    keep(c, 27, person);
    keep(c, 28, out_offer);
    keep(c, 29, out_reaction);
    keep(c, 30, fourth);
    keep(c, 31, pool);
    guest::Fpu fpu(c);

    const auto found = invoke(c, find_contract, 0x103ef5bc, {c.u32(c.toc(toc::contracts)), person});
    c.put32(c.toc(toc::found_contract), found);
    const auto seller = club_of_found(c, found);
    // The club asking must be at least 4 ranks (halfword at +0x1ec) better than the seller.
    const auto seller_rank = std::int32_t(c.i16(seller + 0x1ec));
    if (std::int32_t(c.i16(club + 0x1ec)) - seller_rank < 4) return 0;
    const auto career = c.u32(person + 0x14);
    if (career == 0) return 0;
    const auto career_start = std::int32_t(c.i16(career + 4));
    if (std::int32_t(c.i16(career + 6)) - career_start < 0x19) return 0;
    if (std::int8_t(c.u16(person + 0x5c)) > 0x1e) return 0;

    auto object = invoke(c, allocate, 0x103ef650, {0x54});
    keep(c, 25, object);
    if (object) invoke(c, construct_offer, 0x103ef65c, {object});
    c.put32(out_offer, object);
    invoke(c, fill_offer, 0x103ef674, {c.u32(out_offer), person, club, fourth});
    c.put8(out_reaction, reaction_plain);
    const auto rank = std::int32_t(c.i16(club + 0x1ec));
    // The offer's own fee (+0x20) is rescaled in place.
    const auto rescale_fee = [&] {
        const auto offer = c.u32(out_offer);
        c.put32(offer + 0x20, std::uint32_t(scaled(c, fpu, pool, scale_fee, c.u32(offer + 0x20))));
    };
    const auto value_rank = [&] { return std::int32_t(c.i16(c.u32(person + 0x14) + 10)) / 500 - rank; };
    if (rank < 0xf && value_rank() > 2) {
        rescale_fee();
        c.put8(c.u32(out_offer) + 0x50, 0x14);
        c.put8(out_reaction, reaction_eager);
        return 1;
    }
    const auto contract = c.u32(person + 0xc);
    if (contract != 0 && c.i16(contract + 0xac) > 0x1388 && c.u8(person + 0x5e) == 0) {
        auto offer = c.u32(out_offer);
        c.put32(offer + 0x48, std::uint32_t(scaled(c, fpu, pool, scale_extra, c.u32(offer + 0x20))));
        c.put8(c.u32(out_offer) + 0x4c, 10);
        rescale_fee();
        c.put8(out_reaction, reaction_firm);
        return 1;
    }
    if (value_rank() < -3) {
        auto offer = c.u32(out_offer);
        c.put32(offer + 0x40, std::uint32_t(scaled(c, fpu, pool, scale_extra, c.u32(offer + 0x20))));
        c.put8(c.u32(out_offer) + 0x44, 0x14);
        rescale_fee();
        c.put8(out_reaction, reaction_reluctant);
        return 1;
    }
    invoke(c, release, 0x103ef880, {c.u32(out_offer), 1});
    return 0;
}

// ---- 0x103fb630 ----
void remove_offer(Cpu& c, std::uint32_t manager, std::uint32_t offer) {
    constexpr std::uint32_t slots = 0, count = 8, held = 0xc, held_count = 0x14, lowest_free = 0x990, rows = 0x9a0, list_at = 0xa78;
    constexpr std::uint32_t offer_held = 0x30, offer_slot = 0x28, kind = 0x24, club_field = 8, person_field = 0xc, people_list = 4;
    Frame frame(c, 0x103fb630, 0x60, {.first_gpr = 25});
    keep(c, 26, manager);
    keep(c, 27, offer);
    keep(c, 28, c.toc(toc::people));
    keep(c, 29, c.toc(toc::contracts));
    keep(c, 30, c.toc(toc::found_contract));
    keep(c, 31, c.toc(toc::clubs));
    const auto person_at = [&](std::uint32_t index) { return c.u32(c.toc(toc::people)) + index * person_size; };

    if (c.i32(offer + offer_held) != -1) {
        invoke(c, release, 0x103fb678, {c.u32(c.u32(manager + held) + c.u32(offer + offer_held) * 4), 1});
        c.put32(c.u32(manager + held) + c.u32(offer + offer_held) * 4, 0);
        c.put32(manager + held_count, c.u32(manager + held_count) - 1);
    }
    // A plain offer also leaves the manager's list of open bids (entries: person, club).
    if (c.u8(offer + kind) == 1 && c.u32(manager + list_at) != 0) {
        rewind(c, c.u32(manager + list_at));
        for (;;) {
            const auto entry = invoke(c, list_next, 0x103fb6f0, {c.u32(manager + list_at)});
            if (entry == 0) break;
            if (c.i32(entry + 4) == c.i32(offer + club_field) && c.i32(entry) == c.i32(offer + person_field)) {
                invoke(c, list_remove, 0x103fb6e4, {c.u32(manager + list_at), entry});
                break;
            }
        }
    }
    // Everyone listed in the offer's player list loses a contract slot.
    if (c.u32(offer + people_list) != 0) {
        rewind(c, c.u32(offer + people_list));
        for (;;) {
            const auto member = invoke(c, list_next, 0x103fb798, {c.u32(offer + people_list)});
            keep(c, 25, member);
            if (member == 0) break;
            const auto found = invoke(c, find_contract, 0x103fb72c,
                                      {c.u32(c.toc(toc::contracts)), person_at(c.u32(offer + person_field))});
            c.put32(c.toc(toc::found_contract), found);
            if (club_of_found(c, found) != 0)
                invoke(c, 0x1007f930, 0x103fb774, {c.u32(c.toc(toc::contracts)), person_at(c.u32(member)), c.u32(offer + offer_slot)});
            const auto row = c.u32(manager + rows) + c.u32(member) * 10 + 2;
            c.put8(row, std::uint8_t(c.u8(row) - 1));
        }
    }
    invoke(c, 0x1007f840, 0x103fb7c8,
           {c.u32(c.toc(toc::contracts)), person_at(c.u32(offer + person_field)),
            c.u32(c.toc(toc::clubs)) + c.u32(offer + club_field) * club_size});
    const auto row = c.u32(manager + rows) + c.u32(offer + person_field) * 10 + 1;
    c.put8(row, std::uint8_t(c.u8(row) - 1));
    if (c.i32(offer + offer_slot) < c.i32(manager + lowest_free)) c.put32(manager + lowest_free, c.u32(offer + offer_slot));
    c.put32(c.u32(manager + slots) + c.u32(offer + offer_slot) * 4, 0);
    invoke(c, release, 0x103fb818, {offer, 1});
    c.put32(manager + count, c.u32(manager + count) - 1);
}
}

// TRANSFER_MANAGER.CPP: offer queries (block "queries"): open offers of a person, fee checks, may-offer test.
#include "transfer_manager_native_queries.hpp"
#include "transfer_manager_native_queries_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace queries_detail;
bool person_has_open_offer(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    Frame frame(c, 0x103f4c00, 0x50, {.first_gpr = 30});
    keep(c, 31, person);
    keep(c, 30, manager);
    const auto iterator = frame.at(iterator_slot);
    invoke(c, iterator_construct, 0x103f4c24, {iterator});
    if (std::int8_t(invoke(c, has_offers, 0x103f4c34, {manager, person, 0})) == 0) {
        destroy_iterator(c, iterator, 0x103f4c48);
        return false;
    }
    start_offer_walk(c, frame, manager, person, 0, 0x103f4c64);
    while (const auto step = invoke(c, iterator_next, 0x103f4ce0, {iterator})) {
        const auto kind = c.i8(offer_of_step(c, manager, step) + offer_kind);
        if (kind == 0 || kind == 1 || kind == 5 || kind == 6) {
            destroy_iterator(c, iterator, 0x103f4cd0);
            return true;
        }
    }
    destroy_iterator(c, iterator, 0x103f4cf4);
    return false;
}

bool person_has_offer_from_club(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club_id_at) {
    Frame frame(c, 0x103f5c00, 0x60, {.first_gpr = 29});
    keep(c, 31, club_id_at);
    keep(c, 30, manager);
    keep(c, 29, person);
    const auto iterator = frame.at(iterator_slot);
    invoke(c, iterator_construct, 0x103f5c2c, {iterator});
    // A person beyond the table is refused; a null person is read at guest address 0 as the original does.
    if (person != 0 && c.i32(person) >= std::int32_t(c.u32(c.toc(people_count_cell)))) {
        destroy_iterator(c, iterator, 0x103f5c54);
        return false;
    }
    if ((c.u8(status_row(c, manager, c.u32(person)) + status_row_flags) & 1) == 0) {
        destroy_iterator(c, iterator, 0x103f5c80);
        return false;
    }
    start_offer_walk(c, frame, manager, person, 0, 0x103f5c9c);
    while (const auto step = invoke(c, iterator_next, 0x103f5ce8, {iterator})) {
        const auto index = c.u32(step);
        const auto list = c.u32(manager);
        const auto wanted = c.u32(club_id_at);
        if (c.u32(c.u32(list + index * 4) + offer_club) == wanted) {
            destroy_iterator(c, iterator, 0x103f5cd4);
            return true;
        }
    }
    destroy_iterator(c, iterator, 0x103f5cfc);
    return false;
}

std::uint32_t person_offer_from_club(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club_id_at, std::uint32_t state_flag) {
    Frame frame(c, 0x103f5d20, 0x60, {.first_gpr = 28});
    keep(c, 31, club_id_at);
    keep(c, 30, manager);
    keep(c, 29, state_flag);  // the caller's r6 until the walk replaces it with the offer
    keep(c, 28, person);
    const auto iterator = frame.at(iterator_slot);
    invoke(c, iterator_construct, 0x103f5d54, {iterator});
    if (person != 0 && c.i32(person) >= std::int32_t(c.u32(c.toc(people_count_cell)))) {
        destroy_iterator(c, iterator, 0x103f5d7c);
        return 0;
    }
    if (std::int8_t(state_flag) == 0 && (c.u8(status_row(c, manager, c.u32(person)) + status_row_flags) & 1) == 0) {
        destroy_iterator(c, iterator, 0x103f5db0);
        return 0;
    }
    start_offer_walk(c, frame, manager, person, state_flag, 0x103f5dcc);
    while (const auto step = invoke(c, iterator_next, 0x103f5e18, {iterator})) {
        const auto index = c.u32(step);
        const auto list = c.u32(manager);
        const auto wanted = c.u32(club_id_at);
        const auto offer = c.u32(list + index * 4);
        keep(c, 29, offer);
        if (c.u32(offer + offer_club) == wanted) {
            destroy_iterator(c, iterator, 0x103f5e04);
            return offer;
        }
    }
    destroy_iterator(c, iterator, 0x103f5e2c);
    return 0;
}
}

// TRANSFER_MANAGER.CPP native-first block "approaches": filing offers and approaches (0x103e25d0, 0x103e2db0,
// 0x103e3240, 0x103e3170).
#include "transfer_manager_native_approaches.hpp"
#include "transfer_manager_native_approaches_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace approaches;
namespace {
constexpr std::int32_t pool_cell = -0x4b9c;  // assertion texts (+0x37 file, +0x6ff format, +0x70c title)
constexpr std::uint32_t realloc_block = 0x107751b0, alloc_zeroed = 0x10775200, memset_block = 0x10778300,
                        allocate = 0x10773520, construct_offer = 0x10411e60, destruct_offer = 0x10411f10,
                        construct_list = 0x10738c00, copy_list = 0x10739010, add_to_list = 0x10738d10,
                        next_in_list = 0x10738e00, find_contract = 0x10082f20;
constexpr std::uint32_t growth_step = 0x32;

// A table of record pointers (pointer, capacity and, at the offset given, count words in the manager) grows
// by 0x32 entries when it is full; the new entries are cleared.
}

// ---- 0x103e25d0 ----
void add_offer(Cpu& c, std::uint32_t manager, std::uint32_t source, std::uint32_t news_extra) {
    constexpr std::uint32_t slot_cell = 0x40, other_cell = 0x3c, status_cell = 0x39, flag_cell = 0x38;
    Frame frame(c, 0x103e25d0, 0xd0, {.first_gpr = 18});
    const auto people_cell = c.toc(-0x7be0), found_cell = c.toc(-0x7b78), clubs_cell = c.toc(-0x7be8);
    const auto flag_address = c.toc(-0x7bac), text_a = c.toc(-0x79e0), text_b = c.toc(-0x79dc), buffer = c.toc(-0x7ba8),
               pool = c.toc(pool_cell);
    const auto person = c.u32(source + offer::person), club = c.u32(source + offer::club);
    keep(c, 29, people_cell);
    keep(c, 30, found_cell);
    keep(c, 31, clubs_cell);
    keep(c, 20, flag_address);
    keep(c, 21, text_a);
    keep(c, 22, text_b);
    keep(c, 23, buffer);
    keep(c, 24, pool);
    keep(c, 25, manager);
    keep(c, 26, source);
    keep(c, 27, news_extra);
    keep(c, 19, person);
    keep(c, 28, club);
    c.put32(frame.at(slot_cell), c.u32(manager + manager::last_slot));
    c.put32(frame.at(other_cell), 0xffffffff);
    c.put32(manager + manager::last_slot, 0xffffffff);
    // r18 is 1 when the person's status row has no appearances yet (the first-offer case), then other values.
    const bool fresh_row = c.u8(c.u32(manager + manager::status_rows) + person * status_row_size + 1) == 0;
    keep(c, 18, fresh_row ? 1 : 0);
    if (c.u32(source + 0x20) == 0) c.put8(source + 0x51, 0);

    if (c.u32(manager + manager::offers_count) == c.u32(manager + manager::offers_size)) {
        const auto capacity = c.u32(manager + manager::offers_size) + growth_step;
        c.put32(manager + manager::offers_size, capacity);
        if (c.u32(manager + manager::offers) == 0) {
            c.put32(manager + manager::offers, invoke(c, alloc_zeroed, 0x103e2748, {capacity, 4}));
        } else {
            c.put32(manager + manager::offers,
                    invoke(c, realloc_block, 0x103e2690, {c.u32(manager + manager::offers), capacity << 2}));
            if (c.u32(manager + manager::offers) == 0) {
                // Out of memory: the original reports it with its own text and ends the game's run.
                invoke(c, 0x10739890, 0x103e26b8, {pool + 0x37, 0, 0, frame.at(0x74), frame.at(0x4c)});
                const auto version = invoke(c, 0x10768810, 0x103e26bc);
                invoke(c, 0x1077b720, 0x103e26dc, {buffer, pool + 0x4c, version, frame.at(0x74), frame.at(0x4c), 0x4d2, pool + 0x17a0});
                invoke(c, 0x10727890, 0x103e26e8, {text_b, pool + 0xa6});
                invoke(c, 0x10727890, 0x103e26f8, {text_a, pool + 0xbe, buffer});
                invoke(c, 0x10769b60, 0x103e2708, {text_b, text_a, 0});
                c.put8(flag_address, 0);
                invoke(c, 0x1076f7d0, 0x103e2718, {0xffffffff});
            }
            invoke(c, memset_block, 0x103e2738,
                   {c.u32(manager + manager::offers) + ((c.u32(manager + manager::offers_size) - growth_step) << 2), 0, 200});
        }
    }

    // Walk the table: the slot of this person's offer from the same club (replaced), the first free slot, and
    // the slot of another club's offer for the same person.
    std::int32_t slot = signed_of(c.u32(frame.at(slot_cell))), other = -1;
    {
        std::uint32_t cell = c.u32(manager + manager::offers), count = 0;
        for (std::int32_t index = 0; index < signed_of(c.u32(manager + manager::offers_size)); ++index, cell += 4) {
            const auto entry = c.u32(cell);
            if (entry == 0) {
                if (slot == -1) slot = index;
                if (count == c.u32(manager + manager::offers_count) || fresh_row) break;
            } else {
                ++count;
                if (signed_of(c.u32(entry + offer::person)) == signed_of(person)) {
                    if (signed_of(c.u32(entry + offer::club)) == signed_of(club)) slot = index;
                    else if (other == -1) other = index;
                }
            }
        }
        c.put32(frame.at(slot_cell), unsigned_of(slot));
        c.put32(frame.at(other_cell), unsigned_of(other));
    }
    if (slot == -1) {
        report_assertion(c, {0x103e27fc, pool + 0x37, pool + 0x6ff, pool + 0x70c, frame.at(0x54), frame.at(0x44), 0x511});
        return;
    }

    const auto cell_of = [&](std::int32_t index) { return c.u32(manager + manager::offers) + (unsigned_of(index) << 2); };
    const auto slot_now = [&] { return signed_of(c.u32(frame.at(slot_cell))); };
    const auto other_now = [&] { return signed_of(c.u32(frame.at(other_cell))); };
    const auto stored = [&] { return c.u32(cell_of(slot_now())); };
    if (const auto old = c.u32(cell_of(slot)); old != 0) invoke_native(c, 0x103fb630, 0x103e2858, {manager, old});
    const auto fresh = invoke(c, allocate, 0x103e2860, {offer::size});
    keep(c, 18, fresh);
    if (fresh != 0) invoke(c, construct_offer, 0x103e286c, {fresh});
    c.put32(cell_of(slot_now()), fresh);
    c.put32(manager + manager::offers_count, c.u32(manager + manager::offers_count) + 1);
    copy_offer_fields(c, stored(), source);
    c.put32(stored() + offer::slot, unsigned_of(slot_now()));
    c.put32(source + offer::slot, unsigned_of(slot_now()));
    c.put32(stored() + 0x30, 0xffffffff);
    c.put8(stored() + offer::state, 0);
    c.put8(stored() + 0x10, 0);
    if (c.u8(stored() + 0x35) == 2) c.put8(stored() + 0x35, 1);
    const auto serial = c.u32(manager + manager::next_serial);
    c.put32(manager + manager::next_serial, serial + 1);
    c.put32(stored() + offer::serial, serial);
    c.put32(source + offer::serial, serial);

    if (c.u32(source + offer::list) != 0) {
        const auto list = invoke(c, allocate, 0x103e2a28, {0x10});
        keep(c, 18, list);
        if (list != 0) invoke(c, construct_list, 0x103e2a34, {list});
        c.put32(stored() + offer::list, list);
        invoke(c, copy_list, 0x103e2a64, {c.u32(stored() + offer::list), c.u32(source + offer::list)});
    }
    if (other_now() != -1) {
        // Link this offer to the other club's offer for the person through a shared list of slots.
        const auto shared = c.u32(c.u32(cell_of(other_now())));
        if (shared != 0) {
            invoke(c, add_to_list, 0x103e2a94, {shared, frame.at(slot_cell), 4});
            const auto link = c.u32(c.u32(cell_of(other_now())));
            c.put32(stored(), link);
        } else {
            const auto list = invoke(c, allocate, 0x103e2ac4, {0x10});
            keep(c, 18, list);
            if (list != 0) invoke(c, construct_list, 0x103e2ad0, {list});
            invoke(c, add_to_list, 0x103e2ae0, {list, frame.at(other_cell), 4});
            invoke(c, add_to_list, 0x103e2af0, {list, frame.at(slot_cell), 4});
            c.put32(c.u32(cell_of(other_now())), list);
            c.put32(stored(), list);
        }
    }

    // The person's contract and club decide the first status of the offer.
    keep(c, 18, 0);
    const auto contract = invoke(c, find_contract, 0x103e2b38, {c.u32(c.toc(toc::contracts)), c.u32(people_cell) + c.u32(source + offer::person) * person_size});
    c.put32(found_cell, contract);
    const bool counts = counts_for_club(c, contract);
    if (counts) keep(c, 18, 1);
    const auto contract_club = counts ? club_of(c, clubs_cell, c.u32(found_cell)) : 0;
    c.put8(frame.at(flag_cell), 0);
    const auto is_human = [&](std::uint32_t at, std::uint32_t back) {
        return std::int8_t(invoke(c, 0x1014c2e0, back, {at, 0, 1, 0, 0})) != 0;
    };
    if (std::int8_t(c.u8(source + 0x37)) != 0) {
        const auto human = is_human(c.u32(clubs_cell) + club * club_size, 0x103e2bb8);
        c.put8(stored() + offer::state, 7);
        c.put8(frame.at(status_cell), human ? 0x1c : 0);
    } else if (contract_club != 0 && is_human(contract_club, 0x103e2c24)) {
        c.put8(frame.at(status_cell), 0x1c);
    } else {
        invoke_native(c, 0x103f3bf0, 0x103e2c58, {manager, stored(), frame.at(status_cell), frame.at(flag_cell)});
    }
    if (std::int8_t(c.u8(frame.at(status_cell))) != 0 || std::int8_t(c.u8(frame.at(flag_cell))) != 0)
        invoke(c, 0x103eb4a0, 0x103e2c8c, {manager, stored(), c.u8(frame.at(status_cell)), c.u8(frame.at(flag_cell))});
    if (c.u8(stored() + offer::kind) == 1) invoke_native(c, 0x1040d5c0, 0x103e2cb4, {manager, stored(), news_extra});
    else invoke_native(c, 0x103fdc20, 0x103e2cc0, {manager, stored()});

    const auto rows = [&] { return c.u32(manager + manager::status_rows); };
    const auto row_of_person = c.u32(source + offer::person) * status_row_size;
    c.put8(rows() + row_of_person, c.u8(rows() + row_of_person) | 1);
    {
        const auto at = c.u32(manager + manager::status_rows) + c.u32(source + offer::person) * status_row_size + 1;
        c.put8(at, c.u8(at) + 1);
    }
    if (const auto list = c.u32(source + offer::list); list != 0) {
        c.put32(list + 0xc, c.u32(list + 4));
        while (const auto listed = invoke(c, next_in_list, 0x103e2d34, {c.u32(source + offer::list)})) {
            const auto at = rows() + c.u32(listed) * status_row_size + 2;
            c.put8(at, c.u8(at) + 1);
        }
    }
    if (is_human(c.u32(clubs_cell) + c.u32(source + offer::club) * club_size, 0x103e2d60)) {
        const auto person_record = c.u32(people_cell) + c.u32(source + offer::person) * person_size;
        if (c.u32(person_record + 0x14) != 0)
            invoke(c, 0x1030fda0, 0x103e2d94, {c.u32(c.toc(-0x7b1c)), person_record, 0});
    }
}

// ---- 0x103e2db0 ----
void add_approach(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t copy, std::int8_t mark) {
    Frame frame(c, 0x103e2db0, 0x80, {.first_gpr = 25});
    const auto clubs_cell = c.toc(-0x7be8), buffer = c.toc(-0x7ba8), pool = c.toc(pool_cell);
    keep(c, 29, clubs_cell);
    keep(c, 25, manager);
    keep(c, 30, buffer);
    keep(c, 26, offer);
    keep(c, 31, pool);
    keep(c, 27, copy);
    keep(c, 28, 0xffffffff);
    if (mark != 0) {
        c.put8(offer + 0x35, 1);
        c.put8(copy + 0x35, 1);
        invoke(c, 0x1036fa00, 0x103e2e20,
               {c.u32(manager + manager::shortlist), c.u32(c.toc(-0x7be0)) + c.u32(offer + offer::person) * person_size,
                c.u32(clubs_cell) + c.u32(offer + offer::club) * club_size, c.u32(offer)});
    }
    std::int32_t slot = signed_of(c.u32(offer + 0x30));
    if (slot == -1) {
        if (c.u32(manager + manager::approaches_count) == c.u32(manager + manager::approaches_size)) {
            const auto capacity = c.u32(manager + manager::approaches_size) + growth_step;
            c.put32(manager + manager::approaches_size, capacity);
            if (c.u32(manager + manager::approaches) != 0) {
                c.put32(manager + manager::approaches,
                        invoke(c, realloc_block, 0x103e2e5c, {c.u32(manager + manager::approaches), capacity << 2}));
                invoke(c, memset_block, 0x103e2e80,
                       {c.u32(manager + manager::approaches) + ((c.u32(manager + manager::approaches_size) - growth_step) << 2), 0, 200});
            } else {
                c.put32(manager + manager::approaches, invoke(c, alloc_zeroed, 0x103e2e90, {capacity, 4}));
            }
        }
        std::int32_t found = -1;
        {
            std::uint32_t cell = c.u32(manager + manager::approaches);
            for (std::int32_t index = 0; index < signed_of(c.u32(manager + manager::approaches_size)); ++index, cell += 4)
                if (c.u32(cell) == 0) { found = index; break; }
        }
        if (found != -1) keep(c, 28, unsigned_of(found));
        if (found == -1) {
            report_assertion(c, {0x103e2ee8, pool + 0x37, pool + 0x6ff, pool + 0x70c, frame.at(0x40), frame.at(0x38), 0x60f});
            return;
        }
        c.put32(offer + 0x30, unsigned_of(found));
        c.put8(offer + offer::state, 1);
        slot = found;
    } else {
        keep(c, 28, unsigned_of(slot));
    }
    const auto index_bytes = unsigned_of(slot) << 2;
    keep(c, 30, index_bytes);
    const auto cell = [&] { return c.u32(manager + manager::approaches) + index_bytes; };
    if (const auto old = c.u32(cell()); old != 0) invoke(c, destruct_offer, 0x103e2f5c, {old, 1});
    else c.put32(manager + manager::approaches_count, c.u32(manager + manager::approaches_count) + 1);
    auto fresh = invoke(c, allocate, 0x103e2f74, {offer::size});
    keep(c, 28, fresh);
    if (fresh != 0) invoke(c, construct_offer, 0x103e2f80, {fresh});
    c.put32(cell(), fresh);
    const auto stored = [&] { return c.u32(cell()); };
    copy_offer_fields(c, stored(), copy);
    c.put32(stored() + offer::slot, c.u32(offer + offer::slot));
    const auto serial = c.u32(manager + manager::next_serial);
    c.put32(manager + manager::next_serial, serial + 1);
    c.put32(stored() + offer::serial, serial);
    if (c.u32(copy + offer::list) != 0) {
        fresh = invoke(c, allocate, 0x103e30a4, {0x10});
        keep(c, 28, fresh);
        if (fresh != 0) invoke(c, construct_list, 0x103e30b0, {fresh});
        c.put32(stored() + offer::list, fresh);
        invoke(c, copy_list, 0x103e30d0, {c.u32(stored() + offer::list), c.u32(copy + offer::list)});
    }
    const auto answered = invoke(c, 0x102cdbf0, 0x103e30ec,
                                 {c.u32(c.toc(toc::news)), c.u32(clubs_cell) + c.u32(copy + offer::club) * club_size});
    if (std::int8_t(answered) == 0) {
        invoke(c, 0x103eb4a0, 0x103e3158, {manager, offer, 2, 0});
    } else {
        if (c.u8(offer + offer::kind) == 1) invoke_native(c, 0x104108c0, 0x103e3114, {manager, stored(), offer});
        else invoke_native(c, 0x104019d0, 0x103e312c, {manager, stored(), offer});
        invoke(c, 0x103eb4a0, 0x103e3140, {manager, offer, 0xe, 0});
    }
}

// ---- 0x103e3240 ----
std::uint32_t add_approach_from_terms(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t term_a,
                                      std::uint32_t term_b, std::uint32_t term_c, std::uint32_t term_d) {
    constexpr std::uint32_t local_offer = 0x38, terms = 0x8c;
    Frame frame(c, 0x103e3240, 0x110, {.first_gpr = 26});
    keep(c, 30, manager);
    keep(c, 31, offer);
    keep(c, 26, term_a);
    keep(c, 27, term_b);
    keep(c, 28, term_c);
    keep(c, 29, term_d);
    invoke(c, 0x103721f0, 0x103e3270, {frame.at(terms)});
    invoke(c, construct_offer, 0x103e3278, {frame.at(local_offer)});
    copy_offer_fields(c, frame.at(local_offer), offer);
    c.put32(frame.at(local_offer + 4), 0);
    c.put32(frame.at(local_offer), 0);
    invoke(c, 0x103723b0, 0x103e339c,
           {frame.at(terms), c.u32(c.toc(-0x7be0)) + c.u32(offer + offer::person) * person_size,
            c.u32(c.toc(toc::clubs)) + c.u32(offer + offer::club) * club_size, term_a, term_b, term_c, term_d});
    if (c.u32(manager + manager::reports) == 0) {
        const auto list = invoke(c, allocate, 0x103e33b0, {0x10});
        keep(c, 26, list);
        if (list != 0) invoke(c, construct_list, 0x103e33bc, {list});
        c.put32(manager + manager::reports, list);
    }
    invoke(c, add_to_list, 0x103e33d0, {c.u32(manager + manager::reports), frame.at(terms), 0x64});
    invoke_native(c, 0x103e2db0, 0x103e33e4, {manager, offer, frame.at(local_offer), 0});
    const auto slot_word = c.u32(frame.at(local_offer + offer::slot));
    keep(c, 30, slot_word);
    invoke(c, destruct_offer, 0x103e33f4, {frame.at(local_offer), 0xffffffff});
    return slot_word;
}

// ---- 0x103e3170 ----
std::uint32_t build_and_add_offer(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club,
                                  std::uint32_t term_a, std::uint32_t term_b, std::uint32_t term_c, std::uint32_t term_d) {
    constexpr std::uint32_t local_offer = 0x9c, terms = 0x38;
    Frame frame(c, 0x103e3170, 0x110, {.first_gpr = 25});
    keep(c, 25, manager);
    keep(c, 26, person);
    keep(c, 27, club);
    keep(c, 28, term_a);
    keep(c, 29, term_b);
    keep(c, 30, term_c);
    keep(c, 31, term_d);
    invoke(c, construct_offer, 0x103e31a4, {frame.at(local_offer)});
    invoke(c, 0x104163d0, 0x103e31b4, {frame.at(local_offer), person, club});
    invoke(c, 0x103721f0, 0x103e31bc, {frame.at(terms)});
    invoke(c, 0x103723b0, 0x103e31dc, {frame.at(terms), person, club, term_a, term_b, term_c, term_d});
    invoke_native(c, 0x103e25d0, 0x103e31ec, {manager, frame.at(local_offer), frame.at(terms)});
    invoke(c, 0x1007f200, 0x103e3214,
           {c.u32(c.toc(toc::contracts)), frame.at(terms), c.u32(frame.at(local_offer + offer::slot)),
            c.u32(frame.at(local_offer + offer::serial)), 0xb, 0, 1, 0});
    const auto slot_word = c.u32(frame.at(local_offer + offer::slot));
    keep(c, 31, slot_word);
    invoke(c, destruct_offer, 0x103e3224, {frame.at(local_offer), 0xffffffff});
    return slot_word;
}
}

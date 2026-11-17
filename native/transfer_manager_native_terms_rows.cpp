#include "transfer_manager_native_terms.hpp"
#include "transfer_manager_native_stages_detail.hpp"
#include <optional>
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace stages;

namespace {
constexpr std::uint32_t row_size = 0x1c, row_growth = 0x14;
constexpr std::uint32_t offers_array = 0x0, offers_count = 4, date_rows = 0xa50, date_row_count = 0xa58, date_row_size = 12;
constexpr std::uint32_t is_withdrawn = 0x103fb390, list_append = 0x10738d10, realloc_rows = 0x107751b0, sort_rows = 0x10768300;
constexpr std::uint32_t case_table_first = 0x103f4804, case_table_default = 0x103f483c;

// The return addresses of the calls of the out-of-memory report.
struct GrowReport { std::uint32_t build, result, print, first_text, second_text, finish; };
// A register the original has set by a given call; one it has not yet set keeps the caller's value.
struct Held {
    std::optional<std::uint32_t> r18, r19, r20, r21, r22, r23, r24, r30, r31;
};
}

std::uint32_t club_offer_rows(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t rows_out) {
    Frame frame(c, 0x103f45a0, 0x100, {.first_gpr = 13});
    const auto people_cell = c.toc(toc::people), contracts_cell = c.toc(toc::contracts), found_cell = c.toc(toc::found_contract);
    const auto clubs_cell = c.toc(toc::clubs), strings = c.toc(-0x4b9c), log_cell = c.toc(-0x7ba8);
    const auto message_a = c.toc(-0x79dc), message_b = c.toc(-0x79e0);
    Held h;
    const auto sync = [&] {
        keep(c, 13, message_a);
        keep(c, 14, message_b);
        keep(c, 15, manager);
        keep(c, 16, club);
        keep(c, 17, rows_out);
        const auto put = [&](unsigned n, const std::optional<std::uint32_t>& value) { if (value) keep(c, n, *value); };
        put(18, h.r18); put(19, h.r19); put(20, h.r20); put(21, h.r21); put(22, h.r22); put(23, h.r23);
        put(24, h.r24); put(30, h.r30); put(31, h.r31);
        keep(c, 25, people_cell);
        keep(c, 26, contracts_cell);
        keep(c, 27, found_cell);
        keep(c, 28, clubs_cell);
        keep(c, 29, strings);
    };
    const auto call = [&](std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        sync();
        return invoke(c, target, back, arguments);
    };
    const auto list = frame.at(0x58), today = frame.at(0x50);
    call(list_construct, 0x103f45e0, {list});
    h.r19 = 0; h.r20 = 0; h.r21 = 0; h.r31 = 0;
    call(0x10715880, 0x103f45f8, {today});
    const auto club_id = c.u32(club);
    h.r18 = club_id;
    const auto offer_at = [&](std::uint32_t i) { return c.u32(c.u32(manager + offers_array) + i * 4); };
    // The offers the club's own club bids with (the buying club), and those whose person it holds, by index.
    const auto index_slot = frame.at(0x3c);
    for (c.put32(index_slot, 0); c.i32(index_slot) < c.i32(manager + offers_count); c.put32(index_slot, c.u32(index_slot) + 1)) {
        const auto offer = offer_at(c.u32(index_slot));
        if (!offer) continue;
        if (std::int8_t(call(is_withdrawn, 0x103f4624, {manager, offer})) != 0) continue;
        if (club_id != c.u32(offer_at(c.u32(index_slot)) + offer_club)) continue;
        call(list_append, 0x103f4658, {list, index_slot, 4});
    }
    const auto holder_slot = frame.at(0x38);
    for (c.put32(holder_slot, 0); c.i32(holder_slot) < c.i32(manager + offers_count); c.put32(holder_slot, c.u32(holder_slot) + 1)) {
        const auto offer = offer_at(c.u32(holder_slot));
        if (!offer) continue;
        if (std::int8_t(call(is_withdrawn, 0x103f469c, {manager, offer})) != 0) continue;
        h.r18 = 0;
        const auto contract = call(find_contract, 0x103f46d0, {c.u32(contracts_cell), person_record(c, c.u32(offer_at(c.u32(holder_slot)) + offer_person))});
        c.put32(found_cell, contract);
        const bool has_club = contract && contract_kind(c, contract) != no_club_contract_kind;
        h.r18 = has_club ? 1 : 0;
        if ((has_club ? club_record(c, c.u32(c.u32(found_cell) + 4)) : 0) == club) call(list_append, 0x103f4734, {list, holder_slot, 4});
    }
    rewind_list(c, list);

    const auto fill_date = [&](std::uint32_t offer, std::uint32_t& half, std::uint32_t& second, std::uint32_t& extra) {
        std::uint32_t row = 0;
        for (std::int32_t i = 0; i < c.i32(manager + date_row_count); ++i) {
            const auto rows = c.u32(manager + date_rows);
            if (c.i32(offer + 0x28) == c.i32(rows + i * date_row_size + 8)) { row = rows + i * date_row_size; break; }
        }
        if (row) { second = c.u16(row + 2); half = c.u16(row); extra = c.u8(row + 4); }
        else { second = c.u16(offer + 0x16); half = c.u16(offer + 0x14); extra = c.u8(offer + 0x18); }
    };
    // The report the original makes when the rows cannot grow: two lines to the log, `first`/`second` the frame buffers.
    const auto grow = [&](std::uint32_t first, std::uint32_t second, std::uint32_t line, const GrowReport& back,
                          std::uint32_t realloc_back, std::uint32_t destroy_back) -> bool {
        h.r20 = *h.r20 + row_growth;
        h.r21 = call(realloc_rows, realloc_back, {*h.r21, *h.r20 * row_size});
        if (*h.r21) return true;
        sync();
        invoke(c, 0x10739890, back.build, {strings + 0x37, 0, 0, first, second});
        const auto text = invoke(c, 0x10768810, back.result);
        invoke(c, 0x1077b720, back.print, {log_cell, strings + 0x4c, text, first, second, line, strings + 0x1bdf});
        invoke(c, 0x10727890, back.first_text, {message_a, strings + 0xa6});
        invoke(c, 0x10727890, back.second_text, {message_b, strings + 0xbe, log_cell});
        invoke(c, 0x10769b60, back.finish, {message_a, message_b, 0});
        c.put32(rows_out, 0);
        invoke(c, list_destroy, destroy_back, {list, 0xffffffff});
        return false;
    };

    while (true) {
        sync();
        const auto entry = invoke(c, next_listed, 0x103f4bb0, {list});
        if (!entry) {
            sync();
            invoke(c, sort_rows, 0x103f4bcc, {*h.r21, *h.r19, row_size, c.toc(-0x4bc0)});
            c.put32(rows_out, *h.r21);
            invoke(c, list_destroy, 0x103f4bdc, {list, 0xffffffff});
            return *h.r19;
        }
        const auto offer = offer_at(c.u32(entry));
        h.r22 = offer;
        if (c.u8(offer + 0x24) == 1) continue;
        h.r18 = 0;
        const auto contract = call(find_contract, 0x103f4794, {c.u32(contracts_cell), person_record(c, c.u32(offer + offer_person))});
        c.put32(found_cell, contract);
        const bool has_club = contract && contract_kind(c, contract) != no_club_contract_kind;
        h.r18 = has_club ? 1 : 0;
        const auto other = has_club ? club_record(c, c.u32(c.u32(found_cell) + 4)) : 0;
        h.r24 = other;
        // The stage byte picks the row's kind through the original's jump table (0 for stages out of range).
        std::uint32_t kind = 0;
        if (const auto stage = std::uint32_t(std::int32_t(c.i8(offer + offer_stage))); stage <= 0x13) {
            const auto target = c.u32(c.toc(-0x4bc4) + stage * 4);
            if (target == case_table_default) kind = 0;
            else if (target >= case_table_first && target < case_table_default && ((target - case_table_first) & 7) == 0)
                kind = (target - case_table_first) / 8;
            else throw std::logic_error("transfer_manager_native: stage table entry outside 0x103f4804..0x103f483c");
        }
        h.r18 = kind;
        std::uint32_t half, second, extra;
        fill_date(offer, half, second, extra);

        const auto own = c.i32(club) == c.i32(offer + offer_club);
        if (*h.r20 == *h.r19 && !grow(frame.at(0x88), frame.at(0x48), 0x2b2b, {0x103f4904, 0x103f4908, 0x103f4928, 0x103f4934, 0x103f4944, 0x103f4954}, 0x103f48e4, 0x103f4968))
            return 0;
        {
            const auto row = *h.r21 + *h.r31;
            c.put8(row + 0x19, kind);
            c.put16(row + 2, second);
            c.put16(row, half);
            c.put8(row + 4, extra);
            c.put32(row + 8, c.u32(offer + offer_person));
            c.put8(row + 0x1a, c.u8(offer + 0x35));
            if (own) {
                c.put32(row + 0xc, other ? c.u32(other) : 0xffffffff);
                c.put8(row + 0x10, 0);
            } else {
                c.put32(row + 0xc, c.u32(offer + offer_club));
                c.put8(row + 0x10, 1);
            }
            c.put32(row + 0x14, c.u32(offer + offer_fee));
            const auto kind_nibble = std::int8_t(c.u8(offer + 0x38)) >> 4;
            if (std::int8_t(c.u8(offer + 0x37)) != 0 && c.i32(offer + offer_fee) == 0 && (kind_nibble == 3 || kind_nibble == 4))
                c.put8(row + 0x18, 4);
            else c.put8(row + 0x18, c.u32(offer + offer_listed) ? 1 : 0);
        }
        const auto offer_list = c.u32(offer + offer_listed);
        h.r19 = *h.r19 + 1;
        h.r31 = *h.r31 + row_size;
        if (!offer_list) continue;
        h.r23 = *h.r19 * row_size;
        rewind_list(c, offer_list);
        while (true) {
            const auto listed = call(next_listed, 0x103f4ba0, {c.u32(offer + offer_listed)});
            h.r30 = listed;
            if (!listed) break;
            if (*h.r20 == *h.r19 && !grow(frame.at(0x68), frame.at(0x40), 0x2b62, {0x103f4aa8, 0x103f4aac, 0x103f4acc, 0x103f4ad8, 0x103f4ae8, 0x103f4af8}, 0x103f4a88, 0x103f4b0c))
                return 0;
            const auto row = *h.r21 + *h.r23;
            c.put8(row + 0x19, kind);
            c.put16(row + 2, second);
            c.put16(row, half);
            c.put8(row + 4, extra);
            c.put32(row + 8, c.u32(listed));
            if (own) {
                c.put32(row + 0xc, other ? c.u32(other) : 0xffffffff);
                c.put8(row + 0x10, 1);
            } else {
                c.put32(row + 0xc, c.u32(offer + offer_club));
                c.put8(row + 0x10, 0);
            }
            c.put32(row + 0x14, 0);
            h.r23 = *h.r23 + row_size;
            h.r19 = *h.r19 + 1;
            h.r31 = *h.r31 + row_size;
            c.put8(row + 0x18, 1);
        }
    }
}
}

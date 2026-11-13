// TRANSFER_OFFER.CPP: the offer's form in a saved game (transfer_offer_unit_record.hpp): the 0x54-byte record,
// then its club list (a count, then a club index each) and its exchange list (a count, then 8-byte rows).
#include "transfer_offer_unit_detail.hpp"
#include "transfer_offer_unit_record.hpp"

namespace cm::aot::transfer_offer_unit {
using guest::Cpu;
using guest::Frame;

namespace {
constexpr std::uint32_t file_write = 0x10747cc0, club_limit = 0x28, list_size = 0x10;
constexpr std::uint32_t club_row_size = 4, exchange_row_size = 8;

// Reading: the sites of the seven assertion reports (first call, buffer offsets, source line).
constexpr CheckSite no_record{0x10411ff8, 0x140, 0x78, 0x69}, no_club_count{0x10412068, 0x120, 0x70, 0x70},
    no_club_table{0x104120d0, 0x100, 0x68, 0x7a}, no_club_index{0x10412168, 0xe0, 0x60, 0x8d},
    club_missing{0x104121e0, 0xc0, 0x58, 0x99}, no_exchange_count{0x104122b0, 0xa0, 0x50, 0xbe},
    no_exchange_row{0x1041234c, 0x80, 0x40, 0xd0};
// Writing.
constexpr CheckSite write_record{0x10412430, 0xe4, 0x5c, 0xee}, write_club_count{0x104124bc, 0xc4, 0x54, 0x102},
    write_club{0x10412544, 0xa4, 0x4c, 0x110}, write_exchange_count{0x104125e0, 0x84, 0x44, 0x125},
    write_exchange{0x10412668, 0x64, 0x3c, 0x133};

// A list object the way the game makes one: allocated, constructed when the allocation gave memory.
// The original holds the new list in `holder` (a register) from the allocation on.
std::uint32_t new_list(Cpu& c, std::uint32_t allocated_back, std::uint32_t constructed_back, unsigned holder) {
    const auto list = invoke(c, callee::allocate_object, allocated_back, {list_size});
    c.set(holder, list);
    if (list != 0) invoke(c, callee::list_construct, constructed_back, {list});
    return list;
}
}

void read_from_file(Cpu& c, std::uint32_t offer, std::uint32_t file, std::uint32_t table) {
    Frame frame(c, 0x10411fa0, 0x180, {.first_gpr = 24});
    const auto count_at = frame.at(0x38), item_at = frame.at(0x3c), row_at = frame.at(0x48);
    // r25-r27 keep the arguments, r28 the list being built (0 for now), r29-r31 the globals the reports use.
    c.set({{25, offer}, {26, file}, {27, table}, {28, 0}, {29, c.toc(toc::success)}, {30, c.toc(toc::message)},
           {31, c.toc(toc::pool)}});
    // Items the file layer read, signed; every read here is one item.
    const auto read = [&](std::uint32_t at, std::uint32_t size, std::uint32_t back) {
        return std::int32_t(invoke(c, callee::file_read, back, {at, size, 1, file}));
    };
    if (read(offer, field::size, 0x10411fdc) != 1) return report_check(c, frame, no_record);
    if (read(count_at, 2, 0x1041204c) != 1) return report_check(c, frame, no_club_count);

    std::uint32_t pending = 0;  // the list being built, until it is kept or released
    if (c.i16(count_at) != 0) {
        if (table == 0) return report_check(c, frame, no_club_table);
        c.put32(offer + field::clubs, 0);
        pending = new_list(c, 0x10412120, 0x1041212c, 28);
        c.set(24, 0);  // the item counter
        std::int16_t n = 0;
        for (; n < c.i16(count_at); ++n) {
            c.set(24, std::uint32_t(std::int32_t(n)));
            if (read(item_at, club_row_size, 0x1041214c) != 1) return report_check(c, frame, no_club_index);
            if (c.i32(item_at) < c.i32(offer + club_limit)) {
                // The offer's first word becomes the first word of the club record at that index.
                const auto entry = c.u32(table + (c.u32(item_at) << 2));
                if (entry == 0) report_check(c, frame, club_missing);
                else c.put32(offer + field::clubs, c.u32(entry));
            }
            invoke(c, callee::list_append, 0x10412238, {pending, item_at, club_row_size});
        }
        c.set(24, std::uint32_t(std::int32_t(n)));
        if (c.u32(offer + field::clubs) == 0) {
            c.put32(offer + field::clubs, pending);
            pending = 0;
            c.set(28, 0);
        } else {
            invoke(c, callee::list_reset, 0x1041226c, {pending});
        }
    }
    if (pending != 0) invoke(c, callee::list_destroy, 0x10412280, {pending, 1});

    if (read(count_at, 2, 0x10412294) != 1) return report_check(c, frame, no_exchange_count);
    if (c.i16(count_at) == 0) {
        c.put32(offer + field::exchange, 0);
        return;
    }
    const auto exchange = new_list(c, 0x10412304, 0x10412310, 24);
    c.put32(offer + field::exchange, exchange);
    c.set(27, 0);  // the row counter reuses the table's register
    for (std::int16_t n = 0; n < c.i16(count_at); ++n) {
        c.set(27, std::uint32_t(std::int32_t(n)));
        if (read(row_at, exchange_row_size, 0x10412330) != 1) return report_check(c, frame, no_exchange_row);
        invoke(c, callee::list_append, 0x104123a0, {c.u32(offer + field::exchange), row_at, exchange_row_size});
    }
}

void write_to_file(Cpu& c, std::uint32_t offer, std::uint32_t file) {
    Frame frame(c, 0x104123e0, 0x120, {.first_gpr = 27});
    const auto count_at = frame.at(0x38);
    // r27 keeps the offer, r28 the file, r29 and r30 the globals the reports use, r31 the success cell.
    c.set({{27, offer}, {28, file}, {29, c.toc(toc::message)}, {30, c.toc(toc::pool)}, {31, c.toc(toc::success)}});
    const auto write = [&](std::uint32_t at, std::uint32_t size, std::uint32_t back) {
        return std::int32_t(invoke(c, file_write, back, {at, size, 1, file}));
    };
    // A list's count is its first halfword (0 for no list).
    const auto put_count = [&](std::uint32_t list) { c.put16(count_at, list == 0 ? 0 : c.u16(list)); };

    if (write(offer, field::size, 0x10412414) != 1) return report_check(c, frame, write_record);
    put_count(c.u32(offer + field::clubs));
    if (write(count_at, 2, 0x104124a0) != 1) return report_check(c, frame, write_club_count);
    if (c.u32(offer + field::clubs) != 0) {
        rewind(c, c.u32(offer + field::clubs));
        while (const auto row = invoke(c, callee::list_next, 0x1041258c, {c.u32(offer + field::clubs)}))
            if (write(row, club_row_size, 0x10412528) != 1) return report_check(c, frame, write_club);
    }
    put_count(c.u32(offer + field::exchange));
    if (write(count_at, 2, 0x104125c4) != 1) return report_check(c, frame, write_exchange_count);
    if (c.u32(offer + field::exchange) == 0) return;
    rewind(c, c.u32(offer + field::exchange));
    while (const auto row = invoke(c, callee::list_next, 0x104126b0, {c.u32(offer + field::exchange)}))
        if (write(row, exchange_row_size, 0x1041264c) != 1) return report_check(c, frame, write_exchange);
}
}

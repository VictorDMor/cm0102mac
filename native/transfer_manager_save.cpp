#include "transfer_manager_save.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

constexpr std::uint32_t write_file = 0x10747cc0, next_list_row = 0x10738e00, close_file = 0x10747900;
struct SaveSite {
    std::uint32_t back, report, name, extension, line, reason;
};

constexpr SaveSite open_error{0, 0x103e1288, 0x4d0, 0x128, 0x346, 0xe83};
constexpr SaveSite offer_slots{0x103e1334, 0x103e1354, 0x4b0, 0x120, 0x35a, 0xfb3};
constexpr SaveSite offer_count{0x103e1408, 0x103e1428, 0x490, 0x118, 0x36a, 0xff6};
constexpr SaveSite negotiation_slots{0x103e1518, 0x103e1538, 0x470, 0x110, 0x38a, 0x103d};
constexpr SaveSite negotiation_count{0x103e15a8, 0x103e15c8, 0x450, 0x108, 0x392, 0x1086};
constexpr SaveSite grouped_list_count{0x103e1698, 0x103e16b8, 0x430, 0x100, 0x3b3, 0x10d3};
constexpr SaveSite window_flag{0x103e177c, 0x103e179c, 0x410, 0xf8, 0x3c4, 0x1110};
constexpr SaveSite offer_date{0x103e180c, 0x103e182c, 0x3f0, 0xf0, 0x3cc, 0x1159};
constexpr SaveSite offer_date_flag{0x103e189c, 0x103e18bc, 0x3d0, 0xe8, 0x3d4, 0x1197};
constexpr SaveSite next_reference{0x103e1938, 0x103e1958, 0x3b0, 0xe0, 0x3df, 0x11d5};
constexpr SaveSite history_count{0x103e19c8, 0x103e19e8, 0x390, 0xd8, 0x3e7, 0x121b};
constexpr SaveSite history_rows{0x103e1a58, 0x103e1a7c, 0x370, 0xd0, 0x3f0, 0x1266};
constexpr SaveSite future_count{0x103e1aec, 0x103e1b0c, 0x350, 0xc8, 0x3f8, 0x12a7};
constexpr SaveSite future_rows{0x103e1b7c, 0x103e1ba0, 0x330, 0xc0, 0x401, 0x12f4};
constexpr SaveSite staff_status{0x103e1c10, 0x103e1c34, 0x310, 0xb8, 0x409, 0x1337};
constexpr SaveSite installment_count{0x103e1cac, 0x103e1ccc, 0x2f0, 0xb0, 0x412, 0x1374};
constexpr SaveSite installment_row{0x103e1d48, 0x103e1d68, 0x2d0, 0xa8, 0x41f, 0x13c5};
constexpr SaveSite sell_on_count{0x103e1dec, 0x103e1e0c, 0x2b0, 0xa0, 0x428, 0x140b};
constexpr SaveSite sell_on_rows{0x103e1e7c, 0x103e1ea0, 0x290, 0x98, 0x431, 0x145d};
constexpr SaveSite appearance_count{0x103e1f10, 0x103e1f30, 0x270, 0x90, 0x439, 0x14a5};
constexpr SaveSite appearance_rows{0x103e1fa0, 0x103e1fc4, 0x250, 0x88, 0x442, 0x14f1};
constexpr SaveSite international_count{0x103e2034, 0x103e2054, 0x230, 0x80, 0x44a, 0x1533};
constexpr SaveSite international_rows{0x103e20c4, 0x103e20e8, 0x210, 0x78, 0x453, 0x1587};
constexpr SaveSite club_status{0x103e2158, 0x103e217c, 0x1f0, 0x70, 0x45b, 0x15d1};
constexpr SaveSite activity_count{0x103e21f4, 0x103e2214, 0x1d0, 0x68, 0x464, 0x1613};
constexpr SaveSite activity_row{0x103e228c, 0x103e22ac, 0x1b0, 0x60, 0x470, 0x165b};
constexpr SaveSite regional_count{0x103e2330, 0x103e2350, 0x190, 0x58, 0x479, 0x16a5};
constexpr SaveSite regional_rows{0x103e23c0, 0x103e23e4, 0x170, 0x50, 0x482, 0x16ee};
constexpr SaveSite pair_count{0x103e2470, 0x103e2490, 0x150, 0x48, 0x48b, 0x172d};
constexpr SaveSite pair_row{0x103e2518, 0x103e2538, 0x130, 0x40, 0x49a, 0x1765};

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

struct Writer {
    Cpu& c;
    const Frame& frame;
    std::uint32_t pool, message, title, body;

    std::uint32_t stream_slot() const { return frame.at(0x3c); }
    std::uint32_t count_slot() const { return frame.at(0x38); }
    std::uint32_t stream() const { return c.u32(stream_slot()); }
    void close(std::uint32_t back) { call(c, close_file, back, {stream_slot()}); }

    void report(const SaveSite& site) {
        const auto name = frame.at(site.name), extension = frame.at(site.extension);
        const auto back = site.report;
        call(c, 0x10739890, back, {pool + 0x37, 0, 0, name, extension});
        const auto version = call(c, 0x10768810, back + 4);
        call(c, 0x1077b720, back + 0x24, {message, pool + 0x4c, version, name, extension, site.line, pool + site.reason});
        call(c, 0x10727890, back + 0x30, {title, pool + 0xebf});
        call(c, 0x10727890, back + 0x40, {body, pool + 0xecf, message});
        call(c, 0x10769b60, back + 0x50, {title, body, 0});
        if (site.back) close(back + 0x58);
    }

    bool write(const SaveSite& site, std::uint32_t data, std::uint32_t size, std::uint32_t count_address = 0) {
        const auto count = count_address ? c.u32(count_address) : 1;
        const auto written = call(c, write_file, site.back, {data, size, count, stream()});
        int comparison;
        if (count_address) {
            const auto expected = c.u32(count_address);
            c.set(0, expected);
            comparison = compare(c, expected, written);
        } else {
            comparison = compare(c, written, 1);
        }
        if (comparison == 0) return true;
        report(site);
        return false;
    }

    bool write_counted_array(std::uint32_t data, std::uint32_t count, std::uint32_t size,
                             const SaveSite& count_site, const SaveSite& rows_site) {
        return write(count_site, count, 4) && write(rows_site, c.u32(data), size, count);
    }

    bool write_list(std::uint32_t list, std::uint32_t size, std::uint32_t iterator_back,
                    const SaveSite& count_site, const SaveSite& row_site) {
        const auto length = c.i16(list);
        c.set(0, std::uint32_t(length));
        c.put16(count_slot(), std::uint32_t(length));
        if (!write(count_site, count_slot(), 2)) return false;
        return write_list_rows(list, size, iterator_back, row_site);
    }

    bool write_list_rows(std::uint32_t list, std::uint32_t size, std::uint32_t iterator_back, const SaveSite& site) {
        const auto head = c.u32(list + 4);
        c.set(0, head);
        c.put32(list + 0xc, head);
        for (;;) {
            const auto row = call(c, next_list_row, iterator_back, {list});
            if (!nonzero_logical(c, row)) return true;
            if (!write(site, row, size)) return false;
        }
    }
};

void trim_slots(Cpu& c, std::uint32_t table) {
    auto count = c.u32(table + 4);
    auto last = count - 1;
    c.s.wr<1>(reg::xer_ca, count != 0);
    compare(c, last);
    c.set(0, count);
    c.s.wr<4>(reg::ctr, count);
    if (std::int32_t(last) < 0) return;
    do {
        const auto slots = c.u32(table);
        const auto value = c.u32(slots + last * 4);
        c.set({{0, value}, {3, slots}});
        if (nonzero_logical(c, value)) {
            c.set(0, last + 1);
            c.put32(table + 4, last + 1);
            return;
        }
        --last;
        c.s.wr<4>(reg::ctr, --count);
    } while (count);
}

void count_offers(Cpu& c, std::uint32_t table) {
    c.put32(table + 8, 0);
    for (std::int32_t index = 0; ; ++index) {
        const auto limit = c.u32(table + 4);
        c.set(0, limit);
        if (compare(c, index, limit) >= 0) return;
        const auto slots = c.u32(table);
        if (nonzero_logical(c, c.u32(slots + std::uint32_t(index) * 4)))
            c.put32(table + 8, c.u32(table + 8) + 1);
    }
}

void write_offers(Writer& writer, std::uint32_t table, std::uint32_t back) {
    auto& c = writer.c;
    Kept<22> index(c, 0);
    Kept<24> offset(c, 0);
    for (;; index = index + 1, offset = offset + 4) {
        const auto limit = c.u32(table + 4);
        c.set(0, limit);
        if (compare(c, index, limit) >= 0) return;
        const auto offer = c.u32(c.u32(table) + offset);
        c.set(3, offer);
        if (nonzero_logical(c, offer)) call(c, 0x104123e0, back, {offer, writer.stream()});
    }
}

bool write_grouped_lists(Writer& writer, std::uint32_t manager) {
    auto& c = writer.c;
    Kept<22> group(c, 0);
    Kept<23> group_offset(c, 0);
    do {
        Kept<20> position(c, 0);
        Kept<24> list_offset(c, 0);
        for (;;) {
            const auto index = std::int8_t(position);
            c.set(0, std::uint32_t(index));
            if (compare(c, index, 3) >= 0) break;
            const Kept<21> list(c, manager + 0x18 + group_offset + list_offset);
            const auto length = c.i16(list);
            c.set(0, std::uint32_t(length));
            c.put16(writer.count_slot(), std::uint32_t(length));
            if (!writer.write(grouped_list_count, writer.count_slot(), 2)) return false;
            const auto head = c.u32(list + 4);
            c.set(0, head);
            c.put32(list + 0xc, head);
            for (;;) {
                const auto row = call(c, next_list_row, 0x103e173c, {list});
                if (!nonzero_logical(c, row)) break;
                // The original does not check the return for these four-byte items.
                call(c, write_file, 0x103e1734, {row, 4, 1, writer.stream()});
            }
            position = position + 1;
            list_offset = list_offset + 16;
        }
        group = group + 1;
        group_offset = group_offset + 0x30;
    } while (compare(c, group, 50) < 0);
    return true;
}
}

void save_transfer_manager(Cpu& c, std::uint32_t manager, std::uint32_t archive) {
    Frame frame(c, 0x103e1220, 0x520, {.first_gpr = 20});
    c.set(0, c.lr());
    const Kept<27> transfers(c, manager);
    const Kept<25> clubs(c, c.toc(-0x7b50));
    const Kept<26> staff(c, c.toc(-0x7be4));
    const Kept<28> body(c, c.toc(-0x79e0));
    const Kept<29> message(c, c.toc(-0x7ba8));
    const Kept<30> title(c, c.toc(-0x79dc));
    const Kept<31> pool(c, c.toc(-0x4b9c));
    Writer writer{c, frame, pool, message, title, body};
    const auto stream = call(c, 0x107475b0, 0x103e1264, {archive, pool + 0x257, 1, 22, 4});
    const bool opened = nonzero_logical(c, stream);
    c.put32(writer.stream_slot(), stream);
    if (!opened) {
        writer.report(open_error);
        return;
    }
    trim_slots(c, transfers);
    if (!writer.write(offer_slots, transfers + 4, 4)) return;
    count_offers(c, transfers);
    if (!writer.write(offer_count, transfers + 8, 4)) return;
    write_offers(writer, transfers, 0x103e14ac);
    trim_slots(c, transfers + 0xc);
    if (!writer.write(negotiation_slots, transfers + 0x10, 4) ||
        !writer.write(negotiation_count, transfers + 0x14, 4)) return;
    write_offers(writer, transfers + 0xc, 0x103e1648);
    if (!write_grouped_lists(writer, transfers) ||
        !writer.write(window_flag, transfers + 0x978, 1) ||
        !writer.write(offer_date, transfers + 0x97c, 8) ||
        !writer.write(offer_date_flag, transfers + 0x984, 1)) return;

    call(c, 0x10358b30, 0x103e1924, {c.u32(transfers + 0x988), writer.stream()});
    if (!writer.write(next_reference, transfers + 0x98c, 4) ||
        !writer.write_counted_array(transfers + 0x994, transfers + 0x99c, 40, history_count, history_rows) ||
        !writer.write_counted_array(transfers + 0xa50, transfers + 0xa58, 12, future_count, future_rows) ||
        !writer.write(staff_status, c.u32(transfers + 0x9a0), 10, staff) ||
        !writer.write_list(transfers + 0x9a4, 16, 0x103e1dd0, installment_count, installment_row) ||
        !writer.write_counted_array(transfers + 0x9b4, transfers + 0x9bc, 16, sell_on_count, sell_on_rows) ||
        !writer.write_counted_array(transfers + 0x9c0, transfers + 0x9c8, 16, appearance_count, appearance_rows) ||
        !writer.write_counted_array(transfers + 0x9cc, transfers + 0x9d4, 16, international_count, international_rows) ||
        !writer.write(club_status, c.u32(transfers + 0xa4c), 16, clubs) ||
        !writer.write_list(transfers + 0xa5c, 12, 0x103e2314, activity_count, activity_row) ||
        !writer.write_counted_array(transfers + 0xa6c, transfers + 0xa70, 16, regional_count, regional_rows)) return;

    const auto pairs = c.u32(transfers + 0xa78);
    c.set(3, pairs);
    const auto length = nonzero_logical(c, pairs) ? c.i16(pairs) : 0;
    c.set(0, std::uint32_t(length));
    c.put16(writer.count_slot(), std::uint32_t(length));
    if (!writer.write(pair_count, writer.count_slot(), 2)) return;
    const auto saved_pairs = c.u32(transfers + 0xa78);
    c.set(3, saved_pairs);
    if (nonzero_logical(c, saved_pairs) && !writer.write_list_rows(saved_pairs, 100, 0x103e25a0, pair_row)) return;
    writer.close(0x103e25b0);
}
}

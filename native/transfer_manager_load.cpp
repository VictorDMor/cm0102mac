#include "transfer_manager_load.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

constexpr std::uint32_t open_file = 0x107475b0, read_file = 0x10747a80, close_file = 0x10747900,
    allocate = 0x10775130, allocate_zeroed = 0x10775200, allocate_object = 0x10773520, append_list = 0x10738d10;
enum class ErrorText { load, allocation, assertion };
struct LoadSite {
    std::uint32_t back, report, name, extension, line, reason;
    ErrorText text = ErrorText::load;
    std::uint32_t error_result = 0;
};
constexpr LoadSite open_error{0, 0x103df818, 0x6a8, 0x184, 0x163, 0x264};
constexpr LoadSite offer_slots{0x103df884, 0x103df8a4, 0x688, 0x17c, 0x16a, 0x35d};
constexpr LoadSite offer_count{0x103df918, 0x103df938, 0x668, 0x174, 0x172, 0x3a1};
constexpr LoadSite offer_array{0x103df9ac, 0x103df9d4, 0x648, 0x16c, 0x17e, 0x3e9, ErrorText::allocation};
constexpr LoadSite offer_object{0x103dfa48, 0x103dfa74, 0x628, 0x164, 0x18b, 0x42c, ErrorText::allocation};
constexpr LoadSite negotiation_slots{0x103dfb0c, 0x103dfb2c, 0x608, 0x15c, 0x198, 0x47e};
constexpr LoadSite negotiation_count{0x103dfba0, 0x103dfbc0, 0x5e8, 0x154, 0x1a0, 0x4c8};
constexpr LoadSite negotiation_array{0x103dfc34, 0x103dfc5c, 0x5c8, 0x14c, 0x1ac, 0x516, ErrorText::allocation};
constexpr LoadSite negotiation_object{0x103dfccc, 0x103dfcf8, 0x5a8, 0x144, 0x1b9, 0x55f, ErrorText::allocation};
constexpr LoadSite grouped_list_count{0x103dfdc0, 0x103dfde0, 0x588, 0x13c, 0x1d5, 0x5b0};
constexpr LoadSite window_flag{0x103dfeb8, 0x103dfed8, 0x568, 0x134, 0x1e9, 0x5ef};
constexpr LoadSite offer_date{0x103dff4c, 0x103dff6c, 0x548, 0x12c, 0x1f1, 0x63a};
constexpr LoadSite offer_date_flag{0x103dffe0, 0x103e0000, 0x528, 0x124, 0x1f9, 0x67a};
constexpr LoadSite shortlist_object{0x103e0068, 0x103e00a0, 0x508, 0x11c, 0x202, 0x6ba};
constexpr LoadSite next_reference{0x103e0114, 0x103e0134, 0x4e8, 0x114, 0x20a, 0, ErrorText::assertion};
constexpr LoadSite history_count{0x103e0194, 0x103e01b4, 0x4c8, 0x10c, 0x212, 0x712};
constexpr LoadSite history_array{0x103e0230, 0x103e0258, 0x4a8, 0x104, 0x21e, 0x75f, ErrorText::allocation};
constexpr LoadSite history_rows{0x103e02c8, 0x103e02ec, 0x488, 0xfc, 0x227, 0x7a4};
constexpr LoadSite future_count{0x103e0360, 0x103e0380, 0x468, 0xf4, 0x230, 0x7e7};
constexpr LoadSite future_array{0x103e03fc, 0x103e0424, 0x448, 0xec, 0x23c, 0x836, ErrorText::allocation};
constexpr LoadSite future_rows{0x103e0494, 0x103e04b8, 0x428, 0xe4, 0x245, 0x87d};
constexpr LoadSite staff_status{0x103e052c, 0x103e0550, 0x408, 0xdc, 0x24e, 0x8c2};
constexpr LoadSite installment_count{0x103e05c4, 0x103e05e4, 0x3e8, 0xd4, 0x256, 0x901};
constexpr LoadSite installment_row{0x103e0660, 0x103e0680, 0x3c8, 0xcc, 0x261, 0x951};
constexpr LoadSite sell_on_count{0x103e0718, 0x103e0738, 0x3a8, 0xc4, 0x26d, 0x999};
constexpr LoadSite sell_on_array{0x103e07b8, 0x103e07e0, 0x388, 0xbc, 0x27a, 0x9eb, ErrorText::allocation};
constexpr LoadSite sell_on_rows{0x103e0850, 0x103e0874, 0x368, 0xb4, 0x283, 0xa37};
constexpr LoadSite appearance_count{0x103e08e8, 0x103e0908, 0x348, 0xac, 0x28c, 0xa81};
constexpr LoadSite appearance_array{0x103e0988, 0x103e09b0, 0x328, 0xa4, 0x299, 0xacf, ErrorText::allocation};
constexpr LoadSite appearance_rows{0x103e0a20, 0x103e0a44, 0x308, 0x9c, 0x2a2, 0xb15};
constexpr LoadSite international_count{0x103e0ab8, 0x103e0ad8, 0x2e8, 0x94, 0x2ab, 0xb59};
constexpr LoadSite international_array{0x103e0b58, 0x103e0b80, 0x2c8, 0x8c, 0x2b8, 0xbad, ErrorText::allocation};
constexpr LoadSite international_rows{0x103e0bf0, 0x103e0c14, 0x2a8, 0x84, 0x2c1, 0xbfb};
constexpr LoadSite club_status{0x103e0c88, 0x103e0cac, 0x288, 0x7c, 0x2ca, 0xc47, ErrorText::load, 1};
constexpr LoadSite activity_count{0x103e0d20, 0x103e0d40, 0x268, 0x74, 0x2d2, 0xc8a, ErrorText::load, 1};
constexpr LoadSite activity_row{0x103e0e08, 0x103e0e28, 0x248, 0x6c, 0x2e6, 0xcd3, ErrorText::load, 1};
constexpr LoadSite regional_count{0x103e0ec0, 0x103e0ee0, 0x228, 0x64, 0x2f2, 0xd25};
constexpr LoadSite regional_array{0x103e0f60, 0x103e0f88, 0x208, 0x5c, 0x2ff, 0xd6e, ErrorText::allocation};
constexpr LoadSite regional_rows{0x103e0ff8, 0x103e101c, 0x1e8, 0x54, 0x308, 0xdb1};
constexpr LoadSite pair_count{0x103e1090, 0x103e10b0, 0x1c8, 0x4c, 0x311, 0xdf2};
constexpr LoadSite pair_row{0x103e1154, 0x103e1174, 0x1a8, 0x44, 0x320, 0xe46};

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

struct Reader {
    Cpu& c;
    const Frame& frame;
    std::uint32_t pool, message, title, body;
    std::uint32_t failure = 0;

    std::uint32_t stream_slot() const { return frame.at(0x40); }
    std::uint32_t count_slot() const { return frame.at(0x38); }
    std::uint32_t stream() const { return c.u32(stream_slot()); }
    void close(std::uint32_t back) { call(c, close_file, back, {stream_slot()}); }

    void report(const LoadSite& site, bool close_after = true) {
        failure = site.error_result;
        const auto name = frame.at(site.name), extension = frame.at(site.extension), back = site.report;
        call(c, 0x10739890, back, {pool + 0x37, 0, 0, name, extension});
        const auto version = call(c, 0x10768810, back + 4);
        if (site.text == ErrorText::assertion) {
            call(c, 0x1077b720, back + 0x20, {message, pool + 0x6ff, version, name, extension, site.line});
            call(c, 0x10769b60, back + 0x30, {pool + 0x70c, message, 0});
            const auto success = c.toc(-0x7bac);
            c.set({{0, 0}, {4, success}});
            c.put8(success, 0);
            close(back + 0x44);
            return;
        }
        const bool allocation_failure = site.text == ErrorText::allocation;
        call(c, 0x1077b720, back + 0x24, {message, pool + 0x4c, version, name, extension, site.line, pool + site.reason});
        call(c, 0x10727890, back + 0x30, {title, pool + (allocation_failure ? 0xa6 : 0x2a2)});
        call(c, 0x10727890, back + 0x40, {body, pool + (allocation_failure ? 0xbe : 0x2b2), message});
        call(c, 0x10769b60, back + 0x50, {title, body, 0});
        if (close_after) close(back + 0x58);
    }

    void read_unchecked(std::uint32_t data, std::uint32_t size, std::uint32_t back) {
        call(c, read_file, back, {data, size, 1, stream()});
    }

    bool read(const LoadSite& site, std::uint32_t data, std::uint32_t size, std::uint32_t count_address = 0) {
        const auto count = count_address ? c.u32(count_address) : 1;
        const auto bytes = call(c, read_file, site.back, {data, size, count, stream()});
        int comparison;
        if (count_address) {
            const auto expected = c.u32(count_address);
            c.set(0, expected);
            comparison = compare(c, expected, bytes);
        } else {
            comparison = compare(c, bytes, 1);
        }
        if (comparison == 0) return true;
        report(site);
        return false;
    }

    bool read_array(std::uint32_t array, std::uint32_t capacity, std::uint32_t count, std::uint32_t size,
                    const LoadSite& header, const LoadSite& storage, const LoadSite& rows) {
        if (!read(header, capacity, 4)) return false;
        c.put32(count, c.u32(capacity));
        const auto length = c.u32(capacity);
        c.set(0, length);
        if (!nonzero(c, length)) return true;
        if (size == 16) c.set(0, c.u32(count));
        const auto pointer = call(c, allocate, storage.back, {c.u32(count) * size});
        c.put32(array, pointer);
        const auto target = c.u32(array);
        c.set(3, target);
        if (nonzero_logical(c, target)) return read(rows, target, size, count);
        report(storage);
        return false;
    }
};

template <unsigned Index>
bool read_offers(Reader& reader, std::uint32_t manager, std::uint32_t table, Kept<18>& object,
                 const LoadSite& slot_count, const LoadSite& count, const LoadSite& array,
                 const LoadSite& allocate_offer, std::uint32_t constructor_back, std::uint32_t read_back) {
    auto& c = reader.c;
    if (!reader.read(slot_count, table + 4, 4) || !reader.read(count, table + 8, 4)) return false;
    const auto slots = c.u32(table + 4);
    c.set(3, slots);
    if (nonzero(c, slots)) {
        c.put32(table, call(c, allocate_zeroed, array.back, {slots, 4}));
        c.set(0, c.u32(table));
        if (!nonzero_logical(c, c.r(0))) {
            reader.report(array);
            return false;
        }
    }
    Kept<Index> index(c, 0);
    for (;; index = index + 1) {
        const auto length = c.u32(table + 8);
        c.set(0, length);
        if (compare(c, index, length) >= 0) return true;
        object = call(c, allocate_object, allocate_offer.back, {0x54});
        if (nonzero(c, object)) call(c, 0x10411e60, constructor_back, {object});
        if (!nonzero_logical(c, object)) {
            reader.report(allocate_offer, false);
            return true;
        }
        const auto offer_array = c.u32(manager);
        call(c, 0x10411fa0, read_back, {object, reader.stream(), Index == 21 ? offer_array : 0});
        const auto reference = c.u32(object + 0x28);
        const auto first = c.u32(manager);
        if constexpr (Index == 21) {
            c.set({{0, reference * 4}, {3, first}});
            c.put32(first + reference * 4, object);
        } else {
            const auto offers = c.u32(table);
            const auto primary = c.u32(first + reference * 4);
            const auto slot = c.u32(primary + 0x30) * 4;
            c.set({{0, slot}, {3, primary}, {4, offers}});
            c.put32(offers + slot, object);
        }
    }
}

bool read_grouped_lists(Reader& reader, std::uint32_t manager, Kept<18>& index) {
    auto& c = reader.c;
    Kept<21> group(c, 0);
    Kept<22> group_offset(c, 0);
    do {
        Kept<19> position(c, 0);
        Kept<23> offset(c, 0);
        for (;; position = position + 1, offset = offset + 16) {
            const auto field = std::int16_t(position);
            c.set(0, std::uint32_t(field));
            if (compare(c, field, 3) >= 0) break;
            const Kept<20> list(c, manager + 0x18 + group_offset + offset);
            if (!reader.read(grouped_list_count, reader.count_slot(), 2)) return false;
            for (index = 0; ; index = index + 1) {
                const auto length = c.i16(reader.count_slot()), row = std::int16_t(index);
                c.set({{0, std::uint32_t(length)}, {3, std::uint32_t(row)}});
                if (compare(c, row, length) >= 0) break;
                const auto value = reader.frame.at(0x3c);
                reader.read_unchecked(value, 4, 0x103dfe5c);
                call(c, append_list, 0x103dfe6c, {list, value, 4});
            }
        }
        group = group + 1;
        group_offset = group_offset + 0x30;
    } while (compare(c, group, 50) < 0);
    return true;
}

bool read_installments(Reader& reader, std::uint32_t manager) {
    auto& c = reader.c;
    if (!reader.read(installment_count, reader.count_slot(), 2)) return false;
    const auto row = reader.frame.at(0x198);
    Kept<22> index(c, 0);
    for (;; index = index + 1) {
        const auto length = c.i16(reader.count_slot());
        c.set(0, std::uint32_t(length));
        if (compare(c, index, length) >= 0) return true;
        if (!reader.read(installment_row, row, 16)) return false;
        call(c, append_list, 0x103e06f4, {manager + 0x9a4, row, 16});
    }
}

bool read_activity(Reader& reader, std::uint32_t manager, std::uint32_t version) {
    auto& c = reader.c;
    if (!reader.read(activity_count, reader.count_slot(), 2)) return false;
    const auto row = reader.frame.at(0x18c);
    c.set(3, 0x40000);
    Kept<22> index(c, 0);
    const Kept<23> modern_version(c, 0x3e003);
    for (;; index = index + 1) {
        const auto length = c.i16(reader.count_slot());
        c.set(0, std::uint32_t(length));
        if (compare(c, index, length) >= 0) return true;
        const auto saved_version = call(c, 0x10775550, 0x103e0db8, {c.u32(version)});
        if (compare(c, saved_version, modern_version) < 0) {
            reader.read_unchecked(row, 4, 0x103e0dd4);
            reader.read_unchecked(row + 4, 1, 0x103e0de8);
            c.set(0, 0xffffffff);
            c.put32(row + 8, 0xffffffff);
        } else if (!reader.read(activity_row, row, 12)) {
            return false;
        }
        call(c, append_list, 0x103e0e9c, {manager + 0xa5c, row, 12});
    }
}

bool read_pairs(Reader& reader, std::uint32_t manager, Kept<18>& object) {
    auto& c = reader.c;
    if (!reader.read(pair_count, reader.count_slot(), 2)) return false;
    const auto count = c.i16(reader.count_slot());
    c.set(0, std::uint32_t(count));
    if (nonzero(c, count)) {
        object = call(c, allocate_object, 0x103e1124, {16});
        if (nonzero(c, object)) call(c, 0x10738c00, 0x103e1130);
        c.put32(manager + 0xa78, object);
    }
    Kept<22> index(c, 0);
    const auto row = reader.frame.at(0x6c8);
    for (;; index = index + 1) {
        const auto length = c.i16(reader.count_slot());
        c.set(0, std::uint32_t(length));
        if (compare(c, index, length) >= 0) return true;
        if (!reader.read(pair_row, row, 100)) return false;
        call(c, append_list, 0x103e11e8, {c.u32(manager + 0xa78), row, 100});
    }
}
}

std::uint32_t load_transfer_manager(Cpu& c, std::uint32_t manager, std::uint32_t archive) {
    Frame frame(c, 0x103df790, 0x770, {.first_gpr = 18});
    c.set(0, c.lr());
    Kept<18> object(c, archive);
    const Kept<24> clubs(c, c.toc(-0x7b50));
    const Kept<25> staff(c, c.toc(-0x7be4));
    const Kept<26> version(c, c.toc(-0x7b74));
    const Kept<27> title(c, c.toc(-0x79dc));
    const Kept<28> body(c, c.toc(-0x79e0));
    const Kept<29> message(c, c.toc(-0x7ba8));
    const Kept<30> pool(c, c.toc(-0x4b9c));
    const Kept<31> transfers(c, manager);
    Reader reader{c, frame, pool, message, title, body};
    call(c, 0x103721f0, 0x103df7cc, {frame.at(0x6c8)});
    if (!nonzero_logical(c, object)) return result(c, 0);
    const auto stream = call(c, open_file, 0x103df7f4, {object, pool + 0x257, 0, 22, 4});
    const bool opened = nonzero_logical(c, stream);
    c.put32(reader.stream_slot(), stream);
    if (!opened) {
        reader.report(open_error, false);
        return result(c, 0);
    }
    if (!read_offers<21>(reader, transfers, transfers, object, offer_slots, offer_count, offer_array,
                         offer_object, 0x103dfa54, 0x103dfad8) ||
        !read_offers<23>(reader, transfers, transfers + 0xc, object, negotiation_slots, negotiation_count,
                         negotiation_array, negotiation_object, 0x103dfcd8, 0x103dfd5c) ||
        !read_grouped_lists(reader, transfers, object) ||
        !reader.read(window_flag, transfers + 0x978, 1) ||
        !reader.read(offer_date, transfers + 0x97c, 8) ||
        !reader.read(offer_date_flag, transfers + 0x984, 1)) return result(c, reader.failure);

    object = call(c, allocate_object, shortlist_object.back, {0xe0});
    if (nonzero(c, object)) call(c, 0x10356410, 0x103e0078, {object, reader.stream()});
    c.put32(transfers + 0x988, object);
    c.set(0, c.u32(transfers + 0x988));
    if (!nonzero_logical(c, c.r(0))) {
        reader.report(shortlist_object);
        return result(c, reader.failure);
    }
    if (!reader.read(next_reference, transfers + 0x98c, 4) ||
        !reader.read_array(transfers + 0x994, transfers + 0x998, transfers + 0x99c, 40,
                           history_count, history_array, history_rows) ||
        !reader.read_array(transfers + 0xa50, transfers + 0xa54, transfers + 0xa58, 12,
                           future_count, future_array, future_rows) ||
        !reader.read(staff_status, c.u32(transfers + 0x9a0), 10, staff) ||
        !read_installments(reader, transfers) ||
        !reader.read_array(transfers + 0x9b4, transfers + 0x9b8, transfers + 0x9bc, 16,
                           sell_on_count, sell_on_array, sell_on_rows) ||
        !reader.read_array(transfers + 0x9c0, transfers + 0x9c4, transfers + 0x9c8, 16,
                           appearance_count, appearance_array, appearance_rows) ||
        !reader.read_array(transfers + 0x9cc, transfers + 0x9d0, transfers + 0x9d4, 16,
                           international_count, international_array, international_rows) ||
        !reader.read(club_status, c.u32(transfers + 0xa4c), 16, clubs) ||
        !read_activity(reader, transfers, version) ||
        !reader.read_array(transfers + 0xa6c, transfers + 0xa74, transfers + 0xa70, 16,
                           regional_count, regional_array, regional_rows) ||
        !read_pairs(reader, transfers, object)) return result(c, reader.failure);
    reader.close(0x103e1200);
    return result(c, 1);
}
}

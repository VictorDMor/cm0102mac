// DATABASE.CPP's resolve block: 0x100af8a0 (resolve_database_indexes), see database_resolve.hpp and
// database_resolve_detail.hpp. The original is one long run of loops over the loaded record tables; each stored index
// of a record is turned into a pointer into another table by the same few steps, written once here and driven by the
// tables of links in the detail header.
// Registers: the original loads the TOC words it needs into r13-r30 at entry, and every call (the formatted-output
// routine 0x1077b720, the assertion macro) saves them, so they are set as it sets them (r15 is the inner loop index,
// r16 the record number, r31 the argument). r4-r7 are left as the last path leaves them: a callee's frame never sees
// r0 or r3 of the idioms, but the caller sees r4-r7 after the routine returns (a varargs call stores them).
#include "database_resolve.hpp"
#include "database_resolve_detail.hpp"
#include "guest_values.hpp"
#include <optional>
#include <span>

namespace cm::aot::database {
namespace {
using namespace resolve;
using guest::compare;
constexpr std::uint32_t entry = 0x100af8a0, formatted_output = 0x1077b720;
constexpr std::uint32_t frame_size = 0x100;
// TOC words that are loaded into registers once.
constexpr std::int32_t pool_slot = -0x5618, buffer_slot = -0x7bb8;

// How an array's indexes are range-checked: against the people count with a report, as the squad when its
// argument is 0 (a slack before the report), or not at all (the squad when its argument is nonzero).
enum class Check { reported, squad_lenient, none };

class Resolver {
public:
    Resolver(Cpu& cpu, guest::Frame& frame) : c(cpu), frame_(frame) {}

    // The loop over a table's records: the count is read again at each test, as the original does.
    template <class Body>
    void each_record(const Table& rows, Body body) {
        for (std::uint32_t record = 0;; ++record) {
            const auto count = counted(rows);
            compare(c, std::int32_t(record), count);
            if (std::int32_t(record) >= count) return;
            c.set(16, record);
            body(record);
        }
    }
    void links(const Table& rows, std::uint32_t record, std::span<const Link> list) {
        for (const auto& link : list) resolve_link(rows, record, link);
    }
    void resolve_link(const Table& rows, std::uint32_t number, const Link& link);
    void person_array(const Table& rows, std::uint32_t number, const PersonArray& array, Check check);

private:
    std::uint32_t record_at(const Table& rows, std::uint32_t number) const {
        return c.u32(c.toc(rows.base_slot)) + number * rows.stride;  // 32-bit wrap, as mulli/add
    }
    std::int32_t counted(const Table& table) const { return c.i32(c.toc(table.count_slot)); }
    // The index becomes the target's first record plus index * record size; r4 and r5 keep the two parts.
    void point(const Table& target, std::uint32_t at, std::uint32_t index) {
        const auto first = c.u32(c.toc(target.base_slot)), offset = index * target.stride;
        c.set(4, offset);
        c.set(5, first);
        c.put32(at, first + offset);
    }
    // The formatted-output line "...invalid <thing> %ld" (or with the record number first); r3 is the shared
    // message buffer, the string comes from the pool.
    void say(std::uint32_t call_at, std::uint32_t format, std::optional<std::uint32_t> number, std::uint32_t value) {
        c.set(3, c.r(23));
        c.set(4, c.r(25) + format);
        if (number) {
            c.set(5, *number);
            c.set(6, value);
        } else {
            c.set(5, value);
        }
        c.call(formatted_output, call_at + 4);
    }
    void assert_invalid(const Link& link, std::uint32_t number, std::uint32_t value) {
        const auto& site = assertion_sites[link.assertion];
        say(site.formatted_call, site.format, number, value);
        guest::report_assertion(c, {.at = site.at, .file = c.r(25), .format = c.r(25) + assertion_format,
                                    .title = c.r(25) + assertion_title, .name = frame_.at(std::int32_t(site.name)),
                                    .extension = frame_.at(std::int32_t(site.extension)), .line = site.line});
    }

    Cpu& c;
    guest::Frame& frame_;
};

void Resolver::resolve_link(const Table& rows, std::uint32_t number, const Link& link) {
    const auto row_base = c.u32(c.toc(rows.base_slot));
    const auto record = record_at(rows, number);
    const auto at = record + link.field;
    const auto value = c.i32(at);
    const auto clear = [&](std::uint32_t field) { c.put32(record + field, 0); };
    const auto cleared = link.cleared ? link.cleared : link.field;
    const auto negative = [&] {
        c.set(4, 0);
        clear(link.field);
    };
    if (link.bound == Bound::unbounded) {
        if (value < 0) return negative();
        return point(link.target, at, std::uint32_t(value));
    }
    const auto limit = counted(link.target);
    const bool inclusive = link.bound == Bound::inclusive;
    const auto beyond = [&] { return inclusive ? value > limit : value >= limit; };
    const auto invalid = [&] {
        compare(c, value, limit);  // cr0 of the compare before the call
        switch (link.report) {
        case Report::silent:
            c.set(4, 0);
            break;
        case Report::text:
        case Report::text_indexed:
            say(link.call_at, link.format, link.report == Report::text ? std::nullopt : std::optional(number),
                std::uint32_t(value));
            c.set(4, 0);
            break;
        case Report::assertion:
            assert_invalid(link, number, std::uint32_t(value));
            c.set(4, row_base);
            c.set(5, 0);
            break;
        }
        clear(cleared);
    };
    if (link.bound_first) {
        if (beyond()) return invalid();
        if (value < 0) return negative();
        return point(link.target, at, std::uint32_t(value));
    }
    if (value < 0) return negative();
    if (link.leaves_row_base) c.set(5, row_base);
    if (beyond()) return invalid();
    point(link.target, at, std::uint32_t(value));
}

// An inline array of person indexes of a club record. Valid indexes point into the people table; the registers
// afterwards are those of the array's own idiom (r4 the club record, r5 the club table, r6 the offset, r7 the club table).
void Resolver::person_array(const Table& rows, std::uint32_t number, const PersonArray& array, Check check) {
    const auto row_base = c.u32(c.toc(rows.base_slot));
    const auto record = record_at(rows, number);
    for (unsigned element = 0; element < array.count; ++element) {
        c.set(15, element);
        const auto at = record + array.first + element * 4;
        const auto value = c.i32(at);
        const auto people_count = counted(people);  // read again for every element, as the original does
        const auto zero = [&](std::uint32_t r4) {
            c.set({{4, r4}, {5, 0}});
            c.put32(at, 0);
        };
        const auto point_at_person = [&] {
            const auto offset = std::uint32_t(value) * people.stride;
            c.set({{4, record}, {5, row_base}, {6, offset}, {7, row_base}});
            c.put32(at, c.u32(c.toc(people.base_slot)) + offset);
        };
        const auto resolve_normally = [&] {
            if (value < 0) return zero(row_base);
            point_at_person();
        };
        if (check == Check::none || value < people_count) {
            resolve_normally();
            continue;
        }
        if (check == Check::squad_lenient) {
            // Indexes just past the count are cleared silently (the original adds a slack of 0x52d, which wraps for
            // a huge count). The last compare before the report or the silent clear is against that limit.
            const auto slack_limit = std::int32_t(std::uint32_t(people_count) + std::uint32_t(squad_slack));
            compare(c, value, slack_limit);
            if (value <= slack_limit) {
                zero(row_base);
                continue;
            }
        } else {
            compare(c, value, people_count);
        }
        say(array.call_at, array.format, number, std::uint32_t(value));
        zero(row_base);
    }
    c.set(15, array.count);
}
}

void resolve_database_indexes(Cpu& c, std::uint32_t skip_squad_check) {
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, frame_size, {.first_gpr = 13});
    // The TOC words the original keeps in registers (the others are re-read where used).
    c.set({{14, c.toc(-0x7564)}, {17, c.toc(-0x7548)}, {18, c.toc(-0x7544)}, {19, c.toc(-0x7bbc)},
           {20, c.toc(-0x753c)}, {21, c.toc(-0x7ab4)}, {22, c.toc(-0x7a34)}, {23, c.toc(buffer_slot)},
           {24, c.toc(-0x7b98)}, {25, c.toc(pool_slot)}, {26, c.toc(-0x7be0)}, {13, c.toc(-0x74c4)},
           {27, c.toc(-0x7be8)}, {28, c.toc(-0x7be4)}, {29, c.toc(-0x74bc)}, {30, c.toc(-0x79f8)}});
    c.set(31, skip_squad_check);
    c.set(16, 0);
    Resolver resolver(c, frame);

    // Clubs: their stored indexes, the squad and the other person arrays, and the single person fields.
    resolver.each_record(clubs, [&](std::uint32_t club) {
        resolver.links(clubs, club, club_links);
        // extsb.: only the argument's low byte counts
        resolver.person_array(clubs, club, squad, std::int8_t(c.r(31)) == 0 ? Check::squad_lenient : Check::none);
        for (const auto& array : person_arrays) resolver.person_array(clubs, club, array, Check::reported);
        for (const auto& link : club_person_links) resolver.resolve_link(clubs, club, link);
        resolver.person_array(clubs, club, last_array, Check::reported);
    });
    resolver.each_record(table_128, [&](std::uint32_t record) { resolver.links(table_128, record, table_128_links); });
    resolver.each_record(table_2c, [&](std::uint32_t record) { resolver.links(table_2c, record, table_2c_links); });
    resolver.each_record(table_50, [&](std::uint32_t record) { resolver.links(table_50, record, table_50_links); });
    resolver.each_record(table_34, [&](std::uint32_t record) { resolver.links(table_34, record, table_34_links); });
    resolver.each_record(table_44, [&](std::uint32_t record) { resolver.links(table_44, record, table_44_links); });
    resolver.each_record(people, [&](std::uint32_t record) { resolver.links(people, record, person_links); });
    resolver.each_record(table_68, [&](std::uint32_t record) { resolver.links(table_68, record, table_68_links); });
    resolver.each_record(table_14, [&](std::uint32_t record) { resolver.links(table_14, record, table_14_links); });
    resolver.each_record(table_38, [&](std::uint32_t record) { resolver.links(table_38, record, table_38_links); });
    resolver.each_record(table_1c, [&](std::uint32_t record) { resolver.links(table_1c, record, table_1c_links); });
    resolver.each_record(table_6c, [&](std::uint32_t record) { resolver.links(table_6c, record, table_6c_links); });
    c.set(3, c.toc(table_6c.count_slot));  // the last loop test's lwz r3
}
}

// cm-database-checks fixtures for DATABASE.CPP's resolve block: 0x100af8a0 (resolve_database_indexes).
// Small worlds of every record table (at most three rows each, all zero but the fields a case sets), every stored index
// of every record set in turn to values on both sides of each bound (negative, 0, below, at and past the target's
// count, 0x7fffffff), the squad and the other person arrays with every argument low byte, and loops that do not run
// (a count of 0 or negative) so that the registers a later loop would overwrite are seen. The formatted-output
// routine and the assertion macro are stubbed (their calls are recorded with the registers and the stack image they
// save), and each stub leaves scratch registers different on every call.
#include "database_checks.hpp"
#include "database_resolve_detail.hpp"
#include <array>
#include <limits>
#include <span>

namespace cm::aot::database_checks {
namespace {
using namespace database::resolve;
constexpr std::uint32_t entry = 0x100af8a0;
constexpr std::uint32_t tables_at = data + 0x2000, table_room = 0x1000, words_at = data + 0x100,
                        pool_at = data + 0x1c000, buffer_at = data + 0x1d000, buffer_cell = data + 0x1d800,
                        flag_at = data + 0x1e000;
constexpr std::array<const Table*, 18> all_tables{
    &clubs,    &people,   &table_128, &table_6c,  &table_50,   &table_3c,   &table_38,   &table_c8,   &table_48,
    &table_34, &table_44, &table_2c,  &table_68,  &table_14,   &table_1c,   &table_2c_a, &table_2c_b, &table_2c_c};
constexpr std::uint32_t formatted_output = 0x1077b720, assertion_calls[] = {0x10739890, 0x10768810, 0x10769b60};

// A loop of the routine: its record table and the links it resolves (the club loop also has arrays).
struct Loop {
    const Table& rows;
    std::span<const Link> links;
};
const std::array<Loop, 12>& loops() {
    static const std::array<Loop, 12> found{{{clubs, club_links},       {table_128, table_128_links}, {table_2c, table_2c_links},
                                              {table_50, table_50_links}, {table_34, table_34_links},   {table_44, table_44_links},
                                              {people, person_links},     {table_68, table_68_links},   {table_14, table_14_links},
                                              {table_38, table_38_links}, {table_1c, table_1c_links},   {table_6c, table_6c_links}}};
    return found;
}

struct Case {
    std::array<std::int32_t, 18> counts{};
    // The index under test: a row table (index into all_tables), a field, and the value written to every row.
    int table = -1;
    std::uint32_t field = 0;
    std::uint32_t value = 0;
    std::uint32_t argument = 0;
    bool summary_overflow = false;
    // The first report empties the people table (a huge count would otherwise keep the later loops running).
    bool drain_people = false;
    std::string text;
};
unsigned index_of(const Table& table) {
    for (unsigned i = 0; i < all_tables.size(); ++i)
        if (all_tables[i] == &table) return i;
    return 0;
}

void put_toc(Cpu& c, std::int32_t offset, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(offset), value); }

void build(Cpu& c, const Case& t) {
    for (unsigned i = 0; i < all_tables.size(); ++i) {
        const auto& table = *all_tables[i];
        const auto array = tables_at + i * table_room, base_cell = words_at + i * 8, count_cell = base_cell + 4;
        put_toc(c, table.base_slot, base_cell);
        c.put32(base_cell, array);
        if (table.count_slot) {
            put_toc(c, table.count_slot, count_cell);
            c.put32(count_cell, std::uint32_t(t.counts[i]));
        }
        for (std::uint32_t at = 0; at < table_room; at += 4) c.put32(array + at, 0);
    }
    put_toc(c, -0x5618, pool_at);
    put_toc(c, -0x7bb8, buffer_at);
    put_toc(c, -0x7ba8, buffer_cell);
    c.put32(buffer_cell, buffer_at);
    put_toc(c, -0x7bac, flag_at);
    c.put8(flag_at, 1);
    c.set(3, t.argument);
    if (t.table >= 0) {
        const auto& table = *all_tables[unsigned(t.table)];
        const auto array = tables_at + unsigned(t.table) * table_room;
        for (std::uint32_t row = 0; row < 3; ++row) c.put32(array + row * table.stride + t.field, t.value);
    }
}

// record_call without r0: every callee here starts with mfspr r0,LR, so the caller's r0 (a scratch of the idioms)
// reaches nothing.
void record_arguments(Cpu& c) {
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    const auto add = [&](std::uint32_t value) {
        c.put32(at, value);
        at += 4;
    };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 3; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned f = 0; f < 8; ++f) add(c.cr(f));
    c.put32(trace_at, (at - trace_at - 4) / 4);
}

// The formatted output and the assertion macro, recorded and answered with scratch registers that differ per call.
bool stub(State& s, const Case& t) {
    if (s.pc != formatted_output && s.pc != assertion_calls[0] && s.pc != assertion_calls[1] && s.pc != assertion_calls[2])
        return false;
    Cpu c(s);
    const auto calls = c.u32(trace_at);
    record_arguments(c);
    if (t.drain_people && s.pc == formatted_output) c.put32(words_at + index_of(people) * 8 + 4, 0);
    c.set(3, 0x31000000 + calls);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0x41000000 + calls * 0x100 + r);
    c.compare(0, 9, std::int32_t(calls));
    s.pc = c.lr();
    return true;
}

Fixture make(const Case& t) {
    Fixture f;
    f.entry = entry;
    f.description = t.text + " arg=" + hex_address(t.argument);
    f.setup = [t](Cpu& c) { build(c, t); };
    f.stub = [t](State& s) { return stub(s, t); };
    f.summary_overflow = t.summary_overflow;
    f.results = 0;
    return f;
}

std::string name(const Table& table) { return "table@" + std::to_string(table.base_slot); }

// Every table its own count (2 + its position), so a link that reads another table's count slot shows.
std::array<std::int32_t, 18> distinct_counts() {
    std::array<std::int32_t, 18> counts{};
    for (unsigned i = 0; i < 15; ++i) counts[i] = 2 + std::int32_t(i);  // the last three: targets only, never read
    return counts;
}

std::array<std::int32_t, 18> counts_of(std::int32_t count) {
    std::array<std::int32_t, 18> counts{};
    counts.fill(count);
    for (unsigned i = 15; i < 18; ++i) counts[i] = 0;  // targets only: their counts are never read
    return counts;
}

constexpr std::array<std::uint32_t, 10> values{0x80000000, 0xffffffff, 0, 1, 2, 3, 4, 5, 0x7fffffff, 0x1000};
}

std::vector<Fixture> resolve_fixtures() {
    std::vector<Fixture> found;
    const auto add = [&](const Case& t) { found.push_back(make(t)); };
    // Nothing to do, loops that do not run (negative counts included), and one of each with no links invalid.
    for (const std::int32_t count : {0, -1, std::numeric_limits<std::int32_t>::min()})
        for (const bool overflow : {false, true}) {
            Case t;
            t.counts = counts_of(count);
            t.summary_overflow = overflow;
            t.text = "counts " + std::to_string(count);
            add(t);
        }
    {
        Case t;
        t.counts = distinct_counts();
        t.text = "all zero";
        for (const std::uint32_t argument : {0u, 1u, 0x80u})
            for (const bool overflow : {false, true}) {
                t.argument = argument;
                t.summary_overflow = overflow;
                add(t);
            }
    }
    // Every stored index of every loop at every value, with all tables holding three records.
    for (const auto& loop : loops()) {
        const auto row_table = int(index_of(loop.rows));
        const auto each = [&](std::uint32_t field, const std::string& what, const Table* target = nullptr) {
            // The fixed values, and (for a link) the values around its own target's count.
            std::vector<std::uint32_t> swept(values.begin(), values.end());
            if (target) {
                const auto n = std::uint32_t(distinct_counts()[index_of(*target)]);
                for (const auto v : {n - 1, n, n + 1}) swept.push_back(v);
            }
            for (const auto value : swept) {
                Case t;
                t.counts = distinct_counts();
                t.table = row_table;
                t.field = field;
                t.value = value;
                t.summary_overflow = (value & 1) != 0;
                t.text = name(loop.rows) + " " + what + " +" + hex_address(field) + " = " + hex_address(value);
                add(t);
            }
        };
        for (const auto& link : loop.links) each(link.field, "link", &link.target);
        if (&loop.rows == &clubs) {
            for (const auto& link : club_person_links) each(link.field, "person link", &link.target);
            each(squad.first, "squad");
            for (const auto& array : person_arrays) each(array.first + 4 * (array.count - 1), "array");
            each(last_array.first + 4 * (last_array.count - 1), "array");
        }
    }
    // The squad and the arrays: every element position, around the people count and the squad's slack, with every
    // argument whose low byte is 0 or not (the original sign-extends the byte).
    {
        const auto people_slot = int(index_of(people));
        std::vector<PersonArray> arrays{squad, last_array};
        arrays.insert(arrays.end(), person_arrays.begin(), person_arrays.end());
        for (const auto& array : arrays)
            for (const unsigned element : {0u, 1u, array.count / 2, array.count - 1})
                for (const std::int32_t people_count : {0, 4})
                    for (const std::uint32_t argument : {0u, 1u, 0x100u, 0x80u, 0xffffff00u, 0x7fu})
                        for (const std::int32_t value : {std::numeric_limits<std::int32_t>::min(), -1, 0, 3, 4, 5, 4 + 0x52d,
                                                         4 + 0x52e, 0x52d, 0x52e, std::numeric_limits<std::int32_t>::max()}) {
                            Case t;
                            t.counts = distinct_counts();
                            t.counts[unsigned(people_slot)] = people_count;
                            t.table = int(index_of(clubs));
                            t.field = array.first + 4 * element;
                            t.value = std::uint32_t(value);
                            t.argument = argument;
                            t.summary_overflow = element == 1 && value == 5;
                            t.text = "array +" + hex_address(array.first) + " element " + std::to_string(element) +
                                     " people " + std::to_string(people_count) + " value " + hex_address(std::uint32_t(value));
                            add(t);
                        }
    }
    // The squad's slack wraps for a huge people count (count + 0x52d is negative): reporting values only, since
    // every report empties the table. cr0 before the report is the compare with the wrapped limit.
    for (const std::int32_t people_count : {0x7ffffad3, 0x7ffffad2, 0x7fffffff})
        for (const unsigned element : {0u, 49u})
            for (const std::uint32_t argument : {0u, 0x100u})
                for (const std::int32_t value : {-1, 0, 3, 0x52e, std::numeric_limits<std::int32_t>::max(), people_count})
                    for (const bool overflow : {false, true}) {
                        Case t;
                        t.counts = distinct_counts();
                        t.counts[index_of(people)] = people_count;
                        t.drain_people = true;
                        t.table = int(index_of(clubs));
                        t.field = squad.first + 4 * element;
                        t.value = std::uint32_t(value);
                        t.argument = argument;
                        t.summary_overflow = overflow;
                        t.text = "squad, people " + hex_address(std::uint32_t(people_count)) + " element " +
                                 std::to_string(element) + " value " + hex_address(std::uint32_t(value));
                        add(t);
                    }
    // Only the loops up to one run, the rest hold a count of 0: the registers each loop's last record leaves.
    for (unsigned last = 0; last < loops().size(); ++last)
        for (const auto value : {0u, 1u, 0xffffffffu, 7u, 0x80000000u}) {
            Case t;
            t.counts = counts_of(0);
            for (unsigned i = 0; i <= last; ++i) t.counts[index_of(loops()[i].rows)] = 2;
            t.table = int(index_of(loops()[last].rows));
            for (const auto& link : loops()[last].links) {  // every link of the last loop at the same value
                Case u = t;
                u.field = link.field;
                u.value = value;
                u.text = name(loops()[last].rows) + " last loop link +" + hex_address(link.field) + " = " + hex_address(value);
                add(u);
            }
        }
    return found;
}
}

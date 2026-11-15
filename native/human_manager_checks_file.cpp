// HUMAN_MANAGER.CPP checker cases for the humans' file (human_manager_file.hpp).
//
// The file layer cannot run in a snapshot: every call of it is answered on both sides (Case::answers, and
// Case::nth_answers for the n-th read or write), and what a real read would have stored is poked into the
// buffers when a read is dispatched (Case::pokes), copied from the career's own humans. The reader builds a
// table in scratch memory: its allocation (0x107735f0, the first call) is answered with an address in scratch
// memory so the records' addresses are known; the record constructors, the allocator for the sorted entries
// and the formation routines run for real. The memory gauge (the word 0x10775550 is asked about) is written
// by each case to take the layout the reader chooses. The writer works on the career's table.
#include "human_manager_checks.hpp"
#include "guest_abi.hpp"
#include <climits>
#include <string>

namespace cm::aot::human_manager_checks {
namespace {
constexpr std::uint32_t read_humans = 0x1014a300, write_humans = 0x1014b0a0;
constexpr std::uint32_t open_file = 0x107475b0, read_block = 0x10747a80, write_block = 0x10747cc0, close_file = 0x10747900,
                        allocate = 0x107735f0, construct_array = 0x10773150, allocate_block = 0x10775130;
constexpr std::uint32_t record_size = 0xa65c, slot_size = 0x1eb4, image_size = 0x1e5c;
constexpr std::uint32_t scratch_table = scratch_base + 0x100, scratch_array = scratch_base + 0x20000, fake_file = 0x1234,
                        fake_handle = 0x5150;
constexpr unsigned fields_per_record = 20;

std::uint32_t absolute(std::uint32_t value) { return std::int32_t(value) < 0 ? 0u - value : value; }
bool below(std::uint32_t gauge, std::uint32_t limit) { return std::int32_t(absolute(gauge)) < std::int32_t(limit); }

struct Human {
    std::uint32_t option0 = 0x20000, option1 = 0x20000;  // the words at +0x588 of the first two slots
    std::uint32_t entries = 0;                           // the sorted entries the file holds
    bool linked = true;                                  // the record holds links to rebuild (+0x9a7c non-zero)
};
// Where each read of a run falls (counted from 1), for answering the n-th one.
struct Reads {
    unsigned count = 1, table_c = 0, table_8 = 0;
    std::vector<unsigned> entries_count, entries_data;
    unsigned total = 0;
    Reads(std::uint32_t gauge, const std::vector<Human>& humans) {
        unsigned n = 1;
        for (const auto& human : humans) {
            n += below(gauge, 0x3e021) ? 2 : 1;
            n += fields_per_record + 1;
            entries_count.push_back(++n);
            entries_data.push_back(human.entries ? ++n : 0);
        }
        table_8 = ++n;
        table_c = ++n;
        for (std::size_t i = 0; i < humans.size(); ++i) n += below(gauge, 0x3e021) ? 3 : 1;
        total = n;
    }
};
}

HUMAN_MANAGER_CASES("file") {
    if (scenario.match) return;
    using Writes = std::vector<Write>;
    const auto humans_table = scenario.global(-0x7a54);
    const auto real_records = scenario.r32(humans_table), real_count = scenario.r32(humans_table + 4);
    const auto real_record = [&](std::uint32_t index) { return real_records + (index % real_count) * record_size; };
    const auto gauge_cell = scenario.toc(-0x7b74);
    const auto buffer_base = scenario.toc(-0x73f4);  // the older layouts' 0x1e5c-byte images
    const auto stack_pointer = scenario.s.r<4>(guest::reg::gpr(1));
    const auto list_buffer = stack_pointer - 0x240 + 0x1a4;  // the older list's sixteen words

    // What the reads store, as a real file of these humans would: the tactic words the reader looks at, the
    // links, the count of entries, a list. `humans` are the records the reader builds (at scratch_array + 0x10).
    const auto file_contents = [&](const std::vector<Human>& humans) {
        Writes pokes;
        const auto put = [&](std::uint32_t at, std::uint32_t value, unsigned size = 4) { pokes.push_back({size, at, value}); };
        put(scratch_table + 4, std::uint32_t(humans.size()));
        put(scratch_table + 8, scenario.r32(humans_table + 8));
        put(scratch_table + 0xc, scenario.r32(humans_table + 0xc));
        for (std::size_t i = 0; i < humans.size(); ++i) {
            const auto at = scratch_array + 0x10 + std::uint32_t(i) * record_size, real = real_record(std::uint32_t(i));
            put(at + 0x588, humans[i].option0);
            put(at + slot_size + 0x588, humans[i].option1);
            put(at + 0xa658, humans[i].entries);
            for (const auto offset : {0x9a68u, 0x9a74u, 0x9a78u, 0x9a6cu}) put(at + offset, scenario.r32(real + offset));
            put(at + 0x9a7c, humans[i].linked ? scenario.r32(real + 0x9a7c) | 1 : 0);
        }
        if (humans.empty()) return pokes;
        // The older layouts' images (the first human's options: the buffer is shared) and list.
        for (unsigned image = 0; image < 3; ++image) {
            const auto real = real_record(0) + image * slot_size;
            for (std::uint32_t offset = 0; offset < image_size; offset += 0x20)
                put(buffer_base + image * image_size + offset, scenario.r32(real + offset % 0x1888));
            put(buffer_base + image * image_size + 0x588, image == 0 ? humans[0].option0 : image == 1 ? humans[0].option1 : 0x20000);
        }
        for (unsigned word = 0; word < 16; ++word) put(list_buffer + word * 4, scenario.r32(real_record(0) + 0x9ac0 + word * 8));
        return pokes;
    };

    struct Extra {
        std::uint32_t file = fake_file;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        std::vector<Case::NthAnswer> nth;
        std::uint32_t allocation = scratch_array;  // the first allocation's answer
    };
    const auto read_case = [&](const std::string& label, std::uint32_t gauge, const std::vector<Human>& humans, const Extra& extra = {}) {
        Case item{.entry = read_humans, .registers = {{3, scratch_table}, {4, extra.file}},
                  .memory = {{4, gauge_cell, gauge}, {4, scratch_table, 0x7777}, {4, scratch_table + 4, 0}}, .label = label};
        item.answers = extra.answers;  // first: an answer of the case wins over the defaults
        item.answers.insert(item.answers.end(), {{open_file, fake_handle}, {read_block, 1}, {close_file, 0}});
        item.nth_answers = extra.nth;
        item.nth_answers.push_back({allocate, 1, extra.allocation});
        // Each human's entries are read whole.
        const Reads reads(gauge, humans);
        for (std::size_t i = 0; i < humans.size(); ++i)
            if (humans[i].entries) item.nth_answers.push_back({read_block, reads.entries_data[i], humans[i].entries});
        item.pokes.push_back({read_block, file_contents(humans)});
        cases.push_back(std::move(item));
    };

    // The gauge picks each layout: tactic slots as 0x1e5c images below 0x3e021, the list as sixteen words
    // below 0x3e005, the staff rows' byte below 0x3e001; the original asks for its absolute value.
    const std::uint32_t gauges[] = {0x7fffffffu, 0x3e021u, 0x3e020u, 0x3e005u, 0x3e004u, 0x3e001u, 0x3e000u, 0u, 0xfffc2000u,
                                    0xfffc1fdfu, 0x80000000u, 0x40000000u};
    const Human plain, odd{0x7fff, 0x8000, 3, true}, unlinked{0x8000, 0x7fff, 1, false}, negative{0xffffffff, 0x80000000, 2, true},
                small{0, 0x7fffffff, 0, false};
    for (const auto gauge : gauges) {
        const auto name = " gauge " + std::to_string(std::int32_t(gauge));
        read_case("no humans," + name, gauge, {});
        read_case("one human," + name, gauge, {plain});
        read_case("one human with entries," + name, gauge, {odd});
        read_case("two humans," + name, gauge, {odd, unlinked});
        read_case("three humans," + name, gauge, {negative, small, plain});
    }

    // The failures, on the new layout and on the oldest. Each is a different step of the reader.
    for (const auto gauge : {0x7fffffffu, 0x3e000u}) {
        const auto name = " gauge " + std::to_string(std::int32_t(gauge));
        const std::vector<Human> two{odd, plain};
        const std::vector<Human> entries_two{odd, {0x10, 0x20, 2, true}};
        const Reads reads(gauge, entries_two);
        read_case("no file," + name, gauge, two, {.file = 0});
        read_case("open fails," + name, gauge, two, {.answers = {{open_file, 0}}});
        for (const std::uint32_t answer : {0u, 2u, 0xffffffffu})
            read_case("count read answers " + std::to_string(std::int32_t(answer)) + "," + name, gauge, two,
                      {.nth = {{read_block, 1, answer}}});
        read_case("no memory for the records," + name, gauge, two, {.allocation = 0});
        read_case("the array is not built," + name, gauge, two, {.answers = {{construct_array, 0}}});
        for (std::size_t human = 0; human < 2; ++human) {
            read_case("entry count read fails for human " + std::to_string(human) + "," + name, gauge, entries_two,
                      {.nth = {{read_block, reads.entries_count[human], 0}}});
            read_case("entry count read answers 2 for human " + std::to_string(human) + "," + name, gauge, entries_two,
                      {.nth = {{read_block, reads.entries_count[human], 2}}});
        }
        read_case("no memory for the entries," + name, gauge, entries_two, {.answers = {{allocate_block, 0}}});
        for (std::size_t human = 0; human < 2; ++human) {
            for (const std::uint32_t answer : {0u, 1u, 0xffffffffu}) {
                const auto count = entries_two[human].entries;
                read_case("entries read for human " + std::to_string(human) + " answers " + std::to_string(std::int32_t(answer)) + "," + name,
                          gauge, entries_two, {.nth = {{read_block, reads.entries_data[human], answer}}});
                (void)count;
            }
            read_case("entries read for human " + std::to_string(human) + " is exact," + name, gauge, entries_two,
                      {.nth = {{read_block, reads.entries_data[human], entries_two[human].entries}}});
        }
        for (const std::uint32_t answer : {0u, 2u})
            read_case("first trailing word read answers " + std::to_string(answer) + "," + name, gauge, two,
                      {.nth = {{read_block, Reads(gauge, two).table_8, answer}}});
        for (const std::uint32_t answer : {0u, 2u})
            read_case("second trailing word read answers " + std::to_string(answer) + "," + name, gauge, two,
                      {.nth = {{read_block, Reads(gauge, two).table_c, answer}}});
    }

    // The writer, on the career's table: the count, each record's slots, fields, list and entries, two words
    // and the last slots. Every call is traced, so the arguments (the records' own addresses) are what is compared.
    const auto write_case = [&](const std::string& label, std::uint32_t file, const Writes& memory,
                                std::vector<std::pair<std::uint32_t, std::uint32_t>> answers, std::vector<Case::NthAnswer> nth = {}) {
        Case item{.entry = write_humans, .registers = {{3, humans_table}, {4, file}}, .memory = memory, .label = label};
        item.answers = std::move(answers);
        item.answers.insert(item.answers.end(), {{open_file, fake_handle}, {write_block, 1}, {close_file, 0}});
        item.nth_answers = std::move(nth);
        cases.push_back(std::move(item));
    };
    const auto writes_at = [&](std::uint32_t count, unsigned what) {  // the n-th write: 1 count, 24 per record
        const std::uint32_t records = std::int32_t(count) > 0 ? count : 0;  // the loop's signed test
        return what == 0 ? 1u : what == 1 ? 2 + 24 * records : 3 + 24 * records;
    };
    for (const std::uint32_t count : {real_count, 0u, 1u, 2u, 0x80000000u, 0xffffffffu}) {
        const Writes memory{{4, humans_table + 4, count}};
        const auto name = " count " + std::to_string(std::int32_t(count));
        write_case("write" + name, fake_file, memory, {});
        write_case("write, no file," + name, 0, memory, {});
        write_case("write, open fails," + name, fake_file, memory, {{open_file, 0}});
        for (const std::uint32_t answer : {0u, 2u, 0xffffffffu}) {
            write_case("write, count write answers " + std::to_string(std::int32_t(answer)) + "," + name, fake_file, memory, {},
                       {{write_block, writes_at(count, 0), answer}});
            write_case("write, first trailing word answers " + std::to_string(std::int32_t(answer)) + "," + name, fake_file, memory, {},
                       {{write_block, writes_at(count, 1), answer}});
            write_case("write, second trailing word answers " + std::to_string(std::int32_t(answer)) + "," + name, fake_file, memory, {},
                       {{write_block, writes_at(count, 2), answer}});
        }
        // A short write of a record's data is not checked.
        write_case("write, a record write fails," + name, fake_file, memory, {}, {{write_block, 2, 0}});
    }
    // Records with entries: the sorted entries' pointer and count are the write's arguments.
    for (const std::uint32_t entries : {0u, 1u, 5u, 0x7fffffffu, 0xffffffffu}) {
        const auto record = real_record(0);
        write_case("write, entries " + std::to_string(std::int32_t(entries)), fake_file,
                   {{4, record + 0xa658, entries}, {4, record + 0xa650, entries ? 0x5000u : 0u}, {4, humans_table + 4, 1}}, {});
    }
}
}

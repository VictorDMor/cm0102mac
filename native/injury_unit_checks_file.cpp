// INJURY.CPP checker cases for the file block (injury_unit_file.hpp): the manager's constructor and
// destructor, the fitness pass over the clubs, the saved-game reader and writer, the new game's first
// injuries and the question whether a person may start injured.
//
// The file layer (open, read, write, close), the temp-file name resolver and the history object's
// constructor are other modules' routines and are answered on both sides, so no case touches a file.
// Case::nth_answers give the nth call of the file routine its own answer (a failure at one site) and
// Case::pokes deliver what a read would read: the counts into the manager's words and the note count
// into the reader's frame. Manager-sized objects live in the scratch area; the writer and the fitness
// pass use the game's own manager (global -0x7ba0).
#include "injury_unit_checks.hpp"
#include "guest_abi.hpp"
#include <algorithm>
#include <string>

namespace cm::aot::injury_unit_checks {
namespace {
constexpr std::uint32_t construct = 0x101840e0, destroy = 0x10184310, fitness = 0x101843d0, read_file = 0x10184530,
                        write_file = 0x10184e60, may_start = 0x10188490, give_injuries = 0x101885f0;
constexpr std::uint32_t file_open = 0x107475b0, file_read = 0x10747a80, file_write = 0x10747cc0, file_close = 0x10747900,
                        allocate_block = 0x10775130,
                        make_name = 0x10746280, construct_history = 0x10748bb0,
                        destroy_history = 0x10748c80, allocate_object = 0x10773520;
constexpr std::uint32_t manager_area = scratch_base + 0x10000, spec_area = scratch_base + 0x9000,
                        list_area = scratch_base + 0x20000, row_area = scratch_base + 0x21000,
                        date_area = scratch_base + 0x30000, person_area = scratch_base + 0x31000,
                        fake_handle = 0x1234, fake_name = scratch_base + 0x5000;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;

struct Memory {
    std::vector<Write> writes;
    void put(std::uint32_t at, std::uint32_t value, unsigned size = 4) { writes.push_back({size, at, value}); }
};

// A manager-sized object with every word set, so no case depends on what the area held.
Memory blank_manager(std::uint32_t fill) {
    Memory memory;
    for (std::uint32_t at = 0; at < 0x40; at += 4) memory.put(manager_area + at, fill);
    return memory;
}
// A list in the game's layout (header, then 0x10-byte nodes): the header's first halfword is `count`
// (what the writer saves as the note count), the nodes hold the given row pointers.
void build_list(Memory& memory, std::uint32_t at, std::uint16_t count, const std::vector<std::uint32_t>& rows) {
    const auto node = [&](std::size_t n) { return at + 0x20 + std::uint32_t(n) * 0x10; };
    memory.put(at, std::uint32_t(count) << 16);
    memory.put(at + 4, rows.empty() ? 0 : node(0));
    memory.put(at + 8, rows.empty() ? 0 : node(rows.size() - 1));
    memory.put(at + 0xc, 0);
    for (std::size_t n = 0; n < rows.size(); ++n) {
        memory.put(node(n), rows[n]);
        memory.put(node(n) + 4, 0);
        memory.put(node(n) + 8, n + 1 < rows.size() ? node(n + 1) : 0);
    }
}

// The reader's file calls as a script: the result of each read call in order, and the words a read
// delivers. `frame` is the reader's frame base (the note count lives at +0x38).
struct Form { std::uint32_t injuries = 3, events = 2, reports = 2, notes = 2; bool bulk = false; };
struct Script {
    std::vector<std::uint32_t> expected;
    std::vector<Case::Poke> pokes;
    Answers answers;
    std::vector<Case::NthAnswer> nth;
    std::vector<Write> gauge;  // the cell 0x10775550 (abs) is asked about picks records one by one or in one go
};
Script read_script(const Form& form, std::uint32_t people, std::uint32_t manager, std::uint32_t frame, std::uint32_t gauge_address, std::uint32_t gauge = 0) {
    Script script;
    script.gauge = {{4, gauge_address, gauge ? gauge : form.bulk ? 0x3e024u : 0x1000u}};
    auto& e = script.expected;
    e = {people, 1};
    if (form.bulk) e.push_back(form.injuries);
    else e.insert(e.end(), form.injuries, 1);
    e.push_back(1);
    if (form.events) e.push_back(form.events);
    e.push_back(1);
    if (form.reports) e.push_back(form.reports);
    e.push_back(1);
    e.insert(e.end(), form.notes, 1);
    Memory poke;
    poke.put(manager + 8, form.injuries);
    poke.put(manager + 0x14, form.events);
    poke.put(manager + 0x20, form.reports);
    poke.put(frame + 0x38, form.notes, 2);
    script.pokes = {{file_read, poke.writes}};
    script.answers = {{file_open, fake_handle}, {file_read, 1}, {file_close, 0}};
    for (std::size_t n = 0; n < e.size(); ++n) script.nth.push_back({file_read, unsigned(n + 1), e[n]});
    return script;
}
// The same with the nth read answering `wrong`.
Script failing_read(Script script, std::size_t nth, std::uint32_t wrong) {
    script.nth[nth].answer = wrong;
    return script;
}
}

INJURY_UNIT_CASES("file reader") {
    if (scenario.match) return;
    const auto people = scenario.people();
    const auto stack = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1)));
    const auto manager = scenario.global(-0x7ba0);
    // The game's own notes list when it has one: a real list for the reader to release and replace.
    const auto real_notes = scenario.r32(manager + 0x28);
    const auto reader_frame = stack - 0x300;

    const auto add = [&](const Form& form, const std::string& label, std::uint32_t old_notes, std::size_t fail_at,
                         std::uint32_t wrong, std::vector<Case::NthAnswer> extra = {}) {
        auto script = read_script(form, people, manager_area, reader_frame, scenario.toc(-0x7b74));
        if (fail_at != std::size_t(-1)) script = failing_read(script, fail_at, wrong);
        auto memory = blank_manager(0x5a5a5a5a);
        memory.writes.insert(memory.writes.end(), script.gauge.begin(), script.gauge.end());
        memory.put(manager_area + 0x28, old_notes);
        auto nth = script.nth;
        nth.insert(nth.end(), extra.begin(), extra.end());
        cases.push_back({.entry = read_file, .registers = {{3, manager_area}, {4, spec_area}}, .memory = memory.writes,
                         .label = "read " + label, .answers = script.answers, .pokes = script.pokes, .nth_answers = nth});
    };
    // Every combination of the optional parts, on both paths of the injury records.
    for (const bool bulk : {false, true})
        for (const std::uint32_t injuries : {0u, 3u})
            for (const std::uint32_t events : {0u, 2u})
                for (const std::uint32_t reports : {0u, 2u})
                    for (const std::uint32_t notes : {0u, 2u})
                        add({injuries, events, reports, notes, bulk},
                            std::string(bulk ? "bulk" : "records") + " injuries " + std::to_string(injuries) + " events " +
                                std::to_string(events) + " reports " + std::to_string(reports) + " notes " + std::to_string(notes),
                            0, std::size_t(-1), 0);
    // An old notes list is released first. The game's manager has none, so a live transfer offer stands in:
    // a heap object allocated the same way whose word at +4 (where the list routine finds its head) is null,
    // so the destroy walks no node and frees the object (0x107735a0) for real on both sides.
    {
        const auto transfers = scenario.global(-0x7b40);
        std::uint32_t offer = 0;
        if (transfers)
            for (std::uint32_t i = 0, limit = scenario.r32(transfers + 4); i < limit && !offer; ++i) {
                const auto candidate = scenario.r32(scenario.r32(transfers) + i * 4);
                if (candidate && scenario.r32(candidate + 4) == 0) offer = candidate;
            }
        const auto old_notes = real_notes ? real_notes : offer;
        if (old_notes) {
            add({3, 2, 2, 2, false}, "replacing an old notes list", old_notes, std::size_t(-1), 0);
            add({3, 2, 2, 2, true}, "replacing an old notes list, bulk", old_notes, std::size_t(-1), 0);
            add({3, 2, 2, 0, false}, "keeping an old notes list", old_notes, std::size_t(-1), 0);
        }
    }
    // The memory gauge's boundary and signs: records one by one below 0x3e024, in one go from it.
    for (const std::uint32_t gauge : {0x3e023u, 0x3e024u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 0xfffc1fdcu, 0xfffc1fddu}) {
        const std::int32_t signed_gauge = std::int32_t(gauge);
        const std::uint32_t magnitude = signed_gauge < 0 ? 0u - gauge : gauge;
        const bool bulk = std::int32_t(magnitude) >= 0x3e024;  // abs, then a signed compare
        auto script = read_script({3, 2, 2, 2, bulk}, people, manager_area, reader_frame, scenario.toc(-0x7b74), gauge);
        auto memory = blank_manager(0x5a5a5a5a);
        memory.writes.insert(memory.writes.end(), script.gauge.begin(), script.gauge.end());
        memory.put(manager_area + 0x28, 0);
        cases.push_back({.entry = read_file, .registers = {{3, manager_area}, {4, spec_area}}, .memory = memory.writes,
                         .label = "read, gauge " + std::to_string(gauge), .answers = script.answers, .pokes = script.pokes,
                         .nth_answers = script.nth});
    }
    // Each read short (0 items or one too many), in both paths: the nth read reaches each failure site.
    for (const bool bulk : {false, true}) {
        const Form form{3, 2, 2, 2, bulk};
        const auto count = read_script(form, people, manager_area, reader_frame, scenario.toc(-0x7b74)).expected.size();
        for (std::size_t nth = 0; nth < count; ++nth)
            for (const bool zero : {true, false}) {
                const auto expected = read_script(form, people, manager_area, reader_frame, scenario.toc(-0x7b74)).expected[nth];
                add(form, std::string(bulk ? "bulk" : "records") + ", read " + std::to_string(nth + 1) + (zero ? " none" : " extra"),
                    0, nth, zero ? 0 : expected + 1);
            }
    }
    // Each allocation failing: the status rows, the injury records, the events, the reports.
    for (unsigned nth = 1; nth <= 4; ++nth)
        add({3, 2, 2, 2, false}, "allocation " + std::to_string(nth) + " fails", 0, std::size_t(-1), 0, {{allocate_block, nth, 0}});
    // The notes list allocation failing (a null list is kept and the rows are still read into it).
    add({3, 2, 2, 2, false}, "notes list allocation fails", 0, std::size_t(-1), 0, {{allocate_object, 1, 0}});
    // No file, a file that does not open.
    cases.push_back({.entry = read_file, .registers = {{3, manager_area}, {4, 0}}, .memory = blank_manager(0x5a5a5a5a).writes,
                     .label = "read, no file spec", .answers = {{file_open, fake_handle}}});
    cases.push_back({.entry = read_file, .registers = {{3, manager_area}, {4, spec_area}},
                     .memory = blank_manager(0x5a5a5a5a).writes, .label = "read, open fails", .answers = {{file_open, 0}}});
}

INJURY_UNIT_CASES("file writer") {
    if (scenario.match) return;
    const auto people = scenario.people();
    const auto manager = scenario.global(-0x7ba0);

    // The writer's file calls: the result of each write in order.
    struct Fields { std::uint32_t rows, injuries, injury_count, events, event_capacity, reports, report_capacity, notes; };
    const Fields real{scenario.r32(manager), scenario.r32(manager + 4), scenario.r32(manager + 8), scenario.r32(manager + 0x10),
                      scenario.r32(manager + 0x18), scenario.r32(manager + 0x1c), scenario.r32(manager + 0x24),
                      scenario.r32(manager + 0x28)};
    const auto expected_writes = [&](const Fields& f, std::size_t note_rows) {
        std::vector<std::uint32_t> e = {people, 1, f.injury_count, 1};
        if (f.event_capacity) e.push_back(f.event_capacity);
        e.push_back(1);
        if (f.report_capacity) e.push_back(f.report_capacity);
        e.push_back(1);
        e.insert(e.end(), note_rows, 1);
        return e;
    };
    // A note list of `rows` rows whose header says `count`, in the scratch area (0 for no list).
    const auto make_case = [&](Fields f, std::size_t note_rows, std::uint16_t note_count, const std::string& label,
                               std::size_t fail_at, std::uint32_t wrong, std::uint32_t file_spec = spec_area,
                               std::uint32_t open_answer = fake_handle) {
        Memory memory = blank_manager(0);
        if (note_rows != std::size_t(-1)) {
            std::vector<std::uint32_t> rows;
            for (std::size_t n = 0; n < note_rows; ++n) rows.push_back(row_area + std::uint32_t(n) * 8);
            build_list(memory, list_area, note_count, rows);
            f.notes = list_area;
        }
        const Fields fields = f;
        memory.put(manager_area, fields.rows);
        memory.put(manager_area + 4, fields.injuries);
        memory.put(manager_area + 8, fields.injury_count);
        memory.put(manager_area + 0x10, fields.events);
        memory.put(manager_area + 0x18, fields.event_capacity);
        memory.put(manager_area + 0x1c, fields.reports);
        memory.put(manager_area + 0x24, fields.report_capacity);
        memory.put(manager_area + 0x28, fields.notes);
        const auto notes_walked = note_rows == std::size_t(-1) ? 0 : note_rows;
        auto expected = expected_writes(fields, notes_walked);
        std::vector<Case::NthAnswer> nth;
        for (std::size_t n = 0; n < expected.size(); ++n)
            nth.push_back({file_write, unsigned(n + 1), n == fail_at ? wrong : expected[n]});
        cases.push_back({.entry = write_file, .registers = {{3, manager_area}, {4, file_spec}}, .memory = memory.writes,
                         .label = "write " + label,
                         .answers = {{file_open, open_answer}, {file_write, 1}, {file_close, 0}}, .nth_answers = nth});
    };
    const std::size_t none = std::size_t(-1);
    Fields base = real;
    base.rows = real.rows;
    // The game's own manager, as it is.
    {
        Memory memory;
        std::size_t note_rows = 0;
        // Its notes list is walked by the real list routine, so the script counts the nodes the list has.
        for (auto node = real.notes ? scenario.r32(real.notes + 4) : 0; node; node = scenario.r32(node + 8)) ++note_rows;
        auto expected = expected_writes(real, note_rows);
        std::vector<Case::NthAnswer> nth;
        for (std::size_t n = 0; n < expected.size(); ++n) nth.push_back({file_write, unsigned(n + 1), expected[n]});
        cases.push_back({.entry = write_file, .registers = {{3, manager}, {4, spec_area}}, .label = "write the game's manager",
                         .answers = {{file_open, fake_handle}, {file_write, 1}, {file_close, 0}}, .nth_answers = nth});
    }
    // Every optional part present or not, with and without a list of notes.
    for (const std::uint32_t events : {0u, 3u})
        for (const std::uint32_t reports : {0u, 2u})
            for (const std::size_t note_rows : {none, std::size_t(0), std::size_t(1), std::size_t(3)})
                for (const std::uint16_t note_count : {std::uint16_t(0), std::uint16_t(2)}) {
                    Fields f = real;
                    f.event_capacity = events;
                    f.events = events ? row_area : 0;
                    f.report_capacity = reports;
                    f.reports = reports ? row_area : 0;
                    f.notes = 0;
                    if (note_rows == none && note_count) continue;
                    make_case(f, note_rows, note_count,
                              "events " + std::to_string(events) + " reports " + std::to_string(reports) + " notes " +
                                  (note_rows == none ? std::string("none") : std::to_string(note_rows)) + " count " +
                                  std::to_string(note_count),
                              none, 0);
                }
    // Each write short (none, or one too many): the nth write reaches each failure site of the writer.
    {
        Fields f = real;
        f.event_capacity = 3;
        f.events = row_area;
        f.report_capacity = 2;
        f.reports = row_area;
        const auto expected = expected_writes(f, 2);
        for (std::size_t nth = 0; nth < expected.size(); ++nth)
            for (const bool zero : {true, false})
                make_case(f, 2, 2, "write " + std::to_string(nth + 1) + (zero ? " none" : " extra"), nth,
                          zero ? 0 : expected[nth] + 1);
    }
    // No file, a file that does not open.
    make_case(real, none, 0, "no file spec", none, 0, 0);
    make_case(real, none, 0, "open fails", none, 0, spec_area, 0);
}

INJURY_UNIT_CASES("file construct") {
    if (scenario.match) return;
    const auto people = scenario.people();
    const auto stack = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1)));
    const auto reader_frame = stack - 0x380 - 0x300;
    // The temp-file name resolver and the history object are answered (no file is touched).
    const Answers common = {{0x10748ce0, 0}, {make_name, fake_name}, {construct_history, 0}, {file_open, fake_handle},
                            {file_read, 1}, {file_close, 0}};
    // FSMakeFSSpec's glue is reached directly by the original, so no answer can stand in for it: the
    // import cell (TOC -0x7de0) is pointed at a descriptor of a game routine that returns 0 (0x10002390),
    // or at today's-year getter (0x1048e29c) for the retry case, where the dialog's poke zeroes the year.
    const std::uint32_t import_cell = 0x108bba90u - 0x7de0u;
    const auto stub = [&](std::uint32_t code) {
        Memory memory;
        memory.put(spec_area + 0x100, code);
        memory.put(spec_area + 0x104, 0x108bba90);
        memory.put(import_cell, spec_area + 0x100);
        return memory;
    };
    std::vector<Write> gauge;  // set below for the saved-game cases
    const auto add = [&](const std::string& label, std::uint32_t spec, Answers answers, std::vector<Case::NthAnswer> nth = {},
                         std::vector<Case::Poke> pokes = {}, std::uint32_t code = 0x10002390) {
        answers.insert(answers.end(), common.begin(), common.end());
        auto memory = blank_manager(0x5a5a5a5a);
        for (const auto& write : stub(code).writes) memory.writes.push_back(write);
        memory.writes.insert(memory.writes.end(), gauge.begin(), gauge.end());
        cases.push_back({.entry = construct, .registers = {{3, manager_area}, {4, spec}}, .memory = memory.writes,
                         .label = "construct " + label, .answers = answers, .pokes = pokes, .nth_answers = nth});
    };
    add("new game", 0, {});
    add("new game, no history memory", 0, {}, {{allocate_object, 1, 0}});
    add("new game, rows allocation fails", 0, {}, {{allocate_block, 1, 0}});
    {
        // The year (today +2) is non-zero, so the stub fails; the dialog's poke zeroes it and the retry resolves.
        const auto today = scenario.toc(-0x7b80);
        Memory zero;
        zero.put(today + 2, 0, 2);
        add("name resolves on the second try", 0, {}, {}, {{0x10769b60, zero.writes}}, 0x1048e29c);
    }
    add("saved game does not open", spec_area, {{file_open, 0}});
    for (const bool bulk : {false, true}) {
        auto script = read_script({3, 2, 2, 2, bulk}, people, manager_area, reader_frame, scenario.toc(-0x7b74));
        gauge = script.gauge;
        auto answers = script.answers;
        answers.erase(answers.begin());  // the common answers carry the open
        add(std::string("a saved game, ") + (bulk ? "bulk" : "records"), spec_area, answers, script.nth, script.pokes);
        // A failed read goes on to the new game's pass. The poke would leave a count of injury records over a
        // null table (the new game then faults on both sides), so these use a file with no records.
        const auto empty = read_script({0, 0, 0, 0, bulk}, people, manager_area, reader_frame, scenario.toc(-0x7b74));
        gauge = empty.gauge;
        auto empty_answers = empty.answers;
        empty_answers.erase(empty_answers.begin());
        for (const std::size_t nth : {std::size_t(0), std::size_t(1)}) {
            auto failing = failing_read(empty, nth, 0);
            add(std::string("a saved game, ") + (bulk ? "bulk" : "records") + ", read " + std::to_string(nth + 1) + " fails",
                spec_area, empty_answers, failing.nth, failing.pokes);
        }
        add(std::string("a saved game without records, ") + (bulk ? "bulk" : "records"), spec_area, empty_answers, empty.nth, empty.pokes);
    }
    // The saved game's rows could not be allocated: the new game's pass runs after the failed read.
    {
        auto script = read_script({3, 2, 2, 2, false}, people, manager_area, reader_frame, scenario.toc(-0x7b74));
        gauge = script.gauge;
        auto answers = script.answers;
        answers.erase(answers.begin());
        auto nth = script.nth;
        nth.push_back({allocate_block, 1, 0});
        add("a saved game without memory for its rows", spec_area, answers, nth, script.pokes);
    }
}

INJURY_UNIT_CASES("file destroy") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ba0);
    // The history object's own destructor is another module's (it closes its temp file): answered.
    const Answers released = {{destroy_history, 0}};
    for (const std::uint32_t deleting : {0u, 1u, 0xffffu, 0x8000u, 0x7fffu, 0x10001u, 0x18000u})
        cases.push_back({.entry = destroy, .registers = {{3, manager}, {4, deleting}},
                         .label = "destroy the game's manager, deleting " + std::to_string(deleting), .answers = released});
    cases.push_back({.entry = destroy, .registers = {{3, 0}, {4, 1}}, .label = "destroy null", .answers = released});
    // Every table present or absent, with real heap blocks of the career (freed for real on both sides).
    for (unsigned mask = 0; mask < 32; ++mask) {
        Memory memory = blank_manager(0);
        const std::uint32_t fields[] = {0x00, 0x04, 0x0c, 0x10, 0x1c};
        // Four distinct heap blocks the allocator accepts back: the people and club tables, the manager's own
        // status rows and injury records. (The history slot is answered, so any word will do.)
        const std::uint32_t blocks[] = {scenario.global(-0x7be0), scenario.global(-0x7be8), scenario.r32(manager + 0xc),
                                        scenario.r32(manager + 4), scenario.r32(manager)};
        for (unsigned bit = 0; bit < 5; ++bit)
            if (mask >> bit & 1) memory.put(manager_area + fields[bit], blocks[bit]);
        cases.push_back({.entry = destroy, .registers = {{3, manager_area}, {4, 0u}}, .memory = memory.writes,
                         .label = "destroy pieces " + std::to_string(mask),
                         .answers = {{destroy_history, 0}}});
    }
}

INJURY_UNIT_CASES("file fitness") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ba0);
    const auto club_total = scenario.toc(-0x7b50), reserved = scenario.toc(-0x7b04), people_cell = scenario.toc(-0x7be4);
    cases.push_back({.entry = fitness, .registers = {{3, manager}}, .label = "fitness, the game's clubs"});
    // The loop bound: none, a few clubs, a negative bound (twice the reserved count past the total).
    for (const std::uint32_t total : {0u, 1u, 3u, 0x7fffffffu})
        if (total != 0x7fffffffu) cases.push_back({.entry = fitness, .registers = {{3, manager}}, .memory = {{4, club_total, total}},
                         .label = "fitness, club total " + std::to_string(total)});
    cases.push_back({.entry = fitness, .registers = {{3, manager}}, .memory = {{4, reserved, 100000}},
                     .label = "fitness, reserved huge"});
    cases.push_back({.entry = fitness, .registers = {{3, manager}}, .memory = {{4, club_total, 5}, {4, reserved, 1}},
                     .label = "fitness, five less two"});
    // The days term of every clamp: no date (answered non-zero), then a date and the days answered. The
    // answers hold for every club, so each player slot of the first clubs meets the same clamps.
    cases.push_back({.entry = fitness, .registers = {{3, manager}}, .memory = {{4, club_total, 4}},
                     .label = "fitness, no club date", .answers = {{0x101929b0, 1}}});
    for (const std::uint32_t days : {0u, 5u, 0x10u, 0x27u, 0x28u, 0x29u, 0x30u, 0x4fu, 0x50u, 0x51u, 0x80u, 0x7fffffffu,
                                     0x80000000u, 0xfffffffbu, 0x12345678u})
        cases.push_back({.entry = fitness, .registers = {{3, manager}}, .memory = {{4, club_total, 4}},
                         .label = "fitness, days " + std::to_string(days), .answers = {{0x101929b0, 0}, {0x10718130, days}}});
    // A negative attribute byte (the real ones are 7..16) lifts the term above the 40 ceiling: the first
    // four clubs' players get -128 / -60 / 0 there, with the days answered (0 days: -64 + 0 = 64).
    for (const std::uint32_t byte : {0x80u, 0xc4u, 0u})
        for (const std::uint32_t days : {0u, 0x10u, 0x30u}) {
            std::vector<Write> writes = {{4, club_total, 4}};
            for (unsigned club = 0; club < 4; ++club)
                for (unsigned slot = 0; slot < 50; ++slot)
                    if (const auto person = scenario.r32(scenario.club(club) + 0x98 + slot * 4); person && scenario.r32(person + 0x14))
                        writes.push_back({1, scenario.r32(person + 0x14) + 0x34, byte});
            cases.push_back({.entry = fitness, .registers = {{3, manager}}, .memory = writes,
                             .label = "fitness, attribute byte " + std::to_string(byte) + " days " + std::to_string(days),
                             .answers = {{0x101929b0, 0}, {0x10718130, days}}});
        }
    // People past the count are left alone: only the first few people count.
    cases.push_back({.entry = fitness, .registers = {{3, manager}}, .memory = {{4, people_cell, 3}, {4, club_total, 6}},
                     .label = "fitness, three people"});
    cases.push_back({.entry = fitness, .registers = {{3, manager}}, .memory = {{4, people_cell, 0}},
                     .label = "fitness, no people"});
}

INJURY_UNIT_CASES("file start") {
    if (scenario.match) return;
    const auto people = scenario.people();
    const auto today = scenario.toc(-0x7b80), people_cell = scenario.toc(-0x7be4);
    const auto manager = scenario.global(-0x7ba0);

    // 0x10188490: may this person start injured. Real people first.
    for (std::uint32_t index = 0; index < people; index += std::max(1u, people / 97))
        cases.push_back({.entry = may_start, .registers = {{3, manager}, {4, scenario.person(index)}},
                         .label = "may start, person " + std::to_string(index)});
    cases.push_back({.entry = may_start, .registers = {{3, manager}, {4, 0}}, .label = "may start, no person"});

    // Built people (the fields the rule reads) with the club-side date, the contract word and the
    // answers of the date routines chosen: every branch of the date comparison and the final question.
    const auto today_first = scenario.r32(today) >> 16, today_second = scenario.r32(today) & 0xffff;
    const auto person = person_area, contract = person_area + 0x100, team = person_area + 0x200;
    struct Dates { const char* label; std::uint32_t first, second; };
    // The date routine's answer is a pointer to this date (two halfwords and a byte).
    const std::vector<Dates> dates = {
        {"later second", today_first, (today_second + 1) & 0xffff}, {"earlier second", today_first, (today_second - 1) & 0xffff},
        {"same date", today_first, today_second},                   {"earlier first", (today_first - 3) & 0xffff, today_second},
        {"later first", (today_first + 3) & 0xffff, today_second},  {"negative first", 0xfff0, today_second},
        {"first against negative", 0x0010, today_second},           {"both negative", 0xff00, today_second},
        {"zero first", 0, today_second},                            {"huge", 0x7fff, today_second}};
    for (const auto& date : dates)
        for (const std::uint32_t flags : {0u, 8u, 0xffu})
            for (const std::uint32_t pending : {0xffffffffu, 0u, 7u})
                for (const std::uint32_t obstacle : {0u, 1u}) {
                    Memory memory;
                    memory.put(person, 5);
                    memory.put(person + 0x20, team);
                    memory.put(person + 0x24, contract);
                    memory.put(person + 0x6a, flags, 1);
                    memory.put(team + 0x2c, 9);
                    memory.put(contract + 0x14, pending);
                    memory.put(date_area, date.first << 16 | date.second);
                    memory.put(date_area + 4, 1u << 24);
                    // One days answer per case, from small to large, decides the last question.
                    const std::uint32_t days = (flags + pending + obstacle) % 3 == 0 ? 0x27 : (flags + obstacle) % 2 ? 0x28 : 0x10005;
                    cases.push_back({.entry = may_start, .registers = {{3, manager}, {4, person}}, .memory = memory.writes,
                                     .label = std::string("may start, ") + date.label + " flags " + std::to_string(flags) +
                                              " pending " + std::to_string(pending) + " obstacle " + std::to_string(obstacle),
                                     .answers = {{0x102ae9c0, 3}, {0x102af650, date_area}, {0x101929b0, obstacle}, {0x10718130, days}}});
                }
    // A person with no team, with no contract, with the team's second word zero.
    {
        Memory memory;
        memory.put(person, 5);
        memory.put(person + 0x20, 0);
        memory.put(person + 0x24, contract);
        memory.put(contract + 0x14, 0xffffffff);
        cases.push_back({.entry = may_start, .registers = {{3, manager}, {4, person}}, .memory = memory.writes,
                         .label = "may start, no team", .answers = {{0x101929b0, 0}, {0x10718130, 3}}});
        cases.push_back({.entry = may_start, .registers = {{3, manager}, {4, person}}, .memory = memory.writes,
                         .label = "may start, no team, real date routines"});
        memory.put(person + 0x24, 0);
        cases.push_back({.entry = may_start, .registers = {{3, manager}, {4, person}}, .memory = memory.writes,
                         .label = "may start, no team, no contract"});
    }
    // The days answer's own boundaries (low halfword against 0x28).
    for (const std::uint32_t days : {0u, 0x27u, 0x28u, 0x29u, 0xffffu, 0x10000u, 0x10027u, 0xffff0027u, 0xffff0028u}) {
        Memory memory;
        memory.put(person, 5);
        memory.put(person + 0x20, 0);
        memory.put(person + 0x24, contract);
        memory.put(contract + 0x14, 0xffffffff);
        cases.push_back({.entry = may_start, .registers = {{3, manager}, {4, person}}, .memory = memory.writes,
                         .label = "may start, days " + std::to_string(days), .answers = {{0x101929b0, 0}, {0x10718130, days}}});
    }

    // 0x101885f0: the new game's pass over the people. The first draws come from wherever the shared
    // sample table's cursor stands; moving it reaches the types and the 1-in-10 and 1-in-100 draws.
    const auto cursor = scenario.toc(-0x7b84);
    const auto scratch_manager = [&] { Memory memory; for (std::uint32_t at = 0; at < 0x40; at += 4) memory.put(manager_area + at, 0); return memory; };
    // A recurring injury is logged through the history object (0x10748ce0), which opens a file: answered.
    const Answers history_answers = {{0x10748ce0, 0}};
    for (const std::uint32_t offset : {0u, 2u, 62u, 1000u, 40000u}) {
        auto memory = scratch_manager();
        memory.put(cursor, scenario.r32(cursor) + offset);
        cases.push_back({.entry = give_injuries, .registers = {{3, manager_area}}, .memory = memory.writes,
                         .label = "starting injuries, cursor +" + std::to_string(offset), .answers = history_answers});
    }
    for (const std::uint32_t count : {0u, 1u, 7u, 300u, 0xffffffffu, 0x80000000u}) {
        auto memory = scratch_manager();
        memory.put(people_cell, count);
        cases.push_back({.entry = give_injuries, .registers = {{3, manager_area}}, .memory = memory.writes,
                         .label = "starting injuries, " + std::to_string(count) + " people",
                         .answers = {{allocate_block, scratch_base + 0x40000}, {0x10748ce0, 0}}});
    }
    cases.push_back({.entry = give_injuries, .registers = {{3, manager_area}}, .memory = scratch_manager().writes,
                     .label = "starting injuries, no memory for the rows", .answers = {{allocate_block, 0}}});
    cases.push_back({.entry = give_injuries, .registers = {{3, manager_area}}, .memory = scratch_manager().writes,
                     .label = "starting injuries, the game's rows", .answers = history_answers});
}
}

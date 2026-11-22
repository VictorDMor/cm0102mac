// PLAYER_REGEN.CPP, block `lifecycle_and_drivers`: the regen object's constructor (0x102d8230) and destructor (0x102d85c0) and the two scans
// that fill its retirement list (0x102d86c0 daily, 0x102d8830 for a new game).
//
// Constructor. It builds a Pascal string from two obfuscated 17- and 16-byte pieces of the unit's data (the first byte is the sum of the two
// pieces' first bytes, the rest alternates the first piece's and the second's bytes while the first piece's byte is not zero), asks the
// system for that file (FSMakeFSSpec, the glue 0x1077fee0) and, until it is found, shows the game's alert (two localized texts and the
// dialog) and asks again. Then it allocates the four lists' blocks (A 250 entries of 0x1c bytes, B of 0xc, C and D of 0x10: 0x1b58, 0xbb8,
// 0xfa0 and 0xfa0 bytes), builds the per-nation club lists (0x102e98a0) and either runs the initial scan and seeds list C with ten
// sentinel entries {staff row count - 1 - n, -1, -1} (the cell at TOC -0x79f4 is the count of the 0x44-byte staff rows whose table is TOC -0x79f8: database_load_detail.hpp, database_save_detail.hpp; the first word of a list C entry is a staff row index, as 0x102dabb0 uses it) (a new game: `archive` is 0) or loads retire.dat from the archive (0x102ead70); last it
// reports the game's assertion and calls the quit routine for each list whose block is missing (lines 0x7f, 0x83, 0x87, 0x8b).
//
// Destructor. Frees the four blocks that exist (clearing their pointers), the per-nation club lists (each object deleted, then the array)
// and, for a positive `flag` (an halfword), the object itself through the game's object storage; returns the object.
//
// Scans. Both walk the people table with a retirement entry built on the frame (0x44, its date objects begun by 0x10715880 at +0x4c and
// +0x54, +0x3c): a person qualifies when his id is below the people count less 16, his kind-table class is 0 (0x102de3c0), no list A entry
// has his id (the daily scan only: 0x102eb610) and his contract has no flag in byte +0x54 (0x10082f20's record, when there is one); the
// daily scan skips a person under 30 whose club's link (+0x30) is marked in the link table. The daily scan asks 0x102dcb20 and, when
// that says no, 0x102de6e0; the initial scan asks 0x102dd910; a yes appends the entry to list A (0x102dee70). The list is then sorted
// (qsort4x, 0x1c-byte entries, the comparator of the table slot -0x6328).
//
// Fidelity. The callee-saved registers the original keeps its values in are mirrored (`Run::held`) and set before every call, so a
// callee's prologue saves them: the scans leave r27/r28 (and r26..r29) as the caller held them until the original sets them. The routines
// of this module run as C++ with the arguments the original's registers hold at the call (r3..r5 are set where the callee leaves the
// register as it found it, so the leftovers that flow on stay the original's); the others go through the dispatcher at the original's
// return addresses. The scans read the callee-saved r19 their caller held (the decisions' job byte of a staff member with no job bytes):
// the hook hands it over as `incoming_r19`, as for the decisions themselves.
#include "player_regen_detail.hpp"
#include <array>

namespace cm::aot::player_regen {
namespace {
namespace fn {
constexpr std::uint32_t make_date = 0x10715880, contract_of = 0x10082f20, qsort = 0x10768300, free_block = 0x10775170, allocate = 0x10775130,
                        delete_nation_list = 0x10738c20, delete_object = 0x107735a0, make_spec = 0x1077fee0, format = 0x10727890, dialog = 0x10769b60;
}
constexpr std::int32_t contracts_cell = -0x7b7c, link_table_cell = -0x7b5c, success_cell = -0x7bac, buffer_cell = -0x7ba8, staff_count_cell = -0x79f4,
                       name_data_a = -0x5098, name_data_b = -0x509c;
namespace p { constexpr std::uint32_t id = 0x00, club = 0x24, age = 0x5c; }
namespace cl { constexpr std::uint32_t link = 0x30; }
namespace ct { constexpr std::uint32_t flag = 0x54; }

// r`first`..r31 as the original holds them, set before every call; the registers below `first` are the caller's, which no routine here
// writes.
struct Run {
    Cpu& c;
    unsigned first;
    std::array<std::uint32_t, 32> held{};
    Run(Cpu& cpu, unsigned first_gpr) : c(cpu), first(first_gpr) { for (unsigned n = first; n < 32; ++n) held[n] = c.r(n); }
    void sync() { for (unsigned n = first; n < 32; ++n) c.set(n, held[n]); }
    std::uint32_t call(std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        sync();
        return invoke(c, target, back, arguments);
    }
    // A routine of this module called as C++: the registers the original sets for the call, then the call with its link register.
    template <class F> auto direct(std::uint32_t back, std::initializer_list<std::pair<unsigned, std::uint32_t>> registers, F&& f) {
        sync();
        c.set(registers);
        const auto link = c.lr();
        c.set_lr(back);
        struct Restore { Cpu& c; std::uint32_t link; ~Restore() { c.set_lr(link); } } restore{c, link};
        return f();
    }
};

// The prologue of a scan: three date objects on the frame, begun by 0x10715880 (the retirement entry's dates at +8 and +0x10 are the second
// and the first of them: +0x4c and +0x54).
void begin_dates(Run& run, const Frame& frame, std::uint32_t first_back) {
    run.call(fn::make_date, first_back, {frame.at(0x4c)});
    run.call(fn::make_date, first_back + 8, {frame.at(0x54)});
    run.call(fn::make_date, first_back + 16, {frame.at(0x3c)});
}

// Sorts list A (qsort4x over the 0x1c-byte entries with the retirement entries' comparator).
void sort_retirements(Run& run, std::uint32_t regen, std::uint32_t back) {
    auto& c = run.c;
    run.call(fn::qsort, back, {c.u32(regen + retirement_list.entries), c.u32(regen + retirement_list.count), retirement_entry::size,
                               c.toc(toc::bsearch_comparator)});
}
}

void scan_retirements(Cpu& c, std::uint32_t regen, std::uint32_t incoming_r19) {
    save_link(c);
    Frame frame(c, 0x102d86c0, 0x80, {.first_gpr = 26});
    Run run(c, 26);
    auto& h = run.held;
    const auto count_cell = c.toc(toc::person_count), contracts = c.toc(contracts_cell), link_table = c.toc(link_table_cell);
    h[26] = regen; h[29] = contracts; h[30] = link_table; h[31] = count_cell;
    begin_dates(run, frame, 0x102d86e8);
    const auto first = c.u32(c.toc(toc::people));
    h[28] = first;
    h[27] = first + c.u32(count_cell) * person::size;
    const auto entry = frame.at(0x44);
    for (; h[28] < h[27]; h[28] += person::size) {
        const auto person = h[28];
        // A person under 30 whose club's link is marked in the link table is left alone.
        if (sext8(c.u16(person + p::age)) < 0x1e) {
            const auto club = c.u32(person + p::club);
            if (club != 0) {
                const auto link = c.u32(club + cl::link);
                if (link != 0 && c.u32(c.u32(link_table) + (c.u32(link) << 2)) != 0) continue;
            }
        }
        if (c.i32(person + p::id) >= std::int32_t(c.u32(count_cell) - 0x10)) continue;
        if (sext8(run.direct(0x102d8774, {{3, regen}, {4, person}}, [&] { return std::uint32_t(person_class(c, person)); })) != 0) continue;
        if (sext8(run.direct(0x102d8788, {{3, regen}, {4, person}}, [&] { return std::uint32_t(retirement_queued(c, regen, person)); })) != 0) continue;
        const auto contract = run.call(fn::contract_of, 0x102d879c, {c.u32(contracts), person});
        if (contract != 0 && sext8(c.u8(contract + ct::flag)) != 0) continue;
        if (run.direct(0x102d87c0, {{3, regen}, {4, person}, {5, entry}}, [&] { return decide_retirement(c, person, entry, incoming_r19); })) {
            run.direct(0x102d87d4, {{3, regen}, {4, entry}}, [&] { append_retirement(c, regen, entry); });
            continue;
        }
        if (run.direct(0x102d87e8, {{3, regen}, {4, person}, {5, entry}}, [&] { return decide_unattached_retirement(c, person, entry); }))
            run.direct(0x102d87fc, {{3, regen}, {4, entry}}, [&] { append_retirement(c, regen, entry); });
    }
    sort_retirements(run, regen, 0x102d881c);
}

void scan_initial_retirements(Cpu& c, std::uint32_t regen, std::uint32_t incoming_r19) {
    save_link(c);
    Frame frame(c, 0x102d8830, 0x80, {.first_gpr = 27});
    Run run(c, 27);
    auto& h = run.held;
    const auto count_cell = c.toc(toc::person_count), contracts = c.toc(contracts_cell);
    h[27] = regen; h[30] = contracts; h[31] = count_cell;
    begin_dates(run, frame, 0x102d8854);
    h[28] = 0;
    h[29] = c.u32(c.toc(toc::people));
    const auto entry = frame.at(0x44);
    for (; std::int32_t(h[28]) < c.i32(count_cell); ++h[28], h[29] += person::size) {
        const auto person = h[29];
        if (c.i32(person + p::id) >= std::int32_t(c.u32(count_cell) - 0x10)) continue;
        if (sext8(run.direct(0x102d8890, {{3, regen}, {4, person}}, [&] { return std::uint32_t(person_class(c, person)); })) != 0) continue;
        const auto contract = run.call(fn::contract_of, 0x102d88a4, {c.u32(contracts), person});
        if (contract != 0 && sext8(c.u8(contract + ct::flag)) != 0) continue;
        if (run.direct(0x102d88c8, {{3, regen}, {4, person}, {5, entry}}, [&] { return decide_retirement_with_club(c, person, entry, incoming_r19); }))
            run.direct(0x102d88dc, {{3, regen}, {4, entry}}, [&] { append_retirement(c, regen, entry); });
    }
    sort_retirements(run, regen, 0x102d8904);
}

std::uint32_t destroy_regen_object(Cpu& c, std::uint32_t object, std::uint32_t flag) {
    save_link(c);
    Frame frame(c, 0x102d85c0, 0x50, {.first_gpr = 27});
    Run run(c, 27);
    auto& h = run.held;
    h[27] = object; h[28] = flag; h[31] = c.toc(toc::nation_count);
    if (object == 0) return object;
    // The four blocks (A, B, C, D): freed when present, the pointer cleared.
    const std::array<std::pair<std::uint32_t, std::uint32_t>, 4> blocks{{{retirement_list.entries, 0x102d85f0}, {queue_list.entries, 0x102d8608},
                                                                         {staff_pool_list.entries, 0x102d8620}, {player_pool_list.entries, 0x102d8638}}};
    for (const auto& [slot, back] : blocks)
        if (c.u32(object + slot) != 0) {
            run.call(fn::free_block, back, {c.u32(object + slot)});
            c.put32(object + slot, 0);
        }
    // The per-nation club lists: each object deleted, then the array.
    if (c.u32(object) != 0) {
        h[29] = 0; h[30] = 0;
        for (; std::int32_t(h[29]) < c.i32(h[31]); ++h[29], h[30] += 4)
            run.call(fn::delete_nation_list, 0x102d8668, {c.u32(c.u32(object) + h[30]), 1});
        run.call(fn::free_block, 0x102d8684, {c.u32(object)});
        c.put32(object, 0);
    }
    if (sext16(flag) > 0) run.call(fn::delete_object, 0x102d869c, {object});
    c.set(3, object);
    return object;
}

std::uint32_t construct_regen_object(Cpu& c, std::uint32_t object, std::uint32_t archive) {
    save_link(c);
    Frame frame(c, 0x102d8230, 0x400, {.first_gpr = 22});
    Run run(c, 22);
    auto& h = run.held;
    const auto pool = c.toc(toc::pool);
    h[23] = object; h[24] = archive; h[26] = c.toc(success_cell); h[27] = c.toc(buffer_cell); h[28] = c.toc(staff_count_cell); h[29] = pool;
    h[30] = c.toc(name_data_b); h[31] = c.toc(name_data_a);
    // The file's name, until the system finds it.
    do {
        h[25] = 0;
        const auto a = h[31], b = h[30];
        const std::array<std::uint32_t, 4> first{c.u32(a), c.u32(a + 4), c.u32(a + 8), c.u32(a + 12)};
        const std::array<std::uint32_t, 4> second{c.u32(b), c.u32(b + 4), c.u32(b + 8), c.u32(b + 12)};
        for (unsigned n = 0; n < 4; ++n) { c.put32(frame.at(0xe8 + 4 * n), first[n]); c.put32(frame.at(0x58 + 4 * n), second[n]); }
        c.put8(frame.at(0xf8), c.u8(a + 0x10));
        auto from_first = frame.at(0xe9), from_second = frame.at(0x59), to = frame.at(0x145);
        h[22] = from_second;
        while (sext8(c.u8(from_first)) != 0) {
            c.put8(to, c.u8(from_first++));
            c.put8(to + 1, c.u8(from_second++));
            to += 2;
            h[22] = from_second;
        }
        c.put8(frame.at(0x144), c.u8(frame.at(0xe8)) + c.u8(frame.at(0x58)));
        // What the original leaves in r7..r10 for the glue: the first piece's first byte and its words 2, 1 and 0.
        run.sync();
        c.set({{7, c.u8(frame.at(0xe8))}, {8, first[2]}, {9, first[1]}, {10, first[0]}});
        const auto status = invoke(c, fn::make_spec, 0x102d830c, {0, 0, frame.at(0x144), frame.at(0xfc)});
        c.set(2, c.u32(frame.at(0x14)));  // the glue leaves the table of contents in the linkage area
        if (sext16(status) == 0) h[25] = 1;
        if (h[25] == 0) {
            run.call(fn::format, 0x102d8330, {frame.at(0x30c), pool});
            run.call(fn::format, 0x102d833c, {frame.at(0x244), pool + 0xa});
            run.call(fn::dialog, 0x102d834c, {frame.at(0x30c), frame.at(0x244), 1});
        }
    } while (h[25] == 0);

    // The four blocks.
    const auto allocate = [&](std::uint32_t size, std::uint32_t back, const ListSlots& list) {
        const auto block = run.call(fn::allocate, back, {size});
        c.put32(object + list.entries, block);
        c.set(4, list_growth);
        c.put32(object + list.capacity, list_growth);
        c.put32(object + list.count, 0);
    };
    allocate(0x1b58, 0x102d835c, retirement_list);
    allocate(0xbb8, 0x102d8378, queue_list);
    allocate(0xfa0, 0x102d8394, staff_pool_list);
    allocate(0xfa0, 0x102d83b0, player_pool_list);
    run.direct(0x102d83cc, {{3, object}, {4, list_growth}}, [&] { init_nation_club_lists(c, object); });
    if (archive == 0) {
        const auto incoming_r19 = c.r(19);
        run.direct(0x102d83dc, {{3, object}}, [&] { scan_initial_retirements(c, object, incoming_r19); });
        for (h[22] = 0; std::int32_t(h[22]) < 10; ++h[22]) {
            const auto first = c.u32(h[28]) - 1 - h[22];
            run.direct(0x102d83fc, {{3, object}, {4, first}, {5, 0xffffffffu}, {6, 0xffffffffu}},
                       [&] { append_staff_pool(c, object, first, 0xffffffffu, 0xffffffffu); });
        }
    } else {
        run.direct(0x102d8418, {{3, object}, {4, archive}}, [&] { load_regen_file(c, object, archive); });
    }
    // A missing block ends the game, after the assertion of its line.
    struct Check { std::uint32_t slot, at, name, extension, line, exit_back; };
    for (const Check& check : {Check{0x04, 0x102d8438, 0xc8, 0x50, 0x7f, 0x102d847c}, Check{0x10, 0x102d849c, 0xa8, 0x48, 0x83, 0x102d84e0},
                               Check{0x1c, 0x102d8500, 0x88, 0x40, 0x87, 0x102d8544}, Check{0x3c, 0x102d8564, 0x68, 0x38, 0x8b, 0x102d85a8}}) {
        if (c.u32(object + check.slot) != 0) continue;
        guest::report_assertion(c, {check.at, pool + 0x37, pool + 0x48, pool + 0x55, frame.at(check.name), frame.at(check.extension), check.line},
                                [&](std::uint32_t entry, std::uint32_t back) { run.sync(); invoke(c, entry, back); });
        run.call(callee::quit, check.exit_back, {0xffffffffu});
    }
    c.set(3, object);
    return object;
}
}

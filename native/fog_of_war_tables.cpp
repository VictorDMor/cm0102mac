// fog_of_war.cpp: the table of sixteen slots and the knowledge objects, their lifetime and their file.
//
// "fog.dat" holds the preference byte, then for each of the sixteen slots a present word (0: none) and,
// for a present slot, the manager's person index and one 12-byte entry per person. Saved games older than
// version 0x3e036 have no such file: every human manager starts afresh. ZIPDIR.CPP does the transfers.
//
// Quirks kept from the original: the reader accepts a manager index equal to the number of people (the
// check is `index > count`); a failed allocation while reading leaves the file open; after a failure the
// slots keep pointing at the objects just destroyed; a failed write leaves the file open.
//
// These routines reach the file layer, the allocator and drawing, so each holds the original's
// non-volatile registers at its calls (CallRegisters); the comments name what the original keeps there.
#include "fog_of_war_detail.hpp"

namespace cm::aot::fog_of_war {
namespace {
// Destroy every slot's object (the slots keep their pointers). The original walks r26 over the slots
// with r25 counting.
void destroy_all(Cpu& c, CallRegisters& regs, std::uint32_t table) {
    for (std::uint32_t n = 0; n < slots; ++n) {
        regs.hold({{25, n}, {26, table + n * 4}});
        if (const auto knowledge = c.u32(table + n * 4)) { regs.apply(); destroy_knowledge(c, knowledge, 1); }
    }
    regs.hold({{25, slots}, {26, table + slots * 4}});
}
// The person row of the n-th human from the end of the people table.
std::uint32_t human_candidate(Cpu& c, std::uint32_t n) { return person_row(c, std::uint32_t(people_count(c)) - n - 1); }
}

std::uint32_t construct_table(Cpu& c, std::uint32_t table, std::uint32_t file) {
    const Frame frame(c, 0x106fd120, 0x50, {.first_gpr = 28});
    CallRegisters regs(c);
    regs.hold({{28, table}, {30, c.toc(toc::people_count)}, {31, c.toc(toc::people)}});  // r29: the caller's
    for (std::uint32_t n = 0; n < slots; ++n) c.put32(table + n * 4, 0);
    if (!fog_enabled(c) || !file) return table;
    regs.apply();
    if (read_table(c, table, file)) return table;
    // The file could not be read: every human starts afresh.
    for (std::uint32_t n = 0; n < slots; ++n) {
        regs.hold({{29, n}});
        if (const auto manager = human_candidate(c, n); c.u8(manager + person::human) == 1) { regs.apply(); add_manager(c, table, manager); }
    }
    return table;
}

std::uint32_t destroy_table(Cpu& c, std::uint32_t table, std::uint32_t deleting) {
    const Frame frame(c, 0x106fd230, 0x50, {.first_gpr = 28});
    CallRegisters regs(c);
    regs.hold({{28, table}, {29, deleting}});
    if (!table) return table;
    for (std::uint32_t n = 0; n < slots; ++n) {
        regs.hold({{30, n}, {31, table + n * 4}});
        if (const auto knowledge = c.u32(table + n * 4)) { regs.apply(); destroy_knowledge(c, knowledge, 1); }
    }
    regs.hold({{30, slots}, {31, table + slots * 4}});
    regs.apply();
    if (std::int16_t(deleting) > 0) invoke(c, callee::free_object, 0x106fd294, {table});
    return table;
}

bool read_table(Cpu& c, std::uint32_t table, std::uint32_t file) {
    const Frame frame(c, 0x106fd2c0, 0x150, {.first_gpr = 24});
    CallRegisters regs(c);
    regs.hold({{24, file}, {25, c.toc(toc::enabled)}, {26, table}, {27, c.toc(toc::success)}, {28, c.toc(toc::message)},
               {29, c.toc(toc::pool)}, {30, c.toc(toc::people_count)}, {31, c.toc(toc::people)}});
    constexpr std::int32_t present_at = 0x38, handle_at = 0x3c;
    const auto call = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        regs.apply();
        return invoke(c, entry, back, arguments);
    };
    const auto fail = [&](const CheckSite& site) { regs.apply(); report(c, frame, site); };
    const auto handle = [&] { return c.u32(frame.at(handle_at)); };
    const auto close = [&](std::uint32_t back) { call(callee::close, back, {frame.at(handle_at)}); };
    const auto version = std::int32_t(call(callee::version_magnitude, 0x106fd2fc, {c.u32(c.toc(toc::version))}));
    if (version < first_saved_version) {
        // A saved game from before the fog of war: every human starts afresh.
        for (std::uint32_t n = 0; n < slots; ++n) {
            regs.hold({{25, n}});
            if (const auto manager = human_candidate(c, n); c.u8(manager + person::human) == 1) { regs.apply(); add_manager(c, table, manager); }
        }
        return true;
    }
    if (!file) { fail({0x106fd36c, 0x110, 0x68, 0x97}); return false; }
    c.put32(frame.at(handle_at), call(callee::open, 0x106fd3c8, {file, c.toc(toc::pool) + text::file_name, 0, 0x16, 4}));
    if (!handle()) { fail({0x106fd3e8, 0xf0, 0x60, 0x9f}); return false; }
    if (std::int32_t(call(callee::read, 0x106fd43c, {c.toc(toc::enabled), 1, 1, handle()})) != 1) {
        fail({0x106fd458, 0xd0, 0x58, 0xa5});
        return false;
    }
    if (!fog_enabled(c)) { close(0x106fd4b0); return true; }
    for (std::uint32_t n = 0; n < slots; ++n) {
        const auto slot = table + n * 4;
        regs.hold({{25, n}, {30, slot}});  // r24 keeps the last object made (the file before the first)
        if (std::int32_t(call(callee::read, 0x106fd4d8, {frame.at(present_at), 4, 1, handle()})) != 1) {
            fail({0x106fd4f4, 0xb0, 0x50, 0xb6});
            return false;
        }
        if (!c.u32(frame.at(present_at))) { c.put32(slot, 0); continue; }
        const auto knowledge = call(callee::allocate_object, 0x106fd558, {knowledge::size});
        regs.hold({{24, knowledge}});
        if (knowledge) { regs.apply(); construct_knowledge(c, knowledge, 0); }
        c.put32(slot, knowledge);
        if (!c.u32(slot)) {
            fail({0x106fd58c, 0x90, 0x48, 0xc2});
            destroy_all(c, regs, table);
            return false;
        }
        regs.apply();
        if (!read_knowledge(c, c.u32(slot), handle())) {
            fail({0x106fd620, 0x70, 0x40, 0xcf});
            destroy_all(c, regs, table);
            close(0x106fd68c);
            return false;
        }
    }
    regs.hold({{25, slots}, {30, table + slots * 4}});
    close(0x106fd6b0);
    return true;
}

void write_table(Cpu& c, std::uint32_t table, std::uint32_t file) {
    const Frame frame(c, 0x106fd6d0, 0x130, {.first_gpr = 26});
    CallRegisters regs(c);
    regs.hold({{26, table}, {28, c.toc(toc::message)}, {29, c.toc(toc::pool)}, {30, c.toc(toc::enabled)}, {31, c.toc(toc::success)}});
    constexpr std::int32_t present_at = 0x3c, absent_at = 0x38, handle_at = 0x40;
    const auto call = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        regs.apply();
        return invoke(c, entry, back, arguments);
    };
    const auto fail = [&](const CheckSite& site) { regs.apply(); report(c, frame, site); };
    c.put32(frame.at(present_at), 1);
    c.put32(frame.at(absent_at), 0);
    const auto handle = [&] { return c.u32(frame.at(handle_at)); };
    if (!file) { fail({0x106fd720, 0xec, 0x64, 0xf3}); return; }
    c.put32(frame.at(handle_at), call(callee::open, 0x106fd778, {file, c.toc(toc::pool) + text::file_name, 1, 0x16, 4}));
    if (!handle()) { fail({0x106fd798, 0xcc, 0x5c, 0xfb}); return; }
    if (std::int32_t(call(callee::write, 0x106fd7e8, {c.toc(toc::enabled), 1, 1, handle()})) != 1) {
        fail({0x106fd804, 0xac, 0x54, 0x101});
        return;
    }
    if (!fog_enabled(c)) { call(callee::close, 0x106fd858, {frame.at(handle_at)}); return; }
    for (std::uint32_t n = 0; n < slots; ++n) {
        const auto slot = table + n * 4;
        regs.hold({{27, n}, {30, slot}});
        if (c.u32(slot)) {
            if (std::int32_t(call(callee::write, 0x106fd884, {frame.at(present_at), 4, 1, handle()})) != 1) {
                fail({0x106fd8a0, 0x8c, 0x4c, 0x114});
                return;
            }
            regs.apply();
            write_knowledge(c, c.u32(slot), handle());
        } else if (std::int32_t(call(callee::write, 0x106fd904, {frame.at(absent_at), 4, 1, handle()})) != 1) {
            fail({0x106fd920, 0x6c, 0x44, 0x11d});
            return;
        }
    }
    regs.hold({{27, slots}, {30, table + slots * 4}});
    call(callee::close, 0x106fd978, {frame.at(handle_at)});
}

void age_table(Cpu& c, std::uint32_t table) {
    const Frame frame(c, 0x106fd990, 0x40, {.first_gpr = 30});
    for (std::uint32_t n = 0; n < slots; ++n)
        if (const auto knowledge = c.u32(table + n * 4)) age_knowledge(c, knowledge);
}

void forget_in_table(Cpu& c, std::uint32_t table, std::uint32_t person_at) {
    const Frame frame(c, 0x106fd9f0, 0x50, {.first_gpr = 29});
    if (!person_at || c.i32(person_at + person::id) >= people_count(c) || !fog_enabled(c)) return;
    for (std::uint32_t n = 0; n < slots; ++n)
        if (const auto knowledge = c.u32(table + n * 4)) forget(c, knowledge, person_at);
}

void add_manager(Cpu& c, std::uint32_t table, std::uint32_t manager) {
    const Frame frame(c, 0x106fda80, 0x50, {.first_gpr = 28});
    if (!manager) return;
    CallRegisters regs(c);
    // The slot's index, read before the allocation as the original does (r31).
    const auto index = std::uint32_t(people_count(c)) - 1 - c.u32(manager + person::id);
    regs.hold({{29, table}, {30, manager}, {31, index}});
    regs.apply();
    const auto knowledge = invoke(c, callee::allocate_object, 0x106fdac4, {knowledge::size});
    regs.hold({{28, knowledge}});
    regs.apply();
    if (knowledge) construct_knowledge(c, knowledge, manager);
    c.put32(table + index * 4, knowledge);
}

std::uint32_t knowledge_of_manager(Cpu& c, std::uint32_t table, std::uint32_t manager) {
    return manager ? c.u32(slot_of(c, table, manager)) : 0;
}

std::uint32_t construct_knowledge(Cpu& c, std::uint32_t knowledge, std::uint32_t manager) {
    const Frame frame(c, 0x106fdc90, 0x120, {.first_gpr = 24});
    CallRegisters regs(c);
    const auto gauge = c.toc(toc::progress);
    regs.hold({{24, manager}, {25, knowledge}, {29, c.toc(toc::today)}, {30, gauge}, {31, c.toc(toc::people_count)}});
    const auto call = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        regs.apply();
        return invoke(c, entry, back, arguments);
    };
    constexpr std::int32_t caption_at = 0x38;
    c.put32(knowledge + knowledge::entries, call(callee::allocate_block, 0x106fdcc0, {std::uint32_t(people_count(c)) * entry::size}));
    if (!c.u32(knowledge + knowledge::entries) || !manager) return knowledge;
    c.put32(knowledge + knowledge::manager, manager);
    const auto caption = frame.at(caption_at);
    call(callee::copy_text, 0x106fdcf4, {caption, c.toc(toc::pool) + text::initialising});
    call(callee::progress, 0x106fdd0c, {gauge, caption, 0, std::uint32_t(people_count(c)), 0xffffffffu});
    auto person_at = c.u32(c.toc(toc::people));
    // r24 holds the compiler's reciprocal of 1000 through the loop; r26 counts, r27 walks the rows, r28 the entries.
    // The count is read again at each test; the last reading is what the final report shows.
    for (std::int32_t n = 0;; ++n, person_at += person::size) {
        regs.hold({{24, 0x10624dd3}, {26, std::uint32_t(n)}, {27, person_at}, {28, std::uint32_t(n) * entry::size}});
        const auto count = people_count(c);
        if (n >= count) {
            call(callee::progress, 0x106fddc4, {gauge, caption, std::uint32_t(count), std::uint32_t(count), 0xffffffffu});
            return knowledge;
        }
        stamp_today(c, entry_of(c, knowledge, std::uint32_t(n)) + entry::seen);
        regs.apply();
        const auto level = initial_level(c, knowledge, c.u32(knowledge + knowledge::manager), person_at);
        c.put8(entry_of(c, knowledge, std::uint32_t(n)) + entry::level, std::uint32_t(level));
        if (n % 1000 == 0) call(callee::progress, 0x106fdd98, {gauge, caption, std::uint32_t(n), std::uint32_t(people_count(c)), 0xffffffffu});
    }
}

std::uint32_t destroy_knowledge(Cpu& c, std::uint32_t knowledge, std::uint32_t deleting) {
    const Frame frame(c, 0x106fdde0, 0x40, {.first_gpr = 30});
    CallRegisters regs(c);
    regs.hold({{30, knowledge}, {31, deleting}});
    if (!knowledge) return knowledge;
    if (const auto entries = c.u32(knowledge + knowledge::entries)) {
        regs.apply();
        invoke(c, callee::free_block, 0x106fde10, {entries});
        c.put32(knowledge + knowledge::entries, 0);
    }
    regs.apply();
    if (std::int16_t(deleting) > 0) invoke(c, callee::free_object, 0x106fde28, {knowledge});
    return knowledge;
}

bool read_knowledge(Cpu& c, std::uint32_t knowledge, std::uint32_t file) {
    const Frame frame(c, 0x106fde50, 0xd0, {.first_gpr = 26});
    CallRegisters regs(c);
    regs.hold({{26, knowledge}, {27, file}, {28, c.toc(toc::success)}, {29, c.toc(toc::message)}, {30, c.toc(toc::pool)},
               {31, c.toc(toc::people_count)}});
    regs.apply();
    constexpr std::int32_t index_at = 0x38;
    if (std::int32_t(invoke(c, callee::read, 0x106fde8c, {frame.at(index_at), 4, 1, file})) != 1) {
        report(c, frame, {0x106fdea8, 0x94, 0x4c, 0x22a});
        return false;
    }
    const auto index = c.i32(frame.at(index_at));
    if (index < 0 || index > people_count(c)) { report(c, frame, {0x106fdf18, 0x74, 0x44, 0x22f}); return false; }
    c.put32(knowledge + knowledge::manager, person_row(c, std::uint32_t(index)));
    const auto read = std::int32_t(invoke(c, callee::read, 0x106fdf84,
                                          {c.u32(knowledge + knowledge::entries), entry::size, std::uint32_t(people_count(c)), file}));
    if (people_count(c) != read) { report(c, frame, {0x106fdfa4, 0x54, 0x3c, 0x238}); return false; }
    return true;
}

void write_knowledge(Cpu& c, std::uint32_t knowledge, std::uint32_t file) {
    const Frame frame(c, 0x106fe000, 0xb0, {.first_gpr = 26});
    CallRegisters regs(c);
    regs.hold({{26, knowledge}, {27, file}, {28, c.toc(toc::success)}, {29, c.toc(toc::message)}, {30, c.toc(toc::pool)},
               {31, c.toc(toc::people_count)}});
    regs.apply();
    constexpr std::int32_t index_at = 0x38;
    c.put32(frame.at(index_at), c.u32(c.u32(knowledge + knowledge::manager) + person::id));
    if (std::int32_t(invoke(c, callee::write, 0x106fe048, {frame.at(index_at), 4, 1, file})) != 1) {
        report(c, frame, {0x106fe064, 0x6c, 0x44, 0x24e});
        return;
    }
    const auto written = std::int32_t(invoke(c, callee::write, 0x106fe0b8,
                                             {c.u32(knowledge + knowledge::entries), entry::size, std::uint32_t(people_count(c)), file}));
    if (people_count(c) != written) report(c, frame, {0x106fe0d8, 0x4c, 0x3c, 0x255});
}
}

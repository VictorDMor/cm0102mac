// PLAYER_REGEN.CPP, block `persistence`: the regen object's file in a saved game, "retire.dat" inside the save's archive (the zip entry
// functions 0x107475b0 open, 0x10747cc0 write, 0x10747a80 read, 0x10747900 close, which stay the game's). The file is, for each of the
// lists A (0x1c-byte entries), B (0xc), C (0x10) and D (0x10) in that order, the list's count as a 4-byte word followed by `count` entries
// of the entry size, straight from the list's block; nothing else (no header, no version). The reader sets each list's capacity to the
// count plus 250 entries and reallocates the block (0x107751b0) before reading the entries.
//
// Failures follow the original exactly. Every failed call reports the game's assertion (the line is the call's, 0x1a46.. for the writer,
// 0x1a9d.. for the reader), which clears the success flag (TOC -0x7bac) and shows the dialog; then the writer closes the file and CARRIES ON
// writing through the handle's variable (the stack word at +0x38, which the close routine receives by address) and the reader closes and
// returns 0. A missing archive or file is also a reported failure (a save without retire.dat fails to load with the alert).
#include "player_regen_detail.hpp"

namespace cm::aot::player_regen {
namespace {
// One assertion site: the address of its `bl 0x10739890`, the two stack buffers of the report, the source line.
struct Site { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
// One list of the file: where its count and block sit in the regen object, and the entry size.
struct FileList { std::uint32_t count, entries, size, capacity; };
constexpr FileList lists[4]{{0x08, 0x04, 0x1c, 0x0c}, {0x14, 0x10, 0x0c, 0x18}, {0x20, 0x1c, 0x10, 0x24}, {0x40, 0x3c, 0x10, 0x44}};

constexpr Site save_sites[10]{{0x102ea8b8, 0x1ac, 0x84, 0x1a46}, {0x102ea930, 0x18c, 0x7c, 0x1a4e}, {0x102ea9a0, 0x16c, 0x74, 0x1a55},
                              {0x102eaa20, 0x14c, 0x6c, 0x1a5c}, {0x102eaa94, 0x12c, 0x64, 0x1a63}, {0x102eab14, 0x10c, 0x5c, 0x1a6a},
                              {0x102eab88, 0xec, 0x54, 0x1a71},  {0x102eac08, 0xcc, 0x4c, 0x1a78},  {0x102eac7c, 0xac, 0x44, 0x1a7f},
                              {0x102eacfc, 0x8c, 0x3c, 0x1a86}};
// The writes' return addresses: the file open, then count and entries of each list, the last close.
constexpr std::uint32_t save_open_back = 0x102ea910, save_count_back[4]{0x102ea984, 0x102eaa78, 0x102eab6c, 0x102eac60},
                        save_entries_back[4]{0x102eaa00, 0x102eaaf4, 0x102eabe8, 0x102eacdc}, save_close_back = 0x102ead48;

constexpr Site load_sites[14]{{0x102eadb8, 0x24c, 0xa4, 0x1a9d}, {0x102eae34, 0x22c, 0x9c, 0x1aa5}, {0x102eaea8, 0x20c, 0x94, 0x1aac},
                              {0x102eaf34, 0x1ec, 0x8c, 0x1ab6}, {0x102eafb8, 0x1cc, 0x84, 0x1abe}, {0x102eb034, 0x1ac, 0x7c, 0x1ac7},
                              {0x102eb0c0, 0x18c, 0x74, 0x1ad1}, {0x102eb144, 0x16c, 0x6c, 0x1ad9}, {0x102eb1c0, 0x14c, 0x64, 0x1ae2},
                              {0x102eb24c, 0x12c, 0x5c, 0x1aec}, {0x102eb2d0, 0x10c, 0x54, 0x1af4}, {0x102eb34c, 0xec, 0x4c, 0x1afd},
                              {0x102eb3d8, 0xcc, 0x44, 0x1b07},  {0x102eb45c, 0xac, 0x3c, 0x1b0f}};
constexpr std::uint32_t load_open_back = 0x102eae14, load_count_back[4]{0x102eae8c, 0x102eb018, 0x102eb1a4, 0x102eb330},
                        load_alloc_back[4]{0x102eaf10, 0x102eb09c, 0x102eb228, 0x102eb3b4},
                        load_entries_back[4]{0x102eaf98, 0x102eb124, 0x102eb2b0, 0x102eb43c}, load_close_back = 0x102eb4b0;
constexpr std::uint32_t handle_slot = 0x38;  // the frame word the open call's result is kept in; the close call gets its address

constexpr std::uint32_t zip_open = 0x107475b0, zip_write = 0x10747cc0, zip_read = 0x10747a80, zip_close = 0x10747900;
}

void save_regen_file(Cpu& c, std::uint32_t regen, std::uint32_t archive) {
    // 0x102ea870: the frame saves r28-r31 (stw), 0x1e0 bytes. r28 the regen object, r29 the success flag's slot word, r30 the message
    // buffer's, r31 the unit's string pool.
    Frame frame(c, 0x102ea870, 0x1e0, {.first_gpr = 28});
    const auto pool = c.toc(toc::pool);
    c.set({{28, regen}, {29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, pool}});
    const auto fail = [&](const Site& site) {
        guest::report_assertion(c, {site.at, pool + 0x37, pool + 0x48, pool + 0x55, frame.at(site.name), frame.at(site.extension), site.line});
    };
    // A failed write also closes the file (the handle's variable by address); the next write goes on with what the variable holds then.
    const auto fail_and_close = [&](const Site& site) {
        fail(site);
        invoke(c, zip_close, site.at + 0x44, {frame.at(handle_slot)});
    };
    if (archive == 0) { fail(save_sites[0]); return; }
    const auto handle = invoke(c, zip_open, save_open_back, {archive, pool + 0x4238, 1, 0x16, 4});
    c.put32(frame.at(handle_slot), handle);
    if (handle == 0) { fail(save_sites[1]); return; }
    // The first write takes the handle straight from the call's result, the others read the variable again.
    bool first = true;
    for (unsigned n = 0; n < 4; ++n) {
        const auto& list = lists[n];
        const auto file = first ? handle : c.u32(frame.at(handle_slot));
        first = false;
        if (std::int32_t(invoke(c, zip_write, save_count_back[n], {regen + list.count, 4, 1, file})) != 1) fail_and_close(save_sites[2 + 2 * n]);
        const auto count = c.i32(regen + list.count);
        if (count != 0) {
            const auto written = invoke(c, zip_write, save_entries_back[n], {c.u32(regen + list.entries), list.size, std::uint32_t(count), c.u32(frame.at(handle_slot))});
            if (c.i32(regen + list.count) != std::int32_t(written)) fail_and_close(save_sites[3 + 2 * n]);
        }
    }
    invoke(c, zip_close, save_close_back, {frame.at(handle_slot)});
}

std::uint32_t load_regen_file(Cpu& c, std::uint32_t regen, std::uint32_t archive) {
    // 0x102ead70: as the writer, 0x280 bytes.
    Frame frame(c, 0x102ead70, 0x280, {.first_gpr = 28});
    const auto pool = c.toc(toc::pool);
    c.set({{28, regen}, {29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, pool}});
    const auto fail = [&](const Site& site) {
        guest::report_assertion(c, {site.at, pool + 0x37, pool + 0x48, pool + 0x55, frame.at(site.name), frame.at(site.extension), site.line});
    };
    const auto fail_and_close = [&](const Site& site) {
        fail(site);
        invoke(c, zip_close, site.at + 0x44, {frame.at(handle_slot)});
        return 0u;
    };
    if (archive == 0) { fail(load_sites[0]); return 0; }
    const auto handle = invoke(c, zip_open, load_open_back, {archive, pool + 0x4238, 0, 0x16, 4});
    c.put32(frame.at(handle_slot), handle);
    if (handle == 0) { fail(load_sites[1]); return 0; }
    bool first = true;
    for (unsigned n = 0; n < 4; ++n) {
        const auto& list = lists[n];
        const auto file = first ? handle : c.u32(frame.at(handle_slot));
        first = false;
        if (std::int32_t(invoke(c, zip_read, load_count_back[n], {regen + list.count, 4, 1, file})) != 1) return fail_and_close(load_sites[2 + 3 * n]);
        // The capacity is the count plus 250 entries; the block is reallocated to it (the size wraps in a word for a wild count).
        c.put32(regen + list.capacity, c.u32(regen + list.count) + list_growth);
        const auto block = invoke(c, callee::reallocate, load_alloc_back[n], {c.u32(regen + list.entries), c.u32(regen + list.capacity) * list.size});
        c.put32(regen + list.entries, block);
        if (c.u32(regen + list.entries) == 0) return fail_and_close(load_sites[3 + 3 * n]);
        const auto count = c.i32(regen + list.count);
        // The original reloads the block into r3 and the count into r5 before testing it (what the close call, or a skipped read, leaves).
        c.set({{3, c.u32(regen + list.entries)}, {5, std::uint32_t(count)}});
        if (count != 0) {
            const auto read = invoke(c, zip_read, load_entries_back[n], {c.u32(regen + list.entries), list.size, std::uint32_t(count), c.u32(frame.at(handle_slot))});
            if (c.i32(regen + list.count) != std::int32_t(read)) return fail_and_close(load_sites[4 + 3 * n]);
        }
    }
    invoke(c, zip_close, load_close_back, {frame.at(handle_slot)});
    return 1;
}
}

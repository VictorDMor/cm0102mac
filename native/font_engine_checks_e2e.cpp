// The font engine, end to end: the original engine against the build with the C++ entries, on what the game asks of it.
//
// The game's own font code (FONT.CPP, generated in both runs) drives the engine: for each font file a UI state holds (the
// snapshot's font records, found by the engine's stamps) a fresh record is built as FONT.CPP builds one (the memory object
// 0x10767460, the stream over a copy of the file 0x107631c0), then for each (style flags, size) the font code's own selector
// 0x10750320 creates the scaler, 0x10750580 renders and caches every character 0x20..0xff, and the engine's character
// entry 0x10760e30 renders every character the font's cmap maps. What each glyph hands back (its metrics, its bitmap's
// hash and the bitmap's address) is logged into guest memory; the run's dirty pages (the engine's heap, the glyph caches, the
// log) and the allocator tables are compared between the two runs byte for byte, then the record is released through the
// font code's own 0x10750200. The two runs are two forked children: the original with every hook declining, the other with the
// routed entries (the production path). A run reports the engine entries the dispatcher saw, the sizes and styles it covered.
//
// The hinted mode (--end-to-end-hinted, font_engine_end_to_end_hinted): the same two runs, but the glyphs are rendered with the hinter on, through the
// engine's own entries (the game never asks: FONT.CPP passes hint = 0 and cmd = 2). For each style set and size the font code's selector makes the
// scaler as before, then for each (hint flag, command, quality) variant the hinter is released (purge level 2) and the font's global hint data
// (sfnt + 0x38) is released, so the variant makes both again; T2K_NewTransformation 0x1075eef0 is called with the variant's hint flag, and the engine's
// character entry 0x10760e30 with its command on each character code the font maps. The commands reach the hinter through prepare_hinting
// 0x1075eb10: a grid bit (cmd & 0x281) makes the glyph go through the autogridder 0x107552b0; hint = 1 prepares it at the transformation.
#include "font_engine_checks.hpp"
#include "allocator_adapter.hpp"
#include "application_exit.hpp"
#include "font_engine_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace cm::aot { extern void (*font_engine_leaver_probe)(State&, std::uint32_t); }
namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t game_toc = 0x108bba90;
// The engine's stamps: the scaler (T2K.C: first word and the word at +0x5f4) and the font record FONT.CPP keeps (0x24 bytes).
constexpr std::uint32_t scaler_stamp = 0x5a1234a5, scaler_end_stamp = 0xa5fedc5a;
constexpr std::uint32_t log_base = 0x62000000, log_size = 0x10000000, work = scratch_base + 0x40000;

struct Font {
    std::uint32_t record = 0, data = 0, size = 0;
    std::set<std::pair<unsigned, unsigned>> used;  // (flags, size) of the glyphs its cache holds, and its current setting
};

std::uint32_t word(State& s, std::uint32_t at) { return std::uint32_t(s.memory.read<4>(at)); }

// The scalers in the heap, the font records that name them, and the (flags, size) their caches hold.
std::vector<Font> find_fonts(State& s) {
    std::set<std::uint32_t> scalers;
    for (const auto& [low, size] : s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x5f8 <= std::uint64_t(low) + size; at += 4)
            if (word(s, std::uint32_t(at)) == scaler_stamp && word(s, std::uint32_t(at) + 0x5f4) == scaler_end_stamp) scalers.insert(std::uint32_t(at));
    }
    std::cerr << "font engine scalers in the heap: " << scalers.size() << '\n';
    std::vector<Font> fonts;
    for (const auto& [low, size] : s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 4 <= std::uint64_t(low) + size; at += 4) {
            const auto value = word(s, std::uint32_t(at));
            if (!scalers.count(value) || at < low + 0x14) continue;
            const auto record = std::uint32_t(at) - 0x14;
            if (s.memory.read<1>(record) != 1 || word(s, record + 8) != word(s, value + 4) || word(s, record + 0x10) != word(s, value + 0x144)) continue;
            Font font{record, word(s, record + 4), word(s, word(s, record + 0xc) + 8), {}};
            font.used.insert({unsigned(word(s, record + 0x18)) & 0xffff, word(s, record + 0x1a) & 0xff});
            const auto cache = word(s, record + 0x1c), count = word(s, record + 0x20);
            for (std::uint32_t n = 0; n < count && n < 256; ++n) {
                const auto glyph = word(s, cache + n * 4);
                font.used.insert({unsigned(s.memory.read<2>(glyph + 0x1c)), unsigned(s.memory.read<1>(glyph + 0x1e))});
            }
            fonts.push_back(font);
        }
    }
    return fonts;
}

// The character codes a font's cmap maps (format 4, (3,1) or (3,0) or Unicode), from the file's bytes in guest memory.
std::vector<std::uint32_t> mapped_codes(State& s, const Font& font) {
    const auto u16 = [&](std::uint32_t at) { return std::uint32_t(s.memory.read<2>(font.data + at)); };
    std::uint32_t cmap = 0;
    for (std::uint32_t n = 0, tables = u16(4); n < tables; ++n)
        if (word(s, font.data + 12 + 16 * n) == 0x636d6170) cmap = word(s, font.data + 20 + 16 * n);
    std::vector<std::uint32_t> codes;
    if (!cmap) return codes;
    for (std::uint32_t n = 0, subtables = u16(cmap + 2); n < subtables; ++n) {
        const auto platform = u16(cmap + 4 + 8 * n), offset = cmap + word(s, font.data + cmap + 8 + 8 * n);
        if (platform != 3 && platform != 0) continue;
        if (u16(offset) != 4) continue;
        const auto segments = u16(offset + 6) / 2, ends = offset + 14, starts = ends + segments * 2 + 2;
        for (std::uint32_t seg = 0; seg < segments; ++seg)
            for (auto code = u16(starts + seg * 2); code <= u16(ends + seg * 2) && code != 0xffff; ++code) codes.push_back(code);
        break;
    }
    std::sort(codes.begin(), codes.end());
    codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
    return codes;
}

struct Plan {
    std::vector<unsigned> flags, sizes;
    bool codes_at_every_size = false;
    // The characters requested through the cache and the cmap codes rendered by the engine's entry (the error sweep uses a few).
    unsigned first_char = 0x20, last_char = 0xff;
    std::size_t code_limit = ~std::size_t(0);
    // The hinted mode: (hint flag of the transformation, render command, quality byte) and the step between the mapped codes rendered.
    bool hinted = false;
    struct Variant { unsigned hint, cmd, quality; };
    std::vector<Variant> variants;
    std::size_t code_stride = 1, code_first = 0;
};

// The error sweep (CM_FONT_ENGINE_E2E_FAULTS): a fault injected into the allocator at the n-th malloc TSIMEM.C's tsi_AllocMem makes
// in a run (its return address 0x107678e4), so that the engine's error exits run for real, through the generated API entries'
// setjmp: "oom" answers the malloc with null (error 0x2718); "header", "trailer" and "slot" spoil every block the table holds
// (the next release raises 0x2719 or 0x271f); "count" and "full" make the table full by count and by slots (0x271c, 0x2721).
struct Fault { std::string kind; unsigned nth = 0; };
Fault g_fault;
unsigned g_allocations = 0, g_fired = 0;
// What the two forked children saw, counted in memory they share with the parent: injections that fired and longjmp transfers.
struct Seen { unsigned fired, transfers, renders, errors, pinned; std::uint64_t dispatch[64], hooked[64]; };
Seen* seen = [] {
    void* shared = ::mmap(nullptr, sizeof(Seen), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if (shared == MAP_FAILED) throw std::runtime_error("mmap");
    return static_cast<Seen*>(shared);
}();
constexpr std::uint32_t alloc_malloc_return = 0x107678e4, game_malloc = 0x10775130, longjmp_entry = 0x10773f30;
bool inject_fault(State& s) {
    guest::Cpu c(s);
    if (s.pc == longjmp_entry) {
        __atomic_fetch_add(&seen->transfers, 1, __ATOMIC_SEQ_CST);
        if (std::getenv("CM_FONT_ENGINE_E2E_TRACE"))
            std::cerr << "longjmp code " << std::hex << c.r(4) << std::dec << " (" << g_fault.kind << ' ' << g_fault.nth << ")\n";
    }
    if (s.pc != game_malloc || c.lr() != alloc_malloc_return) return false;
    ++g_allocations;
    // tsi_AllocMem's frame is the sp at its malloc call: the return address into its caller sits at sp + 0x58.
    if (std::getenv("CM_FONT_ENGINE_E2E_TRACE") && g_fault.kind == "census") std::cerr << "allocation from " << std::hex << c.u32(c.sp() + 0x58) << std::dec << '\n';
    if (g_fault.kind.empty() || g_allocations != g_fault.nth) return false;
    ++g_fired;
    __atomic_fetch_add(&seen->fired, 1, __ATOMIC_SEQ_CST);
    const auto mem = c.r(30);  // tsi_AllocMem holds its object in r30
    if (g_fault.kind == "oom") {
        c.set(3, 0);
        s.pc = c.lr();
        return true;
    }
    const auto table = c.u32(mem + 0xc), capacity = c.u32(mem + 8);
    if (g_fault.kind == "count") { c.put32(mem + 4, capacity); return false; }
    for (std::uint32_t slot = 0; slot < capacity; ++slot) {
        const auto header = c.u32(table + 4 * slot);
        if (g_fault.kind == "full") { if (!header) c.put32(table + 4 * slot, 0x20000000 + 4 * slot); continue; }
        if (!header) continue;
        if (g_fault.kind == "header") c.put32(header, c.u32(header) ^ 1);
        else if (g_fault.kind == "trailer") c.put8(header + 0x10 + c.u32(header + 4), 0);
        else if (g_fault.kind == "trailer2") c.put8(header + 0x11 + c.u32(header + 4), 0);
        else if (g_fault.kind == "slot") c.put32(header + 8, (slot + 1) % capacity);
    }
    return false;
}
// The hinter's entries: GHINTS.C, HSTRAT97.C and AUTOGRID.C (blocks ghints, hstrat97, autogrid_a, autogrid_b).
constexpr std::uint32_t hinter_entries[] = {  // 54, as the audit lists them (sorted below)
    0x1075ac40, 0x1075acd0, 0x1075ad60, 0x1075ae30, 0x1075af60, 0x1075c980, 0x1075ca60, 0x1075cbc0,
    0x10751350, 0x10751b10, 0x10751bd0, 0x10751eb0, 0x10751fd0, 0x107522b0, 0x107524b0, 0x10752990, 0x10753110,
    0x10753560, 0x107536b0, 0x107537c0, 0x10753980, 0x10753a80, 0x10753ab0, 0x10753bf0, 0x10753c30, 0x10753c70, 0x10753cd0, 0x10753d20, 0x10753d50,
    0x10753d60, 0x10753d70, 0x10753e70, 0x10753f70, 0x10753fa0, 0x10753fd0, 0x10754050, 0x107540d0, 0x10754130, 0x10754190, 0x10754370, 0x10754550,
    0x107545b0, 0x10754610, 0x10754810, 0x107548d0, 0x10754990, 0x10754cd0, 0x10754f70, 0x10754ff0, 0x107551c0, 0x107552b0, 0x107555d0, 0x10755840,
    0x10755930};
// The index of a hinter entry in hinter_entries, or -1.
int hinter_index(std::uint32_t entry) {
    for (unsigned n = 0; n < std::size(hinter_entries); ++n) if (hinter_entries[n] == entry) return int(n);
    return -1;
}
std::map<std::uint32_t, std::uint64_t> dispatched;  // the engine entries the dispatcher saw in this child (written out at the end)
constexpr std::uint32_t hint_data_maker = 0x1075af60;
bool g_pin_stack = false, g_original_child = false;
bool probe(State& s) {
    if (inject_fault(s)) return true;
    if (s.pc >= module_start && s.pc < module_end) {
        ++dispatched[s.pc];
        if (g_original_child) if (const auto n = hinter_index(s.pc); n >= 0) __atomic_fetch_add(&seen->dispatch[n], 1, __ATOMIC_SEQ_CST);
    }
    // The hinter's own analysis (0x1075af60, for a font without a T2KG table or with a style callback) leaves bytes of the 0x6c buffer the caller passes in r5
    // unwritten, and the caller then copies the whole buffer (stack leftovers) into the font's hint data block: the bytes of the original's frame and of a
    // native frame differ. Both sides fill the buffer with one value before the analysis runs, so what it leaves unwritten is the same.
    if (g_pin_stack && s.pc == hint_data_maker) {
        guest::Cpu c(s);
        static const unsigned pin = std::getenv("CM_FONT_ENGINE_E2E_PIN_VALUE") ? unsigned(std::strtoul(std::getenv("CM_FONT_ENGINE_E2E_PIN_VALUE"), nullptr, 16)) : 0xa5;
        for (std::uint32_t n = 0; n < 0x6c; ++n) c.put8(c.r(5) + n, pin);
        if (g_original_child) __atomic_fetch_add(&seen->pinned, 1, __ATOMIC_SEQ_CST);
    }
    constexpr std::uint32_t exit_entry = 0x1076f7d0, dialog_entry = 0x10769b60;
    if (s.pc == exit_entry) throw ApplicationExit{};
    if (s.pc != dialog_entry) return false;
    s.pc = std::uint32_t(s.r<4>(4128));
    return true;
}

std::vector<Font> g_fonts;
Plan g_plan;
void (*previous_probe)(State&, std::uint32_t) = nullptr;
std::vector<std::vector<std::uint32_t>> g_codes;
// The sweep, in the current process (a forked child): `original` runs every hook declining.
void sweep(State& s, bool original) {
    const auto& fonts = g_fonts;
    const auto& plan = g_plan;
    const auto& codes = g_codes;
    font_engine_declining = original;
    routine_checker = nullptr;
    dispatch_probe = probe;
    s.budget = ~std::uint64_t(0) / 2;
    install_leaver_check(s);
    g_original_child = original;
    g_pin_stack = plan.hinted && !std::getenv("CM_FONT_ENGINE_E2E_NO_PIN");
    if (plan.hinted) {  // count the original's entries at every hooked routine too (a same-page call never reaches the dispatcher)
        previous_probe = font_engine_leaver_probe;
        font_engine_leaver_probe = [](State& state, std::uint32_t entry) {
            if (g_original_child) if (const auto n = hinter_index(entry); n >= 0) __atomic_fetch_add(&seen->hooked[n], 1, __ATOMIC_SEQ_CST);
            if (previous_probe) previous_probe(state, entry);
        };
    }
    guest::Cpu c(s);
    c.set(2, game_toc);
    for (unsigned n = 13; n < 32; ++n) c.set(n, 0x5a000000u + n * 0x10101u);
    std::uint32_t log = log_base;
    const auto put = [&](std::uint32_t value) { s.memory.write<4>(log, value); log += 4; };
    const auto call = [&](std::uint32_t target, std::initializer_list<std::uint32_t> arguments) {
        unsigned n = 3;
        for (const auto argument : arguments) c.set(n++, argument);
        c.call(target, return_sentinel);
        return c.r(3);
    };
    const auto error = work, cell = work + 8, record = work + 0x100;
    const auto hash_of = [&](std::uint32_t bitmap, std::uint32_t bytes) {
        std::uint64_t h = 0xcbf29ce484222325ull;
        for (std::uint32_t n = 0; n < bytes; ++n) h = (h ^ s.memory.read<1>(bitmap + n)) * 0x100000001b3ull;
        return h;
    };
    for (std::size_t index = 0; index < fonts.size(); ++index) {
        const auto& font = fonts[index];
        // FONT.CPP's own loading sequence over a fresh copy of the file's bytes.
        const auto data = call(0x107735f0, {font.size});
        for (std::uint32_t n = 0; n < font.size; ++n) s.memory.write<1>(data + n, s.memory.read<1>(font.data + n));
        for (std::uint32_t n = 0; n < 0x24; n += 4) s.memory.write<4>(record + n, 0);
        s.memory.write<4>(cell, record);
        s.memory.write<4>(record + 4, data);
        s.memory.write<4>(error, 0);
        const auto mem = call(0x10767460, {error});
        const auto stream = call(0x107631c0, {mem, data, font.size, error});
        if (std::getenv("CM_FONT_ENGINE_TRACE")) std::cerr << (original ? "original" : "rewrite") << ": font " << index << " data " << hex_address(data) << " mem " << hex_address(mem) << " stream " << hex_address(stream) << " error " << word(s, error) << '\n';
        s.memory.write<4>(record + 8, mem);
        s.memory.write<4>(record + 0xc, stream);
        s.memory.write<1>(record, 1);  // the record's first byte: loaded
        put(0xf0f00000u + std::uint32_t(index));
        put(word(s, error));
        if (plan.hinted) {
            const auto matrix = work + 0x40;
            for (const auto flags : plan.flags)
                for (const auto size : plan.sizes) {
                    call(0x10750320, {cell, flags, size});  // the font code's selector: the sfnt with the style's callbacks, the scaler, hint = 0
                    const auto scaler = word(s, record + 0x14);
                    const auto sfnt = word(s, scaler + 0x144), memory_object = word(s, scaler + 4);
                    const auto dump_data = [&](unsigned hint_flag) {
                        // CM_FONT_ENGINE_E2E_DATA_DUMP=file: the 0x6c bytes of the font's hint data after the transformation (the original's run), to compare two runs.
                        if (const char* dump = std::getenv("CM_FONT_ENGINE_E2E_DATA_DUMP"); dump && original && word(s, sfnt + 0x38)) {
                            std::ofstream out(dump, std::ios::app);
                            out << index << ' ' << flags << ' ' << size << ' ' << hint_flag << ':';
                            for (std::uint32_t n = 0; n < 0x6c; ++n) { char text[4]; std::snprintf(text, sizeof text, "%02x", unsigned(s.memory.read<1>(word(s, sfnt + 0x38) + n))); out << text; }
                            out << '\n';
                        }
                    };
                    for (const auto& variant : plan.variants) {
                        // A fresh hinter and fresh global hint data for each variant.
                        if (word(s, scaler + 0x148)) {
                            s.memory.write<4>(error, 0);
                            call(0x1075f980, {scaler, 2, error});
                            put(0xbeef0001u); put(word(s, error));
                        }
                        if (const auto data = word(s, sfnt + 0x38)) {
                            call(0x10767bb0, {memory_object, data});
                            s.memory.write<4>(sfnt + 0x38, 0);
                        }
                        // The matrix FONT.CPP builds: the size in 16.16 on the diagonal, a skew for the italic bits.
                        const std::uint32_t diagonal = (size & 0xff) << 16;
                        s.memory.write<4>(matrix, diagonal);
                        s.memory.write<4>(matrix + 4, flags & 4 ? diagonal / 4 : flags & 8 ? 0u - diagonal / 4 : 0u);
                        s.memory.write<4>(matrix + 8, 0);
                        s.memory.write<4>(matrix + 12, diagonal);
                        s.memory.write<4>(error, 0);
                        try { call(0x1075eef0, {scaler, variant.hint, 0x60, 0x60, matrix, 1, error}); }
                        catch (const std::exception& e) {
                            if (std::getenv("CM_FONT_ENGINE_E2E_TRACE") && original)
                                std::cerr << "stopped: " << e.what() << " in the transformation at pc " << hex_address(s.pc) << " flags " << flags << " size " << size << " hint " << variant.hint << '\n';
                            dump_data(variant.hint);
                            throw;
                        }
                        put(0xbeef0002u | variant.hint << 8); put(word(s, error));
                        for (const auto field : {0x12cu, 0x130u, 0x134u, 0x138u, 0x148u}) put(word(s, scaler + field));
                        if (word(s, error) != 0 && original) __atomic_fetch_add(&seen->errors, 1, __ATOMIC_SEQ_CST);
                        for (std::size_t n = plan.code_first, rendered = 0; n < codes[index].size() && rendered < plan.code_limit; n += plan.code_stride, ++rendered) {
                            const auto code = codes[index][n];
                            s.memory.write<4>(error, 0);
                            try { call(0x10760e30, {scaler, code, 0, 0, variant.quality, variant.cmd, error}); }
                            catch (const std::exception& e) {
                                if (std::getenv("CM_FONT_ENGINE_E2E_TRACE") && original)
                                    std::cerr << "stopped: " << e.what() << " at pc " << hex_address(s.pc) << " font " << index << " flags " << flags << " size " << size << " hint " << variant.hint
                                              << " cmd " << std::hex << variant.cmd << std::dec << " quality " << variant.quality << " code " << code << " lr " << hex_address(std::uint32_t(s.r<4>(4128))) << '\n';
                                dump_data(variant.hint);
                                throw;
                            }
                            if (original) __atomic_fetch_add(&seen->renders, 1, __ATOMIC_SEQ_CST);
                            if (word(s, error) != 0 && original) __atomic_fetch_add(&seen->errors, 1, __ATOMIC_SEQ_CST);
                            put(0xc0de0000u | code);
                            put(word(s, error));
                            for (const auto field : {0xa8u, 0xa4u, 0xd0u, 0xd8u, 0xd4u, 0x94u, 0x98u, 0x9cu, 0xa0u, 0xb0u, 0xb4u, 0xb8u, 0xbcu}) put(word(s, scaler + field));
                            const auto hash = hash_of(word(s, scaler + 0xdc), word(s, scaler + 0xd8) * word(s, scaler + 0xd4));
                            put(std::uint32_t(hash >> 32)); put(std::uint32_t(hash));
                            put(0);
                        }
                        dump_data(variant.hint);
                    }
                }
        } else
        for (const auto flags : plan.flags)
            for (const auto size : plan.sizes) {
                // The game's glyph path: the selector, then each character through the cache.
                call(0x10750320, {cell, flags, size});
                for (unsigned ch = plan.first_char; ch <= plan.last_char; ++ch) {
                    const auto glyph = call(0x10750580, {cell, flags, size, ch});
                    const auto width = word(s, glyph + 0x0c), rows = word(s, glyph + 0x10);
                    put(flags << 16 | size << 8 | ch);
                    for (const auto field : {0u, 4u, 8u, 0xcu, 0x10u, 0x14u, 0x18u}) put(word(s, glyph + field));
                    const auto hash = hash_of(word(s, glyph + 0x14), width * rows);
                    put(std::uint32_t(hash >> 32)); put(std::uint32_t(hash));
                    put(0);
                }
                // The engine's character entry on every code the cmap maps, at the sizes selected, through the scaler.
                if (plan.codes_at_every_size || size == plan.sizes.front()) {
                    const auto scaler = word(s, record + 0x14);
                    for (const auto code : std::vector<std::uint32_t>(codes[index].begin(), codes[index].begin() + std::ptrdiff_t(std::min(codes[index].size(), plan.code_limit)))) {
                        s.memory.write<4>(error, 0);
                        call(0x10760e30, {scaler, code, 0, 0, 5, 2, error});
                        put(0xc0de0000u | code);
                        put(word(s, error));
                        for (const auto field : {0xa8u, 0xa4u, 0xd0u, 0xd8u, 0xd4u, 0x94u}) put(word(s, scaler + field));
                        const auto hash = hash_of(word(s, scaler + 0xdc), word(s, scaler + 0xd8) * word(s, scaler + 0xd4));
                        put(std::uint32_t(hash >> 32)); put(std::uint32_t(hash));
                        put(0);
                    }
                }
            }
        // The interpreter mode (blocks fnt_a/b/c): synthetic programs on the font's hint font, if it has one (symbol.ttf's fpgm has run).
        if (!plan.hinted && !std::getenv("CM_FONT_ENGINE_E2E_NO_INTERPRETER"))
            if (const auto sfnt = word(s, record + 0x10))
                if (const auto hint = word(s, sfnt + 4))
                    for (const auto value : run_interpreter_programs(s, hint)) put(value);
        c.set(2, game_toc);
        call(0x10750200, {cell});
        put(0xf0f0ffffu);
    }
    put(0xdeadf00du);
    s.memory.write<4>(log_base + log_size - 8, log - log_base);
    // The census of what the dispatcher saw goes to a file (CM_FONT_ENGINE_CENSUS.original / .rewrite): it differs between the
    // runs by design (an owner called in place never reaches the dispatcher), so it is not part of the compared state.
    if (const char* file = std::getenv("CM_FONT_ENGINE_CENSUS")) {
        std::ofstream out(std::string(file) + (original ? ".original" : ".rewrite"));
        for (const auto& [entry, count] : dispatched) out << hex_address(entry) << ' ' << count << '\n';
    }
    if (std::getenv("CM_FONT_ENGINE_E2E_TRACE")) std::cerr << (original ? "original" : "rewrite") << ": allocations " << g_allocations << " fired " << g_fired << '\n';
    c.set(3, 0);
}
void sweep_original(State& s, std::uint32_t) { sweep(s, true); }
void sweep_rewrite(State& s, std::uint32_t) { sweep(s, false); }

// The error sweep: "kind:n" or "kind:from-to", comma separated. Each injection runs the sweep twice from the same state (the original, the
// routed build) and compares what they leave. The allocator's malloc is made to dispatch on both sides (State::allocator_entry_original)
// so the injection reaches both. A run that faults ends in the same fault on both sides or counts as a difference: a fault with the same
// message in both is counted apart ("faulted alike"), its memory compared like any other.
int run_fault_sweep(State& s, const std::string& spec) {
    const auto malloc_switch = std::ranges::find(allocator_entries, game_malloc) - allocator_entries.begin();
    s.allocator_entry_original[std::size_t(malloc_switch)] = true;
    unsigned runs = 0, identical = 0, alike = 0, different = 0, not_reached = 0, no_transfer = 0;
    std::map<std::string, unsigned> by_kind;
    std::size_t from = 0;
    while (from < spec.size()) {
        auto to = spec.find(',', from);
        if (to == std::string::npos) to = spec.size();
        const auto item = spec.substr(from, to - from);
        from = to + 1;
        const auto colon = item.find(':'), dash = item.find('-');
        const auto kind = item.substr(0, colon);
        const unsigned first = unsigned(std::stoul(item.substr(colon + 1))), last = dash == std::string::npos ? first : unsigned(std::stoul(item.substr(dash + 1)));
        for (unsigned nth = first; nth <= last; ++nth) {
            g_fault = {kind, nth};
            *seen = {};
            auto lines = compare_routine_call(s, 0x10750580, sweep_original, sweep_rewrite, 0, false);
            ++runs;
            if (!seen->fired) { ++not_reached; continue; }
            if (!seen->transfers && kind != "count" && kind != "full") ++no_transfer;
            // Both sides stopped at the same fault: drop the pair of lines, keep the comparison of everything else.
            std::string original_stop, rewrite_stop;
            for (const auto& line : lines) {
                if (line.starts_with("the original stopped: ")) original_stop = line.substr(22);
                if (line.starts_with("the rewrite stopped: ")) rewrite_stop = line.substr(21);
            }
            const bool faulted_alike = !original_stop.empty() && original_stop == rewrite_stop;
            if (faulted_alike) std::erase_if(lines, [](const std::string& line) { return line.starts_with("the original stopped: ") || line.starts_with("the rewrite stopped: "); });
            if (lines.empty()) { ++identical; alike += faulted_alike; ++by_kind[kind]; continue; }
            ++different;
            std::cout << "difference at " << kind << ' ' << nth << ':';
            for (const auto& line : lines) std::cout << " | " << line;
            std::cout << '\n';
        }
    }
    std::cout << "error sweep: " << runs << " injections, " << identical << " identical (" << alike << " ended in the same fault), " << different
              << " different, " << not_reached << " past the run's last allocation, " << no_transfer << " without a longjmp\n";
    for (const auto& [kind, count] : by_kind) std::cout << "  " << kind << ": " << count << " identical\n";
    std::cout << "error sweep: " << (different ? "DIFFERENT" : "identical") << '\n';
    return different ? 1 : 0;
}
}

int run_end_to_end(State& s, bool list_only, bool exhaustive, bool corrupt, const std::vector<std::filesystem::path>& files, bool hinted) {
    auto fonts = find_fonts(s);
    // Font files given on the command line (the game's other shipped files): copied into a mapped area beside the snapshot's.
    constexpr std::uint32_t file_area = 0x64000000;
    std::uint32_t next_file = file_area;
    if (!files.empty()) s.memory.map(file_area, 0x100000);
    for (const auto& path : files) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Missing font file " + path.string());
        const std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        for (std::size_t n = 0; n < bytes.size(); ++n) s.memory.write<1>(next_file + std::uint32_t(n), std::uint8_t(bytes[n]));
        fonts.push_back(Font{0, next_file, std::uint32_t(bytes.size()), {}});
        next_file += std::uint32_t((bytes.size() + 15) & ~15u);
    }
    std::set<std::uint32_t> kept_data;
    std::vector<Font> distinct;
    std::set<std::pair<unsigned, unsigned>> used;
    for (const auto& font : fonts) {
        used.insert(font.used.begin(), font.used.end());
        // The same file in two records is rendered once: equal size and equal bytes.
        const auto same = std::ranges::any_of(distinct, [&](const Font& other) {
            if (other.size != font.size) return false;
            for (std::uint32_t n = 0; n < font.size; ++n) if (s.memory.read<1>(other.data + n) != s.memory.read<1>(font.data + n)) return false;
            return true;
        });
        if (!same) distinct.push_back(font);
    }
    if (const char* only = std::getenv("CM_FONT_ENGINE_E2E_FONT")) {  // one distinct file of the list (an exhaustive run takes minutes a file)
        const auto index = std::size_t(std::stoul(only));
        if (index >= distinct.size()) throw std::runtime_error("CM_FONT_ENGINE_E2E_FONT is past the distinct files");
        distinct = {distinct[index]};
    }
    std::cout << "fonts: " << fonts.size() << " records, " << distinct.size() << " distinct files\n";
    for (const auto& font : fonts) {
        std::cout << "  record " << hex_address(font.record) << " data " << hex_address(font.data) << " size " << font.size << " used (flags:size):";
        for (const auto& [flags, size] : font.used) std::cout << ' ' << std::hex << flags << std::dec << ':' << size;
        std::cout << '\n';
    }
    if (list_only || distinct.empty()) return distinct.empty() ? 1 : 0;
    Plan plan;
    std::set<unsigned> flag_set, size_set;
    if (!exhaustive) {
        flag_set = {0, 1, 2, 3, 0x10, 0x11};
        for (const auto& [flags, size] : used) { flag_set.insert(flags & 0x1f); if (size) size_set.insert(size); }
        for (const unsigned size : {6u, 8u, 10u, 12u, 14u, 16u, 20u, 24u, 32u, 48u, 72u}) size_set.insert(size);
    }
    if (exhaustive) {
        // CM_FONT_ENGINE_E2E_SLICE=k/n keeps the style sets whose number is k modulo n (a large font's exhaustive run takes longer than ten minutes whole).
        unsigned slice = 0, slices = 1;
        if (const char* text = std::getenv("CM_FONT_ENGINE_E2E_SLICE")) std::sscanf(text, "%u/%u", &slice, &slices);
        for (unsigned f = 0; f < 0x20; ++f) if (f % slices == slice) flag_set.insert(f);
        for (unsigned size = 6; size <= 72; ++size) size_set.insert(size);
        plan.codes_at_every_size = true;
    }
    if (hinted) {
        // The hinted plan: its own style sets and sizes (the selector's bits 0/1 give the bold and italic callbacks, which send the hinter's data through the
        // analysis 0x1075af60 even for a font with a T2KG table; bits 2/3 skew the matrix) and the (hint, command, quality) variants.
        flag_set.clear();
        size_set.clear();
        plan.hinted = true;
        plan.variants = {{0, 0x03, 5}, {1, 0x03, 5}, {1, 0x02, 5}, {0, 0x203, 5}, {1, 0x83, 5}, {0, 0x01, 5}, {0, 0x03, 0}, {0, 0x82, 5}, {0, 0x83, 5}};
        if (exhaustive) {
            unsigned slice = 0, slices = 1;
            if (const char* text = std::getenv("CM_FONT_ENGINE_E2E_SLICE")) std::sscanf(text, "%u/%u", &slice, &slices);
            for (unsigned f = 0; f < 0x10; ++f) if (f % slices == slice) flag_set.insert(f);
            for (unsigned size = 6; size <= 72; ++size) size_set.insert(size);
            plan.variants = {};
            for (const unsigned hint : {0u, 1u})
                for (const unsigned cmd : {0x01u, 0x02u, 0x03u, 0x81u, 0x82u, 0x83u, 0x201u, 0x203u, 0x283u, 0x13u})
                    for (const unsigned quality : {0u, 5u}) plan.variants.push_back({hint, cmd, quality});
        } else {
            flag_set = {0, 1, 2, 3, 4, 5, 8};
            for (const unsigned size : {6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 16u, 18u, 20u, 22u, 24u, 28u, 32u, 40u, 48u, 60u, 72u}) size_set.insert(size);
            plan.code_stride = 2;
        }
    }
    if (hinted) {
        // Overrides for a debugging or sliced run: CM_FONT_ENGINE_E2E_FLAGS=0,1 CM_FONT_ENGINE_E2E_SIZES=9,14 CM_FONT_ENGINE_E2E_VARIANTS=hint:cmd:quality,... (cmd in hex)
        // CM_FONT_ENGINE_E2E_STRIDE=n (every n-th mapped code) CM_FONT_ENGINE_E2E_FIRST=n (start at the n-th mapped code) CM_FONT_ENGINE_E2E_LIMIT=n (at most n codes after the start).
        const auto list = [](const char* name) {
            std::vector<unsigned> values;
            if (const char* text = std::getenv(name)) for (const char* at = text; *at;) { char* end; values.push_back(unsigned(std::strtoul(at, &end, 10))); at = *end ? end + 1 : end; }
            return values;
        };
        if (const auto values = list("CM_FONT_ENGINE_E2E_FLAGS"); !values.empty()) flag_set = std::set<unsigned>(values.begin(), values.end());
        if (const auto values = list("CM_FONT_ENGINE_E2E_SIZES"); !values.empty()) size_set = std::set<unsigned>(values.begin(), values.end());
        if (const char* text = std::getenv("CM_FONT_ENGINE_E2E_VARIANTS")) {
            plan.variants.clear();
            unsigned hint, cmd, quality; int used = 0;
            for (const char* at = text; std::sscanf(at, "%u:%x:%u%n", &hint, &cmd, &quality, &used) == 3;) {
                plan.variants.push_back({hint, cmd, quality});
                at += used;
                if (*at == ',') ++at;
            }
        }
        if (const auto values = list("CM_FONT_ENGINE_E2E_STRIDE"); !values.empty()) plan.code_stride = values[0];
        if (const auto values = list("CM_FONT_ENGINE_E2E_FIRST"); !values.empty()) plan.code_first = values[0];
        if (const auto values = list("CM_FONT_ENGINE_E2E_LIMIT"); !values.empty()) plan.code_limit = values[0];
    }
    plan.flags.assign(flag_set.begin(), flag_set.end());
    plan.sizes.assign(size_set.begin(), size_set.end());
    const char* faults = std::getenv("CM_FONT_ENGINE_E2E_FAULTS");
    if (faults) {  // the error sweep: a short plan (the first allocations are where each error lands)
        plan = Plan{.flags = {0, 1, 2}, .sizes = {14}, .first_char = 0x41, .last_char = 0x43, .code_limit = 3};
    }
    g_fonts = distinct;
    g_plan = plan;
    for (const auto& font : distinct) g_codes.push_back(mapped_codes(s, font));
    std::size_t mapped = 0;
    for (const auto& list : g_codes) mapped += list.size();
    if (hinted)
        std::cout << "plan (hinted): " << plan.flags.size() << " style sets, " << plan.sizes.size() << " sizes, " << distinct.size() << " fonts, " << plan.variants.size()
                  << " (hint, command, quality) variants, every " << plan.code_stride << (plan.code_stride == 1 ? "st" : "nd") << " of " << mapped << " mapped codes\n";
    else
        std::cout << "plan: " << plan.flags.size() << " style sets, " << plan.sizes.size() << " sizes, " << distinct.size() << " fonts, "
                  << 224 * plan.flags.size() * plan.sizes.size() * distinct.size() << " cached glyph requests, " << mapped << " mapped codes\n";
    s.memory.map(log_base, log_size);
    routine_check_native_contract = true;  // the entries are compared one by one in the cases; here what the runs leave in memory
    if (faults) return run_fault_sweep(s, faults);
    if (hinted) {
        // One comparison per (font, variant): the hinter faults on a font that has nothing to measure (symbol.ttf has no letter glyphs: its analysis stores zero
        // heights and the first size setup divides by them, in the original and in the production run alike), and a fault ends a run, so a variant that faults
        // is compared up to the fault ("ended in the same fault", the memory both leave still compared) and does not stop the others.
        const auto variants = plan.variants;
        const auto fonts_now = distinct;
        const auto codes_now = g_codes;
        unsigned runs = 0, identical = 0, alike = 0, different = 0;
        std::uint64_t renders = 0, errors = 0, pinned = 0;
        std::map<std::string, std::string> by_font;  // "font n": a summary of the variants that faulted
        for (std::size_t index = 0; index < fonts_now.size(); ++index)
            for (const auto& variant : variants) {
                g_fonts = {fonts_now[index]};
                g_codes = {codes_now[index]};
                g_plan.variants = {variant};
                const auto reach_dispatch = std::vector<std::uint64_t>(seen->dispatch, seen->dispatch + 64), reach_hooked = std::vector<std::uint64_t>(seen->hooked, seen->hooked + 64);
                *seen = {};
                std::copy(reach_dispatch.begin(), reach_dispatch.end(), seen->dispatch);
                std::copy(reach_hooked.begin(), reach_hooked.end(), seen->hooked);
                auto lines = compare_routine_call(s, 0x10750580, sweep_original, std::getenv("CM_FONT_ENGINE_E2E_SELF") ? sweep_original : sweep_rewrite, 0, corrupt);
                ++runs;
                renders += seen->renders; errors += seen->errors; pinned += seen->pinned;
                std::string original_stop, rewrite_stop;
                for (const auto& line : lines) {
                    if (line.starts_with("the original stopped: ")) original_stop = line.substr(22);
                    if (line.starts_with("the rewrite stopped: ")) rewrite_stop = line.substr(21);
                }
                const bool faulted_alike = !original_stop.empty() && original_stop == rewrite_stop;
                if (faulted_alike) std::erase_if(lines, [](const std::string& line) { return line.starts_with("the original stopped: ") || line.starts_with("the rewrite stopped: "); });
                char label[64];
                std::snprintf(label, sizeof label, "font %zu hint %u cmd %x quality %u", index, variant.hint, variant.cmd, variant.quality);
                if (lines.empty()) { ++identical; alike += faulted_alike; if (faulted_alike) by_font[std::to_string(index)] += std::string(" [") + label + ": " + original_stop + "]"; continue; }
                ++different;
                std::cout << "difference at " << label << ':';
                for (const auto& line : lines) std::cout << " | " << line;
                std::cout << '\n';
            }
        std::cout << "hinted runs: " << runs << " (font x variant), " << identical << " identical (" << alike << " ended in the same fault), " << different << " different; "
                  << renders << " glyph renders, " << errors << " non-zero error codes, " << pinned << " hint-data analyses with the stack buffer pinned\n";
        for (const auto& [font, list] : by_font) std::cout << "  ended in the same fault, font " << font << ":" << list << '\n';
        std::string reached, missed;
        for (unsigned n = 0; n < std::size(hinter_entries); ++n) {
            const auto count = std::max(seen->dispatch[n], seen->hooked[n]);
            if (count) reached += " " + hex_address(hinter_entries[n]) + ":" + std::to_string(count);
            else missed += " " + hex_address(hinter_entries[n]);
        }
        std::cout << "hinter entries reached by the original (dispatches or hook entries, all runs):" << reached << "\nhinter entries not seen:" << missed << '\n';
        std::cout << "end-to-end hinted: " << (different ? "DIFFERENT" : "identical") << '\n';
        return different ? 1 : 0;
    }
    // The two runs from the same state, compared: dirty pages (the engine's heap, the glyph caches, the log), the allocator
    // tables and the registers the runs leave.
    // CM_FONT_ENGINE_E2E_SELF=1: the original against the original (does a run repeat itself?).
    const auto differences = compare_routine_call(s, 0x10750580, sweep_original, std::getenv("CM_FONT_ENGINE_E2E_SELF") ? sweep_original : sweep_rewrite, 0, corrupt);
    for (const auto& line : differences) std::cout << "difference: " << line << '\n';
    std::cout << "end-to-end: " << (differences.empty() ? "identical" : "DIFFERENT") << '\n';
    return differences.empty() ? 0 : 1;
}
}

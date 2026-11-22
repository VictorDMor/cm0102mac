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
};

// The error sweep (CM_FONT_ENGINE_E2E_FAULTS): a fault injected into the allocator at the n-th malloc TSIMEM.C's tsi_AllocMem makes
// in a run (its return address 0x107678e4), so that the engine's error exits run for real, through the generated API entries'
// setjmp: "oom" answers the malloc with null (error 0x2718); "header", "trailer" and "slot" spoil every block the table holds
// (the next release raises 0x2719 or 0x271f); "count" and "full" make the table full by count and by slots (0x271c, 0x2721).
struct Fault { std::string kind; unsigned nth = 0; };
Fault g_fault;
unsigned g_allocations = 0, g_fired = 0;
// What the two forked children saw, counted in memory they share with the parent: injections that fired and longjmp transfers.
struct Seen { unsigned fired, transfers; };
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
std::map<std::uint32_t, std::uint64_t> dispatched;  // the engine entries the dispatcher saw in this child (written out at the end)
bool probe(State& s) {
    if (inject_fault(s)) return true;
    if (s.pc >= module_start && s.pc < module_end) ++dispatched[s.pc];
    constexpr std::uint32_t exit_entry = 0x1076f7d0, dialog_entry = 0x10769b60;
    if (s.pc == exit_entry) throw ApplicationExit{};
    if (s.pc != dialog_entry) return false;
    s.pc = std::uint32_t(s.r<4>(4128));
    return true;
}

std::vector<Font> g_fonts;
Plan g_plan;
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
        if (!std::getenv("CM_FONT_ENGINE_E2E_NO_INTERPRETER"))
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

int run_end_to_end(State& s, bool list_only, bool exhaustive, bool corrupt, const std::vector<std::filesystem::path>& files) {
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
    std::cout << "plan: " << plan.flags.size() << " style sets, " << plan.sizes.size() << " sizes, " << distinct.size() << " fonts, "
              << 224 * plan.flags.size() * plan.sizes.size() * distinct.size() << " cached glyph requests, " << mapped << " mapped codes\n";
    s.memory.map(log_base, log_size);
    routine_check_native_contract = true;  // the entries are compared one by one in the cases; here what the runs leave in memory
    if (faults) return run_fault_sweep(s, faults);
    // The two runs from the same state, compared: dirty pages (the engine's heap, the glyph caches, the log), the allocator
    // tables and the registers the runs leave.
    const auto differences = compare_routine_call(s, 0x10750580, sweep_original, sweep_rewrite, 0, corrupt);
    for (const auto& line : differences) std::cout << "difference: " << line << '\n';
    std::cout << "end-to-end: " << (differences.empty() ? "identical" : "DIFFERENT") << '\n';
    return differences.empty() ? 0 : 1;
}
}

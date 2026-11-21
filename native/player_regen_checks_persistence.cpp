// PLAYER_REGEN.CPP, block `persistence`: cases for the save (0x102ea870) and the load (0x102ead70) of "retire.dat". The regen object is the
// career's own (season-day: the word the TOC slot -0x7b28 holds). The zip entry functions are an in-memory file (Case::Archive: the bytes
// written, the calls made and their order, the handle variable all end in scratch memory, which the comparison sees), so:
//  - the serialized bytes: every save case's file is compared between the original and the rewrite, and checked against an independent
//    serialization written here from the format the listing shows (for each of the lists A, B, C and D in turn: the count as a 4-byte
//    word, then `count` entries of 0x1c, 0xc, 0x10 and 0x10 bytes from the list's block; nothing else);
//  - the round trip: the load cases read that reference image (the file a save writes) and check, in both runs, every list's count,
//    capacity (count + 250) and entries against it; with the first, original-save and rewrite-save are the same bytes and both loads read
//    them, so a save by either side loads by either;
//  - the failure paths: an absent archive, a failing open, a failing or short write (each call in turn, the handle variable zeroed by the
//    close or kept), a failing or short read, a refused allocation (each list in turn), truncated and wild files (a count the file cannot
//    hold, a negative count). Each reports the game's assertion (line by line), which the checker answers.
// Labels start with "persistence:" (CM_PLAYER_REGEN_BLOCK=persistence).
#include "player_regen_checks.hpp"
#include <array>
#include <cstdio>
#include <string>

namespace cm::aot::player_regen_checks {
namespace {
constexpr std::uint32_t save = 0x102ea870, load = 0x102ead70, allocator = 0x107751b0;
constexpr std::int32_t regen_slot = -0x7b28;
struct FileList { std::uint32_t count, entries, size, capacity; };
constexpr std::array<FileList, 4> lists{{{0x08, 0x04, 0x1c, 0x0c}, {0x14, 0x10, 0x0c, 0x18}, {0x20, 0x1c, 0x10, 0x24}, {0x40, 0x3c, 0x10, 0x44}}};

using Bytes = std::vector<std::uint8_t>;
void put_word(Bytes& to, std::uint32_t value) { for (int shift = 24; shift >= 0; shift -= 8) to.push_back(std::uint8_t(value >> shift)); }

// What a list holds: its count word and its entries' bytes.
struct Content { std::uint32_t count; Bytes entries; };
struct Image {
    std::array<Content, 4> lists;
    Bytes bytes() const {
        Bytes out;
        for (const auto& list : lists) { put_word(out, list.count); out.insert(out.end(), list.entries.begin(), list.entries.end()); }
        return out;
    }
};

// The career's regen object's lists as they are in memory, `keep` of them nonempty (bit n: list n keeps its entries; the others count 0).
Image current(const Scenario& s, std::uint32_t regen, unsigned keep) {
    Image image;
    for (std::size_t n = 0; n < 4; ++n) {
        const auto& list = lists[n];
        auto& content = image.lists[n];
        content.count = (keep >> n & 1) ? s.r32(regen + list.count) : 0;
        const auto block = s.r32(regen + list.entries);
        for (std::uint32_t at = 0; at < content.count * list.size; ++at) content.entries.push_back(std::uint8_t(s.u8(block + at)));
    }
    return image;
}
// The lists with the given counts (the entries are what the blocks hold: a block has room for 250), and the writes that set the counts.
Image counted(const Scenario& s, std::uint32_t regen, const std::array<std::uint32_t, 4>& counts) {
    Image image;
    for (std::size_t n = 0; n < 4; ++n) {
        image.lists[n].count = counts[n];
        const auto block = s.r32(regen + lists[n].entries);
        for (std::uint32_t at = 0; at < counts[n] * lists[n].size; ++at) image.lists[n].entries.push_back(std::uint8_t(s.u8(block + at)));
    }
    return image;
}
std::vector<Write> set_counts(std::uint32_t regen, const std::array<std::uint32_t, 4>& counts) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < 4; ++n) memory.push_back(w32(regen + lists[n].count, counts[n]));
    return memory;
}
std::vector<Write> preload(const Bytes& bytes) {
    std::vector<Write> memory{w32(archive_state + 0x18, std::uint32_t(bytes.size()))};
    for (std::size_t at = 0; at < bytes.size(); at += 4) {
        std::uint32_t word = 0;
        for (std::size_t k = 0; k < 4; ++k) word = word << 8 | (at + k < bytes.size() ? bytes[at + k] : 0u);
        memory.push_back(w32(archive_data + std::uint32_t(at), word));
    }
    return memory;
}
// The checks of the final state: the saved file is exactly `expected`; or the loaded lists hold exactly `image` (capacity count + 250).
std::function<bool(State&)> file_is(Bytes expected) {
    return [expected](State& s) {
        guest::Cpu c(s);
        if (c.u32(archive_state + 0x18) != expected.size()) return false;
        for (std::size_t at = 0; at < expected.size(); ++at) if (c.u8(archive_data + std::uint32_t(at)) != expected[at]) return false;
        return true;
    };
}
std::function<bool(State&)> lists_are(std::uint32_t regen, Image image) {
    return [regen, image](State& s) {
        guest::Cpu c(s);
        if (c.r(3) != 1) return false;
        for (std::size_t n = 0; n < 4; ++n) {
            const auto& list = lists[n];
            const auto& content = image.lists[n];
            if (c.u32(regen + list.count) != content.count || c.u32(regen + list.capacity) != content.count + 250) return false;
            const auto block = c.u32(regen + list.entries);
            for (std::size_t at = 0; at < content.entries.size(); ++at) if (c.u8(block + std::uint32_t(at)) != content.entries[at]) return false;
        }
        return true;
    };
}
std::vector<Write> counts_of(std::uint32_t regen, unsigned keep, const Scenario& s) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < 4; ++n) memory.push_back(w32(regen + lists[n].count, (keep >> n & 1) ? s.r32(regen + lists[n].count) : 0));
    return memory;
}
std::string number(std::size_t n) { return std::to_string(n); }
std::string counts_label(const std::array<std::uint32_t, 4>& counts) {
    return number(counts[0]) + "/" + number(counts[1]) + "/" + number(counts[2]) + "/" + number(counts[3]);
}
Case::Archive archive_with(bool fail_open = false, unsigned fail_write = 0, unsigned short_write = 0, unsigned fail_read = 0, unsigned short_read = 0, bool keep = false) {
    Case::Archive a;
    a.enabled = true; a.fail_open = fail_open; a.fail_write = fail_write; a.short_write = short_write; a.fail_read = fail_read; a.short_read = short_read;
    a.keep_handle = keep;
    return a;
}
}

PLAYER_REGEN_CASES("persistence") {
    // 0x102ea870. The first cases are the traced ones (only the first 32 calls of an entry are paired): the success, then every assertion
    // site once (no archive, no file, each of the eight writes failing alone: the handle variable kept).
    const auto regen = scenario.global(regen_slot);
    const auto image = current(scenario, regen, 15);
    std::fprintf(stderr, "persistence: regen object %x, counts A %u B %u C %u D %u, file %zu bytes\n", regen, image.lists[0].count, image.lists[1].count,
                 image.lists[2].count, image.lists[3].count, image.bytes().size());
    // B and D are empty in the career: five and three entries of what their blocks hold make all eight writes happen.
    const std::array<std::uint32_t, 4> rich{image.lists[0].count, 5, image.lists[2].count, 3};
    cases.push_back({.entry = save, .registers = {{3, regen}, {4, archive_state}}, .memory = set_counts(regen, rich), .archive = archive_with(),
                     .verify = file_is(counted(scenario, regen, rich).bytes()), .label = "persistence: save, counts " + counts_label(rich)});
    cases.push_back({.entry = save, .registers = {{3, regen}, {4, 0}}, .archive = archive_with(), .label = "persistence: save, no archive"});
    cases.push_back({.entry = save, .registers = {{3, regen}, {4, archive_state}}, .archive = archive_with(true), .label = "persistence: save, the open fails"});
    for (unsigned call = 1; call <= 8; ++call)
        cases.push_back({.entry = save, .registers = {{3, regen}, {4, archive_state}}, .memory = set_counts(regen, rich), .archive = archive_with(false, call, 0, 0, 0, true),
                         .label = "persistence: save, write " + number(call) + " fails, handle kept"});
    // Every combination of empty lists, against the independent serialization.
    for (unsigned keep = 15; keep != 0xff; keep = keep == 0 ? 0xff : keep - 1) {
        const auto expected = current(scenario, regen, keep).bytes();
        cases.push_back({.entry = save, .registers = {{3, regen}, {4, archive_state}}, .memory = counts_of(regen, keep, scenario), .archive = archive_with(),
                         .verify = file_is(expected), .label = "persistence: save, lists kept " + number(keep) + ", " + number(expected.size()) + " bytes"});
    }
    for (const auto& counts : {std::array<std::uint32_t, 4>{1, 1, 1, 1}, std::array<std::uint32_t, 4>{0, 2, 0, 2}}) {
        const auto expected = counted(scenario, regen, counts).bytes();
        cases.push_back({.entry = save, .registers = {{3, regen}, {4, archive_state}}, .memory = set_counts(regen, counts), .archive = archive_with(),
                         .verify = file_is(expected), .label = "persistence: save, counts " + counts_label(counts)});
    }
    // Each write failing (0 items) or short by one item, the handle variable zeroed by the close (the later writes then fail too) or kept.
    for (const bool keep : {false, true})
        for (unsigned call = 1; call <= 8; ++call) {
            if (!keep)
                cases.push_back({.entry = save, .registers = {{3, regen}, {4, archive_state}}, .memory = set_counts(regen, rich), .archive = archive_with(false, call, 0, 0, 0, false),
                                 .label = "persistence: save, write " + number(call) + " fails, handle zeroed"});
            cases.push_back({.entry = save, .registers = {{3, regen}, {4, archive_state}}, .memory = set_counts(regen, rich), .archive = archive_with(false, 0, call, 0, 0, keep),
                             .label = "persistence: save, write " + number(call) + " short" + (keep ? ", handle kept" : ", handle zeroed")});
        }
}

PLAYER_REGEN_CASES("persistence") {
    // 0x102ead70. Traced first: the success of the reference image, then every assertion site once (no archive, no file, each of the eight
    // reads failing alone, the allocator refusing each list's block).
    const auto regen = scenario.global(regen_slot);
    const std::array<std::uint32_t, 4> rich{current(scenario, regen, 15).lists[0].count, 5, current(scenario, regen, 15).lists[2].count, 3};
    const auto full = counted(scenario, regen, rich);
    cases.push_back({.entry = load, .registers = {{3, regen}, {4, archive_state}}, .memory = preload(full.bytes()), .archive = archive_with(),
                     .verify = lists_are(regen, full), .label = "persistence: load, the reference image"});
    cases.push_back({.entry = load, .registers = {{3, regen}, {4, 0}}, .memory = preload(full.bytes()), .archive = archive_with(), .label = "persistence: load, no archive"});
    cases.push_back({.entry = load, .registers = {{3, regen}, {4, archive_state}}, .memory = preload(full.bytes()), .archive = archive_with(true),
                     .label = "persistence: load, the open fails (no retire.dat)"});
    for (unsigned call = 1; call <= 8; ++call)
        cases.push_back({.entry = load, .registers = {{3, regen}, {4, archive_state}}, .memory = preload(full.bytes()), .archive = archive_with(false, 0, 0, call, 0),
                         .label = "persistence: load, read " + number(call) + " fails"});
    for (unsigned call = 1; call <= 4; ++call)
        cases.push_back({.entry = load, .registers = {{3, regen}, {4, archive_state}}, .memory = preload(full.bytes()), .nth_answers = {{allocator, call, 0}},
                         .archive = archive_with(), .original_owners = {allocator}, .label = "persistence: load, the allocator refuses list " + number(call)});
    // The round trip of every combination of empty lists and of small counts.
    for (unsigned keep = 15; keep != 0xff; keep = keep == 0 ? 0xff : keep - 1) {
        const auto kept = current(scenario, regen, keep);
        cases.push_back({.entry = load, .registers = {{3, regen}, {4, archive_state}}, .memory = preload(kept.bytes()), .archive = archive_with(),
                         .verify = lists_are(regen, kept), .label = "persistence: load, lists kept " + number(keep)});
    }
    for (const auto& counts : {std::array<std::uint32_t, 4>{1, 1, 1, 1}, std::array<std::uint32_t, 4>{0, 2, 0, 2}}) {
        const auto image = counted(scenario, regen, counts);
        cases.push_back({.entry = load, .registers = {{3, regen}, {4, archive_state}}, .memory = preload(image.bytes()), .archive = archive_with(),
                         .verify = lists_are(regen, image), .label = "persistence: load, counts " + counts_label(counts)});
    }
    for (unsigned call = 1; call <= 8; ++call)
        cases.push_back({.entry = load, .registers = {{3, regen}, {4, archive_state}}, .memory = preload(full.bytes()), .archive = archive_with(false, 0, 0, 0, call),
                         .label = "persistence: load, read " + number(call) + " short"});
    // Truncated files (the last read comes up short), a count the file cannot hold, a negative count.
    const auto bytes = full.bytes();
    for (const std::size_t cut : {4u, 8u, 0x1cu, 0x20u}) {
        if (bytes.size() <= cut) continue;
        Bytes shorter(bytes.begin(), bytes.end() - std::ptrdiff_t(cut));
        cases.push_back({.entry = load, .registers = {{3, regen}, {4, archive_state}}, .memory = preload(shorter), .archive = archive_with(),
                         .label = "persistence: load, file short by " + number(cut)});
    }
    for (const std::uint32_t wild : {0xffffffffu, 0x80000000u + 7, 500u, 100000u})
        for (std::size_t n = 0; n < 4; ++n) {
            auto image = counted(scenario, regen, rich);
            image.lists[n].count = wild;  // the count word only: the entries are as before
            cases.push_back({.entry = load, .registers = {{3, regen}, {4, archive_state}}, .memory = preload(image.bytes()), .archive = archive_with(),
                             .label = "persistence: load, list " + number(n) + " count " + std::to_string(wild)});
        }
}
}

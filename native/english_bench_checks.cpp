// Controlled copies of an existing capture, with real original serializers.
// All files go to disposable backing; no installed career or game data is edited.
#include "english_competition_rules.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "semantic_io_checks.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

using namespace cm::aot;
using guest::Cpu;
using cm::semantic_io_checks::require;
namespace {
constexpr std::uint32_t sentinel = 0xfffffff0;
void call(Cpu& c, std::uint32_t entry, std::initializer_list<std::uint32_t> args) {
    const auto sp = c.sp();
    unsigned reg = 3;
    for (auto value : args) c.set(reg++, value);
    c.call(entry, sentinel);
    require(c.s.pc == sentinel && c.sp() == sp, "Guest return/stack changed");
}
using Row = std::vector<std::uint8_t>;
using Rows = std::vector<Row>;
std::uint32_t word(const Row& row, unsigned offset, unsigned size) {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < size; ++i) value = (value << 8) | row.at(offset + i);
    return value;
}
Rows rows(Cpu& c, std::uint32_t manager) {
    Rows result;
    const auto bodies = c.u8(c.toc(-0x73f0));
    for (unsigned day = 0; day < 366; ++day)
        for (unsigned slot = 0; slot < 3; ++slot) {
            const auto lists = c.u32(manager + day * 12 + slot * 4);
            for (unsigned body = 0; body < bodies; ++body) {
                const auto list = c.u32(lists + body * 4);
                unsigned count = 0;
                for (auto node = c.u32(list + 4); node; node = c.u32(node + 8)) {
                    require(c.u16(node + 4) == 0x50, "Fixture payload size changed");
                    result.push_back(c.s.memory.copy_out(c.u32(node), 0x50));
                    require(++count <= unsigned(c.u16(list)), "Fixture list cycle/count");
                }
                require(count == c.u16(list), "Fixture list count changed");
            }
        }
    std::ranges::sort(result);
    return result;
}
std::uint32_t new_manager(Cpu& c, unsigned year) {
    call(c, 0x10773520, {0x1184});
    const auto manager = c.r(3); require(manager != 0, "Cannot allocate fixture manager");
    call(c, 0x10487ff0, {manager, year, 0, 0});
    if (c.r(3) != manager || !c.u8(c.toc(-0x7bac)))
        throw std::runtime_error("Fixture constructor/reader failed: year=" + std::to_string(year) +
            " object=" + hex_address(manager) + " result=" + hex_address(c.r(3)) +
            " success=" + std::to_string(c.u8(c.toc(-0x7bac))));
    return manager;
}
Row file_bytes(const std::filesystem::path& root, unsigned year) {
    const auto name = "fixtures_" + std::to_string(year) + ".tmp";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
        if (entry.is_regular_file() && entry.path().filename() == name) {
            std::ifstream input(entry.path(), std::ios::binary);
            return {std::istreambuf_iterator<char>(input), {}};
        }
    throw std::runtime_error("Real fixture writer did not create " + name);
}
void write(Cpu& c, std::uint32_t manager, const std::filesystem::path& root, unsigned count) {
    call(c, 0x1048a580, {manager, 0});
    require(c.r(3) == 1 && c.u8(c.toc(-0x7bac)), "Real fixture writer failed");
    const auto bytes = file_bytes(root, c.u16(manager + 0x1130));
    require(bytes.size() == 4 + count * 0x50 && word(bytes, 0, 4) == count,
            "Fixture binary count or record width changed");
}

void fixtures(Cpu& c, const std::filesystem::path& root) {
    const auto sample = rows(c, c.u32(c.toc(-0x7a60)));
    require(!sample.empty(), "Capture has no fixture template");
    const auto id = c.u32(c.toc(-0x67a0));
    const auto other = c.u32(c.toc(-0x67a4));
    require(id != 0xffffffffu && other != id, "Competition IDs unavailable");
    const auto definitions = c.u32(c.toc(-0x7bbc));
    call(c, 0x10773520, {0x50}); const auto fixture = c.r(3);
    const auto insert = [&](std::uint32_t manager, unsigned competition, unsigned year,
                            unsigned day, bool played, unsigned allowance) {
        c.s.memory.copy_in(fixture, sample.front());
        c.put32(fixture, competition);
        c.put32(fixture + 0x14, definitions + competition * 0x6c);
        c.put16(fixture + 0x28, year); c.put16(fixture + 0x2a, day);
        c.put8(fixture + 0x3f, 0); c.put8(fixture + 0x43, played ? 2 : 255);
        c.put8(fixture + 0x44, played ? 1 : 255);
        c.put8(fixture + 0x4e, 0x50 | allowance);
        call(c, 0x10489600, {manager, fixture});
        require(c.u8(c.toc(-0x7bac)), "Fixture insertion failed");
    };
    const auto source = new_manager(c, 2001);
    require(rows(c, source).empty(), "Backing is not empty");
    // Every low-nibble value must survive, including the usual three changes.
    for (unsigned allowance = 0; allowance < 16; ++allowance)
        insert(source, id, 2001, 100 + allowance, false, allowance);
    insert(source, id, 2001, 120, true, 3);
    insert(source, other, 2001, 121, false, 3);
    write(c, source, root, 18);
    const auto old_bytes = file_bytes(root, 2001);
    setenv("CM_ENGLISH_RULES_ORIGINAL", "1", 1);
    auto expected = rows(c, new_manager(c, 2001));
    unsetenv("CM_ENGLISH_RULES_ORIGINAL");
    require(expected.size() == 18, "Original fixture reader lost rows");
    for (auto& row : expected)
        if (word(row, 0, 4) == id && row[0x43] == 255) row[0x4e] = 0x70 | (row[0x4e] & 15);
    std::ranges::sort(expected);
    const auto loaded = new_manager(c, 2001);
    require(rows(c, loaded) == expected, "Only pending Premier bench nibbles may change");
    write(c, loaded, root, 18);
    auto expected_bytes = old_bytes;
    for (unsigned i = 0; i < 18; ++i) {
        const unsigned at = 4 + i * 0x50;
        if (word(expected_bytes, at, 4) == id && expected_bytes[at + 0x43] == 255)
            expected_bytes[at + 0x4e] = 0x70 | (expected_bytes[at + 0x4e] & 15);
    }
    require(file_bytes(root, 2001) == expected_bytes, "Serialized layout or unrelated fixture bytes changed");
    const auto again = new_manager(c, 2001);
    require(rows(c, again) == expected, "Fixture migration is not idempotent");
    write(c, again, root, 18);
    require(file_bytes(root, 2001) == expected_bytes, "Second save changed fixture bytes");
    const auto future = new_manager(c, 2002);
    insert(future, id, 2002, 100, false, 3); write(c, future, root, 1);
    call(c, 0x10451800, {definitions + id * 0x6c, 0});
    const auto body = c.r(3) & 255;
    call(c, 0x1048a430, {loaded, 2002, 100, 0, body});
    const auto lazy = rows(c, c.u32(loaded + 0x112c));
    require(lazy.size() == 1 && lazy.front()[0x4e] == 0x73, "Lazy year retained the old bench");
    std::cout << "Premier bench: real fixture read/write, all allowance nibbles, played/other exclusion, lazy year and exact save/reload passed\n";
}

// Capture all writer outputs so the compatibility assertion includes the real
// league serializer and its nested fixture-history files.
using Files = std::map<std::filesystem::path, Row>;
Files files(const std::filesystem::path& root) {
    Files result;
    for (const auto& e : std::filesystem::recursive_directory_iterator(root))
        if (e.is_regular_file()) {
            std::ifstream in(e.path(), std::ios::binary);
            result.emplace(e.path().lexically_relative(root), Row{std::istreambuf_iterator<char>(in), {}});
        }
    return result;
}
void leagues(Cpu& c, const std::filesystem::path& root) {
    for (const auto slot : {-0x67a0, -0x67a4}) {
        const auto id = c.u32(c.toc(slot));
        const auto league = c.u32(c.u32(c.toc(-0x7b5c)) + id * 4);
        require(league != 0, "Captured league is absent");
        c.put8(league + 0x46, 5);
        call(c, 0x104a8530, {league, 1});
        const auto load = [&](bool legacy) {
            call(c, 0x10773520, {0xec}); const auto object = c.r(3);
            call(c, 0x1049e4f0, {object});
            c.put32(object, c.u32(league)); c.put32(object + 4, c.u32(league + 4));
            if (legacy) setenv("CM_ENGLISH_RULES_ORIGINAL", "1", 1);
            call(c, 0x104a6850, {object, 1});
            unsetenv("CM_ENGLISH_RULES_ORIGINAL");
            require(c.r(3) == 1, "League reader failed");
            require(c.u8(object + 0x46) == (slot == -0x67a0 && !legacy ? 7 : 5), "Loaded league bench");
            require(c.u8(object + 0x47) == c.u8(league + 0x47), "Substitution allowance changed");
            return object;
        };
        (void)load(true);
        const auto updated = load(false);
        // The original writes transient pointers in standing rows (+8/+c).
        // Different allocations therefore cannot have identical entire files.
        // Compare the SAME loaded object with only its bench byte changed,
        // while both full reader paths above check migration and allowance.
        c.put8(updated + 0x46, 5);
        call(c, 0x104a8530, {updated, 1});
        const auto original = files(root);
        c.put8(updated + 0x46, slot == -0x67a0 ? 7 : 5);
        call(c, 0x104a8530, {updated, 1});
        const auto changed = files(root);
        require(changed.size() == original.size(), "Save file set changed");
        unsigned differences = 0;
        for (const auto& [path, before] : original) {
            const auto& after = changed.at(path);
            require(after.size() == before.size(), "League serialized size changed");
            for (unsigned i = 0; i < before.size(); ++i) if (before[i] != after[i]) {
                require(before[i] == 5 && after[i] == 7, "Unrelated league byte changed");
                ++differences;
            }
        }
        require(differences == (slot == -0x67a0 ? 1u : 0u), "League migration changed more than its bench byte");
        (void)load(false);
        call(c, 0x10773520, {0xec}); const auto missing = c.r(3);
        call(c, 0x1049e4f0, {missing});
        c.put32(missing, c.u32(league)); c.put32(missing + 4, c.u32(league + 4));
        c.put16(missing + 0x40, 1900);
        call(c, 0x104a6850, {missing, 0});
        require(c.r(3) == 0 && c.u8(missing + 0x46) == 5, "Failed league load applied the migration");
    }
    std::cout << "Premier bench: real league serializers, legacy override, other-league exclusion and one-byte save delta passed\n";
}
}

int english_bench_checks(const char* snapshot, const char* game, const char* backing) {
    try {
        const auto root = std::filesystem::absolute(backing) /
            ("bench-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(root / "Files");
        auto state = std::make_unique<State>(); load_match_snapshot(*state, snapshot); Cpu c(*state);
        state->budget = state->executed + 1000000000;
        state->platform = std::make_shared<NativePlatform>(game, root, AudioMode::offline, EventMode::isolated);
        // The capture is mid-operation; each standalone native I/O check starts
        // with the caller-owned success flag, as a normal save/load does.
        c.put8(c.toc(-0x7bac), 1);
        fixtures(c, root); leagues(c, root);
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

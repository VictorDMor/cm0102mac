#include "inter_amer_cup_retirement.hpp"
#include "guest_abi.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>

namespace cm::aot {
bool retirement_fixture_reference(State&);
bool retirement_host_reference(State&);
}

using namespace cm::aot;
using guest::Cpu;
using cm::semantic_io_checks::require;
namespace {
constexpr std::uint32_t sentinel = 0xfffffff0;
struct InitializationReturned {};
bool initialization_boundary(State& s) {
    if (s.pc != 0x10191cd0) return false;
    dispatch_probe = nullptr;
    // Run the entire real initializer, then stop before its outer caller's
    // unrelated work. This also checks CreationScope's exception restoration.
    run_until(s, 0x1034a8e8);
    throw InitializationReturned{};
}
void call(Cpu& c, std::uint32_t entry, std::initializer_list<std::uint32_t> arguments) {
    unsigned reg = 3;
    for (auto value : arguments) c.set(reg++, value);
    c.call(entry, sentinel);
    require(c.s.pc == sentinel, "Retirement dependency did not return");
}
using Row = std::vector<std::uint8_t>;
using Rows = std::vector<Row>;
std::uint32_t word(const Row& row, unsigned offset, unsigned size) {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < size; ++i) value = (value << 8) | row.at(offset + i);
    return value;
}
bool pending_retired(const Row& row) {
    return word(row, 0, 4) == 199 && word(row, 0x28, 2) > 1998 && row.at(0x43) == 255;
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
std::uint32_t new_manager(Cpu& c, unsigned year, bool original_reader = false) {
    call(c, 0x10773520, {0x1184});
    const auto manager = c.r(3); require(manager != 0, "Cannot allocate fixture manager");
    if (original_reader) dispatch_probe = [](State& s) {
        return s.pc >= 0x1048a930 && s.pc < 0x1048ad10 && retirement_fixture_reference(s);
    };
    try { call(c, 0x10487ff0, {manager, year, 0, 0}); }
    catch (...) { dispatch_probe = nullptr; throw; }
    dispatch_probe = nullptr;
    require(c.r(3) == manager && c.u8(c.toc(-0x7bac)), "Fixture constructor/reader failed");
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
void persistence(Cpu& c, const std::filesystem::path& root) {
    const auto live = c.u32(c.toc(-0x7a60));
    const auto sample = rows(c, live);
    require(!sample.empty(), "Real career has no fixture template");
    const auto other_id = word(sample.front(), 0, 4);
    require(other_id != 199 && other_id != 0xffffffffu, "Other competition fixture template invalid");
    call(c, 0x10773520, {0x50}); const auto fixture = c.r(3);
    const auto definitions = c.u32(c.toc(-0x7bbc));
    const auto insert = [&](std::uint32_t manager, unsigned id, unsigned year, unsigned day, bool played) {
        c.s.memory.copy_in(fixture, sample.front());
        c.put32(fixture, id); c.put32(fixture + 0x14, definitions + id * 0x6c);
        c.put16(fixture + 0x28, year); c.put16(fixture + 0x2a, day);
        c.put8(fixture + 0x3f, 0); c.put8(fixture + 0x43, played ? 2 : 255);
        c.put8(fixture + 0x44, played ? 1 : 255);
        call(c, 0x10489600, {manager, fixture});
        require(c.u8(c.toc(-0x7bac)), "Original fixture insertion failed");
    };
    const auto source = new_manager(c, 2001);
    require(rows(c, source).empty(), "Persistence backing was not initially empty");
    insert(source, 199, 2001, 100, false);
    insert(source, 199, 2001, 101, true);
    insert(source, other_id, 2001, 102, false);
    write(c, source, root, 3);
    const auto original = new_manager(c, 2001, true);
    auto expected = rows(c, original);
    require(expected.size() == 3, "Original reader did not retain all fixture rows");
    require(std::erase_if(expected, pending_retired) == 1, "Fixture negative/positive controls missing");
    const auto filtered = new_manager(c, 2001);
    require(rows(c, filtered) == expected, "Load changed played results or another competition");
    write(c, filtered, root, 2);
    const auto saved = file_bytes(root, 2001);
    const auto reloaded = new_manager(c, 2001);
    require(rows(c, reloaded) == expected, "Save/reload changed retained fixture payloads");
    write(c, reloaded, root, 2);
    require(file_bytes(root, 2001) == saved, "Second save/reload was not byte-identical");

    const auto historic = new_manager(c, 1998);
    insert(historic, 199, 1998, 103, false); write(c, historic, root, 1);
    const auto historic_expected = rows(c, new_manager(c, 1998, true));
    require(rows(c, new_manager(c, 1998)) == historic_expected && historic_expected.size() == 1,
            "Last historical edition was filtered");
    const auto future = new_manager(c, 2002);
    insert(future, 199, 2002, 104, false); insert(future, 199, 2002, 105, true);
    write(c, future, root, 2);
    auto future_expected = rows(c, new_manager(c, 2002, true));
    require(std::erase_if(future_expected, pending_retired) == 1, "Future fixture controls missing");
    call(c, 0x10451800, {definitions + 199 * 0x6c, 0});
    const auto body_index = c.r(3) & 255;
    call(c, 0x1048a430, {filtered, 2002, 105, 0, body_index});
    const auto lazy_rows = rows(c, c.u32(filtered + 0x112c));
    require(c.r(3) && c.u16(c.r(3)) == 1 && lazy_rows == future_expected,
            "Lazy-loaded year restored retired pending fixtures or changed results");
    std::cout << "ok real fixture writer/original reader, selective cancellation, historical edition, "
                 "lazy year and byte-identical save/reload\n";
}
void host_announcements(Cpu& c) {
    const auto definitions = c.u32(c.toc(-0x7bbc));
    const auto queue = c.u32(c.toc(-0x7bf0));
    const auto cup_region = c.u32(definitions + 199 * 0x6c + 4);
    const auto other_region = c.u32(definitions + 92 * 0x6c + 4);
    // Broadcast in this isolated fixture so the real career's active manager
    // receives both positive controls regardless of their club's continent.
    c.put32(definitions + 199 * 0x6c + 4, 0);
    c.put32(definitions + 92 * 0x6c + 4, 0);
    call(c, 0x10773520, {0x10}); const auto list = c.r(3);
    call(c, 0x10738c00, {list});
    call(c, 0x10775200, {0x28, 1}); const auto row = c.r(3);
    c.put32(row, 199); c.put16(row + 4, 2006); c.put32(row + 8, 196);
    c.put32(row + 12, 196); c.put8(row + 0x20, 253); // Original fixed-host status -3.
    call(c, 0x10773520, {4}); const auto storage = c.r(3);
    c.put32(storage, row); call(c, 0x10738d10, {list, storage, 4});
    const auto run_news = [&](bool original) {
        if (original) dispatch_probe = [](State& s) {
            return s.pc >= 0x1049df70 && s.pc < 0x1049e1f0 && retirement_host_reference(s);
        };
        try { call(c, 0x1049df70, {0, list}); }
        catch (...) { dispatch_probe = nullptr; throw; }
        dispatch_probe = nullptr;
        require(c.u8(c.toc(-0x7bac)), "Native host announcement failed");
    };
    const auto before_original = c.u32(queue + 16);
    run_news(true);
    require(c.u32(queue + 16) == before_original + 1,
            "Original host-news positive control did not publish");
    const auto seed = c.u32(c.r(2) - 0x34c4), cursor = c.u32(c.toc(-0x7b84));
    const auto pooled_seed = c.u32(c.toc(-0x7b90));
    const auto pool = c.s.memory.copy_out(c.toc(-0x7b8c), 51000 * 2);
    const auto retained_row = c.s.memory.copy_out(row, 0x28);
    const auto before_retired = c.u32(queue + 16);
    run_news(false);
    require(c.u32(queue + 16) == before_retired && c.s.memory.copy_out(row, 0x28) == retained_row &&
            c.u32(c.r(2) - 0x34c4) == seed && c.u32(c.toc(-0x7b84)) == cursor &&
            c.u32(c.toc(-0x7b90)) == pooled_seed &&
            c.s.memory.copy_out(c.toc(-0x7b8c), 51000 * 2) == pool,
            "Retired host announcement changed news, schedule history or RNG");
    call(c, 0x10773520, {0x28}); const auto other = c.r(3);
    c.s.memory.copy_in(other, retained_row); c.put32(other, 92);
    c.put32(storage, other); call(c, 0x10738d10, {list, storage, 4});
    run_news(false);
    const auto id = c.u32(queue + 16);
    require(id == before_retired + 1, "Mixed announcement list omitted another competition");
    bool found = false;
    for (unsigned i = 0; i < c.u32(queue + 4); ++i) {
        const auto item = c.u32(queue) + i * 0xe4;
        if (c.u32(item + 0xdc) != id) continue;
        require(c.u32(item) == 5 && c.u32(item + 8) == 92, "Mixed list published retired news");
        found = true;
    }
    require(found, "Other competition's real news item missing");
    c.put16(row + 4, 1998);
    require(!inter_amer_cup_skip_host_announcement(c.s, row), "Historical host announcement filtered");
    c.put32(definitions + 199 * 0x6c + 4, cup_region);
    c.put32(definitions + 92 * 0x6c + 4, other_region);
    std::cout << "ok original host-news positive control, retired suppression and mixed-list publication\n";
}
}
int inter_amer_cup_retirement_checks(const char* snapshot, const char* game, const char* backing) {
    try {
        const auto root = std::filesystem::absolute(backing) /
            ("retirement-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(root / "Files");
        auto state = std::make_unique<State>(); load_match_snapshot(*state, snapshot); Cpu c(*state);
        state->budget = state->executed + 1000000000;
        state->platform = std::make_shared<NativePlatform>(game, root, AudioMode::offline, EventMode::isolated);
        const auto year_address = c.toc(-0x7b80) + 2;
        const auto original_year = c.u16(year_address);
        const auto id_address = c.toc(-0x6af8), id = c.u32(id_address);
        require(id == 199, "Real career competition identity changed");
        for (const unsigned year : {1997, 1998, 1999, 2001, 2002, 2026}) {
            c.put16(year_address, year);
            const auto before = match_snapshot_memory_hash(*state);
            require(inter_amer_cup_retired(*state) == (year > 1998), "Retirement year boundary");
            require(inter_amer_cup_creation_id(*state, id) == id,
                    "Loading must preserve historical competition identity");
            require(inter_amer_cup_creation_id(*state, 0xffffffffu) == 0xffffffffu,
                    "Already absent competition must remain absent");
            require(match_snapshot_memory_hash(*state) == before && c.u32(id_address) == id,
                    "Retirement query changed guest memory or RNG");
        }
        c.put16(year_address, original_year);
        // Execute the full original ruling-body factory through production
        // generation and real constructors, allocator and native services.
        const auto bodies = c.toc(-0x7b44), north = bodies + 2 * 0x4c;
        const auto saved_body = state->memory.copy_out(north, 0x4c);
        const auto definitions = c.u32(c.toc(-0x7bbc));
        const auto definition_bytes = state->memory.copy_out(definitions + id * 0x6c, 0x6c);
        const auto registry = c.u32(c.toc(-0x7b5c)), saved_cup = c.u32(registry + id * 4);
        const auto cup_bytes = state->memory.copy_out(saved_cup, 0xb4);
        call(c, 0x10773520, {0x4c});
        const auto body = c.r(3); require(body != 0, "Cannot allocate test ruling body");
        state->memory.copy_in(body, saved_body);
        c.put8(c.toc(-0x7bac), 1);
        call(c, 0x1034f130, {body});
        require(c.r(3) == 1 && c.u8(c.toc(-0x7bac)) == 1, "Ruling-body factory failed");
        require(c.u32(body + 0xc) == 3, "Loading no longer restores the legacy cup object");
        // Isolate North America in the real 34-body initializer. Its factory,
        // dependencies and original loop run normally through the production
        // new-career wrapper; other initializer callbacks are outside this check.
        for (unsigned i = 0; i < 34; ++i)
            if (i != 2) c.put32(bodies + i * 0x4c + 8, 0);
        dispatch_probe = initialization_boundary;
        state->pc = 0x1034a8e4;
        bool initialized = false;
        try { run_until(*state, sentinel); }
        catch (const InitializationReturned&) { initialized = true; }
        dispatch_probe = nullptr;
        require(initialized && state->pc == 0x1034a8e8, "Generated new-career call did not complete");
        require(c.r(3) == 1 && c.u8(c.toc(-0x7bac)) == 1, "New-career initializer failed");
        require(c.u32(north + 0xc) == 2, "Retired cup still counted in a new career");
        const auto objects = c.u32(north + 0x10);
        for (unsigned i = 0; i < 2; ++i) {
            const auto object = c.u32(objects + i * 4);
            const auto expected_id = c.u32(c.toc(i == 0 ? -0x6960 : -0x68bc));
            require(object && c.u32(c.u32(object + 4)) == expected_id,
                    "New career changed another North American competition");
        }
        require(c.u32(id_address) == id &&
                state->memory.copy_out(definitions + id * 0x6c, 0x6c) == definition_bytes &&
                state->memory.copy_out(saved_cup, 0xb4) == cup_bytes && c.u32(registry + id * 4) == saved_cup,
                "Factory changed historical definition, registration or records");
        require(inter_amer_cup_creation_id(*state, id) == id, "New-career scope leaked into loading");
        const auto memory = match_snapshot_memory_hash(*state);
        const auto stack = c.sp();
        call(c, 0x10482ca0, {saved_cup, 0, 0});
        require(c.r(3) == 0 && c.sp() == stack && match_snapshot_memory_hash(*state) == memory,
                "Retired daily update changed memory, records or RNG");
        require(inter_amer_cup_retired_object(*state, saved_cup) &&
                !inter_amer_cup_retired_object(*state, c.u32(objects)) &&
                !inter_amer_cup_retired_object(*state, 0), "Annual retirement scope");
        // Execute the generated annual gate, stopping by instruction budget at
        // its original skip continuation before any unrelated body processing.
        const auto registers = state->registers;
        const auto unique = state->unique;
        const auto budget = state->budget;
        c.put32(body + 0xc, 1);
        c.set({{16, saved_cup}, {17, 0}, {21, body}});
        const auto before_annual = match_snapshot_memory_hash(*state);
        state->pc = 0x101312b8; state->budget = state->executed + 1;
        bool annual_stopped = false;
        try { run_until(*state, sentinel); }
        catch (const std::runtime_error& error) {
            require(std::string_view(error.what()) == "Native instruction budget exhausted",
                    "Annual gate stopped for an unrelated error");
            annual_stopped = state->pc == 0x101313a4;
        }
        require(annual_stopped && match_snapshot_memory_hash(*state) == before_annual,
                "Generated annual gate ran competition processing or changed records/RNG");
        state->budget = budget; state->registers = registers; state->unique = unique;
        state->pc = sentinel;
        std::cout << "ok retirement boundary, native new/load factories, history and daily/RNG guard\n";
        persistence(c, root);
        host_announcements(c);
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}

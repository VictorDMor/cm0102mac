// DISCIPLINE.CPP lifecycle block cases (discipline_checks.hpp): the load and the save of discipline.dat against
// a scripted file layer (discipline_checks_lifecycle_service.hpp). Every transfer that can come up short, every
// allocation that can fail, and the counts that skip an array each have a case.
#include "discipline_checks_lifecycle_service.hpp"
#include <string>

namespace {
using namespace cm::aot::discipline_checks;
using lifecycle::Role;
using lifecycle::Script;
constexpr std::uint32_t load = 0x100de760, save = 0x100deda0;
Write half(std::uint32_t at, std::uint32_t value) { return {2, at, value}; }

void load_cases(const Scenario& scenario, std::vector<Case>& cases) {
    const auto manager = scenario.global(-0x7aa4);
    const auto people = scenario.people();
    const auto add = [&](const std::string& label, Script script, std::uint32_t file = lifecycle::file_argument) {
        script.manager = manager;
        script.people = people;
        cases.push_back({.entry = load, .registers = {{3, manager}, {4, file}}, .label = "lifecycle: load " + label,
                         .service = lifecycle::service(script)});
    };
    add("a whole file", {});
    add("no file", {}, 0);
    {
        Script none; none.open_fails = true;
        add("a file that does not open", none);
    }
    // The allocations of the table, the records and the index (the size each asks for).
    {
        Script no_table; no_table.fail_size = people * 0x2c;
        add("table not allocated", no_table);
        Script no_records; no_records.fail_size = 3 * 0x18;
        add("records not allocated", no_records);
        Script no_index; no_index.fail_size = 2 * 8;
        add("index not allocated", no_index);
    }
    // A transfer that comes up short or fails.
    for (auto role : {Role::table, Role::record_count, Role::records, Role::index_count, Role::index, Role::pending_count}) {
        Script script; script.fail = role;
        const char* names[] = {"none", "table", "record count", "records", "index count", "index", "pending count"};
        add(std::string("short ") + names[std::size_t(role)], script);
    }
    for (unsigned item : {0u, 1u, 2u}) {
        Script script; script.fail = Role::item; script.fail_item = item;
        add("pending item " + std::to_string(item) + " short", script);
    }
    {
        Script all; all.fail = Role::item; all.fail_item = 0; all.pending = 1;
        add("the only pending item short", all);
    }
    // Counts: empty arrays and lists skip their transfers; a negative count asks for an impossible allocation.
    for (std::int16_t records : {0, 1, 40}) {
        Script script; script.records = records;
        add("records " + std::to_string(records), script);
    }
    for (std::int16_t index : {0, 1, 25}) {
        Script script; script.index = index;
        add("index " + std::to_string(index), script);
    }
    for (std::int16_t pending : {0, 1, 12}) {
        Script script; script.pending = pending;
        add("pending " + std::to_string(pending), script);
    }
    {
        Script negative; negative.records = -1; negative.fail_size = 0xffffffe8u;
        add("records -1 (the allocation fails)", negative);
        Script negative_index; negative_index.index = -1; negative_index.fail_size = 0xfffffff8u;
        add("index -1 (the allocation fails)", negative_index);
        Script negative_pending; negative_pending.pending = -1;
        add("pending -1 (no item read)", negative_pending);
        Script big; big.records = 0x7fff; big.fail_size = 0x7fff * 0x18u;
        add("records 0x7fff (the allocation fails)", big);
    }
    // A body's own load coming up short is the body's: the manager goes on regardless.
    {
        Script body; body.fail = Role::body; body.fail_body = 1;
        add("a body transfer short", body);
    }
}

void save_cases(const Scenario& scenario, std::vector<Case>& cases) {
    const auto manager = scenario.global(-0x7aa4);
    const auto people = scenario.people();
    const auto add = [&](const std::string& label, Script script, std::vector<Write> memory = {}, std::uint32_t file = lifecycle::file_argument) {
        script.manager = manager;
        script.people = people;
        cases.push_back({.entry = save, .registers = {{3, manager}, {4, file}}, .memory = std::move(memory),
                         .label = "lifecycle: save " + label, .service = lifecycle::service(script)});
    };
    add("the manager as the snapshot has it", {});
    add("no file", {}, {}, 0);
    {
        Script none; none.open_fails = true;
        add("a file that does not open", none);
    }
    for (auto role : {Role::table, Role::record_count, Role::records, Role::index_count, Role::index, Role::pending_count}) {
        Script script; script.fail = role;
        const char* names[] = {"none", "table", "record count", "records", "index count", "index", "pending count"};
        add(std::string("write of the ") + names[std::size_t(role)] + " short", script);
    }
    const auto list_count = scenario.r32(manager + 0x1c) >> 16;
    for (unsigned item : {0u, 1u, 2u, 5u}) {
        Script script; script.fail = Role::item; script.fail_item = item;
        add("pending item " + std::to_string(item) + " fails", script);
    }
    {
        Script body; body.fail = Role::body; body.fail_body = 1;
        add("a body write short", body);
    }
    // Counts the manager holds: no records, no index, no pending items (the list's head null), several.
    add("no ban records", {}, {half(manager + 0xa, 0)});
    add("no sorted index", {}, {half(manager + 0x12, 0)});
    add("no ban records and no index", {}, {half(manager + 0xa, 0), half(manager + 0x12, 0)});
    add("a pending list of count 0 (items still walked)", {}, {half(manager + 0x1c, 0)});
    add("an empty pending list", {}, {{4, manager + 0x20, 0}, half(manager + 0x1c, 0)});
    add("a negative record count", {}, {half(manager + 0xa, 0xffff)});
    add("a negative index count", {}, {half(manager + 0x12, 0xffff)});
    add("a pending count of -1", {}, {half(manager + 0x1c, 0xffff)});
    {
        Script script; script.fail = Role::record_count;
        add("no records, count write fails", script, {half(manager + 0xa, 0)});
    }
    {
        Script script; script.fail = Role::pending_count;
        add("pending count write fails, empty list", script, {{4, manager + 0x20, 0}});
    }
    (void)list_count;
    // Fewer bodies: empty slots are skipped.
    {
        const auto bodies = scenario.r32(manager);
        add("empty body slots", {}, {{4, bodies + 8, 0}, {4, bodies + 4 * 41, 0}});
    }
}

DISCIPLINE_CASES("lifecycle file") {
    if (scenario.match) return;
    load_cases(scenario, cases);
    save_cases(scenario, cases);
}
}

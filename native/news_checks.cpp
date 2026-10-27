// Returned original/rewrite comparisons. Original pages have no modern hooks;
// fixtures supply only assertion-dialog dependencies and log their arguments.
#include "news_adapter.hpp"
#include "news_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <iostream>
#include <map>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool news_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, item = data + 0x1000,
    input = data + 0x2000, output = data + 0x3000, next_slot = data + 0x4000,
    log_at = data + 0x8000, sentinel = 0xfffffff0;
bool reference_mode = false;
struct Case {
    std::uint32_t entry;
    std::string name;
    std::uint32_t index = 0, length = 3, capacity = 16, type = 1;
    bool null_source = false, null_output = false, no_next = false;
    int inbox_first = 0, unread = 2, match = 3, item_count = 100, manager_delta = 0;
    unsigned storage = 3, deleting = 0, id = 7, filter = 0;
    bool null_manager = false, all_managers = false, current = false, current_matches = true;
    int controlled = 1, enabled = 1;
    int alias = 0, year = 2001;
    unsigned priority = 255, month = 10, weekday = 2, language = 4;
    bool read = false;
};
Case active{0, ""};
bool probe(State& s) {
    if (run_leaf(s)) return true;
    const auto target = s.pc;
    if (news_lists(target)) {
        if (reference_mode) return news_reference(s);
        news_run_rewrite(s, target); return true;
    }
    if (reference_mode && target >= 0x102bdf90 && target < 0x102ce710) return news_reference(s);
    if (target != 0x10739890 && target != 0x10768810 && target != 0x1077b720 && target != 0x10769b60 &&
        target != 0x10775170 && target != 0x107735a0 && target != 0x1014c2e0 && target != 0x100c96e0 && target != 0x10147e70) return false;
    Cpu c(s);
    auto count = c.u32(log_at);
    const auto add = [&](std::uint32_t value) { c.put32(log_at + 4 + count++ * 4, value); };
    add(target); add(c.lr());
    for (unsigned reg = 3; reg < 11; ++reg) add(c.r(reg));
    for (unsigned reg = 13; reg < 32; ++reg) add(c.r(reg));
    for (unsigned field = 0; field < 8; ++field) add(c.cr(field));
    c.put32(log_at, count);
    if (target == 0x10739890) {
        c.put32(c.r(6), 0x4e455753); c.put32(c.r(7), 0x43505000);
    } else if (target == 0x10768810) c.set(3, data + 0x5000);
    else if (target == 0x1077b720) c.put32(c.r(3), c.r(8));
    else if (target == 0x1014c2e0) c.set(3, active.controlled);
    else if (target == 0x10147e70) c.set(3, active.enabled);
    else if (target == 0x100c96e0) c.set(3, data + 0x6040);
    else c.set(3, 1);
    s.pc = c.lr(); return true;
}
void initialize(Cpu& c, const Case& test) {
    c.s.memory.map(data, 0x20000); c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x20000, 0xa5));
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, 0x51000000 + reg * 0x101);
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, field + 1);
    c.set(1, 0x71010000); c.set(2, 0x108bba90); c.set_lr(sentinel);
    c.put32(log_at, 0); c.put32(item, test.type);
    c.put32(c.r(2) - 0x7ba8, data + 0x6000); c.put32(c.r(2) - 0x7bac, data + 0x6004);
    c.put32(c.r(2) - 0x7b80, data + 0x6010); c.put32(c.r(2) - 0x79ec, data + 0x6020);
    c.put16(data + 0x6010, test.year); c.put16(data + 0x6012, test.month); c.put8(data + 0x6014, test.weekday);
    c.put8(data + 0x6020, test.language); c.put8(data + 0x6004, 1);
    for (unsigned i = 0; i < test.length; ++i) c.put8(input + i, 'a' + i % 26);
    c.put8(input + test.length, 0);
    for (unsigned i = 0; i < 200; ++i) c.put8(item + 8 + i, 'A' + i % 26);
    c.put8(item + 0xcf, 0);
    c.set({{3, item}, {4, test.index}, {5, 0x12345678}, {6, next_slot}});
    if (test.entry == 0x102bdf90) c.set({{4, test.type}, {5, test.priority}});
    if (test.entry == 0x102cb630) c.set({{5, test.null_source ? 0 : input}, {6, test.no_next ? 0 : next_slot}});
    if (test.entry == 0x102cb7f0) c.set({{5, test.null_output ? 0 : output}, {6, test.capacity}});
    if (test.entry >= 0x102cbb70) {
        constexpr auto records = data + 0x10000, managers = data + 0x5000, manager = data + 0x7000;
        c.put32(c.r(2) - 0x7be4, data + 0x6030); c.put32(data + 0x6030, 32);
        c.put32(c.r(2) - 0x7a54, data + 0x6040); c.put32(data + 0x6040, 0);
        c.put32(manager, 32 + test.manager_delta);
        c.put32(item, test.storage & 1 ? records : 0); c.put32(item + 4, test.item_count);
        c.put32(item + 8, test.storage & 2 ? managers : 0); c.put32(item + 12, 32);
        c.put32(managers + 16 * 12, test.inbox_first); c.put32(managers + 16 * 12 + 4, 100);
        c.put32(managers + 16 * 12 + 8, test.unread);
        for (unsigned i = 0; i < 100; ++i) {
            c.put32(records + i * 0xe4, 1 + (i % 7) * 1000);
            c.put32(records + i * 0xe4 + 0xdc, int(i) == test.match ? test.id : 0);
            c.put8(records + i * 0xe4 + 0xe0, test.read ? 1 : i % 2);
        }
        c.set({{3, item}, {4, test.null_manager || test.all_managers ? 0 : manager}, {5, test.id}, {6, output}});
        if (test.entry == 0x102cbb70) c.set({{3, test.null_source ? 0 : item}, {4, test.deleting}});
        if (test.entry == 0x102ccdd0 && test.null_output) c.set(6, 0);
        if (test.entry == 0x102ccdd0 && test.alias) c.set(6, records + test.match * 0xe4 + test.alias - 16);
        if (test.entry == 0x102cd0c0) {
            c.set({{6, test.index}, {7, 0x12345678}, {8, test.current ? input : 0}});
            c.put32(input + 0xdc, test.current_matches ? test.id : test.id + 1);
        }
        if (test.entry == 0x102cd440) c.set(5, test.filter);
    }
    c.s.budget = 10000000;
}
void returned(State& s) {
    Cpu c(s);
    if (c.sp() != 0x71010000 || c.lr() != sentinel || s.pc != sentinel)
        throw std::runtime_error("NEWS fixture did not return");
}
void original(State& s, std::uint32_t entry) {
    reference_mode = true; s.pc = entry; run_until(s, sentinel); returned(s);
}
void rewrite(State& s, std::uint32_t entry) {
    reference_mode = false; news_run_rewrite(s, entry); returned(s);
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-news-checks <original-segments>\n"; return 2; }
    try {
        auto base = std::make_unique<State>(); load_program(*base, argv[1]); dispatch_probe = probe;
        std::vector<Case> cases{{0x102bdf90, "construct item"}};
        for (int year : {-32768,-1,0,32767}) for (unsigned priority : {0u,1u,255u,256u}) {
            Case test{0x102bdf90, "constructor year " + std::to_string(year) + " priority " + std::to_string(priority)};
            test.year = year; test.priority = priority; test.month = 0xffff; test.weekday = 255; test.language = 255;
            cases.push_back(test);
        }
        for (int type : {-2147483647,-1,0,1,999,1000,1999,2000,2999,3000,3999,4000,4999,5000,5999,6000,6032,6033,2147483647}) {
            Case test{0x102cb340, "category " + std::to_string(type)}; test.type = type; cases.push_back(test);
        }
        for (auto entry : {0x102cb4b0u,0x102cb570u,0x102cb630u,0x102cb7f0u})
            for (int index : {-129,-128,-1,0,1,48,49,50,127,128,255,256,305,306}) {
                Case test{entry, "slot " + std::to_string(index)}; test.index = index; cases.push_back(test);
            }
        for (unsigned index : {0u,1u,48u,49u}) for (unsigned length : {0u,1u,3u,4u,7u,195u,199u,200u,201u,509u}) {
            Case test{0x102cb630, "string length " + std::to_string(length) + " slot " + std::to_string(index)};
            test.index = index; test.length = length; cases.push_back(test);
            test.no_next = true; test.name += " without next slot"; cases.push_back(test);
        }
        Case null_input{0x102cb630, "null source"}; null_input.null_source = true; cases.push_back(null_input);
        Case null_output{0x102cb7f0, "null output"}; null_output.null_output = true; cases.push_back(null_output);
        for (unsigned capacity : {0u,1u,2u,199u,200u,201u,0xffffffffu}) {
            Case test{0x102cb7f0, "capacity " + std::to_string(capacity)};
            test.capacity = capacity; cases.push_back(test);
        }
        for (unsigned storage = 0; storage < 4; ++storage) for (unsigned deleting : {0u,1u,65535u,65537u}) {
            Case test{0x102cbb70, "destroy storage " + std::to_string(storage) + " deleting " + std::to_string(deleting)};
            test.storage = storage; test.deleting = deleting; cases.push_back(test);
        }
        Case null_queue{0x102cbb70, "null queue"}; null_queue.null_source = true; cases.push_back(null_queue);
        for (auto entry : {0x102ccdd0u,0x102cd0c0u,0x102cd440u}) {
            Case missing_manager{entry, "null manager"}; missing_manager.null_manager = true; cases.push_back(missing_manager);
            Case before_managers{entry, "manager before first"}; before_managers.manager_delta = -17; cases.push_back(before_managers);
            Case empty{entry, "unassigned inbox"}; empty.inbox_first = -1; cases.push_back(empty);
            for (int match : {-1,0,3,99}) {
                Case test{entry, "match position " + std::to_string(match)}; test.match = match; cases.push_back(test);
            }
        }
        for (int count : {-1,0,1,100}) for (int match : {-1,0,99}) {
            Case test{0x102ccdd0, "global count " + std::to_string(count) + " match " + std::to_string(match)};
            test.item_count = count; test.match = match; test.all_managers = true; cases.push_back(test);
        }
        Case zero_id{0x102ccdd0, "zero id"}; zero_id.id = 0; cases.push_back(zero_id);
        for (int alias : {12,16,20,24}) {
            Case test{0x102ccdd0, "overlapping output " + std::to_string(alias - 16)};
            test.alias = alias; cases.push_back(test);
        }
        Case missing_output{0x102ccdd0, "missing output"}; missing_output.null_output = true; cases.push_back(missing_output);
        for (unsigned index : {0u,49u,50u,255u,0xffffffffu}) for (bool current : {false,true}) {
            Case test{0x102cd0c0, "update slot " + std::to_string(index) + " current " + std::to_string(current)};
            test.index = index; test.current = current; cases.push_back(test);
        }
        Case mismatch{0x102cd0c0, "mismatching current item"}; mismatch.current = true; mismatch.current_matches = false; cases.push_back(mismatch);
        Case zero_update{0x102cd0c0, "zero update id"}; zero_update.id = 0; cases.push_back(zero_update);
        for (int unread : {-1,0,100}) for (unsigned filter : {0u,1u,2u,3u,7u,255u}) {
            Case test{0x102cd440, "unread " + std::to_string(unread) + " category " + std::to_string(filter)};
            test.unread = unread; test.filter = filter; test.match = 0; cases.push_back(test);
            test.read = true; test.name += " already read"; cases.push_back(test);
        }
        for (int controlled : {0,1,255,256}) for (int enabled : {0,1,255}) {
            Case test{0x102cdbf0, "club controlled " + std::to_string(controlled) + " enabled " + std::to_string(enabled)};
            test.controlled = controlled; test.enabled = enabled; cases.push_back(test);
        }
        Case no_club{0x102cdbf0, "null club"}; no_club.null_manager = true; cases.push_back(no_club);
        unsigned failures = 0;
        std::map<std::uint32_t, unsigned> returns;
        for (const auto& test : cases) {
            active = test;
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize(c, test);
            const auto difference = compare_routine_call(*state, test.entry, original, rewrite, RoutineChecker::result_r3);
            if (!difference.empty()) { ++failures; std::cout << "FAIL " << hex_address(test.entry) << ' ' << test.name << '\n'; for (const auto& line : difference) std::cout << line << '\n'; }
            else ++returns[test.entry];
        }
        auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize(c, cases.front());
        const auto negative = compare_routine_call(*state, cases.front().entry, original, rewrite, RoutineChecker::result_r3, true);
        if (negative.empty()) { ++failures; std::cout << "FAIL corrupted result escaped\n"; }
        else std::cout << "ok corrupted result detected\n";
        for (const auto& [entry, count] : returns) std::cout << hex_address(entry) << ": " << count << " returned comparisons\n";
        std::cout << cases.size() << " returned fixture comparisons, " << failures << " failed\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}

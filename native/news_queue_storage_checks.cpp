// Complete caller comparisons with a scripted file-controller boundary.
// Fatal-return cases are explicitly injected: they do not establish the
// nonreturning application's error-exit implementation.
#include "news_adapter.hpp"
#include "news_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <iostream>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool news_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x61000000, queue_at = data + 0x1000,
    context = data + 0x2000, records = data + 0x10000, inboxes = data + 0x20000,
    output = data + 0x30000, control = data + 0x40000, log_at = data + 0x50000,
    sentinel = 0xfffffff0;
struct Scenario {
    std::uint32_t entry;
    std::string name;
    unsigned items = 0, managers = 16, short_at = 0, partial = 0, allocation_failure = 0;
    bool no_context = false, open_failure = false;
    bool malformed_header = false;
    std::uint32_t header_items = 0;
    unsigned locate_failures = 0;
    bool clobber_toc = false;
};
Scenario active{0, ""};
bool reference_mode = false;
unsigned transfers = 0, allocations = 0, offset = 0;
unsigned locate_calls = 0;
std::vector<std::uint8_t> packet;
void append_word(std::uint32_t word) {
    for (unsigned n = 4; n; --n) packet.push_back(std::uint8_t(word >> ((n - 1) * 8)));
}
void trace(Cpu& c, std::uint32_t target) {
    auto count = c.u32(log_at);
    const auto add = [&](std::uint32_t value) { c.put32(log_at + 4 + count++ * 4, value); };
    add(target); add(c.lr());
    for (unsigned r = 3; r < 11; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned field = 0; field < 8; ++field) add(c.cr(field));
    c.put32(log_at, count);
}
bool probe(State& s) {
    if (run_leaf(s)) return true;
    const auto target = s.pc;
    if (reference_mode && target >= 0x102bdf90 && target < 0x102ce710) return news_reference(s);
    if (news_lists(target)) { news_run_rewrite(s, target); return true; }
    if (target != 0x107475b0 && target != 0x10747a80 && target != 0x10747cc0 &&
        target != 0x10747900 && target != 0x10775130 && target != 0x1076f7d0 &&
        target != 0x10739890 && target != 0x10768810 && target != 0x1077b720 && target != 0x10769b60 &&
        target != 0x1077fee0 && target != 0x10727890) return false;
    Cpu c(s); trace(c, target);
    if (target == 0x107475b0) {
        c.put32(control, active.entry != 0x102ce320 ? std::uint32_t(packet.size()) : 0);
        c.put32(control + 4, 0);
        c.set(3, active.open_failure ? 0 : context + 8);
    } else if (target == 0x10775130) {
        ++allocations;
        if (allocations == active.allocation_failure) c.set(3, 0);
        else {
            const auto allocated = c.lr() == 0x102cdfd8 ? records : inboxes;
            const auto size = c.r(3);
            for (unsigned n = 0; n < size; n += 4) c.put32(allocated + n, 0xcccccccc);
            c.set(3, allocated);
        }
    } else if (target == 0x10747a80 || target == 0x10747cc0) {
        ++transfers;
        const auto size = c.r(4), count = c.r(5);
        const auto completed = transfers == active.short_at ? count - 1 : count;
        const auto bytes = size * completed + (transfers == active.short_at ? active.partial : 0);
        if (offset + bytes > packet.size()) throw std::runtime_error("storage fixture exceeds input packet");
        for (unsigned n = 0; n < bytes; ++n) {
            if (target == 0x10747a80) c.put8(c.r(3) + n, packet[offset + n]);
            else c.put8(output + offset + n, c.u8(c.r(3) + n));
        }
        offset += bytes; c.put32(control + 4, offset);
        if (target == 0x10747cc0) c.put32(control, offset);
        c.set(3, completed);
    } else if (target == 0x10747900) {
        c.put32(c.r(3), 0); c.put32(control + 12, c.u32(control + 12) + 1); c.set(3, 1);
    } else if (target == 0x1076f7d0) {
        c.put32(control + 8, c.u32(control + 8) + 1); c.set(3, 1);
    } else if (target == 0x1077fee0) {
        cm::semantic_io_checks::require(c.s.memory.copy_out(c.r(5),7)==std::vector<std::uint8_t>({6,'x','A','y','B','z','C'}),"queue resource name");
        c.put32(c.sp()+0x14,c.r(2)); c.put32(c.r(6),0x4e455753);
        if(active.clobber_toc) c.set(2,0xdeadbeef);
        c.set(3,locate_calls++<active.locate_failures?0xffffffd5:0);
    } else if (target == 0x10727890) {
        c.put32(c.r(3),0x4e455753); c.put8(c.r(3)+4,0);
    } else if (target == 0x10739890) {
        c.put32(c.r(6), 0x4e455753); c.put32(c.r(7), 0x43505000);
    } else if (target == 0x10768810) c.set(3, data + 0x60200);
    else if (target == 0x1077b720) c.put32(c.r(3), c.r(8));
    else c.set(3, 1);
    s.pc = c.lr(); return true;
}
void initialize(Cpu& c, const Scenario& test) {
    c.s.memory.map(data, 0x80000); c.s.memory.map(0x72000000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x80000, 0xa5));
    c.s.memory.copy_in(0x72000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x52000000 + r * 0x101);
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, field + 1);
    c.set(1, 0x72010000); c.set(2, 0x108bba90); c.set_lr(sentinel);
    c.put32(c.r(2) - 0x7ba8, data + 0x60000); c.put32(c.r(2) - 0x7bac, data + 0x60100);
    c.put8(data + 0x60100, 1); c.put32(log_at, 0);
    c.put32(c.r(2)-0x5114,data+0x61000); c.put32(c.r(2)-0x5110,data+0x61100);
    for (unsigned n=0;n<17;++n) { c.put8(data+0x61000+n,0); c.put8(data+0x61100+n,0); }
    c.put32(data+0x61000,0x03414243); c.put32(data+0x61100,0x0378797a);
    for (unsigned n = 0; n < 16; n += 4) c.put32(control + n, 0);
    for (unsigned n = 0; n < test.items; ++n) {
        const auto item = records + n * 0xe4;
        c.put32(item, 1); c.put8(item + 4, 0);
        for (unsigned byte = 0; byte < 200; ++byte) c.put8(item + 8 + byte, (n * 37 + byte) & 255);
        c.put16(item + 0xd0, 2001); c.put16(item + 0xd2, 10); c.put8(item + 0xd4, 2);
        c.put8(item + 0xd8, 4); c.put32(item + 0xdc, 0); c.put8(item + 0xe0, 1); c.put8(item + 0xe1, 0);
    }
    for (unsigned n = 0; n < test.managers; ++n) {
        const auto first = n < test.items / 100 ? n * 100 : 0xffffffff;
        c.put32(inboxes + n * 12, first); c.put32(inboxes + n * 12 + 4, first); c.put32(inboxes + n * 12 + 8, 0);
    }
    packet.clear(); append_word(test.malformed_header ? test.header_items : test.items);
    const auto item_bytes = c.s.memory.copy_out(records, test.items * 0xe4);
    packet.insert(packet.end(), item_bytes.begin(), item_bytes.end()); append_word(test.managers);
    const auto inbox_bytes = c.s.memory.copy_out(inboxes, test.managers * 12);
    packet.insert(packet.end(), inbox_bytes.begin(), inbox_bytes.end()); append_word(99);
    c.put32(queue_at, records); c.put32(queue_at + 4, test.items);
    c.put32(queue_at + 8, inboxes); c.put32(queue_at + 12, test.managers); c.put32(queue_at + 16, 99);
    if (test.entry == 0x102cde30) for (unsigned n = 0; n < 20; n += 4) c.put32(queue_at + n, 0xa5a5a5a5);
    c.set({{3, queue_at}, {4, test.no_context ? 0 : context}});
    c.s.budget = 10000000; transfers = allocations = offset = locate_calls = 0;
}
void postconditions(State& s) {
    Cpu c(s); using cm::semantic_io_checks::require;
    require(c.sp() == 0x72010000 && c.lr() == sentinel && s.pc == sentinel, "storage did not return");
    if (active.entry==0x102cb930) {
        require(c.r(3)==queue_at,"queue constructor result");
        require(locate_calls==active.locate_failures+1,"queue resource lookup retry count");
        const bool loaded=!active.no_context&&!active.open_failure;
        require(c.u32(queue_at)==(loaded&&active.items?records:0),"constructed record buffer");
        require(c.u32(queue_at+4)==(loaded?active.items:0),"constructed allocated count");
        require(c.u32(queue_at+8)==(active.allocation_failure?0:inboxes),"constructed inbox buffer");
        require(c.u32(queue_at+12)==(active.allocation_failure?0:loaded?active.managers:16),"constructed inbox count");
        require(c.u32(queue_at+16)==(loaded||active.allocation_failure?99:0),"constructed next identifier");
        if (!loaded&&!active.allocation_failure)
            for (unsigned n=0;n<16;++n) require(c.i32(inboxes+n*12)==-1&&c.i32(inboxes+n*12+4)==-1&&c.u32(inboxes+n*12+8)==0,"empty inbox fields");
        require(c.u32(control+8)==unsigned(bool(active.allocation_failure)),"constructor injected fatal boundary");
        return;
    }
    const bool failed = active.no_context || active.open_failure || active.short_at || active.allocation_failure;
    require(c.r(3) == (failed ? 0u : 1u), "storage return status");
    require(c.u8(data + 0x60100) == (failed ? 0 : 1), "storage success flag");
    const bool fatal = failed && !active.no_context && !(active.entry == 0x102cde30 && active.open_failure);
    require(c.u32(control + 8) == unsigned(fatal), "storage fatal-boundary count");
    require(c.u32(control + 12) == unsigned(!active.no_context && !active.open_failure), "storage close count");
    if (failed) return;
    require(c.u32(control + 4) == packet.size(), "storage byte count");
    if (active.entry == 0x102ce320)
        require(c.s.memory.copy_out(output, packet.size()) == packet, "serialized queue bytes/padding");
    else {
        require(c.u32(queue_at + 4) == active.items && c.u32(queue_at + 12) == active.managers &&
                c.u32(queue_at + 16) == 99, "loaded queue header fields");
        require(c.u32(queue_at) == (active.items ? records : 0) && c.u32(queue_at + 8) == inboxes, "loaded queue buffers");
        const auto item_size = active.items * 0xe4, inbox_size = active.managers * 12;
        require(c.s.memory.copy_out(records, item_size) == std::vector<std::uint8_t>(packet.begin() + 4, packet.begin() + 4 + item_size), "loaded item bytes/padding");
        require(c.s.memory.copy_out(inboxes, inbox_size) == std::vector<std::uint8_t>(packet.begin() + 8 + item_size, packet.begin() + 8 + item_size + inbox_size), "loaded inbox bytes");
    }
}
void original(State& s, std::uint32_t entry) { reference_mode = true; s.pc = entry; run_until(s, sentinel); postconditions(s); }
void rewrite(State& s, std::uint32_t entry) { reference_mode = false; news_run_rewrite(s, entry); postconditions(s); }
}
namespace cm::aot {
unsigned check_news_storage(const State& base) {
    struct Restore { bool (*previous)(State&); ~Restore() { dispatch_probe = previous; } } restore{dispatch_probe};
    dispatch_probe = probe;
    std::vector<Scenario> cases;
    for (auto entry : {0x102cde30u,0x102ce320u}) {
        for (unsigned items : {0u,100u,200u}) for (unsigned managers : {16u,32u}) {
            Scenario test{entry, "normal"}; test.items = items; test.managers = managers; cases.push_back(test);
        }
        Scenario null{entry, "null context"}; null.no_context = true; cases.push_back(null);
        Scenario open{entry, "open failure"}; open.open_failure = true; cases.push_back(open);
        for (unsigned items : {0u,100u}) for (unsigned step = 1; step <= (items ? 5u : 4u); ++step)
            for (unsigned partial : {0u,1u}) {
                Scenario test{entry, "injected short transfer " + std::to_string(step) + " partial " + std::to_string(partial)};
                test.items = items; test.short_at = step; test.partial = partial; cases.push_back(test);
            }
    }
    for (unsigned items : {0u,100u,200u}) for (unsigned allocation = 1; allocation <= (items ? 2u : 1u); ++allocation) {
        Scenario test{0x102cde30, "injected allocation failure " + std::to_string(allocation)};
        test.items = items; test.allocation_failure = allocation; cases.push_back(test);
    }
    for (auto count : {0x80000000u,0xffffffffu}) {
        Scenario test{0x102cde30, "malformed signed count / injected allocation failure"};
        test.malformed_header = true; test.header_items = count; test.allocation_failure = 1; cases.push_back(test);
    }
    for (unsigned retries : {0u,1u,2u}) for (bool no_context : {false,true}) {
        Scenario test{0x102cb930,"queue construction / resource retries"};
        test.items=100; test.managers=32; test.locate_failures=retries; test.no_context=no_context; cases.push_back(test);
    }
    Scenario missing_file{0x102cb930,"queue construction / load missing file"}; missing_file.open_failure=true; cases.push_back(missing_file);
    Scenario imported_toc{0x102cb930,"queue construction / import changes TOC"}; imported_toc.clobber_toc=true; imported_toc.locate_failures=1; cases.push_back(imported_toc);
    Scenario no_inboxes{0x102cb930,"queue construction / injected allocation failure"};
    no_inboxes.no_context=true; no_inboxes.allocation_failure=1; cases.push_back(no_inboxes);
    unsigned failures = 0, injected = 0;
    for (const auto& test : cases) {
        active = test;
        injected += bool(test.short_at || test.allocation_failure || (test.entry == 0x102ce320 && test.open_failure));
        auto state = cm::semantic_io_checks::clone(base); Cpu c(*state); initialize(c, test);
        const auto difference = compare_routine_call(*state, test.entry, original, rewrite, RoutineChecker::result_r3);
        if (!difference.empty()) {
            ++failures; std::cout << "FAIL storage " << hex_address(test.entry) << ' ' << test.name << '\n';
            for (const auto& line : difference) std::cout << line << '\n';
        }
    }
    active = cases.front(); auto state = cm::semantic_io_checks::clone(base); Cpu c(*state); initialize(c, active);
    if (compare_routine_call(*state, active.entry, original, rewrite, RoutineChecker::result_r3, true).empty()) ++failures;
    else std::cout << "ok corrupted queue load result detected\n";
    std::cout << cases.size() << " storage comparisons (" << injected << " injected fatal-return boundaries), " << failures << " failed\n";
    return failures;
}
}

// The rb_*.cpp rulebooks: each routine's C++ against an independent copy of its generated original,
// from the same fixture state. Every routine outside the family is scripted here and traced (target,
// return address, the arguments it reads, the non-volatile registers, and the ban record where one is
// passed), except the three date routines, which run. No career is opened.
#include "ruling_bodies_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <iostream>
#include <map>
#include <random>
#include <string>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool ruling_bodies_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, object = data + 0x1000, text_buffer = data + 0x1800, match = data + 0x2000,
                        booking = data + 0x2800, fixture_record = data + 0x2c00, globals = data + 0x3000,
                        trace_at = data + 0x8000, stack = 0x71010000, sentinel = 0xfffffff0, toc_base = 0x108bba90;
// Cells the fixture points the table of contents at.
constexpr std::uint32_t message = globals, success = globals + 0x400, nations_cell = globals + 0x404,
                        ireland_cell = globals + 0x408, northern_cell = globals + 0x40c, group_cell = globals + 0x410,
                        bans_cell = globals + 0x414, today = globals + 0x420, persons_cell = globals + 0x428;
constexpr std::pair<std::int32_t, std::uint32_t> toc_cells[] = {
    {-0x7ba8, message}, {-0x7bac, success}, {-0x7b98, nations_cell}, {-0x7574, ireland_cell}, {-0x7578, northern_cell},
    {-0x7a70, group_cell}, {-0x7aa4, bans_cell}, {-0x7b80, today}, {-0x7be0, persons_cell}};
constexpr std::array entries{0x106abe20u, 0x106abee0u, 0x106abf60u, 0x106abfe0u};
constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 4> bounds{{{0x106abe20, 0x106abed4}, {0x106abee0, 0x106abf58},
                                                                         {0x106abf60, 0x106abfe0}, {0x106abfe0, 0x106ac21c}}};
enum : unsigned { nation_text, next_card, construct, report };
constexpr std::uint32_t register_ban = 0x100dfb30, post_ban_news = 0x100e0700, nation_record_text = 0x10168a00,
                        base_construct = 0x105bb3e0, booking_record = 0x105bc2b0;

// Which of r3-r10 each scripted routine reads (bit 0 is r3): the declared arguments of the two
// DISCIPLINE.CPP routines, tools/call_arguments.py 0x106abe20 0x106ac21c for the others.
struct Mask { std::uint32_t target; unsigned mask; };
constexpr Mask masks[] = {{register_ban, 0x07}, {post_ban_news, 0x0f}, {nation_record_text, 0x0f}, {base_construct, 0x03},
                          {booking_record, 0x07}, {0x10739890, 0x1f}, {0x10769b60, 0xff}, {0x1077b720, 0xff}};

struct Case {
    unsigned routine = report;
    std::uint32_t seed = 1;
    // Nation text.
    std::uint32_t width = 20, nation_index = 3;
    // The query.
    std::uint8_t tally = 0;
    unsigned outputs = 0;  // 0..7: which of the two outputs exist and what they alias
    // The report.
    std::uint8_t yellows = 1, reds = 0, flag_48 = 0;
    std::uint16_t points = 0, day = 0, year = 2001;
    bool ban_flagged = false;
    bool clobber = false;
};
Case active;
bool original_mode = false, production_mode = false;
unsigned calls = 0;

void trace(Cpu& c, std::uint32_t target, unsigned mask) {
    if (++calls > 400) throw std::runtime_error("fixture call bound");
    auto size = c.u32(trace_at);
    const auto add = [&](std::uint32_t value) { c.put32(trace_at + 4 + size++ * 4, value); };
    add(target); add(c.lr());
    for (unsigned n = 0; n < 8; ++n) if (mask >> n & 1) add(c.r(3 + n));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    if (target == register_ban || target == post_ban_news)
        for (unsigned n = 0; n < 0x18; ++n) add(c.u8(c.r(5) + n));
    c.put32(trace_at, size);
}
bool probe(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    // Anywhere inside the routines, not only at their entries: the original comes back here after each call.
    if (std::ranges::any_of(bounds, [&](const auto& routine) { return target >= routine.first && target < routine.second; })) {
        if (original_mode) return ruling_bodies_reference(s);
        if (production_mode || !ruling_bodies_lists(target)) return false;
        ruling_bodies_run_rewrite(s, target);
        return true;
    }
    // Cpu::call lowers leaves before it consults the probe; the original must run them the same way.
    if (run_leaf(s)) return true;
    const auto known = std::ranges::find(masks, target, &Mask::target);
    if (known == std::end(masks)) return false;  // the date routines and what they call
    trace(c, target, known->mask);
    std::uint32_t answer = 0x5a5a0000 + calls;  // routines with no result leave something in r3
    switch (target) {
    case nation_record_text: c.put32(c.r(4), c.r(3) ^ c.r(5) ^ (c.r(6) << 8)); answer = 0x7e000000 + active.seed; break;
    case base_construct:
        for (unsigned n = 0; n < 0x18; n += 4) c.put32(c.r(3) + n, 0xba5e0000 + n);
        break;
    case booking_record: answer = booking; break;
    // The manager flags a ban it merged into an older one; the report then sees the flag.
    case register_ban: if (active.ban_flagged) c.put8(c.r(5) + 0x14, c.u8(c.r(5) + 0x14) | 0x80); break;
    case 0x10739890: c.put32(c.r(6), 0x52425f00); c.put32(c.r(7), 0x43505000); break;
    case 0x1077b720: c.put32(c.r(3), c.r(8)); break;
    default: break;
    }
    if (active.clobber) {
        for (unsigned r = 4; r <= 10; ++r) c.set(r, 0x41000000 + calls * 0x100 + r);
        c.set(0, 0x40000000 + calls); c.set(11, 0x4b000000 + calls); c.set(12, 0x4c000000 + calls);
        c.compare(0, 17, 29);
    }
    c.set(3, answer);
    s.pc = c.lr();
    return true;
}
void initialize(Cpu& c) {
    std::mt19937 random(active.seed);
    c.s.memory.map(data, 0x10000);
    c.s.memory.map(0x71000000, 0x20000);
    std::vector<std::uint8_t> filler(0x20000);
    // The stack holds leftovers: the report reads one byte of its frame before writing it.
    for (auto& byte : filler) byte = std::uint8_t(random());
    c.s.memory.copy_in(0x71000000, filler);
    filler.resize(0x10000);
    for (auto& byte : filler) byte = std::uint8_t(random() | 1);
    c.s.memory.copy_in(data, filler);
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101 + active.seed);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, f + 1);
    c.set(1, stack); c.set(2, toc_base); c.set_lr(sentinel);
    for (const auto [offset, at] : toc_cells) c.put32(toc_base + std::uint32_t(offset), at);
    c.put8(success, 1);
    c.put32(nations_cell, data + 0x4000);
    c.put32(ireland_cell, active.nation_index);
    c.put32(northern_cell, active.nation_index + 7);
    c.put32(bans_cell, data + 0x5000);
    c.put32(persons_cell, data + 0x6000);
    c.put16(today, active.day); c.put16(today + 2, active.year);
    c.put8(today + 4, active.year % 4 == 0 && (active.year % 100 != 0 || active.year % 400 == 0));
    c.put32(trace_at, 0);
    c.set(3, object);
    switch (active.routine) {
    case nation_text: c.set(4, text_buffer); c.set(5, active.width); break;
    case next_card: {
        const std::uint32_t tally = match + 0x100, first = tally + 0x40, second = tally + 0x41;
        constexpr std::array<std::pair<unsigned, unsigned>, 8> shapes{{{0, 0}, {1, 0}, {0, 2}, {1, 2}, {1, 1}, {3, 2}, {1, 3}, {3, 3}}};
        const auto at = [&](unsigned which) { return which == 0 ? 0 : which == 1 ? first : which == 2 ? second : tally + 4; };
        c.put8(tally + 4, active.tally);
        c.set(4, tally); c.set(5, at(shapes[active.outputs].first)); c.set(6, at(shapes[active.outputs].second));
        break;
    }
    case construct: c.set(4, data + 0x7000 + active.seed); break;
    default:
        c.put32(match, 0x40 + active.seed % 50);
        c.put8(match + 0x2c, active.yellows); c.put8(match + 0x2d, active.reds); c.put8(match + 0x48, active.flag_48);
        c.put16(booking + 8, active.points);
        c.set(4, match); c.set(5, fixture_record);
        break;
    }
    c.s.budget = c.s.executed + 1000000;
}
void original(State& s, std::uint32_t entry) {
    original_mode = true;
    calls = 0;
    s.pc = entry;
    run_until(s, sentinel);
}
void rewrite(State& s, std::uint32_t entry) {
    original_mode = false;
    calls = 0;
    if (production_mode) { s.pc = entry; run_until(s, sentinel); }
    else ruling_bodies_run_rewrite(s, entry);
}
void corrupt(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    c.put8(booking + 9, c.u8(booking + 9) ^ 1);
}
struct Observed final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};

std::vector<Case> cases() {
    std::vector<Case> all;
    std::mt19937 random(20261114);
    const auto pick = [&](std::initializer_list<std::uint32_t> values) { return *(values.begin() + random() % values.size()); };
    for (const auto width : {0x80000000u, 0xffffffffu, 0u, 19u, 20u, 21u, 255u, 32767u, 32768u, 0x10013u, 0x10014u, 0x7fffffffu})
        for (const auto index : {0u, 3u, 0xdcu})
            all.push_back({.routine = nation_text, .seed = std::uint32_t(random()), .width = width, .nation_index = index});
    for (unsigned tally = 0; tally < 256; ++tally)
        for (unsigned outputs = 0; outputs < 8; ++outputs)
            all.push_back({.routine = next_card, .seed = tally * 8 + outputs + 1, .tally = std::uint8_t(tally), .outputs = outputs});
    for (unsigned n = 0; n < 16; ++n) all.push_back({.routine = construct, .seed = std::uint32_t(random())});
    for (const auto yellows : {0u, 1u, 2u, 3u, 5u, 13u, 127u})
        for (const auto reds : {0u, 1u, 2u, 255u})
            for (const auto points : {0u, 3u, 7u, 8u, 12u, 15u, 16u, 19u, 20u, 24u, 27u, 28u, 31u, 32u, 37u, 38u, 41u, 42u, 46u,
                                      47u, 50u, 51u, 60u, 0x7fffu, 0x8000u, 0xffd0u, 0xfff0u, 0xfffcu, 0xffffu}) {
                Case t{.routine = report, .seed = std::uint32_t(random())};
                t.yellows = std::uint8_t(yellows); t.reds = std::uint8_t(reds); t.points = std::uint16_t(points);
                t.flag_48 = std::uint8_t(pick({0, 0, 1, 0x80}));
                t.year = std::uint16_t(pick({2001, 2002, 2003, 2004, 2000, 2100}));
                t.day = std::uint16_t(pick({0, 1, 2, 3, 4, 5, 6, 58, 59, 60, 200, 358, 359, 360, 361, 362, 363, 364, 365}));
                t.ban_flagged = random() % 3 == 0;
                all.push_back(t);
            }
    return all;
}
std::string describe(const Case& t, bool production) {
    return hex_address(entries[t.routine]) + " production=" + std::to_string(production) + " seed=" + std::to_string(t.seed) +
           " width=" + hex_address(t.width) + " tally=" + std::to_string(t.tally) + " outputs=" + std::to_string(t.outputs) +
           " yellows=" + std::to_string(t.yellows) + " reds=" + std::to_string(t.reds) + " points=" + std::to_string(t.points) +
           " day=" + std::to_string(t.day) + "/" + std::to_string(t.year) + " clobber=" + std::to_string(t.clobber);
}
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) return 2;
    try {
        auto base = std::make_unique<State>();
        load_program(*base, argv[1]);
        dispatch_probe = probe;
        routine_check_native_contract = true;
        const auto fresh = [&](const Case& test) {
            active = test;
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s);
            initialize(c);
            return s;
        };
        if (argc == 3 && std::string(argv[2]) == "--original-routing") {
            // With CM_RULING_BODIES_ORIGINAL=1 the hooks decline: with no budget, the original stops at its entry.
            unsigned declined = 0;
            production_mode = true;
            for (unsigned routine = 0; routine < entries.size(); ++routine) {
                auto s = fresh({.routine = routine});
                s->budget = s->executed;
                try { rewrite(*s, entries[routine]); }
                catch (const std::exception& e) {
                    declined += std::string(e.what()) == "Native instruction budget exhausted" && s->pc == entries[routine];
                }
            }
            std::cout << declined << "/" << entries.size() << " entries ran their original body\n";
            return declined == entries.size() ? 0 : 1;
        }
        if (argc == 3) return 2;
        const auto all = cases();
        unsigned completed = 0, failures = 0, bans = 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        for (const bool production : {false, true}) {
            production_mode = production;
            for (auto test : all) for (const bool clobber : {false, true}) {
                if (failures >= 6) break;
                test.clobber = clobber;
                auto s = fresh(test);
                Observed observer;
                if (production) s->memory.observer = &observer;
                const auto entry = entries[test.routine];
                const auto differences = compare_routine_call(*s, entry, original, rewrite, ruling_bodies_results(entry));
                if (!differences.empty()) {
                    std::cout << "DIFFERS " << describe(active, production) << '\n';
                    for (const auto& difference : differences) std::cout << "  " << difference << '\n';
                    ++failures;
                } else {
                    ++completed;
                    ++counts[entry][production ? 1 : 0];
                }
            }
            if (failures) break;
        }
        // How many report cases reach a ban, so that the count shows the second half is exercised.
        production_mode = false;
        for (const auto& test : all) {
            if (test.routine != report) continue;
            auto s = fresh(test);
            rewrite(*s, entries[report]);
            bans += Cpu(*s).u32(trace_at) > 30;
        }
        // The comparison must fail when the C++ leaves another byte in the booking record.
        {
            auto s = fresh({.routine = report, .points = 16});
            if (compare_routine_call(*s, entries[report], original, corrupt, 0).empty()) {
                ++failures;
                std::cout << "FAIL corruption control\n";
            } else std::cout << "ok deliberate memory corruption detected\n";
        }
        // Production ownership: with no instruction budget of its own, each entry still returns through its
        // hook, and the original body does not. The text case avoids the assertion, whose version getter is
        // a leaf the runtime lowers and charges to the same budget.
        unsigned ownership = 0;
        for (unsigned routine = 0; routine < entries.size(); ++routine) {
            const Case owned{.routine = routine, .points = 16};
            auto normal = fresh(owned);
            normal->budget = normal->executed;
            Observed observer;
            normal->memory.observer = &observer;
            production_mode = true;
            try { rewrite(*normal, entries[routine]); ++ownership; }
            catch (const std::exception& e) { ++failures; std::cout << "FAIL production ownership " << hex_address(entries[routine]) << ' ' << e.what() << '\n'; }
            production_mode = false;
            auto reference = fresh(owned);
            reference->budget = reference->executed;
            bool rejected = false;
            try { original(*reference, entries[routine]); }
            catch (const std::exception& e) { rejected = std::string(e.what()) == "Native instruction budget exhausted" && reference->pc == entries[routine]; }
            if (!rejected) { ++failures; std::cout << "FAIL original budget control " << hex_address(entries[routine]) << '\n'; }
        }
        for (const auto& [entry, n] : counts) std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        std::cout << bans << " report cases issue a ban\n";
        if (bans < 100) { ++failures; std::cout << "FAIL the report cases no longer reach the ban record\n"; }
        std::cout << ownership << "/" << entries.size() << " production ownership controls; " << completed << " returned comparisons; " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

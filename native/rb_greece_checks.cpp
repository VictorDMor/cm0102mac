#include "rb_greece_adapter.hpp"
#include "calendar.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

#include <random>

#ifdef CM_RB_GREECE_COVERAGE
#include <unistd.h>
extern "C" int __llvm_profile_write_file();
extern "C" void __llvm_profile_set_filename(const char*);
#endif

namespace {
using namespace cm::aot;
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto entry = 0x106ab890u, done = fixture + 0x80;
constexpr auto tally = fixture + 0x100, first = tally + 0x40, second = tally + 0x41;

void check_threshold() {
    std::mt19937 random(0x47524545);
    constexpr std::array outputs{
        std::pair{0u, 0u}, std::pair{first, 0u}, std::pair{0u, second},
        std::pair{first, second}, std::pair{first, first},
        std::pair{tally + 4, second}, std::pair{first, tally + 4}, std::pair{tally + 4, tally + 4},
    };
    for (unsigned count = 0; count < 256; ++count) {
        for (const auto [ban, dismissal] : outputs) {
            auto seed = std::make_unique<State>();
            seed->memory.map(fixture, 0x1000);
            Cpu c(*seed);
            for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, random());
            for (unsigned field = 0; field < 8; ++field) c.set_cr(field, random() & 0xf);
            seed->wr<1>(guest::reg::xer_so, random() & 1);
            c.set({{4, tally}, {5, ban}, {6, dismissal}});
            c.set_lr(done);
            c.put8(tally + 4, count);
            c.put8(first, 0x55);
            c.put8(second, 0xaa);
            seed->pc = entry;
            seed->budget = 32;
            auto old = clone(*seed), modern = clone(*seed);
            Effects old_effects, new_effects;
            old_effects.state = old.get();
            new_effects.state = modern.get();
            old->memory.date_observer = &old_effects;
            modern->memory.date_observer = &new_effects;
            rb_greece_run_original(*old, entry);
            run_until(*modern, done);
            const auto label = "Greek rulebook threshold " + std::to_string(count) + " " +
                hex_address(ban) + " " + hex_address(dismissal);
            require(old->pc == done && modern->pc == done, label + " did not return");
            require(old_effects.bytes == new_effects.bytes, label + " ordered effects mismatch");
            old->memory.date_observer = nullptr;
            modern->memory.date_observer = nullptr;
            const Image expected(*old);
            expected.compare(*modern, label);
            if (count == 2 && ban == first && dismissal == second) {
                Cpu candidate(*modern);
                candidate.set(3, 0);
                bool rejected = false;
                try {
                    expected.compare(*modern, "wrong threshold result");
                } catch (const std::runtime_error& error) {
                    rejected = std::string_view(error.what()).starts_with("wrong threshold result register");
                }
                require(rejected, "threshold corruption control was not rejected");
            }
        }
    }
}

constexpr auto construct = 0x106ab8f0u, format_text = 0x106ab7d0u, report_cards = 0x106ab970u;
constexpr auto nation_output = fixture + 0x180, object = fixture + 0x200, source = fixture + 0x300, message = fixture + 0x400;
constexpr auto success = fixture + 0x500, nation_slot = fixture + 0x510;
constexpr auto nation_array_slot = fixture + 0x514, league_slot = fixture + 0x518, group_slot = fixture + 0x51c;
constexpr auto nation_array = fixture + 0x600, vtable = fixture + 0x900;
constexpr auto report = fixture + 0xb00, details = fixture + 0xb80, booking = fixture + 0xc00;
constexpr auto discipline_slot = fixture + 0xc20, players_slot = fixture + 0xc24, today = fixture + 0xc28;
constexpr auto hearing_enabled = fixture + 0xc30, live_calendar = fixture + 0xc31;
constexpr auto discipline = fixture + 0xd00, date_scratch = fixture + 0xd10;
constexpr auto queue = fixture + 0xa00, hearing_copy = fixture + 0xe00, notice_copy = fixture + 0xec0;
constexpr auto notice_event = fixture + 0xee0, index_base = fixture + 0xf00;
constexpr auto player = 7u;
using Call = std::array<std::uint32_t, 34>;
std::vector<Call>* calls = nullptr;

bool dependency(State& s) {
    Cpu c(s);
    Call call{s.pc, c.lr()};
    for (unsigned reg = 0; reg < 32; ++reg) call[reg + 2] = c.r(reg);
    switch (s.pc) {
    case 0x105bb3e0:
        require(c.r(3) == object && c.r(31) == object && c.sp() == stack - 0x40, "base rulebook ABI");
        c.put32(object + 8, fixture + 0xa00);
        c.set(3, object);
        break;
    case 0x10168a00:
        require(c.r(3) == nation_array + 0x128 && c.r(4) == nation_output && c.r(6) == 13,
                "nation formatting ABI");
        c.put32(c.r(4), 0x47726565);
        c.set(3, c.r(4));
        break;
    case 0x10739890:
        require(c.r(3) == source && !c.r(4) && !c.r(5) && c.r(6) == c.sp() + 0x40 &&
                    c.r(7) == c.sp() + 0x38, "diagnostic filename ABI");
        c.put8(c.r(6), 'G');
        c.put8(c.r(7), 'C');
        break;
    case 0x1077b720:
        require(c.r(3) == message && c.r(4) == source + 0xe && c.r(5) == source + 0x30 &&
                    c.u8(c.r(6)) == 'G' && c.u8(c.r(7)) == 'C' && c.r(8) == 0xbc,
                "diagnostic format ABI: " + hex_address(c.r(3)) + " " + hex_address(c.r(4)) + " " +
                    hex_address(c.r(5)) + " " + hex_address(c.r(6)) + " " + hex_address(c.r(7)) + " " +
                    hex_address(c.r(8)));
        c.put32(message, c.r(8));
        c.set(3, 1);
        break;
    case 0x10769b60:
        require(c.r(3) == source + 0x1b && c.r(4) == message && !c.r(5), "diagnostic display ABI");
        c.set(3, 0);
        break;
    case 0x105bc2b0:
        require(c.r(3) == object && c.r(4) == player && c.r(5) == 1, "booking lookup ABI");
        c.set(3, booking);
        break;
    case 0x105bd010:
        require(c.r(3) == object && c.r(4) == c.sp() + 0x70 && c.r(5) == report &&
                    c.r(6) == 1, "hearing eligibility ABI");
        c.put32(c.r(4), player);
        c.set(3, c.u8(hearing_enabled));
        break;
    case 0x10717890: {
        if (c.u8(live_calendar)) return false;
        const auto from = c.r(4), to = c.r(3), days = c.r(5);
        require((from == today || from == c.sp() + 0x50) &&
                    (days == 1 || days == 14), "discipline date adjustment ABI");
        c.put16(to, c.u16(from) + days);
        c.put16(to + 2, c.u16(from + 2));
        c.put8(to + 4, c.u8(from + 4));
        break;
    }
    case 0x10715b20:
        if (c.u8(live_calendar)) return false;
        require(c.r(3) == c.sp() + 0x50, "hearing weekday ABI");
        c.set(3, cm::calendar::weekday({c.i16(c.r(3)), c.u16(c.r(3) + 2)}, c.u8(c.r(3) + 4)));
        break;
    case 0x10738d10:
        require(c.r(3) == queue && c.r(4) == c.sp() + 0x70 && c.r(5) == 0x78, "hearing queue ABI");
        require(cm::calendar::weekday({c.i16(c.r(4) + 0x18), c.u16(c.r(4) + 0x1a)},
                                      c.u8(c.r(4) + 0x1c)) == 3, "hearing scheduled on Thursday");
        for (unsigned offset = 0; offset < 0x78; ++offset)
            c.put8(hearing_copy + offset, c.u8(c.r(4) + offset));
        break;
    case 0x100dfb30:
        require(c.r(3) == discipline && c.r(4) == player && c.r(5) == c.sp() + 0x58, "suspension registration ABI");
        for (unsigned offset = 0; offset < 0x15; ++offset)
            c.put8(notice_copy + offset, c.u8(c.r(5) + offset));
        break;
    case 0x100e0700:
        require(c.r(3) == discipline && c.r(4) == index_base + player * 0x70 &&
                    c.r(5) == c.sp() + 0x58 && c.r(6) == details, "suspension publication ABI");
        c.put8(notice_event, c.u8(c.r(5)));
        break;
    default: return false;
    }
    calls->push_back(call);
    c.finish();
    return true;
}

struct CheckerScope {
    RoutineChecker* previous = routine_checker;
    ~CheckerScope() { routine_checker = previous; }
};

struct Model {
    decltype(dispatch_probe) previous = dispatch_probe;
    Model() { dispatch_probe = dependency; }
    ~Model() { dispatch_probe = previous; calls = nullptr; }
};

void check_lifetime_and_text() {
    std::mt19937 random(0x52424752);
    Model model;
    bool corrupt_trace_rejected = false;
    const std::array widths{INT32_MIN, -1, 0, 19, 20, 21, 255, 32768, INT32_MAX};
    for (unsigned test = 0; test < 64; ++test) {
        auto seed = std::make_unique<State>();
        seed->memory.map(fixture, 0x1000);
        seed->memory.map(stack - 0x1000, 0x2000);
        seed->memory.map(toc - 0x8000, 0x8000);
        Cpu c(*seed);
        for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, random());
        for (unsigned field = 0; field < 8; ++field) c.set_cr(field, random() & 0xf);
        seed->wr<1>(guest::reg::xer_so, random() & 1);
        for (unsigned offset = 0; offset < 0x100; offset += 4) c.put32(stack - offset, random());
        for (unsigned offset = 0; offset < 0x24; offset += 4) c.put32(object + offset, random());
        c.set({{1, stack}, {2, toc}, {3, object}, {4, nation_output}, {6, 13}});
        c.put32(toc - 0x5ddc, vtable);
        c.put32(toc - 0x7574, league_slot);
        c.put32(toc - 0x7a70, group_slot);
        c.put32(toc - 0x41d4, source);
        // The original inlines the version getter. Both routes read its
        // real TOC slot instead of trying to intercept it.
        c.put32(toc - 0x5b20, source + 0x30);
        c.put32(toc - 0x7ba8, message);
        c.put32(toc - 0x7bac, success);
        c.put32(toc - 0x7598, nation_slot);
        c.put32(toc - 0x7b98, nation_array_slot);
        c.put32(league_slot, random());
        c.put32(group_slot, random());
        c.put32(nation_slot, 1);
        c.put32(nation_array_slot, nation_array);
        c.put8(success, 1);
        const auto target = test < 32 ? construct : format_text;
        c.set(5, widths[test % widths.size()]);
        c.set_lr(done);
        seed->pc = target;
        seed->budget = 100;
        auto old = clone(*seed), modern = clone(*seed);
        std::vector<Call> old_calls, new_calls;
        Effects old_effects, new_effects;
        old_effects.state = old.get();
        new_effects.state = modern.get();
        old->memory.date_observer = &old_effects;
        modern->memory.date_observer = &new_effects;
        calls = &old_calls;
        try {
            rb_greece_run_original(*old, target);
        } catch (const std::exception& error) {
            throw std::runtime_error("original: " + std::string(error.what()));
        }
        calls = &new_calls;
        run_until(*modern, done);
        const auto label = "Greek rulebook " + hex_address(target) + " case " + std::to_string(test);
        require(old->pc == done && modern->pc == done, label + " did not return");
        require(!old_calls.empty() && old_calls == new_calls, label + " ordered call arguments differ");
        require(old_effects.bytes == new_effects.bytes, label + " ordered memory effects differ");
        old->memory.date_observer = nullptr;
        modern->memory.date_observer = nullptr;
        Image(*old).compare(*modern, label);
        if (test == 0) {
            ++new_calls.front()[5];
            corrupt_trace_rejected = old_calls != new_calls;
        }
    }
    require(corrupt_trace_rejected, "wrong dependency argument was not detected");
}


std::unique_ptr<State> discipline_seed(std::mt19937& random) {
    auto seed = std::make_unique<State>();
    seed->memory.map(fixture, 0x2000);
    seed->memory.map(stack - 0x1000, 0x2000);
    seed->memory.map(toc - 0x8000, 0x8000);
    Cpu c(*seed);
    for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, random());
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, random() & 0xf);
    seed->wr<1>(guest::reg::xer_so, random() & 1);
    for (unsigned offset = 0; offset < 0x200; offset += 4) c.put32(stack - offset, random());
    for (unsigned offset = 0; offset < 0x50; offset += 4) c.put32(details + offset, random());
    c.set({{1, stack}, {2, toc}, {3, object}, {4, report}, {5, details}});
    c.put32(toc - 0x7aa4, discipline_slot);
    c.put32(toc - 0x7b80, today);
    c.put32(toc - 0x7be0, players_slot);
    c.put32(discipline_slot, discipline);
    c.put32(players_slot, index_base);
    c.put32(object + 8, queue);
    c.put32(report, player);
    c.put16(today + 2, 2001);
    c.set_lr(done);
    seed->pc = report_cards;
    seed->budget = 10000;
    return seed;
}

void compare_discipline(const State& seed, std::string_view label, bool budget_stop = false) {
    auto old = clone(seed), modern = clone(seed);
    std::vector<Call> old_calls, new_calls;
    Effects old_effects, new_effects;
    const auto invoke = [&](State& s, Effects& effects, std::vector<Call>& trace, bool original) {
        calls = &trace;
        effects.state = &s;
        s.memory.date_observer = &effects;
        bool stopped = false;
        try {
            if (original) rb_greece_run_original(s, report_cards);
            else run_until(s, done);
        } catch (const std::exception& error) {
            if (std::string_view(error.what()) != "Native instruction budget exhausted")
                throw std::runtime_error(std::string(label) + (original ? " original " : " C++ ") +
                    hex_address(s.pc) + ": " + error.what());
            stopped = true;
        }
        s.memory.date_observer = nullptr;
        require(stopped == budget_stop && (stopped || s.pc == done), std::string(label) + " completion");
    };
    invoke(*old, old_effects, old_calls, true);
    invoke(*modern, new_effects, new_calls, false);
    if (old_calls != new_calls) {
        const auto diff = std::mismatch(old_calls.begin(), old_calls.end(), new_calls.begin(), new_calls.end());
        std::string detail = " call " + std::to_string(diff.first - old_calls.begin());
        if (diff.first != old_calls.end() && diff.second != new_calls.end()) {
            const auto args = std::mismatch(diff.first->begin(), diff.first->end(), diff.second->begin());
            detail += " at " + hex_address(diff.first->front()) + " word " +
                std::to_string(args.first - diff.first->begin()) + " expected " +
                hex_address(*args.first) + " got " + hex_address(*args.second);
        }
        throw std::runtime_error(std::string(label) + detail);
    }
    if (old_effects.bytes != new_effects.bytes) {
        const auto diff = std::mismatch(old_effects.log.begin(), old_effects.log.end(),
                                       new_effects.log.begin(), new_effects.log.end(),
            [](const Effects::Access& a, const Effects::Access& b) {
                return a.kind == b.kind && a.address == b.address && a.size == b.size && a.value == b.value;
            });
        throw std::runtime_error(std::string(label) + " effects at access " +
            std::to_string(diff.first - old_effects.log.begin()) +
            (diff.first == old_effects.log.end() ? " end" : " expected " + hex_address(diff.first->address)) +
            (diff.second == new_effects.log.end() ? " end" : " actual " + hex_address(diff.second->address)));
    }
    Image(*old).compare(*modern, std::string(label), budget_stop);
}

void check_discipline() {
    Model model;
    std::mt19937 random(0x52424344);
    for (unsigned total = 0; total < 256; ++total) {
        for (unsigned variant = 0; variant < 8; ++variant) {
            auto seed = discipline_seed(random);
            Cpu c(*seed);
            c.put8(booking + 4, total);
            c.put8(booking + 0xa, variant ? random() : 0);
            c.put8(report + 0x2c, variant / 2);
            c.put8(report + 0x2d, variant % 3);
            c.put8(report + 0x48, variant % 2);
            c.put8(hearing_enabled, (total + variant) % 2);
            c.put16(today, 50 + total % 7);
            compare_discipline(*seed, "report total " + std::to_string(total) + " variant " + std::to_string(variant));
        }
    }
    // A booking in any of the four previous results delays the clean-run
    // reduction. The newest bit is always clear when the current report is clean.
    for (const unsigned history : {0x40u, 0x20u, 0x10u, 0x08u}) {
        auto seed = discipline_seed(random);
        Cpu c(*seed);
        c.put8(booking + 4, 4);
        c.put8(booking + 0xa, history);
        compare_discipline(*seed, "clean match with recent caution " + std::to_string(history));
    }
    auto seed = discipline_seed(random);
    Cpu c(*seed);
    c.put8(report + 0x2c, 128);
    seed->budget = 550;
    compare_discipline(*seed, "signed caution loop limit", true);
}


void check_route_guards() {
    auto state = std::make_unique<State>();
    bool rejected = false;
    try {
        rb_greece_run_rewrite(*state, entry + 4);
    } catch (const std::logic_error& error) {
        rejected = std::string_view(error.what()).starts_with("Unknown rb_greece entry");
    }
    require(rejected, "interior address is not an owned entry");
    state->memory.map(fixture, 0x1000);
    Cpu c(*state);
    c.set({{4, tally}, {5, 0}, {6, 0}});
    c.put8(tally + 4, 2);
    c.set_lr(done);
    state->pc = entry;
    state->budget = 32;
    RoutineChecker checker([](auto) { return false; }, 1, 1);
    CheckerScope restore;
    routine_checker = &checker;
    run_until(*state, done);
    require(c.r(3) == 1 && checker.totals().empty(), "excluded entry takes the independent original route");
}

void check_combined_rulebook() {
    Model model;
    std::mt19937 random(0x4752434d);
    auto seed = discipline_seed(random);
    Cpu initial(*seed);
    initial.put32(toc - 0x5ddc, vtable);
    initial.put32(toc - 0x7574, league_slot);
    initial.put32(toc - 0x7a70, group_slot);
    initial.put32(toc - 0x41d4, source);
    initial.put32(toc - 0x7ba8, message);
    initial.put32(toc - 0x7598, nation_slot);
    initial.put32(toc - 0x7b98, nation_array_slot);
    initial.put32(nation_slot, 1);
    initial.put32(nation_array_slot, nation_array);
    initial.put32(0x108b7ae4, date_scratch);
    initial.put8(live_calendar, 1);
    initial.put8(hearing_enabled, 1);
    initial.put16(today, 363);  // Hearing and suspension dates cross a year boundary.
    initial.put8(booking + 4, 4);
    constexpr auto descriptors = fixture + 0x1500, base_table = fixture + 0x1540;
    constexpr std::array methods{format_text, entry, report_cards};
    for (unsigned i = 0; i < methods.size(); ++i) {
        initial.put32(descriptors + i * 8, methods[i]);
        initial.put32(descriptors + i * 8 + 4, toc);
    }
    initial.put32(base_table + 0x18, descriptors);
    initial.put32(base_table + 0x1c, descriptors + 8);
    initial.put32(vtable + 0x10, descriptors + 16);

    auto old = clone(*seed), modern = clone(*seed);
    std::vector<Call> old_calls, new_calls;
    Effects old_effects, new_effects;
    const auto sequence = [&](State& s, std::vector<Call>& trace, Effects& effects, bool original) {
        Cpu c(s);
        calls = &trace;
        effects.state = &s;
        s.memory.date_observer = &effects;
        const auto call = [&](std::uint32_t target, std::initializer_list<std::pair<unsigned, std::uint32_t>> args,
                              std::uint32_t returned = done) {
            s.pc = target;
            s.budget = s.executed + 10000;
            c.set_lr(returned);
            c.set(args);
            try {
                if (original) rb_greece_run_original(s, target);
                else run_until(s, returned);
            } catch (const std::exception& error) {
                throw std::runtime_error("combined " + hex_address(target) + " at " +
                    hex_address(s.pc) + (original ? " original: " : " C++: ") + error.what());
            }
            require(c.sp() == stack && c.r(2) == toc && s.pc == returned, "combined boundary");
        };
        const auto virtual_call = [&](std::uint32_t table, unsigned slot,
                                      std::initializer_list<std::pair<unsigned, std::uint32_t>> args) {
            c.set(12, c.u32(table + slot));
            call(0x10781320, args);
        };
        call(0x100e80bc, {{3, object}, {4, 0}}, 0x100e80c0);
        require(c.u32(object) == vtable && c.u32(object + 8) == queue, "constructed rulebook and queue");
        virtual_call(base_table, 0x18, {{3, object}, {4, nation_output}, {5, 32}, {6, 13}});
        require(c.u32(nation_output) == 0x47726565, "combined national text");
        c.put8(report + 0x2c, 1);
        virtual_call(c.u32(object), 0x10, {{3, object}, {4, report}, {5, details}});
        require(c.u8(booking + 4) == 5 && c.u8(notice_event) == 1 && c.u8(notice_copy + 0x10) == 1,
                "fifth caution issues a one-match suspension");
        require(c.u16(notice_copy + 6) == 2002, "real calendar crosses the year");
        virtual_call(base_table, 0x1c, {{3, object}, {4, booking}, {5, first}, {6, second}});
        require(c.r(3) == 1 && c.u8(first) == 1 && c.u8(second) == 0xff, "combined threshold lookup");
        c.put8(report + 0x2c, 0);
        c.put8(report + 0x2d, 1);
        virtual_call(c.u32(object), 0x10, {{3, object}, {4, report}, {5, details}});
        require(c.u32(hearing_copy) == player && c.u16(hearing_copy + 0x1a) == 2002 &&
                    c.u8(notice_event) == 1 && c.u8(notice_copy + 0x10) == 2,
                "dismissal queues a hearing and records the suspension");
        s.memory.date_observer = nullptr;
    };
    sequence(*old, old_calls, old_effects, true);
    sequence(*modern, new_calls, new_effects, false);
    require(old_calls == new_calls && old_effects.bytes == new_effects.bytes,
            "combined rulebook ordered effects and dependency arguments");
    Image(*old).compare(*modern, "combined rulebook state");

    RoutineChecker checker([](auto target) {
        return target == construct || target == format_text || target == entry || target == report_cards;
    }, 10, 1);
    checker.strict_volatile = true;
    CheckerScope restore;
    routine_checker = &checker;
    auto checked = clone(*seed);
    Effects checked_effects;
    std::vector<Call> checked_calls;
    sequence(*checked, checked_calls, checked_effects, false);
    for (const auto target : {construct, format_text, entry, report_cards}) {
        const auto expected = target == report_cards ? 2u : 1u;
        const auto& count = checker.totals().at(target);
        require(count.calls == expected && count.checked == expected && count.differences == 0,
                "strict checker reached the direct and virtual rulebook routes");
    }
    std::cout << "Greek rulebook combined: all four entries, real calendar, direct constructor, "
                 "three virtual methods, five strict hook comparisons\n";
}

}

int main() try {
#ifdef CM_RB_GREECE_COVERAGE
    routine_child_exit = [] {
        const auto path = std::string("rb-greece-check-child-") + std::to_string(getpid()) + ".profraw";
        __llvm_profile_set_filename(path.c_str());
        __llvm_profile_write_file();
    };
#endif
    check_threshold();
    check_lifetime_and_text();
    check_discipline();
    check_combined_rulebook();
    check_route_guards();
    std::cout << "Greek rulebook: 4164 completed original/production comparisons, one budget stop and two corruption controls\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}

// DATE.CPP against its generated reference, from a world snapshot and fixtures:
// every entry with dates around year ends, leap years and century rules, counts
// of both signs and beyond a year, aliased arguments, static dates built and not
// yet built, and for the texts every language, style and name. Each call is
// compared with the strict contract (every register at return, the condition
// register, XER and CTR, all of memory): the unit's callers read what it leaves.
#include "date_adapter.hpp"
#include "date.hpp"
#include "match_replay.hpp"
#include "routine_check.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string_view>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool date_reference(State&); }
namespace {
[[maybe_unused]] constexpr std::uint32_t text_at = 0x61000800;
constexpr std::uint32_t fixture = 0x61000000, fixture_size = 0x4000, date_a = fixture + 0x100, date_b = fixture + 0x200,
    result_at = fixture + 0x300, stack_base = 0x71000000, stack_size = 0x20000,
    stack = 0x71010000, sentinel = 0xfffffff0, module_begin = 0x107156b0, module_end = 0x10718b38,
    dialog = 0x10769b60;
constexpr std::int32_t language_slot = -0x35ac;
// The ownership control's instruction budget between calls into other units: the two instructions
// of the version-text leaf (0x10768810) that the assert macro calls, which no dispatch announces.
// The unit's shortest routine has seven, so no generated body of the unit fits.
constexpr std::uint64_t leaf_allowance = 2;
constexpr std::uint64_t shortest_routine = 7;  // 0x10715880
std::uint64_t elsewhere = 0;                   // instructions other units ran during an ownership control
struct Static { std::int32_t date, guard; };
constexpr std::array<Static, 6> statics{{{-0x3fac, -0x35bb}, {-0x3fb0, -0x35bc}, {-0x3fb4, -0x35bd},
                                         {-0x3fb8, -0x35be}, {-0x3fbc, -0x35bf}, {-0x3fc0, -0x35c0}}};

struct Day { std::int32_t day = 0; std::uint32_t year = 2001, leap = 0; };
struct Case {
    std::uint32_t entry = 0;
    std::array<std::uint32_t, 6> r{};  // r3-r8
    Day a, b;
    bool fresh_statics = false;
    int language = -1;                 // -1: the snapshot's
    std::uint32_t seed = 0;
};
bool original_mode = false, production_mode = false, ownership_mode = false, outside = false;
const bool trace = std::getenv("CM_DATE_TRACE") != nullptr;
const char* const only = std::getenv("CM_DATE_ONLY");
std::uint32_t toc = 0;
std::vector<std::uint8_t> saved_statics, saved_guards;
std::uint8_t saved_language = 0;

bool probe(State& s) {
    const auto target = s.pc;
    if (target >= module_begin && target < module_end) {
        if (original_mode) return date_reference(s);
        if (production_mode) return false;
        if (date_lists(target)) { date_run_rewrite(s, target); return true; }
        return false;
    }
    if (target == dialog) {
        // The assertion's dialog needs a window server; both sides skip it the same way.
        Cpu c(s);
        s.pc = c.lr();
        return true;
    }
    if (ownership_mode && !outside) {
        // Other units may run generated code; this one may not (see leaf_allowance).
        outside = true;
        const auto back = Cpu(s).lr();
        s.budget = UINT64_MAX;
        const auto before = s.executed;
        run_until(s, back);
        elsewhere += s.executed - before;
        s.budget = s.executed + leaf_allowance;
        outside = false;
        return true;
    }
    if (trace) std::cerr << "  dispatch " << hex_address(target) << " lr " << hex_address(Cpu(s).lr()) << '\n';
    return false;
}
void put_date(Cpu& c, std::uint32_t at, Day d) {
    c.put16(at, std::uint32_t(d.day));
    c.put16(at + 2, d.year);
    c.put8(at + 4, d.leap);
}
void initialize(Cpu& c, const Case& t) {
    c.s.memory.copy_in(fixture, std::vector<std::uint8_t>(fixture_size, 0xa5));
    c.s.memory.copy_in(stack - 0x4000, std::vector<std::uint8_t>(0x4400, 0xa7));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101 + t.seed * 0x10000);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, (f + 1 + t.seed) & 0xf);
    c.s.wr<1>(reg::xer_ca, t.seed & 1);
    c.s.wr<4>(reg::ctr, 0x43434343);
    c.set(1, stack); c.set(2, toc); c.set_lr(sentinel);
    for (unsigned n = 0; n < t.r.size(); ++n) c.set(3 + n, t.r[n]);
    put_date(c, date_a, t.a);
    put_date(c, date_b, t.b);
    for (unsigned n = 0; n < statics.size(); ++n) {
        const auto at = c.toc(statics[n].date);
        for (unsigned i = 0; i < 8; ++i) c.put8(at + i, t.fresh_statics ? 0x5a + i : saved_statics[n * 8 + i]);
        c.put8(toc + std::uint32_t(statics[n].guard), t.fresh_statics ? 0 : saved_guards[n]);
    }
    c.put8(toc + std::uint32_t(language_slot), t.language < 0 ? saved_language : std::uint32_t(t.language));
    c.s.budget = c.s.executed + 5000000;
}
void original(State& s, std::uint32_t entry) { original_mode = true; s.pc = entry; run_until(s, sentinel); }
void rewrite(State& s, std::uint32_t entry) {
    original_mode = false;
    if (production_mode) { s.pc = entry; run_until(s, sentinel); }
    else date_run_rewrite(s, entry);
}
void spoil_memory(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.put8(result_at + 1, c.u8(result_at + 1) ^ 1); }
void spoil_register(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.set(5, c.r(5) ^ 1); }
void spoil_result(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.set(3, c.r(3) ^ 1); }
void spoil_frame(State& s, std::uint32_t entry) { rewrite(s, entry); Cpu c(s); c.put8(stack - 0x10, c.u8(stack - 0x10) ^ 1); }

std::string describe(const Case& t) {
    std::string text = hex_address(t.entry);
    for (auto value : t.r) text += " " + hex_address(value);
    text += " a=" + std::to_string(t.a.day) + "/" + std::to_string(t.a.year) + "/" + std::to_string(t.a.leap) +
        " b=" + std::to_string(t.b.day) + "/" + std::to_string(t.b.year) + "/" + std::to_string(t.b.leap) +
        " fresh=" + std::to_string(t.fresh_statics) + " language=" + std::to_string(t.language);
    return text;
}
std::uint32_t by_value_high(Day d) { return (std::uint32_t(d.day) & 0xffff) << 16 | (d.year & 0xffff); }
std::uint32_t by_value_low(Day d, std::uint32_t seed) { return d.leap << 24 | (0x00c3d2e1 ^ seed) >> 8; }
bool leap_year(std::uint32_t year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }

std::vector<Case> core_cases() {
    using namespace date;
    std::vector<Case> cases;
    std::uint32_t seed = 0;
    const auto add = [&](Case t) { t.seed = ++seed; cases.push_back(t); };
    const std::array<std::uint32_t, 12> years{0, 1, 1599, 1600, 1601, 1900, 1999, 2000, 2001, 2004, 2100, 65535};
    const auto dated = [](std::int32_t day, std::uint32_t year) { return Day{day, year, std::uint32_t(leap_year(year))}; };
    std::vector<Day> dates;
    for (auto year : {1999u, 2000u, 2001u, 2004u, 2100u})
        for (auto day : {0, 1, 2, 3, 58, 59, 60, 180, 363, 364, 365}) dates.push_back(dated(day, year));
    dates.push_back(Day{366, 2001, 0}); dates.push_back(Day{-1, 2001, 0}); dates.push_back(Day{400, 2000, 1});
    dates.push_back(Day{364, 0, 1}); dates.push_back(Day{0, 65535, 0}); dates.push_back(Day{5, 2001, 1});

    for (std::uint32_t year = 1580; year <= 2420; ++year) add({.entry = entry::is_leap_year, .r = {year}});
    for (std::uint32_t year : {0u, 1u, 3u, 4u, 100u, 400u, 65535u, 65532u, 65500u, 64400u, 0x107d0u, 0xffff07d4u, 0x800007d0u, 0x7fff0064u})
        add({.entry = entry::is_leap_year, .r = {year}});
    add({.entry = entry::set_default, .r = {result_at}});
    for (std::uint32_t day : {0u, 1u, 59u, 365u, 366u, 0xffffu, 0x8000u, 0xabcd0005u})
        for (std::uint32_t year : {1900u, 1999u, 2000u, 2001u, 2004u, 2100u, 0x123407d4u, 0xffffu})
            add({.entry = entry::from_day_and_year, .r = {result_at, day, year}});
    for (auto routine : {entry::from_calendar, entry::assign_from_calendar})
        for (std::uint32_t day : {0x8000u, 0xffffu, 0u, 1u, 15u, 29u, 31u, 32u, 0x7fffu, 0xabcd0010u})
            for (std::uint32_t month : {0x80u, 0xffu, 0u, 1u, 2u, 6u, 11u, 12u, 0x7fu, 0xabcdef03u})
                for (std::uint32_t year : {2000u, 2001u, 1900u})
                    for (std::uint32_t wanted : {0xffffu, 0xabcdffffu, 0u, 3u, 6u, 7u})
                        add({.entry = routine, .r = {result_at, day, month, year, wanted}});
    for (const auto& d : dates)
        for (std::uint32_t wanted : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 0xffffu, 0xabcd0002u})
            for (auto target : {result_at, date_a})
                add({.entry = entry::nearest_weekday, .r = {date_a, wanted, target}, .a = d});
    for (auto year : years)
        for (auto day : {-400, -1, 0, 1, 59, 60, 364, 365, 366, 32767, -32768})
            for (std::uint32_t leap : {0u, 1u, 0x80u, 0xffu}) add({.entry = entry::weekday, .r = {date_a}, .a = {day, year, leap}});
    for (auto routine : {entry::add_days, entry::subtract_days})
        for (const auto& d : dates)
            for (std::uint32_t days : {0u, 1u, 7u, 30u, 200u, 365u, 366u, 367u, 730u, 1500u, 0x7fffu, 0xffffu, 0xfff9u, 0xff38u,
                                        0xfe92u, 0x8001u, 0x10005u, 0xabcdfffbu})  // not -32768: its magnitude is itself, and the original recurses until the stack ends
                for (int shape = 0; shape < 3; ++shape)
                    add({.entry = routine, .r = {shape == 1 ? date_a : result_at, date_a, days}, .a = d, .fresh_statics = shape == 2});
    for (auto routine : {entry::add_weeks, entry::subtract_weeks})
        for (const auto& d : dates)
            for (std::uint32_t weeks : {0u, 1u, 4u, 26u, 52u, 53u, 104u, 255u, 256u, 0x1ffu, 0xabcdef02u})
                for (int shape = 0; shape < 3; ++shape)
                    add({.entry = routine, .r = {shape == 1 ? date_a : result_at, date_a, weeks}, .a = d, .fresh_statics = shape == 2});
    // A static date as the argument: the routine reads what it is writing.
    for (unsigned n = 0; n < 4; ++n)
        for (std::uint32_t count : {1u, 200u, 400u, 0xffffu})
            for (auto routine : {entry::add_days, entry::add_weeks, entry::subtract_days, entry::subtract_weeks})
                add({.entry = routine, .r = {result_at, 0xfffffff0 - n, count}});
    for (const auto& d : dates)
        for (const auto& other : dates)
            for (bool fresh : {false, true}) {
                if (fresh && (d.day + other.day) % 3) continue;
                add({.entry = entry::days_between, .r = {date_a, by_value_high(other), by_value_low(other, seed)}, .a = d, .fresh_statics = fresh});
                add({.entry = entry::interval, .r = {result_at, date_a, by_value_high(other), by_value_low(other, seed)}, .a = d, .fresh_statics = fresh});
            }
    for (const auto& d : dates) {
        add({.entry = entry::days_between, .r = {date_a, by_value_high(dated(100, 1900)), 0}, .a = d});
        add({.entry = entry::days_between, .r = {date_a, by_value_high(dated(100, 2075)), 0}, .a = d});
        add({.entry = entry::interval, .r = {date_a, date_a, by_value_high(dated(100, 2075)), 0}, .a = d});
    }
    for (int day = -3; day <= 370; ++day)
        for (std::uint32_t leap : {0u, 1u, 0x80u})
            for (auto routine : {entry::day_of_month, entry::month}) add({.entry = routine, .r = {date_a}, .a = {day, 2001, leap}});
    for (std::uint32_t language : {0xffu, 0u, 1u, 2u, 3u, 4u, 5u, 8u, 9u, 10u, 0x109u, 0xabcdef03u})
        for (std::uint32_t month : {0xffffu, 0u, 4u, 5u, 6u, 7u, 8u, 11u, 12u, 0x8000u, 0xabcd0005u, 0xabcd0007u})
            add({.entry = entry::month_takes_other_article, .r = {language, month}});
    return cases;
}
std::vector<Case> text_cases() {
    using namespace date;
    std::vector<Case> cases;
    std::uint32_t seed = 0x40;
    const auto add = [&](Case t) { t.seed = ++seed; cases.push_back(t); };
    constexpr std::array formatters{entry::numeric_long_year, entry::numeric, entry::weekday_and_numeric,
        entry::short_weekday_year, entry::short_year, entry::short_weekday, entry::long_weekday_year,
        entry::long_weekday, entry::long_year};
    const std::array<Day, 12> dates{{{0, 2001, 0}, {30, 2001, 0}, {31, 2001, 0}, {58, 2001, 0}, {59, 2000, 1}, {59, 2001, 0},
                                     {150, 1972, 1}, {243, 2002, 0}, {364, 1999, 0}, {365, 2000, 1}, {200, 1600, 1}, {100, 65535, 0}}};
    for (auto routine : formatters) {
        for (const auto& d : dates)
            for (std::uint32_t time : {0u, 1u, 2u, 3u, 0xffu, 0xabcdef01u})
                for (int language : {-1, 0, 3, 4, 5, 9, 10})
                    add({.entry = routine, .r = {date_a, text_at, 100, time}, .a = d, .language = language});
        for (std::uint32_t capacity : {0u, 8u, 9u, 0x11u, 0x12u, 0x31u, 0x32u, 0x7fffu, 0x8000u, 0xabcd0064u})
            for (std::uint32_t time : {0u, 3u})
                add({.entry = routine, .r = {date_a, text_at, capacity, time}, .a = {40, 1972, 1}});
        for (std::uint32_t time : {0u, 3u}) add({.entry = routine, .r = {date_a, 0, 100, time}, .a = {40, 1972, 1}});
        // A day before every month start, a year before 1600, and leap bytes other than 0 and 1.
        for (const auto& d : {Day{-1, 2001, 0}, Day{-400, 2001, 1}, Day{40, 1500, 0}, Day{40, 0, 1}, Day{59, 2001, 0x80}, Day{59, 2001, 0xff}, Day{366, 2001, 0}, Day{32767, 2001, 0}})
            for (std::uint32_t time : {0u, 2u, 3u}) add({.entry = routine, .r = {date_a, text_at, 100, time}, .a = d});
    }
    for (std::uint32_t time : {0u, 1u, 2u, 3u, 0xffu, 0x100u, 0xabcdef02u}) {
        add({.entry = entry::time_of_day_short, .r = {time}});
        for (std::uint32_t capital : {0u, 1u, 0x80u, 0x100u, 0xabcdef00u})
            add({.entry = entry::time_of_day_name, .r = {time, capital}});
    }
    for (int language = 0; language <= 11; ++language)
        for (std::uint32_t style : {0u, 1u, 2u, 3u, 0xffu, 0x102u}) add({.entry = entry::day_month_order, .r = {style}, .language = language});
    for (auto routine : {entry::short_weekday_name, entry::weekday_name, entry::short_month_name, entry::month_name})
        for (std::uint32_t index : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 0xffu, 0x80u, 0x7fu, 0xffffu, 0x8000u, 0x105u, 0xabcd0002u, 0xabcdff02u})
            for (int language : {-1, 0, 4}) add({.entry = routine, .r = {index}, .language = language});
    return cases;
}
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) { std::cerr << "usage: cm-date-checks WORLD_SNAPSHOT [--original-routing]\n"; return 2; }
    try {
        auto s = std::make_unique<State>();
        load_match_snapshot(*s, argv[1]);
        if (trace) {
            for (const auto& [start, size] : s->memory.mapped_ranges()) std::cerr << "mapped " << hex_address(start) << " +" << size << '\n';
            const auto slot = std::uint32_t(s->memory.read<4>(std::uint32_t(s->r<4>(8)) - 0x7e64));
            std::cerr << "import vector " << hex_address(slot) << " -> " << hex_address(std::uint32_t(s->memory.read<4>(slot))) << '\n';
        }
        s->memory.map(fixture, fixture_size);
        s->memory.map(stack_base, stack_size);
        dispatch_probe = probe;
        Cpu c(*s);
        toc = c.r(2);
        for (const auto& which : statics) {
            for (unsigned i = 0; i < 8; ++i) saved_statics.push_back(c.u8(c.toc(which.date) + i));
            saved_guards.push_back(c.u8(toc + std::uint32_t(which.guard)));
        }
        saved_language = c.u8(toc + std::uint32_t(language_slot));
        constexpr unsigned results = RoutineChecker::result_r3;
        auto cases = core_cases();
        for (auto& t : cases)  // 0xfffffff0 - n stands for the n-th static date
            for (auto& value : t.r) if (value > 0xfffffff0 - 6 && value <= 0xfffffff0) value = c.toc(statics[0xfffffff0 - value].date);
        const auto texts = text_cases();
        cases.insert(cases.end(), texts.begin(), texts.end());
        if (argc == 3 && std::string_view(argv[2]) == "--original-routing") {
            // With CM_DATE_ORIGINAL=1 no route may reach the C++.
            for (const auto& t : cases) {
                if (!date_lists(t.entry)) continue;
                initialize(c, t);
                s->pc = t.entry;
                if (try_modern_date(*s, t.entry) || date_dispatch_probe(*s))
                    throw std::runtime_error("original override accepted a replacement");
            }
            std::cout << "original override declined at every entry\n";
            return 0;
        }
        if (argc == 3) return 2;
        unsigned completed = 0, failures = 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        for (bool production : {false, true}) {
            production_mode = production;
            for (const auto& t : cases) {
                if (!date_lists(t.entry) || (only && t.entry != std::stoul(only, nullptr, 16))) continue;
                initialize(c, t);
                const auto differences = compare_routine_call(*s, t.entry, original, rewrite, results, false, true);
                if (!differences.empty()) {
                    std::cout << "DIFFERS production=" << production << ' ' << describe(t) << '\n';
                    for (const auto& d : differences) std::cout << "  " << d << '\n';
                    if (++failures >= 8) break;
                } else { ++completed; ++counts[t.entry][production ? 1 : 0]; }
            }
            if (failures) break;
        }
        production_mode = false;
        // The comparison must fail when memory, a volatile register, the result or the prologue image differs.
        const std::pair<const char*, RoutineRunner> controls[]{
            {"memory", spoil_memory}, {"register", spoil_register}, {"result", spoil_result}, {"frame", spoil_frame}};
        unsigned controlled = 0;
        for (const auto& [name, runner] : controls) {
            const Case t{.entry = date::entry::add_days, .r = {result_at, date_a, 400}, .a = {300, 2000, 1}};
            initialize(c, t);
            if (compare_routine_call(*s, t.entry, original, runner, results, false, true).empty()) {
                ++failures; std::cout << "FAIL " << name << " control\n";
            } else ++controlled;
        }
        // Production ownership: with no instruction budget, the normal route must run each entry
        // without a generated instruction of the unit, and the reference must stop at once.
        unsigned ownership = 0;
        std::map<std::uint32_t, bool> owned;
        for (const auto& t : cases) {
            if (!date_lists(t.entry) || owned[t.entry]) continue;
            owned[t.entry] = true;
            initialize(c, t);
            const auto differences = compare_routine_call(*s, t.entry,
                [](State& state, std::uint32_t entry) {
                    original_mode = false; production_mode = true; ownership_mode = true;
                    const auto before = state.executed;
                    elsewhere = 0;
                    state.budget = state.executed + leaf_allowance; state.pc = entry; run_until(state, sentinel);
                    // A body of one straight-line run is charged after it has run: count what ran here.
                    if (state.executed - before - elsewhere >= shortest_routine)
                        throw std::runtime_error("generated instructions of the unit ran");
                },
                [](State& state, std::uint32_t entry) { original_mode = false; production_mode = false; ownership_mode = false; date_run_rewrite(state, entry); },
                results, false, true);
            if (!differences.empty()) {
                ++failures; std::cout << "FAIL production ownership " << hex_address(t.entry) << '\n';
                for (const auto& d : differences) std::cout << "  " << d << '\n';
            } else ++ownership;
        }
        production_mode = ownership_mode = false;
        for (const auto& [entry, n] : counts)
            std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        std::cout << counts.size() << " entries; " << ownership << " production ownership controls; " << controlled
                  << "/4 negative controls; " << completed << " returned comparisons; " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

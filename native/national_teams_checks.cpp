#include "guest_abi.hpp"
#include "localization_format_adapter.hpp"
#include "utility_object_adapter.hpp"
#include "national_teams_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

#include <random>

namespace {
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto context = 0x102ae9c0u, read_flag = 0x102aea20u, clear_flag = 0x102aea50u;
constexpr auto read_value = 0x102af620u, get_slot = 0x102af650u, get_staff = 0x102b7450u;
constexpr auto tier = 0x102aea80u, prefix = 0x102aeb40u, international_event = 0x102aed80u;
constexpr auto done = fixture + 0x20, manager = fixture + 0x100, nation = fixture + 0x200;
constexpr auto records = fixture + 0x1800, staff_table = fixture + 0x3000, staff_slot = fixture + 0x4100;
constexpr auto club_count = fixture + 0x4200, nation_count = fixture + 0x4204;
constexpr auto club = fixture + 0x4300, unmapped = fixture + 0xa000;
constexpr auto international = 0x102b7c00u, duty = 0x102b7c70u;
constexpr auto player = fixture + 0x5100, date = fixture + 0x5200, national_team = fixture + 0x5300;
constexpr auto regular_club = fixture + 0x5400, team_nation = fixture + 0x5500;
constexpr auto first_star = fixture + 0x5510, second_star = fixture + 0x5514, clubs_slot = fixture + 0x5518;
constexpr auto fixture_slot = fixture + 0x551c, competition = fixture + 0x5520;
constexpr auto fixtures_head = fixture + 0x5600, roster_list = fixture + 0x5700;
constexpr auto roster_nodes = fixture + 0x5800, roster_rows = fixture + 0x5900;
constexpr auto fixture_nodes = fixture + 0x5a00, fixture_rows = fixture + 0x5b00;
constexpr auto label_out = fixture + 0x5c00, label_shared = fixture + 0x5c40, source = fixture + 0x100000;
constexpr auto calendar_slot = fixture + 0x5c80, calendar_index = fixture + 0x5c84, calendar_array = fixture + 0x5c88;
constexpr auto calendar = fixture + 0x6000, calendar_date = fixture + 0x6200, allowed_nation = fixture + 0x6220;
constexpr auto clubs = fixture + 0x7000, first_club = clubs + 0x248, second_club = first_club + 0x248;
constexpr auto first_bound = fixture + 0x6300, second_bound = fixture + 0x6308, cutoff_date = fixture + 0x6310;
constexpr auto disposed = fixture + 0x6318, fixture_count = fixture + 0x631c;
constexpr auto localize = 0x10727890u, dispose_list = 0x10738c20u;
constexpr unsigned record_size = 0x74, group_size = 0x14, staff_size = 0x6c;

std::unique_ptr<State> seed(std::mt19937& random) {
    auto s = std::make_unique<State>();
    s->memory.map(fixture, 0x5000);
    s->memory.map(stack - 0x400, 0x800);
    s->memory.map(toc - 0x8000, 0x8000);
    Cpu c(*s);
    for (unsigned i = 0; i < 32; ++i) c.set(i, random());
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, random() & 0xff);
    s->wr<1>(guest::reg::xer_so, random() & 1);
    s->wr<1>(guest::reg::xer_ca, random() & 1);
    c.set({{1, stack}, {2, toc}});
    c.set_lr(done);
    c.put32(manager + 4, records);
    c.put32(toc - 0x7bbc, staff_slot);
    c.put32(staff_slot, staff_table);
    c.put32(toc - 0x7b50, club_count);
    c.put32(toc - 0x7b04, nation_count);
    c.put32(club_count, 200);
    c.put32(nation_count, 20);
    s->budget = 10000;
    return s;
}

enum class Stop { returned, fault, budget };
enum class Corrupt { none, result, dependency };

using DependencyCall = std::array<std::uint32_t, 34>;
std::vector<DependencyCall>* dependencies = nullptr;

void fixture_date(Cpu& c, std::uint32_t to, int day, unsigned year) {
    c.put16(to, day);
    c.put16(to + 2, year);
    c.put8(to + 4, year % 4 == 0);
}

bool observe_dependency(State& s) {
    Cpu c(s);
    const auto entry = s.pc, resume = c.lr(), frame = c.sp();
    switch (entry) {
    case 0x100ce250: case 0x100c7d00: case 0x10738e00: case 0x10717b20: case 0x10717890:
    case 0x1048b180: case dispose_list: case localize: case 0x107156b0: break;
    default: return false;
    }
    DependencyCall call{entry, resume};
    for (unsigned i = 0; i < 32; ++i) call[i + 2] = c.r(i);
    dependencies->push_back(call);
    switch (entry) {
    case 0x10717b20: case 0x10717890: {
        const bool lower = entry == 0x10717b20;
        require(c.r(3) == frame + (lower ? 0x60 : 0x58) && c.r(4) == date && c.r(5) == 1,
                "duty date window arguments");
        const auto seeded = lower ? first_bound : second_bound;
        fixture_date(c, c.r(3), c.i16(seeded), c.u16(seeded + 2));
        break;
    }
    case 0x1048b180: {
        const bool reserve = (c.u8(player + 0x6a) & 4) != 0;
        require(c.r(3) == c.u32(fixture_slot) && c.r(4) == c.u16(first_bound) &&
                    c.r(5) == c.u16(first_bound + 2) && c.r(6) == c.u16(second_bound) &&
                    c.r(7) == c.u16(second_bound + 2) && c.r(8) == c.u32(national_team) &&
                    c.r(9) == frame + 0x80 && c.r(10) == UINT32_MAX &&
                    c.u32(frame + 0x38) == std::uint32_t(-2) && c.u32(frame + 0x3c) == reserve,
                "duty fixture filters, windows and optional list");
        c.put32(c.r(9) + 4, c.u32(fixtures_head));
        c.set(3, c.u32(fixture_count));
        break;
    }
    case dispose_list:
        require(c.r(3) == frame + 0x80 && c.r(4) == UINT32_MAX, "duty list disposal ABI");
        c.put8(disposed, c.u8(disposed) + 1);
        c.put32(c.r(3) + 4, 0);
        c.set(3, 0xb0b0b0b0);
        break;
    case localize: {
        const auto key = c.r(4) - source;
        const auto output = key == 0x7cb7 ? label_shared : label_out;
        require((key == 0x7c62 || key == 0x7c8d || key == 0x7cb7) &&
                    c.r(3) == output && c.r(5) == source + 0x7c8c, "duty label destination and key");
        c.put32(output, key);
        c.set(3, 0xc0c0c0c0);
        break;
    }
    case 0x107156b0:
        require(c.r(3) == frame + 0x68 && c.r(4) == 90 && c.r(5) == 2000,
                "all-star calendar base and signed year offset");
        fixture_date(c, c.r(3), c.i16(cutoff_date), c.u16(cutoff_date + 2));
        break;
    default:
        return false; // The club classifier and linked-list iterator are real.
    }
    c.finish();
    return true;
}

struct ObserveCalls {
    decltype(dispatch_probe) previous = dispatch_probe;
    explicit ObserveCalls(std::vector<DependencyCall>& trace) {
        dependencies = &trace;
        dispatch_probe = observe_dependency;
    }
    ~ObserveCalls() {
        dispatch_probe = previous;
        dependencies = nullptr;
    }
};

std::unique_ptr<State> compare(const State& input, Stop expected = Stop::returned,
                               Corrupt corrupt = Corrupt::none) {
    auto original = clone(input), rewrite = clone(input);
    Effects original_effects, rewrite_effects;
    std::vector<DependencyCall> original_calls, rewrite_calls;
    const auto run = [&](State& s, Effects& effects, std::vector<DependencyCall>& calls, bool reference) {
        ObserveCalls observer(calls);
        if (input.pc == duty) {
            utility_object_diagnostics(s).original_body = dispose_list;
            localization_format_diagnostics(s).original_body = localize;
        }
        effects.state = &s;
        s.memory.date_observer = &effects;
        std::string error;
        try {
            if (reference) national_teams_run_original(s, input.pc);
            else run_until(s, done);
        } catch (const std::runtime_error& failure) {
            error = failure.what();
        }
        s.memory.date_observer = nullptr;
        return error;
    };
    const auto a = run(*original, original_effects, original_calls, true);
    const auto b = run(*rewrite, rewrite_effects, rewrite_calls, false);
    const auto label = "NATIONAL_TEAMS " + hex_address(input.pc);
    require(a == b && (expected == Stop::fault ? a.starts_with("Unmapped data access at ") :
                      expected == Stop::budget ? a == "Native instruction budget exhausted" : a.empty()),
            label + " outcome " + a + " / " + b);
    if (expected == Stop::returned)
        require(original->pc == done && rewrite->pc == done, label + " normal continuation");
    require(original_effects.bytes == rewrite_effects.bytes, label + " ordered memory effects");
    if (corrupt == Corrupt::dependency) {
        require(!rewrite_calls.empty(), "dependency corruption must reach the shared club query");
        rewrite_calls.front()[5] ^= 1;
    }
    require(original_calls == rewrite_calls, label + " ordered dependency arguments");
    if (corrupt == Corrupt::result) Cpu(*rewrite).set(3, Cpu(*rewrite).r(3) ^ 1);
    Image(*original).compare(*rewrite, label, expected == Stop::fault);
    return rewrite;
}

unsigned check_selection_slots(std::mt19937& random) {
    unsigned comparisons = 0;
    for (const auto entry : {read_flag, clear_flag, read_value, get_slot}) {
        for (int id : {-4, 0, 1, 5}) {
            for (unsigned group : {0u, 1u, 2u, 0x100u, 0x101u, 0x7fu, 0x80u, 0xffu}) {
                auto s = seed(random);
                Cpu c(*s);
                const auto slot = records + std::uint32_t(id) * record_size +
                                  std::uint32_t(std::int8_t(group)) * group_size + 0x40;
                c.put32(nation + 0x68, std::uint32_t(id));
                const auto value = (random() & 0x7f) | 0x80, flag = (random() & 0x7f) + 1;
                c.put8(slot + 0xa, value);
                c.put8(slot + 0xb, flag);
                c.set({{3, manager}, {4, nation}, {5, group}});
                s->pc = entry;
                auto result = compare(*s);
                ++comparisons;
                Cpu after(*result);
                const auto expected = entry == read_flag ? flag : entry == read_value ? value :
                    entry == get_slot ? slot : slot - 0x40;
                require(after.r(3) == expected && after.u8(slot + 0xa) == value &&
                            after.u8(slot + 0xb) == (entry == clear_flag ? 0 : flag),
                        "selection reads unsigned bytes and clears just the flag");
            }
        }
    }
    return comparisons;
}

unsigned check_selection_tiers(std::mt19937& random) {
    unsigned comparisons = 0;
    for (const unsigned mode : {0u, 1u, 0x100u, 0x180u, UINT32_MAX}) {
        for (const int pool : {-1, 0, 49, 50, 51, 99, 100, 101, INT16_MAX}) {
            for (unsigned first_stop = 0; first_stop <= 6; ++first_stop) {
                auto state = seed(random);
                Cpu c(*state);
                c.set({{4, club}, {5, nation}, {6, mode}});
                c.put32(club + 0x14, club + 0x80);
                c.put16(nation, pool);
                for (unsigned score = 0; score < 6; ++score) {
                    const auto value = score < first_stop ? (score % 2 ? 0x80 : 14) :
                        score == first_stop ? 15 : 127;
                    c.put8(club + 0x8f + score, value);
                }
                state->pc = tier;
                const unsigned expected = pool < (std::int8_t(mode) ? 50 : 100) ? 0 :
                    first_stop == 0 ? 0 : first_stop <= 2 ? 1 : first_stop <= 5 ? 2 : 3;
                auto result = compare(*state);
                require(Cpu(*result).r(3) == expected, "signed pool limit and first limiting assessment");
                ++comparisons;
            }
        }
    }
    return comparisons;
}

unsigned check_selection_prefixes(std::mt19937& random) {
    unsigned comparisons = 0;
    for (const auto group : {0u, 1u, 0x17fu, 0x180u, UINT32_MAX}) {
        for (const auto count : {0u, 1u, 7u, 8u, 9u, 15u, 16u, 17u, 24u, 25u, 127u,
                                 0x100u, 0x101u, 0x109u, 0x180u, UINT32_MAX}) {
            auto state = seed(random);
            Cpu c(*state);
            c.set({{4, records}, {5, group}, {6, count}});
            const auto row = records + std::uint32_t(std::int8_t(group)) * 0x18;
            std::uint32_t expected = UINT32_MAX - (random() & 0xf);
            c.put32(row + 8, expected);
            for (int i = 0; i < std::int8_t(count); ++i) {
                const auto weight = (random() & 0x7f) + 0x80;
                c.put8(row + 0xf + i * 4, weight);
                expected += weight;
            }
            state->pc = prefix;
            auto result = compare(*state);
            require(Cpu(*result).r(3) == expected, "signed group and count, unsigned weights and wrapping total");
            ++comparisons;
        }
    }
    return comparisons;
}

unsigned check_international_competition(std::mt19937& random) {
    constexpr std::array toc_offsets{-0x7bc0, -0x7a40, -0x6948, -0x694c, -0x6950,
                                     -0x693c, -0x6a3c, -0x6938, -0x6b00};
    unsigned comparisons = 0;
    for (unsigned match = 0; match <= toc_offsets.size() + 2; ++match) {
        auto state = seed(random);
        Cpu c(*state);
        const auto id = match % 2 ? 7u : UINT32_MAX;
        const auto available = toc_offsets.size();
        for (unsigned slot = 0; slot < available; ++slot) {
            const auto entry = records + slot * 4;
            const auto hit = slot == match || (match == available + 2 && (slot == 2 || slot == 6));
            c.put32(toc + toc_offsets[slot], entry);
            c.put32(entry, hit ? id : slot + 10);
        }
        c.put32(nation, id);
        c.set(4, match == available + 1 ? 0 : nation);
        state->pc = international_event;
        auto result = compare(*state);
        require(Cpu(*result).r(3) == (match < available || match == available + 2),
                "nullable competition and ordered dynamic IDs");
        ++comparisons;
    }
    return comparisons;
}

unsigned check_selection_query_faults(std::mt19937& random) {
    unsigned faults = 0;
    for (const auto entry : {tier, prefix, international_event}) {
        auto state = seed(random);
        Cpu c(*state);
        c.set({{4, unmapped}, {5, unmapped}, {6, 0}});
        state->pc = entry;
        if (entry == international_event) c.put32(toc - 0x7bc0, nation);
        compare(*state, Stop::fault);
        ++faults;
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    for (const unsigned count : {3u, 9u}) {
        auto state = seed(random);
        Cpu c(*state);
        const auto row = fixture + 0x5000 - (count == 3 ? 20 : 24);
        c.set({{4, row}, {5, 0}, {6, count}});
        c.put32(row + 8, 17);
        c.put8(row + 0xf, 3);
        c.put8(row + 0x13, 255);
        state->pc = prefix;
        compare(*state, Stop::fault);
        ++faults;
    }
    return faults;
}

unsigned check_staff(std::mt19937& random) {
    unsigned comparisons = 0;
    for (unsigned nation_arg : {0u, nation}) {
        for (unsigned id : {0u, 1u, 3u, UINT32_MAX}) {
            for (unsigned staff : {0u, 2u, 9u, UINT32_MAX, UINT32_MAX - 1, 0x80000000u}) {
                auto s = seed(random);
                Cpu c(*s);
                c.put32(nation + 0x68, id);
                c.put32(records + id * record_size + 4, staff);
                c.set({{3, manager}, {4, nation_arg}});
                s->pc = get_staff;
                auto result = compare(*s);
                ++comparisons;
                require(Cpu(*result).r(3) == (nation_arg && std::int32_t(staff) >= 0 ? staff_table + staff * staff_size : 0),
                        "signed staff sentinel and record stride");
            }
        }
    }
    return comparisons;
}

unsigned check_context(std::mt19937& random) {
    unsigned comparisons = 0;
    for (unsigned id : {0u, 179u, 180u, 199u, 200u}) {
        for (unsigned mode : {0u, 1u, 0x100u, 0x180u, UINT32_MAX}) {
            auto s = seed(random);
            Cpu c(*s);
            c.put32(club, id);
            c.set({{3, manager}, {4, club}, {5, mode}});
            s->pc = context;
            auto result = compare(*s);
            ++comparisons;
            require(Cpu(*result).r(3) == (std::int8_t(mode) ? 1u : id >= 180 ? 2u : 0u),
                    "selection context uses the actual club-kind query and low byte");
        }
    }
    return comparisons;
}

unsigned check_international_club(std::mt19937& random) {
    unsigned comparisons = 0;
    for (auto id : {0u, 1u, 2u, 3u, 159u, 160u, 199u}) {
        auto state = seed(random);
        Cpu c(*state);
        c.put32(toc - 0x7208, club_count + 8);
        c.put32(toc - 0x7204, club_count + 12);
        c.put32(club_count + 8, 1);
        c.put32(club_count + 12, 2);
        c.put32(club, id);
        c.set(4, club);
        state->pc = international;
        auto result = compare(*state);
        require(Cpu(*result).r(3) == (id >= 160 || id == 1 || id == 2), "international and reserved club classification");
        ++comparisons;
    }
    return comparisons;
}

std::unique_ptr<State> duty_seed(std::mt19937& random) {
    auto state = seed(random);
    state->memory.map(fixture + 0x5000, 0x4000);
    Cpu c(*state);
    c.set({{3, manager}, {4, player}, {5, date}, {6, regular_club}, {7, label_out}, {8, 0}});
    c.put32(toc - 0x7208, first_star);
    c.put32(toc - 0x7204, second_star);
    c.put32(toc - 0x7bb8, label_shared);
    c.put32(toc - 0x7be8, clubs_slot);
    c.put32(toc - 0x513c, source);
    c.put32(toc - 0x7a60, fixture_slot);
    c.put32(toc - 0x6868, calendar_index);
    c.put32(toc - 0x7b5c, calendar_slot);
    c.put32(toc - 0x7b70, allowed_nation);
    c.put32(first_star, 1);
    c.put32(second_star, 2);
    c.put32(clubs_slot, clubs);
    c.put32(fixture_slot, 0x51515151);
    c.put32(regular_club, 7);
    c.put32(national_team, 123);
    c.put32(player + 0x20, national_team);
    c.put32(competition, 42);
    c.put32(calendar_slot, calendar_array);
    c.put32(calendar + 0xa0, calendar_date);
    c.put16(calendar + 0x40, 2001);
    c.put16(calendar_date + 4, 90);
    c.put8(calendar_date + 6, 0xff);
    c.put32(allowed_nation, 14);
    c.put32(team_nation, 14);
    c.put32(regular_club + 0x30, team_nation);
    fixture_date(c, date, 90, 2000);
    fixture_date(c, first_bound, 84, 2000);
    fixture_date(c, second_bound, 96, 2000);
    fixture_date(c, cutoff_date, 90, 2000);
    state->pc = duty;
    return state;
}

void link_two(Cpu& c, std::uint32_t nodes, std::uint32_t values) {
    c.put32(nodes, values);
    c.put32(nodes + 8, nodes + 0x10);
    c.put32(nodes + 0x10, values + 0x20);
}

unsigned check_duty(std::mt19937& random) {
    unsigned comparisons = 0;
    const auto check = [&](const State& state, unsigned result, unsigned key = 0) {
        auto rewrite = compare(state);
        Cpu c(*rewrite);
        require(c.r(3) == result && c.u8(disposed) == 1, "duty result and one list disposal");
        require(c.u32(label_out) == (key == 0x7cb7 ? 0 : key) &&
                    c.u32(label_shared) == (key == 0x7cb7 ? key : 0), "duty label effect");
        ++comparisons;
    };
    for (unsigned id : {1u, 2u, 7u, 160u}) {
        auto state = duty_seed(random);
        Cpu(*state).put32(regular_club, id);
        check(*state, 0);
    }
    for (bool filter : {false, true}) {
        for (bool found : {false, true}) {
            for (bool label : {false, true}) {
                for (unsigned flags : {0u, 4u, 0x84u}) {
                    auto state = duty_seed(random);
                    Cpu c(*state);
                    c.set({{7, label ? label_out : 0}, {8, filter ? competition : 0}});
                    c.put8(player + 0x6a, flags);
                    c.put32(fixture_count, 2);
                    c.put32(fixtures_head, fixture_nodes);
                    link_two(c, fixture_nodes, fixture_rows);
                    if (found) c.put32(fixture_rows + 0x20 + 0x14, competition);
                    const bool eligible = !filter || found;
                    check(*state, eligible ? national_team : 0, eligible && label ? 0x7c62 : 0);
                }
            }
        }
    }
    for (const auto [day, year] : std::array<std::pair<int, unsigned>, 6>{{
             {80, 1999}, {100, 2001}, {89, 2000}, {90, 2000}, {91, 2000}, {-1, 2000}}}) {
        for (unsigned filter : {0u, 1u, 2u}) {
            auto state = duty_seed(random);
            Cpu c(*state);
            c.set(8, filter ? competition : 0);
            c.put8(player + 0x6a, 4);
            c.put32(manager, roster_list);
            c.put32(roster_list + 4, roster_nodes);
            link_two(c, roster_nodes, roster_rows);
            c.put32(roster_rows + 0x20, 123);
            c.put32(roster_rows + 0x24, filter == 2 ? 7 : 42);
            fixture_date(c, roster_rows + 0x28, day, year);
            const bool matches = filter != 2 && (year < 2000 || (year == 2000 && day < 90));
            check(*state, matches ? national_team : 0, matches ? 0x7c8d : 0);
        }
    }
    {
        auto state = duty_seed(random);
        Cpu c(*state);
        c.put32(manager, roster_list);
        c.put32(roster_list + 4, roster_nodes);
        c.put32(roster_nodes, roster_rows);
        c.put32(roster_rows, 123);
        fixture_date(c, roster_rows + 8, 89, 2000);
        check(*state, 0); // The senior-team path skips even an otherwise matching roster.
        c.put8(player + 0x6a, 4);
        c.set(7, 0);
        check(*state, national_team);
    }
    for (unsigned scenario = 0; scenario < 17; ++scenario) {
        auto state = duty_seed(random);
        Cpu c(*state);
        c.set({{6, 0}, {7, scenario == 3 || scenario == 16 ? 0 : label_out}});
        c.put32(player + 0x20, 0);
        if (scenario != 4) c.put32(calendar_array, calendar);
        if (scenario != 5) c.put32(player + 0x24, regular_club);
        if (scenario == 6) c.put32(regular_club + 0x30, 0);
        if (scenario == 7) c.put32(allowed_nation, 13);
        if (scenario == 8) fixture_date(c, cutoff_date, 97, 2000);
        if (scenario == 9) fixture_date(c, cutoff_date, 1, 2001);
        if (scenario == 10) fixture_date(c, cutoff_date, 366, 1999);
        if (scenario == 11) fixture_date(c, cutoff_date, 83, 2000);
        if (scenario == 12) fixture_date(c, cutoff_date, 84, 2000);
        if (scenario == 13) fixture_date(c, cutoff_date, 96, 2000);
        if (scenario == 14) {
            fixture_date(c, first_bound, 366, 1999);
            fixture_date(c, second_bound, 1, 2001);
        }
        if (scenario == 15) {
            fixture_date(c, first_bound, -2, 2000);
            fixture_date(c, cutoff_date, 0, 2000);
        }
        const auto star_club = scenario == 2 || scenario == 16 ? second_club : first_club;
        if (scenario != 0) c.put32(star_club + 0x98 + 49 * 4, player);
        if (scenario == 1) c.put32(second_club + 0x98 + 49 * 4, player);
        const auto expected = scenario == 1 || scenario == 2 || scenario == 3 || scenario >= 12 ? star_club : 0;
        check(*state, expected, expected && c.r(7) ? 0x7cb7 : 0);
    }
    return comparisons;
}

void check_controls(std::mt19937& random) {
    for (const auto entry : {context, read_flag, clear_flag, read_value, get_slot, get_staff}) {
        auto s = seed(random);
        Cpu c(*s);
        c.set({{3, manager}, {4, unmapped}, {5, 0}});
        s->pc = entry;
        compare(*s, Stop::fault);
        s->budget = 0;
        compare(*s, Stop::budget);
    }
    for (const auto entry : {international, duty}) {
        auto state = duty_seed(random);
        Cpu c(*state);
        c.set(4, entry == international ? regular_club : unmapped);
        if (entry == international) c.put32(toc - 0x7204, unmapped);
        state->pc = entry;
        compare(*state, Stop::fault);
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    auto s = seed(random);
    Cpu(*s).set({{3, manager}, {4, nation}, {5, 0}});
    s->pc = read_value;
    bool rejected = false;
    try { compare(*s, Stop::returned, Corrupt::result); }
    catch (const std::runtime_error& error) {
        rejected = std::string_view(error.what()).find("register byte") != std::string_view::npos;
    }
    require(rejected, "corrupted result must fail the differential check");

    Cpu(*s).set({{3, manager}, {4, club}, {5, 0}});
    s->pc = context;
    rejected = false;
    try { compare(*s, Stop::returned, Corrupt::dependency); }
    catch (const std::runtime_error& error) {
        rejected = std::string_view(error.what()).find("ordered dependency arguments") != std::string_view::npos;
    }
    require(rejected, "corrupted dependency argument must fail the differential check");
}
}

int main() try {
    std::mt19937 random(0x4e41544c);
    const auto slots = check_selection_slots(random);
    const auto staff = check_staff(random);
    const auto contexts = check_context(random);
    check_controls(random);
    const auto eligibility = check_international_club(random) + check_duty(random);
    const auto queries = check_selection_tiers(random) + check_selection_prefixes(random) +
        check_international_competition(random);
    const auto query_faults = check_selection_query_faults(random);
    std::cout << queries << " NATIONAL_TEAMS selection tier, prefix and competition comparisons, "
              << query_faults << " faults and 3 budget stops\n";
    std::cout << eligibility << " NATIONAL_TEAMS club and duty eligibility comparisons\n";
    std::cout << slots + staff + contexts << " NATIONAL_TEAMS returned comparisons, "
                 "8 faults, 8 budget stops and two rejected corruptions\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}

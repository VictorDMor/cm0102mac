#include "guest_abi.hpp"
#include "national_teams_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

#include <random>

namespace {
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto context = 0x102ae9c0u, read_flag = 0x102aea20u, clear_flag = 0x102aea50u;
constexpr auto read_value = 0x102af620u, get_slot = 0x102af650u, get_staff = 0x102b7450u;
constexpr auto done = fixture + 0x20, manager = fixture + 0x100, nation = fixture + 0x200;
constexpr auto records = fixture + 0x1800, staff_table = fixture + 0x3000, staff_slot = fixture + 0x4100;
constexpr auto club_count = fixture + 0x4200, nation_count = fixture + 0x4204;
constexpr auto club = fixture + 0x4300, unmapped = fixture + 0x8000;
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

bool observe_dependency(State& s) {
    constexpr auto club_kind = 0x100ce250u;
    if (s.pc == club_kind) {
        Cpu c(s);
        DependencyCall call{s.pc, c.lr()};
        for (unsigned i = 0; i < 32; ++i) call[i + 2] = c.r(i);
        dependencies->push_back(call);
    }
    return false;
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
    std::cout << slots + staff + contexts << " NATIONAL_TEAMS returned comparisons, "
                 "6 faults, 6 budget stops and two rejected corruptions\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}

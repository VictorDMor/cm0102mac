#include "guest_abi.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include "simulated_stats_adapter.hpp"
#include "simulated_stats_detail.hpp"
#include "localization_format_adapter.hpp"

#include <array>
#include <string_view>

#ifdef CM_SIMULATED_STATS_PROFILE_DIR
#include <unistd.h>
extern "C" int __llvm_profile_write_file();
extern "C" void __llvm_profile_set_filename(const char*);
#endif

namespace {
using namespace cm::aot;
using namespace cm::semantic_io_checks;
using guest::Cpu;

constexpr auto destroy = 0x10370db0u, count_squad = 0x10371d90u;
constexpr auto initialize = 0x1036ff60u;
constexpr auto load = 0x10371e60u, save = 0x10372040u, sample = 0x10370830u;
constexpr auto reseed = 0x1077b770u;
constexpr auto cursor = fixture + 0x1e00, salt = cursor + 4, end = cursor + 8;
constexpr auto random_words = cursor + 0x10, constants = cursor + 0x40;
constexpr auto open_file = 0x107475b0u, read_file = 0x10747a80u, write_file = 0x10747cc0u;
constexpr auto close_file = 0x10747900u, allocate_rows = 0x10775130u;
constexpr auto file = fixture + 0x1a00, stream = fixture + 0x1a40, pool = fixture + 0x1b00;
constexpr auto persisted = fixture + 0x1c00, diagnostic = fixture + 0x1d00;
constexpr auto mode = fixture + 0x94, position = fixture + 0x98, save_ok = fixture + 0x9c;
constexpr auto record_size = 0x44u;
constexpr auto free_rows = 0x10775170u, delete_object = 0x107735a0u;
constexpr auto resolve_role = 0x10187680u, check_exclusion = 0x102b7c70u;
constexpr auto object = fixture + 0x100, rows = fixture + 0x300;
constexpr auto club = fixture + 0x800, staff = fixture + 0xd00;
constexpr auto marker = fixture + 0x90, live_profile = fixture + 0xc00;
constexpr auto clubs_slot = fixture + 0x20, role_service = fixture + 0x24;
constexpr auto filter_service = fixture + 0x28, date = fixture + 0x2c;
constexpr auto member_stride = 0x40u, squad = club + 0x98;
constexpr auto club_count = fixture + 0x30, competitions_slot = fixture + 0x34;
constexpr auto competitions = fixture + 0x40, competition = fixture + 0x60;
constexpr auto media_flag = fixture + 0xa0, media_attempt = fixture + 0xa4;
constexpr auto media_first = fixture + 0x1f00, media_second = media_first + 0x20;
constexpr auto initial_clubs = fixture + 0x10000, initial_list = fixture + 0x80000;
constexpr auto initial_rows = fixture + 0x81000;
constexpr auto loader_resume = fixture + 0x1f80;

struct Call {
    std::uint32_t target, continuation;
    std::array<std::uint32_t, 8> arguments;
    bool operator==(const Call&) const = default;
};

struct Trace {
    std::vector<Call> calls;
    bool change_argument = false;
};

Trace* active_trace = nullptr;

bool dependency(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    const auto arg0 = c.r(3);
    const auto member = c.r(4);
    const auto behavior = [&] { return c.u8(mode); };
    switch (target) {
        case reseed:
            require(c.lr() == 0x10370884 || c.lr() == 0x10370970 || c.lr() == 0x103702dc
                        || c.lr() == 0x10370440 || c.lr() == 0x10370598, "Stats RNG reseed call site");
            break;
        case open_file:
            require((arg0 == file || arg0 == 0) && member == pool + 0x5e && c.r(5) <= 1
                        && c.r(6) == 0x16 && c.r(7) == 4, "Simulated stats file-open arguments");
            break;
        case read_file: case write_file: {
            const auto size = c.r(4), count = c.r(5);
            require(c.r(6) == stream && ((arg0 == object && size == 4 && count == 1)
                        || ((arg0 == rows || arg0 == 0) && size == record_size && count <= 3)),
                    "Simulated stats transfer arguments");
            break;
        }
        case allocate_rows:
            require((arg0 == c.u32(object) * record_size && c.lr() == 0x10371f94)
                        || (arg0 == c.u32(club_count) * 4 && c.lr() == 0x103700dc)
                        || (arg0 == c.u32(object) * record_size && c.lr() == 0x10370708),
                    "Simulated stats allocation arguments");
            break;
        case close_file:
            require(arg0 == c.sp() + 0x38 && c.u32(arg0) == stream, "Simulated stats close arguments");
            break;
        case loader_resume:
            require(c.r(3) == 1, "Stats loader nonlocal return value");
            break;
        case 0x10739890:
            require(arg0 == pool + 0x37 && member == 0 && c.r(5) == 0, "Stats diagnostic arguments");
            break;
        case 0x1077b720:
            require(arg0 == diagnostic && member == pool + 0x4b && c.r(5) == 0x10203040,
                    "Stats diagnostic formatting arguments");
            break;
        case 0x10769b60:
            require((arg0 == pool + 0x58 && member == diagnostic && c.r(5) == 0)
                        || (arg0 == c.sp() + 0x2bc && member == c.sp() + 0x1f4 && c.r(5) == 1),
                    "Stats diagnostic alert arguments");
            break;
        case 0x1077fee0:
            require(arg0 == 0 && member == 0 && c.r(5) == c.sp() + 0xf4 && c.r(6) == c.sp() + 0xac
                        && c.u8(c.r(5)) == 4 && c.u32(c.r(5) + 1) == 0x41614262,
                    "Stats media spec arguments and interleaved Pascal name");
            break;
        case 0x10727890:
            require((arg0 == c.sp() + 0x2bc && member == pool)
                        || (arg0 == c.sp() + 0x1f4 && member == pool + 10), "Stats media warning copy");
            break;
        case 0x1076f7d0:
            require(arg0 == UINT32_MAX && c.lr() == 0x1037013c && c.u8(save_ok) == 0
                        && c.u32(diagnostic) == 0x49, "Stats failed allocation exit");
            break;
        case 0x102b7c00:
            require(arg0 == filter_service && c.lr() == 0x103701ac && member >= initial_clubs
                        && member < initial_clubs + c.u32(club_count) * 0x248,
                    "Stats eligible club callback");
            break;
        case free_rows:
            require((arg0 == rows && c.lr() == 0x10370de0)
                        || (arg0 == initial_list && c.lr() == 0x10370814), "Stats row release arguments");
            c.put8(marker, 1);
            break;
        case delete_object:
            require(arg0 == object && c.lr() == 0x10370df0, "Stats object delete arguments");
            c.put8(marker, c.u8(marker) + 2);
            break;
        case resolve_role:
            require(arg0 == role_service && c.lr() == 0x10371df8 && member >= staff,
                    "Squad role service arguments");
            break;
        case check_exclusion:
            require(arg0 == filter_service && c.lr() == 0x10371e20 && member >= staff
                        && c.r(5) == date && c.r(6) == club && c.r(7) == 0 && c.r(8) == 0,
                    "Squad exclusion service arguments");
            break;
        default:
            return false;
    }
    if (active_trace) {
        auto observed = member;
        if (active_trace->change_argument && (target == check_exclusion || target == 0x102b7c00)) observed ^= 4;
        active_trace->calls.push_back({target, c.lr(), {arg0, observed, c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)}});
    }
    if (target == reseed) c.set(3, 0x1234f123);
    if (target == open_file) {
        c.put32(position, 0);
        c.set(3, arg0 == 0 || behavior() == 1 ? 0 : stream);
    }
    if (target == read_file || target == write_file) {
        const auto size = c.r(4), count = c.r(5);
        const auto at = c.u32(position);
        const bool header = size == 4;
        const auto requested = size * count;
        const auto fail = behavior() == (header ? 2 : 3);
        const auto transferred = fail && requested ? requested - 1 : requested;
        if (transferred) {
            if (target == read_file) s.memory.copy_in(arg0, s.memory.copy_out(persisted + at, transferred));
            else s.memory.copy_in(persisted + at, s.memory.copy_out(arg0, transferred));
        }
        c.put32(position, at + transferred);
        if (!header && behavior() == 4) c.put32(object, count + 1);
        c.set(3, fail && count ? count - 1 : count);
    }
    if (target == allocate_rows) {
        const auto allocation = c.lr() == 0x103700dc ? initial_list : c.lr() == 0x10370708 ? initial_rows : rows;
        c.set(3, behavior() == 6 || (behavior() == 7 && c.lr() == 0x10370708) ? 0 : allocation);
    }
    if (target == 0x1077fee0) {
        const auto attempt = c.u8(media_attempt);
        c.put8(media_attempt, attempt + 1);
        c.put32(c.sp() + 0x14, toc);
        c.put32(c.r(6), 0x12345678);
        c.set(3, attempt < behavior() ? 0xffffffd5 : 0);
    }
    if (target == 0x10727890) c.put32(arg0, c.u32(member));
    if (target == 0x1076f7d0) throw std::runtime_error("Stats failed allocation exit");
    if (target == 0x102b7c00) {
        c.set(3, c.u32(member + 0x10));
        const auto next = c.u32(member + 0x14);
        if (next) c.put32(next, 0);
    }
    if (target == close_file) {
        if (behavior() == 8) c.put32(c.sp() + 0xd8, loader_resume);
        c.put32(arg0, 0);
        c.set(3, 0x42);
    }
    if (target == loader_resume) c.set_lr(0x103700c8);
    if (target == 0x10739890) {
        c.put32(c.r(6), arg0);
        c.put32(c.r(7), arg0 + 4);
    }
    if (target == 0x1077b720) c.put32(diagnostic, c.r(8));
    if (target == 0x10769b60) c.set(3, 0x43);
    if (target == resolve_role) c.set(3, c.u32(member + 0x1c));
    if (target == check_exclusion) {
        c.set(3, c.u32(member + 0x20));
        const auto clear_slot = c.u32(member + 0x24);
        if (clear_slot) c.put32(clear_slot, 0);
    }
    c.finish();
    return true;
}

struct Services {
    decltype(dispatch_probe) previous_probe = dispatch_probe;
    RoutineChecker* previous_checker = routine_checker;
    Trace* previous_trace = active_trace;
    Services(Trace* trace, RoutineChecker* checker) {
        dispatch_probe = dependency;
        routine_checker = checker;
        active_trace = trace;
    }
    ~Services() {
        dispatch_probe = previous_probe;
        routine_checker = previous_checker;
        active_trace = previous_trace;
    }
};

std::unique_ptr<State> fixture_for(std::uint32_t entry) {
    auto s = std::make_unique<State>();
    s->registers.fill(0xa5);
    s->unique.fill(0x37);
    for (unsigned bit = 1280; bit < 1312; ++bit) s->wr<1>(bit, 0);
    s->memory.map(fixture, 0x2000);
    s->memory.map(stack - 0x800, 0x900);
    s->memory.map(toc - 0x8000, 0x1000);
    s->memory.map(toc - 0x5000, 0x1000);
    s->memory.map(toc - 0x5b20, 4);
    Cpu c(*s);
    const bool has_object = entry != count_squad && entry != sample;
    const bool file_operation = entry == load || entry == save;
    c.set({{1, stack}, {2, toc}, {3, has_object ? object : 0u}, {4, file_operation ? file : 0u}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x4dd0, constants);
    c.put_f64(constants, 65535.0);
    c.put_f64(constants + 8, 4503601774854144.0);
    c.put32(toc - 0x7b84, cursor);
    c.put32(toc - 0x7b88, end);
    c.put32(toc - 0x7b8c, random_words);
    c.put32(toc - 0x7b90, salt);
    c.put32(toc - 0x4dcc, pool);
    c.put32(toc - 0x5b20, 0x10203040);
    c.put32(toc - 0x7ba8, diagnostic);
    c.put32(toc - 0x7bac, save_ok);
    c.put32(toc - 0x7be8, clubs_slot);
    c.put32(toc - 0x7ba0, role_service);
    c.put32(toc - 0x7a64, filter_service);
    c.put32(toc - 0x7b80, date);
    c.put32(toc - 0x7b50, club_count);
    c.put32(toc - 0x7b5c, competitions_slot);
    c.put32(toc - 0x745c, media_flag);
    c.put32(toc - 0x4dc8, media_first);
    c.put32(toc - 0x4dc4, media_second);
    c.put8(media_flag, 1);
    c.put32(competitions_slot, competitions);
    c.put32(clubs_slot, club);
    c.put32(role_service, role_service);
    c.put32(filter_service, filter_service);
    s->pc = entry;
    s->budget = 10000;
    return s;
}

struct Outcome {
    unsigned instructions;
    std::uint32_t returned;
    std::string stop;
    std::uint32_t bytes_transferred, file_success, message;
    std::vector<std::uint8_t> bytes;
};

Outcome compare(const State& seed, bool negative_value = false, bool negative_trace = false) {
    auto original = clone(seed), modern = clone(seed);
    Effects expected, actual;
    expected.state = original.get();
    actual.state = modern.get();
    original->memory.date_observer = &expected;
    modern->memory.date_observer = &actual;
    Trace old_calls, new_calls;
    new_calls.change_argument = negative_trace;

    const auto execute = [](State& s, Trace& trace, bool reference) {
        RoutineChecker no_hooks([](std::uint32_t) { return false; }, 0, 0);
        Services services(&trace, reference ? &no_hooks : nullptr);
        // Let the fixture observe the allocator call on the generated side
        // too, where the real allocator otherwise enters directly in-page.
        s.allocator_original = true;
        // The generated caller also has a direct CFM-descriptor hook. Leave
        // this external boundary to the scripted service on both routes.
        s.import_descriptor_original_body = 0x1077fee0;
        localization_format_diagnostics(s).original_body = 0x10727890;
        try {
            if (reference) simulated_stats_run_original(s, s.pc);
            else run_until(s, return_sentinel);
            return std::string{};
        } catch (const std::runtime_error& error) {
            return std::string(error.what());
        }
    };
    const auto old_stop = execute(*original, old_calls, true);
    const auto new_stop = execute(*modern, new_calls, false);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto context = "Simulated stats " + hex_address(seed.pc) + " original PC=" + hex_address(original->pc)
        + " C++ PC=" + hex_address(modern->pc) + " limit=" + std::to_string(seed.budget);
    require(old_stop == new_stop, context + " original=" + old_stop + " C++=" + new_stop);
    require(old_stop.empty() || old_stop == "Native instruction budget exhausted"
                || old_stop.starts_with("Unmapped data access at ") || old_stop == "Stats failed allocation exit",
            context + " unexpected stop " + old_stop);
    require(expected.bytes == actual.bytes, context + " changed ordered memory accesses");
    require(original->executed == modern->executed, context + " changed instruction accounting");

    if (negative_trace) {
        const auto target = seed.pc == initialize ? 0x102b7c00u : check_exclusion;
        const bool reached = std::ranges::any_of(old_calls.calls, [target](const Call& call) { return call.target == target; });
        require(reached && old_calls.calls != new_calls.calls, context + " missed the changed exclusion argument");
        for (auto& call : new_calls.calls)
            if (call.target == target) call.arguments[1] ^= 4;
    }
    require(old_calls.calls == new_calls.calls, context + " changed dependency arguments or order");

    const Image image(*original);
    image.compare(*modern, context);
    Cpu c(*modern);
    const auto returned = c.r(3);
    if (negative_value) {
        c.set(3, c.r(3) ^ 1);
        bool caught = false;
        try { image.compare(*modern, context); }
        catch (const std::runtime_error& error) {
            caught = std::string_view(error.what()).starts_with(context + " register byte=");
        }
        require(caught, context + " missed a changed returned count");
    }
    const auto size = c.u32(position);
    std::vector<std::uint8_t> bytes;
    if (seed.pc == save) bytes = modern->memory.copy_out(persisted, size);
    if (seed.pc == load && new_stop.empty() && returned == 1) {
        bytes = modern->memory.copy_out(object, 4);
        if (size > 4) {
            const auto payload = modern->memory.copy_out(c.u32(object + 4), size - 4);
            bytes.insert(bytes.end(), payload.begin(), payload.end());
        }
    }
    return {unsigned(modern->executed), returned, new_stop, size, c.u8(save_ok), c.u32(diagnostic), std::move(bytes)};
}

unsigned check_budgets(State& seed, unsigned instructions, unsigned stride) {
    unsigned budgets = 0;
    for (unsigned limit = 0; limit < instructions; limit += stride) {
        seed.budget = limit;
        const auto stop = compare(seed).stop;
        if (stop == "Native instruction budget exhausted") ++budgets;
        else require(stop.empty(), "Unexpected failure under a guest budget");
    }
    seed.budget = 10000;
    return budgets;
}

void check_hooks() {
    {
        RoutineChecker excluded([](std::uint32_t) { return false; }, 0, 0);
        Services services(nullptr, &excluded);
        auto s = fixture_for(destroy);
        require(!try_modern_simulated_stats(*s, destroy), "Excluded simulated-stats hook must decline");
    }
    for (auto entry : {sample + 4, save + 4}) {
        auto s = fixture_for(entry);
        bool rejected = false;
        try { simulated_stats_run_rewrite(*s, entry); }
        catch (const std::logic_error& error) {
            rejected = std::string_view(error.what()).starts_with("Unknown SIMULATED_STATS.CPP entry");
        }
        require(rejected, "Simulated stats adapter must reject unregistered addresses");
    }

    RoutineChecker checker([](std::uint32_t entry) { return entry == initialize || entry == destroy || entry == count_squad || entry == load || entry == save || entry == sample; }, 1, 1);
    checker.strict_volatile = true;
    Services services(nullptr, &checker);
    for (auto entry : {initialize, destroy, count_squad, load, save, sample}) {
        const auto previous = checker.totals().find(entry);
        const auto checked = previous == checker.totals().end() ? 0 : previous->second.checked;
        auto s = fixture_for(entry);
        // Enter file hooks on the failed-open path, which needs no allocator.
        Cpu(*s).put8(mode, 1);
        if (entry == initialize) Cpu(*s).set(4, file);
        run_until(*s, return_sentinel);
        const auto result = checker.totals().find(entry);
        require(result != checker.totals().end() && result->second.checked == checked + 1 && !result->second.differences,
                "Generated page must select the simulated-stats owner");
    }
}

void check_sampler(unsigned& returned, unsigned& budgets) {
    auto s = fixture_for(sample);
    Cpu c(*s);
    for (const auto bound : {0, 1, -1, 23, -23, 65535, -65535, 65536, -65536,
                             450000, -450000, 429496730, -429496730, INT32_MIN, INT32_MAX}) {
        c.set(3, std::uint32_t(bound));
        for (const auto bias : {0u, 123u, 0x7fff1234u, 0xfffff123u}) {
            c.put32(salt, bias);
            for (const auto position : {random_words, random_words + 2, random_words + 4}) {
                c.put32(cursor, position);
                c.put32(end, random_words + 4);
                c.put16(random_words, 23);
                c.put16(random_words + 2, 0xffff);
                c.put16(random_words + 4, 0x8000);
                const auto out = compare(*s);
                require(out.stop.empty(), "Simulated stats sampler must return");
                if (bound == 0 || bound == 1 || bound == -1)
                    require(out.returned == 0, "Simulated stats narrow range result");
                ++returned;
            }
        }
    }
    c.set(3, 65536);
    c.put32(cursor, random_words + 2);
    // This biased draw gives a negative reciprocal intermediate whose low
    // 15 bits are zero, exercising the no-carry arm of the signed shift.
    c.put32(salt, 0xfffd8003);
    require(compare(*s).stop.empty(), "Simulated stats exact negative shift");
    ++returned;
    budgets += check_budgets(*s, compare(*s).instructions, 1);
}

void check_persistence(unsigned& returned, unsigned& budgets) {
    for (auto entry : {load, save}) {
        auto s = fixture_for(entry);
        Cpu c(*s);
        for (unsigned count : {0u, 1u, 3u}) {
            c.put32(object, count);
            c.put32(object + 4, rows);
            c.put32(persisted, count);
            for (unsigned i = 0; i < count * record_size; ++i) {
                c.put8(rows + i, entry == save ? i * 7 + 3 : 0xa5);
                c.put8(persisted + 4 + i, i * 7 + 3);
            }
            const auto encoded = s->memory.copy_out(persisted, 4 + count * record_size);
            for (unsigned failure : {0u, 1u, 2u, 3u, 4u, 6u}) {
                if (failure == 6 && (entry == save || count != 0)) continue;
                c.put8(mode, failure);
                c.put8(save_ok, 1);
                const auto out = compare(*s);
                require(out.stop.empty(), "Simulated stats persistence must return");
                const bool success = failure == 0 || failure == 6 || (failure == 3 && count == 0);
                require(out.file_success == unsigned(success), "Simulated stats shared save flag");
                require(out.returned == (entry == load ? unsigned(success) : success ? 0x42u : 0x43u),
                        "Simulated stats original file result");
                if (success) {
                    require(out.bytes_transferred == encoded.size(), "Simulated stats file width");
                    require(out.bytes == encoded, "Simulated stats file bytes");
                } else {
                    const auto first_line = entry == load ? 0x1ddu : 0x209u;
                    const auto line = failure == 1 ? first_line : failure == 2 ? first_line + 7
                        : entry == load ? 0x1efu : 0x218u;
                    require(out.message == line, "Simulated stats failure diagnostic");
                }
                ++returned;
            }
        }
        c.put8(mode, 0);
        budgets += check_budgets(*s, compare(*s).instructions, 1);
        c.set(4, 0);
        c.put8(save_ok, 1);
        const auto missing = compare(*s);
        require(missing.stop.empty() && missing.bytes_transferred == 0
                    && missing.returned == (entry == load ? 0u : 0x43u)
                    && missing.file_success == unsigned(entry == load), "Missing file input policy");
        ++returned;
    }
}

std::unique_ptr<State> initial_fixture(unsigned count) {
    auto s = fixture_for(initialize);
    s->memory.map(initial_clubs, 0x40000);
    s->memory.map(initial_list, 0x2000);
    Cpu c(*s);
    c.put32(clubs_slot, initial_clubs);
    c.put32(club_count, count);
    c.put32(cursor, random_words);
    c.put32(end, random_words + 14);
    for (unsigned i = 0; i < 8; ++i) c.put16(random_words + i * 2, i * 1987 + 23);
    c.put8(save_ok, 1);
    c.put32(competition, 1);
    c.put32(competition + 4, 2);
    c.put32(competitions + 4, competition);
    c.put32(competitions + 8, competition + 4);
    c.put32(media_first, 0x02616200);
    c.put32(media_second, 0x02414200);
    for (unsigned i = 0; i < count; ++i) {
        const auto club = initial_clubs + i * 0x248;
        c.put32(club, i);
        c.put16(club + 0x1ec, std::uint16_t(i % 5 == 0 ? -100 : (i * 137) % 200));
    }
    s->budget = 2000000;
    return s;
}

void check_initialization(unsigned& returned, unsigned& budgets) {
    for (unsigned count : {0u, 1u, 15u, 16u, 17u, 31u, 32u, 240u, 241u, 256u, 320u}) {
        auto s = initial_fixture(count);
        const auto out = compare(*s);
        require(out.stop.empty() && out.returned == object, "Stats constructor returns the object");
        ++returned;
    }
    for (bool upper : {false, true}) {
        auto s = initial_fixture(320);
        Cpu c(*s);
        const std::array<unsigned, 6> words = upper ? std::array<unsigned, 6>{0, 319, 1, 0, 0, 1}
            : std::array<unsigned, 6>{0, 0, 0, 1, 1, 0};
        for (unsigned i = 0; i < words.size(); ++i) c.put16(random_words + i * 2, words[i]);
        require(compare(*s).stop.empty(), "Stats constructor retries an out-of-range shuffle destination");
        ++returned;
    }
    std::uint32_t mutation = 0x61ca22b8;
    const auto next = [&mutation] {
        mutation ^= mutation << 13;
        mutation ^= mutation >> 17;
        mutation ^= mutation << 5;
        return mutation;
    };
    for (unsigned scenario = 0; scenario < 8; ++scenario) {
        const auto count = std::array{16u, 240u, 256u, 320u}[next() % 4];
        auto s = initial_fixture(count);
        Cpu c(*s);
        c.put32(salt, next() & 0xffff);
        for (unsigned i = 0; i < count; ++i) {
            const auto club = initial_clubs + i * 0x248;
            c.put16(club + 0x1ec, next());
            c.put32(club + 0x30, next() % 5 == 0 ? competition : 0);
            c.put32(club + 0x38, next() % 5 == 0 ? competition + 4 : 0);
            c.put32(club + 0x10, next() % 5 == 0 ? 0x180 : 0x100);
        }
        const auto out = compare(*s);
        require(out.stop.empty(), "Seeded stats construction " + std::to_string(scenario) + " " + out.stop);
        ++returned;
    }
    {
        auto s = initial_fixture(16);
        budgets += check_budgets(*s, compare(*s).instructions, 53);
    }
    {
        auto s = initial_fixture(16);
        Cpu(*s).put8(mode, 7);
        require(compare(*s).stop.starts_with("Unmapped data access at "),
                "Stats constructor preserves the unguarded row allocation failure");
    }
    auto s = initial_fixture(32);
    Cpu c(*s);
    for (unsigned variant = 0; variant < 5; ++variant) {
        for (unsigned i = 0; i < 32; ++i) {
            const auto club = initial_clubs + i * 0x248;
            c.put32(club + 0x30, variant == 1 || (variant == 4 && i % 4 == 1) ? competition : 0);
            c.put32(club + 0x38, variant == 2 || (variant == 4 && i % 4 == 2) ? competition + 4 : 0);
            c.put32(club + 0x10, variant == 3 || (variant == 4 && i % 4 == 3) ? 0x180 : 0x100);
            c.put32(club + 0x14, variant == 4 && i == 0 ? competitions + 4 : 0);
        }
        const auto out = compare(*s, variant == 4, variant == 4);
        require(out.stop.empty(), "Stats constructor competition and callback filters");
        ++returned;
    }
    s = initial_fixture(0);
    Cpu media(*s);
    media.put8(media_flag, 0);
    for (unsigned retries : {0u, 1u, 2u}) {
        media.put8(mode, retries);
        require(compare(*s).stop.empty(), "Stats constructor media retry");
        ++returned;
    }
    budgets += check_budgets(*s, compare(*s).instructions, 7);
    media.put8(media_flag, 1);
    media.put8(mode, 6);
    require(compare(*s).stop == "Stats failed allocation exit", "Stats constructor expected allocation exit");
    media.put8(mode, 0);
    media.set(4, file);
    media.put32(persisted, 1);
    for (unsigned i = 0; i < record_size; ++i) media.put8(persisted + 4 + i, i * 11);
    const auto loaded = compare(*s);
    require(loaded.stop.empty(), "Stats constructor calls its owned loader: " + loaded.stop);
    ++returned;
    media.put8(mode, 8);
    require(compare(*s).stop.empty(), "Stats constructor resumes after an owned nonlocal return");
    ++returned;
    media.put8(mode, 1);
    media.put32(object, 1);
    media.put32(object + 4, rows);
    require(compare(*s).stop.empty(), "Stats constructor preserves a failed load and returns its object");
    ++returned;
}

unsigned check_inline_draws() {
    using simulated_stats_detail::RangeDraw;
    constexpr std::array draws{
        RangeDraw{0x103702e4, 19, 0x394, 0x38c, 0x384},
        RangeDraw{0x10370448, 18, 0x384, 0x38c, 0x394},
        RangeDraw{0x103705a0, 17, 0x384, 0x38c, 0x394}
    };
    unsigned checked = 0;
    for (auto draw : draws) {
        for (auto bound : {1, -1, 23, -23, 65535, -65535, 65536, -65536, 450000, -450000,
                            429496730, -429496730, -429503283, INT32_MIN, INT32_MAX}) {
            for (auto bias : {0u, 123u, 0x7fff1234u, 0xfffff123u, 0xfffd8003u}) {
                auto seed = initial_fixture(0);
                Cpu c(*seed);
                const auto bound_register = draw.result == 19 ? 16u : 21u;
                c.set(bound_register, std::uint32_t(bound));
                c.set({{22, constants}, {23, salt}, {24, random_words}, {25, end}, {26, cursor}});
                c.put32(salt, bias);
                c.set(1, stack - 0x3f0);
                seed->pc = draw.site;
                auto modern = clone(*seed);
                Effects actual;
                actual.state = modern.get();
                modern->memory.date_observer = &actual;
                Trace new_calls;
                {
                    Services services(&new_calls, nullptr);
                    Cpu cpu(*modern);
                    simulated_stats_detail::draw_range(cpu, {constants, salt, random_words, end, cursor}, draw, bound);
                }
                modern->memory.date_observer = nullptr;
                const auto resume = draw.site + 0x120;
                modern->pc = resume;
                // Stop the generated routine at the next existing block. This
                // compares the entire inline draw, including its real nested
                // sampler, without executing the surrounding constructor.
                auto original = clone(*seed);
                original->budget = modern->executed;
                Effects expected;
                expected.state = original.get();
                original->memory.date_observer = &expected;
                Trace old_calls;
                bool exhausted = false;
                {
                    RoutineChecker no_hooks([](std::uint32_t) { return false; }, 0, 0);
                    Services services(&old_calls, &no_hooks);
                    try { simulated_stats_run_original(*original, draw.site); }
                    catch (const std::runtime_error& error) {
                        exhausted = std::string_view(error.what()) == "Native instruction budget exhausted";
                        require(exhausted, "Unexpected inline original stop " + std::string(error.what()));
                    }
                }
                original->memory.date_observer = nullptr;
                const auto label = "Inline stats draw " + hex_address(draw.site) + " bound=" + std::to_string(bound)
                    + " bias=" + std::to_string(bias);
                require(exhausted && original->pc == resume && original->executed == modern->executed + 1,
                        label + " incomplete original boundary " + hex_address(original->pc));
                require(expected.bytes == actual.bytes && old_calls.calls == new_calls.calls,
                        label + " changed memory or RNG effects");
                Image(*original).compare(*modern, label);
                ++checked;
            }
        }
    }
    return checked;
}

void verify() {
    unsigned returned = 0, budgets = 0;
    {
        auto s = fixture_for(destroy);
        Cpu c(*s);
        for (const auto address : {0u, object})
            for (const auto row_pointer : {0u, rows})
                for (const auto flag : {0u, 1u, 0x7fffu, 0x8000u, 0xffffu, 0x10000u, 0xffff0001u}) {
                    c.set({{3, address}, {4, flag}});
                    c.put32(object + 4, row_pointer);
                    const auto out = compare(*s);
                    require(out.stop.empty() && out.returned == address, "Destructor returns its original object");
                    ++returned;
                }
        c.set({{3, object}, {4, 1}});
        c.put32(object + 4, rows);
        budgets += check_budgets(*s, compare(*s).instructions, 1);
        c.set(3, fixture + 0x2100);
        require(compare(*s).stop.starts_with("Unmapped data access at "), "Expected a partial object fault");
    }

    auto s = fixture_for(count_squad);
    Cpu c(*s);
    for (unsigned scenario = 0; scenario < 20; ++scenario) {
        unsigned expected = 0;
        const auto club_index = scenario & 1 ? UINT32_MAX : 2u;
        c.set(3, club_index);
        c.put32(clubs_slot, club - club_index * 0x248u);
        for (unsigned slot = 0; slot < 50; ++slot) {
            const auto member = staff + slot * member_stride;
            const auto variant = scenario < 5 ? scenario : (slot + scenario) % 5;
            c.put32(squad + 4 * slot, variant == 0 ? 0 : member);
            c.put32(member + 0x14, variant == 1 ? 0 : live_profile);
            c.put32(member + 0x1c, variant == 2 ? 0x102u : 0x180u);
            c.put32(member + 0x20, variant == 3 ? UINT32_MAX : 0);
            expected += variant == 4;
        }
        const auto out = compare(*s);
        require(out.stop.empty() && out.returned == expected, "Squad eligible-player count");
        ++returned;
    }
    const auto initial_count = compare(*s).returned;
    c.put32(staff + 0x24, squad + 4 * 5);
    const auto mutation = compare(*s, true, true);
    require(mutation.returned + 1 == initial_count, "Callback clears a later eligible slot");
    ++returned;
    budgets += check_budgets(*s, mutation.instructions, 7);
    c.put32(squad + 4 * 49, fixture + 0x2100);
    require(compare(*s).stop.starts_with("Unmapped data access at "), "Expected a partial squad fault");

    check_persistence(returned, budgets);
    check_sampler(returned, budgets);
    check_initialization(returned, budgets);
    const auto inline_draws = check_inline_draws();
    check_hooks();
    std::cout << "Simulated stats " << returned << " full comparisons, " << budgets
              << " budget limits, " << inline_draws
              << " complete inline draws, three partial faults, one checked allocation exit, four detected mutations"
              << " and six checked production hooks\n";
}
}

int main() {
    try {
#ifdef CM_SIMULATED_STATS_PROFILE_DIR
        routine_child_exit = [] {
            const auto path = std::string(CM_SIMULATED_STATS_PROFILE_DIR) + "/check-"
                + std::to_string(getpid()) + ".profraw";
            __llvm_profile_set_filename(path.c_str());
            if (__llvm_profile_write_file() != 0) throw std::runtime_error("Cannot flush simulated-stats coverage");
        };
#endif
        verify();
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

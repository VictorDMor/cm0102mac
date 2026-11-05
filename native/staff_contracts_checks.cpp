#include "staff_contracts.hpp"
#include "contract_review_adapter.hpp"
#include "localization_format_adapter.hpp"
#include "staff_contracts_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <optional>
#include <random>
#include <string_view>

#ifdef CM_STAFF_CONTRACTS_PROFILE_DIR
#include <unistd.h>
extern "C" int __llvm_profile_write_file();
extern "C" void __llvm_profile_set_filename(const char*);
#endif

namespace {
using namespace cm::aot;
using cm::semantic_io_checks::Effects;
using cm::semantic_io_checks::Image;
using cm::semantic_io_checks::clone;
using cm::semantic_io_checks::fixture;
using cm::semantic_io_checks::require;
using cm::semantic_io_checks::stack;
using cm::semantic_io_checks::toc;
using staff_contracts::expiry_entry;

constexpr std::array date_slots{-0x7b80, -0x79b4, -0x79a8, -0x79ac, -0x79b0, -0x79a0, -0x79a4, -0x799c};
constexpr std::array date_days{1, 180, 120, 140, 160, 80, 100, 60};
constexpr auto record = fixture + 0x400;
constexpr auto dates = fixture + 0x800;
constexpr auto dates_year = 2001;
constexpr auto normal_budget = 10000;
constexpr auto no_hole = UINT32_MAX;
constexpr std::uint32_t draft_entry = 0x103721f0, delta_entry = 0x10374d10;
constexpr std::uint32_t default_date = 0x10715880, difference = 0x10718130;

constexpr std::uint32_t primary_adjustment = 0x1000a060, alternate_adjustment = 0x1000a170;
constexpr std::uint32_t primary_adjustment_entry = 0x10380560, alternate_adjustment_entry = 0x10380630;
constexpr std::uint32_t preferences_slot = fixture + 0x620, preferences = fixture + 0x640;
constexpr std::uint32_t bonus_name = 0x10380270, option_name = 0x10380300;
constexpr std::uint32_t bonus_description = 0x10374be0, option_description = 0x10374cb0;
constexpr std::uint32_t formatter = 0x10727890, string_copy = 0x1077dbf0;
constexpr std::uint32_t money_value = 0x10009f20, money_text = 0x1000b410;
constexpr std::uint32_t label_pool = fixture + 0x1000, output_buffer = fixture + 0x1800;

constexpr std::uint32_t reset_review_entry = 0x1037f050, clear_review_entry = 0x1037f7d0;
constexpr std::uint32_t clear_all_review_entry = 0x1037f830, refresh_review_entry = 0x1037f590;
constexpr std::uint32_t reset_status_entry = 0x1037fa60;
constexpr std::uint32_t clear_staff_conditions = 0x100ddf30, clear_staff_condition = 0x100de090;
constexpr std::uint32_t review_reputation = 0x103092f0;
constexpr std::uint32_t review_club = fixture + 0x500, review_league = fixture + 0x200;
constexpr std::uint32_t review_service_slot = fixture + 0x590;

constexpr std::uint32_t squad_entry = 0x10380c10, summary_entry = 0x10380d50;
constexpr std::uint32_t diagnostic_parameters = 0x10739890, diagnostic_alert = 0x10769b60;
constexpr std::uint32_t external_continuation = 0x18000e00;
constexpr std::uint32_t squad_table = label_pool + 0x700, diagnostic_text = label_pool + 0x760;
constexpr std::uint32_t diagnostic_flag = label_pool + 0x790;

constexpr std::uint32_t wage_entry = 0x10372590, end_text_entry = 0x10372730;
constexpr std::uint32_t clause_entry = 0x10380400, cfm_entry = 0x10781320;
constexpr std::uint32_t plain_format = 0x1077b720, date_format = 0x10715e00;
constexpr std::uint32_t fixture_club = fixture + 0xb00, competition_index = fixture + 0xb80;
constexpr std::uint32_t competition_slot = fixture + 0xb90, competition_table = fixture + 0xba0;
constexpr std::uint32_t competition = fixture + 0xc00, competition_vtable = fixture + 0xd00;
constexpr std::uint32_t first_descriptor = fixture + 0xe00, second_descriptor = fixture + 0xe08;

constexpr std::uint32_t window_entry = 0x10380f20, interval_entry = 0x10717dd0, age_entry = 0x100c6a10;
constexpr std::uint32_t window_flag = fixture + 0x700, window_reference = fixture + 0x710;
constexpr std::uint32_t staff_slot = fixture + 0x720, staff_rows = fixture + 0x730;
constexpr std::uint32_t boundary_out = fixture + 0x950, threshold_out = fixture + 0x954;

constexpr std::uint32_t sampling_entry = 0x10374570, reseed_entry = 0x1077b770;
constexpr std::uint32_t random_words = fixture + 0xa00, rng_cursor = fixture + 0xa20;
constexpr std::uint32_t rng_end = fixture + 0xa24, rng_salt = fixture + 0xa28;
constexpr std::uint32_t sampling_constants = fixture + 0x2000;

struct WindowInputs {
    std::uint32_t age = 28;
    std::uint16_t years = 0;
    std::int16_t days = 0;
};

struct ExternalInputs {
    WindowInputs window;
    std::uint32_t clause_reply = 1;
    std::optional<std::uint32_t> wage_preferences;
    bool transfer_clause = false;
    std::optional<std::uint32_t> change_release_field;
    std::uint32_t review_reply = 8;
};

bool review_state_entry(std::uint32_t entry) {
    return entry == reset_review_entry || entry == clear_review_entry || entry == clear_all_review_entry;
}

bool release_entry(std::uint32_t entry) {
    return entry == squad_entry || entry == summary_entry;
}

bool label_entry(std::uint32_t entry) {
    return entry == bonus_name || entry == option_name || entry == bonus_description || entry == option_description
        || entry == wage_entry || entry == end_text_entry || release_entry(entry);
}

void write_text(guest::Cpu& c, std::uint32_t destination, std::string_view value) {
    for (const char letter : value) c.put8(destination++, std::uint8_t(letter));
    c.put8(destination, 0);
}

std::string read_text(guest::Cpu& c, std::uint32_t address) {
    std::string result;
    for (unsigned n = 0; n < 100; ++n) {
        const char letter = char(c.u8(address + n));
        if (!letter) return result;
        result += letter;
    }
    throw std::runtime_error("Unterminated label fixture");
}

bool value_adjustment(std::uint32_t entry) {
    return entry == primary_adjustment_entry || entry == alternate_adjustment_entry;
}

struct DependencyCall {
    std::uint32_t entry, return_address, frame, toc, saved_draft, descriptor;
    std::array<std::uint32_t, 8> arguments;
    std::optional<std::uint32_t> adjustment_input;
    bool operator==(const DependencyCall&) const = default;
};

struct DependencyTrace {
    std::vector<DependencyCall> calls;
    std::optional<std::uint32_t> adjustment_output;
    bool corrupt = false, text_fixture = false;
    unsigned dispatched_names = 0, dispatched_review_children = 0;
    std::optional<WindowInputs> window;
    bool sample_fixture = false;
    std::optional<std::uint32_t> clause_reply, wage_preferences;
    bool transfer_clause = false;
    std::optional<std::uint32_t> change_release_field;
    std::optional<std::uint32_t> review_reply;
};
DependencyTrace* dependency_trace = nullptr;

bool is_fixture_service(std::uint32_t entry) {
    return entry == primary_adjustment || entry == alternate_adjustment || entry == money_value
        || entry == money_text || entry == review_reputation;
}

struct ScopedRoutineChecker {
    RoutineChecker* saved = routine_checker;
    explicit ScopedRoutineChecker(RoutineChecker* checker) {
        if (checker) routine_checker = checker;
    }
    ~ScopedRoutineChecker() { routine_checker = saved; }
};

bool trace_dependency(State& s) {
    const bool adjustment = s.pc == primary_adjustment || s.pc == alternate_adjustment;
    auto& trace = *dependency_trace;
    if (trace.text_fixture && (s.pc == bonus_name || s.pc == option_name)) ++trace.dispatched_names;
    const bool label = trace.text_fixture && (s.pc == formatter || s.pc == string_copy
                                               || s.pc == money_value || s.pc == money_text
                                               || s.pc == plain_format || s.pc == date_format
                                               || s.pc == diagnostic_parameters || s.pc == diagnostic_alert
                                               || s.pc == external_continuation);
    const bool window = trace.window && (s.pc == interval_entry || s.pc == age_entry);
    const bool sample = trace.sample_fixture && s.pc == reseed_entry;
    const bool clause = trace.clause_reply && s.pc == cfm_entry;
    if (trace.review_reply && (s.pc == clear_all_review_entry || s.pc == reset_status_entry))
        ++trace.dispatched_review_children;
    const bool review = trace.review_reply && (s.pc == refresh_review_entry || s.pc == review_reputation
        || s.pc == clear_staff_condition || s.pc == clear_staff_conditions);
    if (!adjustment && !label && !window && !sample && !clause && !review && s.pc != default_date && s.pc != difference)
        return false;
    guest::Cpu c(s);
    if (trace.corrupt && s.pc == difference) c.set(4, c.r(4) ^ 0x10000u);
    if (trace.corrupt && s.pc == formatter) c.set(4, c.r(4) + 1);
    if (trace.corrupt && review) c.set(4, c.r(4) ^ 0x10000u);
    if (trace.corrupt && clause) c.set(12, c.r(12) == first_descriptor ? second_descriptor : first_descriptor);
    DependencyCall call{s.pc, c.lr(), c.sp(), c.r(2), c.r(31), c.r(12), {}, std::nullopt};
    for (unsigned n = 0; n < call.arguments.size(); ++n) call.arguments[n] = c.r(n + 3);
    if (adjustment || s.pc == money_value) call.adjustment_input = c.u32(c.r(3));
    trace.calls.push_back(call);
    if (review) {
        if (s.pc == review_reputation) c.set(3, *trace.review_reply);
        else if (s.pc == refresh_review_entry) c.put8(record + 0x45, 9);
        else c.put32(c.r(3) + 0x34, s.pc == clear_staff_condition ? c.r(4) : 0);
        c.finish();
        return true;
    }
    if (clause) {
        require(c.r(3) == competition && (c.r(12) == first_descriptor || c.r(12) == second_descriptor),
                "Valid competition callback descriptor and receiver");
        c.put32(c.sp() + 0x14, c.r(2));
        c.set({{2, 0x12345678}, {3, *trace.clause_reply}});
        if (trace.transfer_clause) {
            s.pc = 0x18000e00;
            throw GuestContextTransfer{&s, nullptr};
        }
        c.finish();
        return true;
    }
    if (sample) {
        c.set(3, 0xaabb1233u + std::uint32_t(trace.calls.size()));
        c.finish();
        return true;
    }
    if (window) {
        if (s.pc == age_entry) {
            c.set(3, trace.window->age);
        } else {
            const bool first = c.lr() == 0x10380fec;
            c.put16(c.r(3), first ? 41 : std::uint16_t(trace.window->days));
            c.put16(c.r(3) + 2, first ? 7 : trace.window->years);
            c.put8(c.r(3) + 4, first ? 1 : 0);
        }
        c.finish();
        return true;
    }
    if (label) {
        switch (s.pc) {
        case formatter:
            if (c.r(4) == label_pool + 0xfb) {
                const auto amount = read_text(c, c.r(5));
                const auto name = read_text(c, c.r(6));
                write_text(c, c.r(3), amount + " " + name);
            } else {
                write_text(c, c.r(3), read_text(c, c.r(4)));
            }
            if (trace.change_release_field) c.put8(record + *trace.change_release_field, 7);
            break;
        case string_copy:
            write_text(c, c.r(3), read_text(c, c.r(4)));
            break;
        case plain_format:
            if (c.r(4) == label_pool + 0x14) write_text(c, c.r(3), "diagnostic");
            else if (c.r(4) == label_pool + 0x5c3) write_text(c, c.r(3), "");
            else write_text(c, c.r(3), read_text(c, c.r(5)));
            break;
        case diagnostic_parameters:
            c.put32(c.r(6), 0x11223344);
            c.put32(c.r(7), 0x55667788);
            break;
        case diagnostic_alert:
            require(read_text(c, c.r(4)) == "diagnostic", "Missing short-buffer diagnostic message");
            break;
        case external_continuation: {
            const auto caller_stack = c.u32(c.sp());
            c.set(1, caller_stack);
            c.set(31, c.u32(caller_stack - 4));
            c.set(30, c.u32(caller_stack - 8));
            c.set(3, 19);
            break;
        }
        case date_format: {
            const auto day = c.i16(c.r(3));
            const auto year = c.u16(c.r(3) + 2);
            write_text(c, c.r(4), std::to_string(year) + "/" + std::to_string(day));
            break;
        }
        case money_value:
            c.set({{3, *call.adjustment_input ^ 0x11220000u}, {4, 0x33445566}});
            break;
        case money_text:
            write_text(c, c.r(3), "amount");
            if (trace.wage_preferences) c.put32(preferences + 4, *trace.wage_preferences);
            break;
        }
        c.finish();
        return true;
    }
    if (!adjustment || !trace.adjustment_output) return false;
    // The fixture models the separate value service's in/out amount. It
    // deliberately clobbers its volatile result and flag state as well.
    c.put32(c.r(3), *trace.adjustment_output);
    c.set(3, 0x76543210);
    c.set_cr(0, 5);
    c.s.wr<1>(guest::reg::xer_ca, 1);
    c.finish();
    return true;
}

struct ScopedDependencyProbe {
    decltype(dispatch_probe) saved = dispatch_probe;
    DependencyTrace* previous = dependency_trace;
    explicit ScopedDependencyProbe(DependencyTrace* trace) {
        if (trace) {
            dispatch_probe = trace_dependency;
            dependency_trace = trace;
        }
    }
    ~ScopedDependencyProbe() { dispatch_probe = saved; dependency_trace = previous; }
};

std::unique_ptr<State> expiry_fixture() {
    auto s = std::make_unique<State>();
    s->registers.fill(0xa5);
    s->unique.fill(0xb7);
    for (unsigned bit = 1280; bit < 1312; ++bit) s->wr<1>(bit, 0);
    s->memory.map(fixture, 0x1000);
    s->memory.map(stack - 0x500, 0x600);
    s->memory.map(label_pool, 0x1000);
    s->memory.map(toc - 0x4da4, 4);
    s->memory.map(toc - 0x4dac, 4);
    s->memory.map(toc - 0x4dc0, 4);
    s->memory.map(toc - 0x5b20, 4);
    s->memory.map(sampling_constants, 0x100);
    s->memory.map(toc - 0x8000, 0x1000);
    s->memory.map(0x108b7ad0, 4);  // Date difference's live scratch pointer.
    s->memory.map(0x108b84d0, 1);  // Date difference's initialization flag.
    guest::Cpu c(*s);
    c.set({{1, stack}, {2, toc}, {3, record}});
    c.set_lr(return_sentinel);
    s->pc = expiry_entry;
    s->budget = normal_budget;
    c.put32(0x108b7ad0, fixture + 0x300);
    c.put32(toc - 0x4da4, label_pool);
    c.put32(toc - 0x4dc0, squad_table);
    c.put32(toc - 0x5b20, label_pool + 0x21);
    c.put32(toc - 0x7ba8, diagnostic_text);
    c.put32(toc - 0x7bac, diagnostic_flag);
    c.put8(diagnostic_flag, 1);
    constexpr std::array squad_labels{0x419, 0x423, 0x427, 0x432, 0x43b, 0x442, 0x44b, 0x455};
    for (unsigned n = 0; n < squad_labels.size(); ++n) {
        c.put32(squad_table + n * 4, 0x10380cb4 + n * 16);
        write_text(c, label_pool + squad_labels[n], "S" + std::to_string(n));
    }
    for (const auto offset : {0x460, 0x4a4, 0x4eb, 0x52b, 0x582})
        write_text(c, label_pool + offset, "label" + std::to_string(offset));
    c.put32(toc - 0x4dac, sampling_constants);
    c.put_f64(sampling_constants, 65535.0);
    c.put64(sampling_constants + 0xc0, UINT64_C(0x4330000080000000));
    c.put32(toc - 0x7b5c, competition_slot);
    c.put32(competition_slot, competition_table);
    c.put32(competition_table + 4, competition);
    c.put32(competition_index, 1);
    c.put32(fixture_club + 0x30, competition_index);
    c.put32(competition, competition_vtable);
    c.put32(competition_vtable + 0xa0, first_descriptor);
    c.put32(competition_vtable + 0xa4, second_descriptor);
    c.put32(toc - 0x7b90, rng_salt);
    c.put32(toc - 0x7b8c, random_words);
    c.put32(toc - 0x7b88, rng_end);
    c.put32(toc - 0x7b84, rng_cursor);
    c.put32(rng_salt, 41);
    c.put32(rng_end, random_words + 8);
    c.put32(rng_cursor, random_words + 2);
    for (unsigned n = 0; n < 8; ++n) c.put16(random_words + n * 2, 12345 + n * 1009);
    c.put32(toc - 0x7a68, window_flag);
    c.put32(toc - 0x79e8, window_reference);
    c.put32(toc - 0x7be0, staff_slot);
    c.put32(staff_slot, staff_rows);
    c.put32(toc - 0x7b3c, review_service_slot);
    c.put32(review_service_slot, 0x1234b3c);
    c.put32(review_club + 0x2c, review_league);
    c.put8(review_league + 0x119, 12);
    for (const auto offset : {0x2a, 0x3f, 0x54, 0x69, 0x83, 0x9e, 0xb9, 0xca, 0xf3,
                              0x289, 0x294, 0x2a1, 0x2b3, 0x2c6, 0x2e3, 0x2fe, 0x319, 0x333, 0x34e})
        write_text(c, label_pool + offset, "label" + std::to_string(offset));
    c.put32(toc - 0x7ac8, preferences_slot);
    c.put32(preferences_slot, preferences);
    c.put8(0x108b84d0, 1);
    c.put8(record + 0x38, 8);
    c.put16(record + 0x30, 150);
    c.put16(record + 0x32, dates_year);
    for (unsigned n = 0; n < date_slots.size(); ++n) {
        const auto address = dates + n * 8;
        c.put32(toc + std::uint32_t(date_slots[n]), address);
        c.put16(address, date_days[n]);
        c.put16(address + 2, dates_year);
        c.put32(address + 4, 0xdead0000 + n);
    }
    return s;
}

std::unique_ptr<State> state_with_hole(const State& input, std::uint32_t hole) {
    if (hole == no_hole) return clone(input);
    auto s = std::make_unique<State>();
    s->registers = input.registers;
    s->unique = input.unique;
    s->pc = input.pc;
    s->budget = input.budget;
    for (const auto& [address, length] : input.memory.mapped_ranges()) {
        const auto copy = [&](std::uint32_t begin, std::uint64_t size) {
            if (!size) return;
            s->memory.map(begin, size);
            s->memory.copy_in(begin, input.memory.copy_out(begin, size));
        };
        const auto end = address + length;
        if (hole >= address && hole < end) {
            copy(address, hole - address);
            copy(hole + 1, end - (hole + 1));
        } else {
            copy(address, length);
        }
    }
    return s;
}

struct Result {
    unsigned value, steps;
    std::uint32_t contract_value;
    std::string text;
    std::vector<Effects::Access> accesses;
    std::uint8_t boundary = 0, threshold = 0;
    std::size_t dependency_calls = 0;
    std::uint32_t cursor = 0, salt = 0;
};
struct Counts {
    unsigned returned = 0, budgets = 0, faults = 0, negatives = 0, transfers = 0;
};

Result compare_entry(const State& input, Counts& counts, std::uint32_t hole = no_hole,
                      bool negative = false, bool call_negative = false,
                      std::optional<std::uint32_t> adjustment = std::nullopt, const ExternalInputs& inputs = {}) {
    auto original = state_with_hole(input, hole);
    auto modern = state_with_hole(input, hole);
    Effects expected, actual;
    expected.state = original.get();
    actual.state = modern.get();
    original->memory.date_observer = &expected;
    modern->memory.date_observer = &actual;
    const auto call = [](State& s, bool reference) {
        try {
            if (reference) staff_contracts_run_original(s, s.pc);
            else run_until(s, return_sentinel);
            return std::string{};
        } catch (const GuestContextTransfer& transfer) {
            require(transfer.state == &s && transfer.destination == nullptr, "Unexpected fixture transfer destination");
            return std::string("Competition callback transfer");
        } catch (const std::runtime_error& error) {
            return std::string(error.what());
        }
    };
    const bool text_fixture = label_entry(input.pc);
    const bool window_fixture = input.pc == window_entry;
    const bool sample_fixture = input.pc == sampling_entry;
    const bool clause_fixture = input.pc == clause_entry;
    const bool review_fixture = review_state_entry(input.pc);
    const bool traced = input.pc == draft_entry || input.pc == delta_entry || adjustment.has_value()
        || text_fixture || window_fixture || sample_fixture || clause_fixture || review_fixture;
    if (text_fixture) {
        // Decline the in-place formatting owner at the external boundary.
        // The dispatcher probe then supplies the same fixture on both sides.
        localization_format_diagnostics(*original).original_body = formatter;
        localization_format_diagnostics(*modern).original_body = formatter;
        original->formatted_output_original = modern->formatted_output_original = true;
    }
    if (review_fixture) {
        contract_review_diagnostics(*original).original_body = refresh_review_entry;
        contract_review_diagnostics(*modern).original_body = refresh_review_entry;
    }
    const auto review_reply = review_fixture ? std::optional(inputs.review_reply) : std::nullopt;
    const auto window_inputs = window_fixture ? std::optional(inputs.window) : std::nullopt;
    const auto clause_reply = clause_fixture ? std::optional(inputs.clause_reply) : std::nullopt;
    DependencyTrace old_calls{.calls = {}, .adjustment_output = adjustment, .text_fixture = text_fixture,
                              .window = window_inputs, .sample_fixture = sample_fixture,
                              .clause_reply = clause_reply, .wage_preferences = inputs.wage_preferences,
                              .transfer_clause = inputs.transfer_clause,
                              .change_release_field = inputs.change_release_field,
                              .review_reply = review_reply};
    DependencyTrace new_calls{.calls = {}, .adjustment_output = adjustment, .corrupt = call_negative,
                              .text_fixture = text_fixture, .window = window_inputs, .sample_fixture = sample_fixture,
                               .clause_reply = clause_reply, .wage_preferences = inputs.wage_preferences,
                              .transfer_clause = inputs.transfer_clause,
                              .change_release_field = inputs.change_release_field,
                              .review_reply = review_reply};
    ScopedDependencyProbe probe(traced ? &old_calls : nullptr);
    // Decline the in-place owners for the modeled external dependencies.
    RoutineChecker fixture_filter([](std::uint32_t entry) { return !is_fixture_service(entry); }, 0, 0);
    const bool amount_description = input.pc == bonus_description && std::int8_t(input.r<4>(28)) != 1;
    const bool reputation_call = input.pc == reset_review_entry && input.r<4>(20) != 0;
    ScopedRoutineChecker checker(adjustment || amount_description || input.pc == wage_entry || reputation_call
                                    ? &fixture_filter : nullptr);
    const auto old_stop = call(*original, true);
    if (traced) dependency_trace = &new_calls;
    const auto new_stop = call(*modern, false);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto test_case = counts.returned + counts.budgets + counts.faults + counts.negatives + counts.transfers;
    const auto label = "Staff contract " + hex_address(input.pc) + " comparison " + std::to_string(test_case);
    require(old_stop == new_stop, label + " original=" + old_stop + " C++=" + new_stop);
    if (call_negative) {
        require(old_stop.empty() && old_calls.calls.size() == 1, label + " missed the call mutation");
        auto changed = old_calls.calls;
        if (clause_fixture)
            changed.front().descriptor = changed.front().descriptor == first_descriptor ? second_descriptor : first_descriptor;
        else if (text_fixture) ++changed.front().arguments[1];
        else changed.front().arguments[1] ^= 0x10000u;
        require(new_calls.calls == changed, label + " did not isolate the changed external argument");
        ++counts.negatives;
        return {unsigned(modern->r<4>(12)), unsigned(modern->executed), 0, {}, std::move(actual.log)};
    }
    require(old_calls.calls == new_calls.calls, label + " changed external arguments or call order");
    if (input.pc == reset_review_entry && !reputation_call && old_stop.empty())
        require(old_calls.dispatched_review_children == 2 && new_calls.dispatched_review_children == 0,
                label + " did not take both direct C++ review calls");
    if (traced && !sample_fixture && !clause_fixture && !release_entry(input.pc)
        && !review_fixture && input.pc != wage_entry && old_stop.empty()) {
        const unsigned expected_calls = input.pc == bonus_description ? (amount_description ? 4 : 2)
            : input.pc == draft_entry ? 2 : 1;
        require(old_calls.calls.size() == expected_calls || (window_fixture && old_calls.calls.size() == 4),
                label + " did not reach the expected external calls");
        if (input.pc == option_description || (input.pc == bonus_description && !amount_description)) {
            require(old_calls.dispatched_names == 1 && new_calls.dispatched_names == 0,
                    label + " did not take the direct C++ label call");
        }
    }
    if (inputs.transfer_clause) {
        require(old_stop == "Competition callback transfer" && modern->pc == 0x18000e00,
                label + " lost nonlocal transfer");
        ++counts.transfers;
    } else if (hole != no_hole) {
        require(old_stop.starts_with("Unmapped data access at "), label + " did not fault at the removed byte");
        ++counts.faults;
    } else if (old_stop == "Native instruction budget exhausted") {
        require(input.budget < normal_budget, label + " exhausted the normal budget");
        ++counts.budgets;
    } else {
        require(old_stop.empty() && modern->pc == return_sentinel, label + " did not return");
        if (!negative) ++counts.returned;
    }
    require(expected.bytes == actual.bytes, label + " changed ordered memory accesses");
    require(original->executed == modern->executed, label + " changed budget accounting");
    const auto band = unsigned(modern->r<4>(12));
    const Image image(*original);
    image.compare(*modern, label);
    if (negative) {
        modern->wr<4>(12, band ^ 1);
        bool rejected = false;
        try {
            image.compare(*modern, label);
        } catch (const std::runtime_error& error) {
            rejected = std::string_view(error.what()).starts_with(label + " register byte=");
        }
        require(rejected, label + " accepted a changed result");
        ++counts.negatives;
    }
    const auto contract_value = adjustment && old_stop.empty() ? std::uint32_t(modern->memory.read<4>(record + 12)) : 0;
    guest::Cpu cpu(*modern);
    const auto text = text_fixture && old_stop.empty() ? read_text(cpu, output_buffer) : std::string{};
    Result result{band, unsigned(modern->executed), contract_value, text, std::move(actual.log)};
    result.dependency_calls = old_calls.calls.size();
    if (window_fixture && old_stop.empty()) {
        result.boundary = cpu.u8(boundary_out);
        result.threshold = cpu.u8(threshold_out);
        result.dependency_calls = old_calls.calls.size();
    }
    if (sample_fixture && old_stop.empty()) {
        result.dependency_calls = old_calls.calls.size();
        result.cursor = cpu.u32(rng_cursor);
        result.salt = cpu.u32(rng_salt);
    }
    return result;
}

void date_boundaries(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    constexpr std::array<std::pair<int, int>, 24> bands{{
        {0, 14}, {1, 14}, {2, 12}, {59, 12}, {60, 13},
        {61, 12}, {79, 12}, {80, 11}, {81, 10}, {99, 10},
        {100, 9}, {101, 8}, {119, 8}, {120, 7}, {121, 6},
        {139, 6}, {140, 5}, {141, 4}, {159, 4}, {160, 3},
        {161, 2}, {179, 2}, {180, 1}, {181, 0},
    }};
    for (const auto& [day, band] : bands) {
        c.put16(record + 0x30, day);
        const auto full = compare_entry(*input, counts);
        require(full.value == unsigned(band), "Unexpected original date boundary " + std::to_string(day));
        for (unsigned cut = 0; cut < full.steps; ++cut) {
            input->budget = cut;
            compare_entry(*input, counts);
        }
        input->budget = normal_budget;
    }
    for (const auto day : {-32768, -2, -1, 32767}) {
        c.put16(record + 0x30, std::uint16_t(day));
        compare_entry(*input, counts);
    }
    c.put16(record + 0x30, 181);
    for (unsigned kind = 0; kind < 256; ++kind) {
        c.put8(record + 0x38, kind);
        compare_entry(*input, counts);
    }
}

void faults_and_mutations(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    for (const auto day : {0, 25, 60, 80, 100, 120, 140, 160, 180, 200}) {
        c.put16(record + 0x30, day);
        const auto full = compare_entry(*input, counts);
        for (const auto& access : full.accesses) {
            compare_entry(*input, counts, access.address);
        }
    }

    // Vary both year sides of each comparison. Keep the live date objects
    // mapped and aligned, and vary the second word to check exact spills.
    std::mt19937 random(0x80710);
    for (unsigned n = 0; n < 512; ++n) {
        c.put16(record + 0x30, 1 + random() % 366);
        c.put16(record + 0x32, dates_year - 1 + random() % 3);
        for (unsigned k = 0; k < date_slots.size(); ++k) {
            const auto address = dates + k * 8;
            c.put16(address, 1 + random() % 366);
            c.put16(address + 2, dates_year - 1 + random() % 3);
            c.put32(address + 4, random());
        }
        compare_entry(*input, counts);
    }
    // The contract may share memory with a date. The same load must still
    // observe a spill through a date pointer overlapping the red zone.
    c.put32(toc + std::uint32_t(date_slots[0]), record + 0x30);
    compare_entry(*input, counts);
    c.put32(toc + std::uint32_t(date_slots[0]), stack - 8);
    c.put16(stack - 8, 1);
    c.put16(stack - 6, dates_year);
    compare_entry(*input, counts);
    compare_entry(*input, counts, no_hole, true);
}

void terms_leaves(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    constexpr std::array indexed_writes{0x10374af0u, 0x10374b40u, 0x10374b60u, 0x10374b80u};
    for (const auto entry : indexed_writes) {
        input->pc = entry;
        for (unsigned selector = 0; selector < 256; ++selector) {
            // Leave high bits set to check the signed low-byte convention.
            c.set(4, 0x10203000 | selector);
            c.set(5, 0xf1e2d3c4 ^ selector);
            compare_entry(*input, counts);
        }
    }
    for (const auto entry : {0x10374ce0u, 0x10374d50u}) {
        input->pc = entry;
        for (unsigned kind = 0; kind < 256; ++kind) {
            c.put8(record + 0x38, kind);
            const auto result = compare_entry(*input, counts);
            require(result.value == unsigned((kind >> 2) == (entry == 0x10374ce0u ? 3u : 1u)),
                    "Packed contract kind");
        }
    }
    input->pc = 0x10380520;
    for (unsigned first = 0; first < 4; ++first) {
        for (unsigned second = 0; second < 4; ++second) {
            for (unsigned fourth = 0; fourth < 4; ++fourth) {
                c.put8(record + 0x1c, first);
                c.put8(record + 0x1d, second);
                c.put8(record + 0x1f, fourth);
                const auto result = compare_entry(*input, counts);
                require(result.value == unsigned(first == 2 || second == 1 || fourth == 2),
                        "Exact release-clause flag");
            }
        }
    }
    for (const auto entry : {0x1037fa50u, 0x1037fa60u, 0x1037fa70u, 0x10380700u}) {
        input->pc = entry;
        for (auto mask : {0u, 0x00000001u, 0x55555555u, 0x80000000u, UINT32_MAX}) {
            for (auto current : {0u, 0x80u, 0x55555555u, 0x80000000u, UINT32_MAX}) {
                c.set(4, mask);
                c.put32(record + 0x48, current);
                compare_entry(*input, counts);
            }
        }
    }

    const auto check_boundaries = [&](std::uint32_t entry, unsigned argument) {
        input->pc = entry;
        c.set(4, argument);
        const auto full = compare_entry(*input, counts);
        for (const auto& access : full.accesses)
            compare_entry(*input, counts, access.address);
        for (unsigned cut = 0; cut < full.steps; ++cut) {
            input->budget = cut;
            compare_entry(*input, counts);
        }
        input->budget = normal_budget;
    };
    for (const auto entry : indexed_writes)
        check_boundaries(entry, 1);
    check_boundaries(0x10374af0, 2);
    for (const auto entry : {0x10374ba0u, 0x10374ce0u, 0x10374d50u, 0x1037fa50u, 0x1037fa60u,
                             0x1037fa70u, 0x10380520u, 0x10380700u})
        check_boundaries(entry, 0);

    input->pc = 0x10374af0;
    compare_entry(*input, counts, no_hole, true);
}

void draft_and_date_calls(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    input->pc = draft_entry;
    input->memory.copy_in(record, std::vector<std::uint8_t>(100, 0x5a));
    const auto draft = compare_entry(*input, counts);
    require(draft.value == record, "Draft constructor result");
    for (const auto& access : draft.accesses)
        compare_entry(*input, counts, access.address);
    for (unsigned cut : {0u, 1u, 7u, 14u}) {
        input->budget = cut;
        compare_entry(*input, counts);
    }
    input->budget = normal_budget;

    input->pc = delta_entry;
    c.put32(dates + 4, 0);
    for (const auto year : {1900, 2000, 2001, 2004, 2100}) {
        c.put16(record + 0x32, year);
        for (const auto day : {-32768, -1, 0, 1, 59, 60, 365, 366, 32767}) {
            c.put16(record + 0x30, std::uint16_t(day));
            c.put8(record + 0x34, year % 4 == 0);
            compare_entry(*input, counts);
        }
    }
    c.put16(record + 0x30, 150);
    c.put16(record + 0x32, dates_year);
    c.put8(record + 0x34, 0);
    require(compare_entry(*input, counts).value == 149, "Independent same-year date difference");
    compare_entry(*input, counts, no_hole, false, true);

    input->pc = 0x103803b0;
    c.put32(record + 0x14, 0);
    require(compare_entry(*input, counts).value == 0, "No staff profile");
    const auto profile = fixture + 0x600;
    c.put32(record + 0x14, profile);
    for (const auto role : {0u, 19u, 20u, 21u, 255u}) {
        c.put8(profile + 0x0f, role);
        for (unsigned index = 0; index < 256; ++index) {
            c.set(4, 0xabcd0000 | index);
            const auto value = compare_entry(*input, counts).value;
            require(value == unsigned(role == 20 ? index == 2 : index != 2), "Bonus role eligibility");
        }
    }
}

void value_adjustments(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    for (const auto entry : {primary_adjustment_entry, alternate_adjustment_entry}) {
        input->pc = entry;
        for (auto flags : {0u, 0x400000u, 0x800000u, 0xc00000u, UINT32_MAX}) {
            c.put32(preferences + 4, flags);
            const int scale = flags & 0x800000u ? 52 : flags & 0x400000u ? 4 : 1;
            for (auto amount : {0u, 1u, 0x7fffffffu, 0x80000000u, UINT32_MAX}) {
                c.put32(record + 12, amount);
                for (int output : {INT32_MIN, -53, -52, -51, -5, -4, -3, -2, -1, 0, 1, 51, 52, 53, INT32_MAX}) {
                    const auto result = compare_entry(*input, counts, no_hole, false, false, std::uint32_t(output));
                    require(result.contract_value == std::uint32_t(output / scale), "Signed periodic adjustment");
                }
            }
            const auto full = compare_entry(*input, counts, no_hole, false, false, 500);
            for (auto cut : {0u, 1u, full.steps / 2, full.steps - 1}) {
                input->budget = cut;
                compare_entry(*input, counts, no_hole, false, false, 500);
            }
            input->budget = normal_budget;
        }
    }
}

void adjustment_sequence() {
    auto original = expiry_fixture();
    auto modern = clone(*original);
    const auto run = [](State& s, bool reference) {
        RoutineChecker fixture_filter([](std::uint32_t entry) { return !is_fixture_service(entry); }, 0, 0);
        ScopedRoutineChecker checker(&fixture_filter);
        DependencyTrace trace;
        ScopedDependencyProbe probe(&trace);
        guest::Cpu c(s);
        c.put32(record + 12, 10);
        c.put32(preferences + 4, 0xc00000);
        trace.adjustment_output = 1093;
        const auto call = [&](std::uint32_t entry) {
            s.pc = entry;
            c.set(3, record);
            c.set_lr(return_sentinel);
            if (reference) staff_contracts_run_original(s, entry);
            else run_until(s, return_sentinel);
        };
        call(primary_adjustment_entry);
        require(c.i32(record + 12) == 21, "Periodic adjustment result before the second call");
        c.put32(preferences + 4, 0x400000);
        trace.adjustment_output = std::uint32_t(-9);
        call(alternate_adjustment_entry);
        require(c.i32(record + 12) == -2, "Signed adjusted value");
        require(trace.calls.size() == 2
                    && trace.calls[0].entry == primary_adjustment && trace.calls[0].arguments[1] == 4
                    && trace.calls[0].adjustment_input == 520
                    && trace.calls[1].entry == alternate_adjustment && trace.calls[1].arguments[1] == 8
                    && trace.calls[1].adjustment_input == 84,
                "Both adjustments must consume the live record in order");
        return trace.calls;
    };
    require(run(*original, true) == run(*modern, false), "Adjustment sequence changed external arguments");
    Image(*original).compare(*modern, "Adjustment sequence");
    require(original->executed == modern->executed, "Adjustment sequence changed accounting");
}

void label_cases(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    constexpr std::array bonus_offsets{0x289, 0x294, 0x2a1, 0x2b3};
    constexpr std::array option_offsets{0x2c6, 0x2e3, 0x2fe, 0x319, 0x333, 0x34e};
    for (const auto entry : {bonus_name, option_name, bonus_description, option_description}) {
        input->pc = entry;
        for (unsigned raw = 0; raw < 256; ++raw) {
            const auto index = std::int8_t(raw);
            const auto selector = 0x9abc0000u | raw;
            if (entry == bonus_name || entry == option_name)
                c.set({{3, selector}, {4, output_buffer}});
            else
                c.set({{3, record}, {4, selector}, {5, output_buffer}, {7, 0x1101}});
            const bool bonus = entry == bonus_name || entry == bonus_description;
            const auto offset = bonus ? bonus_offsets[index >= 0 && index < 3 ? index : 3]
                                      : option_offsets[index >= 0 && index < 5 ? index : 5];
            const auto expected = "label" + std::to_string(offset);
            require(compare_entry(*input, counts).text == expected, "Signed label selector");
            if (entry == bonus_description) {
                c.set(7, 0x1100);
                require(compare_entry(*input, counts).text == "amount " + expected, "Full bonus label");
            }
        }
    }
    for (const auto entry : {bonus_name, option_name, bonus_description, option_description}) {
        input->pc = entry;
        if (entry == bonus_name || entry == option_name)
            c.set({{3, 0}, {4, output_buffer}});
        else
            c.set({{3, record}, {4, 0}, {5, output_buffer}, {7, 1}});
        const auto full = compare_entry(*input, counts);
        for (unsigned cut : {0u, 1u, full.steps / 2, full.steps - 1}) {
            input->budget = cut;
            compare_entry(*input, counts);
        }
        input->budget = normal_budget;
        compare_entry(*input, counts, output_buffer);
    }
    input->pc = bonus_name;
    c.set({{3, 0}, {4, output_buffer}});
    compare_entry(*input, counts, no_hole, false, true);
}

void window_cases(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    input->pc = window_entry;
    c.set({{5, boundary_out}, {6, threshold_out}});
    c.put8(boundary_out, 0xa5);
    c.put8(threshold_out, 0x5a);
    c.put16(record + 0x28, 80);
    c.put16(record + 0x2a, dates_year);
    c.put16(window_reference, 50);
    c.put16(window_reference + 2, dates_year);

    const auto run = [&](WindowInputs data = {}) {
        return compare_entry(*input, counts, no_hole, false, false, std::nullopt, {.window = data});
    };
    const auto disabled = run();
    require(disabled.value == 1 && disabled.boundary == 0 && disabled.threshold == 0x5a
                && disabled.dependency_calls == 1, "Disabled contract window");
    c.put8(window_flag, 1);
    for (const auto year : {dates_year - 1, dates_year, dates_year + 1}) {
        for (const auto day : {-32768, -1, 0, 49, 50, 51, 80, 81, 32767}) {
            c.put16(window_reference, std::uint16_t(day));
            c.put16(window_reference + 2, year);
            const auto result = run();
            const bool after = year > dates_year || (year == dates_year && day > 80);
            require(result.value == 1 && result.dependency_calls == (after ? 1u : 4u),
                    "Signed day and unsigned year order");
        }
    }
    c.put16(window_reference, 50);
    c.put16(window_reference + 2, dates_year);
    for (unsigned age = 0; age < 256; ++age) {
        const unsigned threshold = std::int8_t(age) < 28 ? 3 : 2;
        for (const auto years : {0u, 1u, 2u, 3u, 4u, 65535u}) {
            for (const auto days : {-1, 0, 1}) {
                const auto result = run({age | 0xffff0000u, std::uint16_t(years), std::int16_t(days)});
                require(result.dependency_calls == 4 && result.value == unsigned(years < threshold)
                            && result.threshold == threshold && result.boundary == (years == threshold && days == 0),
                        "Contract age threshold and exact boundary");
            }
        }
    }
    const WindowInputs exact{28, 2, 0};
    const auto full = run(exact);
    for (const auto cut : {0u, 1u, 10u, full.steps / 2, full.steps - 1}) {
        input->budget = cut;
        run(exact);
    }
    input->budget = normal_budget;
    c.set({{5, 0}, {6, 0}});
    require(run(exact).value == 0, "Absent optional window outputs");
    c.set({{5, boundary_out}, {6, boundary_out}});
    require(run(exact).boundary == 1, "Aliased optional window outputs");
}

void sampling_cases(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    input->pc = sampling_entry;
    c.set(3, 0);
    const auto zero = compare_entry(*input, counts);
    require(zero.value == 0 && zero.cursor == random_words + 2 && zero.salt == 41 && zero.dependency_calls == 0,
            "Zero bound must retain the pool");
    for (int bound : {1, 17, 65535, -1, -17, -65535}) {
        c.set(3, std::uint32_t(bound));
        const int residue = (12345 + 2 * 1009 + 41) % bound;
        const auto result = compare_entry(*input, counts);
        require(result.value == std::uint32_t(bound > 0 ? residue : -residue)
                    && result.cursor == random_words + 4 && result.dependency_calls == 0,
                "Signed small range must draw once");
    }

    for (unsigned last : {0u, 2u, 8u}) {
        c.put32(rng_end, random_words + last);
        for (unsigned cursor : {0u, 2u, 4u, 6u, 8u}) {
            if (cursor > last) continue;
            c.put32(rng_cursor, random_words + cursor);
            for (auto salt : {0u, 41u, 65535u}) {
                c.put32(rng_salt, salt);
                for (const int bound : {INT32_MIN, -100000, -65536, -65535, -1, 1, 65535, 65536, 100000, INT32_MAX}) {
                    c.set(3, std::uint32_t(bound));
                    compare_entry(*input, counts);
                }
            }
        }
    }
    // Wrapped tenfold product whose negative reciprocal intermediate has
    // no bits discarded by the shift.
    c.set(3, std::uint32_t(-566327256));
    compare_entry(*input, counts);
    c.put32(rng_end, random_words);
    c.put32(rng_cursor, random_words);
    c.set(3, 100000);
    const auto wrapped = compare_entry(*input, counts);
    require(wrapped.dependency_calls == 2 && wrapped.cursor == random_words && wrapped.salt == 0x1235,
            "Both wide draws must reseed in order");
    for (unsigned cut : {0u, 1u, wrapped.steps / 2, wrapped.steps - 1}) {
        input->budget = cut;
        compare_entry(*input, counts);
    }
    input->budget = normal_budget;
    for (const auto address : {rng_cursor, rng_end, rng_salt, random_words, stack - 0x30})
        compare_entry(*input, counts, address);
    compare_entry(*input, counts, no_hole, true);
}

void clause_cases(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    input->pc = clause_entry;
    c.set(4, fixture_club);
    for (unsigned lookup = 0; lookup < 3; ++lookup) {
        c.put32(fixture_club + 0x30, lookup ? competition_index : 0);
        c.put32(competition_table + 4, lookup == 2 ? competition : 0);
        c.put16(record + 0x5c, 32);
        for (unsigned raw = 0; raw < 256; ++raw) {
            c.set(5, 0x12340000 | raw);
            const bool callback = lookup == 2 && (raw == 0 || raw == 3);
            const auto result = compare_entry(*input, counts);
            require(result.value == unsigned(raw == 1 || raw == 4 || callback)
                        && result.dependency_calls == unsigned(callback),
                    "Competition and clause eligibility");
        }
    }
    c.set(5, 4);
    for (unsigned raw = 0; raw < 256; ++raw) {
        c.put16(record + 0x5c, 0x1200 | raw);
        require(compare_entry(*input, counts).value == unsigned(std::int8_t(raw) > 31), "Signed clause age");
    }
    for (auto selected : {0u, 3u}) {
        c.set(5, selected);
        for (auto reply : {0u, 1u, 127u, 128u, 255u, 256u, UINT32_MAX}) {
            const auto result = compare_entry(*input, counts, no_hole, false, false, std::nullopt,
                                              {.clause_reply = reply});
            require(result.value == unsigned(std::int8_t(reply) != 0) && result.dependency_calls == 1,
                    "Signed competition reply");
        }
    }
    c.set(5, 3);
    const auto full = compare_entry(*input, counts);
    for (const auto cut : {0u, 1u, full.steps / 2, full.steps - 1}) {
        input->budget = cut;
        compare_entry(*input, counts);
    }
    input->budget = normal_budget;
    compare_entry(*input, counts, competition_vtable + 0xa4);
    compare_entry(*input, counts, no_hole, false, true);
    compare_entry(*input, counts, no_hole, false, false, std::nullopt, {.transfer_clause = true});
}

void wage_and_end_text_cases(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    input->pc = wage_entry;
    c.set(4, output_buffer);
    c.put32(record + 12, 0x7fffffff);
    write_text(c, output_buffer, "unchanged");
    for (auto flags : {0u, 0x200000u, 0x400000u, 0x800000u, UINT32_MAX}) {
        c.put32(preferences + 4, flags);
        for (auto style : {0u, 1u, 2u, 255u}) {
            c.set({{5, style}, {6, 0xa5a50080u | style}});
            const auto expected_offset = flags & 0x200000 ? (style == 2 ? 0x2a : 0x69)
                : flags & 0x400000 ? (style == 2 ? 0x3f : 0x83)
                : flags & 0x800000 ? (style == 2 ? 0x54 : 0x9e) : 0;
            const auto expected = !style ? "amount" : expected_offset ? "label" + std::to_string(expected_offset)
                : "unchanged";
            const auto result = compare_entry(*input, counts);
            require(result.text == expected && result.dependency_calls == (style && !expected_offset ? 2u : 3u),
                    "Wage period precedence and text");
        }
    }
    c.set(5, 2);
    c.put32(preferences + 4, 0x200000);
    const auto after = compare_entry(*input, counts, no_hole, false, false, std::nullopt,
                                     {.wage_preferences = 0x800000});
    require(after.text == "label84" && after.dependency_calls == 3, "Reread the live wage period after formatting");
    for (const auto cut : {0u, 1u, after.steps / 2, after.steps - 1}) {
        input->budget = cut;
        compare_entry(*input, counts);
    }
    input->budget = normal_budget;
    compare_entry(*input, counts, output_buffer);

    input->pc = end_text_entry;
    for (unsigned packed = 0; packed < 256; ++packed) {
        c.put8(record + 0x38, packed);
        const auto kind = std::int8_t(packed) >> 2;
        const auto expected = kind == 1 ? "label243" : kind == 3 ? "label185" : kind == 4 ? "label202" : "2001/150";
        require(compare_entry(*input, counts).text == expected, "Packed contract end label");
    }
    const auto full = compare_entry(*input, counts);
    for (const auto cut : {0u, 1u, full.steps / 2, full.steps - 1}) {
        input->budget = cut;
        compare_entry(*input, counts);
    }
    input->budget = normal_budget;
    compare_entry(*input, counts, output_buffer);
}

void release_presentation_cases(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    input->pc = squad_entry;
    c.set({{4, output_buffer}, {5, 20}});
    for (unsigned status = 0; status < 256; ++status) {
        c.put8(record + 0x60, status);
        const auto result = compare_entry(*input, counts);
        require(result.value == 1 && result.dependency_calls == 1
                    && result.text == "S" + std::to_string(std::min(status & 15, 7u)),
                "Squad status must select by the low nibble");
    }
    for (auto entry : {squad_entry, summary_entry}) {
        input->pc = entry;
        for (auto capacity : {0u, 1u, 19u, 20u, 21u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
            c.set(5, capacity);
            const auto result = compare_entry(*input, counts);
            const auto fits = std::int32_t(capacity) >= 20;
            require(result.dependency_calls == (fits ? 1 : 3), "Short-buffer diagnostic order");
            if (!fits) require(result.value == 0 && result.text.empty(), "Short-buffer result");
            for (unsigned budget = 1; budget < result.steps; ++budget) {
                input->budget = budget;
                compare_entry(*input, counts);
            }
            input->budget = normal_budget;
        }
    }

    c.set(5, 20);
    constexpr std::array clause_fields{0x1c, 0x1d, 0x1f, 0x20};
    constexpr std::array clause_labels{0x4a4, 0x460, 0x4eb, 0x582};
    for (unsigned n = 0; n < clause_fields.size(); ++n) {
        for (auto field : clause_fields) c.put8(record + field, 0);
        for (unsigned value = 0; value < 256; ++value) {
            c.put8(record + clause_fields[n], value);
            const auto result = compare_entry(*input, counts);
            require(result.value == (n == 1 && value ? 1 : value)
                        && result.text == (value ? "label" + std::to_string(clause_labels[n]) : ""),
                    "Release label and raw status");
        }
        c.put8(record + clause_fields[n], 3);
        ExternalInputs inputs;
        inputs.change_release_field = clause_fields[n];
        require(compare_entry(*input, counts, no_hole, false, false, std::nullopt, inputs).value == 7,
                "Release text callback must be able to update the returned clause");
    }
    c.put8(record + 0x1d, 0);
    for (auto first : {0, 1, 2, 127, 128, 255}) {
        for (auto second : {0, 1, 2, 127, 128, 255}) {
            c.put8(record + 0x1c, first);
            c.put8(record + 0x1f, second);
            compare_entry(*input, counts);
        }
    }
    compare_entry(*input, counts, output_buffer);

    input->pc = squad_entry;
    c.put8(record + 0x60, 0);
    input->memory.map(0x10380cb0, 0x9c);
    write_text(c, output_buffer, "input");
    constexpr auto relocated = squad_table + 0x20;
    c.put32(toc - 0x4dc0, relocated);
    for (auto target = 0x10380cb4u; target <= 0x10380d34u; target += 4) {
        c.put32(relocated, target);
        const auto result = compare_entry(*input, counts);
        if (target == 0x10380d34) require(result.value == target, "Direct epilogue preserves the incoming result");
    }
    c.put32(relocated, external_continuation);
    const auto continuation = compare_entry(*input, counts);
    require(continuation.value == 19 && continuation.dependency_calls == 1, "Live table external continuation");
    compare_entry(*input, counts, squad_table + 0x20);
}

void review_state_cases(Counts& counts) {
    auto input = expiry_fixture();
    guest::Cpu c(*input);
    c.put32(record, 1);
    c.put8(record + 0x60, 0xef);
    c.put32(record + 0x50, 0x81010001);
    c.put32(record + 0x48, 0xc0c0babe);
    input->pc = reset_review_entry;
    c.set({{4, staff_rows + 0x70}, {5, review_club}});
    for (auto reply : {0u, 4u, 10u, 11u, 127u, 128u, 255u, 256u, 0xffffffffu}) {
        ExternalInputs inputs;
        inputs.review_reply = reply;
        const auto result = compare_entry(*input, counts, no_hole, false, false, std::nullopt, inputs);
        const auto grade = std::min(10, int(std::int8_t(reply)));
        require(result.value == std::uint32_t(grade) && result.dependency_calls == 2,
                "Signed review grade and ordered staff/league calls");
    }
    compare_entry(*input, counts, review_league + 0x119);
    compare_entry(*input, counts, record + 0x48);
    c.put32(review_club + 0x2c, 0);
    require(compare_entry(*input, counts).dependency_calls == 1, "Review reset skips the missing league");
    c.set(5, 0);
    require(compare_entry(*input, counts).dependency_calls == 1, "Review reset skips the missing employer");
    compare_entry(*input, counts, no_hole, false, true);

    input->pc = clear_review_entry;
    for (auto mask : {0u, 1u, 0x10u, 0x100u, 0x10000u, 0x80000000u, 0xffffffffu}) {
        c.set(4, mask);
        const auto result = compare_entry(*input, counts);
        require(result.dependency_calls == 2, "Condition refresh must precede clearing the staff record");
        for (unsigned budget = 1; budget < result.steps; ++budget) {
            input->budget = budget;
            compare_entry(*input, counts);
        }
        input->budget = normal_budget;
    }
    compare_entry(*input, counts, record + 0x50);

    input->pc = clear_all_review_entry;
    for (auto refresh : {0u, 1u, 127u, 128u, 255u, 256u, 0xffffffffu}) {
        c.set(4, refresh);
        const auto result = compare_entry(*input, counts);
        require(result.dependency_calls == (std::int8_t(refresh) ? 2 : 1), "Refresh checks the signed low byte");
    }
    compare_entry(*input, counts, staff_slot);
}

unsigned generated_hooks() {
    constexpr auto entries = std::to_array<std::uint32_t>({
#define STAFF_CONTRACTS_ROUTINE(entry, ...) entry,
#define STAFF_CONTRACTS_CONTINUATION(entry, ...) entry,
#include "staff_contracts_routines.inc"
#undef STAFF_CONTRACTS_CONTINUATION
#undef STAFF_CONTRACTS_ROUTINE
    });
    RoutineChecker checker([](std::uint32_t entry) { return !is_fixture_service(entry); }, 1, 1);
    checker.strict_volatile = true;
    ScopedRoutineChecker scope(&checker);
    for (const auto entry : entries) {
        auto s = expiry_fixture();
        s->pc = entry;
        s->wr<4>(16, 1);
        if (entry == sampling_entry) s->wr<4>(12, 17);
        if (entry == clause_entry) {
            s->wr<4>(16, fixture_club);
            s->wr<4>(20, 1);
        }
        if (entry == window_entry) {
            s->wr<4>(20, boundary_out);
            s->wr<4>(24, threshold_out);
        }
        const bool text_fixture = label_entry(entry);
        if (text_fixture) {
            localization_format_diagnostics(*s).original_body = formatter;
            s->formatted_output_original = true;
            guest::Cpu c(*s);
            if (entry == bonus_name || entry == option_name) c.set({{3, 0}, {4, output_buffer}});
            else if (entry == wage_entry || entry == end_text_entry) c.set({{4, output_buffer}, {5, 0}, {6, 0}});
            else if (release_entry(entry)) c.set({{4, output_buffer}, {5, 20}});
            else c.set({{4, 0}, {5, output_buffer}, {7, 0}});
        }
        const bool review_fixture = review_state_entry(entry);
        if (review_fixture) {
            contract_review_diagnostics(*s).original_body = refresh_review_entry;
            s->wr<4>(20, review_club);
        }
        DependencyTrace trace{.calls = {}, .adjustment_output = 0, .text_fixture = text_fixture,
                              .review_reply = review_fixture ? std::optional(8u) : std::nullopt};
        ScopedDependencyProbe probe(value_adjustment(entry) || text_fixture || review_fixture ? &trace : nullptr);
        const auto before = checker.totals().find(entry);
        const auto prior = before == checker.totals().end() ? 0 : before->second.checked;
        run_until(*s, return_sentinel);
        const auto result = checker.totals().find(entry);
        require(result != checker.totals().end() && result->second.checked == prior + 1
                    && result->second.differences == 0 && s->pc == return_sentinel,
                "Default generated page did not enter the staff-contract owner at " + hex_address(entry));
    }
    return entries.size();
}
}

int main() {
    try {
#ifdef CM_STAFF_CONTRACTS_PROFILE_DIR
        routine_child_exit = [] {
            const auto path = std::string(CM_STAFF_CONTRACTS_PROFILE_DIR) + "/check-"
                + std::to_string(getpid()) + ".profraw";
            __llvm_profile_set_filename(path.c_str());
            if (__llvm_profile_write_file() != 0) throw std::runtime_error("Cannot flush child coverage");
        };
#endif
        Counts counts;
        date_boundaries(counts);
        faults_and_mutations(counts);
        terms_leaves(counts);
        draft_and_date_calls(counts);
        value_adjustments(counts);
        adjustment_sequence();
        label_cases(counts);
        window_cases(counts);
        sampling_cases(counts);
        clause_cases(counts);
        wage_and_end_text_cases(counts);
        release_presentation_cases(counts);
        review_state_cases(counts);
        const auto hooks = generated_hooks();
        std::cout << "Staff contracts " << counts.returned << " returns, " << counts.budgets
                  << " budget stops, " << counts.faults << " partial faults, "
                  << counts.negatives << " rejected mutations, " << counts.transfers << " nonlocal transfers, " << hooks << " production hook comparisons\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

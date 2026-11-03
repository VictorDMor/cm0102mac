// Controlled comparisons against independent original pages, with all owned
// entries enabled together on the production side.
#include "contract_screens_adapter.hpp"
#include "guest_abi.hpp"
#include "player_stats_detail.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <set>

using namespace cm::aot;
using namespace cm::aot::guest;

namespace cm::aot { bool contract_screens_reference(State&); }

namespace {
constexpr std::uint32_t open_news = 0x10093ae0, draw_news = 0x10093bd0, release_draft = 0x100980d0, open_details = 0x1009dab0,
                        restricted_news = 0x10093a50, offer_news = 0x1009ed20, offer_choice = 0x10094ae0, offer_choice_end = 0x100952a4,
                        new_negotiation = 0x100960d0, existing_negotiation = 0x100965c0,
                        choice_draw = 0x100952b0, negotiation_choice = 0x10095db0,
                        open_staff = 0x1009e880, draw_staff = 0x1009e8f0, staff_command = 0x1009eb30,
                        compare_staff = 0x1009ee00, load_language = 0x1009ef30,
                        save_db = 0x1009ef60, load_db = 0x1009ef90;
constexpr std::uint32_t append = 0x1073e4b0, read_history = 0x1073ef70, write_history = 0x1073f1e0,
                        release = 0x107735a0, init_news = 0x102bdf90, fetch_news = 0x102ccdd0,
                        news_payload = 0x102cb570, read_form = 0x10303560;
constexpr std::uint32_t fixture = 0x60000000, trace = fixture + 0x10000, trace_size = 0x40000, screens = fixture + 0x2000,
                        staff = fixture + 0x3000, object = fixture + 0x4000, stack = 0x71010000,
                        sentinel = 0xfffffff0;
constexpr std::uint32_t history_data = fixture + 0x6000;
constexpr std::uint32_t club_payload_mask = 0x00013579, staff_payload_mask = 0x0002468a;
constexpr unsigned trace_words = 3 + 8 + 19 + 8 + 2 + 11;

struct Call {
    std::uint32_t target, back;
    std::vector<std::uint32_t> args, stack_args;
    std::optional<std::uint32_t> result, second_result, pointed_word, candidate_flags;
    std::optional<double> form;
};
std::vector<Call> expected;
unsigned calls = 0;
bool reference_mode = false, production_mode = false, sorting = false;
bool expect_nested_dispatch = false, saw_nested_dispatch = false;
unsigned callbacks = 0;
std::optional<std::uint32_t> expected_result;
std::optional<std::uint32_t> replacement_club;
std::optional<std::uint16_t> group_after_open, group_before_valuation;
struct OfferNewsPayload { std::uint32_t message, person, offer; };
std::optional<OfferNewsPayload> offer_news_payload;
std::uint32_t entry = 0;
std::uint32_t call_site = 0;
unsigned fixture_instruction_budget = 0;

void expect(std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> args,
            std::optional<std::uint32_t> result = {}, std::optional<double> form = {}) {
    expected.push_back({target, back, args, {}, result, {}, {}, {}, form});
}

bool service(State& s) {
    if (sorting) {
        if (s.pc == compare_staff) {
            Cpu c(s);
            const auto at = fixture + 0x5000 + callbacks++ * 12;
            if (at + 12 > fixture + 0x8000) throw std::runtime_error("candidate sort exceeded fixture bound");
            c.put32(at, c.r(3));
            c.put32(at + 4, c.r(4));
            c.put32(at + 8, c.lr());
        }
        if (reference_mode) return contract_screens_reference(s);
        return false;
    }
    if (reference_mode && (contract_screens_reference(s) || ((s.pc == 0x10775540 || s.pc == 0x10768810) && run_leaf(s)))) return true;
    if (expect_nested_dispatch && ((s.pc >= new_negotiation && s.pc < 0x10096c58) ||
                                    (s.pc >= offer_choice && s.pc < offer_choice_end))) {
        if (s.pc == new_negotiation || s.pc == existing_negotiation || s.pc == offer_choice)
            saw_nested_dispatch = true;
        return false;  // Continue the excluded original after each fixture service.
    }
    if (contract_screens_lists(s.pc)) return false;  // Exercise the generated owner hook.
    if (call_site && s.pc == call_site) return false;
    Cpu c(s);
    if (calls >= expected.size()) throw std::runtime_error("unexpected contract screen service " + hex_address(s.pc));
    const auto& call = expected[calls];
    if (s.pc != call.target || c.lr() != call.back)
        throw std::runtime_error("contract screen call " + hex_address(s.pc) + " returning to " +
                                 hex_address(c.lr()) + " expected " + hex_address(call.target) +
                                 " returning to " + hex_address(call.back));
    for (unsigned i = 0; i < call.args.size(); ++i)
        if (c.r(3 + i) != call.args[i])
            throw std::runtime_error("unexpected contract screen argument at " + hex_address(s.pc));
    for (unsigned i = 0; i < call.stack_args.size(); ++i)
        if (c.u32(c.sp() + 0x38 + 4 * i) != call.stack_args[i])
            throw std::runtime_error("unexpected contract screen stack argument at " + hex_address(s.pc));
    if (call.pointed_word && c.u32(c.r(3)) != *call.pointed_word)
        throw std::runtime_error("unexpected contract screen indirect argument at " + hex_address(s.pc));

    auto at = trace + calls++ * trace_words * 4;
    if (at + trace_words * 4 > trace + trace_size) throw std::runtime_error("contract screen trace exceeded fixture bound");
    auto put = [&](std::uint32_t value) { c.put32(at, value); at += 4; };
    put(s.pc);
    put(c.lr());
    put(c.r(0));
    for (unsigned r = 3; r <= 10; ++r) put(c.r(r));
    for (unsigned r = 13; r <= 31; ++r) put(c.r(r));
    for (unsigned field = 0; field < 8; ++field) put(c.cr(field));
    c.put64(at, s.r<8>(reg::fpr(1)));
    at += 8;
    for (unsigned i = 0; i < call.stack_args.size(); ++i) put(c.u32(c.sp() + 0x38 + 4 * i));

    switch (s.pc) {
    case append:
        if (group_after_open) c.put16(screens + 0x2c58, *group_after_open);
        break;
    case init_news:
        c.put32(c.r(3), 0);
        c.put32(c.r(3) + 8, 0);
        break;
    case fetch_news:
        if (offer_news_payload) {
            c.put32(c.r(6), offer_news_payload->message);
            c.put32(c.r(6) + 0x10, offer_news_payload->person);
            c.put32(c.r(6) + 0x18, offer_news_payload->person);
            c.put32(c.r(6) + 0x2c, offer_news_payload->offer);
        } else {
            c.put32(c.r(6), c.r(4) ^ club_payload_mask);
            c.put32(c.r(6) + 8, c.r(5) ^ staff_payload_mask);
        }
        break;
    case news_payload:
        c.set(3, c.u32(c.r(3) + 4 * c.r(4) + (offer_news_payload ? 0x10 : 0)));
        break;
    case write_history:
        c.put32(history_data + c.r(4) * 4, c.r(5));
        if (c.r(4) == 22 && group_before_valuation) c.put16(screens + 0x2c58, *group_before_valuation);
        break;
    case release:
        c.put32(c.r(3), 0);
        break;
    case read_form:
        if (c.f(1) != c.f32(c.toc(-0x56f4) + 8))
            throw std::runtime_error("wrong default form argument");
        c.set_f(1, call.form.value());
        break;
    case 0x10369080:
        c.set_f(1, call.form.value());
        c.put32(c.r(9), call.candidate_flags.value());
        break;
    case 0x10739890:
        c.put32(c.r(6), 0x434f4e54);
        c.put32(c.r(7), 0x43505000);
        break;
    case 0x1077b720:
        c.put32(c.r(3), c.r(8));
        break;
    case 0x100c7a50:
        c.put16(c.r(5), 7);
        c.put16(c.r(6), 13);
        break;
    case 0x100caeb0:
        c.put16(c.r(4), 0x4100);
        break;
    case 0x10727890:
        c.put32(c.r(3), c.r(4));
        break;
    case 0x10084a80:
        // A separate boundary case changes the live club after the old club
        // was saved. Only the roster removal should see the replacement.
        if (replacement_club) c.put32(c.r(4) + 0x24, *replacement_club);
        c.put32(fixture + 0x300 + calls * 4, s.pc);
        break;
    case 0x100c8930:
    case 0x103e68c0:
    case 0x1030f950:
    case 0x103d2b30:
        c.put32(fixture + 0x300 + calls * 4, s.pc);
        break;
    case 0x100a55a0:
    case 0x100abfb0:
        c.put32(fixture + 0x300, c.u32(c.r(3)));
        break;
    }
    if (call.result) c.set(3, *call.result);
    if (call.second_result) c.set(4, *call.second_result);
    c.finish();
    return true;
}

void validate_return(State& s) {
    Cpu c(s);
    const auto back = call_site ? call_site + 4 : sentinel;
    if (s.pc != back || c.sp() != stack || calls != expected.size() ||
        (expected_result && c.r(3) != *expected_result))
        throw std::runtime_error("contract screen call did not complete its expected effects");
}

void original(State& s, std::uint32_t target) {
    calls = 0;
    reference_mode = true;
    s.pc = target;
    if (call_site) Cpu(s).link(call_site + 4);
    run_until(s, call_site ? call_site + 4 : sentinel);
    reference_mode = false;
    validate_return(s);
}

void rewrite(State& s, std::uint32_t target) {
    calls = 0;
    if (production_mode) {
        // Scripted services charge no instructions. Any generated instruction
        // in an owned body fails, including an accidental original fallback.
        // Entering a generated caller at its bl label charges that call once.
        s.budget = s.executed + (call_site ? 1 : 0) + fixture_instruction_budget;
        s.pc = call_site ? call_site : target;
        run_until(s, call_site ? call_site + 4 : sentinel);
    } else {
        if (call_site) Cpu(s).link(call_site + 4);
        contract_screens_run_rewrite(s, target);
    }
    validate_return(s);
}

void corrupt_trace(State& s, std::uint32_t target) {
    rewrite(s, target);
    Cpu c(s);
    c.put32(trace + 12, c.u32(trace + 12) ^ 1);
}

void corrupt_result(State& s, std::uint32_t target) {
    rewrite(s, target);
    Cpu c(s);
    c.set(3, c.r(3) ^ 1);
}

void rewrite_with_original_children(State& s, std::uint32_t target) {
    RoutineChecker selection([target](std::uint32_t address) { return address == target; }, 0, 0);
    routine_checker = &selection;
    expect_nested_dispatch = true;
    saw_nested_dispatch = false;
    calls = 0;
    const auto before = s.executed;
    contract_screens_run_rewrite(s, target);
    routine_checker = nullptr;
    validate_return(s);
    if (!saw_nested_dispatch || s.executed <= before)
        throw std::runtime_error("excluded negotiation child did not execute its original body");
}

Cpu initialize(State& s, std::mt19937& random, std::uint32_t target) {
    expected.clear();
    expected_result.reset();
    replacement_club.reset();
    group_after_open.reset();
    group_before_valuation.reset();
    offer_news_payload.reset();
    entry = target;
    call_site = 0;
    fixture_instruction_budget = 0;
    Cpu c(s);
    const std::vector<std::uint8_t> stack_residue(0x3000, std::uint8_t(random()));
    s.memory.copy_in(stack - stack_residue.size(), stack_residue);
    for (unsigned r = 0; r < 32; ++r) c.set(r, random());
    for (unsigned f = 0; f < 32; ++f) c.set_f(f, double(random()));
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, random() & 0xf);
    c.s.wr<1>(reg::xer_so, random() & 1);
    c.s.wr<1>(reg::xer_ca, random() & 1);
    c.set({{1, stack}, {2, 0x108bba90}});
    c.set_lr(sentinel);
    c.put32(c.r(2) - 0x7bb0, screens);
    c.put32(c.r(2) - 0x7bf0, fixture + 0x10);
    c.put32(c.r(2) - 0x7b3c, fixture + 0x14);
    c.put32(fixture + 0x10, fixture + 0x100);
    c.put32(fixture + 0x14, fixture + 0x200);
    for (unsigned i = 0; i < 8; ++i) c.put32(history_data + 4 * i, random());
    s.budget = s.executed + 100000;
    return c;
}

void history(std::uint32_t back, unsigned slot, std::uint32_t value) {
    expect(write_history, back, {screens, slot, value, 0}, history_data + 4 * slot);
}

struct NegotiationInput {
    bool existing = false, profile = false, link = false, exchange = false, permit = false, clear_link = false;
    bool change_group = false;
    std::uint32_t opened = 1, kind = 1;
    std::int32_t budget = 100'000, current_wages = 8'000;
    int first_club = -1, second_club = 0, final_club = 0;
};

void negotiation_case(Cpu& c, const NegotiationInput& input, std::uint32_t state, std::uint32_t mode,
                      std::uint32_t caller_stack = stack) {
    constexpr std::uint32_t league = fixture + 0x7100, leagues = fixture + 0x7200,
        club = fixture + 0x7500, contracts = fixture + 0x8000, instance = fixture + 0x8200,
        contract = fixture + 0x8400, record = fixture + 0x8500, profile = fixture + 0x8600,
        primary_cache = fixture + 0x8700;
    constexpr std::uint32_t person_id = 37, fee = 120'000, budget_limit = 5'000'000;
    const auto link = !input.link ? 0 : input.clear_link ? history_data + 4 * 20 : object;
    const auto first_back = input.existing ? 0x10096630u : 0x10096138u;
    const auto set_toc = [&](int slot, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(slot), value); };
    const auto manager = [&](int slot, std::uint32_t holder, std::uint32_t value) {
        set_toc(slot, holder);
        c.put32(holder, value);
    };
    manager(-0x7ab8, fixture + 0x20, leagues);
    manager(-0x7b7c, fixture + 0x24, contracts);
    manager(-0x7be8, fixture + 0x28, club);
    set_toc(-0x7b78, primary_cache);
    c.put32(primary_cache, 0);
    c.put32(staff, person_id);
    c.put32(staff + 0x14, input.profile ? profile : 0);
    c.put16(screens + 0x2c58, 0);
    c.put32(screens + 4, instance);
    c.put32(instance + 0x24, club);
    if (input.change_group) {
        group_after_open = 1;
        group_before_valuation = 2;
        for (unsigned group = 1; group <= 2; ++group) {
            c.put32(screens + group * 0x2c0 + 4, instance + group * 0x40);
            c.put32(instance + group * 0x40 + 0x24, club + group * 0x248);
        }
    }
    const auto budget_club = input.change_group ? club + 0x248 : club;
    const auto valuation_club = input.change_group ? club + 0x490 : club;
    if (link) c.put32(link, record);
    c.put32(record + 0xc, input.exchange ? person_id + 1 : person_id);
    c.put32(record + 0x20, fee);
    c.put32(record + 0x28, 0x6b9);
    c.put32(record + 0x2c, 0x37);
    c.put8(record + 0x35, 0xfe);
    for (unsigned i = 0; i <= 2; ++i) {
        c.put32(contract + i * 0x60 + 4, i);
        c.put32(contract + i * 0x60 + 0x50, 0x232 + i);
    }
    const auto signed_byte = [](auto value) { return std::uint32_t(std::int8_t(value)); };
    const auto signed_mode = std::uint32_t(std::int16_t(mode));
    if (input.existing) c.set({{3, link}, {4, staff}, {5, state}, {6, input.kind}, {7, mode}});
    else c.set({{3, staff}, {4, state}, {5, input.kind}, {6, mode}});
    expect(append, first_back, {screens, c.toc(-0x56e0), c.toc(-0x56e4), 1, c.toc(-0x56e8), 0}, input.opened);
    if (std::uint8_t(input.opened) == 0) return;

    const auto fields = [&](std::uint32_t back, unsigned first, std::initializer_list<std::uint32_t> values) {
        for (const auto value : values) { history(back, first++, value); back += 0x14; }
    };
    if (input.existing) {
        fields(0x1009664c, 1, {signed_mode, 0, link});
        fields(0x10096688, 8, {signed_byte(input.kind), 0, 0x76c, 0, 0x76c, signed_byte(state)});
        if (link) {
            history(0x10096708, 0, person_id);
            history(0x10096724, 7, 0xfffffffe);
            history(0x1009673c, 4, 0x6b9);
            history(0x10096754, 5, 0x37);
            history(input.exchange ? 0x10096794 : 0x1009677c, 6, input.exchange);
        } else {
            history(0x100967ac, 0, person_id);
            fields(0x100967c0, 6, {0, 0});
        }
        fields(0x100967e8, 14, {0, 0, 0, 0, 0, 0, 0});
    } else {
        fields(0x10096154, 0, {person_id, signed_mode, 0, 0});
        fields(0x100961a4, 6, {0, 0, signed_byte(input.kind), 0, 0x76c, 0, 0x76c, signed_byte(state),
                              0, 0, 0, 0, 0, 0, 0});
    }
    expect(0x10103510, input.existing ? 0x1009687c : 0x100962c8, {leagues, budget_club}, league);
    expect(0x100f5770, input.existing ? 0x10096894 : 0x100962e0, {league, staff, 0, 0, 1, 0}, 0xa5);
    expected.back().second_result = std::uint32_t(input.budget);
    const auto fee_adjusted = std::int32_t(std::uint32_t(input.budget) - (link && !input.clear_link ? fee : 0));
    const auto available = std::uint32_t(std::clamp(fee_adjusted, 0, std::int32_t(budget_limit)));
    history(input.existing ? 0x100968e8 : 0x10096314, 21, available);
    if (input.existing && input.profile) {
        expect(0x10081b10, 0x10096904, {contracts, staff, budget_club}, 987);
        history(0x10096918, 22, 987);
    } else {
        history(input.existing ? 0x10096930 : 0x10096328, 22, 0);
    }
    if (input.profile) {
        expect(0x10084be0, input.existing ? 0x10096964 : 0x10096350,
               {contracts, staff, valuation_club, 1, 0xffffffff, 0}, 120'000);
        expect(0x1000a170, input.existing ? 0x100969a8 : 0x10096394,
               {caller_stack - (input.existing ? 0xa0u : 0x90u) + 0x38, 4}, 4567);
        expected.back().pointed_word = std::uint32_t(std::int32_t(120'000 * c.f64(c.toc(-0x56d0) + 0x28)));
        history(input.existing ? 0x100969bc : 0x100963a8, 23, 4567);
    } else {
        history(input.existing ? 0x100969d4 : 0x100963c0, 23, 0);
    }

    const auto address_for = [&](int club_id) { return club_id < 0 ? 0u : contract + std::uint32_t(club_id) * 0x60; };
    const bool exceptional = input.kind == 11 || input.kind == 12 || input.kind == 13 || input.kind == 15;
    std::int32_t wage_ceiling = exceptional ? 6700 : 5000;
    if (exceptional) {
        expect(0x10103510, input.existing ? 0x10096af0 : 0x100964dc, {leagues, club}, league);
        expect(0x100f2680, input.existing ? 0x10096afc : 0x100964e8,
               {league, staff, input.existing ? state : 0xffffffffu}, wage_ceiling);
    } else {
        expect(0x10103510, input.existing ? 0x10096a04 : 0x100963f0, {leagues, club}, league);
        expect(0x100f4790, input.existing ? 0x10096a0c : 0x100963f8, {league, input.kind}, wage_ceiling);
        expect(0x10082f20, input.existing ? 0x10096a38 : 0x10096424, {contracts, staff}, address_for(input.first_club));
        if (input.first_club >= 0) {
            expect(0x10082f20, input.existing ? 0x10096a70 : 0x1009645c, {contracts, staff}, address_for(input.second_club));
            if (input.second_club == 0) {
                expect(0x10084b60, input.existing ? 0x10096aa8 : 0x10096494,
                       {contracts, staff}, std::uint32_t(input.current_wages));
                wage_ceiling = std::max(wage_ceiling, input.current_wages);
            }
        }
    }
    fields(input.existing ? 0x10096b38 : 0x10096524, 24, {std::uint32_t(wage_ceiling), 0, 0});
    expect(0x10082f20, input.existing ? 0x10096b6c : 0x10096558, {contracts, staff}, address_for(input.final_club));
    history(input.existing ? 0x10096b94 : 0x1009657c, 27, input.final_club < 0 ? 0 : 0x232 + input.final_club);
    if (!input.existing) fields(0x10096590, 28, {3, 0});
    else {
        if (input.final_club >= 0)
            expect(0x10380f20, 0x10096bb0, {address_for(input.final_club), budget_club, 0, 0}, input.permit);
        if (input.final_club >= 0 && (input.permit || link)) fields(0x10096bd4, 28, {3, 0});
        else {
            history(0x10096c00, 28, 0);
            const bool renew = input.final_club >= 0 && input.profile;
            history(renew ? 0x10096c40 : 0x10096c28, 29, renew);
        }
    }
}

#include "contract_screens_offer_cases.inc"
#include "contract_screens_choice_draw_cases.inc"

struct ChoiceInput {
    unsigned action = 2;
    bool fallback = false, control_default = false, existing = false, linked = false, stale = false;
    std::uint8_t status = 1, term = 5, kind = 1;
    std::uint32_t opened = 1, handled = 0, common = 0;
    int final_club = 0;
};

void negotiation_choice_case(Cpu& c, const ChoiceInput& input) {
    constexpr std::uint32_t layout = fixture + 0x7100, instance = fixture + 0x8200,
                            club = fixture + 0x7500, contracts = fixture + 0x8000,
                            offer = fixture + 0x8500, value = 0xabc123;
    const auto control = input.control_default ? 0x1234ffffu : 2u;
    std::vector<Call> nested;
    const bool refresh = input.linked && (input.stale || input.status == 2 || input.status == 3);
    const auto signed_term = std::int32_t(std::int8_t(input.term));
    const auto term = std::uint32_t(std::int8_t(signed_term < 0 ? -signed_term : signed_term));
    if (input.action == 2) {
        negotiation_case(c, {.existing = input.existing, .profile = true, .link = input.linked,
                             .opened = input.opened, .kind = input.kind, .final_club = input.final_club}, term, 1, stack - 0x60);
        if (!refresh) nested = std::move(expected);
        expected.clear();
        c.put8(offer + 0x34, input.status);
    }
    c.put16(screens + 0x2c58, 0);
    c.put32(screens + 0x24, layout);
    c.put32(layout + 0xc0000 + 2 * 0x17c - 0x5794, input.fallback ? 0 : input.action);
    c.put32(layout + 0xc0000 + 2 * 0x17c - 0x5754, value);
    c.put16(screens + 0x260000 - 0x7e62, input.action);
    c.set(3, control);
    switch (input.action) {
    case 3:
    case 0x2f:
        history(input.action == 3 ? 0x10095e54 : 0x10095ed4, input.action == 3 ? 1 : 4,
                input.control_default ? 0 : value);
        expected_result = std::uint32_t(-11);
        break;
    case 1:
        expected_result = std::uint32_t(-9);
        break;
    case 2: {
        const auto link = input.linked ? object : 0;
        expect(read_history, 0x10095f78, {screens, 0}, staff);
        expect(read_history, 0x10095f8c, {screens, 1}, input.term);
        expect(read_history, 0x10095fa4, {screens, 2}, link);
        expect(read_history, 0x10095fc4, {screens, 3}, input.existing ? 1 : 0);
        expect(read_history, 0x10095fd4, {screens, 4}, input.kind);
        expect(0x1007bb70, 0x10096000, {contracts, staff, instance, club, term});
        expect(read_history, 0x1009600c, {screens, 7}, input.stale ? 0x38 : 0x37);
        if (refresh) expect(0x10422570, 0x10096044, {offer, input.stale ? 0x38u : 0x37u});
        else expected.insert(expected.end(), nested.begin(), nested.end());
        expected_result = refresh ? std::uint32_t(-11) : std::uint32_t(-4);
        break;
    }
    default:
        expect(0x102a54b0, 0x10096094, {control}, input.handled);
        if (!input.handled) expect(0x102a83a0, 0x100960a8, {control}, input.common);
        expected_result = input.handled ? input.handled : input.common;
        break;
    }
}

void opening(Cpu& c, std::uint32_t back, int draw, int command, std::uint32_t result) {
    expect(append, back, {screens, c.toc(draw), c.toc(command), 1, 0, 0}, result);
}

void release_case(Cpu& c, std::uint32_t draft) {
    expect(read_history, 0x100980f0, {screens, 2}, draft);
    if (draft) {
        c.put32(draft, 0x12345678);
        expect(release, 0x100980fc, {draft});
        history(0x10098110, 2, 0);
    }
}

void staff_case(Cpu& c, std::uint32_t person, std::uint32_t opened) {
    c.set(3, person);
    opening(c, 0x1009e8b8, -0x7994, -0x7998, opened);
    if (std::uint8_t(opened)) history(0x1009e8d4, 0, person);
}

void news_case(Cpu& c, std::uint32_t item, std::uint32_t revision, std::uint32_t opened) {
    const auto local = stack - 0x140 + 0x38;
    c.set({{3, item}, {4, revision}});
    expect(init_news, 0x10093b0c, {local, 0, 0});
    expect(fetch_news, 0x10093b24, {fixture + 0x100, item, revision, local});
    expect(news_payload, 0x10093b30, {local, 0});
    expect(news_payload, 0x10093b44, {local, 2});
    opening(c, 0x10093b64, -0x56c4, -0x56c8, opened);
    if (!std::uint8_t(opened)) return;
    history(0x10093b80, 0, item);
    history(0x10093b94, 1, revision);
    history(0x10093ba8, 2, item ^ club_payload_mask);
    history(0x10093bbc, 3, revision ^ staff_payload_mask);
}

void details_case(Cpu& c, unsigned link_kind, std::uint32_t opened, std::mt19937& random) {
    const auto link = link_kind ? object : 0;
    const auto offer = link_kind == 2 ? object + 0x80 : 0;
    c.put32(object, offer);
    const std::uint32_t offer_id = random(), revision = random();
    c.put32(object + 0xa8, offer_id);
    c.put32(object + 0xac, revision);
    c.set(3, link);
    opening(c, 0x1009daf8, -0x5708, -0x570c, opened);
    if (!std::uint8_t(opened)) return;

    const auto option = [&](unsigned reg) { return std::uint32_t(std::int8_t(c.r(reg))); };
    history(0x1009db14, 0, link);
    history(0x1009db28, 3, option(4));
    history(0x1009db3c, 4, option(5));
    history(0x1009db50, 5, option(6));
    history(0x1009db64, 6, option(8));
    history(0x1009db78, 7, option(7));
    history(offer ? 0x1009dba0 : 0x1009dbd0, 1, offer ? offer_id : 0xffffffff);
    history(offer ? 0x1009dbb8 : 0x1009dbe4, 2, offer ? revision : 0xffffffff);
}

void candidate_case(Cpu& c, unsigned left_kind, unsigned right_kind,
                    double left_form, double right_form, std::int16_t left_key, std::int16_t right_key) {
    c.set({{3, object}, {4, object + 4}});
    const auto set_candidate = [&](unsigned kind, unsigned index, double form, std::int16_t key) {
        const auto person = staff + index * 0x70;
        const auto profile = staff + 0x200 + index * 0x100;
        c.put32(object + 4 * index, kind ? person : 0);
        c.put32(person + 0x14, kind == 2 ? profile : 0);
        c.put16(profile + 8, std::uint16_t(key));
        if (kind == 2)
            expect(read_form, index ? 0x1009ee8c : 0x1009ee5c, {fixture + 0x200, profile, 6, 1}, {}, form);
    };
    set_candidate(left_kind, 0, left_form, left_key);
    set_candidate(right_kind, 1, right_form, right_key);
    const double default_form = c.f32(c.toc(-0x56f4) + 4);
    if (left_kind != 2) { left_form = default_form; left_key = 0; }
    if (right_kind != 2) { right_form = default_form; right_key = 0; }
    if (left_form < right_form) expected_result = 1;
    else if (left_form > right_form) expected_result = 0xffffffff;
    else if (left_key < right_key) expected_result = 1;
    else if (left_key > right_key) expected_result = 0xffffffff;
    else expected_result = 0;
}

void draw_case(Cpu& c, std::uint32_t row_height, unsigned group) {
    constexpr std::uint32_t instance = fixture + 0x7000, layout = fixture + 0x7100,
                            text = fixture + 0x7200, grid = fixture + 0x7300,
                            local = stack - 0x1a0, name = local + 0x50;
    constexpr unsigned ink = 17, disabled_ink = 23;
    c.put16(screens + 0x2c58, group);
    c.put32(screens + group * 0x2c0 + 4, instance);
    c.put32(screens + group * 0x2c0 + 0x24, layout);
    c.put32(instance + 0x24, layout);
    c.put32(c.r(2) - 0x7bb8, text);
    c.put16(c.toc(-0x7bc8), ink);
    c.put16(c.toc(-0x7bc4), disabled_ink);
    c.put16(c.toc(-0x7b14), 31);
    const auto strings = c.toc(-0x56cc);

    expect(read_history, 0x1009e954, {screens, 0, instance, c.u16(c.r(2) - 0x39f0), ink}, staff);
    expect(0x107136d0, 0x1009e964, {staff, 0, 0});
    expect(0x100c7a50, 0x1009e978, {layout, 1, local + 0x4e, local + 0x4c});
    expect(0x100caeb0, 0x1009e98c, {staff, name, 300, 1});
    expect(0x10744250, 0x1009e9a0, {screens, 13, 7, name});
    expect(0x10727890, 0x1009e9ac, {text, strings + 0x144e});
    expect(0x107442f0, 0x1009e9b8, {screens, text});
    expect(0x1076d470, 0x1009e9c0, {3}, row_height);
    expect(0x10727890, 0x1009e9e0, {text, strings + 0x1461, 100, name});
    expect(0x1074a350, 0x1009ea18, {110, 140, 780, 545, 2, 0, std::uint32_t(std::int8_t(405 / row_height)), 3});
    expected.back().stack_args = {ink, text, 0};
    expect(0x102a1070, 0x1009ea24, {4, 0});
    expect(0x107198c0, 0x1009ea74, {layout, 100, 555, 790, 590, 2, local + 0x48, 1}, grid);
    expected.back().stack_args = {0, 1, 13, 0xffffffff};
    expect(0x10727890, 0x1009ea84, {text, strings + 0x80});
    expect(0x107444e0, 0x1009eac4, {screens, grid, 0, 0, 0x30, disabled_ink, 12, 3});
    expected.back().stack_args = {ink, text, 2, 0};
    expect(0x10727890, 0x1009ead0, {text, strings + 0x83});
    expect(0x107444e0, 0x1009eb10, {screens, grid, 1, 0, 0x30, disabled_ink, 12, 3});
    expected.back().stack_args = {ink, text, 1, 0};
}

void news_draw_case(Cpu& c, unsigned group, unsigned person_id, unsigned club_id) {
    constexpr std::uint32_t clubs = fixture + 0x8000, people = fixture + 0x9000,
                            layout = fixture + 0x7100, grid = fixture + 0x7300,
                            text = fixture + 0xa000, local = stack - 0x17e0;
    constexpr unsigned ink = 17, disabled_ink = 23;
    const auto person = people + person_id * 0x70, club = clubs + club_id * 0x248;
    c.put32(c.toc(-0x7be0), people);
    c.put32(c.toc(-0x7be8), clubs);
    c.put32(c.r(2) - 0x7bb8, text);
    c.put16(c.toc(-0x7bc8), ink);
    c.put16(c.toc(-0x7bc4), disabled_ink);
    c.put16(screens + 0x2c58, group);
    c.put32(screens + group * 0x2c0 + 0x24, layout);
    const auto strings = c.toc(-0x56cc);
    expect(read_history, 0x10093c10, {screens, 2}, club_id);
    expect(read_history, 0x10093c20, {screens, 3}, person_id);
    expect(0x107136d0, 0x10093c50, {person, 0, 0, club_id * 0x248, clubs});
    expect(0x102a1070, 0x10093c5c, {4, 0});
    expect(0x100c7a50, 0x10093c70, {club, 1, local + 0x4a, local + 0x48});
    expect(0x100caeb0, 0x10093c84, {person, text, 2000, 1});
    expect(0x10727890, 0x10093c94, {local + 0xff0, strings, text});
    expect(0x10744250, 0x10093ca8, {screens, 13, 7, local + 0xff0});
    expect(0x10727890, 0x10093cb4, {local + 0x820, strings + 0x1e});
    expect(0x107442f0, 0x10093cc0, {screens, local + 0x820});
    expect(0x10727890, 0x10093cd0, {local + 0x50, strings + 0x2f, text});
    expect(0x1074a350, 0x10093d0c, {110, 145, 780, 535, 2, 0, 16, 4});
    expected.back().stack_args = {ink, local + 0x50, 0};
    expect(0x107198c0, 0x10093d58, {layout, 100, 555, 790, 590, 3, local + 0x4c, 1}, grid);
    expected.back().stack_args = {0, 1, 0, 0xffffffff};
    expect(0x10727890, 0x10093d68, {text, strings + 0x7b});
    expect(0x107444e0, 0x10093da8, {screens, grid, 0, 0, 0x30, disabled_ink, 12, 3});
    expected.back().stack_args = {ink, text, 0x22, 0};
    expect(0x10727890, 0x10093db4, {text, strings + 0x80});
    expect(0x107444e0, 0x10093df4, {screens, grid, 1, 0, 0x30, disabled_ink, 12, 3});
    expected.back().stack_args = {ink, text, 0x21, 0};
    expect(0x10727890, 0x10093e00, {text, strings + 0x83});
    expect(0x107444e0, 0x10093e40, {screens, grid, 2, 0, 0x30, disabled_ink, 12, 3});
    expected.back().stack_args = {ink, text, 0x20, 0};
}

void command_case(Cpu& c, std::uint32_t control, unsigned selected, unsigned fallback,
                  bool coach, bool with_club, std::uint32_t human, std::uint32_t handled,
                  std::uint32_t common, bool replace_club = false) {
    constexpr std::uint32_t layout = fixture + 0x7100, club_address = fixture + 0x7400;
    c.put16(screens + 0x2c58, 0);
    c.put32(screens + 0x24, layout);
    c.put32(layout + 0xc0000 + 2 * 0x17c - 0x5794, selected);
    c.put16(screens + 0x260000 - 0x7e62, fallback);
    c.set(3, control);
    const auto action = std::int16_t(control) == -1 || !selected ? fallback : selected;
    if (action == 1) {
        const auto club = with_club ? club_address : 0;
        c.put32(staff + 0x24, club);
        c.put8(staff + 0x62, coach ? 9 : 0x89);
        c.put32(club_address + 0x90, 0x45);
        const auto manager = [&](int slot, std::uint32_t address) {
            c.put32(c.toc(slot), address);
            return address;
        };
        expect(read_history, 0x1009ebb0, {screens, 0}, staff);
        expect(0x10084a80, 0x1009ebcc, {manager(-0x7b7c, fixture + 0x800), staff});
        if (replace_club) replacement_club = fixture + 0x7800;
        expect(0x100c8930, 0x1009ebe4, {replacement_club.value_or(club), staff, 0, 1, 0});
        expect(0x103e68c0, 0x1009ec04, {manager(-0x7b40, fixture + 0x900), staff, club, 0, 0, 1});
        if (coach) {
            expect(0x1014c2e0, 0x1009ec30, {club, 1, 1, 0, 0}, human);
            if (std::uint8_t(human))
                expect(0x1030f950, 0x1009ec48, {manager(-0x7b1c, fixture + 0xa00), 0x45});
        }
        if (club) {
            expect(0x1014c2e0, 0x1009ec68, {club, 1, 1, 0, 0}, human);
            if (std::uint8_t(human))
                expect(0x103d2b30, 0x1009ec84, {manager(-0x7a7c, fixture + 0xb00), club, 0});
        }
        expected_result = std::uint32_t(-10);
    } else if (action == 2) {
        expected_result = std::uint32_t(-9);
    } else {
        expect(0x102a54b0, 0x1009ece0, {control}, handled);
        if (!handled) expect(0x102a83a0, 0x1009ecf4, {control}, common);
        expected_result = handled ? handled : common;
    }
}

void database_case(Cpu& c, std::uint32_t target, std::uint32_t status, std::mt19937& random) {
    const std::uint32_t options = random(), version = random();
    c.put32(object, 0x434d4442);
    c.set({{3, object}, {4, options}, {5, version}});
    if (target == save_db)
        expect(0x100abfb0, 0x1009ef80, {object, 1, options, version}, status);
    else
        expect(0x100a55a0, target == load_db ? 0x1009efa4 : 0x1009ef44,
               {object, options, target == load_db ? 0u : 1u}, status);
    expected_result = status;
}

constexpr auto results = RoutineChecker::result_r3 | RoutineChecker::result_f1;

// Use the same pointer width, list size and CFM comparator descriptor as the
// contract selection screen. qsort and PLAYER_STATS run their actual bodies.
constexpr unsigned candidate_count = 50;
constexpr std::uint32_t sort_entry = 0x10768300, candidate_slots = fixture + 0x8000;
constexpr auto candidate_address(unsigned n) { return fixture + 0x9000 + n * 0x70; }
constexpr unsigned appearances(unsigned n) { return n / 2; }

void initialize_sort(Cpu& c, unsigned rotation) {
    using namespace cm::aot::player_stats::detail;
    constexpr std::uint32_t players = fixture + 0xb000, seasons = fixture + 0xc000,
                            profiles = fixture + 0xd000;
    const Stats stats{c, fixture + 0x200};
    stats.players() = players;
    stats.seasons() = seasons;
    c.put32(c.toc(slot::stats_players), candidate_count);
    for (unsigned i = 0; i < candidate_count; ++i) {
        const auto id = (i * 7 + rotation) % candidate_count;
        const auto profile = profiles + id * 0x10;
        c.put32(candidate_slots + i * 4, candidate_address(id));
        c.put32(candidate_address(id) + 0x14, profile);
        c.put32(profile, id);
        c.put16(profile + 8, id);
        const PlayerEntry player{c, players + id * PlayerEntry::size};
        player.season(6) = std::int32_t(id);
        const SeasonRecord record{c, seasons + id * SeasonRecord::size};
        record.appearances() = std::uint8_t(appearances(id));
    }
    c.set({{3, candidate_slots}, {4, candidate_count}, {5, 4}, {6, c.toc(-0x7988)}});
}

void sort(State& s, bool use_reference) {
    sorting = true;
    reference_mode = use_reference;
    callbacks = 0;
    s.pc = sort_entry;
    run_until(s, sentinel);
    sorting = reference_mode = false;
    Cpu c(s);
    if (s.pc != sentinel || c.sp() != stack || !callbacks)
        throw std::runtime_error("guest qsort did not return through the contract comparator");
    for (unsigned i = 0; i < candidate_count; ++i)
        if (c.u32(candidate_slots + 4 * i) != candidate_address(candidate_count - i - 1))
            throw std::runtime_error("guest qsort returned an incorrect candidate order");
}

void sort_original(State& s, std::uint32_t) { sort(s, true); }
void sort_production(State& s, std::uint32_t) { sort(s, false); }
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        auto state = std::make_unique<State>();
        load_program(*state, argv[1]);
        state->memory.map(fixture, 0xe000);
        state->memory.map(trace, trace_size);
        state->memory.map(fixture + 0xc0000, 0x4000);
        state->memory.map(fixture + 0x250000, 0x10000);
        state->memory.map(stack - 0x10000, 0x20000);
        dispatch_probe = service;
        constexpr unsigned seed = 0x5343524e;
        std::mt19937 random(seed);
        std::set<std::uint32_t> compared;
        unsigned passed = 0;
        const auto check = [&] {
            for (const bool production : {false, true}) {
                production_mode = production;
                const auto differences = compare_routine_call(*state, entry, original, rewrite, results, false, true);
                if (!differences.empty()) {
                    for (const auto& difference : differences)
                        std::cerr << hex_address(entry) << " case " << passed << ": " << difference << '\n';
                    throw std::runtime_error("contract screen equivalence failed");
                }
                ++passed;
            }
            compared.insert(entry);
        };
        for (unsigned mutation = 0; mutation < 16; ++mutation) {
            auto c = initialize(*state, random, release_draft);
            release_case(c, 0);
            check();
            auto draft_cpu = initialize(*state, random, release_draft);
            release_case(draft_cpu, object + (random() & 15) * 0x50);
            check();
            for (const auto opened : {0u, 0x100u, 1u, 0xffu}) {
                auto staff_cpu = initialize(*state, random, open_staff);
                staff_case(staff_cpu, staff + (random() & 15) * 0x70, opened);
                check();
                auto news_cpu = initialize(*state, random, open_news);
                news_case(news_cpu, random(), random(), opened);
                check();
                for (unsigned link_kind = 0; link_kind <= 2; ++link_kind) {
                    auto details_cpu = initialize(*state, random, open_details);
                    details_case(details_cpu, link_kind, opened, random);
                    check();
                }
            }
            for (const auto target : {load_language, save_db, load_db})
                for (const auto status : {0u, 1u, 0xffu}) {
                    auto database_cpu = initialize(*state, random, target);
                    database_case(database_cpu, target, status, random);
                    check();
                }
            auto news_cpu = initialize(*state, random, draw_news);
            news_draw_case(news_cpu, mutation & 3, (mutation * 5) % 16, (mutation * 7) % 8);
            check();
            for (const auto height : {3u, 11u, 405u}) {
                auto draw_cpu = initialize(*state, random, draw_staff);
                draw_case(draw_cpu, height, mutation & 3);
                check();
            }
            for (const auto human : {0u, 1u, 0x100u, 0xffu})
                for (const bool coach : {false, true}) {
                    auto command_cpu = initialize(*state, random, staff_command);
                    command_case(command_cpu, 2, 1, 2, coach, true, human, 0, 0, mutation % 2 != 0);
                    check();
                }
            for (const auto control : {2u, 0x1234ffffu})
                for (const auto fallback : {0u, 1u, 2u})
                    for (const auto common : {0u, 0x80000000u, 0xfffffff6u}) {
                        auto command_cpu = initialize(*state, random, staff_command);
                        command_case(command_cpu, control, 0, fallback, false, fallback != 1, 0, 0, common);
                        check();
                    }
            auto command_cpu = initialize(*state, random, staff_command);
            command_case(command_cpu, 2, 3, 0, false, true, 0, 0xfffffffe, 0);
            check();
        }
        constexpr double nan = std::numeric_limits<double>::quiet_NaN();
        constexpr double infinity = std::numeric_limits<double>::infinity();
        for (unsigned left = 0; left <= 2; ++left)
            for (unsigned right = 0; right <= 2; ++right)
                for (const double left_form : {0., 8., -infinity, infinity, nan})
                    for (const double right_form : {0., 8., nan})
                        for (const auto key : {-32768, 0, 32767}) {
                            auto c = initialize(*state, random, compare_staff);
                            candidate_case(c, left, right, left_form, right_form, std::int16_t(key), 0);
                            check();
                        }

        const std::array negotiation_inputs{
            NegotiationInput{.opened = 0x100},
            NegotiationInput{.final_club = -1},
            NegotiationInput{.profile = true, .budget = -10, .first_club = 0, .second_club = 0},
            NegotiationInput{.budget = 6'000'000, .current_wages = 4000, .first_club = 0},
            NegotiationInput{.profile = true, .current_wages = 5000, .first_club = 0},
            NegotiationInput{.first_club = 0, .second_club = -1},
            NegotiationInput{.first_club = 1, .second_club = 1},
            NegotiationInput{.profile = true, .kind = 11},
            NegotiationInput{.kind = 12},
            NegotiationInput{.kind = 13},
            NegotiationInput{.kind = 15},
            NegotiationInput{.existing = true, .opened = 0x100},
            NegotiationInput{.existing = true, .final_club = -1},
            NegotiationInput{.existing = true, .profile = true, .first_club = 0},
            NegotiationInput{.existing = true, .profile = true, .permit = true, .change_group = true},
            NegotiationInput{.existing = true, .profile = true, .link = true, .first_club = 1, .second_club = 1},
            NegotiationInput{.existing = true, .link = true, .exchange = true, .budget = 6'000'000,
                             .first_club = 0, .second_club = -1, .final_club = -1},
            NegotiationInput{.existing = true, .permit = true, .budget = -1, .current_wages = 100, .first_club = 0},
            NegotiationInput{.existing = true, .link = true, .clear_link = true},
            NegotiationInput{.existing = true, .profile = true, .link = true, .kind = 11},
            NegotiationInput{.existing = true, .kind = 12},
            NegotiationInput{.existing = true, .kind = 13},
            NegotiationInput{.existing = true, .kind = 15},
        };
        for (unsigned mutation = 0; mutation < 4; ++mutation)
            for (const auto& input : negotiation_inputs) {
                auto c = initialize(*state, random, input.existing ? existing_negotiation : new_negotiation);
                const std::uint32_t negotiation_state = random(), mode = random();
                negotiation_case(c, input, negotiation_state, mode);
                check();
            }
        using Control = CategoryControl;
        constexpr std::array no_role{Control::unavailable_role, Control::unavailable_role,
            Control::unavailable_role, Control::unavailable_role, Control::unavailable_role, Control::unavailable_role};
        constexpr std::array pending_rows{Control::pending_other, Control::pending_selected,
            Control::pending_other, Control::pending_other, Control::pending_other, Control::pending_other};
        constexpr std::array pending_options{Control::selectable, Control::selected,
            Control::selectable, Control::selectable, Control::selectable, Control::selectable};
        constexpr std::array declined_by_age{Control::selectable, Control::selectable,
            Control::selectable, Control::selectable, Control::rejected, Control::rejected};
        constexpr std::array declined_by_rating{Control::selectable, Control::selectable,
            Control::selectable, Control::rejected, Control::rejected, Control::rejected};
        constexpr unsigned multiple_roles = (1 << 10) | (1 << 12) | (1 << 15);
        const std::array draw_inputs{
            ChoiceDrawInput{},
            ChoiceDrawInput{.profile = false, .category = 0},
            ChoiceDrawInput{.profile = false, .secondary = true, .roles = multiple_roles, .category = -1, .height = 1},
            ChoiceDrawInput{.secondary = true, .roles = multiple_roles, .group = 2, .height = 3},
            ChoiceDrawInput{.player = false, .roles = 1 << 4, .category = 0, .role = 4, .age = 31, .controls = no_role},
            ChoiceDrawInput{.pending = true, .roles = multiple_roles, .group = 3, .category = 2, .proposal = 2,
                            .controls = pending_rows, .warning = 0xe7},
            ChoiceDrawInput{.category = 2, .proposal = 2, .controls = pending_options, .warning = 0x129},
            ChoiceDrawInput{.pending = true, .category = 7, .proposal = 7, .controls = {
                Control::pending_other, Control::pending_other, Control::pending_other,
                Control::pending_other, Control::pending_other, Control::pending_other}},
            ChoiceDrawInput{.offer = true, .same_club = true, .category = 0, .age = 24,
                            .controls = declined_by_age, .warning = 0x142},
            ChoiceDrawInput{.offer = true, .category = 0, .age = 24, .rating = 33001,
                            .controls = declined_by_rating, .warning = 0x174},
            ChoiceDrawInput{.category = 0, .age = 24, .controls = declined_by_age, .warning = 0x142},
        };
        for (unsigned mutation = 0; mutation < 4; ++mutation)
            for (const auto& input : draw_inputs) {
                auto c = initialize(*state, random, choice_draw);
                choice_draw_case(c, input);
                check();
            }
        const std::array offer_inputs{
            OfferChoiceInput{.path = OfferPath::missing_instance},
            OfferChoiceInput{.path = OfferPath::missing_club},
            OfferChoiceInput{.path = OfferPath::pending, .same_day = true, .available = 0, .pending_category = 3},
            OfferChoiceInput{.path = OfferPath::pending, .link = true, .same_day = true,
                             .available = 0, .pending_category = 2},
            OfferChoiceInput{.path = OfferPath::sole_role, .available = 1},
            OfferChoiceInput{.path = OfferPath::sole_role, .with_offer = true, .available = 1},
            OfferChoiceInput{.path = OfferPath::reuse, .restricted = false, .available = 0, .club_category = 3},
            OfferChoiceInput{.path = OfferPath::reuse, .link = true, .restricted = false, .available = 0,
                             .club_category = 3, .primary_kind = 4},
            OfferChoiceInput{.path = OfferPath::reuse, .link = true, .restricted = false, .same_day = true, .same_year = false,
                             .available = 0, .pending_category = 2, .club_category = 3, .primary_kind = 4},
            OfferChoiceInput{.opened = 0x100},
            OfferChoiceInput{},
            OfferChoiceInput{.link = true, .blank_offer = true, .same_day = true, .pending_category = 5},
            OfferChoiceInput{.link = true, .same_day = true, .pending_category = 5},
            OfferChoiceInput{.restricted = false, .available = 0, .club_category = 3, .primary_kind = 5},
            OfferChoiceInput{.restricted = false, .available = 0, .club_category = 3, .club_kind = 6, .primary_kind = 4},
            OfferChoiceInput{.restricted = false, .available = 0, .club_category = 0},
            OfferChoiceInput{.restricted = false, .available = 0},
            OfferChoiceInput{.restricted = false, .profile = true, .available = 0, .club_category = 7},
            OfferChoiceInput{.restricted = false, .profile = true, .available = 0, .club_category = 8},
            OfferChoiceInput{.profile = true, .club_category = 4},
            OfferChoiceInput{.profile = true, .permitted_categories = 1 << 4, .club_category = 4, .preferred_category = 4},
            OfferChoiceInput{.profile = true, .terms_apply = true, .permitted_categories = 0x7e, .club_category = 3},
            OfferChoiceInput{.profile = true, .permitted_categories = 0x7e, .rated_categories = 1 << 4,
                             .preferred_category = 6},
            OfferChoiceInput{.profile = true, .age = 19, .reputation = 3000, .club_reputation = 5000},
            OfferChoiceInput{.profile = true, .age = 20, .club_reputation = 4250},
            OfferChoiceInput{.profile = true, .age = 21, .club_reputation = 3900},
            OfferChoiceInput{.profile = true, .age = 24, .club_reputation = 6000},
            OfferChoiceInput{.available = 1, .preferred = 11},
            OfferChoiceInput{.available = 0, .pending_category = 2},
            OfferChoiceInput{.available = 0, .pending_category = 255},
            OfferChoiceInput{.profile = true, .permitted_categories = 0x7e, .age = 21,
                             .club_reputation = 4150, .preferred_category = 1,
                             .reputation_factor = 7, .interest_factor = -5, .ambition_factor = 11},
        };
        for (unsigned mutation = 0; mutation < 4; ++mutation)
            for (const auto& input : offer_inputs) {
                auto c = initialize(*state, random, offer_choice);
                offer_choice_case(c, input);
                check();
            }
        for (const auto& [target, input] : {
                std::pair{restricted_news, OfferChoiceInput{.path = OfferPath::pending, .same_day = true, .available = 0, .pending_category = 3}},
                std::pair{offer_news, OfferChoiceInput{.restricted = false}},
                std::pair{offer_news, OfferChoiceInput{.path = OfferPath::sole_role, .link = true, .restricted = false,
                                                       .available = 1}},
                std::pair{offer_news, OfferChoiceInput{.path = OfferPath::sole_role, .with_offer = true, .restricted = false,
                                                       .available = 1}}}) {
            auto c = initialize(*state, random, target);
            offer_choice_case(c, input, target);
            check();
        }
        const std::array choice_inputs{
            ChoiceInput{.action = 3},
            ChoiceInput{.action = 3, .control_default = true},
            ChoiceInput{.action = 0x2f, .fallback = true},
            ChoiceInput{.action = 0x2f, .control_default = true},
            ChoiceInput{.action = 1},
            ChoiceInput{.action = 7, .handled = 0xfffffff6},
            ChoiceInput{.action = 7, .fallback = true, .common = 1},
            ChoiceInput{.action = 7, .control_default = true, .common = 0x80000000},
            ChoiceInput{.action = 0},
            ChoiceInput{},
            ChoiceInput{.fallback = true, .term = 0x80, .kind = 11},
            ChoiceInput{.control_default = true, .existing = true, .term = 0xfc},
            ChoiceInput{.existing = true, .linked = true, .term = 0x7f},
            ChoiceInput{.existing = true, .linked = true, .opened = 0x100},
            ChoiceInput{.existing = true, .linked = true, .stale = true},
            ChoiceInput{.existing = true, .linked = true, .status = 3},
            ChoiceInput{.existing = true, .linked = true, .status = 2},
        };
        for (unsigned mutation = 0; mutation < 4; ++mutation)
            for (const auto& input : choice_inputs) {
                auto c = initialize(*state, random, negotiation_choice);
                negotiation_choice_case(c, input);
                check();
            }

        for (const auto& [target, from] : {std::pair{load_language, 0x10346df0u},
                 std::pair{load_db, 0x103353b0u}, std::pair{save_db, 0x1034bc98u}}) {
            auto caller_cpu = initialize(*state, random, target);
            database_case(caller_cpu, target, 1, random);
            call_site = from;
            check();
        }

        for (const bool existing : {false, true}) {
            auto excluded = initialize(*state, random, negotiation_choice);
            // With no primary contract, this routing fixture only needs services
            // that respect the per-entry checker selection.
            negotiation_choice_case(excluded, {.existing = existing, .final_club = -1});
            const auto differences = compare_routine_call(*state, entry, original, rewrite_with_original_children, results);
            if (!differences.empty()) {
                for (const auto& difference : differences) std::cerr << difference << '\n';
                throw std::runtime_error("excluded negotiation child differed from the original");
            }
            ++passed;

            auto selected = initialize(*state, random, negotiation_choice);
            negotiation_choice_case(selected, {.existing = existing});
            RoutineChecker selection([](std::uint32_t) { return true; }, 0, 0);
            routine_checker = &selection;
            rewrite(*state, entry);
            routine_checker = nullptr;
            const auto callee = existing ? existing_negotiation : new_negotiation;
            const auto seen = selection.totals().find(callee);
            if (seen == selection.totals().end() || seen->second.calls != 1)
                throw std::runtime_error("checker parent did not observe the selected negotiation child");
        }

        for (const auto target : {restricted_news, offer_news}) {
            auto excluded = initialize(*state, random, target);
            offer_choice_case(excluded, {.restricted = target == restricted_news, .opened = 0x100}, target);
            const auto differences = compare_routine_call(*state, target, original, rewrite_with_original_children, results);
            if (!differences.empty()) {
                for (const auto& difference : differences) std::cerr << difference << '\n';
                throw std::runtime_error("excluded offer choice differed from the original");
            }
            ++passed;

            auto selected = initialize(*state, random, target);
            const bool linked = target == offer_news;
            offer_choice_case(selected, {.path = OfferPath::pending, .link = linked, .restricted = !linked,
                                         .same_day = true, .available = 0, .pending_category = 3}, target);
            RoutineChecker selection([](std::uint32_t) { return true; }, 0, 0);
            routine_checker = &selection;
            rewrite(*state, target);
            routine_checker = nullptr;
            for (const auto child : {offer_choice, linked ? existing_negotiation : new_negotiation}) {
                const auto seen = selection.totals().find(child);
                if (seen == selection.totals().end() || seen->second.calls != 1)
                    throw std::runtime_error("checker parent did not observe the selected news and negotiation chain");
            }
        }

        auto c = initialize(*state, random, open_staff);
        staff_case(c, staff, 1);
        const auto trace_control = compare_routine_call(*state, entry, original, corrupt_trace, results);
        const auto result_control = compare_routine_call(*state, entry, original, corrupt_result, results);
        if (trace_control.size() != 1 || !trace_control.front().starts_with("byte ") ||
            result_control.size() != 1 || !result_control.front().starts_with("r3 "))
            throw std::runtime_error("contract screen comparison missed a deliberate corruption");
        for (const auto rotation : {0u, 17u, 43u}) {
            auto sort_cpu = initialize(*state, random, sort_entry);
            initialize_sort(sort_cpu, rotation);
            const auto differences = compare_routine_call(*state, sort_entry, sort_original, sort_production, results);
            if (!differences.empty()) {
                for (const auto& difference : differences) std::cerr << difference << '\n';
                throw std::runtime_error("contract candidate sort replay differed");
            }
        }
        std::cout << passed << " completed comparisons over " << compared.size() << " entries, seed " << seed
                  << ", result and argument-trace controls detected, three guest qsort pairs matched\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

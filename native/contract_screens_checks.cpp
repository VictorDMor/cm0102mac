// Controlled comparisons against independent original pages, with all owned
// entries enabled together on the production side.
#include "contract_screens_adapter.hpp"
#include "guest_abi.hpp"
#include "player_stats_detail.hpp"
#include "routine_check.hpp"
#include <algorithm>
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
                        open_staff = 0x1009e880, draw_staff = 0x1009e8f0, staff_command = 0x1009eb30,
                        compare_staff = 0x1009ee00, load_language = 0x1009ef30,
                        save_db = 0x1009ef60, load_db = 0x1009ef90;
constexpr std::uint32_t append = 0x1073e4b0, read_history = 0x1073ef70, write_history = 0x1073f1e0,
                        release = 0x107735a0, init_news = 0x102bdf90, fetch_news = 0x102ccdd0,
                        news_payload = 0x102cb570, read_form = 0x10303560;
constexpr std::uint32_t fixture = 0x60000000, trace = fixture + 0x1000, screens = fixture + 0x2000,
                        staff = fixture + 0x3000, object = fixture + 0x4000, stack = 0x71010000,
                        sentinel = 0xfffffff0;
constexpr std::uint32_t club_payload_mask = 0x00013579, staff_payload_mask = 0x0002468a;
constexpr unsigned trace_words = 3 + 8 + 19 + 8 + 2 + 4;

struct Call {
    std::uint32_t target, back;
    std::vector<std::uint32_t> args, stack_args;
    std::optional<std::uint32_t> result;
    std::optional<double> form;
};
std::vector<Call> expected;
unsigned calls = 0;
bool reference_mode = false, production_mode = false, sorting = false;
unsigned callbacks = 0;
std::optional<std::uint32_t> expected_result;
std::optional<std::uint32_t> replacement_club;
std::uint32_t entry = 0;
std::uint32_t call_site = 0;

void expect(std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> args,
            std::optional<std::uint32_t> result = {}, std::optional<double> form = {}) {
    expected.push_back({target, back, args, {}, result, form});
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
    if (reference_mode && contract_screens_reference(s)) return true;
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

    auto at = trace + calls++ * trace_words * 4;
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
    case init_news:
        c.put32(c.r(3), 0);
        c.put32(c.r(3) + 8, 0);
        break;
    case fetch_news:
        c.put32(c.r(6), c.r(4) ^ club_payload_mask);
        c.put32(c.r(6) + 8, c.r(5) ^ staff_payload_mask);
        break;
    case news_payload:
        c.set(3, c.u32(c.r(3) + 4 * c.r(4)));
        break;
    case write_history:
        c.put32(screens + c.r(4) * 4, c.r(5));
        break;
    case release:
        c.put32(c.r(3), 0);
        break;
    case read_form:
        if (c.f(1) != c.f32(c.toc(-0x56f4) + 8))
            throw std::runtime_error("wrong default form argument");
        c.set_f(1, call.form.value());
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
        s.budget = s.executed + (call_site ? 1 : 0);
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

Cpu initialize(State& s, std::mt19937& random, std::uint32_t target) {
    expected.clear();
    expected_result.reset();
    replacement_club.reset();
    entry = target;
    call_site = 0;
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
    for (unsigned i = 0; i < 8; ++i) c.put32(screens + 4 * i, random());
    s.budget = s.executed + 100000;
    return c;
}

void history(std::uint32_t back, unsigned slot, std::uint32_t value) {
    expect(write_history, back, {screens, slot, value, 0}, screens + 4 * slot);
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

        for (const auto& [target, from] : {std::pair{load_language, 0x10346df0u},
                 std::pair{load_db, 0x103353b0u}, std::pair{save_db, 0x1034bc98u}}) {
            auto caller_cpu = initialize(*state, random, target);
            database_case(caller_cpu, target, 1, random);
            call_site = from;
            check();
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

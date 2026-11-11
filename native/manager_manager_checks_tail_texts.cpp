// MANAGER_MANAGER.CPP checker cases for the tail block's three news texts (manager_manager_tail.hpp): 0x101c12a0 (message
// 0x178a), 0x101c1c20 (0x178d) and 0x101c3b10 (the assessment message 0x178f).
//
// The message record is a scratch array of words (0x102cb570 reads the word at +8+4n); the person, the clubs, the
// competition and the row are real ones of the career, and the formatters and name routines run for real, so the text in
// the shared buffer and the output object are compared. The two routines that hand the text to the output object
// (0x1074b830, 0x1074bab0) are answered in one family of cases and run for real over a scratch object in the other:
// 0x1074bab0's row holds the stale register save area of the callees below it (the original's registers, set by the
// rewrite before its calls).
#include "manager_manager_checks_tail.hpp"
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
using tail::Rng;
constexpr std::uint32_t move_text = 0x101c12a0, rival_text = 0x101c1c20, assessment_text = 0x101c3b10;
constexpr std::uint32_t to_object = 0x1074b830, name_to_object = 0x1074bab0, person_shown = 0x100cbde0;
constexpr std::uint32_t record = scratch_base + 0x9000, out_object = scratch_base + 0xa000;

struct Text {
    std::uint32_t entry;
    std::uint32_t flag = 1;
    std::uint32_t f[11] = {};
    bool answered = true;
    int shown = -1;  // answer of 0x100cbde0: -1 runs it for real
};

Case make(const Text& t, std::string label) {
    Case item{.entry = t.entry, .registers = {{4, record}, {5, t.flag}, {6, out_object}}, .label = "tail text " + std::move(label)};
    for (unsigned i = 0; i < 11; ++i) item.memory.push_back({4, record + 8 + 4 * i, t.f[i]});
    if (t.answered) item.answers = {{to_object, 0}, {name_to_object, 0}};
    if (t.shown >= 0) item.answers.push_back({person_shown, std::uint32_t(t.shown)});
    return item;
}

void moves(const Scenario& sc, std::vector<Case>& cases, Rng& r) {
    unsigned n = 0;
    const auto label = [&](const char* what) { return std::string(what) + " " + std::to_string(n++); };
    (void)sc;
    for (const std::uint32_t flag : {1u, 0u, 0x100u, 0xffu, 0x80u, 0x101u})
        for (const std::uint32_t has_club : {0u, 1u, 0x100u, 0x80u, 0xffu})
            for (const std::uint32_t six : {0xffffffffu, 1u, 0u, 2u, 5u, 0x80000000u, 0x7fffffffu})
                for (const std::uint32_t seven : {0xffffffffu, 1u, 0u, 2u, 7u}) {
                    Text t{.entry = move_text, .flag = flag, .answered = r.chance(40), .shown = int(r.below(3)) - 1};
                    t.f[0] = r.below(40);
                    t.f[1] = r.below(30); t.f[2] = r.below(30); t.f[3] = r.below(30);
                    t.f[4] = has_club; t.f[5] = r.below(6); t.f[6] = six; t.f[7] = seven; t.f[8] = r.next();
                    cases.push_back(make(t, label("move")));
                }
}

void rivals(const Scenario& sc, std::vector<Case>& cases, Rng& r) {
    unsigned n = 0;
    (void)sc;
    for (unsigned i = 0; i < 400; ++i) {
        Text t{.entry = rival_text, .flag = r.pick<std::uint32_t>({1u, 0u, 0x100u, 0xffu, 0x80u, 1u}), .answered = r.chance(40),
               .shown = int(r.below(3)) - 1};
        t.f[0] = r.below(40);
        t.f[1] = r.below(30); t.f[2] = r.below(30); t.f[3] = r.below(30);
        t.f[4] = r.below(8); t.f[5] = r.below(8); t.f[6] = r.below(4); t.f[7] = r.next();
        cases.push_back(make(t, "rival " + std::to_string(n++)));
    }
}

void assessments(const Scenario& sc, std::vector<Case>& cases, Rng& r) {
    unsigned n = 0;
    (void)sc;
    for (const std::uint32_t flag : {1u, 0u, 0x100u, 0xffu})
        for (const std::uint32_t kind : {1u, 0u, 2u, 3u, 0x101u, 0xffffffffu})
            for (const std::uint32_t stage : {0u, 1u, 2u, 3u, 4u, 5u, 0xffffffffu, 0x102u})
                for (const std::uint32_t sector : {0u, 1u, 2u, 3u, 4u, 0xffffffffu}) {
                    Text t{.entry = assessment_text, .flag = flag, .answered = r.chance(40), .shown = int(r.below(3)) - 1};
                    t.f[0] = r.below(40);
                    t.f[1] = r.below(30); t.f[2] = r.below(30); t.f[3] = r.below(30);
                    t.f[4] = r.below(8); t.f[5] = sector; t.f[6] = kind; t.f[7] = stage; t.f[8] = r.next();
                    t.f[9] = r.pick<std::uint32_t>({0u, 1u, 2u}); t.f[10] = r.pick<std::uint32_t>({0u, 1u, 0x100u});
                    cases.push_back(make(t, "assessment " + std::to_string(n++)));
                }
}
}

MANAGER_MANAGER_CASES("tail_texts") {
    if (scenario.match) return;
    Rng r{0x7ec7};
    moves(scenario, cases, r);
    rivals(scenario, cases, r);
    assessments(scenario, cases, r);
}
}

#pragma once
// Shared pieces of the people block's fixtures (database_checks_people*.cpp): scripted answers for the
// random generator and for callees whose state a fixture does not build, a deterministic mixer for the
// cases' choices, and `make`, which turns a case into a Fixture.
#include "database_checks.hpp"
#include "guest_abi.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot::database_checks::people_checks {
using namespace cm::aot::guest;
constexpr std::uint32_t random_entry = 0x1077b770, random_state_slot = 0x34c4;
// Counters in guest memory (the fixture runs several times from the same state): the draws taken and each
// scripted callee's calls taken.
constexpr std::uint32_t draw_counter = data + 0x1f000, answer_counters = data + 0x1f100;

// Appends the call's target, LR, the first `arguments` argument registers, r13-r31 and CR to the trace, so
// the compared memory holds exactly what the stubbed callee could see.
inline void record_arguments(Cpu& c, unsigned arguments) {
    constexpr std::uint32_t kept_words = 5000, digest_at = data + 0x1ffc0;
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    // Past the kept calls (the trace area is limited) the values fold into one word, still compared.
    const bool folding = size >= kept_words;
    auto digest = c.u32(digest_at);
    const auto add = [&](std::uint32_t value) {
        if (folding) {
            digest = std::rotl(digest, 5) ^ value;
        } else {
            c.put32(at, value);
            at += 4;
        }
    };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 3; r < 3 + arguments; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned f = 0; f < 8; ++f) add(c.cr(f));
    if (folding) c.put32(digest_at, digest);
    else c.put32(trace_at, (at - trace_at - 4) / 4);
}

// The draws a case scripts (empty: the real generator, seeded). The last answer repeats.
struct Draws {
    std::vector<std::uint32_t> values;
    std::uint32_t seed = 0;
};
inline bool scripted_draw(State& s, const Draws& d) {
    if (s.pc != random_entry || d.values.empty()) return false;
    Cpu c(s);
    const auto index = c.u32(draw_counter);
    c.put32(draw_counter, index + 1);
    record_arguments(c, 0);
    c.set(3, d.values[std::min<std::size_t>(index, d.values.size() - 1)]);
    s.pc = c.lr();
    return true;
}
inline void seed_generator(Cpu& c, const Draws& d) { c.put32(c.r(2) - random_state_slot, d.seed); }
inline std::string text(const Draws& d) {
    std::string out = d.values.empty() ? "real seed " + std::to_string(d.seed) : "draws";
    for (const auto v : d.values) out += ' ' + std::to_string(v);
    return out;
}

// A callee answered from a list (r3), in order, the last answer repeating; its r3-r(2+arguments), preserved
// registers and CR are recorded.
struct Answer {
    std::uint32_t target;
    std::vector<std::uint32_t> values;
    unsigned arguments = 1;
};
inline bool scripted_answer(State& s, const std::vector<Answer>& answers) {
    for (unsigned k = 0; k < answers.size(); ++k) {
        const auto& a = answers[k];
        if (s.pc != a.target) continue;
        Cpu c(s);
        const auto counter = answer_counters + 4 * k;
        const auto index = c.u32(counter);
        c.put32(counter, index + 1);
        record_arguments(c, a.arguments);
        c.set(3, a.values[std::min<std::size_t>(index, a.values.size() - 1)]);
        s.pc = c.lr();
        return true;
    }
    return false;
}

// A small deterministic generator for the fixtures' choices.
struct Mix {
    std::uint32_t state;
    std::uint32_t next() { state = state * 1664525u + 1013904223u; return state >> 8; }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    template <class T> T pick(std::initializer_list<T> list) { return *(list.begin() + below(std::uint32_t(list.size()))); }
};

// A fixture from a case: the generator's seed and counters, the case's world, and the stubs.
inline Fixture make(std::uint32_t entry, std::string description, const Draws& d, std::function<void(Cpu&)> setup,
                    bool overflow = false, std::vector<Answer> answers = {}) {
    Fixture f;
    f.entry = entry;
    f.description = std::move(description) + ' ' + text(d);
    f.setup = [d, setup = std::move(setup)](Cpu& c) {
        c.put32(draw_counter, 0);
        c.put32(data + 0x1ffc0, 0);
        for (unsigned k = 0; k < 16; ++k) c.put32(answer_counters + 4 * k, 0);
        seed_generator(c, d);
        setup(c);
    };
    f.stub = [d, answers = std::move(answers)](State& s) {
        if (std::getenv("PEOPLE_TRACE")) std::fprintf(stderr, "dispatch %08x r3=%08x\n", unsigned(s.pc), unsigned(s.r<4>(12)));
        return scripted_draw(s, d) || scripted_answer(s, answers);
    };
    f.summary_overflow = overflow;
    return f;
}

// The draws of a case: `count` scripted values from the interesting ones, or a real seed (one case in four).
inline Draws script(Mix& mix, unsigned count) {
    Draws d;
    constexpr std::array<std::uint32_t, 8> edges{0, 1, 2, 16383, 16384, 32766, 32767, 10000};
    if (mix.below(4) == 0) {
        d.seed = mix.next() * 2654435761u;
        return d;
    }
    for (unsigned i = 0; i < count; ++i) d.values.push_back(mix.below(3) == 0 ? edges[mix.below(8)] : mix.below(32768));
    return d;
}
}

// MANAGER_MANAGER.CPP checker cases for the vacancy block's news filers (manager_manager_vacancy.hpp): 0x101bec80,
// 0x101bef90 and 0x101bf3a0. Scratch records carry each call; the news desk is answered (and run for real in some
// cases); the message buffer's routines run for real so each field is compared at its call.
#include "manager_manager_checks_tail.hpp"
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
using tail::Rng;
constexpr std::uint32_t kVacancy = 0x101bec80, kAnswer = 0x101bef90, kOffer = 0x101bf3a0, desk = 0x102cc140, desk_c = 0x102cc400;
constexpr std::uint32_t move = scratch_base + 0x1000, club_a = scratch_base + 0x1100, club_b = scratch_base + 0x1200,
                        person = scratch_base + 0x1300, region = scratch_base + 0x1400, owner = scratch_base + 0x1500,
                        word = scratch_base + 0x1600, copy = scratch_base + 0x1700, rec = scratch_base + 0x1800;

void add_vacancy(const Scenario& sc, std::vector<Case>& cases) {
    Rng r{0xbec80};
    for (unsigned i = 0; i < 200; ++i) {
        Case item{.entry = kVacancy, .label = "vacancy news " + std::to_string(i)};
        item.registers = {{3, 0}, {4, i % 29 == 1 ? 0 : move}, {5, r.next()}, {6, r.next()}};
        auto put = [&](unsigned size, std::uint32_t at, std::uint32_t v) { item.memory.push_back({size, at, v}); };
        put(4, move + 0x1c, club_a); put(4, move + 0x20, club_b); put(4, move + 0x14, person);
        put(4, club_a, r.next()); put(4, club_b, r.next()); put(4, person, r.next()); put(4, person + 8, owner);
        put(4, owner + 0x68, r.next());
        // The date copy (the global at TOC -0x7a9c points at it) against today's.
        const auto today = sc.toc(-0x7b80);
        const auto cell = sc.toc(-0x7a9c) - 0; (void)cell;
        constexpr std::int32_t day = 300, year = 2001;
        put(2, today, day); put(2, today + 2, year);
        const auto yc = r.below(3);
        const std::uint32_t copy_year = yc == 0 ? year - 1 : yc == 1 ? year + 1 : year;
        put(2, sc.toc(-0x7a9c) + 2, copy_year);
        put(2, sc.toc(-0x7a9c), r.pick<std::int32_t>({day, day - 1, day + 1, -1, 0, 0x7fff, -0x8000, 0x4000, -0x4000, day ^ 1, 1}) & 0xffff);
        if (i % 25) item.answers = {{desk, 0}};
        cases.push_back(std::move(item));
    }
    (void)copy;
}
void add_answer(const Scenario&, std::vector<Case>& cases) {
    Rng r{0xbef90};
    for (unsigned i = 0; i < 100; ++i) {
        Case item{.entry = kAnswer, .label = "vacancy answer " + std::to_string(i)};
        item.registers = {{3, 0}, {4, i % 17 == 1 ? 0 : person}, {5, r.next()}, {6, r.next()}, {7, r.next()}};
        auto put = [&](unsigned size, std::uint32_t at, std::uint32_t v) { item.memory.push_back({size, at, v}); };
        put(4, person, r.next()); put(4, person + 0x2c, i % 13 == 2 ? 0 : region); put(4, region + 0x68, r.next());
        if (i % 20) item.answers = {{desk, 0}};
        cases.push_back(std::move(item));
    }
}
void add_offer(const Scenario&, std::vector<Case>& cases) {
    Rng r{0xbf3a0};
    for (unsigned i = 0; i < 100; ++i) {
        Case item{.entry = kOffer, .label = "vacancy offer " + std::to_string(i)};
        item.registers = {{3, 0}, {4, word}, {5, i % 17 == 1 ? 0 : rec}, {6, r.next()}, {7, r.next()}};
        item.memory = {{4, word, r.next()}, {4, rec + 0x68, r.next()}};
        if (i % 20) item.answers = {{desk_c, 0}};
        cases.push_back(std::move(item));
    }
}
}

MANAGER_MANAGER_CASES("vacancy_news") {
    if (scenario.match) return;
    add_vacancy(scenario, cases);
    add_answer(scenario, cases);
    add_offer(scenario, cases);
}
}

// MANAGER_MANAGER.CPP checker cases for 0x101c0230 (manager_manager_tail.hpp), the club's weighing of a manager's
// application: the simulation routine of the block (14 draws, floating point, a record appended to the queue).
//
// A scene of scratch records carries each call: the list object with the club's row, the person with its profile, the
// club with its region record (the league id), its manager and the manager's profile. The career's tables and cells
// (the tier byte, the gate, the league cells, the date, the draw table, the pool of constants) take the values a case
// writes (undone afterwards). The gate level 0x102d5880, the youth and veteran tests (0x100dbb60, 0x100dbb20) and the
// club's reaction 0x106a15f0 are answered; the date routines and, in half of the cases, the list append run for real,
// so the 0x20-byte record the routine builds in its frame (padding bytes included: the frame holds junk at them) is
// compared in the heap. The four stack arguments are written at the entry stack pointer. A generator with a fixed seed
// draws each field from the values at and around its cuts; the draws' values are chosen at the cuts of the modulus
// each draw is taken under, and the reputation near the limit the strength and a draw make.
#include "manager_manager_checks_tail.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
using tail::Rng;
constexpr std::uint32_t entry = 0x101c0230;
constexpr std::uint32_t gate_level = 0x102d5880, is_young = 0x100dbb60, is_veteran = 0x100dbb20, react = 0x106a15f0,
                        append = 0x10738d10;
constexpr std::uint32_t list = scratch_base + 0x100, queue = scratch_base + 0x200, rows = scratch_base + 0x10000,
                        club = scratch_base + 0x1000, region = scratch_base + 0x1200, manager = scratch_base + 0x1300,
                        manager_profile = scratch_base + 0x1400, person = scratch_base + 0x2000,
                        person_profile = scratch_base + 0x2200;
constexpr std::uint32_t club_index = 3;

struct Scene {
    bool person_given = true, person_has_profile = true, gate_set = true, club_given = true, manager_given = true,
         manager_has_profile = true, real_append = true, wrap = false;
    std::uint32_t flag = 1, bytes[5] = {1, 2, 3, 4, 5}, stage = 4, kind = 1;
    std::int32_t number = 0x40, slot = 5, gate_answer = 3, young = 0, veteran = 1;
    std::int32_t reputation = 0x1500, manager_value = 0x1500, level = 2, tier = 0, rating = 0x1500;
    std::int32_t talent = 10, form = 10, trait = 5;
    std::uint32_t league = 0x3300, cell_a = 0x3301, cell_b = 0x3302, cell_c = 0x3303;
    std::uint32_t redraw = 0x77, targets[8] = {1, 1, 1, 1, 1, 1, 1, 1};
};

std::int16_t h(std::int64_t v) { return std::int16_t(std::uint16_t(v)); }

// The strength the routine builds from the person's three bytes (steps truncated to 16 bits), for steering the draws.
std::int32_t strength_of(const Scene& s) {
    std::int32_t v = h(s.talent * s.talent * 10);
    v = h(v + s.form * s.form * 10);
    v = h(v - s.trait * s.trait * 10);
    if (!s.flag || (s.flag & 0xff) == 0) v -= 6000;
    return v;
}

// The cost the routine derives from the reputations, the person's bytes and the pool's two scales (steering only).
std::int32_t cut_cost(const Scene& s, double scale_a, double scale_b, std::int32_t sum) {
    const auto truncated = [](double v) { return std::int32_t(std::max(-2147483648.0, std::min(2147483647.0, std::trunc(v)))); };
    const std::int32_t x = s.talent + s.form - s.trait;
    const std::int32_t second = s.rating > 0x1482 ? truncated(scale_b * double(x)) : x;
    return std::int32_t(std::uint32_t(truncated(scale_a * double(sum))) - std::uint32_t(second) * 0x7du);
}

Scene random_scene(Rng& r, const Scenario& sc) {
    Scene s;
    s.person_given = !r.chance(2);
    s.person_has_profile = !r.chance(2);
    s.gate_set = r.chance(50);
    s.club_given = !r.chance(3);
    s.manager_given = !r.chance(3);
    s.manager_has_profile = !r.chance(3);
    s.real_append = r.chance(50);
    s.wrap = r.chance(20);
    s.flag = r.pick<std::uint32_t>({1, 1, 0, 0x100, 0xff, 0x80, 0x101, 0x7f, 2});
    if (r.chance(30)) s.flag |= r.next() & 0xffffff00u;
    for (auto& b : s.bytes) b = r.chance(50) ? r.below(8) : r.next();
    s.stage = r.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 6, 7, 0x80, 0xff, 0x103 & 0xff});
    s.kind = r.pick<std::uint32_t>({0, 1, 2, 3, 3, 4, 5, 0x83, 0xff, 0x80});
    s.number = r.pick<std::int32_t>({0x3c, 0x3d, 0x3b, 0x40, 0, -1, 0x7fff, -0x8000, 100, 10});
    s.slot = r.pick<std::int32_t>({-1, 0, 5, 40, 0x7fffffff, std::int32_t(0x80000000), -2, 1});
    s.gate_answer = r.pick<std::int32_t>({0, 6, 7, 0x106, 0x80, 0xff, 100, -1, 3, 7});
    s.young = r.pick<std::int32_t>({0, 1, 0x100, 0x80, 0x7f, 1, 1});
    s.veteran = r.pick<std::int32_t>({0, 1, 0x100, 0x80, 1, 1, 0xff});
    const std::int32_t cuts[] = {0x7d0, 0x109a, 0x1a5e, 0x1676, 0x1482};
    auto near_cut = [&]() { return cuts[r.below(5)] + r.pick<std::int32_t>({-1, 0, 1, 1, 0, -1, 100, -100}); };
    s.reputation = r.chance(70) ? near_cut() : r.pick<std::int32_t>({0, -1, 0x7fff, -0x8000, 0x2000, 0x3000, 1, 5000, 9999, 10000, 12500});
    s.manager_value = r.chance(70) ? near_cut() : r.pick<std::int32_t>({0, -1, 0x7fff, -0x8000, 0x2000, 1});
    s.level = r.pick<std::int32_t>({-16, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15});
    s.tier = r.pick<std::int32_t>({-128, -4, -3, -2, -1, 0, 1, 5, 127});
    s.rating = r.chance(60) ? 0x1482 + r.pick<std::int32_t>({-1, 0, 1, 1, 0, 1}) : r.pick<std::int32_t>({0, -1, 0x7fff, -0x8000, 0x2000});
    s.talent = r.pick<std::int32_t>({-128, -1, 0, 1, 5, 10, 12, 15, 20, 30, 50, 127, 100});
    s.form = r.pick<std::int32_t>({-128, -1, 0, 1, 5, 10, 12, 15, 20, 30, 50, 127, 100});
    s.trait = r.pick<std::int32_t>({-128, -1, 0, 1, 5, 10, 12, 15, 20, 30, 50, 127, 100});
    const auto id_cell = r.below(5);
    // Which of the three league cells (and none) hold the club's league.
    s.cell_a = id_cell == 0 ? s.league : s.league + 1 + r.below(3);
    s.cell_b = id_cell == 1 ? s.league : s.league + 4 + r.below(3);
    s.cell_c = id_cell == 2 ? s.league : s.league + 8 + r.below(3);
    s.redraw = r.below(0x20000);
    // The draws' targets: the values at and around each modulus cut (a draw is a halfword plus the base).
    const std::uint32_t pool[] = {0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 15, 19, 20, 21, 49, 50, 51, 999, 1000, 1001, 2000, 2999, 3000, 3001,
                                  9999, 10000, 10001, 12499, 12500, 12501, 14999, 15000, 15001, 0x1fffe};
    for (auto& t : s.targets) t = r.chance(75) ? pool[r.below(sizeof(pool) / sizeof(pool[0]))] : r.below(0x20000);
    // The reputation near the limit a draw and the strength make (the limit is half(strength + draw mod M + base)).
    if (r.chance(50)) {
        const std::int32_t moduli[] = {10000, 12500, 15000};
        const std::int32_t bases[] = {0xcb2, 0x109a};
        const auto t = s.targets[r.below(8)];
        const std::int32_t m = moduli[r.below(3)], b = bases[r.below(2)];
        s.reputation = h(strength_of(s) + std::int32_t(t % std::uint32_t(m)) + b) + r.pick<std::int32_t>({-1, 0, 1});
    }
    // Sometimes the reputations are chosen so that the cost lands next to one of its cuts (0xcb2 and 0x186a): the cost is
    // the first scaled sum minus 0x7d times the second, so a slip in either factor moves it across a cut.
    if (r.chance(40)) {
        const auto pool = sc.toc(-0x536c);
        const auto d = [&](std::uint32_t at) { return std::bit_cast<double>((std::uint64_t(sc.r32(pool + at)) << 32) | sc.r32(pool + at + 4)); };
        const double scale_a = d(0x20), scale_b = d(0x28);
        std::vector<std::int32_t> found;
        const std::int32_t cut = r.chance(50) ? 0xcb2 : 0x186a;
        for (std::int32_t sum = -0x10000; sum < 0x10000; ++sum) {
            const auto cost = cut_cost(s, scale_a, scale_b, sum);
            if (cost >= cut - 1 && cost <= cut + 1) found.push_back(sum);
        }
        if (!found.empty()) {
            const auto sum = found[r.below(unsigned(found.size()))];
            const std::int32_t low = std::max(-0x8000, sum - 0x7fff), high = std::min(0x7fff, sum + 0x8000);
            s.manager_value = low + std::int32_t(r.below(unsigned(high - low + 1)));
            s.reputation = sum - s.manager_value;
        }
    }
    return s;
}

Case make(const Scenario& sc, const Scene& s, std::string label) {
    Case item{.entry = entry, .label = "tail apply " + std::move(label)};
    const auto sp = std::uint32_t(sc.s.r<4>(4));
    item.registers = {{3, list}, {4, s.person_given ? person : 0}, {5, s.flag}, {6, s.bytes[0]}, {7, s.bytes[1]},
                      {8, s.bytes[2]}, {9, s.bytes[3]}, {10, s.bytes[4]}};
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    // The caller's stack arguments: a byte at +0x3b, a halfword at +0x3e, a byte at +0x43 and a word at +0x44.
    put(1, sp + 0x3b, s.stage);
    put(2, sp + 0x3e, std::uint32_t(s.number) & 0xffff);
    put(1, sp + 0x43, s.kind);
    put(4, sp + 0x44, std::uint32_t(s.slot));
    // Junk at the padding bytes of the record in the frame (the frame is at sp-0xf0, the record at +0x60).
    const auto record = sp - 0xf0 + 0x60;
    for (const std::uint32_t pad : {5u, 6u, 7u, 0xdu, 0xeu, 0xfu, 0x19u, 0x1du, 0x1eu, 0x1fu}) put(1, record + pad, 0xa0 + pad);
    // the list object, the club's row and the queue
    put(4, list, rows);
    put(4, list + 0x14, queue);
    put(1, rows + club_index * 0x4c + 0x40, (std::uint32_t(s.level) & 0x1f) | 0xa0);
    // the person
    put(4, person, 0x1234);
    put(4, person + 0x14, s.person_has_profile ? person_profile : 0);
    put(4, person + 0x24, s.club_given ? club : 0);
    put(1, person + 0x67, std::uint32_t(s.trait) & 0xff);
    put(1, person_profile + 0x1c, std::uint32_t(s.talent) & 0xff);
    put(1, person_profile + 0x25, std::uint32_t(s.form) & 0xff);
    put(2, person_profile + 0xc, std::uint32_t(s.rating) & 0xffff);
    // the club, its region record (the league id), its manager and the manager's profile
    put(4, club, club_index);
    put(4, club + 0x2c, region);
    put(4, region + 0x68, s.league);
    put(4, club + 0x90, s.manager_given ? manager : 0);
    put(4, manager + 0x1c, s.manager_has_profile ? manager_profile : 0);
    put(2, manager_profile + 0x2a, std::uint32_t(s.manager_value) & 0xffff);
    put(2, club + 0x1ec, std::uint32_t(s.reputation) & 0xffff);
    // the career's cells
    put(4, sc.toc(-0x7428), s.gate_set ? 0x1000 : 0);
    put(1, sc.global(-0x7984) + club_index * 8 + 4, std::uint32_t(s.tier) & 0xff);
    put(4, sc.toc(-0x7598), s.cell_a);
    put(4, sc.toc(-0x7650), s.cell_b);
    put(4, sc.toc(-0x7a00), s.cell_c);
    tail::put_draws(sc, item, s.wrap, s.redraw, std::vector<std::uint32_t>(s.targets, s.targets + 8));
    item.answers.push_back({gate_level, std::uint32_t(s.gate_answer)});
    item.answers.push_back({is_young, std::uint32_t(s.young)});
    item.answers.push_back({is_veteran, std::uint32_t(s.veteran)});
    item.answers.push_back({react, 0});
    if (!s.real_append) item.answers.push_back({append, 0});
    return item;
}
}

MANAGER_MANAGER_CASES("tail_application") {
    if (scenario.match) return;
    Rng r{0x0230a};
    unsigned n = 0;
    for (unsigned i = 0; i < 6000; ++i) cases.push_back(make(scenario, random_scene(r, scenario), "random " + std::to_string(n++)));
}
}

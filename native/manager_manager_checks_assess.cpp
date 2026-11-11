// MANAGER_MANAGER.CPP checker cases for the assessment block (manager_manager_assess.hpp): 0x101c3860.
//
// No capture reaches it. The person, the club and the records they point to are built in the scratch area, so every
// value a rule reads is chosen: the reputation (a halfword) at and around each cut and where the 16-bit steps wrap,
// the tier byte at each cut, the kind byte against 3 (the pool's scale through the floating point), the budget around
// the limit each case computes, and the standing byte. The news desk (0x102cc140) is answered; the message buffer's
// routines (0x102bdf90, 0x102cb4b0) run for real, so each field's value is compared at its call. The assertion path
// (a null person or club) runs with the report routines answered and, in one case, for real.
#include "manager_manager_checks.hpp"
#include <bit>
#include <climits>
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t post = 0x101c3860, news_desk = 0x102cc140;
constexpr std::uint32_t person = scratch_base + 0x1000, ref_a = scratch_base + 0x1100, ref_b = scratch_base + 0x1200,
                        ref_c = scratch_base + 0x1300, club = scratch_base + 0x2000, tier_record = scratch_base + 0x3000,
                        board = scratch_base + 0x4000;

struct Shape {
    std::string label;
    std::int32_t reputation = 0x1400;
    std::int32_t tier = 5;
    std::uint32_t kind = 0;
    std::int32_t budget = 3000;
    std::int32_t standing = 3;
    std::uint32_t first = 1, second = 2, third = 3, fourth = 4;
    bool null_person = false, null_club = false, answered_report = true;
};

std::int16_t h(std::int64_t v) { return std::int16_t(std::uint16_t(v)); }
// The limit the routine computes before the kind byte.
std::int32_t limit_of(const Shape& s) {
    std::int32_t limit;
    if (s.tier > 14) limit = h(h(std::int64_t(s.reputation) * 3) + 0x1388);
    else if (s.tier > 8) limit = h(h(std::int64_t(s.reputation) * 2) + 0x9c4);
    else limit = h(h(std::int64_t(s.reputation) * 2) + 0x4e2);
    if (s.reputation < 0x109a && h(limit) > 0x1388) limit = 0x1388;
    return limit;
}

void add(const Scenario& sc, std::vector<Case>& cases, const Shape& s) {
    Case item{.entry = post,
              .registers = {{4, s.null_person ? 0 : person}, {5, s.null_club ? 0 : club}, {6, s.first}, {7, s.second}, {8, s.third}, {9, s.fourth}},
              .label = "assess " + s.label};
    item.memory = {{4, person, 0x1234}, {4, person + 0x28, ref_a}, {4, person + 0x2c, ref_b}, {4, person + 0x30, ref_c},
                   {4, ref_a + 0x20, 0xa1}, {4, ref_b + 0x20, 0xb2}, {4, ref_c + 0x20, 0xc3},
                   {4, club, 0x5678}, {4, club + 0x2c, tier_record}, {4, club + 0x3c, board},
                   {2, club + 0x1ec, std::uint32_t(s.reputation) & 0xffff}, {1, club + 0x23d, s.kind},
                   {1, club + 0x240, std::uint32_t(s.standing) & 0xff}, {1, tier_record + 0x11f, std::uint32_t(s.tier) & 0xff},
                   {4, board + 4, std::uint32_t(s.budget)}};
    item.answers = {{news_desk, 0}};
    if (s.answered_report)
        for (std::uint32_t report : {0x10739890u, 0x10768810u, 0x1077b720u, 0x10769b60u}) item.answers.push_back({report, 0});
    (void)sc;
    cases.push_back(std::move(item));
}
}

MANAGER_MANAGER_CASES("assess_post") {
    if (scenario.match) return;
    constexpr std::int32_t reputations[] = {-0x8000, -1, 0, 1, 0x1099, 0x109a, 0x109b, 0x1388, 0x1676, 0x1677, 0x2aaa, 0x2aab, 0x4000, 0x5555, 0x7fff};
    constexpr std::int32_t tiers[] = {-128, -1, 0, 8, 9, 14, 15, 127};
    const auto pool = scenario.toc(-0x536c);
    const double scale = std::bit_cast<double>((std::uint64_t(scenario.r32(pool + 0x20)) << 32) | scenario.r32(pool + 0x24));
    for (const auto reputation : reputations)
        for (const auto tier : tiers)
            for (const std::uint32_t kind : {0u, 3u, 2u, 4u, 0x103u}) {
                Shape s{.reputation = reputation, .tier = tier, .kind = kind};
                std::int32_t limit = limit_of(s);
                if (kind == 3) limit = std::int32_t(std::int64_t(scale * double(h(limit))));
                for (const std::int32_t budget : {limit - 1, limit, limit + 1, std::int32_t(h(limit)) - 1, std::int32_t(h(limit)), std::int32_t(h(limit)) + 1, INT_MIN, -1, 0, INT_MAX}) {
                    s.budget = budget;
                    s.label = "limit rep " + std::to_string(reputation) + " tier " + std::to_string(tier) + " kind " + std::to_string(kind) + " budget " + std::to_string(budget);
                    add(scenario, cases, s);
                }
            }
    for (const auto reputation : reputations)
        for (const std::int32_t standing : {-128, -1, 0, 9, 10, 13, 14, 127}) {
            Shape s{.reputation = reputation, .standing = standing};
            s.label = "band rep " + std::to_string(reputation) + " standing " + std::to_string(standing);
            add(scenario, cases, s);
        }
    for (const std::uint32_t arg : {0u, 1u, 0x7fu, 0x80u, 0xffu, 0x1ffu, 0xdead0081u, 0xdead007fu}) {
        Shape s{.first = arg, .second = ~arg, .third = arg ^ 0x80, .fourth = arg + 0x7f};
        s.label = "bytes " + std::to_string(arg);
        add(scenario, cases, s);
    }
    add(scenario, cases, {.label = "person null", .null_person = true});
    add(scenario, cases, {.label = "club null", .null_club = true});
    add(scenario, cases, {.label = "both null", .null_person = true, .null_club = true});
    add(scenario, cases, {.label = "person null, real report", .null_person = true, .answered_report = false});
}
}

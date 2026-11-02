// 0x106584d0: the start of the manager's verdict (the guards, the contract, the story's value) and the
// dispatch by kind.
#include "media_eval_a_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_a {
Verdict::Verdict(Cpu& cpu, std::uint32_t reporter_, std::uint32_t manager_, std::uint32_t player_, std::uint32_t kind_)
    : c(cpu), frame(cpu, entry, 0x90, {.first_gpr = 17}), fpu(cpu), rng(Stream::world(cpu)), program{cpu}, reporter(reporter_),
      manager(manager_), player(player_), kind(kind_) {
    // r17 the verdict (3 until a story says otherwise), r18 the pool of single floats until the first draw,
    // r19 the reporter until the stories reuse it for the range of a draw, r21 the table of the players' rows,
    // r28 the pool of doubles; r22, r24 and r30 are set by the guards and the story's value.
    keep(c, {{17, 3}, {18, c.toc(-0x4200)}, {19, reporter}, {20, kind}, {21, c.toc(-0x7b9c)}, {23, player}, {25, rng.end}, {26, rng.start},
             {27, manager}, {28, c.toc(slot::floats)}, {29, rng.seed}, {31, rng.cursor}});
}

// The guards: a manager with a staff record, a player (if any) with a player record who is not the manager,
// the club he works for and the player's contract there.
bool Verdict::admit() {
    compare(c, std::int32_t(manager));  // or. r27,r4,r4
    if (manager == 0) return answer(3);
    if (!nonzero_logical(c, staff())) return answer(3);
    if (player != 0 && !nonzero_logical(c, c.u32(player + 0x14))) return answer(3);
    if (compare_logical(c, manager, player) == 0) return answer(3);
    // The club: the manager's own, else the one he is attached to.
    club = c.u32(manager + 0x24);
    if (!nonzero_logical(c, club)) club = c.u32(manager + 0x20);
    c.set(22, club);
    if (!nonzero_logical(c, club)) return answer(3);
    contract = program.call(ask::contract_of, 0x10658578, {c.u32(c.toc(-0x7b7c)), player, club});
    c.set(24, contract);
    if (compare(c, std::int32_t(contract)) == 0) return answer(3);
    return false;
}

// The story's value (0x10661a50) for the player, scaled by the pool's factor, as the amount (r30).
void Verdict::value_of_story() {
    if (compare_logical(c, player) == 0) return;
    const auto singles = c.r(18);
    c.set_f(1, double(c.f32(singles + 8)));
    enter(c, 0x106585b8, {reporter, player, club, contract, kind, 3});
    story_value(c, c.f(1), player, club, contract, kind, 3);
    const auto scaled = fpu.fctiwz(0, fpu.fmuls(double(c.f32(singles + 0xc)), c.f(1)));
    c.put64(frame.at(0x48), scaled);
    c.set(30, c.u32(frame.at(0x4c)));
}

void Verdict::run() {
    if (admit()) return;
    value_of_story();
    const auto story = kind_half();
    if (compare(c, story, 0x83) == 0) { story_83(); return; }
    if (compare(c, story, 0x82) == 0) { story_82(); return; }
    if (compare(c, story, 0x1f) == 0) { story_1f(); return; }
    if (mood()) return;
    if (story_3c()) return;
    if (transfer_story()) return;
    if (story_social()) return;
    if (story_8_25()) return;
    if (story_1()) return;
    if (story_1e_20()) return;
    if (story_rated()) return;
    if (story_3_4()) return;
    if (story_5()) return;
    if (story_6()) return;
    answer(3);
}

std::int32_t Verdict::remainder(const Division& by, std::uint32_t refill_back) {
    rng.advance(refill_back);
    const auto sample = std::int32_t(rng.sample());
    auto high = multiply_high(by.magic, sample);
    if (by.add) high = std::int32_t(std::uint32_t(high) + std::uint32_t(sample));
    const auto shifted = std::uint32_t(high >> by.shift), sign = shifted >> 31;
    const auto rest = std::int32_t(std::uint32_t(sample) - (shifted + sign) * std::uint32_t(by.divisor));
    switch (by.scratch) {
    case Scratch::quotient: leave(c, {{3, sign}, {4, std::uint32_t(sample)}}); break;
    case Scratch::high: leave(c, {{3, std::uint32_t(high)}, {4, std::uint32_t(sample)}}); break;
    case Scratch::loaded: leave(c, {{3, std::uint32_t(rest)}, {4, sign}, {5, std::uint32_t(sample)}}); break;
    }
    return by.negated ? std::int32_t(0u - std::uint32_t(rest)) : rest;
}
}

void manager_verdict(Cpu& c, std::uint32_t reporter, std::uint32_t manager, std::uint32_t player, std::uint32_t kind) {
    eval_a::Verdict(c, reporter, manager, player, kind).run();
}
}

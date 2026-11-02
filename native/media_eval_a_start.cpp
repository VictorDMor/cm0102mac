// 0x106584d0: the start of the manager's verdict (the guards, the contract, the story's value) and the
// dispatch by kind.
#include "media_eval_a_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_a {
Verdict::Verdict(Cpu& cpu, std::uint32_t reporter_, std::uint32_t manager_, std::uint32_t player_, std::uint32_t kind_)
    : c(cpu), frame(cpu, entry, 0x90, {.first_gpr = 17}), fpu(cpu), rng(Stream::world(cpu)), program{cpu},
      reporter(reporter_), manager(manager_), player(player_), kind(kind_), story_amount(cpu) {
    // r17 the verdict (3 until a story says otherwise), r18 the pool of single floats until the first draw,
    // r19 the reporter until the stories reuse it for the range of a draw, r21 the table of the players' rows,
    // r28 the pool of doubles; r22, r24 and r30 are set by the guards and the story's value.
    keep(c, {{17, 3}, {18, c.toc(slot::doubles)}, {19, reporter}, {20, kind}, {21, c.toc(records::tables::tallies)},
             {23, player}, {25, rng.end}, {26, rng.start}, {27, manager}, {28, c.toc(slot::floats)}, {29, rng.seed},
             {31, rng.cursor}});
}

// The guards: a manager with a staff record, a player (if any) with a player record who is not the manager,
// the club he works for and the player's contract there.
bool Verdict::admit() {
    compare(c, std::int32_t(manager));  // or. r27,r4,r4
    if (manager == 0) return answer(3);
    if (!nonzero_logical(c, boss().non_player())) return answer(3);
    if (player != 0 && !nonzero_logical(c, subject().player())) return answer(3);
    if (compare_logical(c, manager, player) == 0) return answer(3);
    // The club: the manager's own, else the one he is attached to.
    club = boss().club();
    if (!nonzero_logical(c, club)) club = boss().national_team();
    c.set(22, club);
    if (!nonzero_logical(c, club)) return answer(3);
    contract = program.call(callee::contract_of, 0x10658578, {c.u32(c.toc(records::tables::standings)), player, club});
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
    story_amount = c.u32(frame.at(0x4c));
}

void Verdict::run() {
    if (admit()) return;
    value_of_story();
    const auto story = kind_half();
    if (compare(c, story, 0x83) == 0) return standing_item();
    if (compare(c, story, 0x82) == 0) return pair_item();
    if (compare(c, story, 0x1f) == 0) return happiness_story();
    if (mood()) return;
    if (fixture_story()) return;
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

std::int32_t Verdict::trait(const records::Person& who, Trait which) const {
    switch (which) {
    case Trait::ambition: return who.ambition().get();
    case Trait::loyalty: return who.loyalty().get();
    case Trait::professionalism: return who.professionalism().get();
    case Trait::sportsmanship: return who.sportsmanship().get();
    case Trait::composure: return who.composure().get();
    default: return who.temperament().get();  // Trait::temperament (the last of an exhaustive switch)
    }
}

std::int32_t Verdict::skill(Skill which) const {
    const auto record = boss_staff();
    switch (which) {
    case Skill::attitude: return record.attitude().get();
    case Skill::man_management: return record.man_management().get();
    case Skill::tactics: return record.tactics().get();
    case Skill::standing: return record.standing().get();
    default: return record.discipline().get();  // Skill::discipline
    }
}

std::int32_t Verdict::attribute(Attribute which) const {
    const auto record = subject().player_record();
    return which == Attribute::dirtiness ? record.dirtiness().get() : record.versatility().get();
}

std::int32_t Verdict::draw_of(const Draw& what) {
    switch (what.from) {
    case From::manager: return draw(trait(boss(), Trait(what.which)), what.refill_back);
    case From::staff: return draw(skill(Skill(what.which)), what.refill_back);
    case From::effective: return draw(effective(Skill(what.which), what.query_back), what.refill_back);
    case From::amount: return draw_amount(what.refill_back);
    case From::subject: return draw(trait(subject(), Trait(what.which)), what.refill_back);
    default: return draw(attribute(Attribute(what.which)), what.refill_back);  // From::subject_attribute
    }
}

bool Verdict::passes(const Step& step) {
    const auto drawn = draw_of(step.draw);
    switch (step.test) {
    case Test::zero: return compare(c, drawn) == 0;
    case Test::nonzero: return compare(c, drawn) != 0;
    case Test::below: return compare_remainder(drawn, step.against, step.reduce_back) < 0;
    case Test::above: return compare_remainder(drawn, step.against, step.reduce_back) > 0;
    default: return compare_remainder(drawn, step.against, step.reduce_back) <= 0;  // Test::at_most
    }
}

bool Verdict::all(std::initializer_list<Step> steps) {
    for (const auto& step : steps)
        if (!passes(step)) return false;
    return true;
}

std::int32_t Verdict::remainder(const Remainder& divisor, std::uint32_t refill_back) {
    rng.advance(refill_back);
    const auto reduced = reduced_by_signed(rng.sample(), divisor.reciprocal);
    switch (divisor.scratch) {
    case Scratch::quotient: leave(c, {{3, reduced.sign}, {4, reduced.sample}}); break;
    case Scratch::high: leave(c, {{3, std::uint32_t(reduced.high)}, {4, reduced.sample}}); break;
    default: leave(c, {{3, std::uint32_t(reduced.rest)}, {4, reduced.sign}, {5, reduced.sample}}); break;  // Scratch::loaded
    }
    return divisor.negated ? std::int32_t(0u - std::uint32_t(reduced.rest)) : reduced.rest;
}
}

void manager_verdict(Cpu& c, std::uint32_t reporter, std::uint32_t manager, std::uint32_t player, std::uint32_t kind) {
    eval_a::Verdict(c, reporter, manager, player, kind).run();
}
}

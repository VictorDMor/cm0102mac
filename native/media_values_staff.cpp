// 0x10647730: the random staff member the press quotes.
#include "media_records.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media {
using namespace detail;
using namespace detail::records;

// Up to 150 rounds, each drawing (up to 99 times) a person from the 0x70-byte table who is established
// (age byte >= 37, +0x5e >= 50), has a +0x1c record, is unattached or has no +0x24 club and is not
// `club`'s manager. The candidate is scored by a draw over (+0x5e times the rating of its +0xc nation),
// doubled for a person of the nation of `club` (or of `other` when given); with `other`, strangers still qualify on a
// random rating test and 0x100d9580. Returns the best-scoring candidate, or 0 (also when both are null).
std::uint32_t pick_staff(Cpu& c, std::uint32_t club, std::uint32_t other) {
    Frame frame(c, 0x10647730, 0x90, {.first_gpr = 17});
    Fpu fpu(c);
    const auto rng = Stream::world(c);
    const Kept<23> person_count_slot(c, c.toc(tables::person_count));
    const Kept<26> persons_slot(c, c.toc(tables::persons));
    const Kept<27> floats_slot(c, c.toc(slot::floats));
    Kept<21> best(c, 0);
    keep(c, {{24, club}, {25, other}, {28, rng.seed}, {29, rng.start}, {30, rng.end}, {31, rng.cursor}});
    const Club home{c, club};
    const Person stranger{c, other};
    compare(c, std::int32_t(other));
    if (other == 0 && !nonzero_logical(c, club)) return result(c, 0);
    constexpr DrawSite by_count{0x106477bc, 0x10647894, 0x48, 0x40, 0x38, 20}, by_weight{0x106479a4, 0x10647a7c, 0x38, 0x40, 0x48, 17};
    Held<19> best_score{c};
    for (Kept<22> round(c, 0); compare(c, std::int32_t(round), 0x96) < 0; round = round + 1) {
        Kept<18> attempts(c, 0);
        Held<20> chosen{c};
        while (true) {
            const auto count = std::int32_t(c.u32(person_count_slot));
            const Kept<17> range(c, std::uint32_t(count));
            const auto drawn = draw_below(c, frame, fpu, rng, std::uint32_t(floats_slot), by_count, count);
            attempts = attempts + 1;
            c.set(3, c.u32(persons_slot));
            chosen = c.u32(persons_slot) + std::uint32_t(drawn) * Person::size;
            const Person candidate{c, chosen};
            if (compare(c, std::int32_t(attempts), 0x64) >= 0) break;
            if (compare(c, candidate.age(), 0x25) < 0) continue;
            if (compare_logical(c, candidate.ability(), 0x32) < 0) continue;
            if (compare_logical(c, candidate.non_player()) == 0) continue;
            if (compare_logical(c, candidate.player()) != 0 && compare_logical(c, candidate.club()) != 0) continue;
            if (compare_logical(c, home.manager(), chosen) == 0) continue;
            break;
        }
        if (compare(c, std::int32_t(attempts), 0x64) >= 0) continue;
        const Person picked{c, chosen};
        // The weight: the ability byte times the rating of the person's nation; no weight scores 0.
        const auto nation = picked.nation_record();
        leave(c, {{3, picked.nation()}, {4, picked.ability()}});
        const Kept<18> weight(c, std::uint32_t(picked.ability()) * std::uint32_t(std::int32_t(nation.rating())));
        Held<17> score{c};
        if (compare(c, std::int32_t(weight)) == 0) score = 0;
        else draw_below(c, frame, fpu, rng, std::uint32_t(floats_slot), by_weight, std::int32_t(weight));
        // A person of the nation of `club` (no `other`), or of `other`, doubles the score.
        bool doubled = false;
        if (compare_logical(c, other) == 0) {
            c.set(3, picked.nation());
            doubled = compare_logical(c, picked.nation(), home.nation()) == 0;
        }
        if (!doubled && compare_logical(c, other) != 0) {
            c.set(3, picked.nation());
            doubled = compare_logical(c, picked.nation(), stranger.nation()) == 0;
        }
        if (doubled) score = score << 1;
        else {
            // Strangers need `other`, a rating test against a draw modulo 100, a staff record rating of at least
            // 6000 and the verdict of 0x100d9580.
            if (compare_logical(c, other) == 0) continue;
            rng.advance(0x10647b40);
            const auto x = std::int32_t(rng.sample());
            const auto rating = std::int32_t(nation.rating()), rest = x % 100;
            leave(c, {{3, std::uint32_t(rating)}, {4, std::uint32_t(rest)}, {5, std::uint32_t(x)}});
            if (compare(c, rating, rest + 0x1964) < 0) continue;
            c.set(3, picked.non_player());
            if (compare(c, std::int32_t(picked.staff_record().world_reputation()), 0x1770) < 0) continue;
            const auto refused = Program{c}.call(0x100d9580, 0x10647bac, {other, picked.at});
            if (nonzero(c, byte_word(refused))) continue;
        }
        if (!nonzero_logical(c, best) || compare(c, std::int32_t(score), std::int32_t(best_score)) > 0) {
            best_score = score;
            best = picked.at;
        }
    }
    return result(c, best);
}
}

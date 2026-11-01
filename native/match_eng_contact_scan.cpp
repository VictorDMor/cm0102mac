// MATCH_ENG.CPP, contact: scans over a side's players: the man of the match, and the performances marked
// in a side's event list.
#include "match_eng_contact.hpp"
#include "match_eng_shots.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace shots_detail;

namespace {
constexpr std::uint32_t flags_of = 0x10289560, mark_event = 0x10113e30;
}

// 0x101f7100: the best player of both squads by a score of his form and his rating bytes; his record's id goes to +0x3100.
void choose_man_of_the_match(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101f7100, 0x80, {.first_gpr = 21});
    Kept<26> match_kept(c, match);
    const auto table = c.toc(-0x52c4);
    keep(c, {{31, table}});
    Kept<24> best_score(c, 0);
    Kept<28> best(c, 0);
    Kept<22> best_key(c, 0);
    Fpu fp(c);
    for (Kept<27> side(c, 0); compare(c, s8(side), 2) < 0; side = side + 1) {
        Kept<30> player(c, match + u(s8(side) * std::int32_t(side_size)) + players_at);
        for (Kept<25> index(c, 0);; index = index + 1, keep(c, {{21, player}}), player = player + player_size) {
            // The squad is eleven players plus the substitutes the fixture names.
            c.set(4, u(s8(index)));
            const auto roster = c.u32(match + fixture_at);
            c.set(3, (c.u8(roster + 0x4e) >> 4) & 0xf);
            if (compare(c, s8(index), s8(c.r(3) + 0xb)) >= 0) break;
            c.set(3, player);
            if (compare(c, s8(position(c, player)), 0) < 0 && compare(c, s8(c.u8(player + 0x20)), -1) == 0) continue;
            const double scale = c.f64(table + 0x2f8);
            const double magic = c.f64(table + 0x60);
            c.set_f(1, magic);
            const double form = from_int(c, fp, frame.at(0x40), u(c.i16(player + 0x36)), magic);
            c.put64(frame.at(0x38), fp.fctiwz(0, fp.fmul(scale, form)));
            keep(c, {{23, c.u32(frame.at(0x3c))}});
            std::uint32_t score = u(c.i16(player + 0x36));
            const auto weigh = [&](std::uint32_t at, std::int32_t weight) { score += u(std::int16_t(c.u8(player + at) * weight)); };
            weigh(0x10, 0x7d); weigh(0xc, 0xfa); weigh(3, 0x32); weigh(6, 0x23);
            keep(c, {{29, score}});
            if (compare_logical(c, best) != 0) {
                if (compare(c, std::int16_t(score), std::int16_t(best_score)) <= 0) continue;
                if (compare(c, std::int16_t(c.r(23)), std::int16_t(best_key)) < 0) continue;
            }
            best = player;
            best_score = score;
            best_key = c.r(23);
        }
    }
    if (compare_logical(c, best) != 0) {
        c.set(3, c.u32(best + 0x6c));
        c.put32(match + 0x3100, c.u32(c.r(3)));
    }
}

// 0x101f7280: players whose score passes 8500 are marked (0x800000) in the side's event list. The score starts from the
// fitness and three rating bytes of the record; the flags halfword of the player raises or lowers it.
void flag_star_performers(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101f7280, 0x70, {.first_gpr = 26});
    Kept<26> match_kept(c, match);
    Kept<27> side_kept(c, side);
    Kept<31> player(c, match + u(s8(side) * std::int32_t(side_size)) + players_at);
    Fpu fp(c);
    Kept<30> score(c, c.r(30));
    Kept<28> previous(c, c.r(28));
    // bl 0x10289560, then the halfword extended and tested against a mask.
    const auto flags = [&](std::uint32_t back) {
        c.set(3, player);
        c.call(flags_of, back);
        return std::int32_t(std::int16_t(c.r(3)));
    };
    const auto has = [&](std::int32_t flag_set, std::int32_t mask) { return compare(c, flag_set & mask) != 0; };
    for (Kept<29> index(c, 0); compare(c, s8(index), 0x14) < 0; index = index + 1, previous = player, player = player + player_size) {
        c.set(3, player);
        if (compare(c, s8(position(c, player)), 0) < 0) continue;
        c.set(3, player);
        const double fitness_now = fitness(c, player);
        const auto table = c.toc(-0x52c4);
        const double magic = c.f64(table + 0x60);
        const auto grade = [&](std::uint32_t at) { return u(s8(c.u8(c.u32(player + 0x70) + at)) * 10); };
        double sum = fp.fadds(from_int_single(c, fp, frame.at(0x50), grade(0x33), magic), fitness_now);
        c.set_f(1, magic);
        sum = fp.fadds(from_int_single(c, fp, frame.at(0x48), grade(0x43), magic), sum);
        sum = fp.fadds(from_int_single(c, fp, frame.at(0x40), grade(0x27), magic), sum);
        c.put64(frame.at(0x38), fp.fctiwz(0, sum));
        score = c.u32(frame.at(0x3c));
        // A player's flag halfword moves the score: flag 1 voids it, 2 adds 500, 4 subtracts (more without
        // 0x800 or 0x80), 8 subtracts 300 unless 0x800 or 0x80, 0x10 adds 1000, 0x20 adds 2000.
        if (const auto f = flags(0x101f737c); has(f, 1)) score = 0;
        else if (const auto g = flags(0x101f739c); has(g, 2)) score = score + 0x1f4;
        else if (const auto h = flags(0x101f73bc); has(h, 4)) {
            if (const auto k = flags(0x101f73d4); has(k, 0x800)) score = score - 0x1f4;
            else if (const auto m = flags(0x101f73ec); has(m, 0x80)) score = score - 0x1f4;
            else score = score - 0x3e8;
        } else if (const auto n = flags(0x101f7414); has(n, 8)) {
            if (const auto p = flags(0x101f742c); !has(p, 0x800)) {
                if (const auto q = flags(0x101f7444); !has(q, 0x80)) score = score - 0x12c;
            }
        } else if (const auto r = flags(0x101f7464); has(r, 0x10)) score = score + 0x3e8;
        else if (const auto s = flags(0x101f7484); has(s, 0x20)) score = score + 0x7d0;
        if (compare(c, std::int16_t(score), 0x2134) <= 0) continue;
        c.set(3, player);
        c.set(4, position(c, player));
        c.set({{3, match + u(s8(side) * team_tactics_size) + team_tactics}, {5, 0}, {6, 0x800000}});
        c.call(mark_event, 0x101f74d0);
    }
}

// 0x101f7500: every player on the pitch of the side is marked (0x400000) in the side's event list.
void flag_players_on_pitch(Cpu& c, std::uint32_t match, std::uint32_t side) {
    Frame frame(c, 0x101f7500, 0x50, {.first_gpr = 27});
    Kept<27> match_kept(c, match);
    Kept<28> side_kept(c, side);
    Kept<31> player(c, match + u(s8(side) * std::int32_t(side_size)) + players_at);
    Kept<29> previous(c, c.r(29));
    for (Kept<30> index(c, 0); compare(c, s8(index), 0x14) < 0; index = index + 1, previous = player, player = player + player_size) {
        c.set(3, player);
        if (compare(c, s8(position(c, player)), 0) < 0) continue;
        c.set(3, player);
        c.set(4, position(c, player));
        c.set({{3, match + u(s8(side) * team_tactics_size) + team_tactics}, {5, 0}, {6, 0x400000}});
        c.call(mark_event, 0x101f7570);
    }
}
}

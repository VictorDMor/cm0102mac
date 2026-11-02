// Cases for the press's text generators: story records laid out in the scratch area over real
// clubs and staff, one per variant and per branch of each text.
#include "media_checks.hpp"
#include <array>
#include <stdexcept>

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
constexpr std::uint32_t record_at = scratch_base + 0x100, output_at = scratch_base + 0x2000;

void put32(Writes& out, std::uint32_t at, std::uint32_t value) { out.push_back({4, at, value}); }
Writes join(std::initializer_list<Writes> parts) {
    Writes all;
    for (const auto& part : parts) all.insert(all.end(), part.begin(), part.end());
    return all;
}
// A story record: the word at +8 + 4 * index.
Writes fields(std::uint32_t at, std::initializer_list<std::uint32_t> values) {
    Writes out;
    std::uint32_t index = 0;
    for (const auto value : values) put32(out, at + 8 + 4 * index++, value);
    return out;
}
// The text object's own routines (set, append, link) answer on both sides: the object is not under test.
const std::vector<std::pair<std::uint32_t, std::uint32_t>> text_answers{{0x1074b830, 0}, {0x1074b960, 0}, {0x1074bab0, 0}};
Case make(std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, Writes memory, std::string label,
          std::vector<std::pair<std::uint32_t, std::uint32_t>> more = {}) {
    auto answers = text_answers;
    answers.insert(answers.end(), more.begin(), more.end());
    return {.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label),
            .answers = std::move(answers)};
}

// A staff member with a name (the ids of his three name parts, behind +0x28, +0x2c and +0x30).
struct Staff { std::uint32_t index, first, middle, last; };
std::vector<Staff> named_staff(const Scenario& scenario, unsigned wanted) {
    std::vector<Staff> out;
    const auto table = scenario.global(-0x7be0);
    for (std::uint32_t index = 1; index < 4000 && out.size() < wanted; ++index) {
        const auto row = table + index * 0x70;
        try {
            const auto a = scenario.r32(row + 0x28), b = scenario.r32(row + 0x2c), c = scenario.r32(row + 0x30);
            if (!a || !b || !c) continue;
            out.push_back({index, scenario.r32(a + 0x20), scenario.r32(b + 0x20), scenario.r32(c + 0x20)});
        } catch (const std::exception&) { break; }
    }
    return out;
}
}

MEDIA_CASES("text_mid") {
    const auto staff = named_staff(scenario, 3);
    if (staff.size() < 3) throw std::runtime_error("text_mid cases: no named staff in the snapshot");
    constexpr std::uint32_t club_a = 7, club_b = 11;
    // 0x106a1860: manager_outburst_text. The record: club, manager (id and three name parts), variant,
    // other staff (id and three name parts), amount flag at 0xb, number at 0xa, player at 0xc, fine at 0xd.
    for (const std::uint32_t full : {1u, 0u})
        for (const std::uint32_t variant : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 0xffffffffu})
            for (const std::uint32_t number : {0xffffffffu, 3u})
                for (const bool named : {true, false}) {
                    const auto& manager = staff[0];
                    const auto& other = staff[1];
                    const auto bad = [&](std::uint32_t id) { return named ? id : id + 1; };
                    for (const std::uint32_t player : {0xffffffffu, 9u}) {
                        if (player != 0xffffffffu && (number != 3u || !named)) continue;
                        cases.push_back(make(0x106a1860, {{3, 0}, {4, record_at}, {5, full}, {6, output_at}},
                            fields(record_at, {club_a, manager.index, bad(manager.first), manager.middle, manager.last, variant, other.index,
                                               bad(other.first), other.middle, other.last, number, 4, player, 7}),
                            "outburst full=" + std::to_string(full) + " variant=" + std::to_string(int(variant)) + " number=" + std::to_string(int(number)) +
                                (named ? " named" : " unnamed") + (player == 0xffffffffu ? "" : " player")));
                    }
                }

    // 0x1069d460: staff_match_text. The record: staff member and name parts, club, kind, then the match
    // 1030dee0 decodes (day, year, competition, two halfwords and a byte, both clubs, both goal bytes, the match id),
    // then the variant.
    const auto& staff_member = staff[0];
    const auto match_record = [&](std::uint32_t kind, std::uint32_t goals_one, std::uint32_t goals_two, std::uint32_t variant, bool named) {
        return fields(record_at, {staff_member.index, named ? staff_member.first : staff_member.first + 1, staff_member.middle, staff_member.last,
                                  club_a, kind, 150, 2001, 3, 1, 2, 4, club_a, club_b, goals_one, goals_two, 5, variant});
    };
    for (const std::uint32_t full : {1u, 0u})
        for (const std::uint32_t kind : full ? std::initializer_list<std::uint32_t>{0, 5} : std::initializer_list<std::uint32_t>{0, 1, 2, 3, 4, 5, 6, 7})
            for (const auto goals : std::initializer_list<std::pair<std::uint32_t, std::uint32_t>>{{2, 1}, {1, 2}, {1, 1}, {0xfe, 1}}) {
                if (!full && goals.first != 2) continue;
                for (const std::uint32_t variant : {0u, 1u, 2u, 3u, 4u, 5u})
                    for (const bool named : {true, false})
                        for (const std::uint32_t exists : {0u, 1u}) {
                            if (!full && (variant != 0 || exists)) continue;
                            if ((variant == 5 || variant == 0) && (goals.first != 1 || goals.second != 1) && !named) continue;
                            cases.push_back(make(0x1069d460, {{3, 0}, {4, record_at}, {5, full}, {6, output_at}}, match_record(kind, goals.first, goals.second, variant, named),
                                "match full=" + std::to_string(full) + " kind=" + std::to_string(kind) + " goals=" + std::to_string(goals.first) + "-" +
                                    std::to_string(goals.second) + " variant=" + std::to_string(variant) + (named ? " named" : " unnamed") + (exists ? " exists" : ""),
                                {{0x1029eb20, exists}}));
                        }
            }

    // 0x106a2330: sent_off_text. The record: staff member a (id, three name parts), b (same), the kind, the club, a second
    // club, a week, staff c and d (id and name parts) and the reply; then the match 1030dee0 decodes.
    const auto user_base = scenario.toc(-0x7bb0);
    const auto user_row = user_base + std::uint32_t(std::int32_t(std::int16_t(scenario.r32(user_base + 0x2c58) >> 16))) * 0x2c0;
    const auto staff_table = scenario.global(-0x7be0);
    std::uint32_t club_with_player = 7;
    for (std::uint32_t index = 0; index < 400; ++index)
        if (scenario.r32(scenario.club(index) + 0x3c) && scenario.r32(scenario.club(index) + 0x2c)) { club_with_player = index; break; }
    for (const std::uint32_t full : {1u, 0u})
        for (const std::uint32_t kind : {0u, 1u, 0xffffffffu})
            for (const std::uint32_t reply : full ? std::initializer_list<std::uint32_t>{0, 1, 2, 3, 4, 0xffffffffu} : std::initializer_list<std::uint32_t>{0})
                for (const bool users : {false, true})
                    for (const bool home : {true, false})
                        for (const auto goals : std::initializer_list<std::pair<std::uint32_t, std::uint32_t>>{{2, 1}, {1, 1}, {1, 2}})
                            for (const bool named : {true, false}) {
                                if (!full && (users || !home || goals.first != 2 || (!named && kind != 0))) continue;
                                if (users && reply == 0) continue;
                                const auto& a = staff[0];
                                const auto& b = staff[1];
                                const auto& c = staff[2];
                                const auto tweak = [&](std::uint32_t id) { return named ? id : id + 1; };
                                auto memory = fields(record_at, {a.index, tweak(a.first), a.middle, a.last, b.index, tweak(b.first), b.middle, b.last, kind, club_a, club_b,
                                                                 12, c.index, tweak(c.first), c.middle, c.last, a.index, tweak(a.first), a.middle, a.last, reply,
                                                                 150, 2001, 3, 1, 2, 4, home ? club_a : club_b, home ? club_b : club_a, goals.first, goals.second, 5});
                                if (users) put32(memory, user_row + 4, staff_table + a.index * 0x70);
                                cases.push_back(make(0x106a2330, {{3, 0}, {4, record_at}, {5, full}, {6, output_at}}, memory,
                                    "sent off full=" + std::to_string(full) + " kind=" + std::to_string(int(kind)) + " reply=" + std::to_string(int(reply)) +
                                        (users ? " users" : "") + (home ? " home" : " away") + " goals=" + std::to_string(goals.first) + "-" + std::to_string(goals.second) +
                                        (named ? " named" : " unnamed")));
                            }
    // 0x106a44d0: outlet_report_text. The record: manager (id, three name parts), a second staff member (same), club, flags,
    // detail, outlet kind, a region, a dropped field, an amount, the story kind, an age, a mood, a reply, an attitude, an
    // undecided word and a choice.
    const auto user_manager = scratch_base + 0x8000, manager_club = scratch_base + 0x8100;
    const auto club_row = scenario.club(club_with_player);
    const auto nation = scenario.r32(club_row + 0x2c);
    enum class Nation { none, same_by_club, same_by_manager, differs };
    const auto nation_writes = [&](Nation how) {
        Writes out;
        switch (how) {
        case Nation::none: put32(out, user_row + 4, 0); break;
        case Nation::same_by_club: put32(out, user_row + 4, user_manager); put32(out, user_manager + 0x24, manager_club); put32(out, manager_club + 0x2c, nation); break;
        case Nation::same_by_manager: put32(out, user_row + 4, user_manager); put32(out, user_manager + 0x24, 0); put32(out, user_manager + 0xc, nation); break;
        case Nation::differs: put32(out, user_row + 4, user_manager); put32(out, user_manager + 0x24, 0); put32(out, user_manager + 0xc, nation + 4); break;
        }
        return out;
    };
    const auto outlet_record = [&](std::uint32_t full_story, std::uint32_t outlet_kind, std::uint32_t region, std::uint32_t story, std::uint32_t flags,
                                   std::uint32_t detail, std::uint32_t reply, std::uint32_t attitude, std::uint32_t mood, std::uint32_t age,
                                   std::uint32_t undecided, std::uint32_t choice, bool named) {
        (void)full_story;
        const auto& m = staff[0];
        const auto& o = staff[1];
        return fields(record_at, {m.index, named ? m.first : m.first + 1, m.middle, m.last, o.index, named ? o.first : o.first + 1, o.middle, o.last,
                                  club_with_player, flags, detail, outlet_kind, region, 77, 5, story, age, mood, reply, attitude, undecided, choice});
    };
    // 0x10727880 reads one byte of the data section; the magazine's article marker is whether it is 8.
    constexpr std::uint32_t day_kind_byte = 0x108bba90 - 0x35ac;
    const Writes magazine_writes[]{{{1, day_kind_byte, 8}}, {{1, day_kind_byte, 0}}};
    // Every outlet, with every nation relation, in the full and the short form.
    for (const std::uint32_t full : {1u, 0u})
        for (const std::uint32_t kind : {0xffffffffu, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 9u, 0u})
            for (const auto how : {Nation::none, Nation::same_by_club, Nation::same_by_manager, Nation::differs})
                for (const std::uint32_t region : {0xffffffffu, 5u})
                    for (const auto& magazine : magazine_writes) {
                        if (kind != 5 && &magazine == &magazine_writes[1]) continue;
                        cases.push_back(make(0x106a44d0, {{3, 0}, {4, record_at}, {5, full}, {6, output_at}},
                            join({outlet_record(full, kind, region, 0, 0, 0, 0, 0, 0, 0, 0, 0, true), nation_writes(how), magazine}),
                            "outlet full=" + std::to_string(full) + " kind=" + std::to_string(int(kind)) + " nation=" + std::to_string(int(how)) +
                                " region=" + std::to_string(int(region)) + (&magazine == &magazine_writes[0] ? "" : " (not 8)")));
                    }
    // The stories, with the reply and the staff naming.
    for (const std::uint32_t full : {1u, 0u})
        for (const std::uint32_t story : {0u, 1u, 2u, 3u, 0xffffffffu})
            for (const std::uint32_t flags : {0u, 1u, 4u, 5u})
                for (const std::uint32_t detail : {0u, 1u, 2u, 0xffffffffu})
                    for (const std::uint32_t reply : {0u, 1u, 2u, 3u, 4u, 0xffffffffu})
                        for (const bool users : {false, true})
                            for (const std::uint32_t choice : {0u, 1u}) {
                                if (!full && (flags != 0 || reply != 0 || users || choice)) continue;
                                if (story != 0 && (flags != 0 || detail != 0 || reply != 0 || users || choice)) {
                                    if (!(story <= 2 && flags == 0 && detail == 0 && reply == 0 && !users)) continue;
                                }
                                if (flags && (detail != 0 || (reply != 0 && !(flags == 1)))) continue;
                                auto memory = join({outlet_record(full, 1, 0xffffffffu, story, flags, detail, reply, 1, 0x20, 20, 1, choice, true), nation_writes(Nation::none)});
                                if (users) put32(memory, user_row + 4, staff_table + staff[0].index * 0x70);
                                cases.push_back(make(0x106a44d0, {{3, 0}, {4, record_at}, {5, full}, {6, output_at}}, memory,
                                    "report full=" + std::to_string(full) + " story=" + std::to_string(int(story)) + " flags=" + std::to_string(flags) + " detail=" +
                                        std::to_string(int(detail)) + " reply=" + std::to_string(int(reply)) + (users ? " users" : "") + " choice=" + std::to_string(choice)));
                            }
    // The thanks and dismissal stories by attitude, mood, age and the undecided word.
    for (const std::uint32_t story : {1u, 2u})
        for (const std::uint32_t attitude : {0u, 1u, 2u, 3u, 4u})
            for (const std::uint32_t mood : {0x14u, 0x15u, 0x22u, 0x23u})
                for (const std::uint32_t age : {0x19u, 0x1au})
                    for (const std::uint32_t undecided : {0u, 1u})
                        cases.push_back(make(0x106a44d0, {{3, 0}, {4, record_at}, {5, 1}, {6, output_at}},
                            join({outlet_record(1, 1, 0xffffffffu, story, 0, 0, 0, attitude, mood, age, undecided, 0, true), nation_writes(Nation::none)}),
                            "story " + std::to_string(story) + " attitude=" + std::to_string(attitude) + " mood=" + std::to_string(mood) + " age=" + std::to_string(age) +
                                " undecided=" + std::to_string(undecided)));

    // 0x106a6230: signing_reaction_text. The record: staff (id, three name parts), club, detail, outlet kind, region, amount,
    // variant, a dropped field, [for variant 2: a spokesman (id, three name parts)], a fan flag, another staff (id, two name
    // parts and the last).
    const auto signing_record = [&](std::uint32_t outlet_kind, std::uint32_t region, std::uint32_t variant, std::uint32_t detail, std::uint32_t fans,
                                    std::uint32_t spokesman, bool named) {
        std::vector<std::uint32_t> values{staff[0].index, named ? staff[0].first : staff[0].first + 1, staff[0].middle, staff[0].last, club_with_player, detail,
                                          outlet_kind, region, 5, variant, 77};
        if (variant == 2) values.insert(values.end(), {spokesman, staff[1].first, staff[1].middle, staff[1].last});
        values.insert(values.end(), {fans, staff[2].index, staff[2].first, staff[2].middle, staff[2].last});
        Writes out;
        for (std::size_t n = 0; n < values.size(); ++n) put32(out, record_at + 8 + 4 * std::uint32_t(n), values[n]);
        return out;
    };
    for (const std::uint32_t full : {1u, 0u})
        for (const std::uint32_t kind : {0xffffffffu, 1u, 2u, 3u, 4u, 5u, 6u, 7u})
            for (const auto how : {Nation::none, Nation::same_by_club, Nation::same_by_manager, Nation::differs})
                for (const std::uint32_t region : {0xffffffffu, 5u})
                    for (const auto& magazine : magazine_writes) {
                        if (kind != 5 && &magazine == &magazine_writes[1]) continue;
                        cases.push_back(make(0x106a6230, {{3, 0}, {4, record_at}, {5, full}, {6, output_at}},
                            join({signing_record(kind, region, 1, 0, 0, 0, true), nation_writes(how), magazine}),
                            "signing outlet full=" + std::to_string(full) + " kind=" + std::to_string(int(kind)) + " nation=" + std::to_string(int(how)) +
                                " region=" + std::to_string(int(region)) + (&magazine == &magazine_writes[0] ? "" : " (not 8)")));
                    }
    for (const std::uint32_t full : {1u, 0u})
        for (const std::uint32_t variant : {0u, 1u, 2u, 3u, 0xffffffffu})
            for (const std::uint32_t detail : {0u, 1u, 2u, 0xffffffffu})
                for (const std::uint32_t fans : {0u, 1u})
                    for (const std::uint32_t spokesman : {0xffffffffu, staff[1].index})
                        for (const bool named : {true, false}) {
                            if (variant != 2 && spokesman != 0xffffffffu) continue;
                            if (!full && (detail != 0 || fans)) continue;
                            cases.push_back(make(0x106a6230, {{3, 0}, {4, record_at}, {5, full}, {6, output_at}},
                                join({signing_record(1, 0xffffffffu, variant, detail, fans, spokesman, named), nation_writes(Nation::none)}),
                                "signing full=" + std::to_string(full) + " variant=" + std::to_string(int(variant)) + " detail=" + std::to_string(int(detail)) +
                                    " fans=" + std::to_string(fans) + (spokesman == 0xffffffffu ? " no spokesman" : " spokesman") + (named ? " named" : " unnamed")));
                        }

    // 0x106a7120: competition_win_text. The record: staff (id, three name parts), club, competition, a dropped field, outlet kind,
    // region, type, rating before, a first-season flag, weeks, rating now.
    constexpr std::uint32_t win_competition = 3;
    const auto win_record = [&](std::uint32_t outlet_kind, std::uint32_t region, std::uint32_t type, std::uint32_t before, std::uint32_t first_season,
                                std::uint32_t weeks, std::uint32_t now, std::uint32_t level, bool named) {
        auto out = fields(record_at, {staff[0].index, named ? staff[0].first : staff[0].first + 1, staff[0].middle, staff[0].last, club_with_player, win_competition, 3,
                                      outlet_kind, region, type, before, first_season, weeks, now});
        out.push_back({2, scenario.competition(win_competition) + 0x18, level});
        return out;
    };
    for (const std::uint32_t full : {1u, 0u})
        for (const std::uint32_t kind : {0xffffffffu, 1u, 2u, 3u, 4u, 5u, 6u, 7u})
            for (const auto how : {Nation::none, Nation::same_by_club, Nation::same_by_manager, Nation::differs})
                for (const std::uint32_t region : {0xffffffffu, 5u})
                    for (const auto& magazine : magazine_writes) {
                        if (kind != 5 && &magazine == &magazine_writes[1]) continue;
                        cases.push_back(make(0x106a7120, {{3, 0}, {4, record_at}, {5, full}, {6, output_at}},
                            join({win_record(kind, region, 2, 0x1000, 0, 100, 0x1000, 5, true), nation_writes(how), magazine}),
                            "win outlet full=" + std::to_string(full) + " kind=" + std::to_string(int(kind)) + " nation=" + std::to_string(int(how)) +
                                " region=" + std::to_string(int(region)) + (&magazine == &magazine_writes[0] ? "" : " (not 8)")));
                    }
    for (const std::uint32_t type : {0u, 1u, 2u, 3u, 4u, 0xffffffffu})
        for (const bool named : {true, false})
            for (const std::uint32_t before : {0x1000u, 0x1500u})
                cases.push_back(make(0x106a7120, {{3, 0}, {4, record_at}, {5, 0}, {6, output_at}},
                    join({win_record(1, 0xffffffffu, type, before, 0, 100, 0x1000, 5, named), nation_writes(Nation::none)}),
                    "win short type=" + std::to_string(int(type)) + (named ? " named" : " unnamed") + " before=" + std::to_string(before)));
    for (const std::uint32_t type : {0u, 1u, 2u, 3u, 4u, 0xffffffffu})
        for (const std::uint32_t first_season : {0u, 1u})
            for (const std::uint32_t level : {5u, 20u})
                for (const std::uint32_t weeks : {100u, 400u})
                    for (const std::uint32_t now : {0xe00u, 0x1000u, 0x1400u, 0x1500u, 0x1a00u, 0x1c52u, 0x2000u, 0x3000u})
                        for (const std::uint32_t before : {0x1000u, 0x1300u, 0x1500u, 0x1a5eu, 0x1c52u, 0x1f41u, 0x2000u}) {
                            if (type != 1 && type != 3 && (weeks != 100 || level != 5 || now != 0x1000 || before != 0x1000)) continue;
                            cases.push_back(make(0x106a7120, {{3, 0}, {4, record_at}, {5, 1}, {6, output_at}},
                                join({win_record(1, 0xffffffffu, type, before, first_season, weeks, now, level, (now + before + weeks) % 3 != 0), nation_writes(Nation::none)}),
                                "win type=" + std::to_string(int(type)) + " first=" + std::to_string(first_season) + " level=" + std::to_string(level) + " weeks=" +
                                    std::to_string(weeks) + " now=" + std::to_string(now) + " before=" + std::to_string(before)));
                        }

    // 0x106a8110: player_sale_text. The record: a buyer-side person (id and three name parts), the staff member (same), another
    // staff member (same), two clubs, the story, the outlet kind, a region and the type.
    const auto sale_record = [&](std::uint32_t outlet_kind, std::uint32_t region, std::uint32_t type, std::uint32_t story, bool named) {
        return fields(record_at, {staff[2].index, staff[2].first, staff[2].middle, staff[2].last, staff[0].index, named ? staff[0].first : staff[0].first + 1,
                                  staff[0].middle, staff[0].last, staff[1].index, named ? staff[1].first : staff[1].first + 1, staff[1].middle, staff[1].last,
                                  club_with_player, club_b, story, outlet_kind, region, type});
    };
    for (const std::uint32_t full : {1u, 0u})
        for (const std::uint32_t kind : {0xffffffffu, 1u, 2u, 3u, 4u, 5u, 6u, 7u})
            for (const auto how : {Nation::none, Nation::same_by_club, Nation::same_by_manager, Nation::differs})
                for (const std::uint32_t region : {0xffffffffu, 5u})
                    for (const auto& magazine : magazine_writes) {
                        if (kind != 5 && &magazine == &magazine_writes[1]) continue;
                        cases.push_back(make(0x106a8110, {{3, 0}, {4, record_at}, {5, full}, {6, output_at}},
                            join({sale_record(kind, region, 0, 0, true), nation_writes(how), magazine}),
                            "sale outlet full=" + std::to_string(full) + " kind=" + std::to_string(int(kind)) + " nation=" + std::to_string(int(how)) +
                                " region=" + std::to_string(int(region)) + (&magazine == &magazine_writes[0] ? "" : " (not 8)")));
                    }
    for (const std::uint32_t full : {1u, 0u})
        for (const std::uint32_t type : {0u, 1u, 2u, 3u})
            for (const std::uint32_t story : {0u, 1u, 2u, 3u, 0xffffffffu})
                for (const bool named : {true, false})
                    cases.push_back(make(0x106a8110, {{3, 0}, {4, record_at}, {5, full}, {6, output_at}},
                        join({sale_record(1, 0xffffffffu, type, story, named), nation_writes(Nation::none)}),
                        "sale full=" + std::to_string(full) + " type=" + std::to_string(type) + " story=" + std::to_string(int(story)) + (named ? " named" : " unnamed")));

    // 0x1069ace0: transfer_rumour_text. The record: staff (id, three name parts), another club, the story type, money, a reply
    // kind, two amount words, a second staff member (id, three name parts), an undecided flag, the number of clubs and the clubs.
    const auto rumour_record = [&](std::uint32_t story, std::uint32_t other_club, std::uint32_t money, std::uint32_t reply, std::uint32_t second,
                                   std::uint32_t undecided, std::uint32_t count, std::uint32_t staff_index, bool named, std::uint32_t lead_club) {
        const auto& s = staff[staff_index % 3];
        const auto& t = staff[1];
        std::vector<std::uint32_t> values{s.index, named ? s.first : s.first + 1, s.middle, s.last, other_club, story, money, reply, 100, 5, second,
                                          t.first, t.middle, t.last, undecided, count};
        for (std::uint32_t n = 0; n < count && n < 6; ++n) values.push_back(n == 0 ? lead_club : 7 + (n * 2) % 9);
        Writes out;
        // The list is read at index 0 even when the record has no club: a value in the frame the routine never wrote.
        put32(out, std::uint32_t(scenario.s.r<4>(4)) - 0x5c0 + 0x54, lead_club);
        for (std::size_t n = 0; n < values.size(); ++n) put32(out, record_at + 8 + 4 * std::uint32_t(n), values[n]);
        return out;
    };
    const auto asked_writes = [&](bool asked) {
        Writes out;
        if (asked) put32(out, user_row + 4, staff_table + staff[1].index * 0x70);
        return out;
    };
    for (const std::uint32_t story : {0x24u, 0u, 1u, 2u, 0xffffffffu})
        for (const std::uint32_t other_club : {0xffffffffu, 8u, 9u, 10u, 11u, 12u, 13u})
            for (const std::uint32_t money : {0u, 5u})
                for (const std::uint32_t count : {0u, 1u, 2u, 3u, 4u, 5u, 6u})
                    for (const auto how : {Nation::none, Nation::same_by_club, Nation::differs}) {
                        if (other_club >= 10 && other_club != 10 && other_club != 13 && count != 3) continue;
                        // (A story that names the user's region while he has no manager reads address 0 + 0x116 in the original: both fault.)
                        if (story == 1 && how == Nation::none) continue;
                        if (how != Nation::none && (count != 2 && count != 5 && other_club == 0xffffffffu)) continue;
                        cases.push_back(make(0x1069ace0, {{3, 0}, {4, record_at}, {5, 1}, {6, output_at}},
                            join({rumour_record(story, other_club, money, 0, 0xffffffffu, 0, count, count, count != 4, club_with_player), nation_writes(how)}),
                            "rumour story=" + std::to_string(int(story)) + " other=" + std::to_string(int(other_club)) + " money=" + std::to_string(money) +
                                " count=" + std::to_string(count) + " nation=" + std::to_string(int(how))));
                    }
    for (const std::uint32_t reply : {0u, 1u, 2u, 3u, 4u, 0xffffffffu})
        for (const bool asked : {false, true})
            for (const std::uint32_t second : {0xffffffffu, 0u})
                for (const std::uint32_t undecided : {0u, 1u})
                    for (const std::uint32_t money : {0u, 5u}) {
                        if (asked && second == 0xffffffffu) continue;
                        const auto second_index = second == 0 ? staff[1].index : second;
                        cases.push_back(make(0x1069ace0, {{3, 0}, {4, record_at}, {5, 1}, {6, output_at}},
                            join({rumour_record(1, 8, money, reply, second_index, undecided, 2, 0, true, club_with_player), nation_writes(Nation::none), asked_writes(asked)}),
                            "rumour reply=" + std::to_string(int(reply)) + (asked ? " asked" : "") + " second=" + std::to_string(int(second_index)) + " undecided=" +
                                std::to_string(undecided) + " money=" + std::to_string(money)));
                    }
    for (const std::uint32_t story : {0x24u, 0u, 1u, 2u})
        for (const std::uint32_t staff_index : {0u, 1u, 2u})
            for (const std::uint32_t count : {0u, 1u, 2u})
                for (const std::uint32_t other_club : {0xffffffffu, 8u})
                    for (const std::uint32_t lead_club : {7u, 0xffffffffu})
                        cases.push_back(make(0x1069ace0, {{3, 0}, {4, record_at}, {5, 0}, {6, output_at}},
                            join({rumour_record(story, other_club, 0, 0, 0xffffffffu, 0, count, staff_index, staff_index != 1, lead_club), nation_writes(Nation::none)}),
                            "rumour short story=" + std::to_string(int(story)) + " staff=" + std::to_string(staff_index) + " count=" + std::to_string(count) +
                                " other=" + std::to_string(int(other_club)) + " lead=" + std::to_string(int(lead_club))));
}

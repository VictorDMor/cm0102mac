// FINANCE.CPP checker cases for the match day block: a fixture's income (0x100f95e0), the home match's takings
// (0x100f7270), the split of a played fixture's income (0x100f8780) and the ground works (0x100f7cf0).
//
// The clubs, accounts, the finance manager and the sample table are the career's own. A fixture, the match record,
// a club's board, owner, league and division records are built in the scratch area (the game keeps them elsewhere
// and only their fields are read) and the club records are pointed at them through `Case::memory`.
#include "finance_checks.hpp"
#include <string>

namespace cm::aot::finance_checks {
namespace {
constexpr std::uint32_t matchday_takings = 0x100f7270, expand_ground = 0x100f7cf0, split_income = 0x100f8780,
                        fixture_income = 0x100f95e0;
constexpr std::uint32_t callee_account = 0x10103510, club_inactive = 0x100c7d00, pace_test = 0x10494d20, board_news = 0x10103380;
constexpr std::uint32_t S = scratch_base + 0x40000;
constexpr std::uint32_t fixture = S, record = S + 0x100, competition = S + 0x200, league = S + 0x300, division = S + 0x400,
                        board = S + 0x600, owner = S + 0x700, profile = S + 0x740;

std::uint32_t sx16(std::int32_t v) { return std::uint32_t(v) & 0xffff; }

// One situation of a home club (and its opponent): every field the four routines read.
struct Situation {
    unsigned home = 0, away = 1;
    std::int32_t home_reputation = 1500, away_reputation = 900;
    bool home_national = false, away_national = false;
    bool home_league = true;           // the club's league record is the fixture's competition
    std::uint32_t league_word = 7, group_word = 9;
    bool has_division = true;
    std::int32_t level = 3;
    std::uint32_t group = 5;
    bool has_board = true;
    std::int32_t seats = 20000, roofed = 4000, peak = 25000;
    bool has_owner = true, has_profile = true;
    std::int32_t patience = 5;
    bool tie = false, has_competition = true;
    bool competition_differs = false;  // the fixture's competition is not the home club's league record
    std::int32_t round = 3;
    std::int32_t band = 1, kind = 5, guaranteed = 0;
    std::int32_t crowd = 12000, carried = 3000;
    bool works = false;
    std::int32_t worth = 40000;
    std::uint32_t sample_offset = 0;   // halfwords from the table's first entry
    std::uint32_t sample_base = 100;
    std::uint32_t sold = 0, gate = 0;  // the account's tickets sold and gate level (0x100f7270)
    std::vector<Write> extra = {};
};

std::vector<Write> edits(const Scenario& sc, const Situation& s) {
    std::vector<Write> w;
    const auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { w.push_back({size, at, value}); };
    const auto home = sc.club(s.home), away = sc.club(s.away);
    put(4, fixture + 0x14, !s.has_competition ? 0 : s.competition_differs ? competition : league);
    put(4, fixture + 0x1c, home);
    put(4, fixture + 0x20, away);
    put(1, fixture + 0x3a, std::uint32_t(s.kind));
    put(1, fixture + 0x3b, std::uint32_t(s.band));
    put(1, fixture + 0x3c, s.tie);
    put(1, fixture + 0x40, std::uint32_t(s.guaranteed));
    put(4, record + 0x34, std::uint32_t(s.crowd));
    put(4, record + 0x38, std::uint32_t(s.carried));
    put(2, competition + 0x18, sx16(s.round));
    put(2, league + 0x18, sx16(s.round));
    put(4, league, s.league_word);
    put(4, division + 0x68, s.group);
    put(1, division + 0x11c, std::uint32_t(s.level));
    put(4, board + 4, std::uint32_t(s.seats));
    put(4, board + 8, std::uint32_t(s.roofed));
    put(4, board + 0xc, std::uint32_t(s.peak));
    put(4, owner + 0x1c, s.has_profile ? profile : 0);
    put(1, profile + 0x40, std::uint32_t(s.patience));
    // The home club's records.
    put(4, home + 0x30, s.home_league ? league : 0);
    put(4, home + 0x2c, s.has_division ? division : 0);
    put(4, home + 0x3c, s.has_board ? board : 0);
    put(4, home + 0x80, s.has_owner ? owner : 0);
    put(2, home + 0x1ec, sx16(s.home_reputation));
    put(1, home + 0x23d, s.home_national);
    put(1, home + 0x23e, s.works);
    put(4, home + 8, std::uint32_t(s.worth));
    put(2, away + 0x1ec, sx16(s.away_reputation));
    put(1, away + 0x23d, s.away_national);
    // The cells that name a well-known league group (0x100f7270, 0x100f95e0) agree with the records on request.
    put(4, sc.toc(-0x7488), s.league_word);
    put(4, sc.toc(-0x7af4), s.group);
    // The sample table: where its cursor stands and what its base is.
    const auto first = sc.toc(-0x7b8c), last = sc.global(-0x7b88);
    const auto at = first + 2 * s.sample_offset;
    put(4, sc.toc(-0x7b84), at <= last ? at : last);
    put(4, sc.toc(-0x7b90), s.sample_base);
    for (const auto& write : s.extra) w.push_back(write);
    return w;
}

std::string name(const Situation& s) {
    return "home " + std::to_string(s.home) + " rep " + std::to_string(s.home_reputation) + " level " + std::to_string(s.level) +
           " sample " + std::to_string(s.sample_offset);
}
}

FINANCE_CASES("matchday") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ab8);
    const auto accounts = scenario.r32(manager);
    const auto clubs = scenario.global(-0x7b50);
    const auto account = [&](unsigned index) { return accounts + index * 0x168; };
    const unsigned homes[] = {0, 3, 57 % clubs, clubs / 2};
    const std::uint32_t last_offset = (scenario.global(-0x7b88) - scenario.toc(-0x7b8c)) / 2;
    const std::int32_t reputations[] = {0, 1, 499, 500, 1250, 4999, 32767, -1, -500, -32768};
    const auto clubs_cell_value = [&](unsigned i) { return scenario.r32(scenario.club(i)); };  // the club's index word

    // ---- 0x100f95e0: a fixture's income ----
    {
        const auto amounts = {1, 1000, 25000, -5000, 7'000'000, 0x7fffffff};
        const auto emit = [&](const Situation& s, std::uint32_t fixture_pointer, std::int32_t base, const std::string& label,
                              std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            cases.push_back({.entry = fixture_income, .registers = {{3, manager}, {4, fixture_pointer}, {5, std::uint32_t(base)}},
                             .memory = edits(scenario, s), .label = label + " " + name(s), .answers = std::move(answers)});
        };
        emit(Situation{}, 0, 1000, "no fixture");
        emit(Situation{}, fixture, 0, "no amount");
        emit(Situation{}, fixture, 1000, "home club inactive", {{club_inactive, 1}});
        for (const auto home : homes)
            for (const bool tie : {false, true})
                for (const bool with_competition : {false, true})
                    for (const std::uint32_t paced : {0u, 1u})
                        for (const int national : {0, 1, 2})
                            for (const bool in_league : {false, true})
                              for (const bool differs : {false, true}) {
                                Situation s;
                                s.competition_differs = differs;
                                s.home = home;
                                s.away = home + 1;
                                s.tie = tie;
                                s.has_competition = with_competition;
                                s.home_national = national == 1;
                                s.away_national = national == 2;
                                s.home_league = in_league;
                                emit(s, fixture, 7777,
                                     std::string("tie ") + char('0' + tie) + " comp " + char('0' + with_competition) + " paced " +
                                         char('0' + paced) + " national " + char('0' + national) + " league " + char('0' + in_league) +
                                         " differs " + char('0' + differs),
                                     {{pace_test, paced}});
                            }
        // The home club null (the original reads through it later and faults on both sides, at 0x100f9898 or
        // 0x100f96f0), and the account of the home club null (0x10103510 answered 0: no price-level adjustment).
        for (const bool with_competition : {false, true})
            for (const bool tie : {false, true}) {
                Situation s;
                s.has_competition = with_competition;
                s.tie = tie;
                s.extra = {{4, fixture + 0x1c, 0}};
                emit(s, fixture, 5000, "home club null, comp " + std::to_string(with_competition), {{pace_test, 0}});
            }
        for (const bool differs : {false, true}) {
            Situation s;
            s.has_competition = false;
            s.home_league = false;
            s.competition_differs = differs;
            emit(s, fixture, 5000, "no account for the home club", {{callee_account, 0}});
        }
        // The price level (account +0x160 is the real account's) and the reputation steps.
        for (const auto reputation : reputations)
            for (const auto base : amounts) {
                Situation s;
                s.home_reputation = reputation;
                s.has_competition = false;
                emit(s, fixture, base, "reputation " + std::to_string(reputation) + " base " + std::to_string(base));
                s.has_competition = true;
                s.home_league = true;
                emit(s, fixture, base, "league of the fixture, base " + std::to_string(base) + " rep " + std::to_string(reputation), {{pace_test, 0}});
            }
        // Level, then the factor chosen by the club (the cells hold a club index each) or its division's group.
        for (const bool has_division : {false, true})
            for (const std::int32_t level : {0, 1, 2, 3, -1, 127}) {
                Situation s;
                s.has_division = has_division;
                s.level = level;
                s.home_league = false;
                emit(s, fixture, 90000, "level " + std::to_string(level) + " division " + std::to_string(has_division), {{pace_test, 1}});
            }
        const std::int32_t cells[] = {-0x748c, -0x7490, -0x7494, -0x7498, -0x749c};
        for (const auto slot : cells)
            for (const bool in_league : {false, true}) {
                Situation s;
                s.home_league = in_league;
                s.league_word = 11;
                s.extra = {{4, scenario.toc(slot), clubs_cell_value(s.home)}, {4, scenario.toc(-0x7488), 12}};
                emit(s, fixture, 123456, "club cell " + std::to_string(slot) + " league " + std::to_string(in_league), {{pace_test, 1}});
            }
        {
            Situation s;
            s.home_league = false;
            s.extra = {{4, scenario.toc(-0x7af4), s.group}};
            emit(s, fixture, 123456, "division group matches", {{pace_test, 1}});
            s.group = 6;
            emit(s, fixture, 123456, "division group differs", {{pace_test, 1}});
        }
    }

    // ---- 0x100f7270: the home match's takings ----
    {
        const auto emit = [&](const Situation& s, std::uint32_t credit, const std::string& label) {
            auto memory = edits(scenario, s);
            memory.push_back({4, account(s.home) + 0x18, s.sold});
            memory.push_back({1, account(s.home) + 0x164, s.gate});
            cases.push_back({.entry = matchday_takings, .registers = {{3, account(s.home)}, {4, credit}}, .memory = std::move(memory),
                             .label = label + " " + name(s)});
        };
        for (const auto home : homes)
            for (const std::uint32_t credit : {0u, 1u, 0x100u, 0xffu}) {
                Situation s;
                s.home = home;
                emit(s, credit, "takings credit " + std::to_string(credit));
            }
        for (const std::int32_t seats : {0, 1, 1499, 1500, 1501, 20000, 60000, -5})
            for (const bool with_board : {false, true}) {
                Situation s;
                s.seats = seats;
                s.has_board = with_board;
                emit(s, 1, "board seats " + std::to_string(seats) + " board " + std::to_string(with_board));
            }
        for (const std::int32_t reputation : {0, 100, 599, 600, -5})
            for (const std::int32_t seats : {0, 10, 1499, 1500})
                for (const std::uint32_t sold : {0u, 16000u}) {
                    Situation s;
                    s.home_reputation = reputation;
                    s.seats = seats;
                    s.sold = sold;
                    emit(s, 1, "small board, reputation " + std::to_string(reputation) + " seats " + std::to_string(seats));
                }
        // The club record null (the original reads club+8 next and faults on both sides, at 0x100f7358).
        {
            Situation s;
            s.home = 3;
            s.extra = {{4, scenario.toc(-0x7be8), 0}, {4, account(3) + 8, 0}};
            emit(s, 1, "club record null");
        }
        // No seats on the board while the crowd is negative (a negative worth) and the tickets sold lower still.
        for (const std::uint32_t sold : {0x80000000u, 0xf0000000u})
            for (const std::int32_t worth : {-5'000'000, -300, -3}) {
                Situation s;
                s.home = 3;
                s.seats = 0;
                s.worth = worth;
                s.sold = sold;
                emit(s, 1, "no seats, worth " + std::to_string(worth));
            }
        for (const std::uint32_t sold : {15000u, 15400u, 16000u, 17000u, 17400u, 17499u, 17500u})
            for (const std::int32_t seats : {20000, 30000}) {
                Situation s;
                s.home = 3;
                s.sold = sold;
                s.seats = seats;
                emit(s, 1, "sold close to the crowd " + std::to_string(sold) + " seats " + std::to_string(seats));
            }
        // A full house: a big crowd against a small ground, the tickets sold already near what it holds (the price scale
        // then passes the limit and is capped).
        for (const std::int32_t seats : {1500, 2000, 3000, 5000, 8000})
            for (const std::int32_t percent : {50, 80, 90, 99, 100})
                for (const std::int32_t level : {1, 8}) {
                    Situation s;
                    s.home = 3;
                    s.worth = 5'000'000;
                    s.seats = seats;
                    s.sold = std::uint32_t(seats * 7 / 8 * percent / 100);
                    s.gate = std::uint32_t(level);
                    emit(s, 1, "full house, seats " + std::to_string(seats) + " sold " + std::to_string(percent) + "%");
                }
        for (const auto reputation : reputations)
            for (const std::int32_t worth : {0, 40000, 5'000'000, -3})
                for (const bool in_league : {false, true}) {
                    Situation s;
                    s.home = 3;
                    s.home_reputation = reputation;
                    s.worth = worth;
                    s.home_league = in_league;
                    emit(s, 1, "reputation " + std::to_string(reputation) + " worth " + std::to_string(worth) + " league " + std::to_string(in_league));
                }
        for (const std::uint32_t sold : {0u, 1u, 100u, 5000u, 19999u, 20000u, 0x7fffffffu, 0xffffffffu})
            for (const std::uint32_t gate : {0u, 1u, 7u, 8u, 9u, 127u, 128u, 255u}) {
                Situation s;
                s.home = 3;
                s.sold = sold;
                s.gate = gate;
                emit(s, 1, "sold " + std::to_string(sold) + " gate " + std::to_string(gate));
            }
        // Which sample the draws and the sale use: every place of the table, the end included.
        for (std::uint32_t offset = 0; offset <= last_offset && offset < 40; ++offset)
            for (const std::int32_t reputation : {1500, 7, 0, -300}) {
                Situation s;
                s.home = 3;
                s.home_reputation = reputation;
                s.sample_offset = offset;
                s.sample_base = offset * 37 % 2000;
                emit(s, 1, "sample place " + std::to_string(offset));
            }
        for (const auto d : {std::int32_t(-2), 0, 1, 2, 3})
            for (const bool in_league : {false, true}) {
                Situation s;
                s.home = 3;
                s.level = d;
                s.home_league = in_league;
                s.has_division = d != 0;
                emit(s, 1, "level " + std::to_string(d) + " league " + std::to_string(in_league));
            }
        for (const auto slot : {-0x748c, -0x7490, -0x7494, -0x7498, -0x749c, -0x7488}) {
            Situation s;
            s.home = 3;
            s.home_league = slot == -0x7488;
            s.extra = {{4, scenario.toc(slot), clubs_cell_value(3)}};
            if (slot == -0x7488) s.extra = {};
            emit(s, 1, "factor cell " + std::to_string(slot));
        }
        {
            Situation s;
            s.home = 3;
            s.home_league = false;
            emit(s, 1, "division group matches");
            s.group = 6;
            emit(s, 1, "division group differs");
        }
    }

    // ---- 0x100f8780: the split of a played fixture's income ----
    {
        const auto emit = [&](const Situation& s, std::uint32_t fixture_pointer, std::uint32_t record_pointer, const std::string& label,
                              std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            cases.push_back({.entry = split_income, .registers = {{3, manager}, {4, fixture_pointer}, {5, record_pointer}},
                             .memory = edits(scenario, s), .label = label + " " + name(s), .answers = std::move(answers)});
        };
        emit(Situation{}, fixture, 0, "no record");
        emit(Situation{}, 0, record, "no fixture");
        emit(Situation{}, fixture, record, "home inactive", {{club_inactive, 1}});
        {
            Situation s;
            cases.push_back({.entry = split_income, .registers = {{3, manager}, {4, fixture}, {5, record}}, .memory = edits(scenario, s),
                             .label = "away inactive", .nth_answers = {{club_inactive, 2, 1}}});
        }
        for (const auto home : homes)
            for (const std::int32_t crowd : {0, 1, 4999, 5000, 5001, 10000, 10001, 20000, 20001, 55000, -4})
                for (const int national : {0, 1, 2}) {
                    Situation s;
                    s.home = home;
                    s.away = home + 1;
                    s.crowd = crowd;
                    s.home_national = national == 1;
                    s.away_national = national == 2;
                    s.guaranteed = crowd & 1;
                    emit(s, fixture, record, "crowd " + std::to_string(crowd) + " national " + std::to_string(national));
                }
        // A club of the fixture null: both sides fault (home: 0x100f89a4 reads club+0x23d; away: the account stays 0
        // and the first booking writes through it, 0x100f92b8 or 0x100f9300).
        for (const std::int32_t national : {0, 1}) {
            Situation s;
            s.home_national = national;
            s.extra = {{4, fixture + 0x1c, 0}};
            emit(s, fixture, record, "home club null national " + std::to_string(national));
            s.extra = {{4, fixture + 0x20, 0}};
            emit(s, fixture, record, "away club null national " + std::to_string(national));
        }
        // A negative sample base lets the crowd bonus's divisor (sample % 10 + 2) reach zero: both sides stop (0x100f8964).
        for (std::uint32_t k = 1; k <= 60; ++k)
            for (std::uint32_t offset : {0u, 1u, 2u}) {
                Situation s;
                s.sample_offset = offset;
                s.sample_base = 0u - k;
                emit(s, fixture, record, "negative base " + std::to_string(k));
            }
        // The levies: the home club's reputation (zero or not) in each family of club, over the table's places.
        for (std::uint32_t offset = 0; offset <= last_offset && offset < 24; ++offset)
            for (const bool national : {false, true})
                for (const std::int32_t reputation : {0, 1, 3, 120, 32767, -20}) {
                    Situation s;
                    s.home_national = national;
                    s.away_national = !national;
                    s.home_reputation = reputation;
                    s.sample_offset = offset;
                    s.sample_base = offset * 91 % 3000;
                    s.crowd = 6000 + std::int32_t(offset) * 211;
                    emit(s, fixture, record, "levies reputation " + std::to_string(reputation) + " national " + std::to_string(national));
                }
        // The broadcasting share: every band, then every kind of round with its reputations and rounds.
        for (const std::int32_t band : {-3, -2, -1, 0, 1, 2, 3, 100})
            for (const std::int32_t kind : {0, 4, 5, 6, 7, 8, 9, -1})
                for (const std::int32_t round : {0, 3, 5, 6, 12, -4}) {
                    Situation s;
                    s.band = band;
                    s.kind = kind;
                    s.round = round;
                    s.home_reputation = 5000 + round * 300;
                    s.away_reputation = 2000 + round * 700;
                    emit(s, fixture, record, "band " + std::to_string(band) + " kind " + std::to_string(kind) + " round " + std::to_string(round));
                }
        for (const std::int32_t home_reputation : {0, 1000, 3000, 4000, 7500, 3750, 3751, 5000, 5001, 9000})
            for (const std::int32_t away_reputation : {0, 1000, 2500, 3750, 4000, 5000, 1}) {
                Situation s;
                s.band = 100;
                s.kind = 7 + (home_reputation & 1);
                s.home_reputation = home_reputation;
                s.away_reputation = away_reputation;
                emit(s, fixture, record, "finals reputations " + std::to_string(home_reputation) + "+" + std::to_string(away_reputation));
            }
        // The level of the home club's division and the groups that count as a lower tier.
        for (const bool has_division : {false, true})
            for (const std::int32_t level : {0, 1, 2, 3, -1})
                for (const std::uint32_t group_cell : {0u, 1u, 2u, 3u}) {
                    Situation s;
                    s.has_division = has_division;
                    s.level = level;
                    s.group = 5;
                    s.extra = {};
                    if (group_cell) s.extra.push_back({4, scenario.toc(group_cell == 1 ? -0x757c : group_cell == 2 ? -0x7574 : -0x7578), 5});
                    emit(s, fixture, record, "level " + std::to_string(level) + " division " + std::to_string(has_division) + " group cell " + std::to_string(group_cell));
                }
        // League match (the home club's league is the fixture's competition) against a cup tie; the stadium works.
        for (const bool tie : {false, true})
            for (const bool in_league : {false, true})
                for (const bool works : {false, true})
                    for (const bool with_board : {false, true})
                        for (std::uint32_t offset : {0u, 3u, last_offset}) {
                            Situation s;
                            s.competition_differs = offset == 3;
                            s.tie = tie;
                            s.home_league = in_league;
                            s.works = works;
                            s.has_board = with_board;
                            s.seats = 5000 + 4000 * std::int32_t(offset);
                            s.sample_offset = offset;
                            emit(s, fixture, record, std::string("league ") + char('0' + in_league) + " tie " + char('0' + tie) +
                                                         " works " + char('0' + works) + " board " + char('0' + with_board));
                        }
        for (const std::int32_t seats : {0, 1, 14, 15, 30000, -9, 0x7fffffff})
            for (std::uint32_t offset = 0; offset <= last_offset && offset < 18; ++offset) {
                Situation s;
                s.works = true;
                s.home_league = true;
                s.seats = seats;
                s.sample_offset = offset;
                s.sample_base = offset * 53 % 700;
                emit(s, fixture, record, "works seats " + std::to_string(seats));
            }
        // The crowd bonus's draws and the carried figure over every sample place; large and negative figures.
        for (std::uint32_t offset = 0; offset <= last_offset && offset < 30; ++offset)
            for (const std::int32_t carried : {0, 7, -500, 0x7fffffff}) {
                Situation s;
                s.sample_offset = offset;
                s.carried = carried;
                s.crowd = 3 + std::int32_t(offset) * 997;
                emit(s, fixture, record, "carried " + std::to_string(carried));
            }
    }

    // ---- 0x100f7cf0: the ground works ----
    {
        const std::pair<std::uint32_t, std::uint32_t> answer_news{board_news, 0};
        const auto emit = [&](const Situation& s, std::int32_t wanted, std::int32_t planned, std::uint32_t with_owner,
                              std::uint64_t balance, const std::string& label,
                              std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            auto memory = edits(scenario, s);
            memory.push_back({4, account(s.home), std::uint32_t(balance >> 32)});
            memory.push_back({4, account(s.home) + 4, std::uint32_t(balance)});
            if (answers.empty()) answers = {answer_news};
            cases.push_back({.entry = expand_ground,
                             .registers = {{3, account(s.home)}, {4, std::uint32_t(wanted)}, {5, std::uint32_t(planned)}, {6, 0x1234}, {7, with_owner}},
                             .memory = std::move(memory), .label = label + " " + name(s), .answers = std::move(answers)});
        };
        const std::uint64_t rich = 50'000'000'000ull, poor = 0, broke = std::uint64_t(-5'000'000'000ll);
        emit(Situation{}, 10, 10, 0, rich, "club inactive", {{club_inactive, 1}, answer_news});
        {
            Situation s;
            s.has_board = false;
            emit(s, 20000, 20000, 0, rich, "no board");
        }
        for (const std::uint32_t with_owner : {0u, 1u, 0x100u, 0x80u, 0xffu})
            for (const std::int32_t seats : {0, 1000, 20000, 119999, 120000, 120001, 200000})
                for (const std::int32_t roofed : {0, 500, 4000, 25000}) {
                    // wanted == planned, and planned past wanted; the extra seats from a few hundred to some thousands.
                    for (const std::int32_t wanted : {roofed - 5, roofed, roofed + 700, seats - 50, seats, seats + 3000})
                        for (const std::int32_t planned : {wanted, wanted + 1500, wanted - 200}) {
                            Situation s;
                            s.seats = seats;
                            s.roofed = roofed;
                            s.peak = seats + 100;
                            emit(s, wanted, planned, with_owner, rich,
                                 "owner " + std::to_string(with_owner) + " seats " + std::to_string(seats) + " roofed " + std::to_string(roofed) +
                                     " wanted " + std::to_string(wanted) + " planned " + std::to_string(planned));
                        }
                }
        // The owner's patience and what the club can pay: cost under and over the patience's limit and the balance.
        for (const std::int32_t patience : {-128, -1, 0, 1, 2, 5, 19, 20, 21, 127})
            for (const std::uint64_t balance : {rich, poor, broke, std::uint64_t(1'000'000), std::uint64_t(300'000'000)})
                for (const std::int32_t extra_seats : {50, 1000, 7000, 40000})
                    for (std::uint32_t offset = 0; offset <= last_offset && offset < 8; ++offset) {
                        Situation s;
                        s.seats = 20000;
                        s.roofed = 4000;
                        s.patience = patience;
                        s.sample_offset = offset;
                        s.sample_base = offset * 29 % 800;
                        emit(s, 4000 + extra_seats, 4000 + extra_seats + 600, 1, balance,
                             "patience " + std::to_string(patience) + " balance " + std::to_string(std::int64_t(balance)) + " seats " + std::to_string(extra_seats));
                    }
        for (const bool has_owner : {false, true})
            for (const bool has_profile : {false, true})
                for (const std::uint64_t balance : {rich, poor, broke, std::uint64_t(2'000'000), std::uint64_t(0x1'0000'0000ull)})
                    for (const std::int32_t extra_seats : {10, 800, 9000, 30000})
                        for (std::uint32_t offset = 0; offset <= last_offset && offset < 12; ++offset) {
                            Situation s;
                            s.has_owner = has_owner;
                            s.has_profile = has_profile;
                            s.seats = 20000;
                            s.roofed = 4000;
                            s.sample_offset = offset;
                            s.sample_base = offset * 7 % 500;
                            emit(s, 4000 + extra_seats, 4000 + extra_seats, 1, balance,
                                 "owner " + std::to_string(has_owner) + " profile " + std::to_string(has_profile) + " balance " +
                                     std::to_string(std::int64_t(balance)) + " seats " + std::to_string(extra_seats));
                        }
        for (std::uint32_t offset = 0; offset <= last_offset && offset < 30; ++offset)
            for (const std::uint32_t base : {0u, 1u, 2u, 3u, 4u}) {
                Situation s;
                s.has_owner = false;
                s.sample_offset = offset;
                s.sample_base = base;
                emit(s, 6000, 6900, 1, poor, "gamble, sample place " + std::to_string(offset) + " base " + std::to_string(base));
            }
        // The owner's patience read again after the draw: the draw's generator call lowers it (poke), so a cost that
        // passed the first test is over the new limit.
        for (const std::int32_t patience : {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16})
            for (const std::int32_t lowered : {0, 1})
                for (std::uint32_t offset : {last_offset}) {
                    Situation s;
                    s.patience = patience;
                    s.seats = 4050;
                    s.sample_offset = offset;
                    auto memory = edits(scenario, s);
                    memory.push_back({4, account(s.home), 0});
                    memory.push_back({4, account(s.home) + 4, 0});
                    cases.push_back({.entry = expand_ground,
                                     .registers = {{3, account(s.home)}, {4, 4050}, {5, 4050}, {6, 0x1234}, {7, 1}},
                                     .memory = std::move(memory), .label = "patience lowered by the draw " + std::to_string(patience),
                                     .answers = {answer_news},
                                     .pokes = {{0x1077b770, {{1, profile + 0x40, std::uint32_t(lowered)}}}}});
                }
        // No owner, a cost under a million that the club cannot cover: no gamble.
        for (const std::int32_t extra_seats : {10, 50, 200})
            for (const std::uint64_t balance : {poor, broke}) {
                Situation s;
                s.has_owner = false;
                s.seats = 4000 + extra_seats;  // no seats beyond the wanted ones, so the cost stays small
                emit(s, 4000 + extra_seats, 4000 + extra_seats, 1, balance, "small cost, no owner, extra " + std::to_string(extra_seats));
            }
        // The same without answering the board's news (its own body runs on both sides).
        {
            Situation s;
            emit(s, 6000, 6000, 0, rich, "news unanswered", {{club_inactive, 0}});
        }
    }
}
}

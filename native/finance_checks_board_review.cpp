// FINANCE.CPP checker cases for the board block: the board's review by wealth band (0x10101400).
//
// The club and its account are the career's own; each case sets the fields the review reads (the account's stage and
// patience counter, the club's flags and patience byte, its league row, its board record) and scripts the sample table.
// The routines of other blocks (the donation, the news builders, the club's contract wishes) are answered where a case
// needs a chosen answer, and run as the originals in the real cases; the three routines of this block run as C++ on the
// rewrite's side and as the originals on the other.
#include "finance_checks_board_support.hpp"
#include <limits>

namespace cm::aot::finance_checks {
using namespace board;
namespace {
constexpr std::uint32_t review = 0x10101400, donation = 0x100fe640, clear_news = 0x10103270, trouble_news = 0x10102a70,
                        spend_news = 0x10102590, calm_news = 0x10102e00, wishes = 0x1014c2e0, who_is = 0x10082f20,
                        record_of = 0x10082f90, cost_of = 0x10084be0;
constexpr std::uint32_t persons = S + 0x2000, wish_answer = S + 0x1800, record_answer = S + 0x1900;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
}

FINANCE_CASES("board-review") {
    if (scenario.match) return;
    Pusher push(scenario, cases, review, 0x4e71);
    const auto club_index = [&](unsigned club) { return scenario.r32(scenario.club(club)); };
    const Answers quiet = {{donation, 0}, {clear_news, 0}, {trouble_news, 0}, {spend_news, 0}, {calm_news, 0}, {wishes, 1}};

    // The real routines on a few clubs, as the game calls it (acted 1 and 0).
    {
        std::mt19937 rng(5);
        const auto manager = scenario.global(-0x7ab8);
        const auto accounts = scenario.r32(manager);
        for (const unsigned other : {1u, 20u, 300u, 1000u})
            for (const std::uint32_t acted : {0u, 1u}) {
                auto memory = Memory{};
                script(scenario, memory, random_values(rng, 12));
                cases.push_back({.entry = review, .registers = {{3, manager}, {4, scenario.club(other)}, {5, acted}},
                                 .memory = std::move(memory.writes), .label = "real club " + std::to_string(other) + " acted " + std::to_string(acted)});
                (void)accounts;
            }
    }

    // Each band with each stage, acted in its forms, the patience counter around its thresholds.
    for (const int band : {-2, -1, 0, 1, 2})
        for (const int stage : {0, 1, 2, 3})
            for (const std::int64_t acted : {0, 1, -1, 0x100, 0x180})
                for (const int counter : {0, 1, 5, 6, 10, 11, 0x32, 0x33, 120, 200})
                    for (const int works : {0, 1}) {
                        if (acted != 1 && (counter != 11 || works)) continue;
                        Setup s;
                        s.band = band;
                        s.stage = stage;
                        s.counter = counter;
                        s.works = works;
                        s.has_league = 0;
                        push(s, "band " + std::to_string(band) + " stage " + std::to_string(stage) + " acted " + std::to_string(acted) + " counter " +
                                std::to_string(counter) + " works " + std::to_string(works), 2, quiet, acted);
                    }
    // The donation answers 1, the gifts' gates: the bands that call them, with the real news builders.
    for (const int band : {-2, -1, 0, 1})
        for (const std::uint32_t donated : {0u, 1u, 0x100u})
            for (const int patience : {0x14, 0x13, 0, 7})
                for (const std::int32_t reputation : {100, 0x1c51, 0x1c52, 9000})
                    for (const int works : {0, 1}) {
                        Setup s;
                        s.band = band;
                        s.stage = band == -2 ? 1 : 2;
                        s.patience = patience;
                        s.reputation = reputation;
                        s.works = works;
                        s.has_league = 0;
                        s.balance = std::int64_t(1) << 40;
                        push(s, "calls band " + std::to_string(band) + " donated " + std::to_string(donated) + " patience " + std::to_string(patience) +
                                " rep " + std::to_string(reputation) + " works " + std::to_string(works), 6,
                             {{donation, donated}, {spend_news, 0}, {clear_news, 0}, {trouble_news, 0}, {calm_news, 0}, {wishes, 1}}, 1);
                    }
    // Real routines on the club (the gifts and bail-outs run for real, with the news and the donation too).
    for (const int band : {-2, -1, 0, 1})
        for (const std::int64_t balance : {std::int64_t(2'000'000'000), std::int64_t(-50'000'000), std::int64_t(0), std::int64_t(-3'000'000'000)})
            for (const std::int32_t reputation : {500, 3000, 6000})
                for (const int owner : {-1, 1}) {
                    Setup s;
                    s.band = band;
                    s.stage = band == -2 ? 1 : 0;
                    s.balance = balance;
                    s.reputation = reputation;
                    s.owner = owner;
                    s.b2f = 20;
                    s.b36 = 3;
                    s.has_league = 0;
                    push(s, "real band " + std::to_string(band) + " balance " + std::to_string(balance) + " rep " + std::to_string(reputation) +
                            " owner " + std::to_string(owner), 8, {}, 1);
                }
    // The club in trouble told once: the news, the flag, the wishes (answered 0 / 1 / real), and the people's loop.
    for (const std::uint32_t answered : {0u, 1u, 2u})
        for (const int refused : {0, 1})
            for (const int other_club : {0, 1, 2})
                for (const int record : {0, 1})
                    for (const std::int32_t cost : {-5, 100, 1000})
                        for (const int person_a : {0, 1, 2}) {
                            if (answered == 0 && (other_club != 1 || record != 1 || cost != 100 || person_a != 1)) continue;
                            Setup s;
                            s.band = -2;
                            s.stage = 0;
                            s.refused = refused;
                            s.has_league = 0;
                            const auto club = scenario.club(s.club);
                            Memory extra;
                            for (unsigned k = 0; k < 4; ++k) {
                                const auto p = persons + k * 0x40;
                                extra.put(club + 0x98 + k * 4, k == 3 ? 0 : p);
                                extra.put(p, 0);
                                extra.put(p + 8, k == 2 ? 50 : 1000);
                                extra.put(p + 0x14, k == unsigned(person_a) ? 0 : 7);
                            }
                            extra.put(wish_answer + 4, other_club == 1 ? club_index(s.club) : club_index(s.club + 1));
                            s.extra = std::move(extra.writes);
                            Answers answers = {{donation, 0}, {trouble_news, 0}, {wishes, answered}};
                            if (other_club != 0) answers.push_back({who_is, wish_answer});
                            else answers.push_back({who_is, 0});
                            answers.push_back({record_of, record ? record_answer : 0});
                            answers.push_back({cost_of, std::uint32_t(cost)});
                            push(s, "trouble told answered " + std::to_string(answered) + " refused " + std::to_string(refused) + " other " +
                                    std::to_string(other_club) + " record " + std::to_string(record) + " cost " + std::to_string(cost) + " person " +
                                    std::to_string(person_a), 1, answers, 1);
                        }
    for (const int band : {-2}) {
        Setup s;
        s.band = band;
        s.stage = 0;
        s.has_league = 0;
        push(s, "trouble told, real routines", 4, {}, 1);
    }
    // The tail: the league row, the counter, the board, the reputation, the funds against the draw; both ways.
    for (const int band : {0, 1, 2})
        for (const int has_league : {0, 1})
            for (const int table_entry : {0, 1})
                for (const int counter : {0x32, 0x33, 60})
                    for (const int has_board : {0, 1})
                        for (const int board_flag : {0, 1, -1})
                            for (const std::int32_t reputation : {0x1099, 0x109a, 0})
                                for (const int patience : {0x14, 0x13, 5, 1, 0, -3}) {
                                    if (!has_league && (table_entry || counter != 60)) continue;
                                    if (!has_board && (board_flag != 1 || patience != 0x14)) continue;
                                    Setup s;
                                    s.band = band;
                                    s.stage = band == 0 ? 0 : 0;
                                    s.has_league = has_league;
                                    s.table_entry = table_entry;
                                    s.counter = counter;
                                    s.has_board = has_board;
                                    s.board_flag = board_flag;
                                    s.reputation = reputation;
                                    s.patience = patience;
                                    push(s, "tail band " + std::to_string(band) + " league " + std::to_string(has_league) + " row " +
                                            std::to_string(table_entry) + " counter " + std::to_string(counter) + " board " + std::to_string(has_board) +
                                            " flag " + std::to_string(board_flag) + " rep " + std::to_string(reputation) + " patience " +
                                            std::to_string(patience), 4, quiet, 1);
                                }
    // The board's spending: its funds, debts and the reputation's draw; the patience byte's wear.
    for (const std::int32_t funds : {0xabe, 0xabf, 5000, 30000, 70000, 123456, 0x77359400, -5})
        for (const std::int32_t owed : {0, 3, 50, 5000, 0x77359400, -5})
            for (const std::int32_t reputation : {0, 1, 100, 3000, 0x7fff, -50, 4249, 0x109a})
                for (const int patience : {0x14, 0x11, 0x10, 0xf, 8, 7, 2, 1, 0, -3, 100}) {
                    if (owed != 5000 && (reputation != 3000 || patience == 0x14 || patience == 1 || patience == 0 || patience == -3)) continue;
                    Setup s;
                    s.band = 0;
                    s.stage = 0;
                    s.has_league = 1;
                    s.table_entry = 1;
                    s.counter = 60;
                    s.board_flag = 0;
                    s.funds = funds;
                    s.owed = owed;
                    s.reputation = reputation;
                    s.patience = patience;
                    push(s, "spend funds " + std::to_string(funds) + " owed " + std::to_string(owed) + " rep " + std::to_string(reputation) +
                            " patience " + std::to_string(patience), 5, quiet, 1);
                }
    // The pool's 0.1 (the tenth of the funds) at its edges.
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (const double tenth : {0.0, 1.0, -0.1, 1e-9, 1e12, nan})
        for (const std::int32_t funds : {5000, 123456, 0x77359400}) {
            Setup s;
            s.band = 0;
            s.has_league = 1;
            s.counter = 60;
            s.board_flag = 0;
            s.funds = funds;
            s.reputation = 3000;
            s.pool = {{0xe0, tenth}};
            push(s, "tenth " + std::to_string(tenth) + " funds " + std::to_string(funds), 6, quiet, 1);
        }
}
}

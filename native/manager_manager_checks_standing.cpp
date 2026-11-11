// MANAGER_MANAGER.CPP checker cases for the standing block (manager_manager_standing.hpp): 0x101bd320.
//
// No capture reaches it. The table cell, the index cell, the club rows and the employer record are built in
// the scratch area; the employer (0x10103510), its relation (0x100f55b0) and the news desk (0x102cc140) are
// answered, so every value the two tests read is chosen: the relation's low byte at and around -2..-1, the
// employer's 64-bit counter at and around 1000000 in each word, and the byte arguments and the row's halfword
// with their sign bits and with high junk in the registers. The message buffer's own routines (0x102bdf90,
// 0x102cb4b0) run for real, and so do the cases without answers.
#include "manager_manager_checks.hpp"
#include <cstdlib>
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t entry = 0x101bd320, employer_record = 0x10103510, relation = 0x100f55b0, news_desk = 0x102cc140;
constexpr std::uint32_t table_cell = scratch_base + 0x100, index_cell = scratch_base + 0x110, table = scratch_base + 0x1000,
                        employer = scratch_base + 0x300;

struct Shape {
    std::string label;
    std::uint32_t relation_answer = 0;
    std::uint32_t high = 0, low = 5000000;
    std::uint32_t first = 3, second = 4;  // r5, r6
    std::uint32_t index = 2;
    std::uint32_t rating = 0x1234;
    bool null_index = false, null_employer = false;
};

void add(std::vector<Case>& cases, const Shape& s) {
    Case item{.entry = entry, .registers = {{3, table_cell}, {4, s.null_index ? 0 : index_cell}, {5, s.first}, {6, s.second}},
              .label = s.label};
    item.memory = {{4, table_cell, table}, {4, index_cell, s.index}, {4, employer, s.high}, {4, employer + 4, s.low}};
    item.memory.push_back({2, table + s.index * 0x4c + 6, s.rating});
    item.answers = {{employer_record, s.null_employer ? 0 : employer}, {relation, s.relation_answer}, {news_desk, 0}};
    cases.push_back(std::move(item));
}
}

MANAGER_MANAGER_CASES("standing") {
    add(cases, {.label = "plain"});
    add(cases, {.label = "index pointer null", .null_index = true});
    add(cases, {.label = "employer null", .null_employer = true});
    for (std::uint32_t answer : {0x80u, 0xfdu, 0xfeu, 0xffu, 0u, 1u, 0x7fu, 0xabcdfffeu, 0x102u, 0xfffffffdu})
        add(cases, {.label = "relation " + std::to_string(answer), .relation_answer = answer});
    constexpr std::uint32_t words[][2] = {{0, 999999},       {0, 1000000},    {0, 1000001},    {0, 0},
                                          {0xffffffff, 0},   {0xffffffff, 0xffffffff}, {0x80000000, 5},
                                          {1, 0},            {0x7fffffff, 0xffffffff}, {0, 0x80000000},
                                          {0xfffffffe, 999999}};
    for (const auto& w : words) {
        add(cases, {.label = "counter " + std::to_string(w[0]) + ":" + std::to_string(w[1]), .high = w[0], .low = w[1]});
        add(cases, {.label = "counter with relation -1 " + std::to_string(w[0]) + ":" + std::to_string(w[1]),
                    .relation_answer = 0xff, .high = w[0], .low = w[1]});
    }
    for (std::uint32_t arg : {0u, 1u, 0x7fu, 0x80u, 0xffu, 0x1ffu, 0xdead0081u, 0xdead007fu})
        add(cases, {.label = "bytes " + std::to_string(arg), .first = arg, .second = ~arg});
    for (std::uint32_t rating : {0u, 0x7fffu, 0x8000u, 0xffffu, 5u})
        add(cases, {.label = "rating " + std::to_string(rating), .rating = rating});
    add(cases, {.label = "index 0", .index = 0});
    add(cases, {.label = "index 40", .index = 40, .rating = 0xfff0});
    add(cases, {.label = "index -1", .index = 0xffffffff, .rating = 0x8001});
    // No answers: the real employer, its relation and the news desk run.
    for (std::uint32_t index : {0u, 1u, 7u}) {
        Case item{.entry = entry, .registers = {{3, table_cell}, {4, index_cell}, {5, 1}, {6, 0xff}}, .label = "real routines, index " + std::to_string(index)};
        item.memory = {{4, table_cell, table}, {4, index_cell, index}, {2, table + index * 0x4c + 6, 0x1111}};
        cases.push_back(std::move(item));
    }
}
}

// ---- 0x101bd460: the board's reply text ----
// The message record is a scratch array of words (0x102cb570 reads the word at +8+4n); the club is a real one of
// the career (its name and standing byte are read, the row's double at +0x38 is written), and the formatters run
// for real, so the text in the shared buffer is compared. The two routines that hand the text to the output
// object (0x1074b830, 0x1074bab0) are answered in one family of cases and run for real over a scratch object in
// the other.
namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t reply = 0x101bd460, to_object = 0x1074b830, name_to_object = 0x1074bab0;
constexpr std::uint32_t record = scratch_base + 0x9000, out_object = scratch_base + 0xa000;

struct Reply {
    std::uint32_t flag = 1, index = 0, kind = 1, reason = 0, rating = 0, extra = 0, standing = 5;
    std::uint64_t fp = 0;
    bool answered = true;
};

Case reply_case(const Scenario& sc, unsigned club_k, const Reply& r, std::string label) {
    Case item{.entry = reply, .registers = {{3, sc.global(-0x79f0)}, {4, record}, {5, r.flag}, {6, out_object}}, .label = std::move(label)};
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    put(4, record + 8, club_k);
    put(4, record + 12, r.kind);
    put(4, record + 16, r.reason);
    put(4, record + 20, r.rating);
    put(4, record + 24, r.extra);
    put(1, sc.club(club_k) + 0x242, r.standing);
    const auto row = sc.r32(sc.global(-0x79f0)) + club_k * 0x4c;
    put(4, row + 0x38, std::uint32_t(r.fp >> 32));
    put(4, row + 0x3c, std::uint32_t(r.fp));
    if (r.answered) item.answers = {{to_object, 0}, {name_to_object, 0}};
    return item;
}
}

MANAGER_MANAGER_CASES("standing_text") {
    if (scenario.match) return;
    const unsigned club_k = 5;
    const auto pool = scenario.toc(-0x536c);
    const auto limit = (std::uint64_t(scenario.r32(pool + 0x2c8)) << 32) | scenario.r32(pool + 0x2cc);
    unsigned n = 0;
    const auto label = [&](const char* what) { return std::string(what) + " " + std::to_string(n++); };
    // every kind and reason of the first section, with the flag's variations on the first reasons
    for (std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 0x7fu, 0x80u, 0xffu, 0x101u})
        for (std::uint32_t reason : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 0xau, 0xbu, 0xcu, 0xdu, 0x7fu, 0x80u, 0xffu, 0x104u})
            cases.push_back(reply_case(scenario, club_k, {.kind = kind, .reason = reason}, label("reply")));
    for (std::uint32_t flag : {1u, 0xffu, 0x101u, 0x80u, 0x7fu, 0x100u, 0u})
        for (std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 0x80u, 0xffu, 0x103u})
            cases.push_back(reply_case(scenario, club_k, {.flag = flag, .kind = kind, .reason = 2}, label("flag")));
    // the transfer funds texts: the shortage flag, the rating cut, the row's double against the limit
    for (std::uint32_t reason : {2u, 4u, 5u, 7u, 9u, 0x80u})
        for (std::uint32_t rating : {0x1c52u, 0x1c53u, 0u, 0xffffu, 0x8000u})
            for (std::uint32_t extra : {0u, 1u, 0x80u, 0x100u})
                for (std::uint64_t fp : {std::uint64_t(0), limit, limit + 1, limit - 1, limit ^ 0x8000000000000000ULL, 0x7ff8000000000000ULL, 0x7ff0000000000000ULL, 0x4059000000000000ULL})
                    cases.push_back(reply_case(scenario, club_k, {.kind = 3, .reason = reason, .rating = rating, .extra = extra, .fp = fp}, label("funds")));
    // the standing byte and another club
    for (std::uint32_t standing : {0u, 0x80u, 0xffu, 0x7fu})
        for (std::uint32_t kind : {1u, 3u, 6u})
            cases.push_back(reply_case(scenario, 7, {.kind = kind, .reason = 1, .standing = standing}, label("standing")));
    // the headlines
    for (std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 0x7fu, 0x80u, 0xffu, 0x100u, 0x101u})
        cases.push_back(reply_case(scenario, club_k, {.flag = 0, .kind = kind}, label("headline")));
    // The same with the output object's routines run for real: 0x1074bab0's row holds the stale register save area.
    for (std::uint32_t kind : {1u, 3u, 6u, 7u})
        for (std::uint32_t flag : {1u, 0u})
            cases.push_back(reply_case(scenario, club_k, {.flag = flag, .kind = kind, .reason = 1, .answered = false}, label("real output")));
}
}

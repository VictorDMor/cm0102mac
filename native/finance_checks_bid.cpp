// FINANCE.CPP checker cases for the bid block (finance_bid.hpp): 0x100f5770 (the bid ceiling),
// 0x100f9f40 (the facilities upgrade) and 0x100fa1f0 (the sampled income).
//
// The club is the career's real record; every record the routines follow from it (the league, the
// competition, the ground, the manager, the agent and their profiles, the other side's bonus slot, the
// deal and staff lists, the gate table) is built in the scratch area and pointed to by a memory write the
// checker undoes after the case. Routines of other modules are answered (their results chosen per case);
// the lists the original gets back through out-parameters are placed with a poke at the frame address
// (the entry stack pointer less the frame size). Two cases at the end show that the checker fails when the
// rewrite is wrong: they are not here but in the checker's own --corrupt run.
#include "finance_checks.hpp"
#include "guest_abi.hpp"
#include <cstdint>
#include <initializer_list>
#include <string>

namespace cm::aot::finance_checks {
namespace {
constexpr std::uint32_t bid_limit = 0x100f5770, upgrade_facilities = 0x100f9f40, sampled_income = 0x100fa1f0;
constexpr std::uint32_t person_counts = 0x100d3040, club_condition = 0x1014c2e0, pending_deals = 0x103f45a0,
                        contract_of = 0x10082f90, post_of = 0x10082f20, staff_list = 0x1007bfc0,
                        agent_of = 0x106fdb00, answer_of = 0x106fe280;

struct Rng {
    std::uint64_t state;
    std::uint32_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return std::uint32_t(state >> 33);
    }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    bool chance(unsigned percent) { return below(100) < percent; }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(unsigned(values.size()))); }
};

// The competition kinds the routine compares with: table-of-contents words whose cells hold a code.
constexpr std::int32_t kind_slots[] = {-0x7a10, -0x7a14, -0x7a18, -0x7af4, -0x7a00, -0x7a1c, -0x7a28, -0x7514, -0x7510,
                                       -0x79fc, -0x75b0, -0x750c, -0x7504, -0x7a20, -0x7594, -0x757c, -0x7574, -0x7578,
                                       -0x7508, -0x7b68};

constexpr std::uint32_t area = scratch_base;
constexpr std::uint32_t request_at = area + 0x1000, other_at = area + 0x1200, bonus_at = area + 0x1280,
                        contract_at = area + 0x4000,
                        post_at = area + 0x4040, agent_record_at = area + 0x4080, competition_at = area + 0x5000,
                        league_at = area + 0x5100, ground_at = area + 0x5200, manager_at = area + 0x5300,
                        profile_at = area + 0x5380, agent_at = area + 0x5400, agent_profile_at = area + 0x5480,
                        persons_at = area + 0x5500, gate_at = area + 0x6000, capped_at = area + 0x6800,
                        gate_rows = area + 0x7000;

struct Plan {
    std::uint32_t club = 0;           // the career's club record used
    std::uint32_t club_index = 0;
    std::int64_t funds = 50'000'000;
    std::int32_t reputation = 5000, standing = 8, income = 40000, bonus = -1;
    std::uint8_t board = 1, ground_flag = 0, ground_kind = 1;
    bool ground = true, manager = true, profile = true, competition = true, league = true, agent = true,
         agent_profile = true, other = true, bonus_record = true, gate = true, negotiating = true, negotiation = true;
    std::uint32_t ground_word = 90000;
    std::int32_t trait = 10, temper = 12, power = 40, quality = 15, other_quality = 12;
    std::uint8_t competition_byte = 5;
    std::uint32_t kind = 0x7777;      // the competition's code
    std::uint32_t request_3 = 7, request_7 = 1000, deal_id = 3;
    std::uint32_t difference_high = 0, difference_low = 0, other_high = 0, other_low = 0;
    std::uint32_t gate_value = 0;
    unsigned squad = 1, deals = 1, checked = 1, capped_flag = 1;
    std::uint32_t staff_persons = 0;  // how many of the club's first staff slots hold a person
    std::int32_t person_index = 1, person_salary = 12000;
    // The callee answers.
    std::uint32_t counts = 1, condition = 1, contract = 1, post_club_mine = 1, post = 1, agent_found = 1, answer = 0;
    // The lists.
    unsigned deal_count = 0, staff_count = 0;
    struct Deal { std::uint32_t id; std::uint8_t flag; std::uint32_t amount; std::uint8_t kind; };
    Deal deals_list[6] = {};
    struct Staff { std::uint32_t person; std::uint32_t id, value; std::uint8_t kind; };
    Staff staff_list_[4] = {};
    std::uint32_t league_table_zero = 0;
    bool league_dropped = false;  // the other side's answer clears the club's league word (a poke)
    std::string tag;
};

class Maker {
public:
    Maker(const Scenario& scenario, std::vector<Case>& cases) : scenario(scenario), cases(cases) {
        sp = scenario.s.r<4>(guest::reg::gpr(1));
        frame = sp - 0xc0;
        // The lists are freed by the original: they live in real heap blocks (the competition and people
        // tables, the game's own), so that the allocator releases a block it knows.
        deals_at = scenario.global(-0x7bbc);
        staff_at = scenario.global(-0x7be0);
    }
    void add(const Plan& p) {
        Case item;
        item.entry = bid_limit;
        const auto put4 = [&](std::uint32_t at, std::uint32_t v) { item.memory.push_back({4, at, v}); };
        const auto put2 = [&](std::uint32_t at, std::uint32_t v) { item.memory.push_back({2, at, v}); };
        const auto put1 = [&](std::uint32_t at, std::uint32_t v) { item.memory.push_back({1, at, v}); };
        const auto club = p.club;
        put4(request_at, std::uint32_t(std::uint64_t(p.funds) >> 32));
        put4(request_at + 4, std::uint32_t(p.funds));
        put4(request_at + 8, p.club_index);
        put4(request_at + 0xc, p.request_3);
        put4(request_at + 0x1c, p.request_7);
        put4(request_at + 0x110, p.difference_high);
        put4(request_at + 0x114, p.difference_low);
        put4(request_at + 0x118, p.other_high);
        put4(request_at + 0x11c, p.other_low);
        put4(other_at, p.deal_id);
        put4(other_at + 0x14, p.bonus_record ? bonus_at : 0);
        put2(bonus_at + 0xa, std::uint32_t(p.other_quality));
        put2(club + 0x1ec, std::uint32_t(p.reputation));
        put1(club + 0x240, std::uint32_t(p.standing));
        put4(club + 8, std::uint32_t(p.income));
        put4(club + 0x14, std::uint32_t(p.bonus));
        put1(club + 0x23e, p.board);
        put1(club + 0x241, p.ground_flag);
        put1(club + 0x23d, p.ground_kind);
        put4(club + 0x3c, p.ground ? ground_at : 0);
        put4(ground_at + 4, p.ground_word);
        put4(club + 0x80, p.manager ? manager_at : 0);
        put4(manager_at + 0x1c, p.profile ? profile_at : 0);
        put1(manager_at + 0x63, std::uint32_t(p.temper));
        put1(profile_at + 0x2f, std::uint32_t(p.trait));
        put4(club + 0x2c, p.competition ? competition_at : 0);
        put4(competition_at + 0x68, p.kind);
        put1(competition_at + 0x11f, p.competition_byte);
        // The league's table entry is set here, not taken from the career's club: a club whose own league has no
        // entry would end every case at the early return.
        const std::uint32_t league_index = 1;
        put4(club + 0x30, p.league ? league_at : 0);
        put4(league_at, league_index);
        put2(league_at + 0x18, std::uint32_t(p.power));
        put4(scenario.global(-0x7b5c) + league_index * 4, p.league_table_zero ? 0u : area + 0x6900);
        put4(club + 0x90, p.agent ? agent_at : 0);
        put4(agent_at + 0x1c, p.agent_profile ? agent_profile_at : 0);
        put2(agent_profile_at + 0x2a, std::uint32_t(p.quality));
        // The gate table, the negotiation flags and the club's staff slots.
        put4(scenario.toc(-0x79f0), p.gate ? gate_at : 0);
        put4(gate_at, gate_rows);
        put4(gate_rows + p.club_index * 0x4c + 0x28, p.gate_value);
        put1(scenario.toc(-0x797c), p.negotiating ? 1 : 0);
        put4(scenario.toc(-0x7978), p.negotiation ? 0x60006000 : 0);
        for (unsigned n = 0; n < p.staff_persons; ++n) {
            const auto person = persons_at + n * 0x70;
            put4(club + 0x98 + n * 4, person);
            put4(person, std::uint32_t(p.person_index) + n);
            put4(person + 8, std::uint32_t(p.person_salary) * (n + 1));
        }
        put4(contract_at + 0xc, 7000);
        put4(post_at + 4, p.post_club_mine ? p.club_index : p.club_index + 1);
        // The lists.
        for (unsigned n = 0; n < p.deal_count; ++n) {
            const auto entry = deals_at + n * 0x1c;
            put4(entry + 8, p.deals_list[n].id);
            put1(entry + 0x10, p.deals_list[n].flag);
            put4(entry + 0x14, p.deals_list[n].amount);
            put1(entry + 0x19, p.deals_list[n].kind);
        }
        for (unsigned n = 0; n < p.staff_count; ++n) {
            const auto entry = staff_at + n * 0x88;
            put4(entry, p.staff_list_[n].person);
            put4(entry + 8, p.staff_list_[n].id);
            put4(entry + 0xc, p.staff_list_[n].value);
            put1(entry + 0x64, p.staff_list_[n].kind);
        }
        item.registers = {{3, request_at}, {4, p.other ? other_at : 0}, {5, p.squad}, {6, p.capped_flag ? capped_at : 0},
                          {7, p.checked}, {8, p.deals}};
        item.answers = {{person_counts, p.counts}, {club_condition, p.condition}, {contract_of, p.contract ? contract_at : 0},
                        {post_of, p.post ? post_at : 0}, {agent_of, p.agent_found ? agent_record_at : 0},
                        {answer_of, p.answer}, {pending_deals, p.deal_count}, {staff_list, p.staff_count}};
        item.pokes = {{pending_deals, {{4, frame + 0x3c, p.deal_count ? deals_at : 0}}},
                      {staff_list, {{4, frame + 0x38, p.staff_count ? staff_at : 0}}}};
        // One entry in a list: the call made for it drops the list pointer, so the original's release of the
        // list (the allocator, which the checker can't run on a snapshot) is not reached and the rest runs.
        if (p.deal_count == 1 && p.deals_list[0].flag != 0 && p.deals_list[0].kind >= 3 && p.deals_list[0].kind <= 6)
            item.pokes.push_back({contract_of, {{4, frame + 0x3c, 0}}});
        if (p.staff_count == 1 && p.staff_list_[0].kind != 4)
            item.pokes.push_back({post_of, {{4, frame + 0x38, 0}}});
        if (p.league_dropped) item.pokes.push_back({answer_of, {{4, club + 0x30, 0}}});
        item.label = "bid " + p.tag;
        cases.push_back(std::move(item));
    }

private:
    const Scenario& scenario;
    std::vector<Case>& cases;
    std::uint32_t sp = 0, frame = 0, deals_at = 0, staff_at = 0;
};
}

FINANCE_CASES("bid") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ab8);
    const auto clubs = scenario.global(-0x7b50);
    Maker maker(scenario, cases);
    Rng rng{0xb1d};
    const std::uint32_t club_indices[] = {2, 11, 37, clubs / 3, clubs / 2};
    const auto base_plan = [&](unsigned which) {
        Plan p;
        p.club_index = club_indices[which % 5];
        p.club = scenario.club(p.club_index);
        return p;
    };
    const auto kind_value = [&](unsigned n) -> std::uint32_t {
        return n < sizeof kind_slots / sizeof kind_slots[0] ? scenario.global(kind_slots[n]) : 0x7777u;
    };
    constexpr unsigned kind_count = sizeof kind_slots / sizeof kind_slots[0] + 1;

    // Each competition kind against a spread of funds: every ladder rung and its threshold.
    for (unsigned n = 0; n < kind_count; ++n)
        for (const std::int64_t funds : {-5'000'000LL, 0LL, 3'000'000LL, 12'000'000LL, 20'000'000LL, 60'000'000LL,
                                         200'000'000LL, 5'000'000'000LL}) {
            Plan p = base_plan(n);
            p.kind = kind_value(n);
            p.funds = funds;
            p.tag = "kind " + std::to_string(n) + " funds " + std::to_string(funds);
            maker.add(p);
        }

    // The cut to a tenth of the funds: the plan's cost follows the request's word at +0xc, swept in small
    // steps through the narrow band where what is left lies under a tenth of the funds.
    for (const std::int32_t reputation : {1500, 5000, 9000})
        for (const std::uint8_t board : {0, 1}) {
            unsigned step = 0;
            for (double debt = 100'000.0; debt < 3e7; debt *= 1.03, ++step) {
                Plan p = base_plan(step + unsigned(reputation));
                p.reputation = reputation;
                p.board = board;
                p.funds = 100'000'000;
                p.request_7 = 0;
                p.request_3 = std::uint32_t(debt);
                p.kind = scenario.global(-0x7a10);
                p.tag = "tenth reputation " + std::to_string(reputation) + " board " + std::to_string(board) + " step " + std::to_string(step);
                maker.add(p);
            }
        }
    // The plan: reputation edges, the board flag, the ground, the level from the manager's rating.
    for (const std::int32_t reputation : {0, 1, 0x4e2, 0x4e3, 0xdab, 0xdac, 0xfa0, 0x109a, 0x109b, 0x1388, 0x1389,
                                          0x1676, 0x1677, 0x2000, 0x7fff, -1, -9000})
        for (const std::uint8_t board : {0, 1})
            for (const bool ground : {false, true})
                for (const std::uint8_t kind_byte : {0, 1}) {
                    Plan p = base_plan(unsigned(reputation) + board);
                    p.reputation = reputation;
                    p.board = board;
                    p.ground = ground;
                    p.ground_kind = kind_byte;
                    p.tag = "reputation " + std::to_string(reputation) + " board " + std::to_string(board) + " ground " +
                            std::to_string(ground) + " kind " + std::to_string(kind_byte);
                    maker.add(p);
                    p.funds = 400'000'000;
                    p.kind = kind_value(unsigned(reputation) % 20);
                    p.tag += " rich";
                    maker.add(p);
                }
    for (const std::int32_t standing : {-3, 0, 5, 6, 9, 15, 16, 17, 20, 127, -128})
        for (const std::int32_t trait : {-100, 0, 8, 120})
            for (const std::int32_t temper : {0, 15, 16, 40, -20, 255}) {
                Plan p = base_plan(unsigned(standing + trait + temper));
                p.standing = standing;
                p.trait = trait;
                p.temper = temper;
                p.reputation = 3000 + standing * 100;
                p.tag = "standing " + std::to_string(standing) + " trait " + std::to_string(trait) + " temper " + std::to_string(temper);
                maker.add(p);
            }
    // No manager, no profile, no competition, no league, a league with no table entry.
    for (unsigned variant = 0; variant < 8; ++variant) {
        Plan p = base_plan(variant);
        p.manager = !(variant & 1);
        p.profile = !(variant & 2);
        p.competition = !(variant & 4);
        p.tag = "records missing " + std::to_string(variant);
        maker.add(p);
        p.league = false;
        p.tag += " no league";
        maker.add(p);
        p.league = true;
        p.league_table_zero = 1;
        p.tag += " empty table";
        maker.add(p);
    }
    for (const std::int32_t bonus : {-1, 0, 5})
        for (const std::uint8_t ground_kind : {0, 1})
            for (const unsigned n : {0u, 4u, 6u, 7u, 11u, 12u, 14u}) {
                Plan p = base_plan(n);
                p.bonus = bonus;
                p.ground_kind = ground_kind;
                p.kind = kind_value(n);
                p.competition = n != 14;
                p.reputation = 900 * int(n) + 100;
                p.tag = "bonus " + std::to_string(bonus) + " ground " + std::to_string(ground_kind) + " kind " + std::to_string(n);
                maker.add(p);
            }

    // The allowance: the difference of the two amounts, the other deals, the funds share and the value block.
    for (const std::uint32_t high : {0u, 1u, 0xffffffffu, 0x80000000u})
        for (const std::uint32_t lowv : {0u, 1u, 7'999'999u, 8'000'000u, 8'000'001u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 20'000'000u})
            for (const std::int64_t funds : {60'000'000LL, 5'000'000LL}) {
                Plan p = base_plan(high ^ lowv);
                p.difference_high = high;
                p.difference_low = lowv;
                p.funds = funds;
                p.kind = scenario.global(-0x7af4);
                p.tag = "difference " + std::to_string(high) + ":" + std::to_string(lowv) + " funds " + std::to_string(funds);
                maker.add(p);
            }
    // The value block by reputation, the agent's quality and the competition's kinds that cap.
    for (const std::int32_t reputation : {0, 300, 5000, 7000, 9000, 14000})
        for (const std::int32_t quality : {0, 5, 1000, 0x7fff, -300})
            for (const unsigned n : {0u, 3u, 7u, 12u, 20u})
                for (const std::uint8_t flag : {0, 1}) {
                    Plan p = base_plan(unsigned(quality) + n);
                    p.reputation = reputation;
                    p.quality = quality;
                    p.kind = kind_value(n);
                    p.ground_flag = flag;
                    p.competition = n != 20;
                    p.funds = 90'000'000;
                    p.difference_high = 0;
                    p.difference_low = 50'000'000;
                    p.other_high = 0;
                    p.other_low = 1;
                    p.tag = "value reputation " + std::to_string(reputation) + " quality " + std::to_string(quality) +
                            " kind " + std::to_string(n) + " flag " + std::to_string(flag);
                    maker.add(p);
                    p.profile = false;
                    p.agent_profile = false;
                    p.tag += " nothing";
                    maker.add(p);
                }

    // The deals and the staff: the open deals of each kind, a person counted twice, the contract and
    // post answers, the staff lists with their entries of kind 4.
    for (unsigned variant = 0; variant < 48; ++variant) {
        Plan p = base_plan(variant);
        p.deal_count = variant % 5;
        for (unsigned n = 0; n < p.deal_count; ++n) {
            p.deals_list[n] = {(n % 2 == 0 && variant % 3 == 0) ? p.deal_id : 100 + n / 2 + variant % 2,
                               std::uint8_t(rng.pick<unsigned>({0, 1, 1})), 1000u * (n + 1) + variant,
                               std::uint8_t(rng.pick<unsigned>({3, 4, 5, 6, 7, 2}))};
        }
        p.staff_count = variant % 4;
        for (unsigned n = 0; n < p.staff_count; ++n)
            p.staff_list_[n] = {n + 1, n * 3 + 1, 500u * (n + 1), std::uint8_t(n == 1 ? 4 : 2)};
        p.staff_persons = variant % 4;
        p.person_index = (variant % 7 == 0) ? 0x7ffffff0 : 1;
        p.counts = variant % 2;
        p.contract = (variant / 2) % 2;
        p.post = (variant / 4) % 2;
        p.post_club_mine = (variant / 3) % 2;
        p.condition = (variant % 9) != 0;
        p.squad = variant % 6 != 0;
        p.deals = variant % 5 != 0;
        p.tag = "deals " + std::to_string(variant);
        maker.add(p);
    }

    // One entry in the lists, each kind and answer: the whole routine runs after the lists.
    for (const std::uint8_t kind : {2, 3, 4, 5, 6, 7})
        for (const std::uint8_t flag : {0, 1})
            for (const unsigned answers : {0u, 1u, 2u, 3u})
                for (const bool deal_list : {true, false}) {
                    Plan p = base_plan(kind + flag + answers);
                    if (deal_list) {
                        p.deal_count = 1;
                        p.deals_list[0] = {p.deal_id + (answers & 1 ? 0 : 5), flag, 2500, kind};
                    } else {
                        p.staff_count = 1;
                        p.staff_list_[0] = {1, 2, 900, kind};
                        p.staff_persons = 2;
                    }
                    p.contract = answers & 1;
                    p.post = (answers >> 1) & 1;
                    p.post_club_mine = (answers & 1) ^ ((answers >> 1) & 1);
                    p.tag = "one entry kind " + std::to_string(kind) + " flag " + std::to_string(flag) + " answers " +
                            std::to_string(answers) + (deal_list ? " deal" : " staff");
                    maker.add(p);
                }

    // The other side's answer: every value, with the club's competition, agent and funds changed.
    for (const std::uint32_t answer : {0u, 1u, 2u, 3u, 4u, 5u, 0xffu, 0xfeu, 0x102u})
        for (unsigned variant = 0; variant < 24; ++variant) {
            Plan p = base_plan(variant);
            p.answer = answer;
            p.funds = (variant % 3 == 0) ? -2'000'000 : 80'000'000 + variant;
            p.difference_low = variant % 4 ? 0 : 30'000'000;
            p.competition_byte = std::uint8_t(rng.pick<unsigned>({0, 8, 9, 30, 200}));
            p.reputation = rng.pick<int>({100, 2500, 4200, 4250, 5000, 9000, 0});
            p.agent = variant % 5 != 0;
            p.agent_profile = variant % 7 != 0;
            p.bonus_record = variant % 2;
            p.other_quality = rng.pick<int>({0, 40, 10000, -500, 7});
            p.league = variant % 11 != 0;
            p.competition = variant % 13 != 0;
            p.negotiating = variant % 17 != 0;
            p.negotiation = variant % 19 != 0;
            p.condition = variant % 6 != 0;
            p.agent_found = variant % 8 != 0;
            p.checked = variant % 9 != 0;
            p.capped_flag = variant % 10 != 0;
            p.quality = rng.pick<int>({1, 20, 5000, 0});
            p.kind = kind_value(variant);
            p.tag = "answer " + std::to_string(answer) + " variant " + std::to_string(variant);
            maker.add(p);
        }

    // Coverage closing: each outcome the first pass left open, with the case that gives it.
    {
        // The deal on the other side's own record is skipped; with no other record every deal counts.
        for (const bool with_other : {true, false})
            for (const std::uint32_t id : {3u, 4u}) {
                Plan p = base_plan(1);
                p.other = with_other;
                p.deal_count = 1;
                p.deals_list[0] = {id, 0, 700, 2};
                p.tag = "cover deal flag 0 other " + std::to_string(with_other) + " id " + std::to_string(id);
                maker.add(p);
            }
        // A staff entry whose post is at this club and whose contract exists (and does not).
        for (const bool contract : {true, false}) {
            Plan p = base_plan(2);
            p.staff_count = 1;
            p.staff_list_[0] = {1, 2, 900, 2};
            p.staff_persons = 1;
            p.post = 1;
            p.post_club_mine = 1;
            p.contract = contract;
            p.tag = "cover staff contract " + std::to_string(contract);
            maker.add(p);
        }
        // A competition that is missing where the amount is scaled by it (the negotiation check is off
        // so that the original does not read the missing record later).
        for (const std::uint8_t checked : {0, 1}) {
            Plan p = base_plan(3);
            p.competition = false;
            p.bonus = 0;
            p.ground_kind = 1;
            p.checked = checked;
            p.negotiating = false;
            p.tag = "cover no competition checked " + std::to_string(checked);
            maker.add(p);
        }
        // The cut to a tenth of the funds when a tenth is under the cap (funds under 25 million).
        for (const std::int32_t reputation : {1500, 9000}) {
            unsigned step = 0;
            for (double debt = 20'000.0; debt < 3e6; debt *= 1.03, ++step) {
                Plan p = base_plan(step);
                p.reputation = reputation;
                p.funds = 10'000'000;
                p.request_7 = 0;
                p.request_3 = std::uint32_t(debt);
                p.kind = scenario.global(-0x7a10);
                p.tag = "cover tenth under cap reputation " + std::to_string(reputation) + " step " + std::to_string(step);
                maker.add(p);
            }
        }
        // The early returns of the answer part: no mark byte in the frame, no negotiation object.
        for (unsigned variant = 0; variant < 4; ++variant) {
            Plan p = base_plan(variant);
            p.checked = variant & 1 ? 0 : 1;
            p.negotiation = !(variant & 2) || (variant & 1);
            p.negotiating = true;
            p.tag = "cover answer early " + std::to_string(variant);
            maker.add(p);
        }
        // The answer part with a null agent, no reputation, no bonus slot, and every ratio of the slot's quality.
        for (const std::int32_t quality : {0, 1, 5, 12, 40, 200, 3000, 32767, -50})
            for (const std::int32_t reputation : {0, 40, 5000})
                for (const bool bonus_record : {true, false})
                    for (const bool agent : {true, false}) {
                        Plan p = base_plan(unsigned(quality) + unsigned(reputation));
                        p.funds = -3'000'000;
                        p.other_quality = quality;
                        p.reputation = reputation;
                        p.bonus_record = bonus_record;
                        p.agent = agent;
                        p.answer = 1;
                        p.difference_low = 0;
                        p.tag = "cover red quality " + std::to_string(quality) + " reputation " + std::to_string(reputation) +
                                " slot " + std::to_string(bonus_record) + " agent " + std::to_string(agent);
                        maker.add(p);
                    }
        // The league word cleared by the answer's call: the club's league is read again afterwards.
        for (const std::uint32_t answer : {0u, 1u, 2u, 4u}) {
            Plan p = base_plan(answer);
            p.answer = answer;
            p.league_dropped = true;
            p.tag = "cover league dropped " + std::to_string(answer);
            maker.add(p);
        }
        // A club record at address 0 (the table base plus the index times 0x248 wraps): both sides read +0x30 and fault.
        {
            std::uint32_t inverse = 73;
            for (int n = 0; n < 5; ++n) inverse *= 2 - 73 * inverse;
            const std::uint32_t table = scenario.global(-0x7be8), need = 0u - table;
            if ((need & 7) == 0) {
                const std::uint32_t index = (need >> 3) * inverse;
                cases.push_back({.entry = bid_limit,
                                 .registers = {{3, request_at}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}},
                                 .memory = {{4, request_at, 0}, {4, request_at + 4, 5000}, {4, request_at + 8, index}},
                                 .label = "bid club record at address 0"});
            }
        }
    }

    // Random combinations, to reach the rest.
    for (unsigned n = 0; n < 600; ++n) {
        Plan p = base_plan(n);
        p.funds = std::int64_t(rng.next()) * (rng.chance(50) ? 1 : 64) - (rng.chance(25) ? 2'000'000'000LL : 0);
        p.reputation = rng.pick<int>({0, 500, 1500, 3500, 4000, 4500, 5000, 6000, 8000, 9500, 12000});
        p.standing = int(rng.below(24)) - 2;
        p.trait = int(rng.below(60)) - 5;
        p.temper = int(rng.below(30));
        p.income = int(rng.below(90000));
        p.board = std::uint8_t(rng.chance(50));
        p.ground = rng.chance(85);
        p.ground_word = rng.below(300000);
        p.ground_kind = std::uint8_t(rng.chance(50));
        p.ground_flag = std::uint8_t(rng.chance(30));
        p.bonus = rng.chance(50) ? -1 : int(rng.below(9));
        p.kind = kind_value(rng.below(kind_count));
        p.request_3 = rng.below(2'000'000);
        p.request_7 = rng.below(500'000);
        p.difference_low = rng.chance(60) ? 0 : rng.below(60'000'000);
        p.other_low = rng.below(3'000'000);
        p.power = int(rng.below(120)) - 10;
        p.quality = int(rng.below(60)) - 5;
        p.other_quality = int(rng.below(40)) - 5;
        p.competition_byte = std::uint8_t(rng.below(14));
        p.gate_value = rng.chance(50) ? 0 : rng.below(4'000'000) - 1'000'000;
        p.gate = rng.chance(70);
        p.answer = rng.pick<unsigned>({0, 1, 2, 3, 4});
        p.condition = rng.chance(70);
        p.agent_found = rng.chance(85);
        p.deal_count = rng.chance(25) ? 1 : 0;
        for (unsigned k = 0; k < p.deal_count; ++k)
            p.deals_list[k] = {100 + rng.below(3), std::uint8_t(rng.chance(60)), rng.below(100000),
                               std::uint8_t(rng.pick<unsigned>({3, 4, 5, 6, 2}))};
        p.staff_persons = rng.below(3);
        p.tag = "random " + std::to_string(n);
        maker.add(p);
    }

    // Facilities upgrade: the standing and the reputation across every threshold, on real clubs.
    for (const std::int32_t standing : {-3, 0, 5, 10, 11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 30, -128, 127})
        for (const std::int32_t reputation : {0, 0x109a, 0x109b, 0x1482, 0x1483, 0x1c52, 0x1c53, 0x203a, 0x203b, 0x7fff, -1}) {
            const auto club = scenario.club(club_indices[unsigned(standing + reputation) % 5]);
            cases.push_back({.entry = upgrade_facilities, .registers = {{3, manager}, {4, club}},
                             .memory = {{1, club + 0x240, std::uint32_t(standing)}, {2, club + 0x1ec, std::uint32_t(reputation)}},
                             .label = "upgrade standing " + std::to_string(standing) + " reputation " + std::to_string(reputation)});
        }
    for (const std::uint32_t low : {0u, 1u, 0x80000000u, 0xfffffff0u, 0x1e848u})
        cases.push_back({.entry = upgrade_facilities, .registers = {{3, manager}, {4, scenario.club(11)}},
                         .memory = {{1, scenario.club(11) + 0x240, 3}, {2, scenario.club(11) + 0x1ec, 0x2100},
                                    {4, scenario.r32(manager) + scenario.r32(scenario.club(11)) * 0x168 + 4, low}},
                         .label = "upgrade balance low " + std::to_string(low)});

    // Sampled income: the owner's rating and the board flag pick a class, the reputation and the
    // competition's kind the factor; the sample table at its end draws again.
    const auto cursor_cell = scenario.toc(-0x7b84), last_cell = scenario.toc(-0x7b88);
    const auto kind_cell = scenario.toc(-0x7a24);
    for (const std::uint32_t at_end : {0u, 1u})
        for (const std::uint8_t board : {0, 1})
            for (const bool owner : {false, true})
                for (const bool profile : {false, true})
                    for (const std::int32_t reputation : {0, 1, 0xea6, 0xea7, 0x1482, 0x1483, 0x2000, -5, 0x7fff})
                        for (const std::int32_t trait : {-100, -40, 0, 20, 80, 127})
                            for (const bool same : {false, true}) {
                                if (!owner && (trait != 0 || profile)) continue;
                                if (!owner && board) continue;
                                const auto index = club_indices[unsigned(reputation + trait) % 5];
                                const auto club = scenario.club(index);
                                Case item{.entry = sampled_income, .registers = {{3, manager}, {4, club}}};
                                item.memory = {{2, club + 0x1ec, std::uint32_t(reputation)}, {1, club + 0x23e, board},
                                               {4, club + 8, 30000},
                                               {4, club + 0x80, owner ? manager_at : 0},
                                               {4, manager_at + 0x1c, profile ? profile_at : 0}, {1, profile_at + 0x2f, std::uint32_t(trait)},
                                               {4, club + 0x2c, competition_at},
                                               {4, competition_at + 0x68, same ? scenario.r32(kind_cell) : 0x7777u}};
                                if (at_end) item.memory.push_back({4, cursor_cell, scenario.r32(last_cell)});
                                item.label = "income reputation " + std::to_string(reputation) + " trait " + std::to_string(trait) +
                                             " board " + std::to_string(board) + " owner " + std::to_string(owner) + std::to_string(profile) +
                                             " same " + std::to_string(same) + " end " + std::to_string(at_end);
                                cases.push_back(std::move(item));
                            }
    for (const std::uint32_t low : {0u, 0xfffffff0u, 0x7fffffffu})
        cases.push_back({.entry = sampled_income, .registers = {{3, manager}, {4, scenario.club(37)}},
                         .memory = {{4, scenario.r32(manager) + scenario.r32(scenario.club(37)) * 0x168 + 4, low},
                                    {4, scenario.club(37) + 0x80, 0}, {2, scenario.club(37) + 0x1ec, 0x3000}},
                         .label = "income balance low " + std::to_string(low)});
}
}

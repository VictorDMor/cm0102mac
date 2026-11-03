#include "contract_screens.hpp"
#include "guest_fpu.hpp"
#include "screen_manager_detail.hpp"

#include <initializer_list>

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Program;
constexpr int screens_slot = -0x7bb0, league_slot = -0x7ab8, primary_slot = -0x7b78,
              contracts_slot = -0x7b7c, clubs_slot = -0x7be8, money_constants = -0x56d0;
constexpr std::uint32_t find_league = 0x10103510, buying_budget = 0x100f5770,
                        league_wages = 0x100f4790, exceptional_wages = 0x100f2680,
                        primary_contract = 0x10082f20, primary_wages = 0x10084b60,
                        open_history = 0x1073e4b0, write_history = 0x1073f1e0,
                        estimate_player_value = 0x10084be0, convert_money = 0x1000a170;
constexpr std::uint32_t integer_bias_word = 0x43300000;
constexpr std::uint32_t maximum_budget = 5'000'000;
constexpr std::uint32_t default_date = 0x76c;

std::uint32_t extended(std::int32_t value) { return std::uint32_t(value); }

struct HistoryValue {
    unsigned slot;
    std::uint32_t value, back;
};

void publish(Program& program, std::uint32_t screen, std::initializer_list<HistoryValue> fields) {
    for (const auto& [slot, value, back] : fields)
        program.call(write_history, back, {screen, slot, value, 0});
}

std::uint32_t club_for_screen(Cpu& c, std::uint32_t screen, std::uint32_t group_offset) {
    return c.u32(c.u32(screen + group_offset + 4) + 0x24);
}

bool begin_negotiation(Cpu& c, Program& program, std::uint32_t screen, std::uint32_t back) {
    const auto opened = std::int8_t(program.call(open_history, back, {
        screen, c.toc(-0x56e0), c.toc(-0x56e4), 1, c.toc(-0x56e8), 0}));
    c.set(0, extended(opened));
    return compare(c, opened) != 0;
}

std::uint32_t clamp_budget(Cpu& c, std::uint32_t amount) {
    if (compare(c, std::int32_t(amount)) < 0) return 0;
    return compare(c, maximum_budget, amount) < 0 ? maximum_budget : amount;
}

// Preserve the integer-conversion temporary and FPSCR effects, including the
// high word of the floating-point result written back to the guest stack.
double signed_amount(Cpu& c, Fpu& fpu, std::uint32_t value, std::uint32_t at) {
    c.put32(at + 4, value ^ 0x80000000u);
    c.set_f(1, c.f64(c.toc(money_constants) + 8));
    c.put32(at, integer_bias_word);
    const auto result = fpu.fsub(c.f64(at), c.f(1));
    c.set_f(0, result);
    c.set(0, integer_bias_word);
    return result;
}

std::uint32_t budget_valuation(Cpu& c, Program& program, const Frame& frame, std::uint32_t value,
                              std::uint32_t back) {
    Fpu fpu{c};
    const auto amount = signed_amount(c, fpu, value, frame.at(0x44));
    const auto multiplier = c.f64(c.toc(money_constants) + 0x28);
    c.set_f(2, multiplier);
    c.put64(frame.at(0x3c), fpu.fctiwz(0, fpu.fmul(multiplier, amount)));
    const auto scaled = c.u32(frame.at(0x40));
    c.put32(frame.at(0x38), scaled);
    c.set({{0, scaled}, {5, c.toc(money_constants)}});
    return program.call(convert_money, back, {frame.at(0x38), 4});
}

bool use_exceptional_wages(Cpu& c, std::uint32_t kind) {
    const auto signed_kind = std::int8_t(kind);
    c.set(0, extended(signed_kind));
    for (const auto exceptional : {11, 12, 15, 13})
        if (compare(c, signed_kind, exceptional) == 0) return true;
    return false;
}

struct WageSites {
    std::uint32_t league, league_quote, first_contract, second_contract, current_wages,
                  exceptional_league, exceptional_quote;
};

void wage_limit(Cpu& c, Program& program, const Frame& frame, std::uint32_t person, std::uint32_t club,
                std::uint32_t kind, std::uint32_t exceptional_argument,
                std::uint32_t leagues, std::uint32_t contracts, std::uint32_t clubs, std::uint32_t primary,
                WageSites sites) {
    Fpu fpu{c};
    const auto constants = c.toc(money_constants);
    if (use_exceptional_wages(c, kind)) {
        const auto league = program.call(find_league, sites.exceptional_league, {c.u32(leagues), club});
        const auto amount = program.call(exceptional_wages, sites.exceptional_quote,
                                         {league, person, exceptional_argument});
        c.set({{3, amount ^ 0x80000000u}, {4, constants}});
        c.set_f(31, signed_amount(c, fpu, amount, frame.at(0x3c)));
        return;
    }

    const auto league = program.call(find_league, sites.league, {c.u32(leagues), club});
    const auto ceiling = program.call(league_wages, sites.league_quote, {league, kind});
    c.set(5, constants);
    c.set_f(31, signed_amount(c, fpu, ceiling, frame.at(0x3c)));

    const auto contract_club = [&](std::uint32_t back) {
        const auto contract = program.call(primary_contract, back, {c.u32(contracts), person});
        const bool exists = compare_logical(c, contract) != 0;
        c.put32(primary, contract);
        std::uint32_t home = 0;
        if (exists) {
            const auto index = c.u32(contract + 4);
            const auto base = c.u32(clubs);
            c.set(3, base);
            home = base + index * 0x248;
        }
        c.set(0, home);
        return home;
    };
    if (compare_logical(c, contract_club(sites.first_contract)) == 0) return;
    // The screen deliberately looks up the primary contract twice.
    if (compare_logical(c, contract_club(sites.second_contract), club) != 0) return;

    const auto amount = program.call(primary_wages, sites.current_wages, {c.u32(contracts), person});
    c.set({{3, amount ^ 0x80000000u}, {4, constants}});
    const auto current = signed_amount(c, fpu, amount, frame.at(0x3c));
    fpu.fcmp(0, current, c.f(31));
    if (c.cr(0) & 4) c.set_f(31, signed_amount(c, fpu, amount, frame.at(0x3c)));
}

std::uint32_t rounded_limit(Cpu& c, const Frame& frame) {
    c.put64(frame.at(0x3c), Fpu(c).fctiwz(0, c.f(31)));
    return c.u32(frame.at(0x40));
}
}

void open_new_negotiation(Cpu& c, std::uint32_t person, std::uint32_t state,
                          std::uint32_t kind, std::uint32_t mode) {
    Frame frame(c, 0x100960d0, 0x90, {.first_gpr = 22, .gpr_offset = -0x38, .first_fpr = 31});
    const auto screens = c.toc(screens_slot), leagues = c.toc(league_slot), primary = c.toc(primary_slot),
               contracts = c.toc(contracts_slot), clubs = c.toc(clubs_slot);
    keep(c, {{22, state}, {23, mode}, {24, person}, {25, kind},
             {27, leagues}, {28, primary}, {29, contracts}, {30, clubs}, {31, screens}});
    const auto group_offset = extended(c.i16(screens + 0x2c58)) * 0x2c0;
    const auto club = club_for_screen(c, screens, group_offset);
    c.set(0, group_offset);
    c.set(26, club);
    Program program{c};
    if (!begin_negotiation(c, program, screens, 0x10096138)) return;

    publish(program, screens, {
        {0, c.u32(person), 0x10096154}, {1, extended(std::int16_t(mode)), 0x10096168},
        {2, 0, 0x1009617c}, {3, 0, 0x10096190}, {6, 0, 0x100961a4}, {7, 0, 0x100961b8},
        {8, extended(std::int8_t(kind)), 0x100961cc}, {9, 0, 0x100961e0},
        {10, default_date, 0x100961f4}, {11, 0, 0x10096208}, {12, default_date, 0x1009621c},
        {13, extended(std::int8_t(state)), 0x10096230}, {14, 0, 0x10096244}, {15, 0, 0x10096258},
        {16, 0, 0x1009626c}, {17, 0, 0x10096280}, {18, 0, 0x10096294},
        {19, 0, 0x100962a8}, {20, 0, 0x100962bc}});
    const auto league = program.call(find_league, 0x100962c8, {c.u32(leagues), club});
    program.call(buying_budget, 0x100962e0, {league, person, 0, 0, 1, 0});
    const auto budget = c.r(4);
    c.set(0, budget);
    publish(program, screens, {{21, clamp_budget(c, budget), 0x10096314}, {22, 0, 0x10096328}});
    const auto profile = c.u32(person + 0x14);
    c.set(0, profile);
    if (compare_logical(c, profile) != 0) {
        const auto value = program.call(estimate_player_value, 0x10096350,
                                        {c.u32(contracts), person, club, 1, 0xffffffff, 0});
        publish(program, screens, {{23, budget_valuation(c, program, frame, value, 0x10096394), 0x100963a8}});
    } else {
        publish(program, screens, {{23, 0, 0x100963c0}});
    }

    wage_limit(c, program, frame, person, club, kind, 0xffffffff, leagues, contracts, clubs, primary,
        {0x100963f0, 0x100963f8, 0x10096424, 0x1009645c, 0x10096494, 0x100964dc, 0x100964e8});
    publish(program, screens, {
        {24, rounded_limit(c, frame), 0x10096524}, {25, 0, 0x10096538}, {26, 0, 0x1009654c}});
    const auto contract = program.call(primary_contract, 0x10096558, {c.u32(contracts), person});
    publish(program, screens, {
        {27, compare(c, contract) ? c.u32(contract + 0x50) : 0, 0x1009657c},
        {28, 3, 0x10096590}, {29, 0, 0x100965a4}});
}

void open_existing_negotiation(Cpu& c, std::uint32_t link, std::uint32_t person,
                               std::uint32_t state, std::uint32_t kind, std::uint32_t mode) {
    Frame frame(c, 0x100965c0, 0xa0, {.first_gpr = 19, .gpr_offset = -0x44, .first_fpr = 31});
    const auto screens = c.toc(screens_slot), leagues = c.toc(league_slot), primary = c.toc(primary_slot),
               contracts = c.toc(contracts_slot), clubs = c.toc(clubs_slot);
    keep(c, {{19, clubs}, {20, screens + 4}, {21, screens + 0x2c58}, {22, leagues},
             {23, primary}, {24, contracts}, {25, screens}, {26, link}, {27, person},
             {28, state}, {29, kind}, {30, mode}});
    auto group_offset = extended(c.i16(screens + 0x2c58)) * 0x2c0;
    const auto club = club_for_screen(c, screens, group_offset);
    c.set(31, club);
    c.set(0, group_offset);
    Program program{c};
    if (!begin_negotiation(c, program, screens, 0x10096630)) return;

    publish(program, screens, {
        {1, extended(std::int16_t(mode)), 0x1009664c}, {2, 0, 0x10096660}, {3, link, 0x10096674},
        {8, extended(std::int8_t(kind)), 0x10096688}, {9, 0, 0x1009669c},
        {10, default_date, 0x100966b0}, {11, 0, 0x100966c4}, {12, default_date, 0x100966d8},
        {13, extended(std::int8_t(state)), 0x100966ec}});
    const auto offer = [&] { return c.u32(link); };
    if (compare_logical(c, link) != 0) {
        publish(program, screens, {{0, c.u32(person), 0x10096708}});
        publish(program, screens, {{7, extended(c.i8(offer() + 0x35)), 0x10096724}});
        publish(program, screens, {{4, c.u32(offer() + 0x28), 0x1009673c}});
        publish(program, screens, {{5, c.u32(offer() + 0x2c), 0x10096754}});
        const auto current_person = c.u32(offer() + 0xc), selected_person = c.u32(person);
        c.set(0, selected_person);
        const bool exchange = compare(c, current_person, selected_person) != 0;
        publish(program, screens, {{6, exchange, exchange ? 0x10096794u : 0x1009677cu}});
    } else {
        publish(program, screens, {{0, c.u32(person), 0x100967ac}, {6, 0, 0x100967c0}, {7, 0, 0x100967d4}});
    }

    publish(program, screens, {{14, 0, 0x100967e8}, {15, 0, 0x100967fc}, {16, 0, 0x10096810},
        {17, 0, 0x10096824}, {18, 0, 0x10096838}, {19, 0, 0x1009684c}, {20, 0, 0x10096860}});
    group_offset = extended(c.i16(screens + 0x2c58)) * 0x2c0;
    const auto current_club = club_for_screen(c, screens, group_offset);
    c.set({{0, group_offset}, {30, current_club}});
    const auto league = program.call(find_league, 0x1009687c, {c.u32(leagues), current_club});
    program.call(buying_budget, 0x10096894, {league, person, 0, 0, 1, 0});
    auto budget = c.r(4);
    if (compare_logical(c, link) != 0) {
        const auto record = offer();
        if (compare_logical(c, record) != 0) {
            const auto fee = c.u32(record + 0x20);
            c.set(0, fee);
            budget -= fee;
        }
    }
    publish(program, screens, {{21, clamp_budget(c, budget), 0x100968e8}});

    auto profile = c.u32(person + 0x14);
    c.set(0, profile);
    if (compare_logical(c, profile) != 0) {
        const auto quote = program.call(0x10081b10, 0x10096904, {c.u32(contracts), person, current_club});
        publish(program, screens, {{22, quote, 0x10096918}});
    } else {
        publish(program, screens, {{22, 0, 0x10096930}});
    }

    profile = c.u32(person + 0x14);
    c.set(0, profile);
    if (compare_logical(c, profile) != 0) {
        const auto offset = extended(c.i16(screens + 0x2c58)) * 0x2c0;
        c.set(0, offset);
        const auto value = program.call(estimate_player_value, 0x10096964,
            {c.u32(contracts), person, club_for_screen(c, screens, offset), 1, 0xffffffff, 0});
        publish(program, screens, {{23, budget_valuation(c, program, frame, value, 0x100969a8), 0x100969bc}});
    } else {
        publish(program, screens, {{23, 0, 0x100969d4}});
    }

    wage_limit(c, program, frame, person, club, kind, state, leagues, contracts, clubs, primary,
        {0x10096a04, 0x10096a0c, 0x10096a38, 0x10096a70, 0x10096aa8, 0x10096af0, 0x10096afc});
    publish(program, screens, {
        {24, rounded_limit(c, frame), 0x10096b38}, {25, 0, 0x10096b4c}, {26, 0, 0x10096b60}});
    const auto contract = program.call(primary_contract, 0x10096b6c, {c.u32(contracts), person});
    c.set({{0, contract}, {19, contract}});
    publish(program, screens, {{27, compare(c, contract) ? c.u32(contract + 0x50) : 0, 0x10096b94}});
    if (compare_logical(c, contract) != 0) {
        const auto permitted = std::int8_t(program.call(0x10380f20, 0x10096bb0, {contract, current_club, 0, 0}));
        c.set(0, extended(permitted));
        if (compare(c, permitted) != 0 || compare_logical(c, link) != 0) {
            publish(program, screens, {{28, 3, 0x10096bd4}, {29, 0, 0x10096be8}});
            return;
        }
    }
    publish(program, screens, {{28, 0, 0x10096c00}});
    if (compare_logical(c, contract) != 0) {
        profile = c.u32(person + 0x14);
        c.set(0, profile);
        if (compare_logical(c, profile) != 0) {
            publish(program, screens, {{29, 1, 0x10096c40}});
            return;
        }
    }
    publish(program, screens, {{29, 0, 0x10096c28}});
}
}

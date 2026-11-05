#include "bra_nat_second_adapter.hpp"
#include "guest_abi.hpp"
#include "import_descriptor_adapter.hpp"
#include "localization_format_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

#include <array>
#include <random>

#ifdef CM_BRA_NAT_SECOND_PROFILE_DIR
#include <unistd.h>
extern "C" int __llvm_profile_write_file();
extern "C" void __llvm_profile_set_filename(const char*);
#endif

namespace {
using namespace cm::aot;
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto group_entry = 0x10518b10u, times_entry = 0x10519540u, base_entry = 0x105195d0u;
constexpr auto construct_entry = 0x105177b0u, destroy_entry = 0x10517af0u;
constexpr auto advance_groups_entry = 0x10519010u, create_group_entry = 0x105190d0u;
constexpr auto record_entry = 0x105192c0u, announce_entry = 0x10519390u;
constexpr auto next_season_entry = 0x10518d70u;
constexpr auto calendar_entry = 0x10517c00u, collect_entry = 0x10518b50u, configure_entry = 0x10518ca0u;
constexpr auto league_init = 0x1049e4f0u, load_test = fixture + 0x102u;
constexpr auto reset_fixtures = 0x104564a0u, finish_season = 0x104aa3a0u, notify_season = 0x102d46c0u;
constexpr auto season_manager = fixture + 0x6e00;
constexpr auto fixtures_new = 0x10773520u, fixtures_init = 0x104533a0u, fixtures_destroy = 0x104536c0u;
constexpr auto initialize_table = 0x104a00a0u, register_league = 0x1049eb30u;
constexpr auto clear_standings = 0x104a6660u, base_destroy = 0x1049e600u;
constexpr auto release = 0x10775170u, object_delete = 0x107735a0u;
constexpr auto finalize = fixture + 0x104u, child_destroy = fixture + 0x106u, configure_external = fixture + 0x108u;
constexpr auto allocate = 0x10775130u, insert_club = 0x104a60e0u;
constexpr auto regular_date = 0x104a9810u, match_slot = 0x104a9890u;
constexpr auto playoff_round = 0x104854f0u, playoff_date = 0x10485570u;
constexpr auto source_path = 0x10739890u, format = 0x1077b720u, report = 0x10769b60u;
constexpr auto lookup = 0x10459b50u, callback = 0x30000100u, done = 0x30000080u;
constexpr auto division = fixture + 0x400, groups = fixture + 0x500, group = fixture + 0x600;
constexpr auto vtable = fixture + 0x700, descriptor = fixture + 0x800, fixtures = fixture + 0x900;
constexpr auto match = fixture + 0xa00, alternate_toc = toc + 0x20000;
constexpr auto rows = fixture + 0x1000, clubs = fixture + 0x2000, clubs_stride = 0x248u;
constexpr auto source = fixture + 0x6800, message = fixture + 0x6900, success = fixture + 0x6a00;
constexpr auto second_division = fixture + 0x6b00, other_division = fixture + 0x6b10;
constexpr auto alloc_failure = fixture + 0x6c00, mutate_year = fixture + 0x6c01;
constexpr auto calendar_descriptor = descriptor + 8, lifecycle_mode = fixture + 0x6c02;
constexpr auto home_definition = fixture + 0x7000, standings = fixture + 0x8000;
constexpr auto groups_storage = fixture + 0x9000, other_group = fixture + 0x9100;
constexpr auto seeds = fixture + 0x9300, history_manager = fixture + 0x9400;
constexpr auto news_item = fixture + 0x9500, news_text = fixture + 0x9600;
constexpr auto base_qualification_query = fixture + 0x10a;
constexpr auto classify_table = 0x104a1c30u, exchange_clubs = 0x104a9210u;
constexpr auto block_move = fixture + 0x10c, classification_flags = fixture + 0x9800;
constexpr auto classification_news = 0x104a44f0u, continental_qualification = 0x104646d0u;
constexpr auto foreign_player_rule = 0x104ae570u;
constexpr auto notify_non_promoted = 0x1007fd00u, notify_division_change = 0x1007fe90u;
constexpr auto notify_squad_rule_change = 0x1007ffb0u;
constexpr auto playoff_group_init = 0x104bc3c0u, playoff_group_start = 0x1047ef50u;
constexpr auto standing_lookup = 0x104aa560u, league_award = 0x103822e0u;
constexpr auto relegation_history = 0x10382e50u, winner_history = 0x10381b70u;
constexpr auto generic_news = 0x10450030u, format_news = 0x10727890u;
constexpr auto set_news_text = 0x1074b830u, news_link = 0x1074bab0u;
constexpr auto load_descriptor = descriptor + 16, configure_descriptor = descriptor + 24;
constexpr auto finalize_descriptor = descriptor + 32, child_descriptor = descriptor + 40;
enum LifecycleMode : unsigned {
    lifecycle_success, load_existing, fail_group_array, fail_calendar, fail_configure,
    fail_standings, fail_table, fail_register, fail_fixtures, shorten_group_scan, fail_seeds, fail_group,
};
using Call = std::array<std::uint32_t, 20>;
std::vector<Call>* calls = nullptr;
bool bad_argument = false;

bool service(State& s) {
    switch (s.pc) {
    case lookup: case callback: case allocate: case insert_club:
    case regular_date: case match_slot: case playoff_round: case playoff_date:
    case source_path: case format: case report:
    case league_init: case load_test: case initialize_table: case register_league:
    case fixtures_new: case fixtures_init: case fixtures_destroy: case finalize:
    case child_destroy: case clear_standings: case base_destroy: case release:
    case object_delete: case configure_external: case reset_fixtures: case finish_season: case notify_season:
    case playoff_group_init: case playoff_group_start: case standing_lookup: case league_award:
    case relegation_history: case winner_history: case generic_news: case format_news:
    case set_news_text: case news_link: case base_qualification_query:
    case block_move: case classification_news: case continental_qualification: case foreign_player_rule:
    case notify_non_promoted: case notify_division_change: case notify_squad_rule_change: break;
    default: return false;
    }
    Cpu c(s);
    Call call{s.pc, c.lr(), c.sp(), c.r(2), c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8),
              c.r(9), c.r(10), c.r(11), c.r(12), c.r(26), c.r(27), c.r(28), c.r(29), c.r(30), c.r(31)};
    if (bad_argument) ++call[5];
    calls->push_back(call);
    if (s.pc == lookup) {
        const bool nested = c.sp() == stack - 0xa0 - 0x50 && c.u32(stack - 0xa0) == stack && c.u32(stack + 8) == done;
        require(c.r(3) == fixtures && c.r(2) == toc && c.lr() == 0x10519574 &&
                    (c.sp() == stack - 0x50 || nested),
                "fixture lookup arguments");
        c.set(3, match);
    } else if (s.pc == callback) {
        require((c.r(3) == group || c.r(3) == other_group) && c.r(2) == alternate_toc &&
                    c.lr() == 0x10518b30 && c.sp() == stack - 0x40,
                "first group callback arguments");
        c.put32(c.r(3) + 4, c.r(4));
        c.set(3, c.r(4) ^ c.r(5));
    } else if (s.pc == allocate) {
        const auto size = c.r(3);
        require(size == 4 || size == 16 || size == 28 || size == 0x804 || size == 0xf0 || size == 28 * 0x44, "allocation size");
        const auto mode = c.u8(lifecycle_mode);
        if (size == 4) c.set(3, mode == fail_group_array ? 0 : groups_storage);
        else if (size == 16) c.set(3, mode == fail_seeds ? 0 : seeds);
        else if (size == 28) c.set(3, classification_flags);
        else if (size == 28 * 0x44) c.set(3, c.u8(alloc_failure) || mode == fail_standings ? 0 : standings);
        else c.set(3, c.u8(alloc_failure) || mode == fail_calendar ? 0 : rows);
    } else if (s.pc == insert_club) {
        require(c.r(3) == division && c.r(4) < 28 && c.r(5) >= clubs && !c.r(6) && !c.r(7),
                "ordered standing insertion");
        c.put32(c.u32(division + 0xb0) + c.r(4) * 0x44, c.r(5));
    } else if (s.pc == regular_date || s.pc == playoff_round || s.pc == playoff_date || s.pc == match_slot) {
        const auto round = c.r(4);
        require(c.r(3) == rows && round < (s.pc == playoff_round || s.pc == playoff_date ? 2 : 27),
                "schedule builder row and round");
        if (s.pc == regular_date || s.pc == playoff_date)
            require(!c.u32(c.sp() + 0x38), "stack date argument");
        const auto size = s.pc == playoff_round || s.pc == playoff_date ? 0x78u : 0x4cu;
        const auto offset = s.pc == match_slot ? 0x20 + c.r(5) * 4 : s.pc == playoff_date ? 4 : 0;
        c.put32(rows + round * size + offset, c.r(6) << 24 | c.r(7) << 16 | c.r(8) << 8 | c.r(9));
        if (s.pc == regular_date && c.u8(mutate_year))
            c.put16(division + 0x40, c.u16(division + 0x40) + 1);
        c.set(3, c.r(5) ^ c.r(6));
    } else if (s.pc == source_path) {
        require(c.r(3) == source && !c.r(4) && !c.r(5), "assertion source arguments");
        c.put8(c.r(6), 'B');
        c.put8(c.r(7), 'C');
    } else if (s.pc == format) {
        require(c.r(3) == message && c.r(4) == source + 0x13 && c.r(5) == 0x12345678 &&
                    c.u8(c.r(6)) == 'B' && c.u8(c.r(7)) == 'C', "assertion format arguments");
        c.put32(message, c.r(8));
    } else if (s.pc == report) {
        require(c.r(3) == source + 0x20 && c.r(4) == message && !c.r(5), "assertion report arguments");
    } else {
        const auto mode = c.u8(lifecycle_mode);
        switch (s.pc) {
        case block_move:
            require(c.r(3) == division + 0xc0 && c.r(4) == c.sp() + 0x3c &&
                        c.r(5) == 4, "classification settings copy");
            c.put32(c.r(4), c.u32(c.r(3)));
            break;
        case classification_news:
        case continental_qualification:
        case foreign_player_rule:
            c.set(3, 0);
            break;
        case notify_non_promoted: case notify_division_change: case notify_squad_rule_change:
            // Movement notifications are external to the division exchange.
            break;
        case base_qualification_query:
            require(c.r(3) == division && c.r(2) == toc, "base table qualification arguments");
            c.set(3, 0);
            break;
        case league_init:
            require(c.r(3) == division && c.lr() == 0x105177e0, "base constructor arguments");
            c.put32(division + 0x8, 0);
            break;
        case load_test:
            require(c.r(3) == division && c.r(4) == 1 && c.r(2) == alternate_toc, "load check descriptor");
            c.set(3, mode == load_existing);
            break;
        case configure_external:
            require(c.r(3) == division && c.r(2) == alternate_toc &&
                        (c.lr() == 0x10517898 || c.lr() == 0x10518e38),
                    "configuration override arguments");
            c.set(3, 0);
            break;
        case initialize_table:
            require(c.r(3) == division && c.u32(division + 0xb0) == standings, "table initialization arguments");
            c.put8(division + 0x62, 1);
            c.set(3, mode != fail_table);
            break;
        case register_league:
            require(c.r(3) == division && !c.r(4), "league registration arguments");
            c.put8(division + 0x63, 1);
            c.set(3, mode != fail_register);
            break;
        case fixtures_new:
            if (c.lr() == 0x105191b8) {
                require(c.r(3) == 0xb4, "playoff group allocation size");
                c.set(3, mode == fail_group ? 0 : other_group);
            } else {
                require(c.r(3) == 0x634 && c.lr() == 0x10517a40, "fixture allocation arguments");
                c.set(3, mode == fail_fixtures ? 0 : fixtures);
            }
            break;
        case fixtures_init:
            require(c.r(3) == fixtures && c.r(4) == division, "fixture construction arguments");
            c.put32(fixtures + 8, c.r(4));
            break;
        case finalize:
            require(c.r(3) == division && c.r(2) == alternate_toc &&
                        (c.lr() == 0x10517acc ? c.u32(division + 8) == fixtures : c.lr() == 0x10518fd8),
                    "finalization descriptor arguments");
            c.put8(division + 0x64, 1);
            break;
        case clear_standings:
        case base_destroy:
            require(c.r(3) == division && !c.r(4), "base teardown arguments");
            c.put32(division + (s.pc == clear_standings ? 0xb0 : 0), 0);
            break;
        case child_destroy:
            require((c.r(3) == group || c.r(3) == other_group) && c.r(4) == 1 &&
                        c.r(2) == alternate_toc, "child teardown descriptor arguments");
            c.put32(c.r(3) + 4, 0);
            if (mode == shorten_group_scan) c.put32(division + 0x30, 0);
            break;
        case fixtures_destroy:
        case reset_fixtures:
            require(c.r(3) == fixtures && c.r(4) == 1, "fixture teardown arguments");
            c.put32(fixtures + 8, 0);
            break;
        case release:
            require(!c.r(3) || c.r(3) == rows || c.r(3) == groups_storage ||
                        c.r(3) == seeds || c.r(3) == classification_flags, "release arguments");
            if (c.r(3)) c.put8(c.r(3), 0);
            break;
        case object_delete:
            require(c.r(3) == division, "operator delete arguments");
            c.put8(division, 0x7f);
            break;
        case finish_season:
            require(c.r(3) == division && c.lr() == 0x10518fe4, "base new-season completion");
            c.put8(division + 0x65, 1);
            break;
        case notify_season:
            require(c.r(3) == season_manager && c.r(4) == home_definition && c.lr() == 0x10518ff4,
                    "new-season notification arguments");
            c.put8(division + 0x66, 1);
            break;
        case playoff_group_init:
            require(c.r(3) == other_group && c.r(4) == division && c.r(5) == 4 && c.r(6) == seeds &&
                        c.r(8) == home_definition && c.r(10) == std::uint32_t(c.i16(division + 0x40)),
                    "playoff group constructor arguments");
            for (const auto [position, standing] : std::array<std::array<unsigned, 2>, 4>{{{0,0},{1,3},{2,1},{3,2}}})
                require(c.u32(seeds + position * 4) == c.u32(standings + standing * 0x44),
                        "playoff seed order");
            require(c.u32(c.sp() + 0x38) == 0 && c.u32(c.sp() + 0x3c) == 1 &&
                        c.u32(c.sp() + 0x44) == 20, "playoff group stack arguments");
            c.put32(other_group, vtable);
            c.put32(other_group + 4, c.r(9));
            break;
        case playoff_group_start:
            require(c.r(3) == other_group && !c.r(4), "playoff group start");
            c.put8(other_group + 8, 1);
            break;
        case standing_lookup:
            require(c.r(3) == division && !c.r(5), "history standing lookup");
            c.set(3, standings + (c.r(4) & 3) * 0x44);
            break;
        case league_award:
        case relegation_history:
        case winner_history:
            require(c.r(3) == history_manager && c.r(5) == home_definition &&
                        (s.pc != league_award || c.r(6) == 100), "competition history arguments");
            c.put32(history_manager + 4, c.r(4));
            break;
        case generic_news:
            require(c.r(3) == division && !c.r(10) && c.u32(c.sp() + 0x40) == news_item, "base news ABI");
            c.put32(news_item, c.u32(c.sp() + 0x38));
            break;
        case format_news:
            require(c.r(3) == news_text && (c.r(4) == source + 0x26 || c.r(4) == source + 0x54) &&
                        c.r(5) == c.r(6) && c.r(7) == c.r(8) && c.r(10) == home_definition + 0x4d,
                    "playoff news text arguments");
            c.put32(news_text, c.r(4));
            break;
        case set_news_text:
            require(c.r(3) == news_item && c.r(4) == news_text, "playoff news body");
            c.put32(news_item, c.u32(news_text));
            break;
        case news_link:
            require(c.r(3) == news_item && ((c.r(5) == 0x7d5 && c.r(4) == c.r(6) + 0x221) ||
                        (c.r(5) == 0x7d0 && c.r(4) == home_definition + 0x4d && c.r(6) == home_definition)),
                    "playoff news link arguments");
            c.put32(news_item + (c.r(5) == 0x7d5 ? 4 : 8), c.r(4));
            break;
        }
    }
    c.finish();
    return true;
}

struct Services {
    decltype(dispatch_probe) previous = dispatch_probe;
    Services(std::vector<Call>& out, bool corrupt) {
        calls = &out;
        bad_argument = corrupt;
        dispatch_probe = service;
    }
    ~Services() { dispatch_probe = previous; calls = nullptr; bad_argument = false; }
};

std::unique_ptr<State> seed(std::uint32_t entry, std::uint32_t stage,
                            std::uint32_t first, std::uint32_t second, std::mt19937& random) {
    auto state = std::make_unique<State>();
    state->pc = entry;
    state->memory.map(fixture, 0x1000);
    state->memory.map(stack - 0x1000, 0x2000);
    Cpu c(*state);
    for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, random());
    for (unsigned cr = 0; cr < 8; ++cr) c.set_cr(cr, random() & 15);
    for (unsigned off = 0; off < 0x100; off += 4) c.put32(stack - off, random());
    c.set_lr(done);
    c.set({{1, stack}, {2, toc}, {3, division}, {4, random()}, {5, stage},
           {6, random()}, {7, first}, {8, second}});
    c.put32(division + 8, fixtures);
    c.put32(division + 0xc, groups);
    c.put32(groups, group);
    c.put32(group, vtable);
    c.put32(vtable + 0x38, descriptor);
    c.put32(descriptor, callback);
    c.put32(descriptor + 4, alternate_toc);
    if (entry == base_entry) {
        c.set(5, match);
        c.put8(match + 0x43, first);
        c.put8(match + 0x44, second);
        c.put32(match + 0x1c, c.r(4));
        c.put8(division + 0xc4, 0x25);
        c.put16(division + 0xa8, 0);
    }
    return state;
}

std::unique_ptr<State> calendar_seed(std::uint32_t entry, std::mt19937& random, unsigned phase,
                                    unsigned club_count, bool failure, bool optional, bool live_year) {
    auto s = seed(entry, 0, 0, 0, random);
    s->memory.map(rows, 0x9000);
    s->memory.map(toc - 0x8000, 0x8000);
    Cpu c(*s);
    c.put32(toc - 0x4550, source);
    c.put32(toc - 0x7ba8, message);
    c.put32(toc - 0x7bac, success);
    c.put32(toc - 0x5b20, 0x12345678);
    c.put32(toc - 0x7b50, fixture + 0x6d00);
    c.put32(toc - 0x6b7c, second_division);
    c.put32(toc - 0x7be8, fixture + 0x6d04);
    c.put32(toc - 0x6b88, fixture + 0x6d08);
    c.put32(toc - 0x6b48, fixture + 0x6d0c);
    c.put32(fixture + 0x6d00, club_count);
    c.put32(fixture + 0x6d04, clubs);
    c.put32(fixture + 0x6d08, 31);
    c.put32(fixture + 0x6d0c, 33);
    c.put32(second_division, 32);
    c.put32(other_division, 34);
    c.put8(success, 1);
    c.put8(alloc_failure, failure);
    c.put8(mutate_year, live_year);
    c.put16(division + 0x40, live_year ? 0xffff : 2002);
    c.put32(division, vtable);
    c.put32(vtable + 0x44, calendar_descriptor);
    c.put32(calendar_descriptor, calendar_entry);
    c.put32(calendar_descriptor + 4, toc);
    for (unsigned n = 0; n < club_count; ++n)
        c.put32(clubs + n * clubs_stride + 0x30, n == 1 ? 0u : n == 3 ? other_division : second_division);
    if (entry == calendar_entry)
        c.set({{4, phase}, {5, division + 0xa6}, {6, division + 0x3a},
               {7, optional ? division + 0x4d : 0}});
    return s;
}

std::unique_ptr<State> lifetime_seed(std::uint32_t entry, std::mt19937& random, LifecycleMode mode,
                                    std::uint32_t deletion, unsigned populated) {
    auto s = calendar_seed(entry, random, 0, 32, false, false, false);
    s->budget = 3000;
    Cpu c(*s);
    c.put32(toc - 0x6044, vtable);
    c.put32(toc - 0x7428, season_manager + 4);
    c.put32(season_manager + 4, season_manager);
    c.put32(division + 4, home_definition);
    c.put8(lifecycle_mode, mode);
    c.put32(vtable + 0x20, load_descriptor);
    c.put32(vtable + 0x64, finalize_descriptor);
    c.put32(vtable + 0x94, configure_descriptor);
    c.put32(vtable + 8, child_descriptor);
    for (const auto& [at, target] : std::array<std::pair<std::uint32_t, std::uint32_t>, 4>{{
            {load_descriptor, load_test}, {finalize_descriptor, finalize},
            {configure_descriptor, mode == fail_configure ? configure_external : configure_entry},
            {child_descriptor, child_destroy}}}) {
        c.put32(at, target);
        c.put32(at + 4, target == configure_entry ? toc : alternate_toc);
    }
    if (entry == construct_entry) {
        c.set({{4, 2001}, {5, home_definition}});
    } else {
        c.set(4, deletion);
        c.put32(division + 0x30, populated == 0 ? UINT32_MAX : 2);
        c.put32(division + 0xbc, populated ? rows : 0);
        c.put32(division + 0xc, populated ? groups_storage : 0);
        c.put32(division + 8, populated ? fixtures : 0);
        c.put32(groups_storage, group);
        c.put32(groups_storage + 4, 0);
        c.put32(groups_storage + 8, other_group);
        c.put32(group, vtable);
        c.put32(other_group, vtable);
        if (entry == destroy_entry && populated == 2) c.set(3, 0);
        if (entry == next_season_entry) c.put16(division + 0x40, deletion);
    }
    return s;
}

std::unique_ptr<State> result_seed(std::uint32_t entry, std::mt19937& random, LifecycleMode mode,
                                  unsigned phase, unsigned outcome, unsigned personal) {
    auto s = lifetime_seed(entry, random, mode, 0, 1);
    Cpu c(*s);
    c.put32(division + 0x2c, 1);
    c.put32(division + 0x30, UINT32_MAX);
    c.put32(division + 0xb0, standings);
    c.put32(toc - 0x7bb8, news_text);
    c.put32(toc - 0x7b48, history_manager);
    c.put32(history_manager, history_manager);
    c.put8(home_definition + 0x67, 0x80);
    for (unsigned n = 0; n < 4; ++n) {
        c.put32(standings + n * 0x44, n + 500);
        c.put8(clubs + n * clubs_stride + 0x23b, 0xff - n);
    }
    c.set({{4, random() & 3}, {5, outcome}, {6, phase}});
    c.put8(stack + 0x3b, random());
    c.put8(stack + 0x3f, personal);
    c.put32(stack + 0x40, news_item);
    return s;
}

enum class Corrupt { no, result, argument };
enum class Stop { returned, budget, unmapped };
Stop compare_case(const State& initial, Corrupt corruption = Corrupt::no,
                  std::uint32_t defer_probe_until_entry = 0) {
    auto old = clone(initial), modern = clone(initial);
    old->allocator_original = modern->allocator_original = true;
    // Decline the generated in-place hook so both sides use the same
    // external formatter model through the dispatcher.
    localization_format_diagnostics(*old).original_body = format_news;
    localization_format_diagnostics(*modern).original_body = format_news;
    Effects expected, actual;
    expected.state = old.get(); actual.state = modern.get();
    old->memory.date_observer = &expected; modern->memory.date_observer = &actual;
    if (defer_probe_until_entry) {
        // The owned call is entered with the production dispatcher. Once
        // inside, its first guest read enables models for external services.
        expected.on_read = actual.on_read = [=](State& s, Effects& effects, std::uint32_t, unsigned) {
            if (!effects.mutated && s.pc == defer_probe_until_entry) {
                effects.mutated = true;
                dispatch_probe = service;
            }
        };
    }
    std::vector<Call> old_calls, new_calls;
    auto run = [&](State& s, bool original, std::vector<Call>& trace) {
        Services services(trace, !original && corruption == Corrupt::argument);
        if (defer_probe_until_entry && !original) dispatch_probe = nullptr;
        try {
            if (original) bra_nat_second_run_original(s, initial.pc);
            else run_until(s, done);
            return std::pair{Stop::returned, std::string{}};
        } catch (const std::runtime_error& e) {
            const std::string reason(e.what());
            if (reason == "Native instruction budget exhausted") return std::pair{Stop::budget, reason};
            if (reason.starts_with("Unmapped data access at ")) return std::pair{Stop::unmapped, reason};
            throw;
        }
    };
    const auto old_returned = run(*old, true, old_calls);
    const auto new_returned = run(*modern, false, new_calls);
    old->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto label = "Brazilian Second " + hex_address(initial.pc) + " budget=" + std::to_string(initial.budget);
    if (defer_probe_until_entry)
        require(expected.mutated && actual.mutated, label + " owned entry was not reached before enabling the probe");
    require(old_returned == new_returned, label + " stop classification: original=" + old_returned.second +
                " at " + hex_address(old->pc) + " C++=" + new_returned.second + " at " + hex_address(modern->pc));
    if (corruption == Corrupt::result) Cpu(*modern).set(3, Cpu(*modern).r(3) ^ 1);
    require(old_calls == new_calls, label + " call trace");
    if (expected.bytes != actual.bytes) {
        const auto same = [](const auto& left, const auto& right) {
            return left.kind == right.kind && left.address == right.address && left.size == right.size && left.value == right.value;
        };
        const auto [before, after] = std::ranges::mismatch(expected.log, actual.log, same);
        const auto detail = [](auto item, const auto& trace) {
            return item == trace.end() ? std::string("end") :
                std::to_string(item->kind) + "/" + hex_address(item->address) + "/" + std::to_string(item->value);
        };
        throw std::runtime_error(label + " effect " + std::to_string(before - expected.log.begin()) +
                                 " original=" + detail(before, expected.log) + " C++=" + detail(after, actual.log));
    }
    require(old->executed == modern->executed, label + " diagnostic instruction accounting");
    Image(*old).compare(*modern, label, old_returned.first == Stop::unmapped);
    return old_returned.first;
}

void check_calendar_boundaries(std::mt19937& random) {
    constexpr auto descriptor_entry = 0x10781320u;
    constexpr auto external_calendar = 0x10577800u; // Portuguese Second Division calendar ABI.
    enum class Mode { normal, external, original_descriptor, observer, scalar };
    struct DescriptorTrace final : ImportDescriptorBoundaryObserver {
        std::vector<std::array<std::uint32_t, 5>> calls;
        void enter(const State& s, std::uint32_t entry, bool original) override {
            calls.push_back({entry, std::uint32_t(s.r<4>(guest::reg::lr)), s.pc, original, 0});
        }
        void leave(State& s, std::uint32_t entry, bool original, ImportDescriptorOutcome outcome) override {
            calls.push_back({entry, std::uint32_t(s.r<4>(guest::reg::lr)), s.pc, original, 1u + unsigned(outcome)});
        }
    };
    for (const auto mode : {Mode::normal, Mode::external, Mode::original_descriptor, Mode::observer, Mode::scalar}) {
        auto original = calendar_seed(configure_entry, random, 0, 32, false, false, false);
        if (mode == Mode::external) Cpu(*original).put32(calendar_descriptor, external_calendar);
        auto modern = clone(*original);
        original->budget = modern->budget = mode == Mode::external ? 47 : 66;
        DescriptorTrace before, after;
        const auto check = [&](State& s, bool reference, DescriptorTrace& descriptor_trace) {
            if (mode == Mode::observer) s.import_descriptor_observer = &descriptor_trace;
            if (mode == Mode::original_descriptor) s.import_descriptor_original_body = descriptor_entry;
            std::unique_ptr<GuestScalarContextCapture> scalar;
            if (mode == Mode::scalar) scalar = std::make_unique<GuestScalarContextCapture>(&s);
            try {
                if (reference) bra_nat_second_run_original(s, configure_entry);
                else run_until(s, done);
            } catch (const std::runtime_error& error) {
                const auto expected = mode == Mode::external ? external_calendar : allocate;
                require(std::string_view(error.what()) == "Native instruction budget exhausted" && s.pc == expected,
                        "calendar descriptor route stopped outside its expected boundary: " + hex_address(s.pc));
                Cpu c(s);
                require(c.r(2) == toc, "calendar descriptor TOC");
                if (mode == Mode::external) {
                    require(c.sp() == stack - 0x50 && c.r(3) == division && c.r(4) == UINT32_MAX &&
                                c.r(5) == division + 0xa6 && c.r(6) == division + 0x3a,
                            "external calendar override arguments");
                } else {
                    require(c.sp() == stack - 0x100 && c.r(3) == 0x804 &&
                                c.u16(division + 0xa6) == 27 && !c.u16(division + 0x3a),
                            "calendar descriptor must execute the owned calendar");
                }
                s.import_descriptor_observer = nullptr;
                return;
            }
            throw std::runtime_error("calendar boundary did not stop");
        };
        check(*original, true, before);
        check(*modern, false, after);
        require(before.calls == after.calls && before.calls.size() == (mode == Mode::observer ? 2u : 0u),
                "calendar descriptor observer trace");
        Image(*original).compare(*modern, "calendar descriptor route");
    }
}

void check_default_group_boundary(std::mt19937& random) {
    auto original = result_seed(advance_groups_entry, random, lifecycle_success, 0, 1, 0);
    auto modern = clone(*original);
    original->budget = modern->budget = 28;
    const auto check = [](State& s, bool reference, bool observe_scalar) {
        std::unique_ptr<GuestScalarContextCapture> scalar;
        if (observe_scalar) scalar = std::make_unique<GuestScalarContextCapture>(&s);
        try {
            if (reference) bra_nat_second_run_original(s, advance_groups_entry);
            else run_until(s, done);
        } catch (const std::runtime_error& error) {
            require(std::string_view(error.what()) == "Native instruction budget exhausted" && s.pc == allocate,
                    "default direct group route must stop at the external allocator");
            Cpu c(s);
            require(c.sp() == stack - 0x140 && c.r(3) == 16 && c.u32(division + 0x30) == 0,
                    "default direct group route must execute the owned playoff setup");
            return;
        }
        throw std::runtime_error("default group boundary did not stop");
    };
    auto observed = clone(*original);
    observed->budget = 28;
    check(*original, true, false);
    check(*modern, false, false);
    check(*observed, false, true);
    Image(*original).compare(*modern, "default direct group route");
    Image(*original).compare(*observed, "scalar-observed group route");
}

void check_entry_hook(State& s, std::uint32_t entry) {
    RoutineChecker checker([=](auto address) { return address == entry; }, 1, 1);
    checker.strict_volatile = checker.compare_f1 = true;
    std::vector<Call> trace;
    Services services(trace, false);
    s.allocator_original = true;
    localization_format_diagnostics(s).original_body = format_news;
    routine_checker = &checker;
    try { run_until(s, done); } catch (...) { routine_checker = nullptr; throw; }
    routine_checker = nullptr;
    require(checker.totals().at(entry).checked == 1 && !checker.differences(), "strict generated hook");
}

void check_lifetime_sequence(std::mt19937& random) {
    auto original = lifetime_seed(construct_entry, random, lifecycle_success, 0, 0);
    auto modern = clone(*original);
    std::vector<Call> original_trace, modern_trace;
    const auto run = [](State& s, bool reference, std::vector<Call>& trace) {
        Services services(trace, false);
        s.allocator_original = true;
        s.budget = 10000;
        Cpu c(s);
        unsigned year = 2001;
        for (const auto entry : {construct_entry, next_season_entry, next_season_entry, destroy_entry}) {
            s.pc = entry;
            c.set_lr(done);
            c.set(3, division);
            if (entry != construct_entry) c.set(4, 0);
            if (entry == next_season_entry) ++year;
            if (reference) bra_nat_second_run_original(s, entry);
            else run_until(s, done);
            require(c.u16(division + 0x40) == year && c.r(2) == toc && c.sp() == stack,
                    "competition lifetime year and call boundary");
        }
        require(!c.u32(division + 0xbc) && !c.u32(division + 0xc) && c.u8(division + 0x66),
                "competition lifetime final state");
    };
    run(*original, true, original_trace);
    run(*modern, false, modern_trace);
    require(original_trace == modern_trace, "competition lifetime sequence call trace");
    Image(*original).compare(*modern, "competition lifetime sequence");
}

void check_complete_competition_sequence(std::mt19937& random) {
    auto original = result_seed(construct_entry, random, lifecycle_success, 0, 1, 0);
    auto modern = clone(*original);
    std::vector<Call> original_trace, modern_trace;
    Effects original_effects, modern_effects;
    const auto run = [](State& s, bool reference, std::vector<Call>& trace, Effects& effects) {
        Services services(trace, false);
        s.allocator_original = true;
        localization_format_diagnostics(s).original_body = format_news;
        s.budget = 10000;
        effects.state = &s;
        s.memory.date_observer = &effects;
        Cpu c(s);
        const auto invoke = [&](std::uint32_t entry, std::initializer_list<std::pair<unsigned, std::uint32_t>> args) {
            s.pc = entry;
            c.set_lr(done);
            c.set(3, division);
            c.set(args);
            if (reference) bra_nat_second_run_original(s, entry);
            else run_until(s, done);
            require(c.r(2) == toc && c.sp() == stack, "combined competition call boundary");
        };
        invoke(construct_entry, {{4, 2001}, {5, home_definition}});
        require(c.u16(division + 0x40) == 2001 && c.u16(division + 0x3e) == 28 &&
                    c.u32(division + 0xc) == groups_storage, "combined competition construction");

        invoke(advance_groups_entry, {});
        require(c.u32(groups_storage) == other_group && c.u8(other_group + 8),
                "combined competition playoff group");
        invoke(group_entry, {{4, 2}, {5, 0}});
        require(c.u32(other_group + 4) == 2, "combined competition group update");

        invoke(times_entry, {{4, 0}, {5, UINT32_MAX}, {6, 7}, {7, 4}, {8, 4}});
        require(c.u8(match + 0x76) == 7 && c.u8(match + 0x77) == 1 && c.u8(match + 0x78) == 4,
                "combined competition fixture times");
        c.put8(match + 0x43, 4);
        c.put8(match + 0x44, 2);
        c.put32(match + 0x1c, 1);
        invoke(base_entry, {{4, 1}, {5, match}});
        require(c.r(3) == 3, "combined competition base result");

        invoke(record_entry, {{4, 1}, {5, 1}, {6, 0}});
        require(c.u8(standings + 0x44 + 0x40) == 5 && c.u32(history_manager + 4) == 1,
                "combined competition standings and history");
        invoke(announce_entry, {{4, 1}, {5, 1}, {6, 0}});
        require(c.u32(news_item) == source + 0x26 && c.u32(news_item + 8) == home_definition + 0x4d,
                "combined competition result announcement");

        invoke(next_season_entry, {{4, 0}});
        require(c.u16(division + 0x40) == 2002 && c.u32(division + 0x30) == UINT32_MAX,
                "combined competition season transition");
        invoke(destroy_entry, {{4, 0}});
        require(!c.u32(division + 0xbc) && !c.u32(division + 0xc) && c.u8(success),
                "combined competition teardown");
        s.memory.date_observer = nullptr;
    };
    run(*original, true, original_trace, original_effects);
    run(*modern, false, modern_trace, modern_effects);
    require(original_trace == modern_trace && original_effects.bytes == modern_effects.bytes,
            "combined competition ordered effects and dependency arguments");
    Image(*original).compare(*modern, "combined competition final state");
    std::cout << "BRA_NAT_SECOND combined competition: all 13 owned entries, final state and ordered effects match\n";
}

void check_single_round_rules() {
    std::mt19937 random(0x42524142);
    auto state = result_seed(construct_entry, random, lifecycle_success, 0, 0, 0);
    std::vector<Call> trace;
    Services services(trace, false);
    state->allocator_original = true;
    Cpu c(*state);
    const auto invoke = [&](std::uint32_t entry, std::initializer_list<std::pair<unsigned, std::uint32_t>> args) {
        trace.clear();
        c.set_lr(done);
        c.set(3, division);
        c.set(args);
        state->pc = entry;
        state->budget = state->executed + 50000;
        try {
            run_until(*state, done);
        } catch (const std::exception& error) {
            throw std::runtime_error(hex_address(entry) + " at " + hex_address(state->pc) +
                                     " (LR " + hex_address(c.lr()) + "): " + error.what());
        }
        require(c.r(2) == toc && c.sp() == stack, "single-round call boundary");
    };
    const auto allocations_of = [&](unsigned size) {
        return std::ranges::count_if(trace, [=](const Call& call) { return call[0] == allocate && call[4] == size; });
    };
    const auto check_format = [&] {
        require(c.u16(division + 0x3e) == 28 && c.u16(division + 0xa6) == 27 &&
                    c.u16(division + 0x3c) == 1 && c.u8(division + 0xc0) == 4 &&
                    c.u8(division + 0xc1) == 0 && c.u16(division + 0xdc) == 3 &&
                    c.u32(division + 0x2c) == 0 &&
                    c.u32(division + 0x30) == UINT32_MAX && c.u32(division + 0xc) == 0,
                "single round, four automatic promotions, and no knockout groups");
        require(allocations_of(4) == 0 && allocations_of(0xf0) == 0 &&
                    std::ranges::count(trace, regular_date, [](const Call& call) { return call[0]; }) == 27,
                "regular calendar only");
    };

    invoke(construct_entry, {{4, 2001}, {5, home_definition}});
    check_format();
    constexpr auto count_out = fixture + 0x9700, start_out = count_out + 2, regular_flag = count_out + 4;
    c.put16(count_out, 0x1234); c.put16(start_out, 0x5678); c.put8(regular_flag, 0x9a);
    invoke(calendar_entry, {{4, 0}, {5, count_out}, {6, start_out}, {7, regular_flag}});
    require(!c.r(3) && trace.empty() && c.u16(count_out) == 0x1234 &&
                c.u16(start_out) == 0x5678 && c.u8(regular_flag) == 0x9a,
            "single-round knockout calendar request declines without side effects");
    invoke(times_entry, {{4, 0}, {5, UINT32_MAX}, {6, 1}, {7, 2}, {8, 3}});
    require(c.u8(match + 0x76) == 1 && c.u8(match + 0x77) == 2 && c.u8(match + 0x78) == 3,
            "table standings are not remapped to playoff seeds");

    // Run the real base finalizer and history writer, using a standings
    // order that is deliberately different from the club database order.
    constexpr auto owners = fixture + 0x9720, table_slot = fixture + 0x9730;
    constexpr auto history_count = fixture + 0x9740, history_rows = fixture + 0x9748;
    constexpr auto year_slot = fixture + 0x9750, winners = fixture + 0x9780;
    constexpr auto qualifier_descriptor = descriptor + 48;
    const auto winner = clubs + 2 * clubs_stride, runner_up = clubs, third = clubs + clubs_stride;
    c.put32(standings, winner);
    c.put32(standings + 0x44, runner_up);
    c.put32(standings + 0x88, third);
    c.put32(winner + 0x30, second_division);
    c.put32(home_definition, 0);
    c.put32(toc - 0x7b5c, table_slot);
    c.put32(table_slot, owners);
    c.put32(owners, division);
    c.put32(vtable + 0x90, qualifier_descriptor);
    c.put32(qualifier_descriptor, base_qualification_query);
    c.put32(qualifier_descriptor + 4, toc);
    c.put32(toc - 0x7a38, history_count);
    c.put32(toc - 0x7a34, history_rows);
    c.put32(toc - 0x7b80, year_slot);
    c.put32(history_count, 2);
    c.put32(history_rows, winners);
    c.put16(year_slot + 2, 2001);
    c.put32(winners + 4, home_definition);
    invoke(group_entry, {});
    const auto recorded = winners + 0x1c;
    require(c.u32(recorded + 4) == home_definition && c.u32(recorded + 8) == winner &&
                c.u32(recorded + 0xc) == runner_up && c.u32(recorded + 0x10) == third &&
                c.u16(recorded + 0x18) == 2001,
            "real history writer records the table champion and vice-champion");
    require(std::ranges::none_of(trace, [](const Call& call) { return call[0] == callback; }),
            "single-round finalization never visits a playoff group");
    invoke(record_entry, {{4, 2}, {5, 5}, {6, UINT32_MAX}});
    require(c.u32(history_manager + 4) == 2, "table champion uses the league history event");

    // Loading an existing format preserves the saved stage. The following
    // season owns the new format and releases the old group's storage.
    c.put8(lifecycle_mode, load_existing);
    c.put16(division + 0xdc, 2);
    c.put32(division + 0x2c, 1);
    c.put32(division + 0x30, 0);
    c.put32(division + 0xc, groups_storage);
    c.put32(groups_storage, group);
    c.put32(group, vtable);
    invoke(construct_entry, {{4, 2001}, {5, home_definition}});
    require(c.u32(division + 0x2c) == 1 && c.u32(division + 0xc) == groups_storage &&
                c.u32(division + 0x30) == 0 && c.u16(division + 0xdc) == 2,
            "legacy save keeps its playoff format");
    invoke(group_entry, {{4, 2}, {5, 0}});
    require(c.u32(group + 4) == 2, "legacy season still finalizes its playoff group");
    c.put8(lifecycle_mode, lifecycle_success);
    c.put32(division + 8, fixtures);
    invoke(next_season_entry, {{4, 0}});
    check_format();
    const auto child = std::ranges::find(trace, child_destroy, [](const Call& call) { return call[0]; });
    const auto array_release = std::ranges::find_if(trace, [](const Call& call) {
        return call[0] == release && call[4] == groups_storage;
    });
    require(child != trace.end() && array_release != trace.end() && child < array_release &&
                c.u16(division + 0x40) == 2002, "rollover frees legacy stage before switching format");
    // Use real shared classification and exchange with a seeded final table.
    // Standings order differs from the underlying club order.
    constexpr auto exchange_data = fixture + 0x10000, first_league = exchange_data + 0x8000;
    constexpr auto first_definition = first_league + 0x200, first_table = first_league + 0x400;
    constexpr auto movement_slot = first_league + 0x700, nation_slot = first_league + 0x704;
    constexpr auto copy_descriptor = descriptor + 56, news_descriptor = descriptor + 64;
    constexpr auto times_descriptor = descriptor + 72;
    state->memory.map(exchange_data, 0x9000);
    c.put32(toc - 0x7be8, fixture + 0x6d04);
    c.put32(fixture + 0x6d04, exchange_data);
    c.put32(toc - 0x7b7c, movement_slot);
    c.put32(toc - 0x79f0, nation_slot);
    c.put32(toc - 0x7a98, nation_slot);
    c.put32(toc - 0x7e64, copy_descriptor);
    c.put32(copy_descriptor, block_move);
    c.put32(copy_descriptor + 4, toc);
    c.put32(vtable + 0x48, news_descriptor);
    c.put32(news_descriptor, classification_news);
    c.put32(news_descriptor + 4, toc);
    c.put32(vtable + 0x70, times_descriptor);
    c.put32(times_descriptor, times_entry);
    c.put32(times_descriptor + 4, toc);
    c.put32(owners + 4, first_league);
    c.put32(first_league + 4, first_definition);
    c.put32(first_league + 0xb0, first_table);
    c.put16(first_league + 0x3e, 8);
    c.put32(first_definition, 1);
    c.put32(division + 0x1c, 1);
    c.put8(division + 0x44, 0xff);
    c.put8(division + 0x45, 1);
    c.put16(division + 0xa4, 27);
    for (unsigned i = 0; i < 36; ++i) {
        const bool second_tier = i < 28;
        const auto club = exchange_data + i * clubs_stride;
        const auto row = second_tier ? standings + ((i + 3) % 28) * 0x44 : first_table + (i - 28) * 0x44;
        c.put32(club, i);
        c.put32(club + 0x30, second_tier ? home_definition : first_definition);
        c.put32(row, club);
        c.put8(row + 0x40, i >= 32 ? 3 : 0xff);
        if (second_tier) {
            const auto position = (i + 3) % 28 + 1;
            c.put8(row + 0x42, position);
            c.put8(row + 0x43, position);
        }
    }
    invoke(classify_table, {});
    for (unsigned i = 0; i < 28; ++i) {
        const auto expected = i == 0 ? 5 : i < 4 ? 0 : i >= 24 ? 3 : 255;
        require(c.u8(standings + i * 0x44 + 0x40) == expected,
                "real table classification marks the champion and promotion boundary");
    }
    // Use the flags from the Brazilian first-tier season exchange.
    invoke(exchange_clubs, {{3, first_league}, {4, first_league}, {5, division},
                            {6, 1}, {7, 1}, {8, UINT32_MAX}, {9, UINT32_MAX}});
    for (unsigned i = 0; i < 36; ++i) {
        const bool in_first_tier = i < 28 ? (i + 3) % 28 < 4 : i < 32;
        require(c.u32(exchange_data + i * clubs_stride + 0x30) ==
                    (in_first_tier ? first_definition : home_definition),
                "real division exchange promotes exactly the first four standings rows");
    }
    std::cout << "BRA_NAT_SECOND single round: 27 rounds, real champion and runner-up history, "
                 "four-club promotion, legacy rollover\n";
}

void reject(const State& initial, Corrupt corruption, std::string_view evidence) {
    try {
        compare_case(initial, corruption);
    } catch (const std::runtime_error& error) {
        require(std::string_view(error.what()).find(evidence) != std::string_view::npos,
                std::string("unexpected corruption result: ") + error.what());
        return;
    }
    throw std::runtime_error("Brazilian Second corruption control passed");
}
}

int main(int argc, char** argv) try {
    if (argc == 2 && std::string_view(argv[1]) == "--single-round") {
        check_single_round_rules();
        return 0;
    }
    require(argc == 1, "unknown Brazilian Second checker option");
#ifdef CM_BRA_NAT_SECOND_PROFILE_DIR
    routine_child_exit = [] {
        const auto name = std::string(CM_BRA_NAT_SECOND_PROFILE_DIR) + "/check-" + std::to_string(getpid()) + ".profraw";
        __llvm_profile_set_filename(name.c_str());
        if (__llvm_profile_write_file()) throw std::runtime_error("Cannot flush Brazilian Second Division coverage");
    };
#endif
    State unknown;
    for (const auto entry : {0x10519014u, 0x105195f0u}) {
        bool rejected = false;
        try { bra_nat_second_run_rewrite(unknown, entry); }
        catch (const std::logic_error& error) {
            rejected = std::string_view(error.what()).starts_with("Unknown BRA_NAT_SECOND.CPP entry ");
        }
        require(rejected, "nonentry dispatch must reject");
    }
    std::mt19937 random(0x105177b0);
    unsigned returned = 0, stops = 0;
    constexpr auto maximum = 1000u;
    for (const auto entry : {group_entry, times_entry, base_entry}) {
        for (const auto stage : {0x1ffu, 0x17fu, 0u})
            for (const auto first : {0u, 4u, 5u, 0x104u, 0xffu})
                for (const auto second : {0u, 4u, 5u, 0x104u, 0xffu}) {
                    auto s = seed(entry, stage, first, second, random);
                    s->budget = maximum;
                    require(compare_case(*s) == Stop::returned, "expected complete Brazilian Second comparison");
                    ++returned;
                }
        auto s = seed(entry, 0x1ff, 4, 5, random);
        for (unsigned budget = 0; budget < 40; ++budget) {
            s->budget = budget;
            const auto stop = compare_case(*s);
            require(stop != Stop::unmapped, "unexpected fault during budget comparison");
            if (stop == Stop::returned) ++returned;
            else ++stops;
        }
        s->budget = maximum;
        reject(*s, Corrupt::result, "register byte");
        if (entry != base_entry) reject(*s, Corrupt::argument, "call trace");
        check_entry_hook(*s, entry);
    }
    auto invalid = seed(group_entry, 0, 0, 0, random);
    invalid->budget = maximum;
    Cpu(*invalid).put32(division + 0xc, 0);
    require(compare_case(*invalid) == Stop::unmapped, "null group array preserves partial fault");
    Cpu(*invalid).put32(division + 0xc, groups);
    Cpu(*invalid).put32(vtable + 0x38, 0);
    require(compare_case(*invalid) == Stop::unmapped, "null callback descriptor preserves partial fault");
    unsigned scheduling_returns = 0, scheduling_stops = 0;
    for (const auto entry : {calendar_entry, collect_entry, configure_entry}) {
        for (const auto phase : {0x1ffu, 0x100u, 0x180u})
            for (const auto clubs : {0u, 7u, 32u})
                for (const bool failure : {false, true}) {
                    auto s = calendar_seed(entry, random, phase, clubs, failure, clubs != 7, clubs != 32);
                    s->budget = 2000;
                    require(compare_case(*s) == Stop::returned, "expected complete scheduling comparison");
                    ++scheduling_returns;
                }
        auto s = calendar_seed(entry, random, 0x1ff, 32, false, true, false);
        for (unsigned budget = 0; budget <= (entry == collect_entry ? 480u : 1020u); budget += 7) {
            s->budget = budget;
            const auto stop = compare_case(*s);
            require(stop != Stop::unmapped, "unexpected scheduling fault");
            if (stop == Stop::returned) ++scheduling_returns;
            else ++scheduling_stops;
        }
        s->budget = 2000;
        reject(*s, Corrupt::result, "register byte");
        reject(*s, Corrupt::argument, "call trace");
        check_entry_hook(*s, entry);
    }
    check_calendar_boundaries(random);
    unsigned lifetime_returns = 0, lifetime_stops = 0;
    for (const auto mode : {lifecycle_success, load_existing, fail_group_array, fail_calendar, fail_configure,
                            fail_standings, fail_table, fail_register, fail_fixtures}) {
        auto s = lifetime_seed(construct_entry, random, mode, 0, 0);
        require(compare_case(*s) == Stop::returned, "expected complete constructor");
        ++lifetime_returns;
    }
    for (const auto mode : {lifecycle_success, shorten_group_scan})
        for (const auto deletion : {0u, 0x10001u, 0x18000u})
            for (unsigned populated = 0; populated <= 2; ++populated) {
                auto s = lifetime_seed(destroy_entry, random, mode, deletion, populated);
                require(compare_case(*s) == Stop::returned, "expected complete destructor");
                ++lifetime_returns;
            }
    for (const auto mode : {lifecycle_success, fail_calendar, fail_configure, fail_standings,
                            fail_table, fail_register, shorten_group_scan})
        for (const auto year : {0u, 2002u, 0x7fffu, 0xffffu}) {
            auto s = lifetime_seed(next_season_entry, random, mode, year, year == 0 ? 0 : 1);
            require(compare_case(*s) == Stop::returned, "expected complete season rollover");
            ++lifetime_returns;
        }
    for (const auto entry : {construct_entry, destroy_entry, next_season_entry}) {
        auto s = lifetime_seed(entry, random, lifecycle_success, 1, 1);
        for (unsigned budget = 0; budget <= 1250; budget += entry == destroy_entry ? 3 : 13) {
            s->budget = budget;
            const auto stop = compare_case(*s);
            require(stop != Stop::unmapped, "unexpected lifetime fault");
            if (stop == Stop::returned) ++lifetime_returns;
            else ++lifetime_stops;
            if (stop == Stop::returned) break;
        }
        s->budget = 3000;
        reject(*s, Corrupt::result, "register byte");
        reject(*s, Corrupt::argument, "call trace");
        check_entry_hook(*s, entry);
    }
    check_lifetime_sequence(random);
    check_complete_competition_sequence(random);
    check_default_group_boundary(random);
    auto direct_group = result_seed(advance_groups_entry, random, lifecycle_success, 0, 1, 0);
    require(compare_case(*direct_group, Corrupt::no, create_group_entry) == Stop::returned,
            "default direct group route must complete");
    auto direct_calendar = calendar_seed(configure_entry, random, 0, 32, false, true, false);
    direct_calendar->budget = 2000;
    require(compare_case(*direct_calendar, Corrupt::no, calendar_entry) == Stop::returned,
            "default descriptor calendar route must complete");
    std::cout << "BRA_NAT_SECOND direct owned group and calendar: completed with matching state and effects\n";
    unsigned progression_returns = 0, progression_stops = 0;
    for (const auto entry : {advance_groups_entry, create_group_entry}) {
        for (const auto mode : {lifecycle_success, fail_seeds, fail_group, fail_calendar}) {
            auto s = result_seed(entry, random, mode, 0, 1, 0);
            require(compare_case(*s) == Stop::returned, "expected complete playoff setup");
            ++progression_returns;
        }
        auto s = result_seed(entry, random, lifecycle_success, 0, 1, 0);
        for (unsigned budget = 0; budget < 200; budget += 3) {
            s->budget = budget;
            const auto stop = compare_case(*s);
            require(stop != Stop::unmapped, "unexpected progression fault");
            if (stop == Stop::returned) { ++progression_returns; break; }
            ++progression_stops;
        }
        s->budget = 2000;
        reject(*s, Corrupt::result, "register byte");
        reject(*s, Corrupt::argument, "call trace");
        check_entry_hook(*s, entry);
    }
    auto advance = result_seed(advance_groups_entry, random, lifecycle_success, 0, 1, 0);
    for (const auto previous : {0u, 2u, UINT32_MAX - 1}) {
        Cpu c(*advance);
        c.put32(division + 0x2c, 2);
        c.put32(division + 0x30, previous);
        require(compare_case(*advance) == Stop::returned, "group count bound");
        ++progression_returns;
    }
    for (const auto entry : {record_entry, announce_entry}) {
        for (const auto phase : {0x1ffu, 0x100u, 0x180u})
            for (const auto outcome : {0u, 1u, 2u, 3u, 0xffu})
                for (const auto personal : {0u, 0x80u}) {
                    auto s = result_seed(entry, random, lifecycle_success, phase, outcome, personal);
                    require(compare_case(*s) == Stop::returned, "expected complete result callback");
                    ++progression_returns;
                }
        auto s = result_seed(entry, random, lifecycle_success, 0, 1, 0);
        reject(*s, Corrupt::result, "register byte");
        reject(*s, Corrupt::argument, "call trace");
        check_entry_hook(*s, entry);
    }
    std::cout << "BRA_NAT_SECOND progression and reporting: " << progression_returns << " complete, "
              << progression_stops << " budget stops, 4 strict entry hooks, 4 result and 4 trace corruption controls\n";
    std::cout << "BRA_NAT_SECOND lifetime: " << lifetime_returns << " complete, " << lifetime_stops
              << " budget stops, 3 strict entry hooks, 3 result and 3 trace corruption controls\n";
    std::cout << "BRA_NAT_SECOND scheduling: " << scheduling_returns << " complete, " << scheduling_stops
              << " budget stops, 3 strict entry hooks, a default descriptor boundary, 3 result and 3 trace corruption controls\n";
    std::cout << "BRA_NAT_SECOND pilot: " << returned << " complete, " << stops
              << " budget stops, 2 matched partial faults, 3 strict entry hooks, 3 result and 2 trace corruption controls\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}

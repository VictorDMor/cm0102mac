#include "guest_abi.hpp"
#include "localization_format_adapter.hpp"
#include "utility_object_adapter.hpp"
#include "national_teams_adapter.hpp"
#include "news_adapter.hpp"
#include "routine_check.hpp"
#include "rng_core.hpp"
#include "semantic_io_checks.hpp"
#include "tactics.hpp"

#include <optional>
#include <random>

namespace {
using namespace cm::semantic_io_checks;
using guest::Cpu;
constexpr auto context = 0x102ae9c0u, read_flag = 0x102aea20u, clear_flag = 0x102aea50u;
constexpr auto read_value = 0x102af620u, get_slot = 0x102af650u, get_staff = 0x102b7450u;
constexpr auto tier = 0x102aea80u, prefix = 0x102aeb40u, international_event = 0x102aed80u;
constexpr auto done = fixture + 0x20, manager = fixture + 0x100, nation = fixture + 0x200;
constexpr auto records = fixture + 0x1800, staff_table = fixture + 0x3000, staff_slot = fixture + 0x4100;
constexpr auto club_count = fixture + 0x4200, nation_count = fixture + 0x4204;
constexpr auto club = fixture + 0x4300, unmapped = fixture + 0xa000;
constexpr auto remove_player = 0x102b8180u, selected_player = 0x102aefb0u;
constexpr auto dedup_rosters = 0x102ace30u, drop_roster = 0x102b8d60u;
constexpr auto club_notice = 0x102b1130u, nation_notice = 0x102b1be0u;
constexpr auto injured_selection = 0x102b8460u;
constexpr auto automatic_replacement = 0x102b8850u, replacement_candidate = fixture + 0x6e00;
constexpr auto replacement_notice = 0x102b16d0u, add_replacement = 0x102b8c50u, replacement_objection = 0x102b8e60u;
constexpr auto international = 0x102b7c00u, duty = 0x102b7c70u;
constexpr auto player = fixture + 0x5100, date = fixture + 0x5200, national_team = fixture + 0x5300;
constexpr auto regular_club = fixture + 0x5400, team_nation = fixture + 0x5500;
constexpr auto first_star = fixture + 0x5510, second_star = fixture + 0x5514, clubs_slot = fixture + 0x5518;
constexpr auto fixture_slot = fixture + 0x551c, competition = fixture + 0x5520;
constexpr auto fixtures_head = fixture + 0x5600, roster_list = fixture + 0x5700;
constexpr auto roster_nodes = fixture + 0x5800, roster_rows = fixture + 0x5900;
constexpr auto fixture_nodes = fixture + 0x5a00, fixture_rows = fixture + 0x5b00;
constexpr auto label_out = fixture + 0x5c00, label_shared = fixture + 0x5c40, source = fixture + 0x100000;
constexpr auto calendar_slot = fixture + 0x5c80, calendar_index = fixture + 0x5c84, calendar_array = fixture + 0x5c88;
constexpr auto calendar = fixture + 0x6000, calendar_date = fixture + 0x6200, allowed_nation = fixture + 0x6220;
constexpr auto clubs = fixture + 0x7000, first_club = clubs + 0x248, second_club = first_club + 0x248;
constexpr auto first_bound = fixture + 0x6300, second_bound = fixture + 0x6308, cutoff_date = fixture + 0x6310;
constexpr auto disposed = fixture + 0x6318, fixture_count = fixture + 0x631c;
constexpr auto news_slot = fixture + 0x6500, injury_slot = news_slot + 4, contracts_slot = news_slot + 8;
constexpr auto contract_scratch = news_slot + 12, notice_policy = news_slot + 16, notice_events = news_slot + 24;
constexpr auto news_system = fixture + 0x6700, injury_system = fixture + 0x6740, contract_system = fixture + 0x6780;
constexpr auto notice_contract = fixture + 0x6800, notice_injury = fixture + 0x6840, injury_types = fixture + 0x6880;
constexpr auto contract_answers = fixture + 0x6900, notice_language = fixture + 0x6940;
constexpr auto injury_policy = notice_policy + 1, injury_active = notice_policy + 2, search_policy = notice_policy + 3;
constexpr auto injury_assert_buffer = fixture + 0x6b00, injury_success = fixture + 0x6c00;
constexpr auto recipient_club = clubs + 5 * 0x248;
constexpr auto news_create = 0x102bdf90u, news_field = 0x102cb4b0u, news_publish = 0x102cc140u;
constexpr auto calendar_availability = 0x102b9e50u, calendar_save = 0x102b96e0u;
constexpr auto calendar_load = 0x102b8ec0u;
constexpr auto create_schedule = 0x102ba1c0u, register_fixture = 0x102ba290u, expire_schedule = 0x102ba610u;
constexpr auto mark_fixtures = 0x102ba7b0u, apply_marks = 0x102baa40u;
constexpr auto mark_groups = fixture + 0xc000, mark_lists = fixture + 0xcc00;
constexpr auto mark_array = fixture + 0xbe20, mark_matches = fixture + 0xbe40;
constexpr auto queue_calendar_notice = 0x102bacc0u, publish_calendar_notices = 0x102bad10u;
constexpr auto calendar_notice_trace = fixture + 0x8400, calendar_notice_pairs = fixture + 0x8600;
constexpr auto calendar_notice_globals = fixture + 0x8700, calendar_variant_words = fixture + 0x8760;
constexpr auto calendar_rng_seed = toc - 0x34c4;
constexpr auto render_calendar_notice = 0x102bb050u;
constexpr auto notice_render_item = fixture + 0xd000, notice_render_output = fixture + 0xd100;
constexpr auto notice_render_trace = fixture + 0xd200, notice_render_cups = fixture + 0xd400;
constexpr auto notice_render_people = fixture + 0xd700, notice_render_cup_slot = fixture + 0xd900;
constexpr auto append_appointment = 0x102b7000u, expire_nation_appointments = 0x102b70a0u;
constexpr auto expire_pair_appointments = 0x102b7140u, replace_assignment = 0x102b71e0u, reconcile_assignments = 0x102b7280u;
constexpr auto direct_assignment = 0x102b7490u, release_assignments = 0x102b7750u;
constexpr auto appointment_trace = fixture + 0x8800, appointment_rows = fixture + 0x8900;
constexpr auto assignment_nations = fixture + 0xc000, assignment_managers = fixture + 0xc500;
constexpr auto assignment_slots = fixture + 0xc800;
constexpr auto assignment_manager = assignment_slots + 0x10, assignment_club_list = assignment_slots + 0x30;
constexpr auto assignment_descriptor = assignment_slots + 0x70, assignment_virtual_table = assignment_slots + 0x80;
constexpr auto assignment_enumerator = assignment_slots + 0x100, assignment_callback_toc = toc + 0x1000;
constexpr auto assignment_clubs = assignment_nations + 0x400;
constexpr auto date_order = 0x102bc180u, score_order = 0x102bc250u, club_order = 0x102bc270u;
constexpr auto release_team = fixture + 0xb000, release_players = fixture + 0xb200;
constexpr auto release_clubs = fixture + 0xb800, release_nation = fixture + 0xba00;
constexpr auto release_limit = fixture + 0xbb40, release_excluded_nation = fixture + 0xbb44;
constexpr auto release_headers = fixture + 0xbb50, release_rows = fixture + 0xbc00, grown_rows = fixture + 0xbd00;
constexpr auto roster_comparator = fixture + 0xbe00, date_comparator = fixture + 0xbe08;
constexpr auto archive_path = fixture + 0x6c80, archive_companion = fixture + 0x6ca0;
constexpr auto archive_primary = fixture + 0x6d00, archive_secondary = fixture + 0x6d20;
constexpr auto archive_years = fixture + 0x6d40, archive_flags = fixture + 0x6d50;
constexpr auto archive_trace = fixture + 0x8000, archive_bytes = fixture + 0x8200;
constexpr auto archive_arena = fixture + 0xb000, archive_nodes = archive_arena + 0x3000;
constexpr auto release_policy = archive_trace + 0x40;
constexpr auto archive_stream = 0x51515151u;
constexpr std::array archive_error_lines{0x2039u, 0x2041u, 0x2048u, 0x2061u, 0x2079u, 0x2092u,
                                         0x20b2u, 0x20c0u, 0x20c0u, 0x20c9u, 0x20d6u, 0x20dfu, 0x20e6u};
constexpr auto calendar_match = fixture + 0x6d00, calendar_headers = fixture + 0x6e00;
constexpr auto home_events = fixture + 0x7200, away_events = fixture + 0x7800;
constexpr auto competition_manager = fixture + 0x6f00, competition_slot = fixture + 0x6f10;
constexpr auto competition_count = fixture + 0x6f14;
constexpr auto localize = 0x10727890u, dispose_list = 0x10738c20u;
constexpr unsigned record_size = 0x74, group_size = 0x14, staff_size = 0x6c;

std::unique_ptr<State> seed(std::mt19937& random) {
    auto s = std::make_unique<State>();
    s->memory.map(fixture, 0x5000);
    s->memory.map(stack - 0x400, 0x800);
    s->memory.map(toc - 0x8000, 0x8000);
    Cpu c(*s);
    for (unsigned i = 0; i < 32; ++i) c.set(i, random());
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, random() & 0xff);
    s->wr<1>(guest::reg::xer_so, random() & 1);
    s->wr<1>(guest::reg::xer_ca, random() & 1);
    c.set({{1, stack}, {2, toc}});
    c.set_lr(done);
    c.put32(manager + 4, records);
    c.put32(toc - 0x7bbc, staff_slot);
    c.put32(staff_slot, staff_table);
    c.put32(toc - 0x7b50, club_count);
    c.put32(toc - 0x7b04, nation_count);
    c.put32(club_count, 200);
    c.put32(nation_count, 20);
    s->budget = 10000;
    return s;
}

enum class Stop { returned, fault, budget };
enum class Corrupt { none, result, dependency };

using DependencyCall = std::array<std::uint32_t, 34>;
std::vector<DependencyCall>* dependencies = nullptr;

void fixture_date(Cpu& c, std::uint32_t to, int day, unsigned year) {
    c.put16(to, day);
    c.put16(to + 2, year);
    c.put8(to + 4, year % 4 == 0);
}

bool observe_dependency(State& s) {
    Cpu c(s);
    const auto entry = s.pc, resume = c.lr(), frame = c.sp();
    const bool calendar_read = resume >= 0x102b8f24 && resume <= 0x102b96c4;
    const bool calendar_archive = calendar_read || (resume >= 0x102b9740 && resume <= 0x102b9e34);
    const bool domestic_schedule = resume >= 0x102ba204 && resume <= 0x102ba5dc;
    const bool fixture_marking = resume >= 0x102ba80c && resume <= 0x102ba9fc;
    const bool calendar_news = resume >= 0x102bad6c && resume <= 0x102bb03c;
    const bool calendar_render = resume >= 0x102bb08c && resume <= 0x102bc164;
    const bool direct_assertion = resume == 0x102b74f8 || resume == 0x102b7518 || resume == 0x102b7528;
    switch (entry) {
    case assignment_enumerator:
        if (resume != 0x102b7794) return false;
        break;
    case 0x101b83f0:
        if (resume != 0x102b77f4) return false;
        break;
    case 0x100ce250: case 0x100c7d00: case 0x10738e00: case 0x10717b20: case 0x10717890:
    case 0x1048b180: case dispose_list: case localize: case 0x107156b0: break;
    case tier:
        if (resume != 0x102b81d8 && resume != 0x102b876c) return false;
        break;
    case 0x100c76c0: case 0x100c7860: break;
    case 0x107475b0: case 0x10747cc0: case 0x10747900: case 0x10487640:
        if (!calendar_archive) return false;
        break;
    case 0x10747a80: case 0x10775130:
        if (!calendar_read) return false;
        break;
    case 0x10773520:
        if (!calendar_read && resume != 0x102b7034) return false;
        break;
    case 0x10738d10:
        if (!calendar_read && resume != 0x102bacf8 && resume != 0x102b708c) return false;
        break;
    case 0x102cb570: case 0x10746a70: case 0x1074b830: case 0x1074bab0:
        if (!calendar_render) return false;
        break;
    case 0x10738ca0: case 0x1049d330: case 0x102cc730: case 0x102cc8e0: case 0x1077b770:
        if (!calendar_news) return false;
        break;
    case 0x10775200:
        if (!calendar_read && !domestic_schedule && !fixture_marking) return false;
        break;
    case 0x10775170:
        if (resume != 0x102ba774 && resume != 0x102b7814) return false;
        break;
    case 0x107751b0: case 0x10768300:
        if (!domestic_schedule) return false;
        break;
    case 0x101b5b20:
        if (resume != 0x102b7268 && resume != 0x102b7414 &&
            resume != 0x102b7580 && resume != 0x102b7704 && resume != 0x102b7734) return false;
        break;
    case 0x1048a020:
        if (resume != 0x102bac8c) return false;
        break;
    case 0x1048d420:
        if (resume != 0x102ba8b8) return false;
        break;
    case 0x10494d20:
        if (resume != 0x102b9f08 && resume != 0x102ba8e8 && resume != 0x102ba978) return false;
        break;
    case 0x1014c2e0:
        if (resume != 0x103b19a4 && resume != 0x102b1da8 && resume != 0x102b1de0 &&
            resume != 0x102b878c && resume != 0x102b87bc && resume != 0x102b8bb0) return false;
        break;
    case news_create: case news_field: case news_publish: case 0x102cdbf0: case 0x1018c340:
    case 0x10082f20: case 0x100c9070: case 0x101484b0: case 0x10187680: case 0x100e7920: case 0x102de3c0:
    case 0x10739890: case 0x1077b720: case 0x10769b60: break;
    default: return false;
    }
    DependencyCall call{entry, resume};
    for (unsigned i = 0; i < 32; ++i) call[i + 2] = c.r(i);
    dependencies->push_back(call);
    if (calendar_render) {
        switch (entry) {
        case 0x102cb570:
            require(c.r(3) == notice_render_item && c.r(4) < 50, "calendar formatter field lookup");
            c.set(3, c.u32(notice_render_item + 8 + c.r(4) * 4));
            break;
        case 0x10746a70:
            require(c.r(3) == c.u32(notice_render_item + 12) && c.r(4) == frame + 0x68 &&
                        c.r(5) == 0x78 && !c.r(6), "calendar formatter localized count");
            c.put32(c.r(4), c.r(3));
            c.set(3, 1);
            break;
        case localize: {
            const auto index = c.u8(notice_render_trace);
            require(index < 2 && (c.r(3) == label_shared || c.r(3) == frame + 0x270),
                    "calendar formatter destination and number of phrases");
            const auto record = notice_render_trace + 8 + index * 0x60;
            for (unsigned arg = 3; arg <= 12; ++arg) c.put32(record + (arg - 3) * 4, c.r(arg));
            for (unsigned arg = 0; arg < 11; ++arg) c.put32(record + 40 + arg * 4, c.u32(frame + 0x38 + arg * 4));
            c.put8(notice_render_trace, index + 1);
            c.put32(c.r(3), c.r(4) - source);
            if (c.u8(notice_render_trace + 1)) {
                require(c.r(3) == frame + 0x270, "calendar prefix fixture stops at the club list");
                c.u8(unmapped);
            }
            break;
        }
        case 0x1074b830:
            require(c.r(3) == notice_render_output && c.r(4) == label_shared, "calendar formatter text assignment");
            c.put32(c.r(3), c.u32(c.r(4)));
            break;
        case 0x100c76c0:
            require(c.r(3) == notice_render_cups || c.r(3) == notice_render_cups + 0x128,
                    "calendar formatter competition link object");
            c.set(3, c.r(3) + 0x10);
            break;
        case 0x1074bab0:
            require(c.r(3) == notice_render_output, "calendar formatter link output");
            break;
        default:
            throw std::logic_error("Unexpected calendar formatter dependency");
        }
        c.finish();
        return true;
    }
    switch (entry) {
    case assignment_enumerator:
        require(c.r(2) == assignment_callback_toc && c.u32(frame + 0x14) == toc &&
                    c.r(3) == assignment_nations && c.r(4) == frame + 0x38 && c.r(5) == 0,
                "queued release virtual call ABI and TOC switch");
        c.put32(c.r(4), c.u32(assignment_club_list + 0x14));
        c.set(3, c.u32(assignment_club_list + 0x10));
        break;
    case 0x100c7d00:
        if (resume != 0x102b77bc) return false;
        require(c.r(2) == toc && c.r(3) >= assignment_clubs && c.r(3) < assignment_clubs + 3 * 0x40,
                "queued release club classification and TOC restore");
        c.set(3, c.u32(c.r(3) + 4));
        break;
    case 0x101b83f0: {
        const auto index = c.u8(appointment_trace + 4);
        require(index < 3 && c.r(3) == news_system && c.r(4) == assignment_manager &&
                    c.r(5) >= assignment_clubs && c.r(5) < assignment_clubs + 3 * 0x40,
                "queued release notification ABI");
        const auto nation = c.u32(c.r(5) + 0x2c);
        require(c.u32(records + c.u32(nation + 0x68) * record_size + 4) == UINT32_MAX,
                "queued release clears the assignment before notification");
        c.put32(appointment_trace + 8 + index * 4, c.r(5));
        c.put8(appointment_trace + 4, index + 1);
        break;
    }
    case 0x100c76c0:
        if (resume != 0x102b7258 && resume != 0x102b7404 &&
            resume != 0x102b7570 && resume != 0x102b76f4 && resume != 0x102b7724) return false;
        require(c.r(3) >= assignment_nations && c.r(3) < assignment_nations + 3 * 0x128,
                "staff assignment identity argument");
        c.set(3, 0x8000 + (c.r(3) - assignment_nations) / 0x128);
        break;
    case 0x101b5b20: {
        const auto index = c.u8(appointment_trace + 4);
        require(index < 3 && c.r(3) == news_system && c.r(5) == 1, "staff assignment callback ABI");
        c.put32(appointment_trace + 8 + index * 4, c.r(4));
        c.put8(appointment_trace + 4, index + 1);
        break;
    }
    case 0x1077b770:
        return false; // Run the real RNG core and compare its ordered writes.
    case 0x10738ca0:
        require(resume == 0x102bb03c && c.r(3) == calendar + 0x28, "calendar notice clear ABI");
        c.put64(c.r(3), 0);
        c.put64(c.r(3) + 8, 0);
        c.put8(calendar_notice_trace + 2, 1);
        break;
    case 0x1049d330:
        require(resume == 0x102baedc && c.r(3) == competition_manager &&
                    c.r(5) == frame + 0x3c && c.r(6) == frame + 0x38, "calendar notice metadata ABI");
        c.put32(c.r(5), 44 + c.r(4));
        c.put32(c.r(6), 81 + c.r(4));
        break;
    case 0x102cc730: case 0x102cc8e0: {
        const auto group = c.u8(calendar_notice_trace + 1);
        require(group < 2 && c.r(3) == news_system && c.r(4) == frame + 0x40,
                "calendar notice delivery ABI");
        if (entry == 0x102cc730)
            require(c.r(5) == calendar_notice_globals + 0xc8, "calendar notice local delivery recipient");
        c.put8(calendar_notice_trace + 4 + group, entry == 0x102cc730 ? 1 : 2);
        c.put8(calendar_notice_trace + 1, group + 1);
        break;
    }
    case 0x100ce250:
        if (resume != 0x102ba33c) return false;
        require(c.r(3) == release_team, "domestic fixture team classifier");
        c.set(3, c.u8(release_policy + 1));
        break;
    case 0x1048a020: {
        const auto index = c.u8(release_policy + 0x24);
        require(index < 3 && c.r(3) == competition_manager && c.r(5) == date,
                "fixture reschedule arguments");
        c.put32(release_policy + 0x28 + index * 4, c.r(4));
        c.put8(release_policy + 0x24, index + 1);
        break;
    }
    case 0x1048d420: {
        const bool nested = c.u8(release_policy + 0x20);
        require(c.r(3) == competition_manager && (c.r(4) == date || (nested && c.r(4) == stack - 0x38)) &&
                    c.r(5) <= 2 && (c.r(6) == 5 || c.r(6) == 7), "fixture marking lookup group, date and slot");
        const auto count = c.u8(release_policy + 6);
        if (nested && c.r(5) == 0) c.put32(release_policy + 0x34 + (count / 3) * 4, c.u32(c.r(4)));
        c.put8(release_policy + 6, count + 1);
        c.set(3, c.u32(mark_lists + (c.r(6) == 7 ? 12 : 0) + c.r(5) * 4));
        break;
    }
    case 0x10775170: {
        if (resume == 0x102b7814) {
            require(c.r(3) == assignment_club_list && c.u8(appointment_trace + 0x18) == 0,
                    "queued release frees the returned array exactly once");
            c.put8(appointment_trace + 0x18, 1);
            break;
        }
        const auto index = c.u32(release_policy + 8);
        require(index < 3 && (c.r(3) == release_rows || c.r(3) == away_events || c.r(3) == grown_rows),
                "domestic schedule release pointer");
        c.put32(release_policy + 12 + 4 * index, c.r(3));
        c.put32(release_policy + 8, index + 1);
        break;
    }
    case 0x107751b0: {
        const auto rows = c.r(3);
        require(resume == 0x102ba50c && c.r(4) == (rows ? 4 : 3) * 12u,
                "domestic schedule grows by three rows");
        require(!rows || rows == release_rows, "domestic schedule grows its current buffer");
        if (c.u8(release_policy)) c.set(3, 0);
        else {
            if (rows) s.memory.copy_in(grown_rows, s.memory.copy_out(rows, 12));
            c.set(3, grown_rows);
        }
        break;
    }
    case 0x10768300: {
        const bool real_sort = c.u8(release_policy + 4);
        if (resume == 0x102ba300) {
            require(c.r(3) == release_team + 0x98 && c.r(4) == 50 && c.r(5) == 4 &&
                        c.r(6) == (real_sort ? roster_comparator : 0x61515151u), "domestic roster sort");
            c.put8(release_policy + 2, 1);
        } else {
            require(resume == 0x102ba5dc && c.r(5) == 12 && c.r(6) == (real_sort ? date_comparator : 0x61616161u) &&
                        c.r(4) && c.r(4) <= 3, "domestic schedule row sort");
            c.put8(release_policy + 3, c.u8(release_policy + 3) + 1);
        }
        if (real_sort) return false;
        break;
    }
    case 0x107475b0:
        require(c.r(3) == archive_path && c.r(4) == source + 0x7cda && c.r(5) == unsigned(!calendar_read) &&
                    c.r(6) == 0x16 && c.r(7) == 4, "calendar archive open ABI");
        c.set(3, c.u8(archive_trace) == 0xff ? 0 : archive_stream);
        break;
    case 0x10747cc0: {
        require(c.r(6) == archive_stream && c.r(4) <= 0x74, "calendar archive write ABI");
        const auto call = c.u32(archive_trace + 4) + 1;
        const auto offset = c.u32(archive_trace + 8);
        const auto requested = c.r(5);
        const auto actual = requested > 64 ? 0u : c.u8(archive_trace) == call && requested ? requested - 1 : requested;
        const auto size = actual * c.r(4);
        require(offset + size < 0x1000 - 0x200, "calendar archive buffer capacity");
        const auto bytes = s.memory.copy_out(c.r(3), size);
        if (!bytes.empty()) s.memory.copy_in(archive_bytes + offset, bytes);
        c.put32(archive_trace + 4, call);
        c.put32(archive_trace + 8, offset + size);
        c.set(3, actual);
        break;
    }
    case 0x10747a80: {
        const auto width = c.r(4), requested = c.r(5);
        require(c.r(6) == archive_stream && width && width <= 0x74, "calendar archive read ABI");
        const auto call = c.u32(archive_trace + 4) + 1;
        const auto offset = c.u32(archive_trace + 8);
        const auto available = c.u32(archive_trace + 0x14);
        require(offset <= available, "calendar archive read cursor");
        auto actual = std::min(requested, (available - offset) / width);
        if (actual && c.u8(archive_trace) == call) --actual;
        const auto size = actual * width;
        if (size) s.memory.copy_in(c.r(3), s.memory.copy_out(archive_bytes + offset, size));
        c.put32(archive_trace + 4, call);
        c.put32(archive_trace + 8, offset + size);
        c.set(3, actual);
        break;
    }
    case 0x10775130: case 0x10775200: case 0x10773520: {
        if (resume == 0x102b7034) {
            require(entry == 0x10773520 && c.r(3) == 0x10, "appointment list allocation ABI");
            c.put8(appointment_trace + 1, 1);
            c.set(3, c.u8(appointment_trace) ? 0 : roster_list);
            break;
        }
        if (fixture_marking) {
            require(resume == 0x102ba80c && c.r(3) == 3 && c.r(4) == 1, "fixture mark array allocation");
            if (c.u8(release_policy)) c.set(3, 0);
            else {
                for (unsigned i = 0; i < 3; ++i) c.put8(mark_array + i, 0);
                c.set(3, mark_array);
            }
            break;
        }
        if (domestic_schedule) {
            require(resume == 0x102ba204 && c.r(3) == 3 && c.r(4) == 8, "domestic schedule header allocation");
            c.set(3, c.u8(release_policy) ? 0 : release_headers);
            break;
        }
        const auto call = c.u32(archive_trace + 0x1c) + 1;
        const auto size = entry == 0x10775200 ? c.r(3) * c.r(4) : c.r(3);
        require(call <= 7 && size <= 0x400, "calendar archive allocation capacity");
        c.put32(archive_trace + 0x1c, call);
        c.set(3, c.u8(archive_trace + 0x18) == call ? 0 : archive_arena + (call - 1) * 0x400);
        break;
    }
    case 0x10738d10: {
        const auto list = c.r(3);
        if (resume == 0x102b708c) {
            require((list == roster_list || list == 0) && c.r(4) == frame + 0x38 && c.r(5) == 0x14,
                    "appointment append ABI");
            s.memory.copy_in(appointment_rows, s.memory.copy_out(c.r(4), c.r(5)));
            c.put8(appointment_trace + 2, 1);
            break;
        }
        if (resume == 0x102bacf8) {
            require(list == calendar + 0x28 && c.r(4) == frame + 0x38 && c.r(5) == 8,
                    "calendar notice append ABI");
            c.put32(calendar_notice_pairs, c.u32(c.r(4)));
            c.put32(calendar_notice_pairs + 4, c.u32(c.r(4) + 4));
            c.put8(calendar_notice_trace, 1);
            break;
        }
        require(c.r(4) == frame + 0x108 && c.r(5) == 0x14, "calendar archive append ABI");
        const auto count = c.u16(list);
        require(count < 2, "calendar archive list capacity");
        const auto node = archive_nodes + count * 0x40, data = node + 0x10, tail = c.u32(list + 8);
        s.memory.copy_in(data, s.memory.copy_out(c.r(4), c.r(5)));
        c.put32(node, data);
        c.put32(node + 4, tail);
        c.put32(node + 8, 0);
        c.put32(tail ? tail + 8 : list + 4, node);
        c.put32(list + 8, node);
        c.put16(list, count + 1);
        break;
    }
    case 0x10747900:
        require(c.r(3) == frame + 0x3c && c.u32(c.r(3)) == archive_stream, "calendar archive close ABI");
        c.put32(c.r(3), 0);
        c.put8(archive_trace + 0xc, 1);
        c.set(3, 0);
        break;
    case 0x10487640:
        require(c.r(3) == archive_companion && c.r(4) == archive_path && c.u8(archive_trace + 0xc),
                "calendar companion save after stream close");
        c.put8(archive_trace + 0xd, 1);
        c.set(3, 0x456789ab);
        break;
    case news_create: case news_field:
        if (calendar_news) {
            require(c.r(3) == frame + 0x40, "calendar notice message ABI");
            if (entry == news_create) {
                require(c.r(4) == 0x2c && !c.r(5), "calendar notice message category");
                c.put8(calendar_notice_trace, 1);
            } else {
                const auto group = c.u8(calendar_notice_trace + 1);
                const auto field = c.r(4);
                require(group < 2 && field < 16, "calendar notice fields");
                c.put32(calendar_notice_trace + 0x20 + group * 0x40 + field * 4, c.r(5));
            }
            break;
        }
        require(c.r(3) == frame + 0x38, "roster notice message frame");
        if (entry == news_create) {
            require((c.r(4) >= 0x20 && c.r(4) <= 0x22) && !c.r(5), "roster notice category and priority");
            c.put32(notice_events + 0x10, c.r(4));
            c.put32(notice_events + 0x14, 0);
        } else {
            const auto field = c.u32(notice_events + 0x14);
            require(c.r(4) == field, "roster notice fields in order");
            c.put32(notice_events + 0x14, field + 1);
        }
        news_run_rewrite(s, entry);
        return true;
    case news_publish: {
        require(c.r(3) == news_system && c.r(4) == frame + 0x38, "roster notice publish arguments");
        const auto index = c.u32(notice_events);
        require(index < 2, "roster notice published at most twice");
        c.put32(notice_events + 4 + index * 4, c.r(5));
        c.put32(notice_events, index + 1);
        c.set(3, 0x12345678);
        break;
    }
    case 0x102cdbf0:
        require(c.r(3) == news_system, "roster notice subscription lookup");
        c.set(3, (c.u8(notice_policy) & (resume == 0x102b1174 || resume == 0x102b1710 ? 1u : 2u)) != 0);
        break;
    case 0x10082f20: {
        require(c.r(3) == contract_system && (c.r(4) == player || c.r(4) == replacement_candidate),
                "roster notice contract lookup");
        const auto index = c.u32(notice_events + 0x18);
        require(index < 4, "roster notice contract lookup count");
        c.put32(notice_events + 0x18, index + 1);
        c.set(3, c.u32(contract_answers + index * 4));
        break;
    }
    case 0x1018c340:
        require(c.r(3) == injury_system && c.r(4) == player, "roster notice injury lookup");
        c.set(3, (c.u8(notice_policy) & 4) ? notice_injury : 0);
        break;
    case 0x100c9070:
        require((c.r(4) == 4 || c.r(4) == 8) && !c.r(5), "roster notice squad count mask");
        c.set(3, c.r(4) == 4 ? 0x82 : 7);
        break;
    case 0x1014c2e0:
        if (resume == 0x103b19a4) {
            if (!c.u8(injury_active)) return false;
            c.set(3, 0);
            break;
        }
        require(c.r(4) == 0 && c.r(5) == 1 && !c.r(6) && !c.r(7), "roster notice managed-team query");
        if (resume == 0x102b8bb0) {
            c.set(3, (c.u8(search_policy) & 8) != 0);
        } else if (resume == 0x102b878c || resume == 0x102b87bc) {
            c.set(3, (c.u8(injury_policy) & (resume == 0x102b878c ? 1u : 2u)) != 0);
        } else {
            c.set(3, (c.u8(notice_policy) & (resume == 0x102b1da8 ? 1u : 2u)) != 0);
        }
        break;
    case 0x101484b0:
        require(resume == 0x102b87e8 && !c.r(5), "injury replacement manager gate");
        c.set(3, (c.u8(injury_policy) & 4) != 0);
        break;
    case 0x10187680:
        require(resume == 0x102b89f4 && c.r(3) == injury_system && c.r(4) == replacement_candidate,
                "automatic replacement candidate injury check");
        c.set(3, (c.u8(search_policy) & 1) != 0);
        break;
    case 0x102de3c0:
        require(resume == 0x102b8a10 && c.r(4) == replacement_candidate, "replacement availability check");
        c.set(3, (c.u8(search_policy) & 2) != 0);
        break;
    case 0x100e7920:
        require(resume == 0x102b8aa4 && c.r(4) == replacement_candidate && c.r(6) == 1 && !c.r(7),
                "automatic replacement candidate eligibility");
        c.set(3, (c.u8(search_policy) & 4) != 0);
        break;
    case 0x10739890:
        require(c.r(3) == source + 0x37, "national assertion source");
        if (direct_assertion) {
            require(resume == 0x102b74f8 && c.r(6) == frame + 0x40 && c.r(7) == frame + 0x38,
                    "direct staff assignment assertion frame");
        } else if (!calendar_archive && !domestic_schedule && !fixture_marking) {
            require(resume == 0x102b84b8 && c.r(6) == frame + 0x50 && c.r(7) == frame + 0x48,
                    "injury missing-team assertion frame");
        }
        c.put8(c.r(6), 7);
        c.put8(c.r(7), 9);
        break;
    case 0x1077b720:
        require(c.r(3) == injury_assert_buffer, "national assertion output");
        if (calendar_archive || domestic_schedule || fixture_marking || direct_assertion)
            c.put32(archive_trace + 0x10, c.r(8));
        else require(resume == 0x102b84d8 && c.r(8) == 0x1df8, "injury missing-team assertion line");
        c.put8(injury_assert_buffer, 13);
        break;
    case 0x10769b60:
        require((calendar_archive || domestic_schedule || fixture_marking || direct_assertion || resume == 0x102b84e8) &&
                    c.r(3) == source + 0x57 && c.r(4) == injury_assert_buffer, "national assertion dialog");
        break;
    case 0x10717b20: case 0x10717890: {
        const bool lower = entry == 0x10717b20;
        if (resume == 0x102b7100 || resume == 0x102b71a8) {
            require(!lower && c.r(3) == frame + 0x38 && c.r(4) == date && c.r(5) == 1,
                    "appointment expiry shifts today forward");
            return false;
        }
        if (resume == 0x102baa84 || resume == 0x102baaac || resume == 0x102baaec || resume == 0x102bab20)
            return false; // Use the real date services in fixture application.
        if (resume == 0x102ba35c) {
            require(lower && c.r(3) == frame + 0x40 && c.r(4) == frame + 0x58 && c.r(5) == 1,
                    "domestic fixture shifts the release date by one");
            fixture_date(c, c.r(3), c.i16(c.r(4)) - 1, c.u16(c.r(4) + 2));
            break;
        }
        const bool availability = resume == 0x102b9eb8 || resume == 0x102b9ee0;
        require(c.r(3) == frame + (lower ? 0x60 : 0x58) &&
                    c.r(4) == (availability ? frame + 0x80 : date) && c.r(5) == 1,
                "calendar window arguments");
        if (availability) {
            require(c.u16(frame + 0x80) == c.u16(calendar_match + 0x2a) &&
                        c.u16(frame + 0x82) == c.u16(calendar_match + 0x28),
                    "calendar converts and retains the match date");
        }
        const auto seeded = lower ? first_bound : second_bound;
        fixture_date(c, c.r(3), c.i16(seeded), c.u16(seeded + 2));
        break;
    }
    case 0x1048b180: {
        const bool reserve = (c.u8(player + 0x6a) & 4) != 0;
        require(c.r(3) == c.u32(fixture_slot) && c.r(4) == c.u16(first_bound) &&
                    c.r(5) == c.u16(first_bound + 2) && c.r(6) == c.u16(second_bound) &&
                    c.r(7) == c.u16(second_bound + 2) && c.r(8) == c.u32(national_team) &&
                    c.r(9) == frame + 0x80 && c.r(10) == UINT32_MAX &&
                    c.u32(frame + 0x38) == std::uint32_t(-2) && c.u32(frame + 0x3c) == reserve,
                "duty fixture filters, windows and optional list");
        c.put32(c.r(9) + 4, c.u32(fixtures_head));
        c.set(3, c.u32(fixture_count));
        break;
    }
    case dispose_list:
        require(c.r(3) == frame + 0x80 && c.r(4) == UINT32_MAX, "duty list disposal ABI");
        c.put8(disposed, c.u8(disposed) + 1);
        c.put32(c.r(3) + 4, 0);
        c.set(3, 0xb0b0b0b0);
        break;
    case localize: {
        const auto key = c.r(4) - source;
        const auto output = key == 0x7cb7 ? label_shared : label_out;
        require((key == 0x7c62 || key == 0x7c8d || key == 0x7cb7) &&
                    c.r(3) == output && c.r(5) == source + 0x7c8c, "duty label destination and key");
        c.put32(output, key);
        c.set(3, 0xc0c0c0c0);
        break;
    }
    case 0x107156b0:
        if (resume == 0x102ba31c) {
            require(c.r(3) == frame + 0x48 && c.r(4) == std::uint32_t(c.i16(calendar_match + 0x2a)) &&
                        c.r(5) == c.u16(calendar_match + 0x28), "domestic fixture date argument widths");
            fixture_date(c, c.r(3), c.i16(calendar_match + 0x2a), c.u16(calendar_match + 0x28));
        } else if (resume == 0x102b9e90) {
            require(c.r(3) == frame + 0x68 && c.r(4) == std::uint32_t(c.i16(calendar_match + 0x2a)) &&
                        c.r(5) == c.u16(calendar_match + 0x28), "calendar match date argument widths");
            fixture_date(c, c.r(3), c.i16(calendar_match + 0x2a), c.u16(calendar_match + 0x28));
        } else {
            require(c.r(3) == frame + 0x68 && c.r(4) == 90 && c.r(5) == 2000,
                    "all-star calendar base and signed year offset");
            fixture_date(c, c.r(3), c.i16(cutoff_date), c.u16(cutoff_date + 2));
        }
        break;
    case 0x10494d20:
        if (fixture_marking) {
            require(c.r(3) == competition_manager && (c.r(4) == competition || c.r(4) == competition + 4),
                    "fixture marking competition lookup");
            c.set(3, c.r(4) == competition ? 0x100u : 0x101u);
            break;
        }
        require(c.r(3) == competition_manager && c.r(4) == competition, "calendar competition lookup");
        return false;
    default:
        return false; // The club classifier and linked-list iterator are real.
    }
    c.finish();
    return true;
}

struct ObserveCalls {
    decltype(dispatch_probe) previous = dispatch_probe;
    explicit ObserveCalls(std::vector<DependencyCall>& trace) {
        dependencies = &trace;
        dispatch_probe = observe_dependency;
    }
    ~ObserveCalls() {
        dispatch_probe = previous;
        dependencies = nullptr;
    }
};

std::unique_ptr<State> compare(const State& input, Stop expected = Stop::returned,
                               Corrupt corrupt = Corrupt::none, bool real_dependencies = false,
                               std::initializer_list<std::uint32_t> nested_owners = {}) {
    auto original = clone(input), rewrite = clone(input);
    Effects original_effects, rewrite_effects;
    std::vector<DependencyCall> original_calls, rewrite_calls;
    const auto run = [&](State& s, Effects& effects, std::vector<DependencyCall>& calls, bool reference) {
        std::optional<ObserveCalls> observer;
        if (!real_dependencies) {
            observer.emplace(calls);
            // The original caller can enter the allocator without dispatching.
            // Decline that direct route so the fixture sees both calls.
            if (input.pc == calendar_load || input.pc == create_schedule || input.pc == register_fixture ||
                input.pc == expire_schedule || input.pc == mark_fixtures || input.pc == apply_marks ||
                input.pc == release_assignments)
                s.allocator_original = true;
        }
        struct RestoreChecker {
            RoutineChecker* previous = routine_checker;
            ~RestoreChecker() { routine_checker = previous; }
        } restore;
        RoutineChecker notice_boundary([](std::uint32_t entry) {
            return entry == club_notice || entry == nation_notice || entry == replacement_notice || entry == injured_selection ||
                entry == tier || entry == drop_roster || entry == dedup_rosters ||
                entry == add_replacement || entry == replacement_objection || entry == automatic_replacement ||
                entry == calendar_save || entry == calendar_load || entry == create_schedule || entry == register_fixture ||
                entry == date_order || entry == score_order || entry == club_order || entry == mark_fixtures || entry == apply_marks || entry == publish_calendar_notices ||
                entry == render_calendar_notice;
        }, 0, 0);
        if (!real_dependencies && (input.pc == club_notice || input.pc == nation_notice ||
                                   input.pc == replacement_notice || input.pc == injured_selection ||
                                   input.pc == automatic_replacement || input.pc == calendar_save ||
                                   input.pc == calendar_load || input.pc == create_schedule ||
                                   input.pc == register_fixture || input.pc == publish_calendar_notices ||
                                   input.pc == render_calendar_notice || nested_owners.size()))
            routine_checker = &notice_boundary;
        if (input.pc == publish_calendar_notices)
            utility_object_diagnostics(s).original_body = 0x10738ca0;
        if (input.pc == duty)
            utility_object_diagnostics(s).original_body = dispose_list;
        if (input.pc == duty || input.pc == render_calendar_notice)
            localization_format_diagnostics(s).original_body = localize;
        effects.state = &s;
        s.memory.date_observer = &effects;
        std::string error;
        try {
            if (reference) national_teams_run_original(s, input.pc);
            else run_until(s, done);
        } catch (const std::runtime_error& failure) {
            error = failure.what();
        }
        s.memory.date_observer = nullptr;
        if (error.empty()) {
            for (const auto entry : nested_owners) {
                const auto& totals = notice_boundary.totals();
                const bool reached = totals.contains(entry) && totals.at(entry).calls > 0;
                require(reached == !reference, "nested NATIONAL_TEAMS owner route " + hex_address(entry));
            }
        }
        return error;
    };
    const auto a = run(*original, original_effects, original_calls, true);
    const auto b = run(*rewrite, rewrite_effects, rewrite_calls, false);
    auto label = "NATIONAL_TEAMS " + hex_address(input.pc);
    if (input.pc == automatic_replacement) {
        label += " mode=" + hex_address(std::uint32_t(input.r<4>(guest::reg::gpr(5)))) +
                 " tier=" + hex_address(std::uint32_t(input.r<4>(guest::reg::gpr(6))));
    }
    if (input.pc == render_calendar_notice) {
        const auto field = [&](unsigned index) {
            return std::int32_t(input.memory.read<4>(notice_render_item + 8 + index * 4));
        };
        label += " count=" + std::to_string(field(1)) + " variant=" + std::to_string(field(2)) +
                 " detail=" + std::to_string(field(3)) + " secondary=" + std::to_string(field(5));
    }
    if (input.pc == calendar_load) {
        label += " read=" + std::to_string(input.memory.read<1>(archive_trace)) +
                 " alloc=" + std::to_string(input.memory.read<1>(archive_trace + 0x18)) +
                 " length=" + std::to_string(input.memory.read<4>(archive_trace + 0x14));
    }
    require(a == b && (expected == Stop::fault ? a.starts_with("Unmapped data access at ") :
                      expected == Stop::budget ? a == "Native instruction budget exhausted" : a.empty()),
            label + " outcome " + a + " at " + hex_address(original->pc) +
                " / " + b + " at " + hex_address(rewrite->pc));
    if (expected == Stop::returned) {
        require(original->pc == done && rewrite->pc == done, label + " normal continuation");
        if (input.pc == create_schedule || input.pc == register_fixture || input.pc == expire_schedule ||
            input.pc == date_order || input.pc == score_order || input.pc == club_order ||
            input.pc == mark_fixtures || input.pc == apply_marks || input.pc == queue_calendar_notice ||
            input.pc == publish_calendar_notices || input.pc == render_calendar_notice ||
            input.pc == append_appointment || input.pc == expire_nation_appointments ||
            input.pc == expire_pair_appointments || input.pc == replace_assignment || input.pc == reconcile_assignments ||
            input.pc == direct_assignment || input.pc == release_assignments)
            require(original->executed == rewrite->executed, label + " instruction accounting original=" +
                std::to_string(original->executed) + " rewrite=" + std::to_string(rewrite->executed));
    }
    if (original_effects.bytes != rewrite_effects.bytes) {
        const auto describe = [](const Effects::Access& a) {
            return " kind=" + std::to_string(a.kind) + " address=" + hex_address(a.address) +
                   " width=" + std::to_string(a.size) + " value=" + std::to_string(a.value);
        };
        const auto [old, now] = std::mismatch(original_effects.log.begin(), original_effects.log.end(),
            rewrite_effects.log.begin(), rewrite_effects.log.end(), [](const auto& a, const auto& b) {
                return a.kind == b.kind && a.address == b.address && a.size == b.size && a.value == b.value;
            });
        throw std::runtime_error(label + " ordered memory effect " +
            std::to_string(old - original_effects.log.begin()) +
            " original:" + (old == original_effects.log.end() ? " end" : describe(*old)) +
            " rewrite:" + (now == rewrite_effects.log.end() ? " end" : describe(*now)));
    }
    if (corrupt == Corrupt::dependency) {
        require(!rewrite_calls.empty(), "dependency corruption must reach the shared club query");
        rewrite_calls.front()[5] ^= 1;
    }
    require(original_calls == rewrite_calls, label + " ordered dependency arguments");
    if (corrupt == Corrupt::result) Cpu(*rewrite).set(3, Cpu(*rewrite).r(3) ^ 1);
    Image(*original).compare(*rewrite, label, expected == Stop::fault);
    return rewrite;
}

unsigned check_selection_slots(std::mt19937& random) {
    unsigned comparisons = 0;
    for (const auto entry : {read_flag, clear_flag, read_value, get_slot}) {
        for (int id : {-4, 0, 1, 5}) {
            for (unsigned group : {0u, 1u, 2u, 0x100u, 0x101u, 0x7fu, 0x80u, 0xffu}) {
                auto s = seed(random);
                Cpu c(*s);
                const auto slot = records + std::uint32_t(id) * record_size +
                                  std::uint32_t(std::int8_t(group)) * group_size + 0x40;
                c.put32(nation + 0x68, std::uint32_t(id));
                const auto value = (random() & 0x7f) | 0x80, flag = (random() & 0x7f) + 1;
                c.put8(slot + 0xa, value);
                c.put8(slot + 0xb, flag);
                c.set({{3, manager}, {4, nation}, {5, group}});
                s->pc = entry;
                auto result = compare(*s);
                ++comparisons;
                Cpu after(*result);
                const auto expected = entry == read_flag ? flag : entry == read_value ? value :
                    entry == get_slot ? slot : slot - 0x40;
                require(after.r(3) == expected && after.u8(slot + 0xa) == value &&
                            after.u8(slot + 0xb) == (entry == clear_flag ? 0 : flag),
                        "selection reads unsigned bytes and clears just the flag");
            }
        }
    }
    return comparisons;
}

unsigned check_selection_tiers(std::mt19937& random) {
    unsigned comparisons = 0;
    for (const unsigned mode : {0u, 1u, 0x100u, 0x180u, UINT32_MAX}) {
        for (const int pool : {-1, 0, 49, 50, 51, 99, 100, 101, INT16_MAX}) {
            for (unsigned first_stop = 0; first_stop <= 6; ++first_stop) {
                auto state = seed(random);
                Cpu c(*state);
                c.set({{4, club}, {5, nation}, {6, mode}});
                c.put32(club + 0x14, club + 0x80);
                c.put16(nation, pool);
                for (unsigned score = 0; score < 6; ++score) {
                    const auto value = score < first_stop ? (score % 2 ? 0x80 : 14) :
                        score == first_stop ? 15 : 127;
                    c.put8(club + 0x8f + score, value);
                }
                state->pc = tier;
                const unsigned expected = pool < (std::int8_t(mode) ? 50 : 100) ? 0 :
                    first_stop == 0 ? 0 : first_stop <= 2 ? 1 : first_stop <= 5 ? 2 : 3;
                auto result = compare(*state);
                require(Cpu(*result).r(3) == expected, "signed pool limit and first limiting assessment");
                ++comparisons;
            }
        }
    }
    return comparisons;
}

unsigned check_selection_prefixes(std::mt19937& random) {
    unsigned comparisons = 0;
    for (const auto group : {0u, 1u, 0x17fu, 0x180u, UINT32_MAX}) {
        for (const auto count : {0u, 1u, 7u, 8u, 9u, 15u, 16u, 17u, 24u, 25u, 127u,
                                 0x100u, 0x101u, 0x109u, 0x180u, UINT32_MAX}) {
            auto state = seed(random);
            Cpu c(*state);
            c.set({{4, records}, {5, group}, {6, count}});
            const auto row = records + std::uint32_t(std::int8_t(group)) * 0x18;
            std::uint32_t expected = UINT32_MAX - (random() & 0xf);
            c.put32(row + 8, expected);
            for (int i = 0; i < std::int8_t(count); ++i) {
                const auto weight = (random() & 0x7f) + 0x80;
                c.put8(row + 0xf + i * 4, weight);
                expected += weight;
            }
            state->pc = prefix;
            auto result = compare(*state);
            require(Cpu(*result).r(3) == expected, "signed group and count, unsigned weights and wrapping total");
            ++comparisons;
        }
    }
    return comparisons;
}

unsigned check_international_competition(std::mt19937& random) {
    constexpr std::array toc_offsets{-0x7bc0, -0x7a40, -0x6948, -0x694c, -0x6950,
                                     -0x693c, -0x6a3c, -0x6938, -0x6b00};
    unsigned comparisons = 0;
    for (unsigned match = 0; match <= toc_offsets.size() + 2; ++match) {
        auto state = seed(random);
        Cpu c(*state);
        const auto id = match % 2 ? 7u : UINT32_MAX;
        const auto available = toc_offsets.size();
        for (unsigned slot = 0; slot < available; ++slot) {
            const auto entry = records + slot * 4;
            const auto hit = slot == match || (match == available + 2 && (slot == 2 || slot == 6));
            c.put32(toc + toc_offsets[slot], entry);
            c.put32(entry, hit ? id : slot + 10);
        }
        c.put32(nation, id);
        c.set(4, match == available + 1 ? 0 : nation);
        state->pc = international_event;
        auto result = compare(*state);
        require(Cpu(*result).r(3) == (match < available || match == available + 2),
                "nullable competition and ordered dynamic IDs");
        ++comparisons;
    }
    return comparisons;
}

unsigned check_selection_query_faults(std::mt19937& random) {
    unsigned faults = 0;
    for (const auto entry : {tier, prefix, international_event}) {
        auto state = seed(random);
        Cpu c(*state);
        c.set({{4, unmapped}, {5, unmapped}, {6, 0}});
        state->pc = entry;
        if (entry == international_event) c.put32(toc - 0x7bc0, nation);
        compare(*state, Stop::fault);
        ++faults;
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    for (const unsigned count : {3u, 9u}) {
        auto state = seed(random);
        Cpu c(*state);
        const auto row = fixture + 0x5000 - (count == 3 ? 20 : 24);
        c.set({{4, row}, {5, 0}, {6, count}});
        c.put32(row + 8, 17);
        c.put8(row + 0xf, 3);
        c.put8(row + 0x13, 255);
        state->pc = prefix;
        compare(*state, Stop::fault);
        ++faults;
    }
    return faults;
}

unsigned check_staff(std::mt19937& random) {
    unsigned comparisons = 0;
    for (unsigned nation_arg : {0u, nation}) {
        for (unsigned id : {0u, 1u, 3u, UINT32_MAX}) {
            for (unsigned staff : {0u, 2u, 9u, UINT32_MAX, UINT32_MAX - 1, 0x80000000u}) {
                auto s = seed(random);
                Cpu c(*s);
                c.put32(nation + 0x68, id);
                c.put32(records + id * record_size + 4, staff);
                c.set({{3, manager}, {4, nation_arg}});
                s->pc = get_staff;
                auto result = compare(*s);
                ++comparisons;
                require(Cpu(*result).r(3) == (nation_arg && std::int32_t(staff) >= 0 ? staff_table + staff * staff_size : 0),
                        "signed staff sentinel and record stride");
            }
        }
    }
    return comparisons;
}

unsigned check_context(std::mt19937& random) {
    unsigned comparisons = 0;
    for (unsigned id : {0u, 179u, 180u, 199u, 200u}) {
        for (unsigned mode : {0u, 1u, 0x100u, 0x180u, UINT32_MAX}) {
            auto s = seed(random);
            Cpu c(*s);
            c.put32(club, id);
            c.set({{3, manager}, {4, club}, {5, mode}});
            s->pc = context;
            auto result = compare(*s);
            ++comparisons;
            require(Cpu(*result).r(3) == (std::int8_t(mode) ? 1u : id >= 180 ? 2u : 0u),
                    "selection context uses the actual club-kind query and low byte");
        }
    }
    return comparisons;
}

unsigned check_international_club(std::mt19937& random) {
    unsigned comparisons = 0;
    for (auto id : {0u, 1u, 2u, 3u, 159u, 160u, 199u}) {
        auto state = seed(random);
        Cpu c(*state);
        c.put32(toc - 0x7208, club_count + 8);
        c.put32(toc - 0x7204, club_count + 12);
        c.put32(club_count + 8, 1);
        c.put32(club_count + 12, 2);
        c.put32(club, id);
        c.set(4, club);
        state->pc = international;
        auto result = compare(*state);
        require(Cpu(*result).r(3) == (id >= 160 || id == 1 || id == 2), "international and reserved club classification");
        ++comparisons;
    }
    return comparisons;
}

std::unique_ptr<State> duty_seed(std::mt19937& random) {
    auto state = seed(random);
    state->memory.map(fixture + 0x5000, 0x4000);
    Cpu c(*state);
    c.set({{3, manager}, {4, player}, {5, date}, {6, regular_club}, {7, label_out}, {8, 0}});
    c.put32(toc - 0x7208, first_star);
    c.put32(toc - 0x7204, second_star);
    c.put32(toc - 0x7bb8, label_shared);
    c.put32(toc - 0x7be8, clubs_slot);
    c.put32(toc - 0x513c, source);
    c.put32(toc - 0x7a60, fixture_slot);
    c.put32(toc - 0x6868, calendar_index);
    c.put32(toc - 0x7b5c, calendar_slot);
    c.put32(toc - 0x7b70, allowed_nation);
    c.put32(first_star, 1);
    c.put32(second_star, 2);
    c.put32(clubs_slot, clubs);
    c.put32(fixture_slot, 0x51515151);
    c.put32(regular_club, 7);
    c.put32(national_team, 123);
    c.put32(player + 0x20, national_team);
    c.put32(competition, 42);
    c.put32(calendar_slot, calendar_array);
    c.put32(calendar + 0xa0, calendar_date);
    c.put16(calendar + 0x40, 2001);
    c.put16(calendar_date + 4, 90);
    c.put8(calendar_date + 6, 0xff);
    c.put32(allowed_nation, 14);
    c.put32(team_nation, 14);
    c.put32(regular_club + 0x30, team_nation);
    fixture_date(c, date, 90, 2000);
    fixture_date(c, first_bound, 84, 2000);
    fixture_date(c, second_bound, 96, 2000);
    fixture_date(c, cutoff_date, 90, 2000);
    state->pc = duty;
    return state;
}

void link_two(Cpu& c, std::uint32_t nodes, std::uint32_t values) {
    c.put32(nodes, values);
    c.put32(nodes + 8, nodes + 0x10);
    c.put32(nodes + 0x10, values + 0x20);
}

unsigned check_duty(std::mt19937& random) {
    unsigned comparisons = 0;
    const auto check = [&](const State& state, unsigned result, unsigned key = 0) {
        auto rewrite = compare(state);
        Cpu c(*rewrite);
        require(c.r(3) == result && c.u8(disposed) == 1, "duty result and one list disposal");
        require(c.u32(label_out) == (key == 0x7cb7 ? 0 : key) &&
                    c.u32(label_shared) == (key == 0x7cb7 ? key : 0), "duty label effect");
        ++comparisons;
    };
    for (unsigned id : {1u, 2u, 7u, 160u}) {
        auto state = duty_seed(random);
        Cpu(*state).put32(regular_club, id);
        check(*state, 0);
    }
    for (bool filter : {false, true}) {
        for (bool found : {false, true}) {
            for (bool label : {false, true}) {
                for (unsigned flags : {0u, 4u, 0x84u}) {
                    auto state = duty_seed(random);
                    Cpu c(*state);
                    c.set({{7, label ? label_out : 0}, {8, filter ? competition : 0}});
                    c.put8(player + 0x6a, flags);
                    c.put32(fixture_count, 2);
                    c.put32(fixtures_head, fixture_nodes);
                    link_two(c, fixture_nodes, fixture_rows);
                    if (found) c.put32(fixture_rows + 0x20 + 0x14, competition);
                    const bool eligible = !filter || found;
                    check(*state, eligible ? national_team : 0, eligible && label ? 0x7c62 : 0);
                }
            }
        }
    }
    for (const auto [day, year] : std::array<std::pair<int, unsigned>, 6>{{
             {80, 1999}, {100, 2001}, {89, 2000}, {90, 2000}, {91, 2000}, {-1, 2000}}}) {
        for (unsigned filter : {0u, 1u, 2u}) {
            auto state = duty_seed(random);
            Cpu c(*state);
            c.set(8, filter ? competition : 0);
            c.put8(player + 0x6a, 4);
            c.put32(manager, roster_list);
            c.put32(roster_list + 4, roster_nodes);
            link_two(c, roster_nodes, roster_rows);
            c.put32(roster_rows + 0x20, 123);
            c.put32(roster_rows + 0x24, filter == 2 ? 7 : 42);
            fixture_date(c, roster_rows + 0x28, day, year);
            const bool matches = filter != 2 && (year < 2000 || (year == 2000 && day < 90));
            check(*state, matches ? national_team : 0, matches ? 0x7c8d : 0);
        }
    }
    {
        auto state = duty_seed(random);
        Cpu c(*state);
        c.put32(manager, roster_list);
        c.put32(roster_list + 4, roster_nodes);
        c.put32(roster_nodes, roster_rows);
        c.put32(roster_rows, 123);
        fixture_date(c, roster_rows + 8, 89, 2000);
        check(*state, 0); // The senior-team path skips even an otherwise matching roster.
        c.put8(player + 0x6a, 4);
        c.set(7, 0);
        check(*state, national_team);
    }
    for (unsigned scenario = 0; scenario < 17; ++scenario) {
        auto state = duty_seed(random);
        Cpu c(*state);
        c.set({{6, 0}, {7, scenario == 3 || scenario == 16 ? 0 : label_out}});
        c.put32(player + 0x20, 0);
        if (scenario != 4) c.put32(calendar_array, calendar);
        if (scenario != 5) c.put32(player + 0x24, regular_club);
        if (scenario == 6) c.put32(regular_club + 0x30, 0);
        if (scenario == 7) c.put32(allowed_nation, 13);
        if (scenario == 8) fixture_date(c, cutoff_date, 97, 2000);
        if (scenario == 9) fixture_date(c, cutoff_date, 1, 2001);
        if (scenario == 10) fixture_date(c, cutoff_date, 366, 1999);
        if (scenario == 11) fixture_date(c, cutoff_date, 83, 2000);
        if (scenario == 12) fixture_date(c, cutoff_date, 84, 2000);
        if (scenario == 13) fixture_date(c, cutoff_date, 96, 2000);
        if (scenario == 14) {
            fixture_date(c, first_bound, 366, 1999);
            fixture_date(c, second_bound, 1, 2001);
        }
        if (scenario == 15) {
            fixture_date(c, first_bound, -2, 2000);
            fixture_date(c, cutoff_date, 0, 2000);
        }
        const auto star_club = scenario == 2 || scenario == 16 ? second_club : first_club;
        if (scenario != 0) c.put32(star_club + 0x98 + 49 * 4, player);
        if (scenario == 1) c.put32(second_club + 0x98 + 49 * 4, player);
        const auto expected = scenario == 1 || scenario == 2 || scenario == 3 || scenario >= 12 ? star_club : 0;
        check(*state, expected, expected && c.r(7) ? 0x7cb7 : 0);
    }
    return comparisons;
}

unsigned check_remove_selection(std::mt19937& random) {
    constexpr auto profile = fixture + 0x6420, queue = fixture + 0x6600;
    constexpr auto country_row = records + 2 * record_size;
    unsigned comparisons = 0;
    for (unsigned level = 0; level <= 3; ++level) {
        for (const int delta : {-1, 0, 1}) {
            for (unsigned scenario = 0; scenario < 10; ++scenario) {
                auto state = duty_seed(random);
                Cpu c(*state);
                c.set({{3, manager}, {4, player}});
                c.put32(player + 0xc, scenario == 8 ? 0 : nation);
                c.put32(player + 0x14, scenario == 9 ? 0 : profile);
                const bool linked = scenario != 6;
                c.put32(player + 0x20, linked ? national_team : 0);
                c.put8(player + 0x6a, 0xf9 + scenario % 4);
                c.put32(nation + 0x68, 2);
                c.put16(country_row, 100);
                constexpr std::array first_blocking_score{0u, 1u, 3u, 6u};
                if (level < 3) c.put8(profile + 0xf + first_blocking_score[level], 15);
                c.put16(profile + 8, -10);
                const auto bucket = country_row + level * 4;
                c.put16(bucket + 0x10, -10 + delta);
                const auto start = 2u + 3u * level;
                c.put32(country_row + 8, 2);
                for (unsigned i = 0; i < level; ++i) c.put8(country_row + 0xf + i * 4, 3);
                const unsigned length = scenario == 0 ? 0 : scenario == 1 ? 1 : 4;
                const unsigned position = scenario == 3 ? 1 : scenario == 4 ? 3 : 0;
                const bool queued = scenario != 0 && scenario != 5;
                c.put8(bucket + 0xe, length);
                c.put32(manager + 0xc, queue);
                for (unsigned i = 0; i < length; ++i)
                    c.put32(queue + (start + i) * 4, queued && i == position ? player : 0x8100 + i);
                c.put32(queue + (start + length) * 4, 0xcafebabe);
                const auto last = length ? c.u32(queue + (start + length - 1) * 4) : 0;
                const auto prior_slot = c.u32(queue + (start + position) * 4);
                const auto roster_index = scenario % 2 ? 0u : 49u;
                const bool roster_has_player = scenario != 7;
                if (roster_has_player) c.put32(national_team + 0x98 + 4 * roster_index, player);
                state->pc = remove_player;
                auto result = compare(*state);
                Cpu after(*result);
                const bool eligible = scenario < 8;
                const bool removed = eligible && delta < 0 && queued;
                require(after.u8(bucket + 0xe) == length - removed &&
                            after.u32(queue + (start + position) * 4) == (removed ? last : prior_slot) &&
                            after.u32(queue + (start + length) * 4) == 0xcafebabe &&
                            (!length || after.u32(queue + (start + length - 1) * 4) == last),
                        "selection removal swaps with the last slot and retains its old contents");
                require(after.u32(player + 0x20) == (eligible || !linked ? 0 : national_team) &&
                            after.u8(player + 0x6a) == (eligible && linked ? c.u8(player + 0x6a) & 3 : c.u8(player + 0x6a)) &&
                            after.u32(national_team + 0x98 + 4 * roster_index) ==
                                (roster_has_player && (!eligible || !linked) ? player : 0),
                        "selection removal clears the first roster slot, link and duty flags");
                if (eligible && delta == -1) {
                    auto direct = clone(*state);
                    run_until(*direct, done);
                    Image(*result).compare(*direct, "direct tier call during selection removal");
                }
                ++comparisons;
            }
        }
    }
    return comparisons;
}

unsigned check_selected_player(std::mt19937& random) {
    unsigned comparisons = 0;
    for (unsigned mode : {0u, 1u, 2u, 3u, 0x100u, 0x101u, 0x102u, 0x180u, UINT32_MAX}) {
        for (unsigned link : {0u, 1u, 2u, 3u}) {
            for (unsigned flags : {0u, 4u, 8u, 0x8cu}) {
                auto state = duty_seed(random);
                Cpu c(*state);
                c.set({{4, player}, {5, nation}, {6, mode}});
                c.put32(nation + 0x68, 5);
                c.put32(toc - 0x7b98, calendar_slot);
                const auto senior = clubs + (200 - 40 + 5) * 0x248;
                const auto alternate = clubs + (200 - 20 + 5) * 0x248;
                const auto team = link == 0 ? 0 : link == 1 ? senior : link == 2 ? alternate : national_team;
                c.put32(player + 0x20, team);
                c.put8(player + 0x6a, flags);
                state->pc = selected_player;
                const auto kind = std::int8_t(mode);
                const bool expected = (kind == 0 && link == 1 && (flags & 4)) ||
                    (kind == 1 && link == 1 && (flags & 8)) || (kind == 2 && link == 2 && (flags & 4));
                auto result = compare(*state);
                require(Cpu(*result).r(3) == expected, "national-team variant, exact team and selection bit");
                ++comparisons;
            }
        }
    }
    return comparisons;
}

constexpr unsigned club_stride = 0x248, national_roster_offset = 0x98, national_roster_size = 50;
constexpr unsigned fixture_nation_id = 5, fixture_club_count = 200, fixture_nation_count = 20;
constexpr auto senior_team = clubs + (fixture_club_count - 2 * fixture_nation_count + fixture_nation_id) * club_stride;
constexpr auto alternate_team = clubs + (fixture_club_count - fixture_nation_count + fixture_nation_id) * club_stride;

std::unique_ptr<State> two_rosters(std::mt19937& random, bool map_alternate = true,
                                   unsigned team_size = club_stride) {
    auto state = duty_seed(random);
    Cpu c(*state);
    c.put32(toc - 0x7b98, calendar_slot);
    c.put32(nation + 0x68, fixture_nation_id);
    c.put32(club_count, fixture_club_count);
    c.put32(nation_count, fixture_nation_count);
    state->memory.map(senior_team, team_size);
    if (map_alternate) state->memory.map(alternate_team, team_size);
    state->budget = 100000;
    return state;
}

unsigned check_dedup_rosters(std::mt19937& random) {
    unsigned comparisons = 0;
    for (unsigned pattern = 0; pattern < 6; ++pattern) {
        for (unsigned trial = 0; trial < 6; ++trial) {
            auto state = two_rosters(random);
            Cpu c(*state);
            c.set(4, nation);
            for (const auto team : {senior_team, alternate_team}) {
                for (unsigned slot = 0; slot < national_roster_size; ++slot) {
                    std::uint32_t value = 0;
                    if (pattern == 1) value = player + 4 * slot;
                    if (pattern == 2) value = player;
                    if (pattern == 3 && (slot == 0 || slot == 49)) value = UINT32_MAX;
                    if (pattern == 4 && (slot % 5 != 0)) value = player + 4 * (random() % (1 + trial * 8));
                    if (pattern == 5 && slot == 49) value = player;
                    c.put32(team + national_roster_offset + slot * 4, value);
                }
            }
            state->pc = dedup_rosters;
            auto result = compare(*state);
            Cpu after(*result);
            for (const auto team : {senior_team, alternate_team}) {
                std::array<std::uint32_t, national_roster_size> seen{};
                for (unsigned slot = 0; slot < seen.size(); ++slot) {
                    const auto address = team + national_roster_offset + slot * 4;
                    const auto prior = c.u32(address);
                    const auto first = std::find(seen.begin(), seen.begin() + slot, prior) == seen.begin() + slot;
                    require(after.u32(address) == (first ? prior : 0), "roster retains its first occurrence in place");
                    seen[slot] = prior;
                }
            }
            ++comparisons;
        }
    }
    return comparisons;
}

constexpr auto tactics_slot = fixture + 0x6400, tactics_state = fixture + 0x7800;
constexpr auto tactics_teams = fixture + 0x8000, staff_count = fixture + 0x6420;

std::unique_ptr<State> drop_seed(std::mt19937& random, bool map_roster = true) {
    auto state = two_rosters(random, true, map_roster ? club_stride : national_roster_offset);
    Cpu c(*state);
    c.set({{4, nation}, {6, player}});
    c.put32(toc - 0x7a6c, tactics_slot);
    c.put32(tactics_slot, tactics_state);
    c.put32(toc - 0x7be0, staff_slot);
    c.put32(staff_slot, player - 7 * tactics::staff_size);
    c.put32(toc - 0x7be4, staff_count);
    c.put32(staff_count, 12);
    c.put32(tactics_state + tactics::field::teams, tactics_teams);
    c.put32(tactics_state + tactics::field::team_count, 3);
    c.put32(senior_team, 1);
    c.put32(alternate_team, 2);
    c.put32(player, 7);
    c.put32(player + 0x14, player + 0x80);
    c.put8(player + 0x60, 0xab);
    c.put8(player + 0x6a, 0xfe);
    state->pc = drop_roster;
    return state;
}

unsigned check_drop_roster(std::mt19937& random) {
    unsigned comparisons = 0;
    for (const unsigned mode : {0u, 1u, 2u, 3u, 0x100u, 0x101u, 0x180u, UINT32_MAX}) {
        for (unsigned scenario = 0; scenario < 6; ++scenario) {
            auto state = drop_seed(random);
            Cpu c(*state);
            const auto variant = std::int8_t(mode);
            const auto team = variant == 0 || variant == 1 ? senior_team : alternate_team;
            const auto other = team == senior_team ? alternate_team : senior_team;
            c.set(5, mode);
            c.put32(player + 0x20, team);
            c.put32(player + 0x24, team);
            const auto roster_slot = team + national_roster_offset + (scenario == 2 ? 49u : 0u) * 4;
            const auto other_slot = other + national_roster_offset;
            const bool listed = scenario != 0 && scenario != 5;
            c.put32(roster_slot, listed ? player : 0);
            if (scenario == 3) {
                c.put32(team + national_roster_offset + 49 * 4, player);
                c.put32(other_slot + 4, player);
            }
            c.put32(other_slot, player);
            auto team_record = tactics_teams + c.u32(team) * tactics::team_size;
            for (unsigned offset = tactics::field::squad; offset < tactics::field::lineup; offset += 4)
                c.put32(team_record + offset, UINT32_MAX);
            if (scenario != 5) {
                c.put32(team_record + tactics::field::squad, 7);
                c.put32(team_record + tactics::field::captains, 7);
                c.put32(team_record + tactics::field::takers + 9 * 4, 7);
            }
            c.put8(tactics_state + tactics::field::backup_count, 2);
            for (unsigned managed : {1u, 0u}) {
                const auto backup = tactics_state + tactics::field::backups + managed * tactics::backup_size;
                c.put32(backup + tactics::field::backup_club, team);
                c.put8(backup + tactics::field::backup_managed, managed);
                c.put8(backup + tactics::field::backup_active, 1);
            }

            auto result = compare(*state);
            Cpu after(*result);
            require(after.u32(roster_slot) == 0 && after.u32(other_slot) == player &&
                        after.u32(player + 0x20) == (listed ? 0 : team) &&
                        after.u8(player + 0x60) == (listed ? 0 : 0xab) &&
                        after.u8(player + 0x6a) == (listed ? 2 : 0xfe),
                    "dropping a listed player clears their roster slot, link, call-up status and duty flags");
            if (scenario == 3) require(after.u32(team + national_roster_offset + 49 * 4) == player,
                                        "individual removal stops after the first roster slot");
            require(after.u32(team_record + tactics::field::squad) == UINT32_MAX &&
                        after.u32(team_record + tactics::field::captains) == UINT32_MAX &&
                        after.u32(team_record + tactics::field::takers + 9 * 4) == UINT32_MAX &&
                        after.u8(tactics_state + tactics::field::backup_count) == 0,
                    "tactics and both managed-status backups are cleared even if the roster omits the player");
            ++comparisons;
            if (scenario == 3) {
                // Continue from the verified removal state and check that
                // deduplication is local to each roster.
                after.set(4, nation);
                result->pc = dedup_rosters;
                auto deduplicated = compare(*result);
                Cpu final(*deduplicated);
                require(final.u32(team + national_roster_offset + 49 * 4) == player &&
                            final.u32(other_slot) == player && final.u32(other_slot + 4) == 0,
                        "deduplication after removal keeps the first appearance in each roster");
                ++comparisons;
            }
        }
    }
    return comparisons;
}

unsigned check_roster_cleanup_faults(std::mt19937& random) {
    auto state = two_rosters(random, false);
    Cpu c(*state);
    c.set(4, nation);
    c.put32(senior_team + national_roster_offset, player);
    c.put32(senior_team + national_roster_offset + 4, player);
    state->pc = dedup_rosters;
    auto result = compare(*state, Stop::fault);
    require(Cpu(*result).u32(senior_team + national_roster_offset + 4) == 0,
            "the first roster is cleaned before an unreadable second roster faults");
    state->budget = 0;
    compare(*state, Stop::budget);

    state = drop_seed(random, false);
    Cpu drop(*state);
    drop.set(5, 0);
    for (unsigned offset = tactics::field::squad; offset < tactics::field::lineup; offset += 4)
        drop.put32(tactics_teams + tactics::team_size + offset, UINT32_MAX);
    drop.put32(tactics_teams + tactics::team_size + tactics::field::squad, 7);
    result = compare(*state, Stop::fault);
    require(Cpu(*result).u32(tactics_teams + tactics::team_size + tactics::field::squad) == UINT32_MAX,
            "tactics changes precede the fault on the roster");
    state->budget = 0;
    compare(*state, Stop::budget);
    return 2;
}

void populate_notice(Cpu& c, unsigned mode, unsigned policy) {
    c.set({{4, player}, {5, nation}, {6, mode}, {7, 0x180}});
    c.put32(toc - 0x7bf0, news_slot);
    c.put32(toc - 0x7ba0, injury_slot);
    c.put32(toc - 0x7b7c, contracts_slot);
    c.put32(toc - 0x7b78, contract_scratch);
    c.put32(toc - 0x64d4, injury_types);
    c.put32(toc - 0x7b80, date);
    c.put32(toc - 0x79ec, notice_language);
    c.put32(news_slot, news_system);
    c.put32(injury_slot, injury_system);
    c.put32(contracts_slot, contract_system);
    c.put8(notice_policy, policy);
    c.put8(notice_language, 1);
    c.put8(notice_injury, 2);
    c.put8(injury_types + 25, 0x81);
    c.put32(notice_contract + 4, 5);
    c.put32(player, 19);
    c.put32(senior_team, 31);
    c.put32(alternate_team, 42);
    for (unsigned i = 1; i <= 3; ++i) {
        const auto name = fixture + 0x6a00 + i * 0x30;
        c.put32(player + 0x24 + i * 4, name);
        c.put32(name + 0x20, 0x9870 + i);
    }
}

std::unique_ptr<State> notice_seed(std::mt19937& random, unsigned mode, unsigned policy, std::uint32_t entry) {
    auto state = two_rosters(random);
    Cpu c(*state);
    populate_notice(c, mode, policy);
    state->pc = entry;
    return state;
}

unsigned check_roster_notices(std::mt19937& random) {
    unsigned comparisons = 0;
    for (unsigned mode : {0u, 1u, 2u, 0x100u, 0x101u, 0x180u, UINT32_MAX}) {
        const auto kind = std::int8_t(mode);
        const auto team = kind == 0 || kind == 1 ? senior_team : alternate_team;
        const auto check_message = [&](Cpu& c, unsigned category, bool injured) {
            const auto news = stack - (category == 0x22 ? 0x150 : 0x140) + 0x38;
            require(c.u32(notice_events + 0x10) == category && c.u32(news) == category &&
                        c.u32(notice_events + 0x14) == (category == 0x20 ? 7 : category == 0x21 ? 6 : 12),
                    "roster news template and field count");
            const std::array expected{19u, 0x9871u, 0x9872u, 0x9873u, team == senior_team ? 31u : 42u,
                                      unsigned(kind == 1), injured ? 0xffffff81u : 0};
            for (unsigned i = 0; i < expected.size() - (category == 0x21); ++i)
                require(c.u32(news + 8 + i * 4) == expected[i], "roster notice player, team and injury payload");
            if (category == 0x22) {
                const std::array tail{0u, kind == 1 ? 7u : 0xffffff82u, 0xffffff80u, std::uint32_t(kind), fixture_nation_id};
                for (unsigned i = 0; i < tail.size(); ++i)
                    require(c.u32(news + 8 + (i + 7) * 4) == tail[i], "national notice signed payload");
            }
        };
        for (unsigned policy = 0; policy < 8; ++policy) {
            auto state = notice_seed(random, mode, policy, nation_notice);
            auto result = compare(*state);
            Cpu c(*result);
            check_message(c, 0x22, policy & 4);
            require(c.u32(notice_events) == unsigned((policy & 3) != 0) &&
                        c.u32(notice_events + 4) == (policy & 1 ? team : policy & 2 ? senior_team : 0),
                    "national notice selects current team or managed senior fallback");
            ++comparisons;
        }
        for (const auto entry : {club_notice, replacement_notice}) {
            for (unsigned scenario = 0; scenario < 10; ++scenario) {
                constexpr std::array policies{0u, 0u, 0u, 1u, 5u, 1u, 2u, 6u, 2u, 2u};
                auto state = notice_seed(random, mode, policies[scenario], entry);
                Cpu c(*state);
                const auto current = scenario == 0 || scenario == 7 ? 0u :
                    scenario == 5 ? recipient_club : regular_club;
                c.put32(player + 0x24, current);
                if (scenario == 2 || scenario >= 4) {
                    for (unsigned i = 0; i < 4; ++i) c.put32(contract_answers + i * 4, notice_contract);
                }
                if (scenario == 8) c.put32(contract_answers + 1 * 4, 0);
                if (scenario == 9) c.put32(contract_answers + 3 * 4, 0);
                auto result = compare(*state);
                Cpu after(*result);
                if (scenario >= 3) check_message(after, entry == replacement_notice ? 0x21 : 0x20, policies[scenario] & 4);
                constexpr std::array deliveries{0u, 0u, 0u, 2u, 2u, 1u, 2u, 1u, 2u, 2u};
                require(after.u32(notice_events) == deliveries[scenario] &&
                            after.u32(notice_events + 4) == (scenario < 3 ? 0 : scenario == 7 ? recipient_club : current) &&
                            after.u32(notice_events + 8) == (scenario == 4 || scenario == 6 || scenario == 8 ? recipient_club : 0),
                        "club notice original recipients and repeated contract reads");
                ++comparisons;
            }
        }
    }
    for (const auto entry : {club_notice, nation_notice, replacement_notice}) {
        auto state = notice_seed(random, 1, 7, entry);
        Cpu c(*state);
        c.put32(player + 0x24, regular_club);
        c.put32(player + 0x28, unmapped);
        compare(*state, Stop::fault);
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    return comparisons;
}

unsigned check_replacement_roster(std::mt19937& random) {
    unsigned comparisons = 0;
    for (unsigned mode : {0u, 1u, 2u, 3u, 0x100u, 0x101u, 0x102u, 0x180u, UINT32_MAX}) {
        for (unsigned first_open : {0u, 1u, 25u, 49u, 50u}) {
            for (const auto day : {-1, 0, 366}) {
                auto state = two_rosters(random);
                Cpu c(*state);
                const auto kind = std::int8_t(mode);
                const auto team = kind == 0 || kind == 1 ? senior_team : alternate_team;
                const auto other = team == senior_team ? alternate_team : senior_team;
                c.set({{4, nation}, {5, mode}, {6, player}});
                c.put32(toc - 0x7b80, date);
                fixture_date(c, date, day, 2000);
                c.put32(player + 0x20, 0);
                c.put16(player + 0x3c, 77);
                c.put16(player + 0x3e, 88);
                c.put8(player + 0x40, 0xbb);
                c.put8(player + 0x60, 0xa);
                c.put8(player + 0x6a, 0xfe);
                for (unsigned i = 0; i < first_open; ++i)
                    c.put32(team + national_roster_offset + i * 4, 0x8000 + i);
                c.put32(other + national_roster_offset, regular_club);
                state->pc = add_replacement;
                auto result = compare(*state);
                Cpu after(*result);
                const bool added = first_open < 50;
                require(after.u32(player + 0x20) == (added ? team : 0) &&
                            after.i16(player + 0x3c) == (added ? day : 77) &&
                            after.u16(player + 0x3e) == (added ? 2000 : 88) &&
                            after.u8(player + 0x40) == (added ? 1 : 0xbb) &&
                            after.u8(player + 0x60) == (added ? 0xb : 0xa) &&
                            after.u8(player + 0x6a) == (added ? (kind == 0 || kind == 2 ? 6 : 10) : 0xfe),
                        "replacement call-up date, team, status and duty bits");
                require(after.u32(other + national_roster_offset) == regular_club,
                        "replacement does not modify the other national team");
                for (unsigned i = 0; i < 50; ++i) {
                    const auto expected = i < first_open ? 0x8000 + i : i == first_open ? player : 0;
                    require(after.u32(team + national_roster_offset + i * 4) == expected,
                            "replacement takes only the first vacant slot");
                }
                ++comparisons;
            }
        }
    }
    constexpr auto relationship = fixture + 0x6e00;
    for (unsigned slot = 0; slot < 5; ++slot) {
        auto state = two_rosters(random);
        Cpu c(*state);
        c.set(4, player);
        c.put32(player + 0xc, nation);
        c.put32(player + 0x18, slot == 4 ? 0 : relationship);
        for (unsigned field = 0; field < 3; ++field)
            c.put32(relationship + 0x10 + field * 4, field == slot ? senior_team : alternate_team);
        state->pc = replacement_objection;
        auto result = compare(*state);
        require(Cpu(*result).r(3) == unsigned(slot < 3), "replacement objections use the senior national club");
        ++comparisons;
    }
    auto state = two_rosters(random);
    Cpu c(*state);
    c.set({{4, nation}, {5, 0}, {6, player}});
    c.put32(toc - 0x7b80, unmapped);
    state->pc = add_replacement;
    auto fault = compare(*state, Stop::fault);
    require(Cpu(*fault).u32(senior_team + national_roster_offset) == player &&
                Cpu(*fault).u32(player + 0x20) == senior_team,
            "replacement roster and link are written before the date fault");
    state->budget = 0;
    compare(*state, Stop::budget);
    c.set(4, unmapped);
    state->pc = replacement_objection;
    state->budget = 10000;
    compare(*state, Stop::fault);
    state->budget = 0;
    compare(*state, Stop::budget);
    return comparisons;
}

constexpr auto search_queue = fixture + 0x6d00, search_assessment = fixture + 0x6d40;
constexpr auto search_objection = fixture + 0x6d80, search_globals = fixture + 0x6dc0;
constexpr auto search_row = records + fixture_nation_id * record_size;

std::unique_ptr<State> replacement_search_seed(std::mt19937& random, unsigned mode, unsigned tier,
                                               unsigned first = 0) {
    auto state = notice_seed(random, mode, 1, automatic_replacement);
    Cpu c(*state);
    c.set({{3, manager}, {4, nation}, {5, mode}, {6, tier}, {7, player}});
    c.put32(toc - 0x7aa4, contracts_slot);
    c.put32(toc - 0x7b28, search_globals);
    c.put32(toc - 0x7bc0, search_globals + 4);
    c.put32(toc - 0x7b9c, search_globals + 8);
    c.put32(search_globals + 8, fixture + 0x2000);
    c.put32(manager + 0xc, search_queue);
    c.put32(manager + 0x10, search_queue);
    const auto group = search_row + (std::int8_t(mode) == 2 ? 0x18 : 0);
    const auto counter = search_row + std::uint32_t(std::int8_t(mode)) * 0x14 + 0x4b;
    c.put32(group + 8, first);
    c.put8(group + std::uint32_t(std::int8_t(tier)) * 4 + 0xe, 2);
    // A signed tier of -1 places its count byte inside the base word.
    auto index = c.u32(group + 8);
    for (int i = 0; i < std::int8_t(tier); ++i) {
        const auto address = group + 0xf + i * 4;
        const unsigned weight = address == counter ? 4 : 1 + i % 3;
        c.put8(address, weight);
        index += weight;
    }
    c.put32(search_queue + index * 4, player);
    c.put32(search_queue + (index + 1) * 4, replacement_candidate);
    c.put8(counter, 4);
    c.put32(player + 0x14, search_assessment);
    c.put8(search_assessment + 0xf, 15);
    c.put32(replacement_candidate, 73);
    c.put32(replacement_candidate + 0xc, nation);
    c.put32(replacement_candidate + 0x14, search_assessment);
    c.put32(replacement_candidate + 0x24, recipient_club);
    for (unsigned i = 1; i <= 3; ++i)
        c.put32(replacement_candidate + 0x24 + i * 4, c.u32(player + 0x24 + i * 4));
    c.put32(contract_answers, notice_contract);
    return state;
}

void expect_search_outcome(Cpu& c, unsigned mode, bool accepted, std::uint32_t linked_team) {
    const auto counter = search_row + std::uint32_t(std::int8_t(mode)) * 0x14 + 0x4b;
    require(c.u32(replacement_candidate + 0x20) == linked_team &&
                c.u8(counter) == (accepted ? 3 : 0) && c.u32(notice_events) == unsigned(accepted),
            "replacement outcome, remaining count and delivered news");
    if (!accepted) return;
    const auto news = stack - 0x90 - 0x140 + 0x38;
    require(c.u32(news) == 0x21 && c.u32(news + 8) == 73 && c.u32(notice_events + 4) == recipient_club,
            "replacement notice carries the chosen candidate");
}

unsigned check_replacement_dispatch(std::mt19937& random) {
    unsigned comparisons = 0;
    for (unsigned mode : {0u, 1u, 2u, 0x102u, UINT32_MAX}) {
        for (unsigned tier : {0u, 1u, 8u, 9u, 15u, 16u, 17u, 127u, 128u, 255u, 0x109u}) {
            const auto state = replacement_search_seed(random, mode, tier, 192);
            auto result = compare(*state);
            Cpu after(*result);
            const auto kind = std::int8_t(mode);
            expect_search_outcome(after, mode, true, kind == 0 || kind == 1 ? senior_team : alternate_team);
            ++comparisons;
        }
    }

    enum class Filter { objection, full, profile, injured, availability_clear, availability_hit,
                        foreign_team, dual, dual_restricted, dual_flag, dual_mismatch, reference, position };
    using enum Filter;
    for (unsigned mode = 0; mode < 3; ++mode) {
        for (const auto filter : {objection, full, profile, injured, availability_clear, availability_hit,
                                  foreign_team, dual, dual_restricted, dual_flag, dual_mismatch, reference, position}) {
            auto state = replacement_search_seed(random, mode, 0);
            Cpu c(*state);
            const auto team = mode == 2 ? alternate_team : senior_team;
            if (filter == objection) {
                c.put32(replacement_candidate + 0x18, search_objection);
                c.put32(search_objection + 0x10, senior_team);
            }
            if (filter == full) {
                for (unsigned slot = 0; slot < 50; ++slot)
                    c.put32(team + national_roster_offset + slot * 4, player);
            }
            if (filter == profile) c.put32(replacement_candidate + 0x14, 0);
            if (filter == injured) c.put8(search_policy, 1);
            if (filter == availability_clear || filter == availability_hit) {
                c.put32(search_globals, injury_system);
                c.put8(search_policy, filter == availability_hit ? 2 : 0);
            }
            if (filter == foreign_team) c.put32(replacement_candidate + 0x20, regular_club);
            if (filter == dual || filter == dual_restricted || filter == dual_flag || filter == dual_mismatch) {
                c.put32(replacement_candidate + 0xc, team_nation);
                c.put32(replacement_candidate + 0x10, filter == dual_mismatch ? team_nation : nation);
                if (filter == dual_restricted) c.put8(replacement_candidate + 0x5e, 1);
                if (filter == dual_flag) c.put8(fixture + 0x2000 + 73 * 0x64 + 0xd, 0x40);
            }
            if (filter == reference) c.put8(search_policy, 4);
            if (filter == position) {
                c.put32(replacement_candidate + 0x14, search_assessment + 0x20);
                c.put8(search_assessment + 0x30, 15);
            }
            const bool accepted = filter == full || filter == availability_clear || filter == dual ||
                (filter == availability_hit && mode == 0);
            auto result = compare(*state);
            Cpu after(*result);
            const auto linked = filter == foreign_team ? regular_club : accepted && filter != full ? team : 0;
            expect_search_outcome(after, mode, accepted, linked);
            ++comparisons;
        }
    }

    enum class Prior { inactive, same, dual, earlier_year, later_year, earlier_day, equal_day, later_day, managed };
    for (unsigned mode = 0; mode < 3; ++mode) {
        for (const auto prior : {Prior::inactive, Prior::same, Prior::dual, Prior::earlier_year, Prior::later_year,
                                 Prior::earlier_day, Prior::equal_day, Prior::later_day, Prior::managed}) {
            if (mode == 0 && prior == Prior::managed) continue;
            auto state = replacement_search_seed(random, mode, 0);
            Cpu c(*state);
            const auto old_mode = prior == Prior::same ? mode : prior == Prior::managed ? 0 : (mode + 1) % 3;
            const auto old_team = old_mode == 2 ? alternate_team : senior_team;
            const auto team = mode == 2 ? alternate_team : senior_team;
            c.put32(senior_team, 1);
            c.put32(alternate_team, 181);
            c.put32(old_team + 0x2c, nation);
            c.put32(old_team + national_roster_offset + 49 * 4, replacement_candidate);
            c.put32(replacement_candidate + 0x20, old_team);
            c.put8(replacement_candidate + 0x60, prior == Prior::inactive ? 0xc : 0xb);
            c.put8(replacement_candidate + 0x6a, old_mode == 1 ? 8 : 4);
            if (prior == Prior::dual) c.put32(replacement_candidate + 0xc, team_nation);
            c.put8(search_policy, prior == Prior::managed ? 8 : 0);
            fixture_date(c, search_row + old_mode * 0x14 + 0x40,
                         prior == Prior::equal_day ? 90 : prior == Prior::later_day ? 91 : -2,
                         prior == Prior::earlier_year ? 1999 : prior == Prior::later_year ? 2001 : 2000);
            c.put32(toc - 0x7a6c, tactics_slot);
            c.put32(tactics_slot, tactics_state);
            c.put32(toc - 0x7be4, staff_count);
            c.put32(staff_count, 80);
            c.put8(injury_active, 1);
            c.put32(tactics_state + tactics::field::teams, tactics_teams);
            const auto tactical_team = tactics_teams + c.u32(old_team) * tactics::team_size;
            if (old_mode == 2) state->memory.map(tactical_team, tactics::team_size);
            for (unsigned offset = tactics::field::squad; offset < tactics::field::lineup; offset += 4)
                c.put32(tactical_team + offset, UINT32_MAX);
            c.put32(tactical_team + tactics::field::squad, 73);
            const bool accepted = prior == Prior::earlier_year || prior == Prior::earlier_day;
            auto result = compare(*state);
            Cpu after(*result);
            expect_search_outcome(after, mode, accepted, accepted ? team : old_team);
            require(after.u32(old_team + national_roster_offset + 49 * 4) == (accepted ? 0 : replacement_candidate) &&
                        after.u32(tactical_team + tactics::field::squad) == (accepted ? UINT32_MAX : 73),
                    "expired selection releases the previous roster and tactics before inserting the replacement");
            ++comparisons;
        }
    }

    auto direct = replacement_search_seed(random, 0, 9, 192);
    Cpu unprobed(*direct);
    unprobed.put32(replacement_candidate + 0x18, search_objection);
    unprobed.put32(search_objection + 0x10, senior_team);
    auto result = compare(*direct, Stop::returned, Corrupt::none, true);
    Cpu after(*result);
    expect_search_outcome(after, 0, false, 0);
    ++comparisons;

    auto state = replacement_search_seed(random, 0, 0);
    Cpu c(*state);
    c.set(4, unmapped);
    compare(*state, Stop::fault);
    state->budget = 0;
    compare(*state, Stop::budget);

    auto partial = replacement_search_seed(random, 0, 9, 192);
    Cpu prefix(*partial);
    const auto row = fixture + 0x9000 - 24;
    prefix.put32(manager + 4, row - fixture_nation_id * record_size);
    prefix.put32(row + 8, 17);
    prefix.put8(row + 0xf, 3);
    prefix.put8(row + 0x13, 7);
    prefix.put8(row + 0x17, 11);
    auto stopped = compare(*partial, Stop::fault);
    require(Cpu(*stopped).r(4) == 27, "batch fault preserves the two weights added before the failing read");
    return comparisons;
}

unsigned check_injured_selection(std::mt19937& random) {
    constexpr auto row = records + fixture_nation_id * record_size, profile = fixture + 0x6d00;
    unsigned comparisons = 0;
    for (unsigned mode = 0; mode < 3; ++mode) {
        for (unsigned scenario = 0; scenario < 18; ++scenario) {
            auto state = drop_seed(random);
            Cpu c(*state);
            populate_notice(c, mode, 1u | (scenario == 6 || scenario == 17 ? 4u : 0u));
            c.set({{3, manager}, {4, player}, {5, scenario == 0 ? 0x1000du : scenario == 16 ? 0xffffu : 14u}});
            c.put32(toc - 0x7ba8, injury_assert_buffer);
            c.put32(toc - 0x7bac, injury_success);
            c.put32(toc - 0x5b20, source + 0x80);
            c.put32(toc - 0x7a54, calendar_slot);
            c.put8(injury_success, 1);
            c.put8(injury_active, 1);
            c.put8(injury_policy, scenario == 10 ? 2 : scenario == 11 ? 1 : scenario == 12 ? 5 : 0);
            c.put32(senior_team, 1);
            c.put32(alternate_team, 181);
            c.put32(senior_team + 0x2c, nation);
            c.put32(alternate_team + 0x2c, nation);
            const auto team = mode == 2 ? alternate_team : senior_team;
            c.put32(team + 0x90, scenario == 13 ? regular_club : 0);
            c.put32(team + 0x98, player);
            c.put32(player, 7);
            c.put32(player + 0x14, profile);
            c.put32(player + 0x20, scenario == 1 ? 0 : team);
            c.put32(player + 0x24, recipient_club);
            c.put8(player + 0x60, scenario == 2 ? 0xc : 0xb);
            c.put8(player + 0x6a, mode == 1 ? 0x8b : 0x87);
            c.put8(notice_injury, scenario == 6 ? 0x51 : 2);
            for (unsigned i = 0; i < 4; ++i) c.put32(contract_answers + i * 4, notice_contract);
            const auto slot = row + mode * 0x14;
            c.put16(row, 100);
            if (scenario % 2 == 0) c.put8(profile + 0xf, 15);
            fixture_date(c, date, 90, 2000);
            fixture_date(c, slot + 0x40, scenario == 3 ? 89 : scenario == 4 ? 90 : 91,
                         scenario == 5 ? 1999 : scenario == 14 ? 2001 : 2000);
            c.put8(slot + 0x4b, scenario == 12 ? 0xff : 9);

            if ((scenario >= 7 && scenario <= 9) || scenario == 15 || scenario == 17) {
                c.put32(manager, roster_list);
                c.put32(roster_list + 4, roster_nodes);
                link_two(c, roster_nodes, roster_rows);
                c.put32(roster_rows, 777);
                c.put32(roster_rows + 0x20, c.u32(team));
                fixture_date(c, roster_rows + 0x28, scenario == 7 ? 89 : scenario == 8 ? 90 : -2,
                             scenario == 9 ? 2001 : scenario == 15 ? 1999 : 2000);
            }

            const auto tactical_team = tactics_teams + c.u32(team) * tactics::team_size;
            if (mode == 2) state->memory.map(tactical_team, tactics::team_size);
            for (unsigned offset = tactics::field::squad; offset < tactics::field::lineup; offset += 4)
                c.put32(tactical_team + offset, UINT32_MAX);
            c.put32(tactical_team + tactics::field::squad, 7);
            state->pc = injured_selection;
            auto result = compare(*state);
            Cpu after(*result);
            const bool hold = scenario <= 5 || scenario == 7 || scenario == 15 || scenario == 16 || scenario == 17;
            const bool removed = !hold;
            const bool managed = scenario == 10 || scenario == 11 || scenario == 12;
            const bool sent_national = removed && (scenario == 12 || (managed && mode == 0));
            require(after.u32(team + 0x98) == (removed ? 0 : player) &&
                        after.u32(player + 0x20) == (removed || scenario == 1 ? 0 : team) &&
                        after.u32(tactical_team + tactics::field::squad) == (removed ? UINT32_MAX : 7),
                    "injury roster and tactical state");
            require(after.u8(injury_success) == unsigned(scenario != 1) &&
                        after.u32(notice_events) == (removed && scenario != 6 ? 1u + sent_national : 0),
                    "injury assertion and notice count");
            const unsigned expected_count = sent_national ? (scenario == 12 ? 0 : 10) :
                removed && scenario != 6 ? 0 : scenario == 12 ? 0xff : 9;
            require(after.u8(slot + 0x4b) == expected_count, "injury replacement counter and wrapping");
            ++comparisons;
        }
    }
    auto state = drop_seed(random);
    state->memory.map(stack - 0x800, 0x400);
    Cpu real(*state);
    populate_notice(real, 0, 0);
    real.set({{3, manager}, {4, player}, {5, 14}});
    real.put32(senior_team, 1);
    real.put32(senior_team + 0x2c, nation);
    real.put32(senior_team + 0x98, player);
    real.put32(player, 7);
    real.put32(player + 0x14, 0);
    real.put32(player + 0x20, senior_team);
    real.put8(player + 0x60, 0xb);
    real.put32(toc - 0x5450, fixture + 0x6f40);
    real.put32(toc - 0x544c, fixture + 0x6f48);
    real.put32(toc - 0x5448, fixture + 0x6f50);
    real.put32(injury_system, fixture + 0x6e00);
    real.put32(injury_system + 4, notice_injury);
    real.put32(fixture + 0x6e00 + 7 * 0x24 + 0x10, 0);
    real.put8(notice_injury, 0x51);
    for (unsigned offset = tactics::field::squad; offset < tactics::field::lineup; offset += 4)
        real.put32(tactics_teams + tactics::team_size + offset, UINT32_MAX);
    real.put32(tactics_teams + tactics::team_size + tactics::field::squad, 7);
    state->pc = injured_selection;
    auto direct = compare(*state, Stop::returned, Corrupt::none, true);
    require(Cpu(*direct).u32(player + 0x20) == 0 &&
                Cpu(*direct).u32(tactics_teams + tactics::team_size + tactics::field::squad) == UINT32_MAX,
            "real injury lookup and direct tactical roster removal");
    ++comparisons;

    auto fault = drop_seed(random);
    Cpu c(*fault);
    c.set({{4, unmapped}, {5, 14}});
    fault->pc = injured_selection;
    compare(*fault, Stop::fault);
    fault->budget = 0;
    compare(*fault, Stop::budget);
    return comparisons;
}

unsigned check_roster_faults(std::mt19937& random) {
    for (const auto entry : {remove_player, selected_player}) {
        auto state = duty_seed(random);
        Cpu(*state).set(4, unmapped);
        state->pc = entry;
        compare(*state, Stop::fault);
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    auto state = duty_seed(random);
    Cpu c(*state);
    c.set({{3, manager}, {4, player}});
    c.put32(player + 0xc, nation);
    c.put32(player + 0x14, fixture + 0x6400);
    c.put16(fixture + 0x6408, 1);
    c.put8(records + 0xe, 1);
    c.put32(manager + 0xc, fixture + 0x8ffc);
    c.put32(fixture + 0x8ffc, player);
    c.put32(records + 8, 1);
    state->pc = remove_player;
    compare(*state, Stop::fault);
    // A match decrements the queue length before reading the replacement.
    c.put32(records + 8, 0);
    c.put32(manager + 0xc, fixture + 0x8ffc);
    c.put8(records + 0xe, 2);
    // The first entry matches, then the replacement reads the unmapped end.
    compare(*state, Stop::fault);
    return 4;
}

std::unique_ptr<State> calendar_seed(std::mt19937& random) {
    auto state = duty_seed(random);
    Cpu c(*state);
    c.set({{3, calendar}, {4, calendar_match}});
    c.put32(toc - 0x7b54, competition_slot);
    c.put32(toc - 0x7b64, competition_count);
    c.put32(competition_slot, competition_manager);
    c.put32(competition_count, 200);
    c.put32(competition, 0);
    c.put32(calendar + 0x20, calendar_headers);
    c.put32(calendar_match + 0x14, competition);
    c.put32(calendar_match + 0x1c, regular_club);
    c.put32(calendar_match + 0x20, national_team);
    c.put32(regular_club, 0);
    c.put32(national_team, 1);
    c.put16(calendar_match + 0x28, 0x8001);
    c.put16(calendar_match + 0x2a, -7);
    c.put32(calendar_headers, home_events);
    c.put32(calendar_headers + 8, away_events);
    fixture_date(c, first_bound, -9, 2000);
    fixture_date(c, second_bound, 9, 2000);
    state->pc = calendar_availability;
    return state;
}

struct CalendarEvent { int day; unsigned year, weight; };

void calendar_events(Cpu& c, bool away, std::initializer_list<CalendarEvent> events) {
    auto row = away ? away_events : home_events;
    c.put8(calendar_headers + (away ? 8 : 0) + 5, events.size());
    for (const auto& event : events) {
        fixture_date(c, row, event.day, event.year);
        c.put8(row + 8, event.weight);
        row += 12;
    }
}

unsigned check_calendar_availability(std::mt19937& random) {
    unsigned comparisons = 0;
    for (bool away : {false, true}) {
        for (unsigned scenario = 0; scenario < 17; ++scenario) {
            auto state = calendar_seed(random);
            Cpu c(*state);
            const auto side = calendar_match + (away ? 0x20 : 0x1c);
            const auto other = calendar_match + (away ? 0x1c : 0x20);
            const auto header = calendar_headers + (away ? 8 : 0);
            c.put32(other, 0);
            switch (scenario) {
            case 0: c.put32(side, 0); break;
            case 1: c.put32(c.u32(side), 160); break;
            case 2: c.put8(header + 5, 0x80); break;
            case 3: break; // Empty schedule.
            case 4: calendar_events(c, away, {{-9, 2000, 1}, {9, 2000, 1}}); break;
            case 5: calendar_events(c, away, {{-9, 2000, 1}, {9, 2000, 2}}); break;
            case 6: calendar_events(c, away, {{366, 1999, 9}, {-10, 2000, 9}, {0, 2000, 2}, {10, 2000, 9}}); break;
            case 7: calendar_events(c, away, {{0, 2000, 2}, {1, 2001, 9}}); break;
            case 8: calendar_events(c, away, {{0, 2000, 127}}); break;
            case 9: calendar_events(c, away, {{0, 2000, 128}}); break;
            case 10: calendar_events(c, away, {{0, 2000, 255}, {1, 2000, 4}}); break;
            case 11: calendar_events(c, away, {{0, 2000, 255}, {1, 2000, 3}}); break;
            case 12:
                fixture_date(c, first_bound, 300, 1999);
                fixture_date(c, second_bound, -9, 2001);
                calendar_events(c, away, {{0, 2000, 3}});
                break;
            case 13:
                c.put32(competition_manager + 4, competition);
                c.put32(side, unmapped);
                break;
            case 14:
                calendar_events(c, away, {{0, 2000, 4}, {10, 2000, 252}});
                break;
            case 15:
                calendar_events(c, away, {{-11, 2000, 4}, {-10, 2000, 252}});
                break;
            case 16:
                calendar_events(c, away, {{9, 2000, 2}, {10, 2000, 9}, {8, 2000, 9}});
                break;
            }
            auto result = compare(*state);
            const auto allowed = scenario != 5 && scenario != 8 && scenario != 10 && scenario != 12 && scenario != 14;
            require(Cpu(*result).r(3) == unsigned(allowed), "calendar window, ordering and signed weight threshold");
            ++comparisons;
        }
    }
    for (unsigned home : {0u, 2u, 3u}) {
        for (unsigned away : {0u, 2u, 3u}) {
            auto state = calendar_seed(random);
            Cpu c(*state);
            calendar_events(c, false, {{0, 2000, home}});
            calendar_events(c, true, {{0, 2000, away}});
            auto result = compare(*state);
            require(Cpu(*result).r(3) == unsigned(home <= 2 && away <= 2), "both club calendars are checked independently");
            ++comparisons;
        }
    }
    auto real = calendar_seed(random);
    Cpu c(*real);
    c.put32(toc - 0x3fb4, fixture + 0x6f20);
    c.put32(toc - 0x3fac, fixture + 0x6f28);
    c.put16(calendar_match + 0x28, 2000);
    c.put16(calendar_match + 0x2a, 90);
    calendar_events(c, false, {{90, 2000, 3}});
    auto result = compare(*real, Stop::returned, Corrupt::none, true);
    require(Cpu(*result).r(3) == 0, "real date helpers find a same-day conflict");
    ++comparisons;
    for (bool away : {false, true}) {
        auto state = calendar_seed(random);
        Cpu bad(*state);
        const auto header = calendar_headers + (away ? 8 : 0);
        bad.put32(header, unmapped);
        bad.put8(header + 5, 1);
        compare(*state, Stop::fault);
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    return comparisons;
}

std::unique_ptr<State> calendar_save_seed(std::mt19937& random, bool empty) {
    auto state = duty_seed(random);
    Cpu c(*state);
    c.set({{3, calendar}, {4, archive_path}});
    c.put32(toc - 0x7be0, staff_slot);
    c.put32(toc - 0x7bac, injury_success);
    c.put32(toc - 0x7ba8, injury_assert_buffer);
    c.put32(toc - 0x5b20, source + 0x80);
    c.put8(injury_success, 1);
    c.put32(nation_count, empty ? 0 : 2);
    c.put32(club_count, empty ? 0 : 6);
    c.put32(calendar + 4, records);
    c.put32(calendar + 8, archive_years);
    c.put32(calendar + 0xc, archive_primary);
    c.put32(calendar + 0x10, archive_secondary);
    c.put32(calendar + 0x14, empty ? 0 : 3);
    c.put32(calendar + 0x18, empty ? 0 : 2);
    c.put32(calendar + 0x1c, archive_companion);
    c.put32(calendar + 0x20, calendar_headers);
    c.put32(calendar + 0x38, archive_flags);
    c.put32(calendar, empty ? 0 : roster_list);
    c.put16(roster_list, 2);
    c.put32(roster_list + 4, roster_nodes);
    link_two(c, roster_nodes, roster_rows);
    c.put32(staff_table, 0);
    c.put32(staff_table + 0x70, 1);
    c.put32(staff_table + 0xe0, 2);
    c.put32(archive_primary, staff_table + 0xe0);
    c.put32(archive_primary + 4, 0);
    c.put32(archive_primary + 8, staff_table);
    c.put32(archive_secondary, 0);
    c.put32(archive_secondary + 4, staff_table + 0x70);
    c.put32(calendar_headers, home_events);
    c.put8(calendar_headers + 5, 2);
    c.put32(calendar_headers + 8, away_events);
    c.put32(archive_years, 0x10203040);
    c.put32(archive_years + 4, 0x50607080);
    c.put16(archive_flags, 0x4281);
    for (unsigned i = 0; i < 2 * record_size; ++i) c.put8(records + i, random());
    for (unsigned i = 0; i < 2 * 0x14; ++i) c.put8(roster_rows + i + (i >= 0x14 ? 12 : 0), random());
    for (unsigned i = 0; i < 24; ++i) c.put8(home_events + i, random());
    state->pc = calendar_save;
    return state;
}

unsigned check_calendar_save(std::mt19937& random) {
    unsigned comparisons = 0;
    for (const bool empty : {false, true}) {
        const unsigned cases = empty ? 1 : 14;
        for (unsigned scenario = 0; scenario < cases; ++scenario) {
            auto state = calendar_save_seed(random, empty);
            Cpu c(*state);
            const bool open_failure = scenario == 13;
            const unsigned failure = open_failure ? 0xff : scenario;
            c.put8(archive_trace, failure);
            using Bytes = std::vector<std::uint8_t>;
            struct Write { Bytes bytes; unsigned width; };
            std::vector<Write> writes;
            const auto raw = [&](std::uint32_t address, unsigned width, unsigned count) {
                writes.push_back({state->memory.copy_out(address, width * count), width});
            };
            raw(records, record_size, empty ? 0 : 2);
            raw(calendar + 0x14, 4, 1);
            writes.push_back({empty ? Bytes{} : Bytes{0,0,0,2, 255,255,255,255, 0,0,0,0}, 4});
            raw(calendar + 0x18, 4, 1);
            writes.push_back({empty ? Bytes{} : Bytes{255,255,255,255, 0,0,0,1}, 4});
            writes.push_back({{0, std::uint8_t(empty ? 0 : 2)}, 2});
            if (!empty) {
                raw(roster_rows, 0x14, 1);
                raw(roster_rows + 0x20, 0x14, 1);
            }
            raw(calendar_headers, 8, empty ? 0 : 2);
            if (!empty) raw(home_events, 12, 2);
            raw(archive_years, 4, empty ? 0 : 2);
            raw(archive_flags, 1, empty ? 0 : 2);
            Bytes expected;
            const auto reached = open_failure ? 0 : failure ? failure : writes.size();
            for (unsigned i = 0; i < reached; ++i) {
                const auto& write = writes[i];
                const auto last = failure == i + 1 ? write.bytes.end() - write.width : write.bytes.end();
                expected.insert(expected.end(), write.bytes.begin(), last);
            }
            auto result = compare(*state);
            Cpu after(*result);
            require(after.u32(archive_trace + 4) == reached && after.u32(archive_trace + 8) == expected.size() &&
                        result->memory.copy_out(archive_bytes, expected.size()) == expected,
                    "calendar archive byte order and completed prefix");
            require(after.u8(injury_success) == unsigned(!failure) &&
                        after.u8(archive_trace + 0xc) == unsigned(!failure) &&
                        after.u8(archive_trace + 0xd) == unsigned(!failure) &&
                        after.u32(archive_trace + 0x10) ==
                            (open_failure ? archive_error_lines.front() : failure ? archive_error_lines[failure] : 0),
                    "calendar archive failure line, close and companion ordering");
            require(after.u32(archive_primary) == (failure == 3 ? 2 : staff_table + 0xe0) &&
                        after.u32(archive_primary + 4) == (failure == 3 ? UINT32_MAX : 0) &&
                        after.u32(archive_secondary + 4) == (failure == 5 ? 1 : staff_table + 0x70),
                    "calendar archive keeps packed IDs only for a failed queue write");
            ++comparisons;
        }
    }
    auto state = calendar_save_seed(random, false);
    Cpu c(*state);
    c.put32(archive_primary + 4, unmapped);
    auto fault = compare(*state, Stop::fault);
    require(Cpu(*fault).u32(archive_primary) == 2 && Cpu(*fault).u32(archive_primary + 4) == unmapped,
            "calendar archive leaves only the preceding ID converted on a pointer fault");
    state->budget = 0;
    compare(*state, Stop::budget);
    return comparisons;
}

unsigned check_calendar_load(std::mt19937& random) {
    constexpr unsigned headers_start = 302, years_start = 342, flags_start = 350;
    constexpr std::array read_errors{0u, 0x1f84u, 0x1f8bu, 0x1f9bu, 0x1fb3u, 0x1fc3u,
                                     0x1fdbu, 0x1fedu, 0x1fedu, 0u, 0x200du, 0u, 0u};
    constexpr std::array allocation_errors{0u, 0x1f76u, 0u, 0u, 0x1f93u, 0x1fbbu, 0u, 0x2006u};
    auto writer = calendar_save_seed(random, false);
    const auto saved = compare(*writer);
    const auto bytes = saved->memory.copy_out(archive_bytes, Cpu(*saved).u32(archive_trace + 8));
    require(bytes.size() == flags_start + 2, "calendar populated archive size");

    const auto loader = [&](unsigned length, unsigned read_failure = 0, unsigned alloc_failure = 0) {
        auto state = clone(*writer);
        state->pc = calendar_load;
        state->memory.map(archive_arena, 0x4000);
        state->memory.copy_in(archive_bytes, bytes);
        Cpu c(*state);
        c.set({{3, calendar}, {4, archive_path}});
        c.put32(calendar, 0);
        c.put64(calendar_headers, 0);
        c.put64(calendar_headers + 8, 0);
        c.put8(archive_trace, read_failure);
        c.put32(archive_trace + 0x14, length);
        c.put8(archive_trace + 0x18, alloc_failure);
        return state;
    };
    unsigned comparisons = 0;
    const auto run = [&](const State& state, unsigned error_line, bool opened = true) {
        auto after = compare(state);
        Cpu c(*after);
        const bool success = opened && !error_line;
        require(c.r(3) == unsigned(success) && c.u8(injury_success) == unsigned(!error_line) &&
                    c.u8(archive_trace + 0xc) == unsigned(success) && !c.u8(archive_trace + 0xd) &&
                    c.u32(archive_trace + 0x10) == error_line,
                "calendar load return, assertion and close ordering");
        ++comparisons;
        return after;
    };

    for (const unsigned length : {unsigned(bytes.size()), flags_start + 1, flags_start,
                                   years_start + 4, years_start, headers_start + 8, headers_start}) {
        const auto state = loader(length);
        const auto after = run(*state, 0);
        Cpu c(*after);
        require(after->memory.copy_out(c.u32(calendar + 4), 2 * record_size) ==
                    writer->memory.copy_out(records, 2 * record_size), "calendar nation records restored");
        const auto primary = c.u32(calendar + 0xc), secondary = c.u32(calendar + 0x10);
        require(c.u32(calendar + 0x14) == 3 && c.u32(calendar + 0x18) == 2 &&
                    c.u32(primary) == staff_table + 0xe0 && !c.u32(primary + 4) &&
                    c.u32(primary + 8) == staff_table && !c.u32(secondary) &&
                    c.u32(secondary + 4) == staff_table + 0x70, "calendar queue IDs restored as staff pointers");
        const auto list = c.u32(calendar);
        require(c.u16(list) == 2 && c.u32(list + 4) == archive_nodes &&
                    c.u32(list + 8) == archive_nodes + 0x40 && !c.u32(list + 12),
                "calendar list construction and append order");
        for (unsigned i = 0; i < 2; ++i) {
            const auto node = archive_nodes + i * 0x40;
            require(after->memory.copy_out(c.u32(node), 0x14) ==
                        writer->memory.copy_out(roster_rows + i * 0x20, 0x14), "calendar list row restored");
        }
        if (length >= years_start) {
            require(c.u8(calendar_headers + 4) == 2 && c.u8(calendar_headers + 5) == 2 &&
                        after->memory.copy_out(c.u32(calendar_headers), 24) ==
                            writer->memory.copy_out(home_events, 24), "calendar club event rows restored");
        } else {
            require(!c.u8(calendar_headers + 4) && c.u32(archive_trace + 0x1c) == 6,
                    "calendar short club headers skip row allocations");
        }
        const auto years = c.u32(calendar + 8), flags = c.u32(calendar + 0x38);
        require(c.u32(years) == (length >= flags_start ? 0x10203040u : UINT32_MAX) &&
                    c.u32(years + 4) == (length >= flags_start ? 0x50607080u : UINT32_MAX) &&
                    c.u8(flags) == (length > flags_start ? 0x42u : 0u) &&
                    c.u8(flags + 1) == (length > flags_start + 1 ? 0x81u : 0u),
                "calendar load trailer defaults and partial flags");
    }

    for (unsigned call = 1; call < read_errors.size(); ++call) {
        if (read_errors[call]) run(*loader(bytes.size(), call), read_errors[call]);
    }
    for (unsigned call = 1; call < allocation_errors.size(); ++call) {
        if (allocation_errors[call]) run(*loader(bytes.size(), 0, call), allocation_errors[call]);
    }
    run(*loader(bytes.size(), 0xff), 0x1f6d, false);
    auto state = loader(bytes.size());
    Cpu(*state).set(4, 0);
    run(*state, 0, false);
    for (const unsigned call : {2, 3}) {
        state = loader(bytes.size(), 0, call);
        compare(*state, Stop::fault);
    }
    state = loader(bytes.size());
    state->budget = 0;
    compare(*state, Stop::budget);

    constexpr unsigned list_rows_start = headers_start - 2 * 0x14;
    auto negative_count = bytes;
    negative_count[list_rows_start - 2] = 0xff;
    negative_count[list_rows_start - 1] = 0xff;
    negative_count.erase(negative_count.begin() + list_rows_start, negative_count.begin() + headers_start);
    for (const unsigned failed_allocation : {0u, 6u}) {
        state = loader(negative_count.size(), 0, failed_allocation);
        state->memory.copy_in(archive_bytes, negative_count);
        const auto loaded = run(*state, 0);
        Cpu c(*loaded);
        const auto list = c.u32(calendar);
        require((failed_allocation ? !list : list == archive_arena + 5 * 0x400 && !c.u16(list)) &&
                    c.u32(archive_trace + 4) == 10 && c.u32(archive_trace + 8) == negative_count.size(),
                "negative list count skips appends even when list allocation fails");
    }

    const auto empty_writer = calendar_save_seed(random, true);
    const auto empty = compare(*empty_writer);
    const auto empty_bytes = empty->memory.copy_out(archive_bytes, Cpu(*empty).u32(archive_trace + 8));
    require(empty_bytes.size() == 10, "calendar empty archive size");
    state = loader(empty_bytes.size());
    state->memory.copy_in(archive_bytes, empty_bytes);
    Cpu c(*state);
    c.put32(nation_count, 0);
    c.put32(club_count, 0);
    c.put32(calendar, roster_list);
    const auto loaded_empty = run(*state, 0);
    Cpu result(*loaded_empty);
    require(result.u32(calendar) == roster_list && result.u32(archive_trace + 0x1c) == 5 &&
                result.u32(archive_trace + 8) == empty_bytes.size(), "empty load preserves an existing list");
    return comparisons;
}

std::unique_ptr<State> domestic_schedule_seed(std::mt19937& random) {
    auto state = calendar_save_seed(random, false);
    state->memory.map(release_team, 0x1000);
    Cpu c(*state);
    c.put32(club_count, 7);
    c.put32(calendar + 0x20, release_headers);
    c.put32(toc - 0x7b68, release_excluded_nation);
    c.put32(toc - 0x7be4, release_limit);
    c.put32(toc - 0x514c, 0x61616161);
    c.put32(toc - 0x5148, 0x61515151);
    c.put32(release_excluded_nation, 4);
    c.put32(release_limit, 100);
    c.put32(release_nation + 0x68, 3);
    c.put8(release_nation + 0x120, 1);
    c.put32(release_clubs, 0);
    c.put32(release_clubs + 0x2c, release_nation);
    c.put32(release_clubs + 0x100, 1);
    c.put32(release_clubs + 0x12c, release_nation);
    for (unsigned i = 0; i < 4; ++i) {
        const auto at = release_players + 0x80 * i;
        c.put32(at, i);
        c.put32(at + 0x24, i < 2 ? release_clubs : i == 2 ? release_clubs + 0x100 : 0);
        c.put32(release_team + 0x98 + 4 * i, at);
    }
    c.put16(calendar_match + 0x28, 2000);
    c.put16(calendar_match + 0x2a, -7);
    c.set({{3, calendar}, {4, release_team}, {5, 0}, {6, calendar_match}});
    state->pc = register_fixture;
    return state;
}

unsigned check_domestic_schedule(std::mt19937& random) {
    unsigned comparisons = 0;
    for (bool failure : {false, true}) {
        const auto state = domestic_schedule_seed(random);
        Cpu c(*state);
        c.put32(calendar + 0x20, home_events);
        c.put8(release_policy, failure);
        state->pc = create_schedule;
        auto after = compare(*state);
        Cpu result(*after);
        require(result.u32(calendar + 0x20) == (failure ? 0 : release_headers) &&
                    result.u8(injury_success) == !failure &&
                    result.u32(archive_trace + 0x10) == (failure ? 0x2156u : 0u),
                "domestic schedule publishes the cleared header array or reports failure");
        ++comparisons;
    }
    for (unsigned scenario = 0; scenario < 17; ++scenario) {
        auto state = domestic_schedule_seed(random);
        Cpu c(*state);
        const auto set_row = [&](unsigned i, int day, unsigned year, unsigned releases) {
            const auto row = release_rows + i * 12;
            fixture_date(c, row, day, year);
            c.put8(row + 8, releases);
        };
        // Give the second club room to append independently of the first.
        c.put32(release_headers + 8, away_events);
        c.put8(release_headers + 12, 3);
        switch (scenario) {
        case 0: c.set(6, 0); break;
        case 1: break;
        case 2: case 3:
            c.put32(release_headers, release_rows);
            c.put8(release_headers + 4, 3);
            c.put8(release_headers + 5, 3);
            set_row(0, -7, 1999, 0x11);
            set_row(1, -8, 2000, 0x22);
            set_row(2, -7, 2000, 0xff);
            if (scenario == 3) c.put8(release_policy + 1, 1);
            break;
        case 4: c.put8(release_policy, 1); break;
        case 5: c.set(5, 0x180); c.put8(release_players + 0x6a, 4); break;
        case 6: c.put8(release_players + 0x6a, 8); break;
        case 7: c.put32(release_players, 100); break;
        case 8: c.put32(release_clubs, 3); break;
        case 9: c.put32(release_clubs, UINT32_MAX); break;
        case 10: c.put32(release_clubs + 0x2c, 0); break;
        case 11: c.put8(release_nation + 0x120, 0); break;
        case 12: c.put32(release_nation + 0x68, 4); break;
        case 13: c.put32(release_team + 0x98 + 3 * 4, 0); break;
        case 14: case 15:
            c.put32(release_headers, release_rows);
            c.put8(release_headers + 4, scenario == 14 ? 3 : 1);
            c.put8(release_headers + 5, 1);
            set_row(0, -5, 2000, 0x11);
            break;
        case 16:
            c.put32(release_team + 0x98, release_players + 0x180);
            for (unsigned i = 1; i <= 3; ++i) c.put32(release_team + 0x98 + 4 * i, 0);
            break;
        }
        auto after = compare(*state);
        Cpu result(*after);
        const bool failed = scenario == 4;
        const bool existing_row = scenario == 14 || scenario == 15;
        const bool append = scenario == 1 || (scenario >= 5 && scenario <= 7) || scenario == 13 || existing_row;
        const bool away_append = scenario != 0 && !failed && scenario != 11 && scenario != 12 &&
                                 scenario != 13 && scenario != 16;
        const unsigned expected_size = append ? (existing_row ? 2 : 1) :
                                       (scenario == 2 || scenario == 3 ? 3 : 0);
        require(result.u8(release_policy + 2) && result.u8(injury_success) == !failed &&
                    result.u32(archive_trace + 0x10) == (failed ? 0x21bbu : 0u) &&
                    result.u8(release_headers + 5) == expected_size &&
                    result.u8(release_headers + 13) == away_append &&
                    result.u8(release_policy + 3) == unsigned(append) + unsigned(away_append),
                "domestic schedule sorts roster first, then each appended date");
        if (failed)
            require(result.u8(release_headers + 4) == 3 && !result.u32(release_headers),
                    "domestic schedule preserves capacity and cleared pointer on failed growth");
        if (append) {
            const auto row = result.u32(release_headers) + (existing_row ? 12 : 0);
            require(result.i16(row) == (scenario == 5 ? -8 : -7) &&
                        result.u16(row + 2) == 2000 && result.u8(row + 4) &&
                        result.u8(row + 8) == (scenario >= 5 && scenario <= 7 ? 1u : 2u),
                    "domestic schedule publishes date and the eligible club count");
        }
        if (scenario == 15)
            require(result.u32(release_headers) == grown_rows && result.u8(release_headers + 4) == 4 &&
                        after->memory.copy_out(grown_rows, 12) == state->memory.copy_out(release_rows, 12),
                    "successful growth keeps the previous date row");
        if (scenario == 2 || scenario == 3)
            require(result.u8(release_rows + 20) == (scenario == 3 ? 0x24u : 0x22u) &&
                        result.u8(release_rows + 32) == (scenario == 2 ? 1u : 0xffu),
                    "domestic schedule increments only the exact day and year with byte wrapping");
        ++comparisons;
    }
    {
        const auto state = domestic_schedule_seed(random);
        Cpu c(*state);
        c.put8(release_policy + 4, 1);
        c.put32(toc - 0x5148, roster_comparator);
        c.put32(toc - 0x514c, date_comparator);
        c.put32(roster_comparator, club_order);
        c.put32(roster_comparator + 4, toc);
        c.put32(date_comparator, date_order);
        c.put32(date_comparator + 4, toc);
        c.put32(release_team + 0x98, release_players + 0x100);
        c.put32(release_team + 0x9c, release_players + 0x180);
        c.put32(release_team + 0xa0, release_players);
        c.put32(release_team + 0xa4, release_players + 0x80);
        c.put32(release_headers, release_rows);
        c.put8(release_headers + 4, 3);
        c.put8(release_headers + 5, 1);
        fixture_date(c, release_rows, 100, 2000);
        c.put8(release_rows + 8, 17);
        c.put16(calendar_match + 0x2a, 90);
        const auto after = compare(*state, Stop::returned, Corrupt::none, false, {club_order, date_order});
        Cpu result(*after);
        require(result.u32(release_team + 0xa0) == release_players + 0x100 &&
                    result.u8(release_headers + 5) == 2 && result.i16(release_rows) == 90 &&
                    result.u8(release_rows + 8) == 2 && result.i16(release_rows + 12) == 100 &&
                    result.u8(release_rows + 20) == 17 && result.u8(release_headers + 13) == 1,
                "real comparators group roster players and order date rows");
        ++comparisons;
    }
    const auto fault = domestic_schedule_seed(random);
    Cpu(*fault).put32(calendar + 0x20, unmapped);
    compare(*fault, Stop::fault);
    for (const auto entry : {create_schedule, register_fixture}) {
        const auto budget = domestic_schedule_seed(random);
        budget->pc = entry;
        budget->budget = 0;
        compare(*budget, Stop::budget);
    }
    return comparisons;
}

unsigned check_schedule_comparators(std::mt19937& random) {
    constexpr auto left = fixture + 0x2000, right = left + 0x20;
    constexpr auto left_player = fixture + 0x2100, right_player = fixture + 0x2200;
    constexpr auto left_club = fixture + 0x2300, right_club = fixture + 0x2400;
    constexpr auto left_assessment = fixture + 0x2500, right_assessment = fixture + 0x2600;
    unsigned comparisons = 0;
    const auto prepare = [&](std::uint32_t entry, std::uint32_t a = left, std::uint32_t b = right) {
        auto state = seed(random);
        Cpu c(*state);
        c.set({{3, a}, {4, b}});
        state->pc = entry;
        return state;
    };
    const auto result_is = [&](State& state, std::int32_t expected) {
        const auto after = compare(state);
        require(std::int32_t(Cpu(*after).r(3)) == expected, "national schedule comparator result");
        ++comparisons;
    };

    constexpr std::array dates{std::pair{0x7fffu, 32767}, std::pair{0x8000u, -32768},
                               std::pair{0x8000u, -1}, std::pair{0x8000u, 0},
                               std::pair{0x8000u, 1}, std::pair{0x8000u, 32767},
                               std::pair{0xffffu, -32768}};
    for (const auto& a : dates) {
        for (const auto& b : dates) {
            const auto state = prepare(date_order);
            Cpu c(*state);
            fixture_date(c, left, a.second, a.first);
            fixture_date(c, right, b.second, b.first);
            c.put32(right + 4, random());
            result_is(*state, (a > b) - (a < b));
        }
    }
    for (const auto& [a, b] : {std::pair{stack - 8, right}, std::pair{stack - 16, right},
                              std::pair{left, stack - 8}, std::pair{left, left}}) {
        const auto state = prepare(date_order, a, b);
        Cpu c(*state);
        fixture_date(c, a, 30, 2000);
        fixture_date(c, b, b == stack - 8 ? 40 : 20, 2000);
        result_is(*state, b == stack - 8 ? -1 : 0);
    }

    const auto seed_roster = [&](std::uint32_t entry) {
        auto state = prepare(entry);
        Cpu c(*state);
        c.put32(left, left_player);
        c.put32(right, right_player);
        c.put32(left_player + 0x24, left_club);
        c.put32(right_player + 0x24, right_club);
        c.put32(left_club, UINT32_MAX);
        c.put32(right_club, 6);
        c.put32(left_player + 0x14, left_assessment);
        c.put32(right_player + 0x14, right_assessment);
        return state;
    };
    for (const int a : {-32768, -2, 0, 32767}) {
        for (const int b : {-32768, -1, 0, 32767}) {
            const auto state = seed_roster(score_order);
            Cpu c(*state);
            c.put16(left_assessment + 8, a);
            c.put16(right_assessment + 8, b);
            result_is(*state, b - a);
        }
    }
    for (unsigned a = 0; a < 3; ++a) {
        for (unsigned b = 0; b < 3; ++b) {
            const auto state = seed_roster(club_order);
            Cpu c(*state);
            if (a == 0) c.put32(left, 0);
            if (a == 1) c.put32(left_player + 0x24, 0);
            if (b == 0) c.put32(right, 0);
            if (b == 1) c.put32(right_player + 0x24, 0);
            const auto expected = a == 2 && b == 2 ? -7 : int(b == 2) - int(a == 2);
            result_is(*state, expected);
        }
    }
    for (const unsigned a : {0x80u, 4u, 8u, 12u}) {
        for (const unsigned b : {0x80u, 4u, 8u, 12u}) {
            const auto state = seed_roster(club_order);
            Cpu c(*state);
            c.put32(right_player + 0x24, left_club);
            c.put8(left_player + 0x6a, a);
            c.put8(right_player + 0x6a, b);
            const auto expected = (a & 4) && (b & 8) ? -1 : (a & 8) && (b & 4) ? 1 : 0;
            result_is(*state, expected);
        }
    }
    {
        const auto state = seed_roster(club_order);
        Cpu(*state).put32(left_club, 6);
        result_is(*state, 0);
        Cpu(*state).put32(left_club, 0x80000000);
        Cpu(*state).put32(right_club, 1);
        result_is(*state, INT32_MAX);
    }

    {
        constexpr auto descriptor = fixture + 0x2700;
        const auto state = seed_roster(0x10768300);
        Cpu c(*state);
        c.put32(left + 4, right_player);
        c.put32(descriptor, score_order);
        c.put32(descriptor + 4, toc);
        c.put16(left_assessment + 8, -32768);
        c.put16(right_assessment + 8, 32767);
        c.set({{3, left}, {4, 2}, {5, 4}, {6, descriptor}});
        const auto after = compare(*state, Stop::returned, Corrupt::none, false, {score_order});
        Cpu result(*after);
        require(result.u32(left) == right_player && result.u32(left + 4) == left_player,
                "real score sort orders signed assessments highest first");
        ++comparisons;
    }

    auto fault = prepare(date_order, unmapped, right);
    compare(*fault, Stop::fault);
    fault = prepare(date_order, left, fixture + 0x4ffc);
    compare(*fault, Stop::fault);
    fault = seed_roster(score_order);
    Cpu(*fault).put32(right_player + 0x14, unmapped);
    compare(*fault, Stop::fault);
    fault = seed_roster(club_order);
    Cpu(*fault).put32(left_player + 0x24, unmapped);
    compare(*fault, Stop::fault);
    for (const auto entry : {date_order, score_order, club_order}) {
        auto budget = prepare(entry);
        budget->budget = 0;
        compare(*budget, Stop::budget);
    }
    return comparisons;
}

unsigned check_schedule_expiry(std::mt19937& random) {
    const auto prepare = [&] {
        auto state = domestic_schedule_seed(random);
        Cpu c(*state);
        c.put32(toc - 0x7b80, date);
        fixture_date(c, date, -1, 0x8000);
        c.put32(date + 4, 0x12345678);
        c.set(3, calendar);
        state->pc = expire_schedule;
        return state;
    };
    const auto header = [](Cpu& c, unsigned club, std::uint32_t rows, unsigned count) {
        const auto at = release_headers + 8 * club;
        c.put32(at, rows);
        c.put8(at + 4, 7);
        c.put8(at + 5, count);
        c.put16(at + 6, 0x55aa);
    };
    const auto row = [](Cpu& c, std::uint32_t at, int day, unsigned year, unsigned weight) {
        fixture_date(c, at, day, year);
        c.put8(at + 8, weight);
        c.put8(at + 7, weight ^ 0x80);
        c.put8(at + 11, weight ^ 0x40);
    };
    unsigned comparisons = 0;
    {
        const auto state = prepare();
        Cpu c(*state);
        header(c, 0, release_rows, 5);
        header(c, 1, away_events, 2);
        header(c, 2, 0, 0);
        row(c, release_rows, 32767, 0x7fff, 1);
        row(c, release_rows + 12, -2, 0x8000, 2);
        row(c, release_rows + 24, -1, 0x8000, 3);
        row(c, release_rows + 36, 4, 0x8000, 4);
        row(c, release_rows + 48, -32768, 0x8001, 5);
        row(c, away_events, 0, 0x7fff, 9);
        row(c, away_events + 12, -4, 0x8000, 10);

        const auto after = compare(*state);
        Cpu result(*after);
        require(result.u32(release_headers) == release_rows && result.u8(release_headers + 4) == 7 &&
                    result.u8(release_headers + 5) == 3 && result.u16(release_headers + 6) == 0x55aa,
                "expired prefix removal retains the live schedule allocation");
        for (unsigned i = 0; i < 3; ++i) {
            const auto target = release_rows + i * 12, source = target + 24;
            require(result.u32(target) == c.u32(source) && result.u8(target + 4) == c.u8(source + 4) &&
                        result.u8(target + 8) == c.u8(source + 8) &&
                        result.u8(target + 7) == c.u8(target + 7) &&
                        result.u8(target + 11) == c.u8(target + 11),
                    "schedule expiry retains today and later dates with destination padding");
        }
        require(!result.u32(release_headers + 8) && !result.u8(release_headers + 12) &&
                    !result.u8(release_headers + 13) && result.u16(release_headers + 14) == 0x55aa &&
                    result.u8(release_headers + 20) == 7 &&
                    result.u32(release_policy + 8) == 1 && result.u32(release_policy + 12) == away_events,
                "schedule expiry frees the emptied buffer and leaves the unallocated header alone");
        ++comparisons;
    }
    for (const unsigned scenario : {0u, 1u, 2u, 3u, 4u, 5u}) {
        const auto state = prepare();
        Cpu c(*state);
        if (scenario < 2) {
            c.put32(club_count, scenario ? 3 : 4);
            c.put32(calendar + 0x20, unmapped);
        } else if (scenario == 2) {
            header(c, 0, unmapped, 0xff);
            c.put32(toc - 0x7b80, unmapped);
        } else if (scenario == 3) {
            header(c, 0, release_rows, 0);
            header(c, 1, grown_rows, 0);
            c.put32(toc - 0x7b80, unmapped);
        } else if (scenario == 4) {
            header(c, 0, release_rows, 2);
            row(c, release_rows, -1, 0x8000, 1);
            row(c, release_rows + 12, 0, 0x8000, 2);
        } else {
            header(c, 0, release_rows, 1);
            row(c, release_rows, -2, 0x8000, 1);
        }
        const auto after = compare(*state);
        Cpu result(*after);
        if (scenario < 3 || scenario == 4) {
            require(!result.u32(release_policy + 8) &&
                        after->memory.copy_out(release_headers, 24) == state->memory.copy_out(release_headers, 24) &&
                        after->memory.copy_out(release_rows, 24) == state->memory.copy_out(release_rows, 24),
                    "empty, skipped and still-current calendar states retain their bytes");
        } else {
            require(!result.u32(release_headers) && !result.u8(release_headers + 4) &&
                        result.u32(release_policy + 8) == (scenario == 3 ? 2u : 1u) &&
                        result.u32(release_policy + 12) == release_rows &&
                        result.u32(release_policy + 16) == (scenario == 3 ? grown_rows : 0u),
                    "schedule expiry releases empty buffers in club order");
        }
        ++comparisons;
    }
    {
        const auto state = prepare();
        Cpu c(*state);
        header(c, 0, release_rows, 2);
        row(c, release_rows, 30, 0x8000, 1);
        row(c, release_rows + 12, -2, 0x8000, 2);
        const auto after = compare(*state);
        Cpu result(*after);
        require(result.u8(release_headers + 5) == 1 && result.i16(release_rows) == -2 &&
                    result.u8(release_rows + 8) == 2 && !result.u32(release_policy + 8),
                "unordered input drops the counted prefix as in the original");
        ++comparisons;
    }
    auto state = prepare();
    {
        Cpu c(*state);
        header(c, 0, release_rows, 0);
        header(c, 1, unmapped, 1);
        const auto after = compare(*state, Stop::fault);
        Cpu result(*after);
        require(!result.u32(release_headers) && result.u32(release_policy + 8) == 1 &&
                    result.u32(release_headers + 8) == unmapped,
                "schedule expiry preserves earlier club effects on a later invalid row");
    }
    state = prepare();
    Cpu(*state).put32(calendar + 0x20, unmapped);
    compare(*state, Stop::fault);
    state = prepare();
    state->budget = 0;
    compare(*state, Stop::budget);
    return comparisons;
}

std::unique_ptr<State> fixture_marking_seed(std::mt19937& random) {
    auto state = domestic_schedule_seed(random);
    state->memory.map(mark_groups, 0x1000);
    Cpu c(*state);
    c.put32(toc - 0x7b44, mark_groups);
    c.put32(toc - 0x7b54, competition_slot);
    c.put32(competition_slot, competition_manager);
    c.put32(fixture_slot, competition_manager);
    c.put32(calendar + 0x24, mark_array);
    for (unsigned i = 2; i < 34; ++i) c.put32(mark_groups + i * 0x4c, team_nation);
    c.put32(mark_groups + 2 * 0x4c, 0);
    c.put8(mark_groups + 2 * 0x4c + 0x14, 5);
    c.put32(mark_groups + 3 * 0x4c, release_nation);
    c.put8(mark_groups + 3 * 0x4c + 0x14, 7);
    c.put8(mark_array - 1, 0x71);
    c.put8(mark_array, 0x72);
    c.put8(mark_array + 1, 0x73);
    c.put8(mark_array + 2, 0x74);
    c.put8(mark_array + 3, 0x75);
    c.set({{3, calendar}, {4, date}});
    state->pc = mark_fixtures;
    return state;
}

unsigned check_fixture_marking(std::mt19937& random) {
    const auto match = [](Cpu& c, unsigned i, std::uint32_t home, std::uint32_t away,
                          std::uint32_t cup = competition) {
        const auto at = mark_matches + i * 0x40;
        c.put32(at + 0x14, cup);
        c.put32(at + 0x1c, home);
        c.put32(at + 0x20, away);
        return at;
    };
    const auto set_list = [](Cpu& c, unsigned slot, std::initializer_list<std::uint32_t> matches) {
        const auto list = fixtures_head + slot * 0x10;
        auto node = fixture_nodes + slot * 0x40;
        c.put32(mark_lists + slot * 4, list);
        c.put32(list + 4, node);
        c.put32(list + 12, unmapped);
        unsigned left = matches.size();
        for (const auto at : matches) {
            c.put32(node, at);
            c.put32(node + 8, --left ? node + 0x10 : 0);
            node += 0x10;
        }
    };
    unsigned comparisons = 0;
    for (unsigned scenario = 0; scenario < 16; ++scenario) {
        const auto state = fixture_marking_seed(random);
        Cpu c(*state);
        const auto home = release_clubs, away = release_clubs + 0x100;
        std::array<unsigned, 4> expected{0x71, 0x72, 0x73, 0x74};
        switch (scenario) {
        case 0:
            c.put32(calendar + 0x24, 0);
            expected = {0x71, 0, 0, 0};
            break;
        case 1:
            c.put32(calendar + 0x24, 0);
            c.put8(release_policy, 1);
            break;
        case 2:
            set_list(c, 0, {match(c, 0, home, away)});
            expected[1] = expected[2] = 3;
            break;
        case 3:
            set_list(c, 0, {match(c, 0, 0, 0), match(c, 1, home, away, competition + 4)});
            break;
        case 4:
            set_list(c, 0, {match(c, 0, home, away)});
            c.put32(home + 0x2c, 0);
            c.put32(away + 0x2c, team_nation);
            break;
        case 5:
            set_list(c, 0, {match(c, 0, home, away)});
            c.put32(home, 3);
            c.put32(away, UINT32_MAX);
            expected[0] = 3;
            break;
        case 6:
            set_list(c, 3, {match(c, 0, home, away)});
            c.put32(home + 0x30, competition);
            expected[1] = expected[2] = 1;
            break;
        case 7:
            set_list(c, 3, {match(c, 0, home, away)});
            c.put32(away + 0x38, competition);
            expected[1] = expected[2] = 1;
            break;
        case 8:
            set_list(c, 3, {match(c, 0, home, away)});
            expected[1] = expected[2] = 2;
            break;
        case 9:
            set_list(c, 3, {match(c, 0, 0, away)});
            expected[2] = 2;
            break;
        case 10:
            set_list(c, 3, {match(c, 0, home, 0)});
            c.put32(home + 0x30, competition);
            expected[1] = 1;
            break;
        case 11:
            set_list(c, 3, {match(c, 0, home, away, competition + 4)});
            break;
        case 12:
            set_list(c, 1, {match(c, 0, home, away)});
            set_list(c, 4, {match(c, 1, home, away)});
            c.put32(home + 0x30, competition);
            expected[1] = expected[2] = 1;
            break;
        case 13:
            set_list(c, 0, {match(c, 0, home, 0)});
            set_list(c, 3, {match(c, 1, home, away)});
            c.put32(home, 3);
            c.put32(away, 4);
            break;
        case 14:
            set_list(c, 0, {match(c, 0, 0, away)});
            c.put32(away + 0x2c, 0);
            break;
        case 15:
            set_list(c, 0, {match(c, 0, home, 0)});
            c.put32(home + 0x2c, team_nation);
            break;
        }
        const auto after = compare(*state);
        Cpu result(*after);
        for (unsigned i = 0; i < expected.size(); ++i)
            require(result.u8(mark_array - 1 + i) == expected[i], "domestic fixture mark byte");
        const bool failed = scenario == 1;
        require(result.u8(mark_array + 3) == 0x75 && result.u32(calendar + 0x24) == (failed ? 0 : mark_array) &&
                    result.u8(injury_success) == !failed &&
                    result.u32(archive_trace + 0x10) == (failed ? 0x221cu : 0u) &&
                    result.u8(release_policy + 6) == (failed ? 0u : 6u),
                "fixture marking allocation, guard bytes and six eligible group queries");
        ++comparisons;
    }
    auto fault = fixture_marking_seed(random);
    {
        Cpu c(*fault);
        const auto home = release_clubs;
        set_list(c, 0, {match(c, 0, home, 0), match(c, 1, unmapped, 0)});
        const auto after = compare(*fault, Stop::fault);
        require(Cpu(*after).u8(mark_array) == 3, "fixture marking preserves earlier writes before a side fault");
    }
    fault = fixture_marking_seed(random);
    {
        Cpu c(*fault);
        set_list(c, 3, {match(c, 0, release_clubs, 0)});
    }
    compare(*fault, Stop::fault);
    const auto budget = fixture_marking_seed(random);
    budget->budget = 0;
    compare(*budget, Stop::budget);
    return comparisons;
}

unsigned check_fixture_application(std::mt19937& random) {
    const auto prepare = [&] {
        auto state = fixture_marking_seed(random);
        Cpu c(*state);
        c.put32(toc - 0x3fb4, mark_lists + 0x80);
        c.put32(toc - 0x3fac, mark_lists + 0x90);
        c.put8(toc - 0x35bd, 1);
        c.put8(toc - 0x35bb, 1);
        c.put8(mark_array, 1);
        c.put8(mark_array + 1, 2);
        c.put8(mark_array + 2, 3);
        c.put8(release_policy + 0x20, 1);
        fixture_date(c, date, 0, 2000);
        state->pc = apply_marks;
        return state;
    };
    struct Row { int day; unsigned year, releases; };
    const auto rows = [](Cpu& c, unsigned club, std::uint32_t base, std::initializer_list<Row> entries) {
        c.put32(release_headers + 8 * club, base);
        c.put8(release_headers + 8 * club + 5, entries.size());
        for (const auto& entry : entries) {
            fixture_date(c, base, entry.day, entry.year);
            c.put8(base + 8, entry.releases);
            base += 12;
        }
    };
    const auto check_requests = [](Cpu& result, std::initializer_list<unsigned> expected) {
        require(result.u8(release_policy + 0x24) == expected.size(), "domestic fixture reschedule count");
        unsigned index = 0;
        for (const auto club : expected) {
            require(result.u32(release_policy + 0x28 + index * 4) == clubs + club * 0x248,
                    "fixture reschedule order and club address");
            ++index;
        }
    };

    unsigned comparisons = 0;
    for (unsigned scenario = 0; scenario < 7; ++scenario) {
        const auto state = prepare();
        Cpu c(*state);
        c.put32(release_headers + 16, unmapped);
        c.put8(release_headers + 21, 1);
        switch (scenario) {
        case 0:
            rows(c, 0, release_rows, {{30, 1998, 90}, {363, 1999, 80}, {364, 1999, 1},
                                      {1, 2000, 2}, {2, 2000, 100}});
            rows(c, 1, grown_rows, {{0, 2000, 130}});
            break;
        case 1:
            rows(c, 0, release_rows, {{0, 2000, 2}, {0, 2001, 200}, {1, 2000, 9}});
            rows(c, 1, grown_rows, {{0, 2000, 255}, {1, 2000, 4}});
            break;
        case 2:
            c.put8(mark_array, 0xff);
            c.put32(release_headers, unmapped);
            c.put8(release_headers + 5, 1);
            c.put32(release_headers + 8, unmapped);
            c.put8(release_headers + 13, 0xff);
            break;
        case 3:
            c.put32(club_count, 3);
            c.put32(calendar + 0x24, unmapped);
            break;
        case 4:
            fixture_date(c, date, 90, 2000);
            rows(c, 0, release_rows, {{-2, 2000, 90}, {-1, 2000, 90}, {89, 2000, 3}});
            rows(c, 1, grown_rows, {{91, 2000, 3}});
            break;
        case 5:
            rows(c, 0, release_rows, {{0, 2000, 127}});
            rows(c, 1, grown_rows, {{0, 2000, 128}});
            break;
        case 6:
            rows(c, 0, release_rows, {{364, 1999, 3}});
            rows(c, 1, grown_rows, {{1, 2000, 1}, {1, 2000, 1}});
            break;
        }
        const auto after = compare(*state);
        Cpu result(*after);
        if (scenario == 0 || scenario == 5 || scenario == 6) check_requests(result, {0});
        else if (scenario == 1) check_requests(result, {1});
        else if (scenario == 4) check_requests(result, {0, 1});
        else check_requests(result, {});
        require(!result.u8(release_policy + 6), "preexisting fixture marks skip the marking window");
        ++comparisons;
    }
    {
        const auto state = prepare();
        Cpu c(*state);
        c.put32(calendar + 0x24, 0);
        c.put32(mark_groups + 2 * 0x4c, team_nation);
        c.put32(mark_lists + 12, fixtures_head);
        c.put32(fixtures_head + 4, fixture_nodes);
        c.put32(fixture_nodes, mark_matches);
        c.put32(mark_matches + 0x14, competition);
        c.put32(mark_matches + 0x1c, release_clubs);
        c.put32(mark_matches + 0x20, release_clubs + 0x100);
        c.put32(release_clubs + 0x30, competition);
        rows(c, 0, release_rows, {{364, 1999, 1}, {1, 2000, 2}});
        const auto after = compare(*state, Stop::returned, Corrupt::none, false, {mark_fixtures});
        Cpu result(*after);
        check_requests(result, {0});
        require(result.u8(release_policy + 6) == 9 && result.u32(release_policy + 0x34) == 2000 &&
                    result.u32(release_policy + 0x38) == (364u << 16 | 1999u) &&
                    result.u32(release_policy + 0x3c) == (1u << 16 | 2000u),
                "lazy fixture marks use the current, previous and following date in order");
        ++comparisons;
    }
    auto fault = prepare();
    Cpu(*fault).put32(calendar + 0x20, unmapped);
    compare(*fault, Stop::fault);
    fault = prepare();
    {
        Cpu c(*fault);
        c.put32(calendar + 0x24, 0);
        c.put8(release_policy, 1);
        const auto after = compare(*fault, Stop::fault);
        Cpu result(*after);
        require(!result.u8(injury_success) && result.u32(archive_trace + 0x10) == 0x221c,
                "failed mark allocation preserves the original assertion before scanning");
    }
    const auto budget = prepare();
    budget->budget = 0;
    compare(*budget, Stop::budget);
    return comparisons;
}

unsigned check_calendar_notices(std::mt19937& random) {
    const auto prepare = [&](std::uint32_t entry) {
        auto state = domestic_schedule_seed(random);
        Cpu c(*state);
        constexpr std::array offsets{-0x7424, -0x7b5c, -0x7a70, -0x7520, -0x7b90,
                                     -0x7b88, -0x7b84, -0x7bf0, -0x6948};
        for (unsigned i = 0; i < offsets.size(); ++i) c.put32(toc + offsets[i], calendar_notice_globals + 4 * i);
        c.put32(toc - 0x7b8c, calendar_variant_words);
        c.put32(calendar_notice_globals, competition_manager);
        c.put32(calendar_notice_globals + 4, calendar_notice_globals + 0x30);
        c.put32(calendar_notice_globals + 8, 1);
        c.put32(calendar_notice_globals + 12, calendar_notice_globals);
        c.put32(calendar_notice_globals + 28, news_system);
        c.put32(calendar_notice_globals + 32, 0);
        c.put32(calendar_notice_globals + 0x30, team_nation);
        c.put32(calendar_notice_globals + 0x34, allowed_nation);
        c.put32(team_nation, 0);
        c.put32(allowed_nation, 1);
        c.put16(team_nation + 0x40, -12);
        c.put16(allowed_nation + 0x40, 33);
        c.put16(calendar_variant_words, 1);
        c.put16(calendar_variant_words + 2, 2);
        c.put16(calendar_variant_words + 4, 3);
        c.put16(calendar_variant_words + 6, 4);
        c.put32(calendar_rng_seed, 0x01234567);
        c.set(3, calendar);
        state->pc = entry;
        return state;
    };
    unsigned comparisons = 0;
    for (const auto& [club, nation] : {std::pair{0u, 0u}, std::pair{clubs, 0u},
                                      std::pair{0u, team_nation}, std::pair{clubs, team_nation}}) {
        const auto state = prepare(queue_calendar_notice);
        Cpu(*state).set({{4, club}, {5, nation}});
        const auto after = compare(*state);
        Cpu result(*after);
        require(result.u8(calendar_notice_trace) == unsigned(club && nation) &&
                    result.u32(calendar_notice_pairs) == (nation ? club : 0) &&
                    result.u32(calendar_notice_pairs + 4) == (club ? nation : 0),
                "calendar queue accepts only complete club and nation pairs");
        ++comparisons;
    }

    const auto seed_pairs = [](Cpu& c, std::initializer_list<std::pair<int, std::uint32_t>> pairs) {
        const auto list = calendar + 0x28;
        c.put16(list, pairs.size());
        c.put32(list + 4, pairs.size() ? fixture_nodes : 0);
        c.put32(list + 12, unmapped);
        unsigned index = 0;
        for (const auto& [club, nation] : pairs) {
            const auto pair = calendar_notice_pairs + index * 8, node = fixture_nodes + index * 16;
            c.put32(pair, club < 0 ? 0 : clubs + unsigned(club) * 0x248);
            c.put32(pair + 4, nation);
            c.put32(node, pair);
            c.put32(node + 8, ++index < pairs.size() ? node + 16 : 0);
            if (club >= 0) c.put32(clubs + unsigned(club) * 0x248, club);
        }
    };
    for (unsigned scenario = 0; scenario < 7; ++scenario) {
        const auto state = prepare(publish_calendar_notices);
        Cpu c(*state);
        const auto initial_rng = c.u32(calendar_rng_seed);
        unsigned groups = 1;
        if (scenario == 0) {
            groups = 0;
        } else if (scenario == 1) {
            seed_pairs(c, {{-1, 0}});
            groups = 0;
        } else if (scenario == 2 || scenario == 3) {
            seed_pairs(c, {{0, team_nation}, {4, allowed_nation}, {-1, 0}, {1, team_nation},
                           {2, team_nation}, {3, team_nation}});
            c.put16(clubs + 0x1ec, -2);
            c.put16(clubs + 0x248 + 0x1ec, 10);
            c.put16(clubs + 2 * 0x248 + 0x1ec, 0);
            c.put16(clubs + 3 * 0x248 + 0x1ec, 127);
            if (scenario == 3) c.put32(calendar_notice_globals + 32, 1);
            groups = 2;
        } else {
            seed_pairs(c, {{2, team_nation}});
        }
        const unsigned end = scenario == 4 ? 0 : scenario == 5 ? 2 : 6;
        c.put32(calendar_notice_globals + 20, calendar_variant_words + end);
        c.put32(calendar_notice_globals + 24, calendar_variant_words);
        c.put32(calendar_notice_globals + 16, scenario == 6 ? 0x80000001u : 3u);

        const auto after = compare(*state);
        Cpu result(*after);
        require(result.u8(calendar_notice_trace) == unsigned(scenario != 0) &&
                    result.u8(calendar_notice_trace + 1) == groups &&
                    result.u8(calendar_notice_trace + 2) == unsigned(scenario != 0),
                "calendar notices create once, publish by nation and clear once");
        std::uint32_t rng = initial_rng, seed = c.u32(calendar_notice_globals + 16);
        unsigned cursor = 0;
        for (unsigned group = 0; group < groups; ++group) {
            const auto fields = calendar_notice_trace + 0x20 + group * 0x40;
            for (const unsigned variants : {4u, 2u}) {
                cursor += 2;
                if (cursor > end) {
                    cursor = 0;
                    rng = cm::rng::next_state(rng);
                    seed = cm::rng::sample(rng);
                }
                const auto expected = std::int32_t(seed + c.u16(calendar_variant_words + cursor)) % int(variants);
                require(result.u32(fields + (variants == 4 ? 8 : 12)) == std::uint32_t(expected),
                        "calendar notice pool wrap and signed remainder");
            }
            require(result.u32(fields) == group && result.u32(fields + 16) == 44 + group &&
                        result.u32(fields + 20) == 81 + group &&
                        result.u32(fields + 24) == (group ? 33u : std::uint32_t(-12)) &&
                        result.u8(calendar_notice_trace + 4 + group) == ((group == unsigned(scenario == 3)) ? 1 : 2),
                    "calendar notice metadata and local versus headline delivery");
        }
        require(result.u32(calendar_rng_seed) == rng, "calendar notice RNG state");
        if (groups == 2) {
            const auto fields = calendar_notice_trace + 0x20;
            require(result.u32(fields + 4) == 4 && result.u32(fields + 28) == 1 &&
                        result.u32(fields + 32) == 2 && result.u32(fields + 36) == 0 &&
                        result.u32(fields + 40) == 3 &&
                        result.u32(fields + 0x40 + 4) == 1 && result.u32(fields + 0x40 + 28) == 4,
                    "calendar notice grouping and the original unsorted final club");
        }
        if (groups)
            require(result.u32(calendar_notice_globals + 24) == calendar_variant_words + cursor &&
                        result.u32(calendar_notice_globals + 16) == seed &&
                        !result.u16(calendar + 0x28), "calendar notices retain final variant state and clear the list");
        ++comparisons;
    }

    const auto fault = prepare(publish_calendar_notices);
    {
        Cpu c(*fault);
        seed_pairs(c, {{0, team_nation}, {1, 0}});
        c.put32(calendar_notice_pairs + 4, 0);
    }
    compare(*fault, Stop::fault);
    for (const auto entry : {queue_calendar_notice, publish_calendar_notices}) {
        auto state = prepare(entry);
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    return comparisons;
}

unsigned check_calendar_notice_formatter_reference(std::mt19937& random) {
    struct Case { unsigned count; int variant, detail, primary, secondary; bool full; };
    const auto prefix = [](unsigned count) -> unsigned {
        if (count == 0) return 0;
        if (count == 1) return 0x7eea;
        if (count == 2) return 0x7f09;
        if (count <= 4) return 0x7f4b;
        return count == 5 ? 0x7fac : 0x802d;
    };
    const auto headline = [](const Case& scenario) -> unsigned {
        switch (scenario.variant) {
        case 0: return scenario.count == 1 ? 0x7ced : scenario.count == 2 ? 0x7d31 : 0x7d83;
        case 1: return 0x7dc3;
        case 2: return scenario.secondary >= 0 ? 0x7e0b : 0x7e46;
        default: return 0x7ea5;
        }
    };
    const auto body = [](const Case& scenario) {
        constexpr unsigned keys[2][4][2]{
            {{0x820b, 0x80cc}, {0x845b, 0x8320}, {0x862f, 0x856c}, {0x87a8, 0x86c8}},
            {{0x899e, 0x885e}, {0x8be8, 0x8ab0}, {0x8dd6, 0x8cf6}, {0x8f89, 0x8e8c}},
        };
        const auto count = scenario.count;
        const unsigned band = count > 6 ? 0 : count > 3 ? 1 : count == 1 ? 2 : 3;
        return keys[scenario.detail != 0][band][scenario.secondary >= 0];
    };
    constexpr auto formatter_frame = stack - 0x6b0;
    const auto make_state = [&](const Case& scenario) {
        auto state = domestic_schedule_seed(random);
        state->memory.map(fixture + 0xd000, 0x1000);
        state->memory.map(stack - 0x1400, 0x1000);
        Cpu c(*state);
        c.put32(toc - 0x7bbc, staff_slot);
        c.put32(staff_slot, notice_render_people);
        c.put32(toc - 0x7b98, notice_render_cup_slot);
        c.put32(notice_render_cup_slot, notice_render_cups);
        c.put8(notice_render_people + 0x67, 0x83);
        c.put8(notice_render_people + 0x6a, 0x84);
        c.put8(notice_render_cups + 0x116, 0x91);
        c.put8(notice_render_cups + 0x128 + 0x116, 0x92);
        for (unsigned club = 0; club <= 8; ++club)
            c.put8(clubs + club * 0x248 + 0x23b, 0x80 + club);
        const std::array<unsigned, 7> fields{0, scenario.count, unsigned(scenario.variant),
            unsigned(scenario.detail), unsigned(scenario.primary), unsigned(scenario.secondary), 77};
        for (unsigned i = 0; i < fields.size(); ++i) c.put32(notice_render_item + 8 + i * 4, fields[i]);
        for (unsigned i = 0; i < scenario.count; ++i) c.put32(notice_render_item + 36 + i * 4, i);
        c.set({{3, calendar}, {4, notice_render_item}, {5, scenario.full ? 0x101u : 0x100u}, {6, notice_render_output}});
        state->pc = render_calendar_notice;
        return state;
    };
    unsigned comparisons = 0;
    const auto check = [&](const Case& scenario) {
        auto state = make_state(scenario);
        compare(*state);
        std::vector<DependencyCall> calls;
        ObserveCalls observer(calls);
        auto* saved_checker = routine_checker;
        struct RestoreChecker {
            RoutineChecker* previous;
            ~RestoreChecker() { routine_checker = previous; }
        } restore{saved_checker};
        // Decline generated direct calls into the modeled dependencies.
        RoutineChecker boundary([](std::uint32_t) { return false; }, 0, 0);
        routine_checker = &boundary;
        localization_format_diagnostics(*state).original_body = localize;
        national_teams_run_original(*state, render_calendar_notice);
        Cpu c(*state);
        require(state->pc == done, "original calendar formatter returns normally");

        std::size_t next = 0;
        const auto take = [&](std::uint32_t target) -> const DependencyCall& {
            require(next < calls.size() && calls[next][0] == target, "calendar formatter dependency order");
            return calls[next++];
        };
        for (unsigned field = 0; field < 7 + scenario.count; ++field)
            require(take(0x102cb570)[6] == field, "calendar formatter reads fields in order");
        take(0x10746a70);
        const auto intro = scenario.full ? prefix(scenario.count) : 0u;
        const auto key = scenario.full ? body(scenario) : headline(scenario);
        if (intro) {
            const auto& call = take(localize);
            require(call[5] == formatter_frame + 0x270 && call[6] == source + intro,
                    "calendar formatter optional club list template");
            const auto included = scenario.count <= 3 ? scenario.count : std::min(scenario.count - 1, 5u);
            const auto first = scenario.count <= 3 ? 0u : 1u;
            const auto arg = [&](unsigned i) {
                const auto at = notice_render_trace + 8;
                return c.u32(at + (i < 6 ? 8 + i * 4 : 40 + (i - 6) * 4));
            };
            for (unsigned i = 0; i < included; ++i) {
                const auto club = first + i;
                const auto grammar = std::uint32_t(std::int8_t(0x80 + club));
                require(arg(i * 2) == grammar && arg(i * 2 + 1) == grammar &&
                            arg(included * 2 + i) == clubs + club * 0x248 + 0x221,
                        "calendar formatter club list grammar and names");
            }
        }
        const auto& final = take(localize);
        require(final[5] == label_shared && final[6] == source + key &&
                    c.u8(notice_render_trace) == 1 + unsigned(intro != 0),
                "calendar formatter final localized template");
        take(0x1074b830);
        require(c.u32(notice_render_output) == key, "calendar formatter assigns the final text");

        const auto link = [&](std::uint32_t token, unsigned kind, std::uint32_t object) {
            const auto& call = take(0x1074bab0);
            require(call[6] == token && call[7] == kind && call[8] == object,
                    "calendar formatter linked token, kind and identity");
        };
        for (unsigned club = 0; club < scenario.count; ++club) {
            const auto object = clubs + club * 0x248;
            link(object + 0x1ee, 0x7d5, object);
            link(object + 0x221, 0x7d5, object);
        }
        for (const int cup : {scenario.primary, scenario.secondary}) {
            if (cup < 0) continue;
            const auto object = notice_render_cups + unsigned(cup) * 0x128;
            require(take(0x100c76c0)[5] == object, "calendar formatter competition identity lookup");
            link(object + 0xe1, 0x7d5, object + 0x10);
        }
        link(notice_render_people + 0x1a, 0x7d0, notice_render_people);
        link(notice_render_people + 0x4d, 0x7d0, notice_render_people);
        require(next == calls.size(), "calendar formatter has no unclassified dependency");
        ++comparisons;
    };
    for (const auto& scenario : {Case{1, 0, 0, 0, -1, false}, Case{2, 0, 0, 0, 1, false},
            Case{4, 0, 0, 0, -1, false}, Case{0, 0, 0, -1, -1, false}, Case{1, 1, 0, 0, -1, false},
            Case{1, 2, 0, 0, 1, false}, Case{1, 2, 0, 0, -1, false}, Case{1, -1, 0, 0, -1, false},
            Case{1, 3, 0, 0, -1, false}}) check(scenario);
    for (unsigned count : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 9u})
        for (int detail : {0, 1})
            for (int secondary : {-1, 1}) check({count, 0, detail, 0, secondary, true});
    check({7, 0, -1, 0, 1, true});
    check({5, 0, 2, 0, -1, true});
    unsigned prefixes = 0;
    for (const unsigned count : {1u, 2u, 3u, 4u, 5u, 6u, 9u}) {
        const auto state = make_state({count, 0, 0, 0, -1, true});
        Cpu(*state).put8(notice_render_trace + 1, 1);
        const auto after = compare(*state, Stop::fault);
        require(Cpu(*after).u32(formatter_frame + 0x270) == prefix(count),
                "calendar prefix compares through the expected localized phrase");
        ++prefixes;
    }
    const Case single{1, 0, 0, 0, -1, false};
    auto fault = make_state(single);
    Cpu(*fault).put32(clubs_slot, unmapped);
    compare(*fault, Stop::fault);
    fault = make_state({1, 2, 0, 0, -1, false});
    Cpu(*fault).put32(notice_render_cup_slot, unmapped);
    compare(*fault, Stop::fault);
    fault = make_state({1, 0, 0, 0, 1, true});
    Cpu(*fault).put32(notice_render_cup_slot, unmapped);
    compare(*fault, Stop::fault);
    fault = make_state({7, 0, 0, 0, -1, true});
    Cpu(*fault).put32(staff_slot, unmapped);
    compare(*fault, Stop::fault);
    const auto budget = make_state(single);
    budget->budget = 0;
    compare(*budget, Stop::budget);
    std::cout << comparisons << " NATIONAL_TEAMS calendar formatter comparisons, 4 faults and 1 budget stop\n";
    std::cout << prefixes << " NATIONAL_TEAMS calendar article prefixes compared through localization\n";
    return comparisons;
}

unsigned check_calendar_appointments(std::mt19937& random) {
    const auto prepare = [&](std::uint32_t entry) {
        auto state = domestic_schedule_seed(random);
        Cpu c(*state);
        c.put32(toc - 0x7b80, date);
        c.put32(toc - 0x3fac, appointment_trace + 0x60);
        c.put8(toc - 0x35bb, 1);
        c.put32(player, 10);
        c.put32(national_team, 20);
        fixture_date(c, date, 365, 2000);
        c.set({{3, calendar}, {4, player}, {5, national_team}, {6, date}});
        state->pc = entry;
        return state;
    };

    unsigned cases = 0;
    for (unsigned mode = 0; mode < 3; ++mode) {
        auto state = prepare(append_appointment);
        Cpu c(*state);
        c.put32(calendar, mode == 0 ? roster_list : 0);
        c.put32(roster_list + 4, roster_nodes);
        c.put8(appointment_trace, mode == 2);
        const auto after = compare(*state);
        Cpu result(*after);
        require(result.u32(calendar) == (mode == 2 ? 0 : roster_list) &&
                    result.u8(appointment_trace + 1) == unsigned(mode != 0) &&
                    result.u8(appointment_trace + 2) == 1 &&
                    result.u32(roster_list + 4) == (mode == 1 ? 0 : roster_nodes) &&
                    result.u32(appointment_rows) == 10 && result.u32(appointment_rows + 4) == 20 &&
                    result.i16(appointment_rows + 8) == 365 && result.u16(appointment_rows + 10) == 2000 &&
                    result.u8(appointment_rows + 12) == 1 && result.u16(appointment_rows + 16) == 1000,
                "calendar appointment payload, allocation and list initialization");
        ++cases;
    }

    for (const auto entry : {expire_nation_appointments, expire_pair_appointments}) {
        for (unsigned mode = 0; mode < 3; ++mode) {
            auto state = prepare(entry);
            Cpu c(*state);
            const bool exact = entry == expire_pair_appointments;
            c.put32(calendar, mode == 0 ? 0 : roster_list);
            c.put32(roster_list + 4, mode == 2 ? fixture_nodes : 0);
            c.put32(roster_list + 12, unmapped);
            if (!exact) c.set(4, national_team);
            for (unsigned index = 0; index < 4; ++index) {
                const auto node = fixture_nodes + index * 0x10, row = appointment_rows + index * 0x20;
                c.put32(node, row);
                c.put32(node + 8, index == 3 ? 0 : node + 0x10);
                c.put32(row, index == 1 ? 11 : 10);
                c.put32(row + 4, index == 2 ? 21 : 20);
                c.put16(row + 8, 12);
                c.put16(row + 10, 1999);
                c.put16(row + 0x10, 1000);
                c.put16(row + 0x12, 0x5aa5);
            }
            const auto after = compare(*state);
            Cpu result(*after);
            require(result.u32(roster_list + 12) == (mode ? 0 : unmapped),
                    "calendar appointment scan leaves the cursor as in the original");
            for (unsigned index = 0; index < 4; ++index) {
                const auto row = appointment_rows + index * 0x20;
                const bool updated = mode == 2 && index != 2 && (!exact || index != 1);
                require(result.u16(row + 0x10) == (updated ? 0 : 1000) &&
                            result.u16(row + 0x12) == 0x5aa5 && result.u16(row + 10) == 1999,
                        "calendar appointment expires the matching rows on the next day of a new year");
            }
            ++cases;
        }
    }

    auto fault = prepare(append_appointment);
    Cpu(*fault).set(6, unmapped);
    Cpu(*fault).put32(calendar, 0);
    const auto after = compare(*fault, Stop::fault);
    require(Cpu(*after).u32(calendar) == roster_list, "appointment allocation precedes date access");
    for (const auto entry : {append_appointment, expire_nation_appointments, expire_pair_appointments}) {
        auto state = prepare(entry);
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    return cases;
}

unsigned check_staff_assignment_updates(std::mt19937& random) {
    constexpr auto local_manager = assignment_managers + 0xc8, other_manager = assignment_managers;
    const auto prepare = [&](std::uint32_t entry) {
        auto state = domestic_schedule_seed(random);
        state->memory.map(assignment_nations, 0x1000);
        Cpu c(*state);
        c.put32(toc - 0x79f0, assignment_slots);
        c.put32(toc - 0x7a70, assignment_slots + 4);
        c.put32(toc - 0x7520, assignment_slots + 8);
        c.put32(toc - 0x7b98, assignment_slots + 12);
        c.put32(assignment_slots, news_system);
        c.put32(assignment_slots + 4, 1);
        c.put32(assignment_slots + 8, assignment_managers);
        c.put32(assignment_slots + 12, assignment_nations);
        c.put32(calendar + 4, records);
        c.put32(calendar + 8, release_rows);
        c.put32(nation_count, 3);
        for (unsigned i = 0; i < 3; ++i) {
            const auto nation = assignment_nations + i * 0x128;
            c.put32(nation + 0x68, i);
            c.put32(nation + 0x74, competition + (i == 2 ? 4 : 0));
            c.put32(records + i * record_size + 4, 1 + 2 * i);
            c.put32(release_rows + i * 4, 2 + 2 * i);
        }
        c.set({{3, calendar}, {4, entry == replace_assignment ? assignment_nations + 0x128 : competition}, {5, 0}});
        state->pc = entry;
        return state;
    };
    const auto notifications_are = [](Cpu& c, std::initializer_list<unsigned> indices) {
        require(c.u8(appointment_trace + 4) == indices.size(), "staff assignment notification count");
        unsigned index = 0;
        for (const auto nation : indices)
            require(c.u32(appointment_trace + 8 + index++ * 4) == 0x8000 + nation,
                    "staff assignment notification order");
    };
    unsigned cases = 0;
    for (unsigned mode = 0; mode < 3; ++mode) {
        auto state = prepare(replace_assignment);
        Cpu c(*state);
        if (mode == 0) c.put32(release_rows + 4, UINT32_MAX);
        if (mode == 1) c.put32(assignment_slots, 0);
        const auto after = compare(*state);
        Cpu result(*after);
        require(result.u32(records + record_size + 4) == (mode == 0 ? UINT32_MAX : 4) &&
                    result.u32(release_rows + 4) == UINT32_MAX && result.u32(records + 4) == 1 &&
                    result.u32(release_rows) == 2, "staff replacement clears the old assignment then promotes its pending staff");
        if (mode == 2) notifications_are(result, {1});
        else notifications_are(result, {});
        ++cases;
    }

    for (unsigned mode = 0; mode < 8; ++mode) {
        auto state = prepare(reconcile_assignments);
        Cpu c(*state);
        const auto employer = [&](unsigned id, std::uint32_t value) { c.put32(staff_table + id * 0x6c + 4, value); };
        for (unsigned id = 1; id <= 6; ++id) employer(id, other_manager);
        switch (mode) {
        case 0: c.put32(nation_count, UINT32_MAX); break;
        case 1: c.set(4, 0); break;
        case 2: employer(1, 0); employer(4, 0); c.set(5, 0x100); break;
        case 3: employer(1, local_manager); employer(4, local_manager); c.set(5, 0x180); break;
        case 4: employer(1, local_manager); employer(2, local_manager); c.set(5, 1); break;
        case 5: employer(1, 0); employer(4, 0); c.put32(assignment_slots, 0); break;
        case 6:
            c.put32(records + 4, UINT32_MAX);
            c.put32(records + record_size + 4, UINT32_MAX);
            break;
        case 7:
            c.put32(release_rows, UINT32_MAX);
            c.put32(release_rows + 4, UINT32_MAX);
            break;
        }
        const auto after = compare(*state);
        Cpu result(*after);
        const auto first = mode == 4 ? UINT32_MAX : mode == 2 || mode == 3 || mode == 5 || mode == 6 ? 2u : 1u;
        const auto second = mode == 6 ? 4u : 3u;
        require(result.u32(records + 4) == first && result.u32(records + record_size + 4) == second &&
                    result.u32(records + 2 * record_size + 4) == 5 && result.u32(release_rows + 8) == 6,
                "staff reconciliation filters nations and validates existing assignments");
        require(result.u32(release_rows) == (mode < 2 ? 2u : UINT32_MAX) &&
                    result.u32(release_rows + 4) == (mode < 2 || mode == 4 ? 4u : UINT32_MAX),
                "staff reconciliation retains only valid pending assignments");
        if (mode == 2 || mode == 3) notifications_are(result, {0});
        else if (mode == 6) notifications_are(result, {0, 1});
        else notifications_are(result, {});
        ++cases;
    }

    auto fault = prepare(replace_assignment);
    Cpu(*fault).put32(calendar + 8, unmapped);
    const auto after = compare(*fault, Stop::fault);
    require(Cpu(*after).u32(records + record_size + 4) == UINT32_MAX, "old assignment clears before pending lookup");
    fault = prepare(reconcile_assignments);
    {
        Cpu c(*fault);
        c.put32(staff_table + 0x6c + 4, 0);
        c.put32(calendar + 8, unmapped);
    }
    const auto reconciliation = compare(*fault, Stop::fault);
    require(Cpu(*reconciliation).u32(records + 4) == UINT32_MAX, "reconciliation clears invalid staff before pending lookup");
    for (const auto entry : {replace_assignment, reconcile_assignments}) {
        auto state = prepare(entry);
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    return cases;
}

unsigned check_direct_staff_assignments(std::mt19937& random) {
    constexpr auto incoming = assignment_slots + 0x14, reserved_ids = assignment_slots + 0x120;
    constexpr std::array offsets{0x7bc0, 0x7a40, 0x6948, 0x694c, 0x6950, 0x693c, 0x6a3c, 0x6938, 0x6b00};
    const auto prepare = [&](std::uint32_t entry) {
        auto state = domestic_schedule_seed(random);
        state->memory.map(assignment_nations, 0x1000);
        Cpu c(*state);
        c.put32(toc - 0x79f0, assignment_slots);
        c.put32(assignment_slots, news_system);
        c.put32(calendar + 4, records);
        c.put32(calendar + 8, release_rows);
        c.put8(injury_success, 1);
        for (unsigned i = 0; i < 3; ++i) {
            c.put32(assignment_nations + i * 0x128 + 0x68, i);
            c.put32(records + i * record_size + 4, i + 1);
            c.put32(release_rows + i * 4, i + 100);
        }
        for (unsigned i = 0; i < offsets.size(); ++i) {
            c.put32(toc - offsets[i], reserved_ids + i * 4);
            c.put32(reserved_ids + i * 4, 20 + i);
        }
        c.put32(staff_table + 0x6c, 3);
        c.put32(incoming, 7);
        c.set({{3, calendar}, {4, assignment_nations}, {5, incoming}});
        state->pc = entry;
        return state;
    };

    unsigned cases = 0;
    for (const auto argument : {4u, 5u}) {
        auto state = prepare(direct_assignment);
        Cpu(*state).set(argument, 0);
        const auto after = compare(*state);
        Cpu result(*after);
        require(result.u32(archive_trace + 0x10) == 0x1ba5 && result.u8(injury_success) == 0 &&
                    result.u32(records + 4) == 1 && result.u32(release_rows) == 100,
                "invalid assignment reports the assertion without changing current or pending staff");
        ++cases;
    }
    for (unsigned i = 0; i < offsets.size(); ++i) {
        auto state = prepare(direct_assignment);
        Cpu c(*state);
        c.put32(staff_table + 0x6c, 20 + i);
        if (i + 1 < offsets.size()) c.put32(toc - offsets[i + 1], unmapped);
        const auto after = compare(*state);
        Cpu result(*after);
        require(result.u32(records + 4) == 1 && result.u32(release_rows) == 7 &&
                    result.u8(appointment_trace + 4) == 0, "protected assignment queues replacement without notification");
        ++cases;
    }
    for (unsigned mode = 0; mode < 9; ++mode) {
        auto state = prepare(direct_assignment);
        Cpu c(*state);
        const bool empty = mode < 2, protected_incoming = mode >= 4 && mode <= 7;
        const bool listener = mode != 0 && mode != 2 && mode != 4;
        if (empty) c.put32(records + 4, UINT32_MAX);
        if (!listener) c.put32(assignment_slots, 0);
        if (protected_incoming) c.put32(incoming, mode == 4 ? 20 : 28);
        if (mode == 6) {
            c.set(5, release_rows);
            c.put32(release_rows, 28);
        }
        if (mode == 7 || mode == 8) {
            c.put32(records + 4, 0);
            c.put32(staff_slot, 0);
        }
        const auto after = compare(*state);
        Cpu result(*after);
        const auto assigned = mode == 4 ? 20u : mode == 5 || mode == 7 ? 28u : mode == 6 ? 1u : 7u;
        require(result.u32(records + 4) == assigned &&
                    result.u32(release_rows) == (protected_incoming ? (mode == 7 ? 0u : 1u) : 100u),
                "direct assignment installs staff and preserves pending order under aliasing");
        require(result.u8(appointment_trace + 4) == listener &&
                    (!listener || result.u32(appointment_trace + 8) == 0x8000),
                "direct assignment notification");
        ++cases;
    }

    const auto prepare_release = [&](unsigned count, bool allocate) {
        auto state = prepare(release_assignments);
        Cpu c(*state);
        c.put32(toc - 0x7b5c, assignment_slots + 4);
        c.put32(assignment_slots + 4, assignment_slots + 8);
        c.put32(assignment_slots + 8, assignment_nations);
        c.put32(assignment_manager, 0);
        c.put32(assignment_nations, assignment_virtual_table);
        c.put32(assignment_virtual_table + 0x60, assignment_descriptor);
        c.put32(assignment_descriptor, assignment_enumerator);
        c.put32(assignment_descriptor + 4, assignment_callback_toc);
        c.put32(assignment_club_list + 0x10, count);
        c.put32(assignment_club_list + 0x14, allocate ? assignment_club_list : 0);
        for (unsigned i = 0; i < 3; ++i) {
            const auto club = assignment_clubs + i * 0x40;
            c.put32(assignment_club_list + i * 4, club);
            c.put32(club + 4, i == 0 ? 0x180 : i == 1 ? 0x100 : 1);
            c.put32(club + 0x2c, assignment_nations + i * 0x128);
        }
        c.set(4, assignment_manager);
        return state;
    };
    for (unsigned mode = 0; mode < 5; ++mode) {
        const bool active = mode >= 3, allocate = mode != 0, listener = mode == 4;
        auto state = prepare_release(active ? 0x12340003u : mode == 2 ? 0x1ffffu : 0u, allocate);
        if (!listener) Cpu(*state).put32(assignment_slots, 0);
        const auto after = compare(*state);
        Cpu result(*after);
        require(result.u32(records + 4) == (active ? UINT32_MAX : 1) &&
                    result.u32(records + record_size + 4) == 2 &&
                    result.u32(records + 2 * record_size + 4) == (active ? UINT32_MAX : 3) &&
                    result.u32(release_rows) == 100 && result.u32(release_rows + 8) == 102,
                "queued releases use the signed count and clear only eligible current assignments");
        require(result.u8(appointment_trace + 4) == (listener ? 2u : 0u) &&
                    (!listener || (result.u32(appointment_trace + 8) == assignment_clubs &&
                                   result.u32(appointment_trace + 12) == assignment_clubs + 0x80)),
                "queued release notifications follow the returned order");
        require(result.u8(appointment_trace + 0x18) == allocate && result.u32(stack - 0x28) == 0 &&
                    result.r(2) == toc, "queued releases dispose of the array and clear the caller scratch slot");
        ++cases;
    }

    auto fault = prepare(direct_assignment);
    Cpu(*fault).put32(toc - 0x79f0, unmapped);
    const auto assigned = compare(*fault, Stop::fault);
    require(Cpu(*assigned).u32(records + 4) == 7, "assignment completes before a listener fault");
    fault = prepare_release(3, true);
    Cpu(*fault).put32(assignment_club_list + 8, unmapped);
    const auto released = compare(*fault, Stop::fault);
    Cpu result(*released);
    require(result.u32(records + 4) == UINT32_MAX && result.u32(records + 2 * record_size + 4) == 3 &&
                result.u8(appointment_trace + 4) == 1 && result.u8(appointment_trace + 0x18) == 0,
            "queued release preserves earlier notifications and stops before disposal on a bad later club");
    for (auto entry : {direct_assignment, release_assignments}) {
        auto state = entry == direct_assignment ? prepare(entry) : prepare_release(3, true);
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    return cases;
}

void check_controls(std::mt19937& random) {
    for (const auto entry : {context, read_flag, clear_flag, read_value, get_slot, get_staff}) {
        auto s = seed(random);
        Cpu c(*s);
        c.set({{3, manager}, {4, unmapped}, {5, 0}});
        s->pc = entry;
        compare(*s, Stop::fault);
        s->budget = 0;
        compare(*s, Stop::budget);
    }
    for (const auto entry : {international, duty}) {
        auto state = duty_seed(random);
        Cpu c(*state);
        c.set(4, entry == international ? regular_club : unmapped);
        if (entry == international) c.put32(toc - 0x7204, unmapped);
        state->pc = entry;
        compare(*state, Stop::fault);
        state->budget = 0;
        compare(*state, Stop::budget);
    }
    auto s = seed(random);
    Cpu(*s).set({{3, manager}, {4, nation}, {5, 0}});
    s->pc = read_value;
    bool rejected = false;
    try { compare(*s, Stop::returned, Corrupt::result); }
    catch (const std::runtime_error& error) {
        rejected = std::string_view(error.what()).find("register byte") != std::string_view::npos;
    }
    require(rejected, "corrupted result must fail the differential check");

    Cpu(*s).set({{3, manager}, {4, club}, {5, 0}});
    s->pc = context;
    rejected = false;
    try { compare(*s, Stop::returned, Corrupt::dependency); }
    catch (const std::runtime_error& error) {
        rejected = std::string_view(error.what()).find("ordered dependency arguments") != std::string_view::npos;
    }
    require(rejected, "corrupted dependency argument must fail the differential check");
}
}

int main() try {
    std::mt19937 random(0x4e41544c);
    const auto slots = check_selection_slots(random);
    const auto staff = check_staff(random);
    const auto contexts = check_context(random);
    check_controls(random);
    const auto eligibility = check_international_club(random) + check_duty(random);
    const auto queries = check_selection_tiers(random) + check_selection_prefixes(random) +
        check_international_competition(random);
    const auto query_faults = check_selection_query_faults(random);
    const auto roster = check_remove_selection(random) + check_selected_player(random);
    const auto roster_faults = check_roster_faults(random);
    const auto cleanup = check_dedup_rosters(random) + check_drop_roster(random);
    const auto cleanup_faults = check_roster_cleanup_faults(random);
    const auto notices = check_roster_notices(random);
    const auto injured = check_injured_selection(random);
    const auto replacement = check_replacement_roster(random);
    const auto dispatch = check_replacement_dispatch(random);
    const auto calendar_cases = check_calendar_availability(random);
    const auto calendar_saves = check_calendar_save(random);
    const auto calendar_loads = check_calendar_load(random);
    const auto schedules = check_domestic_schedule(random);
    const auto sort_comparisons = check_schedule_comparators(random);
    const auto expired_schedules = check_schedule_expiry(random);
    const auto marked_fixtures = check_fixture_marking(random);
    const auto applied_marks = check_fixture_application(random);
    const auto calendar_notices = check_calendar_notices(random);
    const auto formatter_references = check_calendar_notice_formatter_reference(random);
    const auto appointments = check_calendar_appointments(random);
    const auto assignments = check_staff_assignment_updates(random);
    const auto direct_assignments = check_direct_staff_assignments(random);
    std::cout << direct_assignments << " NATIONAL_TEAMS direct assignments and release comparisons, 2 faults and 2 budget stops\n";
    std::cout << appointments << " NATIONAL_TEAMS appointment comparisons, 1 fault and 3 budget stops\n";
    std::cout << assignments << " NATIONAL_TEAMS staff assignment comparisons, 2 faults and 2 budget stops\n";
    std::cout << formatter_references << " NATIONAL_TEAMS calendar formatter reference cases\n";
    std::cout << calendar_notices << " NATIONAL_TEAMS calendar notice comparisons, 1 fault and 2 budget stops\n";
    std::cout << applied_marks << " NATIONAL_TEAMS fixture application comparisons, 2 faults and 1 budget stop\n";
    std::cout << marked_fixtures << " NATIONAL_TEAMS fixture marking comparisons, 2 faults and 1 budget stop\n";
    std::cout << expired_schedules << " NATIONAL_TEAMS schedule expiry comparisons, 2 faults and 1 budget stop\n";
    std::cout << sort_comparisons << " NATIONAL_TEAMS schedule comparator comparisons, 4 faults and 3 budget stops\n";
    std::cout << schedules << " NATIONAL_TEAMS domestic schedule comparisons, 1 fault and 2 budget stops\n";
    std::cout << calendar_loads << " NATIONAL_TEAMS calendar load comparisons, 2 faults and 1 budget stop\n";
    std::cout << calendar_saves << " NATIONAL_TEAMS calendar save comparisons, 1 fault and 1 budget stop\n";
    std::cout << calendar_cases << " NATIONAL_TEAMS calendar comparisons, 2 faults and 2 budget stops\n";
    std::cout << dispatch << " NATIONAL_TEAMS replacement search comparisons, 2 faults and 1 budget stop\n";
    std::cout << replacement << " NATIONAL_TEAMS replacement roster comparisons, 2 faults and 2 budget stops\n";
    std::cout << injured << " NATIONAL_TEAMS injury roster comparisons, 1 fault and 1 budget stop\n";
    std::cout << notices << " NATIONAL_TEAMS roster news comparisons, 3 faults and 3 budget stops\n";
    std::cout << cleanup << " NATIONAL_TEAMS roster cleanup comparisons, " << cleanup_faults
              << " faults and 2 budget stops\n";
    std::cout << roster << " NATIONAL_TEAMS roster comparisons, " << roster_faults << " faults and 2 budget stops\n";
    std::cout << queries << " NATIONAL_TEAMS selection tier, prefix and competition comparisons, "
              << query_faults << " faults and 3 budget stops\n";
    std::cout << eligibility << " NATIONAL_TEAMS club and duty eligibility comparisons\n";
    std::cout << slots + staff + contexts << " NATIONAL_TEAMS returned comparisons, "
                 "8 faults, 8 budget stops and two rejected corruptions\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}

#include "match_day_driver.hpp"
#include "guest_abi.hpp"
#include "guest_scratch.hpp"
#include "guest_worker_pool.hpp"
#include "match_play.hpp"
#include "tactics.hpp"

#include <array>
#include <algorithm>
#include <atomic>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <sys/sysctl.h>
#include <initializer_list>
#include <string_view>
#include <vector>

// The background match day's driver, 0x101cb9d0 (3,640 bytes), written from the listing. The routine works on the
// day's fixture list (records of 0x6c bytes at this+0x1c, their count at this+0x24):
//  1. fixtures of human managers get their viewer record and their matches are played first;
//  2. every other full match is created, set up, played to the end and handed to the post-match routine;
//  3. a last pass over the list files each result and releases it.
// Non-volatile registers hold what the original holds at each call (callees save them, and those saves are
// stack bytes later code may read); volatile ones only carry arguments.
namespace cm::aot {
namespace {
using guest::Cpu;
constexpr std::uint32_t entry_address = 0x101cb9d0;
constexpr std::uint32_t fixture_size = 0x6c, manager_size = 0x70;

MatchDayOrder initial_order() {
    const char* value = std::getenv("CM_MATCH_DAY_ORDER");
    const std::string_view name = value ? value : "";
    return name == "in_order" ? MatchDayOrder::in_order : name == "phases" ? MatchDayOrder::phases
         : name == "parallel" ? MatchDayOrder::parallel : MatchDayOrder::original;
}
// Read on first use: no order of static initialisation to depend on.
std::atomic<MatchDayOrder>& order() {
    static std::atomic<MatchDayOrder> value{initial_order()};
    return value;
}
// phases: at most this many matches are set up before they are played, so their match objects (57 KiB each,
// plus what set-up allocates) do not pile up on a day with hundreds of fixtures.
constexpr std::size_t stretch_limit = 32;

bool finished(std::int8_t state) { return state == 2 || state == 8; }


struct Driver {
    Cpu& c;
    guest::Frame& frame;
    MatchDayOrder order;
    std::uint32_t self = 0;                       // r16
    // The routine's constants (r24-r31, r13, r14 on entry).
    std::uint32_t text = 0, post_match = 0, managers = 0, manager_count = 0, manager_base = 0, screen = 0, calendar = 0,
                  redraw = 0, message = 0, strings = 0;
    bool listed = false;                          // r20: a result line was written

    std::uint32_t sp() const { return frame.base(); }
    // `bl target` at `at` with these arguments in r3 up; the result is r3.
    std::uint32_t call(std::uint32_t target, std::uint32_t at, std::initializer_list<std::uint32_t> arguments = {}) {
        unsigned n = 3;
        for (const auto value : arguments) c.set(n++, value);
        c.call(target, at + 4);
        return c.r(3);
    }
    std::uint32_t fixtures() const { return c.u32(self + 0x1c); }
    std::int32_t fixture_count() const { return c.i32(self + 0x24); }
    // Manager records are visited from the last one back (`reverse`) or over the last sixteen forward.
    std::uint32_t manager_from_end(std::uint32_t n) const { return c.u32(manager_base) + (c.u32(manager_count) - n - 1) * manager_size; }
    std::uint32_t manager_of_last16(std::uint32_t n) const { return c.u32(manager_base) + (c.u32(manager_count) + n - 0x10) * manager_size; }

    // "Out of memory" at a source line: the original's assertion sequence, then the redraw flag cleared.
    void out_of_memory(std::uint32_t at, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
        call(0x10739890, at + 0x14, {strings, 0, 0, sp() + name, sp() + extension});
        const auto file = call(0x10768810, at + 0x18);
        call(0x1077b720, at + 0x34, {message, strings + 0xe, file, sp() + name, sp() + extension, line});
        call(0x10769b60, at + 0x44, {strings + 0x1b, message, 0});
        c.put8(redraw, 0);
    }
    bool human(std::uint32_t club, std::uint32_t side_flag, std::uint32_t manager, std::uint32_t at) {
        return std::int8_t(call(0x1014c2e0, at, {club, 0, side_flag, manager, 0})) != 0;
    }

    // The larger of `best` and the manager's speed setting (the original asks twice when it takes the setting).
    std::uint32_t slower(std::uint32_t best, std::uint32_t manager, std::uint32_t at) {
        const auto setting = std::int16_t(call(0x10147bc0, at, {c.u32(managers), manager}));
        if (std::int32_t(best & 0xff) > setting) return best & 0xff;
        return std::uint32_t(std::int16_t(call(0x10147bc0, at + 0x20, {c.u32(managers), manager}))) & 0xff;
    }
    void attach_viewer(std::uint32_t fixture, std::uint32_t manager, std::uint32_t club, std::uint32_t at) {
        c.put32(call(0x102aa3d0, at, {manager}) + 0xec, fixture);
        c.set(15, club);
        c.put32(call(0x102aa3d0, at + 0x10, {manager}) + 0xf0, club);
    }

    // Part 1 for one fixture (r20): its viewer record and the managers who watch it. False: nothing to do.
    bool prepare_human_fixture(std::uint32_t fixture) {
        const auto kind = c.i8(fixture + 0x47);
        if (kind == -2 || kind > -1 || c.i8(fixture + 0x54) == 0) return false;
        call(0x10430090, 0x101cba70, {c.u32(calendar), fixture + 4, 0xffffffff, 0});
        c.put32(fixture + 0x5c, call(0x10773520, 0x101cba78, {0x58}));
        auto viewers = c.u32(fixture + 0x5c);
        if (!viewers) { out_of_memory(0x101cba8c, 0xe8, 0x70, 0x24d); return false; }
        c.put8(viewers, 0);
        for (const auto offset : {0x8u, 0x18u, 0x28u, 0x38u})
            call(0x10778300, offset == 8 ? 0x101cbaf8 : 0x101cbaf8 + (offset - 8) / 0x10 * 0x14, {c.u32(fixture + 0x5c) + offset, 0, 0x10});
        viewers = c.u32(fixture + 0x5c);
        c.set(18, 0); c.set(17, 0);
        for (const auto offset : {0x48u, 0x49u, 0x4au, 0x4bu}) c.put8(viewers + offset, 0);
        c.put32(viewers + 0x4c, 0); c.put32(viewers + 0x50, 0);
        c.put8(viewers + 0x54, 1); c.put8(viewers + 0x55, 0); c.put8(viewers + 0x56, 0);
        std::uint32_t playing = 0, watching = 0;      // r18, r17: slowest speed among each kind of viewer
        for (std::uint32_t n = 0; n < 0x10; ++n) {
            c.set(21, n);
            const auto manager = manager_from_end(n);
            c.set(19, manager);
            if (c.i8(manager + 0x6b) == 0) continue;
            if (human(c.u32(fixture + 0x20), c.u8(fixture + 0x44), manager, 0x101cbbcc)) {
                attach_viewer(fixture, manager, c.u32(fixture + 0x20), 0x101cbbdc);
                call(0x1001dae0, 0x101cbc18, {manager, c.i8(fixture + 0x44) == 0 ? 1u : 0u, 0, 0, 0});
                playing = slower(playing, manager, 0x101cbc24); c.set(18, playing);
            } else if (human(c.u32(fixture + 0x24), c.u8(fixture + 0x45), manager, 0x101cbc68)) {
                attach_viewer(fixture, manager, c.u32(fixture + 0x24), 0x101cbc78);
                call(0x1001dae0, 0x101cbcb4, {manager, c.i8(fixture + 0x45) == 0 ? 1u : 0u, 0, 0, 0});
                playing = slower(playing, manager, 0x101cbcc0); c.set(18, playing);
            } else {
                if (std::int8_t(call(0x101494b0, 0x101cbcfc, {c.u32(managers), fixture + 4, manager})) == 0) continue;
                if (std::int8_t(call(0x10147e70, 0x101cbd10, {c.u32(managers), manager})) != 0)
                    call(0x10147fd0, 0x101cbd28, {c.u32(managers), 0, manager});
                c.put32(call(0x102aa3d0, 0x101cbd30, {manager}) + 0xec, fixture);
                c.set(15, 0);
                c.put32(call(0x102aa3d0, 0x101cbd40, {manager}) + 0xf0, 0);
                call(0x1029d080, 0x101cbd4c, {manager});
                watching = slower(watching, manager, 0x101cbd58); c.set(17, watching);
            }
            auto slot = std::int16_t(call(0x1073ffe0, 0x101cbd8c, {screen, manager}));
            if (c.i8(c.u32(fixture + 0x5c) + std::uint32_t(slot + 8)) == 0) {
                c.set(15, 1);
                slot = std::int16_t(call(0x1073ffe0, 0x101cbdb4, {screen, manager}));
                c.put8(c.u32(fixture + 0x5c) + std::uint32_t(slot + 8), 1);
                const auto record = c.u32(fixture + 0x5c);
                c.put8(record + 0x48, c.u8(record + 0x48) + 1u);
            }
            c.set(15, 1);
            slot = std::int16_t(call(0x1073ffe0, 0x101cbde4, {screen, manager}));
            c.put8(sp() + 0x78 + std::uint32_t(std::int32_t(slot)), 1);
        }
        c.set(21, 0x10);
        for (std::uint32_t n = 0; n < 0x10; ++n) {
            c.set(15, n);
            const auto manager = manager_from_end(n);
            c.set(19, manager);
            if (c.i8(manager + 0x6b) == 0) continue;
            if (c.u32(call(0x102aa3d0, 0x101cbe2c, {manager}) + 0xec) != fixture) continue;
            if (playing & 0xff) call(0x10147d20, 0x101cbe4c, {c.u32(managers), playing & 0xff, manager});
            else call(0x10147d20, 0x101cbe60, {c.u32(managers), watching & 0xff, manager});
        }
        if (!c.u32(self + 0x38)) c.put32(self + 0x38, fixture);
        return true;
    }

    // Plays one match to its end; the count of finished matches (this+0x28) goes up here in the original.
    void play(std::uint32_t fixture, std::uint32_t at, bool count) {
        while (!finished(c.i8(fixture + 0x68))) {
            c.put8(fixture + 0x68, call(0x101d3f60, at, {c.u32(fixture + 0x58), 0xffffffff}));
            if (count && finished(c.i8(fixture + 0x68))) c.put32(self + 0x28, c.u32(self + 0x28) + 1);
        }
    }

    // Part 1: the matches human managers play or watch, with their screens.
    void human_matches() {
        for (std::uint32_t n = 0; n < 0x10; ++n) {
            c.set(15, n);
            const auto manager = manager_of_last16(n);
            c.set(17, manager);
            if (std::int16_t(call(0x1073ffe0, 0x101cbed4, {screen, manager})) == -1) continue;
            const auto slot = std::int16_t(call(0x1073ffe0, 0x101cbeec, {screen, manager}));
            if (c.i8(sp() + 0x78 + std::uint32_t(std::int32_t(slot))) != 0) continue;
            call(0x1028eca0, 0x101cbf10, {manager, c.u32(c.u32(self + 0x38) + 0x64)});
        }
        call(0x1073c730, 0x101cbf24, {screen});
        c.set(18, 0); c.set(17, 0);
        for (std::uint32_t n = 0; n < 0x10; ++n) {
            c.set(15, n);
            const auto manager = manager_of_last16(n);
            c.set(19, manager);
            c.put32(call(0x102aa3d0, 0x101cbf50, {manager}) + 0xec, 0);
            const auto record = call(0x102aa3d0, 0x101cbf5c, {manager});
            c.set(15, n + 1);
            c.put32(record + 0xf0, 0);
        }
        c.set(17, 0); c.set(15, 0);
        for (std::int32_t n = 0; n < fixture_count(); c.set(17, std::uint32_t(++n)), c.set(15, std::uint32_t(n) * fixture_size)) {
            const auto fixture = fixtures() + std::uint32_t(n) * fixture_size;
            c.set(18, fixture);
            if (c.i8(fixture + 0x54) == 0 || !c.u32(fixture + 0x58)) continue;
            play(fixture, 0x101cbfa8, true);
            call(0x1029e9e0, 0x101cbff8, {c.u32(post_match), c.u32(fixture + 0x58), c.u32(fixture + 0x60)});
            call(0x101d3e50, 0x101cc004, {c.u32(fixture + 0x58), 1});
            c.put32(fixture + 0x58, 0);
            call(0x10430550, 0x101cc018, {c.u32(calendar), fixture + 4});
            call(0x107735a0, 0x101cc020, {c.u32(fixture + 0x5c)});
            c.put32(fixture + 0x5c, 0);
        }
    }

    // Between two background matches while the progress screen shows: the host's events, and the managers'
    // screens redrawn when something asked for it.
    void keep_screen_alive() {
        if (c.i8(c.toc(-0x745c)) == 0) return;
        call(0x1076e920, 0x101cc080, {2});
        call(0x1076fe80, 0x101cc084);
        if (c.i8(redraw) == 0) return;
        bool any = false;
        for (std::uint32_t n = 0; n < 0x10; ++n) {
            c.set(18, n);
            const auto manager = manager_from_end(n);
            if (c.i8(manager + 0x6b) == 0) continue;
            if (c.u32(manager + 0x24) || c.u32(manager + 0x20)) call(0x1001dae0, 0x101cc0e8, {manager, 0, 0, 0, 0});
            else call(0x102ce710, 0x101cc0f4, {manager, 0});
            any = true;
        }
        c.set(18, 0x10);
        if (any) call(0x1073c730, 0x101cc114, {screen});
        c.put8(redraw, 0);
    }

    enum class Prepared { skipped, human, ready };
    // A background fixture (r18): its match object, created and set up. `human`: a manager's club plays it
    // and the day has to start over through the scheduled-match routine.
    Prepared prepare(std::uint32_t fixture) {
        if (c.i8(fixture + 0x47) != -1 || c.u32(fixture + 0x58)) return Prepared::skipped;
        if ((human(c.u32(fixture + 0x20), c.u8(fixture + 0x44), 0, 0x101cc160) ||
             human(c.u32(fixture + 0x24), c.u8(fixture + 0x45), 0, 0x101cc180)) && c.i8(c.toc(-0x642c)) != 0)
            return Prepared::human;
        call(0x10430090, 0x101cc1c8, {c.u32(calendar), fixture + 4, 0xfffffffe, 0});
        const auto match = call(0x10773520, 0x101cc1d4, {0xe0bc});
        c.set(19, match);
        if (match) call(0x101d16a0, 0x101cc1e0);
        c.put32(fixture + 0x58, match);
        if (!c.u32(fixture + 0x58)) { out_of_memory(0x101cc1f4, 0xc8, 0x68, 0x38e); return Prepared::skipped; }
        const auto details = call(0x101ce4d0, 0x101cc24c, {fixture + 4});
        call(0x101d1760, 0x101cc264, {c.u32(fixture + 0x58), fixture + 4, details, 0, 0});
        return Prepared::ready;
    }

    std::int32_t percent_played() const {
        const auto count = fixture_count();
        return count ? std::int32_t(std::int64_t(c.i32(self + 0x28)) * 100 / count) : 0;
    }
    // The result line of the progress screen, in one of the original's four forms.
    void list_result(std::uint32_t fixture) {
        const auto competition = c.u32(fixture + 0x18), home = c.u32(fixture + 0x20), away = c.u32(fixture + 0x24);
        const bool penalties = c.i8(fixture + 0x4b) > -1;
        const auto percent = std::uint32_t(std::int32_t(std::int8_t(percent_played())));
        const auto home_kind = std::uint32_t(std::int32_t(c.i8(home + 0x23b))), away_kind = std::uint32_t(std::int32_t(c.i8(away + 0x23b)));
        const auto home_score = std::uint32_t(std::int32_t(c.i8(fixture + (penalties ? 0x4b : 0x47))));
        const auto away_score = std::uint32_t(std::int32_t(c.i8(fixture + (penalties ? 0x4c : 0x48))));
        const auto arguments = sp() + 0x38;
        if (competition) {
            const auto level = std::uint32_t(std::int32_t(c.i8(competition + 0x6a)));
            c.put32(arguments, competition + 0x1a); c.put32(arguments + 4, home + 0x221); c.put32(arguments + 8, home_score);
            c.put32(arguments + 12, away + 0x221); c.put32(arguments + 16, away_score); c.put32(arguments + 20, percent);
            c.put32(arguments + 24, strings + 0xba);
            if (!penalties) c.set(19, home_score);
            call(0x10727890, penalties ? 0x101cc430 : 0x101cc4cc, {text, strings + 0x21, level, level, home_kind, home_kind, away_kind, away_kind});
        } else {
            c.put32(arguments, away + 0x221); c.put32(arguments + 4, away_score); c.put32(arguments + 8, percent);
            c.put32(arguments + 12, strings + 0xba);
            if (penalties) c.set(18, away_score);
            call(0x10727890, penalties ? 0x101cc558 : 0x101cc5d8, {text, strings + 0xbc, home_kind, home_kind, away_kind, away_kind, home + 0x221, home_score});
        }
        call(0x10743120, 0x101cc5f0, {screen, text, 0xffffffff, 0, 0xffffffff});
        listed = true; c.set(20, 1);
    }
    // The number of contact notes the injury manager holds (the list at +0x28, its count a halfword).
    std::int32_t contact_notes() const {
        const auto manager = c.u32(c.toc(-0x7ba0)), list = manager ? c.u32(manager + 0x28) : 0;
        return list ? c.u16(list) : 0;
    }
    // CM_MATCH_LOG=FILE: one line per background match the driver finishes, for comparing the orders' results
    // (TASKS.md, step 6): game day, competition, the two clubs, goals, penalties (-1: none); then the four
    // counters of the match the referee's grade is made from (0x101fd900: two per side, taken to be sendings-off
    // and cautions), the contact notes the play added (-1: not counted), and each side's formation in the
    // tactics object as the match ends.
    void log_result(std::uint32_t fixture, std::int32_t notes) {
        static std::FILE* const file = [] { const char* path = std::getenv("CM_MATCH_LOG"); return path && *path ? std::fopen(path, "a") : nullptr; }();
        if (!file) return;
        const auto match = c.u32(fixture + 0x58);
        const tactics::Tactics table(c, c.u32(c.toc(-0x7a6c)));
        // A club's first word is its index, the slot of its team in the tactics object (as TACTICS 0x103af130 reads it).
        const auto formation = [&](std::uint32_t club) {
            return club && c.i32(club) >= 0 && c.i32(club) < table.team_count() ? int(table.team(c.i32(club)).formation()) : -1;
        };
        std::fprintf(file, "%u %d %08x %08x %d %d %d %d %d %d %d %d %d %d %d\n", unsigned(c.u16(c.u32(GuestScratch::game_toc - 0x7b80))), c.i32(fixture + 4),
                     c.u32(fixture + 0x20), c.u32(fixture + 0x24), c.i8(fixture + 0x47), c.i8(fixture + 0x48), c.i8(fixture + 0x4b), c.i8(fixture + 0x4c),
                     c.u8(match + 0xe04b), c.u8(match + 0xe04c), c.u8(match + 0xe081), c.u8(match + 0xe082), notes,
                     formation(c.u32(fixture + 0x20)), formation(c.u32(fixture + 0x24)));
        std::fflush(file);
    }
    // After the last minute: the result record, the post-match routine, the match object released.
    void finish(std::uint32_t fixture, std::uint32_t first_byte, std::int32_t notes) {
        const auto result = call(0x10773520, 0x101cc2c8, {0xdfc});
        c.set(21, result);
        if (result) call(0x10773320, 0x101cc2e8, {result + 0x120, c.toc(-0x7acc), 0, 0x664, 2});
        c.put32(fixture + 0x60, result);
        if (!c.u32(fixture + 0x60)) { out_of_memory(0x101cc2fc, 0xa8, 0x60, 0x3a6); return; }
        call(0x1029e9e0, 0x101cc358, {c.u32(post_match), c.u32(fixture + 0x58), c.u32(fixture + 0x60)});
        log_result(fixture, notes);
        call(0x101d3e50, 0x101cc364, {c.u32(fixture + 0x58), 1});
        c.put32(fixture + 0x58, 0);
        call(0x10430550, 0x101cc378, {c.u32(calendar), fixture + 4});
        if (std::int8_t(first_byte) == 0) list_result(fixture);
    }

    // Part 3: files each result of the day and tells the human managers' clubs.
    void file_results() {
        c.set(14, 0); c.set(13, 0);
        for (std::int32_t n = 0; n < fixture_count(); c.set(14, std::uint32_t(++n)), c.set(13, std::uint32_t(n) * fixture_size)) {
            const auto fixture = fixtures() + std::uint32_t(n) * fixture_size;
            c.set(15, fixture);
            if (c.i8(fixture + 0x47) == -2) continue;
            if (!c.u32(fixture + 0x60)) { out_of_memory(0x101cc658, 0x88, 0x58, 0x3ec); continue; }
            call(0x101cd880, 0x101cc6b4, {self, fixture});
            for (const auto [club_at, flag_at, at] : {std::array{0x20u, 0x44u, 0x101cc6ccu}, std::array{0x24u, 0x45u, 0x101cc73cu}}) {
                if (!human(c.u32(fixture + club_at), 1, 0, at)) continue;
                if (!call(0x100c96e0, at + 0x10, {c.u32(fixture + club_at)})) continue;
                const auto person = call(0x100c96e0, at + 0x20, {c.u32(fixture + club_at)});
                call(0x10146830, at + 0x44, {c.u32(managers), c.i8(fixture + flag_at) == 0 ? 1u : 0u, c.u32(fixture + club_at), person});
                call(0x101466c0, at + 0x54, {c.u32(managers), c.u32(c.u32(fixture + club_at) + 0x90)});
                call(0x10114ba0, at + 0x58);
            }
            call(0x107735a0, 0x101cc79c, {c.u32(fixture + 0x60)});
            c.put32(fixture + 0x60, 0);
        }
    }

    struct Pending { std::uint32_t fixture, first_byte; std::unique_ptr<PlayContext> play; };
    // The plays of a stretch on the workers, each taking the next match until none is left. A match's RNG
    // stream comes from the game's cursor and salt as set-up left them and from its fixture, so the result
    // does not depend on which worker plays it or on how many there are. The game thread only waits.
    void play_on_workers(std::vector<Pending>& pending) {
        if (pending.empty()) return;
        auto& list = guest_workers(c.s);
        const std::uint64_t base = std::uint64_t(c.u32(c.toc(-0x7b84))) << 32 | c.u32(c.toc(-0x7b90));   // the cursor and the salt
        const auto first = fixtures();
        std::atomic<std::size_t> next{0};
        const auto job = [&](State& worker) {
            Cpu w{worker};
            for (;;) {
                const auto n = next.fetch_add(1);
                if (n >= pending.size()) return;
                auto& match = pending[n];
                begin_play(worker, *match.play, base ^ (std::uint64_t((match.fixture - first) / fixture_size + 1) * UINT64_C(0x9e3779b97f4a7c15)));
                // Ends the play on every path; its own failure (a play that ended on another TOC) is not thrown over another.
                struct End {
                    State& s; PlayContext& play; int unwinding = std::uncaught_exceptions();
                    ~End() noexcept(false) { try { end_play(s, play); } catch (...) { if (std::uncaught_exceptions() == unwinding) throw; } }
                } end{worker, *match.play};
                while (!finished(w.i8(match.fixture + 0x68))) {
                    w.set({{3, w.u32(match.fixture + 0x58)}, {4, 0xffffffff}, {18, match.fixture}});
                    w.call(0x101d3f60, 0x101cc284);
                    w.put8(match.fixture + 0x68, w.r(3));
                }
            }
        };
        if (list.empty()) throw std::logic_error("No guest worker to play the matches on");
        const auto used = std::min(list.size(), pending.size());
        std::size_t started = 0;
        std::exception_ptr failure;
        try { for (; started < used; ++started) list[started]->start(job); } catch (...) { failure = std::current_exception(); }
        // The game thread only waits: the original's event pump between matches runs guest code, which must not
        // meet the plays. It runs between stretches (keep_screen_alive, in the set-up loop).
        for (std::size_t n = 0; n < started; ++n)
            try { list[n]->wait(); } catch (...) { if (!failure) failure = std::current_exception(); }
        if (failure) std::rethrow_exception(failure);
        for (const auto& match : pending) report_private_writes(*match.play);
    }
    // phases: the matches set up so far are played, then finished in fixture order.
    void flush(std::vector<Pending>& pending) {
        bool first = true;
        if (order == MatchDayOrder::parallel) play_on_workers(pending);
        else for (const auto& match : pending) {
            // As between two matches of the original: the host's events, so the window stays alive.
            if (!first) keep_screen_alive();
            first = false;
            c.set(18, match.fixture); play(match.fixture, 0x101cc280, false);
        }
        for (const auto& match : pending) {
            c.set(18, match.fixture); c.set(19, match.first_byte);
            c.put32(self + 0x28, c.u32(self + 0x28) + 1);
            // What the play held back, on the frame's unused report buffer (0x88, 32 bytes).
            if (match.play) replay_deferred(c, *match.play, sp() + 0x88);
            c.set(18, match.fixture); c.set(19, match.first_byte);
            finish(match.fixture, match.first_byte, match.play ? std::int32_t(match.play->notes.size()) : -1);
        }
        pending.clear();
    }

    void run() {
        self = c.r(3);
        text = c.toc(-0x7bb8); post_match = c.toc(-0x7b4c); managers = c.toc(-0x7a54); manager_count = c.toc(-0x7be4);
        manager_base = c.toc(-0x7be0); screen = c.toc(-0x7bb0); calendar = c.toc(-0x7b20); redraw = c.toc(-0x7bac);
        message = c.toc(-0x7ba8); strings = c.toc(-0x52f0);
        c.set({{24, text}, {16, self}, {13, post_match}, {25, managers}, {26, manager_count}, {27, manager_base}, {28, screen},
               {14, calendar}, {29, redraw}, {30, message}, {31, strings}});
        for (bool again = true; again;) {
            again = false;
            call(0x101cd350, 0x101cba10, {self});
            c.set(15, 0);
            call(0x10778300, 0x101cba24, {sp() + 0x78, 0, 0x10});
            bool any_human = false;
            std::int32_t n = 0;
            c.set(22, 0); c.set(23, 0);
            for (; c.i8(c.toc(-0x642c)) != 0 && n < fixture_count(); ++n) {
                const auto fixture = fixtures() + std::uint32_t(n) * fixture_size;
                c.set(20, fixture);
                if (prepare_human_fixture(fixture)) any_human = true;
                c.set(15, any_human ? 1 : 0);
                c.set(22, std::uint32_t(n) + 1); c.set(23, (std::uint32_t(n) + 1) * fixture_size);
            }
            if (any_human) human_matches();
            c.put8(c.toc(-0x6430), 1);
            c.put8(redraw, 0);
            listed = false; c.set(20, 0);
            const auto cursor = call(0x1076e910, 0x101cc058);
            c.set(22, cursor);
            std::vector<Pending> pending;
            c.set(17, 0); c.set(15, 0);
            for (n = 0; n < fixture_count(); c.set(17, std::uint32_t(++n)), c.set(15, std::uint32_t(n) * fixture_size)) {
                keep_screen_alive();
                const auto fixture = fixtures() + std::uint32_t(n) * fixture_size;
                c.set(18, fixture);
                const auto prepared = prepare(fixture);
                if (prepared == Prepared::skipped) continue;
                if (prepared == Prepared::human) {
                    flush(pending);
                    c.set(18, fixture);
                    call(0x101cd5c0, 0x101cc1a4, {self, 1});
                    c.put8(c.toc(-0x6430), 0);
                    again = true;
                    break;
                }
                const auto first_byte = std::uint32_t(c.u8(c.u32(fixture + 0x58)));
                c.set(19, first_byte);
                if (order == MatchDayOrder::phases || order == MatchDayOrder::parallel) {
                    pending.push_back({fixture, first_byte, order == MatchDayOrder::parallel ? std::make_unique<PlayContext>() : nullptr});
                    if (pending.size() >= stretch_limit) flush(pending);
                    continue;
                }
                const auto notes_before = contact_notes();
                play(fixture, 0x101cc280, true);
                finish(fixture, first_byte, contact_notes() - notes_before);
            }
            if (again) continue;
            flush(pending);
            call(0x1076e920, 0x101cc610, {cursor});
            call(0x101cd5c0, 0x101cc61c, {self, 1});
            c.put8(c.toc(-0x6430), 0);
            file_results();
            if (listed) {
                call(0x10716eb0, 0x101cc7d8, {c.toc(-0x7b80), text, 0x7d0, c.u8(c.toc(-0x79ec))});
                call(0x10743120, 0x101cc7f0, {screen, text, 0xffffffff, 0, 0xffffffff});
            }
        }
        c.set(3, 1);
    }
};
}

MatchDayOrder match_day_order() { return order().load(std::memory_order_relaxed); }
void set_match_day_order(MatchDayOrder value) { order().store(value, std::memory_order_relaxed); }

bool try_modern_match_day_driver(State& s, std::uint32_t entry) {
    const auto mode = match_day_order();
    if (mode == MatchDayOrder::original || entry != entry_address) return false;
    Cpu c{s};
    guest::Frame frame{c, entry, 0x160, guest::Saves{.first_gpr = 13}};
    Driver driver{c, frame, mode};
    driver.run();
    frame.leave();
    c.finish();
    return true;
}
}

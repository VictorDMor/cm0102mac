#include "shortlist_manager_candidates_scan.hpp"
#include "guest_scratch.hpp"
#include "guest_worker_pool.hpp"
#include "match_day_driver.hpp"
#include "match_play.hpp"
#include "shortlist_manager_adapter.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <exception>

// The eligibility of a scan's list entries judged on guest workers (TASKS.md, "The shortlist manager's 37%"),
// part of the game's Parallel Matches setting (see enabled()). A worker has the matches' private context (stack, scratch heap, TOC copy
// with its own RNG words and scratch statics) and a copy of the scan's frame and registers as the walk finds
// them; each entry is judged with its own RNG stream. What judging writes outside that context was measured on
// the reference week and live on the multi-league career: the transfer manager's catch-up of a row's history
// (one worker at a time there) and the cursors embedded in shared lists, which are not kept apart: two workers
// stepping one list at once can make a verdict wrong, never a pointer (a cursor holds a node or null, and every
// reader resets it before use).
namespace cm::aot::shortlist_manager::candidates {
namespace {
// On with the game's Parallel Matches setting (the day driver's parallel order; Victor, 2026-11-25), so replays,
// checkers and a session that records a capture from launch never use it. CM_SHORTLIST_PARALLEL=0 turns it off
// alone (for comparisons).
bool enabled() {
    static const bool refused = [] { const char* value = std::getenv("CM_SHORTLIST_PARALLEL"); return value && value[0] == '0' && !value[1]; }();
    return !refused && match_day_order() == MatchDayOrder::parallel;
}
constexpr std::size_t fewest = 256;                    // below this a scan is judged in place
constexpr std::uint32_t frame_size = 0x1b0;
// What a worker's copy of the frame takes from the game thread's: the scan's locals (not the register saves and
// links below them, which are the worker's own) and the caller's parameter word eligibility reads.
constexpr std::uint32_t locals_from = 0x38, locals_to = 0x140;
constexpr std::array<unsigned, 9> kept{13, 14, 18, 19, 20, 21, 22, 23, 25};
bool said = false;
// The staff score on the workers too (Verdict::score); CM_SHORTLIST_STAFF_AHEAD=0 leaves it in the walk (for
// timing comparisons).
const bool score_ahead = [] { const char* value = std::getenv("CM_SHORTLIST_STAFF_AHEAD"); return !(value && value[0] == '0' && !value[1]); }();
const std::uint32_t score_control = [] { const char* value = std::getenv("CM_SHORTLIST_SCORE_CHECK"); return value && value[0] == '2' ? 1u : 0u; }();
}

void Scan::capture(Verdict& verdict) const {
    for (std::size_t n = 0; n < kept.size(); ++n) verdict.registers[n] = c.r(kept[n]);
    verdict.f29 = c.s.r<8>(guest::reg::fpr(29));
    verdict.f30 = c.s.r<8>(guest::reg::fpr(30));
    const auto bytes = c.s.memory.copy_out(at(0x38), verdict.locals.size());
    std::copy(bytes.begin(), bytes.end(), verdict.locals.begin());
    verdict.current_contract = c.u32(c.toc(slot::current_contract));
}
void Scan::restore(const Verdict& verdict) {
    for (std::size_t n = 0; n < kept.size(); ++n) c.set(kept[n], verdict.registers[n]);
    c.s.wr<8>(guest::reg::fpr(29), verdict.f29);
    c.s.wr<8>(guest::reg::fpr(30), verdict.f30);
    // The locals around the quote object (0x78-0xdc), which the scan's set-up built and judging does not change:
    // its bytes stay the game thread's own.
    c.s.memory.copy_in(at(0x38), std::vector<std::uint8_t>(verdict.locals.begin(), verdict.locals.begin() + (local::quote - 0x38)));
    c.s.memory.copy_in(at(0xdc), std::vector<std::uint8_t>(verdict.locals.begin() + (0xdc - 0x38), verdict.locals.end()));
    // The scratch word judging publishes the candidate's contract in (the worker wrote its own copy).
    c.put32(c.toc(slot::current_contract), verdict.current_contract);
}

void Scan::check_staff_score(std::uint32_t ahead) {
    static const bool on = [] { const char* value = std::getenv("CM_SHORTLIST_SCORE_CHECK"); return value && (value[0] == '1' || value[0] == '2'); }();
    if (!on) return;
    static std::uint64_t checked = 0, differ = 0;
    const auto in_place = staff_score();
    ++checked;
    if (in_place != ahead && ++differ <= 20)
        std::fprintf(stderr, "Staff score differs: person 0x%08x club 0x%08x in place %d, from the worker %d\n", unsigned(person),
                     unsigned(club_address), int(in_place), int(ahead));
    if ((checked & (checked - 1)) == 0 || (checked % 50000) == 0)
        std::fprintf(stderr, "Staff scores checked: %llu, differing: %llu\n", (unsigned long long)checked, (unsigned long long)differ);
}

bool Scan::judge_on_workers() {
    verdicts.clear(); judged_entries.clear(); next_verdict = 0;
    // Not under a diagnostic that watches the judging or the memory (they follow one thread).
    if (!enabled() || play_context(c.s) || shortlist_candidate_hook || c.s.memory.date_observer || c.s.memory.read_trace) return false;
    // The entries in the walk's order (Scan::walk, read only).
    for (std::int32_t category = std::int8_t(c.u8(at(local::category))), offset = c.i32(at(local::category_offset));
         category <= c.i32(at(local::last_category)); ++category, offset += 4) {
        const auto list = c.u32(manager + c.u32(at(local::lists_offset)) + std::uint32_t(offset) + 0x18);
        if (c.i32(list) == -1) break;
        const std::int32_t count = c.i32(manager + std::uint32_t(offset) + 0x98);
        for (std::int32_t index = 0; index < count; ++index) judged_entries.push_back(list + std::uint32_t(index) * 12);
    }
    if (judged_entries.size() < fewest) { judged_entries.clear(); return false; }
    auto& workers = guest_workers(c.s);
    verdicts.resize(judged_entries.size());
    const auto frame_locals = c.s.memory.copy_out(at(locals_from), locals_to - locals_from);
    const auto excluded = c.u32(at(local::excluded));
    // The streams come from the game's own RNG state as this scan finds it and from the row it fills: the same
    // save and day give the same verdicts.
    const std::uint64_t base = (std::uint64_t(c.u32(cursor_slot)) << 32 | c.u32(salt_slot)) ^ (std::uint64_t(row.address) * UINT64_C(0x9e3779b97f4a7c15));
    const auto row_address_now = row.address, club_address_now = club.address;
    const std::size_t used = std::min<std::size_t>(workers.size(), judged_entries.size() / 64);
    std::atomic<std::size_t> next_slice{0};
    const auto job = [&](State& worker) {
        Cpu w{worker};
        PlayContext context;
        begin_play(worker, context, base);
        struct End { State& s; PlayContext& context; ~End() { try { end_play(s, context); } catch (...) {} } } end{worker, context};
        guest::Frame own(w, entry, frame_size, {.first_gpr = 13, .gpr_offset = -0x6c, .first_fpr = 29});
        worker.memory.copy_in(own.at(locals_from), frame_locals);
        w.put32(own.at(local::excluded), excluded);
        Scan scan(w, own, row_address_now, club_address_now);
        // The RNG's variables are reached through the worker's TOC copy.
        scan.salt_slot = w.toc(slot::random_salt);
        scan.cursor_slot = w.toc(slot::random_cursor);
        for (;;) {
            const auto slice = next_slice.fetch_add(1);
            const auto first = slice * 64;
            if (first >= judged_entries.size()) break;
            for (auto n = first; n < std::min(first + 64, judged_entries.size()); ++n) {
                seed_stream(worker, base + n * UINT64_C(0xbf58476d1ce4e5b9));
                if (!scan.eligible(judged_entries[n])) continue;
                verdicts[n].eligible = true;
                scan.capture(verdicts[n]);
                if (score_ahead && !nonzero(w, std::int8_t(scan.player_row))) {
                    verdicts[n].score = scan.staff_score(score_control);
                    verdicts[n].scored = true;
                }
            }
        }
        own.leave();
    };
    std::size_t started = 0;
    std::exception_ptr failure;
    try { for (; started < used; ++started) workers[started]->start(job); } catch (...) { failure = std::current_exception(); }
    for (std::size_t n = 0; n < started; ++n)
        try { workers[n]->wait(); } catch (...) { if (!failure) failure = std::current_exception(); }
    if (failure) {
        // The scan is then judged in place, as without workers; said once, since it should not happen.
        if (!said) {
            said = true;
            try { std::rethrow_exception(failure); }
            catch (const std::exception& error) { std::fprintf(stderr, "Shortlist judging on workers failed (%s): judged in place\n", error.what()); }
            catch (...) { std::fprintf(stderr, "Shortlist judging on workers failed: judged in place\n"); }
        }
        verdicts.clear(); judged_entries.clear();
        return false;
    }
    return true;
}
}

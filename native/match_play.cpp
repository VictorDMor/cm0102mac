#include "match_play.hpp"
#include "guest_abi.hpp"
#include "guest_scratch.hpp"
#include "rng_core.hpp"

#include <algorithm>
#include <cstdio>
#include <mutex>
#include <set>
#include <stdexcept>

namespace cm::aot {
namespace {
constexpr std::uint32_t toc = GuestScratch::game_toc;
// TOC cells (offsets from r2): the RNG's salt cell, pool start, and cursor cell; the C runtime's state is a
// word of the small data itself.
constexpr std::int32_t salt_cell = -0x7b90, pool_slot = -0x7b8c, cursor_cell = -0x7b84, crt_state = -0x34c4;
constexpr std::uint32_t pool_salts = 65535;
// DATE.CPP's static buffers, 0x1098db80-0x1098dbd8, reached through the eight cells -0x3fc8..-0x3fac; and the
// word at 0x1091e154 (cell -0x7b78) MATCH_ENG 0x101d6a90 and MATCH_MAN 0x10211e70 both write.
constexpr std::uint32_t date_low = 0x1098db80, date_high = 0x1098dbd8;
constexpr std::int32_t date_first_cell = -0x3fc8, date_last_cell = -0x3fac, engine_cell = -0x7b78;
constexpr std::uint32_t engine_word = 0x1091e154;
// Where the copies sit in the worker's statics area.
constexpr std::uint32_t salt_at = 0, cursor_at = 4, engine_at = 8, date_at = 0x40;

std::uint64_t mix(std::uint64_t& state) {   // splitmix64
    auto z = (state += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}
std::recursive_mutex league_mutex;
}

void begin_play(State& s, PlayContext& play, std::uint64_t seed) {
    const auto range = GuestScratch::range_of(s);
    if (!range) throw std::logic_error("A play phase needs a worker with a scratch range");
    auto& memory = s.memory;
    const auto copy = range + GuestScratch::toc_copy, r2 = copy + 0x8000, statics = range + GuestScratch::statics;
    memory.copy_in(copy, memory.copy_out(toc - 0x8000, GuestScratch::toc_span));
    const auto cell = [&](std::int32_t offset) { return std::uint32_t(memory.read<4>(toc + std::uint32_t(offset))); };
    // The RNG stream of this match.
    const auto pool = cell(pool_slot);
    memory.write<4>(statics + cursor_at, pool + 2 * std::uint32_t(mix(seed) % rng::pool_samples));
    memory.write<4>(statics + salt_at, std::uint32_t(mix(seed) % pool_salts));
    memory.write<4>(r2 + std::uint32_t(crt_state), std::uint32_t(mix(seed)));
    memory.write<4>(r2 + std::uint32_t(salt_cell), statics + salt_at);
    memory.write<4>(r2 + std::uint32_t(cursor_cell), statics + cursor_at);
    // The scratch statics, as they stand.
    memory.copy_in(statics + date_at, memory.copy_out(date_low, date_high - date_low));
    for (auto offset = date_first_cell; offset <= date_last_cell; offset += 4) {
        const auto target = cell(offset);
        if (target < date_low || target >= date_high) throw std::logic_error("DATE's static cells are not where expected");
        memory.write<4>(r2 + std::uint32_t(offset), statics + date_at + (target - date_low));
    }
    if (cell(engine_cell) != engine_word) throw std::logic_error("The match engine's static cell is not where expected");
    memory.write<4>(statics + engine_at, memory.read<4>(engine_word));
    memory.write<4>(r2 + std::uint32_t(engine_cell), statics + engine_at);
    s.wr<4>(guest::reg::gpr(2), r2);
    GuestScratch::set_play(s, &play);
}
void seed_stream(State& s, std::uint64_t seed) {
    const auto range = GuestScratch::range_of(s);
    const auto r2 = range + GuestScratch::toc_copy + 0x8000, statics = range + GuestScratch::statics;
    const auto pool = std::uint32_t(s.memory.read<4>(toc + std::uint32_t(pool_slot)));
    s.memory.write<4>(statics + cursor_at, pool + 2 * std::uint32_t(mix(seed) % rng::pool_samples));
    s.memory.write<4>(statics + salt_at, std::uint32_t(mix(seed) % pool_salts));
    s.memory.write<4>(r2 + std::uint32_t(crt_state), std::uint32_t(mix(seed)));
}
void end_play(State& s, PlayContext& play) {
    GuestScratch::set_play(s, nullptr);
    const auto range = GuestScratch::range_of(s);
    const auto r2 = range + GuestScratch::toc_copy + 0x8000;
    // The play's callees restore the r2 they were called with; anything else means code ran on another TOC.
    const bool own_toc = s.r<4>(guest::reg::gpr(2)) == r2;
    s.wr<4>(guest::reg::gpr(2), toc);
    if (!own_toc) throw std::logic_error("A play phase ended on another TOC than its own");
    // What the play wrote in its copy of the small data (the game's own is unchanged while plays run).
    const auto mine = s.memory.copy_out(range + GuestScratch::toc_copy, GuestScratch::toc_span);
    const auto game = s.memory.copy_out(toc - 0x8000, GuestScratch::toc_span);
    for (std::uint32_t at = 0; at < GuestScratch::toc_span; at += 4) {
        if (std::equal(mine.begin() + at, mine.begin() + at + 4, game.begin() + at)) continue;
        const auto offset = std::int32_t(at) - 0x8000;
        if (offset == salt_cell || offset == cursor_cell || offset == crt_state || offset == engine_cell ||
            (offset >= date_first_cell && offset <= date_last_cell)) continue;
        play.private_writes.push_back(offset);
    }
}
void report_private_writes(const PlayContext& play) {
    static std::set<std::int32_t> seen;   // the game thread only
    for (const auto offset : play.private_writes)
        if (seen.insert(offset).second)
            std::fprintf(stderr, "play phase: small data at r2%+d (0x%08x) written by a play, kept with its TOC copy\n", offset, toc + std::uint32_t(offset));
}

void PlayContext::note(guest::Cpu& c, std::uint32_t key_at, std::uint32_t kind_at, std::uint32_t strength_at) {
    const auto key = c.u32(key_at);
    for (auto& entry : notes) {
        if (entry.key != key) continue;
        if (std::int8_t(entry.kind) == c.i8(kind_at)) {
            const std::int32_t sum = std::int8_t(entry.strength) + c.i8(strength_at);
            entry.strength = sum > 100 ? 100 : std::uint8_t(entry.strength + std::uint32_t(std::int32_t(c.i8(strength_at))));
        } else if (c.i8(strength_at) > std::int8_t(entry.strength)) {
            entry.strength = c.u8(strength_at);
            entry.kind = c.u8(kind_at);
        }
        c.put8(kind_at, entry.kind);
        c.put8(strength_at, entry.strength);
        return;
    }
    notes.push_back({key, c.u8(kind_at), c.u8(strength_at)});
}

void replay_deferred(guest::Cpu& c, const PlayContext& play, std::uint32_t cells) {
    // Return addresses: the original call sites of the three routines in the engine.
    for (const auto& note : play.notes) {
        c.put32(cells, note.key); c.put8(cells + 4, note.kind); c.put8(cells + 5, note.strength);
        c.set({{3, c.u32(c.toc(-0x7ba0))}, {4, cells}, {5, cells + 4}, {6, cells + 5}});
        c.call(0x10186cd0, 0x101f69e8);
    }
    for (const auto& grade : play.grades) {
        for (unsigned n = 0; n < 8; ++n) c.put8(cells + 8 + n, grade.record[n]);
        c.set({{3, grade.officials}, {4, cells + 8}});
        c.call(0x102d4590, 0x101fdb04);
    }
    for (const auto& result : play.results) {
        c.set({{3, result.tactics}, {4, result.fixture}});
        c.call(0x103af130, 0x101d4948);
    }
}

LeagueTurn::LeagueTurn(const State& s) : held(play_context(s) != nullptr) { if (held) league_mutex.lock(); }
LeagueTurn::~LeagueTurn() { if (held) league_mutex.unlock(); }
}

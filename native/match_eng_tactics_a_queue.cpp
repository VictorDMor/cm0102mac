// MATCH_ENG.CPP, tactics block A: the actor queue and its ordering, and the per-player sweeps.
#include "match_eng_tactics_a.hpp"
#include "guest_profiler.hpp"
#include "match_eng_adapter.hpp"
#include "match_turbo.hpp"
#include "native_drawing.hpp"
#include "routine_check.hpp"
#include <array>
#include <optional>
#include <utility>

namespace cm::aot::match_eng {
using namespace guest;
using namespace tactics_a;

namespace {
constexpr std::uint32_t queue_count_at = 0x792e;   // byte
constexpr std::uint32_t queue_at = 0x79f8;         // up to 40 player words
constexpr std::uint32_t pinned_at = 0x7c68;        // word: a player that always comes first

// Pool offsets (doubles at TOC -0x52c4).
constexpr std::uint32_t signed_bias = 0x60;

// The four-byte heapsort and descriptor thunk keep the original comparison
// order. Their frames are 0x40 + 0x70 bytes, then the comparator uses 0x90.
constexpr unsigned sort_frames = 0xb0, compare_frame = 0x90;
constexpr unsigned sort_depth = sort_frames + compare_frame, player_count = 40;
constexpr std::uint32_t actor_comparator = 0x101d91b0, actor_descriptor = 0x108b8c34;
constexpr unsigned score_status = 1280;

struct ScoreStatus {
    std::uint64_t flags, rounding, classification_and_invalid, enables;
    explicit ScoreStatus(const State& s)
        : flags(s.r<8>(score_status)), rounding(s.r<8>(1288)),
          classification_and_invalid(s.r<8>(1296)), enables(s.r<8>(1304)) {}
    bool same_inputs(const ScoreStatus& rhs) const {
        // The first subtraction replaces FPCC before any operation reads it.
        return flags == rhs.flags && rounding == rhs.rounding &&
               std::uint32_t(classification_and_invalid) == std::uint32_t(rhs.classification_and_invalid) &&
               enables == rhs.enables;
    }
    void publish(State& s) const {
        s.wr<8>(score_status, flags);
        s.wr<8>(1288, rounding);
        s.wr<8>(1296, classification_and_invalid);
    }
};

struct ActorScore {
    ScoreStatus before, after;
    std::uint64_t rating_scratch, form_scratch, result_bits;
    double form_value, ratings_value;
    std::uint32_t secondary_rating, primary_rating;
};

bool overlaps(std::uint32_t first, unsigned size, std::uint32_t begin, std::uint64_t end) {
    return first < end && std::uint64_t(first) + size > begin;
}

struct ActorQueueScores {
    const State* state;
    std::uint32_t match, sort_sp;
    std::uint32_t pool = 0;
    std::array<std::optional<ActorScore>, player_count> entries{};
    std::array<std::uint8_t, player_count> seen_slots{};
};

thread_local ActorQueueScores* active_scores = nullptr;

bool can_reuse_scores(Cpu& c, std::uint32_t match, unsigned count) {
    const auto& s = c.s;
    const auto& memory = s.memory;
    const auto sp = c.sp();
    const auto stack_end = std::uint64_t(sp) + 12;
    const auto match_end = std::uint64_t(match) + queue_at + player_count * 4;
    if (count < 2 || count > player_count || sp < sort_depth ||
        stack_end > (UINT64_C(1) << 32) || match_end > (UINT64_C(1) << 32) ||
        overlaps(match, unsigned(match_end - match), sp - sort_depth, stack_end) ||
        overlaps(actor_descriptor, 8, sp - sort_depth, stack_end) ||
        overlaps(actor_descriptor, 8, match + queue_at, match_end) ||
        memory.date_observer || memory.read_trace ||
        s.budget != UINT64_MAX || routine_checker || dispatch_probe || match_eng_call_observer ||
        match_eng_declining || &s == match_eng_original_state || match_pc_profile_enabled() ||
        s.import_descriptor_observer || guest_profiler::running())
        return false;

    const auto* observer = memory.observer;
    return !observer || (dynamic_cast<const NativeDrawingObserver*>(observer) &&
                         !observer->observes_write(sp - sort_depth, sort_depth + 12) &&
                         !observer->observes_write(match + queue_at, count * 4));
}

void sort_actor_queue(Cpu& c, std::uint32_t match, unsigned count, std::uint32_t descriptor) {
    c.set({{3, match + queue_at}, {4, count}, {5, 4}, {6, descriptor}});
    if (descriptor != actor_descriptor || !can_reuse_scores(c, match, count)) {
        call(c, 0x10768300, 0x101d9184);
        return;
    }
    ActorQueueScores scores{&c.s, match, c.sp()};
    struct Restore {
        ActorQueueScores* previous;
        ~Restore() { active_scores = previous; }
    } restore{std::exchange(active_scores, &scores)};
    call(c, 0x10768300, 0x101d9184);
}

ActorQueueScores* scores_for(Cpu& c, const Frame& frame, std::uint32_t pool) {
    auto* const scores = active_scores;
    const auto back = c.lr();
    if (!scores || scores->state != &c.s || frame.base() != scores->sort_sp - sort_depth ||
        (back != 0x107685cc && back != 0x107685f0) || !pool)
        return nullptr;

    if (scores->pool != pool) {
        // The thunk has already read this descriptor. Confirm it still
        // points straight to this comparator, with the current TOC.
        if (scores->pool || c.u32(actor_descriptor) != actor_comparator ||
            c.u32(actor_descriptor + 4) != c.r(2) || std::uint64_t(pool) + 0xf8 > (UINT64_C(1) << 32) ||
            overlaps(pool, 0xf8, scores->sort_sp - sort_depth, std::uint64_t(scores->sort_sp) + 12) ||
            overlaps(pool, 0xf8, scores->match + queue_at, std::uint64_t(scores->match) + queue_at + player_count * 4))
            return nullptr;
        scores->pool = pool;
    }
    return scores;
}

std::uint64_t integer_bits(std::int32_t value) {
    return UINT64_C(0x4330000000000000) | (std::uint32_t(value) ^ 0x80000000u);
}

// The queue's slot for the player: the count is a byte, the slot index sign-extended.
void enqueue(Cpu& c, std::uint32_t match, std::uint32_t player) {
    const auto count = c.u8(match + queue_count_at);
    c.put8(match + queue_count_at, count + 1u);
    c.put32(match + (u(s8(count)) << 2) + queue_at, player);
}

// The sweep of 0x101d93b0, 0x101d9460 and 0x101d9510: the 20 players of each side in turn.
// `visit` makes the call; `live_only` skips players off the pitch (position below 0).
template <class Visit>
void sweep_players(Cpu& c, std::uint32_t match, bool live_only, Visit visit) {
    auto previous_first = held<29>(c);
    auto previous_second = held<28>(c);
    const auto sweep = [&](std::uint32_t first, auto& previous, unsigned list) {
        Kept<31> player(c, first);
        Kept<30> index(c, 0);
        while (compare(c, s8(index), 0x14) < 0) {
            if (!live_only || compare(c, s8(position(c, player))) >= 0) visit(std::uint32_t(player), list);
            index = index + 1;
            previous = player;
            player = player + player_size;
        }
    };
    sweep(match + players_at, previous_first, 0);
    sweep(match + players_at + side_size, previous_second, 1);
}
}

// 0x101d9080
void build_actor_queue(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101d9080, 0x50, {.first_gpr = 28});
    keep(c, {{31, match}});
    c.put8(match + queue_count_at, 0);
    Kept<29> index(c, 0);
    auto side = held<28>(c);
    auto player = held<30>(c);
    while (compare(c, s8(index), 0x14) < 0) {
        // Both sides' player of this index join, in a random order.
        draw(c, 2, 0x101d90b8);
        side = u(s8(c.r(3)));
        player = match + u(std::int32_t(side) * 0x2350) + u(s8(index) * std::int32_t(player_size)) + players_at;
        if (compare(c, s8(position(c, player))) >= 0) enqueue(c, match, player);
        player = match + u(std::int32_t(s8(side) == 0) * 0x2350) + u(s8(index) * std::int32_t(player_size)) + players_at;
        if (compare(c, s8(position(c, player))) >= 0) enqueue(c, match, player);
        index = index + 1;
    }
    // The comparator reads the match through the game's global.
    c.put32(c.r(2) - 0x3910, match);
    const auto count = u(s8(c.u8(match + queue_count_at)));
    sort_actor_queue(c, match, count, c.toc(-0x6420));
}

// 0x101d91b0
std::int32_t compare_actors(Cpu& c, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, actor_comparator, compare_frame, {.first_gpr = 26, .gpr_offset = -0x28, .first_fpr = 31});
    c.put32(frame.at(0xa8), first);
    c.put32(frame.at(0xac), second);
    const auto doubles = c.toc(-0x52c4);
    keep(c, {{29, doubles}});
    Fpu fp(c);
    Kept<31> a(c, c.u32(first));
    Kept<30> b(c, c.u32(second));
    const auto current = [&] { return c.toc(-0x3910); };
    const auto early = [&](std::uint32_t player, std::uint32_t answer) {
        if (compare(c, c.i8(current() + play_state)) == 0) return false;
        const auto holder = c.u32(current() + pending_player);
        if (compare_logical(c, player, holder) != 0) return false;
        c.set(3, answer);
        return true;
    };
    if (early(a, u(-1))) return std::int32_t(c.r(3));
    if (early(b, 1)) return std::int32_t(c.r(3));
    if (compare_logical(c, c.u32(current() + pinned_at), a) == 0) return std::int32_t(result(c, u(-1)));
    if (compare_logical(c, c.u32(current() + pinned_at), b) == 0) return std::int32_t(result(c, 1));

    // Each player's value: (ratings sum) * fitness * (pool + form / pool) as an integer.
    auto rating = held<26>(c);
    KeptFloat<31> sum(c);
    auto* const scores = scores_for(c, frame, doubles);
    const auto value = [&](std::uint32_t player, std::uint32_t conversion, std::uint32_t form_conversion,
                           std::uint32_t slot, unsigned slot_mask) {
        unsigned index = player_count;
        if (scores) {
            const auto offset = player - (scores->match + players_at);
            if (offset < player_count * player_size && offset % player_size == 0) index = offset / player_size;
        }
        std::optional<ScoreStatus> before;
        if (index < player_count) {
            before.emplace(c.s);
            const auto& score = scores->entries[index];
            // Each lane must complete once at this frame before moving its
            // writes across the loads, in case a noncanonical mapping faults.
            if ((scores->seen_slots[index] & slot_mask) && score && score->before.same_inputs(*before)) {
                // The other player's evaluation may fault before the frame
                // restores saved registers. Publish the whole score boundary.
                c.put64(conversion, score->rating_scratch);
                c.put64(form_conversion, score->form_scratch);
                c.put64(slot, score->result_bits);
                c.set(3, score->secondary_rating);
                c.set(26, score->primary_rating);
                c.set_f(1, score->form_value);
                c.set_f(31, score->ratings_value);
                c.s.wr<8>(reg::fpr(0), score->result_bits);
                score->after.publish(c.s);
                return std::uint32_t(score->result_bits);
            }
        }
        rating = u(s8(primary_rating(c, player)));
        const auto ratings = s8(secondary_rating(c, player)) + s8(rating);
        c.set_f(1, c.f64(doubles + signed_bias));
        sum = fp.fsubs(integer_double(c, conversion, ratings), c.f(1));
        const auto product = fp.fmuls(fitness(c, player), sum);
        const auto offset = c.f64(doubles + 0x90);
        c.set_f(1, c.f64(doubles + signed_bias));
        std::int16_t form_rating = 0;
        const auto form_value = fp.fsub(integer_double(c, form_conversion, form_rating = c.i16(player + 0x3c)), c.f(1));
        c.set_f(1, form_value);
        const auto form = fp.frsp(fp.fadd(offset, fp.fdiv(c.f(1), c.f64(doubles + 0xf0))));
        const auto weighted = fp.fmuls(form, product);
        const auto bits = fp.fctiwz(0, weighted);
        c.put64(slot, bits);
        if (before && weighted >= -2147483648.0 && weighted < 2147483648.0) {
            // Keep exceptional conversions on the live route, including
            // their diagnostics and enabled exceptions.
            scores->entries[index] = ActorScore{*before, ScoreStatus(c.s), integer_bits(ratings),
                                                integer_bits(form_rating), bits, form_value, sum, c.r(3), rating};
            scores->seen_slots[index] |= slot_mask;
        }
        return c.u32(slot + 4);
    };
    Kept<28> value_a(c, value(a, frame.at(0x60), frame.at(0x58), frame.at(0x50), 1));
    Kept<27> value_b(c, value(b, frame.at(0x48), frame.at(0x40), frame.at(0x38), 2));
    return std::int32_t(result(c, value_b - value_a));
}

// 0x101d93b0
void update_players_a(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101d93b0, 0x50, {.first_gpr = 27});
    keep(c, {{27, match}});
    sweep_players(c, match, true, [&](std::uint32_t player, unsigned list) {
        c.set(3, player);
        call(c, 0x10237b50, list ? 0x101d942c : 0x101d93ec);
    });
}

// 0x101d9460
void update_players_b(Cpu& c, std::uint32_t match) {
    Frame frame(c, 0x101d9460, 0x50, {.first_gpr = 27});
    keep(c, {{27, match}});
    sweep_players(c, match, true, [&](std::uint32_t player, unsigned list) {
        c.set(3, player);
        call(c, 0x102393c0, list ? 0x101d94dc : 0x101d949c);
    });
}

// 0x101d9510
void update_all_players(Cpu& c, std::uint32_t match, std::uint32_t argument) {
    Frame frame(c, 0x101d9510, 0x50, {.first_gpr = 26});
    keep(c, {{26, match}, {27, argument}});
    sweep_players(c, match, false, [&](std::uint32_t player, unsigned list) {
        c.set({{3, player}, {4, argument}});
        call(c, 0x10239790, list ? 0x101d9570 : 0x101d9540);
    });
}
}

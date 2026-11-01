// MATCH_ENG.CPP, tactics block A: the actor queue and its ordering, and the per-player sweeps.
#include "match_eng_tactics_a.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace tactics_a;

namespace {
constexpr std::uint32_t queue_count_at = 0x792e;   // byte
constexpr std::uint32_t queue_at = 0x79f8;         // up to 40 player words
constexpr std::uint32_t pinned_at = 0x7c68;        // word: a player that always comes first

// Pool offsets (doubles at TOC -0x52c4).
constexpr std::uint32_t signed_bias = 0x60;

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
    c.set({{3, match + queue_at}, {4, u(s8(c.u8(match + queue_count_at)))}, {5, 4}, {6, c.toc(-0x6420)}});
    call(c, 0x10768300, 0x101d9184);
}

// 0x101d91b0
std::int32_t compare_actors(Cpu& c, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x101d91b0, 0x90, {.first_gpr = 26, .gpr_offset = -0x28, .first_fpr = 31});
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
    const auto value = [&](std::uint32_t player, std::uint32_t conversion, std::uint32_t form_conversion,
                           std::uint32_t slot) {
        rating = u(s8(primary_rating(c, player)));
        const auto ratings = s8(secondary_rating(c, player)) + s8(rating);
        c.set_f(1, c.f64(doubles + signed_bias));
        sum = fp.fsubs(integer_double(c, conversion, ratings), c.f(1));
        const auto product = fp.fmuls(fitness(c, player), sum);
        const auto offset = c.f64(doubles + 0x90);
        c.set_f(1, c.f64(doubles + signed_bias));
        c.set_f(1, fp.fsub(integer_double(c, form_conversion, c.i16(player + 0x3c)), c.f(1)));
        const auto form = fp.frsp(fp.fadd(offset, fp.fdiv(c.f(1), c.f64(doubles + 0xf0))));
        c.put64(slot, fp.fctiwz(0, fp.fmuls(form, product)));
        return c.u32(slot + 4);
    };
    Kept<28> value_a(c, value(a, frame.at(0x60), frame.at(0x58), frame.at(0x50)));
    Kept<27> value_b(c, value(b, frame.at(0x48), frame.at(0x40), frame.at(0x38)));
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

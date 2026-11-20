// CUP.CPP, block `tree`: 0x10479100 `CUP::map_fixture_tree`.
//
// What it builds (names inferred from how the routine uses the rows, "(inferred)"):
//  * cup::round_matches: one block of 0x36-byte match rows per round (a row per tie, round.ties = +0x1c), blanked
//    to the "empty" pattern (halfwords 0xffff, bytes 0xff, a few zero bytes).
//  * the feeding links: for every round k but the last two (`usable`, below, counts the rounds that take part: all of
//    them, or one fewer when the last round's label +8 is 0x8c), a permutation of round k's ties (bytes 0..ties-1, shuffled
//    only when the next round has fewer than half as many ties), and for each tie p of the permutation a tie of
//    round k+1 (round k+2 when none is free): the target row's first free feeder slot (+0xa then +0xc, or the
//    reverse, by a coin from the stream) gets p, the feeding round k at +0xe/+0x10; the tie's own row gets the
//    target tie at +6, the side (1 for the +0xa slot, 0 for +0xc) at +8 and the target round at +0x12.
//  * the entrants: for every round, round.seeded (+0x1e) entrants numbered from round.first_entrant (+0x20) go into
//    free home/away halves (+0 and +2) of its rows; a cup with a region rule (+0x24) swaps a pair whose second club's
//    region id equals the rule so that club comes first.
//  * round.first_place (+0x68): a running total of ties, from the last rounds back.
//
// Draws: every draw is the game's sample stream (Pool): a bounded draw (a bound n, never zero: the original
// skips the draw for 0) and a coin (the sample's parity). The original's bounded draw has a floating-point arm
// for |n| > 65535 that calls 0x1047a9e0; every bound here is `lhax` of a halfword, so n lies in -32768..32767 and
// the arm is dead (listing 0x10479794..0x104797b4, `cmpw n,0xffff`/`cmpw n,-0xffff`); it throws if reached.
//
// Registers: r4-r10 at each call and at the return are the original's, because rand, new[] and delete[] pass them
// through (rand writes only r3 and r4) and the callers can read what is left. The original's code assigns them in
// many small places, and each assignment is repeated where it happens (`c.set(4..10, ...)`, with the instruction
// address) rather than reconstructed. The non-volatile registers are set before the assertion and the dialog
// (imports) as the original holds them there.
#include "competition_overrides.hpp"
#include "cup_base_detail.hpp"
#include "cup_base_tree.hpp"
#include <array>
#include <stdexcept>

namespace cm::aot::cup_base::tree {
namespace {
constexpr std::uint32_t entry = 0x10479100;
constexpr std::uint32_t frame_size = 0x1e0;

// The round row fields this routine uses beyond Round (cup_base_detail.hpp).
namespace round_field {
inline constexpr std::uint32_t final_label = 0x8c;     // Round::label value of the last round that takes no part (inferred)
inline constexpr std::uint32_t seeded = 0x1e,          // halfword: entrants to place into this round (inferred)
                               first_entrant = 0x20,   // halfword: the first of their numbers (inferred)
                               first_place = 0x68;     // byte: the round's first place (inferred)
inline constexpr std::int32_t skip_draw = 0x200,       // Round::flags bit: the tie of the next round is the permutation value / 2
                              avoid_taken = 0x400;     // Round::flags bit: start the search at the first free tie
}
// The 0x36-byte match rows.
namespace match {
inline constexpr std::uint32_t home = 0x00, away = 0x02, next_tie = 0x06, next_side = 0x08, feeder_a = 0x0a,
                               feeder_b = 0x0c, feeder_a_round = 0x0e, feeder_b_round = 0x10, next_round = 0x12;
}
// The original blanks a row with these stores, in this order: halfwords 0xffff, zero halfword +8, bytes.
struct Blank { std::uint8_t offset, size; bool ones; };
constexpr Blank blanks[] = {
    {0x00, 2, true}, {0x02, 2, true}, {0x06, 2, true}, {0x08, 2, false}, {0x0a, 2, true}, {0x0c, 2, true},
    {0x32, 1, false}, {0x0e, 2, true}, {0x10, 2, true}, {0x12, 2, true}, {0x34, 1, false},
    {0x14, 1, true}, {0x15, 1, true}, {0x1c, 1, true}, {0x1d, 1, true}, {0x16, 1, true}, {0x17, 1, true},
    {0x18, 1, true}, {0x19, 1, true}, {0x1a, 1, true}, {0x1b, 1, true}, {0x1e, 1, true}, {0x1f, 1, true},
    {0x26, 1, true}, {0x27, 1, true}, {0x20, 1, true}, {0x21, 1, true}, {0x22, 1, true}, {0x23, 1, true},
    {0x24, 1, true}, {0x25, 1, true}, {0x28, 1, true}, {0x29, 1, true}, {0x30, 1, true}, {0x31, 1, true},
    {0x2a, 1, true}, {0x2b, 1, true}, {0x2c, 1, true}, {0x2d, 1, true}, {0x2e, 1, true}, {0x2f, 1, true},
    {0x33, 1, false}};

inline std::uint32_t u(std::int32_t value) { return std::uint32_t(value); }
inline std::int32_t extsh(std::uint32_t value) { return std::int32_t(std::int16_t(value)); }
inline std::int32_t extsb(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
inline std::int32_t halve(std::int32_t value) { return (value + (value < 0 ? 1 : 0)) >> 1; }  // rlwinm/add/srawi

// A draw below `n`, from the sample stream, as the original inlines it at its seven sites (`back` is the return
// address of the site's rand call). n == 0 draws nothing and answers 0.
std::int32_t draw_below(Cpu& c, std::int32_t n, std::uint32_t back) {
    if (n == 0) return 0;
    const auto pool = Pool::at(c);
    pool.advance(back);
    const auto sample = std::int32_t(pool.sample());
    if (n > 0xffff || n < -0xffff)
        throw std::logic_error("CUP::map_fixture_tree: a draw bound beyond a halfword (dead floating-point arm)");
    // divw in 64 bits, wrapping (0x80000000 / -1 too), mullw, subf; the sign of the bound sets the sign.
    const auto quotient = std::uint32_t(std::int64_t(sample) / n);
    const auto remainder = std::uint32_t(sample) - quotient * u(n);
    return std::int32_t(n > 0 ? remainder : 0u - remainder);
}
// The coin: whether the stream's next sample is even (the original xors the low bit with the sign bit).
bool coin_is_even(Cpu& c, std::uint32_t back) {
    const auto pool = Pool::at(c);
    pool.advance(back);
    const auto sample = pool.sample();
    const auto sign = sample >> 31;
    return ((sample & 1) ^ sign) == sign;
}

struct MemoryFailure { std::uint32_t at; std::int32_t name, extension; std::uint32_t line, text; };
// "CUP::map_fixture_tree - out of memory": the assertion text with the pool's message, the localized dialog, the
// success flag cleared and exit(-1). The caller returns -1 if exit comes back.
void memory_failure(Cpu& c, const Frame& frame, const MemoryFailure& f) {
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message);
    c.set({{3, pool}, {4, 0}, {5, 0}, {6, frame.at(f.name)}, {7, frame.at(f.extension)}});
    c.call(0x10739890, f.at + 4);
    c.call(0x10768810, f.at + 8);
    c.set({{5, c.r(3)}, {3, message}, {4, pool + text::report_format_text}, {6, frame.at(f.name)},
           {7, frame.at(f.extension)}, {9, pool + f.text}, {8, f.line}});
    c.call(0x1077b720, f.at + 0x28);
    c.set({{3, c.r(13)}, {4, pool + text::memory_title}});            // r13, r14: the dialog's two buffers
    c.call(0x10727890, f.at + 0x34);
    c.set({{3, c.r(14)}, {5, message}, {4, pool + text::memory_message}});
    c.call(0x10727890, f.at + 0x44);
    c.set({{3, c.r(13)}, {4, c.r(14)}, {5, 0}});
    c.call(0x10769b60, f.at + 0x54);
    c.put8(c.toc(toc::success), 0);
    invoke(c, callee::exit, f.at + 0x64, {0xffffffffu});
}
constexpr MemoryFailure list_failure{0x10479168, 0x130, 0x68, 0x8b, text::map_tree_memory};
constexpr MemoryFailure rows_failure{0x1047928c, 0xf0, 0x58, 0x9e, text::map_tree_memory_2};
constexpr CheckSite no_ties{0x10479210, 0x110, 0x60, 0x95};          // a round without ties
constexpr CheckSite no_next_round{0x10479f00, 0xd0, 0x50, 0x175};    // no round after the next to fall back to
constexpr CheckSite no_free_tie{0x1047a1dc, 0xb0, 0x48, 0x1a0};      // round k+2 has no free feeder slot either
constexpr CheckSite no_ties_to_seed{0x1047a300, 0x90, 0x40, 0x1c2};  // a skip-draw round without ties
constexpr CheckSite no_free_half{0x1047a75c, 0x70, 0x38, 0x222};     // no free home/away half in the round

class Tree {
public:
    Tree(Cpu& cpu, std::uint32_t object, const Frame& f) : c(cpu), cup(object), frame(f) {}
    std::int32_t run();

private:
    Cpu& c;
    std::uint32_t cup;
    const Frame& frame;

    std::uint32_t rounds() const { return c.u32(cup + cup::rounds); }
    std::uint32_t round_at(std::int32_t k) const { return rounds() + u(k) * cup::round_row; }
    std::int32_t ties(std::int32_t k) const { return c.i16(round_at(k) + Round::ties); }
    std::int32_t round_flags(std::int32_t k) const { return c.i16(round_at(k) + Round::flags); }
    std::int32_t round_count() const { return c.i16(cup + competition::rounds); }
    std::uint32_t match_list() const { return c.u32(cup + cup::round_matches); }
    std::uint32_t matches_of(std::int32_t k) const { return c.u32(match_list() + u(k) * 4); }

    void blank_row(std::uint32_t row);
    void shuffle(std::int32_t k, std::uint32_t permutation);
    // The assertion with the non-volatile registers the original holds in the pairing loops (F) or the entrant
    // loops (G).
    void report_pairing(const CheckSite& site, std::int32_t k, std::uint32_t permutation, std::uint32_t r13,
                        std::uint32_t r14, std::uint32_t r23, std::uint32_t r24, std::uint32_t r25, std::int32_t i,
                        std::int32_t usable, std::int32_t p);
    bool place_in_next(std::int32_t k, std::int32_t p, std::uint32_t r25, std::uint32_t& r14, std::int32_t start);
    void tie_totals();
};

void Tree::blank_row(std::uint32_t row) {
    for (const auto& store : blanks) {
        if (store.size == 2) c.put16(row + store.offset, store.ones ? 0xffffu : 0u);
        else c.put8(row + store.offset, store.ones ? 0xffu : 0u);
    }
}

void Tree::shuffle(std::int32_t k, std::uint32_t permutation) {
    for (std::int32_t i = 0; i < ties(k); ++i) {
        const auto a = draw_below(c, ties(k), 0x1047978c);
        const auto b = draw_below(c, ties(k), 0x104798f0);
        const auto first = c.u8(permutation + u(a));
        const auto second = c.u8(permutation + u(b));
        c.put8(permutation + u(a), second);
        c.put8(permutation + u(b), first);
    }
}

void Tree::report_pairing(const CheckSite& site, std::int32_t k, std::uint32_t permutation, std::uint32_t r13,
                          std::uint32_t r14, std::uint32_t r23, std::uint32_t r24, std::uint32_t r25, std::int32_t i,
                          std::int32_t usable, std::int32_t p) {
    c.set({{13, r13}, {14, r14}, {23, r23}, {24, r24}, {25, r25}, {27, permutation}, {28, u(i)}, {29, u(usable)},
           {30, u(k)}, {31, u(p)}});
    report(c, frame, site);
}

// One tie of round k (permutation value p) tries to take a feeder slot of the tie `r14` of round k+1, a coin
// choosing which slot is tried first. 0x10479cb0..0x10479ed8. `start` is the tie the search began at (r24): the
// walk over the ties ends when it returns there. The registers are the original's after each path.
bool Tree::place_in_next(std::int32_t k, std::int32_t p, std::uint32_t r25, std::uint32_t& r14, std::int32_t start) {
    for (;;) {
        const bool even = coin_is_even(c, 0x10479cd0);
        const auto offset = u(extsh(r14)) * cup::match_row;                // r6
        const auto list = match_list();
        const auto next_row = c.u32(list + u(k + 1) * 4);                  // r7
        c.set({{6, offset}, {4, list}, {7, next_row}});
        // The two slots in the order the coin gives: the odd sample tries +0xa first, the even one +0xc.
        const std::uint32_t slots[2] = {even ? match::feeder_b : match::feeder_a, even ? match::feeder_a : match::feeder_b};
        for (const auto slot : slots) {
            c.set(5, offset + slot);
            const auto found = c.i16(next_row + offset + slot);
            c.set(4, u(found));
            if (found != -1) continue;
            const bool side_a = slot == match::feeder_a;
            c.put16(next_row + offset + slot, u(p));
            const auto own_offset = u(p) * cup::match_row;                 // r7 in the original
            const auto round_slot = slot == match::feeder_a ? match::feeder_a_round : match::feeder_b_round;  // the feeding round
            const auto list_now = match_list();
            const auto rows_of_k = c.u32(list_now + r25);
            c.put16(c.u32(list_now + u(k + 1) * 4) + offset + round_slot, u(k));
            c.put16(rows_of_k + own_offset + match::next_tie, r14);
            c.put16(rows_of_k + own_offset + match::next_side, side_a ? 1u : 0u);
            c.put16(rows_of_k + own_offset + match::next_round, u(k + 1));
            c.set({{4, rows_of_k}, {5, rows_of_k}, {6, side_a ? 1u : 0u}, {7, rows_of_k}, {8, offset + round_slot},
                   {9, next_row}});
            return true;
        }
        // Both taken: the next tie (wrapping at the round's ties), and the end of the walk at the start.
        c.set(4, u(extsh(r14)));
        const auto count = ties(k + 1);
        r14 = extsh(r14) < count - 1 ? r14 + 1 : 0;
        if (start != extsh(r14)) continue;
        return false;
    }
}

std::int32_t Tree::run() {
    // 0x10479108..0x10479130: the prologue's loads of the game's globals.
    c.set({{13, c.toc(-0x79dc)}, {14, c.toc(-0x79e0)}, {15, c.toc(toc::doubles)}, {16, c.toc(-0x7b90)},
           {17, c.toc(-0x7b8c)}, {18, c.toc(-0x7b88)}, {19, c.toc(-0x7b84)}, {20, c.toc(toc::success)}, {21, c.toc(toc::message)},
           {22, c.toc(toc::pool)}, {26, cup}});

    // The array of the rounds' match blocks.
    const auto list = invoke(c, callee::allocate_block, 0x10479148, {u(round_count()) << 2});
    c.put32(cup + cup::round_matches, list);
    if (!list) {
        memory_failure(c, frame, list_failure);
        return -1;
    }
    // A block per round: ties * 0x36 bytes (a round without ties asserts).
    {
        std::uint32_t slot_offset = 0, round_offset = 0;                   // r23, r24
        for (std::int32_t k = 0; k < round_count(); ++k, slot_offset += 4, round_offset += cup::round_row) {
            const auto count = c.i16(rounds() + round_offset + Round::ties);
            c.set({{23, slot_offset}, {24, round_offset}, {31, u(k)}});
            if (count < 1) {
                report(c, frame, no_ties);
                return -1;
            }
            const auto block = invoke(c, callee::allocate_block, 0x10479260, {u(count) * cup::match_row});
            c.set(4, match_list());
            c.put32(match_list() + slot_offset, block);
            if (!c.u32(match_list() + slot_offset)) {
                memory_failure(c, frame, rows_failure);
                return -1;
            }
        }
    }
    // The entrants' second byte: cleared (entrant rows are 8 bytes: club, state, then this byte, +5).
    if (c.u32(cup + cup::entrants)) {
        c.set(5, 0);
        for (std::int32_t i = 0; i < c.i16(cup + competition::club_count); ++i) {
            c.set(4, c.u32(cup + cup::entrants));
            c.put8(c.u32(cup + cup::entrants) + u(i) * cup::entrant_row + 5, 0);
        }
    }
    c.put8(cup + cup::current_round, 0);
    c.put8(cup + cup::byte_a9, 0);
    if (c.u8(cup + competition::byte_45) == 0) return 1;   // extsb. r0,r0 at 0x10479360: any set bit continues

    // D: every match row of every round is blanked.
    c.set(4, 0);
    for (std::int32_t k = 0;; ++k) {
        const auto total = round_count();
        c.set(6, u(total));
        if (!(k < total)) break;
        c.set(5, 0);
        for (std::int32_t m = 0; m < ties(k); ++m) {
            blank_row(matches_of(k) + u(m) * cup::match_row);
            const auto rows = matches_of(k);
            c.set({{5, u(m + 1) * cup::match_row}, {6, rows}, {7, rows}, {8, rows}, {9, rows}, {10, rows}});
        }
        c.set(4, u(k + 1) * 4);
    }

    // The rounds that take part: all, or one fewer when the last round's label is 0x8c.
    const auto total = round_count();
    c.set({{4, u(total) - 1}, {5, rounds()}});
    const auto last_label = c.i16(rounds() + u(total - 1) * cup::round_row + Round::label);
    const std::int32_t usable = last_label == std::int32_t(round_field::final_label) ? extsh(u(total - 1)) : total;
    const std::int32_t limit = usable - 1;

    // F: the permutation of each round's ties and their placement in the next round (or the one after).
    std::uint32_t r25 = 0;                                                  // 4 * k
    for (std::int32_t k = 0; k < limit; ++k, r25 += 4) {
        c.set(4, rounds());
        const auto n = ties(k);
        std::uint32_t permutation = 0;                                      // r27
        if (n) permutation = invoke(c, callee::allocate_block, 0x104796e0, {u(n)});
        c.set(4, permutation);
        std::int32_t filled = 0;
        for (; filled < ties(k); ++filled) c.put8(permutation + u(filled), u(filled));
        c.set({{4, permutation + u(filled)}, {5, u(ties(k))}, {6, rounds()}});
        if (ties(k + 1) < halve(ties(k))) shuffle(k, permutation);

        std::uint32_t r13 = permutation;                                    // the permutation's cursor
        std::uint32_t r14 = 0;                                              // the tie being tried
        const std::uint32_t r23 = u(k + 1) * cup::round_row + Round::ties;  // 0x10479a6c
        for (std::int32_t i = 0; i < ties(k); ++i, ++r13) {
            c.set(4, rounds());
            const auto p = extsb(c.u8(r13));                                // r31
            std::int32_t start;                                             // r24
            if (!(round_flags(k + 1) & round_field::skip_draw)) {
                const auto next_ties = ties(k + 1);
                start = next_ties ? draw_below(c, next_ties, 0x10479ae8) : 0;
                r14 = u(extsh(u(start)));
                start = std::int32_t(r14);
                c.set({{4, u(k + 1)}, {5, rounds()}});
                if (round_flags(k + 1) & round_field::avoid_taken) {
                    c.set(5, u(k + 1) * cup::round_row + Round::ties);
                    const auto first = extsh(r14);
                    for (;;) {
                        c.set(4, u(extsh(r14)));
                        c.set(7, match_list());
                        const auto offset = u(extsh(r14)) * cup::match_row;
                        c.set(6, offset);
                        const auto next_row = c.u32(match_list() + u(k + 1) * 4);
                        c.set(7, next_row);
                        c.set(4, offset + match::feeder_a);
                        const auto a = c.i16(next_row + offset + match::feeder_a);
                        c.set(4, u(a));
                        if (a == -1) {
                            c.set(4, offset + match::feeder_b);
                            const auto b = c.i16(next_row + offset + match::feeder_b);
                            c.set(4, u(b));
                            if (b == -1) { start = std::int32_t(r14); break; }
                        }
                        c.set(4, rounds());
                        r14 += 1;
                        c.set(6, u(extsh(r14)));
                        const auto count = ties(k + 1);
                        c.set(4, u(count));
                        if (!(extsh(r14) < count)) r14 = 0;
                        c.set(4, u(extsh(r14)));
                        if (extsh(r14) == first) break;
                    }
                }
            } else {
                r14 = u(extsh(u(halve(p))));
                start = std::int32_t(r14);
            }
            start = extsh(u(start));                                        // 0x10479ca8
            if (place_in_next(k, p, r25, r14, start)) continue;

            // Not placed in round k+1: round k+2 is the last resort.
            if (!(k < extsh(u(usable)) - 2)) {
                report_pairing(no_next_round, k, permutation, r13, r14, r23, u(start), r25, i, usable, p);
                invoke(c, callee::exit, 0x10479f44, {0xffffffffu});
            }
            const auto k2 = k + 2;
            c.set(4, rounds());
            const auto fallback_ties = ties(k2);                            // r24
            r14 = fallback_ties ? u(draw_below(c, fallback_ties, 0x10479f88)) : 0;
            std::uint32_t r7 = u(extsh(r14)), r8 = r7;
            c.set({{6, u(k2)}, {7, r7}, {8, r8}, {5, u(k2) * 4}, {9, u(k2) * cup::round_row + Round::ties}});
            for (;;) {
                const auto offset = u(extsh(r7)) * cup::match_row;
                c.set(4, match_list());
                const auto next_row = c.u32(match_list() + u(k2) * 4);
                c.set(4, next_row);
                const auto a = c.i16(next_row + offset + match::feeder_a);
                c.set(10, u(a));
                std::uint32_t slot = 0;
                if (a == -1) slot = match::feeder_a;
                else {
                    const auto b = c.i16(next_row + offset + match::feeder_b);
                    c.set(10, u(b));
                    if (b == -1) slot = match::feeder_b;
                }
                if (slot) {
                    const auto side = slot == match::feeder_a ? 1u : 0u;
                    c.put16(next_row + offset + slot, u(p));
                    const auto own_offset = u(p) * cup::match_row;
                    const auto rows_of_k = c.u32(match_list() + r25);
                    const auto round_slot = slot == match::feeder_a ? match::feeder_a_round : match::feeder_b_round;
                    c.put16(c.u32(match_list() + u(k2) * 4) + offset + round_slot, u(k));
                    c.put16(rows_of_k + own_offset + match::next_tie, r7);
                    c.put16(rows_of_k + own_offset + match::next_side, side);
                    c.put16(rows_of_k + own_offset + match::next_round, u(k2));
                    c.set({{4, rows_of_k}, {5, rows_of_k}, {6, u(k2)}, {8, side}, {9, own_offset}, {10, offset + round_slot}});
                    break;
                }
                const auto count = ties(k2);
                r7 = extsh(r7) < count - 1 ? r7 + 1 : 0;
                if (extsh(r7) != extsh(r8)) continue;
                report_pairing(no_free_tie, k, permutation, r13, r14, r23, u(fallback_ties), r25, i, usable, p);
                invoke(c, callee::exit, 0x1047a220, {0xffffffffu});
                return -1;
            }
        }
        if (permutation) invoke(c, callee::free_block, 0x1047a254, {permutation});
    }
    if (c.i16(rounds() + Round::ties) == 0) {
        c.put8(cup + cup::byte_b0, 1);
        return 1;
    }

    // G: each round's entrants go into free halves of its rows.
    std::uint32_t r24 = u(-1);                                              // the search's start, kept across the loops
    std::uint32_t r13 = 0, r23 = 0;                                         // 4 * k, 0x78 * k
    for (std::int32_t k = 0; k < extsh(u(usable)); ++k, r13 += 4, r23 += cup::round_row) {
        const std::uint32_t r27 = r23 + Round::ties, r28 = r23 + Round::flags, r29 = r23 + round_field::seeded;
        for (std::int32_t j = 0; j < c.i16(rounds() + r29); ++j) {
            const auto hold = [&](std::uint32_t r14, std::uint32_t r25_now) {
                c.set({{13, r13}, {14, r14}, {23, r23}, {24, r24}, {25, r25_now}, {27, r27}, {28, r28}, {29, r29},
                       {30, u(k)}, {31, u(j)}});
            };
            c.set(4, rounds());
            std::uint32_t r14;
            if (c.i16(rounds() + r28) & round_field::skip_draw) {
                r14 = 0;
                if (!(0 < c.i16(rounds() + r23 + Round::ties))) {
                    hold(r14, r25);
                    report(c, frame, no_ties_to_seed);
                    return -1;
                }
            } else {
                r14 = u(c.i16(rounds() + r23 + Round::ties));
                r24 = r14 ? u(draw_below(c, std::int32_t(r14), 0x1047a37c)) : 0;
                r14 = u(extsh(r24));
                r24 = r14;
                if (c.i16(rounds() + r23 + Round::flags) & round_field::avoid_taken) {
                    c.set(4, r23 + Round::ties);
                    const auto first = extsh(r14);
                    for (;;) {
                        const auto offset = u(extsh(r14)) * cup::match_row;
                        c.set(6, match_list());
                        c.set(5, offset);
                        const auto rows = c.u32(match_list() + r13);
                        c.set(6, rows);
                        const auto home_free = c.i16(rows + offset + match::home);
                        if (home_free == -1) {
                            const auto away_free = c.i16(rows + offset + match::away);
                            if (away_free == -1) { r24 = r14; break; }
                        }
                        r14 += 1;
                        c.set(5, u(extsh(r14)));
                        if (!(extsh(r14) < c.i16(rounds() + r23 + Round::ties))) r14 = 0;
                        if (extsh(r14) == first) break;
                    }
                }
            }
            const auto r25_now = u(extsh(r24));
            r25 = r25_now;                                                  // 0x1047a530
            const auto seat = [&](std::uint32_t field) {
                c.put16(c.u32(match_list() + r13) + u(extsh(r14)) * cup::match_row + field,
                        u(j) + u(c.i16(rounds() + r23 + round_field::first_entrant)));
            };
            for (;;) {
                const bool even = coin_is_even(c, 0x1047a558);
                const auto offset = u(extsh(r14)) * cup::match_row;
                const auto half_at = [&](std::uint32_t rows, std::uint32_t field) { return c.i16(rows + offset + field); };
                bool seated = false;
                if (!even) {
                    // +0 then +2, each with the opposite slot of the other side free.
                    c.set(4, offset);
                    auto rows = c.u32(match_list() + r13);
                    c.set(5, rows);
                    if (half_at(rows, match::home) == -1 && half_at(rows, match::feeder_a) == -1) {
                        seat(match::home);
                        seated = true;
                    } else {
                        rows = c.u32(match_list() + r13);
                        c.set({{4, offset}, {5, offset + match::away}});
                        if (half_at(rows, match::away) == -1 && half_at(rows, match::feeder_b) == -1) {
                            c.set(4, rounds());
                            seat(match::away);
                            seated = true;
                        }
                    }
                } else {
                    auto rows = c.u32(match_list() + r13);
                    c.set({{4, offset}, {5, offset + match::away}});
                    if (half_at(rows, match::away) == -1 && half_at(rows, match::feeder_b) == -1) {
                        c.set(4, rounds());
                        seat(match::away);
                        seated = true;
                    } else {
                        rows = c.u32(match_list() + r13);
                        c.set({{4, offset}, {5, rows}});
                        if (half_at(rows, match::home) == -1 && half_at(rows, match::feeder_a) == -1) {
                            seat(match::home);
                            seated = true;
                        }
                    }
                }
                if (seated) break;
                // Neither pair of halves is free: a region rule may swap the two halves of a full tie.
                const auto rule = c.u32(cup + competition::word_24);
                c.set(5, rule);
                if (std::int32_t(rule) != -1) {
                    const auto rows = c.u32(match_list() + r13);
                    c.set(4, rows);
                    const auto home = c.i16(rows + offset + match::home), away = c.i16(rows + offset + match::away);
                    c.set({{6, u(home)}, {7, u(away)}});
                    if (home != -1 && away != -1) {
                        const auto entrants = c.u32(cup + cup::entrants);
                        c.set({{9, entrants}, {8, u(away) << 3}});
                        auto club = c.u32(entrants + (u(away) << 3));
                        c.set(8, club);
                        if (club) {
                            club = c.u32(club + 0x3c);
                            c.set(8, club);
                            if (club) {
                                club = c.u32(club);
                                c.set(8, club);
                                if (std::int32_t(rule) == std::int32_t(club)) {
                                    c.put16(rows + offset + match::home, u(away));
                                    c.put16(c.u32(match_list() + r13) + offset + match::away, u(home));
                                }
                            }
                        }
                    }
                }
                c.set(4, u(extsh(r14)));
                r14 = extsh(r14) < c.i16(rounds() + r27) - 1 ? r14 + 1 : 0;
                if (extsh(r25) != extsh(r14)) continue;
                hold(r14, r25);
                report(c, frame, no_free_half);
                return -1;
            }
        }
    }
    tie_totals();
    return 1;
}

// The last part: each round's first place (+0x68), a running total of ties from the last round that takes part
// back to the first. 0x1047a7cc..0x1047a9bc. The original unrolls the loop by four; the registers it leaves
// (r4-r8) are the same as the plain loop's.
void Tree::tie_totals() {
    const auto count = round_count();
    c.set(5, u(count));
    std::int32_t last = -1;
    std::uint32_t total = 0;
    bool counted_to_final = false;
    if (count > 0) {
        c.set(4, rounds());
        counted_to_final = c.i16(rounds() + u(count - 1) * cup::round_row + Round::label) ==
                           std::int32_t(round_field::final_label);
    }
    if (counted_to_final) {
        // The last round is the 0x8c one: places start at 5 two rounds before the end.
        last = extsh(u(count - 3));
        total = 5;
        c.set(8, 5);
        c.set(4, u(last) * cup::round_row);
    } else {
        c.set(4, u(count));
        c.set(5, rounds());
        c.set(4, u(count - 1) * cup::round_row);
        last = extsh(u(count - 2));
        const auto entrants = c.i16(rounds() + u(count - 1) * cup::round_row + Round::entrants);
        c.set(5, u(entrants));
        total = (u(entrants) + 1) & 0xff;
        c.set(4, u(last) * cup::round_row);
        c.set(8, total);
    }
    if (last < 0) return;
    for (std::int32_t q = last; q >= 0; --q) {
        c.put8(rounds() + u(q) * cup::round_row + round_field::first_place, total);
        total += u(c.i16(rounds() + u(q) * cup::round_row + Round::ties)) & 0xff;
    }
    // What the last pass leaves: the pass for round 0, then the loop's step past it.
    c.set({{4, u(-0x78)}, {5, Round::ties}, {6, rounds()}, {7, round_field::first_place}, {8, total}});
}
}

std::int32_t map_fixture_tree(Cpu& c, std::uint32_t cup) {
    // The career's own values (competition_overrides.hpp), after configure built the round table and before the
    // tree or any fixture reads it: only object and round-row fields, no register, stack byte or call. Without
    // entries it only reads the object (the host's record of defaults); without a platform it does nothing.
    competition_overrides::at_setup(c.s, cup, competition_overrides::Kind::cup);
    const Frame frame(c, entry, frame_size, {.first_gpr = 13});
    return Tree(c, cup, frame).run();
}
}

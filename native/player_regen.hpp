#pragma once
// PLAYER_REGEN.CPP (tools/player_regen_audit.py): the regen object (retirements of players and staff, the queue of
// regenerated people, the pools of newgen players and non-player staff). player_regen_detail.hpp names the records the
// routines read. Each block of the module declares its routines in its own section below; a routine takes the `Cpu`
// where the original leaves registers a caller can read (player_regen_routines*.inc lines call it with the guest ABI's
// arguments).
//
// Dependencies still generated, called through the dispatcher (c.call): none of the pilots'. The pooled generator's
// refill 0x1077b770 is C++ (rng_core) and is called through c.call at the original's return addresses.
#include "guest_abi.hpp"

namespace cm::aot::player_regen {
using guest::Cpu;

// Block `regen_queries` (player_regen_queries.cpp): small queries over the regen object's lists and the people records.
// 0x102de3c0: the class (0..3) of the person whose id word `id_pointer` points at: bits 2..3 of byte +0xc of the 100-byte
// kind-table record for that id; nonzero marks a person already retired or inactive (inferred). Leaves r4 (the table's
// base) as the original does.
std::int32_t person_class(Cpu&, std::uint32_t id_pointer);
// 0x102de3f0: the first entry of a list A whose id equals the word `key` points at, if it also passes the test `kind` selects
// (nonzero: its kind byte is 8, a pending injury retirement; zero: its announced byte is nonzero), else 0 (a later entry
// with the same id is not looked at). Leaves r6 (the key), r7 (the entry reached) and r8 (its index) as the original does.
std::uint32_t find_retirement_entry(Cpu&, std::uint32_t list, std::uint32_t key, std::int8_t kind);
// 0x102dbe90: whether the word at +0x20 of the regen object is nonzero (inferred: the non-player staff pool has entries).
bool staff_pool_has_entries(Cpu&, std::uint32_t regen_object);
// 0x102eb4e0: qsort comparator of the list B entries (the sort passes the entries themselves, null ones first): the
// unsigned halfword at +6 ascending, then the signed halfword at +4 ascending, then the id (word +0) ascending. Leaves r4
// (when the halfwords tie) and r5 as the original does.
std::int32_t compare_queue_entries(Cpu&, std::uint32_t first, std::uint32_t second);
// 0x102eb550: comparator of the list A entries (null ones first): each entry's due date is its first date (halfword day +8,
// halfword year +0xa) unless that is earlier than the game's current date (TOC -0x7b80), then the second (+0x10, +0x12);
// the due dates are compared by year then day, then the ids. Leaves r4..r8 as the original does.
std::int32_t compare_retirement_entries(Cpu&, std::uint32_t first, std::uint32_t second);
// 0x102eb610: whether a list A has an entry whose id equals the word `key` points at (false for a null key or an empty
// list). Leaves r5..r7 as the original does.
bool retirement_queued(Cpu&, std::uint32_t list, std::uint32_t key);

// Block `candidate_score` (player_regen_random.cpp, player_regen_candidates.cpp).
// 0x102da630: the game's pooled Random(bound): 0 for a bound of 0 (no draw); else one draw (the cursor moves two bytes; past
// the pool's end it returns to the start and the salt is renewed from the generator's refill) giving the sample halfword plus
// the salt, taken modulo the bound (the remainder's sign made that of the bound); for a bound of 0x10000 or more in magnitude
// the draw is scaled instead: n / 65535 times the remainder by 65535 (doubles, truncated) plus a tenth (rounded) of a second
// draw below n * 10 / 65535 (the product wraps), capped at n - 1. The original inlines four levels and then calls itself; the
// second bound never exceeds 32768 in magnitude, so only two levels run. Leaves r4..r9 as the original does for a scaled draw.
std::int32_t draw_below(Cpu&, std::int32_t bound);
// 0x102dc410: the score of a person as the replacement for a retiring one: -10000 unless the person is a pure player
// (the staff word +0x1c is zero, the kind-table class is 0, age at least 31); otherwise a draw below (age - 31) * 150, the
// club's reputation against the player's value, a cubed term of the player record's byte +0x2f (pow, drawn from), weights of
// the attribute bytes (two of them squared through pow), 75 more for a club with no word +0x90, for an unattached person the
// role byte squared and a doubling when a draw modulo 10000 is below the player's +0xc, and last a draw below the score.
// The first argument register is ignored. Callers read r3 only (tools/register_contract.py --after).
std::int32_t candidate_score(Cpu&, std::uint32_t person);
// 0x102dbf00: the person whose candidate score is highest and positive among those whose nation record's word +0x68 equals
// `key` (the people table without its last 16), or when there is none, among all of them; 0 when no score is positive.
// Callers read r3 only.
std::uint32_t best_candidate(Cpu&, std::uint32_t key);
}

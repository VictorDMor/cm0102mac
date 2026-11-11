#pragma once
// MANAGER_MANAGER.CPP rematch block, written native-first: the board's search for a club's next
// manager and the queue of offers it files (0x101aaa90 and 0x101ac280 call each other), with the
// routines around them. Values live in native locals; the guest ABI appears at the entries and at
// the calls to code outside the block. Each routine keeps the original's frame (size and saved
// registers): the 0x28-byte offer record built in the frame of 0x101aaa90 is copied whole into game
// memory, so the stack the earlier callees left under it reaches the list's padding bytes.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// 0x101aa650: hands its second argument on to 0x101c8a80.
void forward_to_c8a80(guest::Cpu& c, std::uint32_t second);
// 0x101b2030: files a message of kind 0x1775 (the person, his three names, his club, `club`, his status)
// with the news desk, unless the event date is past today's. Needs the person, his club at +0x24 and `club`.
void post_approach_news(guest::Cpu& c, std::uint32_t person, std::uint32_t club);
// 0x101abda0: the best assessment (0x101ad080, job 5) among the people who could take `club`'s job
// (not `exclude`), -10000 when none; r3 = the score.
std::int32_t best_candidate_score(guest::Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t exclude);
// 0x101acf00: for every club with no pending question (0x100c96e0), a manager (0x100d3c40) and a clear
// answer (0x100ce320), makes sure the object's offer list holds an offer of the club, and plans a
// vacancy (0x101aaa90) when it does not.
void plan_all_vacancies(guest::Cpu& c, std::uint32_t object);
// 0x101aa510: settles the offers queued at the object (list at +4): for each, the club's manager is
// released (0x101aa680), a club with a manager of its own gets a new search (0x101aaa90), and the entry is
// removed.
void settle_queue(guest::Cpu& c, std::uint32_t object);
// 0x101aa680: ends `club`'s manager's post (`kind` and `extra` bytes as filed by the offer, `keep_rating`
// a byte): news of the move (0x101b39f0), the manager's links and flags, and the club row's rating reset
// to 0xdac unless `keep_rating`.
void end_tenure(guest::Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t kind, std::uint32_t extra,
                std::uint32_t keep_rating);
// 0x101b39f0: files the message of kind 0x1770 about `person`'s move to `club` (`kind`, `extra` and `mode`
// bytes) unless the event date is past today's or `kind` is 5. Goes to the news desk by the club's
// level (0x102cc400, 0x102cc8e0).
void post_move_news(guest::Cpu& c, std::uint32_t person, std::uint32_t kind, std::uint32_t extra, std::uint32_t club,
                    std::uint32_t mode);
// 0x101b0f90: whether `person` would take `club`'s job, from ranks, status, the club's reputation and,
// last, the tactics object's rating (0x10366470, compared with `bar`); r3 = 0 or 1.
bool candidate_willing(guest::Cpu& c, double bar, std::uint32_t person, std::uint32_t club);
// 0x101aaa90: plans `club`'s next manager (`leaving` is the person who goes, or 0): resets the club's row
// (counters, flags, ratings 5000), picks the person with the best assessment (0x101ad080, job 5) among the
// people who may take the job, falls back to the club's own slots and then to a walk back from the end of the
// table, may add a second pick by chance, chooses the two dates of the move, and appends the 0x28-byte offer
// record built in its frame to the list at object+8. Last, for a club with no pending question, a manager and
// no human flag, it asks 0x101ac280 about each of the last 16 people who may be approached.
void plan_vacancy(guest::Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t leaving);
// 0x101ac280: `person` (one of those last 16) is approached for `club`: a chance (3 in 4) that his own club's
// side reacts (a message, and the ratings of that club's row scaled), the minimum ratings of his staff record,
// his assessment, and the update of the offer already queued for `club` (looked for in the list at object+8,
// planned first by 0x101aaa90 when missing), which a better score replaces.
void approach_candidate(guest::Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t person);
}

// PLAYER_REGEN.CPP, block `detach`: a retiring person leaves his club and national team: a player (0x102e1f20) is removed from the
// club's 50 squad slots and the tactics (0x103afb90) and keeps or loses his links by his job bytes; a staff member (0x102e21c0)
// by the job byte of each side, whose cases the original reaches through two jump tables of the TOC (-0x50b4 for the club side's
// byte +0x62, -0x50b8 for the national side's byte +0x60, read from the executable: case 0 3 4 7 11 13 14 and any value above 15
// do nothing special; 1 clears a record's word +0x80; 2 the three slots at +0x84 that hold the person; 5 and 12 the manager
// slot (+0x90) with the manager manager's calls; 6 clears +0x94; 8 and 15 the five slots at +0x160 after a human-club call;
// 9 the seven slots at +0x174 and the scout manager's release; 10 the three slots at +0x190). Every case ends with the link
// cleared. Outside the module the routines call through the dispatcher at the original's return addresses.
#include "player_regen_detail.hpp"

namespace cm::aot::player_regen {
namespace {
constexpr std::uint32_t link_club = 0x24, link_nation = 0x20, job_club = 0x62, job_nation = 0x60;

// Clears the first of the 50 squad slots (+0x98) of the record the person's link names that holds the person.
void leave_squad(Cpu& c, std::uint32_t person, std::uint32_t link) {
    for (std::int32_t i = 0; sext8(std::uint32_t(i)) < 0x32; ++i) {
        const auto record = c.u32(person + link);
        if (c.u32(record + 0x98 + std::uint32_t(i) * 4) == person) {
            c.put32(record + 0x98 + std::uint32_t(sext8(std::uint32_t(i))) * 4, 0);
            return;
        }
    }
}
}

void detach_player(Cpu& c, std::uint32_t owner, std::uint32_t person) {
    save_link(c);
    (void)owner;
    Frame frame(c, 0x102e1f20, 0x50, {.first_gpr = 28});
    const auto pool = c.toc(toc::pool);
    // The registers the callees' prologues save: r28 the tactics variable's address (then the club link's keep flag), r29 the
    // person, r30 the message buffer's address, r31 the pool text.
    c.set({{28, c.toc(-0x7a6c)}, {29, person}, {30, c.toc(-0x7bb8)}, {31, pool}});
    const auto tactics = [&] { return cell(c, -0x7a6c); };
    if (c.u32(person + link_club) != 0) {
        invoke(c, 0x103afb90, 0x102e1f6c, {tactics(), c.u32(person + link_club), person, 0, 1});
        invoke(c, 0x103afb90, 0x102e1f84, {tactics(), c.u32(person + link_club), person, 1, 1});
        leave_squad(c, person, link_club);
    }
    if (c.u32(person + link_nation) != 0) {
        invoke(c, 0x103afb90, 0x102e1ff4, {tactics(), c.u32(person + link_nation), person, 0, 1});
        invoke(c, 0x103afb90, 0x102e200c, {tactics(), c.u32(person + link_nation), person, 1, 1});
        leave_squad(c, person, link_nation);
    }
    // The job bytes (signed; above 0x10 is an unknown job): 12, 13 and 15 become 5, 6 and 8 and keep the club link; 11, 14 and
    // others become 0x10; 16 reports a retired player in the message buffer; 1..10 leave everything as it is.
    bool keep_club = false, keep_nation = false, keep_club_set = false;
    const auto report = [&](std::uint32_t back) {
        if (keep_club_set) c.set(28, keep_club);
        invoke(c, 0x1077b720, back, {c.toc(-0x7bb8), pool + 0x41e6, c.u32(person + 0x28), c.u32(person + 0x2c), c.u32(person)});
    };
    {
        const auto job = sext8(c.u8(person + job_club));
        if (std::uint32_t(job) > 0x10) { c.put8(person + job_club, 0x10); }
        else switch (job) {
            case 0: break;
            case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: return;
            case 12: c.put8(person + job_club, 5); keep_club = true; break;
            case 13: c.put8(person + job_club, 6); keep_club = true; break;
            case 15: c.put8(person + job_club, 8); keep_club = true; break;
            case 16: report(0x102e20d4); break;
            default: c.put8(person + job_club, 0x10); break;  // 11, 14
        }
        keep_club_set = true;
    }
    {
        const auto job = sext8(c.u8(person + job_nation));
        if (std::uint32_t(job) > 0x10) { c.put8(person + job_nation, 0x10); }
        else switch (job) {
            case 0: break;
            case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: return;
            // The national side's cases 12, 13 and 15 write the club side's byte (+0x62), as the original does.
            case 12: c.put8(person + job_club, 5); keep_nation = true; break;
            case 13: c.put8(person + job_club, 6); keep_nation = true; break;
            case 15: c.put8(person + job_club, 8); keep_nation = true; break;
            case 16: report(0x102e2160); break;
            default: c.put8(person + job_nation, 0x10); break;  // 11, 14
        }
    }
    if (!keep_club) c.put32(person + link_club, 0);
    if (!keep_nation) c.put32(person + link_nation, 0);
}

namespace {
// The return addresses of the calls a staff member's cases make, per side.
struct Backs {
    std::uint32_t transfer, manager5, eligible5, release5, manager12, eligible12, release12, human8, training8, human15, training15, human9, scout9;
};
constexpr Backs club_backs{0x102e2294, 0x102e22ac, 0x102e22b4, 0x102e22cc, 0x102e2308, 0x102e2310, 0x102e2328, 0x102e23bc, 0x102e23d4,
                           0x102e234c, 0x102e2364, 0x102e2468, 0x102e2488};
constexpr Backs nation_backs{0x102e2578, 0x102e2590, 0x102e2598, 0x102e25b0, 0x102e25ec, 0x102e25f4, 0x102e260c, 0x102e26a0, 0x102e26b8,
                             0x102e2630, 0x102e2648, 0x102e274c, 0x102e276c};

// The slots of `count` entries at `offset` of the record the person's link names that hold the person are cleared (the link is
// read afresh for each slot, as the original does).
void clear_slots(Cpu& c, std::uint32_t person, std::uint32_t link, std::uint32_t offset, std::int32_t count) {
    for (std::int32_t i = 0; sext8(std::uint32_t(i)) < count; ++i) {
        const auto record = c.u32(person + link);
        if (c.u32(record + offset + std::uint32_t(i) * 4) == person) c.put32(record + offset + std::uint32_t(i) * 4, 0);
    }
}

// The manager manager's calls for a manager slot held by the person: release (101aa680), and (a record 0x100d3c40 accepts) 101aaa90.
void release_manager(Cpu& c, std::uint32_t person, std::uint32_t record, std::uint32_t manager_back, std::uint32_t eligible_back,
                     std::uint32_t release_back) {
    invoke(c, 0x101aa680, manager_back, {cell(c, -0x79f0), record, 3, 0, 0});
    if (sext8(invoke(c, 0x100d3c40, eligible_back, {record})) != 0) invoke(c, 0x101aaa90, release_back, {cell(c, -0x79f0), record, person});
}

// The training cases (8 and 15): a human-run record has the training manager release the person, then the five slots at +0x160.
void leave_training(Cpu& c, std::uint32_t person, std::uint32_t link, std::uint32_t human_back, std::uint32_t training_back) {
    const auto record = c.u32(person + link);
    if (sext8(invoke(c, 0x1014c2e0, human_back, {record, 1, 1, 0, 0})) != 0)
        invoke(c, 0x103d2b30, training_back, {cell(c, -0x7a7c), c.u32(person + link), 0});
    clear_slots(c, person, link, 0x160, 5);
}

// One side of the detach: the case of the job byte for the record the person's link names.
void leave_side(Cpu& c, std::uint32_t person, std::uint32_t link, std::int32_t job, const Backs& back) {
    const auto record = c.u32(person + link);
    switch (job) {
        case 1: c.put32(record + 0x80, 0); break;
        case 2: clear_slots(c, person, link, 0x84, 3); break;
        case 5:
            // The record's manager is the person: a person near the end of the table has the transfer manager told first.
            if (c.u32(record + 0x90) == person) {
                if (c.i32(person) >= std::int32_t(cell(c, -0x7be4) - 0x10)) invoke(c, 0x103e9ac0, back.transfer, {cell(c, -0x7b40), record});
                release_manager(c, person, record, back.manager5, back.eligible5, back.release5);
            }
            break;
        case 6: c.put32(record + 0x94, 0); break;
        case 8: leave_training(c, person, link, back.human8, back.training8); break;
        case 15: leave_training(c, person, link, back.human15, back.training15); break;
        case 9: {
            clear_slots(c, person, link, 0x174, 7);
            const auto current = c.u32(person + link);
            if (sext8(invoke(c, 0x1014c2e0, back.human9, {current, 1, 1, 0, 0})) != 0) {
                const auto scouted = c.u32(person + link);
                c.put32(person + link, 0);
                invoke(c, 0x1030f950, back.scout9, {cell(c, -0x7b1c), c.u32(scouted + 0x90)});
            }
            break;
        }
        case 10: clear_slots(c, person, link, 0x190, 3); break;
        case 12:
            if (c.u32(record + 0x90) == person) release_manager(c, person, record, back.manager12, back.eligible12, back.release12);
            break;
        default: break;  // 0 3 4 7 11 13 14: nothing special
    }
}
}

void detach_staff(Cpu& c, std::uint32_t owner, std::uint32_t person) {
    save_link(c);
    (void)owner;
    Frame frame(c, 0x102e21c0, 0x60, {.first_gpr = 25});
    // The registers the callees' prologues save: r25 the person, r26 the record of the side, r27-r31 the TOC variables' addresses.
    c.set({{25, person}, {27, c.toc(-0x7b1c)}, {28, c.toc(-0x7a7c)}, {29, c.toc(-0x79f0)}, {30, c.toc(-0x7b40)}, {31, c.toc(-0x7be4)}});
    // The club side, then the national side: the job byte (signed; above 15 an unknown job) selects the case; every case ends with
    // the link cleared.
    for (const bool national : {false, true}) {
        const auto link = national ? link_nation : link_club;
        if (c.u32(person + link) == 0) continue;
        c.set(26, c.u32(person + link));
        const auto job = sext8(c.u8(person + (national ? job_nation : job_club)));
        if (std::uint32_t(job) <= 0xf) leave_side(c, person, link, job, national ? nation_backs : club_backs);
        c.put32(person + link, 0);
    }
    c.put8(person + job_club, 0);
    c.put8(person + job_nation, 0);
}
}

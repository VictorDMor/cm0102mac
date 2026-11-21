// PLAYER_REGEN.CPP, block `club_placement`: the club choice of a regenerated player (0x102e99f0 over every club, 0x102ea3c0 over a
// nation's club list), the two club scores (0x102e9b90 the full one, 0x102ea670 the simple one), the per-nation club lists they
// read (0x102e98a0) and the signing (0x102dc000). Their draws are the pooled generator's (the checker compares its state): a
// score draws up to five times (the mod-20 tests, a draw below the club's kind byte, the mod-75 test, a last draw below the
// score), the bounds being bytes or halfwords so no draw is scaled. The scores are 16-bit quantities the original keeps in a
// 32-bit register, truncated (extsh) only where it compares or returns them: the code does the same. Callees outside the module
// are called through the dispatcher at the original's return addresses (their C++ owners); the module's own scores are called
// directly. r4-r10: the callers read r9 and r10 only (player_regen_volatile_read.inc), which no routine here writes.
#include "player_regen_detail.hpp"

namespace cm::aot::player_regen {
namespace {
// The records the club routines read (offsets of the person and club rows; "(inferred)" meanings).
namespace p {
constexpr std::uint32_t nation = 0x0c, nation_alt = 0x10 /* pointer: its word +0x68 against the English nation's */, player = 0x14, age = 0x5c;
}
namespace pl {  // the player record
constexpr std::uint32_t value_4 = 0x04, value_6 = 0x06, value_a = 0x0a;
}
namespace cl {  // the club row
constexpr std::uint32_t id = 0x00, nation = 0x2c, manager_link = 0x30, manager = 0x90, reputation = 0x1ec, kind = 0x240, squad = 0x98;
}
constexpr std::uint32_t nation_key = 0x68;

std::int32_t absolute(std::int32_t v) { return v < 0 ? std::int32_t(0u - std::uint32_t(v)) : v; }
// Signed remainder as divw / mullw / subf give it.
std::int32_t remainder(std::int32_t sum, std::int32_t n) { return std::int32_t(std::uint32_t(sum) - std::uint32_t(divide_signed(sum, n)) * std::uint32_t(n)); }
// A halfword the way extsh makes it, as a 32-bit value.
std::int32_t half(std::int32_t v) { return sext16(std::uint32_t(v)); }

// Whether a club passes the English-exclusion test shared by the two club choices: a person with no second nation record
// (+0x10), or whose record's word +0x68 differs from the English nation's, or a club among five named ones.
bool club_open_to(Cpu& c, std::uint32_t person, std::uint32_t club) {
    const auto other = c.u32(person + p::nation_alt);
    if (other == 0) return true;
    if (c.i32(other + nation_key) != c.i32(c.toc(-0x6678))) return true;
    const auto id = c.i32(club + cl::id);
    for (const std::int32_t slot : {-0x75cc, -0x75d0, -0x7724, -0x7864, -0x773c})
        if (id == c.i32(c.toc(slot))) return true;
    return false;
}
}

std::int32_t club_rating(Cpu& c, std::uint32_t person, std::uint32_t club) {
    save_link(c);
    const Pool pool(c);
    const auto reputation = sext16(c.u16(club + cl::reputation));
    const auto player = [&] { return c.u32(person + p::player); };
    // 5000 less the distances of the player's three halfwords (the last two times 50) from the club's reputation, plus it.
    auto score = half(5000 - absolute(sext16(c.u16(player() + pl::value_a)) - reputation));
    score -= half(absolute(sext16(c.u16(player() + pl::value_6)) * 50 - reputation));
    score -= half(absolute(sext16(c.u16(player() + pl::value_4)) * 50 - reputation));
    score += reputation;
    const auto bound = half(score);
    // Random(bound) inline; the bound is a halfword, so its scaled form never runs.
    return half(pool.draw(bound, 0x102ea72c, refill_site::random_first));
}

std::int32_t club_score(Cpu& c, std::uint32_t person, std::uint32_t club, std::uint32_t level_register) {
    save_link(c);
    // The original's frame and the registers its callees' prologues save: the callees run at the original's stack depth and
    // find the original's values in the registers they save (r19 person, r20 club, r21 level, r22 the running score, r25-r31
    // the TOC variables' addresses).
    Frame frame(c, 0x102e9b90, 0x90, {.first_gpr = 19});
    c.set({{19, person}, {20, club}, {21, level_register}, {25, c.toc(-0x7a10)}, {26, c.toc(-0x757c)}, {27, c.toc(toc::rng_salt)},
           {28, c.toc(toc::rng_start)}, {29, c.toc(toc::rng_end)}, {30, c.toc(toc::rng_cursor)}, {31, c.toc(toc::constants)}});
    const Pool pool(c);
    const auto reputation = [&] { return sext16(c.u16(club + cl::reputation)); };
    const auto player = [&] { return c.u32(person + p::player); };
    const auto value_6 = [&] { return sext16(c.u16(player() + pl::value_6)); };
    const auto level = sext8(level_register);
    std::int32_t score = 0;
    std::uint32_t r23 = c.r(23), r24 = c.r(24);  // the draw below the kind byte and the kind byte, as the original's registers hold them
    const auto sync = [&] { c.set({{22, std::uint32_t(score)}, {23, r23}, {24, r24}}); };
    const auto asks_human = [&](std::uint32_t back) { sync(); return sext8(invoke(c, 0x1014c2e0, back, {club, 1, 1, 0, 0})) != 0; };
    const auto rate = [&](std::uint32_t back, std::uint32_t staff) { sync(); return sext8(invoke(c, 0x100d2f50, back, {staff, c.u8(staff + 0x38)})); };

    score = half(5000 - absolute(sext16(c.u16(player() + pl::value_a)) - reputation()));
    score -= half(absolute(half(sext16(c.u16(player() + pl::value_6)) * 50) - reputation()));
    score -= half(absolute(half(sext16(c.u16(player() + pl::value_4)) * 50) - reputation()));

    // 1. The first draw: a mod-20 test against the player's value_6.
    pool.advance(0x102e9c54);
    score += reputation();
    if (value_6() > remainder(pool.sum(), 20) + 100) {
        const auto kind = c.u8(club + cl::kind);
        if (kind == 0x14) {
            score += half(reputation() / 2 + 7000);
        } else {
            // A draw below the kind byte (signed), when it is not zero, and a second mod-20 test of it.
            const auto kind_signed = sext8(kind);
            r24 = std::uint32_t(kind_signed);
            c.set({{22, std::uint32_t(score)}, {24, r24}});
            const auto chosen = kind_signed == 0 ? 0 : pool.draw(kind_signed, 0x102e9cf8, refill_site::random_first);
            r23 = std::uint32_t(chosen);
            pool.advance(0x102e9e40);
            if (chosen > remainder(pool.sum(), 20))
                score += half(reputation() / 2 + sext8(c.u8(club + cl::kind)) * 50 + 5000);
        }
    }

    // 2. A club with a manager link whose table entry exists: a human manager's bonus and the reputation's by the level.
    if (value_6() > 0x32) {
        const auto link = c.u32(club + cl::manager_link);
        if (link != 0 && c.u32(cell(c, -0x7b5c) + c.u32(link) * 4) != 0) {
            if (asks_human(0x102e9efc)) {
                const auto holder = cell(c, -0x7984);
                if (holder != 0 && value_6() > 0x78) {
                    const auto mark = sext8(c.u8(holder + c.u32(club + cl::id) * 8 + 4));
                    if (mark < 0) score += 0x1f4;
                    else if (mark == 0) score += 0xfa;
                }
            }
            const auto rep = reputation();
            const auto lvl = level;
            if (rep > 0x1676) score += lvl < 0x28 ? 0x3e8 : lvl < 0x2d ? 0x2ee : 0xfa;
            else if (rep > 0x109a) score += lvl < 0x23 ? 0x2ee : lvl < 0x28 ? 0x1f4 : 0xfa;
            else score += 0xfa;
        }
    }

    // 3. The club's manager record (+0x90): its attribute draws, or (a club with none, or a human's) the plain bonus.
    const auto manager = c.u32(club + cl::manager);
    const bool plain = manager == 0 || c.u32(manager + 0x1c) == 0 || asks_human(0x102e9ff0);
    if (!plain) {
        pool.advance(0x102ea038);
        score += half((sext8(c.u8(c.u32(c.u32(club + cl::manager) + 0x1c) + 0x42)) - 10) * 0x1e);
        if (value_6() > remainder(pool.sum(), 75) + 100) {
            const auto staff = c.u32(c.u32(club + cl::manager) + 0x1c);
            const auto rating = rate(0x102ea094, staff);
            score += half((rating - 10) * 0x3c);
        }
        const auto staff = c.u32(c.u32(club + cl::manager) + 0x1c);
        const auto rating = rate(0x102ea0b8, staff);
        const auto sum = rating + sext8(c.u8(club + cl::kind)) + sext8(c.u8(c.u32(c.u32(club + cl::manager) + 0x1c) + 0x42));
        if (sum > 0x28 && reputation() > 0x109a && value_6() > 0x50) {
            if (half(score) < reputation()) score = reputation();
            else score += 0xfa;
        }
    } else if (asks_human(0x102ea134)) {
        score += 0xc8;
        if (value_6() > 0x78) {
            const auto kind = c.u8(club + cl::kind);
            if (kind == 0x14 && half(score) < reputation()) score = reputation();
            score += half(sext8(kind) * 0x19);
        }
    }

    // 4. The same nation as the club's, and the special clubs and nations (the nation records' word +0x68 is read even for a
    // null nation: address 0x68).
    const auto nation = c.u32(person + p::nation), club_nation = c.u32(club + cl::nation);
    if (nation == club_nation) score = half(score + 0x3e8);
    const auto id = c.i32(club + cl::id);
    if (id == c.i32(c.toc(-0x7abc)) || id == c.i32(c.toc(-0x7ac0)) || (id == c.i32(c.toc(-0x7ac4)) && nation != 0)) {
        const auto key = c.i32(nation + nation_key);
        if (key == c.i32(c.toc(-0x757c))) score += 0x3e8;
        else if (key == c.i32(c.toc(-0x7a10))) score -= 0x2ee;
    }
    const auto marked = c.i32(c.toc(-0x7a1c));
    if (c.i32(club_nation + nation_key) == marked) {
        const auto key = c.i32(nation + nation_key);
        if (key == c.i32(c.toc(-0x757c))) score -= 0x2ee;
        else if (key == c.i32(c.toc(-0x7a10))) score -= 0x2ee;
        else if (key == marked) score += 0x3e8;
    }

    // 5. The last draw, below the score.
    return half(pool.draw(half(score), 0x102ea278, refill_site::random_first));
}

std::uint32_t best_destination_club(Cpu& c, std::uint32_t owner, std::uint32_t person) {
    save_link(c);
    // The original's frame: the two buffers the callees fill (at +0x38 and +0x3a). Its registers as the callees' prologues save
    // them: r16 the member count, r17 the owner, r18 the person, r19 the club index, r20 the best club, r21 the club, r22 the
    // best score, r23-r31 the TOC variables' addresses.
    Frame frame(c, 0x102e99f0, 0x80, {.first_gpr = 16});
    std::uint32_t best = 0, best_raw = std::uint32_t(-25000), members = c.r(16);
    std::int32_t index = 0;
    auto club = cell(c, -0x7be8);
    c.set({{17, owner}, {18, person}, {23, c.toc(-0x773c)}, {24, c.toc(-0x7864)}, {25, c.toc(-0x7724)}, {26, c.toc(-0x75d0)}, {27, c.toc(-0x75cc)},
           {28, c.toc(-0x7b50)}, {29, c.toc(-0x6678)}, {30, c.toc(-0x7b40)}, {31, c.toc(-0x7b04)}});
    const auto call = [&](std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        c.set({{16, members}, {19, std::uint32_t(index)}, {20, best}, {21, club}, {22, best_raw}});
        return invoke(c, target, back, arguments);
    };
    // Every club but the last two of each nation.
    for (; index < std::int32_t(cell(c, -0x7b50) - cell(c, -0x7b04) * 2); ++index, club += 0x248) {
        if (std::int8_t(call(0x100cab10, 0x102e9a58, {person, c.u32(club + cl::nation), 1})) != 0) continue;
        const auto found = call(0x100ce320, 0x102e9a70, {club, frame.at(0x38), 1});
        if (found != 0 && sext8(c.u8(frame.at(0x38))) == 0) continue;
        if (!club_open_to(c, person, club)) continue;
        members = call(0x100c9070, 0x102e9af0, {club, 0, 0});
        if (sext8(call(0x103f83c0, 0x102e9b10, {cell(c, -0x7b40), person, club, frame.at(0x3a), 0})) == 0) continue;
        if (sext8(members) >= 0x32) continue;
        c.set({{16, members}, {19, std::uint32_t(index)}, {20, best}, {21, club}, {22, best_raw}});
        const auto score = std::uint32_t(club_score(c, person, club, members));
        if (sext16(score) > sext16(best_raw)) {
            best = club;
            best_raw = score;
        }
    }
    return best;
}

std::uint32_t best_club_of_nation_list(Cpu& c, std::uint32_t lists, std::uint32_t person) {
    save_link(c);
    Frame frame(c, 0x102ea3c0, 0xb0, {.first_gpr = 15});
    const Pool pool(c);
    std::uint32_t best = 0, best_raw = std::uint32_t(-25000), club = 0;
    std::int32_t nation = 0, index = 0;
    // The registers the callees' prologues save: r15 the message buffer, r16 the pool text, r17 the club, r18 the best score,
    // r19 the list offset, r20-r25 the named ids' and keys' addresses, r26 the club table's, r27 the gate's, r28 the lists object,
    // r29 the person, r30 the nation then the list index, r31 the best club.
    c.set({{15, c.toc(-0x7ba8)}, {16, c.toc(toc::pool)}, {20, c.toc(-0x773c)}, {21, c.toc(-0x7864)}, {22, c.toc(-0x7724)}, {23, c.toc(-0x75d0)},
           {24, c.toc(-0x75cc)}, {25, c.toc(-0x6678)}, {26, c.toc(-0x7be8)}, {27, c.toc(-0x7b40)}, {28, lists}, {29, person}});
    const auto call = [&](std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        c.set({{17, club}, {18, best_raw}, {19, std::uint32_t(nation) * 4}, {30, std::uint32_t(index)}, {31, best}});
        return invoke(c, target, back, arguments);
    };
    // The nation whose list is searched: the person's, or the home nation (-0x7a10) for the person's own if it is one of two
    // special nations or, the third, with a draw mod 5 below 2.
    nation = c.i32(c.u32(person + p::nation) + nation_key);
    bool home = nation == c.i32(c.toc(-0x757c)) || nation == c.i32(c.toc(-0x7574));
    if (!home && nation == c.i32(c.toc(-0x7578))) {
        c.set({{17, c.toc(toc::rng_cursor)}, {19, c.toc(toc::rng_salt)}, {30, std::uint32_t(nation)}});
        pool.advance(0x102ea470);
        home = remainder(pool.sum(), 5) < 2;
    }
    if (home) nation = c.i32(c.toc(-0x7a10));
    const auto list_of = [&] { return c.u32(c.u32(lists) + std::uint32_t(nation) * 4); };
    auto list = list_of();
    if (list == 0 || c.i16(list) == 0) return 0;
    c.put32(list + 0xc, c.u32(list + 4));
    for (; half(index) < c.i16(list_of()); ++index) {
        const auto next = call(0x10738e00, 0x102ea4f8, {list_of()});
        if (next == 0) {
            const auto pool_text = c.toc(toc::pool);
            guest::report_assertion(c, {0x102ea514, pool_text + 0x37, pool_text + 0x48, pool_text + 0x55, frame.at(0x44), frame.at(0x3c), 0x19e9});
            return 0;
        }
        club = cell(c, -0x7be8) + c.u32(next) * 0x248;
        if (!club_open_to(c, person, club)) continue;
        if (sext8(call(0x103f83c0, 0x102ea5e4, {cell(c, -0x7b40), person, club, frame.at(0x38), 0})) == 0) continue;
        if (sext8(call(0x100c9070, 0x102ea5fc, {club, 0, 0})) >= 0x32) continue;
        c.set({{17, club}, {18, best_raw}, {19, std::uint32_t(nation) * 4}, {30, std::uint32_t(index)}, {31, best}});
        const auto score = std::uint32_t(club_rating(c, person, club));
        if (sext16(score) > sext16(best_raw)) {
            best = club;
            best_raw = score;
        }
    }
    return best;
}

void init_nation_club_lists(Cpu& c, std::uint32_t lists) {
    save_link(c);
    Frame frame(c, 0x102e98a0, 0x60, {.first_gpr = 24});
    // r24 the object, r25 the nation index, r26 the lists object, r27 the nation count's address and later the club, r28 the byte
    // offset and later the club index, r29-r31 the club count's and the two excluded ids' addresses.
    std::uint32_t object = c.r(24), club = 0;
    std::int32_t index = 0;
    c.set({{26, lists}, {27, c.toc(-0x7b04)}, {29, c.toc(-0x7b50)}, {30, c.toc(-0x7208)}, {31, c.toc(-0x7204)}});
    const auto call = [&](std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        c.set({{24, object}, {25, std::uint32_t(index)}, {27, club ? club : c.toc(-0x7b04)}, {28, club ? std::uint32_t(index) : std::uint32_t(index) * 4}});
        return invoke(c, target, back, arguments);
    };
    c.put32(lists, call(0x10775130, 0x102e98d0, {cell(c, -0x7b04) << 4}));
    for (; index < c.i32(c.toc(-0x7b04)); ++index) {
        object = call(0x10773520, 0x102e98ec, {0x10});
        if (object != 0) call(0x10738c00, 0x102e98f8, {object});
        c.put32(c.u32(lists) + std::uint32_t(index) * 4, object);
    }
    index = 0;
    club = cell(c, -0x7be8);
    for (; index < c.i32(c.toc(-0x7b50)); ++index, club += 0x248) {
        if (sext8(call(0x100c7d00, 0x102e992c, {club})) != 0) continue;
        const auto nation = c.u32(club + cl::nation);
        if (nation == 0 || c.u32(nation + 0x74) == 0) continue;
        // A nation flagged (+0x120) takes only a club of reputation 2 or less with a manager link; an unflagged one any club
        // with a manager link.
        const auto flagged = sext8(c.u8(nation + 0x120)) != 0;
        if (flagged ? !(sext16(c.u16(club + cl::reputation)) <= 2 && c.u32(club + cl::manager_link) != 0) : c.u32(club + cl::manager_link) == 0) continue;
        const auto id = c.i32(club + cl::id);
        if (id == c.i32(c.toc(-0x7204)) || id == c.i32(c.toc(-0x7208))) continue;
        call(0x10738d10, 0x102e99bc, {c.u32(c.u32(lists) + c.u32(nation + nation_key) * 4), club, 4});
    }
}

bool sign_new_player(Cpu& c, std::uint32_t owner, std::uint32_t person, std::uint32_t club) {
    save_link(c);
    Frame frame(c, 0x102dc000, 0x70, {.first_gpr = 22});
    (void)owner;
    const Pool pool(c);
    // The registers the callees' prologues save: r22 the person, r23 the club, r24 and r28 the date draws' values, r25-r31 the TOC
    // variables' addresses (r26 the salt cell until the second draw's remainder replaces it).
    std::uint32_t r24 = c.r(24), r26 = c.toc(toc::rng_salt), r28 = c.toc(toc::rng_end);
    c.set({{22, person}, {23, club}, {25, c.toc(-0x7a98)}, {27, c.toc(toc::rng_start)}, {29, c.toc(toc::rng_cursor)}, {30, c.toc(-0x7b7c)}, {31, c.toc(-0x7b80)}});
    const auto call = [&](std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        c.set({{24, r24}, {26, r26}, {28, r28}});
        return invoke(c, target, back, arguments);
    };
    std::int32_t slot = 0;
    while (slot < 0x32 && c.u32(club + cl::squad + std::uint32_t(slot) * 4) != 0) ++slot;
    if (slot == 0x32) return false;
    c.put32(club + cl::squad + std::uint32_t(slot) * 4, person);
    c.put32(person + 0x24, club);
    c.put32(person + 4, 0);
    const auto contracts = [&] { return cell(c, -0x7b7c); };
    c.put32(person + 8, call(0x10084be0, 0x102dc0a4, {contracts(), person, club, 1, 0xffffffffu, 0}));
    call(0x100ce200, 0x102dc0b4, {person, 1});
    // One chance in four: the club's favour (0x100da110).
    pool.advance(0x102dc0d4);
    if ((std::uint32_t(pool.sum()) & 3) == 0) call(0x100da110, 0x102dc114, {person, club, 0});
    if (c.u8(club + 0x23d) == 3 && c.i32(person + 8) > 5000) c.put32(person + 8, 5000);
    const auto year = [&] { return std::uint32_t(c.u16(c.toc(toc::year_date) + date::year)); };
    if (sext8(c.u16(person + p::age)) < 0x13) {
        // Under 19: a day 1..28 (a draw mod 28 + 1) of the club's month, in the year after next.
        r28 = year();
        pool.advance(0x102dc168);
        const auto rest = remainder(pool.sum(), 28);
        r24 = std::uint32_t(rest);
        const auto day = rest + 1;
        const auto month = call(0x10089e40, 0x102dc1b0, {contracts(), club});
        call(0x107156f0, 0x102dc1d0, {frame.at(0x40), std::uint32_t(sext8(std::uint32_t(day))), month, (year() + 2) & 0xffff, 0xffffffffu});
        c.put16(person + 0x56, c.u16(frame.at(0x40) + 2));
        c.put16(person + 0x54, c.u16(frame.at(0x40)));
        c.put8(person + 0x58, c.u8(frame.at(0x40) + 4));
    } else {
        // 19 or older: the day as above and a year 1..6 after the next (a second draw mod 6).
        r24 = year();
        pool.advance(0x102dc210);
        const auto first = pool.sum();
        pool.advance(0x102dc268);
        r28 = std::uint32_t(remainder(first, 28));
        const auto day = remainder(first, 28) + 1;
        r26 = std::uint32_t(remainder(pool.sum(), 6));
        const auto years = remainder(pool.sum(), 6) + 1;
        const auto month = call(0x10089e40, 0x102dc2a8, {contracts(), club});
        call(0x107156f0, 0x102dc2cc, {frame.at(0x38), std::uint32_t(sext8(std::uint32_t(day))), month, (year() + std::uint32_t(years)) & 0xffff, 0xffffffffu});
        c.put16(person + 0x56, c.u16(frame.at(0x38) + 2));
        c.put16(person + 0x54, c.u16(frame.at(0x38)));
        c.put8(person + 0x58, c.u8(frame.at(0x38) + 4));
    }
    call(0x10078e70, 0x102dc2f4, {contracts(), person, c.u32(person + 8)});
    if (sext8(call(0x1014c2e0, 0x102dc30c, {club, 1, 1, 0, 0})) != 0) {
        call(0x1007ee50, 0x102dc324, {contracts(), person, club});
        auto record = call(0x10082f20, 0x102dc330, {contracts(), person});
        c.put8(record + 0x60, (c.u8(record + 0x60) & 0xf0) | 6);
        r24 = 1;
        record = call(0x10082f20, 0x102dc350, {contracts(), person});
        c.put8(record + 0x38, (c.u8(record + 0x38) & 0x03) | 4);
    }
    const auto record = call(0x10082f20, 0x102dc368, {contracts(), person});
    if (record != 0) {
        r24 = record;
        call(0x10378d30, 0x102dc378, {record, person});
        c.put8(record + 0x40, 0);
        if (sext8(call(0x1014c2e0, 0x102dc398, {club, 1, 1, 0, 0})) == 0) {
            const auto expiry = year() + 5;
            if (c.u16(record + 0x32) > (expiry & 0xffff)) {
                c.put16(record + 0x32, expiry);
                c.put16(record + 0x30, 0);
            }
        }
    }
    if (sext8(call(0x104ae570, 0x102dc3d4, {cell(c, -0x7a98), club, 0})) != 0) call(0x104ad840, 0x102dc3ec, {cell(c, -0x7a98), club, 0});
    return true;
}
}

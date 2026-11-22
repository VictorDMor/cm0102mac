// PLAYER_REGEN.CPP, block `news_posting`: the three news messages the module posts (0xbce youth promotion, 0xbba and 0xbbb retirements).
// Each routine builds the news item on its own frame (at sp+0x38 or sp+0xb0, as the original: the item's address is an argument every
// call sees), sets its words one callee call at a time and hands it to the news manager. Every callee (the news item's constructor and
// setter, the contract manager, the readers lists) runs through the dispatcher at the original's return address. r13-r31 are set as the
// original holds them at each call (the callees save them in their own frames). r4-r10 at return are the last callee's leftovers, except
// where the original sets them with no call after: the early exits (a null club leaves the club in r4) and the dispatch of 0x102e0090 whose
// loop can run no call (its r4, r5, r6 are set where the original loads them).
//
// 0x102df780 draws once per coach it looks at (the pooled generator, one refill site) and computes the draw's score with the one fused
// multiply-add of the module's news (fmadd f0,f3,f1,f0: the constant at +0x98 of the unit's table times the club's kind byte, plus the
// reputation's half and the noise). Fused against separate matters only if the constant is not exact; see the checks.
#include "player_regen_news_detail.hpp"

namespace cm::aot::player_regen {
namespace {
using namespace news;
constexpr std::uint32_t item_youth = 0x38, item_retire = 0xb0;
constexpr std::uint32_t fmadd_site = 0x102df95c;  // fmadd f0,f3,f1,f0 in 0x102df780

// The "great" or "good" bound of a club's reputation (a halfword): reputation / 50 + 75 and reputation / 100 + 50, as the compiler's
// multiplications by 0x51eb851f give them (round toward zero).
std::int32_t divided(std::int32_t value, int shift) {
    const auto high = mulhw(0x51eb851f, value);
    const auto shifted = high >> shift;
    return shifted + std::int32_t(std::uint32_t(shifted) >> 31);
}
}

void announce_youth_promotion(Cpu& c, std::uint32_t person, std::uint32_t club) {
    Frame frame(c, 0x102df780, 0x170, {.first_gpr = 18});
    c.set({{19, person}, {20, club}, {27, c.toc(toc::rng_salt)}, {28, c.toc(toc::rng_start)}, {29, c.toc(toc::rng_end)},
           {30, c.toc(toc::rng_cursor)}, {31, c.toc(toc::constants)}, {24, 0}, {23, 0}, {22, 0}});
    if (person == 0 || club == 0) return;
    const auto item = frame.at(item_youth);
    invoke(c, entry::begin_item, 0x102df7d8, {item, 0xbce, 0});
    const auto set = [&](std::uint32_t index, std::uint32_t value, std::uint32_t back) { invoke(c, entry::set_word, back, {item, index, value}); };
    const auto name_id = [&](std::uint32_t record_pointer) { return c.u32(c.u32(record_pointer) + 0x20); };
    set(0, c.u32(person), 0x102df7e8);
    set(1, name_id(person + 0x28), 0x102df7fc);
    set(2, name_id(person + 0x2c), 0x102df810);
    set(3, name_id(person + 0x30), 0x102df824);
    set(4, c.u32(club), 0x102df834);
    set(5, std::uint32_t(sext8(c.u16(person + 0x5c))), 0x102df848);
    // The club's kind byte (+0x240): the compiler's test of a signed byte against 15 (x = b ^ 15; ((x >> 1) - (x & b)) >> 31).
    const auto kind_byte = sext8(c.u8(club + 0x240));
    const auto x = std::uint32_t(kind_byte) ^ 0xfu;
    set(6, (std::uint32_t(std::int32_t(x) >> 1) - (x & std::uint32_t(kind_byte))) >> 31, 0x102df874);

    // The assistant manager (+0x94) and the five coaches (+0x160...): the first that scores "great" or "good" for the club's reputation is
    // the prospect's discoverer; the loop looks at both scores of each until one qualified.
    c.set({{25, 0xffffffffu}, {26, 0xfffffffcu}});
    Pool pool(c);
    Fpu fp(c);
    std::uint32_t chosen = 0, first_grade = 0, second_grade = 0;
    for (std::int32_t position = -1; position < 5 && chosen == 0; ++position) {
        const auto coach = position < 0 ? c.u32(club + 0x94) : c.u32(club + 0x160 + std::uint32_t(position) * 4);
        c.set(21, coach);
        if (coach != 0) {
            pool.advance(0x102df8c4);
            const auto noise = pool.sum() - divide_signed(pool.sum(), 2500) * 2500;
            const auto reputation = std::int32_t(c.i16(club + 0x1ec));
            const auto half = (reputation + std::int32_t(std::uint32_t(reputation) >> 31)) >> 1;
            const auto constants = c.toc(toc::constants);
            const auto bias = c.f64(constants + constant::bias);
            const auto kind = fp.fsub(biased(kind_byte), bias);
            const auto base = fp.fsub(biased(half + noise), bias);
            const auto score = fp.multiply_add(fmadd_site, c.f64(constants + 0x98), kind, base);
            const auto expected = std::uint32_t(fp.fctiwz(0, score));
            const auto first = invoke(c, 0x10219e40, 0x102df970, {coach, person, club, 2, expected});
            c.set(18, first);
            const auto second = invoke(c, 0x10219e40, 0x102df990, {coach, person, club, 1, first});
            const auto great = divided(reputation, 4) + 0x4b, good = divided(reputation, 5) + 0x32;
            if (sext16(first) > great) { chosen = coach; first_grade = 2; }
            else if (sext16(first) > good) { first_grade = 1; chosen = coach; }
            c.set({{22, chosen}, {24, first_grade}});
            if (sext16(second) > great) { chosen = coach; second_grade = 2; }
            else if (sext16(second) > good) { second_grade = 1; chosen = coach; }
            c.set({{22, chosen}, {23, second_grade}});
        }
        c.set({{26, std::uint32_t(std::int32_t(c.r(26)) + 4)}, {25, std::uint32_t(position + 1)}});
    }
    if (chosen == 0) {
        constexpr std::uint32_t backs[] = {0x102dfb1c, 0x102dfb2c, 0x102dfb3c, 0x102dfb4c, 0x102dfb5c, 0x102dfb6c, 0x102dfb7c};
        for (std::uint32_t index = 7; index < 14; ++index) set(index, 0xffffffffu, backs[index - 7]);
    } else {
        // The discoverer's own fields, from index 8 on (the original counts in r19 and advances it before each call).
        c.set(19, 8);
        const auto is_assistant = chosen == c.u32(club + 0x94);
        set(7, is_assistant ? 1 : 0, is_assistant ? 0x102dfa70 : 0x102dfa88);
        c.set(19, 9);
        set(8, c.u32(chosen), 0x102dfa9c);
        c.set(19, 10);
        set(9, name_id(chosen + 0x28), 0x102dfab4);
        c.set(19, 11);
        set(10, name_id(chosen + 0x2c), 0x102dfacc);
        c.set(19, 12);
        set(11, name_id(chosen + 0x30), 0x102dfae4);
        c.set(19, 13);
        set(12, std::uint32_t(sext8(first_grade)), 0x102dfaf8);
        set(13, std::uint32_t(sext8(second_grade)), 0x102dfb08);
    }
    invoke(c, 0x102cc030, 0x102dfba0, {cell(c, cells::news_manager), item, club, 0, 0, 0, 0});
}

void announce_retirement(Cpu& c, std::uint32_t person, std::uint32_t reason, std::uint32_t flag) {
    Frame frame(c, 0x102e0090, 0x1c0, {.first_gpr = 25});
    const auto news_cell = c.toc(cells::news_manager), contract_cell = c.toc(cells::contract_result);
    c.set({{30, news_cell}, {31, contract_cell}, {28, person}, {29, reason}, {25, flag}});
    if (person == 0) return;
    const auto player = c.u32(person + 0x14);
    if (player != 0) {
        const auto job = sext8(c.u8(person + 0x62));
        if (job == 8 || job == 5) return;
    }
    const auto club = c.u32(person + 0x24);
    c.set(4, club);
    if (club == 0) return;
    if (sext8(invoke(c, 0x102cdbf0, 0x102e00f4, {c.u32(news_cell), club})) == 0) return;

    const auto item = frame.at(item_retire), list = frame.at(0x38);
    invoke(c, entry::begin_item, 0x102e0110, {item, 0xbba, 0});
    const auto set = [&](std::uint32_t index, std::uint32_t value, std::uint32_t back) { invoke(c, entry::set_word, back, {item, index, value}); };
    const auto name_id = [&](std::uint32_t record_pointer) { return c.u32(c.u32(record_pointer) + 0x20); };
    set(0, c.u32(person), 0x102e0120);
    set(1, name_id(person + 0x28), 0x102e0134);
    set(2, name_id(person + 0x2c), 0x102e0148);
    set(3, name_id(person + 0x30), 0x102e015c);
    set(4, std::uint32_t(sext8(reason)), 0x102e016c);
    set(5, std::uint32_t(sext8(flag)), 0x102e017c);
    if (c.u32(person + 0x24) != 0) set(6, c.u32(c.u32(person + 0x24)), 0x102e0198);
    else set(6, 0xffffffffu, 0x102e01ac);
    if (c.u8(person + 0x5e) != 0) set(7, c.u32(c.u32(person + 0xc) + 0x68), 0x102e01cc);
    else set(7, 0xffffffffu, 0x102e01e0);
    // The job as the readers know it: 15 reads as 8 and 13 as 6.
    auto job = c.u8(person + 0x62);
    if (sext8(job) == 0xf) job = 8;
    else if (sext8(job) == 0xd) job = 6;
    set(8, std::uint32_t(sext8(job)), 0x102e0218);
    set(9, c.u8(person + 0x5e), 0x102e0228);
    set(10, std::uint32_t(sext8(c.u8(person + 0x60))), 0x102e023c);
    if (c.u32(person + 0x14) != 0 && c.u32(person + 0x1c) != 0) set(11, 1, 0x102e0264);
    else set(11, 0, 0x102e0278);
    set(12, std::uint32_t(sext8(c.u16(person + 0x5c))), 0x102e028c);
    if (c.u32(person + 0x1c) != 0) set(13, 1, 0x102e02a8);
    else set(13, 0, 0x102e02bc);
    if (c.u32(person + 0x14) != 0) set(14, 1, 0x102e02d8);
    else set(14, 0, 0x102e02ec);

    // The human managers who may be interested (at most 30, listed at sp+0x38): only for a player, and only with managers.
    const auto managers = cell(c, cells::managers);
    std::uint32_t count = 0;
    if (managers != 0 && c.u32(person + 0x14) != 0) count = invoke(c, 0x103fc710, 0x102e031c, {managers, person, 10, list, 0x1e});
    const auto manager = [&] { return c.u32(news_cell); };
    const auto nation = c.u32(person + 0xc);
    c.set(5, nation);
    // The prominence tests: a nation with a flag byte above 8 and either a player of reputation above 0x2134 (+0xc of the player record) or
    // a staff member of that level at a job 5; second tier: 0x1d4c / 0x1c52; third: the club's manager's post.
    bool top = false;
    if (nation != 0 && sext8(c.u8(nation + 0x11f)) > 8) {
        const auto record = c.u32(person + 0x14);
        c.set(4, record);
        if (record != 0 && c.i16(record + 0xc) > 0x2134) top = true;
        else {
            const auto staff = c.u32(person + 0x1c);
            c.set(4, staff);
            if (staff != 0 && c.i16(staff + 0x2c) > 0x222e && c.u8(person + 0x62) == 5) top = true;
        }
    }
    if (top) {
        invoke(c, 0x102cc8e0, 0x102e0388, {manager(), item});
    } else {
        const auto record = c.u32(person + 0x14);
        c.set(4, record);
        bool second = record != 0 && c.i16(record + 0xc) > 0x1d4c;
        if (!second) {
            const auto staff = c.u32(person + 0x1c);
            c.set(6, staff);
            second = staff != 0 && c.i16(staff + 0x2a) > 0x1c52 && c.u8(person + 0x62) == 5;
        }
        if (second) {
            invoke(c, 0x102cc400, 0x102e03d4, {manager(), item});
            const auto home = c.u32(person + 0x24);
            if (home != 0) {
                c.set(5, c.u32(home + 0x2c));
                if (c.u32(home + 0x2c) != c.u32(person + 0xc)) invoke(c, 0x102cc400, 0x102e03fc, {manager(), item});
            }
        } else {
            const auto home = c.u32(person + 0x24);
            c.set(6, home);
            std::uint32_t managers_post = 0;
            if (home != 0) {
                managers_post = c.u32(home + 0x30);
                c.set(5, managers_post);
            }
            if (home != 0 && managers_post != 0 && c.u8(person + 0x62) == 5) {
                invoke(c, 0x102cc280, 0x102e0430, {manager(), item});
            } else if (record == 0) {
                invoke(c, 0x102cc140, 0x102e044c, {manager(), item, home});
            } else {
                c.set({{26, std::uint32_t(sext8(count))}, {27, list}, {25, 0}});
                for (std::int32_t n = 0; n < sext8(count); ++n) {
                    invoke(c, 0x102cc140, 0x102e0470, {manager(), item, c.u32(list + std::uint32_t(n) * 4)});
                    c.set({{27, list + std::uint32_t(n + 1) * 4}, {25, std::uint32_t(n + 1)}});
                }
            }
        }
    }
    if (sext8(reason) == 8) {
        const auto record = c.u32(person + 0x14);
        if (record != 0 && c.i16(record + 0xa) > 0x109a) {
            const auto holder = invoke(c, 0x10082f20, 0x102e04b8, {cell(c, cells::contract_manager), person});
            c.put32(contract_cell, holder);
            const auto holders_club = holder != 0 ? cell(c, cells::clubs) + c.u32(holder + 4) * row::club_size : 0;
            if (holders_club != 0) invoke(c, 0x100edc10, 0x102e04f4, {holders_club, person, c.u32(person + 8)});
        }
    }
}

void announce_staff_retirement(Cpu& c, std::uint32_t person, std::uint32_t reason) {
    Frame frame(c, 0x102e13c0, 0x1b0, {.first_gpr = 28});
    const auto news_cell = c.toc(cells::news_manager), contract_cell = c.toc(cells::contract_result);
    c.set({{31, news_cell}, {30, person}, {29, reason}, {28, contract_cell}});
    if (person == 0) return;
    if (c.u32(person + 0x14) != 0) {
        const auto job = sext8(c.u8(person + 0x62));
        if (job == 8 || job == 5) return;
    }
    const auto item = frame.at(item_retire), list = frame.at(0x38);
    invoke(c, entry::begin_item, 0x102e1428, {item, 0xbbb, 0});
    const auto set = [&](std::uint32_t index, std::uint32_t value, std::uint32_t back) { invoke(c, entry::set_word, back, {item, index, value}); };
    const auto name_id = [&](std::uint32_t record_pointer) { return c.u32(c.u32(record_pointer) + 0x20); };
    set(0, c.u32(person), 0x102e1438);
    set(1, name_id(person + 0x28), 0x102e144c);
    set(2, name_id(person + 0x2c), 0x102e1460);
    set(3, name_id(person + 0x30), 0x102e1474);
    set(4, std::uint32_t(sext8(reason)), 0x102e1484);
    // The contract's club, unless the contract is of kind 5 (the top six bits of byte +0x38, signed).
    c.set(29, 0);
    const auto holder = invoke(c, 0x10082f20, 0x102e1498, {cell(c, cells::contract_manager), person});
    c.put32(contract_cell, holder);
    if (holder != 0) {
        const auto top_bits = std::int32_t(std::uint32_t(c.u8(holder + 0x38)) << 24 & 0xfe000000u) >> 26;
        if (sext8(std::uint32_t(top_bits)) != 5) c.set(29, 1);
    }
    const auto club = c.r(29) != 0 ? cell(c, cells::clubs) + c.u32(c.u32(contract_cell) + 4) * row::club_size : 0;
    if (club != 0) set(5, c.u32(club), 0x102e1500);
    else set(5, 0xffffffffu, 0x102e1514);
    set(6, std::uint32_t(sext8(c.u16(person + 0x5c))), 0x102e1528);
    const auto managers = cell(c, cells::managers);
    const auto count = managers != 0 ? invoke(c, 0x103fc710, 0x102e154c, {managers, person, 10, list, 0x1e}) : 0;
    c.set({{29, std::uint32_t(sext8(count))}, {30, list}, {28, 0}});
    for (std::int32_t n = 0; n < sext8(count); ++n) {
        invoke(c, 0x102cc140, 0x102e1574, {c.u32(news_cell), item, c.u32(list + std::uint32_t(n) * 4)});
        c.set({{30, list + std::uint32_t(n + 1) * 4}, {28, std::uint32_t(n + 1)}});
    }
}
}

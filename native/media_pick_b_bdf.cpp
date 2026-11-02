// 0x106891b0: the press item 0xbdf about a club, for one of the stories 0x3c-0x63 (r5). It refuses a
// stale date, clubs without the links it needs and stories the history already holds, then builds
// the numbered fields on its stack, draws a few remainders from the shared stream (the story's
// pick, the club's tier) and posts the item through one of six posters chosen by story and club
// standing. Afterwards it records the story in the history and, for most stories, hands over to
// 0x10672800.
#include "media_pick_b.hpp"
#include "media_eval_c.hpp"
#include "media_pick_a_detail.hpp"
#include "media_store.hpp"
#include <array>

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::pick;
namespace {
namespace callee {
constexpr std::uint32_t today = 0x10715880, club_check = 0x100c7d00,
    club_gate = 0x1014c2e0, manager_effect = 0x106584d0, queue_id = 0x102b7450, queue_known = 0x102aed80,
    match_text = 0x1030d4a0, match_fields = 0x1030dcd0, competition_club = 0x10463a90, competition_slots = 0x1049d330,
    tier_text = 0x106a95c0, post_club = 0x102cc140, post_person = 0x102cc280, post_owner = 0x102cc400,
    post_stand = 0x102cc730, post_pair = 0x102cc8e0;
}
namespace slots {
constexpr int link_table = -0x7570, rival_a = -0x7a1c, rival_b = -0x7a10, queue_owner = -0x7a64, copy_date = -0x7a9c,
    competition_clubs = -0x7424, today = -0x7b80, queue = -0x7bf0;
}
constexpr std::uint32_t item_kind = 0xbdf;

// The compiler's switch over story - 0x3c: which draw a story makes (see Bdf::choose_pick).
enum class Roll { remainder3, remainder4, rated, clears_by_3, clears_by_2, clears_by_3_late, unlisted };
constexpr std::array<Roll, 40> rolls = [] {
    std::array<Roll, 40> table{};
    table.fill(Roll::unlisted);
    for (const int story : {0x3c, 0x50, 0x51, 0x52, 0x5b}) table[std::size_t(story - 0x3c)] = Roll::remainder3;
    for (const int story : {0x3d, 0x5e}) table[std::size_t(story - 0x3c)] = Roll::remainder4;
    table[0x3e - 0x3c] = Roll::rated;
    for (const int story : {0x3f, 0x43, 0x5a, 0x5c, 0x5f, 0x60, 0x61, 0x62, 0x63}) table[std::size_t(story - 0x3c)] = Roll::clears_by_3;
    for (const int story : {0x40, 0x41, 0x44}) table[std::size_t(story - 0x3c)] = Roll::clears_by_2;
    for (const int story : {0x42, 0x5d}) table[std::size_t(story - 0x3c)] = Roll::clears_by_3_late;
    return table;
}();
// The second switch, on the player's byte +0x62 (0-15): the kinds that are told, the others get -1. The caller has
// already sent the kinds above 15 to -1 (the original's range test before its jump table).
constexpr bool told_kind(std::uint32_t kind) { return ((0xb17eu >> kind) & 1) != 0; }

// Stories that have no pick and no follow-up: 0x3e, 0x50, 0x51 and 0x5e.
bool exempt_story(Cpu& c, std::int32_t story) {
    if (compare(c, story, 0x52) >= 0) return compare(c, story, 0x5e) == 0;
    const auto order = compare(c, story, 0x3e);
    if (order == 0) return true;
    if (order < 0) return false;
    return compare(c, story, 0x50) >= 0;
}

struct Bdf {
    Cpu& c;
    const Frame& frame;
    Item item;
    Fields numbered;  // the fields from 0x13 on, numbered by the counter byte at sp+0x38
    Stream rng;
    std::uint32_t self, club, story_register, player, match, margin, value, today_date;
    std::uint32_t pick = 0, tier = 0, id = 0;

    std::int32_t story() const { return byte_word(story_register); }
    Club team() const { return {c, club}; }
    Person who() const { return {c, player}; }
    std::uint32_t manager() const { return team().manager(); }
    std::uint32_t queue() const { return c.u32(c.toc(slots::queue)); }
    std::int32_t rating() const { return team().reputation(); }
    void set_player(std::uint32_t record) { player = record; c.set(18, record); }
    void set_pick(std::uint32_t v) { pick = v; c.set(24, v); }
    std::int32_t draw(std::uint32_t back) const {
        rng.advance(back);
        return std::int32_t(rng.sample());
    }
    // The club's link: the division record behind +0x30.
    std::uint32_t link() const { return team().division(); }
    // Posts the item through one of the posters, about `subject`.
    void send(std::uint32_t poster, std::uint32_t subject, std::uint32_t back) const {
        Program{c}.call(poster, back, {queue(), item.at, subject});
    }
    bool check_club(std::uint32_t back) const { return nonzero(c, byte_word(Program{c}.call(callee::club_check, back, {club}))); }
    // 0x1014c2e0 with its first flag 0 or 1 (the club's owner gate).
    bool club_gate(std::uint32_t flag, std::uint32_t back) const {
        return nonzero(c, byte_word(Program{c}.call(callee::club_gate, back, {club, flag, 1, 0, 0})));
    }

    bool date_ok();
    bool admits();
    void choose_pick();
    void put_tier();
    bool put_story_id();
    void put_manager_flags();
    void put_manager_ids();
    void put_player_ids();
    void put_match();
    void put_competition();
    void put_player_state();
    void put_result();
    void put_tier_text();
    void post();
    void post_by_standing();
    void record_story();
};

// True unless the shared date copy lies after today (year, then day).
bool Bdf::date_ok() {
    c.put32(frame.at(0x40), 0);
    c.put32(frame.at(0x3c), none);
    Program{c}.call(callee::today, 0x10689220, {frame.at(0x4c)});
    const auto source = c.toc(slots::copy_date);
    const auto first = c.u32(source), second = c.u32(source + 4);
    c.put32(frame.at(0x44), first);
    c.put32(frame.at(0x48), second);
    const auto year = std::uint32_t(c.u16(frame.at(0x46)));
    c.set(3, year);
    c.set(4, second);
    std::uint32_t later;
    if (compare_logical(c, year, c.u16(today_date + 2)) > 0) later = 1;
    else if (compare_logical(c, year, c.u16(today_date + 2)) < 0) later = 0;
    else {
        // Same year: the days, compared with the compiler's sign-bit trick.
        const auto day = extended(c.i16(frame.at(0x44))), other = extended(c.i16(today_date));
        const auto difference = day ^ other;
        const auto half = std::uint32_t(std::int32_t(difference) >> 1);
        c.set(4, day);
        c.set(3, half);
        later = ((half - (difference & day)) >> 31) & 1;
    }
    return !nonzero(c, byte_word(later));
}

// The club and manager the item needs, a story not yet in the history, a player of standing.
bool Bdf::admits() {
    if (!nonzero_logical(c, club)) return false;
    if (!nonzero_logical(c, link()) && !check_club(0x106892a8)) return false;
    if (!nonzero_logical(c, team().nation())) return false;
    if (!nonzero_logical(c, team().ground()) && !check_club(0x106892d0)) return false;
    if (!check_club(0x106892e0)) {
        c.set(3, team().ground());
        if (!nonzero_logical(c, team().ground_record().location())) return false;
    }
    if (!nonzero_logical(c, manager())) return false;
    c.set(3, manager());
    if (!nonzero_logical(c, team().manager_record().non_player())) return false;
    if (check_club(0x1068931c) && compare(c, story(), 0x5a) != 0 && compare(c, story(), 0x44) != 0) return false;
    const auto known = Program{c}.module_call(0x1069e700, 0x10689358, {self, player, club, item_kind, std::uint32_t(story()), 0});
    if (nonzero(c, byte_word(known))) return false;
    if (nonzero_logical(c, player) && nonzero_logical(c, who().player())) {
        c.set(3, who().player());
        if (compare(c, who().player_record().reputation(), 0xdac) < 0) return false;
    }
    return true;
}

// Field 1: the pick (0 when the story's draw says no); some stories also drop the player.
// Quirk: a story outside 0x3c-0x63 becomes story 0x63 for the rest of the routine.
void Bdf::choose_pick() {
    const auto index = std::uint32_t(story()) - 0x3c;
    const auto roll = compare_logical(c, index, 0x27) > 0 ? Roll::unlisted : rolls[index];
    switch (roll) {
    case Roll::remainder4:
        set_pick(std::uint32_t(draw(0x106893e8) % 4));
        break;
    case Roll::rated:
        if (compare(c, rating(), 0x1482) > 0) set_pick(extended(std::int8_t(draw(0x10689448) % 2 + 1)));
        else set_pick(std::uint32_t(draw(0x106894a0) % 3));
        break;
    case Roll::remainder3:
        set_pick(std::uint32_t(draw(0x106894fc) % 3));
        break;
    case Roll::clears_by_3:
    case Roll::clears_by_3_late:
        set_pick(std::uint32_t(draw(roll == Roll::clears_by_3 ? 0x10689558 : 0x10689614) % 3));
        if (compare(c, std::int32_t(pick)) != 0) set_player(0);
        break;
    case Roll::clears_by_2:
        set_pick(std::uint32_t(draw(0x106895bc) % 2));
        if (compare(c, std::int32_t(pick)) != 0) set_player(0);
        break;
    default:  // Roll::unlisted: the switch has no other value, so the last case takes the default
        set_pick(0);
        story_register = 0x63;
        c.set(17, 0x63);
        if (compare(c, draw(0x10689680) % 2) != 0) set_player(0);
        break;
    }
    item.put(1, pick, 0x106896c0);
}

// Fields 2-4 and 9: the club's tier (a draw against its rating), the owner link, the club and the story.
void Bdf::put_tier() {
    const auto x = draw(0x106896e0);
    const auto remainder = x % 1500;
    bool open = compare(c, rating(), remainder + 0x1194) > 0;
    if (!open) {
        c.set(4, std::uint32_t(remainder));
        c.set(5, std::uint32_t(x));
        open = check_club(0x10689730);
    }
    if (open) {
        const auto roll = std::int8_t(draw(0x10689758) % 100);
        c.set(3, extended(roll));
        if (compare(c, roll) == 0) tier = 6;
        else if (compare(c, roll, 8) < 0) tier = 3;
        else if (compare(c, roll, 0x23) < 0) tier = 5;
        else if (compare(c, roll, 0x3c) < 0) tier = 0;
        else if (check_club(0x106897e4)) tier = 1;
        else tier = compare(c, draw(0x1068980c) % 3) == 0 ? 2 : 1;
    } else {
        const auto third = draw(0x10689874) % 3;
        tier = compare(c, third, 2) < 0 && compare(c, third) >= 0 ? 2 : 4;
    }
    c.set(25, tier);
    item.put(2, tier, 0x106898d8);
    item.put(3, team().nation_record().id(), 0x106898ec);
    if (nonzero_logical(c, club)) item.put(4, team().id(), 0x10689904);
    else item.put(4, none, 0x10689918);
    item.put(9, std::uint32_t(story()), 0x10689928);
}

// Field 0xa: the manager's effect (none for exempt stories or a closed gate); fields 5 and 6: the manager's moods.
void Bdf::put_manager_flags() {
    if (exempt_story(c, story())) set_pick(0);
    else if (club_gate(0, 0x10689978)) set_pick(0);
    else set_pick(Program{c}.module_call(callee::manager_effect, 0x10689994, {self, manager(), player, std::uint32_t(story())}));
    item.put(0xa, pick, 0x106899b0);
    const auto boss = team().manager_record();
    const auto ambition = [&] { return std::int32_t(boss.ambition().get()); };
    const auto trait = [&] { return std::int32_t(boss.trait_69().get()); };
    const auto standing = [&] { return std::int32_t(boss.staff_record().reputation().get()); };
    std::uint32_t proud = 0, restless = 0;
    if (compare(c, ambition(), 0xf) > 0 && compare(c, trait(), 6) < 0 && compare(c, standing(), 0x1676) > 0) { proud = 1; c.set(23, 1); }
    item.put(5, proud, 0x10689a00);
    if (compare(c, ambition(), 0xf) < 0 && compare(c, trait(), 0xe) > 0 && compare(c, standing(), 0x1676) > 0) { restless = 1; c.set(22, 1); }
    item.put(6, restless, 0x10689a50);
}

// Field 7: the id the story is about (the queue's for stories 0x5a/0x44, the club's own for 0x5c, else the link's).
bool Bdf::put_story_id() {
    if (compare(c, story(), 0x5a) == 0 || compare(c, story(), 0x44) == 0) {
        const auto record = Program{c}.call(callee::queue_id, 0x10689a74, {c.u32(c.r(26)), team().nation()});
        if (!nonzero(c, byte_word(Program{c}.call(callee::queue_known, 0x10689a80, {c.u32(c.r(26)), record})))) return false;
        id = c.u32(Program{c}.call(callee::queue_id, 0x10689a94, {c.u32(c.r(26)), team().nation()}));
        c.set(22, id);
        item.put(7, id, 0x10689aac);
    } else if (compare(c, story(), 0x5c) == 0) {
        if (compare(c, c.i32(club + 0x14), -1) == 0) return false;
        id = c.u32(club + 0x14);
        c.set(22, id);
        item.put(7, id, 0x10689ae0);
    } else {
        id = c.u32(link());
        c.set(22, id);
        item.put(7, id, 0x10689afc);
    }
    return true;
}

// Fields 0xb-0xe and 0xf-0x12: the manager's and the player's own id and three linked ids (-1 without a player).
void Bdf::put_manager_ids() {
    put_person(c, item, 0xb, manager(), {0x10689b10, 0x10689b28, 0x10689b40, 0x10689b58});
}
void Bdf::put_player_ids() {
    if (nonzero_logical(c, player)) put_person(c, item, 0xf, player, {0x10689b70, 0x10689b84, 0x10689b98, 0x10689bac});
    else put_no_person(item, 0xf, {0x10689bc0, 0x10689bd0, 0x10689be0, 0x10689bf0});
}

// Fields from 0x13 on, numbered by the counter byte: the match (what the match text routines add after it),
// the competition, the player's state and the result.
void Bdf::put_match() {
    c.put8(frame.at(0x38), 0x13);
    if (nonzero_logical(c, match)) {
        numbered.put(1, 0x10689c18);
        Program{c}.call(callee::match_text, 0x10689c28, {match, club, frame.at(0x4c)});
        Program{c}.call(callee::match_fields, 0x10689c38, {frame.at(0x4c), item.at, frame.at(0x38)});
        numbered.put(std::uint32_t(byte_word(margin)), 0x10689c50);
    } else {
        numbered.put(0, 0x10689c6c);
        numbered.put(std::uint32_t(byte_word(margin)), 0x10689c84);
    }
}

void Bdf::put_competition() {
    const auto competitions = c.u32(c.toc(slot::competitions));
    const auto home = Program{c}.call(callee::competition_club, 0x10689c9c, {competitions + id * 0x6c, 0});
    numbered.put(home == club ? 1 : 0, 0x10689cbc);
    Program{c}.call(callee::competition_slots, 0x10689cd4, {c.u32(c.toc(slots::competition_clubs)), id, frame.at(0x40), frame.at(0x3c)});
    numbered.put(c.u32(frame.at(0x40)), 0x10689cec);
    numbered.put(c.u32(frame.at(0x3c)), 0x10689d04);
    numbered.put(value, 0x10689d1c);
}

// The player's three fields: id, a kind (-1 when untold) and a flag; or the other pair of links.
void Bdf::put_player_state() {
    const auto flagged = [&] {
        if (!nonzero_logical(c, player)) return 0u;
        return nonzero_logical(c, who().ability()) ? 1u : 0u;
    };
    if (nonzero_logical(c, player) && nonzero_logical(c, who().club())) {
        numbered.put_loaded([&] { return c.u32(who().club()); }, 0x10689d4c);
        const auto kind = std::int32_t(who().job().get());
        if (compare_logical(c, std::uint32_t(kind), 0xf) > 0 || !told_kind(std::uint32_t(kind))) {
            numbered.put(none, 0x10689dd8);
            numbered.put(flagged(), 0x10689e08);
        } else {
            numbered.put(std::uint32_t(kind), 0x10689d8c);
            numbered.put(flagged(), 0x10689dbc);
        }
    } else if (nonzero_logical(c, player) && nonzero_logical(c, who().national_team()) && compare(c, who().national_job(), 0xb) != 0) {
        numbered.put_loaded([&] { return c.u32(who().national_team()); }, 0x10689e48);
        numbered.put(std::uint32_t(byte_word(who().national_job())), 0x10689e64);
        numbered.put(flagged(), 0x10689e94);
    } else {
        numbered.put(none, 0x10689eb0);
        numbered.put(none, 0x10689ec8);
        numbered.put(flagged(), 0x10689ef8);
    }
}

// Who is ahead, from the club's side: 0 unplayed, 1 the club's side, 2 the other (nothing for a level score).
void Bdf::put_result() {
    if (!nonzero_logical(c, match)) return;
    pick::put_result(c, numbered, Fixture{c, match}, club, {0x10689f28, 0x10689f68, 0x10689f84, 0x10689fc4, 0x10689fe0, 0});
}

// The tier's text id (0x106a95c0 over the link table), as the last numbered field.
void Bdf::put_tier_text() {
    const auto nation = team().nation();
    std::uint32_t text;
    if (nonzero_logical(c, nation))
        text = Program{c}.module_call(callee::tier_text, 0x1068a004, {c.u32(c.r(13)), club, std::uint32_t(byte_word(tier)), c.u32(nation + 0x68)});
    else text = Program{c}.module_call(callee::tier_text, 0x1068a01c, {c.u32(c.r(13)), club, std::uint32_t(byte_word(tier)), none});
    numbered.put(text, 0x1068a034);
}

// Posts the item: usually with the club, through the poster the story and the club's standing choose.
void Bdf::post() {
    if (!club_gate(1, 0x1068a04c) && compare(c, draw(0x1068a074) % 2) != 0) return send(callee::post_club, club, 0x1068a0b0);
    if (compare(c, story(), 0x5a) == 0 || compare(c, story(), 0x44) == 0) {
        Program{c}.call(callee::post_pair, 0x1068a0d8, {queue(), item.at});
        return;
    }
    const auto owner_stand = [&] { return c.u32(team().nation() + 0x74); };
    if (compare(c, story(), 0x5c) == 0 ||
        (nonzero_logical(c, link()) && compare(c, rating(), 0x2328) >= 0 && compare(c, Division{c, link()}.level(), 0xf) > 0))
        return send(callee::post_stand, owner_stand(), 0x1068a124);
    post_by_standing();
}

// The rest of the dispatch, when the club is not in the top bracket.
void Bdf::post_by_standing() {
    if (nonzero_logical(c, link()) && compare(c, Division{c, link()}.level(), 0xf) > 0) {
        // Quirk: r13 (the link table) is reused for the margin's bonus from here on.
        const auto margin_byte = byte_word(margin);
        c.set(13, compare(c, margin_byte, 0x14) > 0 ? std::uint32_t(margin_byte) * 0x1e + 0x3e8 : 0);
        const auto x = draw(0x1068a184);
        const auto threshold = (0x1f40u - c.r(13)) + std::uint32_t(x % 3000);
        const auto lively = [&](std::int32_t kind) { return compare(c, story(), kind) == 0 && compare(c, margin_byte, 0xf) > 0; };
        if (compare(c, rating(), std::int32_t(threshold)) > 0 &&
            (lively(0x50) || lively(0x51) || compare(c, story(), 0x41) == 0 ||
             compare_logical(c, std::uint8_t(story_register - 0x61), 2) <= 0 || compare(c, story(), 0x5d) == 0))
            return send(callee::post_stand, c.u32(team().nation() + 0x74), 0x1068a23c);
    }
    if (compare(c, story(), 0x3d) == 0) {
        if (nonzero_logical(c, link()) && compare(c, draw(0x1068a278) % 3) == 0) return send(callee::post_person, link(), 0x1068a2c0);
        return send(callee::post_club, club, 0x1068a2d4);
    }
    if (nonzero_logical(c, link()) && compare(c, rating(), 0x1388) >= 0) {
        send(callee::post_owner, team().nation(), 0x1068a300);
        const auto owner_rival = team().nation_record().id().get();
        const auto persons = c.u32(c.toc(slot::persons));
        if (compare(c, std::int32_t(owner_rival), std::int32_t(c.u32(c.toc(slots::rival_b)))) == 0 && compare(c, rating(), 0x1770) >= 0)
            return send(callee::post_owner, persons + c.u32(c.toc(slots::rival_a)) * person_size, 0x1068a344);
        if (compare(c, std::int32_t(owner_rival), std::int32_t(c.u32(c.toc(slots::rival_a)))) == 0 && compare(c, rating(), 0x1964) >= 0)
            return send(callee::post_owner, persons + c.u32(c.toc(slots::rival_b)) * person_size, 0x1068a38c);
        return send(callee::post_club, club, 0x1068a3a0);
    }
    if (nonzero_logical(c, link())) return send(callee::post_person, link(), 0x1068a3c0);
    send(callee::post_club, club, 0x1068a3d4);
}

// The history's record of the story, and the follow-up unless the story is exempt.
void Bdf::record_story() {
    Program{c}.module_call(0x1069e190, 0x1068a3f0, {self, player, club, item_kind, std::uint32_t(story()), std::uint32_t(byte_word(pick))});
    if (exempt_story(c, story()) || club_gate(0, 0x1068a434)) return;
    typed_call(c, 0x1068a454, evaluate_club_story, self, club, std::uint32_t(story()), std::uint32_t(byte_word(pick)), player);
}
}

void post_item_bdf(Cpu& c, std::uint32_t self, std::uint32_t club, std::uint32_t story, std::uint32_t player, std::uint32_t match,
                   std::uint32_t margin, std::uint32_t value) {
    Frame frame(c, 0x106891b0, 0x1b0, {.first_gpr = 13});
    const auto rng = Stream::world(c);
    const auto today = c.toc(slots::today);
    keep(c, {{13, c.toc(slots::link_table)}, {14, c.toc(slots::rival_a)}, {15, self}, {16, club}, {17, story}, {18, player}, {19, match},
             {20, margin}, {21, value}, {22, 0}, {23, 0}, {24, today}, {26, c.toc(slots::queue_owner)}, {27, c.toc(slots::queue)},
             {28, rng.seed}, {29, rng.start}, {30, rng.end}, {31, rng.cursor}});
    c.set(10, 0);
    const Item item{c, frame.at(0x70)};
    Bdf bdf{c, frame, item, Fields{item, Counter{c, frame.at(0x38)}}, rng, self, club, story, player, match, margin, value, today};
    if (!bdf.date_ok() || !bdf.admits()) return;
    bdf.item.begin(item_kind, 0x10689394);
    bdf.item.put(0, none, 0x106893a4);
    bdf.choose_pick();
    bdf.put_tier();
    bdf.put_manager_flags();
    if (!bdf.put_story_id()) return;
    bdf.put_manager_ids();
    bdf.put_player_ids();
    bdf.put_match();
    bdf.put_competition();
    bdf.put_player_state();
    bdf.put_result();
    bdf.put_tier_text();
    bdf.post();
    bdf.record_story();
}
}

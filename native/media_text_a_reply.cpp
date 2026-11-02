// The manager's reply to the column: told to the press (the answer he gave), admitted, or said by
// somebody else, each picked by the kind of story and the player's traits.
#include "media_text_a_detail.hpp"

namespace cm::aot::media::detail::column {

// Kinds 0 and 0x12 with a tone past 7 are worded by the player's traits; every other story, by its kind.
bool Story::reply_goes_by_traits() {
    if (cmp(kind, 0x0) == 0) return cmp(tone, 0x7) > 0;
    return cmp(kind, 0x12) == 0 && cmp(tone, 0x7) > 0;
}

void Story::user_told() {
    if (reply_goes_by_traits()) user_told_by_traits();
    else user_told_by_kind();
}

void Story::user_admitted() {
    if (reply_goes_by_traits()) user_admitted_by_traits();
    else user_admitted_by_kind();
}

void Story::other_told() {
    if (reply_goes_by_traits()) other_told_by_traits();
    else other_told_by_kind();
}

void Story::other_said() {
    if (reply_goes_by_traits()) other_said_by_traits();
    else other_said_by_kind();
}

// The user told the press: by the kind of story (0x106526c0).
void Story::user_told_by_kind() {
    if (cmp(kind, 0x23) == 0) return say(R(0x977e, 0x106526d8));
    if (cmp(kind, 0x13) == 0) {
        if (cmp(byte_31d1(), 0x3) <= 0) return say(R(0x989b, 0x10652720, player_short));
        return say(R(0x97d2, 0x10652708, player_short));
    }
    if (cmp(kind, 0xa) == 0) return say(R(0x9937, 0x10652740, player_short));
    if (cmp(kind, 0xc) == 0 || cmp(kind, 0xd) == 0) return say(R(0x999b, 0x10652764));
    if (cmp(kind, 0xe) == 0) return say(R(0x9a03, 0x10652780));
    if (cmp(kind, 0x11) == 0) return say(R(0x9a43, 0x106527a4, plain, player_short));
    if (cmp(kind, 0x0) != 0) {
        if (cmp(kind, 0x21) == 0 || cmp(kind, 0x22) == 0) return say(R(0x9b0e, 0x10652920, player_short));
        if (cmp(kind, 0x8) == 0) return say(R(0x9b74, 0x1065293c));
        if (cmp(kind, 0x1) == 0) return say(R(0x9bcb, 0x1065295c, player_short));
        say_by_kind_of_eight(std::to_array<Line>({
                R(0x9c21, 0x10652984, player_short),
                R(0x9d28, 0x106529d8),
                R(0x9e58, 0x10652a78, player_short),
                R(0x9f58, 0x10652ab0),
                R(0x9edb, 0x10652a9c, plain, player_short),
                R(0x9dc3, 0x10652a50, club_article, club_article, player_short, club_name, clubs_base),
                R(0x9d64, 0x106529f8, player_short),
                R(0x9c85, 0x106529b4, age_flag_arg, plain, date_text, player_short)
        }));
        return;
    }
    if (cmp(tone, 0x3) == 0) return say(R(0x9aae, 0x106527c8));
    say_by_answer_of_ten(std::to_array<Line>({
            R(0x9657, 0x106528c4, player_short),
            R(0x9519, 0x106528f8, plain, player_short),
            R(0x96ba, 0x106528dc, player_short),
            R(0x9657, 0x106528ac, player_short),
            R(0x95ec, 0x10652894, player_short),
            R(0x9593, 0x10652874, player_short),
            R(0x9519, 0x1065285c, plain, player_short),
            R(0x94db, 0x1065282c),
            R(0x9498, 0x10652808),
            R(0x9519, 0x106528f8, plain, player_short)
    }));
    return;
}

// The user told the press, a kind-0 or 0x12 story with a high tone: by the player's traits (0x10652344).
void Story::user_told_by_traits() {
    if (has_column_trait()) {
        say_by_answer_of_ten(std::to_array<Line>({
                R(0x9657, 0x10652518, player_short),
                R(0x9519, 0x1065254c, plain, player_short),
                R(0x96ba, 0x10652530, player_short),
                R(0x9657, 0x10652500, player_short),
                R(0x95ec, 0x106524e8, player_short),
                R(0x9593, 0x106524c8, player_short),
                R(0x9519, 0x106524b0, plain, player_short),
                R(0x94db, 0x10652480),
                R(0x9498, 0x1065245c),
                R(0x9519, 0x1065254c, plain, player_short)
        }));
        return;
    }
    if (has_secondary_trait()) return say(R(0x971c, 0x1065258c));
    say_by_answer_of_ten(std::to_array<Line>({
            R(0x9657, 0x10652688, player_short),
            R(0x9519, 0x106526bc, plain, player_short),
            R(0x96ba, 0x106526a0, player_short),
            R(0x9657, 0x10652670, player_short),
            R(0x95ec, 0x10652658, player_short),
            R(0x9593, 0x10652638, player_short),
            R(0x9519, 0x10652620, plain, player_short),
            R(0x94db, 0x106525f0),
            R(0x9498, 0x106525cc),
            R(0x9519, 0x106526bc, plain, player_short)
    }));
    return;
}

// The user admitted it: by the kind of story (0x10652c18).
void Story::user_admitted_by_kind() {
    if (cmp(kind, 0x23) == 0) return say(R(0xa012, 0x10652c30));
    if (cmp(kind, 0x11) == 0) return say(R(0xa084, 0x10652c54, plain, player_short));
    if (cmp(kind, 0x0) == 0) return say(R(0x9f59, 0x10652c78, plain, player_short));
    if (cmp(kind, 0xe) == 0) return say(R(0xa0f9, 0x10652c94));
    if (cmp(kind, 0xc) == 0 || cmp(kind, 0xd) == 0) return say(R(0xa146, 0x10652cc0, plain, player_short));
    if (cmp(kind, 0x13) == 0) return say(R(0xa1b5, 0x10652ce0, player_short));
    if (cmp(kind, 0xa) == 0) return say(R(0xa233, 0x10652d00, player_short));
    if (cmp(kind, 0x21) == 0 || cmp(kind, 0x22) == 0) return say(R(0xa233, 0x10652d28, player_short));
    if (cmp(kind, 0x8) == 0) return say(R(0xa2e3, 0x10652d44));
    if (cmp(kind, 0x1) == 0) return say(R(0xa358, 0x10652d64, player_short));
    if (cmp(kind, 0x2) == 0 || cmp(kind, 0x9) == 0) return say(R(0xa3c2, 0x10652d90, plain, player_short));
    if (cmp(kind, 0x25) == 0) return say(R(0xa42e, 0x10652dac));
    if (cmp(kind, 0x1e) == 0 || cmp(kind, 0x20) == 0) return say(R(0xa46e, 0x10652dd8, plain, player_short));
    if (cmp(kind, 0x3) == 0) return say(R(0xa4e0, 0x10652dfc, plain, player_short));
    if (cmp(kind, 0x4) == 0) {
        if (cmp(word_31f0(), 0x0) == 0) return say(R(0xa5e5, 0x10652eb0, club_article, club_article, player_short,
                club_name, clubs_base));
        return say(R(0xa53b, 0x10652e60, club_article, club_article, player_short, club_name, clubs_base));
    }
    if (cmp(kind, 0x5) == 0 || cmp(kind, 0x7) == 0) return say(R(0xa690, 0x10652ed8, player_short));
    if (cmp(kind, 0x6) != 0) return say(R(0x9f58, 0x10652f10));
    return say(R(0xa72b, 0x10652efc, plain, player_short));
}

// The user admitted it, a high-tone kind 0 or 0x12 story: by the traits (0x10652ac4).
void Story::user_admitted_by_traits() {
    if (has_column_trait()) return say(R(0x9f59, 0x10652bb8, plain, player_short));
    if (has_secondary_trait()) return say(R(0x9fb8, 0x10652bf8));
    return say(R(0x9f59, 0x10652c14, plain, player_short));
}

// Another manager told the press: by the kind of story (0x10653384).
void Story::other_told_by_kind() {
    if (cmp(kind, 0xe) == 0) return say(R(0xacc1, 0x106533a0, manager_name));
    if (cmp(kind, 0x11) == 0) return say(R(0xad28, 0x106533c8, plain, manager_name, player_short));
    if (cmp(kind, 0x0) == 0) {
        if (cmp(tone, 0x3) == 0) return say(R(0xadb4, 0x106533f0, manager_name));
        say_by_answer_of_eleven(std::to_array<Line>({
                R(0xab2f, 0x10653510, manager_name, player_short),
                R(0xaa9e, 0x106534f4, manager_name, player_short),
                R(0xaa1f, 0x106534d0, manager_name, player_short),
                R(0xa97f, 0x106534b4, plain, manager_name, player_short),
                R(0xa8fc, 0x10653480, manager_name, player_short),
                R(0xabb8, 0x10653548, manager_name, player_short),
                R(0xa97f, 0x10653568, plain, manager_name, player_short),
                R(0xab2f, 0x1065352c, manager_name, player_short),
                R(0xa7f1, 0x10653438, manager_name, player_short),
                R(0xa874, 0x10653454, manager_name, player_short),
                R(0xa97f, 0x10653568, plain, manager_name, player_short)
        }));
        return;
    }
    if (cmp(kind, 0xc) == 0 || cmp(kind, 0xd) == 0) return say(R(0xae40, 0x10653594, manager_name, player_short));
    if (cmp(kind, 0x8) == 0) return say(R(0xaee5, 0x106535b4, manager_name));
    if (cmp(kind, 0x1) == 0) return say(R(0xaf46, 0x106535d8, manager_name, player_short));
    if (cmp(kind, 0x13) == 0) {
        if (cmp(byte_31d1(), 0x3) <= 0) return say(R(0xb0a7, 0x10653620, player_short));
        return say(R(0xafc6, 0x10653608, player_short));
    }
    if (cmp(kind, 0x21) == 0 || cmp(kind, 0xa) == 0 || cmp(kind, 0x22) == 0) return say(R(0xb15b, 0x10653654,
            manager_short, player_short));
    say_by_kind_of_eight(std::to_array<Line>({
            R(0xb1dd, 0x10653680, manager_name, player_short),
            R(0xb30b, 0x106536d8, manager_name),
            R(0xb4ab, 0x106537bc, club_article, club_article, club_name, manager_name, player_short),
            R(0x9f58, 0x106537f8).plain(),
            R(0xb571, 0x106537e4, plain, manager_name, player_short),
            R(0xb3ef, 0x10653758, club_article, club_article, manager_name, player_short, club_name),
            R(0xb36d, 0x106536fc, manager_name, player_short),
            R(0xb267, 0x106536b0, age_flag_arg, plain, manager_name, date_text)
    }));
    return;
}

// Another manager told the press, a high-tone kind 0 or 0x12 story: by the traits (0x10652f74).
void Story::other_told_by_traits() {
    if (has_column_trait()) {
        say_by_answer_of_eleven(std::to_array<Line>({
                R(0xab2f, 0x1065316c, manager_name, player_short),
                R(0xaa9e, 0x10653150, manager_name, player_short),
                R(0xaa1f, 0x1065312c, manager_name, player_short),
                R(0xa97f, 0x10653110, plain, manager_name, player_short),
                R(0xa8fc, 0x106530dc, manager_name, player_short),
                R(0xabb8, 0x106531a4, manager_name, player_short),
                R(0xa97f, 0x106531c4, plain, manager_name, player_short),
                R(0xab2f, 0x10653188, manager_name, player_short),
                R(0xa7f1, 0x10653094, manager_name, player_short),
                R(0xa874, 0x106530b0, manager_name, player_short),
                R(0xa97f, 0x106531c4, plain, manager_name, player_short)
        }));
        return;
    }
    if (has_secondary_trait()) return say(R(0xac40, 0x10653208, manager_name));
    say_by_answer_of_eleven(std::to_array<Line>({
            R(0xab2f, 0x10653328, manager_name, player_short),
            R(0xaa9e, 0x1065330c, manager_name, player_short),
            R(0xaa1f, 0x106532e8, manager_name, player_short),
            R(0xa97f, 0x106532cc, plain, manager_name, player_short),
            R(0xa8fc, 0x10653298, manager_name, player_short),
            R(0xabb8, 0x10653360, manager_name, player_short),
            R(0xa97f, 0x10653380, plain, manager_name, player_short),
            R(0xab2f, 0x10653344, manager_name, player_short),
            R(0xa7f1, 0x10653250, manager_name, player_short),
            R(0xa874, 0x1065326c, manager_name, player_short),
            R(0xa97f, 0x10653380, plain, manager_name, player_short)
    }));
    return;
}

// Somebody else said it: by the kind of story (0x1065395c).
void Story::other_said_by_kind() {
    if (cmp(kind, 0xe) == 0) return say(R(0xb6e9, 0x10653978, manager_name));
    if (cmp(kind, 0x11) == 0) return say(R(0xb754, 0x106539a0, plain, manager_name, player_short));
    if (cmp(kind, 0x0) == 0) return say(R(0xb62c, 0x106539c0, manager_name));
    if (cmp(kind, 0x2) == 0 || cmp(kind, 0x9) == 0) return say(R(0xb62c, 0x106539e8, manager_name));
    if (cmp(kind, 0xc) == 0 || cmp(kind, 0xd) == 0) return say(R(0xb7ea, 0x10653a18, plain, manager_name,
            player_short));
    if (cmp(kind, 0x8) == 0) return say(R(0xb876, 0x10653a3c, manager_name, player_name));
    if (cmp(kind, 0x13) == 0) {
        if (cmp(byte_31d0(), 0) == 0) return say(R(0xb9b4, 0x10653a88, manager_short, player_short));
        return say(R(0xb91e, 0x10653a6c, manager_short, player_short));
    }
    if (cmp(kind, 0x21) == 0 || cmp(kind, 0xa) == 0 || cmp(kind, 0x22) == 0) return say(R(0xba49, 0x10653abc,
            manager_short, player_short));
    if (cmp(kind, 0x1) == 0) return say(R(0xbb30, 0x10653b18, club_article, club_article, club_name, manager_name,
            player_short));
    if (cmp(kind, 0x25) == 0) return say(R(0xbbe9, 0x10653b38, manager_name));
    if (cmp(kind, 0x1e) == 0 || cmp(kind, 0x20) == 0) return say(R(0xbcd9, 0x10653b60, manager_name));
    if (cmp(kind, 0x3) == 0) return say(R(0xbd45, 0x10653b84, manager_name, player_short));
    if (cmp(kind, 0x4) == 0) return say(R(0xbdcd, 0x10653be0, club_article, club_article, manager_name, player_short,
            club_name));
    if (cmp(kind, 0x6) == 0) return say(R(0xbe95, 0x10653c08, plain, manager_name, player_short));
    if (cmp(kind, 0x5) == 0 || cmp(kind, 0x7) == 0) return say(R(0xbf43, 0x10653c6c, club_article, club_article,
            club_name, manager_name, player_short));
    return say(R(0x9f58, 0x10653c80).plain());
}

// Somebody else said it, a high-tone kind 0 or 0x12 story: by the traits (0x1065380c).
void Story::other_said_by_traits() {
    if (has_column_trait()) return say(R(0xb62c, 0x106538fc, manager_name));
    if (has_secondary_trait()) return say(R(0xb67b, 0x10653940, manager_name));
    return say(R(0xb62c, 0x10653958, manager_name));
}

// One of eleven lines by the answer given; answer 4 goes on to the level and tone.
void Story::say_by_answer_of_eleven(std::span<const Line> lines) {
    if (cmp(answer, 0x4) == 0) {
        say_by_level_and_tone(lines.subspan(0, 5));
        return;
    }
    if (cmp(answer, 0x4) >= 0) {
        if (cmp(answer, 0x6) == 0) return say(lines[5]);
        if (cmp(answer, 0x6) >= 0) return say(lines[6]);
        return say(lines[7]);
    }
    if (cmp(answer, 0x1) == 0) return say(lines[8]);
    if (cmp(answer, 0x1) >= 0) return say(lines[9]);
    return say(lines[10]);
}

// One of ten lines by the answer given; answer 4 goes on to the level and tone.
void Story::say_by_answer_of_ten(std::span<const Line> lines) {
    if (cmp(answer, 0x5) == 0) return say(lines[0]);
    if (cmp(answer, 0x5) >= 0) {
        if (cmp(answer, 0x7) >= 0) return say(lines[1]);
        return say(lines[2]);
    }
    if (cmp(answer, 0x4) < 0) {
        if (cmp(answer, 0x1) >= 0) return say(lines[8]);
        return say(lines[9]);
    }
    say_by_level_and_tone(lines.subspan(3, 5));
    return;
}

// One of eight lines by the kind of story.
void Story::say_by_kind_of_eight(std::span<const Line> lines) {
    if (cmp(kind, 0x2) == 0 || cmp(kind, 0x9) == 0) return say(lines[0]);
    if (cmp(kind, 0x25) == 0) return say(lines[7]);
    if (cmp(kind, 0x1e) == 0 || cmp(kind, 0x20) == 0) return say(lines[1]);
    if (cmp(kind, 0x3) == 0) return say(lines[6]);
    if (cmp(kind, 0x4) == 0) return say(lines[5]);
    if (cmp(kind, 0x5) == 0 || cmp(kind, 0x7) == 0) return say(lines[2]);
    if (cmp(kind, 0x6) != 0) return say(lines[3]);
    return say(lines[4]);
}

// One of five lines by the story's level, the traits (the 0x800000 one) and the tone's parity.
void Story::say_by_level_and_tone(std::span<const Line> lines) {
    if (cmp(level, 0x1a) <= 0) {
        if (cmp(level, 0x17) <= 0) return say(lines[0]);
        return say(lines[1]);
    }
    if (has_any_trait(0x800000)) return say(lines[4]);
    if (cmp((tone % 2), 0) == 0) return say(lines[2]);
    return say(lines[3]);
}
}

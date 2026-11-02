// The short form: a headline over the same story.
#include "media_text_a_detail.hpp"

namespace cm::aot::media::detail::column {

void Story::headline() {
    if (cmp(kind, 0x23) == 0) return say(L(0xc02b, 0x10654414, player_name));
    if (cmp(kind, 0xe) == 0) return say(L(0xc05b, 0x10654438, plain, player_name));
    if (cmp(kind, 0x4) == 0) return say(L(0xc09b, 0x10654458, player_name));
    if (cmp(kind, 0xd) == 0) return say(L(0xc0cb, 0x10654478, player_name));
    if (cmp(kind, 0x11) == 0) {
        if (cmp(pick_three(), 0x1) == 0) return say(L(0xc14d, 0x106544f4, player_name));
        if (cmp(pick_three(), 0x1) >= 0 || cmp(pick_three(), 0x0) < 0) return say(L(0xc186, 0x10654510, plain,
                player_name));
        return say(L(0xc108, 0x106544dc, plain, player_name));
    }
    if (cmp(kind, 0x0) == 0) {
        if (cmp((figure % 3), 0) == 0) return say(L(0xc1d0, 0x10654550, player_name));
        if (cmp((figure % 3), 0x1) != 0) return say(L(0xc234, 0x106545a4, player_name));
        return say(L(0xc203, 0x1065458c, player_name));
    }
    if (cmp(kind, 0x9) == 0) {
        if (cmp(figure, 0x0) == 0 || cmp(figure, 0x1) == 0) return say(L(0xc267, 0x106545dc, plain, second_name,
                player_name));
        return say(L(0xc2cb, 0x106545f8, plain, player_name));
    }
    if (cmp(kind, 0x13) == 0 || cmp(kind, 0x21) == 0 || cmp(kind, 0xa) == 0) return say(L(0xc30e, 0x10654638,
            club_article, club_article, club_name));
    if (cmp(kind, 0x22) == 0) return say(L(0xc345, 0x10654658, player_name));
    if (cmp(kind, 0x8) == 0) return say(L(0xc37c, 0x1065467c, plain, player_name));
    if (cmp(kind, 0x3) == 0) return say(L(0xc3bd, 0x1065469c, player_name));
    if (cmp(kind, 0x5) == 0) return say(L(0xc3f1, 0x106546bc, player_name));
    if (cmp(kind, 0x7) == 0) return say(L(0xc428, 0x106546dc, player_name));
    if (cmp(kind, 0x6) == 0) return say(L(0xc45a, 0x10654738, plain, club_article, club_article, player_name,
            club_name));
    if (cmp(kind, 0xc) == 0) return say(L(0xc4be, 0x1065475c, plain, player_name));
    if (cmp(kind, 0x25) != 0) return say(L(0xc53a, 0x10654798, plain, player_name));
    return say(L(0xc4fd, 0x1065477c, player_name));
}
}

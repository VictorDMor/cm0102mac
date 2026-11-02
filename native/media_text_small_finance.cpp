// The newspaper's reports on a club's finances (0x10688fd0): the long form names the club in
// the text, the short form is a headline. The wording follows how bad the figure is.
#include "media_text_small.hpp"
#include "media_text_small_detail.hpp"

namespace cm::aot::media {
using namespace detail;
using namespace detail::small;

namespace {
constexpr std::int32_t severe_figure = 0x1a5e, bad_figure = 0xcb2;
}

// The item holds the club (value 0) and a figure (value 1); value 2 is read and unused.
void club_finance_story(Cpu& c, std::uint32_t, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x10688fd0, 0x60, {.first_gpr = 25});
    keep(c, {{29, c.toc(slot::text)}, {25, item}, {30, c.toc(slot::clubs)}, {26, output}, {31, c.toc(slot::pool)}});
    const auto text = c.r(29);
    const auto say = [&](std::uint32_t club, unsigned phrase, std::uint32_t back) {
        format(c, text, phrase, back, {sign8(c.u8(club + club_article)), sign8(c.u8(club + club_article)), club + club_name});
    };
    const auto finish = [&](std::uint32_t club, std::uint32_t copied, std::uint32_t linked) {
        copy_text(c, output, text, copied);
        link_text(c, output, club + club_name, 0x7d5, club, linked);
    };
    if (compare(c, std::int8_t(full)) != 0) {
        Kept<28> index{c, item_value(c, item, 0, 0x10689008)};
        Kept<27> figure{c, item_value(c, item, 1, 0x1068901c)};
        item_value(c, item, 2, 0x10689030);
        index = club_record(c, index);
        const std::uint32_t club = index;
        if (compare(c, figure, severe_figure) > 0) say(club, 0x2b412, 0x10689068);
        else if (compare(c, figure, bad_figure) > 0) say(club, 0x2b4da, 0x10689098);
        else say(club, 0x2b591, 0x106890c0);
        finish(club, 0x106890cc, 0x106890e0);
    } else {
        Kept<27> index{c, item_value(c, item, 0, 0x106890f0)};
        Kept<28> figure{c, item_value(c, item, 1, 0x10689104)};
        item_value(c, item, 2, 0x10689118);
        index = club_record(c, index);
        const std::uint32_t club = index;
        if (compare(c, figure, severe_figure) > 0) say(club, 0x2b63f, 0x10689150);
        else say(club, 0x2b671, 0x10689178);
        finish(club, 0x10689184, 0x10689198);
    }
}
}

// The second paragraph of a long story, on the club's prospects.
#include "media_text_b_detail.hpp"
#include <span>

namespace cm::aot::media::detail::press {
using namespace guest;
using namespace detail;

namespace {
// The paragraph each flag writes, by the remainder of the count: the three phrases and the append's return address.
constexpr FlagParagraph flagged[]{
    {1, {L(0x1e74e, 0x10672348), L(0x1e7ad, 0x1067235c), L(0x1e823, 0x10672370)}, 0x1067237c},
    {2, {L(0x1e898, 0x106723cc), L(0x1e8ef, 0x106723e0), L(0x1e823, 0x106723f4)}, 0x10672400},
    {4, {L(0x1e95b, 0x10672450), L(0x1e9b2, 0x10672464), L(0x1e823, 0x10672478)}, 0x10672484},
    {8, {L(0x1ea1b, 0x106724d4), L(0x1ea80, 0x106724e8), L(0x1e823, 0x106724fc)}, 0x10672508},
    {0x10, {L(0x1eae8, 0x10672558), L(0x1eb41, 0x1067256c), L(0x1e823, 0x10672580)}, 0x1067258c},
};
}

void Press::paragraph() {
    out.set(text, 0x10672148);
    if (cmp(about_board, 0) == 0 && cmp(code, 1) != 0) {
        if (cmp(count, 0x251c) <= 0 || cmp(kind, 2) != 0) {
            if (cmp(count, 0x109a) >= 0 || (cmp(kind, 5) != 0 && cmp(kind, 4) != 0)) {
                return;
            }
            static constexpr Line options[]{L(0x1e5a9, 0x10672264, 0x64, manager_short), L(0x1e631, 0x10672280, 0x64, manager_short), L(0x1e6b3, 0x1067229c, 0x64, manager_short)};
            say(options[three_ways(count % 3)]);
            out.append(text, 0x106722a8);
            return;
        }
        static constexpr Line options[]{L(0x1e3b1, 0x106721b8, 0x64, manager_short), L(0x1e44c, 0x106721d4, 0x64, manager_short), L(0x1e4fb, 0x106721f0, 0x64, manager_short)};
        say(options[three_ways(count % 3)]);
        out.append(text, 0x106721fc);
        return;
    }
    if (cmp(flags, 0) == 0 || cmp(own_club, 0) == 0 || cmp(code, 1) == 0) {
        return;
    }
    // The flags in the order the original tests them. Flag 1 only counts for a club of a foreign nation.
    for (const auto& entry : std::span(flagged).first(4)) {
        if (cmp((flags & entry.bit), 0) == 0) continue;
        if (entry.bit == 1 && !nation_is_foreign()) continue;
        return flag_paragraph(entry);
    }
    if (cmp((flags & 0x10), 0) != 0) return flag_paragraph(flagged[4]);
    if (cmp((flags & 0x400), 0) == 0) {
        return;
    }
    switch (three_ways(count % 3)) {
    case 0:
        say(nation_is_foreign() ? L(0x1eba9, 0x10672614) : L(0x1ec07, 0x10672628));
        break;
    case 1:
        say(nation_is_foreign() ? L(0x1ec68, 0x10672674) : L(0x1ecd6, 0x10672688));
        break;
    default:
        say(L(0x1e823, 0x1067269c));
        break;
    }
    out.append(text, 0x106726a8);
}

// Whether the club's nation counts as foreign: it has no nation, its nation has no region, or the region's
// figure is the world's setting.
bool Press::nation_is_foreign() const {
    const auto nation = c.u32(club + 0x2c);
    return cmpl(nation, 0) == 0 || cmpl(c.u32(nation + 0x74), 0) == 0 || cmp(c.u32(c.u32(nation + 0x74) + 8), c.u32(c.toc(-0x7a70))) == 0;
}

// A flag's paragraph: one of three phrases by the remainder of the count, then appended to the story.
void Press::flag_paragraph(const FlagParagraph& entry) const {
    say(entry.lines[three_ways(count % 3)]);
    out.append(text, entry.append_back);
}
}

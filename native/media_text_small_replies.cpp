// The manager's spoken reply to a player's behaviour (0x10657b30): the sentence follows the
// kind of incident (a jump table in the executable) and the tone, 1 or 2 for the two named
// forms and a stock sentence for anything else.
#include "media_text_small.hpp"
#include "media_text_small_detail.hpp"
#include <algorithm>

namespace cm::aot::media {
using namespace detail;
using namespace detail::small;

namespace {
// One jump-table target: the sentences for tone 2 and 1 and the call sites that format them
// (the stock sentence for any other tone is shared).
struct Reply {
    std::uint32_t target;
    unsigned two_phrase;
    std::uint32_t two_back;
    unsigned one_phrase;
    std::uint32_t one_back;
    std::uint32_t other_back;
};
constexpr unsigned stock_phrase = 0xf69c;
constexpr Reply replies[] = {
    {0x10657b7c, 0xf62f, 0x10657bac, 0xf66c, 0x10657bc0, 0x10657bd4},
    {0x10657bd8, 0xf6a7, 0x10657c08, 0xf6cc, 0x10657c1c, 0x10657c30},
    {0x10657c34, 0xf6fd, 0x10657c64, 0xf71b, 0x10657c78, 0x10657c8c},
    {0x10657c90, 0xf737, 0x10657cc0, 0xf746, 0x10657cd4, 0x10657ce8},
    {0x10657cec, 0xf752, 0x10657d1c, 0xf791, 0x10657d30, 0x10657d44},
    {0x10657d48, 0xf7ac, 0x10657d78, 0xf7b7, 0x10657d8c, 0x10657da0},
    {0x10657da4, 0xf7c1, 0x10657dd4, 0xf7d2, 0x10657de8, 0x10657dfc},
    {0x10657e00, 0xf7f2, 0x10657e44, 0xf7e2, 0x10657e30, 0x10657e58},
    {0x10657e5c, 0xf815, 0x10657ea0, 0xf802, 0x10657e8c, 0x10657eb4},
    {0x10657eb8, 0xf840, 0x10657efc, 0xf830, 0x10657ee8, 0x10657f10},
    {0x10657f14, 0xf86f, 0x10657f58, 0xf858, 0x10657f44, 0x10657f6c},
    {0x10657f70, 0xf88d, 0x10657fa0, 0xf89a, 0x10657fb4, 0x10657fc8},
    {0x10657fcc, 0xf8bc, 0x10658010, 0xf8a5, 0x10657ffc, 0x10658024},
    {0x10658028, 0xf8ef, 0x1065806c, 0xf8d0, 0x10658058, 0x10658080},
    {0x10658084, 0xf86f, 0x106580c8, 0xf902, 0x106580b4, 0x106580dc},
    {0x106580e0, 0xf93b, 0x10658124, 0xf924, 0x10658110, 0x10658138},
    {0x1065813c, 0xf961, 0x10658180, 0xf952, 0x1065816c, 0x10658194},
    {0x10658198, 0xf97c, 0x106581dc, 0xf802, 0x106581c8, 0x106581f0},
    {0x106581f4, 0xf999, 0x10658238, 0xf802, 0x10658224, 0x1065824c},
    {0x10658250, 0xf9b1, 0x10658294, 0xf802, 0x10658280, 0x106582a8},
    {0x106582ac, 0xf9d7, 0x106582f0, 0xf9cb, 0x106582dc, 0x10658304},
    {0x10658308, 0xf9e6, 0x10658338, 0xf9f7, 0x1065834c, 0x10658360},
};
// The target of the last row: the incidents the table sends nowhere in particular, and the
// kinds beyond it.
constexpr std::uint32_t general_target = 0x10658308;
constexpr int first_kind = 3;
constexpr std::uint32_t kinds_in_table = 0x80;
}

void manager_reply_text(Cpu& c, std::uint32_t, std::int32_t kind, std::int32_t tone, std::uint32_t output, std::int32_t level) {
    Frame frame(c, 0x10657b30, 0x40, {});
    const auto pool = c.toc(slot::pool);
    c.set(3, pool);
    if (!nonzero_logical(c, output)) return;
    if (compare(c, std::int16_t(level), 0xf) < 0) return;
    const std::int32_t incident = std::int16_t(kind);
    c.set(4, std::uint32_t(incident));
    const auto index = std::uint32_t(incident) - first_kind;
    std::uint32_t target = general_target;
    if (compare_logical(c, index, kinds_in_table - 1) <= 0) target = c.u32(c.toc(-0x4214) + index * 4);
    const auto reply = std::ranges::find(replies, target, &Reply::target);
    if (reply == std::end(replies)) throw std::logic_error("manager_reply_text: jump table target outside the routine");
    const auto level_tone = std::int8_t(tone);
    if (compare(c, level_tone, 2) == 0) format(c, output, reply->two_phrase, reply->two_back);
    else if (compare(c, level_tone, 2) > 0 || compare(c, level_tone, 1) < 0) format(c, output, stock_phrase, reply->other_back);
    else format(c, output, reply->one_phrase, reply->one_back);
}
}

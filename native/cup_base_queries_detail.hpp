#pragma once
// CUP.CPP, block `queries`: what its routines share (the routines the news and the prizes call, the fields of the
// fixtures they read and the registers the original holds at their calls).
#include "cup_base_detail.hpp"
#include "cup_base_queries.hpp"
#include "league_base_news_detail.hpp"

namespace cm::aot::cup_base::queries {
using league_base::news::Held;
namespace item = league_base::news::item;
namespace callee_news {
// NEWS.CPP's filing routines this block calls besides league_base::news::item (the cup's own: a record's filing).
inline constexpr std::uint32_t file_under_record_cup = 0x102cc9e0;
}
// The fixture objects the news and the prizes read (inferred from the reads of this block).
namespace fixture {
inline constexpr std::uint32_t club_id = 0x0c,    // word: a club's id (0x1047ba50 looks it up in the round's ties); +0x1c and +0x20 the two clubs
                               code = 0x30,       // halfword: the competition's code (0x96 and 0x8c: final and its rounds)
                               half_32 = 0x32,    // halfword (inferred: the season's year)
                               round = 0x34,      // halfword: the round
                               flags = 0x38,      // halfword: the match flags (0x10485340)
                               byte_3a = 0x3a,    // byte
                               byte_4b = 0x4b, byte_4c = 0x4c;   // bytes (0x104858b0)
}
// Fields of the cup's rounds (cup::rounds rows) and match rows these routines read.
namespace round {
inline constexpr std::uint32_t label = 0x08,        // halfword: the round's label (0x8c, 0x82 and 0x96 are tested)
                               flags = 0x10,        // halfword: or-ed into a match's flags (0x10485340)
                               ties = 0x1c,         // halfword: the round's ties
                               first_place = 0x68,  // byte (0x10484fd0)
                               prize_a = 0x70, prize_b = 0x74;  // words: the round's prizes, -1: a share of the pool
}
namespace tie {
inline constexpr std::uint32_t side_0 = 0x00, side_1 = 0x02,     // halfwords: the two sides' entrant indexes, -1: undecided
                               feeder_tie_0 = 0x0a, feeder_round_0 = 0x0e,
                               feeder_tie_1 = 0x0c, feeder_round_1 = 0x10;
}
inline constexpr std::uint32_t pool_money = 0x28;  // the cup's word: the prize money left (inferred)
}

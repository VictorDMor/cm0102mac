// Kinds 0x42, 0x52, 0x44 and 0x5a: the stories that move the club's ratings by the pool's rates or by
// the table's sample, with no draw gate. 0x52 asks the club-record queries about the story's other club
// and the club's rival; 0x44 and 0x5a roll the first rating out of a range of 200.
#include "media_eval_c_detail.hpp"

namespace cm::aot::media::eval_c {
namespace {
constexpr std::uint32_t about_leader = 0x100d9480, about_follower = 0x100d94d0, about_rival_leader = 0x100d9520, about_rival_follower = 0x100d9580;
}

void Press::story_42() {
    if (flag(0x6)) scale_by(0x78);
    by_side(
        [&] {
            change_a(truncated(0x38, fpu.fdiv(dbl(0xb8), scale())), 0x106776ec);
            change_b(truncated(0x40, fpu.fmul(fpu.fdiv(dbl(0xb8), scale()), pull())), 0, 0x10677720);
            post(0x50, truncated(0x48, fpu.fdiv(dbl(0xb8), scale())), 0x10677754);
        },
        [&] {
            change_a(truncated(0x38, fpu.fmul(dbl(0xe0), scale())), 0x10677654);
            const auto k = fpu.fmul(scale(), pull());
            change_b(truncated(0x40, fpu.fmul(dbl(0xe0), k)), 0, 0x10677688);
            post(8, truncated(0x48, fpu.fmul(dbl(0xe8), scale())), 0x106776bc);
        },
        [&] { post(4, 0, 0x1067777c); });
}

// 0x52: the other club leads (or follows) the story's club, and the story's rival leads (or follows)
// it: each answer moves a rating by a fixed amount or by a pool rate times the rival's pull.
void Press::story_52() {
    const auto rival = [&] { return c.u32(club + 0x80); };
    // The two ratings and the item, for the answers of the four queries; `reversed` reverses the first pair.
    const auto stories = [&](const std::uint32_t (&asks)[4], std::int32_t amount, std::uint32_t first_pool, std::uint32_t second_pool,
                             const std::uint32_t (&backs)[4], std::uint32_t code, std::uint32_t post_back, bool reversed) {
        if (other_asks(about_leader, asks[0], club)) change_a(reversed ? 0u - amount : amount, backs[0]);
        else if (other_asks(about_follower, asks[1], club)) change_a(reversed ? amount : 0u - amount, backs[1]);
        c.set(4, rival());
        if (other_asks(about_rival_leader, asks[2], rival())) change_b(truncated(0x38, fpu.fmul(dbl(first_pool), pull())), 0, backs[2]);
        else if (other_asks(about_rival_follower, asks[3], rival())) change_b(truncated(0x38, fpu.fmul(dbl(second_pool), pull())), 0, backs[3]);
        post(code, 0, post_back, other);
    };
    by_side(
        [&] { stories({0x106777c8, 0x106777fc, 0x1067782c, 0x10677874}, 0x3e8, 0x50, 0x58, {0x106777ec, 0x10677820, 0x10677864, 0x106778ac}, 0x300, 0x106778d0, false); },
        [&] { stories({0x106778e0, 0x10677914, 0x10677944, 0x1067798c}, 0x3e8, 0x58, 0x50, {0x10677904, 0x10677938, 0x1067797c, 0x106779c4}, 0x80, 0x106779e8, true); },
        [&] { stories({0x106779f8, 0x10677a2c, 0x10677a5c, 0x10677aa4}, 0xc8, 0x60, 0x68, {0x10677a1c, 0x10677a50, 0x10677a94, 0x10677adc}, 4, 0x10677b00, true); });
}

// 0x44 and 0x5a: the first rating moves by the table's sample reduced by 200, up on side 1 and down on side 2.
void Press::story_scored(std::uint32_t down_refill, std::uint32_t down_back, std::uint32_t up_refill, std::uint32_t up_back) {
    const auto s = byte_word(side);
    const auto order = compare(c, s, 2);
    const auto moved = [&](std::uint32_t refill, std::uint32_t back, bool downward) {
        rng.advance(refill);
        const auto drawn = downward ? reduced(-0x51eb851f, 6, -200) : reduced(0x51eb851f, 6, 200);
        const auto amount = downward ? 0u - std::uint32_t(drawn.rest) : std::uint32_t(drawn.rest);
        c.set(9, std::uint32_t(drawn.sample));
        change_a(extended(std::int16_t(amount)), back);
    };
    if (order == 0) moved(down_refill, down_back, true);
    else if (order < 0 && compare(c, s, 1) >= 0) moved(up_refill, up_back, false);
}
}

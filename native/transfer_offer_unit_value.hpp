#pragma once
// TRANSFER_OFFER.CPP: what an offer is worth to the clubs it concerns, and how its terms are adjusted.
#include "guest_abi.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::transfer_offer_unit {
// 0x10415e50: a draw from the game's sample table below `limit` (negative limits draw below their
// magnitude and come out negative); a limit past 0xffff scales the draw and adds a tenth of a draw
// below a tenth of the limit, down to four levels inside one activation, the next level a recursive
// call. 0 when the limit is 0 (no draw).
std::int32_t draw_below(guest::Cpu& c, std::int32_t limit);
// 0x10416840: the offer's value to its buyer: the fee, plus (unless `fee_only`) the instalments and the
// appearance extra weighted by the player and the clubs, plus (with `with_sell_on`) the sell-on share,
// plus the players in exchange (unless `without_exchange`; `exchange_valued` prices them through
// 0x10084be0, otherwise by their record's word at +8), the whole scaled by the club's standing (when
// `by_club_standing`) and truncated through a single-precision multiply.
std::int32_t offer_value(guest::Cpu& c, std::uint32_t offer, std::int8_t fee_only, std::int8_t without_exchange,
                         std::int8_t exchange_valued, std::int8_t by_club_standing, std::int8_t with_sell_on);
// 0x10416e10: whether two offers have the same terms: the fee, instalments, extra, sell-on and months
// (a difference in a term only counts when one side has it), the kind's high nibble, and the same set of
// players in exchange.
std::uint32_t same_terms(guest::Cpu& c, std::uint32_t offer, std::uint32_t other);
// 0x104170f0: takes a player out of the offer's exchange list (destroying the list when it empties), adds
// his value to the fee and counts him out of the club's table at +0x9a0.
void remove_exchange_player(guest::Cpu& c, std::uint32_t offer, std::uint32_t person);
// 0x104172f0: a plain fee for a player the club can pay for twice over, reshaped by the clubs' standing
// into a smaller fee with a sell-on share, instalments or an appearance extra (unfinished terms only).
void reshape_fee(guest::Cpu& c, std::uint32_t offer, std::uint32_t person, std::uint32_t club);

namespace value_detail {
// Words the block's files share. `pool` is the unit's double constants (TOC -0x4b8c).
inline constexpr std::uint32_t magic = 0xc0;  // the int-to-double constant
// A 32-bit integer as the original converts it: the double of high word 0x43300000 and low word
// `x` xor 0x80000000, less the constant at +0xc0.
inline double to_double(guest::Cpu& c, guest::Fpu& fpu, std::uint32_t pool, std::int32_t x) {
    const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u);
    return fpu.fsub(std::bit_cast<double>(bits), c.f64(pool + magic));
}
// `divw` as the generated code does it (never by 0 or 0x80000000 by -1 here).
inline std::int32_t divide(std::int32_t a, std::int32_t b) { return std::int32_t(std::int64_t(a) / b); }
// fctiwz: the low word of the converted value.
inline std::int32_t truncate(guest::Fpu& fpu, double value) { return std::int32_t(fpu.fctiwz(0, value)); }
}
}

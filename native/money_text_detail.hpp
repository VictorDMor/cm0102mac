#pragma once
#include "money_text.hpp"
#include "semantic_guest_call.hpp"
namespace cm::aot::money_text::detail {
using Call=SemanticGuestCall;
void render_grouped(Call&,std::uint32_t destination);
void render_compact(Call&,std::uint32_t destination);
void build_decimal_digits(Call&);
void replace_spaces(Call&);
}

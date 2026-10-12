#pragma once
#include "semantic_guest_call.hpp"
namespace cm::aot::player_movement {
// The corresponding original kind-17/kind-18 blocks have identical register,
// dependency and memory operations, with only local continuation labels moved.
void sample_carrier_condition(SemanticGuestCall&,std::uint32_t label_delta);
void sample_carrier_interest(SemanticGuestCall&,std::uint32_t label_delta);
bool describe_carrier_direction(SemanticGuestCall&,std::int32_t toc_slot,std::uint32_t label_delta,std::uint32_t direct_destination=0);
void describe_carrier_opposition(SemanticGuestCall&,std::uint32_t label_delta);
void complete_carrier_clear_position(SemanticGuestCall&,std::uint32_t label_delta);
}

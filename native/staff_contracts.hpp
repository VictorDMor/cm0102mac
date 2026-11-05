#pragma once

#include "guest_abi.hpp"

namespace cm::aot::staff_contracts {

inline constexpr std::uint32_t expiry_entry = 0x10380710;

void initialize_draft(guest::Cpu&, std::uint32_t contract);
std::int16_t end_date_delta(guest::Cpu&, std::uint32_t contract);
bool supports_bonus(guest::Cpu&, std::uint32_t person, std::int8_t bonus);

void name_bonus(guest::Cpu&, std::int8_t index, std::uint32_t destination);
void name_option(guest::Cpu&, std::int8_t index, std::uint32_t destination);
void describe_bonus(guest::Cpu&, std::uint32_t contract, std::uint32_t index, std::uint32_t destination,
                    std::uint32_t name_only);
void describe_option(guest::Cpu&, std::uint32_t index, std::uint32_t destination);

void enable_option(guest::Cpu&, std::uint32_t contract, std::int8_t option);
void clear_option(guest::Cpu&, std::uint32_t contract, std::int8_t option);
void set_bonus(guest::Cpu&, std::uint32_t contract, std::int8_t bonus, std::uint32_t amount);
void clear_bonus(guest::Cpu&, std::uint32_t contract, std::int8_t bonus);
void reset_terms(guest::Cpu&, std::uint32_t contract);
bool is_expired(guest::Cpu&, std::uint32_t contract);
bool is_rolling(guest::Cpu&, std::uint32_t contract);
bool has_release_terms(guest::Cpu&, std::uint32_t contract);
void adjust_primary_value(guest::Cpu&, std::uint32_t contract);
void adjust_alternate_value(guest::Cpu&, std::uint32_t contract);
void set_release_amount(guest::Cpu&, std::uint32_t contract, std::uint32_t amount);

void clear_status_bits(guest::Cpu&, std::uint32_t contract, std::uint32_t mask);
void reset_status_bits(guest::Cpu&, std::uint32_t contract);
std::uint32_t status_bits(guest::Cpu&, std::uint32_t contract);

bool query_start_date_window(guest::Cpu&, std::uint32_t contract, std::uint32_t exact_boundary,
                             std::uint32_t age_threshold);
std::uint8_t classify_expiry(guest::Cpu&, std::uint32_t contract);

}

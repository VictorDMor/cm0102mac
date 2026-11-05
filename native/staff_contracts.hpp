#pragma once

#include "guest_abi.hpp"

namespace cm::aot::staff_contracts {

inline constexpr std::uint32_t expiry_entry = 0x10380710;
inline constexpr std::uint32_t fixed_date_entry = 0x103810d0;

std::uint32_t sample_range(guest::Cpu&, std::int32_t bound);
void initialize_fixed_date(guest::Cpu&);
void initialize_draft(guest::Cpu&, std::uint32_t contract);
void prepare_new_draft(guest::Cpu&, std::uint32_t contract, std::uint32_t staff, std::uint32_t club,
                       std::uint8_t duration);
void prepare_primary_draft(guest::Cpu&, std::uint32_t contract, std::uint32_t staff, std::uint32_t club,
                           std::uint32_t end, std::uint32_t percentage, std::uint32_t first_clause,
                           std::uint32_t second_clause);
std::int16_t end_date_delta(guest::Cpu&, std::uint32_t contract);
bool supports_bonus(guest::Cpu&, std::uint32_t person, std::int8_t bonus);

bool supports_option(guest::Cpu&, std::uint32_t staff, std::uint32_t club, std::int8_t option);
void format_wage_text(guest::Cpu&, std::uint32_t contract, std::uint32_t destination,
                      std::uint32_t style, std::int8_t format_flags);
void format_contract_end(guest::Cpu&, std::uint32_t contract, std::uint32_t destination);

void describe_expiry_text(guest::Cpu&, std::uint32_t contract, std::uint32_t destination, std::int8_t style);
void value_contract_terms(guest::Cpu&, std::uint32_t contract, std::uint32_t staff, std::uint32_t alternative);
void classify_renewal_offer(guest::Cpu&, std::uint32_t contract, std::uint32_t staff);
void apply_country_wage_quote(guest::Cpu&, std::uint32_t contract, std::uint32_t player);
void describe_squad_status(guest::Cpu&, std::uint32_t contract, std::int32_t capacity);
void describe_release_summary(guest::Cpu&, std::uint32_t contract, std::int32_t capacity);

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

void reset_review_state(guest::Cpu&, std::uint32_t contract, std::uint32_t staff, std::uint32_t club);
bool has_review_trigger(guest::Cpu&, std::uint32_t contract);
void refresh_review_conditions(guest::Cpu&, std::uint32_t contract, std::uint32_t context);
void apply_review_conditions(guest::Cpu&, std::uint32_t contract, std::uint32_t conditions, std::uint32_t context);
void add_review_status_bits(guest::Cpu&, std::uint32_t contract, std::uint32_t bits);
void clear_review_condition(guest::Cpu&, std::uint32_t contract, std::uint32_t mask);
void clear_review_conditions(guest::Cpu&, std::uint32_t contract, std::uint32_t refresh);
void clear_status_bits(guest::Cpu&, std::uint32_t contract, std::uint32_t mask);
void reset_status_bits(guest::Cpu&, std::uint32_t contract);
std::uint32_t status_bits(guest::Cpu&, std::uint32_t contract);

bool query_start_date_window(guest::Cpu&, std::uint32_t contract, std::uint32_t exact_boundary,
                             std::uint32_t age_threshold);
std::uint8_t classify_expiry(guest::Cpu&, std::uint32_t contract);

}

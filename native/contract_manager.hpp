#pragma once
#include "guest_abi.hpp"

namespace cm::aot::contract_manager {
using guest::Cpu;
std::uint32_t primary_contract(Cpu&, std::uint32_t manager, std::uint32_t person);
std::uint32_t secondary_contract(Cpu&, std::uint32_t manager, std::uint32_t person);
std::uint32_t club_contract(Cpu&, std::uint32_t manager, std::uint32_t person, std::uint32_t club);
std::uint32_t primary_terms(Cpu&, std::uint32_t manager, std::uint32_t person);
std::uint32_t type_allows_unattached(Cpu&, std::uint32_t person);
std::uint32_t type_is_nonplaying(Cpu&, std::uint32_t kind);
std::uint32_t type_is_employed(Cpu&, std::uint32_t kind);
std::uint32_t person_status(Cpu&, std::uint32_t person);
std::uint32_t copy_three_words(Cpu&, std::uint32_t destination, std::uint32_t source);
std::uint32_t copy_date_through(Cpu&, std::uint32_t result, std::uint32_t destination, std::uint32_t source);
std::uint32_t compare_record_keys(Cpu&, std::uint32_t first, std::uint32_t second);
std::uint32_t contract_month(Cpu&, std::uint32_t person);
void remove_secondary(Cpu&, std::uint32_t manager, std::uint32_t person);
std::uint32_t reserve_contract(Cpu&, std::uint32_t manager, std::uint32_t append_only);
std::uint32_t bind_primary(Cpu&, std::uint32_t manager, std::uint32_t person, std::uint32_t initial_value);
std::uint32_t compare_scheduled(Cpu&, std::uint32_t first, std::uint32_t second);
std::uint32_t find_keyed_record(Cpu&, std::uint32_t manager, std::uint32_t subject,
                                std::uint32_t target, std::uint32_t club);
void add_keyed_record(Cpu&, std::uint32_t manager, std::uint32_t subject,
                       std::uint32_t target, std::uint32_t club, std::uint32_t kind);
void remove_keyed_records(Cpu&, std::uint32_t manager, std::uint32_t subject,
                           std::uint32_t target, std::uint32_t club);
void initialize_dates(Cpu&);
std::uint32_t find_pending(Cpu&, std::uint32_t manager, std::uint32_t person_id, std::uint32_t club_id);
std::uint32_t find_pending_transfer(Cpu&, std::uint32_t manager, std::uint32_t person_id);
void remove_pending(Cpu&, std::uint32_t manager, std::uint32_t person, std::uint32_t club);
void remove_pending_origin(Cpu&, std::uint32_t manager, std::uint32_t person, std::uint32_t origin);
std::uint32_t construct(Cpu&, std::uint32_t manager, std::uint32_t file);
std::uint32_t destroy(Cpu&, std::uint32_t manager, std::uint32_t deleting);
std::int32_t sample_range(Cpu&, std::int32_t bound);
void initialize_staff_contracts(Cpu&, std::uint32_t manager);
std::uint32_t classify_relationship(Cpu&, std::uint32_t manager, std::uint32_t person, std::uint32_t club);
std::uint32_t update_relationship(Cpu&, std::uint32_t manager, std::uint32_t person,
                                  std::uint32_t club, std::uint32_t other_club);
std::uint32_t update_first_relationship(Cpu&, std::uint32_t manager, std::uint32_t person, std::uint32_t club);
std::uint32_t update_second_relationship(Cpu&, std::uint32_t manager, std::uint32_t person, std::uint32_t club);
std::uint32_t relationship_status(Cpu&, std::uint32_t manager, std::uint32_t person, std::uint32_t club);
std::uint32_t describe_status(Cpu&, std::uint32_t person, std::uint32_t buffer);
std::uint32_t describe_status_difference(Cpu&, std::uint32_t first, std::uint32_t second, std::uint32_t buffer);
std::uint32_t role_available(Cpu&, std::uint32_t role, std::uint32_t person);
void compact_schedule(Cpu&, std::uint32_t manager);
void add_scheduled(Cpu&, std::uint32_t manager, std::uint32_t person, std::uint32_t date,
                    std::uint32_t kind, std::uint32_t origin);
void consume_scheduled(Cpu&, std::uint32_t manager, std::uint32_t date, std::uint32_t kind);
std::uint32_t minimum_contract_quote(Cpu&, std::uint32_t person, std::uint32_t club);
std::uint32_t collect_club_pending(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t output);
std::uint32_t copy_contract(Cpu&, std::uint32_t destination, std::uint32_t source);
std::uint32_t process_deferred(Cpu&, std::uint32_t manager, std::uint32_t person_id);
void remove_deferred(Cpu&, std::uint32_t manager, std::uint32_t person_id);
void remove_primary(Cpu&, std::uint32_t manager, std::uint32_t person);
void add_pending(Cpu&, std::uint32_t manager, std::uint32_t contract, std::uint32_t origin,
                   std::uint32_t offer_id, std::uint32_t kind, std::uint32_t notify, std::uint32_t force_transfer);
std::uint32_t acknowledge_pending(Cpu&, std::uint32_t manager, std::uint32_t offer);
void remove_all_pending(Cpu&, std::uint32_t manager, std::uint32_t person);
void end_secondary_contract(Cpu&, std::uint32_t manager, std::uint32_t person, std::uint32_t club);
void mark_expiring_roster(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t context);
void mark_secondary_roster(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t context);
void reset_roster_interest(Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t notify);
void schedule_pending_review(Cpu&, std::uint32_t manager, std::uint32_t person_id, std::uint32_t origin);
std::uint32_t preferred_contract_role(Cpu&, std::uint32_t manager, std::uint32_t person);
void publish_contract_renewal(Cpu&, std::uint32_t person, std::uint32_t club);
void publish_contract_departure(Cpu&, std::uint32_t person, std::uint32_t club);
std::uint32_t release_quote(Cpu&, std::uint32_t manager, std::uint32_t context,
                             std::uint32_t proposed_club, std::uint32_t person);
std::uint32_t review_priority(Cpu&, std::uint32_t person, std::uint32_t contract);
void adjust_contract_interest(Cpu&, std::uint32_t person, std::uint32_t club, std::uint32_t contract);
void publish_pending_club(Cpu&, std::uint32_t contract, std::uint32_t club);
void publish_pending_outcome(Cpu&, std::uint32_t contract, std::uint32_t outcome);
void review_contract_discontent(Cpu&, std::uint32_t manager, std::uint32_t person,
                                 std::uint32_t club, std::uint32_t contract);
void review_match_contracts(Cpu&, std::uint32_t manager, std::uint32_t fixture, std::uint32_t match_data);
void save_contracts(Cpu&, std::uint32_t manager, std::uint32_t file);
std::uint32_t load_contracts(Cpu&, std::uint32_t manager, std::uint32_t file);
std::uint32_t prepare_contract_offer(Cpu&, std::uint32_t manager, std::uint32_t person,
                                     std::uint32_t club, std::uint32_t output, std::uint32_t origin,
                                     std::uint32_t mode, double scale);
void review_pending_contracts(Cpu&, std::uint32_t manager, std::uint32_t person);
void process_daily_contracts(Cpu&, std::uint32_t manager, std::uint32_t date,
                             std::uint32_t phase, std::uint32_t scan);
std::uint32_t apply_primary_contract(Cpu&, std::uint32_t manager, std::uint32_t proposal, std::uint32_t kind);
void apply_secondary_contract(Cpu&, std::uint32_t manager, std::uint32_t proposal);
void finish_secondary_contract(Cpu&, std::uint32_t manager, std::uint32_t person);
std::uint32_t contract_offer_allowed(Cpu&, std::uint32_t manager, std::uint32_t proposal);
void rebalance_pending_offer(Cpu&, std::uint32_t manager, std::uint32_t proposal);
std::uint32_t base_player_value(Cpu&, std::uint32_t manager, std::uint32_t person,
                                 std::uint32_t club, std::uint32_t adjusted, std::uint32_t supplied_score);
std::uint32_t negotiated_player_value(Cpu&, std::uint32_t manager, std::uint32_t employer,
                                     std::uint32_t buyer, std::uint32_t person, std::uint32_t diagnostics,
                                     std::uint32_t tenure_option, std::uint32_t floor_option);
void render_cancelled_offer(Cpu&, std::uint32_t message, std::uint32_t mode, std::uint32_t output);
void publish_expiring_contract(Cpu&, std::uint32_t contract, std::uint32_t period,
                               std::uint32_t bosman, std::uint32_t distribute);
void render_expiring_contract(Cpu&, std::uint32_t message, std::uint32_t mode, std::uint32_t output);
void render_bosman_departure(Cpu&, std::uint32_t message, std::uint32_t mode, std::uint32_t output);
void render_withdrawn_request(Cpu&, std::uint32_t message, std::uint32_t mode, std::uint32_t output);
void publish_assurance_request(Cpu&, std::uint32_t contract);
void render_assurance_request(Cpu&, std::uint32_t message, std::uint32_t mode, std::uint32_t output);
void render_cancelled_trial(Cpu&, std::uint32_t message, std::uint32_t mode, std::uint32_t output);
void publish_loan_return(Cpu&, std::uint32_t person, std::uint32_t related_club);
void render_loan_return(Cpu&, std::uint32_t message, std::uint32_t mode, std::uint32_t output);
void publish_new_contract_request(Cpu&, std::uint32_t contract);
void render_new_contract_request(Cpu&, std::uint32_t manager, std::uint32_t message,std::uint32_t mode,std::uint32_t output);
void publish_departure_status(Cpu&, std::uint32_t manager,std::uint32_t person,std::uint32_t kind,std::uint32_t important);
void render_departure_status(Cpu&, std::uint32_t message,std::uint32_t mode,std::uint32_t output);
void publish_contract_offer_result(Cpu&, std::uint32_t manager,std::uint32_t proposal,std::uint32_t notify);
void render_contract_offer_result(Cpu&, std::uint32_t message,std::uint32_t mode,std::uint32_t output);
void publish_transfer_unrest(Cpu&, std::uint32_t manager,std::uint32_t person,std::uint32_t club);
void render_transfer_unrest(Cpu&, std::uint32_t message,std::uint32_t mode,std::uint32_t output);
void review_transfer_request(Cpu&, std::uint32_t manager,std::uint32_t contract);
void render_transfer_request(Cpu&, std::uint32_t message,std::uint32_t mode,std::uint32_t output);
}

#pragma once
#include "guest_abi.hpp"

namespace cm::aot::award_manager {
std::uint32_t construct(guest::Cpu& c, std::uint32_t manager, std::uint32_t file);
std::uint32_t destroy(guest::Cpu& c, std::uint32_t manager, std::uint32_t deleting);
void initialize(guest::Cpu& c, std::uint32_t manager, std::uint32_t file);
std::uint32_t load(guest::Cpu& c, std::uint32_t manager, std::uint32_t file);
void save(guest::Cpu& c, std::uint32_t manager, std::uint32_t file);

std::uint32_t register_shortlist(guest::Cpu& c, std::uint32_t manager, std::uint32_t shortlist);
std::uint32_t has_recent_history(guest::Cpu& c, std::uint32_t manager);
void set_ballot_context(guest::Cpu& c, std::uint32_t manager, std::uint32_t voter,
                        std::uint32_t category, std::uint32_t first, std::uint32_t second);
std::uint32_t ballot_context(guest::Cpu& c, std::uint32_t manager, std::uint32_t voter, std::uint32_t category);
std::uint32_t find_award_by_id(guest::Cpu& c, std::uint32_t manager, std::uint32_t id);
std::uint32_t find_award_by_label(guest::Cpu& c, std::uint32_t manager, std::uint32_t label);
std::uint32_t eligible_for_regional_award(guest::Cpu& c, std::uint32_t club, std::uint32_t person);
void register_result(guest::Cpu& c, std::uint32_t manager, std::uint32_t participant, std::uint32_t result_record);
void advance_date(guest::Cpu& c, std::uint32_t manager, std::uint32_t date);
void read_player_history(guest::Cpu& c, std::uint32_t manager, std::uint32_t key,
                         std::uint32_t output, std::uint32_t limit);
void reset_player_history(guest::Cpu& c, std::uint32_t manager);
void read_team_history(guest::Cpu& c, std::uint32_t manager, std::uint32_t key,
                       std::uint32_t output, std::uint32_t limit);
void reset_team_history(guest::Cpu& c, std::uint32_t manager);
std::uint32_t find_national_group(guest::Cpu& c, std::uint32_t manager, std::uint32_t nation);
void award_news_subjects(guest::Cpu& c, std::uint32_t manager, unsigned group, unsigned award_index,
                         std::uint32_t message, std::uint32_t subject, std::uint32_t destination);
void format_award_news(guest::Cpu& c, std::uint32_t manager, std::uint32_t message,
                       std::uint32_t output, std::uint32_t show_detail);
}

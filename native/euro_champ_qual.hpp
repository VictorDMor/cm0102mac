#pragma once
#include "guest_abi.hpp"
namespace cm::aot::euro_champ_qual {
std::uint32_t construct(guest::Cpu&, std::uint32_t object, std::uint32_t year, std::uint32_t definition);
std::uint32_t destroy(guest::Cpu&, std::uint32_t object, std::uint32_t deleting);
std::uint32_t calendar(guest::Cpu&, std::uint32_t object, std::uint32_t phase, std::uint32_t count, std::uint32_t kind, std::uint32_t flag);
void advance_stage(guest::Cpu&, std::uint32_t object);
std::uint32_t group_size(guest::Cpu&,std::uint32_t object,std::uint32_t group);
std::uint32_t configure(guest::Cpu&,std::uint32_t object);
void next_season(guest::Cpu&,std::uint32_t object);
void next_round(guest::Cpu&,std::uint32_t object);
void create_playoffs(guest::Cpu&,std::uint32_t object);
std::uint32_t mark_active(guest::Cpu&,std::uint32_t object);
void finalize(guest::Cpu&,std::uint32_t object);
void publish_qualified(guest::Cpu&,std::uint32_t object);
void record_stage(guest::Cpu&,std::uint32_t object,std::uint32_t stage,std::uint32_t record);
void clear_record(guest::Cpu&,std::uint32_t object);
std::uint32_t definition(guest::Cpu&);
std::uint32_t load(guest::Cpu&,std::uint32_t object,std::uint32_t file);
void save(guest::Cpu&,std::uint32_t object,std::uint32_t file);
void seed_rankings(guest::Cpu&,std::uint32_t object);
void update_record(guest::Cpu&,std::uint32_t object,std::uint32_t index,std::uint32_t stage,std::uint32_t first,std::uint32_t second,std::uint32_t third);
std::uint32_t phase_category(guest::Cpu&,std::uint32_t object,std::uint32_t index);
std::uint32_t compare_categories(guest::Cpu&,std::uint32_t object,std::uint32_t index);
void describe_stage(guest::Cpu&,std::uint32_t object,std::uint32_t buffer,std::uint32_t capacity,std::uint32_t phase,std::uint32_t unused,std::uint32_t event,std::uint32_t club);
std::uint32_t create_group(guest::Cpu&,std::uint32_t object,std::uint32_t phase,std::uint32_t entrants,std::uint32_t count);
std::uint32_t select_entrants(guest::Cpu&,std::uint32_t object);
void refresh_dates(guest::Cpu&,std::uint32_t object);
std::uint32_t random_below(guest::Cpu&,std::uint32_t bound);
void draw_groups(guest::Cpu&,std::uint32_t object);
void insert_fixture(guest::Cpu&,std::uint32_t object,std::uint32_t stage,std::uint32_t day,std::uint32_t month,std::uint32_t year,std::uint32_t home,std::uint32_t home_score,std::uint32_t away,std::uint32_t away_score);
std::uint32_t update(guest::Cpu&,std::uint32_t object,std::uint32_t date,std::uint32_t active);
std::uint32_t notify(guest::Cpu&,std::uint32_t object,std::uint32_t club,std::uint32_t event,std::uint32_t stage,std::uint32_t output,std::uint32_t record);
void describe_event(guest::Cpu&,std::uint32_t object,std::uint32_t club_id,std::uint32_t event,std::uint32_t unused,std::uint32_t phase,std::uint32_t record,std::uint32_t context,std::uint32_t unused2,std::uint32_t narrow,std::uint32_t live,std::uint32_t buffer);
}

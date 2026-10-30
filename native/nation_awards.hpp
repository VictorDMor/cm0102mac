#pragma once
#include "guest_abi.hpp"

namespace cm::aot::nation_awards {
using guest::Cpu;
std::uint32_t construct(Cpu&, std::uint32_t manager);
std::uint32_t destroy(Cpu&, std::uint32_t manager, std::uint32_t deleting);
void update(Cpu&, std::uint32_t manager);
void collect_month(Cpu&, std::uint32_t manager);
void publish_ranked(Cpu&, std::uint32_t manager, std::uint32_t date, std::uint32_t award,
                    std::uint32_t ratings, std::uint32_t filter, std::uint32_t first,
                    std::uint32_t second, std::uint32_t third);
void publish_managers(Cpu&, std::uint32_t date, std::uint32_t award, std::uint32_t ratings,
                      std::uint32_t filter, std::uint32_t first, std::uint32_t second,
                      std::uint32_t selected);
void publish_competition_managers(Cpu&, std::uint32_t date, std::uint32_t award, std::uint32_t competition);
void publish_ratings(Cpu&, std::uint32_t date, std::uint32_t award);
void select_league_players(Cpu&, std::uint32_t competition, std::uint32_t date, std::uint32_t award,
                          std::uint32_t age, std::uint32_t positions, std::uint32_t statistic,
                          std::uint32_t nation);
void select_nation_players(Cpu&, std::uint32_t competition, std::uint32_t date, std::uint32_t award,
                          std::uint32_t age, std::uint32_t positions, std::uint32_t statistic);
void select_world_players(Cpu&, std::uint32_t date, std::uint32_t award,
                         std::uint32_t club_nation, std::uint32_t nation, std::uint32_t excluded_nation,
                         std::uint32_t either_nation, std::uint32_t age);
void select_league_team(Cpu&, std::uint32_t competition, std::uint32_t award);
void select_nation_team(Cpu&, std::uint32_t competition, std::uint32_t award);
void select_recent_team(Cpu&, std::uint32_t competition, std::uint32_t award);
std::uint32_t load(Cpu&, std::uint32_t manager, std::uint32_t file);
void unused(Cpu&);
void save(Cpu&, std::uint32_t manager, std::uint32_t file);
std::uint32_t compare_appearances(Cpu&, std::uint32_t left, std::uint32_t right);
std::uint32_t compare_score(Cpu&, std::uint32_t left, std::uint32_t right);
std::uint32_t compare_score_reverse(Cpu&, std::uint32_t left, std::uint32_t right);
}

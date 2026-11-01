#pragma once
#include "media.hpp"

namespace cm::aot::media {
std::uint32_t lists_kind_two(guest::Cpu&, std::uint32_t list, std::uint32_t key);
std::uint32_t lists_person(guest::Cpu&, std::uint32_t list, std::uint32_t key);
std::uint32_t listed_grade(guest::Cpu&, std::uint32_t list, std::uint32_t person);
void is_grade_band(guest::Cpu&, std::int32_t grade);
void compare_links(guest::Cpu&, std::uint32_t left, std::uint32_t right);
void compare_rows(guest::Cpu&, std::uint32_t left, std::uint32_t right);
void is_ordinary_story(guest::Cpu&, std::int32_t story);
void is_transfer_story(guest::Cpu&, std::int32_t story);
}

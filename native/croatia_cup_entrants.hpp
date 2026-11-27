#pragma once
#include <cstdint>

namespace cm::aot::guest { class Cpu; }

namespace cm::aot::croatia_cup {
struct Profile;
// Both editions draw 48 clubs. Returns 1 on success, -1 on failure. The
// constructor's caller maps failure to zero; the season rollover tests < 0.
std::int32_t select_entrants(guest::Cpu&, std::uint32_t cup, const Profile&);
}

#pragma once
#include <cstdint>

namespace cm::aot::guest { class Cpu; }

namespace cm::aot::croatia_cup {
struct Profile;
// CRO_CUP.CPP, virtual +0x44. Only the root stage (-1) has a calendar.
std::uint32_t calendar(guest::Cpu&, std::uint32_t cup, std::uint32_t stage, std::uint32_t count_out,
                       std::uint32_t code_out, std::uint32_t enabled_out, const Profile&);
}

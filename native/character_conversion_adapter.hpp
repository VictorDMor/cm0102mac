#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 6> character_conversion_entries{
    0x10777f60, 0x10778050, 0x10778170, 0x10778190, 0x10778240, 0x10778260};
struct CharacterConversionBoundaryObserver {
    virtual ~CharacterConversionBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t entry, bool original) = 0;
    virtual void leave(State&, std::uint32_t entry, bool original) = 0;
};
bool try_modern_character_conversion(State&, std::uint32_t entry);
void reset_character_conversion_diagnostics(State&);
std::string character_conversion_call_summary(const State&);
}

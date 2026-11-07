#pragma once
// DATABASE.CPP's record layouts, as far as its rewritten routines read them.
// Meanings marked "inferred" come from how the callers use the values.
#include <cstdint>

namespace cm::aot::database {
// A club: two lists of three people (inferred: favoured and disliked staff).
namespace club {
constexpr std::uint32_t favoured_staff = 0x5c, disliked_staff = 0x68;
}
// A person: the pointer to their preferences record.
namespace person {
constexpr std::uint32_t preferences = 0x18;
}
// A person's preferences: four lists of three ids (inferred: favourite and
// disliked clubs, favourite and disliked staff).
namespace preferences {
constexpr std::uint32_t favourite_clubs = 0x4, disliked_clubs = 0x10, favourite_staff = 0x1c, disliked_staff = 0x28;
}
// The 16-bit values the attribute ratings scale by (meanings not established).
namespace rated {
constexpr std::uint32_t value_04 = 0x4, value_24 = 0x24;
}
}

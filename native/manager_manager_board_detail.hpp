#pragma once
// MANAGER_MANAGER.CPP board reaction (0x101a2eb0, 0x101a05c0): the record fields, frame slots and pool
// constants they use. Offsets are the original's; meanings are named as far as the code shows them.
#include <cstdint>

namespace cm::aot::manager_manager::board_detail {
namespace toc {
// The people table (0x70 bytes per person), the club table (0x248), the context the valuation 0x10084be0
// takes (a cell), the module's two pools (floats; doubles) and the table of employer records.
constexpr std::int32_t people = -0x7be0, clubs = -0x7be8, context = -0x7b7c, pool_floats = -0x5370,
                       pool = -0x536c, employers = -0x7ab8;
}
namespace callee {
constexpr std::uint32_t club_state = 0x1014c2e0, valuation = 0x10084be0, standing = 0x10416710,
                        pair_a = 0x100d9480, pair_b = 0x100d94d0, assistant_a = 0x100d9520,
                        assistant_b = 0x100d9580, club_flag = 0x100c7d00, employer_record = 0x10103510,
                        notify = 0x100d9e10;
}
// The job record the caller passes (react's r4).
namespace job {
constexpr std::uint32_t club = 0x8, person = 0xc, closed = 0x24;
}
namespace person {
constexpr std::uint32_t size = 0x70, career = 0x14, employer = 0x24;
}
namespace club {
constexpr std::uint32_t size = 0x248, id = 0x0, assistant = 0x80, reputation = 0x1ec;
}
namespace employer_record {
constexpr std::uint32_t barred = 0x165;
}
// The module object (TOC -0x79f0): the board's rows, 0x4c bytes per club by the club's id.
namespace object {
constexpr std::uint32_t table = 0x0;
}
namespace row {
constexpr std::uint32_t size = 0x4c, value = 0x6, confidence = 0x14, trust = 0x16, drift = 0x38;
}
// judge's frame (0x90 bytes, r22-r31 saved): five doubles from 0x38 (high word, then the low word at +4) the
// int-to-double idiom and `fctiwz` use. The club's pass: `fourth` and `first` the valuation, `third` the
// reputation, `second` the standing; the employer's pass: `first` and `fourth` the valuation, `second` the
// reputation, `third` the standing. `truncated` takes the fctiwz result of the reputation's square.
namespace slot {
constexpr std::uint32_t first = 0x38, second = 0x40, truncated = 0x48, third = 0x50, fourth = 0x58;
}
// react's frame (0x50 bytes, r29-r31 saved): the two halfwords judge fills.
constexpr std::uint32_t flags_a_slot = 0x38, flags_b_slot = 0x3a;
constexpr std::uint32_t conversion_high = 0x43300000;
constexpr std::int32_t high_valuation = 250000;  // 0x40000 - 0x2f70
// The module's pool (TOC -0x536c): doubles.
namespace pool {
constexpr std::uint32_t bias = 0x38, scale = 0x60;
}
// The float at +0x1c of the pool at TOC -0x5370: the ratio when the valuation is 0.
constexpr std::uint32_t empty_ratio = 0x1c;
}

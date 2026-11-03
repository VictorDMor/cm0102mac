#include "contract_screens.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"

namespace cm::aot::contract_screens {
using namespace guest;
namespace {
constexpr int rating_constants = -0x56f4, player_stats_manager = -0x7b3c;
constexpr std::uint32_t player_value = 0x10303560;
struct Candidate {
    Cpu& cpu;
    std::uint32_t staff;

    std::uint32_t profile() const { return cpu.u32(staff + 0x14); }
};

bool read_appearances(Cpu& c, const Candidate& candidate, std::uint32_t manager,
                      std::uint32_t constants, std::uint32_t back) {
    const auto profile = candidate.profile();
    c.set(4, profile);
    if (compare_logical(c, profile) == 0) return false;

    c.set({{3, c.u32(manager)}, {5, 6}, {6, 1}});
    c.set_f(1, c.f32(constants + 8));
    c.call(player_value, back);
    return true;
}
}

std::uint32_t compare_candidates(Cpu& c, std::uint32_t left_slot, std::uint32_t right_slot) {
    Frame frame(c, 0x1009ee00, 0x60, {.first_gpr = 28, .gpr_offset = -0x20, .first_fpr = 31});
    Kept<31> constants(c, c.toc(rating_constants));
    Kept<30> manager(c, c.toc(player_stats_manager));
    c.set_f(31, c.f32(constants + 4));
    const Candidate left{c, c.u32(left_slot)}, right{c, c.u32(right_slot)};
    keep(c, {{29, left.staff}, {28, right.staff}});
    c.set(0, c.lr());
    if (compare_logical(c, left.staff) != 0 && read_appearances(c, left, manager, constants, 0x1009ee5c))
        c.set_f(31, c.f(1));
    const bool has_right = compare_logical(c, right.staff) != 0;
    c.set_f(1, c.f32(constants + 4));
    if (has_right) read_appearances(c, right, manager, constants, 0x1009ee8c);
    Fpu(c).fcmp(0, c.f(31), c.f(1));
    if (c.cr(0) & 8) return 1;
    if (c.cr(0) & 4) return 0xffffffff;

    // An unordered comparison follows the same secondary key as a tie.
    const auto key = [&](const Candidate& candidate) -> std::int32_t {
        if (compare_logical(c, candidate.staff) == 0) return 0;
        const auto profile = candidate.profile();
        if (compare_logical(c, profile) == 0) return 0;
        return c.i16(profile + 8);
    };
    const auto left_key = key(left), right_key = key(right);
    c.set(4, std::uint32_t(right_key));
    if (compare(c, left_key, right_key) < 0) return 1;
    c.s.wr<1>(reg::xer_ca, left_key == right_key);
    return left_key == right_key ? 0 : 0xffffffff;
}
}

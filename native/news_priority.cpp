#include "news_detail.hpp"
#include <array>
#include <algorithm>
namespace cm::aot::news {
using namespace detail;
namespace {
struct Priority { unsigned category; std::int32_t type; std::uint32_t value; };
// The original shares category-dispatch tails. Unknown types can skip later
// groups or return immediately; retain those calls and their guest frames.
constexpr auto priorities = std::to_array<Priority>({
    {2, 28, 0},
    {2, 29, 0},
    {2, 30, 0},
    {2, 32, 0},
    {2, 33, 0},
    {2, 34, 0},
    {2, 31, 0},
    {2, 35, 0},
    {2, 36, 0},
    {2, 37, 0},
    {2, 27, 0},
    {2, 42, 0},
    {2, 44, 0},
    {2, 47, 0},
    {2, 46, 0},
    {2, 2, 0},
    {2, 41, 0},
    {2, 43, 0},
    {2, 1, 0},
    {2, 3, 0},
    {2, 4, 0},
    {2, 5, 0},
    {2, 6, 0},
    {2, 7, 1},
    {2, 8, 1},
    {2, 9, 1},
    {2, 10, 1},
    {2, 11, 1},
    {2, 12, 0},
    {2, 13, 0},
    {2, 16, 0},
    {2, 17, 0},
    {2, 18, 0},
    {2, 19, 0},
    {2, 20, 0},
    {2, 38, 0},
    {2, 39, 1},
    {2, 21, 0},
    {2, 22, 0},
    {2, 23, 0},
    {2, 24, 0},
    {2, 25, 0},
    {2, 26, 0},
    {2, 40, 1},
    {7, 1000, 0},
    {7, 1001, 0},
    {3, 2000, 0},
    {3, 2001, 0},
    {3, 2002, 0},
    {3, 2003, 0},
    {6, 6015, 0},
    {6, 6009, 0},
    {6, 6016, 0},
    {6, 6013, 0},
    {6, 6010, 0},
    {6, 6011, 0},
    {6, 6019, 0},
    {6, 6020, 0},
    {6, 6022, 0},
    {6, 6012, 0},
    {6, 6005, 0},
    {6, 6008, 0},
    {6, 6004, 0},
    {6, 6007, 0},
    {6, 6006, 0},
    {6, 6003, 0},
    {6, 6014, 0},
    {6, 6001, 0},
    {6, 6002, 0},
    {6, 6018, 0},
    {6, 6017, 0},
    {6, 6023, 0},
    {6, 6024, 0},
    {6, 6025, 0},
    {6, 6030, 0},
    {6, 6029, 0},
    {6, 6031, 0},
    {6, 6032, 0},
    {6, 6028, 0},
    {6, 6026, 0},
    {6, 3039, 0},
    {4, 3003, 0},
    {4, 3002, 0},
    {4, 3024, 0},
    {4, 3039, 0},
    {4, 3035, 0},
    {4, 3037, 0},
    {4, 3036, 0},
    {4, 3023, 0},
    {4, 3034, 0},
    {4, 3038, 0},
    {4, 3033, 0},
    {4, 3022, 0},
    {4, 3021, 0},
    {4, 3000, 1},
    {4, 3015, 0},
    {4, 3016, 0},
    {4, 3017, 0},
    {4, 3019, 0},
    {4, 3018, 0},
    {4, 3014, 0},
    {4, 3013, 0},
    {4, 3001, 1},
    {4, 3004, 0},
    {4, 3006, 0},
    {4, 3005, 0},
    {4, 3046, 0},
    {4, 3050, 0},
    {4, 3048, 0},
    {4, 3007, 0},
    {4, 3041, 0},
    {4, 3042, 0},
    {4, 3044, 0},
    {4, 3045, 0},
    {4, 3043, 0},
    {4, 3008, 0},
    {4, 3009, 0},
    {4, 3010, 0},
    {4, 3011, 0},
    {4, 3012, 1},
    {4, 3020, 0},
    {4, 3040, 0},
    {5, 4028, 0},
    {5, 4029, 0},
    {5, 4030, 0},
    {5, 4000, 1},
    {5, 4002, 0},
    {5, 4003, 0},
    {5, 4012, 0},
    {5, 4004, 1},
    {5, 4005, 0},
    {5, 4007, 0},
    {5, 4006, 0},
    {5, 4008, 0},
    {5, 4009, 0},
    {5, 4011, 0},
    {5, 4013, 1},
    {5, 4014, 1},
    {5, 4015, 1},
    {5, 4016, 1},
    {5, 4017, 0},
    {5, 4018, 1},
    {5, 4019, 0},
    {5, 4020, 1},
    {5, 4021, 0},
    {5, 4023, 1},
    {5, 4024, 1},
    {5, 4025, 0},
    {5, 4026, 0},
    {5, 4027, 1},
    {5, 4031, 1},
    {5, 4032, 1},
    {5, 4033, 0},
    {5, 4035, 0},
    {1, 5005, 0},
    {1, 5004, 0},
    {1, 5000, 1},
    {1, 5001, 0},
    {1, 5002, 0},
    {1, 5003, 0},
    {1, 5006, 0},
    {1, 5007, 0},
    {1, 5008, 0},
    {1, 5009, 0},
    {1, 5011, 0},
    {1, 5012, 0},
    {1, 5010, 0},
});
struct Group { unsigned category; std::uint32_t back; std::int32_t last_type; unsigned unknown_group; };
constexpr std::array<Group, 7> groups{{
    {2, 0x102ca4dc, 40, 3},
    {7, 0x102ca860, 1001, 3},
    {3, 0x102ca89c, 2003, 3},
    {6, 0x102ca900, 3039, 7},
    {4, 0x102cabb4, 3040, 7},
    {5, 0x102caefc, 4035, 7},
    {1, 0x102cb218, 5010, 7},
}};
}
std::uint32_t priority_for_manager(Cpu& c, std::uint32_t address, std::uint32_t manager) {
    Frame frame(c, 0x102ca450, 0x80, {.first_gpr = 27});
    Kept<29> item(c, address); Kept<30> recipient(c, manager);
    keep(c, {{27, c.toc(slot::message)}, {28, c.toc(slot::pool)}, {31, c.toc(-0x7be8)}});
    if (compare(c, recipient) == 0) {
        assertion(c, frame, 0x102ca48c, 0x40, 0x38, 0x1b50);
        c.set(4, c.toc(slot::success)); return 0;
    }
    unsigned next_group = 0;
    while (next_group < groups.size()) {
        const auto& group = groups[next_group];
        enter(c, group.back, {item}); c.set(3, category(c, item));
        if (compare(c, std::int8_t(c.r(3)), group.category) != 0) { ++next_group; continue; }
        const auto type = Item{c, item}.type().get();
        const auto found = std::ranges::find_if(priorities, [&](const auto& rule) {
            return rule.category == group.category && rule.type == type;
        });
        if (found != priorities.end()) { compare(c, type, type); return found->value; }
        if (group.category == 6 && type == 6000) {
            compare(c, type, 6000);
            const auto first = c.u32(c.toc(slot::first_manager)) - 16;
            const auto subject = c.u32(Item{c, item}.payload());
            leave(c, {{4, subject & 0x80000000 ? 0xffffffffu : 0}, {5, subject}});
            // The original signed >= uses subfc/adde without changing CR0.
            // Its final carry equals the subject's sign bit.
            c.s.wr<1>(reg::xer_ca, subject >> 31);
            return std::int32_t(subject) >= std::int32_t(first);
        }
        if (group.category == 5 && type == 4001) {
            compare(c, type, 4001);
            const auto club = c.u32(c.r(31)) + c.u32(item + 0x1c) * 0x248;
            return c.u32(club + 0x90) == std::uint32_t(recipient);
        }
        if (group.category == 5 && type == 4022) {
            compare(c, type, 4022);
            const auto clubs = c.u32(c.r(31));
            const auto first = clubs + c.u32(item + 0x18) * 0x248;
            const auto second = clubs + c.u32(item + 0x1c) * 0x248;
            leave(c, {{3, second}, {4, clubs}, {5, first}});
            if (compare_logical(c, c.u32(first + 0x90), recipient) == 0) return 1;
            return compare_logical(c, c.u32(second + 0x90), recipient) == 0;
        }
        compare(c, type, group.last_type);
        next_group = group.unknown_group;
    }
    return 0;
}
}

// DISCIPLINE.CPP lifecycle block (discipline_lifecycle.hpp): the creation of the manager's 42 ruling bodies.
#include "discipline_lifecycle.hpp"
#include "discipline_lifecycle_detail.hpp"

namespace cm::aot::discipline {
using guest::Cpu;
using guest::Frame;
namespace callee = lifecycle_callee;

namespace {
constexpr CheckSite bodies_missing{0x100e7d24, 0x40, 0x38, 0x161d};
// The slots 0x00 to 0xa0 in the original's order (the last, 0xa4, is made by hand below): each body's allocation
// and constructor, the competitions whose activity decides it. Slots 0, 4 and 8 always exist; 0x8c waits for
// the super cup check.
constexpr BodySlot slots[] = {
    {0x0, 0x105b7030, 0x24, 0x100e7d70, 0x100e7d80, Condition::always, 0, true},
    {0x4, 0x105b5510, 0x24, 0x100e7d90, 0x100e7da0, Condition::always, 0, true},
    {0x8, 0x105b9a30, 0x24, 0x100e7db0, 0x100e7dc0, Condition::always, 0, true},
    {0xc, 0x105b3040, 0x24, 0x100e7df0, 0x100e7e00, Condition::competition, -0x7508, true},
    {0x10, 0x105b3920, 0x24, 0x100e7e30, 0x100e7e40, Condition::competition, -0x7510, true},
    {0x14, 0x105b3390, 0x24, 0x100e7e50, 0x100e7e60, Condition::competition, -0x7510, false},
    {0x18, 0x105b3ec0, 0x24, 0x100e7e90, 0x100e7ea0, Condition::competition, -0x7b68, true},
    {0x1c, 0x105b43c0, 0x24, 0x100e7eb0, 0x100e7ec0, Condition::competition, -0x7b68, false},
    {0x20, 0x105b48d0, 0x24, 0x100e7ef0, 0x100e7f00, Condition::competition, -0x750c, true},
    {0x24, 0x105b4f20, 0x2c, 0x100e7f30, 0x100e7f40, Condition::competition, -0x7a10, true},
    {0x28, 0x106aaed0, 0x24, 0x100e7f70, 0x100e7f80, Condition::competition, -0x7594, true},
    {0x2c, 0x106aaa10, 0x24, 0x100e7f90, 0x100e7fa0, Condition::competition, -0x7594, false},
    {0x30, 0x105b5820, 0x24, 0x100e7fd0, 0x100e7fe0, Condition::competition, -0x7a00, true},
    {0x34, 0x105b6620, 0x24, 0x100e8010, 0x100e8020, Condition::competition, -0x7af4, true},
    {0x38, 0x105b6130, 0x24, 0x100e8030, 0x100e8040, Condition::competition, -0x7af4, false},
    {0x3c, 0x105b6b10, 0x24, 0x100e8070, 0x100e8080, Condition::competition, -0x7a28, true},
    {0x40, 0x106ab8f0, 0x24, 0x100e80b0, 0x100e80c0, Condition::competition, -0x7574, true},
    {0x44, 0x105b7950, 0x24, 0x100e80f0, 0x100e8100, Condition::competition, -0x7a14, true},
    {0x48, 0x105b73f0, 0x24, 0x100e8110, 0x100e8120, Condition::competition, -0x7a14, false},
    {0x4c, 0x105b8250, 0x24, 0x100e8150, 0x100e8160, Condition::competition, -0x7a24, true},
    {0x50, 0x105b7ef0, 0x24, 0x100e8170, 0x100e8180, Condition::competition, -0x7a24, false},
    {0x54, 0x105b8890, 0x24, 0x100e81b0, 0x100e81c0, Condition::competition, -0x7504, true},
    {0x58, 0x105b85c0, 0x24, 0x100e81d0, 0x100e81e0, Condition::competition, -0x7504, false},
    {0x5c, 0x105b8b90, 0x24, 0x100e8210, 0x100e8220, Condition::competition, -0x79fc, true},
    {0x60, 0x105b93e0, 0x24, 0x100e8250, 0x100e8260, Condition::competition, -0x7a1c, true},
    {0x64, 0x105b90a0, 0x24, 0x100e8270, 0x100e8280, Condition::competition, -0x7a1c, false},
    {0x68, 0x105ba1e0, 0x24, 0x100e82b0, 0x100e82c0, Condition::competition, -0x7a18, true},
    {0x6c, 0x105b9d20, 0x24, 0x100e82d0, 0x100e82e0, Condition::competition, -0x7a18, false},
    {0x70, 0x105babb0, 0x24, 0x100e8310, 0x100e8320, Condition::competition, -0x7514, true},
    {0x74, 0x105ba6c0, 0x24, 0x100e8330, 0x100e8340, Condition::competition, -0x7514, false},
    {0x78, 0x105bb090, 0x24, 0x100e8370, 0x100e8380, Condition::competition, -0x7a20, true},
    {0x7c, 0x106ac6d0, 0x24, 0x100e83b0, 0x100e83c0, Condition::competition, -0x75a8, true},
    {0x80, 0x106ab390, 0x24, 0x100e83f0, 0x100e8400, Condition::competition, -0x7598, true},
    {0x84, 0x106aa4b0, 0x24, 0x100e8430, 0x100e8440, Condition::competition, -0x7590, true},
    {0x88, 0x106aa140, 0x24, 0x100e8470, 0x100e8480, Condition::competition, -0x7650, true},
    {0x8c, 0x106ac3d0, 0x24, 0x100e84b4, 0x100e84c4, Condition::super_cup, 0, true},
    {0x90, 0x106abf60, 0x24, 0x100e84f4, 0x100e8504, Condition::competition, -0x7578, true},
    {0x94, 0x106ad630, 0x24, 0x100e8534, 0x100e8544, Condition::competition, -0x75b0, true},
    {0x98, 0x106ad170, 0x24, 0x100e8554, 0x100e8564, Condition::competition, -0x75b0, false},
    {0x9c, 0x106acc60, 0x24, 0x100e8594, 0x100e85a4, Condition::competition, -0x758c, true},
    {0xa0, 0x106adaf0, 0x24, 0x100e85d4, 0x100e85e4, Condition::competition, -0x757c, true},
};
constexpr std::int32_t super_cup_index = -0x7970, super_cup_table = -0x7520, clock_source = -0x7b74;
constexpr std::int32_t first_clock_cutoff = 0x3e044;
constexpr std::uint32_t competition_size = 0x128, competition_active_byte = 0x120, super_cup_size = 200;
constexpr std::uint32_t last_body = 0xa4;

// Allocates the body's object and constructs it (unless the allocation failed); the slot receives the pointer
// either way. The original reads the array's address from the manager again for the store.
void create_body(Cpu& c, std::uint32_t manager, std::uint32_t offset, std::uint32_t size, std::uint32_t constructor,
                 std::uint32_t allocated, std::uint32_t constructed, std::uint32_t argument) {
    c.set(3, size);
    c.call(callee::object_allocate, allocated);
    const auto object = c.r(3);
    if (object != 0) {
        c.set({{3, object}, {4, argument}});
        c.call(constructor, constructed);
    }
    c.put32(c.u32(manager + manager::bodies) + offset, object);
}
}

void create_bodies(Cpu& c, std::uint32_t manager, std::uint32_t file_argument) {
    Frame frame(c, 0x100e7cd0, 0x80, {.first_gpr = 27});
    c.set({{3, manager::body_count}, {4, 4}});
    c.call(callee::allocate_zeroed, 0x100e7d00);
    c.put32(manager + manager::bodies, c.r(3));
    if (c.u32(manager + manager::bodies) == 0) return report_check(c, frame, bodies_missing);
    bool wanted = true;
    for (const auto& slot : slots) {
        if (slot.starts) {
            switch (slot.condition) {
            case Condition::always: wanted = true; break;
            case Condition::competition: {
                const auto record = c.u32(c.toc(lifecycle_toc::competitions)) + c.u32(c.toc(slot.toc)) * competition_size;
                wanted = c.i8(record + competition_active_byte) != 0;
                break;
            }
            case Condition::super_cup:
                c.set(3, c.u32(c.toc(super_cup_table)) + c.u32(c.toc(super_cup_index)) * super_cup_size);
                c.call(callee::super_cup_check, 0x100e84a4);
                wanted = std::int8_t(c.r(3)) != 0;
                break;
            }
        }
        if (wanted) create_body(c, manager, slot.offset, slot.size, slot.constructor, slot.allocated, slot.constructed, file_argument);
    }
    // The last body is always made; before the cutoff of the clock value its constructor is told there is no file.
    c.set(3, c.u32(c.toc(clock_source)));
    c.call(callee::clock_value, 0x100e85f8);
    if (std::int32_t(c.r(3)) < first_clock_cutoff)
        create_body(c, manager, last_body, 0x24, callee::last_constructor, 0x100e8610, 0x100e8620, 0);
    else
        create_body(c, manager, last_body, 0x24, callee::last_constructor, 0x100e8634, 0x100e8644, file_argument);
}
}

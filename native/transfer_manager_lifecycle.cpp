#include "transfer_manager_lifecycle.hpp"
#include "guest_values.hpp"
#include <array>

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr std::uint32_t release_memory = 0x10775170, destroy_offer = 0x10411f10,
    destroy_shortlist = 0x103567b0, destroy_nation = 0x105c41c0, destroy_list = 0x10738c20,
    destroy_list_array = 0x10773420, delete_manager = 0x107735a0;

void destroy(Cpu& c, std::uint32_t entry, std::uint32_t back, std::uint32_t address) {
    c.set({{3, address}, {4, 1}});
    c.call(entry, back);
}

void release_array(Cpu& c, std::uint32_t manager, std::uint32_t field, std::uint32_t back) {
    const auto array = c.u32(manager + field);
    c.set(3, array);
    if (nonzero_logical(c, array)) c.call(release_memory, back);
}

void destroy_offer_array(Cpu& c, std::uint32_t field, std::uint32_t offer_back, std::uint32_t free_back) {
    const auto array = c.u32(field);
    c.set(0, array);
    if (!nonzero_logical(c, array)) return;
    Kept<28> index(c, 0);
    Kept<29> offset(c, 0);
    for (;; index = index + 1, offset = offset + 4) {
        const auto count = c.u32(field + 4);
        c.set(0, count);
        if (compare(c, index, count) >= 0) break;
        const auto offer = c.u32(c.u32(field) + offset);
        c.set(3, offer);
        if (nonzero_logical(c, offer)) destroy(c, destroy_offer, offer_back, offer);
    }
    c.set(3, c.u32(field));
    c.call(release_memory, free_back);
}
}

std::uint32_t destroy_transfer_manager(Cpu& c, std::uint32_t manager, std::uint32_t delete_flag) {
    Frame frame(c, 0x103df570, 0x50, {.first_gpr = 28});
    const Kept<31> deleting(c, delete_flag);
    const Kept<30> transfers(c, manager);
    if (!nonzero(c, manager)) return result(c, transfers);
    destroy_offer_array(c, transfers, 0x103df5c8, 0x103df5e4);
    destroy_offer_array(c, transfers + 0xc, 0x103df618, 0x103df634);
    release_array(c, transfers, 0x994, 0x103df644);
    const auto shortlist = c.u32(transfers + 0x988);
    c.set(3, shortlist);
    if (nonzero_logical(c, shortlist)) destroy(c, destroy_shortlist, 0x103df658, shortlist);
    struct OwnedArray { std::uint32_t field, back; };
    constexpr std::array before_nations{
        OwnedArray{0x9a0, 0x103df668}, OwnedArray{0xa4c, 0x103df678}, OwnedArray{0x9b4, 0x103df688},
        OwnedArray{0x9c0, 0x103df698}, OwnedArray{0x9cc, 0x103df6a8}, OwnedArray{0xa50, 0x103df6b8},
    };
    for (const auto& [field, back] : before_nations) release_array(c, transfers, field, back);

    Kept<28> index(c, 0);
    Kept<29> offset(c, 0);
    for (;; index = index + 1, offset = offset + 4) {
        c.set(0, std::uint32_t(std::int8_t(index)));
        if (compare(c, std::int8_t(index), 27) >= 0) break;
        const auto field = offset + 0x9e0;
        const auto nation = c.u32(transfers + field);
        c.set({{0, field}, {3, nation}});
        if (nonzero_logical(c, nation)) destroy(c, destroy_nation, 0x103df6e0, nation);
    }
    release_array(c, transfers, 0x9d8, 0x103df704);
    release_array(c, transfers, 0xa6c, 0x103df714);
    const auto pairs = c.u32(transfers + 0xa78);
    c.set(3, pairs);
    if (nonzero_logical(c, pairs)) destroy(c, destroy_list, 0x103df728, pairs);
    c.set({{3, transfers + 0xa5c}, {4, 0xffffffff}});
    c.call(destroy_list, 0x103df734);
    c.set({{3, transfers + 0x9a4}, {4, 0xffffffff}});
    c.call(destroy_list, 0x103df740);
    c.set({{3, transfers + 0x18}, {4, c.toc(-0x6314)}, {5, 16}, {6, 150}});
    c.call(destroy_list_array, 0x103df754);
    const auto flag = std::int16_t(deleting);
    c.set(0, std::uint32_t(flag));
    if (compare(c, flag) > 0) {
        c.set(3, transfers);
        c.call(delete_manager, 0x103df764);
    }
    return result(c, transfers);
}
}

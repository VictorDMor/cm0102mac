#include "award_manager_detail.hpp"

namespace cm::aot::award_manager {
using namespace detail;
namespace {
struct ShortlistRegistry {
    Cpu& c;
    std::uint32_t manager;
    Field<std::uint32_t> lists() const { return {c, manager + 0x14}; }
    Field<std::int16_t> size() const { return {c, manager + 0x18}; }
    Field<std::int16_t> capacity() const { return {c, manager + 0x1a}; }
};
}

std::uint32_t register_shortlist(Cpu& c, std::uint32_t manager, std::uint32_t shortlist) {
    Frame frame(c, 0x10437730, 0x70, {.first_gpr = 28});
    const auto source_pool = c.toc(-0x4a84);
    keep(c, {{28, manager}, {29, shortlist}, {30, c.toc(-0x7ba8)}, {31, source_pool}});
    ShortlistRegistry registry{c, manager};
    if (compare(c, registry.size(), registry.capacity()) >= 0) {
        registry.capacity() = std::int16_t(registry.capacity().get() + 5);
        const auto previous = registry.lists().get();
        const auto bytes = std::uint32_t(registry.capacity().get()) * 4;
        c.set(3, previous);
        if (compare_logical(c, previous) == 0) {
            c.set(3, bytes);
            c.call(0x10775130, 0x1043778c);
        } else {
            c.set(4, bytes);
            c.call(0x107751b0, 0x104377a0);
        }
        registry.lists() = c.r(3);
        if (compare_logical(c, registry.lists()) == 0) {
            assertion(c, frame, 0x104377c4, 0x40, 0x38, 0x391);
            c.set(4, c.toc(-0x7bac));
            return result(c, 0xffffffff);
        }
    }
    const auto index = registry.size().get();
    const auto lists = registry.lists().get();
    leave(c, {{4, extended(index)}, {5, lists}});
    registry.size() = std::int16_t(index + 1);
    c.put32(lists + std::uint32_t(index) * 4, shortlist);
    return result(c, extended(std::int16_t(registry.size().get() - 1)));
}

std::uint32_t has_recent_history(Cpu& c, std::uint32_t manager) {
    if (compare_logical(c, manager) == 0) return result(c, 0);
    const auto history = c.u32(manager + 0x14);
    if (compare_logical(c, history) == 0) return result(c, 0);
    return result(c, compare(c, c.i16(history + 0xa), 2000) > 0);
}

void set_ballot_context(Cpu& c, std::uint32_t manager, std::uint32_t voter,
                        std::uint32_t category, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x10437850, 0x70, {.first_gpr = 30});
    keep(c, {{30, c.toc(-0x7ba8)}, {31, c.toc(-0x4a84)}});
    const auto voter_count = c.toc(-0x7be4);
    const auto base_id = c.u32(voter_count) - 16;
    leave(c, {{4, base_id}, {8, voter_count}});
    const auto index = std::int32_t(c.u32(voter) - base_id);
    if (compare(c, index) < 0 || compare(c, index, 16) > 0) {
        assertion(c, frame, 0x104378a0, 0x40, 0x38, 0x3b3);
        c.set(3, c.toc(-0x7bac));
        return;
    }
    // Index 16 is accepted by the original despite the nominal sixteen voters.
    const auto row = manager + std::uint32_t(index) * 12;
    c.set(3, row);
    c.put32(row + 0x1c, category);
    c.put32(row + 0x20, first);
    c.put32(row + 0x24, second);
}

std::uint32_t ballot_context(Cpu& c, std::uint32_t manager, std::uint32_t voter,
                             std::uint32_t category) {
    Frame frame(c, 0x10437910, 0x70, {.first_gpr = 30});
    keep(c, {{30, c.toc(-0x7ba8)}, {31, c.toc(-0x4a84)}});
    c.set(6, manager);
    if (compare_logical(c, voter) == 0) return result(c, 0);
    const auto base_id = c.u32(c.toc(-0x7be4)) - 16;
    const auto index = std::int32_t(c.u32(voter) - base_id);
    if (compare(c, index) < 0 || compare(c, index, 16) > 0) {
        assertion(c, frame, 0x10437974, 0x40, 0x38, 0x3d8);
        c.set(4, c.toc(-0x7bac));
        return result(c, 0);
    }
    const auto offset = std::uint32_t(index) * 12;
    const auto row = manager + offset + 0x1c;
    c.set(4, offset);
    return result(c, compare_logical(c, c.u32(row), category) == 0 ? row : 0);
}

namespace {
std::uint32_t find_award(Cpu& c, Manager manager, std::uint32_t key, unsigned field) {
    leave(c, {{7, 0}, {10, 0}});
    for (unsigned group = 0; group < award_groups; ++group) {
        const auto groups = manager.groups();
        const auto collection = c.u32(groups + group * 4);
        leave(c, {{8, collection}, {9, groups}});
        if (compare_logical(c, collection) != 0) {
            c.set(6, 0);
            for (unsigned index = 0; compare(c, index, c.i16(collection + 8)) < 0; ++index) {
                const auto award = c.u32(c.u32(collection + 4) + index * 4);
                c.set(5, award);
                if (compare_logical(c, c.u32(award + field), key) == 0)
                    return result(c, c.u32(c.u32(c.u32(groups + group * 4) + 4) + index * 4));
                c.set(6, (index + 1) * 4);
            }
        }
        leave(c, {{7, (group + 1) * 4}, {10, group + 1}});
    }
    return result(c, 0);
}
}

std::uint32_t find_award_by_id(Cpu& c, std::uint32_t manager, std::uint32_t id) {
    return find_award(c, {c, manager}, id, 8);
}

std::uint32_t find_award_by_label(Cpu& c, std::uint32_t manager, std::uint32_t label) {
    return find_award(c, {c, manager}, label, 12);
}

std::uint32_t eligible_for_regional_award(Cpu& c, std::uint32_t club, std::uint32_t person) {
    const auto combined = c.toc(-0x7514);
    const auto england = c.toc(-0x7a00);
    leave(c, {{7, england}, {8, combined}});
    // The club and person paths use different links and compare against
    // different IDs in the first pair of checks.
    const auto club_region = [&](unsigned reg) {
        if (compare_logical(c, club) == 0) return std::uint32_t{0};
        const auto nation = c.u32(club + 0xc);
        c.set(reg, nation);
        return compare_logical(c, nation) == 0 ? 0 : nation;
    };
    const auto person_region = [&](unsigned reg) {
        if (compare_logical(c, person) == 0) return std::uint32_t{0};
        const auto assignment = c.u32(person + 0x14);
        c.set(reg == 3 ? 3 : 5, assignment);
        if (compare_logical(c, assignment) == 0) return std::uint32_t{0};
        const auto nation = c.u32(assignment + 8);
        c.set(reg, nation);
        return compare_logical(c, nation) == 0 ? 0 : nation;
    };
    if (const auto nation = club_region(6)) {
        const auto id = c.toc(-0x750c);
        leave(c, {{5, id}, {6, c.u32(nation + 0x68)}});
        if (compare(c, c.r(6), c.u32(id)) == 0) return result(c, 1);
    }
    if (const auto nation = person_region(6)) {
        const auto id = c.toc(-0x7510);
        leave(c, {{5, id}, {6, c.u32(nation + 0x68)}});
        if (compare(c, c.r(6), c.u32(id)) == 0) return result(c, 1);
    }
    if (const auto nation = club_region(5)) {
        c.set(5, c.u32(nation + 0x68));
        if (compare(c, c.r(5), c.u32(england)) == 0) return result(c, 1);
    }
    if (const auto nation = person_region(5)) {
        c.set(5, c.u32(nation + 0x68));
        if (compare(c, c.r(5), c.u32(england)) == 0) return result(c, 1);
    }
    if (const auto nation = person_region(6)) {
        const auto id = c.toc(-0x7a28);
        leave(c, {{5, id}, {6, c.u32(nation + 0x68)}});
        if (compare(c, c.r(6), c.u32(id)) == 0) return result(c, 1);
    }
    if (const auto nation = club_region(3)) {
        if (compare(c, c.u32(nation + 0x68), c.u32(combined)) == 0) return result(c, 1);
    }
    if (const auto nation = person_region(3)) {
        if (compare(c, c.u32(nation + 0x68), c.u32(combined)) == 0) return result(c, 1);
    }
    return result(c, 0);
}
}

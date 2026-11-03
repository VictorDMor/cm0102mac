#include "award_manager_detail.hpp"

namespace cm::aot::award_manager {
using namespace detail;
namespace {
void update_histories(Cpu& c, std::uint32_t manager, std::uint32_t first,
                      std::uint32_t second, std::uint32_t target, std::uint32_t first_back,
                      std::uint32_t second_back, bool use_category) {
    const auto update = [&](unsigned field, int category, std::uint32_t back) {
        const auto history = c.u32(manager + field);
        c.set(3, history);
        if (compare_logical(c, history) == 0) return;
        c.set(4, first);
        if (use_category) leave(c, {{5, second}, {6, c.toc(category)}});
        c.call(target, back);
    };
    update(0x10, -0x6200, first_back);
    update(0xc, -0x6204, second_back);
}
}

void register_result(Cpu& c, std::uint32_t manager, std::uint32_t participant, std::uint32_t result_record) {
    Frame frame(c, 0x104363e0, 0x50, {.first_gpr = 29});
    keep(c, {{29, manager}, {30, participant}, {31, result_record}});
    c.set(4, 0);
    if (compare(c, c.u8(participant + 0x3a), 1) == 0) return;
    std::uint32_t nation = 0;
    if (compare_logical(c, participant) != 0) {
        const auto assignment = c.u32(participant + 0x14);
        c.set(3, assignment);
        if (compare_logical(c, assignment) != 0) {
            const auto associated_nation = c.u32(assignment + 8);
            if (compare_logical(c, associated_nation) != 0) nation = associated_nation;
        }
    }
    c.set(4, nation);
    const auto groups = Manager{c, manager}.groups();
    for (unsigned index = 0; index < award_groups; ++index) {
        c.s.wr<4>(reg::ctr, award_groups - index);
        const auto group = c.u32(groups + index * 4);
        leave(c, {{3, group}, {5, groups + index * 4}});
        if (compare_logical(c, group) != 0 && compare_logical(c, nation, c.u32(group + 0x10)) == 0) {
            leave(c, {{4, participant}, {5, result_record}});
            virtual_call(c, group, 0x10, 0x10436474);
            break;
        }
        c.set(5, groups + (index + 1) * 4);
        c.s.wr<4>(reg::ctr, award_groups - index - 1);
    }
    update_histories(c, manager, participant, result_record, 0x1044e5a0, 0x104364a4, 0x104364c0, true);
}

void advance_date(Cpu& c, std::uint32_t manager, std::uint32_t date) {
    Frame frame(c, 0x104364e0, 0x50, {.first_gpr = 28});
    const auto groups = Manager{c, manager}.groups();
    keep(c, {{28, manager}, {29, date}, {30, 0}, {31, groups}});
    for (unsigned index = 0; compare(c, index, award_groups) < 0; ++index) {
        const auto group = c.u32(groups + index * 4);
        c.set(3, group);
        if (compare_logical(c, group) != 0) {
            c.set(4, date);
            virtual_call(c, group, 0xc, 0x1043652c);
        }
        keep(c, {{30, index + 1}, {31, groups + (index + 1) * 4}});
    }
    if (compare(c, c.u32(manager + 0xdc)) != 0) {
        call(c, 0x10778300, 0x10436560, {manager + 0x1c, 0, 0xc0});
        c.put32(manager + 0xdc, 0);
    }
    update_histories(c, manager, date, 0, 0x1044ec10, 0x1043657c, 0x10436590, false);
}

namespace {
void read_history(Cpu& c, std::uint32_t manager, unsigned field,
                  std::uint32_t entry, std::uint32_t back, std::uint32_t key,
                  std::uint32_t output, std::uint32_t limit) {
    Frame frame(c, entry, 0x40, {});
    call(c, 0x10748ce0, back, {c.u32(manager + field), output, limit, key});
}

void reset_history(Cpu& c, std::uint32_t manager, unsigned field, std::uint32_t entry, std::uint32_t back) {
    Frame frame(c, entry, 0x40, {});
    call(c, 0x10749fd0, back, {c.u32(manager + field)});
}
}

void read_player_history(Cpu& c, std::uint32_t manager, std::uint32_t key,
                         std::uint32_t output, std::uint32_t limit) {
    read_history(c, manager, 0, 0x104370f0, 0x10437114, key, output, limit);
}

void reset_player_history(Cpu& c, std::uint32_t manager) {
    reset_history(c, manager, 0, 0x10437130, 0x10437144);
}

void read_team_history(Cpu& c, std::uint32_t manager, std::uint32_t key,
                       std::uint32_t output, std::uint32_t limit) {
    read_history(c, manager, 4, 0x10437160, 0x10437184, key, output, limit);
}

void reset_team_history(Cpu& c, std::uint32_t manager) {
    reset_history(c, manager, 4, 0x104371a0, 0x104371b4);
}

std::uint32_t find_national_group(Cpu& c, std::uint32_t manager, std::uint32_t nation) {
    Frame frame(c, 0x104371d0, 0x70, {.first_gpr = 30});
    keep(c, {{30, c.toc(-0x7ba8)}, {31, c.toc(-0x4a84)}});
    if (compare_logical(c, nation) == 0) {
        assertion(c, frame, 0x10437208, 0x40, 0x38, 0x2aa);
        c.set(4, c.toc(-0x7bac));
        return result(c, 0);
    }
    leave(c, {{6, 0}, {7, 0}});
    Manager object{c, manager};
    for (unsigned index = 0; compare(c, index, award_groups) < 0; ++index) {
        const auto group = object.group(index);
        c.set(5, group);
        if (compare_logical(c, group) != 0 && compare(c, c.u8(group + 0x14), 1) == 0 &&
            compare_logical(c, c.u32(group + 0x10), nation) == 0) return result(c, group);
        leave(c, {{6, (index + 1) * 4}, {7, index + 1}});
    }
    return result(c, 0);
}

void award_news_subjects(Cpu& c, std::uint32_t manager, unsigned group, unsigned award_index,
                         std::uint32_t message, std::uint32_t subject, std::uint32_t destination) {
    Frame frame(c, 0x104372c0, 0x40, {});
    const auto award = c.u32(c.u32(Manager{c, manager}.group(group) + 4) + 4 * award_index);
    leave(c, {{3, award}, {4, message}, {5, subject}, {6, destination}});
    virtual_call(c, award, 0x14, 0x104372fc);
}
}

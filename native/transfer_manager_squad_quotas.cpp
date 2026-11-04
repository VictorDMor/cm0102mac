#include "transfer_manager_squad_quotas.hpp"
#include "guest_values.hpp"
#include "transfer_manager_adapter.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr auto category_entry = 0x103fdae0, read_entry = 0x103f9550, select_entry = 0x103f97b0;
constexpr std::uint32_t nation_table = 0x9e0, national_records = 27, bridge = 0x10781320, fill = 0x10778300;

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned reg = 3;
    for (const auto arg : args) c.set(reg++, arg);
    c.call(entry, back);
    return c.r(3);
}

bool signed_result(Cpu& c, std::uint32_t value) {
    const auto byte = std::int8_t(value);
    c.set(0, std::uint32_t(byte));
    return nonzero(c, byte);
}

void classify(Cpu& c, const Frame& frame, std::uint32_t manager, std::uint32_t competition, std::uint32_t back) {
    const auto kind = frame.at(0x38), nation = frame.at(0x40), group = frame.at(0x3c);
    c.set({{3, manager}, {4, competition}, {5, kind}, {6, nation}, {7, group}});
    if (!transfer_manager_direct_calls_enabled()) {
        c.call(category_entry, back);
        return;
    }
    c.link(back);
    c.s.pc = category_entry;
    resolve_squad_quota_category(c, competition, kind, nation, group);
    c.finish();
}

std::int32_t quota_kind(Cpu& c, const Frame& frame) {
    const auto raw = c.u8(frame.at(0x38));
    const auto kind = std::int8_t(raw);
    c.set({{0, raw}, {6, std::uint32_t(kind)}});
    compare(c, kind, -1);
    return kind;
}

struct Rule {
    std::uint32_t entry = 0;
    bool by_group = false;
};

Rule find_rule(Cpu& c, const Frame& frame, std::uint32_t manager, std::int32_t kind, bool stop_if_unrestricted) {
    const auto nation = c.u32(frame.at(0x40)), group = c.u32(frame.at(0x3c));
    auto cursor = manager + nation_table;
    c.set({{0, national_records}, {4, nation}, {5, group}, {7, cursor}});
    c.s.wr<4>(reg::ctr, national_records);
    for (auto remaining = national_records; remaining; --remaining, cursor += 4) {
        c.set(7, cursor);
        if (compare(c, kind, 4) == 0) {
            if (stop_if_unrestricted) return {};
        } else {
            const auto grouped = compare(c, kind, 2) == 0;
            const auto entry = result(c, c.u32(cursor));
            const auto value = c.u32(entry + (grouped ? 0x10 : 0xc));
            c.set(0, value);
            if (compare_logical(c, value, grouped ? group : nation) == 0) {
                const auto rule_kind = c.i8(entry + 0x14);
                c.set(0, std::uint32_t(rule_kind));
                if (compare(c, kind, rule_kind) == 0) return {entry, grouped};
            }
        }
        c.s.wr<4>(reg::ctr, remaining - 1);
    }
    c.set(7, cursor);
    return {};
}

void invoke(Cpu& c, const Frame& frame, std::uint32_t rule, std::uint32_t method, std::uint32_t back,
            std::uint32_t second, std::uint32_t third) {
    const auto descriptor = c.u32(c.u32(rule) + method);
    c.set({{3, rule}, {4, second}, {5, third}, {12, descriptor}});
    c.call(bridge, back);
    c.set(2, c.u32(frame.at(0x14)));
}

bool present_byte(Cpu& c, std::uint32_t field) {
    const auto value = c.i8(field);
    c.set(0, std::uint32_t(value));
    return compare(c, value, -1) != 0;
}

std::uint32_t preferred_limit(Cpu& c, std::uint32_t limits, unsigned preferred, unsigned fallback) {
    const auto highest = c.u8(limits + preferred);
    c.set(3, highest);
    if (present_byte(c, limits + preferred)) return highest;
    const auto alternative = c.u8(limits + fallback);
    c.set(3, alternative);
    return present_byte(c, limits + fallback) ? alternative : 0xffffffffu;
}

void reduce_quota(Cpu& c, std::uint32_t limits) {
    Kept<30> nation_limit(c, 0xffffffff);
    Kept<29> group_limit(c, 0xffffffff);
    nation_limit = preferred_limit(c, limits, 5, 3);
    c.set(0, std::uint32_t(std::int8_t(std::uint32_t(nation_limit))));
    if (compare(c, std::int8_t(std::uint32_t(nation_limit)), -1) != 0) {
        call(c, fill, 0x103f96c0, {limits, 0xffffffff, 0x2c});
        c.put8(limits + 5, nation_limit);
        return;
    }
    group_limit = preferred_limit(c, limits, 2, 0);
    c.set(0, std::uint32_t(std::int8_t(std::uint32_t(group_limit))));
    if (compare(c, std::int8_t(std::uint32_t(group_limit)), -1) != 0) {
        call(c, fill, 0x103f9710, {limits, 0xffffffff, 0x2c});
        c.put8(limits + 2, group_limit);
        return;
    }
    nation_limit = c.u8(limits + 0x16);
    if (present_byte(c, limits + 0x16)) {
        group_limit = c.u8(limits + 0x17);
        call(c, fill, 0x103f973c, {limits, 0xffffffff, 0x2c});
        c.put8(limits + 0x16, nation_limit);
        c.put8(limits + 0x17, group_limit);
    } else {
        call(c, fill, 0x103f9758, {limits, 0xffffffff, 0x2c});
    }
}
}

void resolve_squad_quota_category(Cpu& c, std::uint32_t competition, std::uint32_t kind,
                                  std::uint32_t nation, std::uint32_t group) {
    Frame frame(c, category_entry, 0x50, {.first_gpr = 28});
    c.set(0, c.lr());
    const Kept<28> contest(c, competition);
    const Kept<29> out_nation(c, nation);
    const Kept<30> out_kind(c, kind);
    const Kept<31> out_group(c, group);
    const auto league = c.u32(contest + 0x14);
    const auto league_id = c.u32(league), domestic = c.u32(c.toc(-0x7aa8));
    c.set({{0, domestic}, {3, league_id}, {4, league}});
    if (compare(c, league_id, domestic) == 0) {
        const auto member = c.u32(c.u32(contest + 0x1c) + 0x2c);
        c.put32(out_nation, member);
        c.set(3, member);
        const auto region = nonzero_logical(c, member) ? c.u32(member + 0x74) : 0;
        c.put32(out_group, region);
        c.set(0, 1);
        c.put8(out_kind, 1);
        return;
    }
    const auto disabled = call(c, 0x10494d20, 0x103fdb64, {c.u32(c.toc(-0x7b54))});
    if (signed_result(c, disabled)) {
        c.set({{0, 0xffffffff}, {3, 0}});
        c.put32(out_nation, 0);
        c.put32(out_group, 0);
        c.put8(out_kind, 0xff);
        return;
    }
    const auto raw = call(c, 0x10192130, 0x103fdb8c, {c.u32(contest + 0x14)});
    const auto region_index = std::int8_t(raw);
    const auto nation_table = c.toc(-0x7b44);
    c.set(5, std::uint32_t(region_index));
    const auto national = compare(c, region_index, 1) == 0;
    c.put32(out_nation, c.u32(nation_table + std::uint32_t(region_index) * 0x4c));
    const auto association = c.u32(contest + 0x14);
    const auto identifier = c.u32(association + 4);
    c.set({{0, identifier}, {4, association}});
    c.put32(out_group, identifier);
    std::uint32_t mode = 4;
    if (!national && signed_result(c, raw)) {
        const auto distance = std::uint8_t(raw - 3);
        c.set(0, distance);
        mode = compare_logical(c, distance, 3) <= 0 || compare(c, region_index, 7) == 0 ? 2 : 1;
    }
    c.set(0, mode);
    c.put8(out_kind, mode);
}

std::uint32_t team_squad_quota_status(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t competition) {
    Frame frame(c, 0x103f93e0, 0x60, {.first_gpr = 29});
    c.set(0, c.lr());
    const Kept<29> transfers(c, manager);
    const Kept<30> team(c, club);
    const Kept<31> contest(c, competition);
    if (nonzero(c, contest) && !signed_result(c, call(c, 0x100c7d00, 0x103f9410, {c.u32(contest + 0x1c)})) &&
        signed_result(c, call(c, 0x10003dc0, 0x103f942c, {c.u32(c.toc(-0x7b24)), team, 0})))
        return result(c, 9);
    classify(c, frame, transfers, contest, 0x103f9454);
    const auto kind = quota_kind(c, frame);
    if (kind == -1) return result(c, 0);
    const auto rule = find_rule(c, frame, transfers, kind, true);
    if (!rule.entry) return result(c, 0);
    invoke(c, frame, rule.entry, 0x20, rule.by_group ? 0x103f94d0 : 0x103f9510, team, contest);
    return c.r(3);
}

void read_squad_quota_limits(Cpu& c, std::uint32_t manager, std::uint32_t competition, std::uint32_t limits,
                             std::uint32_t reduce) {
    Frame frame(c, read_entry, 0x60, {.first_gpr = 28});
    c.set(0, c.lr());
    const Kept<28> transfers(c, manager);
    const Kept<29> reduce_limits(c, reduce);
    const Kept<30> contest(c, competition);
    const Kept<31> out(c, limits);
    call(c, fill, 0x103f958c, {out, 0xffffffff, 0x2c});
    if (!nonzero_logical(c, contest)) return;
    classify(c, frame, transfers, contest, 0x103f95ac);
    const auto kind = quota_kind(c, frame);
    if (kind == -1) return;
    const auto rule = find_rule(c, frame, transfers, kind, false);
    if (rule.entry) invoke(c, frame, rule.entry, 0x24, rule.by_group ? 0x103f9614 : 0x103f9654, contest, out);
    if (signed_result(c, reduce_limits)) reduce_quota(c, out);
}

void select_squad_quota_limit(Cpu& c, std::uint32_t manager, std::uint32_t competition, std::uint32_t limit) {
    Frame frame(c, select_entry, 0x80, {.first_gpr = 29});
    c.set(0, c.lr());
    const Kept<29> transfers(c, manager);
    const Kept<30> contest(c, competition);
    const Kept<31> out(c, limit);
    call(c, fill, 0x103f97e4, {out, 0, 6});
    const auto limits = frame.at(0x38);
    c.set({{3, transfers}, {4, contest}, {5, limits}, {6, 1}});
    if (transfer_manager_direct_calls_enabled()) {
        c.link(0x103f97f8);
        c.s.pc = read_entry;
        read_squad_quota_limits(c, transfers, contest, limits, 1);
        c.finish();
    } else {
        c.call(read_entry, 0x103f97f8);
    }
    for (auto field : {5u, 3u, 2u, 0u}) {
        if (!present_byte(c, limits + field)) continue;
        c.put8(out, field >= 3 ? 1 : 2);
        const auto maximum = c.u8(limits + field);
        c.set(0, maximum);
        c.put8(out + 5, maximum);
        return;
    }
    if (!present_byte(c, limits + 0x16)) return;
    c.put8(out, 3);
    c.put8(out + 1, c.u8(limits + 0x16));
    const auto maximum = c.u8(limits + 0x17);
    c.set(0, maximum);
    c.put8(out + 5, maximum);
}

namespace {
struct SquadQuota {
    Cpu& c;
    const Frame& frame;
    std::uint32_t summary() const { return frame.at(0x38); }
    std::int32_t field(unsigned offset) const { return c.i8(summary() + offset); }
    std::uint32_t raw(unsigned offset) const { return c.u8(summary() + offset); }

    void increment(unsigned field) const {
        const auto value = raw(field);
        c.set({{0, value + 1}, {3, value}});
        c.put8(summary() + field, value + 1);
    }

    void publish(std::uint32_t counts) const {
        if (!nonzero_logical(c, counts)) return;
        for (unsigned i = 0; i < 6; ++i) {
            const auto value = raw(i);
            c.set(0, value);
            c.put8(counts + i, value);
        }
    }

    void describe_national(std::uint32_t text, std::uint32_t pool, bool by_group,
                           std::int32_t total, std::int32_t maximum) const {
        const auto emit = [&](std::uint32_t national, std::uint32_t group, std::uint32_t back) {
            call(c, 0x10727890, back + (by_group ? 0xb8 : 0), {text, pool + (by_group ? group : national)});
        };
        if (compare(c, total, 1) > 0) {
            const auto limit = compare(c, maximum, 1);
            if (limit > 0) emit(0x1c2a, 0x1e2d, 0x103f9b5c);
            else if (limit == 0) emit(0x1ca8, 0x1ead, 0x103f9b70);
            else emit(0x1d12, 0x1f19, 0x103f9b80);
        } else if (compare(c, maximum, 1) == 0) {
            emit(0x1d77, 0x1f80, 0x103f9b98);
        } else if (!signed_result(c, raw(5))) {
            compare(c, total, 1);
            emit(0x1dde, 0x1fe9, 0x103f9bc8);
        }
    }

    void describe_age(std::uint32_t text, std::uint32_t pool, std::int32_t total) const {
        const auto minimum_age = std::uint32_t(field(1));
        if (compare(c, total, 1) > 0)
            call(c, 0x10727890, 0x103f9ccc, {text, pool + 0x203a, std::uint32_t(total), minimum_age});
        else
            call(c, 0x10727890, 0x103f9ce4, {text, pool + 0x2098, minimum_age});
        const Kept<22> raw_limit(c, raw(5));
        const auto limit = std::int8_t(std::uint32_t(raw_limit));
        c.set(0, std::uint32_t(limit));
        const auto comparison = compare(c, limit, 1);
        if (comparison > 0) {
            const auto length = call(c, 0x1077dbd0, 0x103f9cfc, {text});
            call(c, 0x10727890, 0x103f9d0c, {text + length, pool + 0x20e1, std::uint32_t(limit)});
        } else if (comparison == 0) {
            const auto length = call(c, 0x1077dbd0, 0x103f9d1c, {text});
            call(c, 0x10727890, 0x103f9d28, {text + length, pool + 0x2111});
        } else {
            const auto length = call(c, 0x1077dbd0, 0x103f9d34, {text});
            call(c, 0x10727890, 0x103f9d40, {text + length, pool + 0x212c});
        }
    }

    std::uint32_t violates(std::uint32_t text, std::uint32_t pool) const {
        const auto kind = field(0);
        c.set(0, std::uint32_t(kind));
        const bool national = compare(c, kind, 1) == 0;
        const bool grouped = !national && compare(c, kind, 2) == 0;
        if (national || grouped) {
            const auto total = field(4), maximum = field(5);
            c.set({{0, raw(4)}, {3, raw(5)}, {5, std::uint32_t(total)}, {6, std::uint32_t(maximum)}});
            if (compare(c, total, maximum) <= 0) return result(c, 0);
            if (nonzero_logical(c, text)) describe_national(text, pool, grouped, total, maximum);
            return result(c, 1);
        }
        if (compare(c, kind, 3) != 0) return result(c, 0);
        const auto total = field(4), maximum = field(5);
        c.set({{0, std::uint32_t(maximum)}, {3, raw(4)}, {5, std::uint32_t(total)}});
        if (compare(c, total, maximum) <= 0) return result(c, 0);
        if (nonzero_logical(c, text)) describe_age(text, pool, total);
        return result(c, 1);
    }
};

bool managed_team(Cpu& c, std::uint32_t competition, std::uint32_t club) {
    const auto selected = [&](unsigned club_field, unsigned flag) {
        const auto pointer = c.u32(competition + club_field);
        c.set(0, pointer);
        return compare_logical(c, club, pointer) == 0 && !signed_result(c, c.u8(competition + flag));
    };
    return selected(0x1c, 0x40) || selected(0x20, 0x41);
}

bool counts_toward_quota(Cpu& c, const SquadQuota& quota, std::uint32_t club, std::uint32_t person) {
    const auto kind = quota.field(0);
    c.set(0, std::uint32_t(kind));
    if (compare(c, kind, 2) == 0)
        return signed_result(c, call(c, 0x100cab10, 0x103f99b4, {person, c.u32(club + 0x2c), 1}));
    if (compare(c, kind, 1) == 0) {
        if (!signed_result(c, call(c, 0x100cab10, 0x103f9a0c, {person, c.u32(club + 0x2c), 1}))) return false;
        return !signed_result(c, call(c, 0x100ca6a0, 0x103f9a20, {person, c.u32(club + 0x2c)}));
    }
    if (compare(c, kind, 3) != 0) return false;
    const auto age = c.i8(person + 0x5d);
    const auto limit = quota.field(1);
    c.set({{0, std::uint32_t(limit)}, {3, std::uint32_t(age)}});
    return compare(c, age, limit) > 0;
}
}

std::uint32_t squad_violates_quota(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t competition,
                                   std::uint32_t message, std::uint32_t lineup, std::uint32_t counts) {
    Frame frame(c, 0x103f98d0, 0x70, {.first_gpr = 21});
    c.set(0, c.lr());
    const Kept<23> team(c, club);
    const Kept<24> contest(c, competition);
    const Kept<25> text(c, message);
    const Kept<26> provided(c, lineup);
    const Kept<27> out_counts(c, counts);
    const Kept<30> teams(c, c.toc(-0x7a6c));
    const Kept<31> pool(c, c.toc(-0x4b9c));
    Kept<29> current_team(c, 0);
    const SquadQuota quota{c, frame};
    c.set({{3, manager}, {4, contest}, {5, quota.summary()}});
    if (transfer_manager_direct_calls_enabled()) {
        c.link(0x103f990c);
        c.s.pc = select_entry;
        select_squad_quota_limit(c, manager, contest, quota.summary());
        c.finish();
    } else {
        c.call(select_entry, 0x103f990c);
    }
    if (!signed_result(c, quota.raw(0))) return result(c, 0);
    if (managed_team(c, contest, team)) current_team = 1;
    Kept<22> row(c, provided);
    for (std::uint32_t index = 0; ; ++index, row = row + 4) {
        const Kept<28> slot(c, index);
        const auto substitutes = c.u8(contest + 0x4e) >> 4;
        const auto size = substitutes + 11;
        c.set({{0, std::uint32_t(size)}, {3, std::uint32_t(substitutes)}, {4, slot}});
        if (compare(c, index, size) >= 0) break;
        std::uint32_t person;
        if (!nonzero_logical(c, provided)) {
            person = call(c, 0x103af800, 0x103f997c, {c.u32(teams), team, slot, current_team});
            c.set(21, person);
            if (!nonzero(c, std::int32_t(person))) continue;
        } else {
            person = c.u32(row);
            c.set(21, person);
            if (!nonzero_logical(c, person)) continue;
        }
        if (!counts_toward_quota(c, quota, team, person)) continue;
        c.set(0, slot);
        quota.increment(compare(c, index, 11) < 0 ? 2 : 3);
        quota.increment(4);
    }
    quota.publish(out_counts);
    return quota.violates(text, pool);
}
}

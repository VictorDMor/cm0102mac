#include "transfer_manager_nation_rules.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr std::uint32_t nation_records = 0x9e0, nation_count = 27, club_size = 0x248, call_vector = 0x10781320;

std::uint32_t scan_nations(Cpu& c, std::uint32_t manager, std::uint32_t nation,
                           unsigned cursor_reg, unsigned index_reg, unsigned record_reg) {
    auto cursor = manager + nation_records;
    for (unsigned index = 0; ; ++index, cursor += 4) {
        c.set({{0, index}, {cursor_reg, cursor}, {index_reg, index}});
        if (compare(c, index, nation_count) >= 0) return 0;
        const auto record = c.u32(cursor);
        const auto country = c.u32(record + 0xc);
        c.set({{0, country}, {record_reg, record}});
        if (compare_logical(c, country, nation) == 0) return record;
    }
}

void invoke_rule(Cpu& c, const Frame& frame, std::uint32_t record, std::uint32_t method, std::uint32_t back) {
    const auto vtable = c.u32(record);
    c.set(12, c.u32(vtable + method));
    c.call(call_vector, back);
    c.set(2, c.u32(frame.at(0x14)));
}

void validate_offer(Cpu& c, const Frame& frame, std::uint32_t record, std::uint32_t club, std::uint32_t person,
                    std::uint32_t offer, std::uint32_t context, std::uint32_t back) {
    c.set({{3, record}, {4, club}, {5, person}, {6, offer}, {7, context}, {8, frame.at(0x38)}});
    invoke_rule(c, frame, record, 0x14, back);
}

void copy_reason(Cpu& c, const Frame& frame, std::uint32_t out) {
    if (!nonzero_logical(c, out)) return;
    const auto reason = c.i16(frame.at(0x38));
    c.set(0, std::uint32_t(reason));
    c.put16(out, std::uint32_t(reason));
}

bool offer_retry_allowed(Cpu& c, std::int32_t reason) {
    const auto delta = std::uint16_t(reason - 20);
    c.set({{0, delta}, {3, std::uint32_t(reason)}});
    return compare_logical(c, delta, 1) <= 0 || compare(c, reason, 31) == 0 || compare(c, reason, 17) == 0;
}

bool successful(Cpu& c, std::uint32_t raw) {
    const auto status = std::int8_t(raw);
    c.set(0, std::uint32_t(status));
    return nonzero(c, status);
}
}

std::uint32_t nation_allows_staff_transfer(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t person) {
    Frame frame(c, 0x103fa850, 0x40, {});
    c.set(0, c.lr());
    const auto employer = c.u32(person + 0x24);
    c.set(9, employer);
    if (!nonzero_logical(c, employer)) return result(c, 1);
    const auto nation = c.u32(club + 0x2c), current_nation = c.u32(employer + 0x2c);
    c.set({{0, current_nation}, {10, nation}});
    if (compare_logical(c, nation, current_nation) != 0) return result(c, 1);
    auto cursor = manager + nation_records;
    c.set(0, nation_count);
    c.s.wr<4>(reg::ctr, nation_count);
    for (unsigned remaining = nation_count; remaining; --remaining, cursor += 4) {
        const auto record = c.u32(cursor), country = c.u32(record + 0xc);
        c.set({{0, country}, {3, record}, {9, cursor}});
        if (compare_logical(c, country, nation) == 0) {
            invoke_rule(c, frame, record, 0x2c, 0x103fa8ac);
            return c.r(3);
        }
        c.s.wr<4>(reg::ctr, remaining - 1);
    }
    c.set(9, cursor);
    return result(c, 1);
}

std::uint32_t check_nation_offer_rules(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t context,
                                      std::uint32_t person, std::uint32_t offer, std::uint32_t retry, std::uint32_t reason) {
    Frame frame(c, 0x103f8820, 0x60, {.first_gpr = 25});
    c.set(0, c.lr());
    Kept<25> retry_flag(c, retry);
    const Kept<26> destination(c, club);
    const Kept<27> options(c, context);
    const Kept<28> staff(c, person);
    const Kept<29> proposed(c, offer);
    const Kept<30> output(c, reason);
    const auto country = c.u32(destination + 0x2c);
    c.set(4, country);
    const Kept<31> rules(c, scan_nations(c, manager, country, 3, 5, 31));
    if (nonzero_logical(c, output)) {
        c.set(0, 0);
        c.put16(output, 0);
    }
    if (!nonzero_logical(c, rules)) return result(c, 1);
    validate_offer(c, frame, rules, destination, staff, proposed, options, 0x103f88c4);
    copy_reason(c, frame, output);
    if (successful(c, c.r(3)) || !successful(c, retry_flag)) return c.r(3);

    c.set({{3, destination}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x103f8904);
    if (!successful(c, c.r(3))) {
        copy_reason(c, frame, output);
        return result(c, 0);
    }
    const auto rejected = c.i16(frame.at(0x38));
    if (!offer_retry_allowed(c, rejected)) {
        if (nonzero_logical(c, output)) c.put16(output, std::uint32_t(rejected));
        return result(c, 0);
    }

    const Field<std::uint32_t> staff_nation(c, staff + 0xc);
    retry_flag = staff_nation;
    const auto destination_nation = c.u32(destination + 0x2c);
    c.set(0, destination_nation);
    staff_nation = destination_nation;
    validate_offer(c, frame, rules, destination, staff, proposed, options, 0x103f8960);
    const auto accepted = successful(c, c.r(3));
    staff_nation = retry_flag;
    if (accepted) return result(c, 1);
    copy_reason(c, frame, output);
    return result(c, 0);
}

void dispatch_nation_offer_notification(Cpu& c, std::uint32_t manager, std::uint32_t offer) {
    Frame frame(c, 0x103faba0, 0x40, {});
    c.set(0, c.lr());
    const auto offset = c.u32(offer + 8) * club_size;
    const auto nation = c.u32(c.u32(c.toc(-0x7be8)) + offset + 0x2c);
    c.set({{3, offset}, {5, nation}});
    const auto rules = result(c, scan_nations(c, manager, nation, 6, 7, 3));
    if (!nonzero_logical(c, rules)) return;
    if (successful(c, c.u8(offer + 0x24))) invoke_rule(c, frame, rules, 0x10, 0x103fac34);
    else invoke_rule(c, frame, rules, 0xc, 0x103fac20);
}

void dispatch_club_nation_rule(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t subject,
                               std::uint32_t output) {
    Frame frame(c, 0x1040c1a0, 0x50, {.first_gpr = 28});
    c.set(0, c.lr());
    const Kept<28> transfers(c, manager);
    const Kept<29> destination(c, club);
    const Kept<30> input(c, subject);
    const Kept<31> out(c, output);
    c.set({{3, destination}, {4, 0}});
    c.call(0x102cb570, 0x1040c1d8);
    const auto offset = c.r(3) * 4;
    const auto rules = c.u32(transfers + nation_records + offset);
    c.set({{0, offset}, {3, rules}, {4, destination}, {5, input}, {6, out}});
    invoke_rule(c, frame, rules, 0x38, 0x1040c1fc);
}
}

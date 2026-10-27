#include "contract_manager_detail.hpp"
#include <array>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
template<unsigned Register>
void person_link(Cpu& c, Program& program, const Frame& frame, Kept<Register>& person,
                 std::uint32_t staff, std::uint32_t output, std::uint32_t first,
                 std::uint32_t second, std::uint32_t third, unsigned name_offset,
                 std::uint32_t check_back, std::uint32_t link_back) {
    person = std::uint32_t(person) * Staff::size;
    const auto records = c.u32(staff); c.set(0, records);
    const auto available = std::int8_t(program.call(0x100cbde0, check_back,
        {records + std::uint32_t(person), first, second, third, 0}));
    c.set(0, std::uint32_t(std::int32_t(available)));
    if (nonzero(c, available)) {
        const auto current = c.u32(staff); c.set(0, current);
        program.call(0x1074bab0, link_back,
            {output, frame.at(name_offset), 0x7d7, current + std::uint32_t(person)});
    }
}
}

void render_cancelled_offer(Cpu& c, std::uint32_t message_address, std::uint32_t mode,
                            std::uint32_t output_address) {
    c.set(0, c.lr());
    Frame frame(c, 0x1008c090, 0x190, {.first_gpr = 22});
    Kept<27> message(c, message_address); Kept<22> clubs(c, c.toc(-0x7be8));
    Kept<23> buffer(c, c.toc(-0x7bb8)); Kept<25> club(c, mode);
    Kept<31> staff(c, c.toc(-0x7be0)); Kept<26> output(c, output_address);
    Kept<24> pool(c, c.toc(slot::pool)); Program program(c);
    const auto person_id = program.call(0x102cb570, 0x1008c0c8, {message, 0});
    c.set(0, person_id); Kept<30> person(c, person_id);
    const auto first_value = program.call(0x102cb570, 0x1008c0dc, {message, 1});
    c.set(0, first_value); Kept<29> first(c, first_value);
    const auto second_value = program.call(0x102cb570, 0x1008c0f0, {message, 2});
    c.set(0, second_value); Kept<28> second(c, second_value);
    const auto third_value = program.call(0x102cb570, 0x1008c104, {message, 3});
    c.set(0, third_value); message = third_value;
    const auto club_id = program.call(0x102cb570, 0x1008c118, {message_address, 4});
    const auto detailed = std::int8_t(mode); c.set(0, std::uint32_t(std::int32_t(detailed)));
    const bool full = nonzero(c, detailed); club = club_id;
    program.call(0x100cb590, full ? 0x1008c144 : 0x1008c1fc,
        {first, second, message, frame.at(0x38), 0x12c, full ? 1u : 5u, person});
    if (full) {
        club = std::uint32_t(club) * 0x248;
        const auto records = c.u32(clubs); c.set(0, records);
        const auto gender = c.i8(records + std::uint32_t(club) + 0x23b);
        c.set(7, records + std::uint32_t(club) + 0x221);
        program.call(0x10727890, 0x1008c174,
            {buffer, pool + 0x241a, std::uint32_t(std::int32_t(gender)),
             std::uint32_t(std::int32_t(gender)), records + std::uint32_t(club) + 0x221, frame.at(0x38)});
        program.call(0x1074b830, 0x1008c180, {output, buffer});
        const auto current = c.u32(clubs); c.set(0, current);
        program.call(0x1074bab0, 0x1008c198,
            {output, current + std::uint32_t(club) + 0x221, 0x7d5, current + std::uint32_t(club)});
    } else {
        program.call(0x10727890, 0x1008c210, {buffer, pool + 0x24b4, 0x64, frame.at(0x38)});
        program.call(0x1074b830, 0x1008c21c, {output, buffer});
    }
    clubs = std::uint32_t(person);
    person_link(c, program, frame, clubs, staff, output, first, second, message, 0x38,
                full ? 0x1008c1b8 : 0x1008c23c, full ? 0x1008c1d8 : 0x1008c25c);
}

void render_expiring_contract(Cpu& c, std::uint32_t message_address, std::uint32_t mode,
                              std::uint32_t output_address) {
    c.set(0, c.lr());
    Frame frame(c, 0x1008c4c0, 0x1a0, {.first_gpr = 19});
    Kept<22> message(c, message_address); Kept<28> buffer(c, c.toc(-0x7bb8));
    Kept<29> staff(c, c.toc(-0x7be0)); Kept<19> display(c, mode);
    Kept<30> pool(c, c.toc(slot::pool)); Kept<23> output(c, output_address); Program program(c);
    Kept<31> club(c, program.call(0x102cb570, 0x1008c4f4, {message, 0}));
    const auto person_id = program.call(0x102cb570, 0x1008c504, {message, 2});
    c.set(0, person_id); Kept<27> person(c, person_id);
    const auto first_value = program.call(0x102cb570, 0x1008c518, {message, 3});
    c.set(0, first_value); Kept<26> first(c, first_value);
    const auto second_value = program.call(0x102cb570, 0x1008c52c, {message, 4});
    c.set(0, second_value); Kept<25> second(c, second_value);
    const auto third_value = program.call(0x102cb570, 0x1008c540, {message, 5});
    c.set(0, third_value); Kept<24> third(c, third_value);
    Kept<20> period(c, program.call(0x102cb570, 0x1008c554, {message, 6}));
    Kept<21> bosman(c, program.call(0x102cb570, 0x1008c564, {message, 7}));
    message = program.call(0x102cb570, 0x1008c574, {message, 8});
    program.call(0x100cb590, 0x1008c598, {first, second, third, frame.at(0x38), 0x12c, 1, person});
    const auto detailed = std::int8_t(mode); c.set(0, std::uint32_t(std::int32_t(detailed)));
    const bool full = nonzero(c, detailed);
    if (full) {
        if (compare_logical(c, period, 7) <= 0) {
            const auto table = c.toc(-0x574c); c.set(3, table); c.set(0, std::uint32_t(period) * 4);
            const auto destination = c.u32(table + std::uint32_t(period) * 4);
            c.set(3, destination); c.s.wr<4>(guest::reg::ctr, destination);
            // Code zero deliberately leaves the existing shared text alone.
            struct Text { unsigned offset; std::uint32_t back; };
            constexpr std::array<std::array<Text, 3>, 6> texts{{
                {{{0x298a,0x1008c6a0},{0x2a52,0x1008c6c0},{0x2b3b,0x1008c6d8}}},
                {{{0x2b90,0x1008c6f8},{0x2c56,0x1008c718},{0x2d3d,0x1008c730}}},
                {{{0x2d90,0x1008c750},{0x2e4e,0x1008c770},{0x2f2d,0x1008c788}}},
                {{{0x2f78,0x1008c7a8},{0x3035,0x1008c7c8},{0x3113,0x1008c7e0}}},
                {{{0x26f6,0x1008c630},{0x27bc,0x1008c650},{0x28a3,0x1008c668}}},
                {{{0x24ed,0x1008c5d8},{0x25b6,0x1008c5f8},{0x26a0,0x1008c610}}}
            }};
            if (std::uint32_t(period) != 0) {
                Text selected{0x28f6, 0x1008c680};
                if (std::uint32_t(period) != 7) {
                    unsigned category = 0;
                    if (compare(c, std::int32_t(std::uint32_t(bosman))) == 0)
                        category = compare(c, std::int32_t(std::uint32_t(message))) != 0 ? 1 : 2;
                    selected = texts[std::uint32_t(period) - 1][category];
                }
                program.call(0x10727890, selected.back, {buffer, pool + selected.offset, 0x64, frame.at(0x38)});
            }
        }
        program.call(0x1074b830, 0x1008c7ec, {output, buffer});
        const auto holder = c.toc(-0x7be8); c.set(4, holder);
        const auto offset = std::uint32_t(club) * 0x248; c.set(0, offset);
        const auto records = c.u32(holder); c.set(4, records);
        program.call(0x1074bab0, 0x1008c80c, {output, records + offset + 0x1ee, 0x7d5, records + offset});
    } else {
        program.call(0x10727890, 0x1008c864, {buffer, pool + 0x315d, 0x64, frame.at(0x38)});
        program.call(0x1074b830, 0x1008c870, {output, buffer});
    }
    period = std::uint32_t(person);
    person_link(c, program, frame, period, staff, output, first, second, third, 0x38,
                full ? 0x1008c82c : 0x1008c890, full ? 0x1008c84c : 0x1008c8b0);
}

void render_bosman_departure(Cpu& c, std::uint32_t message_address, std::uint32_t mode,
                             std::uint32_t output_address) {
    c.set(0, c.lr());
    Frame frame(c, 0x1008ca50, 0x1a0, {.first_gpr = 21});
    Kept<21> message(c, message_address); Kept<25> clubs(c, c.toc(-0x7be8));
    Kept<26> buffer(c, c.toc(-0x7bb8)); Kept<30> related(c, mode);
    Kept<27> staff(c, c.toc(-0x7be0)); Kept<31> output(c, output_address);
    Kept<28> pool(c, c.toc(slot::pool)); Program program(c);
    Kept<29> club(c, program.call(0x102cb570, 0x1008ca88, {message, 0}));
    const auto person_id = program.call(0x102cb570, 0x1008ca98, {message, 2});
    c.set(0, person_id); Kept<24> person(c, person_id);
    const auto first_value = program.call(0x102cb570, 0x1008caac, {message, 3});
    c.set(0, first_value); Kept<23> first(c, first_value);
    const auto second_value = program.call(0x102cb570, 0x1008cac0, {message, 4});
    c.set(0, second_value); Kept<22> second(c, second_value);
    const auto third_value = program.call(0x102cb570, 0x1008cad4, {message, 5});
    c.set(0, third_value); message = third_value;
    const auto related_id = program.call(0x102cb570, 0x1008cae8, {message_address, 6});
    const auto detailed = std::int8_t(mode); c.set(0, std::uint32_t(std::int32_t(detailed)));
    const bool full = nonzero(c, detailed); related = related_id;
    program.call(0x100cb590, full ? 0x1008cb14 : 0x1008cc4c,
        {first, second, message, frame.at(0x40), 0x12c, full ? 1u : 5u, person});
    const bool destination = compare(c, std::int32_t(related_id), -1) > 0;
    if (full) {
        if (destination) {
            const auto related_offset = related_id * 0x248; c.set(4, related_offset);
            const auto records = c.u32(clubs); c.set(11, records); c.set(5, records + 0x242);
            const auto club_offset = std::uint32_t(club) * 0x248; c.set(6, club_offset);
            const auto other_gender = c.i8(records + 0x242 + related_offset);
            c.set(7, std::uint32_t(std::int32_t(other_gender)));
            const auto extra = records + related_offset + 0x1ee; c.set(0, extra);
            const auto gender = c.i8(records + 0x242 + club_offset);
            c.put32(frame.at(0x38), extra);
            program.call(0x10727890, 0x1008cb68,
                {buffer, pool + 0x3190, std::uint32_t(std::int32_t(gender)), std::uint32_t(std::int32_t(gender)),
                 std::uint32_t(std::int32_t(other_gender)), std::uint32_t(std::int32_t(other_gender)),
                 frame.at(0x40), records + club_offset + 0x1ee});
        } else {
            const auto offset = std::uint32_t(club) * 0x248; c.set(6, offset);
            const auto records = c.u32(clubs); c.set(0, records);
            const auto gender = c.i8(records + offset + 0x242);
            program.call(0x10727890, 0x1008cb9c,
                {buffer, pool + 0x3263, std::uint32_t(std::int32_t(gender)),
                 std::uint32_t(std::int32_t(gender)), frame.at(0x40), records + offset + 0x1ee});
        }
        program.call(0x1074b830, 0x1008cba8, {output, buffer});
        const auto offset = std::uint32_t(club) * 0x248; c.set(0, offset);
        const auto records = c.u32(clubs); c.set(4, records);
        program.call(0x1074bab0, 0x1008cbc4, {output, records + offset + 0x1ee, 0x7d5, records + offset});
    } else {
        if (destination) {
            const auto offset = related_id * 0x248; c.set(6, offset);
            const auto records = c.u32(clubs); c.set(0, records);
            const auto gender = c.i8(records + offset + 0x23b);
            program.call(0x10727890, 0x1008cc84,
                {buffer, pool + 0x32f5, std::uint32_t(std::int32_t(gender)),
                 std::uint32_t(std::int32_t(gender)), frame.at(0x40), records + offset + 0x221});
        } else program.call(0x10727890, 0x1008cc98, {buffer, pool + 0x333c, frame.at(0x40)});
        program.call(0x1074b830, 0x1008cca4, {output, buffer});
    }
    if (compare(c, std::int32_t(related_id), -1) > 0) {
        const auto offset = related_id * 0x248; c.set(0, offset);
        const auto records = c.u32(clubs); c.set(4, records);
        program.call(0x1074bab0, full ? 0x1008cbe8 : 0x1008ccc8,
            {output, records + offset + (full ? 0x1eeu : 0x221u), 0x7d5, records + offset});
    }
    person_link(c, program, frame, person, staff, output, first, second, message, 0x40,
                full ? 0x1008cc08 : 0x1008cce8, full ? 0x1008cc28 : 0x1008cd08);
}
}

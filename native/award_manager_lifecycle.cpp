#include "award_manager_detail.hpp"

#include <array>

namespace cm::aot::award_manager {
using namespace detail;
namespace {
constexpr std::uint32_t allocate_object = 0x10773520;
constexpr unsigned first_history = 0x10, second_history = 0xc;
constexpr unsigned notification_buffer = 0x1c, notification_count = 0xdc;

struct GroupConstructor {
    std::uint32_t target;
    std::uint32_t allocation_return;
    std::uint32_t constructor_return;
};
struct NationalGroup {
    int nation_slot;
    GroupConstructor primary;
    int variant_slot = 0;
    GroupConstructor alternate{};
};

// The original keeps a fixed slot for every award group even if that
// competition is disabled or its object allocation fails.
constexpr std::array national_groups{
    NationalGroup{-0x7508, {0x10435880, 0x104366bc, 0x104366cc}},
    NationalGroup{-0x7510, {0x10439fe0, 0x104366fc, 0x1043670c}},
    NationalGroup{-0x7b68, {0x1043a840, 0x1043673c, 0x1043674c}},
    NationalGroup{-0x750c, {0x1043c980, 0x1043677c, 0x1043678c}},
    NationalGroup{-0x7a10, {0x1043d160, 0x104367bc, 0x104367cc}},
    NationalGroup{-0x7594, {0x106aef60, 0x1043680c, 0x1043681c},
                  -0x70fc, {0x106b1a10, 0x10436830, 0x10436840}},
    NationalGroup{-0x7a00, {0x1043e5b0, 0x10436880, 0x10436890},
                  -0x70d8, {0x106b2310, 0x104368a4, 0x104368b4}},
    NationalGroup{-0x7af4, {0x1043f350, 0x104368e4, 0x104368f4}},
    NationalGroup{-0x7a28, {0x1043fe40, 0x10436924, 0x10436934}},
    NationalGroup{-0x7574, {0x106b01d0, 0x10436970, 0x10436980},
                  -0x70f0, {0x106b32f0, 0x10436994, 0x104369a4}},
    NationalGroup{-0x7a14, {0x10440c70, 0x104369d4, 0x104369e4}},
    NationalGroup{-0x7a24, {0x104422a0, 0x10436a14, 0x10436a24}},
    NationalGroup{-0x7504, {0x10446d90, 0x10436a54, 0x10436a64}},
    NationalGroup{-0x79fc, {0x10447af0, 0x10436a94, 0x10436aa4}},
    NationalGroup{-0x7a1c, {0x104483a0, 0x10436ad4, 0x10436ae4}},
    NationalGroup{-0x7a18, {0x10449710, 0x10436b14, 0x10436b24}},
    NationalGroup{-0x7514, {0x10449de0, 0x10436b54, 0x10436b64}},
    NationalGroup{-0x7a20, {0x1044c0b0, 0x10436b94, 0x10436ba4}},
    NationalGroup{-0x75a8, {0x106b3fc0, 0x10436bd4, 0x10436be4}},
    NationalGroup{-0x7598, {0x106afa70, 0x10436c14, 0x10436c24}},
    NationalGroup{-0x7590, {0x106ae880, 0x10436c54, 0x10436c64}},
    NationalGroup{-0x7650, {0x106adfe0, 0x10436c94, 0x10436ca4}},
    NationalGroup{-0x7578, {0x106b0f30, 0x10436cd4, 0x10436ce4}},
    NationalGroup{-0x75b0, {0x106b4eb0, 0x10436d14, 0x10436d24}},
    NationalGroup{-0x758c, {0x106b47e0, 0x10436d54, 0x10436d64}},
    NationalGroup{-0x757c, {0x106b5420, 0x10436d94, 0x10436da4}},
};
static_assert(national_groups.size() + 3 == award_groups);

void construct_group(Cpu& c, Manager manager, unsigned index, std::uint32_t file,
                     GroupConstructor constructor) {
    const auto group = call(c, allocate_object, constructor.allocation_return, {28});
    c.set(27, group);
    if (compare(c, group) != 0)
        call(c, constructor.target, constructor.constructor_return, {group, file});
    const auto groups = manager.groups();
    c.set(3, groups);
    c.put32(groups + index * 4, group);
}

void initialize_from(Cpu& c, std::uint32_t manager, std::uint32_t file, std::uint32_t back) {
    constexpr auto entry = 0x104365b0;
    if (dispatch_owned(entry)) call(c, entry, back, {manager, file});
    else {
        enter(c, back, {manager, file});
        initialize(c, manager, file);
    }
}

std::uint32_t load_from(Cpu& c, std::uint32_t manager, std::uint32_t file, std::uint32_t back) {
    constexpr auto entry = 0x10436dc0;
    if (dispatch_owned(entry)) return call(c, entry, back, {manager, file});
    enter(c, back, {manager, file});
    return load(c, manager, file);
}

void construct_history(Cpu& c, std::uint32_t manager, unsigned kept_register, std::uint32_t stream_slot,
                       std::uint32_t date, std::array<std::uint32_t, 5> returns) {
    const auto stream = [&] { return stream_slot ? c.u32(stream_slot) : 0; };
    const auto player_history = call(c, allocate_object, returns[0], {16});
    c.set(kept_register, player_history);
    if (compare(c, player_history) != 0) {
        const auto start = call(c, 0x107156f0, returns[1], {date, 20, 5, 2001, no_id});
        call(c, 0x1044e220, returns[2], {player_history, c.u16(start), stream()});
    }
    c.put32(manager + first_history, player_history);
    const auto team_history = call(c, allocate_object, returns[3], {16});
    c.set(kept_register, team_history);
    if (compare(c, team_history) != 0)
        call(c, 0x1044e220, returns[4], {team_history, 364, stream()});
    c.put32(manager + second_history, team_history);
}
}

std::uint32_t construct(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x10436080, 0x360, {.first_gpr = 25});
    const auto source = c.toc(-0x4a84);
    const auto even = c.toc(-0x4a80), odd = c.toc(-0x4a7c);
    keep(c, {{26, manager}, {27, file}, {29, source}, {30, even}, {31, odd}});
    const auto combined = frame.at(0xac);
    for (;;) {
        const auto odd_copy = frame.at(0x50), even_copy = frame.at(0x40);
        leave(c, {{10, c.u32(odd)}, {9, c.u32(odd + 4)}, {8, c.u32(odd + 8)},
                  {7, c.u32(odd + 12)}, {6, c.u8(odd + 16)}, {5, c.u32(even)},
                  {4, c.u32(even + 4)}, {3, c.u32(even + 8)}});
        const auto last_even = c.u32(even + 12);
        for (unsigned word = 0; word < 4; ++word) c.put32(odd_copy + 4 * word, c.r(10 - word));
        c.put8(odd_copy + 16, c.r(6));
        for (unsigned word = 0; word < 3; ++word) c.put32(even_copy + 4 * word, c.r(5 - word));
        c.put32(even_copy + 12, last_even);
        unsigned i = 0;
        for (; compare(c, c.i8(odd_copy + 1 + i)) != 0; ++i) {
            c.set(3, c.u8(odd_copy + 1 + i));
            c.put8(combined + 1 + 2 * i, c.r(3));
            c.put8(combined + 2 + 2 * i, c.u8(even_copy + 1 + i));
        }
        keep(c, {{25, even_copy + 1 + i}, {28, 0}});
        c.set(7, c.u8(odd_copy));
        c.put8(combined, c.r(7) + c.u8(even_copy));
        const auto status = call(c, 0x1077fee0, 0x1043614c, {0, 0, combined, frame.at(0x64)});
        c.set(2, c.u32(frame.at(0x14)));
        const bool found = compare(c, std::int16_t(status)) == 0;
        c.set(28, found);
        if (compare(c, found) == 0) {
            call(c, 0x10727890, 0x10436170, {frame.at(0x274), source});
            call(c, 0x10727890, 0x1043617c, {frame.at(0x1ac), source + 0xa});
            call(c, 0x10769b60, 0x1043618c, {frame.at(0x274), frame.at(0x1ac), 1});
        }
        if (compare(c, found) != 0) break;
    }
    call(c, 0x10778300, 0x104361a4, {manager + notification_buffer, 0, 0x40});
    c.put32(manager + notification_count, 0);
    struct Store { unsigned extension, record_size; std::uint32_t allocate, open, construct; };
    constexpr std::array stores{
        Store{0x40, 0x4c, 0x104361b4, 0x104361c8, 0x104361d8},
        Store{0x55, 0x244, 0x104361e4, 0x104361f8, 0x10436208},
    };
    for (unsigned i = 0; i < stores.size(); ++i) {
        const auto& store = stores[i];
        const auto object = call(c, allocate_object, store.allocate, {8});
        c.set(25, object);
        if (compare(c, object) != 0) {
            const auto stream = call(c, 0x10746280, store.open, {source + 0x37, source + store.extension});
            call(c, 0x10748bb0, store.construct, {object, stream, store.record_size});
        }
        c.put32(manager + 4 * i, object);
    }
    const bool from_save = compare_logical(c, file) != 0;
    c.put32(manager + 0x14, 0);
    c.put16(manager + 0x18, 0);
    c.put16(manager + 0x1a, 0);
    if (from_save && compare(c, std::int8_t(load_from(c, manager, file, 0x10436230))) != 0)
        return result(c, manager);
    initialize_from(c, manager, 0, 0x1043624c);
    call(c, 0x100a4c70, 0x10436250);
    construct_history(c, manager, 25, 0, frame.at(0x38),
                      {0x10436258, 0x10436278, 0x10436290, 0x1043629c, 0x104362b0});
    return result(c, manager);
}

std::uint32_t destroy(Cpu& c, std::uint32_t manager, std::uint32_t deleting) {
    Frame frame(c, 0x104362d0, 0x50, {.first_gpr = 28});
    keep(c, {{28, manager}, {29, deleting}});
    if (compare(c, manager) == 0) return result(c, manager);
    Manager object{c, manager};
    if (compare_logical(c, object.groups()) != 0) {
        keep(c, {{30, 0}, {31, 0}});
        unsigned group = 0;
        do {
            c.set(3, object.group(group));
            if (compare_logical(c, c.r(3)) != 0) {
                c.set(4, 1);
                c.call(0x104443b0, 0x10436328);
            }
            ++group;
            keep(c, {{30, group}, {31, group * 4}});
        } while (compare(c, group, award_groups) < 0);
        call(c, 0x10775170, 0x10436340, {object.groups()});
    }
    struct OwnedField { unsigned offset; std::uint32_t target, back; };
    constexpr std::array fields{
        OwnedField{0, 0x10748c80, 0x10436354}, OwnedField{4, 0x10748c80, 0x10436368},
        OwnedField{first_history, 0x1044e270, 0x1043637c}, OwnedField{second_history, 0x1044e270, 0x10436390},
    };
    for (const auto& field : fields) {
        c.set(3, c.u32(manager + field.offset));
        if (compare_logical(c, c.r(3)) != 0) {
            c.set(4, 1);
            c.call(field.target, field.back);
        }
    }
    c.set(3, c.u32(manager + 0x14));
    if (compare_logical(c, c.r(3)) != 0) c.call(0x10775170, 0x104363a0);
    if (compare(c, std::int16_t(deleting)) > 0) call(c, 0x107735a0, 0x104363b0, {manager});
    return result(c, manager);
}

void initialize(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x104365b0, 0x50, {.first_gpr = 27});
    const auto nations_global = c.toc(-0x7b98), international_variant = c.toc(-0x70f0);
    keep(c, {{28, international_variant}, {29, nations_global}, {30, file}, {31, manager}});
    Manager object{c, manager};
    c.put32(manager + 8, call(c, 0x10775200, 0x104365dc, {award_groups, 4}));
    construct_group(c, object, 0, file, {0x1044c7d0, 0x104365e8, 0x104365f8});
    const auto legacy = compare(c, c.u32(international_variant), no_id) == 0;
    construct_group(c, object, 1, file, legacy ? GroupConstructor{0x106b2d30, 0x10436638, 0x10436648}
                                                : GroupConstructor{0x10440620, 0x10436614, 0x10436624});
    const auto competition = c.toc(-0x7520);
    c.set(4, competition);
    const auto tournament = c.u32(competition) + c.u32(c.toc(-0x7a70)) * 200;
    if (compare(c, std::int8_t(call(c, 0x101926b0, 0x1043666c, {tournament}))) != 0)
        construct_group(c, object, 2, file, {0x105e4dc0, 0x1043667c, 0x1043668c});
    for (unsigned i = 0; i < national_groups.size(); ++i) {
        const auto& group = national_groups[i];
        const auto nations = c.u32(nations_global);
        const auto nation = nations + c.u32(c.toc(group.nation_slot)) * 0x128;
        leave(c, {{3, nation}, {4, nations}});
        if (compare(c, c.i8(nation + 0x120)) == 0) continue;
        auto constructor = group.primary;
        if (group.variant_slot != 0) {
            const auto variant = group.variant_slot == -0x70f0 ? international_variant : c.toc(group.variant_slot);
            if (group.variant_slot != -0x70f0) c.set(3, variant);
            if (compare(c, c.u32(variant), no_id) == 0) constructor = group.alternate;
        }
        construct_group(c, object, i + 3, file, constructor);
    }
}

std::uint32_t load(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x10436dc0, 0xb0, {.first_gpr = 28});
    const auto source = c.toc(-0x4a84);
    keep(c, {{28, manager}, {29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, source}});
    if (compare_logical(c, file) == 0) {
        assertion(c, frame, 0x10436e08, 0x74, 0x4c, 0x214);
        return result(c, 0);
    }
    const auto stream = call(c, 0x107475b0, 0x10436e64, {file, source + 0x8f, 0, 0x16, 4});
    c.set(4, stream);
    const bool open = compare(c, stream) != 0;
    c.put32(frame.at(0x38), stream);
    if (!open) {
        assertion(c, frame, 0x10436e84, 0x54, 0x44, 0x21c);
        return result(c, 0);
    }
    initialize_from(c, manager, stream, 0x10436ed0);
    construct_history(c, manager, 29, frame.at(0x38), frame.at(0x3c),
                      {0x10436ed8, 0x10436ef8, 0x10436f0c, 0x10436f18, 0x10436f2c});
    call(c, 0x10747900, 0x10436f38, {frame.at(0x38)});
    return result(c, 1);
}

void save(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x10436f60, 0xa0, {.first_gpr = 28});
    const auto source = c.toc(-0x4a84);
    keep(c, {{28, manager}, {29, c.toc(-0x7bac)}, {30, c.toc(-0x7ba8)}, {31, source}});
    if (compare_logical(c, file) == 0) {
        assertion(c, frame, 0x10436fa8, 0x6c, 0x44, 0x23d);
        return;
    }
    const auto stream = call(c, 0x107475b0, 0x10437000, {file, source + 0x8f, 1, 0x16, 4});
    const bool open = compare_logical(c, stream) != 0;
    c.put32(frame.at(0x38), stream);
    if (!open) {
        assertion(c, frame, 0x10437020, 0x4c, 0x3c, 0x245);
        return;
    }
    Manager object{c, manager};
    keep(c, {{29, 0}, {30, 0}});
    unsigned index = 0;
    do {
        c.set(3, object.group(index));
        if (compare_logical(c, c.r(3)) != 0)
            call(c, 0x10446c10, 0x10437084, {c.r(3), c.u32(frame.at(0x38))});
        ++index;
        keep(c, {{29, index * 4}, {30, index}});
    } while (compare(c, index, award_groups) < 0);
    for (const auto& [field, back] : {std::pair{first_history, 0x104370a8u},
                                     std::pair{second_history, 0x104370bcu}}) {
        c.set(3, c.u32(manager + field));
        if (compare_logical(c, c.r(3)) != 0)
            call(c, 0x1044e480, back, {c.r(3), c.u32(frame.at(0x38))});
    }
    call(c, 0x10747900, 0x104370c4, {frame.at(0x38)});
}
}

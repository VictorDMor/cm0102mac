// DATABASE.CPP's generation block, part 6: 0x100c3870, the adjustment of a person's attribute
// slots (database_generation.hpp). No random draws: it copies the person's attributes into a local
// slot array (copy_attributes_to_slots), finds how many are within a limit of 0, shifts and clamps
// them by their average, and writes them back (copy_attributes_from_slots).
#include "database_generation.hpp"
#include "database_generation_detail.hpp"

namespace cm::aot::database {
using namespace generation;
namespace {
constexpr std::uint32_t entry = 0x100c3870;
constexpr std::int32_t terminator = -0x7f;  // ends the slot array
constexpr std::uint32_t slot_size = 4, slot_total = 63;
constexpr std::uint32_t array_at = 0xa0, flags_at = 0x3c, count_at = 0x38;  // frame offsets
constexpr std::uint32_t attributes_record = 0x1c;  // the person's record the copy reads (inferred: its player)
constexpr std::uint32_t kind_a = 0x60, kind_b = 0x62;  // bytes of the person that select the "10" case

// The frame's slot array and its flag bytes.
struct Slots {
    Cpu& c;
    std::uint32_t array, flags;
    std::int32_t get(std::uint32_t index) const { return c.i16(array + slot_size * std::uint32_t(index)); }
    void set(std::uint32_t index, std::int32_t value) const { c.put16(array + slot_size * index, std::uint32_t(value)); }
    std::int32_t flag(std::uint32_t index) const { return c.i8(flags + index); }
    void set_flag(std::uint32_t index, std::uint32_t value) const { c.put8(flags + index, value); }
};
// The index counters are 32-bit registers the original sign-extends from a byte at every use.
std::uint32_t byte_index(std::uint32_t counter) { return std::uint32_t(std::int32_t(std::int8_t(counter))); }
std::int32_t half(std::int32_t value) { return std::int16_t(value); }
}

void balance_attributes(Cpu& c, std::uint32_t person) {
    guest::Frame frame(c, entry, 0x1d0, {.first_gpr = 21});
    c.set(0, c.lr());
    compare(c, std::int32_t(person));
    c.set(21, person);
    if (person == 0) return;
    const auto record = load32(c, 31, person + attributes_record);
    if (compare_logical(c, record) == 0) return;
    const Slots slots{c, frame.at(array_at), frame.at(flags_at)};
    for (unsigned i = 0; i < slot_total; ++i) {
        c.put16(slots.array + i * slot_size, std::uint32_t(terminator) & 0xffff);
        c.put8(slots.array + i * slot_size + 2, std::uint32_t(terminator) & 0xff);
    }
    constexpr std::int32_t base_shift = -5;
    guest::enter(c, entry + 0x70, {record, person, slots.array, frame.at(count_at)});
    copy_attributes_to_slots(c, record, slots.array, frame.at(count_at));
    const auto first_slot = [&] { return std::uint32_t(c.u8(frame.at(count_at))); };
    for (std::int32_t pass = 0; pass < 2; ++pass) {
        std::int32_t positive_count = 0, positive_sum = 0, limit = 0x28;
        std::int32_t within = 0, sum = 0, outlier = -1;
        // Widen the limit by 10 until at least 6 slots are within it; remember the last slot 19 beyond it
        // (the larger one) as the outlier.
        do {
            std::uint32_t i = first_slot();
            sum = 0;
            within = 0;
            outlier = -1;
            while (slots.get(byte_index(i)) != terminator) {
                const auto index = byte_index(i);
                const auto value = slots.get(index);
                // abs is the C library's leaf; its srawi leaves XER[CA], which the checker sees.
                c.set(3, std::uint32_t(value));
                c.call(0x10775540, entry + 0xac);
                if (std::int32_t(c.r(3)) <= half(limit)) {
                    ++within;
                    slots.set_flag(index, 1);
                    sum += value;
                } else {
                    slots.set_flag(index, 0);
                    if (index == 0x13 && (std::int8_t(outlier) == -1 || half(value) > slots.get(byte_index(std::uint32_t(outlier)))))
                        outlier = std::int32_t(i);
                }
                ++i;
            }
            if (std::int8_t(within) < 6) limit = half(limit + 10);
        } while (std::int8_t(within) < 6);
        if (std::int8_t(outlier) > -1) slots.set_flag(byte_index(std::uint32_t(outlier)), 1);
        const auto delta = half(base_shift - half(sum / std::int32_t(std::int8_t(within))));
        std::uint32_t i = first_slot();
        while (slots.get(byte_index(i)) != terminator) {
            const auto index = byte_index(i);
            if (std::int32_t(index) != std::int32_t(std::int8_t(outlier))) {
                if (slots.flag(index) == 1) {
                    // Move the slot by the delta and clamp it to -0x7d..0x7d.
                    slots.set(index, slots.get(index) + delta);
                    const auto moved = slots.get(index);
                    slots.set(index, moved < -0x7d ? -0x7d : moved > 0x7d ? 0x7d : moved);
                } else if (std::int8_t(pass) == 0 && slots.get(index) > 0) {
                    ++positive_count;
                    positive_sum += slots.get(index);
                } else {
                    slots.set_flag(index, 1);
                }
                if (c.u8(person + kind_b) == 10 || c.u8(person + kind_a) == 10) {
                    const auto value = slots.get(index);
                    // A person of kind 10: slot 19 keeps only its positive part, the rest at most -0x50.
                    if (index == 0x13) slots.set(index, value > 0 ? value : 0);
                    else slots.set(index, value >= -0x50 ? -0x50 : value);
                }
            }
            ++i;
        }
        i = first_slot();
        while (slots.get(byte_index(i)) != terminator && positive_count != 0 && positive_sum > 0x1c) {
            const auto index = byte_index(i);
            if (slots.flag(index) != 1) {
                const auto share = half((positive_sum - 0x1c) / positive_count);
                slots.set(index, slots.get(index) - share);
                if (slots.get(index) < 0) slots.set(index, 0);
            }
            ++i;
        }
    }
    compare(c, 2, 2);
    c.set(3, record);
    c.set(5, slots.array);
    guest::enter(c, entry + 0x420, {record, person, slots.array});
    copy_attributes_from_slots(c, record, slots.array);
}
}

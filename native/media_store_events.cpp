// The event rows: adding one for a person and club, and dropping a person's.
// Rows are kept sorted by the game's comparator at -0x5df4.
#include "media_store.hpp"
#include "media_store_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::store;
namespace {
constexpr std::uint32_t amount_cap = 50'000'000;
}

// Records an event of `amount` (capped at 50 million) for the person and club, scaled
// by `weight`, unless the store already has one for the same day.
void add_press_event(Cpu& c, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::int32_t amount, double weight) {
    Frame frame(c, 0x10698bb0, 0xf0, {.first_gpr = 23, .gpr_offset = -0x34, .first_fpr = 31, .fpr_offset = -8});
    c.set_f(31, weight);
    keep(c, {{23, self}, {24, person}, {25, club}, {26, std::uint32_t(amount)}, {27, c.toc(slots::today)},
             {28, c.toc(slots::event_order)}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    const Press press{c, self};
    Program program{c};
    Fpu fpu(c);
    const auto key = frame.at(0x48);
    program.call(library::today, 0x10698bf4, {frame.at(0x50)});
    bool known = false;
    if (compare_logical(c, press.events()) != 0) {
        c.put32(key, c.u32(person));
        c.put32(key + 4, compare_logical(c, club) == 0 ? no_id : c.u32(club));
        known = program.call(library::search, 0x10698c44,
                             {key, press.events(), std::uint32_t(press.event_count()), event_size, c.toc(slots::event_order)}) != 0;
        compare_logical(c, c.r(3));
    }
    if (compare(c, known ? 1 : 0) != 0) return;
    if (compare_logical(c, press.events()) == 0) {
        press.events() = program.call(library::allocate, 0x10698c74, {20 * event_size});
        if (compare_logical(c, press.events()) == 0) out_of_memory(c, frame, 0x10698c98, 0x84, 0x40, 0x660c, 0x10698cdc);
        press.event_capacity() = grow_step;
    }
    if (compare(c, press.event_capacity(), press.event_count()) == 0) {
        press.events() = program.call(library::reallocate, 0x10698d08,
                                      {press.events(), (press.event_capacity() + grow_step) * event_size});
        if (compare_logical(c, press.events()) == 0) out_of_memory(c, frame, 0x10698d2c, 0x64, 0x38, 0x6617, 0x10698d70);
        press.event_capacity() = press.event_capacity() + grow_step;
    }
    const auto row = press.event_row(std::uint32_t(press.event_count()));
    const auto today = c.toc(slots::today);
    row.person() = c.u32(person);
    leave(c, {{7, c.u32(person)}});
    compare(c, amount, std::int32_t(amount_cap));
    row.club() = c.u32(club);
    row.date().assign(today);
    row.mark() = 0;
    const std::int32_t capped = amount < std::int32_t(amount_cap) ? amount : std::int32_t(amount_cap);
    row.amount() = capped;
    const auto scaled = fpu.fmul(c.f64(c.toc(slot::floats) + 0x110), c.f(31));
    c.put64(frame.at(0xa4), fpu.fctiwz(0, scaled));
    row.weight() = c.u32(frame.at(0xa8));
    press.event_count() = press.event_count() + 1;
    program.call(library::sort, 0x10698e68,
                 {press.events(), std::uint32_t(press.event_count()), event_size, c.toc(slots::event_order), c.u32(person)});
}

// Removes every event of the person (an id in `person`).
void remove_press_events(Cpu& c, std::uint32_t self, std::uint32_t person) {
    Frame frame(c, 0x10698e80, 0x50, {.first_gpr = 29});
    keep(c, {{29, self}, {30, person}});
    const Press press{c, self};
    Program program{c};
    Kept<31> index(c, 0);
    for (; compare(c, std::int32_t(index), press.event_count()) < 0; index = std::uint32_t(index) + 1) {
        c.set(3, press.events());
        c.set(4, c.u32(person));
        const auto row = press.event_row(index);
        if (compare(c, c.i32(person), std::int32_t(row.person().get())) != 0) continue;
        const auto last = press.event_row(std::uint32_t(press.event_count()) - 1).at;
        program.call(library::move, 0x10698ee8, {row.at, last, event_size, index * event_size, press.events()});
        c.set(3, std::uint32_t(press.event_count()));
        index = std::uint32_t(index) - 1;
        press.event_count() = press.event_count() - 1;
    }
    if (compare(c, press.event_count()) != 0)
        program.call(library::sort, 0x10698f28,
                     {press.events(), std::uint32_t(press.event_count()), event_size, c.toc(slots::event_order)});
}
}

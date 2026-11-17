#include "transfer_manager_native_terms.hpp"
#include "transfer_manager_native_stages_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace stages;

namespace {
// A club's squad rows (the words at club +0x98, 0x160, 0x174, 0x190, each a row of person-profile pointers) and the
// squad limits they are held to; the five counters below tally the people an offer adds or removes in each row.
constexpr std::uint32_t row_main = 0x98, row_a = 0x160, row_c = 0x174, row_b = 0x190, club_open_slot = 0x94;
constexpr int main_slots = 50, a_slots = 5, c_slots = 7, b_slots = 3;
constexpr std::uint32_t too_many_own = 0x22, too_many_other = 0x23, main_full_own = 4, main_full_other = 5;
constexpr std::uint32_t is_in_row = 0x100c7a20, row_group = 0x100c7a00, held_contract = 0x10083110;

// Which counter a person's role (byte +0x62 of the person, +6 of a listed entry, +0x11 of an offer) feeds.
enum class Group { none, a, c, b, open };
Group group_of(std::int8_t role) {
    if (role == 8 || role == 0xf) return Group::a;
    if (role == 9) return Group::c;
    if (role == 10) return Group::b;
    if (role == 6 || role == 0xd) return Group::open;
    return Group::none;
}

// The registers the original holds at each call below (callees save them and later code reads those frames).
struct Held {
    std::uint32_t r19 = 0, r24 = 0, r25 = 0, r26 = 0, r31 = 0;
    std::uint32_t count_a = 0, count_c = 0, count_b = 0, count_open = 0, best = 0;
    double f30 = 0, f31 = 0;
    bool f30_set = false;
    std::uint32_t& counter(Group g) {
        switch (g) {
        case Group::a: return count_a;
        case Group::c: return count_c;
        case Group::b: return count_b;
        default: return count_open;
        }
    }
};
std::uint32_t signed_byte(std::uint32_t x) { return std::uint32_t(std::int32_t(std::int8_t(x))); }
}

std::uint32_t check_squad_room(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t offer, std::uint32_t trim_flag,
                               std::uint32_t code_out) {
    Frame frame(c, 0x103f4fd0, 0xd0, {.first_gpr = 13, .first_fpr = 30});
    const auto people_cell = c.toc(toc::people), found_cell = c.toc(toc::found_contract), manager_cell = c.toc(toc::manager);
    const auto limits = c.toc(-0x4ba0), contracts_cell = c.toc(toc::contracts), limit_cell = c.toc(-0x7be4);
    const auto strings = c.toc(-0x4b9c), log_cell = c.toc(-0x7ba8);
    Held h;
    h.r19 = people_cell;
    h.r24 = strings;
    h.r26 = trim_flag;
    h.r31 = log_cell;
    h.f31 = c.f64(limits + 0x158);
    guest::Fpu fpu(c);
    const auto sync = [&] {
        keep(c, 13, manager);
        keep(c, 14, found_cell);
        keep(c, 15, club);
        keep(c, 16, offer);
        keep(c, 17, code_out);
        keep(c, 18, h.best);
        keep(c, 19, h.r19);
        keep(c, 20, h.count_open);
        keep(c, 21, h.count_c);
        keep(c, 22, h.count_b);
        keep(c, 23, h.count_a);
        keep(c, 24, h.r24);
        keep(c, 25, h.r25);
        keep(c, 26, h.r26);
        keep(c, 27, manager_cell);
        keep(c, 28, limits);
        keep(c, 29, contracts_cell);
        keep(c, 30, limit_cell);
        keep(c, 31, h.r31);
        c.set_f(31, h.f31);
        if (h.f30_set) c.set_f(30, h.f30);
    };
    const auto call = [&](std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        sync();
        return invoke(c, target, back, arguments);
    };
    const auto refuse = [&](std::uint32_t own, std::uint32_t other) {
        if (code_out) c.put16(code_out, std::uint16_t(c.i32(club) == c.i32(offer + offer_club) ? own : other));
        return 0u;
    };

    if (!club) {
        sync();
        report_bad_offer(c, strings, log_cell, c.toc(-0x7bac), frame.at(0x40), frame.at(0x38), 0x2c3a,
                         {0x103f5054, 0x103f5058, 0x103f5074, 0x103f5084});
        return 0;
    }
    const auto other = call(0x100ce320, 0x103f50a8, {club, 0, 1});
    h.r31 = other;
    const auto offer_list = [&] { return c.u32(offer + offer_listed); };
    const bool own_club = c.i32(club) == c.i32(offer + offer_club);
    if (own_club) {
        const auto person = person_record(c, c.u32(offer + offer_person));
        h.r24 = person;
        const auto contract = call(find_contract, 0x103f50d8, {c.u32(contracts_cell), person});
        c.put32(found_cell, contract);
        bool settled_here = false;
        if (contract && contract_kind(c, contract) == no_club_contract_kind) {
            // A person on loan or between clubs: he counts for the club that holds his contract.
            const auto second = call(find_contract, 0x103f511c, {c.u32(contracts_cell), person});
            c.put32(found_cell, second);
            settled_here = (second ? club_record(c, c.u32(second + 4)) : 0) == club;
        }
        if (settled_here) h.r25 = 0xffffffff;
        else if (call(held_contract, 0x103f5158, {c.u32(contracts_cell), person}) != 0) {
            const auto held = call(held_contract, 0x103f516c, {c.u32(contracts_cell), person});
            if (c.i32(club) == c.i32(held + 4)) h.r25 = 0xffffffff;
        }
        const auto role = std::uint8_t(c.u8(offer + 0x11));
        if (std::int8_t(call(is_in_row, 0x103f5188, {role})) != 0) h.r25 = signed_byte(h.r25 + 1);
        if (std::int8_t(call(row_group, 0x103f51a0, {role})) != 0) {
            if (const auto g = group_of(std::int8_t(role)); g != Group::none) h.counter(g) = 1;
        }
        if (offer_list()) {
            rewind_list(c, offer_list());
            while (true) {
                const auto entry = call(next_listed, 0x103f52a4, {c.u32(offer + offer_listed)});
                if (!entry) break;
                h.r24 = person_record(c, c.u32(entry));
                const auto listed_role = c.u8(h.r24 + 0x62);
                if (std::int8_t(call(is_in_row, 0x103f522c, {listed_role})) != 0) h.r25 = h.r25 - 1;
                if (std::int8_t(call(row_group, 0x103f5240, {listed_role})) != 0) {
                    if (const auto g = group_of(std::int8_t(listed_role)); g != Group::none) h.counter(g) = h.counter(g) - 1;
                }
            }
        }
    } else {
        if (offer_list() == 0) return 1;
        const auto person = person_record(c, c.u32(offer + offer_person));
        h.r19 = person;
        const auto role = c.u8(person + 0x62);
        if (std::int8_t(call(is_in_row, 0x103f52dc, {role})) != 0) h.r25 = 0xffffffff;
        if (std::int8_t(call(row_group, 0x103f52f0, {role})) != 0) {
            if (const auto g = group_of(std::int8_t(role)); g != Group::none) h.counter(g) = 0xffffffff;
        }
        if (offer_list()) {
            rewind_list(c, offer_list());
            while (true) {
                h.r19 = call(next_listed, 0x103f53ec, {c.u32(offer + offer_listed)});
                if (!h.r19) break;
                const auto listed_role = c.u8(h.r19 + 6);
                if (std::int8_t(call(is_in_row, 0x103f5370, {listed_role})) != 0) h.r25 = signed_byte(h.r25 + 1);
                if (std::int8_t(call(row_group, 0x103f5388, {listed_role})) != 0) {
                    if (const auto g = group_of(std::int8_t(listed_role)); g != Group::none) h.counter(g) = h.counter(g) + 1;
                }
            }
        }
    }

    const auto limit = [&] { return c.i32(limit_cell); };
    // Adds, for each person of a row whose profile is held below the limit, one to `count`.
    const auto count_held = [&](std::uint32_t owner, std::uint32_t row, int slots, std::uint32_t& count) {
        const auto bound = limit();
        for (int i = 0; i < slots; ++i) {
            const auto entry = c.u32(owner + row + std::uint32_t(i) * 4);
            if (entry && c.i32(entry) < bound) count += 1;
        }
    };
    // The same rows of the club itself count every person, the other club's those held below the limit.
    const auto count_row = [&](std::uint32_t row, int slots, std::uint32_t& count) {
        for (int i = 0; i < slots; ++i)
            if (c.u32(club + row + std::uint32_t(i) * 4)) count = signed_byte(count + 1);
        if (other) count_held(other, row, slots, count);
    };

    if (std::int8_t(h.r25) > 0) {
        count_held(club, row_main, main_slots, h.r25);
        if (other) count_held(other, row_main, main_slots, h.r25);
        if (std::int8_t(h.r25) > main_slots && std::int8_t(trim_flag) != 0) {
            const auto trimmed = call(0x103e6360, 0x103f54b4, {manager, club});
            h.r25 = h.r25 - trimmed;
        }
        if (std::int8_t(h.r25) > main_slots) {
            if (c.i32(club) == c.i32(offer + offer_club) && std::int8_t(call(club_test, 0x103f54ec, {club, 1, 1, 0, 0})) == 0 && offer_list() == 0) {
                h.r24 = 0;
                h.r26 = 0;
                for (; std::int8_t(h.r24) < main_slots; h.r26 += 4, h.r24 += 1) {
                    h.r19 = c.u32(club + row_main + h.r26);
                    if (!h.r19 || c.i32(h.r19) >= limit()) continue;
                    if (call(held_contract, 0x103f553c, {c.u32(contracts_cell), h.r19}) != 0) continue;
                    const auto contract = call(find_contract, 0x103f5550, {c.u32(contracts_cell), h.r19});
                    if (!contract) continue;
                    // The person's worth: (value) * (rating + a constant) against the best found so far.
                    const auto rating = std::int8_t(c.u8(contract + 0x44));
                    const auto biased = [&](std::uint32_t x) {
                        return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (x ^ 0x80000000u));
                    };
                    const auto bias = c.f64(limits + 8);
                    auto term = fpu.fsub(biased(std::uint32_t(std::int32_t(rating))), bias);
                    term = fpu.fadd(c.f64(limits + 0x160), term);
                    const auto value = fpu.fsub(biased(c.u32(h.r19 + 8)), bias);
                    h.f30 = fpu.fmul(value, term);
                    h.f30_set = true;
                    fpu.fcmp(0, h.f30, h.f31);
                    if (!(h.f30 < h.f31) || rating > 0) continue;
                    const auto again = call(find_contract, 0x103f55bc, {c.u32(contracts_cell), h.r19});
                    c.put32(found_cell, again);
                    if (again && contract_kind(c, again) == no_club_contract_kind) continue;
                    if (call(held_contract, 0x103f5600, {c.u32(contracts_cell), h.r19}) != 0) continue;
                    const auto objects = c.u32(manager_cell);
                    if (call(0x103eaea0, 0x103f561c, {objects, h.r19, 0, 0}) != 0) continue;
                    if (call(0x103eb020, 0x103f5634, {objects, h.r19, 0}) != 0) continue;
                    if (std::int8_t(call(0x103f43b0, 0x103f564c, {objects, h.r19, 0})) != 0) continue;
                    h.r25 = objects;
                    const auto list = objects + 0xa5c;
                    rewind_list(c, list);
                    bool listed = false;
                    while (true) {
                        const auto entry = call(next_listed, 0x103f5688, {list});
                        if (!entry) break;
                        if (c.i32(h.r19) == c.i32(entry)) { listed = true; break; }
                    }
                    if (listed) continue;
                    const auto role = std::int8_t(c.u8(h.r19 + 0x62));
                    if (role == 0xb || role == 0xf || role == 0xd) {
                        h.best = h.r19;
                        h.f31 = h.f30;
                    }
                }
                if (h.best) {
                    call(0x10386360, 0x103f56f0, {c.u32(c.toc(-0x7b48)), h.best});
                    invoke_native(c, 0x103e68c0, 0x103f570c, {manager, h.best, club, 0, 0xffffffff, 0});
                }
            }
            if (!h.best) {
                if (code_out) c.put16(code_out, std::uint16_t(c.i32(club) == c.i32(offer + offer_club) ? main_full_own : main_full_other));
                return 0;
            }
        }
    }
    if (std::int8_t(h.count_a) > 0) {
        count_row(row_a, a_slots, h.count_a);
        if (std::int8_t(h.count_a) > a_slots) return refuse(too_many_own, too_many_other);
    }
    if (std::int8_t(h.count_b) > 0) {
        count_row(row_b, b_slots, h.count_b);
        if (std::int8_t(h.count_b) > b_slots) return refuse(too_many_own, too_many_other);
    }
    if (std::int8_t(h.count_c) > 0) {
        count_row(row_c, c_slots, h.count_c);
        if (std::int8_t(h.count_c) > c_slots) return refuse(too_many_own, too_many_other);
    }
    if (std::int8_t(h.count_open) > 0 &&
        (c.u32(club + club_open_slot) != 0 || (other && c.u32(other + club_open_slot) != 0)))
        return refuse(too_many_own, too_many_other);
    return 1;
}
}

#include "awol_detail.hpp"

namespace cm::aot::awol {
using namespace detail;
namespace {
// Fields these two events read. Names follow use; the original gives none.
constexpr std::uint32_t contract_mood = 0x4c, contract_flags = 0x50;
constexpr std::uint32_t staff_record = 0x1c;       // the record holding the staff member's traits
constexpr std::uint32_t record_trait = 0x34, record_coded_trait = 0x39;
constexpr std::uint32_t daily_record_size = 100, daily_record_counter = 0xa;
// The compiler's divisions by 5, 10 and 20 (multiply by 0x66666667, shift,
// add the sign): the sign it leaves in a register and the signed remainder.
struct Remainder { std::uint32_t sign; std::int32_t value; };
Remainder remainder_by(std::uint32_t sum, unsigned shift, std::uint32_t divisor) {
    const auto high = multiply_high(0x66666667, std::int32_t(sum)) >> shift;
    const auto sign = std::uint32_t(high) >> 31;
    const auto quotient = std::uint32_t(high) + sign;
    return {sign, std::int32_t(sum - quotient * divisor)};
}
// The word at the cursor plus the salt.
std::uint32_t current_sum(Cpu& c, const Pool& pool) { return c.u16(pool.position()) + pool.bias(); }

// ---- training_event ----

// One conversion of a training absence into a primary absence: a contract
// flag (mood 0x2000, then 0x800) lets two draws on the staff member's traits
// (+0x67, +0x65) both come out zero, which registers the absence instead.
enum class Conversion { skipped, declined, registered };
Conversion convert_to_primary(Cpu& c, const Pool& pool, std::uint32_t records, std::uint32_t staff, std::uint32_t contract,
                              std::uint32_t flag, const Draw& first, const Draw& second, std::uint32_t registered_back) {
    if (compare(c, std::int32_t(c.u32(contract + contract_flags) & flag)) == 0) return Conversion::skipped;
    const auto roll = [&](std::uint32_t offset, const Draw& site) {
        const auto trait = signed_byte(c.u8(staff + offset));
        c.set({{3, trait}, {16, trait + 10}});
        draw_or_zero(c, pool, site);
        return compare(c, std::int32_t(c.r(15)));
    };
    if (roll(0x67, first) != 0) return Conversion::declined;
    if (roll(0x65, second) != 0) return Conversion::declined;
    enter(c, registered_back, {records, staff, 0, 0});
    register_absence(c, records, staff, 0, 0);
    return Conversion::registered;
}
}

void training_event(Cpu& c, std::uint32_t records, std::uint32_t staff) {
    Frame frame(c, 0x10007a50, 0xb0, {.first_gpr = 15});
    const auto pool = Pool::at(c);
    keep(c, {{17, c.toc(-0x7ba4)}, {26, records}, {18, c.toc(-0x7ba0)}, {31, staff}, {28, pool.constants}, {29, pool.salt},
             {25, pool.base}, {24, pool.end}, {30, pool.cursor}, {22, c.toc(-0x7b80)}, {23, c.toc(-0x7b7c)},
             {21, c.toc(-0x7b78)}, {20, c.toc(-0x7be8)}});
    call(c, date_construct, 0x10007aa0, {frame.at(0x3c)});
    // The constructor runs before the checks.
    if (compare_logical(c, staff) == 0) return;
    if (compare_logical(c, c.u32(staff + staff_profile)) == 0) return;
    // The original tests the staff pointer again here; it is known nonzero.
    compare_logical(c, staff);
    c.set({{3, c.toc(-0x7be4)}, {4, c.u32(staff)}});
    if (compare(c, std::int32_t(c.r(4)), c.i32(c.r(3))) >= 0) return;
    const auto contract_for = [&](std::uint32_t back) { return call(c, contract_of, back, {c.u32(c.r(23)), staff}); };
    // The staff member's club, through the contract; the contract is cached.
    const auto contract = contract_for(0x10007ae0);
    compare_logical(c, contract);
    c.put32(c.r(21), contract);
    const auto club = club_of_contract(c, contract);
    c.set(27, club);
    if (compare_logical(c, club) == 0) return;
    // Already on a training absence.
    c.set(3, c.u32(records + flags_buffer));
    if (compare(c, std::int32_t(c.u8(c.r(3) + c.u32(staff)) & secondary_bit)) != 0) return;

    const auto current = contract_for(0x10007b2c);
    c.set(19, current);
    // A contract with a low enough mood (-100 or less) may turn into a primary
    // absence instead; a null contract, a better mood, or no conversion that
    // registers, all go on to the training absence.
    bool converted = false;
    if (compare(c, std::int32_t(current)) != 0 && compare(c, c.i8(current + contract_mood), -100) <= 0) {
        const auto primary = convert_to_primary(c, pool, records, staff, current, 0x2000,
            {.start = 0x10007b68, .bound = 16, .result = 15 },
            {.start = 0x10007cd0, .bound = 16, .result = 15 },
            0x10007e34);
        converted = primary == Conversion::registered;
        if (!converted) {
            converted = convert_to_primary(c, pool, records, staff, current, 0x800,
                {.start = 0x10007e5c, .bound = 16, .result = 15 },
                {.start = 0x10007fc4, .bound = 16, .result = 15 },
                0x10008128) == Conversion::registered;
        }
    }
    if (converted) return;

    // Flag the training absence and tell the club.
    const auto flags = c.u32(records + flags_buffer), id = c.u32(staff);
    c.put8(flags + id, c.u8(flags + id) | secondary_bit);
    const auto after = call(c, contract_of, 0x1000814c, {c.u32(c.r(23)), staff, flags});
    compare_logical(c, after);
    c.put32(c.r(21), after);
    const auto reporting = club_of_contract(c, after);
    c.set(5, reporting);
    enter(c, 0x1000817c, {records, staff, reporting});
    notify_training(c, staff, reporting);

    // The event's fitness cost: a digit added to the staff member's record
    // byte (it wraps), then a draw on the profile's ability.
    pool.advance(0x1000819c);
    const auto sum = current_sum(c, pool);
    // The id is read again here and for the record: the calls above can change it.
    const auto record = c.u32(c.toc(-0x7b9c)) + c.u32(staff) * daily_record_size;
    const auto digit = std::uint32_t(remainder_by(sum, 2, 10).value) & 0xff;
    const auto before = c.u8(record + daily_record_counter);
    c.put8(record + daily_record_counter, before + digit);
    const auto clock = call(c, 0x101878d0, 0x10008208, {c.u32(c.r(18)), staff, 0, sum, before, record});
    c.set(4, c.u32(staff + staff_profile));
    c.set(20, clock);
    const auto ability = extended(std::int16_t((21 - std::uint32_t(c.i8(c.r(4) + record_trait))) * 25));
    c.set(19, ability);
    draw_or_zero(c, pool, {.start = 0x10008230, .bound = 19, .result = 15 });
    const auto padded = extended(std::int16_t(c.r(19) + c.r(15) + 100));
    call(c, 0x10186a20, 0x1000839c, {c.u32(c.r(18)), staff, extended(std::int16_t(clock - padded)), 0});

    // The dated record of the absence (sixteen bytes).
    const auto date = c.r(22);
    const auto entry = frame.at(0x38), stored_id = c.u32(staff);
    c.put32(entry, stored_id);
    c.put16(entry + 6, c.u16(date + 2));
    c.put16(entry + 4, c.u16(date));
    c.put8(entry + 8, c.u8(date + 4));
    call(c, list_append, 0x100083cc, {c.u32(records + secondary_list), entry, 0x10, c.u16(date + 2), stored_id});

    // Discipline: a bad day for the club's manager may fine or warn the staff.
    call(c, 0x1014c2e0, 0x100083e4, {club, 0, 1, 0, 0});
    if (compare(c, std::int8_t(c.r(3))) != 0) return;
    c.set(3, c.u32(club + club_second));
    if (compare_logical(c, c.r(3)) == 0) return;
    c.set(3, c.u32(c.r(3) + staff_record));
    if (compare_logical(c, c.r(3)) == 0) return;
    c.set(16, signed_byte(c.u8(c.r(3) + record_trait)));
    compare(c, std::int32_t(c.r(16)));
    draw_or_zero(c, pool, {.start = 0x10008418, .bound = 16, .result = 15 });
    if (compare(c, std::int32_t(c.r(15))) == 0) return;
    const auto manager = c.u32(c.u32(club + club_second) + staff_record);
    call(c, 0x100d2f50, 0x10008578, {manager, c.u8(manager + record_coded_trait)});
    c.set(16, signed_byte(c.r(3)));
    compare(c, std::int32_t(c.r(16)));
    draw_or_zero(c, pool, {.start = 0x10008588, .bound = 16, .result = 15 });
    pool.advance(0x100086f0);
    const auto tail = remainder_by(current_sum(c, pool), 1, 5);
    const auto action = compare(c, std::int32_t(c.r(15)), tail.value) > 0 ? 1u : 2u;
    call(c, 0x1010a080, action == 1 ? 0x10008744 : 0x1000875c, {c.u32(c.r(17)), staff, action, 1});
}

// ---- fitness_event ----

namespace {
// What the fitness concern does once its notice is out, in the same shape
// whether or not a reporter was found: two draws on the manager's traits
// decide whether a fine (kind 1) or a warning (kind 2) goes out.
// `at` is the first call of the copy (0x10009148 or 0x10009758); the copies lie
// 0x610 apart and differ only in the first draw's stack slots.
void discipline(Cpu& c, const Pool& pool, std::uint32_t club, std::uint32_t staff, std::uint32_t at) {
    call(c, 0x1014c2e0, at + 0x18, {club, 0, 1, 0, 0});
    if (compare(c, std::int8_t(c.r(3))) != 0) return;
    c.set(3, c.u32(club + club_second));
    if (compare_logical(c, c.r(3)) == 0) return;
    c.set(3, c.u32(c.r(3) + staff_record));
    if (compare_logical(c, c.r(3)) == 0) return;
    c.set(22, signed_byte(c.u8(c.r(3) + record_trait)));
    compare(c, std::int32_t(c.r(22)));
    draw_or_zero(c, pool, {.start = at + 0x4c, .bound = 22, .result = 24 });
    pool.advance(at + 0x1b4);
    const auto first = current_sum(c, pool);
    const auto roll = remainder_by(first, 2, 10);
    leave(c, {{3, roll.sign}, {4, first}, {5, pool.position()}});
    if (compare(c, std::int32_t(c.r(24)), roll.value) <= 0) return;
    const auto manager = c.u32(c.u32(club + club_second) + staff_record);
    c.set(22, signed_byte(call(c, 0x100d2f50, at + 0x204, {manager, c.u8(manager + record_coded_trait)})));
    compare(c, std::int32_t(c.r(22)));
    draw_or_zero(c, pool, {.start = at + 0x214, .bound = 22, .result = 23 });
    pool.advance(at + 0x37c);
    const auto tail = remainder_by(current_sum(c, pool), 1, 5);
    const auto kind = compare(c, std::int32_t(c.r(23)), tail.value) > 0 ? 1u : 2u;
    call(c, 0x1010a080, at + (kind == 1 ? 0x3d0 : 0x3e8), {c.u32(c.r(27)), staff, kind, 1});
}
}

void fitness_event(Cpu& c, std::uint32_t records, std::uint32_t staff, std::uint32_t club) {
    Frame frame(c, 0x100090a0, 0x180, {.first_gpr = 21});
    const auto pool = Pool::at(c);
    keep(c, {{27, c.toc(-0x7ba4)}, {24, records}, {22, c.toc(-0x7bf0)}, {25, staff}, {23, pool.constants}, {26, club},
             {28, pool.salt}, {29, pool.base}, {30, pool.end}, {31, pool.cursor}, {21, c.toc(-0x7b80)}});
    call(c, date_construct, 0x100090e8, {frame.at(0x40)});
    if (compare_logical(c, staff) == 0) return;
    if (compare_logical(c, club) == 0) return;
    // The twenty-byte record of the concern.
    const auto date = c.r(21);
    const auto entry = frame.at(0x38);
    c.put32(entry, c.u32(staff));
    c.put32(entry + 4, c.u32(club));
    c.put16(entry + 10, c.u16(date + 2));
    c.put16(entry + 8, c.u16(date));
    c.put8(entry + 12, c.u8(date + 4));
    call(c, list_append, 0x10009134, {c.u32(records + pending_list), entry, 0x14, c.u16(date + 2), c.u32(club)});
    const auto announced = call(c, 0x102cdbf0, 0x10009140, {c.u32(c.r(22)), club});
    if (compare(c, std::int8_t(announced)) == 0) {
        discipline(c, pool, club, staff, 0x10009148);
        return;
    }

    // The club has been told before: pick a reporter among five slots of the
    // club (+0x160), retrying until a reporter exists or a draw lands on 0
    // (mod 20). The reporter is whoever the last try found; with none the
    // fields name nobody. Quirk: with no reporter at all and no draw landing
    // on 0 the loop never ends (the original stops only when its budget does).
    c.set({{3, 0x66660000}, {24, 0x66666667}});
    for (;;) {
        pool.advance(0x1000955c);
        const auto cursor = pool.position();
        const auto word_sum = c.u16(cursor) + pool.bias();
        const auto slot = remainder_by(word_sum, 1, 5);
        const auto next = cursor + 2;
        c.set(21, c.u32(club + (std::uint32_t(slot.value) << 2) + 0x160));
        c.set({{3, next}, {4, std::uint32_t(slot.value) << 2}, {5, word_sum}});
        const auto beyond = compare_logical(c, next, c.u32(pool.end)) > 0;
        c.put32(pool.cursor, next);
        if (beyond) {
            c.put32(pool.cursor, pool.base);
            c.call(reseed, 0x100095b4);
            c.put32(pool.salt, c.r(3) & 0xffff);
        }
        const auto sum = current_sum(c, pool);
        const auto grade = remainder_by(sum, 3, 20);
        leave(c, {{3, grade.sign}, {4, sum}});
        if (compare(c, grade.value) == 0) break;
        if (compare_logical(c, c.r(21)) != 0) break;
    }
    const auto reporter = c.r(21);
    const auto message = frame.at(0x4c);
    const auto field = [&](std::uint32_t index, std::uint32_t value, std::uint32_t back) { call(c, 0x102cb4b0, back, {message, index, value}); };
    const auto name = [&](std::uint32_t who, std::uint32_t offset) { return c.u32(c.u32(who + offset) + 0x20); };
    call(c, 0x102bdf90, 0x10009600, {message, 0x138f, 0});
    field(0, c.u32(staff), 0x10009610);
    field(1, c.u32(club), 0x10009620);
    pool.advance(0x10009640);
    // The sign-folded parity of the next word (-1, 0 or 1).
    field(2, std::uint32_t(std::int32_t(current_sum(c, pool)) % 2), 0x10009674);
    field(3, name(staff, 0x28), 0x10009688);
    field(4, name(staff, 0x2c), 0x1000969c);
    field(5, name(staff, 0x30), 0x100096b0);
    if (compare_logical(c, reporter) != 0) {
        field(6, c.u32(reporter), 0x100096c8);
        field(7, name(reporter, 0x28), 0x100096dc);
        field(8, name(reporter, 0x2c), 0x100096f0);
        field(9, name(reporter, 0x30), 0x10009704);
    } else {
        field(6, 0xffffffff, 0x10009718);
        field(7, 0xffffffff, 0x10009728);
        field(8, 0xffffffff, 0x10009738);
        field(9, 0xffffffff, 0x10009748);
    }
    call(c, 0x102cc140, 0x10009758, {c.u32(c.r(22)), message, club});
    discipline(c, pool, club, staff, 0x10009758);
}
}

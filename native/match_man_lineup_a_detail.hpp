#pragma once
// The lineup_a block's record offsets and the routine's working state. Meanings
// marked "inferred" come from how the code uses a field; the offsets are the
// original's.
#include "guest_values.hpp"
#include "match_man_adapter.hpp"
#include "match_man_detail.hpp"

namespace cm::aot::match_man {
// The side record of a match (r24 of the routine).
namespace lineup_side {
constexpr std::uint32_t match = 0x14;       // the match object (its +0x3118 word points at the settings record)
constexpr std::uint32_t changed = 0x18;     // byte: set when a swap is wanted (inferred)
constexpr std::uint32_t slot = 0x1a;        // byte: the side's number (0 or 1)
constexpr std::uint32_t human = 0x1b;       // byte: nonzero for a human-managed side
constexpr std::uint32_t urgency = 0x1c;     // byte 0-3, set by the strength comparison
constexpr std::uint32_t manager = 0x20;     // the manager's person record (inferred)
constexpr std::uint32_t assistant = 0x24;   // staff record the final flags consult (inferred)
constexpr std::uint32_t squad_list = 0x2c;  // 50 person pointers
constexpr std::uint32_t tactic = 0x30;      // the tactic object the final flags are set on
constexpr std::uint32_t club = 0x34;
constexpr std::uint32_t extras = 0x3c;      // record with the bench size at +0x4e and a word at +0 (-2: none)
constexpr std::uint32_t choice = 0x40;      // result of the picker 0x102102d0
constexpr std::uint32_t picks = 0x8;        // three words of the best candidate of each pass
}
namespace lineup_extras {
constexpr std::uint32_t kind = 0x0;
constexpr std::uint32_t reference = 0x14;
constexpr std::uint32_t kind_byte = 0x3a;
}
namespace lineup_person {
constexpr std::uint32_t id = 0x0;
constexpr std::uint32_t player = 0x14;
constexpr std::uint32_t age = 0x5c;
constexpr std::uint32_t status = 0x6a;   // byte of bits (0x1, 0x2, 0x4, 0x8 are tested)
}
// The pool at TOC -0x5278 (doubles), by offset.
namespace lineup_pool {
constexpr std::uint32_t floor = 0x0, divisor = 0x8, ceiling = 0x10, conversion = 0x90;
}
// MATCH_MAN.CPP's entries called, by role.
namespace lineup_entries {
constexpr std::uint32_t picker = 0x102102d0, arranger = 0x10210c50, orderer = 0x1020d7d0, slotter = 0x1020e8f0;
constexpr std::uint32_t refresher = 0x1020a290, classifier = 0x1020c5d0, deployer = 0x1021a240;
}

// The routine's working state. r13-r31 are kept in the guest registers, as the
// original holds them across its calls (the callees save them in the stack
// image); the phases give them the names of their roles there.
class ComputerLineup {
public:
    ComputerLineup(guest::Cpu& cpu, std::uint32_t side, std::uint32_t team, std::uint32_t mode);
    std::uint32_t run();

    guest::Cpu& c;
    guest::Frame frame;
    guest::Fpu fpu;
    guest::Kept<13> r13;
    guest::Kept<14> r14;
    guest::Kept<15> r15;
    guest::Kept<16> r16;
    guest::Kept<17> r17;
    guest::Kept<18> r18;
    guest::Kept<19> r19;
    guest::Kept<20> r20;
    guest::Kept<21> r21;
    guest::Kept<22> r22;
    guest::Kept<23> r23;
    guest::Kept<24> r24;
    guest::Kept<25> r25;
    guest::Kept<26> r26;
    guest::Kept<27> r27;
    guest::Kept<28> r28;
    guest::Kept<29> r29;
    guest::Kept<30> r30;
    guest::Kept<31> r31;

    // Values as registers hold them after extsb, extsh.
    static std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
    static std::int32_t s16(std::uint32_t value) { return std::int16_t(value); }

    // Stack addresses.
    std::uint32_t at(std::int32_t offset) const { return frame.at(offset); }
    std::uint32_t candidates() const { return at(0x504); }   // 50 person pointers
    std::uint32_t slot_sets() const { return at(0x200); }    // preset halfwords per slot
    std::uint32_t entered() const { return at(0x25c); }
    std::uint32_t ordered() const { return at(0x694); }      // 50 words
    // Values the routine reads again from its own frame.
    std::uint32_t team() const { return c.u32(at(0x97c)); }  // the caller's parameter slot
    std::uint32_t side() const { return r24; }
    std::uint32_t club() const { return c.u32(side() + lineup_side::club); }
    std::uint32_t pool() const { return c.toc(-0x5278); }
    std::uint8_t human_side() const { return c.u8(side() + lineup_side::human); }
    std::int32_t mode() const { return std::int8_t(std::uint32_t(r25)); }

    // Phases, in the order the original runs them.
    void begin();
    void assess_replacement();
    void opponent_strength();
    void apply_date_factor();
    void set_urgency();
    void first_pass();
    void preset_slots();
    void first_scan();
    void first_arrangement();
    void second_scan();
    void second_candidate(std::uint32_t person);
    void finish_candidate(std::uint32_t person);
    void claim(std::uint32_t person, std::uint32_t first_back);
    void choose_flags();
    void reorder_candidates();
    void fill_slots();
    void fill_one_slot();
    void second_arrangement();
    void check_duplicates();
    void rotate_lists();
    void final_picks();
    void tail_flags();
    void flag(std::uint32_t value, std::uint32_t bl_address);
    bool assistant_below(std::uint32_t base);

    // Helpers.
    double convert(std::uint32_t at_address, std::int32_t value);
    std::uint32_t scaled(std::uint32_t value, std::uint32_t factor, std::uint32_t convert_at = 0x8ec, std::uint32_t store_at = 0x8f4);
    std::int32_t rating_of(std::uint32_t club_record, std::uint32_t row) const;
    bool is_club_slot(std::uint32_t club_record, std::uint32_t person);
    std::uint32_t draw(std::uint32_t back);
    void report(std::uint32_t at_address, std::uint32_t line, std::int32_t name, std::int32_t extension);
    void assert_that(bool condition, std::uint32_t at_address, std::uint32_t line, std::int32_t name, std::int32_t extension);
    bool skip_by_age(std::uint32_t person);
    std::uint32_t club_of_owner(std::uint32_t person, std::uint32_t back);
};
}

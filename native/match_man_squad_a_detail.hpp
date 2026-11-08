#pragma once
// MATCH_MAN.CPP squad_a block: record offsets and constants. Meanings are
// inferred from how 0x1020a290 uses the fields.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::match_man {
// The side record (r3 of 0x1020a290; match_man_callbacks_detail.hpp has more of it).
namespace squad_side {
constexpr std::uint32_t others[3] = {0x8, 0xc, 0x10};  // three persons (compared with a player's person)
constexpr std::uint32_t match = 0x14;
constexpr std::uint32_t own_side = 0x1a;   // signed byte: which of the match's two player tables
constexpr std::uint32_t kind = 0x1b;       // signed byte
constexpr std::uint32_t judge = 0x20;      // a person handed to 0x100d2a80
constexpr std::uint32_t staff = 0x24;      // the manager's staff data (0x100d2f50, bytes 0x2e..0x3f)
constexpr std::uint32_t orders = 0x30;     // the side's orders object (0x10113b60 and friends; +0x588, +0x1880 flags)
constexpr std::uint32_t club = 0x34;       // its club (rating halfword at club+0x1ec)
constexpr std::uint32_t rival_club = 0x38;
constexpr std::uint32_t extras = 0x3c;     // record with a signed byte at +0x3a
}
namespace squad_staff {
constexpr std::uint32_t id = 0x0;          // first word
constexpr std::uint32_t b2e = 0x2e, b33 = 0x33, b35 = 0x35, b3c = 0x3c, b3f = 0x3f;
}
namespace squad_club {
constexpr std::uint32_t rating = 0x1ec;    // halfword
constexpr std::uint32_t person = 0x90;     // a person record (word 0 = id, byte 0x6b a flag)
}
// A player record of the match (0x1c4 bytes at match + 0x311c + side * 0x2350).
namespace squad_player {
constexpr std::uint32_t slot = 0x19;       // signed byte; negative: no player
constexpr std::uint32_t opponent_key = 0x27;
constexpr std::uint32_t form = 0x50;       // float
constexpr std::uint32_t person = 0x6c;
constexpr std::uint32_t data = 0x70;       // the player's staff-like data record
constexpr std::uint32_t flag24 = 0x24;
constexpr std::uint32_t table_stride = 0x2350, first = 0x311c, size = 0x1c4;
}
// Bytes of the data record at player+0x70.
namespace squad_data {
constexpr std::uint32_t b12 = 0x12, b13 = 0x13, b14 = 0x14, b15 = 0x15, b17 = 0x17, b18 = 0x18, b19 = 0x19,
                        b1a = 0x1a, b2e = 0x2e, b3c = 0x3c, b40 = 0x40, b44 = 0x44;
constexpr std::uint32_t ability_a = 0xa;   // halfword
}
// Entries of other modules the routine calls.
namespace squad_entries {
constexpr std::uint32_t attribute = 0x100d1cb0, judge_attribute = 0x100d2a80, staff_attribute = 0x100d2f50,
                        place = 0x10111310, orders_flags = 0x10113b60, orders_query = 0x10113e10,
                        orders_apply = 0x10113e30, mark_pair = 0x10114b50, registry_check = 0x10147e70,
                        registry_act = 0x10149df0, opponent_index = 0x101fd100, role = 0x10289560,
                        role_more = 0x102895e0, rating_of = 0x10303560, notice = 0x103b14d0, power = 0x10780fc0,
                        random = 0x1077b770;
}
// Offsets into the pool of doubles at TOC -0x5278 and the singles at TOC -0x527c.
namespace squad_pool {
constexpr std::uint32_t conversion = 0x90;  // 2^52 + 2^31
}
// A double the original keeps in non-volatile FPR N across its calls.
template <unsigned N>
class KeptDouble {
    static_assert(N >= 14 && N <= 31, "KeptDouble: not a non-volatile register");

public:
    // Without a value the register keeps what the caller left: the original only saves it.
    explicit KeptDouble(guest::Cpu& cpu) : c(cpu) {}
    KeptDouble(guest::Cpu& cpu, double value) : c(cpu) { c.set_f(N, value); }
    KeptDouble(const KeptDouble&) = delete;
    KeptDouble& operator=(double value) {
        c.set_f(N, value);
        return *this;
    }
    operator double() const { return c.f(N); }

private:
    guest::Cpu& c;
};
}

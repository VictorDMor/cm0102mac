#include "club_records.hpp"
#include "guest_values.hpp"
#include <array>
#include <span>

namespace cm::aot::club_records {
using namespace guest;
namespace {
constexpr std::array<unsigned,38> club_dates{
    0x1c,0x44,0x6c,0x98,0xbc,0xe0,0x104,0x128,0x150,0x158,
    0x164,0x16c,0x178,0x180,0x18c,0x194,0x1b0,0x1d0,0x1f0,
    0x1f8,0x230,0x250,0x270,0x290,0x2b0,0x2cc,0x2d4,0x2ec,
    0x2f4,0x318,0x330,0x338,0x370,0x38c,0x398,0x3b8,0x3c4,0x3f0};
}

std::uint32_t initialize_dates(Cpu& c, std::uint32_t record, std::uint32_t entry) {
    Frame frame(c, entry, 0x40, {.first_gpr=31});
    Kept<31> object(c, record);
    // Original constructors only initialize their embedded dates; counters,
    // identifiers and padding keep the caller's existing bytes.
    const auto initialize = [&](unsigned offset, std::uint32_t back) {
        c.set(3, object + offset); c.call(0x10715880, back);
    };
    if (entry == 0x10012d30) {
        auto back = entry + 0x1c;
        for (auto offset : club_dates) { initialize(offset, back); back += 8; }
    } else {
        switch (entry) {
        case 0x100190b0: initialize(0x10, entry+0x1c); break;
        case 0x100190f0: case 0x10019130:
            initialize(0x14,entry+0x1c); initialize(0x20,entry+0x24); break;
        case 0x10019170: case 0x10019230: case 0x10019270:
        case 0x100192b0: case 0x100192f0: case 0x10019330: case 0x10019370:
        case 0x100193f0: case 0x10019430: initialize(0x14,entry+0x1c); break;
        case 0x100191b0: case 0x100191f0:
            initialize(0,entry+0x18); initialize(0x18,entry+0x20); initialize(0x20,entry+0x28); break;
        case 0x100193b0: initialize(0x14,entry+0x1c); initialize(0x1c,entry+0x24); break;
        case 0x10019470: case 0x100194b0: case 0x100194f0: case 0x10019530:
            initialize(4,entry+0x1c); initialize(12,entry+0x24); break;
        case 0x10019570: case 0x100195a0: case 0x100195d0: case 0x10019600: case 0x10019630:
            initialize(0,entry+0x18); break;
        case 0x10019660: case 0x100196a0: case 0x100196e0: initialize(4,entry+0x1c); break;
        default: throw std::logic_error("Unknown club-record date constructor");
        }
    }
    c.set(3,object); return object;
}

std::uint32_t destroy_record(Cpu& c, std::uint32_t object, std::uint32_t deleting, std::uint32_t entry) {
    Frame frame(c,entry,0x40,{.first_gpr=31}); Kept<31> record(c,object);
    if (compare(c,record) != 0 && compare(c,std::int16_t(deleting)) > 0) {
        c.set(3,record); c.call(0x107735a0,entry == 0x100163b0 ? 0x100163d4 : 0x1001dac8);
    }
    c.set(3,record); return record;
}

std::uint32_t has_pending_record(Cpu& c,std::uint32_t object,std::uint32_t club,std::uint32_t kind) {
    const auto record_kind=std::int32_t(std::int8_t(kind));
    std::uint32_t index=0, offset=0;
    c.set({{5,std::uint32_t(record_kind)},{8,0},{6,0}});
    while (compare(c,std::int32_t(index),c.i32(object+0x2c)) < 0) {
        const auto row=c.u32(object+0x24)+offset; c.set(7,row);
        if (compare(c,std::int32_t(club),c.i32(row)) == 0 && compare(c,record_kind,c.i8(row+4)) == 0) {
            c.set(3,1); return 1;
        }
        offset+=8; ++index; c.set({{6,offset},{8,index}});
    }
    c.set(3,0); return 0;
}
}

#include "club_records.hpp"
#include <initializer_list>

namespace cm::aot::club_records {
namespace {
// These are guest record views, not host structures. Padding remains untouched.
// Reads happen immediately before their writes so overlapping assignment has
// the same forward field semantics as the original operators.
struct Assignment {
    Cpu& c;
    std::uint32_t destination, source;
    void byte(unsigned offset) const { c.put8(destination + offset, c.u8(source + offset)); }
    void half(unsigned offset) const { c.put16(destination + offset, c.u16(source + offset)); }
    void word(unsigned offset) const { c.put32(destination + offset, c.u32(source + offset)); }
    void words(std::initializer_list<unsigned> offsets) const { for (auto offset : offsets) word(offset); }
    Assignment at(unsigned offset) const { return {c, destination + offset, source + offset}; }
    void date() const { half(2); half(0); byte(4); }
    void people() const { words({0, 4, 8, 12}); }
    void result() const {
        word(0); half(4); half(6); byte(8);
        words({12, 16}); byte(20); byte(21); word(24);
    }
    void player_total() const { people(); at(16).date(); half(24); }
    void player_count() const { half(0); at(4).people(); at(20).date(); }
    void transfer() const { words({0,4,8,12,16}); at(20).date(); word(28); at(32).date(); byte(40); }
    void match() const { date(); at(8).people(); at(24).date(); at(32).date(); at(40).result(); }
    void count() const { byte(0); at(4).people(); at(20).date(); half(28); }
    void score() const { byte(0); at(4).people(); at(20).date(); at(28).date(); at(36).result(); }
    void dated_total() const { word(0); at(4).date(); at(12).date(); }
    void dated_match() const { date(); at(8).result(); }
    void dated_score() const { word(0); at(4).date(); at(12).result(); }
    void position() const { byte(0); word(4); half(8); }
    void money() const { word(0); half(4); }
    void average() const {
        // lfs/stfs quiets signaling NaNs while retaining sign and payload.
        // A C++ float/double/float round trip can be folded to a raw copy.
        auto average = c.u32(source);
        if ((average & 0x7f800000) == 0x7f800000 && (average & 0x007fffff)) average |= 0x00400000;
        c.put32(destination, average);
        at(4).people(); at(20).date(); byte(28); half(30);
    }
    void club() const {
        position(); at(0xc).position();
        for (auto offset : {0x18u,0x40u,0x68u}) at(offset).dated_score();
        at(0x90).money();
        for (auto offset : {0x98u,0xbcu,0xe0u,0x104u,0x128u}) at(offset).dated_match();
        for (auto offset : {0x14cu,0x160u,0x174u,0x188u}) at(offset).dated_total();
        at(0x19c).count(); at(0x1bc).count(); at(0x1dc).score();
        at(0x21c).count(); at(0x23c).average(); at(0x25c).count();
        at(0x27c).byte(0); at(0x27c).byte(1); at(0x280).player_total();
        at(0x29c).player_count(); at(0x2b8).player_count();
        at(0x2d4).match(); at(0x318).match();
        at(0x35c).player_count(); at(0x378).transfer();
        at(0x3a4).transfer(); at(0x3d0).money();
        at(0x3d8).money(); at(0x3e0).player_total();
    }
};
}

std::uint32_t assign(Cpu& c, std::uint32_t destination, std::uint32_t source, RecordKind kind) {
    const Assignment record{c, destination, source};
    switch (kind) {
    case RecordKind::player_total: record.player_total(); break;
    case RecordKind::money: record.money(); break;
    case RecordKind::transfer: record.transfer(); break;
    case RecordKind::player_count: record.player_count(); break;
    case RecordKind::match: record.match(); break;
    case RecordKind::two_scores: record.byte(0); record.byte(1); record.at(4).player_total(); break;
    case RecordKind::count: record.count(); break;
    case RecordKind::average: record.average(); break;
    case RecordKind::score: record.score(); break;
    case RecordKind::dated_total: record.dated_total(); break;
    case RecordKind::dated_match: record.dated_match(); break;
    case RecordKind::dated_score: record.dated_score(); break;
    case RecordKind::position: record.position(); break;
    case RecordKind::club: record.club(); break;
    }
    return destination;
}
}

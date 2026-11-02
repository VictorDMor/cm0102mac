// The press's reaction to a player's sale: the selling and the buying club, the player (a staff
// member by three name parts) and a second person involved.
#include "media_text_mid.hpp"
#include "media_text_mid_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::text;

namespace {
constexpr std::uint32_t sprintf_entry = 0x1077b720;
}

void player_sale_text(Cpu& c, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x106a8110, 0x2180, {.first_gpr = 13});
    keep(c, {{16, record}, {29, c.toc(user_slot)}, {30, c.toc(staff_slot)}, {15, output}, {31, c.toc(slot::pool)}});
    c.put8(frame.at(0x21a3), full);
    constexpr std::uint32_t backs[]{0x106a814c, 0x106a8160, 0x106a8174, 0x106a8188, 0x106a819c, 0x106a81b0, 0x106a81c4, 0x106a81d8, 0x106a81ec,
                                    0x106a8200, 0x106a8214, 0x106a8228, 0x106a823c, 0x106a8250, 0x106a8264, 0x106a8278, 0x106a828c};
    RecordReader read{.c = c, .record = record, .cursor = 14, .backs = backs};
    Kept<14> cursor(c, 0);
    const auto buyer_id = frame.at(0x212c), buyer_first = frame.at(0x2128), buyer_middle = frame.at(0x2124), buyer_last = frame.at(0x2120),
               seller_id = frame.at(0x211c);
    c.put32(buyer_id, read.next());
    c.put32(buyer_first, read.next());
    c.put32(buyer_middle, read.next());
    c.put32(buyer_last, read.next());
    Kept<26> staff_id(c, read.next());
    Kept<25> first(c, read.next());
    Kept<24> middle(c, read.next());
    Kept<23> last(c, read.next());
    Kept<22> other(c, read.next());
    Kept<21> o_first(c, read.next());
    Kept<20> o_middle(c, read.next());
    Kept<19> o_last(c, read.next());
    Kept<13> buying_club_id(c, read.next());
    c.put32(seller_id, read.next());
    Kept<18> story(c, read.next());
    Kept<17> outlet_kind(c, read.next());
    cursor = read.next();
    // The last field is read with r14 already holding the previous one.
    Kept<16> type(c, Program{c}.call(entry::read_field, 0x106a829c, {record, 17}));
    Kept<28> buying(c, table(c, -0x7be8) + std::uint32_t(buying_club_id) * club_size);
    Kept<27> selling(c, table(c, -0x7be8) + c.u32(seller_id) * club_size);
    const Club club_a{c, buying}, club_b{c, selling};
    const auto name = frame.at(0xc04), short_name = frame.at(0xa74), buyer_name = frame.at(0x8e4), buyer_short = frame.at(0x754),
               other_name = frame.at(0x5c4), other_short = frame.at(0x434), text = frame.at(0xd94);
    Program program{c};
    program.call(entry::person_name, 0x106a82e8, {first, middle, last, name, 0x190, 1, staff_id});
    program.call(entry::person_name, 0x106a8308, {first, middle, last, short_name, 0x190, 5, staff_id});
    program.call(entry::person_name, 0x106a8328, {c.u32(buyer_first), c.u32(buyer_middle), c.u32(buyer_last), buyer_name, 0x190, 1, c.u32(buyer_id)});
    program.call(entry::person_name, 0x106a8348, {c.u32(buyer_first), c.u32(buyer_middle), c.u32(buyer_last), buyer_short, 0x190, 5, c.u32(buyer_id)});
    program.call(entry::person_name, 0x106a8368, {o_first, o_middle, o_last, other_name, 0x190, 1, other});
    program.call(entry::person_name, 0x106a8388, {o_first, o_middle, o_last, other_short, 0x190, 5, other});
    describe_outlet(c, {0x106a8458, frame.at(0x240), frame.at(0x4c), frame.at(0x48), false, true}, outlet_kind, buying_club_id, cursor);
    const Output out{c, output};
    if (compare(c, signed_byte(c.u8(frame.at(0x21a3)))) == 0) {
        program.call(entry::person_name, 0x106a893c, {first, middle, last, name, 0x190, 5, staff_id});
        if (compare(c, std::int32_t(std::uint32_t(type)), 2) == 0) {
            if (compare(c, std::int32_t(std::uint32_t(story)), 2) < 0 && compare(c, std::int32_t(std::uint32_t(story)), 0) >= 0)
                format(c, text, 0x41efb, 0x106a8978, {other_short});
            else format(c, text, 0x41f3a, 0x106a8994, {short_name, other_short});
        } else program.call(sprintf_entry, 0x106a89a8, {text, c.toc(slot::pool) + 0x41960});
        out.set(text, 0x106a89b4);
        out.staff_link(staff_at(c, staff_id), first, middle, last, short_name, 0x106a89d4, 0x106a89f8);
        out.staff_link(staff_at(c, other), o_first, o_middle, o_last, other_short, 0x106a8a18, 0x106a8a3c);
        return;
    }
    if (compare(c, std::int32_t(std::uint32_t(type)), 2) == 0) {
        switch (story_of(c, std::int32_t(std::uint32_t(story)))) {
        case 0:
            { StackWords{c} << other_name << club_b.short_name(); format(c, text, 0x41985, 0x106a873c, {club_a.long_marker(), club_a.long_marker(), 0x64, club_b.short_marker(), club_b.short_marker(), club_a.long_name()}); }
            break;
        case 1:
            { StackWords{c} << other_name << club_b.long_name() << club_a.short_name(); format(c, text, 0x41ae0, 0x106a8790, {club_b.long_marker(), club_b.long_marker(), club_a.short_marker(), club_a.short_marker(), 0x64, buyer_name}); }
            break;
        case 2:
            { StackWords{c} << club_a.short_name() << other_name << name << buyer_short; format(c, text, 0x41bef, 0x106a87ec, {club_b.long_marker(), club_b.long_marker(), club_a.short_marker(), club_a.short_marker(), 0x64, club_b.long_name()}); }
            break;
        default:
            { StackWords{c} << club_a.long_name() << name << other_name << club_b.short_name(); format(c, text, 0x41d5a, 0x106a8848, {club_a.long_marker(), club_a.long_marker(), 0x64, 0x64, club_b.short_marker(), club_b.short_marker()}); }
            break;
        }
    } else program.call(sprintf_entry, 0x106a885c, {text, c.toc(slot::pool) + 0x41e71});
    out.set(text, 0x106a8868);
    out.link(club_a.short_name(), link_kind::club, club_a.at, 0x106a887c);
    out.link(club_a.long_name(), link_kind::club, club_a.at, 0x106a8890);
    out.link(club_b.short_name(), link_kind::club, club_b.at, 0x106a88a4);
    out.link(club_b.long_name(), link_kind::club, club_b.at, 0x106a88b8);
    const auto member = staff_at(c, staff_id);
    if (out.named(member, first, middle, last, 0x106a88d8)) {
        out.link(name, link_kind::staff, member, 0x106a88fc);
        out.link(short_name, link_kind::staff, member, 0x106a8918);
    }
}
}

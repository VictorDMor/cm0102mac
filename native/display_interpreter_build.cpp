// DISPLAY.CPP 0x1071de80: commands 6-8 (and command 8's eleven sub-commands). See display_interpreter.hpp.
//   6  create a GUI object from the stream (0x10725a00), then optionally a second one (the "linked" part)
//   7  a text and two words handed to 0x1076e5f0 (plus 0x1076e920 when the human row's flag is set)
//   8  change one field of a GUI object, by sub-command
// The stream holds little-endian values (the swap in the original); every read checks its length and
// reports "network.h" on failure, then carries on with what the original's variable held.
#include "display_interpreter.hpp"

namespace cm::aot::display {
namespace {
using std::int16_t;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

// Offsets in the human's row of the main object, and in a GUI object (0x17c bytes).
inline constexpr uint32_t game_flag_byte = 0x2c61;
namespace guio {
inline constexpr uint32_t flags = 0xc, text_pointer = 0x54, field_5c = 0x5c, field_4c = 0x4c, text = 0x78,
                          extra_copy = 0x60, link_index = 0x76;
}
namespace entry {
inline constexpr uint32_t copy_bytes = 0x10739ef0, create = 0x10725a00, copy_text = 0x1077dbf0,
                          block_move_data = 0x1077fc28, set_text_option = 0x1076e5f0, refresh = 0x1076e920,
                          set_option = 0x10726ae0, set_text = 0x107268c0;
}
inline constexpr uint32_t buffer_slot = 0x1714;  // the frame's scratch text buffer
inline constexpr int32_t move_slot = 0x4ec;      // 8 bytes copied into a GUI object (four halfwords)

constexpr Site net(uint32_t at, int32_t name, int32_t extension, uint32_t line) {
    return {at, name, extension, line, text::network_header};
}

// The stream reads of the original: a length check (assertion "network.h" at `site` when it fails; the
// position then stays) and a little-endian read, loaded byte by byte, that advances the position. A
// failed read leaves its out-parameter as it was.
class Reader {
public:
    explicit Reader(Interpreter& state)
        : s(state), c(state.c), live_toc(state.c.r(2)),
          stream(state.c, state.frame, state.stream,
                 // The assertion helpers read the table of contents through r2: give them the real one even
                 // when command 6 has loaded a stale r2 (see command_6), and put the stale one back.
                 [this] { s.regs.apply(); stale_toc = c.r(2); c.set(2, live_toc); },
                 [this] { c.set(2, stale_toc); }) {}
    bool s16(const Site& site, int16_t& out) { return get(2, site, out); }
    bool u16(const Site& site, uint16_t& out) { return get(2, site, out); }
    bool u32(const Site& site, uint32_t& out) { return get(4, site, out); }
    bool u8(const Site& site, uint8_t& out) { return get(1, site, out); }
    // Skips `size` bytes without reading them (0x10720d58).
    bool skip(uint32_t size, const Site& site) {
        if (!stream.reserve(size, site)) return false;
        stream.advance(size);
        return true;
    }
    uint32_t position() const { return stream.position(); }
    // The address of the next unread byte.
    uint32_t cursor() const { return stream.at_position(); }
    void advance(uint32_t size) { stream.advance(size); }

private:
    template <class T>
    bool get(unsigned size, const Site& site, T& out) {
        const auto value = stream.read_le_bytes(size, site);
        if (!value) return false;
        out = T(*value);
        return true;
    }
    Interpreter& s;
    Cpu& c;
    uint32_t live_toc, stale_toc = 0;
    CheckedStream stream;
};

// The values the original keeps in frame slots between reads. A read that fails leaves its slot as it
// was, so each starts from the frame's current content (what the original would load).
struct Slots {
    // flags: frame +0xb0; text_pointer: +0x1818; target_index: +0x1814 (the object later commands change or link to);
    // the others are values whose use is not named here: w_ac +0xac, w_a8 +0xa8, w_a4 +0xa4, w_a0 +0xa0, h1828..h181e
    // and u78/u76/u74, b181d/b181c are the slots at those frame offsets.
    uint32_t flags, w_ac, w_a8, w_a4, w_a0, text_pointer;
    int16_t h1828, h1826, h1824, h1822, h1820, h181e, target_index;
    uint16_t u78, u76, u74;
    uint8_t b181d, b181c;
    Slots(const Cpu& c, const Frame& f)
        : flags(c.u32(f.at(0xb0))), w_ac(c.u32(f.at(0xac))), w_a8(c.u32(f.at(0xa8))), w_a4(c.u32(f.at(0xa4))),
          w_a0(c.u32(f.at(0xa0))), text_pointer(c.u32(f.at(0x1818))), h1828(c.i16(f.at(0x1828))),
          h1826(c.i16(f.at(0x1826))), h1824(c.i16(f.at(0x1824))), h1822(c.i16(f.at(0x1822))),
          h1820(c.i16(f.at(0x1820))), h181e(c.i16(f.at(0x181e))), target_index(c.i16(f.at(0x1814))),
          u78(c.u16(f.at(0x78))), u76(c.u16(f.at(0x76))), u74(c.u16(f.at(0x74))), b181d(c.u8(f.at(0x181d))),
          b181c(c.u8(f.at(0x181c))) {}
};

uint32_t sign(int32_t v) { return uint32_t(v); }

// Copies `size` bytes of the stream to the frame's text buffer and skips them (0x10739ef0 with the
// position advanced by the same amount, also when the length read failed and `size` is stale).
void copy_stream_text(Interpreter& s, Reader& in, uint32_t size, uint32_t back) {
    s.regs.apply();
    invoke(s.c, entry::copy_bytes, back, {s.frame.at(buffer_slot), in.cursor(), size});
}

}  // namespace

std::uint32_t command_6(Interpreter& s) {
    Cpu& c = s.c;
    const Frame& frame = s.frame;
    Reader in(s);
    Slots v(c, frame);
    // Registers the command changes: object_index (the object's index), r14 (its record), text_length (text length),
    // linked_text_length (the second block's text length), halfword_word (a word read into a halfword slot).
    auto object_index = s.regs[13], text_length = s.regs[17], linked_text_length = s.regs[20], halfword_word = s.regs[22];
    int16_t ignored = 0;
    {
        int16_t index = 0;
        if (in.s16(net(0x10720170, 0x1014, 0x37c, 0x90), index)) {
            object_index = sign(index);
            s.regs.hold({{13, object_index}});
        }
    }
    in.u32(net(0x107201f0, 0xff4, 0x374, 0x92), v.flags);
    in.s16(net(0x10720278, 0xfd4, 0x36c, 0x90), v.h1828);
    in.s16(net(0x107202fc, 0xfb4, 0x364, 0x90), v.h1826);
    in.s16(net(0x10720380, 0xf94, 0x35c, 0x90), v.h1824);
    in.s16(net(0x10720404, 0xf74, 0x354, 0x90), v.h1822);
    in.u8(net(0x10720488, 0xf54, 0x34c, 0x8f), v.b181d);
    in.s16(net(0x10720500, 0xf34, 0x344, 0x90), v.h1820);
    in.u32(net(0x10720584, 0xf14, 0x33c, 0x92), v.w_ac);
    in.u16(net(0x1072060c, 0xef4, 0x334, 0x95), v.u78);
    in.u16(net(0x10720698, 0xed4, 0x32c, 0x95), v.u76);
    in.u32(net(0x10720724, 0xeb4, 0x324, 0x92), v.w_a8);
    in.u8(net(0x107207ac, 0xe94, 0x31c, 0x8f), v.b181c);
    in.u16(net(0x10720824, 0xe74, 0x314, 0x95), v.u74);
    {
        int16_t length = 0;
        if (in.s16(net(0x107208b0, 0x594, 0xdc, 0x90), length)) {
            text_length = sign(length);
            s.regs.hold({{17, text_length}});
        }
    }
    copy_stream_text(s, in, text_length, 0x10720920);
    in.advance(text_length);
    in.s16(net(0x10720954, 0xe54, 0x30c, 0x90), v.h181e);
    in.u32(net(0x107209d8, 0xe34, 0x304, 0x92), v.w_a4);
    in.u32(net(0x10720a60, 0xe14, 0x2fc, 0x93), v.w_a0);
    in.s16(net(0x10720ae8, 0xdf4, 0x2f4, 0x90), ignored);  // read and dropped
    in.u32(net(0x10720b78, 0xdd4, 0x2ec, 0x93), v.text_pointer);

    // The object's record: GUIO.CPP's constructor (0x10725a00) fills it from these values.
    const auto create = [&](uint32_t index, uint32_t call_back, bool second_block) {
        const auto record = guio_at(s.table, index);
        s.regs.hold({{14, record}});
        c.put32(frame.at(0x38), sign(v.h1822));
        c.put32(frame.at(0x3c), sign(int8_t(v.b181d)));
        c.put32(frame.at(0x40), sign(v.h1820));
        c.put32(frame.at(0x44), v.w_ac);
        c.put32(frame.at(0x48), v.u78);
        c.put32(frame.at(0x4c), v.u76);
        c.put32(frame.at(0x50), v.w_a8);
        c.put32(frame.at(0x54), sign(int8_t(v.b181c)));
        c.put32(frame.at(0x58), v.u74);
        c.put32(frame.at(0x5c), frame.at(buffer_slot));
        c.put32(frame.at(0x60), sign(v.h181e));
        c.put32(frame.at(0x64), v.w_a4);
        c.put32(frame.at(0x68), v.w_a0);
        c.put32(frame.at(0x6c), 0xffffffffu);
        if (second_block) {  // the second block overwrites r15 and r16 before the call (0x10721a68, 0x10721a90)
            s.regs.hold({{15, v.u78}, {16, sign(int8_t(v.b181c))}});
        }
        s.regs.apply();
        invoke(c, entry::create, call_back,
               {record, s.table + table::areas, s.table + table::guios, index, v.flags, sign(v.h1828), sign(v.h1826),
                sign(v.h1824)});
        return record;
    };
    // After the call: the text pointer and the two cleared words, and the text copy when asked for.
    const auto finish = [&](uint32_t record, uint32_t back) {
        c.put32(record + guio::text_pointer, v.text_pointer);
        c.put32(record + guio::field_5c, 0);
        c.put32(record + guio::field_4c, 0);
        if ((c.u32(record + guio::flags) & 8) != 0 && c.u32(record + guio::text_pointer) != 0) {
            s.regs.apply();
            invoke(c, entry::copy_text, back, {c.u32(record + guio::text_pointer), record + guio::text});
        }
    };
    const auto first = create(object_index, 0x10720c8c, false);
    finish(first, 0x10720cc0);

    in.s16(net(0x10720ce8, 0xdb4, 0x2e4, 0x90), v.target_index);
    if (v.target_index == -1) return object_index;
    in.skip(2, net(0x10720d78, 0xd94, 0x2dc, 0x90));
    // Four words into halfword slots: a failed read stores halfword_word as it is (0x10720e44).
    const auto halfword_slot = [&](const Site& site, int32_t slot) {
        uint32_t value = 0;
        if (in.u32(site, value)) {
            halfword_word = value;
            s.regs.hold({{22, halfword_word}});
        }
        c.put16(frame.at(slot), halfword_word);
    };
    halfword_slot(net(0x10720de4, 0xd74, 0x2d4, 0x92), 0x4ee);
    halfword_slot(net(0x10720e70, 0xd54, 0x2cc, 0x92), 0x4ec);
    halfword_slot(net(0x10720efc, 0xd34, 0x2c4, 0x92), 0x4f2);
    halfword_slot(net(0x10720f88, 0xd14, 0x2bc, 0x92), 0x4f0);
    c.put16(first + guio::link_index, uint16_t(v.target_index));
    s.regs.apply();
    invoke(c, entry::block_move_data, 0x10721004, {frame.at(move_slot), first + guio::extra_copy, 8});
    // 0x10721004 `lwz r2,0x14(r1)`: the original reloads r2 from the frame word that the call's glue is
    // meant to have saved. Where the call is answered without the glue it is whatever the frame held.
    c.set(2, c.u32(frame.at(0x14)));
    if (v.target_index == int32_t(object_index)) return object_index;

    // The linked object: the same values read again; its index is the one just read.
    in.u32(net(0x1072103c, 0xcf4, 0x2b4, 0x92), v.flags);
    in.s16(net(0x107210c4, 0xcd4, 0x2ac, 0x90), v.h1828);
    in.s16(net(0x10721148, 0xcb4, 0x2a4, 0x90), v.h1826);
    in.s16(net(0x107211cc, 0xc94, 0x29c, 0x90), v.h1824);
    in.s16(net(0x10721250, 0xc74, 0x294, 0x90), v.h1822);
    in.u8(net(0x107212d4, 0xc54, 0x28c, 0x8f), v.b181d);
    in.s16(net(0x1072134c, 0xc34, 0x284, 0x90), v.h1820);
    in.u32(net(0x107213d0, 0xc14, 0x27c, 0x92), v.w_ac);
    in.u16(net(0x10721458, 0xbf4, 0x274, 0x95), v.u78);
    in.u16(net(0x107214e4, 0xbd4, 0x26c, 0x95), v.u76);
    in.u32(net(0x10721570, 0xbb4, 0x264, 0x92), v.w_a8);
    in.u8(net(0x107215f8, 0xb94, 0x25c, 0x8f), v.b181c);
    in.u16(net(0x10721670, 0xb74, 0x254, 0x95), v.u74);
    {
        int16_t length = 0;
        if (in.s16(net(0x107216fc, 0x574, 0xd4, 0x90), length)) {
            linked_text_length = sign(length);
            s.regs.hold({{20, linked_text_length}});
        }
    }
    copy_stream_text(s, in, linked_text_length, 0x1072176c);
    in.advance(linked_text_length);
    in.s16(net(0x107217a0, 0xb54, 0x24c, 0x90), v.h181e);
    in.u32(net(0x10721824, 0xb34, 0x244, 0x92), v.w_a4);
    in.u32(net(0x107218ac, 0xb14, 0x23c, 0x93), v.w_a0);
    in.s16(net(0x10721934, 0xaf4, 0x234, 0x90), ignored);  // read and dropped
    in.u32(net(0x107219c4, 0xad4, 0x22c, 0x93), v.text_pointer);
    const auto second = create(sign(v.target_index), 0x10721adc, true);
    finish(second, 0x10721b10);
    return object_index;
}

std::uint32_t command_7(Interpreter& s) {
    Cpu& c = s.c;
    Reader in(s);
    auto text_length = s.regs[13], first_word = s.regs[17], second_word = s.regs[20];
    {
        uint32_t value = 0;
        if (in.u32(net(0x10721b40, 0xab4, 0x224, 0x92), value)) { first_word = value; s.regs.hold({{17, first_word}}); }
        if (in.u32(net(0x10721bc8, 0xa94, 0x21c, 0x92), value)) { second_word = value; s.regs.hold({{20, second_word}}); }
        int16_t length = 0;
        if (in.s16(net(0x10721c50, 0x554, 0xcc, 0x90), length)) { text_length = sign(length); s.regs.hold({{13, text_length}}); }
    }
    copy_stream_text(s, in, text_length, 0x10721cc0);
    in.advance(text_length);
    c.put8(s.table + table::unsent, 0);
    s.regs.apply();
    invoke(c, entry::set_text_option, 0x10721ce8, {s.frame.at(buffer_slot), first_word, second_word});
    if (c.i8(s.regs[14] + game_flag_byte) != 0) {
        s.regs.apply();
        invoke(c, entry::refresh, 0x10721cfc, {1});
    }
    return common_exit_result;
}

// Command 8: its sub-commands each change fields of the GUI object `v.target_index` (the first read).
std::uint32_t command_8(Interpreter& s) {
    Cpu& c = s.c;
    Reader in(s);
    Slots v(c, s.frame);
    auto sub_command = s.regs[13], text_length = s.regs[17];
    in.s16(net(0x10721d28, 0xa74, 0x214, 0x90), v.target_index);
    {
        uint8_t sub = 0;
        if (in.u8(net(0x10721dac, 0xa54, 0x20c, 0x8f), sub)) {
            sub_command = sub;
            s.regs.hold({{13, sub_command}});
        }
    }
    const auto gate = c.i16(s.table + table::announced_area_count);
    if (gate != 0) {
        if (c.i16(s.table + table::area_count) != gate) return common_exit_result;
        if (c.i16(s.table + table::guio_count) != c.i16(s.table + table::announced_guio_count)) return common_exit_result;
    }
    const auto sub = uint32_t(int32_t(int8_t(sub_command)));  // extsb, then an unsigned compare against 10
    if (sub > 10) return common_exit_result;
    const auto record = [&] { return guio_at(s.table, sign(v.target_index)); };
    switch (sub) {
    case 0:  // 0x10721e44: word at +0xc
        in.u32(net(0x10721e6c, 0xa34, 0x204, 0x92), v.flags);
        c.put32(record() + 0xc, v.flags);
        break;
    case 1:  // 0x10721ee8: four halfwords, each into two fields
        in.s16(net(0x10721f10, 0xa14, 0x1fc, 0x90), v.h1828);
        in.s16(net(0x10721f94, 0x9f4, 0x1f4, 0x90), v.h1826);
        in.s16(net(0x10722018, 0x9d4, 0x1ec, 0x90), v.h1824);
        in.s16(net(0x1072209c, 0x9b4, 0x1e4, 0x90), v.h1822);
        {
            const auto r = record();
            c.put32(r + 0x28, sign(v.h1828));
            c.put32(r + 0x2c, sign(v.h1824));
            c.put32(r + 0x30, sign(v.h1826));
            c.put32(r + 0x34, sign(v.h1822));
            c.put32(r + 0x10, sign(v.h1828));
            c.put32(r + 0x18, sign(v.h1824));
            c.put32(r + 0x14, sign(v.h1826));
            c.put32(r + 0x1c, sign(v.h1822));
        }
        break;
    case 2:  // 0x10722150: word at +0x38
        in.u32(net(0x10722178, 0x994, 0x1dc, 0x92), v.w_ac);
        c.put32(record() + 0x38, v.w_ac);
        break;
    case 3:  // 0x107221f4: halfword at +0x6a, then option 1
    case 4:  // 0x107222b0: halfword at +0x6c, then option 2
    case 7: {  // 0x107224a8: halfword at +0x70, then option 4
        const Site site = sub == 3 ? net(0x1072221c, 0x974, 0x1d4, 0x95)
                        : sub == 4 ? net(0x107222d8, 0x954, 0x1cc, 0x95)
                                   : net(0x107224d0, 0x8f4, 0x1b4, 0x95);
        const uint32_t field_offset = sub == 3 ? 0x6a : sub == 4 ? 0x6c : 0x70;
        const uint32_t option = sub == 3 ? 1 : sub == 4 ? 2 : 4;
        const uint32_t back = sub == 3 ? 0x107222ac : sub == 4 ? 0x10722368 : 0x10722560;
        in.u16(site, v.u78);
        const auto r = record();
        c.put16(r + field_offset, v.u78);
        s.regs.apply();
        invoke(c, entry::set_option, back, {r, c.u8(s.table + table::mode), option});
        break;
    }
    case 5:  // 0x1072236c: word at +0x3c
        in.u32(net(0x10722394, 0x934, 0x1c4, 0x92), v.w_a8);
        c.put32(record() + 0x3c, v.w_a8);
        break;
    case 6:  // 0x10722410: byte (sign-extended) as a halfword at +0x6e
        in.u8(net(0x10722438, 0x914, 0x1bc, 0x8f), v.b181c);
        c.put16(record() + 0x6e, sign(int8_t(v.b181c)));
        break;
    case 8: {  // 0x10722564: a text for 0x107268c0
        int16_t length = 0;
        if (in.s16(net(0x1072258c, 0x534, 0xc4, 0x90), length)) {
            text_length = sign(length);
            s.regs.hold({{17, text_length}});
        }
        copy_stream_text(s, in, text_length, 0x107225fc);
        const auto r = record();
        in.advance(text_length);
        s.regs.apply();
        invoke(c, entry::set_text, 0x10722628, {r, s.frame.at(buffer_slot), 0});
        break;
    }
    case 9:  // 0x1072262c: word at +0x8
        in.u32(net(0x10722654, 0x8d4, 0x1ac, 0x92), v.w_a4);
        c.put32(record() + 0x8, v.w_a4);
        break;
    default:  // 10, 0x107226d0: word at +0x48
        in.u32(net(0x107226f8, 0x8b4, 0x1a4, 0x93), v.w_a0);
        c.put32(record() + 0x48, v.w_a0);
        break;
    }
    return common_exit_result;
}
}

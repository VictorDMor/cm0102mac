// TRANSFER_MANAGER.CPP, block "text_d": the text of transfer news message 4004 (0x10404530): a person and
// two clubs (an offer made, refused, agreed ...), with up to three more clubs and further people in the
// long text. The long text picks its sentence from the item's flag word, its kind and the days left.
#include "guest_fpu.hpp"
#include "transfer_manager_native_text_a_detail.hpp"
#include "transfer_manager_native_text_d.hpp"
#include <array>
#include <span>
#include <stdexcept>
#include <vector>

namespace cm::aot::transfer_manager_native {
using namespace text_a;

namespace {
// The words of the item the original keeps in its frame, its text buffers and the club records collected
// from the item's tail (offsets in the frame).
constexpr std::uint32_t day_slot = 0x868, month_slot = 0x864, year_slot = 0x860;
constexpr std::uint32_t fee_slot = 0x86c, buyer_slot = 0x85c, buyer_day = 0x858, buyer_month = 0x854, buyer_year = 0x850;
constexpr std::uint32_t second_day = 0x84c, second_month = 0x848, second_year = 0x844;
constexpr std::uint32_t third_day = 0x840, third_month = 0x83c, third_year = 0x838;
constexpr std::uint32_t magic_high = 0x830, magic_low = 0x834;
constexpr std::uint32_t description = 0x6a0, second_text = 0x380, third_text = 0x510, refusal_text = 0x1f0;
constexpr std::uint32_t buyer_text = 0xc4, money_text = 0x60, money_slot = 0x50, extra_clubs = 0x54;
constexpr std::uint32_t manager_slot = 0x8e8, full_slot = 0x8f3;
constexpr std::int32_t pool_cell = -0x4ba0, scale_cell = -0x4ba4, league_cell = -0x7a70;
constexpr std::uint32_t club_league = 0x2c, league_window = 0x74, window_word = 8;
constexpr std::uint32_t pool_early = 0x68, pool_late = 0x1a8, pool_final = 0x80, pool_refused = 0x40;
// Bits of the item's flag word that choose the sentence.
constexpr std::uint32_t flag_4 = 4, flag_8 = 8, flag_10 = 0x10, flag_40 = 0x40, flag_100 = 0x100, flag_400 = 0x400,
                        flag_800 = 0x800, flag_4000 = 0x4000, flag_20000 = 0x20000, flag_200000 = 0x200000,
                        flag_40000000 = 0x40000000;
constexpr std::int32_t large_fee = 100000;
constexpr std::uint32_t page = text_page;  // formats below the page are written `page - n`

struct Message {
    Cpu& c;
    Frame& frame;
    guest::Fpu fpu;
    std::uint32_t item, output, buffer, text, pool;
    Clubs clubs;
    // The item's fields, under the registers the original keeps them in.
    std::uint32_t person = 0, club_a = 0, club_b = 0, fee = 0, replacement = 0, flags = 0, second_person = 0, third_person = 0;
    std::int8_t kind = 0, mark = 0, reason = 0, count = 0;
    double days = 0;  // f31

    Message(Cpu& cpu, Frame& f, std::uint32_t news_item, std::uint32_t news_output)
        : c(cpu), frame(f), fpu(cpu), item(news_item), output(news_output), buffer(cpu.toc(buffer_cell)), text(cpu.toc(text_cell)),
          pool(cpu.toc(pool_cell)), clubs{cpu, cpu.toc(toc::clubs)} {}

    std::uint32_t at(std::uint32_t offset) const { return frame.at(offset); }
    std::uint32_t slot(std::uint32_t offset) const { return c.u32(frame.at(offset)); }
    std::uint32_t desc() const { return at(description); }
    std::uint32_t len(std::uint32_t club) const { return clubs.long_len(club); }
    std::uint32_t name(std::uint32_t club) const { return clubs.long_name(club); }
    std::uint32_t record_len(std::uint32_t record) const { return std::uint32_t(std::int32_t(c.i8(record + club_len_long))); }
    std::uint32_t record_name(std::uint32_t record) const { return record + club_name_long; }

    // The formatter with the format `from_text` bytes after the text base and its values.
    void speak(std::uint32_t site, std::uint32_t from_text, std::span<const std::uint32_t> values) {
        unsigned reg = 5, stacked = 0;
        for (const auto value : values) {
            if (reg <= 10) c.set(reg++, value);
            else c.put32(frame.at(0x38 + 4 * stacked++), value);
        }
        c.set(3, buffer);
        c.set(4, text + from_text);
        c.call(format_text, after(site));
    }
    // The shapes the sentences share: A's length twice, B's twice, A's name, the description, B's name (+ more
    // stack words); or B before A (B's lengths, A's lengths, description, B's name, A's name).
    void a_then_b(std::uint32_t site, std::uint32_t cut, std::vector<std::uint32_t> more = {}) {
        std::vector<std::uint32_t> v{len(club_a), len(club_a), len(club_b), len(club_b), name(club_a), desc(), name(club_b)};
        v.insert(v.end(), more.begin(), more.end());
        speak(site, cut, v);
    }
    void b_then_a(std::uint32_t site, std::uint32_t cut, std::vector<std::uint32_t> more = {}) {
        std::vector<std::uint32_t> v{len(club_b), len(club_b), len(club_a), len(club_a), desc(), name(club_b), name(club_a)};
        v.insert(v.end(), more.begin(), more.end());
        speak(site, cut, v);
    }
    void only_b(std::uint32_t site, std::uint32_t cut, std::vector<std::uint32_t> more = {}) {
        std::vector<std::uint32_t> v{len(club_b), len(club_b)};
        v.insert(v.end(), more.begin(), more.end());
        speak(site, cut, v);
    }

    void read_item();
    void collect_extra_clubs();
    void describe_people();
    bool league_matches(std::uint32_t club);
    void sentence_with_both_clubs();
    void sentence_without_club_a();
    void sentence_for_decided();
    void sentence_of_fee();
    void long_text();
    void short_text();
    void extra_club_sentence();
    void close_long_text();
};

void Message::read_item() {
    keep(c, 29, item);
    keep(c, 24, buffer);
    keep(c, 30, output);
    keep(c, 25, clubs.cell);
    keep(c, 26, text);
    person = field(c, item, 0, 0x10404568);
    keep(c, 23, person);
    c.put32(at(day_slot), field(c, item, 1, 0x1040457c));
    c.put32(at(month_slot), field(c, item, 2, 0x10404590));
    c.put32(at(year_slot), field(c, item, 3, 0x104045a4));
    club_a = field(c, item, 4, 0x104045b8);
    keep(c, 22, club_a);
    club_b = field(c, item, 5, 0x104045cc);
    keep(c, 21, club_b);
    kind = std::int8_t(field(c, item, 6, 0x104045e0));
    keep_signed(c, 20, kind);
    mark = std::int8_t(field(c, item, 9, 0x104045f0));
    keep_signed(c, 19, mark);
    fee = field(c, item, 10, 0x10404600);
    keep(c, 14, fee);
    c.put32(at(fee_slot), field(c, item, 11, 0x10404610));
    c.put32(at(buyer_slot), field(c, item, 12, 0x10404620));
    c.put32(at(buyer_day), field(c, item, 13, 0x10404634));
    c.put32(at(buyer_month), field(c, item, 14, 0x10404648));
    c.put32(at(buyer_year), field(c, item, 15, 0x1040465c));
    replacement = field(c, item, 16, 0x10404670);
    keep(c, 18, replacement);
    reason = std::int8_t(field(c, item, 17, 0x10404684));
    keep_signed(c, 13, reason);
    second_person = field(c, item, 18, 0x10404694);
    keep(c, 17, second_person);
    c.put32(at(second_day), field(c, item, 19, 0x104046a8));
    c.put32(at(second_month), field(c, item, 20, 0x104046bc));
    c.put32(at(second_year), field(c, item, 21, 0x104046d0));
    third_person = field(c, item, 22, 0x104046e4);
    keep(c, 16, third_person);
    c.put32(at(third_day), field(c, item, 23, 0x104046f8));
    c.put32(at(third_month), field(c, item, 24, 0x1040470c));
    c.put32(at(third_year), field(c, item, 25, 0x10404720));
    flags = field(c, item, 26, 0x10404734);
    keep(c, 15, flags);
    // Days left (f31): word 27 as a double less the pool's magic, divided by the pool's scale.
    c.put32(at(magic_low), field(c, item, 27, 0x10404748) ^ 0x80000000u);
    c.put32(at(magic_high), 0x43300000);
    const auto scale = c.toc(scale_cell);
    const auto whole = fpu.fsubs(c.f64(at(magic_high)), c.f64(pool + 8));
    days = fpu.fdivs(whole, double(c.f32(scale + 0x34)));
    c.set_f(31, days);
    count = std::int8_t(field(c, item, 28, 0x10404780));
    keep_signed(c, 27, count);
}

// The first (up to three) clubs of the item's tail: their records, kept in the frame.
void Message::collect_extra_clubs() {
    std::uint32_t pointer = at(extra_clubs);
    keep(c, 28, pointer);
    keep(c, 31, 0);
    const std::int8_t limit = count > 3 ? 3 : count;
    for (std::uint32_t i = 0; std::int8_t(i) < limit; ++i) {
        const auto club = field(c, item, std::uint32_t(std::int32_t(std::int8_t(i + 0x1d))), 0x104047a0);
        c.put32(pointer, c.u32(clubs.cell) + club * club_size);
        keep(c, 31, i + 1);
        pointer += 4;
        keep(c, 28, pointer);
    }
}

// The texts of the two further people, when the item names them.
void Message::describe_people() {
    if (std::int32_t(second_person) > -1)
        describe(c, 0x10404800, slot(second_day), slot(second_month), slot(second_year), at(second_text), 400, 1, second_person);
    if (std::int32_t(third_person) > -1)
        describe(c, 0x10404828, slot(third_day), slot(third_month), slot(third_year), at(third_text), 400, 1, third_person);
}

// Whether club B's league window holds the current season's word (a test the original repeats).
bool Message::league_matches(std::uint32_t club) {
    const auto league = c.u32(clubs.at(club) + club_league);
    if (league == 0) return false;
    const auto window = c.u32(league + league_window);
    if (window == 0) return false;
    return std::int32_t(c.u32(c.toc(league_cell))) == c.i32(window + window_word);
}

// Item kind 2 (or a replacement club) with club A named: the sentence by flag word and days left.
void Message::sentence_with_both_clubs() {
    if (mark == 1) {
        const auto limit = c.f64(pool + pool_early);
        fpu.fcmp(0, days, limit);
        if (days < limit && (flags & flag_20000)) a_then_b(0x10404904, 0x7feb);
        else a_then_b(0x10404958, page - 0x7f55);
        return;
    }
    if ((flags & flag_10) && (flags & flag_8) && (flags & flag_4000)) {
        b_then_a(0x104049c4, page - 0x7ebe);
        return;
    }
    if ((flags & flag_10) && (flags & flag_4000)) {
        a_then_b(0x10404a28, page - 0x7dc5);
        return;
    }
    if ((flags & flag_10) && (flags & flag_200000) && league_matches(club_b)) {
        a_then_b(0x10404abc, page - 0x7cc7);
        return;
    }
    bool plain = (flags & flag_10) != 0;
    if (!plain) {
        const auto limit = c.f64(pool + pool_late);
        fpu.fcmp(0, days, limit);
        plain = limit < days;
    }
    if (plain) a_then_b(0x10404b28, page - 0x7bd2);
    else sentence_of_fee();
}

// The sentences chosen by the flag word alone (the days left do not matter any more).
void Message::sentence_of_fee() {
    if ((flags & flag_4000) && (flags & flag_200000) && league_matches(club_b)) {
        speak(0x10404bcc, page - 0x7b2f,
              std::array<std::uint32_t, 9>{len(club_a), len(club_a), 100, 100, len(club_b), len(club_b), name(club_a), desc(), name(club_b)});
    } else if (flags & flag_4000) {
        a_then_b(0x10404c28, page - 0x7a34);
    } else if ((flags & flag_200000) && league_matches(club_b)) {
        a_then_b(0x10404cb4, page - 0x7947);
    } else if ((flags & flag_40) && std::int32_t(third_person) > -1) {
        speak(0x10404d28, page - 0x7861,
              std::array<std::uint32_t, 9>{len(club_a), len(club_a), len(club_b), len(club_b), 100, name(club_a), desc(), name(club_b), at(third_text)});
    } else if ((flags & flag_800) || (flags & flag_4)) {
        b_then_a(0x10404d8c, page - 0x7764);
    } else if ((flags & flag_100) && std::int32_t(second_person) > -1) {
        b_then_a(0x10404df8, page - 0x76a9, {at(second_text)});
    } else if ((flags & flag_40000000) && (std::int32_t(slot(fee_slot)) > large_fee || std::int32_t(fee) > large_fee) && mark == 0) {
        if ((flags & flag_8) && (flags & flag_400)) b_then_a(0x10404e88, page - 0x7595);
        else if (flags & flag_400) b_then_a(0x10404ee4, page - 0x749c);
        // 0x10404ef4..0x10404f44 (a sentence for flag 4) cannot run: the flag word has no bit 4 on this path
        // (0x10404d38 sends every item with bit 4 to the sentence above).
        else a_then_b(0x10404f94, page - 0x72ee);
    } else {
        a_then_b(0x10404fe8, page - 0x72ee);
    }
}

// Item kind 2 (or a replacement club) without club A.
void Message::sentence_without_club_a() {
    if (flags & flag_10) only_b(0x10405028, page - 0x725c, {desc(), name(club_b)});
    else if ((flags & flag_40) && std::int32_t(third_person) > -1) only_b(0x10405078, page - 0x71e6, {100, desc(), name(club_b), at(third_text)});
    else only_b(0x104050b0, page - 0x710c, {desc(), name(club_b)});
}

// Kind 4: the club A and B have agreed (a fee changed hands), or the move was refused.
void Message::sentence_for_decided() {
    if (club_a == 0xffffffff) {
        only_b(0x1040545c, page - 0x6b53, {desc(), name(club_b)});
        return;
    }
    if (mark == 1) {
        const auto limit = c.f64(pool + pool_final);
        fpu.fcmp(0, days, limit);
        if (days < limit) a_then_b(0x104051c8, page - 0x6f3a);
        else a_then_b(0x1040521c, page - 0x6e99);
        return;
    }
    const auto agreed_fee = slot(fee_slot);
    if (fee == agreed_fee) {
        // The refusal reasons (0x10410e50) write into the buffer at 0x1f0 and say whether one applies.
        c.put32(frame.at(0x38), at(refusal_text));
        c.set_f(1, days);
        const auto refused = std::int8_t(invoke_native(c, 0x10410e50, 0x10405364, {c.u32(at(manager_slot)), club_a, club_b, second_person, third_person, flags})) != 0;
        if (refused) {
            const auto limit = c.f64(pool + pool_refused);
            fpu.fcmp(0, days, limit);
            if (limit < days) {
                a_then_b(0x104053d0, page - 0x6cb1, {at(refusal_text)});
                return;
            }
        }
        a_then_b(0x10405424, page - 0x6bf4);
        return;
    }
    // A fee was agreed: the money text, the buyer's date and the buyer's name, then the sentence.
    c.put32(at(money_slot), fee);
    const auto amount = invoke(c, 0x10009f20, 0x10405240, {at(money_slot), 6});
    invoke(c, 0x1000b410, 0x10405258, {at(money_text), 0x64, amount, c.r(4), 6});
    describe(c, 0x10405274, slot(buyer_day), slot(buyer_month), slot(buyer_year), at(buyer_text), 300, 1, slot(buyer_slot));
    speak(0x104052e8, page - 0x6dfc,
          std::array<std::uint32_t, 12>{len(club_a), len(club_a), len(club_b), len(club_b), len(club_a), len(club_a), name(club_a), desc(),
                                        name(club_b), name(club_a), at(buyer_text), at(money_text)});
    const auto buyer_offset = slot(buyer_slot) * person_size;
    keep(c, 13, buyer_offset);
    const auto record = c.u32(c.toc(toc::people)) + buyer_offset;
    if (known(c, 0x10405310, record, slot(buyer_day), slot(buyer_month), slot(buyer_year)))
        link(c, 0x10405334, output, at(buyer_text), link_person, record);
}

void Message::long_text() {
    describe(c, 0x1040485c, slot(day_slot), slot(month_slot), slot(year_slot), desc(), 400, 1, person);
    if (kind == 2 || replacement != 0xffffffff) {
        if (replacement != 0xffffffff) {
            club_b = replacement;
            keep(c, 21, club_b);
            mark = reason == 6 ? 1 : 0;
            keep_signed(c, 19, mark);
        }
        if (club_a == 0xffffffff) sentence_without_club_a();
        else sentence_with_both_clubs();
    } else if (kind == 3) {
        if (club_a != 0xffffffff) a_then_b(0x10405114, page - 0x70a7);
        else only_b(0x1040514c, page - 0x6fda, {desc(), name(club_b)});
    } else if (kind == 4) {
        sentence_for_decided();
    }
    // Every sentence goes on to the message's text and links.
    close_long_text();
}

void Message::short_text() {
    describe(c, 0x104056a8, slot(day_slot), slot(month_slot), slot(year_slot), desc(), 400, 5, person);
    if (kind == 2 || replacement != 0xffffffff) {
        if (replacement != 0xffffffff) {
            club_b = replacement;
            keep(c, 21, club_b);
            mark = reason == 6 ? 1 : 0;
            keep_signed(c, 19, mark);
        }
        if (mark == 1) only_b(0x10405718, page - 0x6915, {desc(), name(club_b)});
        else if (std::int32_t(person + club_b) % 3 == 0) only_b(0x10405774, page - 0x68c1, {desc(), name(club_b)});
        else only_b(0x104057ac, page - 0x6877, {desc(), name(club_b)});
    } else if (mark == 1) {
        only_b(0x104057ec, page - 0x6827, {desc(), name(club_b)});
    } else if (std::int32_t(person) % 3 == 0) {
        only_b(0x10405844, page - 0x67c8, {desc(), name(club_b)});
    } else {
        only_b(0x1040587c, page - 0x677b, {desc(), name(club_b)});
    }
    begin_text(c, 0x10405888, output, buffer);
    link(c, 0x104058a8, output, desc(), link_person, c.u32(c.toc(toc::people)) + person * person_size);
    link(c, 0x104058c4, output, name(club_b), link_club, clubs.at(club_b));
}

// The sentence that names the extra clubs of the item's tail.
void Message::extra_club_sentence() {
    const auto first = slot(extra_clubs), second = slot(extra_clubs + 4), third = slot(extra_clubs + 8);
    if (count == 1) {
        speak(0x104054f4, page - 0x6adf, std::array<std::uint32_t, 3>{record_len(first), record_len(first), record_name(first)});
    } else if (count == 2) {
        speak(0x10405538, page - 0x6a9d,
              std::array<std::uint32_t, 6>{record_len(first), record_len(first), record_len(second), record_len(second), record_name(first),
                                           record_name(second)});
    } else if (count == 3) {
        speak(0x1040559c, page - 0x6a3e,
              std::array<std::uint32_t, 9>{record_len(first), record_len(first), record_len(second), record_len(second), record_len(third),
                                           record_len(third), record_name(first), record_name(second), record_name(third)});
    } else if (count > 3) {
        speak(0x10405600, page - 0x69bf,
              std::array<std::uint32_t, 10>{record_len(first), record_len(first), record_len(second), record_len(second), record_len(third),
                                            record_len(third), std::uint32_t(std::int32_t(count)), record_name(first), record_name(second),
                                            record_name(third)});
    }
}

// The end of the long text: the message's text, the links to the person and the clubs, and the extra clubs.
void Message::close_long_text() {
    begin_text(c, 0x10405468, output, buffer);
    const auto people = c.u32(c.toc(toc::people));
    link(c, 0x10405488, output, desc(), link_person, people + person * person_size);
    link(c, 0x104054a4, output, name(club_b), link_club, clubs.at(club_b));
    if (club_a != 0xffffffff) link(c, 0x104054c8, output, name(club_a), link_club, clubs.at(club_a));
    extra_club_sentence();
    if (count > 0) {
        extend_text(c, 0x10405614, output, buffer);
        keep_signed(c, 14, count);
        std::uint32_t pointer = at(extra_clubs);
        keep(c, 13, pointer);
        keep(c, 15, 0);
        const std::int8_t limit = count > 3 ? 3 : count;
        for (std::uint32_t i = 0; std::int8_t(i) < limit; ++i) {
            const auto record = c.u32(pointer);
            link(c, 0x10405638, output, record_name(record), link_club, record);
            pointer += 4;
            keep(c, 13, pointer);
            keep(c, 15, i + 1);
        }
    }
    if (std::int32_t(third_person) > -1)
        link(c, 0x10405684, output, at(third_text), link_person, c.u32(c.toc(toc::people)) + third_person * person_size);
}
}

// Kind 4004 (NEWS.CPP 0x102c6304): `manager` is r3 (the original saves it for the refusal texts), `full` the
// flag byte (kept in the caller's parameter area), `output` the NEWS.CPP message.
void news_text_4004(Cpu& c, std::uint32_t manager, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x10404530, 0x8d0, {.first_gpr = 13, .first_fpr = 31, .gpr_offset = -0x5c});
    Message m(c, frame, item, output);
    c.put32(frame.at(manager_slot), manager);
    c.put8(frame.at(full_slot), full);
    m.read_item();
    m.collect_extra_clubs();
    m.describe_people();
    c.put8(frame.at(refusal_text), 0);
    if (std::int8_t(c.u8(frame.at(full_slot))) != 0) m.long_text();
    else m.short_text();
}
}

// TRANSFER_MANAGER.CPP, block "text_a": the text of the transfer news message of kind 4000 (0x103fe1a0).
#include "transfer_manager_native_text_a.hpp"
#include "transfer_manager_native_text_a_detail.hpp"
#include <algorithm>
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using namespace text_a;

namespace {
// Locals of the original's frame that other routines are given the address of, or read as arguments.
namespace at {
constexpr std::uint32_t person_slot = 0x48, subject = 0x4c0, lead = 0x330, count_text = 0x16c, first_person = 0x5ec;
constexpr std::uint32_t fee_now = 0x138, fee_was = 0x104, fee_only = 0xd0, extra_fee = 0x9c;
constexpr std::uint32_t date_a = 0x268, date_b = 0x1a0, date_object_a = 0x64, date_object_b = 0x5c;
constexpr std::uint32_t slot_now = 0x58, slot_was = 0x54, slot_only = 0x50, slot_extra = 0x4c;
// The people named after the first one: four arrays of three words (day, month, year words and the person record).
constexpr std::uint32_t year_words = 0x6c, month_words = 0x78, day_words = 0x84, people = 0x90;
constexpr std::uint32_t person_text_size = 300, person_text_step = 0x12c;
}
constexpr std::uint32_t lead_words = 100, news_item_club_word = 0xdc;
constexpr std::int32_t default_lead_cell = -0x4bd0, manager_cell = -0x7b40, news_cell = -0x7bf0, table_cell = -0x7bb0;

// The manager's offer in slot `index` of its offer array (slot 0 when out of range).
std::uint32_t offer_in_slot(Cpu& c, std::uint32_t index) {
    const auto offers = c.u32(c.toc(manager_cell));
    const auto slot = std::int32_t(index) < c.i32(offers + 4) ? c.u32(offers) + index * 4 : c.u32(offers);
    return c.u32(slot);
}

// The money text: 10009f20 turns the amount (stored at `slot`) into a pair of words, 1000b410 into text.
void money_text(Cpu& c, const Frame& frame, std::uint32_t amount, std::uint32_t slot, std::uint32_t text, std::uint32_t site_pair,
                std::uint32_t site_text) {
    c.put32(frame.at(slot), amount);
    const auto high = invoke(c, 0x10009f20, after(site_pair), {frame.at(slot), 6});
    invoke(c, 0x1000b410, after(site_text), {frame.at(text), 0x32, high, c.r(4), 6});
}

struct Sentence {
    std::uint32_t site, format;
};

// What the message names: the people and clubs, the fee words and the flags of the item.
struct Message {
    Cpu& c;
    Frame& frame;
    Clubs clubs;
    std::uint32_t text, buffer, output;
    std::uint32_t buyer, seller, third;
    std::int8_t kind, count;
    std::uint32_t terms;
    bool seller_elsewhere;  // the original's r17 after the flag test

    std::uint32_t at(std::uint32_t offset) const { return frame.at(offset); }
    // The sentence goes to the output buffer; its values are the names and lengths the formats want.
    void say(const Sentence& s, std::initializer_list<std::uint32_t> values) const {
        text_a::say(c, frame, s.site, buffer, text + s.format, values);
    }
    std::uint32_t SL() const { return clubs.short_len(buyer); }
    std::uint32_t LL() const { return clubs.long_len(seller); }
    std::uint32_t SN() const { return clubs.short_name(buyer); }
    std::uint32_t LN() const { return clubs.long_name(seller); }
    std::uint32_t TL() const { return clubs.short_len(third); }
    std::uint32_t TN() const { return clubs.short_name(third); }
    // The three sentences of one test on the item's kind (1, 2, anything else) that take the same values.
    void by_kind(const Sentence& one, const Sentence& two, const Sentence& other, std::initializer_list<std::uint32_t> values) const {
        say(kind == 1 ? one : kind == 2 ? two : other, values);
    }
};

// The two clubs and the earlier fee: the sentence of a loan-free offer whose fee fell (0x103feab0-0x103ff118).
void fee_dropped(const Message& m) {
    const auto fee_now = m.at(at::fee_now), fee_was = m.at(at::fee_was), subject = m.at(at::subject), lead = m.at(at::lead);
    const auto terms = m.terms;
    if (m.count == 0) {
        if (m.seller_elsewhere) {
            if (m.kind == 1) m.say({0x103feb1c, 0x2da5}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_now, m.LN(), subject});
            else if (m.kind == 2) m.say({0x103feb80, 0x2e57}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_now, m.LN(), subject});
            else if ((terms & 3) == 0) {
                // 0x103feca8 (terms & 2 with neither bit set) cannot be reached.
                if ((terms & 2) != 0) throw std::logic_error("0x103feca8: unreachable branch");
                m.say({0x103fed04, 0x3094}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_now, m.LN(), subject});
            } else if (std::int32_t(m.third) == -1) {
                m.say({0x103fec44, 0x2fc7}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_was, m.LN(), subject});
            } else {
                m.say({0x103febe8, 0x2f00}, {m.SL(), m.SL(), m.TL(), m.TL(), m.SN(), m.TN(), subject, fee_was});
            }
        } else if (m.kind == 1) {
            m.say({0x103fed44, 0x3138}, {m.SL(), m.SL(), m.SN(), fee_now, subject});
        } else if (m.kind == 2) {
            m.say({0x103fed84, 0x31c5}, {m.SL(), m.SL(), m.SN(), fee_now, subject});
        } else if ((terms & 1) == 0 || (terms & 2) == 0) {
            if ((terms & 2) == 0) m.say({0x103fef10, 0x3094}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_now, m.LN(), subject});
            else m.say({0x103feeb4, 0x3249}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_was, m.LN(), subject});
        } else if (std::int32_t(m.third) == -1) {
            m.say({0x103fee50, 0x3249}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_was, m.LN(), subject});
        } else {
            m.say({0x103fedf4, 0x2f00}, {m.SL(), m.SL(), m.TL(), m.TL(), m.SN(), m.TN(), subject, fee_was});
        }
    } else if (m.seller_elsewhere) {
        m.by_kind({0x103fef84, 0x3315}, {0x103feff0, 0x3402}, {0x103ff054, 0x34e6},
                  {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_now, lead, m.LN(), subject});
    } else {
        m.by_kind({0x103ff098, 0x35c5}, {0x103ff0dc, 0x368d}, {0x103ff118, 0x374c}, {m.SL(), m.SL(), m.SN(), fee_now, lead, subject});
    }
}

// The sentence of a loan-free offer at a single fee, with `fee` the fee word (0x103ff148-0x103ffa9c).
void single_fee(const Message& m, std::int32_t fee) {
    const auto fee_only = m.at(at::fee_only), subject = m.at(at::subject), lead = m.at(at::lead);
    const auto terms = m.terms;
    const bool elsewhere = m.seller_elsewhere;
    if (m.count == 0) {
        if (fee > 0) {
            if (elsewhere) {
                if (m.kind == 1) {
                    m.say({0x103ff1c0, 0x3806}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_only, m.LN(), subject});
                } else if (m.kind == 2) {
                    m.say({0x103ff224, 0x38b5}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_only, m.LN(), subject});
                } else if ((terms & 1) == 0 || (terms & 2) == 0) {
                    if ((terms & 1) == 0 || std::int32_t(m.third) == -1 || fee < 500000) {
                        if ((terms & 2) == 0) m.say({0x103ff42c, 0x3af2}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_only, m.LN(), subject});
                        else m.say({0x103ff3d0, 0x3a2e}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_only, m.LN(), subject});
                    } else {
                        m.say({0x103ff36c, 0x2f00}, {m.SL(), m.SL(), m.TL(), m.TL(), m.SN(), m.TN(), subject, fee_only});
                    }
                } else if (std::int32_t(m.third) == -1) {
                    m.say({0x103ff2f0, 0x3a2e}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_only, m.LN(), subject});
                } else {
                    m.say({0x103ff294, 0x395b}, {m.SL(), m.SL(), m.TL(), m.TL(), m.SN(), m.TN(), subject, fee_only});
                }
            } else {
                m.by_kind({0x103ff46c, 0x3b93}, {0x103ff4ac, 0x3c1d}, {0x103ff4e4, 0x3c9e}, {m.SL(), m.SL(), m.SN(), fee_only, subject});
            }
        } else if (elsewhere) {
            m.by_kind({0x103ff548, 0x3d1a}, {0x103ff5a4, 0x3daa}, {0x103ff5f8, 0x3e31},
                      {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), m.LN(), subject});
        } else {
            m.by_kind({0x103ff634, 0x3eb3}, {0x103ff670, 0x3f1e}, {0x103ff6a4, 0x3f80}, {m.SL(), m.SL(), m.SN(), subject});
        }
    } else if (fee > 0) {
        if (elsewhere) {
            m.by_kind({0x103ff724, 0x3fdd}, {0x103ff790, 0x40c7}, {0x103ff7f4, 0x41a8},
                      {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), fee_only, lead, m.LN(), subject});
        } else {
            m.by_kind({0x103ff838, 0x4284}, {0x103ff87c, 0x4349}, {0x103ff8b8, 0x4405}, {m.SL(), m.SL(), m.SN(), fee_only, lead, subject});
        }
    } else if (elsewhere) {
        m.by_kind({0x103ff924, 0x44bc}, {0x103ff988, 0x4591}, {0x103ff9e4, 0x465d},
                  {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), lead, m.LN(), subject});
    } else {
        m.by_kind({0x103ffa24, 0x4724}, {0x103ffa64, 0x47d4}, {0x103ffa9c, 0x487b}, {m.SL(), m.SL(), m.SN(), lead, subject});
    }
}
}

void news_text_4000(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x103fe1a0, 0x9f0, {.first_gpr = 13});
    const auto table = c.toc(table_cell);
    const auto text = c.toc(text_cell), buffer = c.toc(buffer_cell);
    const Clubs clubs{c, c.toc(toc::clubs)};
    const auto slot_cell = table + 0x2c58;
    const auto news_slot = [&] { return c.u32(table + 4 + std::uint32_t(std::int32_t(c.i16(slot_cell))) * 0x2c0); };
    keep(c, 13, slot_cell);
    keep(c, 14, table + 4);
    keep(c, 15, item);
    const auto own_club = news_slot();
    keep(c, 16, own_club);
    keep(c, 17, full);
    keep(c, 22, 0x18);
    keep(c, 26, buffer);
    keep(c, 27, clubs.cell);
    keep(c, 28, text);
    c.put32(frame.at(0xa14), output);

    // The message's stage (word 9); when it is not set, an offer of the manager that names it sets it.
    auto stage = std::int8_t(field(c, item, 9, 0x103fe1ec));
    if (stage == 0) {
        const auto first = field(c, item, 6, 0x103fe204);
        keep(c, 18, first);
        const auto second = field(c, item, 7, 0x103fe214);
        const auto offer = offer_in_slot(c, first);
        if (offer && c.u32(offer + 0x2c) == second) {
            const auto stored = std::int8_t(c.u8(offer + 0x34));
            if (stored != 0) {
                stage = stored;
                invoke(c, put_news_word, 0x103fe290,
                       {c.u32(c.toc(news_cell)), news_slot(), c.u32(item + news_item_club_word), 9, std::uint32_t(std::int32_t(stored)), item});
            }
        }
    }

    if (std::int8_t(full) == 0) {
        const auto person = field(c, item, 0, 0x103ffd4c);
        keep(c, 14, person);
        const auto day = field(c, item, 1, 0x103ffd60);
        keep(c, 16, day);
        const auto month = field(c, item, 2, 0x103ffd74);
        keep(c, 17, month);
        const auto year = field(c, item, 3, 0x103ffd88);
        keep(c, 15, year);
        const auto club = field(c, item, 4, 0x103ffd9c);
        keep(c, 13, club);
        describe(c, 0x103ffdc0, day, month, year, frame.at(at::subject), at::person_text_size, 5, person);
        const auto club_offset = club * club_size;
        keep(c, 13, club_offset);
        text_a::say(c, frame, 0x103ffdf0, buffer, text + 0x4a9f, {clubs.long_len(club), clubs.long_len(club), clubs.long_name(club), frame.at(at::subject)});
        begin_text(c, 0x103ffdfc, output, buffer);
        link(c, 0x103ffe14, output, clubs.long_name(club), link_club, clubs.at(club));
        const auto person_offset = person * person_size;
        keep(c, 13, person_offset);
        const auto record = c.u32(c.toc(toc::people)) + person_offset;
        if (known(c, 0x103ffe38, record, day, month, year)) link(c, 0x103ffe5c, output, frame.at(at::subject), link_person, record);
        return;
    }

    // The words the long text starts from: the default text, replaced below when people are named.
    const auto default_text = c.toc(default_lead_cell);
    for (std::uint32_t word = 0; word < lead_words; ++word) c.put32(frame.at(at::lead + word * 4), c.u32(default_text + word * 4));

    const auto person = field(c, item, 0, 0x103fe2cc);
    const auto day = field(c, item, 1, 0x103fe2e0);
    const auto month = field(c, item, 2, 0x103fe2f4);
    const auto year = field(c, item, 3, 0x103fe308);
    const auto buyer = field(c, item, 4, 0x103fe31c);
    keep(c, 29, buyer);
    const auto seller = field(c, item, 5, 0x103fe32c);
    keep(c, 30, seller);
    const auto fee = std::int32_t(field(c, item, 8, 0x103fe33c));
    const auto fee_limit = std::int32_t(field(c, item, 10, 0x103fe34c));
    const auto on_loan = std::int8_t(field(c, item, 0xb, 0x103fe35c));
    const auto date_month = std::uint16_t(field(c, item, 0xc, 0x103fe370));
    const auto date_year = std::uint16_t(field(c, item, 0xd, 0x103fe384));
    const auto kind = std::int8_t(field(c, item, 0xe, 0x103fe398));
    keep_signed(c, 20, kind);
    const auto stage_word = std::uint8_t(field(c, item, 0xf, 0x103fe3a8));
    const auto terms = std::int8_t(field(c, item, 0x10, 0x103fe3bc));
    keep_signed(c, 19, terms);
    const auto third = field(c, item, 0x11, 0x103fe3cc);
    keep(c, 31, third);
    const auto agent = std::uint8_t(field(c, item, 0x12, 0x103fe3dc));
    const auto count = std::int8_t(field(c, item, 0x17, 0x103fe3f0));
    keep_signed(c, 18, count);

    bool seller_elsewhere = false;
    if (std::int32_t(seller) != -1) {
        const auto club_word = c.u32(clubs.at(seller) + 0x90);
        seller_elsewhere = club_word == 0 || club_word != own_club;
    }
    keep(c, 17, seller_elsewhere ? 1 : 0);

    // The other people the message names (up to three): their person texts follow the first one.
    std::uint32_t people[3] = {}, people_days[3] = {}, people_months[3] = {}, people_years[3] = {};
    std::uint32_t next_field = 0x18;
    if (count != 0) {
        keep(c, 25, 0);
        keep(c, 23, frame.at(at::day_words));
        keep(c, 24, 0);
        keep(c, 14, frame.at(at::month_words));
        keep(c, 13, frame.at(at::year_words));
        keep(c, 21, 0);
        const std::int8_t shown = count < 3 ? count : 3;
        for (std::int8_t i = 0; i < shown; ++i) {
            keep(c, 22, next_field + 1);
            const auto index = field(c, item, next_field, 0x103fe468);
            keep(c, 16, index);
            people[i] = c.u32(c.toc(toc::people)) + index * person_size;
            keep(c, 22, next_field + 2);
            people_days[i] = field(c, item, next_field + 1, 0x103fe494);
            keep(c, 22, next_field + 3);
            people_months[i] = field(c, item, next_field + 2, 0x103fe4a8);
            keep(c, 22, next_field + 4);
            people_years[i] = field(c, item, next_field + 3, 0x103fe4bc);
            next_field += 4;
            c.put32(frame.at(at::people + 4 * i), people[i]);
            c.put32(frame.at(at::day_words + 4 * i), people_days[i]);
            c.put32(frame.at(at::month_words + 4 * i), people_months[i]);
            c.put32(frame.at(at::year_words + 4 * i), people_years[i]);
            describe(c, 0x103fe4e4, people_days[i], people_months[i], people_years[i],
                     frame.at(at::first_person + at::person_text_step * i), at::person_text_size, 1, index);
            keep(c, 25, 4 * (i + 1u));
            keep(c, 24, at::person_text_step * (i + 1u));
            keep(c, 21, i + 1u);
        }
        const auto first = frame.at(at::first_person), second = frame.at(at::first_person + at::person_text_step),
                   third_text = frame.at(at::first_person + 2 * at::person_text_step);
        if (count == 1) {
            invoke(c, 0x1077dbf0, after(0x103fe524), {frame.at(at::lead), first});
        } else if (count == 2) {
            text_a::say(c, frame, 0x103fe544, frame.at(at::lead), text + 0x243a, {first, second});
        } else if (count == 3) {
            text_a::say(c, frame, 0x103fe568, frame.at(at::lead), text + 0x248f, {first, second, third_text});
        } else {
            invoke(c, 0x10746a70, after(0x103fe580), {std::uint32_t(std::int32_t(count)), frame.at(at::count_text), 0x32, 1});
            text_a::say(c, frame, 0x103fe590, frame.at(at::lead), text + 0x250e, {frame.at(at::count_text)});
        }
    }

    describe(c, 0x103fe5b0, day, month, year, frame.at(at::subject), at::person_text_size, 1, person);
    const auto subject_club = std::int8_t(field(c, item, next_field, 0x103fe5bc));
    keep_signed(c, 14, subject_club);

    const Message m{c, frame, clubs, text, buffer, output, buyer, seller, third, kind, count, std::uint32_t(terms), seller_elsewhere};
    if (on_loan == 0) {
        if (fee_limit > fee) {
            money_text(c, frame, std::uint32_t(fee_limit), at::slot_now, at::fee_now, 0x103fea68, 0x103fea80);
            money_text(c, frame, std::uint32_t(fee), at::slot_was, at::fee_was, 0x103fea94, 0x103feaac);
            fee_dropped(m);
        } else {
            money_text(c, frame, std::uint32_t(fee), at::slot_only, at::fee_only, 0x103ff12c, 0x103ff144);
            single_fee(m, fee);
        }
    } else {
        const auto subject = frame.at(at::subject);
        const auto person_known = invoke(c, 0x100c6bf0, after(0x103fe5e0), {std::uint32_t(std::int32_t(subject_club)), 0, 0, frame.at(at::person_slot)});
        keep(c, 13, person_known);
        const auto listed = std::int8_t(invoke(c, 0x100c7a00, after(0x103fe5ec), {std::uint32_t(std::int32_t(subject_club))}));
        const auto scratch = [&] { return std::uint32_t(std::int32_t(c.i8(frame.at(at::person_slot)))); };
        if (listed != 0 && person_known != 0) {
            if (std::int32_t(seller) > -1) {
                m.say({0x103fe66c, 0x252e}, {m.SL(), m.SL(), m.LL(), m.LL(), scratch(), scratch(), m.SN(), m.LN(), subject, person_known});
            } else {
                m.say({0x103fe6b0, 0x25cf}, {m.SL(), m.SL(), scratch(), scratch(), m.SN(), subject, person_known});
            }
        } else if (seller_elsewhere) {
            if (stage_word == 1 || stage_word == 3) {
                m.say({0x103fe720, 0x264a}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), m.clubs.long_name(seller), subject});
            } else if (date_year == 0) {
                m.say({0x103fe8d4, 0x29ce}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), m.clubs.long_name(seller), subject});
            } else {
                const auto date = invoke(c, 0x107156b0, after(0x103fe744), {frame.at(at::date_object_a), std::uint32_t(std::int32_t(std::int16_t(date_month))), date_year});
                invoke(c, 0x10716eb0, after(0x103fe754), {date, frame.at(at::date_a), 0xc8, 0xff});
                if (agent == 3) m.say({0x103fe7c0, 0x26e9}, {m.SL(), m.SL(), m.LL(), m.LL(), 100, m.SN(), m.clubs.long_name(seller), subject, frame.at(at::date_a)});
                else if (agent == 5) m.say({0x103fe82c, 0x27e6}, {m.SL(), m.SL(), m.LL(), m.LL(), 100, m.SN(), m.clubs.long_name(seller), subject, frame.at(at::date_a)});
                else m.say({0x103fe880, 0x28f4}, {m.SL(), m.SL(), m.LL(), m.LL(), m.SN(), m.clubs.long_name(seller), subject});
            }
        } else if (stage_word == 1 || stage_word == 3) {
            m.say({0x103fe91c, 0x2a57}, {m.SL(), m.SL(), m.SN(), subject});
        } else if (date_year == 0) {
            m.say({0x103fea40, 0x2d41}, {m.SL(), m.SL(), m.SN(), subject});
        } else {
            const auto date = invoke(c, 0x107156b0, after(0x103fe940), {frame.at(at::date_object_b), std::uint32_t(std::int32_t(std::int16_t(date_month))), date_year});
            invoke(c, 0x10716eb0, after(0x103fe950), {date, frame.at(at::date_b), 0xc8, 0xff});
            if (agent == 3) m.say({0x103fe994, 0x2ad1}, {m.SL(), m.SL(), 100, m.SN(), subject, frame.at(at::date_b)});
            else if (agent == 5) m.say({0x103fe9d8, 0x2ba7}, {m.SL(), m.SL(), 100, m.SN(), subject, frame.at(at::date_b)});
            else m.say({0x103fea0c, 0x2c8e}, {m.SL(), m.SL(), m.SN(), subject});
        }
    }

    // The closing sentence, its links and the people named after the first.
    begin_text(c, 0x103ffaa8, output, buffer);
    if (on_loan == 0 || fee == 0) {
        if (stage == 1) m.say({0x103ffb58, 0x4992}, {m.LL(), m.LL(), m.LN()});
        else if (stage == 5) m.say({0x103ffb90, 0x49db}, {m.LL(), m.LL(), m.LN()});
        else if (stage == 2) m.say({0x103ffbc8, 0x4a18}, {m.LL(), m.LL(), m.LN()});
        else if (stage == 0x11) m.say({0x103ffc00, 0x4a55}, {m.LL(), m.LL(), m.LN()});
        else c.put8(buffer, 0);
    } else {
        money_text(c, frame, std::uint32_t(fee), at::slot_extra, at::extra_fee, 0x103ffad0, 0x103ffae8);
        m.say({0x103ffb18, 0x491d}, {m.LL(), m.LL(), m.LN(), frame.at(at::extra_fee)});
    }
    if (std::int8_t(c.u8(buffer)) != 0) extend_text(c, 0x103ffc24, output, buffer);
    link(c, 0x103ffc40, output, clubs.short_name(buyer), link_club, clubs.at(buyer));
    if (std::int32_t(seller) != -1) link(c, 0x103ffc64, output, clubs.long_name(seller), link_club, clubs.at(seller));
    const auto person_offset = person * person_size;
    keep(c, 13, person_offset);
    const auto record = c.u32(c.toc(toc::people)) + person_offset;
    if (known(c, 0x103ffc8c, record, day, month, year)) link(c, 0x103ffcb0, output, frame.at(at::subject), link_person, record);
    if (count != 0 && count < 4) {
        keep(c, 20, 0);
        keep(c, 16, frame.at(at::people));
        keep(c, 17, 0);
        keep(c, 15, frame.at(at::day_words));
        keep(c, 14, frame.at(at::month_words));
        keep(c, 13, frame.at(at::year_words));
        for (std::int8_t i = 0; i < count; ++i) {
            keep(c, 19, people[i]);
            if (known(c, 0x103ffcf8, people[i], people_days[i], people_months[i], people_years[i]))
                link(c, 0x103ffd18, output, frame.at(at::first_person + at::person_text_step * i), link_person, people[i]);
            keep(c, 13, frame.at(at::year_words + 4 * (i + 1u)));
            keep(c, 14, frame.at(at::month_words + 4 * (i + 1u)));
            keep(c, 15, frame.at(at::day_words + 4 * (i + 1u)));
            keep(c, 16, frame.at(at::people + 4 * (i + 1u)));
            keep(c, 17, at::person_text_step * (i + 1u));
            keep(c, 20, i + 1u);
        }
    }
}
}

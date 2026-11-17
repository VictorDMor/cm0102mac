// TRANSFER_MANAGER.CPP, block "text_d": the text of transfer news message 4005 (0x10405ca0), which also
// writes the message's stage back into the stored news item.
#include "transfer_manager_native_text_d.hpp"
#include "transfer_manager_native_text_a_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_a;

namespace {
constexpr std::uint32_t stage_word = 9, news_slot_size = 0x2c0, news_slot_club = 0x24;
constexpr std::uint32_t offer_serial = 0x2c, offer_stage = 0x34, offer_buyer = 8;

// What the stage of the stored offer says of this message: the original stores the stage in word 9 of the
// stored item (through NEWS.CPP) and keeps it in r31 for the closing sentence. Returns the stage.
std::int8_t update_stage(Cpu& c, std::uint32_t item, std::int8_t stage) {
    const auto session = c.toc(-0x7bb0), news = c.toc(-0x7bf0), clubs_cell = c.toc(toc::clubs);
    const auto slot_index = session + 0x2c58;
    keep(c, 15, slot_index);
    keep(c, 16, session + 4);
    // The slot of the current club in the news table (re-read at each update, as the original does).
    const auto slot = [&] { return c.u32(session + 4 + std::uint32_t(std::int32_t(c.i16(slot_index))) * news_slot_size); };
    const auto first_slot = slot();
    keep(c, 18, first_slot);
    const auto offer_index = field(c, item, 6, 0x10405d38);
    keep(c, 20, offer_index);
    const auto serial = field(c, item, 7, 0x10405d48);
    const auto offers = c.u32(c.toc(-0x7b40));
    const auto at = std::int32_t(offer_index) < c.i32(offers + 4) ? c.u32(offers) + offer_index * 4 : c.u32(offers);
    const auto offer = c.u32(at);
    const auto update = [&](std::uint32_t site, std::uint32_t value) {
        invoke(c, put_news_word, after(site), {c.u32(news), slot(), c.u32(item + 0xdc), stage_word, value, item});
    };
    if (offer == 0 || c.u32(offer + offer_serial) != serial) {
        keep_signed(c, 31, -1);
        invoke(c, put_news_word, 0x10405f34, {c.u32(news), first_slot, c.u32(item + 0xdc), stage_word, 0xffffffff, item});
        return -1;
    }
    const auto stored = c.u8(offer + offer_stage);
    const auto code = std::int8_t(stored);
    if (code == 0) return stage;
    const auto stored_club = c.u32(clubs_cell) + c.u32(offer + offer_buyer) * club_size;
    const auto announce = [&](std::uint32_t site, std::uint32_t value, std::uint32_t kept) {
        keep(c, 31, kept);
        update(site, value);
        return std::int8_t(kept);
    };
    if (c.u32(first_slot + news_slot_club) == stored_club) {
        if (code == 0x12 || code == 3) return announce(0x10405dec, std::uint32_t(std::int32_t(code)), stored);
        if (code == 0xd || code == 0xe) return announce(0x10405e28, 0xd, 0xd);
        if (code == 0x13) return announce(0x10405e5c, 0xd, 0xd);
    } else {
        if (code == 0x11 || code == 2) return announce(0x10405e9c, std::uint32_t(std::int32_t(code)), stored);
        if (code == 0xc || code == 0xe) return announce(0x10405ed8, 0xc, 0xc);
        if (code == 0x13) return announce(0x10405f0c, 0xc, 0xc);
    }
    return stage;
}
}

// Kind 4005 (NEWS.CPP 0x102c632c): a person and one or two clubs. The stage of the offer is first brought up
// to date (unless the item's stage already is one of the final ones); then the short text names the club,
// the long text gives the fee, the date and a closing sentence by stage.
void news_text_4005(Cpu& c, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x10405ca0, 0x360, {.first_gpr = 13});
    const auto text = c.toc(text_cell), buffer = c.toc(buffer_cell);
    const auto people_cell = c.toc(toc::people);
    const Clubs clubs{c, c.toc(toc::clubs)};
    constexpr std::uint32_t reason = 0x58, names = 0xe0, date_holder = 0x50, date_text = 0x114, description = 0x1dc;
    constexpr std::uint32_t fee_slot = 0x4c, fee_text = 0xac, bid_slot = 0x48, bid_text = 0x78;
    keep(c, 17, item);
    keep(c, 16, c.toc(-0x7bb0));
    keep(c, 27, buffer);
    keep(c, 19, full);
    keep(c, 14, c.toc(-0x7bf0));
    keep(c, 25, output);
    keep(c, 13, people_cell);
    keep(c, 26, clubs.cell);
    keep(c, 29, text);
    const auto first_stage = std::int8_t(field(c, item, 9, 0x10405cdc));
    keep_signed(c, 31, first_stage);
    std::int8_t stage = first_stage;
    if (first_stage != 0xe && first_stage != 3 && first_stage != 0x10 && first_stage != 2 && first_stage != 0x12 &&
        first_stage != 0x11 && first_stage != -1)
        stage = update_stage(c, item, first_stage);
    if (std::int8_t(full) != 0) {
        const auto person = field(c, item, 0, 0x10405f44);
        keep(c, 19, person);
        const auto day = field(c, item, 1, 0x10405f58);
        keep(c, 20, day);
        const auto month = field(c, item, 2, 0x10405f6c);
        keep(c, 21, month);
        const auto year = field(c, item, 3, 0x10405f80);
        keep(c, 22, year);
        const auto club = field(c, item, 4, 0x10405f94);
        keep(c, 28, club);
        const auto other = field(c, item, 5, 0x10405fa4);
        keep(c, 30, other);
        const auto bid = field(c, item, 8, 0x10405fb4);
        keep(c, 16, bid);
        const auto limit = field(c, item, 10, 0x10405fc4);
        keep(c, 15, limit);
        const auto reason_kind = std::int8_t(field(c, item, 11, 0x10405fd4));
        keep_signed(c, 18, reason_kind);
        const auto flag = std::int8_t(field(c, item, 12, 0x10405fe4));
        keep_signed(c, 24, flag);
        const auto date_flag = std::int8_t(field(c, item, 13, 0x10405ff4));
        keep_signed(c, 23, date_flag);
        const auto date_month = field(c, item, 14, 0x10406004) & 0xffff;
        keep(c, 14, date_month);
        const auto date_year = field(c, item, 15, 0x10406014) & 0xffff;
        keep(c, 17, date_year);
        if (reason_kind != 0) {
            invoke(c, 0x10746a70, 0x10406038, {std::uint32_t(std::int32_t(reason_kind)), frame.at(names), 0x32, 1});
            if (reason_kind == 1) say(c, frame, 0x1040604c, frame.at(reason), text + text_page - 0x672c, {});
            else say(c, frame, 0x10406060, frame.at(reason), text + 0x250e, {frame.at(names)});
        }
        describe(c, 0x10406080, day, month, year, frame.at(description), 300, 1, person);
        const auto club_long = clubs.long_len(club), other_long = clubs.long_len(other);
        const auto club_name = clubs.long_name(club), other_name = clubs.long_name(other);
        const auto desc = frame.at(description);
        const auto money = [&](std::uint32_t slot, std::uint32_t value, std::uint32_t digits, std::uint32_t site_a, std::uint32_t site_b) {
            c.put32(frame.at(slot), value);
            const auto amount = invoke(c, 0x10009f20, after(site_a), {frame.at(slot), 6});
            invoke(c, 0x1000b410, after(site_b), {frame.at(digits), 0x32, amount, c.r(4), 6});
        };
        if (std::int32_t(limit) > std::int32_t(bid) && flag == 0) {
            money(fee_slot, limit, fee_text, 0x104060a0, 0x104060b8);
            if (reason_kind == 0)
                say(c, frame, 0x10406120, buffer, text + text_page - 0x6723,
                    {other_long, other_long, 100, club_long, club_long, other_name, desc, club_name, frame.at(fee_text)});
            else
                say(c, frame, 0x1040618c, buffer, text + text_page - 0x666c,
                    {other_long, other_long, 100, club_long, club_long, frame.at(fee_text), frame.at(reason), other_name, desc, club_name});
        } else {
            money(bid_slot, bid, bid_text, 0x104061a0, 0x104061b8);
            if (reason_kind == 0) {
                if (std::int32_t(bid) > 0 && flag == 0)
                    say(c, frame, 0x10406238, buffer, text + text_page - 0x6572,
                        {100, other_long, other_long, 100, club_long, club_long, frame.at(bid_text), other_name, desc, club_name});
                else if (std::int32_t(other) > -1)
                    say(c, frame, 0x1040629c, buffer, text + text_page - 0x64bf,
                        {other_long, other_long, 100, club_long, club_long, other_name, desc, club_name});
                else
                    say(c, frame, 0x104062d8, buffer, text + text_page - 0x6428, {100, club_long, club_long, desc, club_name});
            } else if (std::int32_t(bid) > 0) {
                say(c, frame, 0x1040634c, buffer, text + text_page - 0x63b2,
                    {other_long, other_long, 100, club_long, club_long, frame.at(bid_text), frame.at(reason), other_name, desc, club_name});
            } else {
                say(c, frame, 0x104063a8, buffer, text + text_page - 0x62c3,
                    {other_long, other_long, 100, club_long, club_long, other_name, desc, club_name});
            }
        }
        begin_text(c, 0x104063b4, output, buffer);
        if (date_flag != 0) {
            const auto when = invoke(c, 0x107156b0, 0x104063d0, {frame.at(date_holder), std::uint32_t(std::int32_t(std::int16_t(date_month))), date_year});
            invoke(c, 0x10717620, 0x104063e0, {when, frame.at(date_text), 200, 0xff});
            // The two branches of the original (date_flag 1 or not) are the same sentence.
            say(c, frame, date_flag == 1 ? 0x104063fc : 0x10406418, buffer, text + text_page - 0x6218, {100, frame.at(date_text)});
            extend_text(c, 0x10406424, output, buffer);
        }
        // The closing sentences name one club: its length twice, then its name.
        const auto closing = [&](std::uint32_t site, std::uint32_t cut, std::uint32_t index) {
            say(c, frame, site, buffer, text + text_page - cut, {clubs.long_len(index), clubs.long_len(index), clubs.long_name(index)});
        };
        if (stage == 0xe && std::int32_t(other) != -1) say(c, frame, 0x10406448, buffer, text + text_page - 0x61c4, {});
        else if (stage == 0xe) closing(0x10406490, 0x619a, club);
        else if (stage == 0xd) {
            if (std::int32_t(other) != -1) closing(0x104064d8, 0x619a, club);
            else closing(0x1040650c, 0x6159, club);
        } else if (std::int32_t(other) != -1 && stage == 0xc && flag == 0) closing(0x10406558, 0x619a, other);
        else if (stage == 3) closing(0x10406598, 0x611c, club);
        else if (stage == 2 && std::int32_t(other) != -1) closing(0x104065dc, 0x611c, other);
        else if (stage == 0x10) say(c, frame, 0x104065fc, buffer, text + text_page - 0x60db, {});
        else if (stage == 0x12) closing(0x10406638, 0x60b8, club);
        else if (stage == 0x11) closing(0x10406674, 0x60b8, other);
        else c.put8(buffer, 0);
        if (std::int8_t(c.u8(buffer)) != 0) extend_text(c, 0x10406698, output, buffer);
        link(c, 0x104066b4, output, club_name, link_club, clubs.at(club));
        if (std::int32_t(other) != -1) link(c, 0x104066d8, output, other_name, link_club, clubs.at(other));
        const auto person_offset = person * person_size;
        keep(c, 14, person_offset);
        const auto person_record = c.u32(people_cell) + person * person_size;
        if (known(c, 0x104066f8, person_record, day, month, year)) link(c, 0x10406718, output, desc, link_person, person_record);
    } else {
        const auto person = field(c, item, 0, 0x10406728);
        keep(c, 15, person);
        const auto day = field(c, item, 1, 0x1040673c);
        keep(c, 16, day);
        const auto month = field(c, item, 2, 0x10406750);
        keep(c, 18, month);
        const auto year = field(c, item, 3, 0x10406764);
        keep(c, 19, year);
        const auto club = field(c, item, 4, 0x10406778);
        keep(c, 14, club);
        const auto fee = field(c, item, 10, 0x10406788);
        keep(c, 20, fee);
        const auto flag = std::int8_t(field(c, item, 12, 0x10406798));
        keep_signed(c, 21, flag);
        const auto reason_kind = std::int8_t(field(c, item, 17, 0x104067a8));
        keep_signed(c, 17, reason_kind);
        describe(c, 0x104067cc, day, month, year, frame.at(description), 300, 5, person);
        const auto known_reason = std::int8_t(invoke(c, 0x100c7a00, 0x104067d8, {std::uint32_t(std::int32_t(reason_kind))})) != 0;
        const auto length = clubs.long_len(club), name = clubs.long_name(club);
        if (known_reason && flag != 0) say(c, frame, 0x10406818, buffer, text + text_page - 0x6067, {length, length, frame.at(description), name});
        else if (fee != 0) say(c, frame, 0x10406858, buffer, text + text_page - 0x6012, {length, length, name, frame.at(description)});
        else say(c, frame, 0x10406890, buffer, text + text_page - 0x5fbf, {length, length, name, frame.at(description)});
        begin_text(c, 0x1040689c, output, buffer);
        link(c, 0x104068b8, output, name, link_club, clubs.at(club));
        const auto person_offset = person * person_size;
        keep(c, 14, person_offset);
        const auto person_record = c.u32(people_cell) + person * person_size;
        if (known(c, 0x104068d8, person_record, day, month, year)) link(c, 0x104068f8, output, frame.at(description), link_person, person_record);
    }
}
}

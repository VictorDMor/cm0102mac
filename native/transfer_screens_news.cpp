#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;

void open_existing_from_news(Cpu& c, std::uint32_t manager, std::uint32_t item) {
    Frame f(c, 0x10421a20, 0x130, {.first_gpr = 30});
    Kept<31> offer_id(c, manager); Kept<30> revision(c, item); Program p{c};
    p.call(0x102bdf90, 0x10421a4c, {f.at(0x38), 0, 0});
    p.call(0x102ccdd0, 0x10421a64, {indirect(c, -0x7bf0), offer_id, revision, f.at(0x38)});
    offer_id = p.call(0x102cb570, 0x10421a70, {f.at(0x38), 6});
    revision = p.call(0x102cb570, 0x10421a84, {f.at(0x38), 7});
    c.set(5, c.toc(slot::transfers));
    const auto offer_link = p.call(0x103eb440, 0x10421a98, {c.u32(c.r(5)), offer_id});
    c.set(4, c.u32(offer_link));
    bool previous = compare(c, c.u32(c.r(4) + 0x2c), revision) != 0;
    if (!previous) {
        c.set(4, c.u32(offer_link)); previous = compare(c, c.u8(c.r(4) + 0x34), 3) == 0;
    }
    if (previous) {
        const auto old_offer = c.u32(offer_link);
        enter(c, 0x10421ac4, {old_offer, revision}); open_previous_offer(c, old_offer, revision);
    } else {
        enter(c, 0x10421ad0, {offer_link, 1}); open_existing_offer(c, offer_link, 1);
    }
}

void open_player_from_news(Cpu& c, std::uint32_t manager, std::uint32_t item) {
    Frame f(c, 0x10421af0, 0x130, {.first_gpr = 30});
    Kept<31> offer_id(c, manager); Kept<30> revision(c, item); Program p{c};
    p.call(0x102bdf90, 0x10421b1c, {f.at(0x38), 0, 0});
    p.call(0x102ccdd0, 0x10421b34, {indirect(c, -0x7bf0), offer_id, revision, f.at(0x38)});
    offer_id = p.call(0x102cb570, 0x10421b40, {f.at(0x38), 6});
    revision = p.call(0x102cb570, 0x10421b54, {f.at(0x38), 7});
    c.set(5, c.toc(slot::transfers));
    const auto offer_link = p.call(0x103eb440, 0x10421b68, {c.u32(c.r(5)), offer_id});
    c.set(4, c.u32(offer_link));
    bool previous = compare(c, c.u32(c.r(4) + 0x2c), revision) != 0;
    if (!previous) {
        c.set(4, c.u32(offer_link)); previous = compare(c, c.u8(c.r(4) + 0x34), 3) == 0;
        if (!previous) {
            c.set(4, c.u32(offer_link)); previous = compare(c, c.u8(c.r(4) + 0x34), 2) == 0;
        }
    }
    if (previous) {
        const auto old_offer = c.u32(offer_link);
        enter(c, 0x10421ba4, {old_offer, revision}); open_previous_offer(c, old_offer, revision);
    } else {
        c.set(5, c.u32(offer_link));
        const auto person = indirect(c, slot::people) + c.u32(c.r(5) + 12) * 0x70;
        enter(c, 0x10421bc8, {person, offer_link}); open_player_offer(c, person, offer_link);
    }
}

void open_contract_from_news(Cpu& c, std::uint32_t manager, std::uint32_t item) {
    Frame f(c, 0x10421be0, 0x130, {.first_gpr = 30});
    Kept<31> person_or_offer(c, manager); Kept<30> revision(c, item); Program p{c};
    p.call(0x102bdf90, 0x10421c0c, {f.at(0x38), 0, 0});
    p.call(0x102ccdd0, 0x10421c24, {indirect(c, -0x7bf0), person_or_offer, revision, f.at(0x38)});
    person_or_offer = p.call(0x102cb570, 0x10421c30, {f.at(0x38), 7});
    revision = p.call(0x102cb570, 0x10421c44, {f.at(0x38), 8});
    const auto person_id = p.call(0x102cb570, 0x10421c58, {f.at(0x38), 0});
    c.set({{4, c.toc(slot::people)}, {6, person_id * 0x70}, {5, c.toc(slot::transfers)}});
    const auto offer_id = std::uint32_t(person_or_offer);
    person_or_offer = c.u32(c.r(4)) + c.r(6);
    const auto offer_link = p.call(0x103eb440, 0x10421c78, {c.u32(c.r(5)), offer_id});
    c.set({{4, offer_link}, {3, c.u32(offer_link)}});
    bool previous = compare(c, c.u32(c.r(3) + 0x2c), revision) != 0;
    if (!previous) {
        c.set(3, c.u32(offer_link)); previous = compare(c, c.u8(c.r(3) + 0x34), 3) == 0;
    }
    if (previous) {
        const auto old_offer = c.u32(offer_link);
        enter(c, 0x10421ca8, {old_offer, revision}); open_previous_offer(c, old_offer, revision);
    } else p.call(0x10094ae0, 0x10421cbc, {person_or_offer, offer_link, 1, 0});
}

void open_offer_news_context(Cpu& c, std::uint32_t manager, std::uint32_t item) {
    Frame f(c, 0x10421ce0, 0x130, {.first_gpr = 28});
    Kept<31> screens(c, c.toc(slot::screens)); Kept<29> news_id(c, item);
    Kept<28> recipient(c, manager); Program p{c};
    p.call(0x102bdf90, 0x10421d18, {f.at(0x38), 0, 0});
    p.call(0x102ccdd0, 0x10421d30, {indirect(c, -0x7bf0), recipient, news_id, f.at(0x38)});
    const auto offer_id = p.call(0x102cb570, 0x10421d3c, {f.at(0x38), 9});
    c.set(5, c.toc(slot::transfers));
    const auto offer_link = p.call(0x103eb440, 0x10421d4c, {c.u32(c.r(5)), offer_id});
    Kept<30> selected(c, offer_link);
    if (!byte_result(c, p.call(0x1073e4b0, 0x10421d6c,
        {screens, c.toc(-0x4af8), c.toc(-0x4afc), 1, 0, 0}))) return;
    p.history(0x10421d88, screens, 0, recipient);
    p.history(0x10421d9c, screens, 1, news_id);
    p.history(0x10421db0, screens, 2, selected);
}

void reopen_player_from_news(Cpu& c, std::uint32_t manager, std::uint32_t item) {
    Frame f(c, 0x10421dd0, 0x130, {.first_gpr = 29});
    Kept<31> people(c, c.toc(slot::people)); Kept<30> offer_id(c, manager);
    Kept<29> revision(c, item); Program p{c};
    p.call(0x102bdf90, 0x10421e04, {f.at(0x38), 0, 0});
    p.call(0x102ccdd0, 0x10421e1c, {indirect(c, -0x7bf0), offer_id, revision, f.at(0x38)});
    offer_id = p.call(0x102cb570, 0x10421e28, {f.at(0x38), 11});
    revision = p.call(0x102cb570, 0x10421e3c, {f.at(0x38), 12});
    c.set(5, c.toc(slot::transfers));
    const auto offer_link = p.call(0x103eb440, 0x10421e50, {c.u32(c.r(5)), offer_id});
    c.set(4, c.u32(offer_link));
    bool previous = compare(c, c.u32(c.r(4) + 0x2c), revision) != 0;
    if (!previous) {
        c.set(4, c.u32(offer_link)); previous = compare(c, c.u8(c.r(4) + 0x34), 3) == 0;
        if (!previous) {
            c.set(4, c.u32(offer_link)); previous = compare(c, c.u8(c.r(4) + 0x34), 2) == 0;
        }
    }
    if (previous) {
        c.set({{3, c.u32(offer_link)}, {4, 0}, {5, c.u32(people)}});
        const auto person = c.r(5) + c.u32(c.r(3) + 12) * 0x70;
        enter(c, 0x10421e9c, {person, 0}); open_player_offer(c, person, 0);
    } else {
        c.set(5, c.u32(offer_link));
        const auto person = c.u32(people) + c.u32(c.r(5) + 12) * 0x70;
        enter(c, 0x10421ebc, {person, offer_link}); open_player_offer(c, person, offer_link);
    }
}
void open_completion_from_news(Cpu& c, std::uint32_t manager, std::uint32_t item) {
    Frame f(c, 0x10423040, 0x140, {.first_gpr = 25});
    Kept<31> news(c, item); Kept<27> handler(c, c.toc(-0x4b04));
    Kept<28> renderer(c, c.toc(-0x4b00)); Kept<30> recipient(c, manager);
    Kept<29> screens(c, c.toc(slot::screens)); Program p{c};
    p.call(0x102bdf90, 0x10423074, {f.at(0x38), 0, 0});
    p.call(0x102ccdd0, 0x1042308c, {indirect(c, -0x7bf0), recipient, news, f.at(0x38)});
    const auto offer_id = p.call(0x102cb570, 0x10423098, {f.at(0x38), 6});
    Kept<26> selected(c, offer_id);
    const auto news_revision = p.call(0x102cb570, 0x104230ac, {f.at(0x38), 7});
    c.set(5, c.toc(slot::transfers)); Kept<25> revision(c, news_revision);
    const auto offer_link = p.call(0x103eb440, 0x104230c0, {c.u32(c.r(5)), selected});
    bool previous = false;
    if (compare_logical(c, c.u32(offer_link)) != 0) {
        c.set(4, c.u32(offer_link)); previous = compare(c, c.u32(c.r(4) + 0x2c), revision) != 0;
    }
    // A null payload still reaches this original dereference. Do not add a new
    // recovery rule; callers provide a live offer link.
    if (!previous) {
        c.set(4, c.u32(offer_link)); previous = compare(c, c.u8(c.r(4) + 0x34), 3) == 0;
    }
    if (previous) {
        recipient = c.u32(offer_link);
        if (!byte_result(c, p.call(0x1073e4b0, 0x1042310c, {screens, renderer, handler, 1, 0, 0}))) return;
        p.history(0x10423128, screens, 0, recipient); p.history(0x1042313c, screens, 1, revision);
    } else {
        enter(c, 0x1042314c, {offer_link, recipient, news});
        open_transfer_completion_confirmation(c, offer_link, recipient, news);
    }
}
}

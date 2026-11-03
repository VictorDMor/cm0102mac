#include "contract_screens.hpp"
#include "contract_screens_calls.hpp"
#include "screen_manager_detail.hpp"

namespace cm::aot::contract_screens {
using namespace guest;
using detail::call_owned;
namespace {
using screen_manager::detail::Program;
constexpr int news_slot = -0x7bf0, people_slot = -0x7be0;
constexpr std::uint32_t initialize_news = 0x102bdf90, fetch_news = 0x102ccdd0, news_payload = 0x102cb570,
                        choice_entry = 0x10094ae0;

std::uint32_t person_from_payload(Cpu& c, std::uint32_t people, std::uint32_t index) {
    const auto offset = index * 0x70;
    c.set(0, offset);
    return c.u32(people) + offset;
}
}

void open_restricted_contract_from_news(Cpu& c, std::uint32_t item, std::uint32_t revision) {
    Frame frame(c, 0x10093a50, 0x130, {.first_gpr = 30});
    keep(c, {{30, item}, {31, revision}});
    c.set(0, c.lr());
    const auto record = frame.at(0x38);
    Program program{c};
    program.call(initialize_news, 0x10093a7c, {record, 0, 0});
    program.call(fetch_news, 0x10093a94, {c.u32(c.toc(news_slot)), item, revision, record});
    const auto person = program.call(news_payload, 0x10093aa0, {record, 2});
    call_owned(c, choice_entry, 0x10093ac0, open_contract_choice,
               person_from_payload(c, c.toc(people_slot), person), 0u, 0u, 1u);
}

void open_offer_contract_from_news(Cpu& c, std::uint32_t item, std::uint32_t revision) {
    Frame frame(c, 0x1009ed20, 0x130, {.first_gpr = 29});
    const auto people = c.toc(people_slot);
    keep(c, {{29, item}, {30, people}, {31, revision}});
    c.set(0, c.lr());
    const auto record = frame.at(0x38);
    Program program{c};
    program.call(initialize_news, 0x1009ed54, {record, 0, 0});
    program.call(fetch_news, 0x1009ed6c, {c.u32(c.toc(news_slot)), item, revision, record});
    const auto person_index = program.call(news_payload, 0x1009ed78, {record, 0});
    c.set(31, person_index);
    const auto message_kind = c.u32(record);
    c.set(0, message_kind);
    if (compare(c, message_kind, 0xfa4) == 0) {
        const auto offer_id = program.call(news_payload, 0x1009ed94, {record, 7});
        const auto offers = c.toc(-0x7b40);
        c.set(5, offers);
        const auto link = program.call(0x103eb440, 0x1009eda4, {c.u32(offers), offer_id});
        call_owned(c, choice_entry, 0x1009edc0, open_contract_choice,
                   person_from_payload(c, people, person_index), link, 1u, 0u);
    } else {
        call_owned(c, choice_entry, 0x1009ede0, open_contract_choice,
                   person_from_payload(c, people, person_index), 0u, 0u, 0u);
    }
}
}

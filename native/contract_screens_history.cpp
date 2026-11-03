#include "contract_screens.hpp"
#include "guest_values.hpp"

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
constexpr int screen_manager = -0x7bb0;
constexpr int news_manager = -0x7bf0;
constexpr int news_contract_draw = -0x56c4, news_contract_command = -0x56c8;
constexpr int offer_details_draw_slot = -0x5708, offer_details_command_slot = -0x570c;
constexpr int staff_contract_draw = -0x7994;
constexpr int staff_contract_command = -0x7998;
constexpr unsigned staff_history = 0;
constexpr unsigned draft_history = 2;
constexpr std::uint32_t append_screen = 0x1073e4b0, history_value = 0x1073ef70,
                        set_history_value = 0x1073f1e0, delete_object = 0x107735a0;

void set_history(Cpu& c, std::uint32_t screens, unsigned slot, std::uint32_t value, std::uint32_t back) {
    c.set({{3, screens}, {4, slot}, {5, value}, {6, 0}});
    c.call(set_history_value, back);
}

bool append_contract_screen(Cpu& c, std::uint32_t screens, int draw, int command, std::uint32_t back) {
    c.set({{3, screens}, {4, c.toc(draw)}, {5, c.toc(command)}, {6, 1}, {7, 0}, {8, 0}});
    c.call(append_screen, back);
    const auto opened = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(opened));
    return compare(c, opened) != 0;
}

struct OfferLink {
    Cpu& cpu;
    std::uint32_t address;
    std::uint32_t offer() const { return cpu.u32(address); }
};
}

void open_transfer_request_from_news(Cpu& c, std::uint32_t item, std::uint32_t revision) {
    Frame frame(c, 0x10093ae0, 0x140, {.first_gpr = 27});
    Kept<27> item_id(c, item);
    Kept<28> item_revision(c, revision);
    Kept<31> screens(c, c.toc(screen_manager));
    const auto news_item = frame.at(0x38);
    c.set({{0, c.lr()}, {3, news_item}, {4, 0}, {5, 0}});
    c.call(0x102bdf90, 0x10093b0c);
    c.set({{3, c.u32(c.toc(news_manager))}, {4, item_id}, {5, item_revision}, {6, news_item}});
    c.call(0x102ccdd0, 0x10093b24);
    c.set({{3, news_item}, {4, 0}});
    c.call(0x102cb570, 0x10093b30);
    c.set(0, c.r(3));
    Kept<29> first_payload(c, c.r(3));
    c.set({{3, news_item}, {4, 2}});
    c.call(0x102cb570, 0x10093b44);
    Kept<30> second_payload(c, c.r(3));
    if (!append_contract_screen(c, screens, news_contract_draw, news_contract_command, 0x10093b64)) return;

    set_history(c, screens, 0, item_id, 0x10093b80);
    set_history(c, screens, 1, item_revision, 0x10093b94);
    set_history(c, screens, 2, first_payload, 0x10093ba8);
    set_history(c, screens, 3, second_payload, 0x10093bbc);
}

void release_offer_draft(Cpu& c) {
    Frame frame(c, 0x100980d0, 0x40, {.first_gpr = 31});
    Kept<31> screens(c, c.toc(screen_manager));
    c.set({{0, c.lr()}, {3, screens}, {4, draft_history}});
    c.call(history_value, 0x100980f0);
    const auto draft = c.r(3);
    c.compare_logical(0, draft, 0);
    if (draft == 0) return;

    c.call(delete_object, 0x100980fc);
    set_history(c, screens, draft_history, 0, 0x10098110);
}

void open_offer_details(Cpu& c, std::uint32_t offer_link, std::uint32_t first,
                        std::uint32_t second, std::uint32_t third, std::uint32_t fifth,
                        std::uint32_t fourth) {
    Frame frame(c, 0x1009dab0, 0x60, {.first_gpr = 25});
    keep(c, {{25, offer_link}, {26, first}, {27, second}, {28, third}, {29, fifth}, {30, fourth}});
    Kept<31> screens(c, c.toc(screen_manager));
    c.set(0, c.lr());
    if (!append_contract_screen(c, screens, offer_details_draw_slot, offer_details_command_slot, 0x1009daf8)) return;

    set_history(c, screens, 0, offer_link, 0x1009db14);
    set_history(c, screens, 3, std::uint32_t(std::int8_t(first)), 0x1009db28);
    set_history(c, screens, 4, std::uint32_t(std::int8_t(second)), 0x1009db3c);
    set_history(c, screens, 5, std::uint32_t(std::int8_t(third)), 0x1009db50);
    set_history(c, screens, 6, std::uint32_t(std::int8_t(fourth)), 0x1009db64);
    set_history(c, screens, 7, std::uint32_t(std::int8_t(fifth)), 0x1009db78);
    const OfferLink link{c, offer_link};
    if (compare_logical(c, offer_link) != 0) {
        const auto offer = link.offer();
        c.set(3, offer);
        if (compare_logical(c, offer) != 0) {
            set_history(c, screens, 1, c.u32(offer + 0x28), 0x1009dba0);
            set_history(c, screens, 2, c.u32(link.offer() + 0x2c), 0x1009dbb8);
            return;
        }
    }
    set_history(c, screens, 1, 0xffffffff, 0x1009dbd0);
    set_history(c, screens, 2, 0xffffffff, 0x1009dbe4);
}

void open_staff_release(Cpu& c, std::uint32_t staff) {
    Frame frame(c, 0x1009e880, 0x40, {.first_gpr = 30});
    Kept<31> screens(c, c.toc(screen_manager));
    Kept<30> selected_staff(c, staff);
    c.set(0, c.lr());
    if (append_contract_screen(c, screens, staff_contract_draw, staff_contract_command, 0x1009e8b8))
        set_history(c, screens, staff_history, selected_staff, 0x1009e8d4);
}
}

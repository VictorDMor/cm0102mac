// MANAGER_MANAGER.CPP vacancy block (manager_manager_vacancy.hpp), native-first: the three small routines that file a
// news message with the news desk. Values live in native locals; the guest ABI appears only at the calls to code
// outside the block.
#include "manager_manager_vacancy.hpp"
#include "manager_manager_vacancy_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
using namespace vacancy;
namespace {
// The original's order test of the date copy against today's: the year halfwords (+2) compare unsigned, equal ones
// go through a bit trick on the two sign-extended day halfwords (+0) whose result is its sign bit. True when the
// copy is later than today's date; the original then files nothing.
bool copy_is_later(Cpu& c) {
    const auto copy = c.toc(toc_cell::today_copy), today = c.toc(toc_cell::date);
    const std::uint32_t copy_year = c.u16(copy + 2), today_year = c.u16(today + 2);
    if (copy_year > today_year) return true;
    if (copy_year < today_year) return false;
    const auto a = std::uint32_t(std::int32_t(c.i16(copy))), b = std::uint32_t(std::int32_t(c.i16(today)));
    const auto x = a ^ b;
    const auto result = std::uint32_t(std::int32_t(x) >> 1) - (x & a);
    return (result >> 31) != 0;
}
}

void file_vacancy_news(Cpu& c, std::uint32_t move, std::uint32_t manager_index, std::int8_t kind) {
    guest::Frame frame(c, 0x101bec80, 0x140, {.first_gpr = 28});
    if (!move || copy_is_later(c)) return;
    const auto buffer = frame.at(0x40);
    start_message(c, buffer, 0x29, 0x101bed24);
    const auto club_a = c.u32(move + 0x1c), club_b = c.u32(move + 0x20), person = c.u32(move + 0x14);
    set_field(c, buffer, 0, c.u32(club_a), 0x101bed38);
    set_field(c, buffer, 1, c.u32(club_b), 0x101bed4c);
    set_field(c, buffer, 2, c.u32(person), 0x101bed60);
    set_field(c, buffer, 3, manager_index, 0x101bed70);
    set_field(c, buffer, 4, c.u32(c.u32(person + 8) + 0x68), 0x101bed88);
    set_field(c, buffer, 5, sext8(kind), 0x101bed98);
    const auto desk = c.u32(c.toc(toc_cell::news_desk));
    c.set({{3, desk}, {4, buffer}, {5, club_a}});
    c.call(callee::file_message, 0x101beda8);
    c.set({{3, desk}, {4, buffer}, {5, club_b}});
    c.call(callee::file_message, 0x101bedb8);
}

void file_answer_news(Cpu& c, std::uint32_t club, std::int8_t first, std::int8_t second, std::int8_t third) {
    guest::Frame frame(c, 0x101bef90, 0x130, {.first_gpr = 28});
    if (!club || !c.u32(club + 0x2c)) return;
    const auto buffer = frame.at(0x38);
    start_message(c, buffer, 0x1789, 0x101befdc);
    set_field(c, buffer, 0, c.u32(club), 0x101befec);
    set_field(c, buffer, 1, sext8(first), 0x101beffc);
    set_field(c, buffer, 2, sext8(second), 0x101bf00c);
    set_field(c, buffer, 3, c.u32(c.u32(club + 0x2c) + 0x68), 0x101bf020);
    set_field(c, buffer, 4, sext8(third), 0x101bf030);
    c.set({{3, c.u32(c.toc(toc_cell::news_desk))}, {4, buffer}, {5, club}});
    c.call(callee::file_message, 0x101bf044);
}

void file_offer_news(Cpu& c, std::uint32_t id_pointer, std::uint32_t record, std::uint32_t first, std::uint32_t second) {
    guest::Frame frame(c, 0x101bf3a0, 0x130, {.first_gpr = 28});
    if (!record) return;
    const auto buffer = frame.at(0x38);
    start_message(c, buffer, 0x2b, 0x101bf3e0);
    set_field(c, buffer, 0, c.u32(record + 0x68), 0x101bf3f0);
    set_field(c, buffer, 1, first, 0x101bf400);
    set_field(c, buffer, 2, second, 0x101bf410);
    set_field(c, buffer, 3, c.u32(id_pointer), 0x101bf420);
    c.set({{3, c.u32(c.toc(toc_cell::news_desk))}, {4, buffer}, {5, record}});
    c.call(callee::file_message_c, 0x101bf434);
}
}

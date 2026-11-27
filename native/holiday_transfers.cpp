#include "holiday_transfers.hpp"
#include "cash.hpp"
#include "gui_guest_scope.hpp"
#include "holiday_digest.hpp"
#include "holiday_digest_store.hpp"
#include "holiday_return.hpp"
#include "platform.hpp"
#include <array>
#include <format>

namespace cm::aot::holiday_transfers {
namespace {
using guest::Cpu;

std::string text(Cpu& c, std::uint32_t address, unsigned limit) {
    std::string result;
    for (unsigned i = 0; address && i < limit; ++i) {
        const auto ch = c.u8(address + i);
        if (!ch) break;
        result += char(ch);
    }
    return result;
}

std::string person_name(Cpu& c, std::uint32_t person) {
    const auto common = text(c, c.u32(person + 0x30), 32);
    if (!common.empty()) return common;
    const auto first = text(c, c.u32(person + 0x28), 32);
    const auto last = text(c, c.u32(person + 0x2c), 32);
    return first.empty() ? last : last.empty() ? first : first + " " + last;
}

struct Label { const char* en; const char* pt; };
std::string role(Cpu& c, std::uint32_t person, unsigned job, bool pt) {
    // These are the club job codes used by DATABASE's job-name table at 0x100c6bf0.
    constexpr std::array<Label, 18> jobs{{
        {"Unemployed", "Sem emprego"}, {"Chairman", "Presidente"}, {"Managing director", "Diretor executivo"},
        {"General manager", "Diretor geral"}, {"Director of football", "Diretor de futebol"},
        {"Manager", "Treinador"}, {"Assistant manager", "Treinador adjunto"}, {"Reserves manager", "Treinador das reservas"},
        {"Coach", "Preparador"}, {"Scout", "Olheiro"}, {"Physio", "Fisioterapeuta"}, {"Player", "Jogador"},
        {"Player/manager", "Jogador/treinador"}, {"Player/assistant manager", "Jogador/adjunto"},
        {"Staff", "Comiss\343o t\351cnica"}, {"Player/coach", "Jogador/preparador"},
        {"Retired player", "Jogador aposentado"}, {"Media pundit", "Comentarista"}
    }};
    auto label = [pt](const Label& value) { return pt ? value.pt : value.en; };
    const auto profile = c.u32(person + 0x14);
    if (profile && (job == 11 || job == 0)) {
        // A free agent can have job 0. The playable positions are the seven
        // consecutive ratings before the right/left/centre side ratings.
        constexpr std::array<Label, 7> positions{{
            {"Goalkeeper","Goleiro"}, {"Sweeper","L\355bero"}, {"Defender","Defensor"},
            {"Defensive midfielder","Volante"}, {"Midfielder","Meio-campista"},
            {"Attacking midfielder","Meia ofensivo"}, {"Forward","Atacante"}
        }};
        unsigned best = 0;
        for (unsigned i = 1; i < positions.size(); ++i)
            if (c.i8(profile + 0xf + i) > c.i8(profile + 0xf + best)) best = i;
        if (c.i8(profile + 0xf + best) > 0) return label(positions[best]);
        return label(jobs[11]);
    }
    return job < jobs.size() ? label(jobs[job]) : label(jobs[14]);
}

std::string consideration(Cpu& c, std::uint32_t row, bool pt) {
    const auto kind = c.u8(row + 0x1c);
    const auto fee = c.i32(row + 0x18);
    std::string value;
    if (fee > 0) {
        // CASH's valid formatting path writes its output and private stack.
        // Use the same conversion/precision as the transfer history browser.
        GuiGuestScope preserve(c.s);
        c.set(1, c.sp() - 0x400);
        const auto output = c.sp() + 0x100;
        const auto displayed = cash::display_amount(c, row + 0x18, 5);
        cash::format(c, output, 200, displayed, 5);
        value = text(c, output, 199);
    }
    if (kind == 1) {
        const char* exchange = pt ? "Troca" : "Exchange";
        return value.empty() ? exchange : value + " + " + exchange;
    }
    if (kind == 2) {
        const char* loan = pt ? "Empr\351stimo" : "Loan";
        return value.empty() ? loan : value + " (" + loan + ")";
    }
    if (kind == 4 && value.empty()) return "Bosman";
    return value.empty() ? (pt ? "Sem custo" : "Free") : value;
}

void report(Cpu& c, std::uint32_t person, std::uint32_t from, std::uint32_t to,
            std::uint32_t date_address, std::uint32_t history_row, std::uint32_t offer) {
    if (from == to) return;
    const bool incoming = holiday_digest::tracking(c.s, to);
    const bool outgoing = holiday_digest::tracking(c.s, from);
    if (!incoming && !outgoing) return;
    const auto name = person_name(c, person);
    const auto pt = c.u8(c.r(2) - 0x35ac) == 9;
    const HolidayDate date{c.i16(date_address), c.u16(date_address + 2)};
    const auto month = holiday_return::month_index(date);
    const auto day = date.day - holiday_return::first_of_month(month, date.year).day + 1;
    const auto prefix = std::format("{:02}/{:02}/{}: {} - ", day, month + 1, date.year, name);
    const auto value = history_row ? consideration(c, history_row, pt) :
                                     (pt ? "Retorno de empr\351stimo" : "Loan return");
    const auto endpoint = [&](std::uint32_t address) -> std::string {
        if (!address) return pt ? "Sem clube" : "No club";
        const auto short_name = text(c, address + 0x221, 26);
        return short_name.empty() ? text(c, address + 0x1ee, 51) : short_name;
    };
    const auto suffix = " - " + value + " - " + endpoint(from) + " -> " + endpoint(to);
    const auto old_job = c.u8(person + 0x62);
    if (incoming) {
        const auto new_job = offer && c.u32(offer + 0xc) == c.u32(person) ? c.u8(offer + 0x11) : old_job;
        holiday_digest::record_event(c.s, to, "incoming", prefix + role(c, person, new_job, pt) + suffix);
    }
    if (outgoing)
        holiday_digest::record_event(c.s, from, "outgoing", prefix + role(c, person, old_job, pt) + suffix);
}
}

void recorded(Cpu& c, std::uint32_t row, std::uint32_t offer) {
    if (!c.s.platform || c.s.platform->holiday_digests().active_ids.empty()) return;
    const auto clubs = c.u32(c.toc(-0x7be8));
    const auto count = c.u32(c.toc(-0x7b50));
    const auto club = [&](unsigned offset) {
        const auto id = c.u32(row + offset);
        return clubs && id < count ? clubs + id * 0x248 : 0u;
    };
    const auto person_id = c.u32(row);
    const auto people = c.u32(c.toc(-0x7be0));
    if (!people || person_id >= c.u32(c.toc(-0x7be4))) return;
    report(c, people + person_id * 0x70, club(0x14), club(0x10), row + 0x20, row, offer);
}

void returned_from_loan(Cpu& c, std::uint32_t person, std::uint32_t from, std::uint32_t to) {
    if (!c.s.platform || c.s.platform->holiday_digests().active_ids.empty()) return;
    report(c, person, from, to, c.toc(-0x7b80), 0, 0);
}
}

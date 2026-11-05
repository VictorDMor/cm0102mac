#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"
#include "guest_fpu.hpp"
#include "shortlist_quotation_adapter.hpp"
#include <utility>

namespace cm::aot::staff_contracts {
namespace {
using detail::ContractRecord;
using guest::Cpu;
using guest::Fpu;

constexpr auto common_offset = reference_quote_entry - country_quote_entry;
constexpr auto frame_size = 0xc0u, high_word = 0x43300000u, club_stride = 0x248u;
constexpr auto player_profile = 0x14u, player_nation = 0x1cu, player_club = 0x20u;
struct QuoteCpu : Cpu {
    SemanticGuestObserver* const observer;
    explicit QuoteCpu(Cpu& c, SemanticGuestObserver* trace) : Cpu(c.s), observer(trace) {}
};

void quote_block(QuoteCpu& c, std::uint32_t pc, unsigned instructions) {
    if (c.observer) [[unlikely]] c.observer->before_step(c.s, pc);
    detail::block(c, pc, instructions);
}

constexpr auto player_age = 0x5cu, preferred_role = 0x60u, current_role = 0x62u;
constexpr auto club_rating = 0x1ecu, club_type = 0x23du;

template<bool Reference>
struct WageQuote {
    QuoteCpu& c;
    Fpu fp;
    const guest::Kept<15> player;
    const guest::Kept<31> manager_slot;
    const guest::Kept<16> target_club;
    const guest::Kept<26> eligibility_mode;
    const guest::Kept<17> quote_mode;
    const guest::Kept<27> reputation_manager;
    const guest::Kept<18> role_override;
    guest::Kept<21> band;
    const guest::Kept<22> category;

    WageQuote(QuoteCpu& cpu, std::uint32_t player_address, std::uint32_t club, std::uint32_t mode,
              std::uint32_t override_role, std::uint32_t category_id, std::uint32_t adjustment)
        : c(cpu), fp(cpu), player(cpu, player_address), manager_slot(cpu, cpu.toc(-0x7b7c)),
          target_club(cpu, club), eligibility_mode(cpu, cpu.toc(-0x7a68)), quote_mode(cpu, mode),
          reputation_manager(cpu, cpu.toc(-0x7ab8)), role_override(cpu, override_role),
          band(cpu, cpu.toc(-0x7b78)), category(cpu, category_id) {
        c.set(28, c.toc(-0x7a18));
        c.set(19, adjustment);
        c.set(14, c.toc(-0x7a14));
        c.set(20, 0);
        c.set(13, c.toc(-0x7a10));
        c.set(29, c.toc(-0x4dac));
        c.set(30, c.toc(-0x4da8));
    }

    static constexpr auto site(std::uint32_t country) {
        return country + (Reference ? common_offset : 0);
    }
    void block(std::uint32_t country, unsigned count) { quote_block(c, site(country), count); }
    double number(unsigned offset) const { return c.f64(c.r(29) + offset); }
    double single(unsigned offset) const { return c.f32(c.r(30) + offset); }
};

template<bool Reference>
void resolve_current_club(WageQuote<Reference>& quote) {
    QuoteCpu& c = quote.c;
    c.set(3, c.u32(quote.manager_slot));
    c.call(0x10082f20, quote.site(0x10378e00));

    quote.block(0x10378e00, 3);
    const auto contract = c.r(3);
    const auto has_contract = guest::nonzero_logical(c, contract);
    c.put32(quote.band, contract);
    if (has_contract) {
        quote.block(0x10378e0c, 6);
        const auto packed = c.u8(contract + ContractRecord::packed_kind);
        const auto kind = detail::unpack_kind(packed);
        c.s.wr<1>(guest::reg::xer_ca, (packed & 0x82) == 0x82);
        c.set(0, std::uint32_t(kind));
        if (guest::compare(c, kind, 5) != 0) {
            quote.block(0x10378e24, 1);
            c.set(20, 1);
        }
    }

    quote.block(0x10378e28, 2);
    if (!guest::nonzero(c, std::int32_t(c.r(20)))) {
        quote.block(0x10378e4c, 1);
        c.set(25, 0);
        return;
    }

    quote.block(0x10378e30, 7);
    const auto live = c.u32(quote.band);
    c.set(3, live);
    const auto club_slot = c.toc(-0x7be8);
    c.set(4, club_slot);
    const auto index = c.u32(live + ContractRecord::club_id);
    c.set(0, index);
    const auto clubs = c.u32(club_slot);
    c.set({{3, clubs}, {0, index * club_stride}, {25, clubs + index * club_stride}});
}

template<bool Reference>
bool initialize_player_quote(WageQuote<Reference>& quote) {
    QuoteCpu& c = quote.c;
    quote.block(0x10378e50, 21);
    const auto denominator_slot = c.toc(-0x7be4);
    c.set({{3, denominator_slot}, {4, high_word}});
    const auto id = c.u32(quote.player);
    c.set(7, id);
    const auto has_destination = guest::nonzero_logical(c, quote.target_club);
    const auto denominator = c.u32(denominator_slot);
    c.set({{3, denominator}, {5, id ^ 0x80000000}});
    const auto age = c.i16(quote.player + player_age);
    c.set({{0, guest::extended(age)}, {3, denominator ^ 0x80000000}});
    const auto frame = c.sp();
    c.put32(frame + 0x54, id ^ 0x80000000);
    c.set_f(2, quote.number(0xc0));
    c.set(20, std::uint32_t(std::int8_t(age)));
    c.put32(frame + 0x50, high_word);
    c.set(24, c.u32(quote.player + player_profile));
    c.set_f(0, c.f64(frame + 0x50));
    c.put32(frame + 0x4c, denominator ^ 0x80000000);
    c.set_f(1, quote.fp.fsubs(c.f(0), c.f(2)));
    c.put32(frame + 0x48, high_word);
    c.set_f(0, c.f64(frame + 0x48));
    c.set_f(0, quote.fp.fsubs(c.f(0), c.f(2)));
    c.set_f(30, quote.fp.fdivs(c.f(1), c.f(0)));
    return has_destination;
}

void keep_rating_division(QuoteCpu& c, std::int16_t rating) {
    const auto high = (INT64_C(0x10624dd3) * rating) >> 32;
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 31));
}

template<bool Reference>
void use_rating_band(WageQuote<Reference>& quote, std::uint32_t object, unsigned offset,
                     std::uint32_t start) {
    QuoteCpu& c = quote.c;
    quote.block(start, 10);
    c.set(3, 0x10620000);
    const auto rating = c.i16(object + offset);
    const auto scaled = rating / 500;
    keep_rating_division(c, rating);
    c.set({{3, rating < 0 ? 1u : 0u}, {4, 1}, {0, std::uint32_t(scaled)}});
    if (guest::compare(c, scaled, 1) >= 0) {
        quote.block(start + 40, 1);
        c.set(4, std::uint32_t(scaled));
    }
    quote.block(start + 44, 3);
    const auto band = std::int8_t(c.r(4));
    c.set(0, std::uint32_t(band));
    quote.band = std::uint32_t(band);
}

template<bool Reference>
void select_rating_band(WageQuote<Reference>& quote, bool has_destination) {
    QuoteCpu& c = quote.c;
    if (!has_destination) {
        quote.block(0x10378f00, 2);
        const auto profile = c.r(24);
        if (guest::nonzero_logical(c, profile)) {
            use_rating_band(quote, profile, 12, 0x10378f08);
            return;
        }
        quote.block(0x10378f40, 3);
        const auto nation = c.u32(quote.player + player_nation);
        c.set(4, nation);
        if (guest::nonzero_logical(c, nation)) use_rating_band(quote, nation, 0x2c, 0x10378f4c);
        else {
            quote.block(0x10378f84, 1);
            quote.band = 1;
        }
        return;
    }

    quote.block(0x10378ea4, 12);
    c.set(3, 0x10620000);
    const auto rating = c.i16(quote.target_club + club_rating);
    c.set({{0, guest::extended(rating)}, {3, 0x10624dd3}});
    const auto has_current_club = guest::nonzero_logical(c, c.r(25));
    keep_rating_division(c, rating);
    const auto base = rating / 500;
    const auto band = std::int8_t(base + 1);
    c.set({{0, std::uint32_t(base + 1)}, {3, std::uint32_t(band)}});
    quote.band = std::uint32_t(band);
    if (!has_current_club) return;

    quote.block(0x10378ed4, 3);
    const auto type = c.u8(quote.target_club + club_type);
    c.set(0, type);
    if (guest::compare(c, type, 2) != 0) return;

    quote.block(0x10378ee0, 3);
    const auto adjusted = int(band) - 1;
    quote.band = std::uint32_t(adjusted);
    if (guest::compare(c, adjusted, 1) < 0) {
        quote.block(0x10378eec, 1);
        quote.band = 1;
    }
    quote.block(0x10378ef0, 2);
    if (guest::compare(c, std::int32_t(std::uint32_t(quote.band)), 4) > 0) {
        quote.block(0x10378ef8, 2);
        quote.band = 4;
    }
}

template<bool Reference>
bool quote_role_field(WageQuote<Reference>& quote, unsigned field, std::uint32_t start) {
    QuoteCpu& c = quote.c;
    quote.block(start, 3);
    const auto raw = c.u8(quote.player + field);
    c.set(3, raw);
    if (!detail::record_nonzero(c, std::int8_t(raw))) return false;
    quote.block(start + 12, 2);
    c.set(0, raw);
    return true;
}

template<bool Reference>
void choose_quote_role(WageQuote<Reference>& quote) {
    QuoteCpu& c = quote.c;
    quote.block(0x10378f88, 2);
    if (guest::nonzero_logical(c, c.r(25)) && quote_role_field(quote, current_role, 0x10378f90)) return;

    quote.block(0x10378fa4, 3);
    const auto linked_club = c.u32(quote.player + player_club);
    c.set(0, linked_club);
    if (guest::nonzero_logical(c, linked_club) && quote_role_field(quote, preferred_role, 0x10378fb0)) return;
    if (quote_role_field(quote, current_role, 0x10378fc4)) return;

    quote.block(0x10378fd8, 2);
    if (guest::nonzero_logical(c, c.r(24))) {
        quote.block(0x10378fe0, 2);
        c.set(0, 11);
        return;
    }
    if (quote_role_field(quote, preferred_role, 0x10378fe8)) return;
    quote.block(0x10378ffc, 1);
    c.set(0, 9);
}

template<bool Reference>
std::optional<std::uint32_t> select_quote_case(WageQuote<Reference>& quote) {
    QuoteCpu& c = quote.c;
    quote.block(0x10379000, 4);
    const auto role = std::int8_t(c.r(0));
    const auto index = std::uint32_t(int(role) - 5);
    c.set({{3, std::uint32_t(role)}, {0, index}});
    if (guest::compare_logical(c, index, 10) > 0) return std::nullopt;

    quote.block(0x10379010, 5);
    const auto table = c.toc(Reference ? -0x4dbc : -0x4db8);
    c.set({{3, table}, {0, index * 4}});
    const auto target = c.u32(table + index * 4);
    c.set(3, target);
    c.s.wr<4>(guest::reg::ctr, target);
    return target;
}

template<bool Reference>
int band_for_club(WageQuote<Reference>& q, std::uint32_t club, int bias, unsigned destination) {
    QuoteCpu& c = q.c;
    c.set(3, 0x10620000);
    const auto rating = c.i16(club + club_rating);
    const auto high = (INT64_C(0x10624dd3) * rating) >> 32;
    const auto floor = std::int32_t(high >> 5);
    keep_rating_division(c, rating);
    const auto truncated = rating / 500;
    const auto band = truncated + bias;
    c.set({{0, std::uint32_t(floor)}, {3, std::uint32_t(truncated)}, {destination, std::uint32_t(band)}});
    return band;
}

template<bool Reference>
void raise_role_band(WageQuote<Reference>& q, std::uint32_t role_site, bool specialist) {
    QuoteCpu& c = q.c;
    q.block(role_site, 2);
    const auto current_club = c.r(25);
    if (guest::nonzero_logical(c, current_club)) {
        q.block(role_site + 8, 10);
        const auto band = band_for_club(q, current_club, 1, 0);
        if (guest::compare(c, band, std::int32_t(std::uint32_t(q.band))) > 0) {
            q.block(role_site + 0x30, 2);
            q.band = std::uint32_t(band);
            return;
        }
    }

    q.block(role_site + 0x38, 3);
    const auto nation = c.u32(q.player + player_nation);
    c.set(0, nation);
    if (!guest::nonzero_logical(c, nation)) return;
    q.block(role_site + 0x44, 3);
    const auto linked_club = c.u32(q.player + player_club);
    c.set(4, linked_club);
    if (!guest::nonzero_logical(c, linked_club)) return;

    if (specialist) {
        q.block(role_site + 0x50, 3);
        const auto role = c.u8(q.player + preferred_role);
        c.set(0, role);
        if (guest::compare(c, role, 11) == 0) return;
    }
    q.block(role_site + (specialist ? 0x5c : 0x50), 10);
    const auto band = band_for_club(q, linked_club, 1, specialist ? 0 : 3);
    if (guest::compare(c, band, std::int32_t(std::uint32_t(q.band))) <= 0) return;

    if (!specialist) {
        q.block(role_site + 0x78, 3);
        const auto role = c.u8(q.player + preferred_role);
        c.set(0, role);
        if (guest::compare(c, role, 11) == 0) return;
    }
    q.block(role_site + 0x84, 1);
    q.band = std::uint32_t(band);
}

struct RoleQuote {
    std::uint32_t entry, value_site;
    unsigned weight, minimum;
    bool specialist, bounded;
};

constexpr std::array role_quotes{
    RoleQuote{0x10379024, 0x103790ac, 0xd0, 0xd4, true, false},
    RoleQuote{0x1037918c, 0x10379214, 0x14, 0xa4, false, false},
    RoleQuote{0x10379378, 0x10379400, 0x14, 0xa4, false, false},
    RoleQuote{0x1037955c, 0x103795e4, 0x2c, 0xdc, false, true},
    RoleQuote{0x10379718, 0x103797a0, 0xa8, 0xe0, false, true},
    RoleQuote{0x10379884, 0x1037990c, 0xa8, 0xe4, false, true},
};

template<bool Reference>
void compute_role_value(WageQuote<Reference>& q, const RoleQuote& role) {
    QuoteCpu& c = q.c;
    const auto site = role.value_site;
    const auto output = role.bounded ? 4u : 3u;
    q.block(site, 3);
    const auto nation = c.u32(q.player + player_nation);
    c.set(3, nation);
    if (!guest::nonzero_logical(c, nation)) {
        q.block(site + 0x6c, 8);
        c.set_f(1, q.single(0x40));
        c.set_f(0, q.single(0x58));
        c.set_f(2, q.single(role.weight));
        c.set_f(1, q.fp.multiply_add(q.site(site + 0x78), c.f(1), c.f(30), c.f(0)));
        c.set_f(0, q.number(0x58));
        c.set_f(1, q.fp.fmuls(c.f(2), c.f(1)));
        c.set_f(output, q.fp.fdiv(c.f(1), c.f(0)));
        c.set_f(output, q.fp.frsp(c.f(output)));
        return;
    }

    q.block(site + 12, 24);
    const auto strength = c.i16(nation + 0x2a);
    c.set({{3, guest::extended(strength)}, {0, high_word}});
    const auto first = c.sp() + (role.specialist ? 0x50 : 0x40);
    const auto second = c.sp() + (role.specialist ? 0x40 : 0x48);
    const auto third = c.sp() + (role.specialist ? 0x48 : 0x38);
    c.put32(first, high_word);
    const auto encoded = std::uint32_t(strength) ^ 0x80000000;
    c.set(3, encoded);
    c.set_f(4, q.number(0xc0));
    c.put32(first + 4, encoded);
    c.set_f(5, q.single(role.weight));
    c.set_f(0, c.f64(first));
    c.put32(second + 4, encoded);
    c.set_f(2, q.fp.fsub(c.f(0), c.f(4)));
    c.set_f(0, q.number(0x1a8));
    c.put32(second, high_word);
    c.set_f(1, c.f64(second));
    c.put32(third + 4, encoded);
    c.set_f(1, q.fp.fsub(c.f(1), c.f(4)));
    c.put32(third, high_word);
    c.set_f(3, c.f64(third));
    c.set_f(1, q.fp.fmul(c.f(2), c.f(1)));
    c.set_f(2, q.fp.fsub(c.f(3), c.f(4)));
    c.set_f(1, q.fp.fmul(c.f(2), c.f(1)));
    c.set_f(1, q.fp.fmul(c.f(5), c.f(1)));
    c.set_f(output, q.fp.fdiv(c.f(1), c.f(0)));
    c.set_f(output, q.fp.frsp(c.f(output)));
}

template<bool Reference>
void combine_role_value(WageQuote<Reference>& q, const RoleQuote& role) {
    QuoteCpu& c = q.c;
    const auto site = role.value_site + 0x8c;
    q.block(site, 14);
    const auto encoded = std::uint32_t(q.band) ^ 0x80000000;
    c.set({{3, encoded}, {0, high_word}});
    const auto slot = c.sp() + (role.specialist ? 0x38 : 0x50);
    c.put32(slot + 4, encoded);
    c.set_f(1, q.number(0xc0));
    c.put32(slot, high_word);
    c.set_f(2, q.number(0x48));
    c.set_f(0, c.f64(slot));
    const auto minimum = role.bounded ? 3u : 31u;
    c.set_f(minimum, q.single(role.minimum));
    c.set_f(0, q.fp.fsubs(c.f(0), c.f(1)));
    c.set_f(0, q.fp.fmuls(c.f(role.bounded ? 4 : 3), c.f(0)));
    c.set_f(0, q.fp.fmul(c.f(2), c.f(0)));
    c.set_f(0, q.fp.frsp(c.f(0)));
    q.fp.fcmp(0, c.f(minimum), c.f(0));
    if (c.cr(0) & 4) q.block(site + 0x38, 1);
    else {
        q.block(site + 0x3c, 1);
        c.set_f(minimum, c.f(0));
    }
}

template<bool Reference>
void cap_role_value(WageQuote<Reference>& q, std::uint32_t site, unsigned limit, unsigned replacement) {
    QuoteCpu& c = q.c;
    q.block(site, 3);
    c.set_f(0, q.single(limit));
    q.fp.fcmp(0, c.f(31), c.f(0));
    if (c.cr(0) & 4) {
        q.block(site + 12, 2);
        c.set_f(31, q.single(replacement));
    }
}

template<bool Reference>
void limit_to_role_band(WageQuote<Reference>& q, const RoleQuote& role) {
    QuoteCpu& c = q.c;
    const auto site = role.value_site + 0xcc;
    q.block(site, 3);
    c.set_f(31, q.single(role.weight));
    q.fp.fcmp(0, c.f(31), c.f(3));
    const bool below = (c.cr(0) & 8) != 0;
    if (below) {
        q.block(site + 12, 1);
        if (role.entry != 0x1037955c) return;
    } else {
        q.block(site + 16, role.entry == 0x1037955c ? 1 : 2);
        c.set_f(31, c.f(3));
        if (role.entry != 0x1037955c) return;
    }

    q.block(0x103796c4, 2);
    const auto current = c.r(25);
    if (!guest::nonzero_logical(c, current)) return;
    q.block(0x103796cc, 2);
    if (guest::compare_logical(c, current, q.target_club) == 0) return;
    q.block(0x103796d4, 3);
    const auto category = std::int8_t(std::uint32_t(q.category));
    c.set(0, std::uint32_t(category));
    if (guest::compare(c, category, 6) == 0) return;
    q.block(0x103796e0, 2);
    if (guest::compare(c, category, 11) == 0) return;
    q.block(0x103796e8, 10);
    const auto band = band_for_club(q, current, 4, 0);
    if (guest::compare(c, band, std::int32_t(std::uint32_t(q.band))) > 0) {
        q.block(0x10379710, 2);
        c.set_f(31, q.single(0xd8));
    }
}

template<bool Reference>
void cap_unprotected_role(WageQuote<Reference>& q, std::uint32_t site) {
    QuoteCpu& c = q.c;
    q.block(site, 3);
    const auto category = std::int8_t(std::uint32_t(q.category));
    c.set(0, std::uint32_t(category));
    if (guest::compare(c, category, 11) == 0) return;
    q.block(site + 12, 2);
    if (guest::compare(c, category, 6) == 0) return;
    cap_role_value(q, site + 20, 0xd0, 0xd8);
}

template<bool Reference>
void restrict_contract_role(WageQuote<Reference>& q, const RoleQuote& role) {
    QuoteCpu& c = q.c;
    const auto base = role.value_site + 0xcc;
    const auto first_role = role.entry == 0x1037918c;
    q.block(base, 2);
    const auto current = c.r(25);
    if (guest::nonzero_logical(c, current)) {
        q.block(base + 8, 2);
        if (guest::compare_logical(c, current, q.target_club) != 0) {
            if (!first_role) {
                q.block(0x103794dc, 10);
                const auto band = band_for_club(q, current, 4, 0);
                if (guest::compare(c, band, std::int32_t(std::uint32_t(q.band))) <= 0)
                    cap_unprotected_role(q, 0x1037950c);
                else {
                    q.block(0x10379504, 2);
                    c.set_f(31, q.single(0xd8));
                }
                return;
            }

            q.block(0x103792f0, 3);
            const auto category = std::int8_t(std::uint32_t(q.category));
            c.set(0, std::uint32_t(category));
            bool allowed = guest::compare(c, category, 11) == 0;
            if (!allowed) {
                q.block(0x103792fc, 2);
                allowed = guest::compare(c, category, 6) == 0;
            }
            if (allowed) {
                q.block(0x10379304, 10);
                const auto band = band_for_club(q, current, 4, 0);
                if (guest::compare(c, band, std::int32_t(std::uint32_t(q.band))) <= 0) {
                    q.block(0x10379334, 3);
                    c.set_f(0, q.single(0x80));
                    q.fp.fcmp(0, c.f(31), c.f(0));
                    if (c.cr(0) & 8) {
                        q.block(0x10379340, 4);
                        c.set_f(0, q.number(0x148));
                        c.set_f(31, q.fp.fmul(c.f(0), c.f(31)));
                        c.set_f(31, q.fp.frsp(c.f(31)));
                    }
                    return;
                }
            }
            q.block(0x1037932c, 2);
            c.set_f(31, q.single(0xd8));
            return;
        }
    }
    cap_unprotected_role(q, first_role ? 0x10379350 : 0x10379534);
}

template<bool Reference>
void quote_ranked_role(WageQuote<Reference>& q, const RoleQuote& role) {
    raise_role_band(q, role.entry, role.specialist);
    compute_role_value(q, role);
    combine_role_value(q, role);
    if (role.bounded) limit_to_role_band(q, role);
    else if (role.specialist) cap_role_value(q, role.value_site + 0xcc, 0xd0, 0xd8);
    else restrict_contract_role(q, role);
}

template<bool Reference>
bool profile_exceeds_club(WageQuote<Reference>& q, std::uint32_t site, std::int16_t profile,
                          std::int16_t club, unsigned multiplier, bool first) {
    QuoteCpu& c = q.c;
    q.block(site, 16);
    const auto frame = c.sp();
    const auto club_word = std::uint32_t(club) ^ 0x80000000;
    c.set({{3, high_word}, {0, club_word}});
    c.put32(frame + 0x44, club_word);
    const auto profile_word = std::uint32_t(profile) ^ 0x80000000;
    c.set(0, profile_word);
    c.set_f(3, q.number(0xc0));
    c.put32(frame + 0x40, high_word);
    c.set_f(1, q.number(multiplier));
    c.set_f(0, c.f64(frame + 0x40));
    c.put32(frame + 0x3c, profile_word);
    c.set_f(0, q.fp.fsub(c.f(0), c.f(3)));
    c.put32(frame + 0x38, high_word);
    c.set_f(2, c.f64(frame + 0x38));
    const auto threshold = first ? 4u : 0u;
    const auto value = first ? 0u : 1u;
    c.set_f(threshold, q.fp.fmul(c.f(1), c.f(0)));
    c.set_f(value, q.fp.fsub(c.f(2), c.f(3)));
    q.fp.fcmp(0, c.f(value), c.f(threshold));
    return (c.cr(0) & 4) != 0;
}

template<bool Reference>
bool choose_profile_mode(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    q.block(0x103799f0, 2);
    const auto profile = c.r(24);
    if (!guest::nonzero_logical(c, profile)) return false;

    q.block(0x103799f8, 3);
    const auto mode = std::int8_t(std::uint32_t(q.role_override));
    c.set(0, std::uint32_t(mode));
    if (guest::compare(c, mode, -1) != 0) return true;
    q.block(0x10379a04, 2);
    if (!guest::nonzero_logical(c, q.target_club)) return true;

    q.block(0x10379a0c, 4);
    const auto profile_rating = c.i16(profile + 10);
    c.set(5, guest::extended(profile_rating));
    const auto club = c.i16(q.target_club + club_rating);
    c.set(4, guest::extended(club));
    const auto age = std::int32_t(c.r(20));
    if (guest::compare(c, profile_rating, club) > 0) {
        q.block(0x10379a1c, 2);
        if (guest::compare(c, age, 24) >= 0) {
            q.block(0x10379a24, 2);
            if (guest::compare(c, age, 34) < 0) {
                q.block(0x10379a2c, 2);
                c.set(18, 1);
                return true;
            }
        }
    }
    if (profile_exceeds_club(q, 0x10379a34, profile_rating, club, 0x30, true)) {
        q.block(0x10379a74, 2);
        if (guest::compare(c, age, 21) >= 0) {
            q.block(0x10379a7c, 2);
            c.set(18, 2);
            return true;
        }
    }
    if (profile_exceeds_club(q, 0x10379a84, profile_rating, club, 0x28, false)) {
        q.block(0x10379ac4, 2);
        if (guest::compare(c, age, 17) >= 0) {
            q.block(0x10379acc, 2);
            c.set(18, 4);
            return true;
        }
    }
    q.block(0x10379ad4, 2);
    if (guest::compare(c, age, 24) <= 0) {
        q.block(0x10379adc, 11);
        const auto seasons = c.i16(profile + 6);
        c.set({{3, guest::extended(seasons)}, {0, high_word}});
        c.put32(c.sp() + 0x38, high_word);
        const auto points = std::uint32_t(seasons * 50);
        c.set(0, points);
        c.set_f(1, q.number(0xc0));
        const auto encoded = points ^ 0x80000000;
        c.set(0, encoded);
        c.put32(c.sp() + 0x3c, encoded);
        c.set_f(0, c.f64(c.sp() + 0x38));
        c.set_f(0, q.fp.fsub(c.f(0), c.f(1)));
        q.fp.fcmp(0, c.f(0), c.f(4));
        if (c.cr(0) & 4) {
            q.block(0x10379b08, 2);
            c.set(18, 5);
            return true;
        }
    }
    q.block(0x10379b10, 2);
    if (guest::compare(c, age, 24) <= 0) {
        q.block(0x10379b18, 2);
        c.set(18, 6);
    } else {
        q.block(0x10379b20, 1);
        c.set(18, 4);
    }
    return true;
}

template<bool Reference>
void profile_floor_at(WageQuote<Reference>& q, std::uint32_t pc, unsigned coefficient, unsigned instructions = 2) {
    quote_block(q.c, pc, instructions);
    q.c.set_f(31, q.single(coefficient));
}

int signed_byte_at(QuoteCpu& c, std::uint32_t pc, std::uint32_t object, unsigned field, int boundary) {
    quote_block(c, pc, 4);
    const auto value = c.i8(object + field);
    c.set(0, std::uint32_t(value));
    return guest::compare(c, value, boundary);
}

int signed_short_at(QuoteCpu& c, std::uint32_t pc, std::uint32_t object, unsigned field, int boundary) {
    quote_block(c, pc, 3);
    const auto value = c.i16(object + field);
    c.set(0, guest::extended(value));
    return guest::compare(c, value, boundary);
}

bool either_signed_mode(QuoteCpu& c, std::uint32_t site, std::uint32_t raw, int first, int second) {
    quote_block(c, site, 3);
    const auto mode = std::int8_t(raw);
    c.set(0, std::uint32_t(mode));
    if (guest::compare(c, mode, first) == 0) return true;
    quote_block(c, site + 12, 2);
    return guest::compare(c, mode, second) == 0;
}

bool compare_age(QuoteCpu& c, std::uint32_t pc, std::int32_t age, int boundary, bool greater) {
    quote_block(c, pc, 2);
    const auto result = guest::compare(c, age, boundary);
    return greater ? result > 0 : result < 0;
}

template<bool Reference>
bool fixed_profile_floor(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    if constexpr (Reference) {
        quote_block(c, 0x1037c474, 3);
        const auto adjustment = std::int8_t(c.r(19));
        c.set(0, std::uint32_t(adjustment));
        if (guest::compare(c, adjustment, 3) == 0) {
            profile_floor_at(q, 0x1037c480, 0x18);
            return true;
        }
        quote_block(c, 0x1037c488, 2);
        if (guest::compare(c, adjustment, 1) == 0) {
            const bool less_available = signed_byte_at(c, 0x1037c490, q.player, 0x65, 15) < 0;
            const bool high_morale = less_available && signed_byte_at(c, 0x1037c4a0, q.player, 0x66, 15) > 0;
            profile_floor_at(q, high_morale ? 0x1037c4b0 : 0x1037c4b8, high_morale ? 0x134 : 0x6c);
            return true;
        }
        quote_block(c, 0x1037c4c0, 2);
        if (guest::compare(c, adjustment, 2) == 0) {
            const auto age = std::int32_t(c.r(20));
            const bool prime = compare_age(c, 0x1037c4c8, age, 22, true)
                && compare_age(c, 0x1037c4d0, age, 32, false);
            profile_floor_at(q, prime ? 0x1037c4d8 : 0x1037c4e0, prime ? 0x6c : 0x138);
            return true;
        }
    }

    constexpr auto site = Reference ? 0x1037c4e8u : 0x10379b24u;
    const auto profile = c.r(24);
    const auto age = std::int32_t(c.r(20));
    if (either_signed_mode(c, site, q.category, 13, 15) && compare_age(c, site + 20, age, 35, true)) {
        profile_floor_at(q, site + 28, 0x14);
        return true;
    }
    if (either_signed_mode(c, site + 36, q.role_override, 6, 5)
        && signed_short_at(c, site + 56, profile, 10, 2500) < 0) {
        profile_floor_at(q, site + 68, Reference ? 0x13c : 0xe8);
        return true;
    }
    if (either_signed_mode(c, site + 76, q.role_override, 6, 5)
        && signed_short_at(c, site + 96, profile, 10, 6250) < 0) {
        profile_floor_at(q, site + 108, Reference ? 0x10c : 0x14);
        return true;
    }
    return false;
}

template<bool Reference>
void country_profile_floor(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    const auto profile = c.r(24);
    if (signed_byte_at(c, 0x10379b98, profile, 0x15, 15) >= 0) {
        if (signed_short_at(c, 0x10379ba8, profile, 10, 6750) <= 0) profile_floor_at(q, 0x10379bd4, 0xf4);
        else if (signed_byte_at(c, 0x10379bb4, q.player, 0x65, 15) < 0) profile_floor_at(q, 0x10379bc4, 0xec);
        else profile_floor_at(q, 0x10379bcc, 0xf0);
        return;
    }
    if (signed_byte_at(c, 0x10379bdc, profile, 0x12, 15) >= 0
        || signed_byte_at(c, 0x10379bec, profile, 0x14, 15) >= 0
        || signed_byte_at(c, 0x10379bfc, profile, 0x13, 15) >= 0) {
        if (signed_short_at(c, 0x10379c0c, profile, 10, 7250) <= 0) profile_floor_at(q, 0x10379c38, 0x100);
        else if (signed_byte_at(c, 0x10379c18, q.player, 0x65, 14) < 0) profile_floor_at(q, 0x10379c28, 0xf8);
        else profile_floor_at(q, 0x10379c30, 0xfc);
        return;
    }

    quote_block(c, 0x10379c40, 3);
    const auto qualification = c.u8(profile + 0xf);
    c.set(0, qualification);
    if (guest::compare(c, qualification, 20) == 0) {
        const bool selected = signed_short_at(c, 0x10379c4c, profile, 12, 7750) > 0
            && signed_byte_at(c, 0x10379c58, q.player, 0x65, 3) < 0;
        profile_floor_at(q, selected ? 0x10379c68 : 0x10379c70, selected ? 0xd0 : 0x104);
        return;
    }
    bool qualifies = signed_byte_at(c, 0x10379c78, profile, 0x10, 15) >= 0;
    if (!qualifies) {
        quote_block(c, 0x10379c88, 3);
        qualifies = detail::record_nonzero(c, c.i8(profile + 0x11));
    }
    if (qualifies) {
        const bool selected = signed_short_at(c, 0x10379c94, profile, 12, 7500) > 0
            && signed_byte_at(c, 0x10379ca0, q.player, 0x65, 6) < 0;
        profile_floor_at(q, selected ? 0x10379cb0 : 0x10379cb8, selected ? 0x108 : 0xd0);
    } else profile_floor_at(q, 0x10379cc0, 0xd0, 1);
}

template<bool Reference>
void reference_profile_floor(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    const auto profile = c.r(24);
    const auto age = std::int32_t(c.r(20));
    if (signed_byte_at(c, 0x1037c55c, profile, 0x15, 15) >= 0) {
        const bool qualifies = signed_short_at(c, 0x1037c56c, profile, 10, 6750) > 0
            && compare_age(c, 0x1037c578, age, 31, false);
        if (!qualifies) profile_floor_at(q, 0x1037c5a0, 0xf0);
        else if (signed_byte_at(c, 0x1037c580, q.player, 0x65, 15) < 0) profile_floor_at(q, 0x1037c590, 0x138);
        else profile_floor_at(q, 0x1037c598, 0x140);
        return;
    }
    if (signed_byte_at(c, 0x1037c5a8, profile, 0x12, 15) >= 0
        || signed_byte_at(c, 0x1037c5b8, profile, 0x14, 15) >= 0
        || signed_byte_at(c, 0x1037c5c8, profile, 0x13, 15) >= 0) {
        const bool qualifies = signed_short_at(c, 0x1037c5d8, profile, 10, 7250) > 0
            && compare_age(c, 0x1037c5e4, age, 31, false);
        if (!qualifies) profile_floor_at(q, 0x1037c60c, 0x14c);
        else if (signed_byte_at(c, 0x1037c5ec, q.player, 0x65, 14) < 0) profile_floor_at(q, 0x1037c5fc, 0x144);
        else profile_floor_at(q, 0x1037c604, 0x148);
        return;
    }

    quote_block(c, 0x1037c614, 3);
    const auto qualification = c.u8(profile + 0xf);
    c.set(0, qualification);
    if (guest::compare(c, qualification, 20) == 0) {
        const bool selected = signed_short_at(c, 0x1037c620, profile, 12, 7250) > 0
            && signed_byte_at(c, 0x1037c62c, q.player, 0x65, 3) < 0
            && compare_age(c, 0x1037c63c, age, 34, false);
        profile_floor_at(q, selected ? 0x1037c644 : 0x1037c64c, selected ? 0x150 : 0x154);
        return;
    }
    if (signed_byte_at(c, 0x1037c654, profile, 0x10, 15) >= 0
        || signed_byte_at(c, 0x1037c664, profile, 0x11, 15) >= 0) {
        const bool selected = signed_short_at(c, 0x1037c674, profile, 10, 7500) > 0
            && signed_byte_at(c, 0x1037c680, q.player, 0x65, 6) < 0
            && compare_age(c, 0x1037c690, age, 33, false);
        profile_floor_at(q, selected ? 0x1037c698 : 0x1037c6a0, selected ? 0xf4 : 0x158);
    } else profile_floor_at(q, 0x1037c6a8, 0x158, 1);
}

template<bool Reference>
void choose_profile_floor(WageQuote<Reference>& q) {
    if (fixed_profile_floor(q)) return;
    if constexpr (Reference) reference_profile_floor(q);
    else country_profile_floor(q);
}

template<bool Reference>
void scale_quote(WageQuote<Reference>& q, std::uint32_t pc, unsigned coefficient, unsigned count = 4) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, count);
    c.set_f(0, q.number(coefficient));
    c.set_f(31, q.fp.fmul(c.f(0), c.f(31)));
    c.set_f(31, q.fp.frsp(c.f(31)));
}

template<bool Reference>
void apply_profile_club_discount(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto site = Reference ? 0x1037c7ccu : 0x10379cc4u;
    quote_block(c, site, 3);
    const auto enabled = c.u8(q.eligibility_mode);
    c.set(0, enabled);
    if (guest::compare(c, enabled, 1) != 0) return;

    quote_block(c, site + 12, 3);
    c.set_f(0, q.single(0x10c));
    q.fp.fcmp(0, c.f(31), c.f(0));
    if (!(c.cr(0) & 4)) return;
    quote_block(c, site + 24, 2);
    if (!guest::nonzero_logical(c, q.target_club)) return;
    quote_block(c, site + 32, 2);
    c.set(3, q.target_club);
    c.call(0x100de260, site + 40);
    quote_block(c, site + 40, 2);
    if (!detail::record_nonzero(c, std::int8_t(c.r(3)))) return;

    const auto rating = signed_short_at(c, site + 48, c.r(24), 10, 6750);
    if (rating > 0) {
        const auto availability = signed_byte_at(c, site + 60, q.player, 0x65, 1);
        if (!availability) scale_quote(q, site + 76, 0x150);
        else {
            quote_block(c, site + 92, 2);
            if (guest::compare(c, std::int32_t(c.r(0)), 6) < 0) scale_quote(q, site + 100, 0x1b0);
            else scale_quote(q, site + 116, 0x158);
        }
    } else {
        quote_block(c, site + 132, 2);
        if (guest::compare(c, std::int32_t(c.r(0)), 4250) > 0) scale_quote(q, site + 140, 0x1b8, 3);
    }
}

template<bool Reference>
void promote_reference_floor(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    quote_block(c, 0x1037c6ac, 3);
    const auto destination = std::uint32_t(q.target_club);
    bool needs_promotion = true;
    const auto has_destination = guest::nonzero_logical(c, destination);
    c.set(5, 1);
    if (has_destination) {
        quote_block(c, 0x1037c6b8, 3);
        const auto league = c.u32(destination + 0x2c);
        c.set(3, league);
        if (guest::nonzero_logical(c, league)) {
            const auto lower_band = signed_short_at(c, 0x1037c6c4, destination, club_rating, 5750) < 0;
            if (lower_band || signed_byte_at(c, 0x1037c6d0, league, 0x11f, 12) < 0) {
                quote_block(c, 0x1037c6e0, 3);
                const auto represented = c.u32(q.player + 0x24);
                c.set(4, represented);
                if (guest::compare_logical(c, represented, destination) == 0) {
                    quote_block(c, 0x1037c6ec, 2);
                    needs_promotion = false;
                    c.set(5, 0);
                } else {
                    quote_block(c, 0x1037c6f4, 2);
                    if (guest::nonzero_logical(c, represented)) {
                        quote_block(c, 0x1037c6fc, 3);
                        const auto represented_league = c.u32(represented + 0x2c);
                        c.set(0, represented_league);
                        if (guest::compare_logical(c, represented_league, league) == 0) {
                            quote_block(c, 0x1037c708, 1);
                            needs_promotion = false;
                            c.set(5, 0);
                        }
                    }
                }
            }
        }
    }
    const auto age = std::int32_t(c.r(20));
    if (!compare_age(c, 0x1037c70c, age, 18, true) || !compare_age(c, 0x1037c714, age, 32, false)) return;
    quote_block(c, 0x1037c71c, 2);
    if (!detail::record_nonzero(c, needs_promotion)) return;

    quote_block(c, 0x1037c724, 11);
    const auto reputation = c.u8(q.player + 0x63);
    c.set(3, reputation);
    const auto availability = c.u8(q.player + 0x65);
    c.set({{0, availability}, {3, std::uint32_t(std::int8_t(reputation))}});
    const auto rating = c.i16(c.r(24) + 12);
    c.set(4, guest::extended(rating));
    const auto reputation_adjustment = std::int8_t(reputation) * 10;
    const auto availability_adjustment = std::int8_t(availability) * 25;
    const auto minimum = 5750 - reputation_adjustment + availability_adjustment;
    c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(reputation_adjustment) <= 5750u);
    c.set({{3, std::uint32_t(5750 - reputation_adjustment)}, {0, std::uint32_t(minimum)}});
    if (guest::compare(c, rating, minimum) <= 0) return;

    quote_block(c, 0x1037c750, 3);
    const auto qualification = c.u8(c.r(24) + 0xf);
    c.set(0, qualification);
    if (guest::compare(c, qualification, 20) == 0) {
        quote_block(c, 0x1037c75c, 3);
        c.set_f(0, q.single(0x6c));
        q.fp.fcmp(0, c.f(31), c.f(0));
        if (c.cr(0) & 8) {
            quote_block(c, 0x1037c768, 2);
            c.set_f(31, c.f(0));
        }
        return;
    }
    quote_block(c, 0x1037c770, 3);
    c.set_f(1, q.single(0x134));
    q.fp.fcmp(0, c.f(1), c.f(31));
    if (c.cr(0) & 2) {
        profile_floor_at(q, 0x1037c77c, 0x1c);
        return;
    }
    quote_block(c, 0x1037c784, 3);
    c.set_f(0, q.single(0x6c));
    q.fp.fcmp(0, c.f(0), c.f(31));
    if (c.cr(0) & 2) {
        quote_block(c, 0x1037c790, 2);
        if (guest::compare(c, rating, 7250) > 0) profile_floor_at(q, 0x1037c798, 0x1c);
        else {
            quote_block(c, 0x1037c7a0, 2);
            c.set_f(31, c.f(1));
        }
        return;
    }
    quote_block(c, 0x1037c7a8, 2);
    if (guest::compare(c, rating, 7250) > 0) {
        quote_block(c, 0x1037c7b0, 2);
        q.fp.fcmp(0, c.f(31), c.f(1));
        if (c.cr(0) & 8) {
            quote_block(c, 0x1037c7b8, 2);
            c.set_f(31, c.f(1));
        }
    } else {
        quote_block(c, 0x1037c7c0, 2);
        q.fp.fcmp(0, c.f(31), c.f(0));
        if (c.cr(0) & 8) {
            quote_block(c, 0x1037c7c8, 1);
            c.set_f(31, c.f(0));
        }
    }
}

void raise_profile_rating(QuoteCpu& c, std::uint32_t site, int minimum, unsigned output, unsigned operand, bool jump) {
    quote_block(c, site, 3);
    const auto rating = std::int16_t(c.r(output));
    c.set(operand, guest::extended(rating));
    if (guest::compare(c, rating, minimum) < 0) {
        quote_block(c, site + 12, jump ? 2 : 1);
        c.set(output, std::uint32_t(minimum));
    }
}

template<bool Reference>
void prepare_profile_rating(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    const auto profile = c.r(24);
    constexpr auto site = Reference ? 0x1037c864u : 0x10379d5cu;
    constexpr unsigned profile_reputation = Reference ? 3 : 23, trained_reputation = Reference ? 23 : 3;
    constexpr unsigned maximum = Reference ? 4 : 5, auxiliary = Reference ? 5 : 4;
    quote_block(c, site, 5);
    const auto main = c.i16(profile + 10);
    c.set(profile_reputation, guest::extended(main));
    const auto secondary = c.i16(profile + 8);
    c.set({{0, guest::extended(secondary)}, {maximum, guest::extended(main)}});
    if (guest::compare(c, secondary, main) > 0) {
        quote_block(c, site + 20, 1);
        c.set(maximum, guest::extended(secondary));
    }
    quote_block(c, site + 24, 4);
    const auto training = c.i16(profile + 12);
    const auto peak = std::int16_t(c.r(maximum));
    c.set({{trained_reputation, guest::extended(training)}, {auxiliary, guest::extended(peak)}});
    if (guest::compare(c, training, peak) > 0) {
        quote_block(c, site + 40, 1);
        c.set(maximum, guest::extended(training));
    }

    if constexpr (Reference) {
        quote_block(c, 0x1037c890, 2);
        if (guest::compare(c, training, 1750) < 0) {
            quote_block(c, 0x1037c898, 3);
            const auto live_peak = std::int16_t(c.r(4));
            c.set(5, guest::extended(live_peak));
            if (guest::compare(c, live_peak, 3250) > 0) {
                quote_block(c, 0x1037c8a4, 16);
                const auto encoded = std::uint32_t(live_peak) ^ 0x80000000;
                c.set({{5, encoded}, {4, high_word}});
                c.put32(c.sp() + 0x3c, encoded);
                c.set_f(2, q.number(0xc0));
                c.put32(c.sp() + 0x38, high_word);
                c.set_f(0, q.number(0xb0));
                c.set_f(1, c.f64(c.sp() + 0x38));
                c.set_f(3, q.number(0xa0));
                c.set_f(1, q.fp.fsub(c.f(1), c.f(2)));
                c.set_f(0, q.fp.fsub(c.f(1), c.f(0)));
                c.set_f(0, q.fp.fmul(c.f(3), c.f(0)));
                c.put64(c.sp() + 0x40, q.fp.fctiwz(0, c.f(0)));
                const auto narrowed = c.u32(c.sp() + 0x44) + 3250;
                c.set(4, guest::extended(std::int16_t(narrowed)));
            }
        }
    }

    constexpr auto adjust = Reference ? 0x1037c8e4u : 0x10379d88u;
    quote_block(c, adjust, 3);
    const auto mode = std::int8_t(c.r(19));
    c.set(auxiliary, std::uint32_t(mode));
    if (guest::compare(c, mode, 1) == 0) raise_profile_rating(c, adjust + 12, 7250, maximum, auxiliary, true);
    else {
        quote_block(c, adjust + 32, 2);
        if (guest::compare(c, mode, 2) == 0)
            raise_profile_rating(c, adjust + 40, 6750, maximum, auxiliary, Reference);
        else if constexpr (Reference) {
            quote_block(c, 0x1037c920, 3);
            const auto displaced = std::uint32_t(mode) - 0x10000;
            c.set(5, displaced);
            // The sign-extended byte cannot match this unsigned word.
            guest::compare_logical(c, displaced, 0x4244);
        }
    }
    if constexpr (Reference) {
        const auto quote = c.f(31);
        bool protected_floor = false;
        for (const auto& [pc, offset] : std::array{
                 std::pair{0x1037c93cu, 0x6cu}, std::pair{0x1037c948u, 0x134u}, std::pair{0x1037c954u, 0x1cu}}) {
            quote_block(c, pc, 3);
            c.set_f(0, q.single(offset));
            q.fp.fcmp(0, c.f(0), quote);
            if (c.cr(0) & 2) {
                protected_floor = true;
                break;
            }
        }
        if (protected_floor) raise_profile_rating(c, 0x1037c960, 7250, 4, 5, false);
    }
}

template<bool Reference>
void calculate_profile_points(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto site = Reference ? 0x1037c970u : 0x10379dc0u;
    constexpr unsigned second_season_reg = Reference ? 8 : 7;
    const auto base_rating = std::int32_t(c.r(0));
    const auto main_rating = std::int32_t(c.r(3));
    const auto trained_rating = std::int32_t(c.r(23));

    quote_block(c, site, 21);
    const auto first_seasons = c.i16(c.r(24) + 4);
    const auto peak = std::int16_t(c.r(Reference ? 4 : 5));
    c.set({{6, guest::extended(first_seasons)}, {5, guest::extended(peak)}});
    const auto second_seasons = c.i16(c.r(24) + 6);
    c.set({{second_season_reg, guest::extended(second_seasons)}, {4, 0x38e40000}});
    const auto first_points = first_seasons * 50;
    const auto pair = base_rating + (Reference ? trained_rating : main_rating);
    c.set({{Reference ? 7u : 6u, std::uint32_t(first_points)}, {Reference ? 6u : 3u, std::uint32_t(pair)},
           {0, std::uint32_t(peak * 4)}});
    c.set_f(1, q.single(0x110));
    c.set_f(0, q.single(0x40));
    c.set({{Reference ? 5u : 4u, 0x38e38e39}, {0, std::uint32_t(pair + peak * 4)}});
    c.set_f(1, q.fp.multiply_add(site + 44, c.f(1), c.f(30), c.f(0)));
    const auto second_points = second_seasons * 50;
    const auto total = first_points + second_points + peak * 4 + base_rating + main_rating + trained_rating;
    const auto high = (INT64_C(0x38e38e39) * total) >> 32;
    const auto floor = high >> 1;
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 1));
    c.set({{Reference ? 4u : 3u, std::uint32_t(second_points)}, {0, std::uint32_t(floor)},
           {3, floor < 0 ? 1u : 0u}, {19, std::uint32_t(total / 9)}});
    c.call(0x10773780, site + 84);
}

template<bool Reference>
void adjust_profile_league(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto site = Reference ? 0x1037c9c4u : 0x10379e14u;
    quote_block(c, site, 4);
    const auto represented = c.u32(q.player + 12);
    c.set(22, represented);
    c.set(19, c.r(19) + c.r(3));
    if (!guest::nonzero_logical(c, represented)) return;
    quote_block(c, site + 16, 3);
    const auto league = c.u32(q.target_club + 0x2c);
    c.set(3, league);
    if (!guest::nonzero_logical(c, league)) return;

    quote_block(c, site + 28, 6);
    const auto player_code = c.u8(represented + 0x118);
    c.set(0, player_code);
    const auto destination = c.i8(league + 0x118);
    c.set({{0, std::uint32_t(std::int8_t(player_code))}, {3, std::uint32_t(destination)}});
    if (guest::compare(c, std::int8_t(player_code), destination) == 0) return;
    quote_block(c, site + 52, 2);
    const auto current = c.r(25);
    if (guest::nonzero_logical(c, current)) {
        quote_block(c, site + 60, 3);
        const auto current_league = c.u32(current + 0x2c);
        c.set(4, current_league);
        if (!guest::nonzero_logical(c, current_league)) return;
        quote_block(c, site + 72, 4);
        const auto code = c.i8(current_league + 0x118);
        c.set(0, std::uint32_t(code));
        if (guest::compare(c, code, destination) == 0) return;
    }

    quote_block(c, site + 88, 2);
    const auto points = c.r(19);
    if (guest::compare_logical(c, points, 1500) < 0) {
        quote_block(c, site + 96, 2);
        c.set(19, 1500);
        return;
    }
    quote_block(c, site + 104, 9);
    c.set(0, high_word);
    c.put32(c.sp() + 0x3c, points);
    c.set_f(1, q.number(0xc8));
    c.put32(c.sp() + 0x38, high_word);
    c.set_f(2, q.number(0x198));
    c.set_f(0, c.f64(c.sp() + 0x38));
    c.set_f(0, q.fp.fsub(c.f(0), c.f(1)));
    c.set_f(1, q.fp.fmul(c.f(2), c.f(0)));
    c.call(0x10773780, site + 140);
    quote_block(c, site + 140, 1);
    c.set(19, c.r(3));
}

template<bool Reference>
void apply_profile_points(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto site = Reference ? 0x1037ca54u : 0x10379ea4u;
    quote_block(c, site, 3);
    auto points = c.r(19);
    const auto scale = guest::compare_logical(c, points, 1) > 0;
    c.set(0, 1);
    if (scale) {
        quote_block(c, site + 12, 1);
        c.set(0, points);
    } else points = 1;

    quote_block(c, site + 16, 14);
    c.set_f(0, q.number(0x1c0));
    const auto square = points * points;
    c.set({{3, square}, {0, high_word}});
    c.set_f(2, q.number(0xc8));
    c.set_f(3, q.fp.fdiv(c.f(31), c.f(0)));
    c.put32(c.sp() + 0x38, high_word);
    c.set_f(0, q.single(0x114));
    c.put32(c.sp() + 0x3c, square);
    c.set_f(1, c.f64(c.sp() + 0x38));
    c.set_f(1, q.fp.fsub(c.f(1), c.f(2)));
    c.set_f(31, q.fp.fmul(c.f(3), c.f(1)));
    c.set_f(31, q.fp.frsp(c.f(31)));
    q.fp.fcmp(0, c.f(31), c.f(0));
    if (c.cr(0) & 8) {
        quote_block(c, site + 72, 1);
        c.set_f(31, c.f(0));
    }
}

template<bool Reference>
int profile_league_level(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto start = Reference ? 0x1037caa0u : 0x10379ef0u;
    constexpr auto fallback = Reference ? 0x1037caccu : 0x10379f0cu;
    quote_block(c, start, 2);
    const auto current = c.r(25);
    if (guest::nonzero_logical(c, current)) {
        quote_block(c, start + 8, 3);
        const auto league = c.u32(current + 0x2c);
        c.set(3, league);
        if (guest::nonzero_logical(c, league)) {
            quote_block(c, start + 20, 2);
            const auto level = c.u8(league + 0x11c);
            c.set(0, level);
            return std::int8_t(level);
        }
    }
    if constexpr (Reference) {
        quote_block(c, 0x1037cabc, 2);
        if (guest::compare(c, std::int32_t(c.r(23)), 4250) > 0) {
            quote_block(c, 0x1037cac4, 2);
            c.set(0, 1);
            return 1;
        }
    }
    quote_block(c, fallback, 2);
    const auto represented = c.r(22);
    if (guest::nonzero_logical(c, represented)) {
        quote_block(c, fallback + 8, 2);
        const auto level = c.u8(represented + 0x11c);
        c.set(0, level);
        return std::int8_t(level);
    }
    quote_block(c, fallback + 16, 1);
    c.set(0, 3);
    return 3;
}

template<bool Reference>
void scale_profile_league(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    const auto level = profile_league_level(q);
    constexpr auto start = Reference ? 0x1037cae0u : 0x10379f20u;
    quote_block(c, start, 3);
    c.set(0, std::uint32_t(level));
    if (guest::compare(c, level, 2) == 0) {
        quote_block(c, start + 36, 5);
        c.set_f(1, q.single(0x54));
        c.set_f(0, q.single(0));
        c.set_f(1, q.fp.fmuls(c.f(1), c.f(31)));
        c.set_f(31, q.fp.fmuls(c.f(1), c.f(0)));
        return;
    }
    quote_block(c, start + 12, 1);
    if (level < 2) {
        quote_block(c, start + 16, 2);
        if (guest::compare(c, level, 1) >= 0) return;
        quote_block(c, start + 24, 1);
    }
    quote_block(c, start + 56, 2);
    c.set_f(0, q.single(0x98));
    c.set_f(31, q.fp.fmuls(c.f(31), c.f(0)));
}

template<bool Reference>
void scale_profile_destination(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto start = Reference ? 0x1037cb20u : 0x10379f60u;
    quote_block(c, start, 2);
    if (!guest::nonzero_logical(c, q.target_club)) return;
    quote_block(c, start + 8, 3);
    const auto rating = c.i16(q.target_club + club_rating);
    c.set({{3, guest::extended(rating)}, {0, guest::extended(rating)}});
    if (!guest::nonzero(c, rating)) return;

    quote_block(c, start + 20, 2);
    const auto training = c.r(23) ^ 0x80000000;
    const auto frame = c.sp();
    if (guest::compare(c, rating, 2500) < 0) {
        quote_block(c, start + 28, 10);
        c.set({{3, training}, {0, high_word}});
        c.put32(frame + 0x3c, training);
        c.set_f(2, q.number(0xc0));
        c.put32(frame + 0x38, high_word);
        c.set_f(0, q.single(0x80));
        c.set_f(1, c.f64(frame + 0x38));
    } else {
        quote_block(c, start + 68, 14);
        const auto band_target = (c.r(21) * 500) ^ 0x80000000;
        c.set({{3, high_word}, {4, training}});
        c.put32(frame + 0x40, high_word);
        c.set_f(2, q.number(0xc0));
        c.set(0, band_target);
        c.put32(frame + 0x44, training);
        c.set_f(0, c.f64(frame + 0x40));
        c.put32(frame + 0x4c, band_target);
        c.set_f(1, q.fp.fsubs(c.f(0), c.f(2)));
        c.put32(frame + 0x48, high_word);
        c.set_f(0, c.f64(frame + 0x48));
        c.set_f(0, q.fp.fsubs(c.f(0), c.f(2)));
    }
    if (rating < 2500) c.set_f(1, q.fp.fsubs(c.f(1), c.f(2)));
    c.set_f(1, q.fp.fdivs(c.f(1), c.f(0)));

    quote_block(c, start + 124, 3);
    c.set_f(0, q.number(0x38));
    q.fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 8) {
        quote_block(c, start + 136, 1);
        c.set_f(1, q.single(0x118));
    }
    quote_block(c, start + 140, 3);
    c.set_f(0, q.number(0x150));
    q.fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 4) {
        quote_block(c, start + 152, 1);
        c.set_f(1, q.single(0x4c));
    }
    quote_block(c, start + 156, 1);
    c.set_f(31, q.fp.fmuls(c.f(31), c.f(1)));
}

enum class ProfileCap { hard, tapered, rechecked };

template<bool Reference>
void cap_profile_quote(WageQuote<Reference>& q, std::uint32_t pc, unsigned units, ProfileCap cap) {
    QuoteCpu& c = q.c;
    const auto frame = c.sp();
    const auto threshold = (c.r(21) * units) ^ 0x80000000;
    const auto bias = cap == ProfileCap::hard ? 1u : 3u;
    quote_block(c, pc, 10);
    c.set({{3, c.r(21) * units}, {0, high_word}});
    c.put32(frame + 0x38, high_word);
    c.set_f(bias, q.number(0xc0));
    c.set(3, threshold);
    c.put32(frame + 0x3c, threshold);
    c.set_f(0, c.f64(frame + 0x38));
    c.set_f(0, q.fp.fsubs(c.f(0), c.f(bias)));
    q.fp.fcmp(0, c.f(31), c.f(0));
    if (!(c.cr(0) & 4)) return;

    pc += 40;
    if (cap != ProfileCap::tapered) {
        quote_block(c, pc, cap == ProfileCap::hard ? 5 : 6);
        c.put32(frame + 0x3c, threshold);
        c.put32(frame + 0x38, high_word);
        c.set_f(0, c.f64(frame + 0x38));
        if (cap == ProfileCap::hard) {
            c.set_f(31, q.fp.fsubs(c.f(0), c.f(1)));
            return;
        }
        c.set_f(0, q.fp.fsubs(c.f(0), c.f(3)));
        q.fp.fcmp(0, c.f(31), c.f(0));
        if (!(c.cr(0) & 4)) return;
        pc += 24;
    }

    quote_block(c, pc, cap == ProfileCap::rechecked ? 12 : 13);
    c.put32(frame + 0x3c, threshold);
    c.set_f(2, q.number(0x88));
    c.put32(frame + 0x38, high_word);
    c.set_f(0, c.f64(frame + 0x38));
    c.put32(frame + 0x44, threshold);
    c.set_f(1, q.fp.fsubs(c.f(0), c.f(3)));
    c.put32(frame + 0x40, high_word);
    c.set_f(0, c.f64(frame + 0x40));
    c.set_f(1, q.fp.fsubs(c.f(31), c.f(1)));
    c.set_f(0, q.fp.fsub(c.f(0), c.f(3)));
    c.set_f(31, q.fp.multiply_add(pc + 40, c.f(2), c.f(1), c.f(0)));
    c.set_f(31, q.fp.frsp(c.f(31)));
}

template<bool Reference>
void limit_profile_by_band(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto start = Reference ? 0x1037cbc0u : 0x1037a000u;
    const auto band = std::int32_t(c.r(21));
    quote_block(c, start, 2);
    if (guest::compare(c, band, 10) < 0) {
        cap_profile_quote(q, start + 8, 2500, ProfileCap::hard);
        return;
    }
    quote_block(c, start + 68, 2);
    if (guest::compare(c, band, 15) < 0) {
        cap_profile_quote(q, start + 76, 2750, ProfileCap::hard);
        return;
    }

    quote_block(c, start + 136, 3);
    const auto league = c.u32(q.target_club + 0x2c);
    c.set(3, league);
    bool preferential_league = false;
    if (guest::nonzero_logical(c, league)) {
        quote_block(c, start + 148, 4);
        const auto region = c.u32(league + 0x68);
        c.set(3, region);
        const auto matches = [&](unsigned reg) {
            const auto candidate = c.u32(c.r(reg));
            c.set(0, candidate);
            return guest::compare(c, std::int32_t(region), std::int32_t(candidate)) == 0;
        };
        preferential_league = matches(13);
        if (!preferential_league) {
            quote_block(c, start + 164, 3);
            preferential_league = matches(28);
        }
        if (!preferential_league) {
            quote_block(c, start + 176, 3);
            preferential_league = matches(14);
        }
    }
    if (!preferential_league) {
        cap_profile_quote(q, start + 188, 3250, Reference ? ProfileCap::tapered : ProfileCap::hard);
        return;
    }

    constexpr auto status = Reference ? 0x1037ccd8u : 0x1037a0f8u;
    quote_block(c, status, 3);
    const auto modifier = c.i8(q.target_club + 0x241);
    c.set(0, std::uint32_t(modifier));
    if (guest::nonzero(c, modifier))
        cap_profile_quote(q, status + 12, Reference ? 5250 : 3250, ProfileCap::tapered);
    else
        cap_profile_quote(q, status + 104, Reference ? 7500 : 5000, ProfileCap::rechecked);
}

template<bool Reference>
void taper_profile_by_attributes(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto start = Reference ? 0x1037cdb0u : 0x1037a1d0u;
    constexpr auto mode = Reference ? 0x1037ce10u : 0x1037a230u;
    constexpr auto cap = Reference ? 0x1037ce4cu : 0x1037a278u;
    const auto profile = c.r(24);
    const auto maximum_attribute = [&](std::uint32_t pc, unsigned field) {
        quote_block(c, pc, 3);
        const auto rating = c.u8(profile + field);
        c.set(0, rating);
        return guest::compare(c, rating, 20) == 0;
    };
    if (maximum_attribute(start, 0x15)) {
        quote_block(c, start + 12, 2);
        c.set_f(30, q.single(Reference ? 0xd8 : 0x18));
    } else if (maximum_attribute(start + 20, 0x14) || maximum_attribute(start + 32, 0x13)) {
        quote_block(c, start + 44, 2);
        c.set_f(30, q.single(Reference ? 0x7c : 0x6c));
    } else if (maximum_attribute(start + 52, 0x12)) {
        quote_block(c, start + 64, 2);
        c.set_f(30, q.single(Reference ? 0x7c : 0xec));
    } else if (maximum_attribute(start + 72, 0x11)) {
        quote_block(c, start + 84, 2);
        c.set_f(30, q.single(Reference ? 0x15c : 0xf4));
    } else {
        quote_block(c, start + 92, 1);
        c.set_f(30, q.single(Reference ? 0x160 : 0x100));
    }

    quote_block(c, mode, 3);
    const auto enabled = c.u8(q.eligibility_mode);
    c.set(0, enabled);
    const auto higher_club_standard = [&] {
        if (guest::compare(c, enabled, 1) != 0) return false;
        if constexpr (!Reference) {
            quote_block(c, mode + 12, 3);
            c.set_f(0, q.single(0x10c));
            q.fp.fcmp(0, c.f(31), c.f(0));
            if (!(c.cr(0) & 4)) return false;
        }
        constexpr auto club_check = Reference ? mode + 12 : mode + 24;
        quote_block(c, club_check, 2);
        if (!guest::nonzero_logical(c, q.target_club)) return false;
        quote_block(c, club_check + 8, 2);
        c.set(3, q.target_club);
        c.call(0x100de260, club_check + 16);
        quote_block(c, club_check + 16, 2);
        if (!detail::record_nonzero(c, std::int8_t(c.r(3)))) return false;
        quote_block(c, club_check + 24, 3);
        const auto reputation = c.i16(profile + 10);
        c.set(0, guest::extended(reputation));
        return guest::compare(c, reputation, 7250) > 0;
    };
    if (higher_club_standard()) {
        quote_block(c, cap - 12, 3);
        c.set_f(0, q.number(0x158));
        c.set_f(30, q.fp.fmul(c.f(0), c.f(30)));
        c.set_f(30, q.fp.frsp(c.f(30)));
    }

    quote_block(c, cap, 2);
    q.fp.fcmp(0, c.f(31), c.f(30));
    if (c.cr(0) & 4) {
        quote_block(c, cap + 8, 4);
        c.set_f(0, q.fp.fsubs(c.f(31), c.f(30)));
        c.set_f(1, q.number(0xa0));
        c.set_f(31, q.fp.multiply_add(cap + 16, c.f(1), c.f(0), c.f(30)));
        c.set_f(31, q.fp.frsp(c.f(31)));
    }
}

template<bool Reference>
bool needs_profile_date_adjustment(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto start = Reference ? 0x1037ce64u : 0x1037a290u;
    quote_block(c, start, 3);
    const auto player_club = c.u32(q.player + 0x24);
    c.set(0, player_club);
    if (guest::nonzero_logical(c, player_club)) return false;
    quote_block(c, start + 12, 2);
    if (guest::compare(c, std::int32_t(c.r(20)), 33) < 0) return true;
    quote_block(c, start + 20, 3);
    const auto experienced = c.u8(q.player + 0x5e);
    c.set(0, experienced);
    if (!guest::nonzero_logical(c, experienced)) return true;
    quote_block(c, start + 32, 7);
    const auto player_metric = c.i8(q.player + 0x64);
    c.set(3, std::uint32_t(player_metric));
    const auto profile_metric = c.i8(c.r(24) + 0x44);
    const auto combined = player_metric + profile_metric;
    c.set(0, std::uint32_t(combined));
    if (guest::compare(c, combined, 30) > 0) return true;
    quote_block(c, start + 60, 11);
    c.set(3, 0x88890000);
    const auto id = c.u32(q.player);
    c.set(4, id);
    const auto signed_id = std::int32_t(id);
    const auto high = (INT64_C(-2004318071) * signed_id) >> 32;
    const auto sum = std::int32_t(std::uint32_t(high) + id);
    c.s.wr<1>(guest::reg::xer_ca, sum < 0 && (sum & 7));
    c.set(3, sum < 0 ? 1u : 0u);
    if (!detail::record_nonzero(c, signed_id % 15)) return true;
    quote_block(c, start + 104, 3);
    const auto club_reputation = c.i16(q.target_club + club_rating);
    c.set(0, guest::extended(club_reputation));
    return guest::compare(c, club_reputation, 4500) > 0;
}

template<bool Reference>
void adjust_profile_for_dates(WageQuote<Reference>& q) {
    if (!needs_profile_date_adjustment(q)) return;
    QuoteCpu& c = q.c;
    constexpr auto start = Reference ? 0x1037ced8u : 0x1037a304u;
    constexpr auto eligibility_return = Reference ? 0x1037cf20u : 0x1037a33cu;
    constexpr auto apply = Reference ? 0x1037cf50u : 0x1037a34cu;
    quote_block(c, start, 4);
    c.set(4, c.u32(q.player + 0x4c));
    c.set(3, c.toc(-0x7b80));
    c.set(5, c.u32(q.player + 0x50));
    c.call(0x10718130, start + 16);

    quote_block(c, start + 16, 10);
    const auto duration = c.r(3) ^ 0x80000000;
    c.set({{3, duration}, {0, high_word}});
    const auto frame = c.sp();
    c.put32(frame + 0x3c, duration);
    if constexpr (Reference) {
        c.set_f(2, q.number(0xc0));
        c.put32(frame + 0x38, high_word);
        c.set_f(0, q.single(0xe0));
        c.set_f(1, c.f64(frame + 0x38));
        c.set_f(30, q.fp.fsubs(c.f(1), c.f(2)));
        q.fp.fcmp(0, c.f(30), c.f(0));
        if (c.cr(0) & 4) {
            quote_block(c, 0x1037cf10, 1);
            c.set_f(30, c.f(0));
        }
        quote_block(c, 0x1037cf14, 3);
        c.set(4, c.toc(-0x7b80));
        c.set(3, q.target_club);
    } else {
        c.set(3, q.target_club);
        c.set_f(1, q.number(0xc0));
        c.put32(frame + 0x38, high_word);
        c.set(4, c.toc(-0x7b80));
        c.set_f(0, c.f64(frame + 0x38));
        c.set_f(30, q.fp.fsubs(c.f(0), c.f(1)));
    }
    c.call(0x100d9440, eligibility_return);
    quote_block(c, eligibility_return, 2);
    if (detail::record_nonzero(c, std::int8_t(c.r(3)))) {
        quote_block(c, eligibility_return + 8, 2);
        c.set_f(0, q.single(0x11c));
        c.set_f(30, q.fp.fadds(c.f(30), c.f(0)));
    }

    if constexpr (Reference) {
        quote_block(c, 0x1037cf30, 3);
        c.set_f(0, q.single(0x164));
        q.fp.fcmp(0, c.f(30), c.f(0));
        if (c.cr(0) & 4) {
            quote_block(c, 0x1037cf3c, 5);
            c.set_f(1, q.fp.fsubs(c.f(30), c.f(0)));
            c.set_f(2, q.number(0xa0));
            c.set_f(0, q.number(0x238));
            c.set_f(30, q.fp.multiply_add(0x1037cf48, c.f(2), c.f(1), c.f(0)));
            c.set_f(30, q.fp.frsp(c.f(30)));
        }
    }
    quote_block(c, apply, 6);
    c.set_f(1, q.single(0x128));
    c.set_f(0, q.single(0x124));
    c.set_f(2, q.single(0x120));
    c.set_f(0, q.fp.multiply_add(apply + 12, c.f(1), c.f(30), c.f(0)));
    q.fp.fcmp(0, c.f(2), c.f(0));
    if (c.cr(0) & 4) quote_block(c, apply + 24, 1);
    else {
        quote_block(c, apply + 28, 1);
        c.set_f(2, c.f(0));
    }
    quote_block(c, apply + 32, 2);
    c.set_f(31, q.fp.fmuls(c.f(31), c.f(2)));
}

template<bool Reference>
void quote_without_profile(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto start = Reference ? 0x1037cf78u : 0x1037a374u;
    quote_block(c, start, 6);
    c.set(4, c.toc(-0x4da4));
    c.set(3, c.toc(-0x7bb8));
    c.set(5, c.u32(q.player + 0x28));
    c.set(4, c.r(4) + 0x233);
    c.set(6, c.u32(q.player + 0x2c));
    c.call(0x1077b720, start + 24);
    quote_block(c, start + 24, 1);
    c.call(0x1077b770, start + 28);

    quote_block(c, start + 28, 21);
    // The reseed returns exactly 15 bits. The diagnostic scale uses their
    // top two bits, and the unused companion quotient always truncates to zero.
    const auto draw = c.r(3);
    const auto bucket = draw / 8192;
    c.s.wr<1>(guest::reg::xer_ca, 0);
    c.set({{0, high_word}, {3, 0}, {4, bucket}, {5, 0}, {6, draw * 4}});
    c.put32(c.sp() + 0x38, high_word);
    c.set_f(2, q.number(0xc0));
    c.set_f(0, q.single(0x12c));
    c.set(0, bucket ^ 0x80000000);
    c.put32(c.sp() + 0x3c, c.r(0));
    c.set_f(1, c.f64(c.sp() + 0x38));
    c.set_f(1, q.fp.fsubs(c.f(1), c.f(2)));
    c.set_f(31, q.fp.fmuls(c.f(0), c.f(1)));
}

template<bool Reference>
constexpr std::uint32_t destination_site(std::uint32_t country) {
    return country + (Reference ? 0x2c04 : 0);
}

template<bool Reference>
void destination_block(WageQuote<Reference>& q, std::uint32_t country, unsigned count) {
    quote_block(q.c, destination_site<Reference>(country), count);
}

template<bool Reference>
void calculate_destination_curve(WageQuote<Reference>& q, bool special_club) {
    QuoteCpu& c = q.c;
    const auto frame = c.sp();
    const std::uint32_t club = q.target_club;
    if (special_club) {
        destination_block(q, 0x1037a460, 10);
        const auto rating = c.i16(club + club_rating);
        c.set({{3, guest::extended(rating)}, {0, high_word}});
        c.put32(frame + 0x38, high_word);
        const auto word = guest::extended(rating) ^ 0x80000000;
        c.set(0, word);
        c.set_f(1, q.number(0xc0));
        c.put32(frame + 0x3c, word);
        c.set_f(2, q.number(0x1d0));
    } else {
        destination_block(q, 0x1037a404, 3);
        const auto rating = c.i16(club + club_rating);
        c.set(0, guest::extended(rating));
        const auto strong = guest::compare(c, rating, 2250) > 0;
        destination_block(q, strong ? 0x1037a410 : 0x1037a438, 10);
        const auto word = guest::extended(rating) ^ 0x80000000;
        c.set({{3, word}, {0, high_word}});
        c.put32(frame + 0x3c, word);
        c.set_f(1, q.number(0xc0));
        c.put32(frame + 0x38, high_word);
        c.set_f(2, q.number(strong ? 0x1c8 : 0xe0));
    }
    c.set_f(0, c.f64(frame + 0x38));
    c.set_f(0, q.fp.fsub(c.f(0), c.f(1)));
    c.set_f(1, q.fp.fmul(c.f(2), c.f(0)));
    destination_block(q, 0x1037a488, 2);
    c.set_f(2, q.number(0x1d8));
    c.call(0x10780fc0, destination_site<Reference>(0x1037a490));
    destination_block(q, 0x1037a490, 5);
    c.set(2, c.u32(frame + 0x14));
    c.set_f(30, c.f(1));
    c.set_f(0, q.number(0x108));
    q.fp.fcmp(0, c.f(30), c.f(0));
    if (c.cr(0) & 4) {
        destination_block(q, 0x1037a4a4, 1);
        c.set_f(30, c.f(0));
    }
}

template<bool Reference>
void raise_destination_quote(WageQuote<Reference>& q, std::uint32_t pc, unsigned factor, bool jump = true) {
    QuoteCpu& c = q.c;
    const auto frame = c.sp();
    destination_block(q, pc, factor ? 14 : 12);
    const auto rating = c.i16(q.target_club + club_rating);
    c.set({{3, guest::extended(rating)}, {0, high_word}});
    c.put32(frame + 0x38, high_word);
    c.set(0, std::uint32_t(rating * 3));
    c.set_f(1, q.number(0xc0));
    if (factor) c.set_f(2, q.number(factor));
    const auto scaled = c.r(0) ^ 0x80000000;
    c.set(0, scaled);
    c.put32(frame + 0x3c, scaled);
    c.set_f(0, c.f64(frame + 0x38));
    c.set_f(0, q.fp.fsub(c.f(0), c.f(1)));
    if (factor) c.set_f(0, q.fp.fmul(c.f(2), c.f(0)));
    c.set_f(0, q.fp.fmul(c.f(30), c.f(0)));
    q.fp.fcmp(0, c.f(31), c.f(0));
    if (!(c.cr(0) & 8)) return;

    destination_block(q, pc + (factor ? 56 : 48), jump ? 5 : 4);
    c.set_f(1, q.fp.fadd(c.f(31), c.f(0)));
    c.set_f(0, q.number(0x28));
    c.set_f(31, q.fp.fmul(c.f(1), c.f(0)));
    c.set_f(31, q.fp.frsp(c.f(31)));
}

template<bool Reference>
bool use_senior_destination_floor(WageQuote<Reference>& q, std::uint32_t pc) {
    QuoteCpu& c = q.c;
    destination_block(q, pc, 2);
    if (guest::compare(c, std::int32_t(c.r(20)), 24) < 0) return false;
    destination_block(q, pc + 8, 2);
    c.set(3, q.target_club);
    c.call(0x100de260, destination_site<Reference>(pc + 16));
    destination_block(q, pc + 16, 2);
    return detail::record_nonzero(c, std::int8_t(c.r(3)));
}

struct DestinationFloors {
    std::uint32_t first;
    unsigned young_factor, standard_factor, senior_factor;
};

template<bool Reference>
void apply_destination_floors(WageQuote<Reference>& q, DestinationFloors floors, bool follow_young) {
    QuoteCpu& c = q.c;
    destination_block(q, floors.first, 2);
    if (guest::compare(c, std::int32_t(c.r(20)), 20) < 0) {
        raise_destination_quote(q, floors.first + 8, floors.young_factor, !follow_young);
        if (!follow_young) return;
    }
    const auto senior_check = floors.first + (follow_young ? 0x50 : 0x54);
    if (use_senior_destination_floor(q, senior_check))
        raise_destination_quote(q, senior_check + 0x64, floors.senior_factor);
    else
        raise_destination_quote(q, senior_check + 0x18, floors.standard_factor);
}

template<bool Reference>
void apply_destination_role(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr std::array floors{
        DestinationFloors{0x1037a4e0, 0x1e0, 0x118, 0x130},
        DestinationFloors{0x1037a5e4, 0x1e8, 0x1e0, 0},
        DestinationFloors{0x1037a6e0, 0x48, 0x1e8, 0x1e0},
        DestinationFloors{0x1037a7e0, 0x88, 0x78, 0x1e8},
    };
    destination_block(q, 0x1037a4a8, 3);
    const auto role = std::int8_t(c.r(18));
    c.set(19, std::uint32_t(role));
    if (guest::compare(c, role, 4) == 0) {
        apply_destination_floors(q, floors[3], false);
        return;
    }
    destination_block(q, 0x1037a4b4, 1);
    if (role >= 4) {
        destination_block(q, 0x1037a4d0, 2);
        if (guest::compare(c, role, 6) == 0) {
            raise_destination_quote(q, 0x1037a930, 0x1f0, false);
            return;
        }
        destination_block(q, 0x1037a4d8, 1);
        if (role < 6) {
            destination_block(q, 0x1037a4dc, 1);
            raise_destination_quote(q, 0x1037a8e4, 0xd8);
        }
        return;
    }
    destination_block(q, 0x1037a4b8, 2);
    if (guest::compare(c, role, 2) == 0) {
        apply_destination_floors(q, floors[1], false);
        return;
    }
    destination_block(q, 0x1037a4c0, 1);
    if (role > 2) {
        apply_destination_floors(q, floors[2], true);
        return;
    }
    destination_block(q, 0x1037a4c4, 2);
    if (guest::compare(c, role, 1) >= 0) apply_destination_floors(q, floors[0], false);
    else destination_block(q, 0x1037a4cc, 1);
}

template<bool Reference>
void reduce_quote_for_primary_contract(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    destination_block(q, 0x1037a978, 3);
    c.set(3, c.u32(q.manager_slot));
    c.set(4, q.player);
    c.call(0x10082f20, destination_site<Reference>(0x1037a984));
    destination_block(q, 0x1037a984, 2);
    const auto contract = c.r(3);
    if (!guest::nonzero_logical(c, contract)) return;
    destination_block(q, 0x1037a98c, 3);
    if (!detail::record_nonzero(c, c.u32(contract + 0x50) & 0x10)) return;

    const auto apply_factor = [&](std::uint32_t pc, unsigned factor, bool jump = true) {
        destination_block(q, pc, jump ? 4 : 3);
        c.set_f(0, q.number(factor));
        c.set_f(31, q.fp.fmul(c.f(0), c.f(31)));
        c.set_f(31, q.fp.frsp(c.f(31)));
    };
    const auto role = std::int32_t(c.r(19));
    destination_block(q, 0x1037a998, 2);
    if (guest::compare(c, role, 4) == 0) {
        apply_factor(0x1037a9e8, 0x200);
        return;
    }
    destination_block(q, 0x1037a9a0, 1);
    if (role > 4) {
        destination_block(q, 0x1037a9b8, 2);
        if (guest::compare(c, role, 6) == 0) {
            apply_factor(0x1037aa08, 0x208, false);
            return;
        }
        destination_block(q, 0x1037a9c0, 1);
        if (role < 6) {
            destination_block(q, 0x1037a9c4, 1);
            apply_factor(0x1037a9f8, 0x110);
        }
        return;
    }
    destination_block(q, 0x1037a9a4, 2);
    if (guest::compare(c, role, 3) >= 0) {
        apply_factor(0x1037a9d8, 0x38);
        return;
    }
    destination_block(q, 0x1037a9ac, 2);
    if (guest::compare(c, role, 1) >= 0) apply_factor(0x1037a9c8, 0x1f8);
    else destination_block(q, 0x1037a9b4, 1);
}

template<bool Reference>
void apply_destination_quote(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    destination_block(q, 0x1037a3e8, 2);
    if (!guest::nonzero_logical(c, q.target_club)) return;
    destination_block(q, 0x1037a3f0, 2);
    if (!guest::nonzero_logical(c, c.r(24))) return;
    destination_block(q, 0x1037a3f8, Reference ? 1 : 3);
    const auto type = c.u8(q.target_club + club_type);
    c.set(0, type);
    if constexpr (Reference) quote_block(c, 0x1037d000, 2);
    const auto special = guest::compare(c, type, 1) == 0;
    calculate_destination_curve(q, special);
    apply_destination_role(q);
    reduce_quote_for_primary_contract(q);
}

template<bool Reference>
bool has_local_wage_market(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    destination_block(q, 0x1037aa14, 2);
    if (!guest::nonzero_logical(c, q.target_club)) return false;
    destination_block(q, 0x1037aa1c, 3);
    const auto league = c.u32(q.target_club + 0x2c);
    c.set(3, league);
    if (!guest::nonzero_logical(c, league)) return false;
    destination_block(q, 0x1037aa28, 3);
    const auto registered_club = c.u32(q.player + 0x24);
    c.set(4, registered_club);
    const auto in_league = [&](std::uint32_t pc, std::uint32_t object, unsigned field) {
        destination_block(q, pc, 3);
        const auto other = c.u32(object + field);
        c.set(0, other);
        return guest::compare_logical(c, other, league) == 0;
    };
    if (guest::nonzero_logical(c, registered_club)
        && in_league(0x1037aa34, registered_club, 0x2c) && in_league(0x1037aa40, q.player, 0xc))
        return true;

    destination_block(q, 0x1037aa4c, 2);
    if (guest::nonzero_logical(c, registered_club)
        && in_league(0x1037aa54, registered_club, 0x2c) && !in_league(0x1037aa60, q.player, 0xc)) {
        if (signed_byte_at(c, destination_site<Reference>(0x1037aa6c), q.player, 0x65, 3) > 0) return true;
        if (signed_byte_at(c, destination_site<Reference>(0x1037aa7c), q.player, 0x63, 15) < 0) return true;
    }
    destination_block(q, 0x1037aa8c, 2);
    return !guest::nonzero_logical(c, registered_club) && in_league(0x1037aa94, q.player, 0xc);
}

template<bool Reference>
bool prepare_local_wage_market(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    destination_block(q, 0x1037aaa0, 13);
    c.set(4, 0x51ec0000);
    const auto reputation = c.i16(q.target_club + club_rating);
    c.set({{0, guest::extended(reputation)}, {5, 0x51eb851f}});
    c.set(4, c.toc(-0x7a24));
    const auto high = (INT64_C(0x51eb851f) * reputation) >> 32;
    c.set(5, std::uint32_t(high));
    const auto region = c.u32(c.r(3) + 0x68);
    c.set(0, region);
    const auto default_region = c.u32(c.r(4));
    c.set(4, default_region);
    const auto is_default = guest::compare(c, std::int32_t(region), std::int32_t(default_region)) == 0;
    const auto units = std::uint32_t(reputation / 50);
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 15));
    c.set({{4, units}, {5, high < 0 ? 1u : 0u}, {Reference ? 4u : 5u, units}});
    return is_default;
}

template<bool Reference>
void taper_market_quote(WageQuote<Reference>& q, std::uint32_t pc, unsigned cap, unsigned fraction,
                        bool jump = true, bool round_difference = false) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, 3);
    c.set_f(2, q.number(cap));
    q.fp.fcmp(0, c.f(31), c.f(2));
    if (!(c.cr(0) & 4)) return;
    quote_block(c, pc + 12, (jump ? 5 : 4) + unsigned(round_difference));
    c.set_f(0, q.fp.fsub(c.f(31), c.f(2)));
    c.set_f(1, q.number(fraction));
    if (round_difference) c.set_f(0, q.fp.frsp(c.f(0)));
    c.set_f(31, q.fp.multiply_add(pc + (round_difference ? 24 : 20), c.f(1), c.f(0), c.f(2)));
    c.set_f(31, q.fp.frsp(c.f(31)));
}

template<bool Reference>
void apply_default_wage_market(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    const auto frame = c.sp();
    destination_block(q, 0x1037aad4, 11);
    const auto band = c.r(21);
    c.set({{3, band * band}, {0, high_word}});
    c.put32(frame + 0x38, high_word);
    c.set_f(1, q.number(0xc0));
    const auto minimum = (band * band * 10) ^ 0x80000000;
    c.set(3, minimum);
    c.put32(frame + 0x3c, minimum);
    c.set_f(0, c.f64(frame + 0x38));
    c.set_f(0, q.fp.fsubs(c.f(0), c.f(1)));
    q.fp.fcmp(0, c.f(31), c.f(0));
    if (c.cr(0) & 8) {
        destination_block(q, 0x1037ab00, 4);
        c.put32(frame + 0x44, minimum);
        c.put32(frame + 0x40, high_word);
        c.set_f(0, c.f64(frame + 0x40));
        c.set_f(31, q.fp.fsubs(c.f(0), c.f(1)));
    }
    taper_market_quote(q, destination_site<Reference>(0x1037ab10), 0x210, 0x48);
}

template<bool Reference>
bool market_region_matches(WageQuote<Reference>& q, std::uint32_t pc, int toc_offset,
                           unsigned destination = Reference ? 5 : 4) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, 4);
    const auto slot = c.toc(toc_offset);
    c.set(destination, slot);
    const auto region = c.u32(slot);
    c.set(destination, region);
    return guest::compare(c, std::int32_t(c.r(0)), std::int32_t(region)) == 0;
}

template<bool Reference>
bool excluded_market_region(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    for (auto [site, reg] : std::array{std::pair{0x1037ab30u, 13u},
                                      std::pair{0x1037ab3cu, 14u},
                                      std::pair{0x1037ab48u, 28u}}) {
        destination_block(q, site, 3);
        const auto region = c.u32(c.r(reg));
        c.set(Reference ? 5 : 4, region);
        if (guest::compare(c, std::int32_t(c.r(0)), std::int32_t(region)) == 0) return true;
    }
    return false;
}

template<bool Reference>
void apply_special_wage_market(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    destination_block(q, 0x1037ab64, 6);
    c.set_f(0, q.number(0x30));
    const auto competition = c.u32(q.target_club + 0x30);
    c.set(5, competition);
    c.set_f(31, q.fp.fmul(c.f(31), c.f(0)));
    const auto has_competition = guest::nonzero_logical(c, competition);
    c.set_f(31, q.fp.frsp(c.f(31)));
    if (has_competition) {
        destination_block(q, 0x1037ab7c, 5);
        const auto featured_slot = c.toc(-0x6778);
        c.set(4, featured_slot);
        const auto competition_id = c.u32(competition);
        c.set(5, competition_id);
        const auto featured = c.u32(featured_slot);
        c.set(0, featured);
        if (guest::compare(c, std::int32_t(competition_id), std::int32_t(featured)) == 0) {
            taper_market_quote(q, destination_site<Reference>(0x1037ab90), 0x68, 0x78);
            return;
        }
    }

    destination_block(q, 0x1037abb0, 3);
    const auto league = c.u32(q.player + 0xc);
    c.set(0, league);
    if (guest::compare_logical(c, league, c.r(3)) != 0) {
        taper_market_quote(q, destination_site<Reference>(0x1037ac4c), 0xb0, 0x48);
        return;
    }
    destination_block(q, 0x1037abbc, 4);
    const auto age = std::int8_t(c.i16(q.player + player_age));
    c.set(0, std::uint32_t(age));
    if (guest::compare(c, age, 31) > 0) {
        taper_market_quote(q, destination_site<Reference>(0x1037abcc), 0x98, 0xa0);
        return;
    }
    destination_block(q, 0x1037abec, 2);
    if (guest::compare(c, age, 24) < 0) {
        destination_block(q, 0x1037abf4, 3);
        const auto profile = c.u32(q.player + player_profile);
        c.set(3, profile);
        if (guest::nonzero_logical(c, profile)
            && signed_short_at(c, destination_site<Reference>(0x1037ac00), profile, 12, 3750) < 0) {
            taper_market_quote(q, destination_site<Reference>(0x1037ac0c), 0x98, 0xa0);
            return;
        }
    }
    taper_market_quote(q, destination_site<Reference>(0x1037ac2c), 0xa8, 0xa0);
}

template<bool Reference>
void price_league_level(WageQuote<Reference>& q, std::uint32_t pc, unsigned factor, bool reference_exception = false) {
    QuoteCpu& c = q.c;
    constexpr unsigned wage_units_reg = Reference ? 4 : 5;
    constexpr unsigned level_reg = Reference ? 5 : 4;
    const auto units = std::int32_t(c.r(wage_units_reg));
    const auto frame = c.sp();
    quote_block(c, pc, 6);
    const auto raw_level = c.u8(c.r(3) + 0x11f);
    c.set(0, raw_level);
    c.set_f(5, q.number(factor));
    const auto level = std::int8_t(raw_level);
    const auto threshold = level * 10;
    c.set({{level_reg, std::uint32_t(level)}, {reference_exception ? 6u : 0u, std::uint32_t(threshold)}});
    if (guest::compare(c, units, threshold) <= 0) return;

    quote_block(c, pc + 24, 10);
    const auto encoded = std::uint32_t(units) ^ 0x80000000;
    c.set({{0, encoded}, {3, high_word}});
    c.put32(frame + 0x3c, encoded);
    const auto valid_level = guest::compare(c, level, 1) >= 0;
    c.set_f(1, q.number(0xc0));
    c.set(0, 1);
    c.put32(frame + 0x38, high_word);
    c.set_f(0, c.f64(frame + 0x38));
    c.set_f(4, q.fp.fsub(c.f(0), c.f(1)));
    if (valid_level) {
        quote_block(c, pc + 64, 1);
        c.set(0, std::uint32_t(level));
    }
    if (pc == 0x1037afb0) quote_block(c, pc + 68, 3);
    else quote_block(c, pc + 68, 14);
    const auto encoded_level = c.r(0) ^ 0x80000000;
    c.set({{3, encoded_level}, {0, high_word}});
    c.put32(frame + 0x44, encoded_level);
    if (pc == 0x1037afb0) quote_block(c, pc + 80, 11);
    c.set_f(3, q.number(0xc0));
    c.put32(frame + 0x40, high_word);
    c.set_f(1, q.number(0x170));
    c.set_f(2, c.f64(frame + 0x40));
    c.set_f(0, q.number(0x108));
    c.set_f(2, q.fp.fsub(c.f(2), c.f(3)));
    c.set_f(2, q.fp.fdiv(c.f(4), c.f(2)));
    c.set_f(1, q.fp.fdiv(c.f(2), c.f(1)));
    c.set_f(5, q.fp.fmul(c.f(5), c.f(1)));
    q.fp.fcmp(0, c.f(5), c.f(0));
    if (c.cr(0) & 4) {
        quote_block(c, pc + 124, 1);
        c.set_f(5, c.f(0));
    }
}

template<bool Reference>
void taper_by_club_type(WageQuote<Reference>& q, std::uint32_t pc, unsigned standard_cap, unsigned special_cap,
                        unsigned fraction, bool scaled, bool round_special = false) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, scaled ? 5 : 3);
    if (scaled) c.set_f(31, q.fp.fmul(c.f(31), c.f(5)));
    const auto type = c.u8(q.target_club + club_type);
    c.set(0, type);
    const auto special = guest::compare(c, type, 1) == 0;
    if (scaled) c.set_f(31, q.fp.frsp(c.f(31)));
    pc += scaled ? 20 : 12;
    if (special) pc += 32;
    taper_market_quote(q, pc, special ? special_cap : standard_cap, fraction, true, special && round_special);
}

template<bool Reference>
void scale_and_taper_market(WageQuote<Reference>& q, std::uint32_t pc, unsigned cap, unsigned fraction,
                            bool default_scale = false) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, default_scale ? 6 : 5);
    if (default_scale) c.set_f(0, q.number(0x1e8));
    if (default_scale) c.set_f(2, q.number(cap));
    c.set_f(31, q.fp.fmul(c.f(31), c.f(default_scale ? 0 : 5)));
    if (!default_scale) c.set_f(2, q.number(cap));
    c.set_f(31, q.fp.frsp(c.f(31)));
    q.fp.fcmp(0, c.f(31), c.f(2));
    if (!(c.cr(0) & 4)) return;
    quote_block(c, pc + (default_scale ? 24 : 20), default_scale ? 4 : 5);
    c.set_f(0, q.fp.fsub(c.f(31), c.f(2)));
    c.set_f(1, q.number(fraction));
    c.set_f(31, q.fp.multiply_add(pc + (default_scale ? 32 : 28), c.f(1), c.f(0), c.f(2)));
    c.set_f(31, q.fp.frsp(c.f(31)));
}

void apply_reference_market_ceiling(WageQuote<true>& q) {
    QuoteCpu& c = q.c;
    quote_block(c, 0x1037d900, 4);
    c.set_f(31, q.fp.fmul(c.f(31), c.f(5)));
    const auto strong = guest::compare(c, std::int32_t(c.r(4)), std::int32_t(c.r(6))) > 0;
    c.set_f(31, q.fp.frsp(c.f(31)));
    if (strong) {
        taper_market_quote(q, 0x1037d910, 0x58, 0x48);
        return;
    }
    quote_block(c, 0x1037d930, 2);
    const auto profile = c.r(24);
    if (!guest::nonzero_logical(c, profile)) {
        taper_market_quote(q, 0x1037d9c4, 0x240, 0x48);
        return;
    }
    quote_block(c, 0x1037d938, 3);
    const auto reputation = c.i16(profile + 12);
    c.set(3, guest::extended(reputation));
    if (guest::compare(c, reputation, 1250) < 0) {
        taper_market_quote(q, 0x1037d944, 0x98, 0x48);
        return;
    }
    quote_block(c, 0x1037d964, 4);
    const auto age = std::int8_t(c.i16(q.player + player_age));
    c.set(0, std::uint32_t(age));
    bool typical_age = false;
    if (guest::compare(c, age, 20) >= 0) {
        quote_block(c, 0x1037d974, 2);
        typical_age = guest::compare(c, age, 32) <= 0;
    }
    if (!typical_age) {
        quote_block(c, 0x1037d97c, 2);
        if (guest::compare(c, reputation, 2750) < 0) {
            taper_market_quote(q, 0x1037d984, 0xb0, 0x48);
            return;
        }
    }
    taper_market_quote(q, 0x1037d9a4, 0x240, 0x48);
}

template<bool Reference>
bool middle_market_region(WageQuote<Reference>& q) {
    const auto matches = [&](std::uint32_t country, std::uint32_t reference, int slot, unsigned country_register) {
        return market_region_matches(q, Reference ? reference : country, slot, Reference ? 5 : country_register);
    };
    if (matches(0x1037aee4, 0x1037dba4, -0x757c, 4)) return true;
    if constexpr (Reference) {
        QuoteCpu& c = q.c;
        quote_block(c, 0x1037dbb4, 4);
        const auto slot = c.toc(-0x7594);
        c.set(5, slot);
        const auto region = c.u32(slot);
        c.set(6, region);
        if (guest::compare(c, std::int32_t(c.r(0)), std::int32_t(region)) == 0) return true;
    } else if (matches(0x1037aef4, 0, -0x7594, 4)) return true;
    return matches(0x1037af04, 0x1037dbc4, -0x7574, 6)
        || matches(0x1037af14, 0x1037dbd4, -0x7578, 6)
        || matches(0x1037af24, 0x1037dbe4, -0x7650, 6);
}

template<bool Reference>
void adjust_for_wage_market(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    if (!has_local_wage_market(q)) return;
    if (prepare_local_wage_market(q)) {
        apply_default_wage_market(q);
        return;
    }
    if (excluded_market_region(q)) return;
    if (market_region_matches(q, destination_site<Reference>(0x1037ab54), -0x7a20)) {
        apply_special_wage_market(q);
        return;
    }
    if (market_region_matches(q, destination_site<Reference>(0x1037ac6c), -0x7a1c)) {
        constexpr auto site = destination_site<Reference>(0x1037ac7c);
        price_league_level(q, site, Reference ? 0x48 : 0x218, Reference);
        if constexpr (Reference) apply_reference_market_ceiling(q);
        else scale_and_taper_market(q, site + 128, 0x58, 0x48);
        return;
    }
    constexpr auto market_site = [](std::uint32_t country) { return country + (Reference ? 0x2cc0 : 0); };
    const auto matches = [&](std::uint32_t country, int offset, unsigned country_register = 4) {
        return market_region_matches(q, market_site(country), offset, Reference ? 5 : country_register);
    };
    if (matches(0x1037ad24, -0x7af4) || matches(0x1037ad34, -0x7a00) || matches(0x1037ad44, -0x7a28)) {
        constexpr auto site = market_site(0x1037ad54);
        price_league_level(q, site, 0x30);
        scale_and_taper_market(q, site + 128, 0x60, 0x48);
        return;
    }
    if (matches(0x1037adfc, -0x7514)) {
        constexpr auto site = market_site(0x1037ae0c);
        price_league_level(q, site, 0x48);
        taper_by_club_type(q, site + 128, 0x220, 0x40, 0xa0, true, true);
        return;
    }
    if (middle_market_region(q)) {
        taper_by_club_type(q, market_site(0x1037af34), 0x228, 0x80, 0x78, false);
        return;
    }
    if (matches(0x1037af80, -0x750c, 6) || matches(0x1037af90, -0x7504, 6)
        || (!Reference && matches(0x1037afa0, -0x75a8, 6))) {
        constexpr auto site = Reference ? 0x1037dc60u : 0x1037afb0u;
        price_league_level(q, site, 0x28);
        taper_by_club_type(q, site + 128, 0x68, 0x90, 0x88, true);
        return;
    }

    constexpr auto late_site = [](std::uint32_t country) { return country + (Reference ? 0x2cb0 : 0); };
    if (market_region_matches(q, late_site(0x1037b084), -0x7510, Reference ? 5 : 6)) {
        constexpr auto site = late_site(0x1037b094);
        price_league_level(q, site, 0xf8);
        taper_by_club_type(q, site + 128, 0x68, 0x40, 0xa0, true);
        return;
    }
    quote_block(c, late_site(0x1037b168), 2);
    if (guest::compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(Reference ? 6 : 4))) == 0) {
        constexpr auto site = late_site(0x1037b170);
        price_league_level(q, site, 0xf8);
        taper_by_club_type(q, site + 128, 0x68, 0x40, 0xa0, true);
        return;
    }
    if (market_region_matches(q, late_site(0x1037b244), -0x79fc)
        || market_region_matches(q, late_site(0x1037b254), -0x7590)
        || (Reference && market_region_matches(q, 0x1037df14, -0x75a8))) {
        constexpr auto site = Reference ? 0x1037df24u : 0x1037b264u;
        price_league_level(q, site, 0x28);
        taper_by_club_type(q, site + 128, 0x68, 0x40, 0xa0, true);
        return;
    }
    const auto first_final_region = [&] {
        if constexpr (!Reference) return market_region_matches(q, 0x1037b338, -0x7b68);
        quote_block(c, 0x1037dff8, 2);
        c.set(5, c.toc(-0x7b68));
        c.set(5, c.u32(c.r(5)));
        quote_block(c, 0x1037e000, 2);
        return guest::compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(5))) == 0;
    };
    if (first_final_region() || market_region_matches(q, Reference ? 0x1037e008u : 0x1037b348u, -0x7508)) {
        constexpr auto site = Reference ? 0x1037e018u : 0x1037b358u;
        price_league_level(q, site, 0xf8);
        scale_and_taper_market(q, site + 128, 0xb8, 0x48);
        return;
    }
    scale_and_taper_market(q, Reference ? 0x1037e0c0u : 0x1037b400u, 0x230, 0x48, true);
}

template<bool Reference>
constexpr std::uint32_t closing_site(std::uint32_t country) {
    return country + (Reference ? 0x2cc0 : 0);
}

template<bool Reference>
void closing_block(WageQuote<Reference>& q, std::uint32_t country, unsigned count) {
    quote_block(q.c, closing_site<Reference>(country), count);
}

template<bool Reference>
void lookup_current_club_contract(WageQuote<Reference>& q, std::uint32_t pc) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, 4);
    c.set(3, c.u32(q.manager_slot));
    c.set({{4, q.player}, {5, c.r(25)}});
    c.call(0x10082f90, pc + 16);
}

template<bool Reference>
void cap_at_current_club_contract(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    closing_block(q, 0x1037b438, 2);
    const auto current = c.r(25);
    if (!guest::nonzero_logical(c, current)) return;
    closing_block(q, 0x1037b440, 2);
    if (guest::compare_logical(c, q.target_club, current) == 0) return;
    lookup_current_club_contract(q, closing_site<Reference>(0x1037b448));
    closing_block(q, 0x1037b458, 2);
    const auto contract = c.r(3);
    if (!guest::nonzero_logical(c, contract)) return;
    closing_block(q, 0x1037b460, 4);
    const auto clause = c.u8(contract + 0x60) & 0xf;
    c.set(0, clause);
    if (guest::compare(c, clause, 7) != 0) return;

    closing_block(q, 0x1037b470, 10);
    const auto wage = c.u32(contract + 12);
    const auto frame = c.sp();
    c.set({{3, wage}, {0, high_word}});
    c.put32(frame + 0x38, high_word);
    const auto encoded = wage ^ 0x80000000;
    c.set(3, encoded);
    c.set_f(1, q.number(0xc0));
    c.put32(frame + 0x3c, encoded);
    c.set_f(0, c.f64(frame + 0x38));
    c.set_f(0, q.fp.fsubs(c.f(0), c.f(1)));
    q.fp.fcmp(0, c.f(31), c.f(0));
    if (c.cr(0) & 4) {
        closing_block(q, 0x1037b498, 4);
        c.put32(frame + 0x3c, encoded);
        c.put32(frame + 0x38, high_word);
        c.set_f(0, c.f64(frame + 0x38));
        c.set_f(31, q.fp.fsubs(c.f(0), c.f(1)));
    }
}

template<bool Reference>
bool query_destination_wage(WageQuote<Reference>& q, std::uint32_t pc) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, 3);
    c.set(3, c.u32(q.reputation_manager));
    c.set(4, q.target_club);
    c.call(0x10103510, pc + 12);
    quote_block(c, pc + 12, 2);
    if (!guest::nonzero_logical(c, c.r(3))) {
        quote_block(c, pc + 20, 2);
        c.set_f(1, c.f(31));
        return false;
    }
    quote_block(c, pc + 28, 3);
    c.set({{4, q.player}, {5, q.role_override}});
    c.call(0x100f2680, pc + 40);
    return true;
}

template<bool Reference>
bool requires_destination_wage_cap(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    closing_block(q, 0x1037b4a8, 2);
    if (detail::record_nonzero(c, std::int8_t(q.quote_mode))) return true;
    closing_block(q, 0x1037b4b0, 2);
    if (guest::nonzero_logical(c, c.r(25))) return false;
    closing_block(q, 0x1037b4b8, 4);
    const auto profile = c.u32(q.player + player_profile);
    c.set(3, profile);
    const auto rating = c.i16(profile + 10);
    c.set(0, guest::extended(rating));
    return guest::compare(c, rating, 2250) < 0;
}

template<bool Reference>
void cap_at_destination_wage(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    closing_block(q, 0x1037b4f0, 9);
    const auto encoded = c.r(3) ^ 0x80000000;
    const auto frame = c.sp();
    c.set({{3, encoded}, {0, high_word}});
    c.put32(frame + 0x3c, encoded);
    c.set_f(1, q.number(0xc0));
    c.put32(frame + 0x38, high_word);
    c.set_f(0, c.f64(frame + 0x38));
    c.set_f(0, q.fp.fsubs(c.f(0), c.f(1)));
    q.fp.fcmp(0, c.f(0), c.f(31));
    if (c.cr(0) & 8) {
        closing_block(q, 0x1037b514, 1);
        c.set_f(31, c.f(0));
    }
}

template<bool Reference>
bool merits_final_wage_adjustment(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    closing_block(q, 0x1037b518, 2);
    if (!guest::nonzero_logical(c, c.r(25))) return false;
    closing_block(q, 0x1037b520, 2);
    if (detail::record_nonzero(c, std::int8_t(q.quote_mode))) return false;
    if (signed_byte_at(c, closing_site<Reference>(0x1037b528), q.player, 0x63, 10) <= 0) return false;
    if (signed_byte_at(c, closing_site<Reference>(0x1037b538), q.player, 0x65, 6) >= 0) return false;
    const auto profile = c.r(24);
    if (signed_short_at(c, closing_site<Reference>(0x1037b548), profile, 10, 6750) <= 0) return false;
    for (const auto [pc, attribute] : std::array{std::pair{0x1037b554u, 0x15u}, std::pair{0x1037b560u, 0x14u},
                                                std::pair{0x1037b56cu, 0x13u}}) {
        closing_block(q, pc, 3);
        const auto value = c.u8(profile + attribute);
        c.set(0, value);
        if (guest::compare(c, value, 20) != 0) continue;
        closing_block(q, 0x1037b578, 4);
        const auto age = std::int8_t(c.i16(q.player + player_age));
        c.set(0, std::uint32_t(age));
        return guest::compare(c, age, 20) > 0;
    }
    return false;
}

template<bool Reference>
void scale_final_quote(WageQuote<Reference>& q, std::uint32_t pc, unsigned factor, bool jump = true) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, jump ? 4 : 3);
    c.set_f(0, q.number(factor));
    c.set_f(31, q.fp.fmul(c.f(0), c.f(31)));
    c.set_f(31, q.fp.frsp(c.f(31)));
}

template<bool Reference>
bool final_quote_exceeds_destination(WageQuote<Reference>& q, std::uint32_t pc) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, 4);
    c.set_f(0, q.number(0x118));
    c.set_f(0, q.fp.fmul(c.f(0), c.f(31)));
    q.fp.fcmp(0, c.f(1), c.f(0));
    return c.cr(0) & 4;
}

void scale_reference_for_experience(WageQuote<true>& q, std::uint32_t pc, unsigned senior, unsigned standard,
                                    bool jump = true) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, 3);
    const auto experience = c.u8(q.player + 0x65);
    c.set(0, experience);
    if (guest::compare(c, experience, 1) == 0) scale_final_quote(q, pc + 12, senior);
    else scale_final_quote(q, pc + 28, standard, jump);
}

template<bool Reference>
void adjust_final_wage(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto start = Reference ? 0x1037e270u : 0x1037b5c0u;
    const auto frame = c.sp();
    quote_block(c, start, 10);
    const auto encoded = c.r(3) ^ 0x80000000;
    c.set({{0, encoded}, {3, high_word}});
    c.put32(frame + 0x3c, encoded);
    const auto profile = c.r(24);
    const auto first = c.u8(profile + 0x14);
    c.set(0, first);
    c.put32(frame + 0x38, high_word);
    c.set_f(1, q.number(0xc0));
    const auto strong = guest::compare(c, first, 20) == 0;
    c.set_f(0, c.f64(frame + 0x38));
    c.set_f(1, q.fp.fsubs(c.f(0), c.f(1)));

    if (strong) {
        if constexpr (Reference) scale_reference_for_experience(q, 0x1037e298, 0x150, 0x158);
        else if (final_quote_exceeds_destination(q, 0x1037b5e8)) scale_final_quote(q, 0x1037b5f8, 0x158);
        return;
    }
    constexpr auto second = Reference ? 0x1037e2c4u : 0x1037b608u;
    quote_block(c, second, 3);
    const auto other = c.u8(profile + 0x13);
    c.set(0, other);
    if (guest::compare(c, other, 20) == 0) {
        if (final_quote_exceeds_destination(q, second + 12)) {
            if constexpr (Reference) scale_reference_for_experience(q, 0x1037e2e0, 0x158, 0x198);
            else scale_final_quote(q, 0x1037b624, 0x198);
        }
        return;
    }
    constexpr auto fallback = Reference ? 0x1037e30cu : 0x1037b634u;
    if (final_quote_exceeds_destination(q, fallback)) {
        if constexpr (Reference) scale_reference_for_experience(q, 0x1037e31c, 0x128, 0x150, false);
        else scale_final_quote(q, 0x1037b644, 0x150, false);
    }
}

template<bool Reference>
bool finish_player_wage_adjustments(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    closing_block(q, 0x1037b428, 2);
    if (!guest::nonzero_logical(c, q.target_club)) return true;
    closing_block(q, 0x1037b430, 2);
    if (!guest::nonzero_logical(c, c.r(24))) return true;
    cap_at_current_club_contract(q);
    if (requires_destination_wage_cap(q)) {
        if (!query_destination_wage(q, closing_site<Reference>(0x1037b4c8))) return false;
        cap_at_destination_wage(q);
    }
    if (!merits_final_wage_adjustment(q)) return true;
    if constexpr (!Reference) lookup_current_club_contract(q, 0x1037b588);
    if (!query_destination_wage(q, Reference ? 0x1037e248u : 0x1037b598u)) return false;
    adjust_final_wage(q);
    return true;
}

template<bool Reference>
bool raise_final_quote_floor(WageQuote<Reference>& q, std::uint32_t pc, unsigned floor, bool jump = true) {
    QuoteCpu& c = q.c;
    quote_block(c, pc, 3);
    c.set_f(0, q.single(floor));
    q.fp.fcmp(0, c.f(31), c.f(0));
    if (!(c.cr(0) & 8)) return false;
    quote_block(c, pc + 12, jump ? 2 : 1);
    c.set_f(31, c.f(0));
    return true;
}

template<bool Reference>
void apply_final_wage_floor(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto start = Reference ? 0x1037e344u : 0x1037b650u;
    quote_block(c, start, 2);
    if (!guest::nonzero_logical(c, q.target_club)) return;
    quote_block(c, start + 8, 3);
    const auto type = c.u8(q.target_club + club_type);
    c.set(0, type);
    if (guest::compare(c, type, 1) == 0) {
        quote_block(c, start + 72, 2);
        if (guest::compare(c, std::int32_t(c.r(20)), 20) < 0
            && raise_final_quote_floor(q, start + 80, 0xc4))
            return;
        raise_final_quote_floor(q, start + 100, 0x130, false);
        return;
    }
    if (signed_short_at(c, start + 20, q.target_club, club_rating, 2250) > 0
        && raise_final_quote_floor(q, start + 32, 0xc4))
        return;
    raise_final_quote_floor(q, start + 52, 0xcc);
}

template<bool Reference>
void restore_wage_quote_frame(QuoteCpu& c) {
    quote_block(c, Reference ? 0x1037e3bcu : 0x1037b6c8u, 7);
    const auto caller = c.u32(c.sp() + frame_size + 8);
    const auto stack = c.sp() + frame_size;
    c.set({{0, caller}, {1, stack}});
    c.s.wr<8>(guest::reg::fpr(31), c.u64(stack - 8));
    c.s.wr<8>(guest::reg::fpr(30), c.u64(stack - 16));
    c.set_lr(caller);
    detail::transfer_saved_registers<13, true>(c, stack - 16);
}

template<bool Reference>
bool quote_base_wage(WageQuote<Reference>& q) {
    QuoteCpu& c = q.c;
    constexpr auto profile_case = WageQuote<Reference>::site(0x103799f0);
    constexpr auto default_case = Reference ? 0x1037cfe8u : 0x1037a3e4u;
    const auto target = select_quote_case(q).value_or(default_case);
    if (target == default_case) {
        quote_block(c, default_case, 1);
        c.set_f(31, q.single(0xa8));
        return true;
    }
    if (target == profile_case) {
        if (choose_profile_mode(q)) {
            choose_profile_floor(q);
            if constexpr (Reference) promote_reference_floor(q);
            apply_profile_club_discount(q);
            prepare_profile_rating(q);
            calculate_profile_points(q);
            adjust_profile_league(q);
            apply_profile_points(q);
            scale_profile_league(q);
            scale_profile_destination(q);
            limit_profile_by_band(q);
            taper_profile_by_attributes(q);
            adjust_profile_for_dates(q);
        } else {
            quote_without_profile(q);
        }
        return true;
    }
    for (const auto& role : role_quotes) {
        if (target != WageQuote<Reference>::site(role.entry)) continue;
        quote_ranked_role(q, role);
        return true;
    }
    constexpr auto entry = Reference ? reference_quote_entry : country_quote_entry;
    constexpr auto end = Reference ? 0x1037e3d8u : 0x1037b6e4u;
    if (target >= entry && target < end)
        throw std::logic_error("Unclassified staff-contract wage quote continuation");
    c.s.pc = target;
    return false;
}

template<bool Reference>
void run_wage_quote(Cpu& cpu, SemanticGuestObserver* observer = nullptr) {
    QuoteCpu c(cpu, observer);
    constexpr auto entry = Reference ? reference_quote_entry : country_quote_entry;
    quote_block(c, entry, 24);
    GuestNativeScope scope(c.s, entry);
    const auto stack = c.sp(), caller = c.lr();
    c.set(0, caller);
    c.put64(stack - 8, c.s.r<8>(guest::reg::fpr(31)));
    c.put64(stack - 16, c.s.r<8>(guest::reg::fpr(30)));
    detail::transfer_saved_registers<13>(c, stack - 16);
    WageQuote<Reference> q(c, c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9));
    c.put32(stack + 8, caller);
    c.put32(stack - frame_size, stack);
    c.set(1, stack - frame_size);
    scope.frame(stack - frame_size);

    resolve_current_club(q);
    const auto destination = initialize_player_quote(q);
    select_rating_band(q, destination);
    choose_quote_role(q);
    if (!quote_base_wage(q)) {
        run_until(c.s, caller);
        return;
    }
    apply_destination_quote(q);
    adjust_for_wage_market(q);
    if (finish_player_wage_adjustments(q)) {
        apply_final_wage_floor(q);
        quote_block(c, Reference ? 0x1037e3b8u : 0x1037b6c4u, 1);
        c.set_f(1, c.f(31));
    }
    restore_wage_quote_frame<Reference>(c);
    c.finish();
}
}

void quote_country_wage(Cpu& c) { run_wage_quote<false>(c); }
void quote_reference_wage(Cpu& c) {
    run_wage_quote<true>(c, shortlist_quotation_diagnostics(std::as_const(c.s)).observer);
}
}

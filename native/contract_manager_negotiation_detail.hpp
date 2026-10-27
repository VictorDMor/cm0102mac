#pragma once
#include "contract_manager_random.hpp"
#include <optional>

namespace cm::aot::contract_manager::negotiation {
using namespace detail;

// One original negotiation call. Named values drive the policy; the retained
// guest registers and their stack images remain visible to its dependencies.
struct Context {
    Cpu& c;
    Frame frame;
    Program program;
    guest::Fpu fp;
    Kept<13> country;
    Kept<14> transfers_or_age_gap;
    Kept<15> special_club_or_floor;
    Kept<16> option;
    Kept<17> national_context_or_role_score;
    Kept<18> today;
    Kept<19> suitability;
    Kept<20> singles_or_finance;
    Kept<21> tenure_or_shortlist;
    Kept<22> age;
    Kept<23> manager_or_base;
    Kept<24> mood;
    Kept<25> diagnostics;
    Kept<26> price;
    Kept<27> person;
    Kept<28> contract;
    Kept<29> buyer;
    Kept<30> employer;
    Kept<31> constants;
    double age_factor, finance_factor, relation_basis, market_factor, finance_balance;
    double quote_factor, promise_factor, potential_factor, tenure_factor, relationship_factor;
    bool has_employer;

    Context(Cpu& cpu, std::uint32_t manager, std::uint32_t employer_address,
            std::uint32_t buyer_address, std::uint32_t person_address,
            std::uint32_t diagnostic_address, std::uint32_t tenure_option, std::uint32_t other_option)
        : c(cpu), frame(c, 0x10085860, 0x150, {.first_gpr = 13, .first_fpr = 22}), program(c), fp(c),
          country(c,c.r(13)), transfers_or_age_gap(c,c.r(14)), special_club_or_floor(c,c.r(15)),
          option(c,c.r(16)), national_context_or_role_score(c,c.r(17)), today(c,c.r(18)),
          suitability(c,c.r(19)), singles_or_finance(c,c.r(20)), tenure_or_shortlist(c,c.r(21)),
          age(c,c.r(22)), manager_or_base(c,c.r(23)), mood(c,c.r(24)), diagnostics(c,c.r(25)),
          price(c,c.r(26)), person(c,c.r(27)), contract(c,c.r(28)), buyer(c,c.r(29)),
          employer(c,c.r(30)), constants(c,c.r(31)) {
        c.set(11,frame.caller());
        acquire_floating();
        constants = c.toc(-0x571c); employer = employer_address;
        has_employer = nonzero(c,std::int32_t(employer_address));
        special_club_or_floor = c.toc(-0x7a08); manager_or_base = manager;
        tenure_factor = c.f64(constants+0x18); c.set_f(30,tenure_factor);
        buyer = buyer_address; national_context_or_role_score = c.toc(-0x7a54); person = person_address;
        age_factor = tenure_factor; c.set_f(22,age_factor); country = c.toc(-0x79fc);
        relationship_factor = tenure_factor; c.set_f(31,relationship_factor); singles_or_finance = c.toc(-0x5734);
        potential_factor = tenure_factor; c.set_f(29,potential_factor); transfers_or_age_gap = c.toc(-0x7b40);
        today = c.toc(-0x7b80); diagnostics = diagnostic_address; tenure_or_shortlist = tenure_option; option = other_option;
    }
    void retain_floating() {
        c.set_f(22,age_factor); c.set_f(23,finance_factor); c.set_f(24,relation_basis);
        c.set_f(25,market_factor); c.set_f(26,finance_balance); c.set_f(27,quote_factor);
        c.set_f(28,promise_factor); c.set_f(29,potential_factor); c.set_f(30,tenure_factor); c.set_f(31,relationship_factor);
    }
    void acquire_floating() {
        age_factor=c.f(22); finance_factor=c.f(23); relation_basis=c.f(24); market_factor=c.f(25);
        finance_balance=c.f(26); quote_factor=c.f(27); promise_factor=c.f(28); potential_factor=c.f(29);
        tenure_factor=c.f(30); relationship_factor=c.f(31);
    }
    std::uint32_t call(std::uint32_t entry, std::uint32_t back,
                       std::initializer_list<std::uint32_t> arguments={}) {
        retain_floating();
        const auto answer=program.call(entry,back,arguments);
        acquire_floating();
        return answer;
    }
    void leave() { frame.leave(); c.set(11,frame.caller()); }
};

// Returns an early result, or leaves the complete live context ready for the
// tenure, relationship, finance and price-policy stages. This is not a
// production entry until whole-call checks and captured game states pass.
std::optional<std::uint32_t> prepare(Context&);
void terms_and_finance(Context&);
void relationships_and_potential(Context&);
void initial_quote_adjustment(Context&);
void protect_and_bound_price(Context&);
void regional_quote(Context&);
void young_value_floor(Context&);
std::uint32_t final_price(Context&);
}

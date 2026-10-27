#include "contract_manager_negotiation_detail.hpp"

namespace cm::aot::contract_manager::negotiation {
namespace {
bool byte_result(Cpu& c,std::uint32_t value) {
    const auto byte=std::int8_t(value);c.set(0,std::uint32_t(std::int32_t(byte)));return nonzero(c,byte);
}
double market_weight(Context& n) {
    auto& c=n.c;auto& fp=n.fp;const auto constants=std::uint32_t(n.constants),club=std::uint32_t(n.employer);
    const auto competition=c.u32(club+0x30);c.set(4,competition);double market;
    if(nonzero_logical(c,competition)) {
        const auto division=c.u32(club+0x2c);c.set(3,division);
        if(!nonzero_logical(c,division)){market=c.f64(constants+0x18);c.set_f(1,market);}
        else {
            const auto raw_level=c.u8(division+0x11c);const auto level=std::int8_t(raw_level);c.set(3,raw_level);c.set(0,0x43300000);c.put32(n.frame.at(0xa4),0x43300000);
            c.set_f(2,c.f64(constants+8));c.set(0,std::uint32_t(std::int32_t(level))^0x80000000u);c.set_f(0,c.f64(constants+0x18));c.put32(n.frame.at(0xa8),c.r(0));c.set_f(1,c.f64(n.frame.at(0xa4)));
            auto league=fp.fsub(c.f(1),c.f(2));c.set_f(1,league);fp.fcmp(0,league,c.f(0));bool reset=league<c.f(0);
            if(!reset){c.set_f(0,c.f64(constants+0x100));fp.fcmp(0,league,c.f(0));reset=league>c.f(0);}
            if(reset){league=c.f64(constants+0x100);c.set_f(1,league);}
            const auto half=c.f64(constants+0xb8);c.set_f(0,half);const auto kind=c.i8(club+0x23d);c.set(0,std::uint32_t(std::int32_t(kind)));
            league=fp.multiply_add(0x100859dc,league,half,half);c.set_f(1,league);
            const auto kind_order=compare(c,kind,3);
            if(kind_order==0){c.set_f(0,c.f64(constants+0x230));league=fp.fmul(league,c.f(0));c.set_f(1,league);}
            else if(kind_order<0&&compare(c,kind,2)>=0){c.set_f(0,c.f64(constants+0x280));league=fp.fmul(league,c.f(0));c.set_f(1,league);}
            const auto standing=c.i16(competition+0x18);c.set(3,extended(standing));c.set(0,0x43300000);c.set_f(0,c.f64(constants+0x238));
            const auto square=std::uint32_t(std::int32_t(standing)*std::int32_t(standing));c.set(3,square);const auto club_score=c.i16(club+0x1ec);c.set(4,extended(club_score));c.put32(n.frame.at(0xa4),0x43300000);
            const auto denominator=fp.fmul(c.f(0),league);c.set_f(0,denominator);c.set_f(2,c.f64(constants+8));const auto score=square*25+extended(club_score);c.set(0,score^0x80000000u);c.put32(n.frame.at(0xa8),c.r(0));c.set_f(1,c.f64(n.frame.at(0xa4)));
            const auto numerator=fp.fsub(c.f(1),c.f(2));c.set_f(1,numerator);market=fp.fdiv(numerator,denominator);c.set_f(1,market);
        }
    } else {
        const auto division=c.u32(club+0x2c);c.set(4,division);
        if(!nonzero_logical(c,division)){market=c.f64(constants+0x18);c.set_f(1,market);}
        else {
            const auto raw_level=c.u8(division+0x11c);const auto level=std::int8_t(raw_level);c.set(3,raw_level);c.set(0,0x43300000);c.put32(n.frame.at(0xa4),0x43300000);c.set_f(2,c.f64(constants+8));
            c.set(0,std::uint32_t(std::int32_t(level))^0x80000000u);c.set_f(0,c.f64(constants+0x18));c.put32(n.frame.at(0xa8),c.r(0));c.set_f(1,c.f64(n.frame.at(0xa4)));
            auto league=fp.fsub(c.f(1),c.f(2));c.set_f(1,league);fp.fcmp(0,league,c.f(0));bool reset=league<c.f(0);
            if(!reset){c.set_f(0,c.f64(constants+0x100));fp.fcmp(0,league,c.f(0));reset=league>c.f(0);}
            if(reset){league=c.f64(constants+0x100);c.set_f(1,league);}
            const auto kind=c.i8(club+0x23d);c.set(0,std::uint32_t(std::int32_t(kind)));const auto kind_order=compare(c,kind,3);
            if(kind_order==0){c.set_f(0,c.f64(constants+0x230));league=fp.fmul(league,c.f(0));c.set_f(1,league);}
            else if(kind_order<0&&compare(c,kind,2)>=0){c.set_f(0,c.f64(constants+0x98));league=fp.fmul(league,c.f(0));c.set_f(1,league);}
            const auto raw_standing=c.u8(division+0x11f);const auto standing=std::int8_t(raw_standing);c.set(3,raw_standing);c.set(0,0x43300000);c.set_f(0,c.f64(constants+0x238));c.set(3,std::uint32_t(std::int32_t(standing)));
            const auto club_score=c.i16(club+0x1ec);c.set(4,extended(club_score));const auto square=std::uint32_t(std::int32_t(standing)*std::int32_t(standing));c.set(3,square);c.put32(n.frame.at(0xa4),0x43300000);c.set_f(2,c.f64(constants+8));
            const auto denominator=fp.fmul(c.f(0),league);c.set_f(0,denominator);const auto score=square*25+extended(club_score);c.set(0,score^0x80000000u);c.put32(n.frame.at(0xa8),c.r(0));c.set_f(1,c.f64(n.frame.at(0xa4)));
            const auto numerator=fp.fsub(c.f(1),c.f(2));c.set_f(1,numerator);market=fp.fdiv(numerator,denominator);c.set_f(1,market);
        }
    }
    c.set_f(0,c.f64(constants+0x1f8));c.set_f(3,c.f64(constants+0x188));auto distance=fp.fsub(c.f(0),market);c.set_f(4,distance);
    const auto divisor=c.f64(constants+0x288),one=c.f64(constants+0x18),floor=c.f64(constants+0xc0);c.set_f(1,divisor);c.set_f(2,one);c.set_f(0,floor);
    distance=fp.fmul(distance,distance);c.set_f(4,distance);distance=fp.fdiv(distance,c.f(3));c.set_f(3,distance);auto weight=fp.fdiv(distance,divisor);c.set_f(1,weight);weight=fp.fsub(one,weight);c.set_f(25,weight);fp.fcmp(0,weight,floor);
    if(weight<floor){weight=floor;c.set_f(25,weight);}const auto ceiling=c.f64(constants+0x28);c.set_f(0,ceiling);fp.fcmp(0,weight,ceiling);if(weight>ceiling){weight=ceiling;c.set_f(25,weight);}return weight;
}
void score_squad_role(Context& n) {
    auto& c=n.c;const auto squad_role=c.u8(n.contract+0x60)&15u;c.set(0,squad_role);const auto raw_mood=c.u8(n.contract+0x44);n.mood=raw_mood;
    const auto order=compare_logical(c,squad_role,7);n.mood=std::uint32_t(std::int32_t(std::int8_t(raw_mood)));
    if(order>0){n.national_context_or_role_score=0;return;}
    // Preserve the original jump's volatile state while the policy uses a
    // normal C++ switch. The indexed table is pinned in the private oracle.
    const auto table=c.toc(-0x5744);c.set(3,table);c.set(0,squad_role*4);const auto destination=c.u32(table+squad_role*4);c.set(3,destination);c.s.wr<4>(guest::reg::ctr,destination);
    unsigned baseline=0;
    switch(squad_role){case 1:baseline=75;break;case 2:case 5:baseline=50;break;case 3:baseline=40;break;case 4:baseline=25;break;case 6:baseline=0;break;
        case 7:n.mood=0xffffff9c;n.national_context_or_role_score=0;return;
        default:n.national_context_or_role_score=0;return;}
    const auto difference=std::uint32_t(baseline)-std::uint32_t(n.mood);c.set(4,difference);c.set(3,0);
    const bool nonnegative=std::int32_t(difference)>=0;c.s.wr<1>(guest::reg::xer_ca,nonnegative);const auto selected=nonnegative?difference:0u;c.set(0,selected);n.national_context_or_role_score=extended(std::int16_t(selected));
}
}

std::optional<std::uint32_t> prepare(Context& n) {
    auto& c=n.c;auto& fp=n.fp;
    if(!n.has_employer)return result(c,0);
    const auto found=n.call(0x10082f90,0x100858d8,{n.manager_or_base,n.person,n.employer});n.contract=found;
    if(!nonzero(c,std::int32_t(found)))return result(c,0);
    const auto flags=c.u8(n.contract+0x38);const auto role=std::int32_t(std::int8_t(flags))>>2;c.set(0,std::uint32_t(role));c.s.wr<1>(guest::reg::xer_ca,role<0&&(flags&2));
    if(compare(c,role,2)!=0)return result(c,0);
    const auto locked=c.i8(n.contract+0x54);c.set(0,std::uint32_t(std::int32_t(locked)));if(nonzero(c,locked))return result(c,0);
    if(byte_result(c,n.call(0x1014c2e0,0x10085924,{n.employer,1,1,0,0}))){
        const auto context=n.call(0x100c96e0,0x10085934,{n.employer});c.set(4,context);
        if(byte_result(c,n.call(0x10147e70,0x10085940,{c.u32(n.national_context_or_role_score),context}))){
            const auto repeated=n.call(0x100c96e0,0x10085950,{n.employer});c.set(5,repeated);
            if(byte_result(c,n.call(0x10149df0,0x10085960,{c.u32(n.national_context_or_role_score),0,repeated}))){
                const auto listed=c.u8(n.contract+0x5c)&4u;c.set(0,listed);if(!nonzero(c,std::int32_t(listed)))return result(c,c.u32(n.person+8));
            }
        }
    }
    n.market_factor=market_weight(n);
    const auto stored_age=c.i16(n.person+0x5c);c.set(0,extended(stored_age));n.age=std::uint32_t(std::int32_t(std::int8_t(stored_age)));
    if(compare(c,std::int32_t(std::uint32_t(n.age)),28)>0){
        const auto excess=std::uint32_t(n.age)-28;c.set(3,excess^0x80000000u);c.set(0,0x43300000);c.put32(n.frame.at(0xa4),0x43300000);c.set_f(2,c.f64(n.constants+8));c.put32(n.frame.at(0xa8),c.r(3));
        const auto divisor=c.f64(n.constants+0xc8);c.set_f(0,divisor);c.set_f(1,c.f64(n.frame.at(0xa4)));const auto one=c.f64(n.constants+0x18);c.set_f(3,one);
        const auto years=fp.fsub(c.f(1),c.f(2));c.set_f(1,years);c.set_f(2,c.f64(n.constants+0x98));const auto fraction=fp.fdiv(years,divisor);c.set_f(0,fraction);c.set_f(1,fp.fsub(one,fraction));
        n.call(0x10780fc0,0x10085bac);c.set(2,c.u32(n.frame.at(0x14)));n.age_factor=c.f(1);c.set_f(22,n.age_factor);
    }
    score_squad_role(n);
    const auto registry=c.toc(-0x7b24);c.set(3,registry);
    if(byte_result(c,n.call(0x10003dc0,0x10085cdc,{c.u32(registry),n.person,1}))){
        const auto context=n.call(0x100c96e0,0x10085cec,{n.employer});
        if(!nonzero_logical(c,context))n.mood=0xffffff9c;
        else {
            const auto repeated=n.call(0x100c96e0,0x10085d04,{n.employer});const auto traits=c.u32(repeated+0x1c);c.set(0,traits);
            if(nonzero_logical(c,traits)){
                const auto current=n.call(0x100c96e0,0x10085d18,{n.employer});const auto attributes=c.u32(current+0x1c);c.set(3,attributes);
                auto selected_mood=std::int32_t(std::int16_t(std::uint32_t(n.mood)));c.set(4,std::uint32_t(selected_mood));
                const auto target=-5*std::int32_t(c.i8(attributes+0x34));c.set(0,std::uint32_t(target));
                if(compare(c,selected_mood,target)>=0){const auto latest=n.call(0x100c96e0,0x10085d40,{n.employer});const auto last=c.u32(latest+0x1c);c.set(3,last);const auto trait=c.i8(last+0x34);c.set(0,std::uint32_t(std::int32_t(trait)));selected_mood=-5*std::int32_t(trait);c.set(4,std::uint32_t(selected_mood));}
                n.mood=extended(std::int16_t(selected_mood));
            }
        }
        n.national_context_or_role_score=0;n.tenure_or_shortlist=0;
    }
    const auto raw_promise=c.u8(n.contract+0x4c);const auto promise=std::int8_t(raw_promise);c.set(3,raw_promise);c.set(0,std::uint32_t(std::int32_t(promise)));const bool positive=compare(c,promise)>0;
    c.set(3,std::uint32_t(std::int32_t(promise))^0x80000000u);c.set(0,0x43300000);const auto local=positive?0xa4:0x9c;c.put32(n.frame.at(local),0x43300000);c.set_f(2,c.f64(n.constants+8));c.put32(n.frame.at(local+4),c.r(3));
    const auto divisor=c.f64(n.constants+(positive?0x148:0x290));c.set_f(0,divisor);c.set_f(1,c.f64(n.frame.at(local)));const auto one=c.f64(n.constants+0x18);c.set_f(3,one);
    const auto promise_value=fp.fsub(c.f(1),c.f(2));c.set_f(1,promise_value);const auto contribution=fp.fdiv(promise_value,divisor);c.set_f(0,contribution);n.promise_factor=fp.fadd(one,contribution);c.set_f(28,n.promise_factor);
    const auto terms=c.i32(n.contract+0x50);c.set(0,std::uint32_t(terms));const bool has_terms=nonzero(c,terms);const auto scale=c.f64(n.constants+0x20);c.set_f(0,scale);
    n.promise_factor=has_terms?fp.fdiv(n.promise_factor,scale):fp.fmul(n.promise_factor,scale);c.set_f(28,n.promise_factor);
    const auto attributes=c.u32(n.person+0x14);c.set(7,attributes);c.set(4,n.person);c.set(3,c.u32(n.employer+0x90));c.set(5,n.employer);
    const auto reputation=c.i16(attributes+10),ability=c.i16(attributes+6);c.set(8,extended(reputation));c.set(6,1);c.set(0,extended(ability));
    const auto adjusted_reputation=extended(reputation)+(extended(reputation)>>31);c.set(7,adjusted_reputation);c.set(0,std::uint32_t(std::int32_t(ability)*25));
    const auto half=std::uint32_t(std::int32_t(adjusted_reputation)>>1);c.s.wr<1>(guest::reg::xer_ca,std::int32_t(adjusted_reputation)<0&&(adjusted_reputation&1));
    const auto combined=half+std::uint32_t(std::int32_t(ability)*25);c.set(0,combined);c.set(7,extended(std::int16_t(combined)));
    const auto assessed=n.call(0x10219e40,0x10085e24);n.suitability=extended(std::int16_t(assessed));
    const auto value=n.call(0x10084be0,0x10085e44,{n.manager_or_base,n.person,n.employer,0,n.suitability,0});n.price=value;
    const bool available=nonzero(c,std::int32_t(value));n.manager_or_base=value;if(!available)return result(c,0);
    return std::nullopt;
}
}

namespace cm::aot::contract_manager {
std::uint32_t negotiated_player_value(Cpu& c, std::uint32_t manager, std::uint32_t employer,
                                     std::uint32_t buyer, std::uint32_t person, std::uint32_t diagnostics,
                                     std::uint32_t tenure_option, std::uint32_t floor_option) {
    negotiation::Context context(c,manager,employer,buyer,person,diagnostics,tenure_option,floor_option);
    const auto early=negotiation::prepare(context);
    std::uint32_t answer;
    if(early)answer=*early;
    else {
        negotiation::terms_and_finance(context);
        negotiation::relationships_and_potential(context);
        negotiation::initial_quote_adjustment(context);
        negotiation::protect_and_bound_price(context);
        negotiation::regional_quote(context);
        negotiation::young_value_floor(context);
        answer=negotiation::final_price(context);
    }
    context.leave();return answer;
}
}

#include "contract_manager_random.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
struct RequestSampleSites {
    std::uint32_t reseed, recursive, convert;
    unsigned bound_slot, draw_slot, result_slot;
};

// Ordinary bounds come from signed bytes and stay within [-2588,2767].
// Retain the original wide algorithm too, with explicitly injected bounds
// distinguished from normal inputs by the independent checks.
template<unsigned Register>
void sample_request(Cpu& c,Program& p,const Frame& f,const RandomTable& random,
                    std::uint32_t constants,Kept<23>& bound,bool present,RequestSampleSites sites) {
    Kept<Register> sample(c,c.r(Register));
    if(!present){sample=0;return;}
    random.advance(sites.reseed);
    c.set(3,0x10000);c.set(0,0xffff);
    bool wide=compare(c,std::int32_t(std::uint32_t(bound)),0xffff)>0;
    if(!wide){c.set(3,0xffff0000);c.set(0,0xffff0001);wide=compare(c,std::int32_t(std::uint32_t(bound)),-0xffff)<0;}
    if(!wide){
        const auto position=random.position();c.set(3,position);
        const bool positive=compare(c,std::int32_t(std::uint32_t(bound)))>0;
        const auto salt=c.u32(random.salt);c.set(0,salt);
        const auto draw=std::uint32_t(c.u16(position))+salt;c.set(3,draw);
        const auto quotient=std::uint32_t(divide_signed<4>(draw,std::uint32_t(bound)));
        const auto remainder=draw-quotient*std::uint32_t(bound);c.set(0,remainder);
        sample=positive?remainder:0u-remainder;return;
    }
    guest::Fpu fp(c);
    c.set(4,0x43300000);c.set(0,std::uint32_t(bound)^0x80000000u);
    c.put32(f.at(sites.bound_slot+4),std::uint32_t(bound)^0x80000000u);c.set(5,0x80010000);
    const auto position=random.position();c.set(7,position);c.set(3,0x10000);
    c.put32(f.at(sites.bound_slot),0x43300000);c.set(6,0x80008001);
    const auto bias=c.f64(constants+8);c.set_f(2,bias);
    const auto ten_times=std::uint32_t(bound)*10;c.set(0,ten_times);
    const auto encoded_bound=c.f64(f.at(sites.bound_slot));c.set_f(1,encoded_bound);c.set(5,0xffff);
    const auto divisor=c.f64(constants);c.set_f(0,divisor);
    const auto amount=fp.fsub(encoded_bound,bias);c.set_f(1,amount);
    const auto high=multiply_high(std::int32_t(0x80008001),std::int32_t(ten_times));c.set(3,std::uint32_t(high));
    const auto word=c.u16(position);c.set(8,word);const auto salt=c.u32(random.salt);c.set(7,salt);
    const auto fraction=fp.fdiv(amount,divisor);c.set_f(1,fraction);c.put32(f.at(sites.draw_slot),0x43300000);
    const auto draw=word+salt;c.set(7,draw);const auto draw_high=multiply_high(std::int32_t(0x80008001),std::int32_t(draw));c.set(4,std::uint32_t(draw_high));
    const auto floor=std::int32_t(std::uint32_t(high)+ten_times)>>15;
    const auto draw_floor=std::int32_t(std::uint32_t(draw_high)+draw)>>15;c.set(3,std::uint32_t(draw_floor));c.set(4,std::uint32_t(draw_floor)>>31);
    c.set(0,std::uint32_t(floor));c.s.wr<1>(guest::reg::xer_ca, std::int32_t(std::uint32_t(high)+ten_times)<0&&((std::uint32_t(high)+ten_times)&0x7fff));
    const auto draw_quotient=std::uint32_t(draw_floor)+(std::uint32_t(draw_floor)>>31);c.set(3,draw_quotient);c.set(4,draw_quotient*0xffff);
    const auto sign=std::uint32_t(floor)>>31;c.set(3,sign);c.set(3,std::uint32_t(floor)+sign);
    const auto encoded_draw=(draw-draw_quotient*0xffff)^0x80000000u;c.set(0,encoded_draw);c.put32(f.at(sites.draw_slot+4),encoded_draw);
    const auto encoded=c.f64(f.at(sites.draw_slot));c.set_f(0,encoded);const auto random_amount=fp.fsub(encoded,bias);c.set_f(0,random_amount);
    const auto product=fp.fmul(fraction,random_amount);c.set_f(0,product);c.s.pc=sites.convert;
    c.put64(f.at(sites.result_slot),fp.fctiwz(0,product));sample=c.u32(f.at(sites.result_slot+4));
    const auto refined=p.call(0x100788f0,sites.recursive);
    c.set(4,0x66666667);c.set(3,refined+5);const auto last=std::uint32_t(bound)-1;c.set(0,last);
    const auto refined_high=multiply_high(0x66666667,std::int32_t(refined+5));c.set(3,std::uint32_t(refined_high));
    const auto refined_floor=refined_high>>2;c.set(3,std::uint32_t(refined_floor));c.set(4,std::uint32_t(refined_floor)>>31);
    c.s.wr<1>(guest::reg::xer_ca,refined_high<0&&(std::uint32_t(refined_high)&3));
    const auto rounded=std::uint32_t(refined_floor)+(std::uint32_t(refined_floor)>>31);c.set(3,rounded);sample=sample+rounded;
    if(compare(c,std::int32_t(std::uint32_t(sample)),std::int32_t(last))>0)sample=last;
}
}

void review_transfer_request(Cpu& c,std::uint32_t owner_address,std::uint32_t contract_address) {
    c.set(0,c.lr());Frame f(c,0x1008e350,0x1a0,{.first_gpr=21,.gpr_offset=-0x3c,.first_fpr=31});
    Kept<30> contract(c,contract_address);const bool present=nonzero(c,std::int32_t(contract_address));
    Kept<23> registry(c,c.toc(-0x7b40));Kept<22> owner(c,owner_address);Kept<24> system(c,c.toc(-0x7bf0));
    Kept<25> constants(c,c.toc(-0x571c));Kept<26> salt(c,c.toc(-0x7b90));Kept<27> base(c,c.toc(-0x7b8c));
    Kept<28> end(c,c.toc(-0x7b88));Kept<29> cursor(c,c.toc(-0x7b84));Kept<31> error(c,c.toc(-0x7ba8));
    Kept<21> pool(c,c.toc(slot::pool));Program p(c);guest::Fpu fp(c);
    if(!present){
        p.call(0x10739890,0x1008e3ac,{pool+0x37,0,0,f.at(0x40),f.at(0x38)});
        const auto version=p.call(0x10768810,0x1008e3b0);
        p.call(0x1077b720,0x1008e3cc,{error,pool+0x23b,version,f.at(0x40),f.at(0x38),0x2957});
        p.call(0x10769b60,0x1008e3dc,{pool+0x248,error,0});
        const auto success=c.toc(-0x7bac);c.set(3,success);c.set(0,0);c.put8(success,0);return;
    }
    const auto person_offset=c.u32(contract)*112;c.set(0,person_offset);const auto staff_holder=c.toc(-0x7be0);c.set(3,staff_holder);
    const auto club_id=c.u32(contract+4);c.set(4,club_id);const auto staff=c.u32(staff_holder);c.set(3,staff);
    const auto club_holder=c.toc(-0x7be8);c.set(5,club_holder);Kept<31> person(c,staff+person_offset);
    const auto attributes=c.u32(person+0x14);c.set(0,attributes);c.set(3,club_id*0x248);const auto clubs=c.u32(club_holder);c.set(4,clubs);
    const bool player=nonzero_logical(c,attributes);Kept<21> club(c,clubs+club_id*0x248);if(!player)return;
    const auto registered=p.call(0x102cdbf0,0x1008e42c,{c.u32(system),club});const auto registered_byte=std::int8_t(registered);c.set(0,std::uint32_t(std::int32_t(registered_byte)));
    const bool public_news=nonzero(c,registered_byte);const RandomTable random{c,salt,base,end,cursor};
    if(!public_news){
        const auto role=c.u8(contract+0x60)&15u;c.set(0,role);
        if(compare(c,std::int32_t(role),1)!=0){
            const auto value=p.call(0x10084be0,0x1008e460,{owner,person,club,0,0xffffffff,0});c.set(3,value^0x80000000u);c.set(0,0x43300000);c.put32(f.at(0x158),value^0x80000000u);
            const auto bias=c.f64(constants+8);c.set_f(2,bias);c.put32(f.at(0x154),0x43300000);const auto multiplier=c.f64(constants+0xe0);c.set_f(0,multiplier);
            const auto encoded=c.f64(f.at(0x154));c.set_f(1,encoded);const auto amount=fp.fsub(encoded,bias);c.set_f(1,amount);c.set_f(31,fp.fmul(multiplier,amount));
            const auto negotiated=p.call(0x10085860,0x1008e4a4,{owner,club,0,person,0,1,1});c.set(3,negotiated^0x80000000u);c.set(0,0x43300000);c.put32(f.at(0x150),negotiated^0x80000000u);
            const auto fresh_bias=c.f64(constants+8);c.set_f(1,fresh_bias);c.put32(f.at(0x14c),0x43300000);const auto quoted=c.f64(f.at(0x14c));c.set_f(0,quoted);
            const auto quote=fp.fsub(quoted,fresh_bias);c.set_f(0,quote);const auto minimum=c.f(31);fp.fcmp(0,quote,minimum);
            if(quote<minimum){
                const auto flags=c.u8(contract+0x5c);c.set(3,flags);const auto bit=flags&2u;c.set(0,bit);
                if(nonzero_logical(c,bit)){c.set(0,flags&~2u);c.put8(contract+0x5c,flags&~2u);}
                const auto current=c.u8(contract+0x5c);c.set(0,current|8u);c.put8(contract+0x5c,current|8u);
                p.call(0x100d3570,0x1008e4f8,{person,12,0});const auto object=c.u32(registry);c.set(3,object);
                p.call(0x10363ca0,0x1008e50c,{c.u32(object+0x988),person,1});p.call(0x1008cd20,0x1008e51c,{owner,person,club});return;
            }
        }
        const auto ambition=c.i8(person+0x65);const auto reaction=c.u8(contract+0x4c);c.set(3,std::uint32_t(std::int32_t(ambition)));c.set(0,reaction);c.set(4,std::uint32_t(std::int32_t(ambition)*20));c.set(3,std::uint32_t(std::int32_t(std::int8_t(reaction))));
        Kept<23> tolerance_bound(c,std::uint32_t(std::int32_t(ambition)*20+std::int8_t(reaction)+100));const bool bound_present=nonzero(c,std::int32_t(std::uint32_t(tolerance_bound)));
        sample_request<24>(c,p,f,random,constants,tolerance_bound,bound_present,{0x1008e568,0x1008e630,0x1008e620,0x14c,0x154,0x144});
        random.advance(0x1008e6b0);const auto position=random.position();c.set(5,position);c.set(3,0x51ec0000);const auto seed=c.u32(salt);c.set(4,seed);c.set(0,0x51eb851f);
        const auto word=c.u16(position);c.set(3,word);const auto draw=word+seed;c.set(4,draw);const auto high=multiply_high(0x51eb851f,std::int32_t(draw));c.set(0,std::uint32_t(high));
        const auto floor=high>>4;c.set(0,std::uint32_t(floor));c.s.wr<1>(guest::reg::xer_ca,high<0&&(std::uint32_t(high)&15));c.set(3,std::uint32_t(floor)>>31);
        const auto quotient=std::uint32_t(floor)+(std::uint32_t(floor)>>31),remainder=draw-quotient*50;c.set(0,remainder);
        if(compare(c,std::int32_t(c.r(24)),std::int32_t(remainder))<0)p.call(0x1037fa80,0x1008e704,{contract,0x10000,0});
        p.call(0x100d3570,0x1008e714,{person,0xfffffff4,0});
        const auto raw_ambition=c.u8(person+0x65),old=c.u8(contract+0x4c);c.set(0,raw_ambition);c.set(3,old);const auto change=std::int8_t(raw_ambition*5u-200);c.set(4,std::uint32_t(std::int32_t(change)));
        const auto adjusted=std::int32_t(std::int8_t(old))+change;c.set(0,std::uint32_t(adjusted));
        if(compare(c,adjusted,100)>0){c.set(0,100);c.put8(contract+0x4c,100);}
        else if(compare(c,adjusted,-100)<0){c.set(0,std::uint32_t(-100));c.put8(contract+0x4c,std::uint32_t(-100));}
        else {c.set(0,old+std::uint32_t(std::int32_t(change)));c.put8(contract+0x4c,old+std::uint32_t(std::int32_t(change)));}return;
    }
    const auto first=p.call(0x103e6470,0x1008e770,{c.u32(registry),person});const auto first_byte=std::int8_t(first);c.set(0,std::uint32_t(std::int32_t(first_byte)));if(nonzero(c,first_byte))return;
    if(compare(c,std::int32_t(p.call(0x103eaea0,0x1008e78c,{c.u32(registry),person,0,0})))!=0)return;
    if(compare(c,std::int32_t(p.call(0x103eb020,0x1008e7a4,{c.u32(registry),person,0})))!=0)return;
    const auto message=f.at(0x60);p.call(0x102bdf90,0x1008e7c0,{message,0xbbf,0});
    p.call(0x102cb4b0,0x1008e7d0,{message,0,c.u32(club)});p.call(0x102cb4b0,0x1008e7e0,{message,2,c.u32(person)});
    for(unsigned field=3;field<=5;++field){const auto token=c.u32(person+0x1c+field*4);p.call(0x102cb4b0,0x1008e7e0+(field-2)*0x14,{message,field,c.u32(token+0x20)});}
    p.call(0x102cb4b0,0x1008e82c,{message,6,c.u32(contract+0x50)});p.call(0x102cb4b0,0x1008e83c,{message,7,0});p.call(0x102cb4b0,0x1008e84c,{message,8,0});
    const auto ambition=c.i8(person+0x65);Kept<23> agent_bound(c,std::uint32_t(std::int32_t(ambition)));const bool ambitious=nonzero(c,ambition);
    sample_request<22>(c,p,f,random,constants,agent_bound,ambitious,{0x1008e880,0x1008e948,0x1008e938,0x144,0x14c,0x154});
    bool use_profile=compare(c,std::int32_t(c.r(22)))==0;
    if(!use_profile){const auto trait=c.i8(person+0x67);agent_bound=std::uint32_t(std::int32_t(trait));const bool active=nonzero(c,trait);
        sample_request<22>(c,p,f,random,constants,agent_bound,active,{0x1008e9e4,0x1008eaac,0x1008ea9c,0x144,0x14c,0x154});use_profile=compare(c,std::int32_t(c.r(22)))==0;}
    bool agent=false;
    if(use_profile){const auto profile=c.u32(person+0x14);c.set(3,profile);if(nonzero_logical(c,profile)){const auto score=c.i16(profile+0xc);c.set(0,extended(score));agent=compare(c,score,4750)>0;}}
    p.call(0x102cb4b0,agent?0x1008eb3c:0x1008eb50,{message,9,agent?1u:0u});
    const auto nation=c.u32(person+0xc);c.set(3,nation);const bool nationality=nonzero_logical(c,nation);
    p.call(0x102cb4b0,nationality?0x1008eb6c:0x1008eb80,{message,10,nationality?c.u32(nation+0x68):0xffffffffu});
    const auto raw_age=c.i16(person+0x5c);const auto age=std::int8_t(raw_age);c.set(0,std::uint32_t(std::int32_t(age)));const bool retiring=compare(c,age,32)>0;
    p.call(0x102cb4b0,retiring?0x1008eba0:0x1008ebb4,{message,11,retiring?1u:0u});
    const auto status=p.call(0x1037fa70,0x1008ebbc,{contract});p.call(0x102cb4b0,0x1008ebcc,{message,12,status});p.call(0x102cc140,0x1008ebdc,{c.u32(system),message,club});
}
}

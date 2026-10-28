#include "oceania_nations_detail.hpp"
#include <array>
namespace cm::aot::oceania_nations {
using namespace detail;
void describe_event(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t event, std::uint32_t stage) {
    Frame f(c,0x106ea780,0x50,{.first_gpr=27}); Program p{c};
    c.set({{31,club},{28,c.toc(-0x7bb8)},{27,c.toc(-0x7be8)},{30,object},{4,c.toc(-0x4094)}});
    const auto buffer=c.r(28), clubs=c.r(27), pool=c.r(4), output=c.u32(f.at(0x90));
    const auto qualifier=c.u8(f.at(0x8f)), first=c.u8(f.at(0x8b)); c.set(29,output);
    c.compare(0,std::int8_t(stage),1); c.set({{6,event},{11,first},{10,qualifier}});
    if (std::int8_t(stage)<1) {
        c.compare(0,std::int8_t(event),6);
        if (std::int8_t(event)==6) {
            c.compare(0,std::int8_t(qualifier),0);
            if (qualifier) { p.call(0x104698c0,0x106ea7ec,{club,c.r(7),c.u32(object+4),event,qualifier,output}); return; }
            const auto offset=club*0x248; c.set(30,offset);
            const auto base=c.u32(clubs), gender=signed_byte(c.u8(base+offset+0x23b));
            p.call(0x10727890,0x106ea81c,{buffer,pool+0x27,gender,gender,base+offset+0x221});
            p.call(0x1074b830,0x106ea828,{output,buffer});
            const auto current=c.u32(clubs)+offset;
            p.call(0x1074bab0,0x106ea840,{output,current+0x221,2005,current}); return;
        }
        c.compare(0,std::int8_t(event),-2);
        if (std::int8_t(event)==-2) p.call(0x104698c0,0x106ea864,{club,c.r(7),c.u32(object+4),event,qualifier,output});
        return;
    }
    if (std::int8_t(stage)!=1) return;
    c.compare(0,std::int8_t(qualifier),0);
    if (qualifier) { p.call(0x10464d00,0x106ea88c,{club,c.r(8),first,event,c.r(9),output}); return; }
    struct Message { int event; unsigned offset, back; };
    constexpr std::array messages{Message{1,0x66,0x106ea8dc},Message{2,0xb6,0x106ea928},Message{3,0x117,0x106ea974},Message{4,0x179,0x106ea9c0}};
    unsigned text=0x1dc, back=0x106eaa0c;
    bool matched=false;
    for (const auto& m:messages) { c.compare(0,std::int8_t(event),m.event); if (std::int8_t(event)==m.event) { text=m.offset;back=m.back;matched=true;break; } }
    if (!matched) {
        c.compare(0,std::int8_t(event),-1);
        // The repeated beq at aa10 makes its following text block unreachable.
        if (std::int8_t(event)==-1) { text=0x241;back=0x106eaa9c; }
    }
    const auto offset=club*0x248, base=c.u32(clubs), definition=c.u32(object+4);
    const auto gender=signed_byte(c.u8(base+offset+0x23b)), competition_gender=signed_byte(c.u8(definition+0x67));
    p.call(0x10727890,back,{buffer,pool+text,gender,gender,competition_gender,competition_gender,base+offset+0x221,definition+0x4d});
    p.call(0x1074b830,0x106eaaa8,{output,buffer});
    const auto current=c.u32(clubs)+offset;
    p.call(0x1074bab0,0x106eaac4,{output,current+0x221,2005,current});
    const auto current_definition=c.u32(object+4);
    p.call(0x1074bab0,0x106eaad8,{output,current_definition+0x4d,2000,current_definition});
}
std::uint32_t notify(Cpu& c, std::uint32_t object, std::uint32_t club, std::uint32_t event, std::uint32_t stage, std::uint32_t record) {
    Frame f(c,0x106eac00,0x50,{.first_gpr=28}); Program p{c};
    c.set({{31,c.toc(-0x7b48)},{30,record},{29,club},{28,object}}); const auto news=c.r(31);
    c.compare(0,std::int8_t(stage),1);
    if (std::int8_t(stage)<1) {
        c.compare(0,std::int8_t(event),6);
        if (std::int8_t(event)==6) p.call(0x10384500,0x106eac5c,{c.u32(news),club,c.u32(object+4),0,0x82,0x19});
        else { c.compare(0,std::int8_t(event),-2); if (std::int8_t(event)==-2) p.call(0x10384820,0x106eac7c,{c.u32(news),club,c.u32(object+4),0x41c,0x19}); }
    } else if (std::int8_t(stage)==1) {
        c.compare(0,std::int8_t(event),0);
        if (std::int8_t(event)==0 || std::int8_t(event)==-1) {
            if (std::int8_t(event)) c.compare(0,std::int8_t(event),-1);
            const bool winner=std::int8_t(event)==0;
            p.call(0x10476780,winner?0x106eaca0:0x106eacf8,{40,100,signed_half(c.u16(record+0x34)),1}); const auto value=c.r(3);
            c.set(8,value);
            const auto position=signed_half(c.u16(record+0x32));
            if (winner) {
                const auto group=c.u32(c.u32(object+0xc)+4), rounds=c.u32(group+0xa0);
                const auto index=signed_half(c.u16(record+0x34));
                const auto place=signed_half(c.u16(rounds+index*0x78+0x80));
                p.call(0x10384500,0x106eacd4,{c.u32(news),club,c.u32(object+4),position,place,value});
            } else p.call(0x103846e0,0x106ead14,{c.u32(news),club,c.u32(object+4),position,signed_half(c.u16(record+0x30)),value});
        } else {
            c.compare(0,std::int8_t(event),-1); c.compare(0,std::int8_t(event),4);
            if (std::int8_t(event)!=4) {
                c.compare(0,std::int8_t(event),1);
                if (std::int8_t(event)==1) p.call(0x10383410,0x106ead38,{c.u32(news),club,record,c.r(9)});
                else { c.compare(0,std::int8_t(event),2);
                    if (std::int8_t(event)==2) p.call(0x10383b70,0x106ead54,{c.u32(news),club,record,c.r(9)});
                    else { c.compare(0,std::int8_t(event),3); if (std::int8_t(event)==3) p.call(0x10384020,0x106ead70,{c.u32(news),club,record,c.r(9)}); }
                }
            }
        }
    }
    return 0;
}
void record_positions(Cpu& c, std::uint32_t object) {
    Frame f(c,0x106ebb10,0xa0,{.first_gpr=27}); Program p{c};
    c.set({{27,c.toc(-0x7bac)},{30,object},{28,c.toc(-0x7ba8)},{29,c.toc(-0x4094)}});
    const auto fixtures=c.u32(object+8); c.compare_logical(0,fixtures,0);
    if (!fixtures) { assertion(c,f,0x106ebb50,0x68,0x40,0x436); return; }
    const auto bytes=signed_half(c.u16(object+0xf0))*4;
    const auto sorted=p.call(0x10775130,0x106ebb9c,{bytes}); c.set(31,sorted); c.compare(0,std::int32_t(sorted),0);
    if (!sorted) { assertion(c,f,0x106ebbb8,0x48,0x38,0x43e); return; }
    p.call(0x10739ef0,0x106ebc08,{sorted,c.u32(object+0xec),signed_half(c.u16(object+0xf0))*4});
    p.call(0x10768300,0x106ebc1c,{sorted,signed_half(c.u16(object+0xf0)),4,c.toc(-0x6188)});
    constexpr std::array<unsigned,6> positions{1,2,3,3,5,5};
    for (unsigned index=0;index<6;++index) p.call(0x10457490,0x106ebc34+index*0x18,{c.u32(object+8),c.u32(sorted+index*4),positions[index]});
    p.call(0x10775170,0x106ebcb4,{sorted});
}
void update_record(Cpu& c, std::uint32_t object, std::uint32_t event, std::uint32_t stage,
                   std::uint32_t first, std::uint32_t second, std::uint32_t third) {
    Frame f(c,0x106ebcd0,0x50,{.first_gpr=28}); Program p{c};
    c.set({{31,third},{30,second},{29,first},{28,stage}});
    const auto record=p.call(0x10459b50,0x106ebd04,{c.u32(object+8),event});
    c.compare(0,std::int8_t(stage),1);
    if (std::int8_t(stage)<1) {
        const auto map=[&](std::uint32_t value, unsigned low, unsigned high) {
            c.compare(0,std::uint8_t(value),3); if (std::uint8_t(value)==3) return high;
            if (std::uint8_t(value)<3) { c.compare(0,std::uint8_t(value),1); if (std::uint8_t(value)>=1) return low; }
            return value;
        };
        second=map(second,1,5); c.set(30,second); third=map(third,3,5); c.set(31,third);
        c.compare(0,std::uint8_t(first),2);
        if (std::uint8_t(first)==2) first=3;
        else if (std::uint8_t(first)<2) { c.compare(0,std::uint8_t(first),1); if (std::uint8_t(first)>=1) first=1; }
        else { c.compare(0,std::uint8_t(first),4); if (std::uint8_t(first)<4) first=5; }
        c.set(29,first);
    }
    c.put8(record+0x76,first); c.put8(record+0x77,second); c.put8(record+0x78,third);
}
}

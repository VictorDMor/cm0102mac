#include "nation_awards_detail.hpp"

namespace cm::aot::nation_awards {
using namespace detail;
void publish_ranked(Cpu& c,std::uint32_t manager,std::uint32_t date,std::uint32_t award,
                    std::uint32_t ratings,std::uint32_t filter,std::uint32_t first,
                    std::uint32_t second,std::uint32_t third) {
    Frame frame(c,0x10444580,0x140,{.first_gpr=13}); Program p{c}; Fpu fp(c);
    const auto history=Award{c,award}.ratings();
    keep(c,{{13,c.toc(-0x7bac)},{14,manager},{15,c.toc(-0x7ba8)},
            {16,ratings},{17,filter},{18,first},{19,second},{20,third},
            {21,c.u32(frame.at(0x178))},{22,c.u8(frame.at(0x17f))},
            {23,c.u8(frame.at(0x183))},{25,3},{27,history},{28,c.toc(-0x7be0)},
            {29,c.toc(-0x4a18)},{30,date},{31,award}});
    if (compare_logical(c,history)!=0) c.set(25,std::uint32_t(c.i16(history+6)));
    const auto count=std::int16_t(c.r(25)); c.set(26,std::uint32_t(count));
    const auto selected=p.call(0x10775130,0x104445ec,{std::uint32_t(count)*4}); c.set(24,selected);
    if (compare(c,selected)==0) { assertion(c,frame,0x10444608,0x70,0x48,0xab); return; }
    if (compare_logical(c,ratings)==0) { assertion(c,frame,0x10444664,0x50,0x40,0xb2); return; }
    const auto statistic=std::int8_t(c.r(22));
    c.put32(frame.at(0x38),c.r(22)); c.put32(frame.at(0x3c),c.r(23));
    p.call(0x1044ec90,0x104446d0,{ratings,selected,c.r(25),filter,first,second,third,c.r(21)});
    if (compare(c,std::int16_t(c.r(3)),count)<0) { p.call(0x10775170,0x104446e4,{selected}); return; }
    if (compare_logical(c,history)!=0) {
        p.call(0x10439ea0,0x104446f8,{history});
        keep(c,{{13,0},{15,selected},{16,std::uint32_t(count)}});
        for (unsigned index=0;compare(c,std::int16_t(index),count)<0;++index) {
            const auto rating=c.u32(selected+index*4);
            c.set(3,rating); compare(c,statistic);
            c.set(4,indirect(c,-0x7be0)+c.u32(rating)*Person::size);
            c.set(5,c.u32(c.r(4)+0x24));
            c.set(6,0x43300000); c.set(7,c.toc(-0x4a1c));
            c.set_f(2,c.f64(c.r(7)));
            if (statistic==0) {
                c.set(3,std::uint32_t(c.i16(rating+8))^0x80000000);
                c.put32(frame.at(0xe4),0x43300000); c.put32(frame.at(0xe8),c.r(3));
                c.set_f(0,c.f64(frame.at(0xe4)));
                c.put32(frame.at(0xe0),std::uint32_t(std::int8_t(c.u8(rating+14)))^0x80000000);
                c.set_f(1,fp.fsubs(c.f(0),c.f(2)));
                c.put32(frame.at(0xdc),0x43300000); c.set_f(0,c.f64(frame.at(0xdc)));
            } else {
                compare(c,statistic,1);
                std::uint32_t numerator;
                if (statistic==1) numerator=std::uint32_t(c.i16(rating+10));
                else {
                    compare(c,statistic,2);
                    if (statistic==2) numerator=std::uint32_t(c.i16(rating+12));
                    else {
                        c.set(7,std::uint32_t(c.i16(rating+16))); c.set(8,std::uint32_t(c.i16(rating+18)));
                        numerator=c.r(7)*2+c.r(8); c.set(7,c.toc(-0x4a1c));
                    }
                }
                c.set(3,numerator^0x80000000);
                c.put32(frame.at(0xdc),0x43300000); c.put32(frame.at(0xe0),c.r(3));
                c.set_f(0,c.f64(frame.at(0xdc))); c.set_f(1,fp.fsubs(c.f(0),c.f(2)));
                c.put32(frame.at(0xe8),std::uint32_t(std::int8_t(c.u8(rating+14)))^0x80000000);
                c.put32(frame.at(0xe4),0x43300000); c.set_f(0,c.f64(frame.at(0xe4)));
            }
            c.set_f(0,fp.fsubs(c.f(0),c.f(2))); c.set_f(1,fp.fdivs(c.f(1),c.f(0)));
            p.call(0x104382c0,0x1044486c,{history});
            keep(c,{{13,index+1},{15,selected+(index+1)*4}});
        }
        p.call(0x10438450,0x10444894,{history,date,c.u32(manager+24),c.u32(award+20)});
    } else {
        c.put16(frame.at(0x90),c.u16(date+2));
        leave(c,{{3,selected},{5,indirect(c,-0x7be0)},{6,0}});
        for (unsigned index=0;compare(c,std::int8_t(index),count)<0;++index) {
            const auto rating=c.u32(c.r(3)); c.set(4,rating);
            const Person person{c,c.r(5)+c.u32(rating)*Person::size};
            c.set(9,missing); c.set(10,person.address); c.set(4,person.club());
            if (compare_logical(c,c.r(4))!=0) c.set(9,c.u32(c.r(4)));
            c.set(4,person.name(0));
            c.set(7,compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing);
            c.set(4,person.name(1));
            c.set(8,compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing);
            c.set(4,person.name(2));
            c.set(4,compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing);
            compare(c,std::int8_t(index)); if (index) compare(c,std::int8_t(index),1);
            const auto record=frame.at(0x94+(index<2?index:2)*24);
            store_winner(c,record,c.r(7),c.r(8),c.r(4),person.id(),c.r(9));
            c.set(3,c.r(3)+4); c.set(6,index+1);
        }
        p.call(0x1044cf90,0x104449a4,{award,frame.at(0x90),1});
    }
    p.call(0x10775170,0x104449ac,{selected});
}
void publish_managers(Cpu& c,std::uint32_t date,std::uint32_t award,std::uint32_t ratings,
                      std::uint32_t filter,std::uint32_t first,std::uint32_t second,std::uint32_t selected_list) {
    Frame frame(c,0x104449c0,0x110,{.first_gpr=20}); Program p{c};
    keep(c,{{20,date},{21,ratings},{22,filter},{23,first},{24,second},{25,c.toc(-0x4a18)},
            {26,award},{27,selected_list},{28,c.toc(-0x7ba8)},{29,c.toc(-0x7bac)},{31,c.toc(-0x7be0)}});
    const auto selected=p.call(0x10775130,0x10444a04,{12}); c.set(30,selected);
    if (compare(c,selected)==0) { assertion(c,frame,0x10444a20,0x6c,0x44,0x153); return; }
    if (compare_logical(c,ratings)==0) { assertion(c,frame,0x10444a7c,0x4c,0x3c,0x15a); return; }
    p.call(0x1044efb0,0x10444adc,{ratings,selected,3,filter,first,second,selected_list});
    if (compare(c,std::int16_t(c.r(3)),3)<0) { p.call(0x10775170,0x10444af0,{selected}); return; }
    c.put16(frame.at(0x8c),c.u16(date+2)); keep(c,{{25,selected},{29,0}});
    for (unsigned index=0;compare(c,std::int8_t(index),3)<0;++index) {
        c.set(3,c.u32(c.r(25))); compare_logical(c,selected_list);
        c.set(4,indirect(c,-0x7be0)); c.set(28,missing);
        c.put32(frame.at(0x38),c.r(4)+c.u32(c.r(3))*Person::size);
        if (selected_list) p.call(0x10738d10,0x10444b3c,{selected_list,frame.at(0x38),4});
        c.set(4,c.u32(frame.at(0x38))); const Person person{c,c.r(4)};
        c.set(3,person.club()); if (compare_logical(c,c.r(3))!=0) c.set(28,c.u32(c.r(3)));
        c.set(3,person.name(0)); c.set(5,compare_logical(c,c.r(3))!=0?c.u32(c.r(3)+0x20):missing);
        c.set(3,person.name(1)); c.set(6,compare_logical(c,c.r(3))!=0?c.u32(c.r(3)+0x20):missing);
        c.set(3,person.name(2)); c.set(3,compare_logical(c,c.r(3))!=0?c.u32(c.r(3)+0x20):missing);
        compare(c,std::int8_t(index)); if (index) compare(c,std::int8_t(index),1);
        store_winner(c,frame.at(0x90+index*24),c.r(5),c.r(6),c.r(3),person.id(),c.r(28));
        keep(c,{{25,c.r(25)+4},{29,index+1}});
    }
    p.call(0x1044cf90,0x10444c20,{award,frame.at(0x8c),1});
    p.call(0x10775170,0x10444c28,{selected});
}
void publish_competition_managers(Cpu& c,std::uint32_t date,std::uint32_t award,std::uint32_t competition) {
    Frame frame(c,0x10444c40,0x100,{.first_gpr=27}); Program p{c};
    keep(c,{{27,date},{28,c.toc(-0x7bac)},{29,c.toc(-0x7ba8)},{30,c.toc(-0x4a18)},{31,award}});
    c.set(3,c.u32(indirect(c,-0x7b5c)+competition*4));
    if (compare_logical(c,c.r(3))==0) { assertion(c,frame,0x10444c90,0x70,0x48,0x1d4); return; }
    if (compare(c,c.u8(c.r(3)+0x43),2)!=0) { assertion(c,frame,0x10444cf0,0x50,0x40,0x1db); return; }
    c.set(4,frame.at(0x3c)); c.set(5,frame.at(0x38)); p.virtual_call(c.r(3),0xa8,0x10444d44);
    p.call(0x10388b80,0x10444d5c,{indirect(c,-0x7b48),c.u32(frame.at(0x3c)),std::uint32_t(c.i16(frame.at(0x38)))});
    c.set(4,std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c,c.r(4),3)!=0) { p.call(0x10775170,0x10444d70,{c.u32(frame.at(0x3c))}); return; }
    c.put16(frame.at(0x90),c.u16(date+2)); c.set(3,c.u32(frame.at(0x3c))); c.set(8,0);
    for (unsigned index=0;compare(c,std::int8_t(index),c.r(4))<0;++index) {
        c.set(6,c.u32(c.r(3))); c.set(7,c.u32(c.r(6)+0x90)); const Person person{c,c.r(7)};
        c.set(5,person.name(0)); c.set(9,compare_logical(c,c.r(5))!=0?c.u32(c.r(5)+0x20):missing);
        c.set(5,person.name(1)); c.set(10,compare_logical(c,c.r(5))!=0?c.u32(c.r(5)+0x20):missing);
        c.set(5,person.name(2)); c.set(5,compare_logical(c,c.r(5))!=0?c.u32(c.r(5)+0x20):missing);
        compare(c,std::int8_t(index)); c.set(6,c.u32(c.r(6))); if (index) compare(c,std::int8_t(index),1);
        store_winner(c,frame.at(0x94+index*24),c.r(9),c.r(10),c.r(5),person.id(),c.r(6));
        c.set(3,c.r(3)+4); c.set(8,index+1);
    }
    p.call(0x1044cf90,0x10444e64,{award,frame.at(0x90),1});
    p.call(0x10775170,0x10444e6c,{c.u32(frame.at(0x3c))});
}
void publish_ratings(Cpu& c,std::uint32_t date,std::uint32_t award) {
    Frame frame(c,0x10444e80,0x100,{.first_gpr=25}); Program p{c};
    const auto ratings=Award{c,award}.ratings();
    keep(c,{{25,date},{26,ratings},{27,c.toc(-0x7bac)},{28,c.toc(-0x7ba8)},{29,c.toc(-0x4a18)},{30,award}});
    if (compare_logical(c,ratings)==0) { assertion(c,frame,0x10444ec4,0x68,0x40,0x24c); return; }
    const auto selected=p.call(0x10775130,0x10444f0c,{12}); c.set(31,selected);
    if (compare(c,selected)==0) { assertion(c,frame,0x10444f28,0x48,0x38,0x254); return; }
    p.call(0x104386b0,0x10444f78,{ratings,selected,3}); c.set(27,std::uint32_t(std::int16_t(c.r(3))));
    p.call(0x10439ea0,0x10444f84,{ratings});
    if (compare(c,c.r(27),3)<0) { p.call(0x10775170,0x10444f94,{selected}); return; }
    c.put16(frame.at(0x88),c.u16(date+2)); c.set(3,selected); c.set(6,0);
    for (unsigned index=0;compare(c,std::int8_t(index),3)<0;++index) {
        c.set(5,c.u32(c.r(3))); c.set(10,missing); const Person person{c,c.r(5)};
        c.set(4,person.name(0)); c.set(7,compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing);
        c.set(4,person.name(1)); c.set(8,compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing);
        c.set(4,person.name(2)); c.set(9,compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing);
        c.set(4,person.club()); if (compare_logical(c,c.r(4))!=0) c.set(10,c.u32(c.r(4)));
        compare(c,std::int8_t(index)); if (index) compare(c,std::int8_t(index),1);
        store_winner(c,frame.at(0x8c+index*24),c.r(7),c.r(8),c.r(9),person.id(),c.r(10));
        c.set(3,c.r(3)+4); c.set(6,index+1);
    }
    p.call(0x1044cf90,0x10445098,{award,frame.at(0x88),1});
    p.call(0x10775170,0x104450a0,{selected});
}
}

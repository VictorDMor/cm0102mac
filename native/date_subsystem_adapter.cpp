#include "date_subsystem_adapter.hpp"
#include "calendar_adapter.hpp"
#include "calendar.hpp"

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s,unsigned n) {return std::uint32_t(s.r<4>(n*4));}
void set(State& s,unsigned n,std::uint32_t value) {s.wr<4>(n*4,value);}
std::int32_t signed_reg(const State& s,unsigned n) {return std::bit_cast<std::int32_t>(reg(s,n));}
std::int32_t short_value(std::uint32_t value) {return cm::calendar::narrow_day(value);}
std::int32_t byte_value(std::uint32_t value) {return std::bit_cast<std::int8_t>(std::uint8_t(value));}
void compare(State& s,std::int64_t first,std::int64_t second,bool record=false) {
    s.wr<1>(2304,(record ? s.r<1>(2304)&0xf0 : 0) |
        (first<second?8:first>second?4:2) | (s.r<1>(1024)&1));
}
void record(State& s,unsigned n) {compare(s,signed_reg(s,n),0,true);}
void finish(State& s) {s.pc=std::uint32_t(s.r<4>(4128));}
void save_multiple(State& s,unsigned first,std::uint32_t address) {
    s.wr<4>(2432,address);
    for(unsigned n=first;n<32;++n) {
        s.memory.write<4>(s.r<4>(2432),reg(s,n));s.wr<4>(2432,s.r<4>(2432)+4);
    }
}
void restore_multiple(State& s,unsigned first,std::uint32_t address) {
    s.wr<4>(2432,address);
    for(unsigned n=first;n<32;++n) {
        set(s,n,s.memory.read<4>(s.r<4>(2432)));s.wr<4>(2432,s.r<4>(2432)+4);
    }
}
void frame(State& s,unsigned size) {
    const auto previous=reg(s,1);s.memory.write<4>(previous-size,previous);set(s,1,previous-size);
}
void restore_frame(State& s,unsigned size,unsigned first,bool multiple=false) {
    set(s,0,s.memory.read<4>(reg(s,1)+size+8));set(s,1,reg(s,1)+size);
    s.wr<4>(4128,reg(s,0));
    if(multiple) restore_multiple(s,first,reg(s,1)-(32-first)*4);
    else for(unsigned n=31;;--n) {
        set(s,n,s.memory.read<4>(reg(s,1)-(32-n)*4));if(n==first)break;
    }
    finish(s);
}
// Unlike callback marshalling this call preserves the directly observable ABI.
// Every child enters the same generated destination hook as a game caller,
// preserving private mode selection, real counters, nesting and observation.
void call(State& s,std::uint32_t entry,std::uint32_t link) {
    s.wr<4>(2440,s.r<4>(8));s.wr<4>(4128,link);s.pc=entry;
    run_until(s,link);
}
void leap(State& s) {
    const auto year=std::uint16_t(reg(s,3));set(s,5,year);set(s,3,0);set(s,0,year&1);
    compare(s,reg(s,0),1);
    if(year&1) {finish(s);return;}
    set(s,0,year%4);record(s,0);
    if(year%4) {finish(s);return;}
    set(s,4,(INT64_C(0x51eb851f)*year)>>32);s.wr<1>(1027,0);
    set(s,0,year%100);record(s,0);
    if(year%100) {set(s,3,1);finish(s);return;}
    set(s,0,year%400);record(s,0);set(s,3,cm::calendar::is_leap(year));finish(s);
}
void initialize_date(State& s) {
    set(s,0,40);set(s,4,1972);s.memory.write<2>(reg(s,3),reg(s,0));set(s,0,1);
    s.memory.write<2>(reg(s,3)+2,reg(s,4));s.memory.write<1>(reg(s,3)+4,reg(s,0));finish(s);
}
void construct_ordinal(State& s) {
    set(s,0,s.r<4>(4128));s.memory.write<4>(reg(s,1)-4,reg(s,31));set(s,31,reg(s,3));
    s.memory.write<4>(reg(s,1)+8,reg(s,0));frame(s,64);
    s.memory.write<2>(reg(s,3),reg(s,4));set(s,3,reg(s,5));
    s.memory.write<2>(reg(s,31)+2,reg(s,5));call(s,0x10718440,0x107156d4);
    s.memory.write<1>(reg(s,31)+4,reg(s,3));set(s,3,reg(s,31));restore_frame(s,64,31);
}
void construction_diagnostic(State& s,bool variant_b,bool month) {
    const auto first=variant_b?(month?0x10715960u:0x107158f8u):(month?0x107157b0u:0x10715748u);
    const auto text=reg(s,28), message=reg(s,27);
    const auto scratch=reg(s,1)+(month?0x48u:0x68u), tail=reg(s,1)+(month?0x38u:0x40u);
    set(s,3,text);set(s,6,scratch);set(s,7,tail);set(s,4,0);set(s,5,0);
    call(s,0x10739890,first+4);call(s,0x10768810,first+8);
    set(s,5,reg(s,3));set(s,3,message);set(s,4,text+9);set(s,6,scratch);set(s,7,tail);
    set(s,8,variant_b?(month?0x79:0x72):(month?0x3b:0x34));call(s,0x1077b720,first+0x24);
    set(s,4,reg(s,27));set(s,3,reg(s,28)+0x16);set(s,5,0);call(s,0x10769b60,first+0x34);
    set(s,0,0);set(s,month?24:30,month?0:1);s.memory.write<1>(reg(s,26),0);
}
void construct_civil(State& s,bool variant_b) {
    set(s,0,s.r<4>(4128));save_multiple(s,24,reg(s,1)-32);
    set(s,30,reg(s,4));set(s,26,s.memory.read<4>(reg(s,2)-0x7bac));
    set(s,27,s.memory.read<4>(reg(s,2)-0x7ba8));set(s,29,reg(s,3));
    set(s,28,s.memory.read<4>(reg(s,2)-0x3fa0));set(s,24,reg(s,5));set(s,25,reg(s,6));set(s,31,reg(s,7));
    s.memory.write<4>(reg(s,1)+8,reg(s,0));set(s,0,short_value(reg(s,30)));compare(s,signed_reg(s,0),1);frame(s,176);
    if(signed_reg(s,0)<1) construction_diagnostic(s,variant_b,false);
    else {compare(s,signed_reg(s,0),31);if(signed_reg(s,0)>31)construction_diagnostic(s,variant_b,false);}
    set(s,0,byte_value(reg(s,24)));record(s,0);
    if(signed_reg(s,0)<0) construction_diagnostic(s,variant_b,true);
    else {set(s,0,byte_value(reg(s,24)));compare(s,signed_reg(s,0),11);if(signed_reg(s,0)>11)construction_diagnostic(s,variant_b,true);}
    s.memory.write<2>(reg(s,29)+2,reg(s,25));set(s,3,reg(s,25));
    call(s,0x10718440,variant_b?0x107159ac:0x107157fc);
    set(s,0,byte_value(reg(s,3)));record(s,0);
    const bool is_leap=reg(s,0)!=0;set(s,3,is_leap);set(s,0,byte_value(reg(s,24)));
    s.memory.write<1>(reg(s,29)+4,reg(s,3));set(s,0,reg(s,0)*2);
    set(s,3,s.memory.read<4>(reg(s,2)-(is_leap?0x3fa4:0x3fa8)));
    set(s,0,short_value(s.memory.read<2>(std::uint32_t(reg(s,3)+reg(s,0)))));
    set(s,0,reg(s,30)+reg(s,0));s.memory.write<2>(reg(s,29),reg(s,0));
    set(s,0,short_value(reg(s,31)));compare(s,signed_reg(s,0),-1);
    if(signed_reg(s,0)!=-1) {
        set(s,3,reg(s,29));set(s,4,reg(s,31));set(s,5,reg(s,29));
        call(s,0x10715a30,variant_b?0x10715a14:0x10715864);
    }
    if(!variant_b)set(s,3,reg(s,29));restore_frame(s,176,24,true);
}
void adjust_weekday(State& s) {
    set(s,0,s.r<4>(4128));s.memory.write<4>(reg(s,1)-4,reg(s,31));set(s,31,reg(s,5));
    s.memory.write<4>(reg(s,1)-8,reg(s,30));set(s,30,reg(s,4));
    s.memory.write<4>(reg(s,1)-12,reg(s,29));set(s,29,reg(s,3));
    s.memory.write<4>(reg(s,1)+8,reg(s,0));frame(s,96);call(s,0x10715b20,0x10715a58);
    set(s,4,s.memory.read<2>(reg(s,29)+2));set(s,3,short_value(reg(s,3)));set(s,0,short_value(reg(s,30)));
    s.memory.write<2>(reg(s,31)+2,reg(s,4));compare(s,signed_reg(s,3),signed_reg(s,0));
    const bool equal=signed_reg(s,3)==signed_reg(s,0);
    set(s,0,short_value(s.memory.read<2>(reg(s,29))));s.memory.write<2>(reg(s,31),reg(s,0));
    set(s,0,s.memory.read<1>(reg(s,29)+4));s.memory.write<1>(reg(s,31)+4,reg(s,0));
    if(!equal) {
        set(s,0,reg(s,3)-reg(s,30));set(s,5,short_value(reg(s,0)));record(s,5);
        const bool negative=signed_reg(s,5)<0;
        compare(s,signed_reg(s,5),negative?-3:3);
        if(negative?signed_reg(s,5)<-3:signed_reg(s,5)>3) {
            set(s,0,reg(s,5)+(negative?7u:std::uint32_t(-7)));set(s,5,short_value(reg(s,0)));
        }
        const auto local=negative?0x40u:0x38u;
        set(s,4,reg(s,31));set(s,3,reg(s,1)+local);set(s,5,short_value(reg(s,5)));
        call(s,0x10717b20,negative?0x10715aac:0x10715ae8);
        set(s,0,s.memory.read<2>(reg(s,1)+local+2));s.memory.write<2>(reg(s,31)+2,reg(s,0));
        set(s,0,short_value(s.memory.read<2>(reg(s,1)+local)));s.memory.write<2>(reg(s,31),reg(s,0));
        set(s,0,s.memory.read<1>(reg(s,1)+local+4));s.memory.write<1>(reg(s,31)+4,reg(s,0));
    }
    restore_frame(s,96,29);
}
void shift_weeks(State& s,bool backward) {
    set(s,0,s.r<4>(4128));s.memory.write<4>(reg(s,1)-4,reg(s,31));
    set(s,31,s.memory.read<4>(reg(s,2)-(backward?0x3fb8:0x3fb0)));
    s.memory.write<4>(reg(s,1)-8,reg(s,30));if(!backward)set(s,30,reg(s,3));
    s.memory.write<4>(reg(s,1)-12,reg(s,29));if(backward)set(s,29,reg(s,3));
    s.memory.write<4>(reg(s,1)+8,reg(s,0));frame(s,80);
    const auto guard=reg(s,2)-(backward?0x35be:0x35bc);
    set(s,0,byte_value(s.memory.read<1>(guard)));record(s,0);
    if(!reg(s,0)) {
        set(s,3,40);set(s,0,1);s.memory.write<2>(reg(s,31),reg(s,3));set(s,3,1972);
        s.memory.write<2>(reg(s,31)+2,reg(s,3));s.memory.write<1>(reg(s,31)+4,reg(s,0));s.memory.write<1>(guard,reg(s,0));
    }
    set(s,3,s.memory.read<2>(reg(s,4)+2));set(s,0,std::uint8_t(reg(s,5)));
    if(!backward)set(s,0,reg(s,0)*7);set(s,backward?30:29,reg(s,31)+2);
    s.memory.write<2>(reg(s,31)+2,reg(s,3));
    if(backward) {set(s,3,reg(s,0)*7);set(s,0,short_value(s.memory.read<2>(reg(s,4))));set(s,0,reg(s,0)-reg(s,3));}
    else {set(s,3,short_value(s.memory.read<2>(reg(s,4))));set(s,0,reg(s,3)+reg(s,0));}
    s.memory.write<2>(reg(s,31),reg(s,0));
    const auto year_pointer=backward?30u:29u;
    for(;;) {
        set(s,3,s.memory.read<2>(reg(s,year_pointer)));
        if(backward) {set(s,0,reg(s,3)-1);set(s,3,std::uint16_t(reg(s,0)));}
        call(s,0x10718440,backward?0x10717d28:0x10717a80);set(s,0,byte_value(reg(s,3)));record(s,0);
        const auto length=reg(s,0)?366u:365u;
        set(s,3,short_value(s.memory.read<2>(reg(s,31))));
        if(backward) {set(s,0,short_value(reg(s,3)));record(s,0);if(signed_reg(s,0)>=0)break;set(s,0,reg(s,3)+length);}
        else {compare(s,signed_reg(s,3),length-1);if(signed_reg(s,3)<=int(length-1))break;set(s,0,reg(s,3)-length);}
        s.memory.write<2>(reg(s,31),reg(s,0));set(s,3,s.memory.read<2>(reg(s,year_pointer)));
        set(s,0,reg(s,3)+(backward?std::uint32_t(-1):1u));s.memory.write<2>(reg(s,year_pointer),reg(s,0));
    }
    set(s,3,s.memory.read<2>(reg(s,year_pointer)));call(s,0x10718440,backward?0x10717d74:0x10717acc);
    set(s,0,byte_value(reg(s,3)));record(s,0);set(s,0,reg(s,0)!=0);s.memory.write<1>(reg(s,31)+4,reg(s,0));
    const auto destination=backward?29u:30u;
    set(s,0,short_value(s.memory.read<2>(reg(s,31))));s.memory.write<2>(reg(s,destination),reg(s,0));
    set(s,0,s.memory.read<2>(reg(s,year_pointer)));s.memory.write<2>(reg(s,destination)+2,reg(s,0));
    set(s,0,s.memory.read<1>(reg(s,31)+4));s.memory.write<1>(reg(s,destination)+4,reg(s,0));restore_frame(s,80,29);
}
void interval_initialize(State& s,std::uint32_t guard) {
    set(s,3,40);set(s,0,1);s.memory.write<2>(reg(s,31),reg(s,3));set(s,3,1972);
    s.memory.write<2>(reg(s,31)+2,reg(s,3));s.memory.write<1>(reg(s,31)+4,reg(s,0));s.memory.write<1>(guard,reg(s,0));
}
bool interval_order(State& s,unsigned left_year,unsigned right_year,std::uint32_t left_day,std::uint32_t right_day,unsigned day_register,unsigned half_register) {
    compare(s,reg(s,left_year),reg(s,right_year));
    if(reg(s,left_year)>reg(s,right_year))set(s,0,1);
    else if(reg(s,left_year)<reg(s,right_year))set(s,0,0);
    else {
        set(s,day_register,short_value(s.memory.read<2>(left_day)));set(s,0,short_value(s.memory.read<2>(right_day)));
        set(s,0,reg(s,day_register)^reg(s,0));set(s,half_register,signed_reg(s,0)>>1);
        s.wr<1>(1027,signed_reg(s,0)<0&&(reg(s,0)&1));set(s,0,reg(s,0)&reg(s,day_register));
        set(s,0,reg(s,half_register)-reg(s,0));set(s,0,reg(s,0)>>31);set(s,0,byte_value(reg(s,0)));
    }
    set(s,0,byte_value(reg(s,0)));record(s,0);return reg(s,0)!=0;
}
void interval_copy(State& s) {
    set(s,0,short_value(s.memory.read<2>(reg(s,31))));s.memory.write<2>(reg(s,29),reg(s,0));
    set(s,0,s.memory.read<2>(reg(s,31)+2));s.memory.write<2>(reg(s,29)+2,reg(s,0));
    set(s,0,s.memory.read<1>(reg(s,31)+4));s.memory.write<1>(reg(s,29)+4,reg(s,0));
}
void record_interval(State& s) {
    set(s,0,s.r<4>(4128));s.memory.write<4>(reg(s,1)-4,reg(s,31));set(s,31,s.memory.read<4>(reg(s,2)-0x3fbc));
    s.memory.write<4>(reg(s,1)-8,reg(s,30));set(s,30,reg(s,4));s.memory.write<4>(reg(s,1)-12,reg(s,29));
    set(s,29,reg(s,3));s.memory.write<4>(reg(s,1)-16,reg(s,28));s.memory.write<4>(reg(s,1)+8,reg(s,0));frame(s,112);
    const auto sp=reg(s,1),guard=reg(s,2)-0x35bf;
    s.memory.write<4>(sp+0x90,reg(s,5));s.memory.write<4>(sp+0x94,reg(s,6));
    set(s,0,byte_value(s.memory.read<1>(guard)));record(s,0);if(!reg(s,0))interval_initialize(s,guard);
    set(s,3,s.memory.read<4>(sp+0x90));set(s,0,s.memory.read<4>(sp+0x94));s.memory.write<4>(sp+0x50,reg(s,3));
    set(s,3,s.memory.read<2>(reg(s,30)+2));set(s,4,s.memory.read<2>(sp+0x52));s.memory.write<4>(sp+0x54,reg(s,0));
    if(!interval_order(s,4,3,sp+0x50,reg(s,30),5,4)) {
        set(s,28,short_value(s.memory.read<2>(sp+0x90)));set(s,0,short_value(s.memory.read<2>(reg(s,30))));
        set(s,0,reg(s,0)-reg(s,28));record(s,0);
        if(signed_reg(s,0)>=0) {
            s.memory.write<2>(reg(s,31),reg(s,0));set(s,3,s.memory.read<2>(sp+0x92));
            set(s,0,s.memory.read<2>(reg(s,30)+2));set(s,0,reg(s,0)-reg(s,3));s.memory.write<2>(reg(s,31)+2,reg(s,0));
        } else {
            // This branch really calls leap with the residual r3, which may
            // be a day-derived value in the equal-year comparison.
            call(s,0x10718440,0x107180b4);set(s,0,byte_value(reg(s,3)));record(s,0);
            const auto length=reg(s,0)?366u:365u;
            set(s,0,short_value(s.memory.read<2>(reg(s,30))));set(s,3,reg(s,0)-reg(s,28));
            set(s,0,reg(s,3)+length);set(s,0,short_value(reg(s,0)));s.memory.write<2>(reg(s,31),reg(s,0));
            set(s,3,s.memory.read<2>(sp+0x92));set(s,0,s.memory.read<2>(reg(s,30)+2));
            set(s,3,reg(s,0)-reg(s,3));set(s,0,reg(s,3)-1);s.memory.write<2>(reg(s,31)+2,reg(s,0));
        }
        interval_copy(s);
    } else {
        set(s,0,s.memory.read<1>(guard));set(s,4,s.memory.read<4>(reg(s,30)));set(s,3,s.memory.read<4>(reg(s,30)+4));
        set(s,0,byte_value(reg(s,0)));record(s,0);s.memory.write<4>(sp+0x48,reg(s,4));s.memory.write<4>(sp+0x4c,reg(s,3));
        if(!reg(s,0))interval_initialize(s,guard);
        set(s,3,s.memory.read<4>(sp+0x48));set(s,0,s.memory.read<4>(sp+0x4c));s.memory.write<4>(sp+0x40,reg(s,3));
        set(s,30,s.memory.read<2>(sp+0x92));set(s,3,s.memory.read<2>(sp+0x42));s.memory.write<4>(sp+0x44,reg(s,0));
        if(!interval_order(s,3,30,sp+0x40,sp+0x90,4,3)) {
            set(s,0,short_value(s.memory.read<2>(sp+0x48)));set(s,28,short_value(s.memory.read<2>(sp+0x90)));
            set(s,3,reg(s,28)-reg(s,0));record(s,3);
            if(signed_reg(s,3)>=0) {
                set(s,0,s.memory.read<2>(sp+0x4a));s.memory.write<2>(reg(s,31),reg(s,3));
                set(s,0,reg(s,30)-reg(s,0));s.memory.write<2>(reg(s,31)+2,reg(s,0));
            } else {
                set(s,3,reg(s,30));call(s,0x10718440,0x1071802c);set(s,0,byte_value(reg(s,3)));record(s,0);
                const auto length=reg(s,0)?366u:365u;
                set(s,0,short_value(s.memory.read<2>(sp+0x48)));set(s,3,reg(s,28)-reg(s,0));set(s,0,reg(s,3)+length);
                set(s,3,short_value(reg(s,0)));set(s,0,s.memory.read<2>(sp+0x4a));s.memory.write<2>(reg(s,31),reg(s,3));
                set(s,3,reg(s,30)-reg(s,0));set(s,0,reg(s,3)-1);s.memory.write<2>(reg(s,31)+2,reg(s,0));
            }
            interval_copy(s);
        } else {
            set(s,0,s.memory.read<1>(guard));set(s,4,s.memory.read<4>(sp+0x90));set(s,3,s.memory.read<4>(sp+0x94));
            set(s,0,byte_value(reg(s,0)));record(s,0);s.memory.write<4>(sp+0x38,reg(s,4));s.memory.write<4>(sp+0x3c,reg(s,3));
            if(!reg(s,0)) {
                set(s,3,reg(s,31));call(s,0x10715880,0x10717f3c);set(s,0,1);s.memory.write<1>(guard,reg(s,0));
            }
            set(s,4,s.memory.read<4>(sp+0x38));set(s,3,sp+0x48);set(s,5,s.memory.read<4>(sp+0x3c));
            call(s,0x10331860,0x10717f54);set(s,0,byte_value(reg(s,3)));record(s,0);
            if(reg(s,0)) {
                set(s,5,s.memory.read<4>(sp+0x48));set(s,3,reg(s,29));set(s,6,s.memory.read<4>(sp+0x4c));set(s,4,sp+0x38);
                call(s,0x10717dd0,0x10717f70);
            } else {
                set(s,3,short_value(s.memory.read<2>(sp+0x38)));set(s,0,short_value(s.memory.read<2>(sp+0x48)));
                set(s,4,reg(s,0)-reg(s,3));record(s,4);
                if(signed_reg(s,4)>=0) {
                    set(s,3,s.memory.read<2>(sp+0x3a));set(s,0,s.memory.read<2>(sp+0x4a));s.memory.write<2>(reg(s,31),reg(s,4));
                    set(s,0,reg(s,0)-reg(s,3));s.memory.write<2>(reg(s,31)+2,reg(s,0));
                } else {
                    set(s,3,s.memory.read<2>(sp+0x4a));call(s,0x10718440,0x10717fa4);set(s,0,byte_value(reg(s,3)));record(s,0);
                    const auto length=reg(s,0)?366u:365u;
                    set(s,3,short_value(s.memory.read<2>(sp+0x38)));set(s,0,short_value(s.memory.read<2>(sp+0x48)));
                    set(s,3,reg(s,0)-reg(s,3));set(s,0,reg(s,3)+length);set(s,4,short_value(reg(s,0)));
                    set(s,3,s.memory.read<2>(sp+0x3a));set(s,0,s.memory.read<2>(sp+0x4a));s.memory.write<2>(reg(s,31),reg(s,4));
                    set(s,3,reg(s,0)-reg(s,3));set(s,0,reg(s,3)-1);s.memory.write<2>(reg(s,31)+2,reg(s,0));
                }
                set(s,3,reg(s,29));set(s,4,reg(s,31));call(s,0x101fe9f0,0x10717ffc);
            }
        }
    }
    restore_frame(s,112,28);
}

void shift_days(State& s,bool backward) {
    set(s,0,s.r<4>(4128));s.memory.write<4>(reg(s,1)-4,reg(s,31));
    set(s,31,s.memory.read<4>(reg(s,2)-(backward?0x3fb4:0x3fac)));
    s.memory.write<4>(reg(s,1)-8,reg(s,30));set(s,30,reg(s,3));
    s.memory.write<4>(reg(s,1)-12,reg(s,29));s.memory.write<4>(reg(s,1)-16,reg(s,28));
    s.memory.write<4>(reg(s,1)+8,reg(s,0));frame(s,80);
    const auto guard=reg(s,2)-(backward?0x35bd:0x35bb);
    set(s,0,byte_value(s.memory.read<1>(guard)));record(s,0);
    // This initialization is inline in both original bodies. There is no
    // independent initialization call to count or record at this boundary.
    if(!reg(s,0))interval_initialize(s,guard);
    set(s,0,short_value(reg(s,5)));record(s,0);
    if(signed_reg(s,0)<0) {
        set(s,3,s.memory.read<2>(reg(s,4)+2));set(s,0,0-reg(s,5));set(s,5,short_value(reg(s,0)));
        set(s,29,reg(s,31)+2);s.memory.write<2>(reg(s,31)+2,reg(s,3));set(s,28,reg(s,31)+4);
        set(s,3,reg(s,1)+0x38);set(s,0,short_value(s.memory.read<2>(reg(s,4))));s.memory.write<2>(reg(s,31),reg(s,0));
        set(s,0,s.memory.read<1>(reg(s,4)+4));set(s,4,reg(s,31));s.memory.write<1>(reg(s,31)+4,reg(s,0));
        call(s,backward?0x10717890:0x10717b20,backward?0x10717ba8:0x10717918);
        set(s,3,s.memory.read<2>(reg(s,1)+0x3a));set(s,0,s.memory.read<1>(reg(s,1)+0x3c));
        s.memory.write<2>(reg(s,29),reg(s,3));set(s,3,short_value(s.memory.read<2>(reg(s,1)+0x38)));
        s.memory.write<2>(reg(s,31),reg(s,3));s.memory.write<1>(reg(s,28),reg(s,0));
        set(s,0,short_value(s.memory.read<2>(reg(s,31))));s.memory.write<2>(reg(s,30),reg(s,0));
        set(s,0,s.memory.read<2>(reg(s,29)));s.memory.write<2>(reg(s,30)+2,reg(s,0));
        set(s,0,s.memory.read<1>(reg(s,28)));s.memory.write<1>(reg(s,30)+4,reg(s,0));
    } else {
        set(s,0,s.memory.read<2>(reg(s,4)+2));set(s,28,reg(s,31)+2);s.memory.write<2>(reg(s,31)+2,reg(s,0));
        set(s,0,short_value(s.memory.read<2>(reg(s,4))));set(s,0,backward?reg(s,0)-reg(s,5):reg(s,0)+reg(s,5));
        s.memory.write<2>(reg(s,31),reg(s,0));
        for(;;) {
            set(s,3,s.memory.read<2>(reg(s,28)));
            if(backward){set(s,0,reg(s,3)-1);set(s,3,std::uint16_t(reg(s,0)));}
            call(s,0x10718440,backward?0x10717c08:0x1071796c);set(s,0,byte_value(reg(s,3)));record(s,0);
            const auto length=reg(s,0)?366u:365u;
            set(s,3,short_value(s.memory.read<2>(reg(s,31))));
            if(backward){set(s,0,short_value(reg(s,3)));record(s,0);if(signed_reg(s,0)>=0)break;set(s,0,reg(s,3)+length);}
            else{compare(s,signed_reg(s,3),length-1);if(signed_reg(s,3)<=int(length-1))break;set(s,0,reg(s,3)-length);}
            s.memory.write<2>(reg(s,31),reg(s,0));set(s,3,s.memory.read<2>(reg(s,28)));
            set(s,0,reg(s,3)+(backward?std::uint32_t(-1):1u));s.memory.write<2>(reg(s,28),reg(s,0));
        }
        set(s,3,s.memory.read<2>(reg(s,28)));call(s,0x10718440,backward?0x10717c54:0x107179b8);
        set(s,0,byte_value(reg(s,3)));record(s,0);set(s,0,reg(s,0)!=0);s.memory.write<1>(reg(s,31)+4,reg(s,0));
        set(s,0,short_value(s.memory.read<2>(reg(s,31))));s.memory.write<2>(reg(s,30),reg(s,0));
        set(s,0,s.memory.read<2>(reg(s,28)));s.memory.write<2>(reg(s,30)+2,reg(s,0));
        set(s,0,s.memory.read<1>(reg(s,31)+4));s.memory.write<1>(reg(s,30)+4,reg(s,0));
    }
    restore_frame(s,80,28);
}
void scalar_difference(State& s) {
    set(s,0,s.r<4>(4128));s.memory.write<4>(reg(s,1)-4,reg(s,31));set(s,6,s.memory.read<4>(reg(s,2)-0x3fc0));
    s.memory.write<4>(reg(s,1)-8,reg(s,30));set(s,30,reg(s,3));s.memory.write<4>(reg(s,1)-12,reg(s,29));
    s.memory.write<4>(reg(s,1)-16,reg(s,28));s.memory.write<4>(reg(s,1)+8,reg(s,0));frame(s,80);
    const auto sp=reg(s,1),guard=reg(s,2)-0x35c0;
    s.memory.write<4>(sp+0x6c,reg(s,4));s.memory.write<4>(sp+0x70,reg(s,5));
    set(s,0,byte_value(s.memory.read<1>(guard)));record(s,0);
    if(!reg(s,0)) {
        set(s,3,40);set(s,0,1);s.memory.write<2>(reg(s,6),reg(s,3));set(s,3,1972);
        s.memory.write<2>(reg(s,6)+2,reg(s,3));s.memory.write<1>(reg(s,6)+4,reg(s,0));s.memory.write<1>(guard,reg(s,0));
    }
    set(s,3,s.memory.read<4>(sp+0x6c));set(s,31,0);set(s,0,s.memory.read<4>(sp+0x70));s.memory.write<4>(sp+0x38,reg(s,3));
    set(s,5,s.memory.read<2>(reg(s,30)+2));set(s,3,s.memory.read<2>(sp+0x3a));s.memory.write<4>(sp+0x3c,reg(s,0));
    const bool later=interval_order(s,3,5,sp+0x38,reg(s,30),4,3);
    const auto terminal_register=later?28u:29u,year_pointer=later?29u:28u;
    set(s,terminal_register,s.memory.read<2>(sp+0x6e));compare(s,reg(s,5),reg(s,terminal_register));
    if(reg(s,5)!=reg(s,terminal_register)) {
        s.memory.write<2>(reg(s,6)+2,reg(s,5));set(s,year_pointer,reg(s,6)+2);
        set(s,0,short_value(s.memory.read<2>(reg(s,30))));s.memory.write<2>(reg(s,6),reg(s,0));
        set(s,0,s.memory.read<1>(reg(s,30)+4));s.memory.write<1>(reg(s,6)+4,reg(s,0));
        for(;;) {
            set(s,3,s.memory.read<2>(reg(s,year_pointer)));compare(s,reg(s,3),reg(s,terminal_register));
            if(later?reg(s,3)>=reg(s,terminal_register):reg(s,3)<=reg(s,terminal_register))break;
            set(s,0,reg(s,3)-1);set(s,3,std::uint16_t(reg(s,0)));call(s,0x10718440,later?0x1071821c:0x10718290);
            set(s,0,byte_value(reg(s,3)));record(s,0);const bool leap_year=reg(s,0)!=0;
            // Ascending years intentionally classify current-1, as the Mac
            // original does. This preserves the separately deferred quirk.
            if(later) {
                set(s,31,reg(s,31)-(leap_year?366u:365u));set(s,3,s.memory.read<2>(reg(s,year_pointer)));set(s,0,reg(s,3)+1);
            } else {
                set(s,0,reg(s,31)+(leap_year?366u:365u));set(s,3,s.memory.read<2>(reg(s,year_pointer)));
                set(s,31,reg(s,0));set(s,0,reg(s,3)-1);
            }
            s.memory.write<2>(reg(s,year_pointer),reg(s,0));
        }
    }
    set(s,3,short_value(s.memory.read<2>(sp+0x6c)));set(s,0,short_value(s.memory.read<2>(reg(s,30))));
    set(s,0,reg(s,0)-reg(s,3));set(s,31,reg(s,31)+reg(s,0));set(s,3,reg(s,31));restore_frame(s,80,28);
}

void month_parameter(State& s) {
    set(s,0,byte_value(reg(s,3)));compare(s,signed_reg(s,0),9);
    if(signed_reg(s,0)==9) {
        set(s,0,short_value(reg(s,4)));compare(s,signed_reg(s,0),7);
        if(signed_reg(s,0)<7) {compare(s,signed_reg(s,0),5);set(s,3,signed_reg(s,0)<5);}
        else set(s,3,1);
    } else {
        s.wr<1>(1027,reg(s,0)<=3);set(s,0,3-reg(s,0));
        set(s,0,std::countl_zero(reg(s,0)));set(s,0,reg(s,0)>>5);set(s,3,byte_value(reg(s,0)));
    }
    finish(s);
}
void phase_text(State& s,bool full) {
    set(s,0,s.r<4>(4128));s.memory.write<4>(reg(s,1)-4,reg(s,31));
    set(s,31,s.memory.read<4>(reg(s,2)-(full?0x3fc8:0x3fc4)));
    s.memory.write<4>(reg(s,1)+8,reg(s,0));set(s,0,std::uint8_t(reg(s,3)));compare(s,reg(s,0),1);
    set(s,5,s.memory.read<4>(reg(s,2)-0x3fa0));frame(s,64);
    const auto phase=reg(s,0);
    if(phase!=1) {
        if(phase<1)compare(s,reg(s,0),0);else compare(s,reg(s,0),3);
    }
    if(phase<3) {
        if(full) {
            set(s,0,byte_value(reg(s,4)));record(s,0);
            constexpr std::array<unsigned,3> nonzero{0x77a,0x78a,0x79e},zero{0x782,0x794,0x7a6};
            constexpr std::array<std::uint32_t,3> nonzero_links{0x107185c4,0x107185ec,0x10718614},zero_links{0x107185d4,0x107185fc,0x10718624};
            const bool mode=reg(s,0)!=0;set(s,3,reg(s,31));set(s,4,reg(s,5)+(mode?nonzero[phase]:zero[phase]));
            call(s,0x10727890,mode?nonzero_links[phase]:zero_links[phase]);
        } else {
            constexpr std::array<unsigned,3> offsets{0x725,0x741,0x75e};
            constexpr std::array<std::uint32_t,3> links{0x10718520,0x10718534,0x10718548};
            set(s,3,reg(s,31));set(s,4,reg(s,5)+offsets[phase]);set(s,5,reg(s,5)+0x740);call(s,0x10727890,links[phase]);
        }
    } else {
        set(s,3,reg(s,31));set(s,4,reg(s,5)+0x740);call(s,0x1077dbf0,full?0x10718634:0x10718558);
    }
    set(s,3,reg(s,31));restore_frame(s,64,31);
}
void classify_phase(State& s) {
    set(s,0,s.r<4>(4128));s.memory.write<4>(reg(s,1)-4,reg(s,31));set(s,31,reg(s,3));
    s.memory.write<4>(reg(s,1)+8,reg(s,0));frame(s,64);
    call(s,0x10727880,0x10718668);set(s,0,byte_value(reg(s,3)));compare(s,signed_reg(s,0),9);
    bool first=reg(s,0)==9;
    if(!first) {
        call(s,0x10727880,0x10718678);set(s,0,byte_value(reg(s,3)));compare(s,signed_reg(s,0),3);first=reg(s,0)==3;
    }
    if(first)set(s,3,1);
    else {
        call(s,0x10727880,0x10718690);set(s,0,byte_value(reg(s,3)));compare(s,signed_reg(s,0),5);
        if(reg(s,0)==5)set(s,3,2);
        else {
            call(s,0x10727880,0x107186a8);set(s,0,byte_value(reg(s,3)));compare(s,signed_reg(s,0),4);
            if(reg(s,0)==4) {
                set(s,0,std::uint8_t(reg(s,31)));compare(s,reg(s,0),2);
                if(reg(s,0)==2)set(s,3,3);
                else if(reg(s,0)<2) {compare(s,reg(s,0),0);set(s,3,1);}
                else set(s,3,0);
            } else set(s,3,0);
        }
    }
    restore_frame(s,64,31);
}
struct NameBoundary {
    std::uint32_t scratch_offset,dispatch_offset,first_case,fallback_link;
    unsigned count;
    bool month;
    std::span<const unsigned> template_offsets;
};
constexpr std::array<unsigned,7> weekday_a_offsets{0x7ae,0x7b2,0x7b6,0x7ba,0x7be,0x7c2,0x7c6};
constexpr std::array<unsigned,7> weekday_b_offsets{0x7ca,0x7d1,0x7d8,0x7e0,0x7ea,0x7f3,0x7fa};
constexpr std::array<unsigned,12> month_a_offsets{0x803,0x807,0x80b,0x830,0x834,0x838,0x83c,0x840,0x844,0x848,0x84c,0x850};
constexpr std::array<unsigned,12> month_b_offsets{0x878,0x880,0x889,0x88f,0x895,0x8ba,0x8bf,0x8c4,0x8cb,0x8d5,0x8dd,0x8e6};
constexpr std::array<NameBoundary,4> name_boundaries{{
    {0x3fcc,0x3fd0,0x10718738,0x107187b4,7,false,weekday_a_offsets},
    {0x3fd4,0x3fd8,0x10718808,0x10718884,7,false,weekday_b_offsets},
    {0x3fdc,0x3fe0,0x107188d8,0x107189ac,12,true,month_a_offsets},
    {0x3fe4,0x3fe8,0x10718a08,0x10718ad8,12,true,month_b_offsets}
}};
void date_name(State& s,const NameBoundary& boundary) {
    set(s,0,s.r<4>(4128));s.memory.write<4>(reg(s,1)-4,reg(s,31));
    set(s,31,s.memory.read<4>(reg(s,2)-boundary.scratch_offset));s.memory.write<4>(reg(s,1)+8,reg(s,0));
    set(s,0,boundary.month?byte_value(reg(s,3)):short_value(reg(s,3)));
    compare(s,reg(s,0),boundary.count-1);set(s,boundary.month?5:4,s.memory.read<4>(reg(s,2)-0x3fa0));frame(s,64);
    if(reg(s,0)>=boundary.count) {
        set(s,3,reg(s,31));set(s,4,reg(s,boundary.month?5:4)+0x740);call(s,0x1077dbf0,boundary.fallback_link);
    } else {
        set(s,3,s.memory.read<4>(reg(s,2)-boundary.dispatch_offset));set(s,0,reg(s,0)*4);
        set(s,3,s.memory.read<4>(std::uint32_t(reg(s,3)+reg(s,0))));s.wr<4>(4132,reg(s,3));
        // The mutable dispatch table selects a semantic case, rather than
        // freezing the caller's index or entering a generated Date body.
        const auto target=reg(s,3);
        bool found=false;
        // Aliased mutable tables can also select a case from another audited
        // name body. Those case blocks share this frame but use their own
        // template-base register, call link and identical epilogue contract.
        for(const auto& selected_boundary:name_boundaries) {
            if(target==selected_boundary.fallback_link){found=true;break;}
            if(target==selected_boundary.fallback_link-12) {
                set(s,3,reg(s,31));set(s,4,reg(s,selected_boundary.month?5:4)+0x740);
                call(s,0x1077dbf0,selected_boundary.fallback_link);found=true;break;
            }
            auto address=selected_boundary.first_case;
            for(unsigned selected=0;selected<selected_boundary.count;++selected) {
                const bool extra=(selected_boundary.scratch_offset==0x3fdc&&(selected==2||selected==11)) ||
                                 (selected_boundary.scratch_offset==0x3fe4&&selected==4);
                if(target==address) {
                    set(s,3,reg(s,31));set(s,4,reg(s,selected_boundary.month?5:4)+selected_boundary.template_offsets[selected]);
                    if(extra)set(s,5,reg(s,5)+0x740);
                    call(s,0x10727890,address+(extra?16u:12u));found=true;break;
                }
                address+=extra?20u:16u;
            }
            if(found)break;
        }
        if(!found)throw std::runtime_error("Unrecognized semantic Date name dispatch at "+hex_address(target));
    }
    set(s,3,reg(s,31));restore_frame(s,64,31);
}
struct FormatBoundary {
    std::uint32_t entry,diagnostic_first;
    unsigned frame_size,saved_first,buffer,date,error_flag,error_buffer;
    unsigned month,flag,table,cursor,counter,group,threshold,minimum,line;
};
constexpr std::array<FormatBoundary,9> formats{{
    {0x10715bc0,0x10715bfc,160,26,29,28,26,27,30,0,5,3,8,7,4,9,0xf8},
    {0x10715e00,0x10715e40,176,25,27,26,25,29,30,0,5,3,7,6,4,9,0x132},
    {0x10716130,0x1071616c,176,25,27,30,28,26,29,8,5,3,9,7,4,18,0x181},
    {0x107164f0,0x1071652c,176,25,27,30,28,26,29,8,5,3,9,7,4,18,0x1bf},
    {0x107168a0,0x107168dc,160,26,30,29,28,27,26,0,4,4,7,5,3,18,0x1fb},
    {0x10716b00,0x10716b38,160,26,28,3,29,27,30,9,7,4,10,8,5,18,0x234},
    {0x10716eb0,0x10716eec,176,25,27,30,28,26,29,8,5,3,9,7,4,50,0x270},
    {0x10717270,0x107172a8,160,26,28,3,29,27,30,9,7,4,10,8,5,50,0x2af},
    {0x10717620,0x1071765c,160,26,29,28,26,27,30,0,5,3,8,7,4,50,0x2ed}
}};
void format_diagnostic(State& s,const FormatBoundary& b,bool capacity) {
    const auto first=b.diagnostic_first+(capacity?0x64u:0);
    const auto text=reg(s,31),scratch=reg(s,1)+(capacity?0x48u:0x68u),tail=reg(s,1)+(capacity?0x38u:0x40u);
    set(s,3,text);set(s,6,scratch);set(s,7,tail);set(s,4,0);set(s,5,0);
    call(s,0x10739890,first+4);call(s,0x10768810,first+8);
    set(s,5,reg(s,3));set(s,3,reg(s,b.error_buffer));set(s,4,reg(s,31)+9);set(s,6,scratch);set(s,7,tail);
    set(s,8,b.line+(capacity?7:0));call(s,0x1077b720,first+0x24);
    set(s,4,reg(s,b.error_buffer));set(s,3,reg(s,31)+0x16);set(s,5,0);call(s,0x10769b60,first+0x34);
    set(s,0,0);set(s,3,0);s.memory.write<1>(reg(s,b.error_flag),0);
}
bool begin_format(State& s,const FormatBoundary& b) {
    set(s,0,s.r<4>(4128));save_multiple(s,b.saved_first,reg(s,1)-(32-b.saved_first)*4);
    set(s,b.buffer,reg(s,4));record(s,b.buffer);
    set(s,b.error_flag,s.memory.read<4>(reg(s,2)-0x7bac));set(s,b.error_buffer,s.memory.read<4>(reg(s,2)-0x7ba8));
    if(b.date!=3)set(s,b.date,reg(s,3));set(s,31,s.memory.read<4>(reg(s,2)-0x3fa0));
    if(b.entry==0x10715e00)set(s,28,reg(s,6));
    s.memory.write<4>(reg(s,1)+8,reg(s,0));frame(s,b.frame_size);
    if(!reg(s,b.buffer)) {format_diagnostic(s,b,false);return false;}
    set(s,0,short_value(reg(s,5)));compare(s,signed_reg(s,0),b.minimum);
    if(signed_reg(s,0)<int(b.minimum)) {format_diagnostic(s,b,true);return false;}
    return true;
}
void scan_format_month(State& s,const FormatBoundary& b) {
    set(s,b.flag,s.memory.read<1>(reg(s,b.date)+4));set(s,0,byte_value(reg(s,b.flag)));record(s,0);
    set(s,b.table,s.memory.read<4>(reg(s,2)-(reg(s,0)?0x3fa4:0x3fa8)));
    set(s,0,2);if(b.cursor!=b.table)set(s,b.cursor,reg(s,b.table));
    set(s,b.counter,0);set(s,b.group,0);s.wr<4>(4132,2);
    // Ascending recognition deliberately retains the incoming month register
    // when no threshold is less than day. Each probe reloads the signed day.
    for(unsigned group=0;group<2;++group) {
        for(unsigned index=0;index<6;++index) {
            set(s,b.threshold,short_value(s.memory.read<2>(reg(s,b.cursor)+index*2)));
            if(index)set(s,b.counter,reg(s,b.counter)+1);
            set(s,0,short_value(s.memory.read<2>(reg(s,b.date))));compare(s,signed_reg(s,b.threshold),signed_reg(s,0));
            if(cm::calendar::recognizes_month(short_value(reg(s,0)),short_value(reg(s,b.threshold))))
                set(s,b.month,byte_value(reg(s,b.counter)));
        }
        set(s,b.group,reg(s,b.group)+10);set(s,b.cursor,reg(s,b.cursor)+12);set(s,b.counter,reg(s,b.counter)+1);
        s.wr<4>(4132,s.r<4>(4132)-1);
    }
}
bool format_components(State& s,const FormatBoundary& b) {
    if(b.entry==0x107168a0) {
        set(s,0,std::uint8_t(reg(s,6)));compare(s,reg(s,0),1);if(reg(s,0)>1)compare(s,reg(s,0),2);return reg(s,0)<=2;
    }
    unsigned phase_reg=0,day_reg=0;
    if(b.entry==0x10716130||b.entry==0x107164f0||b.entry==0x10716eb0) {
        set(s,0,byte_value(reg(s,b.month)));set(s,3,std::uint8_t(reg(s,6)));phase_reg=3;day_reg=28;
        set(s,4,reg(s,0)*2);set(s,0,short_value(s.memory.read<2>(reg(s,b.date))));
        set(s,4,short_value(s.memory.read<2>(std::uint32_t(reg(s,5)+reg(s,4)))));compare(s,reg(s,3),1);
        set(s,4,reg(s,0)-reg(s,4));set(s,28,short_value(reg(s,4)));
    } else if(b.date==3) {
        set(s,0,byte_value(reg(s,b.month)));set(s,4,std::uint8_t(reg(s,6)));phase_reg=4;day_reg=29;
        set(s,5,reg(s,0)*2);set(s,0,short_value(s.memory.read<2>(reg(s,3))));
        set(s,5,short_value(s.memory.read<2>(std::uint32_t(reg(s,7)+reg(s,5)))));compare(s,reg(s,4),1);
        set(s,5,reg(s,0)-reg(s,5));set(s,29,short_value(reg(s,5)));
    } else {
        const auto month_reg=b.entry==0x10715bc0?8u:3u;
        set(s,month_reg,byte_value(reg(s,b.month)));set(s,0,std::uint8_t(reg(s,b.entry==0x10715e00?28:6)));
        phase_reg=0;day_reg=b.entry==0x10715e00?29u:26u;
        set(s,4,reg(s,month_reg)*2);set(s,3,short_value(s.memory.read<2>(reg(s,b.date))));
        set(s,4,short_value(s.memory.read<2>(std::uint32_t(reg(s,5)+reg(s,4)))));compare(s,reg(s,0),1);
        set(s,3,reg(s,3)-reg(s,4));set(s,day_reg,short_value(reg(s,3)));
    }
    (void)day_reg;
    const auto phase=reg(s,phase_reg);if(phase>1)compare(s,phase,2);return phase<=2;
}
void format_numeric(State& s,const FormatBoundary& b,bool phase) {
    if(b.entry==0x10715bc0) {
        if(phase) {
            set(s,3,reg(s,6));call(s,0x107184d0,0x10715da0);set(s,6,byte_value(reg(s,30)));
            set(s,7,s.memory.read<2>(reg(s,28)+2));set(s,8,reg(s,3));set(s,3,reg(s,29));set(s,5,reg(s,26));
            set(s,4,reg(s,31)+0x1c);set(s,6,reg(s,6)+1);call(s,0x10727890,0x10715dc0);
        } else {
            set(s,7,s.memory.read<2>(reg(s,28)+2));set(s,3,reg(s,29));set(s,5,reg(s,26));
            set(s,4,reg(s,31)+0x83);set(s,6,reg(s,8)+1);call(s,0x10727890,0x10715ddc);
        }
        return;
    }
    call(s,0x10727880,phase?0x10715fe4:0x1071608c);set(s,0,byte_value(reg(s,3)));compare(s,signed_reg(s,0),10);
    const bool language_ten=reg(s,0)==10;
    if(phase) {set(s,3,reg(s,28));call(s,0x107184d0,language_ten?0x10715ff8:0x10716044);}
    const auto year_reg=phase?9u:8u;
    set(s,phase?4:3,0x51ec0000);set(s,year_reg,s.memory.read<2>(reg(s,26)+2));
    set(s,0,0x51eb851f);set(s,language_ten?5:6,byte_value(reg(s,30)));
    set(s,0,(INT64_C(0x51eb851f)*reg(s,year_reg))>>32);
    if(phase)set(s,8,reg(s,3));
    if(language_ten) {
        set(s,6,reg(s,5)+1);set(s,3,reg(s,27));set(s,7,reg(s,29));set(s,4,reg(s,31)+(phase?0xcf:0x142));
    } else {
        set(s,3,reg(s,27));set(s,5,reg(s,29));set(s,4,reg(s,31)+(phase?0xe1:0x151));set(s,6,reg(s,6)+1);
    }
    set(s,0,std::uint32_t(signed_reg(s,0)>>5));s.wr<1>(1027,0);
    set(s,language_ten?5:7,signed_reg(s,0)<0);set(s,0,reg(s,0)+reg(s,language_ten?5:7));set(s,0,reg(s,0)*100);
    set(s,language_ten?5:7,reg(s,year_reg)-reg(s,0));
    call(s,language_ten?0x1077b720:0x10727890,
        phase?(language_ten?0x10716038:0x10716084):(language_ten?0x107160d4:0x10716114));
}
void format_month_text(State& s,const FormatBoundary& b,bool phase) {
    const bool full=b.entry==0x10717620;
    if(phase) {
        set(s,3,reg(s,6));if(full)set(s,4,1);call(s,full?0x10718570:0x107184d0,full?0x10717804:0x10716a68);set(s,27,reg(s,3));
    }
    set(s,3,reg(s,b.month));call(s,full?0x107189d0:0x107188a0,
        full?(phase?0x10717810:0x10717848):(phase?0x10716a74:0x10716ab4));set(s,full?30:28,reg(s,3));
    if(!full) {
        set(s,3,reg(s,29));call(s,0x107182f0,phase?0x10716a80:0x10716ac0);set(s,3,short_value(reg(s,3)));
    } else set(s,3,reg(s,26));
    set(s,4,1);call(s,0x107467a0,full?(phase?0x10717820:0x10717858):(phase?0x10716a8c:0x10716acc));
    set(s,7,s.memory.read<2>(reg(s,b.date)+2));set(s,5,reg(s,3));set(s,3,reg(s,b.buffer));set(s,6,reg(s,full?30:28));
    if(phase)set(s,8,reg(s,27));
    set(s,4,reg(s,31)+(full?(phase?0x665:0x6d6):(phase?0x34b:0x3b0)));
    call(s,0x10727890,full?(phase?0x1071783c:0x10717870):(phase?0x10716aa8:0x10716ae4));
}
// The formatter's weekday expression uses its early flag snapshot and its
// final day reload. The year is read at this boundary, so overlapping output
// and mutable calendar data retain the original observation order.
void format_weekday(State& s,const FormatBoundary& b,bool phase) {
    const auto day=reg(s,0);
    const auto flag=byte_value(reg(s,b.flag));
    const auto year=s.memory.read<2>(reg(s,b.date)+2);
    set(s,phase||b.date==3?5:30,year);set(s,3,0x51ec0000);set(s,4,0x51eb851f);
    set(s,3,flag);record(s,3);
    const auto elapsed=std::int32_t(year)-1600;
    const auto scaled=std::int32_t((INT64_C(0x51eb851f)*elapsed)>>32);
    const auto century_floor=scaled>>5,quadricentury_floor=scaled>>7;
    const auto quarter_floor=elapsed>>2;
    const auto quarter_carry=elapsed<0 && (std::uint32_t(elapsed)&3)!=0;
    const auto century=century_floor+int(century_floor<0);
    const auto quadricentury=quadricentury_floor+int(quadricentury_floor<0);
    const auto quarter=quarter_floor+int(quarter_carry);
    const auto total=std::int32_t(std::uint32_t(365*elapsed+quarter-century+quadricentury)+day-unsigned(flag!=0));
    set(s,phase?7:6,quadricentury_floor);set(s,4,quadricentury);
    set(s,5,century_floor<0);set(s,phase?8:7,quadricentury_floor<0);set(s,phase?9:8,total);
    const auto scaled_week=std::int32_t((INT64_C(-1840700269)*total)>>32)+total;
    const auto week_floor=scaled_week>>2;
    s.wr<1>(1027,scaled_week<0 && (std::uint32_t(scaled_week)&3)!=0);
    set(s,3,week_floor<0);
    set(s,0,total-(week_floor+int(week_floor<0))*7);
    const auto destination=b.date==3?26u:(!phase&&b.entry==0x10716130?3u:25u);
    set(s,destination,short_value(reg(s,0)));record(s,destination);
    if(signed_reg(s,destination)>0)set(s,destination,reg(s,destination)-1);
    else {
        set(s,0,short_value(reg(s,destination)));record(s,0);
        if(!reg(s,0))set(s,destination,6);
    }
}
void format_numeric_weekday(State& s,bool phase) {
    if(phase) {
        set(s,3,reg(s,6));call(s,0x107184d0,0x107163a4);set(s,26,reg(s,3));
        set(s,3,reg(s,25));call(s,0x107187d0,0x107163b0);
        set(s,4,0x51ec0000);set(s,9,s.memory.read<2>(reg(s,30)+2));set(s,0,0x51eb851f);
        set(s,8,byte_value(reg(s,29)));set(s,0,(INT64_C(0x51eb851f)*reg(s,9))>>32);
        set(s,6,reg(s,3));set(s,3,reg(s,27));set(s,7,reg(s,28));set(s,10,reg(s,26));
        set(s,4,reg(s,31)+0x197);set(s,0,signed_reg(s,0)>>5);s.wr<1>(1027,0);
        set(s,8,reg(s,8)+1);set(s,5,signed_reg(s,0)<0);set(s,0,reg(s,0)+reg(s,5));
        set(s,0,reg(s,0)*100);set(s,5,100);set(s,9,reg(s,9)-reg(s,0));call(s,0x10727890,0x107163f8);
    } else {
        call(s,0x107187d0,0x10716494);set(s,4,0x51ec0000);set(s,5,byte_value(reg(s,29)));set(s,0,0x51eb851f);
        set(s,6,reg(s,3));set(s,0,(INT64_C(0x51eb851f)*reg(s,30))>>32);set(s,8,reg(s,5)+1);
        set(s,3,reg(s,27));set(s,7,reg(s,28));set(s,4,reg(s,31)+0x213);set(s,5,100);
        set(s,0,signed_reg(s,0)>>5);s.wr<1>(1027,0);set(s,9,signed_reg(s,0)<0);set(s,0,reg(s,0)+reg(s,9));
        set(s,0,reg(s,0)*100);set(s,9,reg(s,30)-reg(s,0));call(s,0x10727890,0x107164d4);
    }
}
struct WeekdayTextBoundary {
    std::uint32_t entry,phase_link,month_link,ordinal_link,weekday_link,format_link;
    unsigned phase_template,plain_template;
    bool full,year;
};
constexpr std::array<WeekdayTextBoundary,4> weekday_texts{{
    {0x107164f0,0x10716764,0x10716770,0x10716780,0x1071678c,0x107167ac,0x274,0x2ed,false,true},
    {0x10716b00,0x10716d74,0x10716d80,0x10716d90,0x10716d9c,0x10716db8,0x3fa,0x45d,false,false},
    {0x10716eb0,0x10717128,0x10717134,0x10717144,0x10717150,0x10717174,0x4a5,0x531,true,true},
    {0x10717270,0x107174e8,0x107174f4,0x10717504,0x10717510,0x10717530,0x59b,0x611,true,false}
}};
void format_weekday_text(State& s,const FormatBoundary& b,bool phase,const WeekdayTextBoundary& text) {
    const unsigned phase_register=text.year?26:27,month_register=text.year?29:30,day_register=text.year?28:29;
    const auto delta=text.entry==0x107164f0?0xdcu:text.entry==0x10716b00?0xd8u:text.entry==0x10716eb0?0xe0u:0xdcu;
    if(phase) {
        set(s,3,reg(s,6));if(text.full)set(s,4,1);call(s,text.full?0x10718570:0x107184d0,text.phase_link);
        set(s,phase_register,reg(s,3));
    }
    set(s,3,reg(s,month_register));call(s,text.full?0x107189d0:0x107188a0,text.month_link+(phase?0:delta));
    set(s,month_register,reg(s,3));set(s,3,reg(s,day_register));set(s,4,1);
    call(s,0x107467a0,text.ordinal_link+(phase?0:delta));set(s,day_register,reg(s,3));
    set(s,3,reg(s,text.year?25:26));call(s,text.full?0x107187d0:0x10718700,text.weekday_link+(phase?0:delta));
    if(phase&&text.year)set(s,text.full?9:8,s.memory.read<2>(reg(s,b.date)+2));
    set(s,text.full?6:5,reg(s,3));set(s,3,reg(s,b.buffer));set(s,text.full?7:6,reg(s,day_register));
    set(s,text.full?8:7,reg(s,month_register));
    if(text.year&&!phase)set(s,text.full?9:8,reg(s,30));
    if(phase)set(s,text.full?(text.year?10:9):(text.year?9:8),reg(s,phase_register));
    set(s,4,reg(s,31)+(phase?text.phase_template:text.plain_template));if(text.full)set(s,5,100);
    call(s,0x10727890,text.format_link+(phase?0:delta-4));
}

void format_date(State& s,const FormatBoundary& b) {
    if(begin_format(s,b)) {
        scan_format_month(s,b);const bool phase=format_components(s,b);
        if(b.entry==0x10715bc0||b.entry==0x10715e00)format_numeric(s,b,phase);
        else if(b.entry==0x107168a0||b.entry==0x10717620)format_month_text(s,b,phase);
        else {
            format_weekday(s,b,phase);
            if(b.entry==0x10716130)format_numeric_weekday(s,phase);
            else for(const auto& text:weekday_texts)if(text.entry==b.entry) {format_weekday_text(s,b,phase,text);break;}
        }
        set(s,3,1);
    }
    restore_frame(s,b.frame_size,b.saved_first,true);
}
}
bool execute_semantic_calendar_boundary(State& s,std::uint32_t entry) {
    if(entry==0x10717890||entry==0x10717b20){shift_days(s,entry==0x10717b20);return true;}
    if(entry==0x10718130){scalar_difference(s);return true;}return false;
}
bool execute_semantic_date(State& s,std::uint32_t entry) {
    for(const auto& b:formats)if(b.entry==entry) {format_date(s,b);return true;}
    switch(entry) {
    case 0x107156b0: construct_ordinal(s);break;
    case 0x107156f0: construct_civil(s,false);break;
    case 0x10715880: initialize_date(s);break;
    case 0x107158a0: construct_civil(s,true);break;
    case 0x10715a30: adjust_weekday(s);break;
    case 0x10717a10: shift_weeks(s,false);break;
    case 0x10717cb0: shift_weeks(s,true);break;
    case 0x10717dd0: record_interval(s);break;
    case 0x10718440: leap(s);break;
    case 0x10718af0: month_parameter(s);break;
    case 0x107184d0: phase_text(s,false);break;
    case 0x10718570: phase_text(s,true);break;
    case 0x10718650: classify_phase(s);break;
    case 0x10718700: case 0x107187d0: case 0x107188a0: case 0x107189d0:
        for(const auto& boundary:name_boundaries)if(boundary.first_case-0x38==entry){date_name(s,boundary);break;}break;
    default:return false;
    }
    return true;
}
}

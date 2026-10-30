#include "fifa_rankings_detail.hpp"

namespace cm::aot::fifa_rankings {
using namespace detail;
std::uint32_t ranked(Cpu& c,unsigned manager,unsigned count) {
    Frame frame(c,0x10487020,0x80,{.first_gpr=24}); Program p{c}; Manager m{c,manager};
    keep(c,{{24,0},{26,c.toc(-0x7ba8)},{27,c.toc(-0x47b4)},
            {28,c.toc(-0x7b04)},{29,0},{30,manager},{31,count}});
    c.set(25,p.call(0x10775130,0x10487058,{c.u32(c.r(28))*4}));
    if (compare(c,c.r(25))==0) {
        assertion(c,frame,0x10487060,0x40,0x38,0x1e3);
        c.set(4,c.toc(-0x7bac));c.put8(c.r(4),0);return result(c,0);
    }
    const auto countries=c.u32(c.toc(-0x7b98));
    leave(c,{{3,countries},{4,0},{5,0}});
    unsigned selected=0;
    for (unsigned index=0;compare(c,index,c.i32(c.r(28)))<0;++index) {
        if (compare_logical(c,c.u32(countries+index*0x128+0x74))!=0) {
            c.put32(c.r(25)+selected*4,m.at(index).address);
            ++selected;keep(c,{{24,selected},{29,selected*4}});
        }
        leave(c,{{3,countries+(index+1)*0x128},{4,(index+1)*Ranking::size},{5,index+1}});
    }
    p.call(0x10768300,0x10487120,{c.r(25),selected,4,c.toc(-0x6180)});
    c.put32(count,c.r(24));return result(c,c.r(25));
}
void assign_positions(Cpu& c,unsigned manager) {
    Frame frame(c,0x10487c30,0x40,{}); Program p{c};
    enter(c,0x10487c44,{manager,frame.at(0x38)}); c.set(3,ranked(c,manager,frame.at(0x38)));
    const auto array=c.r(3);leave(c,{{4,array},{7,0}});
    for (unsigned index=0;compare(c,index,c.i32(frame.at(0x38)))<0;++index) {
        const auto row=c.u32(array+index*4);
        c.set(6,row);c.set(5,extended(c.i16(row+16)));c.put16(row+18,c.r(5));
        c.set(5,c.u32(array+index*4));c.put16(c.r(5)+16,index+1);
        leave(c,{{4,array+(index+1)*4},{7,index+1}});
    }
    p.call(0x10775170,0x10487c80);
}
void initialize(Cpu& c,unsigned manager) {
    Frame frame(c,0x10487970,0x80,{.first_gpr=26}); Program p{c};Fpu fp(c);Manager m{c,manager};
    keep(c,{{26,c.toc(-0x7ba8)},{27,c.toc(-0x47b4)},{28,manager},
            {29,c.toc(-0x7b98)},{30,c.toc(-0x47ac)},{31,c.toc(-0x7b04)}});
    c.put32(manager,p.call(0x10775200,0x104879a4,{c.u32(c.r(31)),Ranking::size}));
    if (compare_logical(c,m.records())==0) {
        assertion(c,frame,0x104879b4,0x40,0x38,0x2da);c.set(3,c.toc(-0x7bac));c.put8(c.r(3),0);return;
    }
    c.set(26,0);leave(c,{{10,0}});
    for (unsigned index=0;compare(c,index,c.i32(c.r(31)))<0;++index) {
        const auto row=m.at(index).address;
        c.put32(row,index);
        c.set_f(3,c.f64(c.r(30)+0x58));c.set_f(1,c.f64(c.r(30)+0x60));c.set_f(0,c.f64(c.r(30)+0x48));
        c.set_f(2,c.f64(c.u32(c.r(29))+index*0x128));
        c.set_f(2,fp.fmul(c.f(3),c.f(2)));c.set_f(1,fp.fdiv(c.f(2),c.f(1)));c.set_f(1,fp.frsp(c.f(1)));
        c.set_f(0,fp.fmul(c.f(0),c.f(1)));c.put64(frame.at(0x60),fp.fctiwz(0,c.f(0)));
        const auto points=c.u32(frame.at(0x64));
        for (unsigned year=0;year<6;++year)c.put32(row+20+year*4,points);
        c.put_f32(row+12,float(c.f(1)));
        c.set(26,index+1);
        leave(c,{{3,m.records()},{4,m.records()},{5,m.records()},{6,m.records()},{7,m.records()},
                 {8,m.records()},{9,m.records()},{10,(index+1)*Ranking::size}});
    }
    enter(c,0x10487ac8,{manager});assign_positions(c,manager);
}
std::uint32_t category(Cpu& c,unsigned manager,unsigned nation) {
    const Manager m{c,manager};const auto reputation=c.i16(nation+0xac);c.set(6,extended(reputation));
    if (compare(c,reputation,3000)<0) {
        c.set(5,m.nation(nation).address);
        if (compare(c,m.nation(nation).position(),50)<0) return result(c,3);
    }
    if (compare(c,reputation,6000)<0) {
        c.set(5,m.nation(nation).address);
        if (compare(c,m.nation(nation).position(),15)<0) return result(c,2);
    }
    const auto position=std::int16_t(m.nation(nation).position());
    if (compare(c,position,21)<0)return result(c,1);
    if (compare(c,position,61)<0)return result(c,2);
    c.set(4,101);return result(c,position<101?3:4);
}
std::uint32_t compare_matches(Cpu& c,unsigned left,unsigned right) {
    return result(c,std::uint32_t(std::int32_t(c.i16(right)))-std::uint32_t(std::int32_t(c.i16(left))));
}
std::uint32_t compare_rankings(Cpu& c,unsigned left,unsigned right) {
    Fpu fp(c);c.set(5,c.u32(left));c.set(3,c.u32(right));
    c.set_f(1,c.f32(c.r(5)+12));c.set_f(0,c.f32(c.r(3)+12));fp.fcmp(0,c.f(1),c.f(0));
    return result(c,(c.cr(0)&8)?1u:(c.cr(0)&4)?0xffffffffu:0u);
}
std::uint32_t count_matches(Cpu& c,unsigned manager,unsigned nation,unsigned first_month,unsigned first_year,
                          unsigned last_month,unsigned last_year) {
    const Manager m{c,manager};const auto row=m.nation(nation);
    const auto first=std::uint32_t(std::int32_t(std::int8_t(first_month)))+(first_year&0xffff)*12;
    const auto last=std::uint32_t(std::int32_t(std::int8_t(last_month)))+(last_year&0xffff)*12;
    const auto initial=std::uint32_t(std::int32_t(std::int16_t(unsigned(row.count())-1)));
    leave(c,{{3,0},{4,(last_year&0xffff)*12},{5,c.u32(nation+0x68)*Ranking::size+4},
             {6,initial*6},{7,last},{8,first},{9,initial},{10,std::uint32_t(std::int32_t(std::int8_t(first_month)))}});
    unsigned count=0;
    for (auto index=initial; ;--index) {
        if (compare(c,std::int16_t(index))<0) break;
        const auto at=std::uint32_t(row.matches())+index*6;
        const auto month=c.u8(at+2); const auto date=month+c.u16(at+4)*12u;
        c.set(4,month);
        if (compare(c,date,first)>=0 && compare(c,date,last)<=0) { ++count;c.set(3,count); }
        else if (compare(c,date,first)<0) return count;
        leave(c,{{6,(index-1)*6},{9,index-1}});
    }
    return result(c,count);
}
void update(Cpu& c,unsigned manager,unsigned date) {
    Frame frame(c,0x10486e40,0x60,{.first_gpr=25});Program p{c};Manager m{c,manager};
    keep(c,{{25,manager},{26,date},{30,c.toc(-0x617c)},{31,c.toc(-0x7b04)}});
    p.call(0x107182f0,0x10486e68,{date});
    if (compare(c,std::int16_t(c.r(3)),18)!=0)return;
    p.call(0x10718360,0x10486e7c,{date});const auto month=c.r(3)&0xff;
    keep(c,{{27,0},{28,month},{29,0}});copy_date(c,manager+8,date);
    for (unsigned country=0;compare(c,country,c.i32(c.r(31)))<0;++country) {
        const auto row=m.at(country);
        auto index=std::uint32_t(std::int32_t(std::int8_t(unsigned(row.count())-1)));
        c.set(3,index);c.set(6,index*6);
        while (compare(c,std::int8_t(index))>=0) {
            const auto at=std::uint32_t(row.matches())+index*6;
            const auto year=c.u16(date+2),record_year=c.u16(at+4);
            leave(c,{{5,year},{7,at},{8,row.address},{9,row.matches()}});
            bool expired=compare(c,record_year+1,year)<0;
            if (!expired && c.cr(0)&2) {c.set(0,c.u8(at+2));expired=compare_logical(c,c.u8(at+2),month)<0;}
            c.set(4,record_year);
            if (expired) {
                const auto last=std::uint32_t(row.matches())+std::uint32_t(std::int32_t(row.count())-1)*6;
                c.put16(at,c.u16(last));c.put8(at+2,c.u8(last+2));c.put16(at+4,c.u16(last+4));
                c.set(4,m.records());c.set(5,m.records()+10);row.count()=std::int16_t(unsigned(row.count())-1);
            }
            --index;c.set(3,index);c.set(6,index*6);
        }
        p.call(0x10768300,0x10486f5c,{row.matches(),extended(row.count()),6,c.r(30)});
        keep(c,{{27,country+1},{29,(country+1)*Ranking::size}});
    }
    if (compare_logical(c,month,c.u8(manager+4))==0) {
        const auto now=c.toc(-0x7a9c);c.set(3,now);c.set(4,c.u16(date+2));
        if (compare_logical(c,c.r(4),c.u16(now+2))!=0) {
            leave(c,{{4,0},{8,0},{10,0}});
            for (unsigned country=0;compare(c,country,c.i32(c.r(31)))<0;++country) {
                const auto row=m.at(country).address;
                for (int year=5;year>0;--year)c.put32(row+20+unsigned(year)*4,c.u32(row+16+unsigned(year)*4));
                c.put32(row+20,0);
                leave(c,{{3,m.records()},{5,row},{6,0},{7,0},{8,(country+1)*Ranking::size},{9,0},{10,country+1}});
                compare(c,0);
            }
        }
    }
    enter(c,0x10487004,{manager});calculate(c,manager);
    enter(c,0x1048700c,{manager});assign_positions(c,manager);
}
void calculate(Cpu& c,unsigned manager) {
    Frame frame(c,0x10487c90,0x80,{.first_gpr=22});Program p{c};Fpu fp(c);Manager m{c,manager};
    keep(c,{{22,0x43300000},{23,manager},{24,0},{26,0},{27,c.toc(-0x7b80)},
            {28,c.toc(-0x47ac)},{29,c.toc(-0x7b04)},{30,c.toc(-0x7a9c)},{31,0x2aaaaaab}});
    c.set(4,0x2aab0000);
    for (unsigned country=0;compare(c,country,c.i32(c.r(29)))<0;++country) {
        auto row=m.at(country);std::uint32_t best=0,total=0,index=0;
        c.set(25,0);leave(c,{{4,0},{8,0}});
        while (true) {
            c.set(3,std::uint32_t(std::int32_t(std::int8_t(index))));c.set(5,row.address);c.set(6,extended(row.count()));
            if (compare(c,std::int8_t(index),row.count())>=0 || compare(c,std::int8_t(index),8)>=0)break;
            c.set(3,row.matches());best+=std::uint32_t(std::int32_t(c.i16(std::uint32_t(row.matches())+index*6)));
            ++index;c.set(8,index);c.set(4,index*6);c.set(25,best);
        }
        total=best;c.set(7,total);c.set(4,std::uint32_t(std::int32_t(std::int8_t(index)))*6);
        while (compare(c,std::int8_t(index),row.count())<0) {
            c.set(3,row.matches());total+=std::uint32_t(std::int32_t(c.i16(std::uint32_t(row.matches())+index*6)));
            ++index;leave(c,{{4,index*6},{7,total},{8,index}});
        }
        if (compare(c,row.count(),8)>0) {
            const auto scaled=std::uint32_t(signed_divide(total*8,row.count()));
            c.set(3,best+scaled);c.put32(row.address+20,std::uint32_t(signed_divide(best+scaled,2)));
        } else c.put32(row.address+20,best);
        c.put32(frame.at(0x38),c.u32(c.r(30)));c.put32(frame.at(0x3c),c.u32(c.r(30)+4));
        p.call(0x10717b20,0x10487d94,{frame.at(0x40),c.r(27),365});
        c.set(3,c.u16(frame.at(0x3a)));
        bool later=compare_logical(c,c.r(3),c.u16(frame.at(0x42)))>0;
        if (c.cr(0)&2) {c.set(4,extended(c.i16(frame.at(0x38))));later=c.i16(frame.at(0x38))>c.i16(frame.at(0x40));c.set(3,std::uint32_t(std::int32_t(c.i16(frame.at(0x38))^c.i16(frame.at(0x40)))>>1));}
        compare(c,later);
        if (later) {
            c.set(5,row.address);c.set(4,extended(row.count()));
            if (compare(c,row.count(),9)<0) {
                if (compare(c,row.count())!=0) {
                    c.set(3,best+c.u32(row.address+24));
                    c.put32(row.address+20,std::uint32_t(signed_divide(c.r(3),std::int32_t(row.count())+8))*8);
                } else c.put32(row.address+20,c.u32(row.address+24));
            }
        }
        c.set(24,country+1);c.put32(frame.at(0x48),0x43300000);c.set(3,row.address);
        c.set_f(2,c.f64(c.r(28)+0x50));
        const auto sixth=[](std::uint32_t value) {
            const auto high=std::int32_t((std::int64_t(std::int32_t(value))*0x2aaaaaab)>>32);
            return std::uint32_t(high)+std::uint32_t(high<0);
        };
        const auto a=c.u32(row.address+24)*5,b=c.u32(row.address+28)*4,d=c.u32(row.address+32)*3,
                   e=c.u32(row.address+36)*2,f=c.u32(row.address+40);
        const auto weighted=sixth(a)+sixth(b)+sixth(d)+sixth(e)+sixth(f)+c.u32(row.address+20);
        c.set(26,(country+1)*Ranking::size);
        leave(c,{{4,sixth(f)},{5,std::uint32_t(std::int32_t((std::int64_t(std::int32_t(f))*0x2aaaaaab)>>32)<0)},
                 {6,c.u32(row.address+20)},{7,std::uint32_t((std::int64_t(std::int32_t(f))*0x2aaaaaab)>>32)},
                 {8,std::uint32_t((std::int64_t(std::int32_t(e))*0x2aaaaaab)>>32)},{9,f}});
        c.put32(frame.at(0x4c),weighted^0x80000000);c.set_f(1,c.f64(frame.at(0x48)));
        c.set_f(1,fp.fsubs(c.f(1),c.f(2)));c.set_f(0,fp.fdiv(c.f(1),c.f64(c.r(28)+0x48)));
        c.set_f(0,fp.frsp(c.f(0)));c.put_f32(row.address+12,float(c.f(0)));
    }
}
}

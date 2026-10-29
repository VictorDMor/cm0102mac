#include "dispute_random.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::dispute {
using namespace detail;
void start(Cpu& c,std::uint32_t object,std::uint32_t person,std::uint32_t club,
           std::uint32_t reason,std::uint32_t temporary) {
    Frame frame(c,0x1070c8f0,0xc0,{.first_gpr=15}); Program p{c};
    keep(c,{{21,c.toc(-0x4008)},{22,c.toc(-0x7b90)},{23,c.toc(-0x7b8c)},
            {24,c.toc(-0x7b88)},{25,c.toc(-0x7b84)},{26,c.toc(-0x7b80)},
            {27,object},{28,person},{29,club},{30,reason},{31,temporary}});
    c.set(17,frame.at(0x48)); p.call(0x10715880,0x1070c938,{c.r(17)});
    if (compare_logical(c,person)==0) return;
    if (compare_logical(c,c.u32(person+0x14))==0) return;
    compare_logical(c,person);
    c.set(3,c.toc(-0x7be4)); c.set(4,c.u32(person));
    if (compare(c,c.r(4),c.u32(c.r(3)))>=0) return;
    c.set(3,c.u32(object+4));
    if (compare(c,c.i8(c.r(3)+c.u32(person)))!=0) return;
    p.call(0x10003dc0,0x1070c994,{indirect(c,-0x7b24),person,0});
    if (compare(c,std::int8_t(c.r(3)))!=0) return;
    c.set(5,c.u32(object+4)); c.set(4,c.u32(person));
    if (compare(c,c.i8(c.r(5)+c.r(4)))!=0) return;
    c.set(3,c.toc(-0x7a68));
    if (compare(c,c.i8(c.r(3)))==0) return;
    c.put8(c.r(5)+c.r(4),1);
    const SamplePool pool{c,c.r(22),c.r(23),c.r(24),c.r(25)};
    Fpu fp(c);
    constexpr unsigned offsets[]={10,8,12};
    constexpr unsigned backs[]={0x1070c9ec,0x1070caa0,0x1070cb54};
    for (unsigned i=0;i<3;++i) {
        pool.advance(backs[i]);
        const auto at=pool.position(),attributes=c.u32(person+0x14);
        c.set(3,attributes);
        const auto word=c.u16(at);
        const auto attribute=c.i16(attributes+offsets[i]);
        const auto sum=std::uint32_t(word)+pool.bias();
        c.put32(frame.at(0x5c),0x43300000);
        c.set(6,0x43300000); c.set(7,std::uint32_t(attribute));
        c.set_f(4,c.f64(c.r(21)+24)); c.set_f(0,c.f64(c.r(21)+8));
        c.put32(frame.at(i?0x58:0x68),std::uint32_t(attribute)^0x80000000);
        c.set_f(2,c.f64(c.r(21)));
        c.put32(frame.at(i?0x54:0x64),0x43300000);
        c.set(4,std::int32_t(sum)<0?1:0);
        c.set_f(1,c.f64(frame.at(i?0x54:0x64)));
        c.set_f(3,fp.fsub(c.f(1),c.f(4)));
        const auto rest=remainder(sum,21);
        c.put32(frame.at(0x60),rest^0x80000000);
        c.set_f(1,c.f64(frame.at(0x5c))); c.set_f(1,fp.fsub(c.f(1),c.f(4)));
        c.set_f(0,fp.fdiv(c.f(1),c.f(0))); c.set_f(0,fp.fsub(c.f(2),c.f(0)));
        c.set_f(0,fp.fmul(c.f(3),c.f(0)));
        c.put64(frame.at(i?0x64:0x54),fp.fctiwz(0,c.f(0)));
        c.put16(attributes+offsets[i],c.u32(frame.at(i?0x68:0x58)));
        c.set(5,sum);
    }
    c.set(3,c.u32(person+0x24));
    if (compare_logical(c,c.r(3))!=0) {
        p.call(0x100d9b80,0x1070cc04,{person,c.u32(c.r(3)+0x90),1});
        p.call(0x1014c2e0,0x1070cc1c,{c.u32(person+0x24),1,1,0,0});
        if (compare(c,std::int8_t(c.r(3)))==0)
            p.call(0x100d9b80,0x1070cc38,{c.u32(c.u32(person+0x24)+0x90),person,1});
        if (compare(c,std::int8_t(temporary))==0) {
            c.set(18,0); c.set(20,0);
            do {
                c.set(3,c.u32(person+0x24)); c.set(19,c.u32(c.r(3)+c.r(20)+0x98));
                if (compare_logical(c,c.r(19))!=0 && compare_logical(c,c.r(19),person)!=0) {
                    const auto peer=c.r(19);
                    const auto draw_byte=[&](unsigned offset,unsigned back) {
                        const auto trait=c.i8(peer+offset); c.set(16,std::uint32_t(std::int32_t(trait)));
                        if (compare(c,trait)==0) { c.set(15,0); return; }
                        pool.advance(back);
                        c.set(3,0x10000); compare(c,trait,65535);
                        c.set(3,0xffff0000); compare(c,trait,-65535);
                        c.set(3,pool.position()); compare(c,trait);
                        c.set(3,c.u16(c.r(3))+pool.bias()); c.set(15,oriented(c.r(3),trait));
                        // Signed-byte traits cannot take the original wide-range arm.
                    };
                    draw_byte(0x65,0x1070cc98);
                    if (compare(c,c.r(15),5)>0) {
                        draw_byte(0x67,0x1070cdfc);
                        if (compare(c,c.r(15))==0) p.call(0x100d9b80,0x1070cf3c,{peer,person,1});
                    }
                }
                c.set(18,c.r(18)+1); c.set(20,c.r(20)+4);
            } while (compare(c,c.r(18),50)<0);
        }
        p.call(0x100d9b80,0x1070cf60,{c.u32(c.u32(person+0x24)+0x80),person,1});
    }
    c.put32(frame.at(0x40),c.u32(person));
    const auto next=c.u32(c.r(25))+2;
    const auto end=c.u32(c.r(24));
    const auto club_id=c.u32(c.r(29));
    c.set(4,c.u8(c.r(26)+4)); c.set(5,c.u16(c.r(26)+2)); c.set(6,club_id);
    compare_logical(c,next,end);
    c.put32(frame.at(0x44),club_id); c.put16(frame.at(0x4a),c.r(5));
    c.put16(frame.at(0x48),c.u16(c.r(26))); c.put8(frame.at(0x4c),c.r(4));
    c.set(3,next); c.put32(c.r(25),next);
    if (next>end) { c.put32(c.r(25),c.r(23)); p.call(0x1077b770,0x1070cfa8); c.put32(c.r(22),c.r(3)&0xffff); }
    const auto at=pool.position(); const auto bias=pool.bias(); const auto word=c.u16(at);
    const auto sum=std::uint32_t(word)+bias;
    c.set(6,sum);
    p.call(0x10717890,0x1070cff4,{frame.at(0x38),c.r(17),std::uint32_t(std::int32_t(std::int16_t(remainder(sum,20)+2)))});
    compare(c,std::int8_t(temporary));
    c.set(3,c.u8(frame.at(0x3c))); c.set(4,c.u16(frame.at(0x3a)));
    c.put16(frame.at(0x4a),c.r(4)); c.put16(frame.at(0x48),c.u16(frame.at(0x38))); c.put8(frame.at(0x4c),c.r(3));
    bool suppress=false;
    if (std::int8_t(temporary)) {
        pool.advance(0x1070d034);
        const auto cursor=pool.position(); const auto shift=pool.bias();
        c.set(5,cursor); c.set(3,c.u16(cursor)); c.set(4,c.r(3)+shift);
        suppress=compare(c,remainder(c.r(4),10))==0;
    }
    c.put8(frame.at(0x50),suppress?0:temporary);
    p.call(0x10738d10,0x1070d090,{c.u32(object),frame.at(0x40),20});
    compare_logical(c,club);
    const auto reported_club=club?club:c.u32(person+0x24);
    enter(c,0x1070d0bc,{object,person,reported_club,0,reason,temporary});
    publish(c,object,person,reported_club,0,reason,temporary);
}
}

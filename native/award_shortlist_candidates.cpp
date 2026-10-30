#include "award_shortlist_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::award_shortlist {
using namespace detail;
std::uint32_t add(Cpu& c,std::uint32_t object,std::uint32_t person,std::uint32_t club,double score) {
    Frame frame(c,0x104382c0,0xa0,{.first_gpr=29});Shortlist list{c,object};
    keep(c,{{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(-0x4a64)}});
    const auto count=list.count().get();c.set(7,std::uint32_t(count));
    if (compare(c,count,list.capacity())>=0) {
        assertion(c,frame,0x10438308,0x68,0x40,0xbf);return result(c,1);
    }
    if (compare_logical(c,person)==0) {
        assertion(c,frame,0x10438368,0x48,0x38,0xc6);return result(c,0);
    }
    const auto candidate=list.candidate(count);
    auto identity=c.u32(person);c.set(6,identity);c.set(4,list.candidates());
    c.put32(candidate,identity);
    if (compare_logical(c,club)!=0) identity=c.u32(club);else identity=0xffffffff;
    leave(c,{{6,identity},{5,0},{4,candidate}});
    c.put32(candidate+4,identity);c.put_f32(candidate+8,float(score));c.put16(candidate+12,0);
    list.count()=std::int16_t(std::uint32_t(count)+1);
    c.set(4,std::uint32_t(list.count().get()));
    return result(c,list.capacity().get()==list.count().get()?1:0);
}
void record_vote(Cpu& c,std::uint32_t object,std::uint32_t manager,std::uint32_t person) {
    Frame frame(c,0x10439a40,0xc0,{.first_gpr=29});Shortlist list{c,object};
    keep(c,{{29,c.toc(-0x7ba8)},{30,c.toc(-0x4a64)},{31,c.toc(-0x7bac)}});
    c.set(6,c.toc(-0x7be4));
    const auto index=c.u32(manager)-(c.u32(c.r(6))-16);c.set(4,c.u32(c.r(6))-16);
    if (compare(c,index)<0 || compare(c,index,16)>=0) {
        assertion(c,frame,0x10439a98,0x90,0x48,0x21c);return;
    }
    const auto ballot=object+index*2;c.set(8,ballot);
    if (compare(c,c.i16(ballot+8),-1)>0) {
        assertion(c,frame,0x10439b00,0x70,0x40,0x223);return;
    }
    std::uint32_t offset=0,index_word=0;
    c.set(9,0);c.set(7,0);
    while (true) {
        c.set(4,std::uint32_t(std::int16_t(index_word)));
        if (compare(c,std::int16_t(index_word),list.count())>=0) break;
        const auto storage=list.candidates().get();c.set(4,storage);
        const auto identity=c.u32(person);c.set(6,identity);
        if (compare(c,identity,c.u32(storage+offset))==0) {
            c.put16(ballot+8,index_word);
            const auto candidate=list.candidate(std::int16_t(index_word));
            c.set(4,candidate);c.set(3,std::uint32_t(c.i16(candidate+12)));
            c.put16(candidate+12,c.r(3)+1);return;
        }
        offset+=16;++index_word;c.set(7,offset);c.set(9,index_word);
    }
    assertion(c,frame,0x10439bb4,0x50,0x38,0x237);
}
std::uint32_t eligibility(Cpu& c,std::uint32_t object,std::uint32_t manager) {
    Frame frame(c,0x10439c10,0xa0,{.first_gpr=29});Shortlist list{c,object};
    keep(c,{{29,c.toc(-0x7ba8)},{30,c.toc(-0x4a64)},{31,c.toc(-0x7bac)}});
    if (compare(c,list.count())==0) return result(c,0xfffffffe);
    const auto count_address=c.toc(-0x7be4);c.set(5,count_address);
    const auto manager_base=c.u32(count_address)-16;c.set(5,manager_base);
    const auto index=c.u32(manager)-manager_base;
    if (compare(c,index)<0 || compare(c,index,16)>=0) {
        assertion(c,frame,0x10439c7c,0x68,0x40,0x24f);return result(c,0xfffffffe);
    }
    c.set(5,object+index*2);
    const auto ballot=c.i16(object+index*2+8);
    if (compare(c,ballot,-1)>0) return result(c,std::uint32_t(ballot));
    const auto electorate=list.electorate().get();
    if (compare_logical(c,std::uint32_t(electorate),6)>0) {
        assertion(c,frame,0x10439e38,0x48,0x38,0x2a5);return result(c,0xfffffffe);
    }
    // This indexed branch reads the table base and destination into r5.
    const auto table=c.toc(-0x4a78);c.set(5,c.u32(table+std::uint32_t(electorate)*4));
    if (electorate==6) return result(c,0xffffffff);
    auto entity=c.u32(manager+(electorate==3 || electorate==5?0x20:0x24));
    if (electorate==4 || electorate==5) {
        return result(c,compare_logical(c,entity)!=0?0xffffffff:0xfffffffe);
    }
    c.set(4,entity);
    if (compare_logical(c,entity)==0) return result(c,0xfffffffe);
    if (electorate>=1) {
        entity=c.u32(entity+0x2c);c.set(4,entity);
        if (compare_logical(c,entity)==0) return result(c,0xfffffffe);
    }
    if (electorate>=2) {
        entity=c.u32(entity+0x74);c.set(4,entity);
        if (compare_logical(c,entity)==0) return result(c,0xfffffffe);
    }
    const auto identity=c.u32(entity+(electorate==0?0:electorate==1?0x68:8));c.set(4,identity);
    // The two regional cases compare +0x30, then test +0x34 for -1.
    const auto selected=c.u32(object+(electorate==0?0x2c:0x30));
    if (compare(c,identity,selected)!=0) return result(c,0xfffffffe);
    const auto sentinel=electorate>=2?c.u32(object+0x34):selected;
    return result(c,compare(c,sentinel,-1)!=0?0xffffffff:0xfffffffe);
}
std::uint32_t compare_names(Cpu& c,std::uint32_t left,std::uint32_t right) {
    Frame frame(c,0x10439ee0,0xfe0,{.first_gpr=31});Program p{c};
    c.set(31,0);c.set(5,c.toc(-0x7be0));
    const auto first_id=c.u32(left);c.set(3,0);
    if (compare(c,first_id,-1)!=0) c.set(3,c.u32(c.r(5))+first_id*0x70);
    const auto second_id=c.u32(right);
    if (compare(c,second_id,-1)!=0) { c.set(4,c.u32(c.r(5)));c.set(31,c.r(4)+second_id*0x70); }
    c.put8(frame.at(0x808),0);
    if (compare_logical(c,c.r(3))!=0) p.call(0x100caeb0,0x10439f4c,{c.r(3),frame.at(0x808),0x7d0,4});
    c.put8(frame.at(0x38),0);
    if (compare_logical(c,c.r(31))!=0) p.call(0x100caeb0,0x10439f70,{c.r(31),frame.at(0x38),0x7d0,4});
    return p.call(0x107465f0,0x10439f84,{frame.at(0x808),frame.at(0x38),1,0});
}
std::uint32_t compare_scores(Cpu& c,std::uint32_t left,std::uint32_t right) {
    Fpu fp(c);c.set_f(0,c.f32(left+8));c.set_f(1,c.f32(right+8));
    fp.fcmp(0,c.f(0),c.f(1));
    if (c.f(0)>c.f(1)) return result(c,0xffffffff);
    fp.fcmp(0,c.f(1),c.f(0));return result(c,c.f(1)>c.f(0)?1:0);
}
std::uint32_t compare_votes(Cpu& c,std::uint32_t left,std::uint32_t right) {
    return result(c,std::uint32_t(c.i16(right+12))-std::uint32_t(c.i16(left+12)));
}
}

#include "award_shortlist_detail.hpp"

namespace cm::aot::award_shortlist {
using namespace detail;
std::uint32_t construct(Cpu& c,std::uint32_t object,std::uint32_t specification,std::uint32_t file) {
    Frame frame(c,0x10437cc0,0xa0,{.first_gpr=26}); Program p{c}; Shortlist list{c,object};
    keep(c,{{26,specification},{27,file},{28,c.toc(-0x7bac)},
            {29,c.toc(-0x7ba8)},{30,c.toc(-0x4a64)},{31,object}});
    p.call(0x10715880,0x10437cf0,{object+0x40});
    if (compare_logical(c,specification)==0) {
        assertion(c,frame,0x10437d0c,0x68,0x40,0x30); return result(c,object);
    }
    c.put16(object+0x3c,0xffff);
    list.capacity()=c.i8(specification+0x10);list.count()=0;
    list.candidates()=p.call(0x10775130,0x10437d78,{std::uint32_t(list.capacity().get())*16});
    if (compare_logical(c,list.candidates())==0) {
        assertion(c,frame,0x10437d9c,0x48,0x38,0x3e); return result(c,object);
    }
    c.put8(object+0x28,c.u8(specification));
    for (unsigned offset:{4u,8u,12u}) c.put32(object+0x28+offset,c.u32(specification+offset));
    c.put8(object+0x38,c.u8(specification+0x10));
    if (compare_logical(c,file)!=0) { enter(c,0x10437e1c,{object,file});c.set(3,load(c,object,file)); }
    return result(c,object);
}
std::uint32_t destroy(Cpu& c,std::uint32_t object,std::uint32_t deleting) {
    Frame frame(c,0x10437e40,0x40,{.first_gpr=30});Program p{c};
    keep(c,{{30,object},{31,deleting}});
    if (compare(c,object)!=0) {
        c.set(3,c.u32(object));
        if (compare_logical(c,c.r(3))!=0) p.call(0x10775170,0x10437e70);
        if (compare(c,std::int16_t(deleting))>0) p.call(0x107735a0,0x10437e80,{object});
    }
    return result(c,object);
}
std::uint32_t load(Cpu& c,std::uint32_t object,std::uint32_t file) {
    Frame frame(c,0x10437ea0,0xf0,{.first_gpr=27});Program p{c};Shortlist list{c,object};
    keep(c,{{27,object},{28,file},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(-0x4a64)}});
    const auto fail=[&](unsigned at,int name,int ext,unsigned line) {
        assertion(c,frame,at,name,ext,line);return result(c,0);
    };
    p.call(0x10747a80,0x10437ed8,{object+4,4,1,file});
    if (compare(c,c.r(3),1)!=0) return fail(0x10437ef4,0xb8,0x50,0x67);
    c.set(5,std::uint32_t(list.count().get()));
    if (compare(c,std::int16_t(c.r(5)))!=0) {
        p.call(0x10747a80,0x10437f54,{list.candidates(),16,c.r(5),file});
        if (compare(c,list.count(),c.r(3))!=0) return fail(0x10437f74,0x98,0x48,0x71);
    }
    p.call(0x10747a80,0x10437fcc,{object+8,2,16,file});
    if (compare(c,c.r(3),16)!=0) return fail(0x10437fe8,0x78,0x40,0x7a);
    p.call(0x10747a80,0x10438040,{object+0x40,8,1,file});
    if (compare(c,c.r(3),1)!=0) return fail(0x1043805c,0x58,0x38,0x81);
    return result(c,1);
}
void save(Cpu& c,std::uint32_t object,std::uint32_t file) {
    Frame frame(c,0x104380c0,0xf0,{.first_gpr=27});Program p{c};Shortlist list{c,object};
    keep(c,{{27,object},{28,file},{29,c.toc(-0x7bac)},{30,c.toc(-0x7ba8)},{31,c.toc(-0x4a64)}});
    p.call(0x10747cc0,0x104380f8,{object+4,4,1,file});
    if (compare(c,c.r(3),1)!=0) { assertion(c,frame,0x10438114,0xb8,0x50,0x93);return; }
    c.set(5,std::uint32_t(list.count().get()));
    if (compare(c,std::int16_t(c.r(5)))!=0) {
        p.call(0x10747cc0,0x10438170,{list.candidates(),16,c.r(5),file});
        if (compare(c,list.count(),c.r(3))!=0) { assertion(c,frame,0x10438190,0x98,0x48,0x9c);return; }
    }
    p.call(0x10747cc0,0x104381e4,{object+8,2,16,file});
    if (compare(c,c.r(3),16)!=0) { assertion(c,frame,0x10438200,0x78,0x40,0xa5);return; }
    p.call(0x10747cc0,0x10438254,{object+0x40,8,1,file});
    if (compare(c,c.r(3),1)!=0) assertion(c,frame,0x10438270,0x58,0x38,0xac);
}
void clear(Cpu& c,std::uint32_t object) {
    Frame frame(c,0x10439ea0,0x40,{});Program p{c};
    c.put16(object+4,0);p.call(0x10778300,0x10439ec4,{object+8,0xffffffff,32});
}
}

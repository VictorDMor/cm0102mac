#include "dispute_detail.hpp"

namespace cm::aot::dispute {
using namespace detail;
std::uint32_t active(Cpu& c,std::uint32_t object,std::uint32_t person,std::uint32_t unchecked) {
    Frame frame(c,0x1070c3f0,0x50,{.first_gpr=29}); Program p{c}; Manager manager{c,object};
    keep(c,{{29,c.toc(-0x7b78)},{30,object},{31,person}});
    if (compare(c,person)==0) return result(c,0);
    c.set(3,c.toc(-0x7be4)); c.set(4,c.u32(person));
    if (compare(c,c.r(4),c.u32(c.r(3)))>=0) return result(c,0);
    c.set(4,c.u32(person)); c.set(3,c.u8(manager.flags().get()+c.u32(person)));
    if (compare(c,std::int8_t(c.r(3)))!=0) {
        c.set(4,c.toc(-0x7a68));
        if (compare(c,c.i8(c.r(4)))==0) {
            enter(c,0x1070c470,{object,person}); remove_person(c,object,person);
            return result(c,0);
        }
    }
    if (compare(c,std::int8_t(unchecked))!=0) return c.r(3);
    if (compare(c,std::int8_t(c.r(3)))!=0) {
        p.call(0x10082f20,0x1070c49c,{indirect(c,-0x7b7c),person});
        compare_logical(c,c.r(3)); c.put32(c.r(29),c.r(3));
        const auto club=c.r(3)?indirect(c,-0x7be8)+c.u32(c.r(3)+4)*0x248:0;
        if (c.r(3)) c.set(3,indirect(c,-0x7be8));
        if (compare_logical(c,club)==0) {
            enter(c,0x1070c4d8,{object,person}); remove_person(c,object,person);
            return result(c,0);
        }
    }
    c.set(3,c.u8(manager.flags().get()+c.u32(person)));
    return c.r(3);
}
std::uint32_t club_id(Cpu& c,std::uint32_t object,std::uint32_t person) {
    Frame frame(c,0x1070c710,0x50,{.first_gpr=29}); Program p{c}; Manager manager{c,object};
    keep(c,{{29,object},{30,person}});
    if (compare(c,person)==0) return result(c,0xffffffff);
    c.set(3,manager.flags());
    if (compare_logical(c,c.r(3))==0) return result(c,0xffffffff);
    if (compare(c,c.i8(c.r(3)+c.u32(person)))==0) return result(c,0xffffffff);
    c.set(3,manager.records());
    if (compare_logical(c,c.r(3))!=0) {
        c.put32(c.r(3)+12,c.u32(c.r(3)+4)); c.set(31,0);
        while (true) {
            c.set(3,manager.records());
            if (compare(c,c.r(31),c.i16(c.r(3)))>=0) break;
            p.call(0x10738e00,0x1070c78c);
            if (compare_logical(c,c.r(3))==0) return result(c,0xffffffff);
            c.set(4,c.u32(c.r(3)));
            if (compare(c,c.r(4),c.u32(person))==0) return result(c,c.u32(c.r(3)+4));
            c.set(31,c.r(31)+1);
        }
    }
    return result(c,0xffffffff);
}
void remove_person(Cpu& c,std::uint32_t object,std::uint32_t person) {
    Frame frame(c,0x1070c7f0,0x50,{.first_gpr=29}); Program p{c}; Manager manager{c,object};
    keep(c,{{29,object},{30,person}});
    if (compare(c,person)==0) return;
    c.set(4,manager.flags());
    if (compare_logical(c,c.r(4))==0) return;
    c.set(3,c.u32(person));
    if (compare(c,c.i8(c.r(4)+c.r(3)))==0) return;
    c.put8(c.r(4)+c.r(3),0); c.set(3,manager.records());
    if (compare_logical(c,c.r(3))==0) return;
    c.put32(c.r(3)+12,c.u32(c.r(3)+4)); c.set(31,0);
    while (true) {
        c.set(3,manager.records());
        if (compare(c,c.r(31),c.i16(c.r(3)))>=0) return;
        p.call(0x10738e00,0x1070c85c); c.set(4,c.r(3));
        if (compare(c,c.r(4))==0) return;
        c.set(3,c.u32(c.r(4)));
        if (compare(c,c.r(3),c.u32(person))==0) {
            p.call(0x10738e60,0x1070c87c,{manager.records()}); return;
        }
        c.set(31,c.r(31)+1);
    }
}
void remove_record(Cpu& c,std::uint32_t object,std::uint32_t record) {
    Frame frame(c,0x1070c8b0,0x40,{}); Program p{c};
    if (compare_logical(c,record)==0) return;
    c.set(5,c.u32(object+4)); c.set(6,0); c.put8(c.r(5)+c.u32(record),0);
    p.call(0x10738e60,0x1070c8dc,{c.u32(object)});
}
void update(Cpu& c,std::uint32_t object) {
    Frame frame(c,0x1070c510,0x70,{.first_gpr=23}); Program p{c}; Manager manager{c,object};
    keep(c,{{25,object},{28,c.toc(-0x7b40)},{29,c.toc(-0x7be0)},
            {30,c.toc(-0x7b80)},{31,c.toc(-0x7be4)}});
    c.set(3,manager.records());
    if (compare_logical(c,c.r(3))==0) return;
    c.put32(c.r(3)+12,c.u32(c.r(3)+4)); c.set(27,0);
    while (true) {
        c.set(3,manager.records());
        if (compare(c,c.r(27),c.i16(c.r(3)))>=0) return;
        c.set(26,p.call(0x10738e00,0x1070c554));
        if (compare(c,c.r(26))==0) return;
        Record record{c,c.r(26)};
        c.set(3,c.u32(c.r(30))); c.put32(frame.at(0x38),c.r(3));
        c.put32(frame.at(0x3c),c.u32(c.r(30)+4));
        c.set(3,c.u16(record.address+10)); c.set(4,c.u16(frame.at(0x3a)));
        const auto order=compare_logical(c,c.r(4),c.r(3));
        bool expired=order>0;
        if (!order) { c.set(4,std::uint32_t(c.i16(frame.at(0x38)))); expired=c.i16(frame.at(0x38))>c.i16(record.address+8); }
        if (compare(c,expired?1:0)!=0) {
            c.set(4,record.person());
            if (compare(c,c.r(4),-1)>0) {
                c.set(3,c.u32(c.r(31)));
                if (compare(c,c.r(4),c.r(3)-1)<0) {
                    const auto person=c.u32(c.r(29))+c.r(4)*0x70;
                    c.set(23,c.u32(person+0x24)); c.set(24,person);
                    if (compare_logical(c,c.r(23))!=0) {
                        c.set(3,c.u32(c.r(23)));
                        if (compare(c,c.r(3),record.club())==0) {
                            if (compare(c,std::int8_t(record.temporary()))!=0) {
                                compare_logical(c,c.r(23));
                                enter(c,0x1070c634,{object,person,c.r(23),1,0,0});
                                publish(c,object,person,c.r(23),1,0,0);
                            } else {
                                p.call(0x103e6290,0x1070c648,{c.u32(c.r(28)),person,0,0xffffffff});
                                c.set(4,c.u32(c.r(23)+0x90));
                                if (compare_logical(c,c.r(4))!=0) {
                                    p.call(0x100d9520,0x1070c65c,{person});
                                    if (compare(c,std::int8_t(c.r(3)))!=0)
                                        p.call(0x100d9e10,0x1070c670,{person,c.u32(c.r(23)+0x90)});
                                    p.call(0x100d9480,0x1070c67c,{person,c.r(23)});
                                    if (compare(c,std::int8_t(c.r(3)))!=0) p.call(0x100d9db0,0x1070c690,{c.r(23),person});
                                    p.call(0x100d9b80,0x1070c6a0,{person,c.u32(c.r(23)+0x90),1});
                                }
                                p.call(0x100d97a0,0x1070c6b0,{c.r(23),person,1});
                                compare_logical(c,c.r(23));
                                enter(c,0x1070c6dc,{object,person,c.r(23),2,0,0});
                                publish(c,object,person,c.r(23),2,0,0);
                            }
                        }
                    }
                    enter(c,0x1070c6e8,{object,record.address}); remove_record(c,object,record.address);
                }
            }
        }
        c.set(27,c.r(27)+1);
    }
}
}

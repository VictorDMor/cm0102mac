#include "screen_manager_packets.hpp"
#include "screen_manager_sound.hpp"

namespace cm::aot::screen_manager {
using namespace detail;
std::uint32_t run_client(Cpu& c,std::uint32_t address) {
    Frame frame(c,0x1073da60,0x1d0,{.first_gpr=16});
    keep(c,{{22,c.toc(-0x7bd0)},{30,address},{21,c.toc(-0x7bd4)},{27,missing},{19,c.toc(-0x5c24)},
            {26,0},{28,c.toc(-0x5c04)},{24,c.toc(-0x7468)},{23,c.toc(-0x7bb8)},
            {25,c.toc(slot::success)},{29,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    Manager manager{c,address};Program p{c};
    auto screen=[&]{return c.u32(manager.group()+0x24);};
    if(!byte_nonzero(c,c.u8(c.r(2)-0x358c))) {c.set(3,0);c.put8(c.r(2)-0x358b,0);c.put8(c.r(2)-0x358c,1);}
    if(compare_logical(c,c.u32(address+0x2c64))==0) {
        assertion(c,frame,0x1073dadc,0x168,0x80,0x3cf);return c.r(3);
    }
    manager.current()=0;c.put32(address+0x24,address+0x12d6b0);
    do {
        c.set(3,c.u32(c.r(24)));
        if(compare_logical(c,c.r(3))!=0) {
            enter(c,0x1073db40);c.set(16,sound_pending(c,c.r(3)));result(c,c.r(16));
            if(compare(c,std::int8_t(c.r(16)),-1)!=0) {
                enter(c,0x1073db5c,{c.u32(c.r(24)),c.r(16)});result(c,sound_stop(c,c.r(3),c.r(4)));
                keep(c,{{17,c.r(28)+4},{18,c.r(28)+8}});
                Packet out{c,frame,c.r(28),c.r(17),c.r(18)};c.put32(out.position,0);
                out.integer(1,14,0x1073db94,0x128,0x70,0x85);out.integer(1,c.r(16),0x1073dc14,0x108,0x68,0x85);
                p.call(0x1073a4c0,0x1073dc7c,{c.u32(address+0x2c64),c.u32(out.data),c.u32(out.position),extended(c.i16(c.r(19)))});
                if(!byte_nonzero(c,c.r(3)))assertion(c,frame,0x1073dc98,0x148,0x78,0x3ec);
            }
        }
        p.call(0x1073a7f0,0x1073dce4,{c.u32(address+0x2c64),c.u32(address+0x2c2c),address+0x2c5c});
        if(byte_nonzero(c,c.r(3))) {
            c.set(3,c.u32(address+0x2c2c));
            if(!byte_nonzero(c,c.u8(c.r(3))))c.put8(c.r(2)-0x358b,1);
            p.call(0x1071de80,0x1073dd24,{screen(),c.u32(address+0x2c64),extended(c.i16(address+0x2c5c)),address+0x2c2c});
            c.set(26,std::int16_t(c.r(3))==-2);
            if(compare(c,std::int8_t(c.r(26)))!=0) {
                p.call(0x1073a480,0x1073dd48,{c.u32(address+0x2c64),extended(c.i16(address+0x2c5c))});break;
            }
            p.call(0x1077f900,0x1073dd54,{frame.at(0x44)});
        }
        p.call(0x1076fe80,0x1073dd58);
        c.set(3,screen()+0x130000);
        if(byte_nonzero(c,c.u8(c.r(3)-0x56bd))) {
            c.put8(c.r(2)-0x358b,0);
            c.set(20,p.call(0x10719350,0x1073dd94,{screen()}));c.set(4,extended(std::int16_t(c.r(20))));
            if(compare(c,c.r(4),-1)!=0 && compare(c,c.r(4),std::int16_t(c.r(27)))!=0) {
                const auto target=c.u32(address+0x2c1c);
                if(compare_logical(c,target)!=0) {
                    c.set(4,extended(std::int16_t(c.r(20))));compare(c,c.r(4),-1);c.set(5,screen());
                    const auto control=std::int16_t(c.r(4));
                    auto category=control==-1?0:c.u32(c.r(5)+std::uint32_t(control)*0x17c+0xba86c);
                    const bool category_set=compare(c,std::int16_t(category))!=0;c.set(4,c.u32(address+0x2c20));
                    if(!category_set)category=extended(c.i16(address+0x25819e));
                    if(compare(c,c.r(4),std::int16_t(category))==0) {
                        c.set(3,1);p.callback(target,0x1073de18);
                        do {
                            c.set(16,0);p.call(0x10719560,0x1073de38,{screen(),1});
                            c.set({{4,1},{3,screen()+0x130000}});c.put8(c.r(3)-0x56c2,1);
                            c.set(3,manager.history());p.callback(c.u32(c.r(3)),0x1073de6c);
                            c.set({{4,missing},{5,missing},{6,missing},{7,missing}});
                            c.put8(screen()+0x12a93e,0);c.put8(screen()+0x12a900,0);
                            p.call(0x10718fc0,0x1073dec8,{screen()});
                            do {
                                p.call(0x1076fe80,0x1073ded0);p.call(0x10719350,0x1073dee4,{screen()});
                                c.set(4,manager.history());p.callback(c.u32(c.r(4)+8),0x1073defc);c.set(16,c.r(3));
                                if(compare(c,c.r(16),-9)==0)break;
                            }while(compare(c,c.r(16),-11)!=0);
                        }while(compare(c,c.r(16),-9)!=0);
                        keep(c,{{16,0},{17,0},{18,c.u32(manager.group()+12)}});
                        c.set(4,c.u32(c.r(18)+0x1f4));manager.history()=c.r(4);c.put32(manager.group()+12,c.r(4));
                        c.set(3,c.u32(manager.group()+12));c.put32(c.r(3)+0x1f8,c.r(16));
                        while(compare(c,std::int8_t(c.r(16)),60)<0) {
                            c.set(4,c.r(18)+c.r(17));c.set(3,c.u32(c.r(4)+20));
                            if(compare_logical(c,c.r(3))!=0 && byte_nonzero(c,c.u8(c.r(4)+24)))p.call(0x10773610,0x1073df84);
                            c.set(17,c.r(17)+8);c.set(16,c.r(16)+1);
                        }
                        p.call(0x107735a0,0x1073dfa0,{c.r(18)});
                        const bool valid=compare(c,std::int16_t(c.r(20)),-1)!=0;
                        c.set(4,manager.group()-address+22);c.set(3,extended(c.i16(address+c.r(4)))-1);c.put16(address+c.r(4),c.r(3));
                        c.set({{5,c.u32(address+0x2c24)},{4,screen()}});
                        if(valid)c.put32(c.r(4)+std::uint32_t(std::int16_t(c.r(20)))*0x17c+0xba86c,c.r(5));
                    }
                }
                if(byte_nonzero(c,c.u8(address+0x2c60))) {
                    p.call(0x1076a580,0x1073e010,{10,0x226,80,0x24e,16,c.u16(c.r(21))});
                    p.call(0x10727890,0x1073e01c,{c.r(23),c.r(31)+0x9f});
                    c.put32(frame.at(0x38),missing);
                    p.call(0x1076e160,0x1073e048,{10,0x226,80,0x24e,12,1,c.u16(c.r(22)),c.r(23)});
                    p.call(0x107692f0,0x1073e05c,{10,0x226,80,0x24e});
                }
                p.call(0x10723cb0,0x1073e078,{screen(),c.u32(address+0x2c64),c.r(20)});
                if(!byte_nonzero(c,c.r(3)))c.set(26,1);
                else c.set(27,c.r(20));
            } else c.set(27,c.r(20));
        } else if(byte_nonzero(c,c.u8(c.r(2)-0x358b))) {
            p.call(0x1077f900,0x1073e0a4,{frame.at(0x40)});
            c.set({{3,c.u32(frame.at(0x44))},{4,c.u32(frame.at(0x40))}});
            if(compare(c,c.r(4),c.r(3)+30)>0 || compare(c,c.r(4),c.r(3))<0) {
                p.call(0x10723a30,0x1073e0d8,{screen(),c.u32(address+0x2c64)});
                p.call(0x1077f900,0x1073e0e0,{frame.at(0x44)});
            }
        }
    }while(!byte_nonzero(c,c.r(26)));
    auto out=packet(c,frame,address);c.put32(out.position,0);
    out.integer(1,19,0x1073e118,0xe8,0x60,0x85);out.integer(2,missing,0x1073e198,0xc8,0x58,0x86);
    p.call(0x1071de80,0x1073e208,{address+0x12d6b0,c.u32(address+0x2c64),0,address+0x2c2c});
    c.put32(out.position,0);out.integer(1,20,0x1073e238,0xa8,0x50,0x85);out.integer(2,missing,0x1073e2b8,0x88,0x48,0x86);
    p.call(0x1071de80,0x1073e328,{address+0x12d6b0,c.u32(address+0x2c64),0,address+0x2c2c});
    c.put32(address+0x24,address+0x2c68);return c.r(3);
}
}

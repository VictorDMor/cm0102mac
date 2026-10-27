#include "screen_manager_packets.hpp"
#include "screen_manager_sound.hpp"

namespace cm::aot::screen_manager {
using namespace detail;
namespace {
struct Monitor {
    Cpu& c;
    const Frame& frame;
    std::uint32_t address;
    Manager manager() const {return {c,address};}
    std::uint32_t group() const {return manager().group();}
    std::uint32_t screen() const {return c.u32(group()+0x24);}
    Program program() const {return {c};}
    void history_callback(unsigned offset,std::uint32_t back,bool optional) const {
        c.set(3,manager().history());const auto target=c.u32(c.r(3)+offset);
        if(!optional || compare_logical(c,target)!=0)program().callback(target,back);
    }
    void free_node(std::uint32_t node,unsigned node_register,std::uint32_t first_back,
                   std::uint32_t delete_back) const {
        // Both event variants use r18/r19 for owned-value cleanup; their node
        // lives in r16 or r20, which the allocator records in its stack image.
        c.set(node_register,node);keep(c,{{18,0},{19,0}});
        while(compare(c,std::int16_t(c.r(18)),60)<0) {
            c.set(4,c.r(node_register)+c.r(19));c.set(3,c.u32(c.r(4)+20));
            if(compare_logical(c,c.r(3))!=0 && byte_nonzero(c,c.u8(c.r(4)+24)))program().call(0x10773610,first_back);
            c.set(19,c.r(19)+8);c.set(18,c.r(18)+1);
        }
        program().call(0x107735a0,delete_back,{c.r(node_register)});
        c.set(4,group()-address+22);c.set(3,extended(c.i16(address+c.r(4))));c.put16(address+c.r(4),c.r(3)-1);
    }
    void close_group_state(std::uint32_t delta) const {
        c.set(3,group());c.set(16,c.u32(c.r(3)+16));
        if(compare_logical(c,c.r(16))!=0) {
            manager().history()=c.u32(c.r(3)+8);
            for(;;) {
                c.set(3,manager().history());
                if(compare_logical(c,c.r(3))==0)break;
                const auto callback=c.u32(c.r(3)+12);
                if(compare_logical(c,callback)!=0)program().callback(callback,0x1073d844+delta);
                c.set(4,group()-address+16);c.set(3,manager().history());manager().history()=c.u32(c.r(3)+0x1f8);
            }
            manager().history()=c.r(16);history_callback(4,0x1073d89c+delta,true);
            program().call(0x10719560,0x1073d8b8+delta,{screen(),1});
            c.set(4,0);manager().history()=0;
        }
        c.set({{5,0},{4,group()-address}});c.put8(group()+24,0);
        enter(c,0x1073d8e8+delta,{address});result(c,clear_history(c,address));
    }
    // Returns true when the handler returned another event that must be
    // processed immediately (the original's local retry branch).
    bool event() const {
        auto p=program();auto m=manager();
        if(compare(c,c.r(30),-12)==0) {
            auto out=packet(c,frame,address);c.put32(out.position,0);out.integer(1,4,0x1073cd34,0xf8,0x60,0x85);
            p.call(0x1073a4c0,0x1073cda0,{c.u32(address+0x2c64),c.u32(out.data),c.u32(out.position),extended(c.i16(address+0x2c5c))});
        }
        compare_logical(c,c.r(30)+13,12);
        switch(std::int32_t(c.r(30))) {
        case -13:case -8:case -6:case -5: {
            c.set({{5,0},{3,group()},{6,m.history()}});
            const auto previous=c.r(6);
            if(compare(c,c.i16(group()+20))>0) {
                m.history()=c.u32(group()+12);
                for(;;) {
                    c.set({{4,group()},{3,m.history()}});
                    if(compare_logical(c,c.r(3))==0)break;
                    if(byte_nonzero(c,c.u8(c.r(3)+16)) &&
                       (compare_logical(c,c.u32(c.r(3)+12))==0 || compare(c,c.r(30),-13)==0)) {
                        c.put8(c.r(4)+24,1);c.set(5,1);break;
                    }
                    m.history()=c.u32(c.r(3)+0x1f4);
                }
                c.set(30,0);
            }
            if(!byte_nonzero(c,c.r(5))) {
                m.history()=previous;
                if(compare(c,c.r(30),-6)==0){c.set(4,1);c.put8(group()+26,1);c.put8(group()+25,1);}
                else {c.set(3,group());if(!byte_nonzero(c,c.u8(c.r(3)+25)))c.put8(c.r(3)+26,c.i8(c.r(3)+26)==0);}
                c.set({{5,0},{6,1},{4,0}});c.put8(group()+24,1);
                for(;;) {
                    c.set({{7,extended(m.current())},{6,extended(std::int16_t(c.r(5)))},{3,std::uint32_t(std::int32_t(m.current()))*2}});
                    if(compare(c,c.r(6),m.count(m.current()))>=0)break;
                    if(compare(c,c.r(30),-8)==0) {
                        c.set(3,group());c.set(7,c.r(3)+c.r(4));
                        if(!byte_nonzero(c,c.u8(c.r(7)+25))) {
                            c.put8(c.r(7)+26,c.u8(c.r(3)+26));c.set(6,1);c.put8(group()+c.r(4)+24,1);
                        }
                    }
                    c.set({{3,group()},{6,group()+c.r(4)}});
                    if(!byte_nonzero(c,c.u8(c.r(6)+25)) && !byte_nonzero(c,c.u8(c.r(6)+26))) {
                        const auto row=std::uint32_t(std::int16_t(c.r(5)))*0x2c;
                        p.call(0x10745e00,0x1073cf50,{c.r(3),c.r(3)+row,0x2c});
                        c.set(4,1);c.put8(group()+24,1);break;
                    }
                    c.set(4,c.r(4)+0x2c);c.set(5,c.r(5)+1);
                }
            }
            c.set(4,c.u32(address+0x2c28));
            if(compare_logical(c,c.r(4))!=0) {
                enter(c,0x1073cfa0,{address,c.r(4)});result(c,activate_screen(c,address,c.r(4)));c.put32(address+0x2c28,0);
            }
            break;
        }
        case -11:case -4:c.set(4,1);c.put8(group()+24,1);break;
        case -1:
            history_callback(4,0x1073cfe4,true);c.set(3,group());
            if(compare_logical(c,c.u32(c.r(3)+8))!=0){m.history()=c.u32(c.r(3)+8);c.set(4,1);c.put8(group()+24,1);}break;
        case -2:
            history_callback(4,0x1073d03c,true);c.set({{5,group()-address+16},{3,m.history()}});c.set(4,c.u32(c.r(3)+0x1f4));
            if(compare_logical(c,c.r(4))!=0 && !byte_nonzero(c,c.u8(c.r(3)+16))) {m.history()=c.r(4);c.set(4,1);c.put8(group()+24,1);}break;
        case -3:
            history_callback(4,0x1073d0a4,true);c.set({{4,group()-address+16},{3,m.history()}});
            if(compare_logical(c,c.u32(c.r(3)+0x1f8))!=0){m.history()=c.u32(c.r(3)+0x1f8);c.set(4,1);c.put8(group()+24,1);}break;
        case -9: {
            history_callback(4,0x1073d100,true);keep(c,{{17,0},{16,c.u32(group()+12)}});
            while(compare_logical(c,c.r(16))!=0 && !byte_nonzero(c,c.r(17))) {
                c.set(17,c.u8(c.r(16)+16));
                if(byte_nonzero(c,c.r(17)))c.put16(group()+20,c.i16(group()+20)-1);
                c.set(4,c.u32(c.r(16)+0x1f4));
                if(compare_logical(c,c.r(4))==0){c.put16(group()+20,0);c.set(30,std::uint32_t(-5));return true;}
                m.history()=c.r(4);c.put32(group()+12,c.r(4));c.set(3,c.u32(group()+12));c.put32(c.r(3)+0x1f8,0);
                free_node(c.r(16),16,0x1073d1cc,0x1073d1e8);c.set(16,c.u32(group()+12));
            }
            c.put8(address+0x2c5f,1);c.set({{3,missing},{4,m.history()}});p.callback(c.u32(c.r(4)+8),0x1073d244);
            c.put8(address+0x2c5f,0);c.set(30,c.r(3));if(compare(c,c.r(30))!=0)return true;
            c.set(4,1);c.put8(group()+24,1);break;
        }
        case -10: {
            history_callback(4,0x1073d290,true);c.set(4,group());c.set(20,c.u32(c.r(4)+12));
            bool removed=false;
            while(compare_logical(c,c.r(20))!=0) {
                if(byte_nonzero(c,c.u8(c.r(20)+16))) {
                    c.set(3,c.u32(c.r(20)+0x1f4));
                    if(compare_logical(c,c.r(3))==0 || !byte_nonzero(c,c.u8(c.r(3)+16))) {
                        if(compare_logical(c,c.r(20),c.u32(c.r(4)+8))==0) {c.put16(c.r(4)+20,0);c.set(30,std::uint32_t(-5));return true;}
                        c.put32(c.r(4)+12,c.r(3));c.set(3,c.u32(c.r(20)+0x1f4));c.put32(c.r(3)+0x1f8,0);
                        do {
                            if(byte_nonzero(c,c.u8(c.r(20)+16)))c.put16(group()+20,c.i16(group()+20)-1);
                            m.history()=c.u32(c.r(20)+0x1f8);
                            free_node(c.r(20),20,0x1073d36c,0x1073d388);c.set(20,m.history());
                        }while(compare_logical(c,c.r(20))!=0);
                        m.history()=c.u32(group()+12);removed=true;break;
                    }
                }
                c.set(20,c.u32(c.r(20)+0x1f4));
            }
            (void)removed;
            c.put8(address+0x2c5f,1);c.set({{3,missing},{4,m.history()}});p.callback(c.u32(c.r(4)+8),0x1073d404);
            c.put8(address+0x2c5f,0);c.set(30,c.r(3));if(compare(c,c.r(30))!=0)return true;
            c.set(4,1);c.put8(group()+24,1);break;
        }
        default:
            if(compare(c,std::int16_t(c.r(27)),-1)!=0 && !byte_nonzero(c,c.u8(group()+24))) {
                enter(c,0x1073d464,{address,0,0});result(c,refresh_screens(c,address,0,0));
            }
            break;
        }
        return false;
    }
};
}
std::uint32_t run(Cpu& c,std::uint32_t address) {
    Frame frame(c,0x1073c730,0x1a0,{.first_gpr=16});
    keep(c,{{31,address},{22,c.toc(-0x7468)},{21,c.toc(-0x7bb8)},{24,c.toc(-0x5c0c)},{29,0},
            {25,c.toc(slot::success)},{26,c.toc(slot::message)},{28,c.toc(slot::pool)}});
    Manager manager{c,address};Program p{c};Monitor monitor{c,frame,address};
    c.put8(address+0x2c61,1);c.put8(address+0x257ff5,0);c.put8(address+0x12d5ad,0);c.put8(address+0x12d568,0);
    p.call(0x1077f900,0x1073c788,{frame.at(0x3c)});
    keep(c,{{20,0},{17,0},{19,0}});
    while(compare(c,std::int16_t(c.r(17)),16)<0) {
        keep(c,{{18,address+c.r(20)},{27,c.r(19)+0x2c38},{16,0},{30,0}});
        for(;;) {
            c.set(3,extended(std::int16_t(c.r(16))));
            if(compare(c,c.r(3),c.i16(address+c.r(27)))>=0)break;
            c.set(4,c.r(18)+c.r(30));
            if(!byte_nonzero(c,c.u8(c.r(4)+24))){c.put8(c.r(4)+25,1);c.put8(c.r(4)+26,1);}
            else {
                c.put8(c.r(4)+25,0);c.put8(c.r(4)+26,0);c.put32(c.r(4)+40,c.u32(frame.at(0x3c)));
                if(compare(c,c.u8(c.r(18)+25),1)==0)p.call(0x10745e00,0x1073c7f8,{c.r(18),c.r(4),0x2c});
            }
            c.set(30,c.r(30)+0x2c);c.set(16,c.r(16)+1);
        }
        c.set(20,c.r(20)+0x2c0);c.set(19,c.r(19)+2);c.set(17,c.r(17)+1);
    }
    for(;;) {
        c.set(3,c.u32(address+0x2c64));if(compare_logical(c,c.r(3))==0)break;
        p.call(0x1073a7f0,0x1073c840,{c.r(3),c.u32(address+0x2c2c),address+0x2c5c});
        if(!byte_nonzero(c,c.r(3)))break;
    }
    for(;;) {
        c.set(30,0);manager.current()=0;
        for(;;) {
            c.set(4,extended(manager.current()));if(compare(c,c.r(4),16)>=0)break;
            const auto group=manager.group();
            if(byte_nonzero(c,c.u8(group+24))) {
                if(compare(c,manager.count(manager.current()))<=0)c.put8(group+24,0);
                else {
                    if(compare(c,std::int16_t(c.r(4)))==0){c.put8(c.r(25),0);c.put8(c.r(24),0);}
                    c.set({{5,0},{4,1}});c.put8(manager.group()+24,0);
                    p.call(0x10719560,0x1073c8d0,{monitor.screen()});
                    c.set({{4,1},{3,monitor.screen()+0x130000}});c.put8(c.r(3)-0x56c2,1);
                    monitor.history_callback(0,0x1073c904,false);
                    c.set({{4,0},{3,monitor.screen()+0x130000}});c.put8(c.r(3)-0x56c2,0);
                    c.set(3,monitor.screen()+0x130000);c.put8(c.r(3)-0x5700,c.r(4));
                    c.set(3,manager.group());c.set(5,extended(c.i16(c.r(3))));
                    if(compare(c,std::int16_t(c.r(5)))==0)p.call(0x10718fc0,0x1073c96c,{c.u32(c.r(3)+0x24),missing,missing,missing,missing});
                    else p.call(0x1071c1d0,0x1073c97c,{c.u32(c.r(3)+0x24),c.u32(address+0x2c64)});
                }
            }
            manager.current()=std::int16_t(manager.current().get()+1);
        }
        p.call(0x1076fe80,0x1073c998);c.set(4,extended(c.i16(address+0x2c5a)));
        bool process=false;
        if(compare(c,c.r(4),-1)!=0) {
            c.set(16,c.r(28)+0x1d);
            if(compare(c,manager.count(std::int32_t(c.r(4))))<=0) {c.put16(address+0x2c5a,missing);c.set(23,0);}
            else {manager.current()=std::int16_t(c.r(4));c.put16(address+0x2c5a,missing);c.set(27,missing);process=true;}
        } else {
            c.set(3,c.u32(address+0x2c64));
            if(compare_logical(c,c.r(3))!=0 && byte_nonzero(c,p.call(0x1073a7f0,0x1073c9f8,{c.r(3),c.u32(address+0x2c2c),address+0x2c5c}))) {
                c.set({{7,0},{3,address},{5,0},{16,c.r(28)+0x2f},{6,0}});
                while(compare(c,std::int16_t(c.r(7)),16)<0) {
                    if(compare(c,c.i16(address+c.r(5)+0x2c38))!=0 && compare(c,c.i16(address+0x2c5c),c.i16(c.r(3)))==0) {manager.current()=std::int16_t(c.r(7));break;}
                    c.set(5,c.r(5)+2);c.set(6,c.r(6)+0x2c0);c.set(3,c.r(3)+0x2c0);c.set(7,c.r(7)+1);
                }
                if(compare(c,std::int16_t(c.r(7)),16)<0) {
                    c.set(3,manager.group());
                    if(!byte_nonzero(c,c.u8(c.r(3)+25)))c.set(27,p.call(0x1071de80,0x1073ca94,{c.u32(c.r(3)+0x24),c.u32(address+0x2c64),extended(c.i16(address+0x2c5c)),address+0x2c2c}));
                    else c.set(27,missing);process=true;
                }
            } else {
                c.set(16,c.r(28)+0x3b);
                for(;;) {
                    c.set(29,c.r(29)+1);manager.current()=std::int16_t(c.r(29));c.set(4,extended(manager.current()));
                    if(compare(c,c.r(4),16)>=0){c.set(29,0);manager.current()=0;break;}
                    if(compare(c,manager.count(manager.current()))==0)continue;
                    if(byte_nonzero(c,c.u8(manager.group()+25)))continue;break;
                }
                c.set({{3,extended(manager.current())},{27,missing}});
                if(compare(c,std::int16_t(c.r(3)))==0) {
                    c.set(3,manager.group());if(!byte_nonzero(c,c.u8(c.r(3)+25)))c.set(27,p.call(0x10719350,0x1073cb20,{c.u32(c.r(3)+0x24)}));
                }
                process=true;
            }
        }
        if(process) {
            c.set(4,extended(manager.current()));bool valid=compare(c,std::int16_t(c.r(4)))>=0;
            if(valid)valid=compare(c,manager.count(manager.current()))>0;
            if(valid)valid=compare_logical(c,c.u32(manager.group()+16))!=0;
            if(!valid) {
                p.call(0x1077b720,0x1073cb68,{c.r(21),c.r(28)+0x48,c.r(16)});
                assertion(c,frame,0x1073cb7c,0x138,0x70,0x1c5);process=false;
            } else {
                c.set(3,address+0x260000);
                if(compare_logical(c,c.u16(c.r(3)-0x7e54))!=0) {
                    if(compare(c,c.u32(c.r(3)-0x7e58))==0)p.call(0x1077f900,0x1073cbe0,{address+0x2581a8});
                    if(compare(c,std::int16_t(c.r(27)),-1)!=0)p.call(0x1077f900,0x1073cbf8,{address+0x2581a8});
                    p.call(0x1077f900,0x1073cc00,{frame.at(0x38)});
                    c.set({{4,address+0x260000},{5,c.u32(frame.at(0x38))},{3,c.u32(address+0x2581a8)}});
                    if(compare(c,c.r(5),c.r(3)+c.u16(c.r(4)-0x7e54))>=0) {
                        c.set(3,c.u32(frame.at(0x3c)));
                        if(compare(c,c.r(5),c.r(3)+c.u16(c.r(4)-0x7e52))>=0)c.set(30,std::uint32_t(-5));
                    }
                }
                if(compare(c,c.r(30))==0) {
                    c.set(5,manager.group());
                    if(!byte_nonzero(c,c.u8(c.r(5)+25))) {
                        const auto callback=c.u32(address+0x2c1c);bool intercepted=false;
                        if(compare_logical(c,callback)!=0) {
                            c.set(4,monitor.screen());const auto control=std::int16_t(c.r(27));compare(c,control,-1);
                            auto category=control==-1?0:c.u32(c.r(4)+std::uint32_t(control)*0x17c+0xba86c);
                            const bool set=compare(c,std::int16_t(category))!=0;c.set(4,c.u32(address+0x2c20));
                            if(!set)category=extended(c.i16(address+0x25819e));
                            if(compare(c,c.r(4),std::int16_t(category))==0){c.set(3,0);p.callback(callback,0x1073ccb4);c.set(30,std::uint32_t(-4));intercepted=true;}
                        }
                        if(!intercepted){c.set({{4,c.u32(c.r(5)+16)},{3,c.r(27)}});p.callback(c.u32(c.r(4)+8),0x1073ccd0);c.set(30,c.r(3));}
                    }
                }
                c.set(3,c.u32(c.r(22)));
                if(compare_logical(c,c.r(3))!=0) {
                    enter(c,0x1073cce8);result(c,sound_pending(c,c.r(3)));c.set(4,std::uint32_t(std::int8_t(c.r(3))));
                    if(compare(c,c.r(4),-1)!=0){enter(c,0x1073ccfc,{c.u32(c.r(22))});result(c,sound_stop(c,c.r(3),c.r(4)));}
                }
                while(monitor.event()) {}
                c.set({{5,0},{23,1},{3,0},{4,0}});
                for(unsigned index=0;compare(c,std::int16_t(c.r(5)),16)<0;++index) {
                    if(compare(c,c.i16(address+c.r(3)+0x2c38))!=0 && !byte_nonzero(c,c.u8(address+c.r(4)+26)))c.set(23,0);
                    c.set(3,c.r(3)+2);c.set(4,c.r(4)+0x2c0);c.set(5,index+1);
                }
                c.set(3,address+0x260000);const auto callback=c.u32(c.r(3)-0x7f08);
                if(compare_logical(c,callback)!=0)p.callback(callback,0x1073d4c8);
            }
        }
        if(byte_nonzero(c,c.r(23)) || compare(c,c.r(30),-7)==0)break;
    }
    p.call(0x10727890,0x1073d4e8,{c.r(21),c.r(28)+0x83});
    auto out=packet(c,frame,address);c.put32(out.position,0);
    out.integer(1,7,0x1073d518,0xd8,0x58,0x85);out.integer(4,0,0x1073d598,0xb8,0x50,0x88);out.integer(4,0,0x1073d618,0x98,0x48,0x88);
    c.set(22,extended(std::int16_t(p.call(0x1077dbd0,0x1073d678,{c.r(21)})+1)));
    out.integer(2,c.r(22),0x1073d6a8,0x78,0x40,0x86);out.string(c.r(21),c.r(22),0x1073d71c);
    keep(c,{{18,0},{19,0},{16,address}});
    while(compare(c,std::int16_t(c.r(18)),16)<0) {
        if(compare(c,c.i16(address+c.r(19)+0x2c38))>0) {
            c.set(6,extended(c.i16(c.r(16))));
            if(compare(c,std::int16_t(c.r(6)))==0)p.call(0x1071de80,0x1073d76c,{address+0x2c68,c.u32(address+0x2c64),0,address+0x2c2c});
            else {
                p.call(0x1073a4c0,0x1073d780,{c.u32(address+0x2c64),c.u32(out.data),c.u32(out.position)});
                if(!byte_nonzero(c,c.r(3)))assertion(c,frame,0x1073d79c,0x118,0x68,0x363);
            }
        }
        c.set(19,c.r(19)+2);c.set(16,c.r(16)+0x2c0);c.set(18,c.r(18)+1);
    }
    manager.current()=0;
    for(;;) {
        c.set(4,extended(manager.current()));if(compare(c,c.r(4),16)>=0)break;
        if(compare(c,manager.count(manager.current()))!=0) {
            monitor.close_group_state(0);keep(c,{{18,1},{19,0x2c}});
            for(;;) {
                c.set({{5,extended(manager.current())},{4,extended(std::int16_t(c.r(18)))},{3,std::uint32_t(std::int32_t(manager.current()))*2}});
                if(compare(c,c.r(4),manager.count(manager.current()))>=0)break;
                p.call(0x10745e00,0x1073d908,{manager.group(),manager.group()+c.r(19),0x2c});
                monitor.close_group_state(0xfc);c.set(19,c.r(19)+0x2c);c.set(18,c.r(18)+1);
            }
        }
        c.set(3,extended(manager.current()));manager.current()=std::int16_t(c.r(3)+1);
    }
    c.set({{5,0},{4,1}});manager.current()=0;c.put8(address+0x12d568,1);c.put8(address+0x2c61,0);
    return c.r(30)==std::uint32_t(-5);
}
}

#include "screen_manager_packets.hpp"
#include "screen_manager_sound.hpp"

namespace cm::aot::screen_manager {
using namespace detail;
std::uint32_t send_shared_value(Cpu& c,std::uint32_t address,std::uint32_t id,std::uint32_t value) {
    Frame frame(c,0x107403c0,0x100,{.first_gpr=23});
    Kept<28> buffer(c,c.toc(-0x5c04));Kept<24> identifier(c,id);Kept<23> object(c,address);
    keep(c,{{29,c.toc(slot::success)},{30,c.toc(slot::message)},{27,buffer+4},{31,c.toc(slot::pool)},{26,buffer+8},{25,value}});
    Packet out{c,frame,buffer,buffer+4,buffer+8};c.put32(out.position,0);
    out.integer(1,15,0x10740424,0x98,0x48,0x85);out.integer(2,identifier,0x107404a4,0x78,0x40,0x86);
    out.integer(4,c.r(25),0x10740528,0x58,0x38,0x88);
    c.set({{3,c.toc(-0x7bb0)+0x130000},{4,c.u32(object+0x24)}});
    if(compare_logical(c,c.r(4),c.r(3)-0x2950)==0) {
        Program{c}.call(0x1073a4c0,0x107405ac,{c.u32(object+0x2c64),c.u32(out.data),c.u32(out.position),extended(c.i16(c.toc(-0x5c24)))});
        if(!byte_nonzero(c,c.r(3)))assertion(c,frame,0x107405c8,0xb8,0x50,0xa09);
    }
    return c.r(3);
}
std::uint32_t send_title(Cpu& c,std::uint32_t address,std::uint32_t text) {
    Frame frame(c,0x107439b0,0xd0,{.first_gpr=26});
    Kept<27> source(c,text);keep(c,{{29,c.toc(slot::success)},{30,c.toc(slot::message)},{26,address},{31,c.toc(slot::pool)}});
    if(compare(c,source)==0)source=c.r(31)+0xb6;
    auto out=packet(c,frame,address);c.put32(out.position,0);
    out.integer(1,25,0x10743a0c,0x70,0x40,0x85);
    Kept<28> size(c,extended(std::int16_t(Program{c}.call(0x1077dbd0,0x10743a6c,{source})+1)));
    out.integer(2,size,0x10743a9c,0x50,0x38,0x86);out.string(source,size,0x10743b10);
    send_current(c,frame,address,{0x10743b40,0,0x10743b54,0x10743b74,0x90,0x48,0xd00});return c.r(3);
}
std::uint32_t play_sound(Cpu& c,std::uint32_t address,std::uint32_t result_address,std::uint32_t side,
                       std::uint32_t filename,std::uint32_t repeat) {
    Frame frame(c,0x10742740,0x150,{.first_gpr=23});
    Kept<31> object(c,address);Kept<23> sound_global(c,c.toc(-0x7468));
    keep(c,{{28,c.toc(slot::success)},{24,result_address},{29,c.toc(slot::message)},
            {25,side},{30,c.toc(slot::pool)},{26,filename},{27,repeat}});
    c.put8(result_address,missing);Manager manager{c,address};Program p{c};
    if(compare(c,c.i16(manager.group()))==0) {
        c.set(3,c.u32(sound_global));
        if(compare_logical(c,c.r(3))!=0) {
            enter(c,0x107427ac,{c.r(3),c.r(25),c.r(26),c.r(27)});result(c,sound_load(c,c.r(3),c.r(4),c.r(5),c.r(6)));
            c.put8(c.r(24),c.r(3));enter(c,0x107427c0,{c.u32(sound_global),c.u8(c.r(24)),100});
            result(c,sound_set_volume(c,c.r(3),c.r(4),100));
        }
        return c.r(3);
    }
    auto out=packet(c,frame,address);c.put32(out.position,0);
    out.integer(1,10,0x107427f4,0xe8,0x58,0x85);out.integer(4,c.r(24),0x10742874,0xc8,0x50,0x89);
    out.integer(1,c.r(25),0x107428f0,0xa8,0x48,0x85);out.integer(1,c.r(27),0x1074296c,0x88,0x40,0x85);
    c.set(27,extended(std::int16_t(p.call(0x1077dbd0,0x107429c8,{c.r(26)})+1)));
    out.integer(2,c.r(27),0x107429f8,0x68,0x38,0x86);out.string(c.r(26),c.r(27),0x10742a6c);
    p.call(0x1073a4c0,0x10742a94,{c.u32(address+0x2c64),c.u32(out.data),c.u32(out.position),extended(c.i16(manager.group()))});
    if(!byte_nonzero(c,c.r(3)))assertion(c,frame,0x10742ab0,0x108,0x60,0xbae);
    return c.r(3);
}
std::uint32_t sound_status(Cpu& c,std::uint32_t address,std::uint32_t index) {
    c.set(5,std::uint32_t(std::int8_t(index)));compare(c,c.r(5),-1);
    Frame frame(c,0x10742b00,0x40,{});if(std::int8_t(index)==-1)return 0;
    Manager manager{c,address};c.set(6,extended(manager.current()));
    if(compare(c,c.i16(manager.group()))==0) {
        c.set(3,c.u32(c.toc(-0x7468)));
        if(compare_logical(c,c.r(3))==0)return 0;
        enter(c,0x10742b50);return std::uint32_t(std::int8_t(sound_finished(c,c.r(3),index)));
    }
    c.set(3,address+0x260000+std::uint32_t(std::int32_t(manager.current()))*10+std::uint32_t(std::int8_t(index)));
    return c.u8(c.r(3)-0x7f04);
}
std::uint32_t stop_sound(Cpu& c,std::uint32_t address,std::uint32_t index) {
    Frame frame(c,0x10742b80,0xd0,{.first_gpr=27});
    Kept<27> object(c,address);c.set(3,c.toc(-0x7468));
    keep(c,{{29,c.toc(slot::success)},{28,index},{30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    Manager manager{c,address};Program p{c};
    if(compare(c,c.i16(manager.group()))==0) {
        const bool all=compare(c,std::int8_t(c.r(28)),-1)==0;c.set(3,c.u32(c.r(3)));
        if(compare_logical(c,c.r(3))!=0) {
            enter(c,all?0x10742bd8:0x10742bec);
            result(c,all?sound_clear(c,c.r(3)):sound_stop(c,c.r(3),c.r(4)));
        }
    } else {
        auto out=packet(c,frame,address);c.put32(out.position,0);
        out.integer(1,11,0x10742c20,0x70,0x40,0x85);out.integer(1,c.r(28),0x10742ca0,0x50,0x38,0x85);
        p.call(0x1073a4c0,0x10742d10,{c.u32(address+0x2c64),c.u32(out.data),c.u32(out.position),extended(c.i16(manager.group()))});
        if(!byte_nonzero(c,c.r(3)))assertion(c,frame,0x10742d2c,0x90,0x48,0xbe1);
    }
    c.set(5,std::uint32_t(std::int8_t(c.r(28))));
    if(compare(c,c.r(5),-1)==0) {
        for(unsigned batch=0;batch<2;++batch) {
            const auto offset=std::uint32_t(std::int32_t(manager.current()))*10+0x2580fc;
            for(unsigned i=0;i<5;++i)c.put8(address+offset+batch*5+i,0);
            c.set({{3,(batch+1)*5},{4,offset+batch*5+4},{5,std::uint32_t(std::int32_t(manager.current()))*10},
                   {6,std::uint32_t(std::int32_t(manager.current()))*10},{7,std::uint32_t(std::int32_t(manager.current()))*10},
                   {8,std::uint32_t(std::int32_t(manager.current()))*10},{9,std::uint32_t(std::int32_t(manager.current()))*10}});
            compare(c,std::int16_t(c.r(3)),10);
        }
    } else {
        c.set({{3,address+0x260000+std::uint32_t(std::int32_t(manager.current()))*10+c.r(5)},{4,0}});c.put8(c.r(3)-0x7f04,0);
    }
    return c.r(3);
}
std::uint32_t remember_sound(Cpu& c,std::uint32_t address,std::uint32_t index,std::uint32_t output) {
    c.set(6,std::uint32_t(std::int8_t(index)));c.put8(output,index);
    if(compare(c,c.r(6),-1)==0)return address;
    c.set({{3,address+0x260000+std::uint32_t(std::int32_t(Manager{c,address}.current()))*10+c.r(6)},{4,1}});
    c.put8(c.r(3)-0x7f04,1);return c.r(3);
}
std::uint32_t forget_sound(Cpu& c,std::uint32_t address,std::uint32_t index) {
    c.set({{5,extended(Manager{c,address}.current())},{6,address+0x260000},{4,0}});
    c.set(3,c.r(6)+c.r(5)*10+std::uint32_t(std::int8_t(index)));c.put8(c.r(3)-0x7f04,0);return c.r(3);
}
std::uint32_t refresh_screens(Cpu& c,std::uint32_t address,std::uint32_t all,std::uint32_t exclude_local) {
    Frame frame(c,0x10742ec0,0xd0,{.first_gpr=24});
    Kept<26> object(c,address);keep(c,{{29,c.toc(slot::success)},{30,c.toc(slot::message)},
                                    {27,all},{31,c.toc(slot::pool)},{28,exclude_local}});
    Manager manager{c,address};Program p{c};c.set(3,c.u32(manager.group()+0x24)+0x130000);
    if(byte_nonzero(c,c.u8(c.r(3)-0x56c2)))return c.r(3);
    auto out=packet(c,frame,address);c.put32(out.position,0);out.integer(1,9,0x10742f38,0x50,0x38,0x85);
    if(byte_nonzero(c,c.r(27))) {
        Kept<27> group(c,address);Kept<24> index(c,0);Kept<25> count_offset(c,0);
        while(compare(c,std::int16_t(std::uint32_t(index)),16)<0) {
            if(compare(c,c.i16(object+count_offset+0x2c38))>0) {
                c.set(6,extended(c.i16(group)));
                if(compare(c,std::int16_t(c.r(6)))==0) {
                    if(!byte_nonzero(c,c.r(28)))p.call(0x1071de80,0x10742fe0,{object+0x2c68,c.u32(object+0x2c64),0,object+0x2c2c});
                } else {
                    p.call(0x1073a4c0,0x10742ff4,{c.u32(object+0x2c64),c.u32(out.data),c.u32(out.position)});
                    if(!byte_nonzero(c,c.r(3)))assertion(c,frame,0x10743010,0x90,0x48,0xc3e);
                }
            }
            count_offset=count_offset+2;group=group+0x2c0;index=index+1;
        }
    } else {
        c.set(6,extended(c.i16(manager.group())));
        if(compare(c,std::int16_t(c.r(6)))==0) {
            if(!byte_nonzero(c,c.r(28)))p.call(0x1071de80,0x10743098,{object+0x2c68,c.u32(object+0x2c64),0,object+0x2c2c});
        } else {
            p.call(0x1073a4c0,0x107430ac,{c.u32(object+0x2c64),c.u32(out.data),c.u32(out.position)});
            if(!byte_nonzero(c,c.r(3)))assertion(c,frame,0x107430c8,0x70,0x40,0xc52);
        }
    }
    return c.r(3);
}
std::uint32_t send_message(Cpu& c,std::uint32_t address,std::uint32_t text,std::uint32_t key,std::uint32_t value,std::uint32_t selected_group) {
    Frame frame(c,0x10743120,0x150,{.first_gpr=24});
    Kept<26> source(c,text);Kept<29> object(c,address);keep(c,{{28,c.toc(slot::success)},{24,key},
        {30,c.toc(slot::message)},{25,value},{31,c.toc(slot::pool)},{27,selected_group}});Program p{c};Manager manager{c,address};
    auto out=packet(c,frame,address);c.put32(out.position,0);
    out.integer(1,7,0x10743180,0xc8,0x50,0x85);out.integer(4,c.r(24),0x10743200,0xa8,0x48,0x88);
    out.integer(4,c.r(25),0x1074327c,0x88,0x40,0x88);
    c.set(25,extended(std::int16_t(p.call(0x1077dbd0,0x107432d8,{source})+1)));
    out.integer(2,c.r(25),0x10743308,0x68,0x38,0x86);out.string(source,c.r(25),0x1074337c);
    c.set(3,extended(std::int16_t(c.r(27))));
    if(compare(c,c.r(3),-1)!=0) {
        const auto group=std::int32_t(c.r(3));c.set(6,extended(c.i16(manager.group(group))));
        if(compare(c,std::int16_t(c.r(6)))==0)p.call(0x1071de80,0x107433b8,{object+0x2c68,c.u32(object+0x2c64),0,object+0x2c2c});
        else {
            c.set(3,object+std::uint32_t(group)*2);
            if(compare(c,c.i16(c.r(3)+0x2c38))>0) {
                p.call(0x1073a4c0,0x107433e0,{c.u32(object+0x2c64),c.u32(out.data),c.u32(out.position)});
                if(!byte_nonzero(c,c.r(3)))assertion(c,frame,0x107433fc,0x108,0x60,0xc72);
            }
        }
    } else {
        c.set({{24,object},{26,0},{25,0}});
        do {
            if(compare(c,c.i16(object+c.r(25)+0x2c38))>0) {
                c.set(6,extended(c.i16(c.r(24))));
                if(compare(c,std::int16_t(c.r(6)))==0)p.call(0x1071de80,0x10743478,{object+0x2c68,c.u32(object+0x2c64),0,object+0x2c2c});
                else {
                    p.call(0x1073a4c0,0x1074348c,{c.u32(object+0x2c64),c.u32(out.data),c.u32(out.position)});
                    if(!byte_nonzero(c,c.r(3)))assertion(c,frame,0x107434a8,0xe8,0x58,0xc8a);
                }
            }
            c.set(26,c.r(26)+1);c.set(24,c.r(24)+0x2c0);compare(c,c.r(26),16);c.set(25,c.r(25)+2);
        }while(std::int32_t(c.r(26))<16);
    }
    return c.r(3);
}
}

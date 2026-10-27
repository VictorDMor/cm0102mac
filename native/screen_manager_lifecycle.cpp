#include "screen_manager_packets.hpp"

namespace cm::aot::screen_manager {
using namespace detail;
std::uint32_t construct(Cpu& c,std::uint32_t address) {
    Frame frame(c,0x1073c1b0,0x40,{});Program p{c};c.put32(frame.at(0x58),address);
    auto object=[&]{return c.u32(frame.at(0x58));};
    p.call(0x1073ae10,0x1073c1d4,{object()+0x2c2c,0xc350});
    p.call(0x10718b40,0x1073c1e4,{object()+0x2c68,0});
    p.call(0x10718b40,0x1073c1f8,{object()+0x12d6b0,1});
    c.set({{3,0},{4,0},{5,0}});
    for(unsigned offset:{0x2c64,0x2581b0,0x2581b4,0x2580f8,0x2c28,0x2581a8,0x2581a0,0x2581a4})c.put32(object()+offset,0);
    for(unsigned offset:{0x2c61,0x2c5e,0x25819c,0x2c5f})c.put8(object()+offset,0);
    for(unsigned offset:{0x2581ac,0x2581ae,0x2c58,0x25819e})c.put16(object()+offset,0);
    c.put16(object()+0x2c5a,missing);
    for(unsigned group=0;compare(c,std::int16_t(group),16)<0;++group) {
        c.put16(object()+0x2c38+group*2,0);
        for(unsigned batch=0;batch<2;++batch) {
            for(unsigned i=0;i<5;++i)c.put8(object()+0x2580fc+group*10+batch*5+i,0);
            c.set({{6,(batch+1)*5},{7,object()+0x2580fc+batch*5+4},{8,object()+0x2580fc+batch*5+2},
                   {9,object()+0x2580fc+batch*5+1},{10,object()+0x2580fc+batch*5}});
            compare(c,std::int16_t(c.r(6)),10);
        }
        c.set({{3,(group+1)*2},{4,(group+1)*10},{5,group+1}});
    }
    c.put16(object()+0x2c38,1);
    c.put16(object(),0);for(unsigned offset:{4,8,12,16,0x20,0x2c1c,0x2c20,0x2c24,0x2c00,0x2c04,0x2c0c,0x2c10,0x2c14})c.put32(object()+offset,0);
    c.put16(object()+0x14,0);c.put16(object()+0x16,0);c.put8(object()+0x18,1);
    c.put32(object()+0x24,object()+0x2c68);c.put16(object()+0x1c,missing);c.put8(object()+0x2c08,0);c.put16(object()+0x2c18,0);
    c.set({{3,object()},{4,0},{5,object()},{8,object()},{9,object()},{10,object()}});return c.r(3);
}
std::uint32_t destroy(Cpu& c,std::uint32_t address,std::uint32_t disposal) {
    Frame frame(c,0x1073c460,0x40,{.first_gpr=30});Kept<31> mode(c,disposal);Kept<30> object(c,address);Program p{c};
    if(compare(c,object)!=0) {
        enter(c,0x1073c484);result(c,reset(c,object));
        enter(c,0x1073c48c,{object});result(c,disconnect(c,object));
        p.call(0x10718ba0,0x1073c49c,{object+0x12d6b0,missing});
        p.call(0x10718ba0,0x1073c4a8,{object+0x2c68,missing});
        p.call(0x1073aee0,0x1073c4b4,{object+0x2c2c,missing});
        if(compare(c,std::int16_t(std::uint32_t(mode)))>0)p.call(0x107735a0,0x1073c4c4,{object});
    }
    return object;
}
std::uint32_t reset(Cpu& c,std::uint32_t address) {
    Frame frame(c,0x1073c4e0,0x60,{.first_gpr=24});Kept<28> group_offset(c,0);Kept<24> object(c,address);
    Kept<27> count_offset(c,0);Kept<26> group_index(c,0);Manager manager{c,address};Program p{c};
    while(compare(c,std::int16_t(std::uint32_t(group_index)),16)<0) {
        Kept<30> history_offset(c,group_offset+0x10);Kept<31> count(c,count_offset+0x2c38);
        Kept<25> row(c,0);Kept<29> offset(c,0);
        for(;;) {
            c.set(3,extended(std::int16_t(std::uint32_t(row))));
            if(compare(c,c.r(3),c.i16(object+count))>=0)break;
            const bool additional=compare(c,std::int16_t(std::uint32_t(row)))!=0;
            manager.current()=std::int16_t(std::uint32_t(group_index));
            if(additional)p.call(0x10745e00,0x1073c534,{object+group_offset+offset,object+group_offset,0x2c});
            c.set(3,c.u32(object+history_offset));
            if(compare_logical(c,c.r(3))!=0) {
                const auto target=c.u32(c.r(3)+4);
                if(compare_logical(c,target)!=0)p.callback(target,0x1073c550);
            }
            enter(c,0x1073c55c,{object});result(c,clear_history(c,object));
            offset=offset+0x2c;row=row+1;
        }
        group_offset=group_offset+0x2c0;count_offset=count_offset+2;group_index=group_index+1;
    }
    return c.r(3);
}
std::uint32_t disconnect(Cpu& c,std::uint32_t address) {
    Frame frame(c,0x1073f4e0,0x40,{.first_gpr=31});Kept<31> object(c,address);
    if(compare_logical(c,c.u32(address+0x2c64))!=0) {
        enter(c,0x1073f504);result(c,stop_network(c,object));
        Program{c}.call(0x1073a120,0x1073f510,{c.u32(object+0x2c64),1});c.put32(object+0x2c64,0);
    }
    return c.r(3);
}
std::uint32_t connect(Cpu& c,std::uint32_t address,std::uint32_t opened,std::uint32_t closed) {
    Frame frame(c,0x1073f450,0x50,{.first_gpr=30});Kept<31> object(c,address);Program p{c};
    c.set(6,object+0x260000);c.put32(c.r(6)-0x7e50,opened);c.put32(c.r(6)-0x7e4c,closed);
    enter(c,0x1073f478);result(c,disconnect(c,object));
    Kept<30> network(c,p.call(0x10773520,0x1073f484,{0xc39b0}));
    if(compare(c,network)!=0)p.call(0x10739f70,0x1073f49c,{network,frame.at(0x38),c.toc(-0x5b9c),c.toc(-0x5ba0)});
    c.put32(object+0x2c64,network);
    if(!byte_nonzero(c,c.u8(frame.at(0x38)))) {
        p.call(0x1073a120,0x1073f4b8,{c.u32(object+0x2c64),1});c.put32(object+0x2c64,0);
    }
    return c.u8(frame.at(0x38));
}
std::uint32_t network_poll(Cpu& c,std::uint32_t address) {
    Frame frame(c,0x1073c700,0x40,{});return Program{c}.call(0x1073a220,0x1073c714,{c.u32(address+0x2c64)});
}
std::uint32_t stop_network(Cpu& c,std::uint32_t address) {
    Frame frame(c,0x1073c5a0,0x80,{.first_gpr=26});keep(c,{{29,c.toc(slot::message)},{31,address},{30,c.toc(slot::pool)}});
    Program p{c};
    if(compare_logical(c,c.u32(address+0x2c64))!=0) {
        p.call(0x10718ef0,0x1073c5d4,{address+0x12d6b0});
        if(byte_nonzero(c,c.u8(address+0x2c5e))) {
            c.put8(address+0x2c5e,0);auto out=packet(c,frame,address);c.put32(out.position,0);
            out.integer(1,4,0x1073c614,0x40,0x38,0x85);
            p.call(0x1073a4c0,0x1073c684,{c.u32(address+0x2c64),c.u32(address+0x2c2c),c.u32(address+0x2c30),0});
            Kept<26> index(c,1);Kept<29> offset(c,0x2c0);Kept<28> count_offset(c,2);
            while(compare(c,std::int16_t(std::uint32_t(index)),16)<0) {
                Kept<27> group(c,address+offset);Kept<30> count(c,count_offset+0x2c38);
                while(compare(c,c.i16(address+count))>0) {
                    enter(c,0x1073c6b0,{address,c.u32(group+4),missing});result(c,close_screen(c,address,c.r(4),missing));
                    p.call(0x1073a480,0x1073c6bc,{c.u32(address+0x2c64),extended(c.i16(group))});
                }
                offset=offset+0x2c0;count_offset=count_offset+2;index=index+1;
            }
        }
    }
    return c.r(3);
}
std::uint32_t initialize(Cpu& c) {
    Frame frame(c,0x10744640,0x40,{});enter(c,0x10744654,{c.toc(-0x7bb0)});result(c,construct(c,c.r(3)));
    return Program{c}.call(0x10772df0,0x10744660,{c.r(3),c.toc(-0x5ba4),c.toc(-0x3e64)});
}
}

#include "screen_manager_packets.hpp"

namespace cm::aot::screen_manager {
using namespace detail;
std::uint32_t open_screen(Cpu& c,std::uint32_t id) {
    Frame frame(c,0x1073f530,0x50,{.first_gpr=26});Kept<31> address(c,c.toc(-0x7bb0));Kept<29> screen_id(c,id);
    Manager manager{c,address};Program p{c};
    if(compare_logical(c,c.u32(address+0x2c64))!=0 && !byte_nonzero(c,c.u8(address+0x2c5e)))return c.r(3);
    c.set({{3,address},{4,extended(std::int16_t(std::uint32_t(screen_id)))},{5,0}});Kept<30> group(c,0);
    while(compare(c,std::int16_t(std::uint32_t(group)),16)<0) {
        if(compare(c,manager.count(group))==0)break;
        if(compare(c,c.r(4),c.i16(c.r(3)))==0) {
            c.set(4,extended(std::int16_t(std::uint32_t(group))));c.set(3,extended(manager.count(group)));
            if(compare(c,c.r(3),16)>=0)return c.r(3);
            Kept<26> row(c,c.r(3));Kept<28> offset(c,c.r(3)*0x2c);Kept<27> base(c,manager.group(group));
            while(compare(c,std::int16_t(std::uint32_t(row)))>0) {
                c.set(4,extended(std::int16_t(std::uint32_t(row))));
                p.call(0x10745e00,0x1073f5dc,{base+offset,base+(c.r(4)-1)*0x2c,0x2c});
                offset=offset-0x2c;row=row-1;
            }
            break;
        }
        c.set(5,c.r(5)+2);c.set(3,c.r(3)+0x2c0);group=group+1;
    }
    c.set(6,extended(std::int16_t(std::uint32_t(group))));
    if(compare(c,c.r(6),16)>=0)return c.r(3);
    const auto base=manager.group(std::int32_t(c.r(6)));
    c.set({{4,0},{5,address+std::uint32_t(group)*2}});const bool local=compare(c,std::int16_t(std::uint32_t(screen_id)))==0;
    c.set(3,extended(manager.count(group)));manager.count(group)=std::int16_t(c.r(3)+1);c.set(3,base);c.set(7,base+0x18);
    c.put16(base,screen_id);for(unsigned off:{4,8,12,16})c.put32(base+off,0);c.put16(base+20,0);c.put16(base+22,0);c.put8(base+24,1);
    c.set(4,local?address+0x2c68:address+0x12d6b0);c.set(6,base);c.set(5,0);
    c.put32(base+0x24,c.r(4));Kept<27> current_address(c,address+0x2c58);
    c.put8(base+0x19,0);c.set(8,base+0x19);c.put16(base+0x1c,missing);c.put32(base+0x20,0);c.put32(base+0x28,missing);
    Kept<28> previous(c,extended(manager.current()));manager.current()=std::int16_t(std::uint32_t(group));
    c.set(4,address+0x260000);
    const auto target=c.u32(address+0x2581b0);
    if(compare_logical(c,target)!=0) {
        c.set(3,screen_id);p.callback(target,0x1073f6c0);c.put16(address+0x2c5a,group);
    } else {
        c.put8(c.r(7),c.r(5));c.put8(c.r(8),1);
        p.call(0x10745e00,0x1073f6e4,{c.r(3),c.r(3)+0x2c,0x2c});
    }
    c.put16(current_address,previous);return c.r(3);
}
std::uint32_t close_group(Cpu& c,std::uint32_t id) {
    Frame frame(c,0x1073f700,0x60,{.first_gpr=24});Kept<29> address(c,c.toc(-0x7bb0));
    if(byte_nonzero(c,c.u8(address+0x2c5e))) {
        Kept<28> offset(c,0);Kept<30> selected(c,extended(std::int16_t(id)));Kept<27> count_offset(c,0);
        Kept<24> index(c,0);Kept<31> zero(c,0);
        while(compare(c,std::int16_t(std::uint32_t(index)),16)<0) {
            Kept<26> group(c,address+offset);
            if(compare(c,selected,c.i16(group))==0) {
                Kept<25> count(c,count_offset+0x2c38);
                while(compare(c,c.i16(address+count))>0) {
                    enter(c,0x1073f760,{address,c.u32(group+4),index});result(c,close_screen(c,address,c.r(4),index));
                }
                c.put16(address+count,zero);
            }
            offset=offset+0x2c0;count_offset=count_offset+2;index=index+1;
        }
    }
    return c.r(3);
}
std::uint32_t configure_screen(Cpu& c,std::uint32_t address,std::uint32_t opened,std::uint32_t closed,std::uint32_t key) {
    Frame frame(c,0x1073f860,0x50,{.first_gpr=29});Kept<31> screen_key(c,key);Kept<30> on_open(c,opened);Kept<29> object(c,address);
    const bool supplied=compare(c,screen_key)!=0;Manager manager{c,address};
    if(supplied) {
        c.put32(object+0x2581b4,closed);c.put32(object+0x2581b0,0);
        enter(c,0x1073f8a8,{extended(c.i16(manager.group()))});result(c,open_screen(c,c.r(3)));
        c.set({{3,object+0x260000},{4,manager.group()}});c.put32(c.r(4)+0x30,screen_key);c.put32(c.r(3)-0x7e50,on_open);
    } else {
        c.put32(object+0x2581b0,on_open);c.put32(object+0x2581b4,closed);
        enter(c,0x1073f8e0,{extended(c.i16(manager.group()))});result(c,open_screen(c,c.r(3)));
    }
    return c.r(3);
}
std::uint32_t close_screen(Cpu& c,std::uint32_t address,std::uint32_t key,std::uint32_t selected_group) {
    Frame frame(c,0x1073f900,0x90,{.first_gpr=21});Kept<29> global(c,c.toc(-0x7bb0));Kept<23> object(c,address);
    keep(c,{{30,c.toc(slot::message)},{31,c.toc(slot::pool)}});Manager manager{c,address};Manager global_manager{c,global};Program p{c};
    Kept<25> group(c,selected_group);Kept<26> row(c,0);
    if(compare(c,std::int16_t(selected_group),-1)==0) {
        group=0;c.set({{6,0},{7,0}});
        bool found=false;
        while(compare(c,std::int16_t(std::uint32_t(group)),16)<0) {
            c.set(3,extended(c.i16(object+c.r(6)+0x2c38)));row=extended(std::int16_t(c.r(3)-1));c.set(5,row*0x2c);
            while(compare(c,std::int16_t(std::uint32_t(row)))>=0) {
                if(compare_logical(c,c.u32(object+c.r(7)+c.r(5)+4),key)==0){found=true;break;}
                c.set(5,c.r(5)-0x2c);row=row-1;
            }
            if(found)break;c.set(6,c.r(6)+2);c.set(7,c.r(7)+0x2c0);group=group+1;
        }
        if(!found)return c.r(3);
    }
    c.set(3,extended(manager.current()));if(compare(c,c.r(3),std::int16_t(std::uint32_t(group)))==0)manager.current()=0;
    Kept<28> current_address(c,global+0x2c58);const bool first=compare(c,std::int16_t(std::uint32_t(row)))==0;
    Kept<24> previous(c,extended(global_manager.current()));global_manager.current()=std::int16_t(std::uint32_t(group));
    if(first) {
        c.set(3,c.u32(manager.group(std::int16_t(std::uint32_t(group)))+0x10));
        if(compare_logical(c,c.r(3))!=0) {const auto callback=c.u32(c.r(3)+4);if(compare_logical(c,callback)!=0)p.callback(callback,0x1073f9f4);}
    }
    if(compare(c,std::int16_t(std::uint32_t(row)))!=0) {
        const auto base=manager.group(std::int16_t(std::uint32_t(group)));
        p.call(0x10745e00,0x1073fa20,{base+std::uint32_t(std::int16_t(std::uint32_t(row)))*0x2c,base,0x2c});
    }
    enter(c,0x1073fa28,{global});result(c,clear_history(c,global));
    Kept<22> group_index(c,extended(std::int16_t(std::uint32_t(group))));Kept<27> offset(c,group_index*0x2c0);
    p.call(0x10718f30,0x1073fa40,{c.u32(object+offset+0x24),group_index});
    c.set(3,global+0x260000);const auto callback=c.u32(c.r(3)-0x7e4c);
    if(compare_logical(c,callback)!=0){c.set(3,extended(c.i16(object+offset)));p.callback(callback,0x1073fa58);}
    row=group_index*2;c.put16(current_address,previous);group_index=global+offset;
    Kept<21> index(c,1);previous=row+0x2c38;current_address=0x2c;
    for(;;) {
        c.set(3,extended(std::int16_t(std::uint32_t(index))));
        if(compare(c,c.r(3),c.i16(global+previous))>=0)break;
        p.call(0x10739ef0,0x1073fa94,{group_index+(c.r(3)-1)*0x2c,group_index+current_address,0x2c});
        current_address=current_address+0x2c;index=index+1;
    }
    const auto last=(c.r(3)-1)*0x2c;current_address=global+offset;
    p.call(0x10778300,0x1073fac8,{current_address+last,0,0x2c});
    c.set(8,global+row);c.set(3,extended(c.i16(c.r(8)+0x2c38))-1);const bool any=compare(c,std::int16_t(c.r(3)))!=0;
    c.set(8,c.r(8)+0x2c38);c.put16(c.r(8),c.r(3));
    if(any){c.put8(current_address+0x18,1);return c.r(3);}
    if(compare(c,std::int16_t(std::uint32_t(group)))==0) {
        c.put16(c.r(8),c.i16(c.r(8))+1);c.set({{7,0},{5,1},{4,global+0x2c68}});
        c.put16(current_address,0);for(unsigned off:{4,8,12,16,0x20})c.put32(current_address+off,0);
        c.put16(current_address+20,0);c.put16(current_address+22,0);c.put8(current_address+24,1);c.put32(current_address+0x24,c.r(4));
        c.put8(current_address+0x19,0);c.put16(current_address+0x1c,missing);c.put32(current_address+0x28,missing);
        enter(c,0x1073fb60,{object,c.u32(object+0x2c00),c.u32(object+0x2c04),c.u8(object+0x2c08),c.u32(object+0x2c0c),c.u32(object+0x2c10)});
        return append_history(c,object,c.r(4),c.r(5),c.r(6),c.r(7),c.r(8));
    }
    auto out=packet(c,frame,object);c.put32(out.position,0);out.integer(1,4,0x1073fb94,0x40,0x38,0x85);
    return p.call(0x1073a4c0,0x1073fc04,{c.u32(object+0x2c64),c.u32(object+0x2c2c),c.u32(object+0x2c30),group});
}
std::uint32_t activate_screen(Cpu& c,std::uint32_t address,std::uint32_t key) {
    Frame frame(c,0x1073fc20,0x40,{.first_gpr=31});Kept<31> object(c,address);Manager manager{c,address};
    std::uint32_t row=0;c.set({{8,0},{5,0}});
    for(;;++row) {
        c.set({{6,extended(manager.current())},{3,extended(std::int16_t(row))},{7,std::uint32_t(std::int32_t(manager.current()))*2}});
        if(compare(c,c.r(3),manager.count(manager.current()))>=0)break;
        if(compare_logical(c,c.u32(manager.group()+row*0x2c+4),key)==0)break;
        c.set({{5,(row+1)*0x2c},{8,row+1}});
    }
    c.set({{3,address+c.r(7)},{4,extended(std::int16_t(row))}});
    if(compare(c,c.r(4),manager.count(manager.current()))<0) {
        auto base=manager.group();c.set({{4,base},{3,base+std::uint32_t(std::int16_t(row))*0x2c}});
        if(!byte_nonzero(c,c.u8(c.r(3)+0x19)) && !byte_nonzero(c,c.u8(c.r(3)+0x1a))) {
            if(compare(c,std::int16_t(row))!=0)Program{c}.call(0x10745e00,0x1073fccc,{c.r(3),c.r(4),0x2c});
            c.set({{4,1},{3,manager.group()}});c.put8(c.r(3)+0x18,1);
        }
    }
    return c.r(3);
}
std::uint32_t close_current_history(Cpu& c,std::uint32_t address) {
    Frame frame(c,0x107440c0,0x40,{.first_gpr=30});Kept<31> object(c,address);Manager manager{c,address};Program p{c};
    c.set(3,manager.group());
    if(compare(c,c.i16(c.r(3)+0x14))>0) {
        manager.history()=c.u32(c.r(3)+12);
        while(true) {
            c.set(4,manager.group()-address);c.set(5,c.r(4)+16);c.set(3,manager.history());
            if(compare_logical(c,c.r(3))==0)break;
            if(byte_nonzero(c,c.u8(c.r(3)+16)) && compare_logical(c,c.u32(c.r(3)+12))==0) {
                c.set(3,address+c.r(4));c.put8(c.r(3)+24,1);return 0;
            }
            manager.history()=c.u32(c.r(3)+0x1f4);
        }
    }
    c.set(3,manager.group());Kept<30> selected(c,c.u32(c.r(3)+16));
    if(compare_logical(c,selected)!=0) {
        manager.history()=c.u32(c.r(3)+8);
        for(;;) {
            c.set(4,manager.group()-address);c.set(3,manager.history());
            if(compare_logical(c,c.r(3))==0)break;
            const auto callback=c.u32(c.r(3)+12);if(compare_logical(c,callback)!=0)p.callback(callback,0x10744188);
            c.set(4,manager.group()-address+16);c.set(3,manager.history());manager.history()=c.u32(c.r(3)+0x1f8);
        }
        manager.history()=selected;c.set(3,manager.history());
        const auto callback=c.u32(c.r(3)+4);if(compare_logical(c,callback)!=0)p.callback(callback,0x107441e4);
        p.call(0x10719560,0x10744200,{c.u32(manager.group()+0x24),1});
        c.set({{4,0},{3,manager.group()}});manager.history()=0;
    }
    c.set({{5,0},{4,manager.group()}});c.put8(c.r(4)+24,0);
    enter(c,0x10744230,{object});result(c,clear_history(c,object));return 1;
}
}

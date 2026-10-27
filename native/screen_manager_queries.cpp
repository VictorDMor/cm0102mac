#include "screen_manager_detail.hpp"

namespace cm::aot::screen_manager {
using namespace detail;

std::uint32_t set_default_history(Cpu& c, std::uint32_t address, std::uint32_t key,
    std::uint32_t value, std::uint32_t active, std::uint32_t callback, std::uint32_t context,
    std::uint32_t values, std::uint32_t count) {
    c.put32(address + 0x2c00, key);
    c.put32(address + 0x2c04, value);
    c.put8(address + 0x2c08, active);
    c.put32(address + 0x2c0c, callback);
    c.put32(address + 0x2c10, context);
    c.put32(address + 0x2c14, values);
    c.put16(address + 0x2c18, count);
    return address;
}

std::uint32_t pending_screens(Cpu& c, std::uint32_t address) {
    Manager manager{c, address};
    const auto group = extended(manager.current());
    const auto count = manager.count(std::int32_t(group)).get();
    const auto base = manager.group(std::int32_t(group));
    std::uint32_t pending = 0, index = 0, offset = 0;
    while (std::int16_t(index) < count) {
        const auto row = base + offset;
        if (!c.u8(row + 0x19) && !c.u8(row + 0x1a)) ++pending;
        offset += 0x2c;
        ++index;
    }
    // No calls or guest writes occur in the scan. Publish exactly the
    // registers and final comparison the original leaves to its caller.
    c.set({{7, group}, {5, extended(std::int16_t(index))}, {4, group * 2},
           {9, pending}, {8, index}, {6, offset}});
    compare(c, std::int16_t(index), count);
    return pending;
}

namespace {
// All four history queries use the same diagnostic frame. An absent current
// history is fatal in the original, which continues only if the quit call returns.
void require_history(Cpu& c, const Frame& frame, Manager manager, std::uint32_t at, unsigned line) {
    if (compare_logical(c, manager.history()) == 0) {
        assertion(c, frame, at, 0x40, 0x38, line);
        c.set(4, c.toc(slot::success));
        Program{c}.call(0x1076f7d0, at + 0x48, {missing});
    }
}
}
std::uint32_t history_at_end(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x1073ea20, 0x70, {.first_gpr = 29});
    keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(slot::message)}, {29, address}});
    Manager manager{c, address};
    require_history(c, f, manager, 0x1073ea70, 0x5cc);
    c.set(3, manager.history());
    return c.u32(c.r(3) + 0x1f8) == 0;
}
std::uint32_t history_at_start(Cpu& c, std::uint32_t address, std::uint32_t include_active) {
    Frame f(c, 0x1073eb00, 0x70, {.first_gpr = 28});
    keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(slot::message)}, {29, include_active}, {28, address}});
    Manager manager{c, address};
    require_history(c, f, manager, 0x1073eb58, 0x5df);
    c.set({{4, 0}, {3, manager.history()}});
    if (compare_logical(c, c.u32(c.r(3) + 0x1f4)) == 0 ||
        (!byte_nonzero(c, include_active) && byte_nonzero(c, c.u8(c.r(3) + 0x10)))) c.set(4, 1);
    return std::uint32_t(std::int8_t(c.r(4)));
}
std::uint32_t history_is_active(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x1073ec00, 0x70, {.first_gpr = 29});
    keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(slot::message)}, {29, address}});
    Manager manager{c, address};
    require_history(c, f, manager, 0x1073ec50, 0x5f3);
    return c.u8(manager.history() + 0x10);
}
std::uint32_t history_has_callback(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x1073ecd0, 0x70, {.first_gpr = 29});
    keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(slot::message)}, {29, address}});
    Manager manager{c, address};
    require_history(c, f, manager, 0x1073ed20, 0x606);
    return c.u32(manager.history() + 0xc) != 0;
}
}

namespace cm::aot::screen_manager {
namespace {
// Screen IDs may occur in more than one group. Queries return the first;
// swapping current-group IDs deliberately uses the last occurrence of each.
template<bool writing>
std::uint32_t locate_screen(Cpu& c, std::uint32_t address, std::uint32_t key) {
    Manager manager{c,address};
    std::uint32_t rows = 0;
    for (std::uint32_t group = 0; group < 16; ++group) {
        const auto count = manager.count(group).get();
        rows = 0;
        for (; std::int16_t(rows) < count; ++rows) {
            const auto row = rows;
            const auto screen=manager.group(group)+row*0x2c;
            if (c.u32(screen + 4) == key) {
                c.set({{writing ? 6u : 5u, group * 2 + 0x2c38},
                       {writing ? 7u : 6u, screen - address},
                       {writing ? 8u : 7u, row * 0x2c},
                       {writing ? 12u : 11u, row},
                       {writing ? 9u : 8u, group * 0x2c0},
                       {writing ? 10u : 9u, group * 2},
                       {writing ? 11u : 10u, group}, {4, group * 0x2c0}, {3, screen}});
                compare_logical(c, key, key);
                return screen;
            }
        }
    }
    c.set({{writing ? 6u : 5u, 0x2c56}, {writing ? 7u : 6u, rows},
           {writing ? 8u : 7u, rows * 0x2c}, {writing ? 12u : 11u, rows},
           {writing ? 9u : 8u, 16 * 0x2c0}, {writing ? 10u : 9u, 32},
           {writing ? 11u : 10u, 16}});
    compare(c, 16, 16);
    return 0;
}
}
std::uint32_t screen_flag(Cpu& c,std::uint32_t address,std::uint32_t key,unsigned flag) {
    const auto screen=locate_screen<false>(c,address,key);
    return screen?c.u8(screen+flag):1;
}
std::uint32_t set_screen_disabled(Cpu& c,std::uint32_t address,std::uint32_t key,std::uint32_t disabled) {
    const auto screen=locate_screen<true>(c,address,key);
    if(screen)c.put8(screen+0x1a,disabled);
    return c.r(3);
}
std::uint32_t group_disabled(Cpu& c,std::uint32_t address,std::uint32_t id) {
    Manager manager{c,address};id=extended(std::int16_t(id));c.set({{5,0},{4,id},{6,0},{8,0}});
    for(std::uint32_t group=0;compare(c,std::int16_t(group),16)<0;++group) {
        if(compare(c,manager.count(group))>0) {
            c.set(7,manager.group(group));
            if(compare(c,id,c.i16(c.r(7)))==0 && !byte_nonzero(c,c.u8(c.r(7)+0x1a)))return 0;
        }
        c.set({{5,(group+1)*2},{6,(group+1)*0x2c0},{8,group+1}});
    }
    return 1;
}
std::uint32_t inactive_groups(Cpu& c,std::uint32_t address) {
    Manager manager{c,address};std::uint32_t inactive=0;c.set({{4,0},{7,0},{5,0},{6,0}});
    for(std::uint32_t group=0;compare(c,std::int16_t(group),16)<0;++group) {
        if(compare(c,manager.count(group))<=0 || byte_nonzero(c,c.u8(manager.group(group)+0x19)))++inactive;
        c.set({{4,(group+1)*2},{5,(group+1)*0x2c0},{6,group+1},{7,inactive}});
    }
    return inactive;
}
std::uint32_t screen_group(Cpu& c,std::uint32_t address,std::uint32_t key) {
    if (key == 0) { compare_logical(c, key); return missing; }
    Manager manager{c,address};
    std::uint32_t rows = 0;
    for (std::uint32_t group = 0; group < 16; ++group) {
        const auto count = manager.count(group).get();
        rows = 0;
        for (; std::int16_t(rows) < count; ++rows) {
            const auto offset = group * 0x2c0 + rows * 0x2c;
            if (key == c.u32(address + offset + 4)) {
                c.set({{5, group * 2 + 0x2c38}, {6, offset}, {7, rows * 0x2c},
                       {10, rows}, {8, group * 0x2c0}, {9, group * 2}, {11, group}});
                compare_logical(c, key, key);
                return group;
            }
        }
    }
    c.set({{5, 0x2c56}, {6, rows}, {7, rows * 0x2c}, {10, rows},
           {8, 16 * 0x2c0}, {9, 32}, {11, 16}});
    compare(c, 16, 16);
    return missing;
}
std::uint32_t copy_group_keys(Cpu& c,std::uint32_t address,std::uint32_t output,std::uint32_t count_out,std::uint32_t group) {
    group=extended(std::int16_t(group));Manager manager{c,address};
    c.set({{7,group*0x2c0},{9,0},{8,0},{6,address+group*2}});
    c.put16(count_out,std::uint32_t(manager.count(std::int32_t(group)).get()));
    std::uint32_t index=0;
    while(true) {
        c.set(6,extended(std::int16_t(index)));
        if(compare(c,c.r(6),c.i16(count_out))>=0)break;
        c.put32(output,c.u32(manager.group(std::int32_t(group))+index*0x2c+4));
        output+=4;++index;c.set({{8,index*0x2c},{9,index},{4,output}});
    }
    return address;
}
std::uint32_t swap_screen_keys(Cpu& c,std::uint32_t address,std::uint32_t first,std::uint32_t second) {
    Manager manager{c,address};std::uint32_t left=missing,right=missing,index=0;
    c.set({{10,missing},{8,0}});
    for(;;++index) {
        auto group=extended(manager.current());c.set({{9,group},{7,extended(std::int16_t(index))},{6,group*2}});
        if(compare(c,c.r(7),manager.count(std::int32_t(group)))>=0)break;
        const auto screen=manager.group()+index*0x2c;
        c.set(6,screen-address);
        if(compare_logical(c,c.u32(screen+4),first)==0)left=index;
        if(compare_logical(c,c.u32(screen+4),second)==0)right=index;
        c.set({{8,(index+1)*0x2c},{10,left}});
    }
    c.set(4,extended(std::int16_t(left)));
    if(compare(c,c.r(4),-1)<=0)return 0;
    c.set(5,extended(std::int16_t(right)));
    if(compare(c,c.r(5),-1)<=0)return 0;
    const auto left_address=manager.group()+std::uint32_t(std::int16_t(left))*0x2c+4;
    const auto right_address=manager.group()+std::uint32_t(std::int16_t(right))*0x2c+4;
    c.set(7,c.u32(left_address));c.put32(left_address,c.u32(right_address));c.put32(right_address,c.r(7));
    c.set({{6,manager.group()+4},{4,right_address-4},{5,std::uint32_t(std::int16_t(right))*0x2c}});return 1;
}
std::uint32_t screen_count(Cpu& c,std::uint32_t address) {
    compare(c,0,16);std::uint32_t count=0;
    for(unsigned group=0;group<16;++group)count+=std::uint32_t(c.i16(address+0x2c38+group*2));
    c.set({{6,count},{4,extended(c.i16(address+0x2c54))},{5,extended(c.i16(address+0x2c48))}});return count;
}
std::uint32_t active_history_count(Cpu& c,std::uint32_t address) {
    std::uint32_t count=0;
    for(unsigned group=0;group<16;++group) {
        for(unsigned batch=0;batch<2;++batch) {
            const auto base=address+group*0x2c0+batch*0x160+0x14;
            for(unsigned i=0;i<8;++i)count+=std::uint32_t(c.i16(base+i*0x2c));
            c.set({{4,extended(c.i16(base+0x108))},{7,base},{9,(batch+1)*8},{5,(batch+1)*0x160}});
            compare(c,std::int16_t(c.r(9)),16);
        }
        c.set({{6,(group+1)*0x2c0},{8,group+1}});compare(c,std::int16_t(c.r(8)),16);
    }
    c.set(10,count);return count;
}
std::uint32_t control_value(Cpu& c,std::uint32_t address,std::uint32_t control) {
    Frame frame(c,0x10743510,0x40,{});Manager manager{c,address};
    const auto group=manager.group();c.set(5,group-address);
    if(compare(c,c.i16(group))==0) {
        c.set(3,group);c.set(4,c.u32(group+0x24));
        if(compare(c,std::int16_t(control),-1)==0)return 0;
        return Program{c}.call(0x107145b0,0x10743554,{c.r(4)+std::uint32_t(std::int16_t(control))*0xbf0+4});
    }
    c.set({{3,group},{4,extended(std::int16_t(control))}});
    return compare(c,c.r(4),c.i16(group+0x1c))==0?c.u32(group+0x20):0;
}
std::uint32_t current_screen(Cpu& c,std::uint32_t address) {
    Frame frame(c,0x10743590,0x40,{});
    return Program{c}.call(0x1071c070,0x107435b0,{c.u32(Manager{c,address}.group()+0x24)});
}
}

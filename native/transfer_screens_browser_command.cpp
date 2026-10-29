#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
std::uint32_t transfers_browser_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x10427df0,0x60,{.first_gpr=22});
    Kept<22> nation(c,c.r(22));
    Kept<23> current_date(c,c.toc(slot::transfer_context));
    Kept<24> count_or_pages(c,c.r(24));
    Kept<25> start_date(c,c.toc(-0x7a9c));
    Kept<26> page(c,c.r(26));
    Kept<27> monthly(c,c.r(27));
    Kept<28> screens(c,c.toc(slot::screens));
    Kept<29> year(c,c.r(29));
    Kept<30> control(c,control_value);
    Kept<31> month(c,c.r(31));
    Program p{c};
    nation=p.call(0x1073ef70,0x10427e1c,{screens,0});
    month=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x10427e30,{screens,1}))));
    year=std::uint16_t(p.call(0x1073ef70,0x10427e44,{screens,2}));
    monthly=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x10427e58,{screens,3}))));
    count_or_pages=p.call(0x1073ef70,0x10427e6c,{screens,5});
    page=std::uint32_t(std::int32_t(std::int16_t(p.call(0x1073ef70,0x10427e80,{screens,6}))));
    const auto page_count=p.call(0x1073ef70,0x10427e94,{screens,7});
    const bool has_rows=compare(c,count_or_pages)>0;count_or_pages=std::uint32_t(std::int32_t(std::int16_t(page_count)));
    if(has_rows && compare(c,std::int16_t(control),-1)!=0) {
        const auto grid=p.call(0x1073ef70,0x10427eb8,{screens,9});
        const auto selection=p.call(0x10743510,0x10427ec8,{screens,std::uint32_t(std::int32_t(std::int16_t(grid)))});
        p.history(0x10427edc,screens,8,selection);
    }
    const auto payload=[&](bool into_r5) {
        const bool none=compare(c,std::int16_t(control),-1)==0;
        c.set(3,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24));
        std::uint32_t result=0;
        if(!none) {c.set(3,c.r(3)+0xc0000+std::uint32_t(std::int16_t(control))*0x17c);result=c.u32(c.r(3)-0x5754);}
        if(into_r5)c.set(5,result);return result;
    };
    const auto invalidate=[&](std::uint32_t cleanup_back,std::uint32_t history_back) {
        enter(c,cleanup_back);clear_transfers_browser_cache(c);
        p.history(history_back,screens,6,1);
    };
    if(compare(c,control_action(c,screens,control),1)==0) {
        monthly=std::uint32_t(std::int32_t(std::int8_t(payload(false))));
        p.history(0x10427f84,screens,3,monthly);
        if(byte_result(c,monthly)) {
            c.set(3,std::uint16_t(year));
            if(compare_logical(c,c.r(3),c.u16(start_date+2))==0) {
                const auto first=p.call(0x10718360,0x10427fa4,{start_date});
                c.set(3,std::uint32_t(std::int32_t(std::int8_t(first))));
                if(compare(c,std::int8_t(month),c.r(3))>0)c.set(3,month);
                else p.call(0x10718360,0x10427fc4,{start_date});
                month=c.r(3);
            }
        }
        if(byte_result(c,monthly)) {
            c.set(3,std::uint16_t(year));
            if(compare_logical(c,c.r(3),c.u16(current_date+2))==0) {
                const auto first=p.call(0x10718360,0x10427fe8,{current_date});
                c.set(3,std::uint32_t(std::int32_t(std::int8_t(first))));
                if(compare(c,std::int8_t(month),c.r(3))<0)c.set(3,month);
                else p.call(0x10718360,0x10428008,{current_date});
                month=c.r(3);
            }
        }
        p.history(0x10428020,screens,1,std::uint32_t(std::int32_t(std::int8_t(month))));
        invalidate(0x10428024,0x10428038);return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),2)==0) {
        if(byte_result(c,monthly)) {
            c.set(3,std::uint16_t(year));bool can_move=compare_logical(c,c.r(3),c.u16(start_date+2))>0;
            if(!can_move) {
                c.set(3,std::uint16_t(year));
                if(compare_logical(c,c.r(3),c.u16(start_date+2))==0) {
                    const auto start=p.call(0x10718360,0x104280c8,{start_date});
                    c.set(3,std::uint32_t(std::int32_t(std::int8_t(start))));
                    can_move=compare(c,std::int8_t(month),c.r(3))>0;
                }
            }
            if(can_move) {
                if(!byte_result(c,month)) {month=11;year=year-1;}else month=month-1;
                p.history(0x10428104,screens,1,std::uint32_t(std::int32_t(std::int8_t(month))));
                p.history(0x10428118,screens,2,std::uint16_t(year));invalidate(0x1042811c,0x10428130);return std::uint32_t(-11);
            }
        }
        if(!byte_result(c,monthly)) {
            c.set(3,std::uint16_t(year));
            if(compare_logical(c,c.r(3),c.u16(start_date+2))>0) {
                p.history(0x10428168,screens,2,std::uint32_t(std::uint16_t(year))-1);
                invalidate(0x1042816c,0x10428180);return std::uint32_t(-11);
            }
        }
    }
    if(compare(c,control_action(c,screens,control),3)==0) {
        if(byte_result(c,monthly)) {
            c.set(3,std::uint16_t(year));bool can_move=compare_logical(c,c.r(3),c.u16(current_date+2))<0;
            if(!can_move) {
                c.set(3,std::uint16_t(year));
                if(compare_logical(c,c.r(3),c.u16(current_date+2))==0) {
                    const auto current=p.call(0x10718360,0x10428210,{current_date});
                    c.set(3,std::uint32_t(std::int32_t(std::int8_t(current))));
                    can_move=compare(c,std::int8_t(month),c.r(3))<0;
                }
            }
            if(can_move) {
                if(compare(c,std::int8_t(month),11)==0) {month=0;year=year+1;}else month=month+1;
                p.history(0x10428250,screens,1,std::uint32_t(std::int32_t(std::int8_t(month))));
                p.history(0x10428264,screens,2,std::uint16_t(year));invalidate(0x10428268,0x1042827c);return std::uint32_t(-11);
            }
        }
        if(!byte_result(c,monthly)) {
            c.set(3,std::uint16_t(year));
            if(compare_logical(c,c.r(3),c.u16(current_date+2))<0) {
                p.history(0x104282b4,screens,2,std::uint32_t(std::uint16_t(year))+1);
                invalidate(0x104282b8,0x104282cc);return std::uint32_t(-11);
            }
        }
    }
    if(compare(c,control_action(c,screens,control),4)==0) {
        if(compare(c,nation,payload(false))!=0) {
            const auto selected=payload(true);p.history(0x104283b4,screens,0,selected);
            invalidate(0x104283b8,0x104283cc);return std::uint32_t(-11);
        }
    }
    if(compare(c,control_action(c,screens,control),5)==0) {
        const auto value=std::int32_t(std::int16_t(page))-10;
        const auto first=compare(c,value,1)<0 ? 1 : value;
        p.history(0x10428458,screens,6,std::uint32_t(first));return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),6)==0) {
        const auto value=std::int32_t(std::int16_t(page))-1;
        const auto first=compare(c,value,1)<0 ? 1 : value;
        p.history(0x104284e4,screens,6,std::uint32_t(first));return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),7)==0) {
        const auto value=std::int32_t(std::int16_t(page))+1;
        const auto last=compare(c,std::int16_t(count_or_pages),value)<0 ? std::int32_t(std::int16_t(count_or_pages)) : value;
        p.history(0x10428574,screens,6,std::uint32_t(last));return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),8)==0) {
        const auto value=std::int32_t(std::int16_t(page))+10;
        const auto last=compare(c,std::int16_t(count_or_pages),value)<0 ? std::int32_t(std::int16_t(count_or_pages)) : value;
        p.history(0x10428604,screens,6,std::uint32_t(last));return std::uint32_t(-11);
    }
    const auto command=p.call(0x102a54b0,0x10428614,{control});if(compare(c,command)!=0)return command;
    const auto fallback=p.call(0x102a83a0,0x10428628,{control});c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0);return fallback;
}
}

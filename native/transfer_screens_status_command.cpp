#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
std::uint32_t player_transfer_status_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x10429a10,0x90,{.first_gpr=17});
    Kept<17> contracts_or_control_or_flag(c,c.toc(slot::contracts));
    Kept<18> people_or_person_or_zero(c,c.toc(slot::people));
    Kept<19> current_contract_or_manager(c,c.toc(slot::current_contract));
    Kept<20> news(c,c.toc(-0x7bf0));
    Kept<21> foreground_or_squad(c,c.toc(-0x7bc8));
    Kept<22> disabled_or_item(c,c.toc(-0x7bc4));
    Kept<23> limited_or_listed(c,c.r(23));
    Kept<24> limit(c,c.r(24));
    Kept<25> transfers(c,c.toc(slot::transfers));
    Kept<26> squad_status(c,c.r(26));
    Kept<27> text_or_contract(c,c.toc(-0x7bb8));
    Kept<28> automatic(c,c.r(28));
    Kept<29> fee(c,c.r(29));
    Kept<30> control(c,control_value);
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    fee=p.call(0x1073ef70,0x10429a54,{screens,1});
    squad_status=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x10429a68,{screens,3}))));
    automatic=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x10429a7c,{screens,10}))));
    limited_or_listed=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x10429a90,{screens,12}))));
    limit=p.call(0x1073ef70,0x10429aa4,{screens,15});
    if(compare(c,c.i8(screens+0x2c5f))!=0) {
        c.set(3,screens+0x260000);
        if(compare(c,c.u32(c.r(3)-0x7e5c),3)==0) { c.put32(c.r(3)-0x7e5c,0); return std::uint32_t(-9); }
    }
    const auto selected_payload=[&] {
        const bool none=compare(c,std::int16_t(control),-1)==0;
        c.set(3,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24));
        if(none)c.set(5,0);
        else { c.set(3,c.r(3)+0xc0000+std::uint32_t(std::int16_t(control))*0x17c);c.set(5,c.u32(c.r(3)-0x5754)); }
        return c.r(5);
    };
    if(compare(c,control_action(c,screens,control),5)==0) {
        const auto id=p.call(0x1073ef70,0x10429b3c,{screens,0});
        c.set(5,c.u32(people_or_person_or_zero)); c.set(7,id*0x70);
        automatic=std::int8_t(automatic)==0 ? 1u : 0u;
        people_or_person_or_zero=c.r(5)+c.r(7);
        p.history(0x10429b6c,screens,10,automatic);
        if(byte_result(c,automatic)) {
            const auto record=p.call(0x10082f20,0x10429b80,{c.u32(contracts_or_control_or_flag),people_or_person_or_zero});
            compare_logical(c,record);c.put32(current_contract_or_manager,record);
            if(record) {
                c.set(3,c.u32(current_contract_or_manager));c.set(4,c.toc(slot::clubs));
                const auto owner_id=c.u32(c.r(3)+4);c.set(3,c.u32(c.r(4)));c.set(5,c.r(3)+owner_id*0x248);
            } else c.set(5,0);
            fee=p.call(0x10084be0,0x10429bc4,{c.u32(contracts_or_control_or_flag),people_or_person_or_zero,c.r(5),0,missing,1});
            p.history(0x10429be0,screens,1,fee);
        }
        return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),11)==0) {
        p.history(0x10429c54,screens,10,0);const auto value=selected_payload();
        p.history(0x10429c9c,screens,1,value);return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),7)==0) {
        const auto value=selected_payload();p.history(0x10429d44,screens,3,value);return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),9)==0) {
        const auto old=p.call(0x1073ef70,0x10429db0,{screens,4});
        contracts_or_control_or_flag=0;people_or_person_or_zero=0;
        p.history(0x10429ddc,screens,4,std::int8_t(old)==0 ? 1u : 0u);
        p.history(0x10429df0,screens,5,contracts_or_control_or_flag);
        p.history(0x10429e04,screens,13,people_or_person_or_zero);return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),8)==0) {
        const auto old=p.call(0x1073ef70,0x10429e70,{screens,5});
        contracts_or_control_or_flag=std::int8_t(old)==0 ? 1u : 0u;people_or_person_or_zero=0;
        p.history(0x10429e9c,screens,4,0);p.history(0x10429eb0,screens,5,contracts_or_control_or_flag);
        p.history(0x10429ec4,screens,13,people_or_person_or_zero);return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),10)==0) {
        const auto old=p.call(0x1073ef70,0x10429f30,{screens,13});
        contracts_or_control_or_flag=std::int8_t(old)==0 ? 1u : 0u;people_or_person_or_zero=0;
        p.history(0x10429f5c,screens,4,0);p.history(0x10429f70,screens,5,people_or_person_or_zero);
        p.history(0x10429f84,screens,13,contracts_or_control_or_flag);return std::uint32_t(-11);
    }
    const auto adjust_fee=[&](bool increase) {
        const unsigned delta=increase ? 0x1ec : 0, scratch=increase ? 0x3c : 0x40;
        c.put32(f.at(scratch),fee);
        p.call(increase ? 0x1000a060 : 0x1000a170,0x10429ff4+delta,{f.at(scratch),4});fee=c.u32(f.at(scratch));
        p.history(0x1042a00c+delta,screens,1,fee);p.history(0x1042a020+delta,screens,10,0);
        const auto first=p.call(0x1073ef70,0x1042a02c+delta,{screens,2});
        const bool positive=compare(c,fee)>0;contracts_or_control_or_flag=std::uint32_t(std::int32_t(std::int16_t(first)));
        if(!positive) {
            p.call(0x10740620,0x1042a048+delta,{screens,contracts_or_control_or_flag,6});
            p.call(0x10741a80,0x1042a058+delta,{screens,contracts_or_control_or_flag,c.u16(disabled_or_item)});
            p.call(0x10742420,0x1042a068+delta,{screens,contracts_or_control_or_flag,0});
        } else {
            p.call(0x10740620,0x1042a07c+delta,{screens,contracts_or_control_or_flag,2});
            p.call(0x10741a80,0x1042a08c+delta,{screens,contracts_or_control_or_flag,c.u16(foreground_or_squad)});
            p.call(0x10742100,0x1042a09c+delta,{screens,contracts_or_control_or_flag,2});
        }
        contracts_or_control_or_flag=contracts_or_control_or_flag+1;
        p.call(0x10009f20,0x1042a0ac+delta,{f.at(scratch),5});
        p.call(0x1000b410,0x1042a0c4+delta,{text_or_contract,2000,c.r(3),c.r(4),5});
        p.call(0x10741da0,0x1042a0d4+delta,{screens,contracts_or_control_or_flag,text_or_contract});
        c.set(3,30000000);contracts_or_control_or_flag=contracts_or_control_or_flag+1;
        bool capped=compare(c,fee,c.r(3))>=0;
        if(increase && !capped && byte_result(c,limited_or_listed))capped=compare(c,fee,limit)>=0;
        if(capped) {
            p.call(0x10740620,increase ? 0x1042a2f4 : 0x1042a0f8,{screens,contracts_or_control_or_flag,6});
            p.call(0x10741a80,increase ? 0x1042a304 : 0x1042a108,{screens,contracts_or_control_or_flag,c.u16(disabled_or_item)});
            p.call(increase ? 0x10742100 : 0x10742420,increase ? 0x1042a314 : 0x1042a118,{screens,contracts_or_control_or_flag,0});
        } else {
            p.call(0x10740620,increase ? 0x1042a328 : 0x1042a12c,{screens,contracts_or_control_or_flag,2});
            p.call(0x10741a80,increase ? 0x1042a338 : 0x1042a13c,{screens,contracts_or_control_or_flag,c.u16(foreground_or_squad)});
            p.call(0x10742100,increase ? 0x1042a348 : 0x1042a14c,{screens,contracts_or_control_or_flag,3});
        }
        contracts_or_control_or_flag=contracts_or_control_or_flag+1;
        p.call(0x10740e10,increase ? 0x1042a35c : 0x1042a160,{screens,contracts_or_control_or_flag,0x30});
        p.call(0x10742ec0,increase ? 0x1042a36c : 0x1042a170,{screens,0,0});
    };
    if(compare(c,control_action(c,screens,control),2)==0) { adjust_fee(false);return 0; }
    if(compare(c,control_action(c,screens,control),3)==0) { adjust_fee(true);return 0; }
    if(compare(c,control_action(c,screens,control),1)==0) {
        const auto id=p.call(0x1073ef70,0x1042a3d8,{screens,0});
        p.history(0x1042a3fc,screens,1,c.u32(c.u32(people_or_person_or_zero)+id*0x70+8));return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),4)==0)return std::uint32_t(-9);
    if(compare(c,control_action(c,screens,control),6)==0) {
        const auto id=p.call(0x1073ef70,0x1042a4c8,{screens,0});c.set(5,id*0x70);
        people_or_person_or_zero=c.u32(people_or_person_or_zero)+c.r(5);
        current_contract_or_manager=p.call(0x1073ef70,0x1042a4e0,{screens,7});
        const auto owner=p.call(0x1073ef70,0x1042a4f4,{screens,11});
        c.set(3,c.u32(contracts_or_control_or_flag));contracts_or_control_or_flag=owner;
        text_or_contract=p.call(0x10082f90,0x1042a50c,{c.r(3),people_or_person_or_zero,contracts_or_control_or_flag});
        foreground_or_squad=c.u8(text_or_contract+0x60)&15;
        disabled_or_item=p.call(0x1073ef70,0x1042a52c,{screens,8});
        limited_or_listed=c.u8(text_or_contract+0x5c);c.put8(f.at(0x38),0);
        if(compare_logical(c,current_contract_or_manager)!=0 && compare(c,disabled_or_item,-1)>0) {
            p.call(0x102cd0c0,0x1042a568,{c.u32(news),current_contract_or_manager,disabled_or_item,6,1,0});
            p.call(0x102cd0c0,0x1042a584,{c.u32(news),current_contract_or_manager,disabled_or_item,7,squad_status,0});
            p.call(0x102cd0c0,0x1042a5a0,{c.u32(news),current_contract_or_manager,disabled_or_item,9,std::uint32_t(std::int32_t(std::int8_t(limited_or_listed))),0});
        }
        c.put8(text_or_contract+0x60,(c.u8(text_or_contract+0x60)&0xf0)|std::uint32_t(foreground_or_squad));
        p.call(0x103f6240,0x1042a5d0,{c.u32(transfers),people_or_person_or_zero,contracts_or_control_or_flag,fee,f.at(0x38),squad_status,limited_or_listed,0});
        if(compare_logical(c,current_contract_or_manager)!=0 && compare(c,disabled_or_item,-1)>0)
            p.call(0x102cd0c0,0x1042a600,{c.u32(news),current_contract_or_manager,disabled_or_item,10,std::uint32_t(std::int32_t(c.i8(f.at(0x38)))),0});
        const auto encoded=(std::uint32_t(c.u8(text_or_contract+0x38))<<24)&0xfe000000u;
        c.s.wr<1>(reg::xer_ca,std::int32_t(encoded)<0 && (encoded&0x03ffffffu)!=0);
        if(compare(c,std::int8_t(std::int32_t(encoded)>>26),6)!=0) {
            contracts_or_control_or_flag=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042a624,{screens,4}))));
            current_contract_or_manager=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042a638,{screens,5}))));
            const auto ignored=p.call(0x1073ef70,0x1042a64c,{screens,13});
            const bool listed=byte_result(c,contracts_or_control_or_flag);
            contracts_or_control_or_flag=std::uint32_t(std::int32_t(std::int8_t(ignored)));
            if(listed) p.call(0x10363130,0x1042a66c,{c.u32(c.u32(transfers)+0x988),people_or_person_or_zero,1});
            else if(byte_result(c,p.call(0x100d3040,0x1042a67c,{people_or_person_or_zero,0})))
                p.call(0x10365c30,0x1042a694,{c.u32(c.u32(transfers)+0x988),people_or_person_or_zero});
            if(byte_result(c,current_contract_or_manager))p.call(0x10365c80,0x1042a6ac,{c.u32(c.u32(transfers)+0x988),people_or_person_or_zero});
            else if(byte_result(c,p.call(0x100d3230,0x1042a6bc,{people_or_person_or_zero,0})))
                p.call(0x10365cc0,0x1042a6d4,{c.u32(c.u32(transfers)+0x988),people_or_person_or_zero});
            if(byte_result(c,contracts_or_control_or_flag)) {
                if(compare(c,c.u8(text_or_contract+0x5c)&2)!=0)c.put8(text_or_contract+0x5c,c.u8(text_or_contract+0x5c)&~2u);
                c.put8(text_or_contract+0x5c,c.u8(text_or_contract+0x5c)|8u);
            } else c.put8(text_or_contract+0x5c,c.u8(text_or_contract+0x5c)&~8u);
            if(byte_result(c,automatic))c.put8(text_or_contract+0x5c,c.u8(text_or_contract+0x5c)|4u);
            else c.put8(text_or_contract+0x5c,c.u8(text_or_contract+0x5c)&~4u);
        }
        return std::uint32_t(-9);
    }
    const auto command=p.call(0x102a54b0,0x1042a744,{control});if(compare(c,command)!=0)return command;
    const auto fallback=p.call(0x102a83a0,0x1042a758,{control});c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0);return fallback;
}
}

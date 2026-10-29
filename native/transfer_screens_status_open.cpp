#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void open_player_transfer_status(Cpu& c, std::uint32_t person_address,
                                std::uint32_t manager_address, std::uint32_t news_item,
                                std::uint32_t from_news) {
    Frame f(c,0x10428750,0x60,{.first_gpr=24});
    Kept<24> source_flag(c,from_news);
    Kept<25> item_or_loan(c,news_item);
    Kept<26> manager_or_listed(c,manager_address);
    Kept<27> unavailable(c,c.r(27));
    Kept<28> person(c,person_address);
    Kept<29> contract(c,c.r(29));
    Kept<30> contracts(c,c.toc(slot::contracts));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    if (!byte_result(c,p.call(0x1073e4b0,0x10428794,{screens,c.toc(-0x4b40),c.toc(-0x4b44),1,0,0}))) return;
    const auto manager=c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4);
    contract=p.call(0x10082f90,0x104287bc,{c.u32(contracts),person,c.u32(manager+0x24)});
    p.history(0x104287d4,screens,0,c.u32(person));
    p.history(0x104287e8,screens,1,c.u32(person+8));
    p.history(0x104287fc,screens,2,0);
    p.history(0x10428810,screens,7,manager_or_listed);
    p.history(0x10428824,screens,8,item_or_loan);
    p.history(0x10428838,screens,9,std::uint32_t(std::int32_t(std::int8_t(source_flag))));
    manager_or_listed=c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4);
    p.history(0x1042885c,screens,11,c.u32(manager_or_listed+0x24));
    bool reserve=false;
    if (compare_logical(c,c.u32(manager_or_listed+0x24))!=0) {
        const auto club=p.call(0x10103510,0x10428878,{indirect(c,-0x7ab8),c.u32(manager_or_listed+0x24)});
        reserve=compare(c,c.u8(club+0x165),1)==0;
    }
    p.history(reserve ? 0x10428898 : 0x104288b0,screens,12,reserve ? 1u : 0u);
    if (compare_logical(c,contract)!=0) {
        const auto unavailable_flag=std::int8_t(c.u8(contract+0x5c)&8);
        const auto listed_value=c.u32(contract+0x50);
        unavailable=unavailable_flag!=0;
        manager_or_listed=listed_value!=0;
        c.set({{6,std::uint32_t(std::int32_t(unavailable_flag))},{7,listed_value},{5,unavailable}});
        item_or_loan=p.call(0x100d3040,0x104288f4,{person,0});
        source_flag=p.call(0x100d3230,0x10428908,{person,0});
        p.history(0x10428920,screens,4,std::uint32_t(std::int32_t(std::int8_t(item_or_loan))));
        p.history(0x10428934,screens,5,std::uint32_t(std::int32_t(std::int8_t(source_flag))));
        p.history(0x10428948,screens,6,unavailable);
        p.history(0x1042895c,screens,13,unavailable);
        p.history(0x10428970,screens,14,manager_or_listed);
        p.history(0x1042898c,screens,3,c.u8(contract+0x60)&15);
        p.history(0x104289a8,screens,10,(c.u8(contract+0x5c)>>2)&1);
    } else {
        p.history(0x104289c0,screens,3,7);
        p.history(0x104289d4,screens,4,0);
        p.history(0x104289e8,screens,5,0);
        p.history(0x104289fc,screens,6,0);
        p.history(0x10428a10,screens,13,0);
        p.history(0x10428a24,screens,14,0);
        p.history(0x10428a38,screens,10,0);
    }
    if (compare_logical(c,c.u32(person+0x14))!=0) {
        const auto current_manager=c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4);
        const auto available=p.call(0x10084be0,0x10428a70,{c.u32(contracts),person,c.u32(current_manager+0x24),0,missing,1});
        p.history(0x10428a84,screens,15,available);
    } else p.history(0x10428a9c,screens,15,0);
}

void open_player_transfer_status_from_news(Cpu& c,std::uint32_t manager_address,std::uint32_t item_address) {
    Frame f(c,0x104286c0,0x130,{.first_gpr=30});
    Kept<30> manager(c,manager_address); Kept<31> item(c,item_address); Program p{c};
    p.call(0x102bdf90,0x104286ec,{f.at(0x38),0,0});
    p.call(0x102ccdd0,0x10428704,{indirect(c,-0x7bf0),manager,item,f.at(0x38)});
    const auto person_id=p.call(0x102cb570,0x10428710,{f.at(0x38),2});
    const auto person=indirect(c,slot::people)+person_id*0x70;
    enter(c,0x10428730,{person,manager,item,1});
    open_player_transfer_status(c,person,manager,item,1);
}
}

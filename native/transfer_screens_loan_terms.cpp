#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_loan_terms(Cpu& c) {
    Frame f(c,0x1042ca40,0x3c0,{.first_gpr=13});
    Kept<13> button_background(c,c.toc(-0x7bdc)); Kept<14> cup(c,c.r(14));
    Kept<15> parent(c,c.r(15)); Kept<16> offer(c,c.r(16));
    Kept<17> limit_or_affordable(c,c.r(17)); Kept<18> year(c,c.r(18));
    Kept<19> month(c,c.r(19)); Kept<20> percent(c,c.r(20));
    Kept<21> salary(c,c.r(21)); Kept<22> error_or_top_or_eligible(c,c.toc(-0x7ba8));
    Kept<23> assertion_flag_or_grid(c,c.toc(-0x7bac)); Kept<24> owner(c,c.r(24));
    Kept<25> club(c,c.r(25)); Kept<26> person(c,c.r(26));
    Kept<27> disabled(c,c.toc(-0x7bc4)); Kept<28> foreground(c,c.toc(-0x7bc8));
    Kept<29> text(c,c.toc(-0x7bb8)); Kept<30> pool(c,c.toc(slot::pool));
    Kept<31> screens(c,c.toc(slot::screens)); Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38;for(auto value:values){c.put32(f.at(offset),value);offset+=4;}
    };
    const auto container=[&]{return c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24);};
    const auto row=[&]{return c.u32(f.at(0x74));};
    const auto advance=[&](unsigned amount){c.set(5,row());c.put32(f.at(0x74),c.r(5)+amount);};
    const auto control=[&](bool active,unsigned back,unsigned column,unsigned y,unsigned style,unsigned bg,
                           unsigned flags,unsigned font,unsigned colour,unsigned caption,unsigned action=0) {
        stack_args({colour,caption});if(active){c.put32(f.at(0x40),action);c.put32(f.at(0x44),0);}
        return p.call(active?0x107444e0:0x107445a0,back,{screens,assertion_flag_or_grid,column,y,style,bg,flags,font});
    };
    const auto disabled_control=[&](unsigned back,unsigned column,unsigned font,unsigned action) {
        stack_args({0x30,c.u16(disabled),c.u16(disabled),0x2c,font,c.u16(foreground),text,0,action,0,assertion_flag_or_grid});
        p.call(0x10719740,back,{container(),6,0,0,0,0,column,0});
    };
    const auto query_owner=[&](unsigned back) {
        const auto record=p.call(0x10082f20,back,{indirect(c,slot::contracts),person});
        c.set(4,c.toc(slot::current_contract));compare_logical(c,record);c.put32(c.r(4),record);
        if(record){c.set(4,c.u32(c.toc(slot::current_contract)));c.set(3,indirect(c,slot::clubs));return c.r(3)+c.u32(c.r(4)+4)*0x248;}
        return 0u;
    };
    const auto matches_snapshot=[&](unsigned first,unsigned back) {
        if(compare(c,std::int8_t(percent),p.call(0x1073ef70,back,{screens,first}))!=0)return false;
        const auto saved_month=p.call(0x1073ef70,back+0x18,{screens,first+1});c.set(3,std::uint16_t(saved_month));
        if(compare(c,std::int8_t(month),c.r(3))!=0)return false;
        const auto saved_year=p.call(0x1073ef70,back+0x34,{screens,first+2});c.set(3,std::uint16_t(saved_year));
        if(compare_logical(c,std::uint16_t(year),c.r(3))!=0)return false;
        const auto saved_cup=p.call(0x1073ef70,back+0x50,{screens,first+3});c.set(3,std::uint32_t(std::int32_t(std::int8_t(saved_cup))));
        if(compare(c,std::int8_t(cup),c.r(3))!=0)return false;
        const auto saved_parent=p.call(0x1073ef70,back+0x6c,{screens,first+4});c.set(3,std::uint32_t(std::int32_t(std::int8_t(saved_parent))));
        return compare(c,std::int8_t(parent),c.r(3))==0;
    };
    const auto layout=c.toc(-0x4b74);
    c.set(7,c.u16(c.r(2)-0x3788));c.set(10,c.u32(layout));c.set(9,c.u16(layout+4));c.set(8,c.u8(layout+6));
    c.set(6,c.u16(c.r(2)-0x3784));c.set(5,c.u8(c.r(2)-0x3782));
    c.put32(f.at(0xa4),c.r(10));c.put16(f.at(0xa8),c.r(9));c.put8(f.at(0xaa),c.r(8));
    c.put16(f.at(0x68),c.r(7));c.put16(f.at(0x78),c.r(6));c.put8(f.at(0x7a),c.r(5));c.put32(f.at(0x74),0);
    p.call(0x1074b6e0,0x1042cab8,{f.at(0x9c),0});p.call(0x103721f0,0x1042cac0,{f.at(0x10c)});
    person=p.call(0x1073ef70,0x1042cacc,{screens,0});salary=p.call(0x1073ef70,0x1042cae0,{screens,1});
    owner=query_owner(0x1042caf8);
    percent=std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042cb3c,{screens,11})));
    if(compare(c,percent)<0){percent=0;p.history(0x1042cb5c,screens,11,0);}
    limit_or_affordable=p.call(0x1073ef70,0x1042cb68,{screens,2});
    c.put8(f.at(0x36d),p.call(0x1073ef70,0x1042cb7c,{screens,3}));
    c.put16(f.at(0x36a),p.call(0x1073ef70,0x1042cb94,{screens,4}));
    c.put8(f.at(0x36c),p.call(0x1073ef70,0x1042cbac,{screens,5}));
    c.put16(f.at(0x368),p.call(0x1073ef70,0x1042cbc4,{screens,6}));
    offer=p.call(0x1073ef70,0x1042cbdc,{screens,7});
    c.put32(f.at(0x364),p.call(0x1073ef70,0x1042cbf0,{screens,10}));
    month=std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042cc04,{screens,12})));
    year=std::uint16_t(p.call(0x1073ef70,0x1042cc18,{screens,13}));
    cup=std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042cc2c,{screens,14})));
    parent=std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042cc40,{screens,15})));
    const auto assertion=[&](unsigned first,unsigned buffer,unsigned detail,unsigned line,unsigned back) {
        report_assertion(c,{first,pool,pool+0x15,pool+0x22,f.at(buffer),f.at(detail),line});
        p.call(0x1074b7c0,back,{f.at(0x9c),missing});
    };
    if(compare_logical(c,person)==0){assertion(0x1042cc60,0xec,0x94,0x23dc,0x1042cca8);return;}
    c.set(3,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4));
    if(compare(c,c.r(3))==0){assertion(0x1042cce0,0xcc,0x8c,0x23ea,0x1042cd28);return;}
    club=c.u32(c.r(3)+0x24);
    if(compare_logical(c,club)==0){assertion(0x1042cd48,0xac,0x84,0x23f1,0x1042cd90);return;}
    p.call(0x107136d0,0x1042cda4,{person,0,0});
    p.call(0x100c7a50,0x1042cdb8,{club,1,f.at(0x6e),f.at(0x6c)});
    p.call(0x10744250,0x1042cdcc,{screens,c.u16(f.at(0x6c)),c.u16(f.at(0x6e)),club+0x221});
    p.call(0x100caeb0,0x1042cde0,{person,f.at(0x1d4),400,5});
    if(compare_logical(c,offer)!=0 && compare(c,c.u32(offer+8),c.u32(club))!=0) {
        const auto bidder=indirect(c,slot::clubs)+c.u32(offer+8)*0x248;
        c.set(9,indirect(c,slot::clubs));c.set(10,c.u32(offer+8)*0x248);
        p.call(0x10727890,0x1042ce58,{text,pool+0x2f5e,std::uint32_t(std::int32_t(c.i8(bidder+0x23b))),std::uint32_t(std::int32_t(c.i8(bidder+0x23b))),bidder+0x221,f.at(0x1d4)});
    } else p.call(0x10727890,0x1042ce6c,{text,pool+0x2faf,f.at(0x1d4)});
    p.call(0x107442f0,0x1042ce78,{screens,text});
    const auto existing=compare_logical(c,offer)!=0;error_or_top_or_eligible=140;
    if(existing) {
        stack_args({0,1,0,missing});assertion_flag_or_grid=p.call(0x107198c0,0x1042ced4,{container(),530,error_or_top_or_eligible,780,error_or_top_or_eligible+20,2,0,1});
        if(!matches_snapshot(16,0x1042cee8)) {
            p.call(0x10727890,0x1042cf70,{text,pool+0x5c9});control(true,0x1042cfb0,1,0,0x30,c.u16(disabled),12,1,c.u16(foreground),text,14);
        } else {p.call(0x10727890,0x1042cfc0,{text,pool+0x5c9});disabled_control(0x1042d03c,1,1,14);}
        if(compare_logical(c,c.u32(f.at(0x364)))!=0) {
            if(!matches_snapshot(21,0x1042d054)) {
                p.call(0x10727890,0x1042d0dc,{text,pool+0x5cf});control(true,0x1042d11c,0,0,0x30,c.u16(disabled),12,1,c.u16(foreground),text,13);
            } else {p.call(0x10727890,0x1042d12c,{text,pool+0x5cf});disabled_control(0x1042d1a8,0,1,13);}
        }
        error_or_top_or_eligible=error_or_top_or_eligible+25;
    } else p.history(0x1042d1c4,screens,30,missing);
    const auto rows=std::uint8_t(divide_signed<4>(545-error_or_top_or_eligible,p.call(0x1076d470,0x1042d1cc,{3})));
    stack_args({0,2,0,missing});assertion_flag_or_grid=p.call(0x107198c0,0x1042d228,{container(),110,error_or_top_or_eligible,780,545,7,f.at(0xa4),rows});
    c.put32(f.at(0x118),salary);p.call(0x10372590,0x1042d248,{f.at(0x10c),f.at(0x170),2,2});
    p.call(0x100caeb0,0x1042d25c,{person,f.at(0x1d4),400,1});
    p.call(0x10727890,0x1042d284,{text,pool+0x2fdc,std::uint32_t(std::int32_t(c.i8(owner+0x23b))),std::uint32_t(std::int32_t(c.i8(owner+0x23b))),f.at(0x1d4),f.at(0x170),owner+0x221});
    p.call(0x1074b830,0x1042d290,{f.at(0x9c),text});
    p.call(0x1074bab0,0x1042d2a4,{f.at(0x9c),f.at(0x1d4),2007,person});
    p.call(0x1074bab0,0x1042d2b8,{f.at(0x9c),f.at(0x170),7,salary});
    p.call(0x1074bab0,0x1042d2cc,{f.at(0x9c),owner+0x221,2005,owner});
    if(compare_logical(c,club,query_owner(0x1042d2dc))!=0) {
        c.put32(f.at(0x118),limit_or_affordable);p.call(0x10372590,0x1042d330,{f.at(0x10c),f.at(0x170),2,2});
        p.call(0x10727890,0x1042d340,{text,pool+0x306b,f.at(0x170)});
        p.call(0x1074b960,0x1042d34c,{f.at(0x9c),text});
        p.call(0x1074bab0,0x1042d360,{f.at(0x9c),f.at(0x170),7,limit_or_affordable});
    }
    stack_args({0});p.call(0x1074bcd0,0x1042d38c,{f.at(0x9c),assertion_flag_or_grid,120,770,f.at(0x74),3,c.u16(foreground),0});
    p.call(0x1074c570,0x1042d394,{f.at(0x9c)});advance(1);
    control(false,0x1042d3d4,0,row(),1,0,12,3,c.u16(foreground),0);
    advance(1);p.call(0x10727890,0x1042d3ec,{text,pool+0x30b2});
    control(true,0x1042d42c,1,row(),1,0,1,3,c.u16(foreground),text);
    const bool before_min=compare_logical(c,std::uint16_t(year),c.u16(f.at(0x36a)))==0 && compare(c,std::int8_t(month),c.i8(f.at(0x36d)))<=0;
    control(!before_min,before_min?0x1042d484:0x1042d4cc,2,row(),0x30,c.u16(button_background),12,3,c.u16(before_min?disabled:foreground),pool+0x4a9,1);
    const auto first_day=p.call(0x107182f0,0x1042d4d4,{c.toc(slot::transfer_context)});
    const auto day=compare(c,std::int16_t(first_day),28)>0 ? 28 : std::int16_t(p.call(0x107182f0,0x1042d4f0,{c.toc(slot::transfer_context)}));
    p.call(0x107156f0,0x1042d50c,{f.at(0x7c),std::uint32_t(std::int32_t(day)),month,year,missing});
    p.call(0x107168a0,0x1042d520,{f.at(0x7c),text,2000,255});
    const auto current_owner=query_owner(0x1042d530);
    const auto eligible=p.call(0x103fa850,0x1042d580,{indirect(c,slot::transfers),current_owner,person,0,f.at(0x7c),1});
    if(byte_result(c,eligible)) {control(false,0x1042d5b8,3,row(),1,0,12,3,c.u16(foreground),text);error_or_top_or_eligible=1;}
    else {control(false,0x1042d5f0,3,row(),1,0,12,3,c.u16(disabled),text);error_or_top_or_eligible=0;}
    const bool after_max=compare_logical(c,std::uint16_t(year),c.u16(f.at(0x368)))==0 && compare(c,std::int8_t(month),c.i8(f.at(0x36c)))>=0;
    control(!after_max,after_max?0x1042d64c:0x1042d694,4,row(),0x30,c.u16(button_background),12,3,c.u16(after_max?disabled:foreground),pool+0x4ab,2);
    advance(1);p.call(0x10727890,0x1042d6ac,{text,pool+0x30c3});
    control(true,0x1042d6ec,1,row(),1,0,1,3,c.u16(foreground),text);
    const bool has_contribution=byte_result(c,percent);
    control(has_contribution,has_contribution?0x1042d770:0x1042d728,2,row(),0x30,c.u16(button_background),12,3,c.u16(has_contribution?foreground:disabled),pool+0x4a9,3);
    // Wrapped multiplication precedes the signed division by 100.
    const auto product=std::uint32_t(std::int32_t(std::int8_t(percent)))*salary;
    const auto high=std::int32_t((std::int64_t(std::int32_t(product))*0x51eb851f)>>32);
    c.s.wr<1>(reg::xer_ca,high<0 && (std::uint32_t(high)&31u)!=0);
    const auto floor=high>>5;const auto amount=std::uint32_t(floor)+(floor<0);
    c.put32(f.at(0x70),amount);c.set(6,floor<0);
    p.call(0x1077b720,0x1042d7a4,{text,pool+0x4f9,std::uint32_t(std::int32_t(std::int8_t(percent)))});
    const auto live_amount=c.u32(f.at(0x70));
    c.set(4,std::uint32_t(std::int32_t(limit_or_affordable)>>31));c.set(3,live_amount>>31);
    const auto carry=std::uint32_t(limit_or_affordable)>=live_amount;
    const auto signed_comparison=std::uint64_t(c.r(4))+c.r(3)+carry;
    c.s.wr<1>(reg::xer_ca,(signed_comparison>>32)!=0);
    limit_or_affordable=std::int32_t(std::int8_t(signed_comparison));
    const auto affordable=byte_result(c,limit_or_affordable);
    control(false,affordable?0x1042d830:0x1042d7f8,3,row(),1,c.u16(c.toc(-0x7bcc)),12,3,c.u16(affordable?std::uint32_t(foreground):c.toc(-0x7a74)),text);
    const bool full_contribution=compare(c,std::int8_t(percent),100)==0;
    control(!full_contribution,full_contribution?0x1042d870:0x1042d8b8,4,row(),0x30,c.u16(button_background),12,3,c.u16(full_contribution?disabled:foreground),pool+0x4ab,4);
    advance(1);c.set(7,c.u32(f.at(0x70)));c.put32(f.at(0x118),c.r(7));
    p.call(0x10372590,0x1042d8e0,{f.at(0x10c),f.at(0x170),0,1});
    p.call(0x1077b720,0x1042d8f0,{text,pool+0x30d5,f.at(0x170)});
    control(false,0x1042d924,3,row(),1,0,12,3,c.u16(c.toc(-0x7bf4)),text);
    advance(2);p.call(0x10727890,0x1042d93c,{text,pool+0x30da});
    p.call(0x1074b830,0x1042d948,{f.at(0x9c),text});p.call(0x1074bab0,0x1042d95c,{f.at(0x9c),text,8,0});
    const auto cup_selected=compare_logical(c,p.call(0x1073ef70,0x1042d968,{screens,14}))!=0;
    stack_args({0});p.call(0x1074bcd0,cup_selected?0x1042d99c:0x1042d9cc,{f.at(0x9c),assertion_flag_or_grid,144,700,f.at(0x74),3,c.u16(cup_selected?foreground:disabled),2});
    p.call(0x1074c570,0x1042d9d4,{f.at(0x9c)});advance(1);
    p.call(0x10727890,0x1042da00,{text,pool+0x30f9,std::uint32_t(std::int32_t(c.i8(owner+0x23b))),std::uint32_t(std::int32_t(c.i8(owner+0x23b))),owner+0x221});
    p.call(0x1074b830,0x1042da0c,{f.at(0x9c),text});p.call(0x1074bab0,0x1042da20,{f.at(0x9c),text,9,0});
    const auto parent_selected=compare_logical(c,p.call(0x1073ef70,0x1042da2c,{screens,15}))!=0;
    stack_args({0});p.call(0x1074bcd0,parent_selected?0x1042da60:0x1042da90,{f.at(0x9c),assertion_flag_or_grid,144,700,f.at(0x74),3,c.u16(parent_selected?foreground:disabled),2});
    p.call(0x1074c570,0x1042da98,{f.at(0x9c)});p.call(0x102a1070,0x1042daa4,{4,0});
    const auto footer=[&](unsigned back,unsigned columns,unsigned layout_offset) {
        stack_args({0,1,c.u16(f.at(0x6c)),missing});
        assertion_flag_or_grid=p.call(0x107198c0,back,{container(),100,555,790,590,columns,f.at(layout_offset),1});
    };
    const auto footer_button=[&](unsigned format,unsigned format_back,unsigned back,unsigned column,unsigned action) {
        p.call(0x10727890,format_back,{text,pool+format});
        control(true,back,column,0,0x30,c.u16(disabled),12,3,c.u16(foreground),text,action);
    };
    const auto disabled_footer=[&](unsigned back,unsigned action) {
        cup=c.u16(foreground);button_background=0x2c;disabled_control(back,2,3,action);
    };
    if(compare_logical(c,offer)!=0 && compare_logical(c,c.u32(f.at(0x364)))==0) {
        footer(0x1042db08,3,0x78);footer_button(0x5ed,0x1042db1c,0x1042db5c,0,5);footer_button(0x608,0x1042db68,0x1042dba8,1,12);
        if(!matches_snapshot(16,0x1042dbb4)) {
            p.call(0x10727890,0x1042dc3c,{text,pool+0x60f});
            if(byte_result(c,error_or_top_or_eligible))control(true,0x1042dc84,2,0,0x30,c.u16(disabled),12,3,c.u16(foreground),text,10);
            else disabled_footer(0x1042dd04,10);
        } else {
            p.call(0x10727890,0x1042dd14,{text,pool+0x619});
            if(byte_result(c,error_or_top_or_eligible))control(true,0x1042dd5c,2,0,0x30,c.u16(disabled),12,3,c.u16(foreground),text,11);
            else disabled_footer(0x1042dddc,11);
        }
    } else if(compare_logical(c,offer)!=0) {
        footer(0x1042de38,3,0x78);footer_button(0x5ed,0x1042de4c,0x1042de8c,0,5);footer_button(0x5fb,0x1042de98,0x1042ded8,1,15);
        p.call(0x10727890,0x1042dee4,{text,pool+0x602});
        if(byte_result(c,error_or_top_or_eligible) && byte_result(c,limit_or_affordable))control(true,0x1042df34,2,0,0x30,c.u16(disabled),12,3,c.u16(foreground),text,6);
        else disabled_footer(0x1042dfb4,6);
    } else {
        footer(0x1042e008,2,0x68);footer_button(0x5ed,0x1042e01c,0x1042e05c,0,5);
        p.call(0x10727890,0x1042e068,{text,pool+0x602});
        if(byte_result(c,error_or_top_or_eligible) && byte_result(c,limit_or_affordable))control(true,0x1042e0b8,1,0,0x30,c.u16(disabled),12,3,c.u16(foreground),text,6);
        else control(false,0x1042e0ec,1,0,0x30,c.u16(disabled),0x2c,3,c.u16(foreground),text);
    }
    p.call(0x1074b7c0,0x1042e0f8,{f.at(0x9c),missing});
}
}

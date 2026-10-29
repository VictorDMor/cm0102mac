#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_player_status(Cpu& c) {
    Frame f(c,0x10428ab0,0x350,{.first_gpr=13});
    Kept<13> price_ceiling_or_choices_grid(c,c.r(13));
    Kept<14> error_or_rows(c,c.toc(-0x7ba8));
    Kept<15> kind_or_initial_price(c,c.r(15));
    Kept<16> grid(c,c.r(16));
    Kept<17> limited_or_listed(c,c.r(17));
    Kept<18> owner_or_price_menu(c,c.r(18));
    Kept<19> squad(c,c.r(19));
    Kept<20> price_or_loan(c,c.r(20));
    Kept<21> person(c,c.r(21));
    Kept<22> disabled(c,c.toc(-0x7bc4));
    Kept<23> button_background_or_requested(c,c.toc(-0x7bdc));
    Kept<24> foreground(c,c.toc(-0x7bc8));
    Kept<25> amount_background(c,c.toc(-0x7bd4));
    Kept<26> selected(c,c.toc(-0x7bd0));
    Kept<27> menu_background(c,c.toc(-0x7bcc));
    Kept<28> text(c,c.toc(-0x7bb8));
    Kept<29> pool(c,c.toc(slot::pool));
    Kept<30> transfers(c,c.toc(slot::transfers));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for(auto value:values) {c.put32(f.at(offset),value);offset+=4;}
    };
    const auto container=[&] {return c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24);};
    const auto row=[&] {return c.u32(f.at(0x78));};
    const auto advance=[&](unsigned count) {c.put32(f.at(0x78),row()+count);};
    // Factories differ by whether an action/payload is appended to the stack.
    const auto control=[&](bool active,unsigned back,unsigned parent,unsigned column,unsigned y,
                           unsigned style,unsigned background,unsigned flags,unsigned font,
                           unsigned colour,unsigned caption,unsigned action=0,unsigned payload=0) {
        stack_args({colour,caption});
        if(active) {c.put32(f.at(0x40),action);c.put32(f.at(0x44),payload);}
        return p.call(active?0x107444e0:0x107445a0,back,
            {screens,parent,column,y,style,background,flags,font});
    };
    const auto descriptor=c.toc(-0x4b48);
    c.set(5,c.u16(c.r(2)-0x379c)); c.set(8,c.u32(descriptor));
    c.set(7,c.u16(descriptor+4));c.set(6,c.u8(descriptor+6));
    c.put32(f.at(0x8c),c.r(8));c.put16(f.at(0x90),c.r(7));c.put8(f.at(0x92),c.r(6));
    c.put16(f.at(0x6c),c.r(5));c.put16(f.at(0x68),c.u16(c.r(2)-0x3798));
    p.call(0x1074b6e0,0x10428b24,{f.at(0x84),0});
    c.put32(f.at(0x78),0);c.put32(f.at(0x74),0);
    const auto id=p.call(0x1073ef70,0x10428b40,{screens,0});
    c.set(5,id*0x70);person=indirect(c,slot::people)+c.r(5);
    price_or_loan=p.call(0x1073ef70,0x10428b5c,{screens,1});
    squad=std::int32_t(std::int8_t(p.call(0x1073ef70,0x10428b70,{screens,3})));
    c.put32(f.at(0x2f4),p.call(0x1073ef70,0x10428b84,{screens,8}));
    c.put8(f.at(0x2f0),p.call(0x1073ef70,0x10428b98,{screens,10}));
    owner_or_price_menu=p.call(0x1073ef70,0x10428bb0,{screens,11});
    p.call(0x1073ef70,0x10428bc4,{screens,12});
    limited_or_listed=std::int32_t(std::int8_t(p.call(0x1073ef70,0x10428bd0,{screens,12})));
    price_ceiling_or_choices_grid=p.call(0x1073ef70,0x10428be4,{screens,15});
    const auto prices=c.toc(-0x4b4c);
    for(unsigned i=0;i<10;++i)c.put32(f.at(0xd4+i*4),c.u32(prices+i*4));
    for(unsigned r=6;r<=10;++r)c.set(r,c.u32(prices+(13-r)*4));
    grid=c.u32(prices+36);
    const auto contract=p.call(0x10082f90,0x10428c50,{indirect(c,slot::contracts),person,owner_or_price_menu});
    if(compare_logical(c,contract)==0) {
        report_assertion(c,{0x10428c6c,pool,pool+0x15,pool+0x22,f.at(0xb4),f.at(0x7c),0x1d01});
        c.set(5,c.toc(-0x7bac));
        p.call(0x1074b7c0,0x10428cb8,{f.at(0x84),missing});return;
    }
    const auto encoded=(std::uint32_t(c.u8(contract+0x38))<<24)&0xfe000000u;
    c.s.wr<1>(reg::xer_ca,std::int32_t(encoded)<0 && (encoded&0x03ffffffu)!=0);
    kind_or_initial_price=std::uint32_t(std::int32_t(encoded)>>26);
    p.call(0x107136d0,0x10428cd8,{person,0,0});
    p.call(0x100c7a50,0x10428cec,{owner_or_price_menu,1,f.at(0x72),f.at(0x70)});
    p.call(0x100caeb0,0x10428d00,{person,f.at(0xfc),500,1});
    p.call(0x10744250,0x10428d14,{screens,c.u16(f.at(0x70)),c.u16(f.at(0x72)),f.at(0xfc)});
    p.call(0x10727890,0x10428d20,{text,pool+0x29a8});
    p.call(0x107442f0,0x10428d2c,{screens,text});
    error_or_rows=std::int32_t(std::int8_t(divide_signed<4>(405,p.call(0x1076d470,0x10428d34,{3}))));
    stack_args({0,2,0,missing});
    grid=p.call(0x107198c0,0x10428d98,{container(),110,140,780,545,7,f.at(0x8c),std::uint8_t(error_or_rows)});
    if(compare(c,std::int8_t(kind_or_initial_price),6)!=0) {
        if(compare(c,std::int8_t(kind_or_initial_price),3)==0)
            p.call(0x10727890,0x10428dc4,{text,pool+0x29c2,f.at(0xfc)});
        else if(compare(c,std::int8_t(kind_or_initial_price),1)==0)
            p.call(0x10727890,0x10428de4,{text,pool+0x2a07,f.at(0xfc)});
        else {
            c.put32(f.at(0x74),c.u32(person+8));
            p.call(0x10009f20,0x10428dfc,{f.at(0x74),6});
            p.call(0x1000b410,0x10428e14,{f.at(0x94),30,c.r(3),c.r(4),6});
            p.call(0x10727890,0x10428e28,{text,pool+0x2a59,f.at(0xfc),f.at(0x94)});
            p.call(0x1074bab0,0x10428e3c,{f.at(0x84),f.at(0x94),1,0});
        }
        p.call(0x1074b830,0x10428e48,{f.at(0x84),text});
        p.call(0x1074bab0,0x10428e5c,{f.at(0x84),f.at(0xfc),2007,person});
        c.put32(f.at(0x78),0);stack_args({0});
        p.call(0x1074bcd0,0x10428e90,{f.at(0x84),grid,110,780,f.at(0x78),3,c.u16(foreground),0});
        c.set(5,row());advance(2);p.call(0x10727890,0x10428ea8,{text,pool+0x2abd});
        control(false,0x10428ed8,grid,1,row(),1,0,1,3,c.u16(foreground),text);
        const bool decrease_disabled=compare(c,price_or_loan)<=0 || compare(c,std::int8_t(kind_or_initial_price),3)==0 || compare(c,std::int8_t(kind_or_initial_price),1)==0;
        const auto decrement=control(!decrease_disabled,decrease_disabled?0x10428f2c:0x10428f78,
            grid,2,row(),0x30,c.u16(button_background_or_requested),12,3,
            c.u16(decrease_disabled?disabled:foreground),pool+0x4a9,2);
        p.history(0x10428f8c,screens,2,std::uint32_t(std::int32_t(std::int16_t(decrement))));
        if(compare(c,std::int8_t(kind_or_initial_price),3)==0 || compare(c,std::int8_t(kind_or_initial_price),1)==0) {
            control(false,0x10428fd8,grid,3,row(),1,c.u16(amount_background),12,3,c.u16(disabled),pool+0x2aca);
            owner_or_price_menu=missing;
        } else {
            c.put32(f.at(0x74),price_or_loan);p.call(0x10009f20,0x10428ff0,{f.at(0x74),5});
            p.call(0x1000b410,0x10429008,{text,2000,c.r(3),c.r(4),5});
            const auto amount=control(true,0x10429048,grid,3,row(),1,c.u16(amount_background),12,3,c.u16(foreground),text);
            if(!byte_result(c,limited_or_listed)) {
                stack_args({0,0x130,c.u16(menu_background),std::uint32_t(std::int32_t(std::int16_t(amount)))});
                owner_or_price_menu=p.call(0x107198c0,0x104290a4,{container(),0,0,0,0,0,0,0});
            } else owner_or_price_menu=missing;
        }
        c.set(3,30000000);
        const bool increase_disabled=compare(c,price_or_loan,c.r(3))>=0 || compare(c,std::int8_t(kind_or_initial_price),3)==0 ||
            compare(c,std::int8_t(kind_or_initial_price),1)==0 || (byte_result(c,limited_or_listed) && compare(c,price_or_loan,price_ceiling_or_choices_grid)>=0);
        control(!increase_disabled,increase_disabled?0x1042911c:0x10429164,grid,4,row(),0x30,
            c.u16(button_background_or_requested),12,3,c.u16(increase_disabled?disabled:foreground),pool+0x4ab,3);
        p.call(0x10727890,0x10429174,{text,pool+0x2acc,pool+0x2af8});
        const bool automatic=byte_result(c,c.u8(f.at(0x2f0)));
        control(true,automatic?0x104291c0:0x10429204,grid,5,row(),automatic?0x70:0x30,
            c.u16(button_background_or_requested),12,2,c.u16(foreground),text,5,automatic?0:1);
        advance(2);
    } else {c.put32(f.at(0x78),0);owner_or_price_menu=missing;}
    p.call(0x10727890,0x1042922c,{text,pool+0x2af9});
    control(true,0x1042926c,grid,1,row(),1,0,1,3,c.u16(foreground),text);
    advance(2);stack_args({0,1,0,missing});
    price_ceiling_or_choices_grid=p.call(0x107198c0,0x104292c8,
        {container(),110,140,780,545,2,f.at(0x68),std::uint8_t(error_or_rows)});
    error_or_rows=1;
    while(compare(c,std::int16_t(error_or_rows),8)<0) {
        p.call(0x1036f7e0,0x104292ec,{c.u32(c.u32(transfers)+0x988),std::uint32_t(std::int32_t(std::int8_t(error_or_rows))),text,2000});
        const auto eligible=p.call(0x1036f920,0x10429300,{c.u32(c.u32(transfers)+0x988),person,std::uint32_t(std::int32_t(std::int8_t(error_or_rows)))});
        if(!byte_result(c,eligible))control(false,0x10429338,price_ceiling_or_choices_grid,1,row(),1,0,1,3,c.u16(disabled),text);
        else {
            const bool chosen=compare(c,std::int8_t(squad),std::int16_t(error_or_rows))==0;
            control(true,chosen?0x1042938c:0x104293d0,price_ceiling_or_choices_grid,1,row(),1,0,1,3,
                c.u16(chosen?selected:foreground),text,7,std::uint32_t(std::int32_t(std::int16_t(error_or_rows))));
        }
        error_or_rows=error_or_rows+1;advance(1);
    }
    const bool show_transfer_status=compare(c,std::int8_t(kind_or_initial_price),6)!=0;advance(1);
    if(show_transfer_status) {
        error_or_rows=std::int32_t(std::int8_t(p.call(0x1073ef70,0x10429410,{screens,6})));
        kind_or_initial_price=std::int32_t(std::int8_t(p.call(0x1073ef70,0x10429424,{screens,14})));
        limited_or_listed=std::int32_t(std::int8_t(p.call(0x1073ef70,0x10429438,{screens,4})));
        price_or_loan=std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042944c,{screens,5})));
        button_background_or_requested=std::int32_t(std::int8_t(p.call(0x1073ef70,0x10429460,{screens,13})));
        p.call(0x10727890,0x10429474,{text,pool+0x2b06});
        control(true,0x104294b4,grid,1,row(),1,0,1,3,c.u16(foreground),text);
        c.set(5,row());advance(1);p.call(0x10727890,0x104294cc,{text,pool+0x2b16});
        const auto listing=[&](unsigned value,unsigned action,unsigned disabled_back,unsigned selected_back,unsigned active_back) {
            if(byte_result(c,error_or_rows) && byte_result(c,kind_or_initial_price))
                control(false,disabled_back,price_ceiling_or_choices_grid,1,row(),1,0,1,3,c.u16(disabled),text);
            else {
                const bool chosen=byte_result(c,value);
                control(true,chosen?selected_back:active_back,price_ceiling_or_choices_grid,1,row(),1,0,1,3,c.u16(chosen?selected:foreground),text,action);
            }
        };
        listing(limited_or_listed,9,0x1042950c,0x10429558,0x1042959c);advance(1);
        if(compare(c,c.u8(person+0x62),15)!=0) {
            p.call(0x10727890,0x104295c0,{text,pool+0x2b35});
            listing(price_or_loan,8,0x10429600,0x1042964c,0x10429690);advance(1);
        }
        p.call(0x10727890,0x104296a8,{text,pool+0x2b57});
        if(byte_result(c,error_or_rows) && !byte_result(c,button_background_or_requested))
            control(true,0x104296f8,price_ceiling_or_choices_grid,1,row(),1,0,1,3,c.u16(foreground),text,10);
        else if(byte_result(c,button_background_or_requested) && !byte_result(c,kind_or_initial_price))
            control(true,0x1042974c,price_ceiling_or_choices_grid,1,row(),1,0,1,3,c.u16(selected),text,10);
        else if(byte_result(c,button_background_or_requested) && byte_result(c,kind_or_initial_price))
            control(false,0x10429790,price_ceiling_or_choices_grid,1,row(),1,0,1,3,c.u16(selected),text);
        else control(false,0x104297c4,price_ceiling_or_choices_grid,1,row(),1,0,1,3,c.u16(disabled),text);
    }
    p.call(0x102a1070,0x104297d0,{4,0});stack_args({0,1,c.u16(f.at(0x70)),missing});
    grid=p.call(0x107198c0,0x10429820,{container(),100,555,790,590,2,f.at(0x6c),1});
    p.call(0x10727890,0x10429834,{text,pool+0x5ed});
    control(true,0x10429874,grid,0,0,0x30,c.u16(disabled),12,3,c.u16(foreground),text,4);
    p.call(0x10727890,0x10429884,{text,pool+0x2b81,pool+0x2af8});
    // With no selected squad status, only the absence of a news item enables Set.
    const bool can_apply=byte_result(c,squad) || compare(c,c.u32(f.at(0x2f4)),missing)==0;
    control(can_apply,can_apply?0x104298d8:0x1042990c,grid,1,0,0x30,c.u16(disabled),can_apply?12:0x2c,3,c.u16(foreground),text,6);
    if(compare(c,std::int16_t(owner_or_price_menu),-1)!=0) {
        error_or_rows=0;
        while(compare(c,std::int16_t(error_or_rows),10)<0) {
            p.call(0x1000a330,0x10429934,{f.at(0x74),std::uint32_t(std::int32_t(std::int8_t(error_or_rows))),f.at(0xd4),10});
            p.call(0x10009f20,0x10429940,{f.at(0x74),5});
            p.call(0x1000b410,0x10429958,{text,2000,c.r(3),c.r(4),5});
            kind_or_initial_price=c.u32(f.at(0x74));price_ceiling_or_choices_grid=c.u16(selected);
            stack_args({16,c.u16(menu_background),price_ceiling_or_choices_grid,1,1,c.u16(amount_background),text,0,11,kind_or_initial_price,owner_or_price_menu});
            p.call(0x10719740,0x104299d8,{container(),2,0,0,0,0,0,0});error_or_rows=error_or_rows+1;
        }
    }
    p.call(0x1074b7c0,0x104299f4,{f.at(0x84),missing});
}
}

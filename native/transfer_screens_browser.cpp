#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_transfers_browser(Cpu& c) {
    Frame f(c,0x10426aa0,0x1b0,{.first_gpr=13});
    Kept<13> year_or_row(c,c.r(13)); Kept<14> people(c,c.toc(slot::people));
    Kept<15> group_or_record(c,missing); Kept<16> glyph_or_nation_or_grid(c,c.toc(-0x740c));
    Kept<17> menu_or_indices_or_to(c,c.r(17)); Kept<18> submenu_or_from(c,c.r(18));
    Kept<19> menu_or_capacity(c,c.r(19)); Kept<20> item(c,c.r(20));
    Kept<21> top_or_name_colour(c,c.r(21)); Kept<22> nation_filter(c,c.r(22));
    Kept<23> to_colour(c,c.toc(-0x7bf4)); Kept<24> foreground(c,c.toc(-0x7bc8));
    Kept<25> background(c,c.toc(-0x7bd4)); Kept<26> selected(c,c.toc(-0x7bd0));
    Kept<27> menu_background(c,c.toc(-0x7bcc)); Kept<28> text(c,c.toc(-0x7bb8));
    Kept<29> pool(c,c.toc(slot::pool)); Kept<30> clubs(c,c.toc(slot::clubs));
    Kept<31> screens(c,c.toc(slot::screens)); Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38;for(auto value:values){c.put32(f.at(offset),value);offset+=4;}
    };
    const auto container=[&]{return c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24);};
    const auto menu_item=[&](unsigned back,unsigned parent,unsigned style,unsigned flags,unsigned caption,unsigned action,unsigned payload) {
        stack_args({flags,c.u16(menu_background),c.u16(selected),1,1,c.u16(background),caption,0,action,payload,parent});
        return p.call(0x10719740,back,{container(),style,0,0,0,0,0,0});
    };
    const auto menu=[&](unsigned back,unsigned parent,unsigned style) {
        stack_args({0,style,c.u16(menu_background),parent});
        return p.call(0x107198c0,back,{container(),0,0,0,0,0,0,0});
    };
    const auto screen_button=[&](unsigned back,unsigned x,unsigned y,unsigned right,unsigned bottom,unsigned colour,unsigned format,
                                unsigned caption,unsigned action,unsigned flags) {
        stack_args({format,c.u16(foreground),caption,action,0});
        return p.call(0x10744430,back,{screens,x,y,right,bottom,0x30,colour,flags});
    };
    const auto cell=[&](bool active,unsigned back,unsigned column,unsigned style,unsigned colour,unsigned bg,unsigned flags,unsigned font,unsigned action=0,unsigned payload=0) {
        stack_args({colour,text});if(active){c.put32(f.at(0x40),action);c.put32(f.at(0x44),payload);}
        return p.call(active?0x107444e0:0x107445a0,back,{screens,glyph_or_nation_or_grid,column,year_or_row,style,bg,flags,font});
    };
    c.set(5,c.u32(c.toc(-0x4b3c)));c.put32(f.at(0x78),c.r(5));c.put8(f.at(0x7c),c.u8(c.toc(-0x4b3c)+4));
    nation_filter=p.call(0x1073ef70,0x10426b00,{screens,0});
    c.put8(f.at(0x157),p.call(0x1073ef70,0x10426b14,{screens,1}));
    year_or_row=std::uint16_t(p.call(0x1073ef70,0x10426b2c,{screens,2}));
    c.put8(f.at(0x156),p.call(0x1073ef70,0x10426b40,{screens,3}));
    c.put32(f.at(0x70),p.call(0x1073ef70,0x10426b58,{screens,4}));
    c.put32(f.at(0x74),p.call(0x1073ef70,0x10426b6c,{screens,5}));
    c.put16(f.at(0x154),p.call(0x1073ef70,0x10426b80,{screens,6}));
    if(compare(c,nation_filter,-1)>0)p.call(0x10712eb0,0x10426bac,{indirect(c,-0x7b98)+nation_filter*0x128,0,0});
    else p.call(0x10712d00,0x10426bb4);
    p.call(0x10727890,0x10426bc0,{text,pool+0x28c1});
    p.call(0x10744250,0x10426bd8,{screens,c.u16(c.toc(-0x7bdc)),c.u16(foreground),text});
    p.call(0x102a1070,0x10426be4,{4,0});
    if(byte_result(c,c.u8(f.at(0x156)))) {
        const auto month_name=p.call(0x107189d0,0x10426bf8,{c.u8(f.at(0x157))});
        p.call(0x10727890,0x10426c0c,{text,pool+0x28cb,month_name,std::uint16_t(year_or_row)});
    } else p.call(0x10727890,0x10426c20,{text,pool+0x28e4,std::uint16_t(year_or_row)});
    p.call(0x107442f0,0x10426c2c,{screens,text});
    const auto view_control=screen_button(0x10426c78,105,85,120,105,c.u16(background),1,0,0,12);
    menu_or_indices_or_to=menu(0x10426cc4,view_control,0x30);
    p.call(0x10727890,0x10426cd8,{text,pool+0x28f0});
    if(byte_result(c,c.u8(f.at(0x156)))) {
        item=0;menu_or_capacity=c.u16(selected);menu_item(0x10426d64,menu_or_indices_or_to,0x2002,16,text,0,0);
    } else {item=1;menu_or_capacity=c.u16(selected);menu_item(0x10426de8,menu_or_indices_or_to,2,16,text,1,1);}
    p.call(0x10727890,0x10426df4,{text,pool+0x28f7});
    if(!byte_result(c,c.u8(f.at(0x156)))) {
        item=0;menu_or_capacity=c.u16(selected);menu_item(0x10426e80,menu_or_indices_or_to,0x2002,16,text,0,0);
    } else {item=0;menu_or_capacity=c.u16(selected);menu_item(0x10426f04,menu_or_indices_or_to,2,16,text,1,0);}
    if(compare(c,c.u32(glyph_or_nation_or_grid),-1)!=0) {
        p.call(0x1077b720,0x10426f20,{text,pool+0x28fd,42});
        screen_button(0x10426f6c,743,85,763,105,c.u16(c.toc(-0x7bdc)),0,text,2,12);
    } else screen_button(0x10426fc0,743,85,763,105,c.u16(c.toc(-0x7bdc)),1,pool+0x2900,2,12);
    if(compare(c,c.u32(glyph_or_nation_or_grid),-1)!=0) {
        p.call(0x1077b720,0x10426fdc,{text,pool+0x28fd,43});
        screen_button(0x10427028,765,85,785,105,c.u16(c.toc(-0x7bdc)),0,text,3,12);
    } else screen_button(0x1042707c,765,85,785,105,c.u16(c.toc(-0x7bdc)),1,pool+0x2902,3,12);
    top_or_name_colour=145;p.call(0x10727890,0x1042708c,{text,pool+0x2904});
    const auto nation_control=screen_button(0x104270d8,110,top_or_name_colour,235,top_or_name_colour+20,c.u16(c.toc(-0x7bc4)),1,text,0,12);
    menu_or_capacity=menu(0x10427124,nation_control,0x130);
    p.call(0x10727890,0x10427138,{text,pool+0x2909});
    if(compare(c,nation_filter,-1)==0) {
        menu_or_indices_or_to=0;glyph_or_nation_or_grid=c.u16(selected);menu_item(0x104271c0,menu_or_capacity,0x2002,16,text,0,0);
    } else {menu_or_indices_or_to=missing;glyph_or_nation_or_grid=c.u16(selected);menu_item(0x10427244,menu_or_capacity,2,16,text,4,missing);}
    menu_or_indices_or_to=0;item=0;glyph_or_nation_or_grid=c.u16(selected);
    menu_item(0x104272cc,menu_or_capacity,1,0x1000010,0,0,0);
    menu_or_indices_or_to=p.call(0x10192f30,0x104272d4,{1});
    p.call(0x101932f0,0x104272f0,{f.at(0x80),0,menu_or_indices_or_to,1});
    item=0;
    while(compare(c,item,34)<0) {
        glyph_or_nation_or_grid=c.u32(c.toc(-0x7b44)+std::uint32_t(std::int32_t(c.i8(menu_or_indices_or_to+item)))*0x4c);
        if(compare_logical(c,glyph_or_nation_or_grid)!=0 && byte_result(c,c.u8(glyph_or_nation_or_grid+0x120))) {
            bool new_group=false;
            if(compare(c,std::int8_t(p.call(0x101933f0,0x10427328,{f.at(0x80)})),1)>0) {
                const auto group=p.call(0x101934f0,0x10427340,{f.at(0x80),glyph_or_nation_or_grid});
                new_group=compare(c,std::int8_t(group_or_record),std::int8_t(group))!=0;
            }
            if(new_group) {
                group_or_record=p.call(0x101934f0,0x1042735c,{f.at(0x80),glyph_or_nation_or_grid});
                p.call(0x10193420,0x10427374,{f.at(0x80),text,2000,group_or_record});
                submenu_or_from=0;const auto heading=menu_item(0x104273f4,menu_or_capacity,2,16,text,0,0);
                submenu_or_from=menu(0x10427440,heading,0x30);
            } else if(compare(c,std::int8_t(p.call(0x101933f0,0x10427450,{f.at(0x80)})),1)==0)submenu_or_from=menu_or_capacity;
            p.call(0x1077b720,0x10427470,{text,pool+0x2918,glyph_or_nation_or_grid+0xe1});
            if(compare(c,c.u32(glyph_or_nation_or_grid+0x68),nation_filter)==0) {
                glyph_or_nation_or_grid=0;menu_item(0x104274fc,submenu_or_from,0x2002,16,text,0,0);
            } else {
                const auto nation=c.u32(glyph_or_nation_or_grid+0x68);glyph_or_nation_or_grid=0;
                menu_item(0x10427580,submenu_or_from,2,16,text,4,nation);
            }
        }
        item=item+1;
    }
    top_or_name_colour=top_or_name_colour+25;
    if(compare_logical(c,c.u32(f.at(0x70)))==0) {
        if(byte_result(c,c.u8(f.at(0x156))))p.call(0x103f3670,0x104275cc,
            {indirect(c,slot::transfers),c.u8(f.at(0x157)),year_or_row,c.u8(f.at(0x157)),year_or_row,f.at(0x70),f.at(0x74)});
        else p.call(0x103f3670,0x104275f4,{indirect(c,slot::transfers),0,year_or_row,11,year_or_row,f.at(0x70),f.at(0x74)});
        item=0;
        while(compare(c,item,c.u32(f.at(0x74)))<0) {
            group_or_record=c.u32(f.at(0x70))+item*0x28;
            menu_or_indices_or_to=compare(c,c.u32(group_or_record+0x10),-1)>0 ? c.u32(clubs)+c.u32(group_or_record+0x10)*0x248 : 0;
            submenu_or_from=compare(c,c.u32(group_or_record+0x14),-1)>0 ? c.u32(clubs)+c.u32(group_or_record+0x14)*0x248 : 0;
            const auto matches_nation=[&](unsigned club) {
                if(compare_logical(c,club)==0 || compare_logical(c,c.u32(club+0x2c))==0)return false;
                c.set(3,c.u32(club+0x2c));return compare(c,c.u32(c.r(3)+0x68),nation_filter)==0;
            };
            if(compare(c,nation_filter,-1)>0 && !matches_nation(menu_or_indices_or_to) && !matches_nation(submenu_or_from)) {
                const auto last=c.u32(f.at(0x74))-1, target=c.u32(f.at(0x70))+item*0x28;
                c.set(5,c.u32(f.at(0x70)));c.put32(f.at(0x74),last);item=item-1;
                c.set(3,target);c.set(4,c.r(5)+last*0x28);
                // Swap in only declared fields; original array padding stays put.
                for(unsigned offset=0;offset<=0x18;offset+=4)c.put32(c.r(3)+offset,c.u32(c.r(4)+offset));
                c.put8(c.r(3)+0x1c,c.u8(c.r(4)+0x1c));c.put16(c.r(3)+0x22,c.u16(c.r(4)+0x22));
                c.put16(c.r(3)+0x20,c.u16(c.r(4)+0x20));c.put8(c.r(3)+0x24,c.u8(c.r(4)+0x24));
            }
            item=item+1;
        }
        p.call(0x10768300,0x10427748,{c.u32(f.at(0x70)),c.u32(f.at(0x74)),0x28,c.toc(-0x6210)});
        p.history(0x1042775c,screens,4,c.u32(f.at(0x70)));p.history(0x10427770,screens,5,c.u32(f.at(0x74)));
    }
    if(compare(c,c.u32(f.at(0x74)))>0) {
        const auto height=p.call(0x1076d470,0x10427784,{2});
        menu_or_capacity=std::uint8_t(divide_signed<4>(535-top_or_name_colour-16,height));
        const auto count=c.u32(f.at(0x74));const auto whole=divide_signed<4>(count,menu_or_capacity);
        const auto remainder=count-whole*menu_or_capacity;
        year_or_row=std::int32_t(std::int16_t(whole+(remainder!=0)));
        c.set(7,count);c.set(8,menu_or_capacity);c.set(9,count);
        p.history(0x104277e0,screens,7,year_or_row);
        p.call(0x1074b270,0x10427804,{780,145,std::uint32_t(std::int32_t(c.i16(f.at(0x154)))),year_or_row,5,6,7,8});
        stack_args({0,2,0,missing});
        glyph_or_nation_or_grid=p.call(0x107198c0,0x10427854,{container(),110,top_or_name_colour,780,535,5,f.at(0x78),menu_or_capacity});
        year_or_row=0;item=std::uint32_t(std::int32_t(c.i16(f.at(0x154)))-1)*std::uint8_t(menu_or_capacity);
        while(compare(c,year_or_row,std::uint8_t(menu_or_capacity))<0 && compare(c,item,c.u32(f.at(0x74)))<0) {
            group_or_record=c.u32(f.at(0x70))+item*0x28;
            p.call(0x10716b00,0x10427890,{group_or_record+0x20,text,2000,255});
            cell(false,0x104278c4,0,0x30,c.u16(foreground),c.u16(c.toc(-0x7bdc)),12,1);
            menu_or_indices_or_to=compare(c,c.u32(group_or_record+0x10),-1)>0 ? c.u32(clubs)+c.u32(group_or_record+0x10)*0x248 : 0;
            submenu_or_from=compare(c,c.u32(group_or_record+0x14),-1)>0 ? c.u32(clubs)+c.u32(group_or_record+0x14)*0x248 : 0;
            if(compare_logical(c,menu_or_indices_or_to)!=0 && byte_result(c,p.call(0x1014c2e0,0x1042792c,{menu_or_indices_or_to,0,1,0,0})))top_or_name_colour=c.u16(selected);
            else if(compare_logical(c,submenu_or_from)!=0 && byte_result(c,p.call(0x1014c2e0,0x1042795c,{submenu_or_from,0,1,0,0})))top_or_name_colour=c.u16(selected);
            else top_or_name_colour=c.u16(foreground);
            p.call(0x100cb590,0x10427990,{c.u32(group_or_record+4),c.u32(group_or_record+8),c.u32(group_or_record+12),f.at(0x8c),200,1,c.u32(group_or_record)});
            p.call(0x1077b720,0x104279a0,{text,pool+0x291c,f.at(0x8c)});
            const auto clickable=p.call(0x100cbde0,0x104279c4,{c.u32(people)+c.u32(group_or_record)*0x70,c.u32(group_or_record+4),c.u32(group_or_record+8),c.u32(group_or_record+12),0});
            if(byte_result(c,clickable))cell(true,0x10427a14,1,1,top_or_name_colour,0,1,2,2007,c.u32(people)+c.u32(group_or_record)*0x70);
            else cell(false,0x10427a44,1,1,top_or_name_colour,0,1,2);
            if(compare_logical(c,submenu_or_from)!=0) {
                p.call(0x10727890,0x10427a6c,{text,pool+0x2921,std::uint32_t(std::int32_t(c.i8(submenu_or_from+0x23b))),std::uint32_t(std::int32_t(c.i8(submenu_or_from+0x23b))),submenu_or_from+0x221});
                cell(true,0x10427aa8,2,1,c.u16(selected),0,1,2,2005,submenu_or_from);
            } else {p.call(0x10727890,0x10427ab8,{text,pool+0x2931});cell(false,0x10427ae8,2,1,c.u16(selected),0,1,2);}
            if(compare_logical(c,menu_or_indices_or_to)!=0) {
                p.call(0x10727890,0x10427b10,{text,pool+0x293f,std::uint32_t(std::int32_t(c.i8(menu_or_indices_or_to+0x23b))),std::uint32_t(std::int32_t(c.i8(menu_or_indices_or_to+0x23b))),menu_or_indices_or_to+0x221});
                cell(true,0x10427b4c,3,1,c.u16(to_colour),0,1,2,2005,menu_or_indices_or_to);
            } else {p.call(0x10727890,0x10427b5c,{text,pool+0x2952});cell(false,0x10427b8c,3,1,c.u16(to_colour),0,1,2);}
            if(compare(c,c.u32(group_or_record+0x18))!=0 && compare(c,c.u8(group_or_record+0x1c),1)==0) {
                c.put32(f.at(0x6c),c.u32(group_or_record+0x18));p.call(0x10009f20,0x10427bb8,{f.at(0x6c),5});
                p.call(0x1000b410,0x10427bd0,{f.at(0x8c),200,c.r(3),c.r(4),5});
                p.call(0x10727890,0x10427be0,{text,pool+0x295b,f.at(0x8c)});
            } else if(compare(c,c.u32(group_or_record+0x18))!=0) {
                c.put32(f.at(0x68),c.u32(group_or_record+0x18));p.call(0x10009f20,0x10427c04,{f.at(0x68),5});
                p.call(0x1000b410,0x10427c1c,{text,2000,c.r(3),c.r(4),5});
            } else if(compare(c,c.u8(group_or_record+0x1c),1)==0)p.call(0x10727890,0x10427c38,{text,pool+0x2976});
            else if(compare(c,c.u8(group_or_record+0x1c),2)==0)p.call(0x10727890,0x10427c54,{text,pool+0x297b});
            else if(compare(c,c.u8(group_or_record+0x1c),4)==0)p.call(0x10727890,0x10427c70,{text,pool+0x2980});
            else p.call(0x10727890,0x10427c80,{text,pool+0x2984});
            cell(false,0x10427cb4,4,0x30,c.u16(foreground),c.u16(c.toc(-0x7bec)),12,1);
            year_or_row=year_or_row+1;item=item+1;
        }
        const auto selection=p.call(0x1073ef70,0x10427ce0,{screens,8});
        const auto has_grid=compare(c,std::int16_t(glyph_or_nation_or_grid),-1)!=0;
        c.set(5,container());
        if(has_grid)p.call(0x107145f0,0x10427d14,{c.r(5)+std::uint32_t(std::int32_t(std::int16_t(glyph_or_nation_or_grid)))*0xbf0+4,selection});
        p.history(0x10427d28,screens,9,std::uint32_t(std::int32_t(std::int16_t(glyph_or_nation_or_grid))));
    } else {
        const auto height=p.call(0x1076d470,0x10427d34,{3});
        menu_or_capacity=std::uint8_t(divide_signed<4>(535-top_or_name_colour-16,height));
        p.call(0x10727890,0x10427d58,{text,pool+0x2989,height});
        stack_args({c.u16(foreground),text,0});
        p.call(0x1074a350,0x10427d90,{110,top_or_name_colour,780,535,2,0,std::uint8_t(menu_or_capacity),3});
    }
    year_or_row=std::int8_t(p.call(0x1073ea20,0x10427d98,{screens}))==0;
    const auto next=p.call(0x1073eb00,0x10427db8,{screens,0});
    p.call(0x1074b0a0,0x10427dd0,{std::int8_t(next)==0,year_or_row});
}
}

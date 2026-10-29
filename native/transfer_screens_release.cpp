#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_release_confirmation(Cpu& c) {
    Frame f(c,0x1042ae90,0x2a0,{.first_gpr=21});
    Kept<21> contract_or_owner_or_rows_or_grid(c,c.r(21));
    Kept<22> disabled_colour(c,c.toc(-0x7bc4));
    Kept<23> foreground(c,c.toc(-0x7bc8));
    Kept<24> compensation_or_owner(c,0);
    Kept<25> pool(c,c.toc(slot::pool));
    Kept<26> clubs(c,c.toc(slot::clubs));
    Kept<27> screens(c,c.toc(slot::screens));
    Kept<28> text(c,c.toc(-0x7bb8));
    Kept<29> contracts(c,c.toc(slot::contracts));
    Kept<30> current_contract(c,c.toc(slot::current_contract));
    Kept<31> person(c,c.r(31));
    Program p{c};
    const auto stack_args=[&](std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for(auto value:values) { c.put32(f.at(offset),value); offset+=4; }
    };
    const auto owner=[&](std::uint32_t back,unsigned destination) {
        const auto record=p.call(0x10082f20,back,{c.u32(contracts),person});
        compare_logical(c,record); c.put32(current_contract,record);
        if (record) { c.set({{3,c.u32(current_contract)},{4,c.u32(clubs)}}); c.set(destination,c.r(4)+c.u32(c.r(3)+4)*0x248); }
        else c.set(destination,0);
    };
    c.set({{5,c.u16(c.r(2)-0x3790)},{6,c.u16(foreground)}});
    c.put16(f.at(0x64),c.r(6)); c.put16(f.at(0x66),c.u16(c.toc(-0x7b14)));
    c.put16(f.at(0x60),c.r(5)); c.put32(f.at(0x70),0);
    person=p.call(0x1073ef70,0x1042aef4,{screens,0});
    p.call(0x107136d0,0x1042af04,{person,0,0});
    owner(0x1042af10,3);
    p.call(0x100c7a50,0x1042af48,{c.r(3),1,f.at(0x66),f.at(0x64)});
    owner(0x1042af54,6);
    p.call(0x10744250,0x1042af90,{screens,c.u16(f.at(0x64)),c.u16(f.at(0x66)),c.r(6)+0x1ee});
    p.call(0x100caeb0,0x1042afa4,{person,f.at(0xd8),400,1});
    p.call(0x10727890,0x1042afb4,{text,pool+0x2db5,f.at(0xd8)});
    p.call(0x107442f0,0x1042afc0,{screens,text});
    p.call(0x100caeb0,0x1042afd4,{person,f.at(0xd8),400,1});
    contract_or_owner_or_rows_or_grid=p.call(0x10082f20,0x1042afe0,{c.u32(contracts),person});
    if (compare(c,contract_or_owner_or_rows_or_grid)!=0) {
        const auto encoded=(std::uint32_t(c.u8(contract_or_owner_or_rows_or_grid+0x38))<<24)&0xfe000000u;
        c.s.wr<1>(reg::xer_ca,std::int32_t(encoded)<0 && (encoded&0x03ffffffu)!=0);
        if (compare(c,std::int32_t(encoded)>>26,2)==0) {
            const auto days=p.call(0x10374d10,0x1042b008,{contract_or_owner_or_rows_or_grid});
            if (compare(c,std::int16_t(days))>0) {
                const auto product=std::int32_t(std::uint32_t(std::int16_t(days))*c.u32(contract_or_owner_or_rows_or_grid+12));
                compensation_or_owner=std::uint32_t(product/7);
                // Preserve carry from the original constant-divide lowering,
                // independently of the semantic quotient used above.
                const auto high=std::int32_t((std::int64_t(product)*std::int32_t(0x92492493u))>>32);
                const auto adjusted=std::int32_t(std::uint32_t(product)+std::uint32_t(high));
                c.s.wr<1>(reg::xer_ca,adjusted<0 && (std::uint32_t(adjusted)&3u)!=0);
                c.set(4,0x92492493);
            } else compensation_or_owner=0;
            if (byte_result(c,p.call(0x1037f110,0x1042b048,{contract_or_owner_or_rows_or_grid}))) {
                const auto value=std::int32_t(std::uint32_t(compensation_or_owner));
                compensation_or_owner=std::uint32_t(value/4);
                c.s.wr<1>(reg::xer_ca,value<0 && value>-4);
            }
        }
    }
    p.history(0x1042b06c,screens,1,compensation_or_owner);
    c.put32(f.at(0x6c),compensation_or_owner);
    p.call(0x10009f20,0x1042b07c,{f.at(0x6c),4});
    const bool positive=std::int32_t(c.r(3))>0 || (c.r(3)==0 && c.r(4)!=0);
    c.set({{3,positive ? 1u : 0u},{5,0x80000000},{6,0}});
    c.s.wr<1>(reg::xer_ca,!positive);
    if (compare(c,c.r(3))!=0) {
        c.put32(f.at(0x68),compensation_or_owner); c.put32(f.at(0x70),c.u32(f.at(0x68)));
        p.call(0x10009f20,0x1042b0b8,{f.at(0x70),6});
        p.call(0x1000b410,0x1042b0d0,{f.at(0x74),100,c.r(3),c.r(4),6});
        owner(0x1042b0dc,24); owner(0x1042b110,21); owner(0x1042b144,3);
        p.call(0x10727890,0x1042b194,{text,pool+0x2de2,
            std::uint32_t(std::int32_t(c.i8(c.r(3)+0x23b))),
            std::uint32_t(std::int32_t(c.i8(contract_or_owner_or_rows_or_grid+0x23b))),
            f.at(0xd8),compensation_or_owner+0x221,f.at(0x74)});
    } else p.call(0x10727890,0x1042b1a8,{text,pool+0x2ea6,f.at(0xd8)});
    contract_or_owner_or_rows_or_grid=std::uint32_t(std::int32_t(std::int8_t(divide_signed<4>(405,p.call(0x1076d470,0x1042b1b0,{3})))));
    owner(0x1042b1c8,24); owner(0x1042b1fc,3);
    stack_args({c.u16(foreground),text,f.at(0xd8),0x7d7,person,c.r(3)+0x221,0x7d5,compensation_or_owner,0});
    p.call(0x1074a350,0x1042b284,{110,140,780,545,2,0,std::uint32_t(std::int32_t(std::int8_t(contract_or_owner_or_rows_or_grid))),3});
    p.call(0x102a1070,0x1042b290,{4,0});
    stack_args({0,1,0,missing});
    contract_or_owner_or_rows_or_grid=p.call(0x107198c0,0x1042b2e0,
        {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,2,f.at(0x60),1});
    p.call(0x10727890,0x1042b2f4,{text,pool+0x5b2});
    stack_args({c.u16(foreground),text,1,0});
    p.call(0x107444e0,0x1042b334,{screens,contract_or_owner_or_rows_or_grid,0,0,0x30,c.u16(disabled_colour),12,3});
    p.call(0x10727890,0x1042b340,{text,pool+0x5b5});
    stack_args({c.u16(foreground),text,2,0});
    p.call(0x107444e0,0x1042b380,{screens,contract_or_owner_or_rows_or_grid,1,0,0x30,c.u16(disabled_colour),12,3});
}
}

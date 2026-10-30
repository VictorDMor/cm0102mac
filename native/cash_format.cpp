#include "cash_detail.hpp"
#include <algorithm>

namespace cm::aot::cash {
using namespace detail;
namespace {
unsigned decimal_digits(Cpu& c,const Frame& frame,std::uint64_t amount) {
    Program p{c};const auto digits=frame.at(0xa0),powers=c.r(25);
    unsigned count=0;std::uint64_t reconstructed=0;
    c.put8(c.r(15),0);c.put8(digits,0);
    keep(c,{{24,0},{31,digits},{23,0},{18,0},{19,0},{20,0}});
    while(compare(c,int(count),15)<0) {
        c.set(4,unsigned(reconstructed>>32)^0x80000000u);
        const bool remaining=std::bit_cast<std::int64_t>(reconstructed)<std::bit_cast<std::int64_t>(amount);
        c.set(3,remaining);if(compare(c,int(remaining))==0)break;
        const auto divisor=c.u64(powers+count*8+8);
        p.call(0x10773b70,0x1000b500,{unsigned(amount>>32),unsigned(amount),unsigned(divisor>>32),unsigned(divisor)});
        const auto power=c.u64(powers+count*8);
        keep(c,{{22,unsigned(power>>32)},{21,unsigned(power)}});
        c.set(5,unsigned(power>>32));c.set(6,unsigned(power));p.call(0x10773960,0x1000b518);
        for(unsigned shift=count+1;shift>0;--shift) {
            compare(c,int(shift));c.set(3,shift);c.set(5,shift-1);
            c.put8(digits+shift,c.u8(digits+shift-1));
        }
        compare(c,0);
        const auto digit=signed_byte(c.r(4));c.put8(digits,digit+'0');
        const auto high_product=unsigned((std::uint64_t(unsigned(power))*digit)>>32);
        const auto signed_high=std::int32_t(digit)<0?~0u:0u;
        const auto contribution=power*std::uint64_t(std::int64_t(std::int8_t(digit)));
        c.set(3,unsigned(power)*signed_high);
        c.set(4,high_product+unsigned(power>>32)*digit+c.r(3));c.set(5,unsigned(power)*digit);
        reconstructed+=contribution;++count;
        keep(c,{{24,count*8},{23,count*8},{18,count},{19,unsigned(reconstructed)},{20,unsigned(reconstructed>>32)}});
    }
    return count;
}
constexpr std::array<unsigned,16> grouped_back{0x1000bd10,0x1000bcf8,0x1000bcd8,0x1000bcb0,
    0x1000bc48,0x1000bbfc,0x1000bbb0,0x1000bb58,0x1000baf0,0x1000ba7c,0x1000b9fc,0x1000b96c,
    0x1000b8d0,0x1000b828,0x1000b770,0x1000b6ac};
constexpr std::array<unsigned,16> grouped_targets{0x1000bcfc,0x1000bcdc,0x1000bcb4,0x1000bc84,
    0x1000bc00,0x1000bbb4,0x1000bb5c,0x1000baf4,0x1000ba80,0x1000ba00,0x1000b970,0x1000b8d4,
    0x1000b82c,0x1000b774,0x1000b6b0,0x1000b5e0};
constexpr std::array<unsigned,16> compact_targets{0x1000c708,0x1000c6e8,0x1000c6c0,0x1000c690,
    0x1000c5f8,0x1000c548,0x1000c47c,0x1000c3e4,0x1000c334,0x1000c268,0x1000c184,0x1000c07c,
    0x1000bf4c,0x1000beb4,0x1000be04,0x1000bd38};
// Columns are zero, one and two fractional characters. The original trims
// only these last two positions; integer zeros are always retained.
constexpr std::array<std::array<unsigned,3>,12> compact_back{{
    {0x1000c68c,0x1000c66c,0x1000c634},{0x1000c5f4,0x1000c5cc,0x1000c58c},
    {0x1000c544,0x1000c514,0x1000c4cc},{0x1000c478,0x1000c458,0x1000c420},
    {0x1000c3e0,0x1000c3b8,0x1000c378},{0x1000c330,0x1000c300,0x1000c2b8},
    {0x1000c224,0x1000c204,0x1000c1cc},{0x1000c134,0x1000c10c,0x1000c0cc},
    {0x1000c020,0x1000bff0,0x1000bfa8},{0x1000bf48,0x1000bf28,0x1000bef0},
    {0x1000beb0,0x1000be88,0x1000be48},{0x1000be00,0x1000bdd0,0x1000bd88}
}};
struct Layout {
    unsigned digits,suffix,back;
    unsigned separator=0,separator_after=0;
    bool grouped=false;
};
unsigned format_offset(unsigned characters) {
    constexpr std::array<unsigned,20> offsets{0x352,0x34b,0x342,0x337,0x32a,0x31b,0x30a,0x2f7,0,
        0x2e0,0x2c7,0x2ac,0,0x28d,0x26c,0x249,0,0x222,0x1f9,0x1ce};
    if(characters>=offsets.size() || !offsets[characters])throw std::logic_error("Invalid CASH character count");
    return offsets[characters];
}
Layout layout(Cpu& c,const Frame& frame,unsigned length,bool grouped) {
    if(grouped) {
        bool separators=length>3;
        if(length==4)separators=compare(c,std::int8_t(c.u8(c.r(2)-0x3b01)))==0;
        return {length,c.r(28),length==4&&!separators?0x1000bc80:grouped_back[length],0,0,separators};
    }
    if(length<=3) {
        constexpr std::array back{0x1000c71cu,0x1000c704u,0x1000c6e4u,0x1000c6bcu};
        return {length,c.r(28),back[length]};
    }
    const auto group=(length-1)/3,leading=(length-1)%3+1;
    const std::array suffixes{c.r(28),c.r(13),c.r(27),c.r(26),c.r(14)};
    if(group==3 && compare(c,std::int8_t(c.u8(c.r(26))))==0) {
        constexpr std::array back{0x1000c264u,0x1000c180u,0x1000c078u};
        return {leading+3,c.r(27),back[leading-1],0x3aff,leading};
    }
    unsigned fraction=2;
    for(;fraction>0;--fraction) {
        const auto position=leading+fraction-1,character=signed_byte(c.u8(frame.at(0xa0)+position));
        if(position==1)c.set(8,character);
        if(position==2)c.set(9,character);
        if(position==3)c.set(10,character);
        if(compare(c,std::int32_t(character),'0')!=0)break;
    }
    return {leading+fraction,suffixes[group],compact_back[length-4][fraction],fraction?0x3b00u:0u,leading};
}
void render(Cpu& c,const Frame& frame,const Layout& layout) {
    // These tokens describe printf arguments, not guest instruction state.
    // Keep sources lazy: the original writes spilled arguments before loading
    // its first four digits, and guest aliases can observe that order.
    struct Character { bool separator;unsigned digit; };
    std::array<Character,19> characters{};unsigned count=0;
    for(unsigned n=0;n<layout.digits;++n) {
        if(n && ((layout.grouped && (layout.digits-n)%3==0) ||
                 (layout.separator && n==layout.separator_after)))characters[count++]={true,0};
        characters[count++]={false,n};
    }
    const auto read=[&](unsigned index) {
        return index==count?layout.suffix:characters[index].separator?
            signed_byte(c.u8(c.r(2)-(layout.grouped?0x3affu:layout.separator))):
            signed_byte(c.u8(frame.at(0xa0)+characters[index].digit));
    };
    // Grouping reuses its first separator register for all later separators.
    // That value is fetched before any spill which might alias the setting.
    unsigned separator=0;
    if(layout.grouped && layout.digits>=7) {
        const auto first=(layout.digits-1)%3+1;separator=read(first);c.set(6+first,separator);
    }else if(layout.separator==0x3aff) {
        separator=signed_byte(c.u8(c.r(2)-0x3aff));c.set(6+layout.separator_after,separator);
    }
    for(unsigned index=5;index<=count;++index) {
        const auto value=index<count && characters[index].separator && layout.grouped?separator:read(index);
        c.put32(frame.at(0x38+int(index-5)*4),value);
    }
    c.set(3,c.r(15));c.set(4,c.r(29)+format_offset(count));c.set(5,c.r(30));
    for(unsigned index=0;index<=count && index<5;++index) {
        const auto value=index<count && characters[index].separator && layout.grouped && layout.digits>=7?separator:read(index);
        c.set(6+index,value);
    }
    Program{c}.call(0x1077b720,layout.back);
}
}
std::uint32_t format(Cpu& c,unsigned output,unsigned capacity,std::uint64_t amount,unsigned flags) {
    Frame frame(c,0x1000b410,0x110,{.first_gpr=13});Program p{c};
    keep(c,{{15,output},{25,c.toc(-0x59fc)},{14,c.toc(-0x59f0)},
            {17,unsigned(amount>>32)},{26,c.toc(-0x59ec)},{16,unsigned(amount)},
            {27,c.toc(-0x59e8)},{13,c.toc(-0x59e4)},{28,c.toc(-0x59e0)},
            {18,c.toc(-0x7ba8)},{29,c.toc(-0x59d8)},{30,c.toc(-0x59d0)}});
    c.put8(frame.at(0x13b),flags);
    bool valid=compare(c,std::int32_t(output))!=0;
    if(valid)valid=compare(c,std::int16_t(capacity),25)>=0;
    if(!valid) {
        assertion(c,frame,0x1000b478,c.r(29),c.r(18),0x80,0x78,0x2dd);
        c.set(4,c.toc(-0x7bac));c.put8(c.r(4),0);return result(c,0);
    }
    decimal_digits(c,frame,amount);
    const bool grouped=compare(c,c.u8(frame.at(0x13b))&2)!=0;
    const auto length=p.call(0x1077dbd0,grouped?0x1000b5c4:0x1000bd1c,{frame.at(0xa0)});
    unsigned selected=0;
    if(compare_logical(c,length,15)>0)selected=0;
    else {
        const auto destination=c.u32(c.toc(grouped?-0x5a00:-0x5a04)+length*4);
        c.set(4,destination);
        const auto& targets=grouped?grouped_targets:compact_targets;
        const auto found=std::ranges::find(targets,destination);
        if(found==targets.end())throw std::runtime_error("Invalid CASH amount layout");
        selected=unsigned(found-targets.begin());
    }
    render(c,frame,layout(c,frame,selected,grouped));
    unsigned length_out=0;c.set(6,0);c.set(4,'|');
    for(;;++length_out) {
        const auto index=extended(std::int16_t(length_out));c.set(3,index);
        const auto character=c.u8(c.r(15)+index);c.set(5,character);
        if(compare(c,std::int8_t(character))==0)break;
        if(compare(c,std::int8_t(character),' ')==0)c.put8(c.r(15)+index,'|');
        c.set(6,length_out+1);
    }
    return result(c,1);
}
}

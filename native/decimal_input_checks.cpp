#include "decimal_input_checks.hpp"
#include "semantic_io_checks.hpp"
#include <cfenv>
using namespace cm::semantic_io_checks;
namespace {
void record(State& s, const std::string& digits, std::int16_t exponent, bool negative) {
    s.memory.copy_in(left,std::vector<std::uint8_t>(42,0xa5));
    s.memory.write<1>(left,negative);s.memory.write<2>(left+2,std::uint16_t(exponent));
    s.memory.write<1>(left+4,digits.size());s.memory.copy_in(left+5,{digits.begin(),digits.end()});
}
void prepare(State& s, unsigned mode) {
    s.registers.fill(0x5b);s.unique.fill(0x6d);
    for(unsigned flag=1280;flag<1309;++flag)s.wr<1>(flag,(flag+mode)%2);
    s.wr<1>(1309,0);s.wr<1>(1310,mode>>1);s.wr<1>(1311,mode&1);
    s.wr<4>(12,left);s.wr<4>(4,stack);s.wr<4>(8,toc);s.wr<4>(4128,return_sentinel);
    s.pc=0x10bf7b10;
}
void abi(const State& s, const decltype(State::registers)& before, const std::vector<std::uint8_t>& memory, std::uint64_t bits, const std::string& label) {
    auto expected=before;write_be<8>(expected.data()+16400,bits);
    require(s.registers==expected,label+" registers/FPSCR or independent result bits");
    require(s.memory.copy_out(left,128)==memory,label+" immutable record/tag and guards");
    require(s.pc==return_sentinel,label+" LR return");
}
}
unsigned decimal_input_bridge_checks(const char* program) {
    auto owner=std::make_unique<State>();auto& s=*owner;load_program(s,program);s.memory.map(fixture,0x2000);
    struct Example {const char* digits;std::int16_t exponent;std::array<std::uint64_t,4> bits;};
    constexpr std::uint64_t zero=0,inf=UINT64_C(0x7ff0000000000000),max=UINT64_C(0x7fefffffffffffff);
    const Example examples[]{
        {"0",0,{zero,zero,zero,zero}}, {"0913",123,{zero,zero,zero,zero}},
        {"15",-1,{UINT64_C(0x3ff8000000000000),UINT64_C(0x3ff8000000000000),UINT64_C(0x3ff8000000000000),UINT64_C(0x3ff8000000000000)}},
        {"208",3,{UINT64_C(0x4109640000000000),UINT64_C(0x4109640000000000),UINT64_C(0x4109640000000000),UINT64_C(0x4109640000000000)}},
        {"1",-1,{UINT64_C(0x3fb999999999999a),UINT64_C(0x3fb9999999999999),UINT64_C(0x3fb999999999999a),UINT64_C(0x3fb9999999999999)}},
        {"9007199254740993",0,{UINT64_C(0x4340000000000000),UINT64_C(0x4340000000000000),UINT64_C(0x4340000000000001),UINT64_C(0x4340000000000000)}},
        {"1",400,{inf,max,inf,max}}, {"1",-400,{zero,zero,1,zero}},
        {"I",0,{inf,inf,inf,inf}},
        {"N15",0,{UINT64_C(0x7ff802a000000000),UINT64_C(0x7ff802a000000000),UINT64_C(0x7ff802a000000000),UINT64_C(0x7ff802a000000000)}},
        {"N4021000000000000",0,{UINT64_C(0x7ff8042000000000),UINT64_C(0x7ff8042000000000),UINT64_C(0x7ff8042000000000),UINT64_C(0x7ff8042000000000)}}
    };
    std::fenv_t saved{};require(!std::fegetenv(&saved),"Save test floating environment");
    constexpr int modes[]{FE_TONEAREST,FE_TOWARDZERO,FE_UPWARD,FE_DOWNWARD};unsigned count=0;
    for(const auto& example:examples)for(unsigned mode=0;mode<4;++mode)for(bool negative:{false,true}){
        prepare(s,mode);record(s,example.digits,example.exponent,negative);
        const auto before=s.registers;const auto memory=s.memory.copy_out(left,128);
        std::fesetround(modes[(mode+1)%4]);std::feclearexcept(FE_ALL_EXCEPT);std::feraiseexcept(FE_DIVBYZERO);
        require(bridge_import(s,"CarbonLib::dec2num"),"dec2num production dispatch");
        // For negative values upward/downward exchange their magnitude rounding.
        const auto magnitude=example.bits[negative&&mode>=2?5-mode:mode];
        abi(s,before,memory,magnitude|(negative?UINT64_C(0x8000000000000000):0),"dec2num");
        require(std::fegetround()==modes[(mode+1)%4]&&std::fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO,"dec2num native environment preserved");++count;
    }
    for(const auto& tag:std::vector<std::pair<std::string,unsigned>>{{"0",0},{"1",1},{"32",32},{"255",255},{"256",255},{"999999999999999999999999",255},{"-1",0},{"-999",0},{"+17",17},{" 32",32},{"",0}}){
        prepare(s,0);s.memory.copy_in(left,std::vector<std::uint8_t>(128,0xa5));s.memory.copy_in(left,{tag.first.begin(),tag.first.end()});s.memory.write<1>(left+tag.first.size(),0);
        const auto before=s.registers;const auto memory=s.memory.copy_out(left,128);
        require(bridge_import(s,"CarbonLib::nan"),"nan production dispatch");
        abi(s,before,memory,UINT64_C(0x7ff8000000000000)|(std::uint64_t(tag.second)<<37),"PPC nan");++count;
    }
    // Each bridge has an independent control: an altered result cannot satisfy
    // the exact documented bits. Undefined record lengths and missing NULs fail.
    prepare(s,0);record(s,"15",-1,false);auto before=s.registers;auto memory=s.memory.copy_out(left,128);
    bridge_import(s,"CarbonLib::dec2num");s.wr<8>(16400,s.r<8>(16400)^1);bool rejected=false;
    try{abi(s,before,memory,UINT64_C(0x3ff8000000000000),"dec2num negative");}catch(const std::exception&){rejected=true;}require(rejected,"dec2num independent negative");
    prepare(s,0);s.memory.write<1>(left,'1');s.memory.write<1>(left+1,0);before=s.registers;memory=s.memory.copy_out(left,128);bridge_import(s,"CarbonLib::nan");s.wr<8>(16400,UINT64_C(0x7ff8000000000001));rejected=false;
    try{abi(s,before,memory,UINT64_C(0x7ff8002000000000),"nan negative");}catch(const std::exception&){rejected=true;}require(rejected,"nan independent negative");
    prepare(s,0);record(s,"15",-1,false);s.memory.write<1>(left+4,37);before=s.registers;memory=s.memory.copy_out(left,128);rejected=false;
    try{bridge_import(s,"CarbonLib::dec2num");}catch(const std::exception&){rejected=true;}require(rejected&&before==s.registers&&memory==s.memory.copy_out(left,128),"Undefined count rejected without effects");
    for(const auto& name:{"CarbonLib::dec2num","CarbonLib::nan"}){
        State missing;missing.memory.map(left,1);missing.memory.write<1>(left,'1');missing.wr<4>(12,left);missing.pc=0x12345678;before=missing.registers;rejected=false;
        try{bridge_import(missing,name);}catch(const std::exception&){rejected=true;}
        require(rejected&&missing.registers==before&&missing.pc==0x12345678,"Unmapped record/tag fault prefix");
    }
    // The real CFM glues retain descriptor/TOC dispatch and import accounting.
    for(const auto& item:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{0x10780c00,UINT64_C(0x3ff8000000000000)},{0x10780be8,UINT64_C(0x7ff8040000000000)}}){
        prepare(s,0);if(item.first==0x10780c00)record(s,"15",-1,false);else{s.memory.write<1>(left,'3');s.memory.write<1>(left+1,'2');s.memory.write<1>(left+2,0);}
        s.pc=item.first;s.budget=s.executed+1000;const auto calls=s.native_calls;run(s);
        require(s.r<8>(16400)==item.second&&s.pc==return_sentinel&&s.native_calls==calls+1,"Real CFM bridge/import transition");++count;
    }
    std::fesetenv(&saved);return count;
}

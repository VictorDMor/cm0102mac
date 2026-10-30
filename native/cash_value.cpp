#include "cash_detail.hpp"

namespace cm::aot::cash {
using namespace detail;
namespace {
std::uint64_t scaled(Cpu& c,const Frame& frame,unsigned amount,unsigned call_back,bool display) {
    Program p{c};Fpu fp(c);
    const auto numbers=c.toc(-0x59d4);
    c.set(display?5:4,numbers);c.set_f(2,c.f64(numbers));
    const auto bits=c.u32(amount)^0x80000000u;
    c.put32(frame.at(0x38),0x43300000);c.set_f(0,c.f64(c.r(2)-0x3aec));
    c.set(3,bits);c.put32(frame.at(0x3c),bits);c.set_f(1,c.f64(frame.at(0x38)));
    c.set_f(1,fp.fsub(c.f(1),c.f(2)));c.set_f(1,fp.fmul(c.f(1),c.f(0)));
    p.call(0x10773de0,call_back);return returned_pair(c);
}
void unscale(Cpu& c,const Frame& frame,unsigned output,unsigned temporary) {
    Fpu fp(c);c.set_f(0,c.f64(c.r(2)-0x3aec));c.set_f(0,fp.fdiv(c.f(1),c.f(0)));
    c.put64(frame.at(temporary),fp.fctiwz(0,c.f(0)));
    c.put32(output,c.u32(frame.at(temporary+4)));
}
std::uint64_t align(Cpu& c,std::uint64_t value,unsigned flags,unsigned quantum_back,unsigned divide_back,
                    unsigned quantum_high,unsigned quantum_low,bool display) {
    enter(c,quantum_back,{unsigned(value>>32),unsigned(value),flags});
    const auto step=quantum(c,value,flags);
    c.set(quantum_high,unsigned(step>>32));c.set(quantum_low,unsigned(step));
    // Arithmetic half with truncation toward zero, followed by the original
    // wrapped addition, signed division and low-64-bit multiplication.
    const auto half=std::uint64_t(std::bit_cast<std::int64_t>(step)/2);
    const auto dividend=value+half;
    c.set({{3,unsigned(dividend>>32)},{4,unsigned(dividend)},{5,unsigned(step>>32)},{6,unsigned(step)}});
    Program{c}.call(0x10773960,divide_back);
    const auto quotient=returned_pair(c),aligned=step*quotient;
    const auto partial=unsigned((std::uint64_t(unsigned(step))*unsigned(quotient))>>32)
                       +unsigned(step>>32)*unsigned(quotient);
    c.set(5,unsigned(step>>32)*unsigned(quotient));c.set(4,partial);
    if(display)c.set(6,unsigned(aligned));
    return aligned;
}
std::uint32_t edit(Cpu& c,unsigned amount,unsigned flags,bool upwards) {
    Frame frame(c,upwards?0x1000a060:0x1000a170,0x60,{.first_gpr=26});Program p{c};
    if(upwards)keep(c,{{30,flags},{29,amount}});else keep(c,{{31,flags},{30,amount}});
    auto displayed=scaled(c,frame,amount,upwards?0x1000a0a8:0x1000a1b8,false);
    if(upwards)keep(c,{{31,unsigned(displayed)},{26,unsigned(displayed>>32)}});
    else keep(c,{{26,unsigned(displayed)},{27,unsigned(displayed>>32)}});
    if(compare(c,flags&0x20)==0) {
        displayed=align(c,displayed,flags,upwards?0x1000a0c0:0x1000a1d0,
                         upwards?0x1000a0f8:0x1000a208,upwards?28:29,upwards?27:28,false);
        if(upwards)keep(c,{{31,unsigned(displayed)},{26,unsigned(displayed>>32)}});
        else keep(c,{{26,unsigned(displayed)},{27,unsigned(displayed>>32)}});
    }
    const auto sampled=displayed+(upwards?1:0);
    enter(c,upwards?0x1000a128:0x1000a230,{unsigned(sampled>>32),unsigned(sampled),flags});
    const auto step=quantum(c,sampled,flags),next=upwards?displayed+step:displayed-step;
    p.call(0x10773c80,upwards?0x1000a134:0x1000a23c,{unsigned(next>>32),unsigned(next)});
    unscale(c,frame,amount,0x38);return result(c,c.u32(amount));
}
}
std::uint64_t display_amount(Cpu& c,unsigned amount,unsigned flags) {
    Frame frame(c,0x10009f20,0x50,{.first_gpr=28});c.set(28,flags);
    auto value=scaled(c,frame,amount,0x10009f70,true);
    keep(c,{{30,unsigned(value)},{31,unsigned(value>>32)}});
    if(compare(c,flags&0x20)==0) {
        value=align(c,value,flags,0x10009f88,0x10009fc0,29,28,true);
        keep(c,{{30,unsigned(value)},{31,unsigned(value>>32)}});
    }
    return result_pair(c,value);
}
void store_amount(Cpu& c,unsigned amount,std::uint64_t displayed) {
    Frame frame(c,0x1000a010,0x50,{.first_gpr=31});c.set(31,amount);
    Program{c}.call(0x10773c80,0x1000a030,{unsigned(displayed>>32),unsigned(displayed)});
    unscale(c,frame,amount,0x38);
}
std::uint32_t increase(Cpu& c,unsigned amount,unsigned flags) {return edit(c,amount,flags,true);}
std::uint32_t decrease(Cpu& c,unsigned amount,unsigned flags) {return edit(c,amount,flags,false);}
std::uint32_t standard(Cpu& c,unsigned amount,unsigned index) {
    Frame frame(c,0x1000a270,0x70,{.first_gpr=29});
    keep(c,{{31,c.toc(-0x59d8)},{30,c.toc(-0x7ba8)},{29,amount}});
    const auto selected=std::int8_t(index);
    bool valid=compare(c,selected)>=0;
    if(valid)valid=compare(c,selected,7)<0;
    if(!valid) {
        assertion(c,frame,0x1000a2bc,c.r(31),c.r(30),0x40,0x38,170);
        c.set(3,c.toc(-0x7bac));c.put8(c.r(3),0);return result(c,c.u32(c.r(29)));
    }
    const auto table=c.toc(-0x59dc);enter(c,0x1000a310,{amount,index,table,7});
    return result(c,preset(c,amount,index,table,7));
}
std::uint32_t preset(Cpu& c,unsigned amount,unsigned index,unsigned table,unsigned count) {
    Frame frame(c,0x1000a330,0x80,{.first_gpr=29});Program p{c};Fpu fp(c);
    keep(c,{{31,c.toc(-0x59d8)},{30,c.toc(-0x7ba8)},{29,amount}});
    bool valid=compare_logical(c,table)!=0;
    const auto selected=std::int8_t(index);
    if(valid) {
        c.set(3,signed_byte(index));valid=compare(c,selected,std::int16_t(count))<0;
        if(valid)valid=compare(c,selected)>=0;
    }
    if(!valid) {
        assertion(c,frame,0x1000a388,c.r(31),c.r(30),0x40,0x38,192);
        c.set(3,c.toc(-0x7bac));c.put8(c.r(3),0);return result(c,c.u32(c.r(29)));
    }
    const auto raw=c.u32(table+unsigned(selected)*4)^0x80000000u;
    c.set(4,c.toc(-0x59d4));c.put32(frame.at(0x68),0x43300000);
    c.set_f(1,c.f64(c.r(4)));c.set(3,raw);c.put32(frame.at(0x6c),raw);
    c.set_f(2,c.f64(c.r(2)-0x3af4));c.set_f(0,c.f64(frame.at(0x68)));
    c.set_f(0,fp.fsub(c.f(0),c.f(1)));c.set_f(1,fp.fmul(c.f(2),c.f(0)));
    p.call(0x10773de0,0x1000a404);p.call(0x10773c80,0x1000a408);
    unscale(c,frame,c.r(29),0x60);return result(c,c.u32(c.r(29)));
}
}

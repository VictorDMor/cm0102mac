#include "floating_input_adapter.hpp"
#include <sstream>

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void set(State& s, unsigned n, std::uint32_t value) { s.wr<4>(n * 4, value); }
std::int32_t word(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
void compare(State& s, std::int64_t a, std::int64_t b = 0, bool record = false) {
    s.wr<1>(2304, (record ? s.r<1>(2304) & 0xf0 : 0) | (a < b ? 8 : a > b ? 4 : 2) | (s.r<1>(1024) & 1));
}
template<unsigned N> std::uint64_t read(State& s, std::uint32_t address, std::uint32_t pc) { s.pc = pc; return s.memory.read<N>(address); }
template<unsigned N> void write(State& s, std::uint32_t address, std::uint64_t value, std::uint32_t pc) { s.pc = pc; s.memory.write<N>(address, value); }
void call(State& s, std::uint32_t entry, std::uint32_t link) { s.wr<4>(2440, reg(s, 2)); s.wr<4>(4128, link); s.pc = entry; run_until(s, link); }
void words(State& s, std::uint32_t pc, bool restore) {
    s.wr<4>(2432, reg(s, 1) - 76);
    for (unsigned n = 13; n < 32; ++n) {
        const auto address = std::uint32_t(s.r<4>(2432));
        if (restore) set(s,n,read<4>(s,address,pc)); else write<4>(s,address,reg(s,n),pc);
        s.wr<4>(2432,address+4);
    }
}
// Decimal and hexadecimal field phases, special-word matching and the decimal
// record are separate named operations. All records and tables remain live guest
// memory, including callback aliases into the parser's frame.
class FloatingParser {
    State& s;
    std::uint32_t get(unsigned n) const { return reg(s,n); }
    void put(unsigned n,std::uint32_t v) { set(s,n,v); }
    std::uint32_t local(unsigned off) const { return get(1)+off; }
    template<unsigned N> void load(unsigned n,unsigned off,std::uint32_t pc) { put(n,read<N>(s,local(off),pc)); }
    template<unsigned N> void store(unsigned off,unsigned n,std::uint32_t pc) { write<N>(s,local(off),get(n),pc); }
    void next(std::uint32_t link,bool increment=true) {
        put(3,get(17));put(12,get(16));put(4,0);put(5,0);
        if(increment)put(27,get(27)+1);
        call(s,0x10781320,link);put(2,read<4>(s,local(20),link));put(4,get(3));
    }
    void mapped_character(unsigned destination,std::uint32_t pc) {
        compare(s,word(get(4)),-1);
        if(get(4)==UINT32_MAX)put(destination,UINT32_MAX);
        else {put(destination,get(4)&255);put(destination,read<1>(s,get(30)+get(destination),pc));}
    }
    bool digit(std::uint32_t pc,bool hex=false,unsigned index=0) {
        put(index,get(4)&255);put(0,read<1>(s,get(31)+get(index),pc));put(0,get(0)&(hex?32:16));compare(s,word(get(0)),0,true);return get(0)!=0;
    }
    void initialize() {
        words(s,0x1077de10,false);put(16,get(4));put(0,s.r<4>(4128));put(18,get(7));
        put(13,read<4>(s,get(2)-0x3b2c,0x1077de20));put(4,read<4>(s,get(2)-0x5a2c,0x1077de24));put(17,get(5));put(7,0);
        put(29,read<4>(s,get(2)-0x3b38,0x1077de30));put(30,read<4>(s,get(2)-0x62bc,0x1077de34));put(31,read<4>(s,get(2)-0x5a48,0x1077de38));put(12,get(16));
        put(28,0);put(26,0);put(25,0);put(23,0);put(22,0);put(27,1);
        write<4>(s,get(1)+8,get(0),0x1077de58);const auto previous=get(1);write<4>(s,previous-272,previous,0x1077de5c);put(1,previous-272);
        constexpr std::array<unsigned,10> registers{15,14,0,21,20,19,11,10,9,8};
        constexpr std::array<std::uint32_t,10> pcs{0x1077de64,0x1077de68,0x1077de60,0x1077de6c,0x1077de70,0x1077de74,0x1077de78,0x1077de7c,0x1077de80,0x1077de84};
        // The exponent word is read before the first two template words.
        put(0,read<4>(s,get(13)+8,pcs[2]));put(15,read<4>(s,get(13),pcs[0]));put(14,read<4>(s,get(13)+4,pcs[1]));
        for(unsigned i=3;i<registers.size();++i)put(registers[i],read<4>(s,get(13)+i*4,pcs[i]));
        put(5,read<2>(s,get(13)+40,0x1077de88));put(13,local(64));store<4>(124,0,0x1077de90);put(0,0);store<4>(308,6,0x1077de98);
        put(6,read<4>(s,get(4),0x1077de9c));put(4,0);store<4>(116,15,0x1077dea4);put(15,1);store<4>(120,14,0x1077deac);put(14,0);
        store<4>(176,0,0x1077deb4);put(0,0);store<2>(156,5,0x1077debc);put(5,0);store<4>(172,0,0x1077dec4);put(0,0);store<4>(168,0,0x1077decc);put(0,0);store<4>(164,0,0x1077ded4);put(0,0);store<4>(296,3,0x1077dedc);put(3,get(17));
        for(unsigned i=3;i<registers.size();++i)store<4>(128+(i-3)*4,registers[i],0x1077dee4+(i-3)*4);
        put(21,read<1>(s,get(6),0x1077df00));store<4>(160,0,0x1077df04);write<4>(s,get(18),get(7),0x1077df08);
        call(s,0x10781320,0x1077df10);put(2,read<4>(s,local(20),0x1077df10));put(4,get(3));
    }
    std::uint32_t phase() {
        // Preserve the original comparisons and its sole temporary r3/r0
        // construction while resolving a named phase, including invalid phases.
        compare(s,word(get(15)),0x80);if(get(15)==0x80)return 0x80;
        if(word(get(15))<0x80){
            compare(s,word(get(15)),8);if(get(15)==8)return 8;
            if(word(get(15))<8){compare(s,word(get(15)),3);if(get(15)==3)return 0;
                if(word(get(15))<3){compare(s,word(get(15)),1);return get(15)==1?1:word(get(15))>=1?2:0;}
                compare(s,word(get(15)),5);return word(get(15))<5?4:0;}
            compare(s,word(get(15)),32);if(get(15)==32)return 32;
            if(word(get(15))<32){compare(s,word(get(15)),16);return get(15)==16?16:0;}
            compare(s,word(get(15)),64);return get(15)==64?64:0;
        }
        compare(s,word(get(15)),0x2000);if(get(15)==0x2000)return 0x2000;
        if(word(get(15))<0x2000){compare(s,word(get(15)),0x200);if(get(15)==0x200)return 0x200;
            if(word(get(15))<0x200){compare(s,word(get(15)),0x100);return get(15)==0x100?0x100:0;}
            compare(s,word(get(15)),0x400);return get(15)==0x400?0x400:0;}
        put(3,0x10000);put(0,get(3)-0x8000);compare(s,word(get(15)),word(get(0)));if(get(15)==get(0))return 0x8000;
        if(word(get(15))>=word(get(0)))return 0;
        compare(s,word(get(15)),0x4000);return get(15)==0x4000?0x4000:0;
    }
    void whitespace_and_sign() {
        put(3,get(4)&255);put(0,read<1>(s,get(31)+get(3),0x1077dfd4));put(0,get(0)&6);compare(s,word(get(0)),0,true);
        if(get(0)){next(0x1077dff4,false);put(14,get(14)+1);return;}
        compare(s,word(get(4)),-1);if(get(4)==UINT32_MAX)put(0,UINT32_MAX);else put(0,read<1>(s,get(30)+get(3),0x1077e014));
        compare(s,word(get(0)),'I');
        if(get(0)=='I'){next(0x1077e0a0);put(15,0x4000);return;}
        if(word(get(0))<'I'){
            compare(s,word(get(0)),',');if(get(0)==','){put(15,2);return;}
            if(word(get(0))<','){compare(s,word(get(0)),'+');if(word(get(0))<'+'){put(15,2);return;}}
            else{compare(s,word(get(0)),'.');if(word(get(0))>='.'){put(15,2);return;}put(0,1);store<4>(176,0,0x1077e058);}
            next(0x1077e074);put(0,1);store<4>(168,0,0x1077e07c);put(4,get(3));return;
        }
        compare(s,word(get(0)),'N');if(get(0)=='N'){next(0x1077e0c8);put(15,0x2000);}else put(15,2);
    }
    void decimal_start() {
        compare(s,word(get(4)),word(get(21)));
        if(get(4)==get(21)){put(15,16);next(0x1077e370);return;}
        if(!digit(0x1077e380)){put(15,0x1000);return;}
        compare(s,word(get(4)),'0');if(get(4)!='0'){put(15,8);return;}
        next(0x1077e3b4);compare(s,word(get(3)),-1);put(4,get(3));
        if(get(3)==UINT32_MAX)put(0,UINT32_MAX);else{put(0,get(3)&255);put(0,read<1>(s,get(30)+get(0),0x1077e3d0));}
        compare(s,word(get(0)),'X');if(get(0)=='X'){put(3,0x10000);put(15,get(3)-0x8000);put(28,1);}else put(15,4);
    }
    void leading_zero(bool exponent) {
        compare(s,word(get(4)),'0');if(get(4)=='0')next(exponent?0x1077e67c:0x1077e41c);else put(15,exponent?0x400:8);
    }
    void decimal_integer() {
        if(!digit(0x1077e434)){
            compare(s,word(get(4)),word(get(21)));if(get(4)==get(21)){put(15,32);next(0x1077e464);}else put(15,64);return;
        }
        load<1>(5,120,0x1077e478);compare(s,get(5),20);
        if(get(5)<20){put(0,get(5)+1);store<1>(120,0,0x1077e488);put(3,local(116));put(0,get(5)+5);write<1>(s,get(3)+get(0),get(4),0x1077e494);}else put(25,get(25)+1);
        next(0x1077e4b8);
    }
    void decimal_fraction(bool require_digit) {
        if(!digit(require_digit?0x1077e4c8:0x1077e4e8)){put(15,require_digit?0x1000:64);return;}
        if(require_digit){put(15,32);return;}
        load<1>(0,120,0x1077e4fc);compare(s,get(0),20);
        if(get(0)<20){compare(s,word(get(4)),'0');bool keep=get(4)!='0';if(!keep){compare(s,get(0));keep=get(0)!=0;}
            if(keep){load<1>(5,120,0x1077e51c);put(3,local(116));put(0,get(5)+1);store<1>(120,0,0x1077e524);put(0,get(5)+5);write<1>(s,get(3)+get(0),get(4),0x1077e52c);}
            put(25,get(25)-1);
        }next(0x1077e54c);
    }
    void decimal_exponent() {
        mapped_character(0,0x1077e56c);compare(s,word(get(0)),'E');if(get(0)=='E'){put(15,0x80);next(0x1077e594);}else put(15,0x800);
    }
    void exponent_sign() {
        compare(s,word(get(4)),'+');if(get(4)=='+')next(0x1077e5c8);
        else{compare(s,word(get(4)),'-');if(get(4)=='-'){next(0x1077e5f4);put(0,1);store<4>(172,0,0x1077e5fc);put(4,get(3));}}
        put(15,0x100);
    }
    void exponent_start() {
        if(!digit(0x1077e610)){put(15,0x1000);return;}
        compare(s,word(get(4)),'0');if(get(4)=='0'){put(15,0x200);next(0x1077e648);}else put(15,0x400);
    }
    void exponent_digits() {
        if(!digit(0x1077e694)){put(15,0x800);return;}
        put(0,get(26)*10);put(26,get(4)+get(0)-'0');compare(s,word(get(26)),0x7fff);
        if(word(get(26))>0x7fff){put(0,1);write<4>(s,get(18),get(0),0x1077e6c0);}next(0x1077e6dc);
    }
    bool special_word(bool nan);
    void hex_phase();
    void finish_decimal();
    void finish_hex();
    void floating_compare(unsigned left,unsigned right) {
        const auto a=float_value<8>(s.r<8>(16384+left*16)),b=float_value<8>(s.r<8>(16384+right*16));
        s.wr<1>(1296,a<b);s.wr<1>(1297,b<a);s.wr<1>(1298,a==b);
        s.wr<1>(2304,(a<b?8:b<a?4:a==b?2:0)|(std::isnan(a)||std::isnan(b)));
    }
    void load_float(unsigned target,std::uint32_t address,std::uint32_t pc,bool single=false) {
        const auto bits=single?float_bits<8>(float_value<4>(read<4>(s,address,pc))):read<8>(s,address,pc);s.wr<8>(16384+target*16,bits);
    }
    void negate(unsigned target,unsigned source) { s.wr<8>(16384+target*16,float_bits<8>(-float_value<8>(s.r<8>(16384+source*16)))); }
    void restore() {
        load<4>(0,280,0x1077ee00);put(1,get(1)+272);words(s,0x1077ee08,true);s.wr<4>(4128,get(0));s.pc=std::uint32_t(s.r<4>(4128));
    }
public:
    explicit FloatingParser(State& state):s(state){}
    void parse() {
        initialize();
        for(;;){
            s.step(0x1077eb2c);load<4>(0,296,0x1077eb2c);compare(s,word(get(27)),word(get(0)));bool end=word(get(27))>word(get(0));
            if(!end){compare(s,word(get(4)),-1);end=get(4)==UINT32_MAX;}
            if(!end){put(0,get(15)&0x1800);compare(s,word(get(0)),0,true);end=get(0)!=0;}
            if(end)break;
            switch(phase()){
            case 1:whitespace_and_sign();break;case 2:decimal_start();break;case 4:leading_zero(false);break;case 8:decimal_integer();break;
            case 16:decimal_fraction(true);break;case 32:decimal_fraction(false);break;case 64:decimal_exponent();break;case 0x80:exponent_sign();break;
            case 0x100:exponent_start();break;case 0x200:leading_zero(true);break;case 0x400:exponent_digits();break;
            case 0x4000:if(special_word(false)){restore();return;}break;case 0x2000:if(special_word(true)){restore();return;}break;
            case 0x8000:hex_phase();break;default:break;
            }
        }
        put(13,get(15)&0xe2c);compare(s,word(get(13)),0,true);
        if(!get(13)){load<4>(3,308,0x1077eb50);put(0,0);write<4>(s,get(3),get(0),0x1077eb58);}
        else{put(3,get(27)+get(14));put(0,get(3)-1);load<4>(3,308,0x1077eb68);write<4>(s,get(3),get(0),0x1077eb6c);}
        put(3,get(17));put(12,get(16));put(5,1);call(s,0x10781320,0x1077eb80);put(2,read<4>(s,local(20),0x1077eb80));compare(s,word(get(28)));
        if(get(28))finish_hex();else finish_decimal();restore();
    }
};
bool FloatingParser::special_word(bool nan) {
    put(3,read<4>(s,get(2)-(nan?0x3b34:0x3b30),nan?0x1077e1b8:0x1077e0e0));put(15,local(nan?57:73));
    if(nan){
        put(9,read<4>(s,get(29),0x1077e1c0));put(20,1);put(11,read<4>(s,get(3),0x1077e1c8));put(10,read<1>(s,get(3)+4,0x1077e1cc));put(19,0);
        constexpr std::array<unsigned,7> slots{8,7,6,5,3,0,12};
        for(unsigned i=0;i<slots.size();++i)put(slots[i],read<4>(s,get(29)+4+i*4,0x1077e1d4+i*4));
        store<4>(56,11,0x1077e1f0);store<1>(60,10,0x1077e1f4);store<4>(84,9,0x1077e1f8);
        for(unsigned i=0;i<slots.size();++i)store<4>(88+i*4,slots[i],0x1077e1fc+i*4);
    }else{put(19,1);put(6,read<4>(s,get(3),0x1077e0ec));put(5,read<4>(s,get(3)+4,0x1077e0f0));put(0,read<1>(s,get(3)+8,0x1077e0f4));store<4>(72,6,0x1077e0f8);store<4>(76,5,0x1077e0fc);store<1>(80,0,0x1077e100);}
    const unsigned counter=nan?20:19;
    for(;;){
        s.step(nan?0x1077e244:0x1077e130);compare(s,word(get(counter)),nan?4:8);if(word(get(counter))>=(nan?4:8))break;
        put(0,read<1>(s,get(15),nan?0x1077e24c:0x1077e138));compare(s,word(get(4)),-1);put(3,std::uint32_t(signed_value<1>(get(0))));
        if(get(4)==UINT32_MAX)put(0,UINT32_MAX);else{put(0,get(4)&255);put(0,read<1>(s,get(30)+get(0),nan?0x1077e268:0x1077e154));}
        compare(s,word(get(3)),word(get(0)));if(get(3)!=get(0))break;
        put(3,get(17));put(12,get(16));put(4,0);put(5,0);put(counter,get(counter)+1);put(15,get(15)+1);put(27,get(27)+1);
        const auto link=nan?0x1077e23cu:0x1077e128u;call(s,0x10781320,link);put(2,read<4>(s,local(20),link));put(4,get(3));
    }
    compare(s,word(get(counter)),3);bool matched=get(counter)==3;
    if(!matched){compare(s,word(get(counter)),nan?4:8);matched=get(counter)==(nan?4u:8u);}
    if(!matched){put(15,0x1000);return false;}
    if(nan){
        compare(s,word(get(20)),4);
        if(get(20)==4){
            put(15,local(84));for(;;){
                s.step(0x1077e2c0);compare(s,word(get(19)),32);bool accept=word(get(19))<32;
                if(accept){put(0,get(4)&255);put(3,read<1>(s,get(31)+get(0),0x1077e2cc));put(0,get(3)&16);compare(s,word(get(0)),0,true);accept=get(0)!=0;
                    if(!accept){put(0,get(3)&0xc0);compare(s,word(get(0)),0,true);accept=get(0)!=0;}}
                if(!accept)break;
                write<1>(s,get(15),get(4),0x1077e294);put(3,get(17));put(12,get(16));put(4,0);put(5,0);put(19,get(19)+1);put(15,get(15)+1);put(27,get(27)+1);
                call(s,0x10781320,0x1077e2b8);put(2,read<4>(s,local(20),0x1077e2b8));put(4,get(3));
            }
            compare(s,word(get(4)),')');if(get(4)!=')'){put(15,0x1000);return false;}put(19,get(19)+1);
        }
        load<4>(0,176,0x1077e2f4);put(3,local(84));compare(s,word(get(0)));put(0,0);write<1>(s,get(3)+get(19),get(0),0x1077e304);
        const bool negative=(s.r<1>(2304)&2)==0;const auto link=negative?0x1077e310u:0x1077e320u;call(s,0x10780be8,link);put(2,read<4>(s,local(20),link));if(negative)negate(1,1);
        load<4>(0,168,0x1077e324);put(3,get(20)+get(19));write<8>(s,local(64),s.r<8>(16400),0x1077e32c);put(0,get(3)+get(0));load<4>(3,308,0x1077e334);put(0,get(14)+get(0));write<4>(s,get(3),get(0),0x1077e33c);
    }else{
        load<4>(0,176,0x1077e170);compare(s,word(get(0)));const bool negative=get(0)!=0;
        put(3,read<4>(s,get(2)-0x5a30,negative?0x1077e17c:0x1077e18c));load_float(negative?0:1,get(3),negative?0x1077e180:0x1077e190,true);if(negative)negate(1,0);
        load<4>(0,168,0x1077e194);load<4>(3,308,0x1077e198);put(0,get(19)+get(0));write<8>(s,local(64),s.r<8>(16400),0x1077e1a0);put(0,get(14)+get(0));write<4>(s,get(3),get(0),0x1077e1a8);
    }return true;
}
void FloatingParser::hex_phase() {
    compare(s,word(get(28)),16);unsigned phase=0;
    if(get(28)==16)phase=16;
    else if(word(get(28))<16){compare(s,word(get(28)),4);if(get(28)==4)phase=4;
        else if(word(get(28))<4){compare(s,word(get(28)),2);if(get(28)==2)phase=2;else if(word(get(28))<2){compare(s,word(get(28)),1);if(word(get(28))>=1)phase=1;}}
        else{compare(s,word(get(28)),8);if(get(28)==8)phase=8;}}
    else{compare(s,word(get(28)),64);if(get(28)==64)phase=64;else if(word(get(28))<64){compare(s,word(get(28)),32);if(get(28)==32)phase=32;}
        else{compare(s,word(get(28)),0x100);if(get(28)==0x100)phase=0x100;}}
    if(phase==1){
        put(0,0);store<1>(64,0,0x1077e74c);put(3,get(17));put(12,get(16));store<1>(65,0,0x1077e758);put(24,2);put(28,2);store<1>(66,0,0x1077e764);put(4,0);put(5,0);store<1>(67,0,0x1077e770);put(27,get(27)+1);
        for(unsigned i=0;i<4;++i)store<1>(68+i,0,0x1077e778+i*4);call(s,0x10781320,0x1077e78c);put(2,read<4>(s,local(20),0x1077e78c));put(4,get(3));return;
    }
    if(phase==2){compare(s,word(get(4)),'0');if(get(4)=='0')next(0x1077e7b8);else put(28,4);return;}
    if(phase==4||phase==8){
        const bool fraction=phase==8;
        if(!digit(fraction?0x1077e8ec:0x1077e7d0,true,fraction?3:5)){
            if(!fraction){compare(s,word(get(4)),word(get(21)));if(get(4)==get(21)){put(28,8);next(0x1077e800);return;}}
            put(28,16);return;
        }
        compare(s,word(get(24)),17);
        if(word(get(24))<17){
            put(0,std::rotl(get(24),1)&1);
            if(!fraction)load<4>(3,160,0x1077e820);
            put(0,get(0)+get(24));s.wr<1>(1027,word(get(0))<0&&(get(0)&1));put(0,shift_signed<4>(get(0),1));
            if(!fraction){put(3,get(3)+1);put(6,read<1>(s,get(13)+get(0),0x1077e830));compare(s,word(get(4)),-1);store<4>(160,3,0x1077e838);
                if(get(4)==UINT32_MAX)put(3,UINT32_MAX);else put(3,read<1>(s,get(30)+get(5),0x1077e848));}
            else{compare(s,word(get(4)),-1);put(5,read<1>(s,get(13)+get(0),0x1077e918));if(get(4)==UINT32_MAX)put(3,UINT32_MAX);else put(3,read<1>(s,get(30)+get(3),0x1077e928));}
            compare(s,word(get(3)),'A');const unsigned value=fraction?6:5;put(value,get(3)-(word(get(3))>='A'?55:48));
            put(4,std::rotl(get(24),1)&1);put(0,get(24)&1);put(3,get(0)^get(4));put(0,(get(value)<<4)&0xff0);put(0,get(fraction?5:6)|get(0));put(3,get(3)-get(4));compare(s,word(get(3)),0,true);
            put(4,get(value)&255);const bool low=get(3)!=0;put(3,get(0)&255);
            if(low){put(0,get(fraction?5:6)|get(4));put(3,get(0)&255);}
            put(0,std::rotl(get(24),1)&1);put(0,get(0)+get(24));s.wr<1>(1027,word(get(0))<0&&(get(0)&1));put(0,shift_signed<4>(get(0),1));write<1>(s,get(13)+get(0),get(3),fraction?0x1077e978:0x1077e898);
            put(24,get(24)+1);next(fraction?0x1077e998:0x1077e8b8);
        }else next(fraction?0x1077e9bc:0x1077e8dc);
        return;
    }
    if(phase==16){mapped_character(0,0x1077e9dc);compare(s,word(get(0)),'P');if(get(0)=='P'){put(28,32);put(23,get(23)+1);next(0x1077ea08);}else put(15,0x800);return;}
    if(phase==32){
        compare(s,word(get(4)),'-');put(23,get(23)+1);
        if(get(4)=='-'){put(0,1);store<4>(164,0,0x1077ea2c);}
        else{compare(s,word(get(4)),'+');if(get(4)!='+'){put(3,get(17));put(12,get(16));put(5,1);call(s,0x10781320,0x1077ea4c);put(2,read<4>(s,local(20),0x1077ea4c));put(23,get(23)-1);}}
        put(28,64);next(0x1077ea70);return;
    }
    if(phase==64){if(!digit(0x1077ea80)){put(15,0x1000);return;}compare(s,word(get(4)),'0');if(get(4)=='0'){put(28,0x80);put(23,get(23)+1);next(0x1077eabc);}else put(28,0x100);return;}
    if(phase==0x100){
        if(!digit(0x1077ead4)){put(15,0x800);return;}
        put(0,get(22)*10);put(3,get(4)+get(0));put(0,get(3)-'0');compare(s,word(get(26)),0x7fff);put(22,std::uint32_t(signed_value<2>(get(0))));
        if(word(get(26))>0x7fff){put(0,1);write<4>(s,get(18),get(0),0x1077eb04);}put(23,get(23)+1);next(0x1077eb24);
    }
    // The legacy dispatch has no 0x80 hexadecimal exponent phase. It retains
    // the lookahead and repeats the outer loop; the diagnostic budget bounds it.
}
void FloatingParser::finish_decimal() {
    load<4>(0,172,0x1077eb8c);compare(s,word(get(0)));if(get(0))put(26,0-get(26));
    load<1>(3,120,0x1077eb9c);put(0,local(116));put(4,get(3)+5);put(4,get(0)+get(4));
    for(;;){s.step(0x1077ebb4);compare(s,word(get(3)));put(3,get(3)-1);if((s.r<1>(2304)&2)!=0)break;
        put(0,read<1>(s,get(4)-1,0x1077ebc0));put(4,get(4)-1);compare(s,get(0),'0');if(get(0)!='0')break;put(25,get(25)+1);}
    put(0,get(3)+1);store<1>(120,0,0x1077ebd0);load<1>(4,120,0x1077ebd4);compare(s,get(4));
    if(!get(4)){put(0,get(4)+1);store<1>(120,0,0x1077ebe4);put(3,local(121));put(0,'0');write<1>(s,get(3)+get(4),get(0),0x1077ebf0);}
    put(26,get(26)+get(25));compare(s,word(get(26)),-0x8000);bool range=word(get(26))>=-0x8000;
    if(range){compare(s,word(get(26)),0x7fff);range=word(get(26))<=0x7fff;}
    if(!range){put(0,1);write<4>(s,get(18),get(0),0x1077ec0c);}
    put(0,read<4>(s,get(18),0x1077ec10));compare(s,word(get(0)));
    if(get(0)){
        load<4>(0,172,0x1077ec1c);compare(s,word(get(0)));if(get(0)){load_float(1,get(2)-0x34b4,0x1077ec28);return;}
        load<4>(0,176,0x1077ec30);compare(s,word(get(0)));const bool negative=get(0)!=0;
        put(3,read<4>(s,get(2)-0x5a34,negative?0x1077ec3c:0x1077ec4c));load_float(negative?0:1,get(3),negative?0x1077ec40:0x1077ec50);if(negative)negate(1,0);return;
    }
    store<2>(118,26,0x1077ec58);put(3,local(116));call(s,0x10780c00,0x1077ec64);put(2,read<4>(s,local(20),0x1077ec64));load_float(0,get(2)-0x34b4,0x1077ec68);write<8>(s,local(64),s.r<8>(16400),0x1077ec6c);floating_compare(0,1);
    bool underflow=false;
    if((s.r<1>(2304)&2)==0){put(3,read<4>(s,get(2)-0x5a38,0x1077ec78));load_float(0,get(3),0x1077ec7c);floating_compare(1,0);underflow=(s.r<1>(2304)&8)!=0;}
    if(underflow){put(0,1);write<4>(s,get(18),get(0),0x1077ec8c);}
    else{put(3,read<4>(s,get(2)-0x5a3c,0x1077ec94));load_float(0,get(3),0x1077ec98);floating_compare(1,0);
        if((s.r<1>(2304)&4)!=0){put(0,1);put(3,read<4>(s,get(2)-0x5a34,0x1077eca8));write<4>(s,get(18),get(0),0x1077ecac);load_float(0,get(3),0x1077ecb0);write<8>(s,local(64),s.r<8>(16384),0x1077ecb4);}}
    load<4>(0,176,0x1077ecb8);compare(s,word(get(0)));bool negative=get(0)!=0;
    if(negative){compare(s,word(get(13)));negative=get(13)!=0;}
    if(negative){load_float(0,local(64),0x1077eccc);negate(0,0);write<8>(s,local(64),s.r<8>(16384),0x1077ecd4);}load_float(1,local(64),0x1077ecd8);
}
void FloatingParser::finish_hex() {
    load_float(1,local(64),0x1077ece0);load_float(0,get(2)-0x34bc,0x1077ece4);floating_compare(1,0);
    if((s.r<1>(2304)&2)!=0){load_float(0,get(2)-0x34b4,0x1077edf4);write<8>(s,local(64),s.r<8>(16384),0x1077edf8);load_float(1,local(64),0x1077edfc);return;}
    load<4>(0,164,0x1077ecf0);compare(s,word(get(0)));if(get(0)){put(0,0-get(22));put(22,std::uint32_t(signed_value<2>(get(0))));}
    for(;;){
        s.step(0x1077ed28);put(4,std::uint32_t(signed_value<2>(read<2>(s,local(64),0x1077ed28))));put(0,get(4)&0xf0);compare(s,word(get(0)),16);if(get(0)==16)break;
        load<4>(4,64,0x1077ed08);put(22,get(22)+1);load<4>(3,68,0x1077ed10);put(0,get(4)>>1);put(3,std::rotr(get(3),1));store<4>(64,0,0x1077ed1c);put(3,(get(3)&0x7fffffff)|((get(4)&1)<<31));store<4>(68,3,0x1077ed24);
    }
    load<4>(3,160,0x1077ed38);load_float(1,get(2)-0x34b4,0x1077ed3c);put(0,get(3)-1);put(3,get(0)*4);put(0,get(4)&15);store<2>(64,0,0x1077ed4c);put(0,get(22)+get(3));put(22,std::uint32_t(signed_value<2>(get(0))));put(0,get(22)+0x3ff);
    put(3,std::uint32_t(signed_value<2>(read<2>(s,local(64),0x1077ed5c))));put(0,(get(0)<<4)&0xfffffff0);put(0,get(3)|get(0));store<2>(64,0,0x1077ed68);load<4>(0,168,0x1077ed6c);load_float(2,local(64),0x1077ed70);
    put(3,get(0)+get(24));put(0,get(3)+1);floating_compare(1,2);put(0,get(0)+get(23));load<4>(3,308,0x1077ed84);put(0,get(14)+get(0));write<4>(s,get(3),get(0),0x1077ed8c);
    bool underflow=false;if((s.r<1>(2304)&2)==0){put(3,read<4>(s,get(2)-0x5a38,0x1077ed94));load_float(0,get(3),0x1077ed98);floating_compare(2,0);underflow=(s.r<1>(2304)&8)!=0;}
    if(underflow){put(0,1);write<8>(s,local(64),s.r<8>(16400),0x1077eda8);write<4>(s,get(18),get(0),0x1077edac);}
    else{put(3,read<4>(s,get(2)-0x5a3c,0x1077edb4));load_float(0,get(3),0x1077edb8);floating_compare(2,0);if((s.r<1>(2304)&4)!=0){put(0,1);put(3,read<4>(s,get(2)-0x5a34,0x1077edc8));write<4>(s,get(18),get(0),0x1077edcc);load_float(0,get(3),0x1077edd0);write<8>(s,local(64),s.r<8>(16384),0x1077edd4);}}
    load<4>(0,176,0x1077edd8);compare(s,word(get(0)));if(get(0)){put(0,std::uint32_t(signed_value<2>(read<2>(s,local(64),0x1077ede4))));put(0,get(0)|0x8000);store<2>(64,0,0x1077edec);}load_float(1,local(64),0x1077edfc);
}
}
bool try_modern_floating_input(State& s,std::uint32_t entry) {
    if(s.floating_input_original_resume==entry){s.floating_input_original_resume=0;return false;}
    if(entry!=floating_input_entries[0])throw std::runtime_error("Unknown floating input entry");
    const bool original=s.floating_input_original;++(original?s.floating_input_original_calls[0]:s.floating_input_calls[0]);
    if(original&&!s.floating_input_observer)return false;
    const auto caller=std::uint32_t(s.r<4>(4128));if(s.floating_input_observer)s.floating_input_observer->enter(s,entry,original);
    if(original){s.floating_input_original_resume=entry;try{run_until(s,caller);}catch(...){s.floating_input_original_resume=0;throw;}}
    else{s.step(entry);FloatingParser(s).parse();}
    if(s.floating_input_observer)s.floating_input_observer->leave(s,entry,original);return true;
}
void reset_floating_input_diagnostics(State& s) {
    s.floating_input_original=false;s.floating_input_calls.fill(0);s.floating_input_original_calls.fill(0);s.floating_input_dispatch_escapes=0;s.floating_input_original_resume=0;s.floating_input_observer=nullptr;
}
std::string floating_input_call_summary(const State& s) {
    std::ostringstream out;out<<"floating.entry=0x1077de10 mode="<<(s.floating_input_original?"original":"modern")<<" modern="<<s.floating_input_calls[0]<<" original="<<s.floating_input_original_calls[0]<<" escapes="<<s.floating_input_dispatch_escapes<<'\n';return out.str();
}
}

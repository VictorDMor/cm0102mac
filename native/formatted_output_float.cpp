#include "formatted_output_internal.hpp"
namespace cm::aot::formatted_output {
namespace {
void backfill(State& s,unsigned cursor,unsigned value,std::uint32_t pc,unsigned distance=1){write<1>(s,reg(s,cursor)-distance,reg(s,value),pc);set(s,cursor,reg(s,cursor)-distance);}
void shift_half(State& s,unsigned destination,unsigned source){const auto value=reg(s,source);s.wr<1>(1027,word(value)<0&&(value&1));set(s,destination,shift_signed<4>(value,1));}
// The finite hexadecimal path projects a signed decimal exponent record into
// the word32 formatter. Padding in that Mac record is deliberately retained.
void hex_exponent_record(State& s){
    const auto local=reg(s,1);set(s,25,0);set(s,0,std::uint32_t(signed_value<2>(read<2>(s,local+100,0x10779ccc))));set(s,24,1);write<1>(s,local+154,reg(s,25),0x10779cd4);set(s,7,'d');set(s,5,read<4>(s,local+168,0x10779cdc));write<1>(s,local+157,reg(s,7),0x10779ce0);set(s,0,(std::rotl(reg(s,0),28)&0x7ff));set(s,3,std::uint32_t(signed_value<2>(reg(s,0))));set(s,0,read<4>(s,local+172,0x10779cec));write<1>(s,local+152,reg(s,24),0x10779cf0);set(s,26,read<4>(s,local+176,0x10779cf4));set(s,4,reg(s,27));write<1>(s,local+153,reg(s,24),0x10779cfc);set(s,3,reg(s,3)-1023);set(s,12,read<4>(s,local+180,0x10779d04));write<1>(s,local+155,reg(s,25),0x10779d08);set(s,11,read<4>(s,local+184,0x10779d0c));set(s,6,read<4>(s,local+152,0x10779d10));write<1>(s,local+156,reg(s,25),0x10779d14);set(s,10,read<4>(s,local+188,0x10779d18));write<4>(s,local+32,reg(s,6),0x10779d1c);set(s,6,read<4>(s,local+156,0x10779d20));set(s,9,read<4>(s,local+192,0x10779d24));write<4>(s,local+36,reg(s,6),0x10779d28);set(s,8,read<4>(s,local+196,0x10779d2c));write<4>(s,local+40,reg(s,25),0x10779d30);set(s,7,read<4>(s,local+200,0x10779d34));write<4>(s,local+44,reg(s,24),0x10779d38);set(s,6,read<4>(s,local+204,0x10779d3c));write<4>(s,local+48,reg(s,5),0x10779d40);set(s,5,read<2>(s,local+208,0x10779d44));write<4>(s,local+52,reg(s,0),0x10779d48);set(s,0,read<2>(s,local+210,0x10779d4c));
    constexpr std::array<unsigned,8> values{26,12,11,10,9,8,7,6};for(unsigned i=0;i<8;++i)write<4>(s,local+56+i*4,reg(s,values[i]),0x10779d50+i*4);
    write<2>(s,local+88,reg(s,5),0x10779d70);write<2>(s,local+90,reg(s,0),0x10779d74);write<4>(s,local+160,reg(s,25),0x10779d78);set(s,5,read<4>(s,local+32,0x10779d7c));write<4>(s,local+164,reg(s,24),0x10779d80);for(unsigned n=6;n<=10;++n)set(s,n,read<4>(s,local+36+(n-6)*4,0x10779d84+(n-6)*4));call(s,0x10779620,0x10779d9c);
}
void hex_special(State& s,bool negative,bool nan){
    compare(s,reg(s,29),'A');set(s,24,reg(s,27)-(negative?5:4));const bool upper=reg(s,29)=='A';const auto root=nan?0x10779c6c:0x10779bfc;
    if(negative){set(s,4,read<4>(s,reg(s,2)-(nan?(upper?0x3b60:0x3b64):(upper?0x3b58:0x3b5c)),root+(upper?12:28)));set(s,3,reg(s,24));call(s,0x1077dbf0,root+(upper?24:40));}
    else{set(s,3,reg(s,24));set(s,4,reg(s,2)-(nan?(upper?0x34e0:0x34dc):(upper?0x34e8:0x34e4)));call(s,0x1077dbf0,(nan?0x10779ca4:0x10779c34)+(upper?12:28));}set(s,3,reg(s,24));
}
}
void format_hex_float(State& s){
    words(s,24,0x10779b70,false);set(s,0,s.r<4>(4128));set(s,27,reg(s,5));write<4>(s,reg(s,1)+8,reg(s,0),0x10779b7c);frame(s,256,0x10779b80);write<8>(s,reg(s,1)+280,s.r<8>(16400),0x10779b84);
    for(unsigned n=6;n<=10;++n)write<4>(s,reg(s,1)+292+(n-6)*4,reg(s,n),0x10779b88+(n-6)*4);
    set(s,28,read<4>(s,reg(s,1)+304,0x10779b9c));s.wr<8>(16400,read<8>(s,reg(s,1)+280,0x10779ba0));compare(s,word(reg(s,28)),509);set(s,29,read<1>(s,reg(s,1)+297,0x10779ba8));write<8>(s,reg(s,1)+100,s.r<8>(16400),0x10779bac);set(s,30,read<1>(s,reg(s,1)+295,0x10779bb0));set(s,31,read<1>(s,reg(s,1)+293,0x10779bb4));
    if(word(reg(s,28))>509)set(s,3,0);else{
        set(s,3,0);set(s,0,32);write<1>(s,reg(s,1)+96,reg(s,3),0x10779bcc);set(s,3,reg(s,1)+96);write<2>(s,reg(s,1)+98,reg(s,0),0x10779bd4);set(s,6,reg(s,1)+108);call(s,0x107800d8,0x10779be0);set(s,2,read<4>(s,reg(s,1)+20,0x10779be0));set(s,0,read<1>(s,reg(s,1)+113,0x10779be4));compare(s,reg(s,0),'I');
        if(reg(s,0)=='I'){set(s,0,std::uint32_t(signed_value<2>(read<2>(s,reg(s,1)+100,0x10779bf0))));set(s,0,reg(s,0)&0x8000);compare(s,word(reg(s,0)),0,true);hex_special(s,reg(s,0)!=0,false);}
        else{compare(s,reg(s,0),'N');if(reg(s,0)=='N'){set(s,0,read<1>(s,reg(s,1)+280,0x10779c60));set(s,0,reg(s,0)&0x80);compare(s,word(reg(s,0)),0,true);hex_special(s,reg(s,0)!=0,true);}
            else{
                hex_exponent_record(s);compare(s,reg(s,29),'a');set(s,0,reg(s,29)=='a'?'p':'P');backfill(s,3,0,reg(s,29)=='a'?0x10779da8:0x10779db4);
                compare(s,word(reg(s,28)),1);s.wr<4>(4132,reg(s,28));set(s,8,reg(s,28));set(s,6,reg(s,1)+280);
                if(word(reg(s,28))>=1){do{
                    s.step(0x10779dcc);set(s,5,std::rotl(reg(s,8),1)&1);set(s,0,reg(s,5)+reg(s,8));shift_half(s,4,0);set(s,4,reg(s,4)+1);set(s,0,reg(s,8)&1);set(s,7,read<1>(s,reg(s,6)+reg(s,4),0x10779de0));set(s,4,reg(s,0)^reg(s,5));set(s,4,reg(s,4)-reg(s,5));compare(s,word(reg(s,4)),0,true);const bool low=reg(s,4)!=0;set(s,0,std::rotl(reg(s,7),28)&15);set(s,4,byte(reg(s,0)));if(low)set(s,4,reg(s,7)&15);set(s,0,byte(reg(s,4)));compare(s,word(reg(s,0)),10);
                    if(word(reg(s,0))<10)set(s,0,reg(s,4)+'0');else{compare(s,reg(s,29),'a');set(s,0,reg(s,4)+(reg(s,29)=='a'?87:55));}backfill(s,3,0,0x10779e24);set(s,8,reg(s,8)-1);s.wr<4>(4132,s.r<4>(4132)-1);
                }while(s.r<4>(4132));}
                compare(s,word(reg(s,28)));bool point=reg(s,28)!=0;if(!point){compare(s,reg(s,30));point=reg(s,30)!=0;}if(point){set(s,0,'.');backfill(s,3,0,0x10779e44);}
                set(s,0,'1');compare(s,reg(s,29),'a');write<1>(s,reg(s,3)-1,reg(s,0),0x10779e50);set(s,0,reg(s,29)=='a'?'x':'X');backfill(s,3,0,reg(s,29)=='a'?0x10779e5c:0x10779e68,2);set(s,0,std::uint32_t(signed_value<2>(read<2>(s,reg(s,1)+100,0x10779e6c))));set(s,4,'0');backfill(s,3,4,0x10779e74);set(s,0,reg(s,0)&0x8000);compare(s,word(reg(s,0)),0,true);
                if(reg(s,0)){set(s,0,'-');backfill(s,3,0,0x10779e84);}else{compare(s,reg(s,31),1);if(reg(s,31)==1){set(s,0,'+');backfill(s,3,0,0x10779e98);}else{compare(s,reg(s,31),2);if(reg(s,31)==2){set(s,0,' ');backfill(s,3,0,0x10779eac);}}}
            }
        }
    }
    set(s,0,read<4>(s,reg(s,1)+264,0x10779eb0));set(s,1,reg(s,1)+256);words(s,24,0x10779eb8,true);s.wr<4>(4128,reg(s,0));finish(s);
}
namespace {
class DecimalOutput {
    State& s;
    std::uint32_t get(unsigned n)const{return reg(s,n);}
    void put(unsigned n,std::uint32_t v){set(s,n,v);}
    std::uint32_t local(unsigned off)const{return get(1)+off;}
    template<unsigned N> void load(unsigned n,unsigned off,std::uint32_t pc){put(n,read<N>(s,local(off),pc));}
    template<unsigned N> void store(unsigned off,unsigned n,std::uint32_t pc){write<N>(s,local(off),get(n),pc);}
    void exponent(unsigned n,unsigned off,std::uint32_t pc){put(n,std::uint32_t(signed_value<2>(read<2>(s,local(off),pc))));}
    void back(unsigned n,std::uint32_t pc){backfill(s,31,n,pc);}
    bool limit(std::uint32_t pc){compare(s,word(get(0)),509);if(word(get(0))<=509)return false;s.pc=pc;put(3,0);return true;}
    void sign(bool scientific){
        load<1>(0,60,scientific?0x1077a3c0:0x1077a630);put(0,byte(get(0)));compare(s,word(get(0)),0,true);
        if(get(0)){put(0,'-');back(0,scientific?0x1077a3d0:0x1077a640);return;}
        compare(s,get(29),1);if(get(29)==1){put(0,'+');back(0,scientific?0x1077a3e4:0x1077a654);return;}
        compare(s,get(29),2);if(get(29)==2){put(0,' ');back(0,scientific?0x1077a3f8:0x1077a668);}
    }
    void special(bool nan){
        bool negative;
        if(nan){load<1>(0,60,0x1077a15c);put(0,byte(get(0)));compare(s,word(get(0)),0,true);negative=get(0)!=0;}
        else{s.wr<8>(16384,read<8>(s,get(2)-0x34cc,0x1077a0ec));const auto a=float_value<8>(s.r<8>(16880)),b=float_value<8>(s.r<8>(16384));s.wr<1>(1296,a<b);s.wr<1>(1297,b<a);s.wr<1>(1298,a==b);s.wr<1>(2304,(a<b?8:b<a?4:a==b?2:0)|unsigned(std::isnan(a)||std::isnan(b)));negative=a<b;}
        const auto pc=nan?(negative?0x1077a168:0x1077a198):(negative?0x1077a0f8:0x1077a128);
        put(0,read<1>(s,get(31)+get(27),pc));put(25,get(25)-(negative?5:4));put(0,get(0)&0x80);compare(s,word(get(0)),0,true);const bool upper=get(0)!=0;
        if(negative){put(4,read<4>(s,get(2)-(nan?(upper?0x3b60:0x3b64):(upper?0x3b58:0x3b5c)),pc+(upper?16:32)));put(3,get(25));call(s,0x1077dbf0,pc+(upper?28:44));}
        else{put(3,get(25));put(4,get(2)-(nan?(upper?0x34e0:0x34dc):(upper?0x34e8:0x34e4)));call(s,0x1077dbf0,pc+(upper?28:44));}put(3,get(25));
    }
    void strip_trailing_zeroes(){
        load<1>(0,64,0x1077a074);put(30,local(65));put(5,get(30)+get(0));
        for(;;){load<1>(0,64,0x1077a0a0);compare(s,get(0),1);if(get(0)<=1)break;const auto address=get(5)-1;put(0,read<1>(s,address,0x1077a0ac));put(5,address);compare(s,word(get(0)),'0');if(get(0)!='0')break;
            s.step(0x1077a088);load<1>(4,64,0x1077a088);exponent(3,62,0x1077a08c);put(4,get(4)-1);put(0,get(3)+1);store<1>(64,4,0x1077a098);store<2>(62,0,0x1077a09c);
        }
    }
    enum class Notation {invalid,fixed,scientific,general};
    Notation notation(){
        exponent(4,62,0x1077a1cc);compare(s,word(get(27)),'e');load<1>(3,64,0x1077a1d4);put(0,0);put(31,get(25)-1);put(3,get(3)+get(4));put(3,get(3)-1);store<2>(62,3,0x1077a1e8);write<1>(s,get(25)-1,get(0),0x1077a1ec);
        if(get(27)=='e')return Notation::scientific;
        if(word(get(27))<'e'){compare(s,word(get(27)),'F');if(get(27)=='F')return Notation::fixed;
            if(word(get(27))<'F'){compare(s,word(get(27)),'E');return word(get(27))>='E'?Notation::scientific:Notation::invalid;}compare(s,word(get(27)),'H');return word(get(27))<'H'?Notation::general:Notation::invalid;}
        compare(s,word(get(27)),'g');if(get(27)=='g')return Notation::general;return word(get(27))<'g'?Notation::fixed:Notation::invalid;
    }
    Notation general(){
        load<1>(0,64,0x1077a22c);compare(s,word(get(0)),word(get(26)));if(word(get(0))>word(get(26))){put(4,get(26));put(3,local(60));call(s,0x10779ed0,0x1077a244);}
        exponent(3,62,0x1077a244);compare(s,word(get(3)),-4);bool scientific=word(get(3))<-4;if(!scientific){compare(s,word(get(3)),word(get(26)));scientific=word(get(3))>=word(get(26));}
        compare(s,get(28));if(scientific){if(get(28))put(26,get(26)-1);else{load<1>(3,64,0x1077a268);put(26,get(3)-1);}compare(s,get(27),'g');put(27,get(27)=='g'?'e':'E');return Notation::scientific;}
        if(get(28)){put(0,get(3)+1);put(26,get(26)-get(0));}
        else{load<1>(0,64,0x1077a29c);put(3,get(3)+1);put(26,get(0)-get(3));compare(s,word(get(26)),0,true);if(word(get(26))<0)put(26,0);}return Notation::fixed;
    }
    bool scientific(){
        load<1>(0,64,0x1077a2b4);put(4,get(26)+1);compare(s,word(get(0)),word(get(4)));if(word(get(0))>word(get(4))){put(3,local(60));call(s,0x10779ed0,0x1077a2cc);}
        exponent(6,62,0x1077a2cc);put(8,'+');compare(s,word(get(6)));if(word(get(6))<0){put(6,0-get(6));put(8,'-');}put(3,0x66660000);put(4,get(3)+0x6667);put(7,0);
        for(;;){compare(s,word(get(6)));bool digit=get(6)!=0;if(!digit){compare(s,word(get(7)),2);digit=word(get(7))<2;}if(!digit)break;
            s.step(0x1077a2f4);put(5,std::uint32_t((std::int64_t(word(get(4)))*word(get(6)))>>32));const auto high=get(5);s.wr<1>(1027,word(high)<0&&(high&3));put(0,shift_signed<4>(high,2));put(3,std::rotl(get(0),1)&1);put(0,get(0)+get(3));put(0,get(0)*10);put(3,get(6)-get(0));put(3,get(3)+'0');write<1>(s,get(31)-1,get(3),0x1077a310);s.wr<1>(1027,word(high)<0&&(high&3));put(0,shift_signed<4>(high,2));put(3,std::rotl(get(0),1)&1);put(6,get(0)+get(3));put(7,get(7)+1);put(31,get(31)-1);
        }
        write<1>(s,get(31)-1,get(8),0x1077a338);backfill(s,31,27,0x1077a33c,2);put(0,get(25)-get(31));put(0,get(26)+get(0));if(limit(0x1077a350))return false;
        load<1>(3,64,0x1077a358);put(0,get(26)+1);compare(s,word(get(3)),word(get(0)));if(word(get(3))<word(get(0))){put(0,get(26)+2);put(3,get(0)-get(3));put(0,'0');for(;;){add_record(s,3,3,-1);if(!get(3))break;s.step(0x1077a378);back(0,0x1077a378);}}
        load<1>(3,64,0x1077a384);put(4,get(30)+get(3));for(;;){add_record(s,3,3,-1);if(!get(3))break;s.step(0x1077a390);const auto address=get(4)-1;put(0,read<1>(s,address,0x1077a390));put(4,address);back(0,0x1077a394);}
        compare(s,word(get(26)));bool point=get(26)!=0;if(!point){compare(s,get(28));point=get(28)!=0;}if(point){put(0,'.');back(0,0x1077a3b4);}load<1>(0,65,0x1077a3b8);back(0,0x1077a3bc);sign(true);return true;
    }
    // Reverse zero padding preserves the original eight-store groups and the
    // remainder's CTR/CR residues. The two call sites have distinct ABI locals.
    void fractional_zeroes(){
        compare(s,word(get(4)),8);put(3,get(4)-8);if(word(get(4))>8){put(0,get(3)+7);put(0,get(0)>>3);compare(s,word(get(3)));s.wr<4>(4132,get(0));put(0,'0');if(word(get(3))>0){do{s.step(0x1077a4a4);write<1>(s,get(31)-1,get(0),0x1077a4a4);put(5,get(5)+8);for(unsigned i=2;i<=8;++i)write<1>(s,get(31)-i,get(0),0x1077a4ac+(i-2)*4);put(31,get(31)-8);s.wr<4>(4132,s.r<4>(4132)-1);}while(s.r<4>(4132));}}
        put(0,get(4)-get(5));compare(s,word(get(5)),word(get(4)));s.wr<4>(4132,get(0));put(0,'0');if(word(get(5))<word(get(4))){do{s.step(0x1077a4e0);back(0,0x1077a4e0);s.wr<4>(4132,s.r<4>(4132)-1);}while(s.r<4>(4132));}
    }
    void leading_fractional_zeroes(){
        put(0,get(3)>>3);compare(s,word(get(0)),0,true);s.wr<4>(4132,get(0));const bool groups=get(0)!=0;
        if(groups){do{s.step(0x1077a52c);for(unsigned i=1;i<=8;++i)write<1>(s,get(31)-i,get(4),0x1077a52c+(i-1)*4);put(31,get(31)-8);s.wr<4>(4132,s.r<4>(4132)-1);}while(s.r<4>(4132));put(3,get(3)&7);compare(s,word(get(3)),0,true);}
        if(!groups||get(3)){s.wr<4>(4132,get(3));do{s.step(0x1077a560);back(4,0x1077a560);s.wr<4>(4132,s.r<4>(4132)-1);}while(s.r<4>(4132));}
    }
    void integer_digits(){
        put(0,get(3)>>3);compare(s,word(get(0)),0,true);s.wr<4>(4132,get(0));const bool groups=get(0)!=0;
        if(groups){do{s.step(0x1077a5c8);for(unsigned i=1;i<=8;++i){put(0,read<1>(s,get(6)-i,0x1077a5c8+(i-1)*8));write<1>(s,get(31)-i,get(0),0x1077a5cc+(i-1)*8);if(i==8)put(6,get(6)-8);}put(31,get(31)-8);s.wr<4>(4132,s.r<4>(4132)-1);}while(s.r<4>(4132));put(3,get(3)&7);compare(s,word(get(3)),0,true);}
        if(!groups||get(3)){s.wr<4>(4132,get(3));do{s.step(0x1077a618);const auto address=get(6)-1;put(0,read<1>(s,address,0x1077a618));put(6,address);back(0,0x1077a61c);s.wr<4>(4132,s.r<4>(4132)-1);}while(s.r<4>(4132));}
    }
    bool fixed(){
        exponent(0,62,0x1077a400);load<1>(4,64,0x1077a404);put(3,get(4)-get(0));add_record(s,8,3,-1);if(word(get(8))<0)put(8,0);compare(s,word(get(8)),word(get(26)));
        if(word(get(8))>word(get(26))){put(0,get(8)-get(26));put(3,local(60));put(4,get(4)-get(0));call(s,0x10779ed0,0x1077a430);exponent(3,62,0x1077a430);load<1>(0,64,0x1077a434);put(3,get(0)-get(3));add_record(s,8,3,-1);if(word(get(8))<0)put(8,0);}
        exponent(3,62,0x1077a448);add_record(s,7,3,1);if(word(get(7))<0)put(7,0);put(0,get(7)+get(8));if(limit(0x1077a464))return false;
        load<1>(0,64,0x1077a46c);put(4,get(26)-get(8));compare(s,word(get(4)),0,true);const bool padding=word(get(4))>0;put(5,0);put(6,get(30)+get(0));if(padding)fractional_zeroes();put(3,0);
        for(;;){compare(s,word(get(3)),word(get(8)));if(word(get(3))>=word(get(8)))break;load<1>(0,64,0x1077a504);compare(s,word(get(3)),word(get(0)));if(word(get(3))>=word(get(0)))break;s.step(0x1077a4f0);const auto address=get(6)-1;put(0,read<1>(s,address,0x1077a4f0));put(6,address);put(3,get(3)+1);back(0,0x1077a4f8);}
        compare(s,word(get(3)),word(get(8)));const bool leading=word(get(3))<word(get(8));put(3,get(8)-get(3));put(4,'0');if(leading)leading_fractional_zeroes();
        compare(s,word(get(26)));bool point=get(26)!=0;if(!point){compare(s,get(28));point=get(28)!=0;}if(point){put(0,'.');back(0,0x1077a57c);}compare(s,word(get(7)));
        if(get(7)){put(4,0);put(3,'0');for(;;){load<1>(0,64,0x1077a5a0);put(0,get(7)-get(0));compare(s,word(get(4)),word(get(0)));if(word(get(4))>=word(get(0)))break;s.step(0x1077a598);back(3,0x1077a598);put(4,get(4)+1);}compare(s,word(get(4)),word(get(7)));const bool digits=word(get(4))<word(get(7));put(3,get(7)-get(4));if(digits)integer_digits();}
        else{put(0,'0');back(0,0x1077a62c);}sign(false);return true;
    }
public:
    explicit DecimalOutput(State& state):s(state){}
    void format(){
        write<8>(s,get(1)-8,s.r<8>(16880),0x1077a000);put(0,s.r<4>(4128));s.wr<8>(16880,s.r<8>(16400));words(s,25,0x1077a00c,false,44);put(25,get(5));put(31,read<4>(s,get(2)-0x5a48,0x1077a014));write<4>(s,get(1)+8,get(0),0x1077a018);frame(s,160,0x1077a01c);
        for(unsigned n=6;n<=10;++n)store<4>(196+(n-6)*4,n,0x1077a020+(n-6)*4);load<4>(26,208,0x1077a034);load<1>(27,201,0x1077a038);compare(s,word(get(26)),509);load<1>(28,199,0x1077a040);load<1>(29,197,0x1077a044);
        if(word(get(26))>509)put(3,0);else{
            put(3,0);put(0,32);store<1>(56,3,0x1077a05c);put(3,local(56));store<2>(58,0,0x1077a064);put(6,local(60));call(s,0x107800d8,0x1077a070);load<4>(2,20,0x1077a070);strip_trailing_zeroes();load<1>(0,65,0x1077a0b8);compare(s,word(get(0)),'I');
            if(get(0)=='I')special(false);else{if(word(get(0))>='I'){compare(s,word(get(0)),'N');if(get(0)=='N'){special(true);epilogue();return;}}
                else{compare(s,word(get(0)),'0');if(get(0)=='0'){put(0,0);store<2>(62,0,0x1077a0e4);}}
                auto kind=notation();if(kind==Notation::general)kind=general();bool result=true;if(kind==Notation::scientific)result=scientific();else if(kind==Notation::fixed)result=fixed();if(result)put(3,get(31));
            }
        }epilogue();
    }
    void epilogue(){load<4>(0,168,0x1077a670);put(1,get(1)+160);s.wr<8>(16880,read<8>(s,get(1)-8,0x1077a678));words(s,25,0x1077a67c,true,44);s.wr<4>(4128,get(0));finish(s);}
};
}
void format_decimal_float(State& s){DecimalOutput(s).format();}
}

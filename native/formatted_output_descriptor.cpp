#include "formatted_output_internal.hpp"
namespace cm::aot::formatted_output {
namespace {
class DescriptorOutput {
    State& s;
    std::uint32_t get(unsigned n)const{return reg(s,n);}
    void put(unsigned n,std::uint32_t v){set(s,n,v);}
    std::uint32_t local(unsigned off)const{return get(1)+off;}
    template<unsigned N> void load(unsigned n,unsigned off,std::uint32_t pc){put(n,read<N>(s,local(off),pc));}
    template<unsigned N> void store(unsigned off,unsigned n,std::uint32_t pc){write<N>(s,local(off),get(n),pc);}
    void floating_load(unsigned n,std::uint32_t address,std::uint32_t pc,bool single=false){const auto bits=single?float_bits<8>(float_value<4>(read<4>(s,address,pc))):read<8>(s,address,pc);s.wr<8>(16384+n*16,bits);}
    // Every emission re-enters the generated descriptor stub. It observes the
    // descriptor's live entry and TOC and leaves all callback clobbers visible.
    bool callback(std::uint32_t link){call(s,0x10781320,link);load<4>(2,20,link);compare(s,get(3));if(get(3))return true;put(3,UINT32_MAX);return false;}
    bool literal(bool remainder){
        if(remainder){put(3,get(22));call(s,0x1077dbd0,0x1077a6e0);put(5,get(3));compare(s,word(get(5)),0,true);put(28,get(28)+get(5));if(!get(5))return true;put(3,get(30));put(4,get(22));put(12,get(31));return callback(0x1077a6fc);}
        put(5,get(21)-get(22));compare(s,word(get(5)),0,true);put(28,get(28)+get(5));if(!get(5))return true;put(3,get(30));put(4,get(22));put(12,get(31));return callback(0x1077a72c);
    }
    enum class Conversion {signed_integer,unsigned_integer,decimal,hexadecimal,string,count,character,percent,invalid};
    Conversion conversion(){
        load<1>(0,121,0x1077a760);compare(s,word(get(0)),'h');if(get(0)=='h')return Conversion::invalid;
        if(word(get(0))<'h'){
            compare(s,word(get(0)),'X');if(get(0)=='X')return Conversion::unsigned_integer;
            if(word(get(0))<'X'){compare(s,word(get(0)),'A');if(get(0)=='A')return Conversion::hexadecimal;
                if(word(get(0))<'A'){compare(s,word(get(0)),'%');return get(0)=='%'?Conversion::percent:Conversion::invalid;}
                compare(s,word(get(0)),'H');if(word(get(0))>='H')return Conversion::invalid;compare(s,word(get(0)),'E');return word(get(0))>='E'?Conversion::decimal:Conversion::invalid;
            }
            compare(s,word(get(0)),'c');if(get(0)=='c')return Conversion::character;
            if(word(get(0))<'c'){compare(s,word(get(0)),'a');return get(0)=='a'?Conversion::hexadecimal:Conversion::invalid;}
            compare(s,word(get(0)),'e');return word(get(0))>='e'?Conversion::decimal:Conversion::signed_integer;
        }
        compare(s,word(get(0)),'t');if(get(0)=='t')return Conversion::invalid;
        if(word(get(0))<'t'){compare(s,word(get(0)),'o');if(get(0)=='o')return Conversion::unsigned_integer;
            if(word(get(0))<'o'){compare(s,word(get(0)),'n');if(word(get(0))>='n')return Conversion::count;compare(s,word(get(0)),'j');return word(get(0))>='j'?Conversion::invalid:Conversion::signed_integer;}
            compare(s,word(get(0)),'s');return word(get(0))>='s'?Conversion::string:Conversion::invalid;}
        compare(s,word(get(0)),'x');if(get(0)=='x')return Conversion::unsigned_integer;
        if(word(get(0))<'x'){compare(s,word(get(0)),'v');return word(get(0))>='v'?Conversion::invalid:Conversion::unsigned_integer;}compare(s,word(get(0)),255);return Conversion::invalid;
    }
    void packed_argument(std::uint32_t pc){
        load<4>(3,804,pc);put(0,get(3)+15);put(3,get(0)&0xfffffff0);put(6,get(3)+16);store<4>(804,6,pc+16);put(5,read<4>(s,get(3),pc+20));put(4,read<4>(s,get(3)+4,pc+24));put(3,read<4>(s,get(3)+8,pc+28));put(0,read<4>(s,get(6)-4,pc+32));store<4>(100,5,pc+36);store<4>(104,4,pc+40);store<4>(108,3,pc+44);store<4>(112,0,pc+48);
    }
    void integer_argument(bool signed_integer){
        const auto start=signed_integer?0x1077a828:0x1077aabc;load<1>(0,120,start);compare(s,get(0),6);
        if(get(0)==6){compare(s,word(get(27)));if(!get(27)){load<4>(0,164,start+20);compare(s,word(get(0)));if(get(0)){put(3,16);put(0,0);store<4>(132,3,start+40);store<1>(173,0,start+44);}packed_argument(start+48);}
            const auto dispatch=start+100;load<1>(0,173,dispatch);compare(s,word(get(0)),1);unsigned lane=0;
            if(get(0)==1)lane=2;else if(word(get(0))<1){compare(s,word(get(0)),0);if(word(get(0))>=0)lane=1;}else{compare(s,word(get(0)),3);if(word(get(0))<3)lane=4;}
            if(lane){const auto pc=dispatch+(lane==1?40:lane==2?(signed_integer?56:52):(signed_integer?68:64));put(3,local(100));const auto index=lane==1?get(27):lane==2?get(25):get(26);if(lane==1)put(29,read<1>(s,get(3)+index,pc+4));else if(lane==2)put(29,read<2>(s,get(3)+index,pc+4));else put(29,read<4>(s,get(3)+index,pc+4));if(signed_integer){if(lane==1)put(29,byte(get(29)));else if(lane==2)put(29,std::uint32_t(signed_value<2>(get(29))));}}
        }else{
            const auto pc=signed_integer?0x1077a8dc:0x1077ab6c;compare(s,get(0),3);if(get(0)==3){load<4>(3,804,pc+8);put(3,get(3)+4);store<4>(804,3,pc+16);put(29,read<4>(s,get(3)-4,pc+20));}
            else{compare(s,get(0),4);if(get(0)==4){load<4>(3,804,pc+36);put(3,get(3)+8);store<4>(804,3,pc+44);put(23,read<4>(s,get(3)-8,pc+48));put(24,read<4>(s,get(3)-4,pc+52));}else{load<4>(3,804,pc+60);put(3,get(3)+4);store<4>(804,3,pc+68);put(29,read<4>(s,get(3)-4,pc+72));}}
        }
        const auto pc=signed_integer?0x1077a928:0x1077abb8;load<1>(3,120,pc);compare(s,get(3),2);if(get(3)==2){put(0,signed_integer?std::uint32_t(signed_value<2>(get(29))):get(29)&65535);put(29,get(0));}compare(s,get(3),1);if(get(3)==1){put(0,signed_integer?byte(get(29)):get(29)&255);put(29,get(0));}compare(s,get(3),4);
    }
    // Copies each field pair in original read/write order. The 60-byte record
    // contains padding and caller aliases, so a bulk or zeroed copy would differ.
    void marshal_number(std::uint32_t pc,bool wide,bool floating){
        const unsigned first=wide?6:floating?3:5;const unsigned destination=wide||floating?36:32;load<4>(first,116,pc);
        if(wide)put(4,get(24));else if(floating)s.wr<8>(16400,s.r<8>(16880));else put(3,get(29));load<4>(0,120,pc+8);if(wide)put(3,get(23));if(wide||floating)put(5,local(688));else put(4,local(688));
        const auto next=pc+(wide?28:24);store<4>(destination,first,pc+(wide?20:16));store<4>(destination+4,0,pc+(wide?24:20));
        for(unsigned i=1;i<7;++i){load<4>(first,116+i*8,next+(i-1)*16);load<4>(0,120+i*8,next+(i-1)*16+4);store<4>(destination+i*8,first,next+(i-1)*16+8);store<4>(destination+i*8+4,0,next+(i-1)*16+12);}
        const auto half=next+96;load<2>(0,172,half);store<2>(destination+56,0,half+4);load<2>(0,174,half+8);store<2>(destination+58,0,half+12);for(unsigned i=0;i<(wide||floating?5u:6u);++i)load<4>((wide||floating?6u:5u)+i,destination+i*4,half+16+i*4);
    }
    bool integer(bool signed_integer){
        integer_argument(signed_integer);const bool wide=get(3)==4;const auto pc=signed_integer?(wide?0x1077a954:0x1077aa04):(wide?0x1077abe4:0x1077ac94);
        marshal_number(pc,wide,false);call(s,wide?0x10779880:0x10779620,pc+(wide?164:164));put(19,get(3));compare(s,word(get(19)),0,true);if(!get(19))return false;put(0,local(687));put(20,get(0)-get(19));return true;
    }
    void floating_argument(bool hexadecimal){
        const auto pc=hexadecimal?0x1077af14:0x1077ad4c;load<1>(0,120,pc);compare(s,get(0),hexadecimal?5:6);
        if(!hexadecimal&&get(0)==6){compare(s,word(get(27)));if(!get(27)){load<4>(0,164,0x1077ad60);compare(s,word(get(0)));if(get(0)){put(3,4);put(0,2);store<4>(132,3,0x1077ad74);store<1>(173,0,0x1077ad78);}packed_argument(0x1077ad7c);}
            load<1>(0,173,0x1077adb0);compare(s,word(get(0)),1);unsigned lane=0;if(get(0)==1)lane=2;else if(word(get(0))<1){compare(s,word(get(0)),0);if(word(get(0))>=0)lane=1;}else{compare(s,word(get(0)),3);if(word(get(0))<3)lane=4;}
            if(lane==4){put(3,local(100));floating_load(31,get(3)+get(26),0x1077ae2c,true);}
            else if(lane){const auto pc=lane==1?0x1077add8:0x1077ae00;put(4,local(100));put(3,read<4>(s,get(2)-0x3b68,pc+4));put(4,lane==1?read<1>(s,get(4)+get(27),pc+8):read<2>(s,get(4)+get(25),pc+8));put(0,0x43300000);store<4>(688,0,pc+16);floating_load(1,get(3),pc+20);store<4>(692,4,pc+24);floating_load(0,local(688),pc+28);s.wr<8>(16880,float_bits<8>(float_value<8>(s.r<8>(16384))-float_value<8>(s.r<8>(16400))));}
        }else{
            const auto start=hexadecimal?(get(0)==5?0x1077af20:0x1077af34):(get(0)==5?0x1077ae3c:0x1077ae50);if(!hexadecimal)compare(s,get(0),5);load<4>(3,804,start);put(3,get(3)+8);store<4>(804,3,start+8);floating_load(31,get(3)-8,start+12);
        }
    }
    bool floating(bool hexadecimal){floating_argument(hexadecimal);const auto pc=hexadecimal?0x1077af44:0x1077ae60;marshal_number(pc,false,true);call(s,hexadecimal?0x10779b70:0x1077a000,pc+160);put(19,get(3));compare(s,word(get(19)),0,true);if(!get(19))return false;put(0,local(687));put(20,get(0)-get(19));return true;}
    bool string(){
        load<1>(0,120,0x1077aff8);compare(s,get(0),7);if(get(0)==7){load<4>(3,804,0x1077b004);put(3,get(3)+4);store<4>(804,3,0x1077b00c);put(4,read<4>(s,get(3)-4,0x1077b010));compare(s,get(4));if(!get(4))put(4,get(2)-0x34d8);put(3,local(176));put(5,512);call(s,0x10778260,0x1077b02c);compare(s,word(get(3)));if(word(get(3))<0)return false;put(19,local(176));}
        else{load<4>(3,804,0x1077b03c);put(3,get(3)+4);store<4>(804,3,0x1077b044);put(19,read<4>(s,get(3)-4,0x1077b048));}
        compare(s,get(19));if(!get(19))put(19,get(2)-0x34ec);load<1>(0,119,0x1077b058);compare(s,get(0));if(get(0)){load<1>(0,118,0x1077b064);put(20,read<1>(s,get(19),0x1077b068));put(19,get(19)+1);compare(s,get(0));if(get(0)){load<4>(0,128,0x1077b078);compare(s,word(get(20)),word(get(0)));if(word(get(20))>word(get(0)))put(20,get(0));}}
        else{load<1>(0,118,0x1077b08c);compare(s,get(0));if(get(0)){load<4>(20,128,0x1077b098);put(3,get(19));put(4,0);put(5,get(20));call(s,0x10778330,0x1077b0ac);compare(s,get(3));if(get(3))put(20,get(3)-get(19));}
            else{put(3,get(19));call(s,0x1077dbd0,0x1077b0c4);put(20,get(3));}}return true;
    }
    void count(){
        load<4>(3,804,0x1077b0cc);put(3,get(3)+4);store<4>(804,3,0x1077b0d4);load<1>(0,120,0x1077b0d8);put(3,read<4>(s,get(3)-4,0x1077b0dc));compare(s,word(get(0)),2);
        if(get(0)==2)write<2>(s,get(3),get(28),0x1077b110);else if(word(get(0))<2){compare(s,word(get(0)));if(!get(0))write<4>(s,get(3),get(28),0x1077b108);}
        else{compare(s,word(get(0)),4);if(get(0)==4){write<4>(s,get(3)+4,get(28),0x1077b120);const auto value=get(28);s.wr<1>(1027,word(value)<0&&(value&0x7fffffff));put(0,shift_signed<4>(value,31));write<4>(s,get(3),get(0),0x1077b128);}else if(word(get(0))<4)write<4>(s,get(3),get(28),0x1077b118);}
    }
    void character(){
        load<1>(0,120,0x1077b130);put(19,local(176));compare(s,get(0),6);if(get(0)==6){compare(s,word(get(27)));if(!get(27)){load<4>(0,164,0x1077b148);compare(s,word(get(0)));if(get(0)){put(3,16);put(0,0);store<4>(132,3,0x1077b15c);store<1>(173,0,0x1077b160);}load<4>(0,148,0x1077b164);compare(s,word(get(0)));if(!get(0)){load<4>(3,140,0x1077b170);store<4>(136,3,0x1077b174);put(0,read<1>(s,get(3),0x1077b178));put(3,byte(get(0)));put(0,0-get(3));put(0,get(0)|get(3));put(0,std::rotl(get(0),1)&1);store<4>(144,0,0x1077b18c);}packed_argument(0x1077b190);}
            load<1>(0,173,0x1077b1c4);compare(s,word(get(0)),1);unsigned lane=0;if(get(0)==1)lane=2;else if(word(get(0))<1){compare(s,word(get(0)),0);if(word(get(0))>=0)lane=1;}else{compare(s,word(get(0)),3);if(word(get(0))<3)lane=4;}
            if(lane){const auto pc=lane==1?0x1077b1ec:lane==2?0x1077b1fc:0x1077b20c;put(3,local(100));put(0,lane==1?read<1>(s,get(3)+get(27),pc+4):lane==2?read<2>(s,get(3)+get(25),pc+4):read<4>(s,get(3)+get(26),pc+4));store<1>(176,0,pc+8);}}
        else{load<4>(3,804,0x1077b21c);put(3,get(3)+4);store<4>(804,3,0x1077b224);put(0,read<4>(s,get(3)-4,0x1077b228));store<1>(176,0,0x1077b22c);}put(20,1);
    }
    bool invalid(){put(3,get(21));call(s,0x1077dbd0,0x1077b254);put(5,get(3));compare(s,word(get(5)),0,true);put(28,get(28)+get(5));if(get(5)){put(3,get(30));put(4,get(21));put(12,get(31));if(!callback(0x1077b270))return false;}put(3,get(28));return true;}
    bool padding_and_emit(){
        load<1>(0,116,0x1077b28c);put(18,get(20));compare(s,get(0));if(get(0)){compare(s,get(0),2);const bool zero=get(0)==2;put(0,' ');if(zero)put(0,'0');store<1>(97,0,0x1077b2ac);put(0,read<1>(s,get(19),0x1077b2b0));put(0,byte(get(0)));compare(s,word(get(0)),'+');bool sign=get(0)=='+';if(!sign){compare(s,word(get(0)),'-');sign=get(0)=='-';if(!sign){compare(s,word(get(0)),' ');sign=get(0)==' ';}}
            if(sign){load<1>(0,97,0x1077b2d0);compare(s,word(get(0)),'0');if(get(0)=='0'){put(3,get(30));put(4,get(19));put(12,get(31));put(5,1);if(!callback(0x1077b2f0))return false;put(19,get(19)+1);put(20,get(20)-1);}}
            for(;;){load<4>(0,124,0x1077b33c);compare(s,word(get(18)),word(get(0)));if(word(get(18))>=word(get(0)))break;s.step(0x1077b310);put(3,get(30));put(12,get(31));put(4,local(97));put(5,1);if(!callback(0x1077b324))return false;put(18,get(18)+1);}}
        compare(s,word(get(20)));if(get(20)){put(3,get(30));put(4,get(19));put(5,get(20));put(12,get(31));if(!callback(0x1077b364))return false;}
        load<1>(0,116,0x1077b378);compare(s,get(0));if(!get(0)){put(19,' ');for(;;){load<4>(0,124,0x1077b3c4);compare(s,word(get(18)),word(get(0)));if(word(get(18))>=word(get(0)))break;s.step(0x1077b390);store<1>(96,19,0x1077b390);put(3,get(30));put(12,get(31));put(4,local(96));put(5,1);if(!callback(0x1077b3a8))return false;put(18,get(18)+1);}}
        return true;
    }
    bool packed_delimiter(){
        load<1>(0,120,0x1077b3d0);put(28,get(28)+get(18));compare(s,get(0),6);if(get(0)!=6)return true;load<4>(0,132,0x1077b3e0);put(27,get(27)+1);put(26,get(26)+4);compare(s,word(get(27)),word(get(0)));put(25,get(25)+2);if(word(get(27))>=word(get(0)))return true;
        load<4>(4,136,0x1077b3f8);compare(s,get(4));if(get(4)){load<4>(5,144,0x1077b404);compare(s,word(get(5)));if(word(get(5))>0){compare(s,word(get(5)),1);if(get(5)==1){put(3,get(30));put(12,get(31));put(5,1);if(!callback(0x1077b428))return false;}else{put(3,get(30));put(12,get(31));if(!callback(0x1077b448))return false;}load<4>(0,144,0x1077b45c);put(28,get(28)+get(0));}}
        load<4>(4,156,0x1077b464);put(0,' ');load<4>(3,160,0x1077b46c);store<4>(124,4,0x1077b470);store<4>(128,3,0x1077b474);store<1>(97,0,0x1077b478);return true;
    }
    bool more_lanes(){load<1>(0,120,0x1077b47c);compare(s,get(0),6);if(get(0)!=6)return false;load<4>(0,132,0x1077b488);compare(s,word(get(27)),word(get(0)));return word(get(27))<word(get(0));}
public:
    explicit DescriptorOutput(State& state):s(state){}
    void format(){
        write<8>(s,get(1)-8,s.r<8>(16880),0x1077a690);put(0,s.r<4>(4128));words(s,18,0x1077a698,false,72);put(31,get(3));put(30,get(4));put(22,get(5));put(28,0);write<4>(s,get(1)+8,get(0),0x1077a6ac);put(0,' ');frame(s,768,0x1077a6b4);store<4>(804,6,0x1077a6b8);store<1>(97,0,0x1077a6bc);
        for(;;){
            put(0,read<1>(s,get(22),0x1077b494));put(0,byte(get(0)));compare(s,word(get(0)),0,true);if(!get(0)){put(3,get(28));break;}s.step(0x1077a6c4);put(3,get(22));put(4,'%');call(s,0x1077dd10,0x1077a6d0);put(21,get(3));compare(s,word(get(21)),0,true);if(!get(21)){if(literal(true))put(3,get(28));break;}if(!literal(false))break;
            put(3,get(21));put(4,local(804));put(5,local(116));call(s,0x10778db0,0x1077a750);put(25,0);put(22,get(3));put(26,get(25));put(27,0);
            bool done=false;
            do{const auto kind=conversion();bool valid=true;if(kind==Conversion::signed_integer)valid=integer(true);else if(kind==Conversion::unsigned_integer)valid=integer(false);else if(kind==Conversion::decimal)valid=floating(false);else if(kind==Conversion::hexadecimal)valid=floating(true);else if(kind==Conversion::string)valid=string();else if(kind==Conversion::count){count();if(more_lanes())continue;break;}else if(kind==Conversion::character)character();else if(kind==Conversion::percent){put(0,'%');store<1>(176,0,0x1077b23c);put(19,local(176));put(20,1);}else valid=false;
                if(!valid){invalid();done=true;break;}if(!padding_and_emit()||!packed_delimiter()){done=true;break;}
            }while(more_lanes());if(done)break;
        }
        load<4>(0,776,0x1077b4a4);put(1,get(1)+768);floating_load(31,get(1)-8,0x1077b4ac);words(s,18,0x1077b4b0,true,72);s.wr<4>(4128,get(0));finish(s);
    }
};
}
void format_descriptor(State& s){DescriptorOutput(s).format();}
}

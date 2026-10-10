#include "formatted_output_internal.hpp"
namespace cm::aot::formatted_output {
namespace {
class FormatParser {
    State& s;
    const std::uint32_t caller;
    std::uint32_t get(unsigned n) const{return reg(s,n);}
    void put(unsigned n,std::uint32_t value){set(s,n,value);}
    std::uint32_t local(unsigned off) const{return get(1)+off;}
    template<unsigned N> void load(unsigned n,unsigned off,std::uint32_t pc){put(n,read<N>(s,local(off),pc));}
    template<unsigned N> void store(unsigned off,unsigned n,std::uint32_t pc){write<N>(s,local(off),get(n),pc);}
    void next(std::uint32_t pc){const auto address=get(30)+1;put(29,read<1>(s,address,pc));put(30,address);put(29,byte(get(29)));}
    void invalid(std::uint32_t pc){put(0,255);store<1>(61,0,pc);}
    void escape(std::uint32_t target){++s.formatted_output_dispatch_escapes;s.pc=target;run_until(s,caller);escaped=true;}
    bool escaped=false;
    void initialize(){
        words(s,27,0x10778db0,false);put(0,s.r<4>(4128));put(7,0);put(31,read<4>(s,get(2)-0x5a48,0x10778dbc));put(6,1);put(27,get(4));put(28,get(5));put(30,get(3)+1);write<4>(s,get(1)+8,get(0),0x10778dd0);frame(s,144,0x10778dd4);
        put(29,read<1>(s,get(3)+1,0x10778dd8));store<1>(57,7,0x10778ddc);put(29,byte(get(29)));compare(s,word(get(29)),'%');store<1>(56,6,0x10778de8);store<1>(58,7,0x10778dec);store<1>(59,7,0x10778df0);store<1>(60,7,0x10778df4);store<4>(64,7,0x10778df8);store<4>(68,7,0x10778dfc);
    }
    void percent_record(){
        store<1>(61,29,0x10778e04);put(3,get(30)+1);load<4>(4,56,0x10778e0c);load<4>(0,60,0x10778e10);write<4>(s,get(28),get(4),0x10778e14);
        load<4>(6,72,0x10778e18);write<4>(s,get(28)+4,get(0),0x10778e1c);load<4>(5,76,0x10778e20);write<4>(s,get(28)+8,get(7),0x10778e24);
        load<4>(4,80,0x10778e28);write<4>(s,get(28)+12,get(7),0x10778e2c);load<4>(0,84,0x10778e30);write<4>(s,get(28)+16,get(6),0x10778e34);
        load<4>(6,88,0x10778e38);write<4>(s,get(28)+20,get(5),0x10778e3c);load<4>(5,92,0x10778e40);write<4>(s,get(28)+24,get(4),0x10778e44);
        load<4>(4,96,0x10778e48);write<4>(s,get(28)+28,get(0),0x10778e4c);load<4>(0,100,0x10778e50);write<4>(s,get(28)+32,get(6),0x10778e54);
        load<4>(6,104,0x10778e58);write<4>(s,get(28)+36,get(5),0x10778e5c);load<4>(5,108,0x10778e60);write<4>(s,get(28)+40,get(4),0x10778e64);
        load<2>(4,112,0x10778e68);write<4>(s,get(28)+44,get(0),0x10778e6c);load<2>(0,114,0x10778e70);write<4>(s,get(28)+48,get(6),0x10778e74);write<4>(s,get(28)+52,get(5),0x10778e78);write<2>(s,get(28)+56,get(4),0x10778e7c);write<2>(s,get(28)+58,get(0),0x10778e80);
    }
    void copy_width_failure(){
        put(0,255);load<4>(4,56,0x10779008);store<1>(61,0,0x1077900c);put(3,get(30)+1);load<4>(0,68,0x10779014);write<4>(s,get(28),get(4),0x10779018);
        load<4>(4,60,0x1077901c);load<4>(6,72,0x10779020);write<4>(s,get(28)+4,get(4),0x10779024);load<4>(5,76,0x10779028);write<4>(s,get(28)+8,get(7),0x1077902c);
        load<4>(4,80,0x10779030);write<4>(s,get(28)+12,get(0),0x10779034);load<4>(0,84,0x10779038);write<4>(s,get(28)+16,get(6),0x1077903c);
        load<4>(6,88,0x10779040);write<4>(s,get(28)+20,get(5),0x10779044);load<4>(5,92,0x10779048);write<4>(s,get(28)+24,get(4),0x1077904c);
        load<4>(4,96,0x10779050);write<4>(s,get(28)+28,get(0),0x10779054);load<4>(0,100,0x10779058);write<4>(s,get(28)+32,get(6),0x1077905c);
        load<4>(6,104,0x10779060);write<4>(s,get(28)+36,get(5),0x10779064);load<4>(5,108,0x10779068);write<4>(s,get(28)+40,get(4),0x1077906c);
        load<2>(4,112,0x10779070);write<4>(s,get(28)+44,get(0),0x10779074);load<2>(0,114,0x10779078);write<4>(s,get(28)+48,get(6),0x1077907c);write<4>(s,get(28)+52,get(5),0x10779080);write<2>(s,get(28)+56,get(4),0x10779084);write<2>(s,get(28)+58,get(0),0x10779088);
    }
    void copy_packed_failure(){
        put(3,255);load<4>(0,56,0x107792a4);store<1>(61,3,0x107792a8);put(3,get(30));load<4>(4,64,0x107792b0);write<4>(s,get(28),get(0),0x107792b4);
        load<4>(5,60,0x107792b8);load<4>(0,68,0x107792bc);write<4>(s,get(28)+4,get(5),0x107792c0);load<4>(5,72,0x107792c4);write<4>(s,get(28)+8,get(4),0x107792c8);
        load<4>(4,76,0x107792cc);write<4>(s,get(28)+12,get(0),0x107792d0);load<4>(0,80,0x107792d4);write<4>(s,get(28)+16,get(5),0x107792d8);
        load<4>(5,84,0x107792dc);write<4>(s,get(28)+20,get(4),0x107792e0);load<4>(4,88,0x107792e4);write<4>(s,get(28)+24,get(0),0x107792e8);
        load<4>(0,92,0x107792ec);write<4>(s,get(28)+28,get(5),0x107792f0);load<4>(5,96,0x107792f4);write<4>(s,get(28)+32,get(4),0x107792f8);
        load<4>(4,100,0x107792fc);write<4>(s,get(28)+36,get(0),0x10779300);load<4>(0,104,0x10779304);write<4>(s,get(28)+40,get(5),0x10779308);
        load<4>(5,108,0x1077930c);write<4>(s,get(28)+44,get(4),0x10779310);load<2>(4,112,0x10779314);write<4>(s,get(28)+48,get(0),0x10779318);load<2>(0,114,0x1077931c);write<4>(s,get(28)+52,get(5),0x10779320);write<2>(s,get(28)+56,get(4),0x10779324);write<2>(s,get(28)+58,get(0),0x10779328);
    }
    void copy_record(){
        load<4>(0,56,0x10779584);put(3,get(30)+1);load<4>(7,64,0x1077958c);write<4>(s,get(28),get(0),0x10779590);
        load<4>(0,60,0x10779594);load<4>(8,68,0x10779598);write<4>(s,get(28)+4,get(0),0x1077959c);load<4>(5,72,0x107795a0);write<4>(s,get(28)+8,get(7),0x107795a4);
        load<4>(4,76,0x107795a8);write<4>(s,get(28)+12,get(8),0x107795ac);load<4>(0,80,0x107795b0);write<4>(s,get(28)+16,get(5),0x107795b4);
        load<4>(5,84,0x107795b8);write<4>(s,get(28)+20,get(4),0x107795bc);load<4>(4,88,0x107795c0);write<4>(s,get(28)+24,get(0),0x107795c4);
        load<4>(0,92,0x107795c8);write<4>(s,get(28)+28,get(5),0x107795cc);load<4>(6,104,0x107795d0);write<4>(s,get(28)+32,get(4),0x107795d4);
        load<4>(5,108,0x107795d8);write<4>(s,get(28)+36,get(0),0x107795dc);load<2>(4,112,0x107795e0);write<4>(s,get(28)+40,get(7),0x107795e4);
        load<2>(0,114,0x107795e8);write<4>(s,get(28)+44,get(8),0x107795ec);write<4>(s,get(28)+48,get(6),0x107795f0);write<4>(s,get(28)+52,get(5),0x107795f4);write<2>(s,get(28)+56,get(4),0x107795f8);store<4>(100,8,0x107795fc);store<4>(96,7,0x10779600);write<2>(s,get(28)+58,get(0),0x10779604);
    }
    bool flag_destination(std::uint32_t target){
        switch(target){
        case 0x10778ecc:put(0,0);store<1>(56,0,0x10778ed0);break;
        case 0x10778ed8:put(0,1);store<1>(57,0,0x10778edc);break;
        case 0x10778ee4:load<1>(0,57,0x10778ee4);compare(s,get(0),1);if(get(0)!=1){put(0,2);store<1>(57,0,0x10778ef4);}break;
        case 0x10778efc:put(0,1);store<1>(59,0,0x10778f00);break;
        case 0x10778f08:load<1>(0,56,0x10778f08);compare(s,get(0));if(get(0)){put(0,2);store<1>(56,0,0x10778f18);}break;
        case 0x10778f20:load<1>(0,112,0x10778f20);compare(s,get(0));if(get(0))put(4,0);else{put(0,1);store<4>(88,0,0x10778f30);}break;
        case 0x10778f38:put(4,0);break;
        default:escape(target);return false;
        }return true;
    }
    void flags(){
        for(;;){
            s.step(0x10778ea8);put(0,get(29)-32);compare(s,get(0),32);put(4,1);
            if(get(0)>32)put(4,0);
            else{put(3,read<4>(s,get(2)-0x3b48,0x10778eb8));put(0,get(0)*4);put(3,read<4>(s,get(3)+get(0),0x10778ec0));s.wr<4>(4132,get(3));if(!flag_destination(get(3)))return;}
            compare(s,word(get(4)));if(!get(4))break;
            put(29,read<1>(s,get(30)+1,0x10778f44));put(0,1);store<4>(80,30,0x10778f4c);put(30,get(30)+1);put(29,byte(get(29)));store<4>(84,0,0x10778f58);
        }
        load<4>(0,88,0x10778f60);compare(s,word(get(0)));if(get(0)){
            put(3,read<4>(s,get(27),0x10778f6c));put(3,get(3)+4);write<4>(s,get(27),get(3),0x10778f74);put(3,read<4>(s,get(3)-4,0x10778f78));store<4>(76,3,0x10778f7c);call(s,0x1077dbd0,0x10778f84);store<4>(84,3,0x10778f84);
        }
    }
    void field(bool precision){
        const unsigned off=precision?68:64;const auto base=precision?0x107790a8u:0x10778f88u;
        compare(s,word(get(29)),'*');if(get(29)=='*'){
            put(3,read<4>(s,get(27),base+8));put(3,get(3)+4);write<4>(s,get(27),get(3),base+16);put(0,read<4>(s,get(3)-4,base+20));compare(s,word(get(0)));store<4>(off,0,base+28);
            if(word(get(0))<0){if(precision){put(0,0);store<1>(58,0,0x107790d0);}else{put(0,0-get(0));put(3,0);store<4>(64,0,0x10778fb4);store<1>(56,3,0x10778fb8);}}
            next(precision?0x107790d4:0x10778fbc);return;
        }
        for(;;){
            s.step(precision?0x10779100:0x10778fe8);put(0,get(29)&255);put(0,read<1>(s,get(31)+get(0),precision?0x10779104:0x10778fec));put(0,get(0)&16);compare(s,word(get(0)),0,true);if(!get(0))break;
            load<4>(0,off,precision?0x107790e4:0x10778fcc);put(0,get(0)*10);put(3,get(29)+get(0));
            const auto address=get(30)+1;put(29,read<1>(s,address,precision?0x107790f0:0x10778fd8));put(30,address);put(0,get(3)-'0');store<4>(off,0,precision?0x107790f8:0x10778fe0);put(29,byte(get(29)));
        }
    }
    void modifiers();
    bool conversion();
    void restore(){load<4>(0,152,0x10779608);put(1,get(1)+144);words(s,27,0x10779610,true);s.wr<4>(4128,get(0));finish(s);}
public:
    explicit FormatParser(State& state):s(state),caller(std::uint32_t(state.r<4>(4128))){}
    void parse(){
        initialize();if(get(29)=='%'){percent_record();restore();return;}
        put(3,get(2)-0x34f0);store<1>(112,7,0x10778e8c);put(0,get(2)-0x34ec);store<4>(76,3,0x10778e94);store<4>(80,0,0x10778e98);store<4>(84,6,0x10778e9c);
        for(;;){
            put(0,0);store<4>(88,0,0x10778ea4);flags();if(escaped)return;field(false);load<4>(7,64,0x10778ff8);compare(s,word(get(7)),509);
            if(word(get(7))>509){copy_width_failure();restore();return;}
            compare(s,word(get(29)),'.');if(get(29)=='.'){next(0x10779098);put(0,1);store<1>(58,0,0x107790a0);field(true);}
            modifiers();load<1>(0,112,0x10779288);compare(s,get(0),1);bool invalid_packed=get(0)==1;
            if(invalid_packed){load<1>(0,60,0x10779294);compare(s,get(0),6);invalid_packed=get(0)!=6;}
            if(invalid_packed){copy_packed_failure();restore();return;}
            if(conversion()){if(escaped)return;copy_record();restore();return;}
        }
    }
};
void FormatParser::modifiers(){
    put(0,1);compare(s,word(get(29)),'v');store<4>(104,0,0x10779118);
    if(get(29)=='v'){next(0x10779120);put(4,2);put(3,6);store<1>(112,4,0x1077912c);store<1>(60,3,0x10779134);}
    compare(s,word(get(29)),'h');
    if(get(29)=='h'){
        load<1>(3,60,0x1077915c);compare(s,get(3));if(!get(3)){put(3,2);store<1>(60,3,0x1077916c);}
        put(5,read<1>(s,get(30)+1,0x10779170));put(3,8);put(4,1);store<4>(72,3,0x1077917c);put(5,byte(get(5)));put(3,0);store<1>(113,4,0x10779188);compare(s,word(get(5)),'h');store<4>(104,3,0x10779190);
        if(get(5)=='h'){store<1>(60,4,0x10779198);put(29,get(5));put(30,get(30)+1);}
        else{compare(s,word(get(5)),'v');if(get(5)=='v'){load<1>(3,112,0x107791b0);compare(s,get(3),2);if(get(3)!=2){put(4,2);put(3,6);store<1>(112,4,0x107791c4);put(29,get(5));store<1>(60,3,0x107791cc);put(30,get(30)+1);}}}
    }else{
        if(word(get(29))>='h')compare(s,word(get(29)),'l');else compare(s,word(get(29)),'L');
        if(get(29)=='l'){
            load<1>(3,60,0x107791d8);compare(s,get(3));if(!get(3)){put(3,3);store<1>(60,3,0x107791e8);}
            put(3,read<1>(s,get(30)+1,0x107791ec));put(4,4);put(5,2);store<4>(72,4,0x107791f8);put(4,byte(get(3)));put(3,0);store<1>(113,5,0x10779204);compare(s,word(get(4)),'v');store<4>(104,3,0x1077920c);
            if(get(4)=='v'){load<1>(3,112,0x10779214);compare(s,get(3),2);if(get(3)!=2){put(3,6);store<1>(112,5,0x10779224);put(29,get(4));store<1>(60,3,0x1077922c);put(30,get(30)+1);}}
            put(3,read<1>(s,get(30)+1,0x10779234));put(5,byte(get(3)));compare(s,word(get(5)),'l');
            if(get(5)=='l'){put(4,4);put(3,2);store<1>(60,4,0x1077924c);put(29,get(5));store<1>(112,3,0x10779254);put(30,get(30)+1);}
        }else if(get(29)=='L'){put(4,5);put(3,2);store<1>(60,4,0x10779268);store<1>(112,3,0x1077926c);}
        else put(0,0);
    }
    compare(s,word(get(0)));if(get(0))next(0x10779280);
}
bool FormatParser::conversion(){
    put(0,get(29)-'A');store<1>(61,29,0x10779334);compare(s,get(0),55);
    std::uint32_t target=0x1077954c;
    if(get(0)<=55){put(3,read<4>(s,get(2)-0x3b4c,0x10779340));put(0,get(0)*4);put(3,read<4>(s,get(3)+get(0),0x10779348));s.wr<4>(4132,get(3));target=get(3);}
    switch(target){
    case 0x10779354:{
        load<1>(0,60,0x10779354);compare(s,get(0),6);bool accepted=get(0)==6;
        if(!accepted){compare(s,get(0),5);accepted=get(0)!=5;}
        if(!accepted){invalid(0x1077936c);break;}
        load<1>(0,58,0x10779374);compare(s,get(0));
        if(!get(0)){put(0,1);store<4>(68,0,0x10779384);}
        else{load<1>(0,56,0x1077938c);compare(s,get(0),2);if(get(0)==2){put(0,1);store<1>(56,0,0x1077939c);}}break;
    }
    case 0x107793a4:{
        load<1>(0,60,0x107793a4);compare(s,get(0),6);bool invalid_length=false;
        if(get(0)!=6){compare(s,get(0),2);invalid_length=get(0)==2;if(!invalid_length){compare(s,get(0),4);invalid_length=get(0)==4;}}
        if(invalid_length){invalid(0x107793c4);break;}
        load<1>(0,58,0x107793cc);compare(s,get(0));if(!get(0)){put(0,6);store<4>(68,0,0x107793dc);}break;
    }
    case 0x107793e4:{
        load<1>(0,58,0x107793e4);compare(s,get(0));if(!get(0)){put(0,13);store<4>(68,0,0x107793f4);}
        load<1>(0,60,0x107793f8);compare(s,get(0),2);bool invalid_length=get(0)==2;
        if(!invalid_length){compare(s,get(0),4);invalid_length=get(0)==4;if(!invalid_length){compare(s,get(0),1);invalid_length=get(0)==1;}}
        if(invalid_length)invalid(0x10779418);break;
    }
    case 0x10779420:case 0x10779434:{
        if(target==0x10779420){load<4>(0,68,0x10779420);compare(s,word(get(0)));if(!get(0)){put(0,1);store<4>(68,0,0x10779430);}}
        load<1>(0,60,0x10779434);compare(s,get(0),6);bool invalid_length=false;
        if(get(0)!=6){compare(s,get(0),2);invalid_length=get(0)==2;if(!invalid_length){compare(s,get(0),4);invalid_length=get(0)==4;if(!invalid_length){compare(s,get(0),1);invalid_length=get(0)==1;}}}
        if(invalid_length){invalid(0x1077945c);break;}load<1>(0,58,0x10779464);compare(s,get(0));if(!get(0)){put(0,6);store<4>(68,0,0x10779474);}break;
    }
    case 0x1077947c:
        load<1>(0,60,0x1077947c);put(4,'x');put(3,1);store<1>(61,4,0x10779488);compare(s,get(0),6);store<1>(59,3,0x10779490);
        if(get(0)!=6){put(3,3);put(0,8);store<1>(60,3,0x107794a0);store<4>(68,0,0x107794a4);}else store<4>(68,3,0x107794ac);break;
    case 0x107794b4:{
        load<1>(3,60,0x107794b4);compare(s,get(3),6);if(get(3)==6)break;compare(s,get(3),3);
        if(get(3)==3){put(0,7);store<1>(60,0,0x107794cc);break;}
        load<1>(0,58,0x107794d4);compare(s,get(0));bool accepted=false;if(!get(0)){compare(s,get(3));accepted=get(3)==0;}if(!accepted)invalid(0x107794ec);break;
    }
    case 0x107794f4:
        load<1>(0,60,0x107794f4);compare(s,get(0),6);if(get(0)==6){put(0,0);store<1>(60,0,0x10779504);break;}
        compare(s,get(0),3);if(get(0)==3){put(0,7);store<1>(60,0,0x10779518);break;}
        compare(s,get(0));if(get(0))invalid(0x1077952c);break;
    case 0x10779534:
        load<1>(0,60,0x10779534);compare(s,get(0),5);if(get(0)==5)invalid(0x10779544);break;
    case 0x1077954c:
        load<1>(0,112,0x1077954c);compare(s,get(0));if(get(0)){invalid(0x10779580);break;}
        put(0,1);put(29,read<1>(s,get(30)+1,0x1077955c));store<1>(112,0,0x10779560);put(29,byte(get(29)));store<4>(80,30,0x10779568);store<4>(76,30,0x1077956c);put(30,get(30)+1);store<4>(84,0,0x10779574);return false;
    default:escape(target);break;
    }return true;
}
}
void parse_format(State& s){FormatParser(s).parse();}
}

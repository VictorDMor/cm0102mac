#include "formatted_output_internal.hpp"
namespace cm::aot::formatted_output {
namespace {
void decrementing_store(State& s,unsigned cursor,unsigned value,std::uint32_t pc,unsigned decrement=1){const auto address=reg(s,cursor)-decrement;write<1>(s,address,reg(s,value),pc);set(s,cursor,address);}
void signed_half_shift(State& s,unsigned destination,unsigned source){const auto value=reg(s,source);s.wr<1>(1027,word(value)<0&&(value&1));set(s,destination,shift_signed<4>(value,1));}
}
// Despite the initial handoff label, this leaf is a word32 integer formatter.
// Its live destination table selects signed decimal, octal, unsigned decimal
// or hexadecimal. The by-value Mac record arrives in r5..r10 stack slots.
void format_word32(State& s){
    const auto caller=std::uint32_t(s.r<4>(4128));
    write<4>(s,reg(s,1)-4,reg(s,31),0x10779620);compare(s,word(reg(s,3)));const bool zero=reg(s,3)==0;set(s,31,0);write<4>(s,reg(s,1)-8,reg(s,30),0x1077962c);set(s,11,reg(s,4)-1);set(s,12,0);write<4>(s,reg(s,1)-12,reg(s,29),0x10779638);
    for(unsigned n=5;n<=8;++n)write<4>(s,reg(s,1)+12+n*4,reg(s,n),0x1077963c+(n-5)*4);set(s,8,0);
    write<4>(s,reg(s,1)+48,reg(s,9),0x10779650);write<4>(s,reg(s,1)+52,reg(s,10),0x10779654);
    set(s,29,read<4>(s,reg(s,1)+44,0x10779658));set(s,6,read<1>(s,reg(s,1)+35,0x1077965c));set(s,5,read<1>(s,reg(s,1)+37,0x10779660));set(s,30,read<4>(s,reg(s,1)+40,0x10779664));set(s,0,read<1>(s,reg(s,1)+33,0x10779668));write<1>(s,reg(s,4)-1,reg(s,31),0x1077966c);
    bool omitted=false;
    if(zero){compare(s,word(reg(s,29)));if(!reg(s,29)){compare(s,reg(s,6));omitted=!reg(s,6);if(!omitted){compare(s,reg(s,5),'o');omitted=reg(s,5)!='o';}}}
    if(omitted)set(s,3,reg(s,11));
    else{
        set(s,10,reg(s,5)-'X');compare(s,reg(s,10),32);
        std::uint32_t destination=0x107796ec;
        if(reg(s,10)<=32){set(s,9,read<4>(s,reg(s,2)-0x3b50,0x107796a0));set(s,10,reg(s,10)*4);set(s,9,read<4>(s,reg(s,9)+reg(s,10),0x107796a8));s.wr<4>(4132,reg(s,9));destination=reg(s,9);}
        switch(destination){
        case 0x107796b4:compare(s,word(reg(s,3)));set(s,7,10);if(word(reg(s,3))<0){set(s,3,0-reg(s,3));set(s,12,1);}break;
        case 0x107796cc:set(s,7,8);set(s,0,0);break;
        case 0x107796d8:set(s,7,10);set(s,0,0);break;
        case 0x107796e4:set(s,7,16);set(s,0,0);break;
        case 0x107796ec:break;
        default:++s.formatted_output_dispatch_escapes;s.pc=destination;run_until(s,caller);return;
        }
        for(;;){
            s.step(0x107796f0);set(s,9,divide_unsigned(reg(s,3),reg(s,7)));set(s,9,reg(s,9)*reg(s,7));set(s,9,reg(s,3)-reg(s,9));s.pc=0x107796fc;set(s,3,divide_unsigned(reg(s,3),reg(s,7)));compare(s,word(reg(s,9)),10);
            if(word(reg(s,9))<10)set(s,9,reg(s,9)+'0');else{compare(s,reg(s,5),'x');set(s,9,reg(s,9)+(reg(s,5)=='x'?87:55));}
            compare(s,reg(s,3));write<1>(s,reg(s,11)-1,reg(s,9),0x10779728);set(s,11,reg(s,11)-1);set(s,8,reg(s,8)+1);if(!reg(s,3))break;
        }
        compare(s,reg(s,7),8);if(reg(s,7)==8){compare(s,reg(s,6));if(reg(s,6)){set(s,3,read<1>(s,reg(s,11),0x10779748));compare(s,word(reg(s,3)),'0');if(reg(s,3)!='0'){set(s,3,'0');decrementing_store(s,11,3,0x10779758);set(s,8,reg(s,8)+1);}}}
        set(s,3,read<1>(s,reg(s,1)+32,0x10779760));compare(s,reg(s,3),2);
        if(reg(s,3)==2){compare(s,word(reg(s,12)));set(s,29,reg(s,30));bool sign=reg(s,12)!=0;
            if(!sign){set(s,3,reg(s,0)&255);compare(s,word(reg(s,3)),0,true);sign=reg(s,3)!=0;}if(sign)set(s,29,reg(s,29)-1);
            compare(s,reg(s,7),16);if(reg(s,7)==16){compare(s,reg(s,6));if(reg(s,6))set(s,29,reg(s,29)-2);}
        }
        set(s,3,reg(s,4)-reg(s,11));set(s,3,reg(s,29)+reg(s,3));compare(s,word(reg(s,3)),509);
        if(word(reg(s,3))>509)set(s,3,0);
        else{
            compare(s,word(reg(s,8)),word(reg(s,29)));const bool padding=word(reg(s,8))<word(reg(s,29));set(s,4,reg(s,29)-reg(s,8));set(s,8,'0');
            if(padding){
                set(s,3,reg(s,4)>>3);compare(s,word(reg(s,3)),0,true);s.wr<4>(4132,reg(s,3));const bool groups=reg(s,3)!=0;
                if(groups){do{s.step(0x107797cc);for(unsigned i=1;i<=8;++i)write<1>(s,reg(s,11)-i,reg(s,8),0x107797cc+(i-1)*4);set(s,11,reg(s,11)-8);s.wr<4>(4132,s.r<4>(4132)-1);}while(s.r<4>(4132));set(s,4,reg(s,4)&7);compare(s,word(reg(s,4)),0,true);}
                if(!groups||reg(s,4)){s.wr<4>(4132,reg(s,4));do{s.step(0x10779800);decrementing_store(s,11,8,0x10779800);s.wr<4>(4132,s.r<4>(4132)-1);}while(s.r<4>(4132));}
            }
            compare(s,reg(s,7),16);if(reg(s,7)==16){compare(s,reg(s,6));if(reg(s,6)){write<1>(s,reg(s,11)-1,reg(s,5),0x10779818);set(s,3,'0');decrementing_store(s,11,3,0x10779820,2);}}
            compare(s,word(reg(s,12)));
            if(reg(s,12)){set(s,0,'-');decrementing_store(s,11,0,0x10779830);}
            else{set(s,0,reg(s,0)&255);compare(s,reg(s,0),1);if(reg(s,0)==1){set(s,0,'+');decrementing_store(s,11,0,0x10779848);}else{compare(s,reg(s,0),2);if(reg(s,0)==2){set(s,0,' ');decrementing_store(s,11,0,0x1077985c);}}}
            set(s,3,reg(s,11));
        }
    }
    set(s,31,read<4>(s,reg(s,1)-4,0x10779864));set(s,30,read<4>(s,reg(s,1)-8,0x10779868));set(s,29,read<4>(s,reg(s,1)-12,0x1077986c));finish(s);
}
// A wide magnitude is divided through the existing unsigned word64 helpers;
// the ABI-visible remainder, quotient and carry residues belong to that call.
void format_integer(State& s){
    const auto caller=std::uint32_t(s.r<4>(4128));words(s,18,0x10779880,false);set(s,0,caller);set(s,26,reg(s,5));set(s,19,reg(s,4));set(s,20,reg(s,3));set(s,29,reg(s,26)-1);set(s,27,0);set(s,28,0);
    write<4>(s,reg(s,1)+8,reg(s,0),0x107798a0);frame(s,112,0x107798a4);write<4>(s,reg(s,1)+148,reg(s,6),0x107798a8);set(s,6,0);set(s,5,reg(s,4)^reg(s,6));write<4>(s,reg(s,1)+152,reg(s,7),0x107798b4);set(s,0,reg(s,3)^reg(s,6));set(s,0,reg(s,5)|reg(s,0));compare(s,word(reg(s,0)),0,true);const bool zero=reg(s,0)==0;
    for(unsigned n=8;n<=10;++n)write<4>(s,reg(s,1)+156+(n-8)*4,reg(s,n),0x107798c0+(n-8)*4);
    set(s,21,read<4>(s,reg(s,1)+160,0x107798cc));set(s,22,read<1>(s,reg(s,1)+151,0x107798d0));set(s,23,read<1>(s,reg(s,1)+153,0x107798d4));set(s,24,read<4>(s,reg(s,1)+156,0x107798d8));set(s,25,read<1>(s,reg(s,1)+149,0x107798dc));write<1>(s,reg(s,26)-1,reg(s,6),0x107798e0);
    bool omitted=false;if(zero){compare(s,word(reg(s,21)));if(!reg(s,21)){compare(s,reg(s,22));omitted=!reg(s,22);if(!omitted){compare(s,reg(s,23),'o');omitted=reg(s,23)!='o';}}}
    if(omitted)set(s,3,reg(s,29));else{
        set(s,0,reg(s,23)-'X');compare(s,reg(s,0),32);std::uint32_t target=0x1077998c;
        if(reg(s,0)<=32){set(s,5,read<4>(s,reg(s,2)-0x3b54,0x10779914));set(s,0,reg(s,0)*4);set(s,5,read<4>(s,reg(s,5)+reg(s,0),0x1077991c));s.wr<4>(4132,reg(s,5));target=reg(s,5);}
        switch(target){
        case 0x10779928:{
            set(s,0,0);set(s,5,reg(s,3)^0x80000000);set(s,3,0x80000000);
            const auto low=reg(s,4);s.wr<1>(1027,1);set(s,0,low);const auto high=std::uint64_t(reg(s,5))+std::uint64_t(~reg(s,3))+1;
            set(s,3,std::uint32_t(high));s.wr<1>(1027,high>>32);const auto carry=s.r<1>(1027);set(s,3,carry?0:UINT32_MAX);s.wr<1>(1027,carry);set(s,3,0-reg(s,3));compare(s,word(reg(s,3)),0,true);set(s,30,10);set(s,31,0);
            if(reg(s,3)){s.wr<1>(1027,reg(s,19)==0);set(s,19,0-reg(s,19));const auto sum=std::uint64_t(~reg(s,20))+s.r<1>(1027);set(s,20,std::uint32_t(sum));s.wr<1>(1027,sum>>32);set(s,27,1);}break;
        }
        case 0x10779960:set(s,30,8);set(s,31,0);set(s,25,0);break;
        case 0x10779970:set(s,30,10);set(s,31,0);set(s,25,0);break;
        case 0x10779980:set(s,30,16);set(s,31,0);set(s,25,0);break;
        case 0x1077998c:break;
        default:++s.formatted_output_dispatch_escapes;s.pc=target;run_until(s,caller);return;
        }
        do{
            s.step(0x10779990);set(s,3,reg(s,20));set(s,4,reg(s,19));set(s,5,reg(s,31));set(s,6,reg(s,30));call(s,0x10773a90,0x107799a4);set(s,18,reg(s,4));
            set(s,3,reg(s,20));set(s,4,reg(s,19));set(s,5,reg(s,31));set(s,6,reg(s,30));call(s,0x10773870,0x107799bc);compare(s,word(reg(s,18)),10);set(s,19,reg(s,4));set(s,20,reg(s,3));
            if(word(reg(s,18))<10)set(s,0,reg(s,18)+'0');else{compare(s,reg(s,23),'x');set(s,0,reg(s,18)+(reg(s,23)=='x'?87:55));}
            set(s,4,0);decrementing_store(s,29,0,0x107799ec);set(s,3,reg(s,19));set(s,0,reg(s,20));set(s,0,reg(s,3)|reg(s,0));compare(s,word(reg(s,0)),0,true);set(s,28,reg(s,28)+1);
        }while(reg(s,0));
        set(s,0,8);set(s,3,reg(s,30)^reg(s,0));set(s,0,reg(s,31)^reg(s,4));set(s,0,reg(s,3)|reg(s,0));compare(s,word(reg(s,0)),0,true);
        if(!reg(s,0)){compare(s,reg(s,22));if(reg(s,22)){set(s,0,read<1>(s,reg(s,29),0x10779a20));compare(s,word(reg(s,0)),'0');if(reg(s,0)!='0'){set(s,0,'0');decrementing_store(s,29,0,0x10779a30);set(s,28,reg(s,28)+1);}}}
        set(s,0,read<1>(s,reg(s,1)+148,0x10779a38));compare(s,reg(s,0),2);
        if(reg(s,0)==2){compare(s,word(reg(s,27)));set(s,21,reg(s,24));bool sign=reg(s,27)!=0;if(!sign){set(s,0,reg(s,25)&255);compare(s,word(reg(s,0)),0,true);sign=reg(s,0)!=0;}if(sign)set(s,21,reg(s,21)-1);
            set(s,3,16);set(s,0,0);set(s,3,reg(s,30)^reg(s,3));set(s,0,reg(s,31)^reg(s,0));set(s,0,reg(s,3)|reg(s,0));compare(s,word(reg(s,0)),0,true);if(!reg(s,0)){compare(s,reg(s,22));if(reg(s,22))set(s,21,reg(s,21)-2);}}
        set(s,0,reg(s,26)-reg(s,29));set(s,0,reg(s,21)+reg(s,0));compare(s,word(reg(s,0)),509);
        if(word(reg(s,0))>509)set(s,3,0);else{
            compare(s,word(reg(s,28)),word(reg(s,21)));bool padding=word(reg(s,28))<word(reg(s,21));set(s,3,reg(s,21)-reg(s,28));set(s,4,'0');
            if(padding){set(s,0,reg(s,3)>>3);compare(s,word(reg(s,0)),0,true);s.wr<4>(4132,reg(s,0));const bool groups=reg(s,0)!=0;
                if(groups){do{s.step(0x10779ab4);for(unsigned i=1;i<=8;++i)write<1>(s,reg(s,29)-i,reg(s,4),0x10779ab4+(i-1)*4);set(s,29,reg(s,29)-8);s.wr<4>(4132,s.r<4>(4132)-1);}while(s.r<4>(4132));set(s,3,reg(s,3)&7);compare(s,word(reg(s,3)),0,true);}
                if(!groups||reg(s,3)){s.wr<4>(4132,reg(s,3));do{s.step(0x10779ae8);decrementing_store(s,29,4,0x10779ae8);s.wr<4>(4132,s.r<4>(4132)-1);}while(s.r<4>(4132));}}
            set(s,3,16);set(s,0,0);set(s,3,reg(s,30)^reg(s,3));set(s,0,reg(s,31)^reg(s,0));set(s,0,reg(s,3)|reg(s,0));compare(s,word(reg(s,0)),0,true);
            if(!reg(s,0)){compare(s,reg(s,22));if(reg(s,22)){write<1>(s,reg(s,29)-1,reg(s,23),0x10779b10);set(s,0,'0');decrementing_store(s,29,0,0x10779b18,2);}}
            compare(s,word(reg(s,27)));if(reg(s,27)){set(s,0,'-');decrementing_store(s,29,0,0x10779b28);}else{set(s,0,reg(s,25)&255);compare(s,reg(s,0),1);if(reg(s,0)==1){set(s,0,'+');decrementing_store(s,29,0,0x10779b40);}else{compare(s,reg(s,0),2);if(reg(s,0)==2){set(s,0,' ');decrementing_store(s,29,0,0x10779b54);}}}set(s,3,reg(s,29));
        }
    }
    set(s,0,read<4>(s,reg(s,1)+120,0x10779b5c));set(s,1,reg(s,1)+112);words(s,18,0x10779b64,true);s.wr<4>(4128,reg(s,0));finish(s);
}
void reset_decimal_tail(State& s){
    ++s.formatted_output_shared_tail_calls;
    set(s,0,0);write<2>(s,reg(s,3)+2,reg(s,0),0x10779edc);set(s,4,1);set(s,0,'0');write<1>(s,reg(s,3)+4,reg(s,4),0x10779ee8);write<1>(s,reg(s,3)+5,reg(s,0),0x10779eec);finish(s);
}
void reset_decimal_length(State& s){compare(s,word(reg(s,4)));if(!reg(s,4))reset_decimal_tail(s);else{write<1>(s,reg(s,3)+4,reg(s,4),0x10779ff8);finish(s);}}
void round_decimal(State& s){
    compare(s,word(reg(s,4)));if(word(reg(s,4))<0){reset_decimal_tail(s);return;}
    set(s,7,read<1>(s,reg(s,3)+4,0x10779ef4));compare(s,word(reg(s,4)),word(reg(s,7)));if(word(reg(s,4))>=word(reg(s,7))){finish(s);return;}
    set(s,6,reg(s,3)+reg(s,4));set(s,5,read<1>(s,reg(s,6)+5,0x10779f04));set(s,8,reg(s,6)+5);set(s,0,reg(s,5)-'0');set(s,6,byte(reg(s,0)));compare(s,word(reg(s,6)),5);
    if(reg(s,6)==5){
        set(s,5,reg(s,3)+reg(s,7));set(s,5,reg(s,5)+5);
        for(;;){s.step(0x10779f28);set(s,5,reg(s,5)-1);compare(s,reg(s,5),reg(s,8));if(reg(s,5)<=reg(s,8))break;set(s,0,read<1>(s,reg(s,5),0x10779f34));compare(s,word(reg(s,0)),'0');if(reg(s,0)!='0')break;}
        compare(s,reg(s,5),reg(s,8));if(reg(s,5)==reg(s,8)){set(s,0,read<1>(s,reg(s,8)-1,0x10779f48));set(s,5,reg(s,0)&1);}else set(s,5,1);
    }else{set(s,0,5);set(s,0,reg(s,6)^reg(s,0));signed_half_shift(s,5,0);set(s,0,reg(s,0)&reg(s,6));set(s,0,reg(s,5)-reg(s,0));set(s,5,std::rotl(reg(s,0),1)&1);}
    set(s,6,9);
    for(;;){
        compare(s,word(reg(s,4)));if(!reg(s,4))break;
        s.step(0x10779f78);const auto address=reg(s,8)-1;set(s,0,read<1>(s,address,0x10779f78));set(s,8,address);set(s,5,reg(s,0)+reg(s,5));set(s,0,reg(s,5)-'0');set(s,7,byte(reg(s,0)));
        set(s,0,reg(s,7)^reg(s,6));signed_half_shift(s,5,0);set(s,0,reg(s,0)&reg(s,7));set(s,0,reg(s,5)-reg(s,0));set(s,5,std::rotl(reg(s,0),1)&1);compare(s,word(reg(s,5)),0,true);
        bool shorten=reg(s,5)!=0;if(!shorten){set(s,0,byte(reg(s,7)));compare(s,word(reg(s,0)),0,true);shorten=reg(s,0)==0;}
        if(shorten)set(s,4,reg(s,4)-1);else{set(s,0,reg(s,7)+'0');write<1>(s,reg(s,8),reg(s,0),0x10779fb4);break;}
    }
    compare(s,word(reg(s,5)));if(!reg(s,5)){s.pc=0x10779ff0;run_until(s,std::uint32_t(s.r<4>(4128)));return;}
    set(s,5,std::uint32_t(signed_value<2>(read<2>(s,reg(s,3)+2,0x10779fd0))));set(s,4,1);set(s,0,'1');set(s,5,reg(s,5)+1);write<2>(s,reg(s,3)+2,reg(s,5),0x10779fe0);write<1>(s,reg(s,3)+4,reg(s,4),0x10779fe4);write<1>(s,reg(s,3)+5,reg(s,0),0x10779fe8);finish(s);
}
}

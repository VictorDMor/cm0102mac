#include "formatted_output_internal.hpp"
namespace cm::aot::formatted_output {
void stream_callback(State& s){
    write<4>(s,reg(s,1)-4,reg(s,31),0x1077b4c0);set(s,0,s.r<4>(4128));set(s,31,reg(s,5));
    write<4>(s,reg(s,1)-8,reg(s,30),0x1077b4cc);set(s,30,reg(s,3));set(s,3,reg(s,4));write<4>(s,reg(s,1)+8,reg(s,0),0x1077b4d8);set(s,6,reg(s,30));set(s,4,1);frame(s,64,0x1077b4e4);
    call(s,0x10776360,0x1077b4ec);compare(s,reg(s,31),reg(s,3));set(s,3,reg(s,31)==reg(s,3)?reg(s,30):0);
    set(s,0,read<4>(s,reg(s,1)+72,0x1077b500));set(s,1,reg(s,1)+64);set(s,31,read<4>(s,reg(s,1)-4,0x1077b508));s.wr<4>(4128,reg(s,0));set(s,30,read<4>(s,reg(s,1)-8,0x1077b510));finish(s);
}
void bounded_callback(State& s){
    write<4>(s,reg(s,1)-4,reg(s,31),0x1077b520);set(s,0,s.r<4>(4128));write<4>(s,reg(s,1)-8,reg(s,30),0x1077b528);set(s,30,reg(s,3));write<4>(s,reg(s,1)+8,reg(s,0),0x1077b530);frame(s,64,0x1077b534);
    set(s,3,read<4>(s,reg(s,3)+8,0x1077b538));set(s,6,read<4>(s,reg(s,30)+4,0x1077b53c));set(s,0,reg(s,3)+reg(s,5));compare(s,reg(s,0),reg(s,6));set(s,31,reg(s,6)-reg(s,3));if(reg(s,0)<=reg(s,6))set(s,31,reg(s,5));
    set(s,0,read<4>(s,reg(s,30),0x1077b554));set(s,5,reg(s,31));set(s,3,reg(s,0)+reg(s,3));call(s,0x10739ef0,0x1077b564);
    set(s,0,read<4>(s,reg(s,30)+8,0x1077b564));set(s,3,1);set(s,0,reg(s,0)+reg(s,31));write<4>(s,reg(s,30)+8,reg(s,0),0x1077b570);
    set(s,0,read<4>(s,reg(s,1)+72,0x1077b574));set(s,1,reg(s,1)+64);s.wr<4>(4128,reg(s,0));set(s,31,read<4>(s,reg(s,1)-4,0x1077b580));set(s,30,read<4>(s,reg(s,1)-8,0x1077b584));finish(s);
}
void standard_stream_facade(State& s){
    write<4>(s,reg(s,1)-4,reg(s,31),0x1077b590);set(s,0,s.r<4>(4128));write<4>(s,reg(s,1)+8,reg(s,0),0x1077b598);frame(s,64,0x1077b59c);write<4>(s,reg(s,1)+88,reg(s,3),0x1077b5a0);
    set(s,3,read<4>(s,reg(s,2)-0x5a68,0x1077b5a4));write<4>(s,reg(s,1)+92,reg(s,4),0x1077b5a8);set(s,4,UINT32_MAX);set(s,31,reg(s,3)+84);write<4>(s,reg(s,1)+96,reg(s,5),0x1077b5b4);set(s,3,reg(s,31));
    for(unsigned n=6;n<=10;++n)write<4>(s,reg(s,1)+76+n*4,reg(s,n),0x1077b5bc+(n-6)*4);
    call(s,0x1077fa50,0x1077b5d4);compare(s,word(reg(s,3)));
    if(word(reg(s,3))>=0)set(s,3,UINT32_MAX);
    else{set(s,3,read<4>(s,reg(s,2)-0x5a4c,0x1077b5e4));set(s,4,reg(s,31));set(s,5,read<4>(s,reg(s,1)+88,0x1077b5ec));set(s,6,reg(s,1)+92);call(s,0x1077a690,0x1077b5f8);}
    set(s,0,read<4>(s,reg(s,1)+72,0x1077b5f8));set(s,1,reg(s,1)+64);set(s,31,read<4>(s,reg(s,1)-4,0x1077b600));s.wr<4>(4128,reg(s,0));finish(s);
}
void explicit_stream_facade(State& s){
    write<4>(s,reg(s,1)-4,reg(s,31),0x1077b610);set(s,0,s.r<4>(4128));set(s,31,reg(s,3));write<4>(s,reg(s,1)+8,reg(s,0),0x1077b61c);frame(s,64,0x1077b620);
    write<4>(s,reg(s,1)+92,reg(s,4),0x1077b624);set(s,4,UINT32_MAX);
    for(unsigned n=5;n<=10;++n)write<4>(s,reg(s,1)+76+n*4,reg(s,n),0x1077b62c+(n-5)*4);
    call(s,0x1077fa50,0x1077b648);compare(s,word(reg(s,3)));
    if(word(reg(s,3))>=0)set(s,3,UINT32_MAX);
    else{set(s,3,read<4>(s,reg(s,2)-0x5a4c,0x1077b658));set(s,4,reg(s,31));set(s,5,read<4>(s,reg(s,1)+92,0x1077b660));set(s,6,reg(s,1)+96);call(s,0x1077a690,0x1077b66c);}
    set(s,0,read<4>(s,reg(s,1)+72,0x1077b66c));set(s,1,reg(s,1)+64);set(s,31,read<4>(s,reg(s,1)-4,0x1077b674));s.wr<4>(4128,reg(s,0));finish(s);
}
void bounded_vector_facade(State& s){
    write<4>(s,reg(s,1)-4,reg(s,31),0x1077b680);set(s,0,s.r<4>(4128));set(s,31,reg(s,4));write<4>(s,reg(s,1)-8,reg(s,30),0x1077b68c);set(s,30,reg(s,3));set(s,3,read<4>(s,reg(s,2)-0x5a50,0x1077b694));write<4>(s,reg(s,1)+8,reg(s,0),0x1077b698);set(s,0,0);frame(s,80,0x1077b6a0);
    write<4>(s,reg(s,1)+56,reg(s,30),0x1077b6a4);set(s,4,reg(s,1)+56);write<4>(s,reg(s,1)+60,reg(s,31),0x1077b6ac);write<4>(s,reg(s,1)+64,reg(s,0),0x1077b6b0);call(s,0x1077a690,0x1077b6b8);
    compare(s,reg(s,30));if(reg(s,30)){compare(s,reg(s,3),reg(s,31));set(s,4,reg(s,31)-1);if(reg(s,3)<reg(s,31))set(s,4,reg(s,3));set(s,0,0);write<1>(s,reg(s,30)+reg(s,4),reg(s,0),0x1077b6d4);}
    set(s,0,read<4>(s,reg(s,1)+88,0x1077b6d8));set(s,1,reg(s,1)+80);set(s,31,read<4>(s,reg(s,1)-4,0x1077b6e0));s.wr<4>(4128,reg(s,0));set(s,30,read<4>(s,reg(s,1)-8,0x1077b6e8));finish(s);
}
void unbounded_vector_facade(State& s){
    set(s,0,s.r<4>(4128));set(s,6,reg(s,5));write<4>(s,reg(s,1)+8,reg(s,0),0x1077b6f8);set(s,5,reg(s,4));set(s,4,UINT32_MAX);frame(s,64,0x1077b704);call(s,0x1077b680,0x1077b70c);
    set(s,0,read<4>(s,reg(s,1)+72,0x1077b70c));set(s,1,reg(s,1)+64);s.wr<4>(4128,reg(s,0));finish(s);
}
void buffer_variadic_facade(State& s){
    set(s,0,s.r<4>(4128));write<4>(s,reg(s,1)+8,reg(s,0),0x1077b724);frame(s,64,0x1077b728);write<4>(s,reg(s,1)+92,reg(s,4),0x1077b72c);set(s,4,UINT32_MAX);write<4>(s,reg(s,1)+96,reg(s,5),0x1077b734);write<4>(s,reg(s,1)+100,reg(s,6),0x1077b738);set(s,6,reg(s,1)+96);
    for(unsigned n=7;n<=10;++n)write<4>(s,reg(s,1)+76+n*4,reg(s,n),0x1077b740+(n-7)*4);
    set(s,5,read<4>(s,reg(s,1)+92,0x1077b750));call(s,0x1077b680,0x1077b758);set(s,0,read<4>(s,reg(s,1)+72,0x1077b758));set(s,1,reg(s,1)+64);s.wr<4>(4128,reg(s,0));finish(s);
}
}

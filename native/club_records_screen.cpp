#include "club_records_detail.hpp"
#include <array>

namespace cm::aot::club_records {
using namespace detail;
std::uint32_t configure_screen(Cpu& c,std::uint32_t club,std::uint32_t mode,std::uint32_t preserve,
                               std::uint32_t alternate,std::uint32_t include_unavailable) {
    Frame frame(c,0x1001d410,0x170,{.first_gpr=21});Program p{c};
    keep(c,{{29,c.toc(-0x7b80)},{31,club},{30,c.toc(-0x7bb0)},
            {25,mode},{24,preserve},{22,alternate},{21,include_unavailable}});
    p.call(0x10064030,0x1001d444,{frame.at(0x38)});
    if(compare_logical(c,club)!=0 && !nonzero_byte(c,p.call(0x100d12d0,0x1001d454,{club})))c.set(25,0);
    if(compare(c,std::int8_t(preserve))!=0) {
        Kept<23> index(c,0);
        while(compare(c,std::int8_t(std::uint32_t(index)),60)<0) {
            const auto saved=p.call(0x1073ef70,0x1001d47c,{c.r(30),std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(index))))});
            c.set(4,frame.at(0x48));c.put32(frame.at(0x48+std::uint32_t(index)*4),saved);index=std::uint32_t(index)+1;
        }
    }
    bool controlled=false;
    if(compare_logical(c,club)!=0)controlled=nonzero_byte(c,p.call(0x1014c490,0x1001d4b4,{club,0,1}));
    if(controlled)c.set(23,p.call(0x10149c90,0x1001d4cc,{c.u32(c.toc(-0x7a54)),0}));
    else c.set(23,frame.at(0x38));
    if(!nonzero_byte(c,p.call(0x1073e4b0,0x1001d4f4,{c.r(30),c.toc(-0x57e8),c.toc(-0x57ec),0,c.toc(-0x57f0),0})))return c.r(3);
    const auto put=[&](unsigned index,std::uint32_t value,std::uint32_t back){p.call(0x1073f1e0,back,{c.r(30),index,value,0});};
    constexpr std::array<std::uint32_t,32> backs{
        0x1001d510,0x1001d524,0x1001d538,0x1001d54c,0,
        0x1001d594,0x1001d5a8,0x1001d5bc,0x1001d5d0,0x1001d5e8,
        0x1001d5fc,0x1001d610,0x1001d624,0x1001d638,0x1001d64c,0x1001d660,
        0x1001d674,0x1001d688,0x1001d69c,0x1001d6b0,0x1001d6c4,0x1001d6d8,
        0x1001d6ec,0x1001d700,0x1001d714,0x1001d72c,0x1001d744,0,
        0x1001d78c,0x1001d7a0,0x1001d7b4,0x1001d7c8};
    for(unsigned index=0;index<32;++index) {
        std::uint32_t value=0,back=backs[index];
        switch(index) {
        case 0:value=club;break;
        case 1:value=std::uint32_t(std::int32_t(std::int8_t(c.r(25))));break;
        case 4:
            value=compare(c,std::int8_t(alternate))==0?1:5;back=value==1?0x1001d580:0x1001d568;break;
        case 7:value=c.u32(c.r(23));break;
        case 8:value=c.u32(c.r(23)+4);break;
        case 9:value=std::uint32_t(std::int32_t(c.i8(c.r(23)+12)));break;
        case 12:case 23:value=c.u16(c.r(29)+2);break;
        case 15:case 18:value=1;break;
        case 24:value=c.u32(c.r(23)+8);break;
        case 25:value=std::uint32_t(std::int32_t(c.i8(c.r(23)+13)));break;
        case 26:value=std::uint32_t(std::int32_t(c.i8(c.r(23)+14)));break;
        case 27:
            value=compare(c,std::int8_t(alternate))==0?1:2;back=value==1?0x1001d778:0x1001d760;break;
        }
        put(index,value,back);
    }
    if(compare(c,std::int8_t(preserve))!=0) {
        constexpr std::array<unsigned,11> fields{4,7,8,9,16,17,18,24,25,26,27};
        for(unsigned i=0;i<fields.size();++i)put(fields[i],c.u32(frame.at(0x48+fields[i]*4)),0x1001d7e4+i*20);
    }
    if(compare(c,std::int8_t(include_unavailable))!=0)put(7,c.u32(c.r(23))|0x800,0x1001d8cc);
    const auto nonnull=compare_logical(c,club)!=0;
    Kept<22> count(c,0);Kept<29> best_score(c,std::uint32_t(-10000));
    Kept<28> second_score(c,std::uint32_t(-10000));Kept<27> third_score(c,std::uint32_t(-10000));
    Kept<26> best(c,0);Kept<25> second(c,0);Kept<24> third(c,0);
    if(nonnull) {
        Kept<23> index(c,0);
        do {
            c.set(3,club+std::uint32_t(index)*4);c.set(21,c.u32(c.r(3)+0x98));const auto player=c.r(21);
            if(compare_logical(c,player)!=0 && compare_logical(c,c.u32(player+20))!=0) {
                const auto national=nonzero_byte(c,p.call(0x100c7d00,0x1001d924,{club}));
                c.set(3,c.u32(player+20));c.set(4,std::uint32_t(std::int32_t(c.i16(c.r(3)+(national?12:10)))));
                compare(c,std::int8_t(std::uint32_t(count)),3);
                const auto score=c.r(4)+std::uint32_t(std::int32_t(std::int16_t(c.i16(c.r(3)+4)*10)));
                c.set(4,score);if(std::int8_t(std::uint32_t(count))<3)count=std::uint32_t(count)+1;
                c.set(3,std::uint32_t(std::int32_t(std::int16_t(score))));
                if(compare(c,c.r(3),std::int16_t(std::uint32_t(best_score)))>0) {
                    third_score=second_score;third=second;second_score=best_score;second=best;best_score=score;best=player;
                }else if(compare(c,c.r(3),std::int16_t(std::uint32_t(second_score)))>0) {
                    third_score=second_score;third=second;second_score=score;second=player;
                }else if(compare(c,c.r(3),std::int16_t(std::uint32_t(third_score)))>0) {third_score=score;third=player;}
            }
            index=std::uint32_t(index)+1;
        }while(compare(c,std::int8_t(std::uint32_t(index)),50)<0);
    }
    const bool first=compare(c,std::int8_t(std::uint32_t(count)))>0;
    put(32,first?c.u32(best):missing,first?0x1001d9fc:0x1001da14);
    const bool next=compare(c,std::int8_t(std::uint32_t(count)),1)>0;
    put(33,next?c.u32(second):missing,next?0x1001da34:0x1001da4c);
    const bool last=compare(c,std::int8_t(std::uint32_t(count)),2)>0;
    put(34,last?c.u32(third):missing,last?0x1001da6c:0x1001da84);
    return c.r(3);
}
}

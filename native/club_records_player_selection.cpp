#include "club_records_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::club_records {
using namespace detail;

std::uint32_t qualifies_transfer(Cpu& c,std::uint32_t manager,std::uint32_t offer,std::uint32_t strict) {
    Frame frame(c,0x10012e90,0x60,{.first_gpr=29});Program p{c};Fpu fp(c);
    keep(c,{{30,strict},{29,offer}});
    if(compare(c,offer)==0){c.set(3,0);return 0;}
    c.set(4,c.u32(c.toc(slot::clubs)));Kept<31> club(c,c.r(4)+c.u32(offer+8)*0x248);
    const auto modern=p.call(0x1001b830,0x10012edc,{manager,club,1});
    if(compare_logical(c,modern)==0 || compare_logical(c,club)==0){c.set(3,0);return 0;}
    const auto reputation=c.i16(club+0x1ec);c.set(5,std::uint32_t(reputation));c.set(6,c.u32(offer+0x20));
    c.set(4,std::uint32_t(reputation*50)+0x80000);
    if(compare(c,c.r(6),c.r(4)-0x5ee0)<0){c.set(3,0);return 0;}
    if(compare(c,std::int8_t(strict))!=0) {
        const auto shifted=std::uint32_t(reputation)^0x80000000;c.set(4,shifted);c.set(5,c.toc(slot::doubles));
        c.put32(frame.at(0x4c),shifted);const auto bias=c.f64(c.r(5)+0x38);
        c.put32(frame.at(0x48),0x43300000);const auto factor=c.f64(c.r(5)+0x50);
        c.set_f(1,fp.fsub(c.f64(frame.at(0x48)),bias));
        c.put32(frame.at(0x44),shifted);c.put32(frame.at(0x40),0x43300000);
        const auto converted=fp.fsub(c.f64(frame.at(0x40)),bias);
        const auto squared=fp.fmul(c.f(1),converted);const auto threshold=fp.fmul(factor,squared);
        c.put64(frame.at(0x38),fp.fctiwz(0,threshold));
        if(compare(c,c.r(6),c.i32(frame.at(0x3c)))<0){c.set(3,0);return 0;}
    }
    const auto amount=c.r(6);
    const auto result=compare(c,amount)!=0 && compare(c,amount,c.i32(modern+0x378))>0;
    c.set(3,result);return result;
}

std::uint32_t calculate_average(Cpu& c,std::uint32_t manager,std::uint32_t club) {
    Frame frame(c,0x1001bb50,0xa0,{.first_gpr=19});Program p{c};Fpu fp(c);
    keep(c,{{21,club},{27,c.toc(slot::floats)},{28,c.toc(slot::stats)},{29,c.toc(slot::doubles)},
            {30,c.toc(slot::staff_count)},{23,c.toc(slot::message)},{22,c.toc(slot::pool)}});
    if(compare(c,club)==0){assertion(c,frame,0x1001bb94,0x40,0x38,0x16f2,true);c.set(3,0);return 0;}
    Kept<25> index(c,c.u32(manager+28)+c.u32(club)*index_size);c.set(5,c.u32(manager+28));
    if(compare(c,c.i8(index+16))==0){c.set(3,0);return 0;}
    Kept<31> season(c,p.call(0x1001b9c0,0x1001bc08,{manager,club,0}));
    if(compare(c,season)==0){c.set(3,0);return 0;}
    c.put_f32(season+0x23c,c.f32(c.r(27)+4));
    Kept<24> changed(c,0);Kept<23> player_index(c,0);Kept<26> offset(c,0);
    do {
        Kept<22> player(c,c.u32(club+0x98+std::uint32_t(offset)));
        if(compare_logical(c,player)!=0) {
            Kept<20> stats(c,c.u32(player+0x14));
            if(compare_logical(c,stats)!=0 &&
               (compare_logical(c,player)==0 || (c.set(3,c.u32(player)),compare(c,c.r(3),c.i32(c.r(30)))<0))) {
                if(compare_logical(c,c.u32(player+0x24),club)==0 ||
                   compare_logical(c,c.u32(player+0x24),p.call(0x100ce320,0x1001bc80,{club,0,1}))==0) {
                    stats=p.call(0x103054e0,0x1001bc9c,{c.u32(c.r(28)),stats,6});
                    if(compare(c,stats)!=0) {
                        c.set_f(1,c.f32(c.r(27)));
                        p.call(0x10303980,0x1001bcb8,{c.u32(c.r(28)),stats,1});
                        const auto appearances=fp.fctiwz(0,c.f(1));
                        const auto matches=c.i8(index+16);c.set(3,std::uint32_t(matches));c.put64(frame.at(0x60),appearances);
                        Kept<19> played(c,c.u32(frame.at(0x64)));c.set(3,std::uint32_t(played)&0xff);
                        // srawi after adding the sign bit divides signed bytes toward zero.
                        if(compare(c,c.r(3),matches/2)>=0) {
                            c.set_f(1,c.f32(c.r(27)));p.call(0x10303980,0x1001bcf8,{c.u32(c.r(28)),stats,17});
                            const auto previous=double(c.f32(season+0x23c));
                            fp.fcmp(0,c.f64(c.r(29)+0x48),previous);
                            bool improve=(c.cr(0)&2)!=0;
                            if(!improve){fp.fcmp(0,c.f(1),previous);improve=(c.cr(0)&4)!=0;}
                            if(improve) {
                                c.put_f32(season+0x23c,float(c.f(1)));c.put8(season+0x258,played);
                                p.call(0x1030d6e0,0x1001bd24,{player,season+0x240});changed=1;c.put16(season+0x25a,0);
                            }
                        }
                    }
                }
            }
        }
        player_index=std::uint32_t(player_index)+1;offset=std::uint32_t(offset)+4;
    }while(compare(c,player_index,50)<0);
    c.set(3,changed);return changed;
}

std::uint32_t select_best_player(Cpu& c,std::uint32_t manager,std::uint32_t club,std::uint32_t record) {
    Frame frame(c,0x1001bec0,0xb0,{.first_gpr=20,.gpr_offset=-0x50,.first_fpr=29});Program p{c};Fpu fp(c);
    keep(c,{{20,manager},{29,c.toc(slot::doubles)},{21,club},{28,c.toc(slot::stats)},
            {22,record},{30,c.toc(slot::staff_count)},{24,0}});
    c.set_f(31,c.f64(c.r(29)+0x58));c.set_f(30,c.f64(c.r(29)+0x60));
    auto matches=c.i8(c.u32(manager+28)+c.u32(club)*index_size+16);c.set(3,std::uint32_t(matches));
    if(compare(c,matches)==0 && compare(c,c.i16(club+0x1ec),3000)<0){c.set(3,0);return 0;}
    Kept<31> conversion_high(c,0x43300000);
    for(;;) {
        fp.fcmp(0,c.f(31),c.f64(c.r(29)+0x30));
        if(!(c.cr(0)&4) || compare_logical(c,c.r(24))!=0)break;
        Kept<26> player_index(c,0);Kept<27> offset(c,0);
        c.set(4,c.u32(manager+28));c.set(3,c.u32(club)*index_size);c.put32(frame.at(0x50),conversion_high);
        c.set_f(1,c.f64(c.r(29)+0x38));matches=c.i8(c.r(4)+c.r(3)+16);
        c.put32(frame.at(0x54),std::uint32_t(matches)^0x80000000);
        const auto converted=fp.fsub(c.f64(frame.at(0x50)),c.f(1));
        c.put64(frame.at(0x48),fp.fctiwz(0,fp.fmul(converted,c.f(31))));Kept<23> minimum(c,c.u32(frame.at(0x4c)));
        while(compare(c,std::int8_t(std::uint32_t(player_index)),50)<0) {
            Kept<25> player(c,c.u32(club+0x98+std::uint32_t(offset)));
            if(compare_logical(c,player)!=0) {
                c.set(3,c.u32(player));
                if(compare(c,c.r(3),c.i32(c.r(30)))<0) {
                    c.set(4,c.u32(player+20));
                    if(compare_logical(c,c.r(4))!=0 && compare_logical(c,c.u32(player+0x24),club)==0) {
                        const auto stats=p.call(0x103054e0,0x1001bfd0,{c.u32(c.r(28)),c.r(4),6});
                        if(compare_logical(c,stats)!=0) {
                            const auto played=c.u8(stats);c.set(8,played);
                            if(compare(c,played,std::int16_t(std::uint32_t(minimum)))>=0 && compare_logical(c,played)!=0) {
                                const auto rating=c.i16(stats+14);
                                const auto goals=c.u8(stats+2),assists=c.u8(stats+6);
                                c.set({{5,goals},{6,0x43300000},{7,assists}});
                                c.put32(frame.at(0x54),goals);c.put32(frame.at(0x50),0x43300000);
                                c.put32(frame.at(0x44),std::uint32_t(rating)^0x80000000);c.put32(frame.at(0x40),0x43300000);
                                const auto goal_value=fp.fsub(c.f64(frame.at(0x50)),c.f64(c.r(29)+0x80));
                                const auto weighted_goals=fp.fdiv(goal_value,c.f64(c.r(29)+0x70));
                                c.put32(frame.at(0x3c),played);c.put32(frame.at(0x38),0x43300000);
                                c.put32(frame.at(0x4c),assists);c.put32(frame.at(0x48),0x43300000);
                                const auto rating_value=fp.fsub(c.f64(frame.at(0x40)),c.f64(c.r(29)+0x38));
                                const auto appearances=fp.fsub(c.f64(frame.at(0x38)),c.f64(c.r(29)+0x80));
                                const auto assist_value=fp.fsub(c.f64(frame.at(0x48)),c.f64(c.r(29)+0x80));
                                const auto per_match=fp.fdiv(rating_value,appearances);
                                c.set_f(1,fp.fdiv(assist_value,c.f64(c.r(29)+0x68)));
                                const auto subtotal=fp.fadd(weighted_goals,per_match);c.set_f(29,fp.fadd(c.f(1),subtotal));
                                if(nonzero_byte(c,p.call(0x100d9480,0x1001c074,{player,club})))
                                    c.set_f(29,fp.fadd(c.f(29),c.f64(c.r(29)+0x30)));
                                else if(nonzero_byte(c,p.call(0x100d94d0,0x1001c094,{player,club})))
                                    c.set_f(29,fp.fsub(c.f(29),c.f64(c.r(29)+0x78)));
                                if(nonzero_byte(c,p.call(0x100da0b0,0x1001c0b0,{player,club})))
                                    c.set_f(29,fp.fadd(c.f(29),c.f64(c.r(29)+0x30)));
                                else if(nonzero_byte(c,p.call(0x100da050,0x1001c0d0,{player,club})))
                                    c.set_f(29,fp.fsub(c.f(29),c.f64(c.r(29))));
                                fp.fcmp(0,c.f(29),c.f(30));if(c.cr(0)&4){c.set(24,player);c.set_f(30,c.f(29));}
                            }
                        }
                    }
                }
            }
            offset=std::uint32_t(offset)+4;player_index=std::uint32_t(player_index)+1;
        }
        c.set_f(31,fp.fsub(c.f(31),c.f64(c.r(29)+0x50)));
    }
    if(compare_logical(c,c.r(24))!=0){p.call(0x1030d6e0,0x1001c134,{c.r(24),record+0x3e0});c.set(3,1);return 1;}
    c.set(3,0);return 0;
}
}

#include "fifa_rankings_detail.hpp"

namespace cm::aot::fifa_rankings {
using namespace detail;
void record_match(Cpu& c,unsigned manager,unsigned match) {
    Frame frame(c,0x10486830,0xd0,{.first_gpr=19,.gpr_offset=-0x54,.first_fpr=29});
    Program p{c};Fpu fp(c);
    const auto singles=c.toc(-0x47b0),doubles=c.toc(-0x47ac);
    keep(c,{{24,c.toc(-0x7ba8)},{25,c.toc(-0x47b4)},{26,c.toc(-0x7a64)},
            {27,c.toc(-0x7bc0)},{28,singles},{29,doubles},{30,manager},{31,match}});
    double home_points=c.f32(singles),away_points=home_points,confederation=c.f64(doubles);
    c.set_f(30,home_points);c.set_f(29,away_points);c.set_f(31,confederation);
    const auto home=c.u32(match+0x1c),away=c.u32(match+0x20);
    keep(c,{{21,home},{20,away}});c.set(4,c.u32(home+0x2c));
    if (compare_logical(c,c.r(4))==0)return;
    const auto home_nation=c.r(4);c.set(5,c.u32(home_nation+0x74));
    if (compare_logical(c,c.r(5))==0)return;
    c.set(3,c.u32(away+0x2c));if (compare_logical(c,c.r(3))==0)return;
    const auto away_nation=c.r(3);c.set(3,c.u32(away_nation+0x74));
    if (compare_logical(c,c.r(3))==0)return;
    if (compare_logical(c,c.u32(match+0x14))==0)return;
    if (compare_logical(c,c.r(5),c.r(3))==0)confederation=c.f64(c.r(5));
    c.set_f(31,confederation);
    enter(c,0x104868d4,{manager});c.set(3,category(c,manager,home_nation));c.set(19,c.r(3));
    enter(c,0x104868e4,{manager,away_nation});c.set(3,category(c,manager,away_nation));
    const auto home_category=std::int8_t(c.r(19)),away_category=std::int8_t(c.r(3));
    c.set(4,std::uint32_t(home_category));c.set(3,std::uint32_t(away_category));
    int difference;
    if (compare(c,home_category-1,away_category)>0)difference=2;
    else if (compare(c,away_category-1,home_category)>0)difference=-2;
    else if (compare(c,home_category,away_category)>0)difference=1;
    else if (compare(c,away_category,home_category)>0)difference=-1;
    else difference=0;
    c.set(19,std::uint32_t(difference));
    c.set(3,c.u8(match+0x47));
    const bool penalties=compare(c,std::int8_t(c.r(3)),-1)!=0;
    unsigned home_goals=penalties?c.r(3):c.u8(match+0x43);
    c.set(23,home_goals);
    compare(c,std::int8_t(c.r(3)),-1);
    unsigned away_goals=c.u8(match+(penalties?0x48:0x44));c.set(22,away_goals);
    c.set(3,c.u32(match+0x14));c.set(4,c.u32(c.r(3)));
    double importance=0;
    if (compare(c,c.r(4),c.i32(c.r(27)))==0)importance=c.f64(doubles+8);
    else {
        p.call(0x10451800,0x10486990,{c.r(3),0});
        if (compare(c,std::int8_t(c.r(3)))==0)importance=c.f64(doubles+16);
        else {
            c.set(4,c.u32(match+0x14));
            bool continental=false;
            if (compare_logical(c,c.u32(c.r(4)+4))!=0) {
                p.call(0x102aed80,0x104869b8,{c.u32(c.r(26))});
                continental=compare(c,std::int8_t(c.r(3)))!=0;
            }
            if (continental) {
                c.set(3,c.u32(match+0x14));c.set(4,c.u32(c.r(3)));
                importance=c.f64(doubles+16);
                if (compare(c,c.r(4),c.i32(c.r(27)))!=0 &&
                    compare(c,c.r(4),c.i32(c.toc(-0x6948)))!=0) importance=c.f64(doubles+24);
            } else {
                p.call(0x10451800,0x104869fc,{c.u32(match+0x14),0});
                bool qualifier=compare(c,std::int8_t(c.r(3)),1)==0;
                if (!qualifier) {
                    c.set(4,c.u32(match+0x14));
                    if (compare_logical(c,c.u32(c.r(4)+4))==0) {
                        p.call(0x102aed80,0x10486a20,{c.u32(c.r(26))});
                        qualifier=compare(c,std::int8_t(c.r(3)))!=0;
                    }
                }
                if (qualifier)importance=c.f64(doubles+24);
                else {
                    p.call(0x10494d20,0x10486a40,{c.u32(c.toc(-0x7b54)),c.u32(match+0x14)});
                    if (compare(c,std::int8_t(c.r(3)))==0) {
                        assertion(c,frame,0x10486a50,0x40,0x38,0xde);c.set(3,c.toc(-0x7bac));return;
                    }
                    importance=c.f64(doubles);
                }
            }
        }
    }
    c.set_f(0,importance);
    // Constants and precision follow each original instruction. The asymmetric
    // weaker-opponent cases are part of the original ranking rules.
    const auto add_double=[&](double& points,double amount) {
        points=fp.fadd(points,amount);
    };
    const auto round_both=[&] {home_points=fp.frsp(home_points);away_points=fp.frsp(away_points);};
    compare(c,std::int8_t(difference));
    if (difference) compare(c,difference,difference<0?-2:2);
    c.set(3,std::uint32_t(std::int32_t(std::int8_t(home_goals))));
    const auto score=compare(c,std::int8_t(home_goals),std::int8_t(away_goals));
    if (difference==2 || difference==-2) {
        const bool strong_home=difference==2;
        const int relative=strong_home?score:-score;
        if (relative>0) {
            c.set_f(1,c.f64(doubles));
            add_double(home_points,c.f(1));add_double(away_points,c.f(1));round_both();
        } else if (relative==0) {
            c.set_f(1,c.f64(doubles+32));
            if (strong_home)away_points=fp.frsp(fp.fadd(away_points,c.f(1)));
            else home_points=fp.frsp(fp.fadd(home_points,c.f(1)));
        } else {
            c.set_f(2,c.f64(doubles));c.set_f(1,c.f64(doubles+40));
            if (strong_home) {home_points=fp.fsub(home_points,c.f(2));away_points=fp.fadd(away_points,c.f(1));round_both();}
            else {away_points=fp.fsub(away_points,c.f(2));home_points=fp.fadd(home_points,c.f(1));away_points=fp.frsp(away_points);home_points=fp.frsp(home_points);}
        }
    } else if (difference==1 || difference==-1) {
        const bool strong_home=difference==1;const auto relative=strong_home?score:-score;
        const auto a=relative>0?4u:relative==0?12u:8u,b=relative>0?8u:relative==0?16u:20u;
        c.set_f(2,c.f32(singles+a));c.set_f(1,c.f32(singles+b));
        auto& strong=strong_home?home_points:away_points;auto& weak=strong_home?away_points:home_points;
        strong=relative<0?fp.fsubs(strong,c.f(2)):fp.fadds(strong,c.f(2));weak=fp.fadds(weak,c.f(1));
    } else {
        if (score>0) {c.set_f(1,c.f64(doubles+32));home_points=fp.frsp(fp.fadd(home_points,c.f(1)));}
        else if (!score) {c.set_f(1,c.f64(doubles));add_double(home_points,c.f(1));add_double(away_points,c.f(1));round_both();}
        else {c.set_f(1,c.f64(doubles+32));away_points=fp.frsp(fp.fadd(away_points,c.f(1)));}
    }
    c.set_f(30,home_points);c.set_f(29,away_points);
    if (compare(c,std::int8_t(home_goals),5)>0)home_goals=5;
    c.set(23,home_goals);
    if (compare(c,std::int8_t(away_goals),5)>0)away_goals=5;
    c.set(22,away_goals);
    c.set(4,std::uint32_t(std::int32_t(std::int8_t(home_goals)))^0x80000000);
    c.put32(frame.at(0x6c),c.r(4));c.put32(frame.at(0x68),0x43300000);
    c.set(3,0x43300000);c.set(5,c.u32(match+0x18));compare_logical(c,c.r(5));
    c.set_f(4,c.f64(doubles+0x50));c.set_f(3,c.f64(doubles+0x38));c.set_f(1,c.f64(frame.at(0x68)));
    c.put32(frame.at(0x64),std::uint32_t(std::int32_t(std::int8_t(away_goals)))^0x80000000);
    c.set_f(2,fp.fsub(c.f(1),c.f(4)));c.set_f(5,c.f64(doubles+0x30));c.put32(frame.at(0x60),0x43300000);
    c.set_f(1,c.f64(frame.at(0x60)));c.set_f(2,fp.fdiv(c.f(2),c.f(3)));c.set_f(1,fp.fsub(c.f(1),c.f(4)));
    c.set_f(2,fp.fmul(c.f(2),home_points));c.set_f(1,fp.fdiv(c.f(1),c.f(3)));c.set_f(1,fp.fmul(c.f(1),away_points));
    c.set_f(2,fp.fmul(c.f(5),c.f(2)));c.set_f(1,fp.fmul(c.f(5),c.f(1)));
    c.set_f(2,fp.frsp(c.f(2)));c.set_f(1,fp.frsp(c.f(1)));
    home_points=fp.fadds(home_points,c.f(2));away_points=fp.fadds(away_points,c.f(1));
    if (c.r(5)) {
        c.set(3,c.u32(c.r(5)+16));
        if (compare_logical(c,c.r(3))!=0) {
            c.set(3,c.u32(c.r(3)+24));
            if (compare_logical(c,c.r(3),c.u32(home+0x2c))==0) {c.set_f(1,c.f32(singles+24));away_points=fp.fadds(away_points,c.f(1));}
        }
    }
    c.set_f(1,fp.fmul(confederation,importance));c.set_f(0,c.f32(singles));c.set_f(1,fp.frsp(c.f(1)));
    home_points=fp.fmuls(home_points,c.f(1));away_points=fp.fmuls(away_points,c.f(1));
    fp.fcmp(0,home_points,c.f(0));if (home_points<c.f(0))home_points=c.f(0);
    c.set_f(0,c.f32(singles));fp.fcmp(0,away_points,c.f(0));if (away_points<c.f(0))away_points=c.f(0);
    c.set(4,c.u32(home+0x2c));const auto home_rep=c.i16(c.r(4)+0xac);c.set(5,extended(home_rep));
    if (compare(c,home_rep,3500)<0) {c.set_f(0,c.f32(singles+28));fp.fcmp(0,home_points,c.f(0));if(home_points>c.f(0))home_points=c.f(0);}
    c.set(3,c.u32(away+0x2c));const auto away_rep=c.i16(c.r(3)+0xac);
    if (compare(c,away_rep,3500)<0) {c.set_f(0,c.f32(singles+28));fp.fcmp(0,away_points,c.f(0));if(away_points>c.f(0))away_points=c.f(0);}
    if (compare(c,away_rep,3500)<0) {c.set_f(0,c.f64(doubles+64));fp.fcmp(0,home_points,c.f(0));if(!(home_points>c.f(0)))home_points=c.f(0);home_points=fp.frsp(home_points);}
    if (compare(c,home_rep,3500)<0) {c.set_f(0,c.f64(doubles+64));fp.fcmp(0,away_points,c.f(0));if(!(away_points>c.f(0)))away_points=c.f(0);away_points=fp.frsp(away_points);}
    c.set_f(30,home_points);c.set_f(29,away_points);
    c.set_f(0,fp.fmul(c.f64(doubles+72),home_points));c.put64(frame.at(0x60),fp.fctiwz(0,c.f(0)));
    enter(c,0x10486df4,{manager,c.r(4),c.u32(frame.at(0x64)),match});append(c,manager,c.r(4),c.r(5),match);
    c.set_f(0,fp.fmul(c.f64(doubles+72),away_points));c.put64(frame.at(0x68),fp.fctiwz(0,c.f(0)));
    enter(c,0x10486e18,{manager,c.u32(away+0x2c),c.u32(frame.at(0x6c)),match});append(c,manager,c.r(4),c.r(5),match);
}
}

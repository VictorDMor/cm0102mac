#include "news_message_detail.hpp"

namespace cm::aot::news::detail {
std::uint32_t Message::intertoto_application() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto id=c.u32(c.toc(-0x7a2c)),team=club(1),competition=competition_name(id);
    keep(c,{{13,competition},{14,team}});leave(c,{{3,c.u32(c.r(28))},{4,id*0x6c},{5,c.u32(c.r(30))}});
    if(full) {
        format(scratch(),0x273d,0x102c3900,{std::uint32_t(std::int32_t(c.i8(team+0x23b))),
            std::uint32_t(std::int32_t(c.i8(team+0x23b))),std::uint32_t(std::int32_t(c.i8(competition+0x6a))),
            std::uint32_t(std::int32_t(c.i8(competition+0x6a))),team+0x221,competition+0x1a});
        assign(scratch(),0x102c390c);
        const auto application=byte(0),accepted=byte(3);
        leave(c,{{3,std::uint32_t(application)},{4,std::uint32_t(accepted)}});
        if(compare(c,application)!=0&&compare(c,accepted)!=0) {
            format(scratch(),0x27b3,0x102c3934);append(scratch(),0x102c3940);
        } else if(compare(c,application)!=0&&compare(c,accepted)==0) {
            format(scratch(),0x27d4,0x102c3960);append(scratch(),0x102c396c);
        }
        link(team+0x221,0x7d5,team,0x102c3980);link(competition+0x1a,0x7d0,competition,0x102c3994);
    } else {
        format(scratch(),0x27f8,0x102c39b8,{std::uint32_t(std::int32_t(c.i8(competition+0x67))),
            std::uint32_t(std::int32_t(c.i8(competition+0x67))),competition+0x4d});
        assign(scratch(),0x102c39c4);link(competition+0x4d,0x7d0,competition,0x102c39d8);
    }
    return 1;
}

std::uint32_t Message::intertoto_selection() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto id=c.u32(c.toc(-0x7a2c)),team=club(0),competition=competition_name(id);
    keep(c,{{13,competition},{14,team}});leave(c,{{3,c.u32(c.r(28))},{4,id*0x6c},{5,c.u32(c.r(30))}});
    const auto grammar=full?0x6au:0x67u,name=full?0x1au:0x4du;
    format(scratch(),full?0x2823:0x28b1,full?0x102c3a4c:0x102c3ab8,
        {std::uint32_t(std::int32_t(c.i8(team+0x23b))),std::uint32_t(std::int32_t(c.i8(team+0x23b))),
         std::uint32_t(std::int32_t(c.i8(competition+grammar))),std::uint32_t(std::int32_t(c.i8(competition+grammar))),
         team+0x221,competition+name});
    assign(scratch(),full?0x102c3a58:0x102c3ac4);
    link(team+0x221,0x7d5,team,full?0x102c3a6c:0x102c3ad8);
    link(competition+name,0x7d0,competition,full?0x102c3a80:0x102c3aec);return 1;
}

std::uint32_t Message::geographical_reshuffle() const {
    const auto team=club(0),competition=competition_name(value(1));keep(c,{{13,team},{14,competition}});
    leave(c,{{3,c.u32(c.r(30))},{4,value(0)*0x248},{5,c.u32(c.r(28))}});
    bool valid=compare(c,team)!=0;
    if(valid)valid=compare_logical(c,c.u32(team+0x3c))!=0;
    if(valid){c.set(3,c.u32(team+0x3c));valid=compare_logical(c,c.u32(c.r(3)+0x10))!=0;}
    if(valid)valid=compare_logical(c,competition)!=0;
    if(valid)valid=compare_logical(c,c.u32(competition+8))!=0;
    if(!valid){assign(pool(0x18f8),0x102c19ac);return 1;}
    if(compare(c,std::int8_t(detail))!=0) {
        const auto origin=c.u32(c.u32(team+0x3c)+0x10),destination=c.u32(competition+8);
        format(scratch(),0x1918,0x102c1a54,
            {std::uint32_t(std::int32_t(c.i8(team+0x242))),std::uint32_t(std::int32_t(c.i8(team+0x242))),
             std::uint32_t(std::int32_t(c.i8(competition+0x67))),std::uint32_t(std::int32_t(c.i8(competition+0x67))),
             std::uint32_t(std::int32_t(c.i8(origin+0x36))),std::uint32_t(std::int32_t(c.i8(origin+0x36))),
             std::uint32_t(std::int32_t(c.i8(destination+0x115))),std::uint32_t(std::int32_t(c.i8(destination+0x115))),
             team+0x1ee,competition+0x4d,origin+0x1c,destination+0xae});
        assign(scratch(),0x102c1a60);link(team+0x1ee,0x7d5,team,0x102c1a74);link(competition+0x4d,0x7d0,competition,0x102c1a88);
        const auto associated=Program{c}.call(0x100c76c0,0x102c1a90,{c.u32(competition+8)});
        link(c.u32(competition+8)+0xae,0x7d5,associated,0x102c1aa8);
    } else {
        const auto destination=c.u32(competition+8);
        format(scratch(),0x19b6,0x102c1ad8,{std::uint32_t(std::int32_t(c.i8(destination+0x116))),
            std::uint32_t(std::int32_t(c.i8(destination+0x116))),destination+0xe1});
        assign(scratch(),0x102c1ae4);
        const auto associated=Program{c}.call(0x100c76c0,0x102c1aec,{c.u32(competition+8)});
        link(c.u32(competition+8)+0xe1,0x7d5,associated,0x102c1b04);
    }
    return 1;
}

std::uint32_t Message::draw_today() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto competition_id=value(0),team_id=value(3);const auto round=short_value(1),stage=short_value(2);const auto unknown=byte(7);
    keep(c,{{13,competition_id},{14,std::uint32_t(round)},{17,std::uint32_t(stage)},{15,team_id},{18,std::uint32_t(unknown)}});
    Program program{c};const auto name_buffer=frame.at(0xa7c),mode=full?1u:2u;
    program.call(0x104769d0,full?0x102c2720:0x102c2834,{competition_name(competition_id),std::uint32_t(round),std::uint32_t(stage),mode,name_buffer,200});
    const auto grammar=program.call(0x10476ec0,full?0x102c273c:0x102c2850,{competition_name(competition_id),std::uint32_t(round),std::uint32_t(stage),mode});
    c.set(8,grammar);
    if(full) {
        if(compare(c,unknown)!=0)
            format(scratch(),0x1e76,0x102c2760,{std::uint32_t(std::int32_t(std::int8_t(grammar))),std::uint32_t(std::int32_t(std::int8_t(grammar))),name_buffer});
        else {
            const auto team=c.u32(c.r(28))+team_id*0x248;
            format(scratch(),0x1ece,0x102c27b4,{std::uint32_t(std::int32_t(c.i8(team+0x23b))),
                std::uint32_t(std::int32_t(c.i8(team+0x23b))),std::uint32_t(std::int32_t(std::int8_t(grammar))),
                std::uint32_t(std::int32_t(std::int8_t(grammar))),team+0x221,name_buffer});
        }
        assign(scratch(),0x102c27c0);
        if(compare(c,unknown)==0) {
            c.set(7,c.u32(c.r(28)));const auto team=c.u32(c.r(28))+team_id*0x248;link(team+0x221,0x7d5,team,0x102c27f0);
        }
        link(name_buffer,0x7d0,competition_name(competition_id),0x102c280c);
    } else {
        format(scratch(),0x1f68,0x102c2870,{std::uint32_t(std::int32_t(std::int8_t(grammar))),
            std::uint32_t(std::int32_t(std::int8_t(grammar))),name_buffer});
        assign(scratch(),0x102c287c);link(name_buffer,0x7d0,competition_name(competition_id),0x102c2898);
    }
    return 1;
}

std::uint32_t Message::cup_bye() const {
    const auto full=compare(c,std::int8_t(detail))!=0;
    const auto current_team=[&]{return c.u32(c.r(28))+c.u32(c.r(20))*0x248;};
    const auto first=[&]{return competition_name(c.u32(c.toc(-0x6918)));};
    const auto second=[&]{return competition_name(c.u32(c.toc(-0x6960)));};
    if(full) {
        const auto team=current_team(),competition1=first(),competition2=second();c.set(13,c.u32(c.r(30)));
        format(scratch(),0x28f6,0x102c3cac,{std::uint32_t(std::int32_t(c.i8(team+0x23b))),std::uint32_t(std::int32_t(c.i8(team+0x23b))),
            std::uint32_t(std::int32_t(c.i8(competition1+0x67))),std::uint32_t(std::int32_t(c.i8(competition1+0x67))),
            std::uint32_t(std::int32_t(c.i8(competition2+0x67))),std::uint32_t(std::int32_t(c.i8(competition2+0x67))),
            team+0x221,competition1+0x4d,competition2+0x4d});
        assign(scratch(),0x102c3cb8);
        if(compare(c,value(0),2)==0){format(scratch(),0x29ef,0x102c3cd0);append(scratch(),0x102c3cdc);}
        if(compare(c,value(0),3)==0){format(scratch(),0x2a15,0x102c3cf4);append(scratch(),0x102c3d00);}
        c.set(7,c.u32(c.r(28)));link(current_team()+0x221,0x7d5,current_team(),0x102c3d30);
        c.set(7,c.u32(c.r(30)));link(first()+0x4d,0x7d0,first(),0x102c3d68);
        c.set(7,c.u32(c.r(30)));link(second()+0x4d,0x7d0,second(),0x102c3da0);
    } else {
        const auto team=current_team(),base=c.u32(c.r(28));leave(c,{{8,base},{9,base},{10,base}});
        format(scratch(),0x2a62,0x102c3df4,{std::uint32_t(std::int32_t(c.i8(team+0x23b))),
            std::uint32_t(std::int32_t(c.i8(team+0x23b))),team+0x221});
        assign(scratch(),0x102c3e00);c.set(7,c.u32(c.r(28)));link(current_team()+0x221,0x7d5,current_team(),0x102c3e30);
    }
    return 1;
}
}

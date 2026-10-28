#include "news_message_detail.hpp"

namespace cm::aot::news::detail {
namespace {
// An article keeps identity, match and language choices separate from the
// original register homes exposed at the external formatter boundaries.
struct InjuryArticle {
    const Message& m;
    Cpu& c;
    Program p;
    const Frame& f;
    std::uint32_t person,first_name,common_name,surname,nation,club,home,away,competition;
    std::int8_t injury;
    std::uint32_t at(unsigned offset) const {return f.at(offset);}
    std::uint32_t team(std::uint32_t id) const {return c.u32(c.r(28))+id*0x248;}
    std::uint32_t country(std::uint32_t id) const {return c.u32(c.r(27))+id*0x128;}
    std::uint32_t player() const {return c.u32(c.toc(-0x7be0))+person*0x70;}
    std::uint32_t injury_service() const {return c.u32(c.r(26));}
    std::uint32_t grammar(std::uint32_t id) const {return extended(c.i8(team(id)+0x23b));}
    std::int8_t kind(std::uint32_t back) const {return std::int8_t(p.call(0x1018c180,back,{injury_service(),extended(injury)}));}
    std::uint32_t injury_grammar(std::uint32_t id,std::uint32_t back) const {return extended(std::int8_t(p.call(0x1018d140,back,{injury_service(),id})));}
    void person_link(std::uint32_t back) const {m.link(at(0x364),0x7d8,player(),back);}
    bool alive(std::uint32_t back) const {
        return compare(c,std::int8_t(p.call(0x100cbde0,back,{player(),first_name,common_name,surname,at(0xd0)})))!=0;
    }
    void club_link(std::uint32_t id,std::uint32_t back) const {c.set(7,c.u32(c.r(28)));m.link(team(id)+0x221,0x7d5,team(id),back);}
    // The specialized category uses an ordered signed-byte decision tree.
    unsigned special_variant() const {
        if(compare(c,injury,79)==0)return 4;
        if(compare(c,injury,79)>=0) {
            if(compare(c,injury,81)==0)return 0;
            if(compare(c,injury,81)>=0)return 4;
            return 3;
        }
        if(compare(c,injury,77)==0)return 1;
        if(compare(c,injury,77)>=0)return 2;
        return 4;
    }
    void headline() const {
        p.call(0x100cb590,0x102c4188,{first_name,common_name,surname,at(0x364),105,5,person});
        if(compare(c,kind(0x102c4194),12)==0) {
            constexpr std::array<unsigned,5> phrases{0x2ace,0x2b06,0x2b4b,0x2b87,0x2bce};
            constexpr std::array<std::uint32_t,5> backs{0x102c41e0,0x102c41f4,0x102c4208,0x102c421c,0x102c4230};
            const auto variant=special_variant();m.format(m.scratch(),phrases[variant],backs[variant],{at(0x364)});
            m.assign(m.scratch(),0x102c423c);if(alive(0x102c4260))person_link(0x102c4288);return;
        }
        if(compare(c,kind(0x102c4298))==0) {
            p.call(0x1018c560,0x102c42b8,{injury_service(),extended(injury),at(0x2cc),50,1});
            m.format(m.scratch(),0x2c05,0x102c42cc,{at(0x364),at(0x2cc)});m.assign(m.scratch(),0x102c42d8);
            if(alive(0x102c42fc))person_link(0x102c4324);return;
        }
        const auto duration=c.u32(at(0x2568));const bool long_term=compare(c,duration,100)>0;
        const auto sum=std::int32_t(person+extended(injury)),remainder=sum%3;
        if(compare(c,remainder,1)==0)m.format(m.scratch(),long_term?0x2c84:0x2d2e,long_term?0x102c4394:0x102c4424,{at(0x364)});
        else if(compare(c,remainder,1)>=0 || compare(c,remainder,0)<0) {
            p.call(0x1030e880,long_term?0x102c43ac:0x102c443c,{duration,at(0x298),50,1});
            m.format(m.scratch(),0x2cb6,long_term?0x102c43c0:0x102c4450,{at(0x364),at(0x298)});
        } else m.format(m.scratch(),long_term?0x2c53:0x2d02,long_term?0x102c4380:0x102c4410,{at(0x364)});
        m.assign(m.scratch(),0x102c445c);if(alive(0x102c4480))person_link(0x102c44a8);
    }
    void initial_full() const {
        p.call(0x100cb590,0x102c44cc,{first_name,common_name,surname,at(0x364),105,1,person});
        p.call(0x1018c560,0x102c44e4,{injury_service(),extended(injury),at(0x2cc),50,1});
        p.call(0x1030e880,0x102c44f8,{c.u32(at(0x2568)),at(0x298),50,1});
    }
    void medical_report() const {
        bool affiliations=compare(c,nation,-1)!=0;
        if(affiliations)affiliations=compare(c,club,-1)!=0;
        const auto first=injury_grammar(extended(injury),affiliations?0x102c4528:0x102c45f0);c.set(15,first);
        const auto second=injury_grammar(extended(injury),affiliations?0x102c453c:0x102c4604);
        if(affiliations)m.format(m.scratch(),0x2d5f,0x102c45e0,{second,first,grammar(club),grammar(club),
            extended(c.i8(country(nation)+0x116)),extended(c.i8(country(nation)+0x116)),at(0x364),at(0x298),at(0x2cc),team(club)+0x221,country(nation)+0xe1});
        else m.format(m.scratch(),0x2e2e,0x102c4628,{second,first,at(0x364),at(0x298),at(0x2cc)});
        m.assign(m.scratch(),0x102c4634);if(alive(0x102c4658))person_link(0x102c4680);
        if(compare(c,nation,-1)!=0) {
            const auto national_club=p.call(0x100c76c0,0x102c4698,{country(nation)});
            // This link uses r0 for the nation base, leaving r7 as returned.
            m.link(country(nation)+0xe1,0x7d5,national_club,0x102c46b8);
        }
        if(compare(c,club,-1)!=0)club_link(club,0x102c46e8);
    }
    struct Weekday {std::uint32_t text,index,grammar;};
    Weekday weekday(const std::array<std::uint32_t,5>& backs,unsigned text_home,unsigned index_home,unsigned grammar_home) const {
        const auto day=p.call(0x10715b20,backs[0],{at(0xd8)});
        const auto text=p.call(0x107187d0,backs[1]);c.set(text_home,text);
        const auto index=p.call(0x10715b20,backs[2],{at(0xd8)});c.set(index_home,index);
        const auto language=p.call(0x10727880,backs[3]);
        const auto grammatical=extended(std::int8_t(p.call(0x10718af0,backs[4],{language,index})));c.set(grammar_home,grammatical);
        (void)day;return {text,index,grammatical};
    }
    void full_without_match(bool own_club) const {
        bool special_language=compare(c,std::int8_t(p.call(0x10727880,0x102c4e6c)),10)==0;
        if(special_language && compare(c,injury,59)!=0)special_language=compare(c,injury,71)==0;
        if(special_language) {
            p.call(0x10715b20,0x102c4e98,{at(0xd8)});const auto day=p.call(0x107187d0,0x102c4e9c);
            p.call(0x1077dbf0,0x102c4ea8,{at(0x1914),day});
            if(compare(c,c.i8(at(0x1914)))!=0) {
                const auto length=p.call(0x1077dbd0,0x102c4ebc,{at(0x1914)});
                const auto last=c.i8(at(0x1913)+length);
                const auto index=std::uint32_t(std::int32_t(last)-'C');
                bool suffix=false;if(compare_logical(c,index,0x37)<=0) {
                    suffix=last=='C'||last=='S'||last=='X'||last=='Z'||last=='c'||last=='s'||last=='x'||last=='z';
                }
                p.call(0x1077b720,suffix?0x102c4f04:0x102c4f24,{m.scratch(),m.pool(suffix?0x345d:0x34a4),at(0x364),at(0x298),at(0x2cc),at(0x1914)});
            }
        } else {
            bool general=compare(c,own_club)!=0;
            if(!general)general=compare(c,club,-1)==0;
            if(general) {
                if(compare(c,kind(0x102c4f44),12)==0) {
                    constexpr std::array<unsigned,5> phrases{0x34ec,0x357f,0x3609,0x3697,0x370d};
                    constexpr std::array<std::uint32_t,5> backs{0x102c4f90,0x102c4fa8,0x102c4fc0,0x102c4fd4,0x102c4fec};
                    const auto n=special_variant();if(n==0||n==3)m.format(m.scratch(),phrases[n],backs[n],{at(0x364)});
                    else m.format(m.scratch(),phrases[n],backs[n],{at(0x364),at(0x298)});
                } else {
                    const auto day=weekday({0x102c4ff8,0x102c4ffc,0x102c5008,0x102c5010,0x102c5018},15,13,14);
                    const auto first=injury_grammar(extended(injury),0x102c502c);c.set(13,first);
                    const auto second=injury_grammar(extended(injury),0x102c5040);
                    m.format(m.scratch(),0x3784,0x102c5070,{second,first,day.grammar,100,at(0x364),at(0x298),at(0x2cc),day.text});
                }
            } else if(compare(c,kind(0x102c5080),12)==0) {
                const auto replacement=p.call(0x1018c2f0,0x102c50a4,{injury_service(),player()});c.set(13,replacement);
                if(compare(c,std::int8_t(replacement),-1)==0) {
                    const auto n=special_variant();
                    constexpr std::array<unsigned,5> phrases{0x3851,0x38e1,0x398a,0x3a37,0x3ad1};
                    constexpr std::array<std::uint32_t,5> backs{0x102c50f4,0x102c5144,0x102c5194,0x102c51e0,0x102c5230};
                    if(n==0)m.format(m.scratch(),phrases[n],backs[n],{at(0x364)});
                    else {if(n==3)c.set(9,c.u32(c.r(28)));else c.set(10,c.u32(c.r(28)));
                        if(n==3)m.format(m.scratch(),phrases[n],backs[n],{grammar(club),grammar(club),team(club)+0x221,at(0x364)});
                        else m.format(m.scratch(),phrases[n],backs[n],{grammar(club),grammar(club),team(club)+0x221,at(0x364),at(0x298)});}
                } else {
                    p.call(0x1018c560,0x102c524c,{injury_service(),replacement,at(0x2cc),50,1});
                    // This tree intentionally has no dedicated type81 phrase.
                    unsigned n;
                    if(compare(c,injury,79)==0)n=4;
                    else if(compare(c,injury,79)>=0)n=compare(c,injury,81)>=0?4:3;
                    else if(compare(c,injury,77)==0)n=1;
                    else n=compare(c,injury,77)>=0?2:4;
                    if(n==3) {c.set(9,c.u32(c.r(28)));m.format(m.scratch(),0x3a37,0x102c53c8,{grammar(club),grammar(club),team(club)+0x221,at(0x364)});}
                    else {
                        const auto first=injury_grammar(replacement,n==1?0x102c5284:n==2?0x102c5308:0x102c53d8);c.set(13,first);
                        const auto second=injury_grammar(replacement,n==1?0x102c5298:n==2?0x102c531c:0x102c53ec);
                        if(n==4)m.format(m.scratch(),0x3d00,0x102c5454,{grammar(club),grammar(club),100,second,first,team(club)+0x221,at(0x364),at(0x298),at(0x2cc)});
                        else m.format(m.scratch(),n==1?0x3b67:0x3c31,n==1?0x102c52f8:0x102c537c,{grammar(club),grammar(club),second,first,team(club)+0x221,at(0x364),at(0x298),at(0x2cc)});
                    }
                }
            } else {
                const auto day=weekday({0x102c5460,0x102c5464,0x102c5470,0x102c5478,0x102c5480},13,14,15);
                const auto first=injury_grammar(extended(injury),0x102c5494);c.set(14,first);
                const auto second=injury_grammar(extended(injury),0x102c54a8);
                m.format(m.scratch(),0x3dcd,0x102c552c,{grammar(club),grammar(club),100,100,100,second,first,day.grammar,100,team(club)+0x221,at(0x364),at(0x298),at(0x2cc),day.text});
            }
        }
        m.assign(m.scratch(),0x102c5538);person_link(0x102c5558);
        if(compare(c,club,-1)>0)club_link(club,0x102c5588);
    }
    void full_match() const;
};
}

namespace {
void InjuryArticle::full_match() const {
    bool special_language=compare(c,std::int8_t(p.call(0x10727880,0x102c46f8)),10)==0;
    if(special_language&&compare(c,injury,59)!=0)special_language=compare(c,injury,71)==0;
    if(special_language) {
        p.call(0x10715b20,0x102c4724,{at(0xd8)});const auto day=p.call(0x107187d0,0x102c4728);
        c.put32(at(0x38),day);
        p.call(0x1077b720,0x102c4778,{m.scratch(),m.pool(0x2ea6),at(0x364),at(0x298),at(0x2cc),m.competition_name(competition)+0x1a,team(home)+0x221,team(away)+0x221});
    } else {
        const auto name=m.competition_name(competition);
        leave(c,{{3,name},{4,competition*0x6c},{5,c.u32(c.r(30))}});
        const auto special_competition=c.u32(c.toc(-0x7a50));
        auto competition_grammar=std::uint32_t(c.u8(name+0x6a));c.set(17,competition_grammar);
        if(compare(c,competition,special_competition)==0){competition_grammar=100;c.set(17,100);}
        const bool no_record=compare(c,c.i8(at(0x2564)))!=0;
        if(no_record) {
            const auto day=weekday({0x102c47b4,0x102c47b8,0x102c47c4,0x102c47cc,0x102c47d4},15,13,14);
            const auto first=injury_grammar(extended(injury),0x102c47e8);c.set(13,first);
            const auto second=injury_grammar(extended(injury),0x102c47fc);
            keep(c,{{18,100},{19,home*0x248},{23,away*0x248+0x221},{13,home*0x248}});
            m.format(m.scratch(),0x2efd,0x102c48d4,{second,first,extended(std::int8_t(competition_grammar)),extended(std::int8_t(competition_grammar)),grammar(home),grammar(home),grammar(away),grammar(away),day.grammar,100,
                at(0x364),at(0x298),at(0x2cc),m.competition_name(competition)+0x1a,team(home)+0x221,team(away)+0x221,day.text});
        } else {
            const auto match_competition=c.u32(at(0x308));const auto round=c.i16(at(0x30c)),stage=c.i16(at(0x30e));
            p.call(0x104769d0,0x102c4900,{m.competition_name(match_competition),extended(round),extended(stage),1,at(0x9b4),200});
            if(compare(c,p.call(0x1077dc90,0x102c490c,{at(0x9b4),m.pool(0x1c)}))==0)c.put8(at(0x9b4),'f');
            else if(compare(c,p.call(0x1077dc90,0x102c492c,{at(0x9b4),m.pool(0x25)}))==0)c.put8(at(0x9b4),'r');
            const auto round_grammar=p.call(0x10476ec0,0x102c495c,{m.competition_name(match_competition),extended(round),extended(stage),1});c.set(13,round_grammar);
            c.set(4,c.u8(at(0x31c)));c.set(3,extended(c.i8(at(0x31c))));
            const auto result=compare(c,c.i8(at(0x31c)),c.i8(at(0x31d)));
            const bool won=result>0;
            bool drawn=false;if(!won) {c.set(3,c.u8(at(0x31c)));drawn=compare(c,c.i8(at(0x31c)),c.i8(at(0x31d)))==0;}
            const auto day=weekday(won?std::array<std::uint32_t,5>{0x102c4980,0x102c4984,0x102c4990,0x102c4998,0x102c49a0}:
                drawn?std::array<std::uint32_t,5>{0x102c4ac0,0x102c4ac4,0x102c4ad0,0x102c4ad8,0x102c4ae0}:
                      std::array<std::uint32_t,5>{0x102c4bf8,0x102c4bfc,0x102c4c08,0x102c4c10,0x102c4c18},17,14,15);
            const auto first=injury_grammar(extended(injury),won?0x102c49b4:drawn?0x102c4af4:0x102c4c2c);c.set(14,first);
            const auto second=injury_grammar(extended(injury),won?0x102c49c8:drawn?0x102c4b08:0x102c4c40);
            if(drawn) {
                keep(c,{{18,100},{13,at(0x364)},{23,home*0x248},{19,home*0x248},{24,away*0x248+0x221}});
                m.format(m.scratch(),0x31a1,0x102c4bec,{100,100,second,first,grammar(home),grammar(home),extended(std::int8_t(round_grammar)),extended(std::int8_t(round_grammar)),grammar(away),grammar(away),day.grammar,100,
                    at(0x364),at(0x298),at(0x2cc),team(home)+0x221,at(0x178),at(0x9b4),team(away)+0x221,day.text});
            } else {
                keep(c,{{18,home*0x248+0x221},{19,at(0x2cc)},{23,at(0x298)},{24,at(0x364)},{13,team(home)+0x221}});
                m.format(m.scratch(),won?0x304a:0x3300,won?0x102c4a9c:0x102c4d14,{second,first,grammar(home),grammar(home),extended(std::int8_t(round_grammar)),extended(std::int8_t(round_grammar)),grammar(away),grammar(away),day.grammar,100,
                    at(0x364),at(0x298),at(0x2cc),team(home)+0x221,at(0x178),at(0x9b4),team(away)+0x221,day.text});
            }
        }
    }
    m.assign(m.scratch(),0x102c4d20);
    if(compare(c,std::int8_t(p.call(0x104abf40,0x102c4d38,{c.u32(c.toc(-0x7b34)),m.competition_name(competition)})))==0) {
        c.set(7,c.u32(c.r(30)));m.link(m.competition_name(competition)+0x1a,0x7d0,m.competition_name(competition),0x102c4d68);
    }
    person_link(0x102c4d88);club_link(home,0x102c4db0);club_link(away,0x102c4dd8);
    if(compare(c,c.i8(at(0x31c)),-1)==0)return;
    if(compare(c,c.i8(at(0x2564)))!=0)return;
    const auto exists=p.call(0x1029eb20,0x102c4e18,{c.u32(c.toc(-0x7b4c)),c.u32(at(0x320)),c.u32(at(0x314)),c.u32(at(0x318)),c.u16(at(0x300)),c.u16(at(0x302))});
    if(compare(c,std::int8_t(exists))!=0)m.link(at(0x178),0x7d3,c.u32(at(0x320)),0x102c4e34);
    const auto opponent=c.u32(at(0x318));c.set(7,c.u32(c.r(28)));m.link(team(opponent)+0x1ee,0x7d5,team(opponent),0x102c4e64);
}
}

std::uint32_t Message::injury_article() const {
    Program p{c};p.call(0x10715880,0x102c3fc8,{frame.at(0x300)});
    const auto person=value(0),first=value(1),common=value(2),surname=value(3);const auto injury=byte(4);
    keep(c,{{23,extended(injury)},{25,person},{13,first},{14,common},{24,surname}});
    p.call(0x107156b0,0x102c3ffc,{frame.at(0xd8),extended(short_value(5)),value(6)&0xffff});
    c.put32(frame.at(0x2568),value(7));
    p.call(0x107156b0,0x102c4020,{frame.at(0xd0),extended(short_value(8)),value(9)&0xffff});
    const auto home=value(10),away=value(11),competition_id=value(12),nation=value(13),club_id=value(14);
    const bool no_record=value(15)!=0x420e9;
    keep(c,{{22,home},{21,away},{20,competition_id},{19,nation},{18,club_id}});
    c.put8(frame.at(0x80),16);c.put8(frame.at(0x2564),no_record);
    p.call(0x1030dee0,0x102c4070,{item,frame.at(0x80),frame.at(0x300)});
    const auto default_title=c.toc(-0x5104);
    for(unsigned offset=0;offset<200;offset+=8) {
        const auto first_word=c.u32(default_title+offset),second_word=c.u32(default_title+offset+4);
        c.put32(frame.at(0x9b4+offset),first_word);c.put32(frame.at(0x9b8+offset),second_word);
    }
    const auto a=c.i8(frame.at(0x31c)),b=c.i8(frame.at(0x31d));
    const auto minimum=compare(c,a,b)<0?a:b;const auto maximum=compare(c,a,b)>0?a:b;
    p.call(0x1077b720,0x102c40f8,{frame.at(0x178),pool(0x2ac8),extended(maximum),extended(minimum)});
    const auto managers=c.toc(-0x7bb0);const auto selected=c.i16(managers+0x2c58);
    const auto active_manager=c.u32(managers+std::uint32_t(selected)*0x2c0+4);c.set(3,active_manager);
    bool own_club;
    if(compare(c,active_manager)!=0 && compare_logical(c,c.u32(active_manager+0x24))!=0) {
        c.set(4,0);own_club=compare(c,club_id,-1)==0;
        if(!own_club) {const auto team=c.u32(active_manager+0x24);c.set(3,team);own_club=compare(c,club_id,c.u32(team))==0;}
        if(own_club)c.set(4,1);
    } else own_club=club_id==0xffffffffu;
    c.set(15,own_club);
    InjuryArticle article{*this,c,p,frame,person,first,common,surname,nation,club_id,home,away,competition_id,injury};
    if(compare(c,std::int8_t(detail))==0)article.headline();
    else {
        article.initial_full();
        if(compare(c,article.kind(0x102c4504))==0)article.medical_report();
        else if(compare(c,home,-1)!=0)article.full_match();
        else article.full_without_match(own_club);
    }
    return 1;
}
}

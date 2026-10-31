// Cases of the AWOL checker for awol_text.cpp: the primary, training and
// fitness message renderers (0x100072e0, 0x10008b60, 0x10009b60).
//
// The message is a table of words in guest memory that the field service
// (0x102cb570) reads, so a case can change a field between two reads. The
// text services write words built from every register they are given, so a
// wrong argument shows in memory as well as in the call trace.
namespace text_world {
using namespace world;
constexpr unsigned message=data+0x44000,pool=data+0x46000,buffer=data+0x48000,log=data+0x4a000,counters=data+0x4e000,
    nations=data+0x4f000,sessions=data+0x56000,desks=data+0x59000;
constexpr unsigned genders[8]={0,1,0x80,0xff,0x7f,2,0x81,0x40};
constexpr unsigned none=0xffffffff;
unsigned digest(Cpu& c,unsigned at,unsigned bytes){unsigned h=0x811c9dc5;for(unsigned n=0;n<bytes;++n)h=(h^c.u8(at+n))*16777619u;return h;}
unsigned mix(unsigned h,unsigned value){return (h^value)*16777619u;}
// How the desk answers "does the player manage this club".
enum class Desk{no_desk,no_club_list,held};
struct Scene {
    std::array<unsigned,12> field{};
    std::vector<unsigned> answers{1,1};   // name-match answers, by call
    std::function<void(Cpu&,unsigned)> after_read;
    Desk desk=Desk::held;
    unsigned held=0;                      // the club id the desk holds
    int index=0;                          // the desk number (signed halfword)
    bool drift=false;                     // the services move the tables
};
// Moves the tables a routine reads again after a call.
void drift(Cpu& c,unsigned staffs,unsigned clubs,unsigned nation_step) {
    c.put32(c.toc(-0x7be0),c.u32(c.toc(-0x7be0))+staffs*0x70);
    c.put32(c.toc(-0x7be8),c.u32(c.toc(-0x7be8))+clubs*0x248);
    c.put32(c.toc(-0x7b98),c.u32(c.toc(-0x7b98))+nation_step*0x128);
}
void install(Cpu& c,const Scene& scene) {
    script.services[0x102cb570]=[scene](Cpu& c){
        const auto field=c.r(4),value=c.u32(message+field*4);
        if(scene.after_read)scene.after_read(c,field);
        if(scene.drift)drift(c,0,0,1);
        return value;
    };
    script.services[0x100cb590]=[scene](Cpu& c){
        unsigned h=0x1234567;for(unsigned r:{3u,4u,5u,7u,8u,9u})h=mix(h,c.r(r));
        for(unsigned n=0;n<6;++n)c.put32(c.r(6)+n*4,h+n);
        if(scene.drift)drift(c,0,1,0);
        return h;
    };
    script.services[0x10727890]=[scene](Cpu& c){
        unsigned h=0x7654321;for(unsigned r=4;r<=10;++r)h=mix(h,c.r(r));
        h=mix(h,c.u32(c.sp()+0x38));
        for(unsigned n=0;n<8;++n)c.put32(c.r(3)+n*4,h+n);
        c.put8(c.r(3)+32,0);
        if(scene.drift)drift(c,1,0,1);
        return 32u;
    };
    script.services[0x1074b830]=[scene](Cpu& c){
        const auto n=c.u32(counters+4);c.put32(counters+4,n+1);
        const auto at=log+0x100+n*16;
        c.put32(at,c.r(3));c.put32(at+4,c.r(4));c.put32(at+8,digest(c,c.r(4),0x24));
        if(scene.drift)drift(c,1,0,0);
        return 0u;
    };
    script.services[0x100cbde0]=[scene](Cpu& c){
        const auto n=c.u32(counters);c.put32(counters,n+1);
        if(scene.drift)drift(c,1,1,0);
        return n<scene.answers.size()?scene.answers[n]:0u;
    };
    script.services[0x1074bab0]=[scene](Cpu& c){
        const auto n=c.u32(counters+8);c.put32(counters+8,n+1);
        const auto at=log+0x800+n*20;
        c.put32(at,c.r(3));c.put32(at+4,c.r(4));c.put32(at+8,c.r(5));c.put32(at+12,c.r(6));c.put32(at+16,digest(c,c.r(4),0x24));
        if(scene.drift)drift(c,0,1,0);
        return 0u;
    };
    (void)c;
}
// Builds the scene and the call: message, brief and output are r4-r6.
void prepare(Cpu& c,const Scene& scene,unsigned brief) {
    c.put32(c.r(2)-0x5a08,pool);c.put32(c.r(2)-0x7bb8,buffer);c.put32(c.r(2)-0x7bb0,sessions);
    c.put32(c.toc(-0x7b98),nations);
    for(unsigned id=0;id<10;++id)c.put8(club(id)+0x23b,genders[id%8]);
    for(unsigned id=0;id<10;++id)c.put8(nations+id*0x128+0x116,genders[(id+3)%8]);
    for(unsigned n=0;n<scene.field.size();++n)c.put32(message+n*4,scene.field[n]);
    c.put16(sessions+0x2c58,unsigned(scene.index)&0xffff);
    const auto slot=sessions+unsigned(scene.index)*0x2c0,p=desks+0x400*unsigned(scene.index+1),q=p+0x100;
    c.put32(slot+4,scene.desk==Desk::no_desk?0:p);
    c.put32(p+0x24,scene.desk==Desk::no_club_list?0:q);
    c.put32(q,scene.held);
    c.put32(counters,0);c.put32(counters+4,0);c.put32(counters+8,0);
    install(c,scene);
    c.set({{3,0x777},{4,message},{5,brief},{6,log}});
}
}
void add_text_cases(std::vector<Case>& cases) {
    using namespace text_world;
    const auto add=[&](unsigned entry,const std::string& name,const Scene& scene,unsigned brief) {
        cases.push_back({entry,name,[=](Cpu& c){prepare(c,scene,brief);}});
    };
    const auto label=[](unsigned value){return std::to_string(value);};
    // Message fields: 0 id, 1-3 name parts, then the routine's own.
    const unsigned ids[]={0,5,none,0x7fffffff,0x80000000};
    const unsigned clubs3[]={none,0,5};

    // ---- primary (0x100072e0) ----
    {
        constexpr unsigned entry=0x100072e0;
        const auto full=[&](unsigned staff,unsigned previous,unsigned club,unsigned kind,unsigned selector,unsigned nation) {
            Scene s;s.field={staff,0x111,0x222,0x333,previous,club,kind,selector,nation};return s;
        };
        // Full view: both templates, every kind band, both selector bands, nations.
        for(const bool own:{false,true})
            for(const unsigned kind:{none,0u,1u,2u,3u,4u,5u,0x7fffffffu,0x80000000u}) {
                const bool four=kind==4;
                for(const unsigned club:clubs3)
                    for(const unsigned previous:four?std::vector<unsigned>{0}:std::vector<unsigned>{none,0,6})
                        for(const unsigned selector:four?std::vector<unsigned>{none,0,6,7,8,0x80000000}:std::vector<unsigned>{0})
                            for(const unsigned nation:four?std::vector<unsigned>{none,0,3,7}:std::vector<unsigned>{0}) {
                                auto s=full(5,previous,club,kind,selector,nation);
                                s.held=own?club:club+1;
                                add(entry,"full-"+std::string(own?"own":"other")+"-kind"+label(kind)+"-club"+label(club)+"-prev"+label(previous)+"-sel"+label(selector)+"-nation"+label(nation),s,1);
                            }
            }
        // The brief byte is the low byte, signed.
        for(const unsigned brief:{1u,0x7fu,0x80u,0xffu,0x1ffu,0xffffff80u,0x7fffff01u}) {
            auto s=full(5,3,2,2,0,0);s.held=2;add(entry,"brief-byte-full-"+label(brief),s,brief);
        }
        for(const unsigned brief:{0u,0x100u,0xffffff00u,0x12345600u}) {
            auto s=full(5,3,2,2,0,0);add(entry,"brief-byte-brief-"+label(brief),s,brief);
        }
        // Staff ids, name-match answers (one link), desks.
        for(const unsigned staff:ids)
            for(const unsigned answer:{0u,1u,0x100u,0x180u,0xffffff01u,0x7fu}) {
                auto s=full(staff,none,0,0,0,0);s.answers={answer};s.held=0;
                add(entry,"full-staff"+label(staff)+"-answer"+label(answer),s,1);
            }
        for(const unsigned kind:{0u,2u,4u})
            for(const Desk desk:{Desk::no_desk,Desk::no_club_list,Desk::held})
                for(const int index:{0,1,-1})
                    for(const unsigned held:{0u,3u,0x80000000u}) {
                        auto s=full(5,4,3,kind,0,1);s.desk=desk;s.index=index;s.held=held;
                        add(entry,"desk-kind"+label(kind)+"-"+std::to_string(int(desk))+"-"+std::to_string(index)+"-"+label(held),s,1);
                    }
        for(const bool own:{false,true})
            for(const unsigned kind:{0u,2u,4u}) {
                auto s=full(5,4,3,kind,7,2);s.drift=true;s.held=own?3:0;
                add(entry,"drift-"+std::string(own?"own":"other")+label(kind),s,1);
            }
        // Brief view: kinds, the staff id modulo 3 (truncated, so negatives differ), clubs.
        for(const unsigned kind:{none,0u,1u,2u,3u,4u,5u,0x80000000u})
            for(const unsigned staff:{0u,1u,2u,3u,4u,5u,none,0xfffffffeu,0xfffffffdu,0xfffffffcu,0x7fffffffu,0x80000000u,0x80000001u})
                for(const unsigned club:{none,0u,4u}) {
                    Scene s;s.field={staff,0x111,0x222,0x333,0x444,club,kind};
                    add(entry,"brief-kind"+label(kind)+"-staff"+label(staff)+"-club"+label(club),s,0);
                }
        // Field six is read twice: the second read wins.
        for(const unsigned first:{0u,1u,2u,4u,none})
            for(const unsigned second:{0u,1u,2u,4u,none}) {
                Scene s;s.field={7,0x111,0x222,0x333,0x444,2,first};
                s.after_read=[=](Cpu& c,unsigned field){if(field==6)c.put32(message+24,second);if(field==4)c.put32(message+16,0x999);};
                add(entry,"brief-reread-"+label(first)+"-"+label(second),s,0);
            }
        for(const unsigned answer:{0u,1u,0x180u,0x100u}) {
            Scene s;s.field={4,0x111,0x222,0x333,0x444,2,0};s.answers={answer};add(entry,"brief-answer"+label(answer),s,0);
        }
        for(const unsigned kind:{0u,2u,4u}) {
            Scene s;s.field={7,0x111,0x222,0x333,0x444,2,kind};s.drift=true;add(entry,"brief-drift"+label(kind),s,0);
        }
    }

    // ---- training (0x10008b60) ----
    {
        constexpr unsigned entry=0x10008b60;
        const auto full=[&](unsigned staff,unsigned club,unsigned selector,unsigned reporter) {
            Scene s;s.field={staff,0x111,0x222,0x333,club,selector,0x6666,reporter,0x444,0x555,0x666};return s;
        };
        for(const bool own:{false,true})
            for(const unsigned selector:{none,0u,1u,2u,5u,0x80000000u})
                for(const unsigned reporter:{none,0u,7u,0xfffffffeu,0x80000000u,0x7fffffffu})
                    for(const unsigned club:clubs3) {
                        auto s=full(5,club,selector,reporter);s.held=own?club:club+1;
                        add(entry,"full-"+std::string(own?"own":"other")+"-sel"+label(selector)+"-rep"+label(reporter)+"-club"+label(club),s,1);
                    }
        for(const unsigned brief:{1u,0x7fu,0x80u,0xffu,0x1ffu,0xffffff80u}) {
            auto s=full(5,3,1,7);s.held=3;add(entry,"brief-byte-full-"+label(brief),s,brief);
        }
        for(const unsigned brief:{0u,0x100u,0xffffff00u}) add(entry,"brief-byte-brief-"+label(brief),full(5,3,1,7),brief);
        for(const unsigned staff:ids)
            for(const unsigned reporter:{none,9u})
                for(const auto& answers:{std::vector<unsigned>{0,0},{1,0},{0,1},{1,1},{0x180,0x100},{0x100,0x180},{0xffffff01,0x7f}}) {
                    auto s=full(staff,0,0,reporter);s.answers=answers;
                    add(entry,"full-staff"+label(staff)+"-rep"+label(reporter)+"-answers"+label(answers[0])+"-"+label(answers[1]),s,1);
                }
        for(const Desk desk:{Desk::no_desk,Desk::no_club_list,Desk::held})
            for(const int index:{0,1,-1})
                for(const unsigned held:{0u,3u,0x80000000u}) {
                    auto s=full(5,3,0,none);s.desk=desk;s.index=index;s.held=held;
                    add(entry,"desk-"+std::to_string(int(desk))+"-"+std::to_string(index)+"-"+label(held),s,1);
                }
        for(const bool own:{false,true})
            for(const unsigned selector:{0u,1u,3u}) {
                auto s=full(5,3,selector,8);s.drift=true;s.held=own?3:0;
                add(entry,"drift-"+std::string(own?"own":"other")+label(selector),s,1);
            }
        // Brief view: the two flags.
        for(const unsigned kind:{0u,1u,none,0x100u,0x80000000u})
            for(const unsigned flag:{0u,1u,2u,none,0x100u}) {
                Scene s;s.field={9,0x111,0x222,0x333,0x444,flag,kind};
                add(entry,"brief-kind"+label(kind)+"-flag"+label(flag),s,0);
            }
        for(const unsigned staff:ids)
            for(const unsigned answer:{0u,1u,0x180u,0x100u}) {
                Scene s;s.field={staff,0x111,0x222,0x333,0x444,1,0};s.answers={answer};
                add(entry,"brief-staff"+label(staff)+"-answer"+label(answer),s,0);
            }
        {Scene s;s.field={9,0x111,0x222,0x333,0x444,1,0};s.drift=true;add(entry,"brief-drift",s,0);}
    }

    // ---- fitness (0x10009b60) ----
    {
        constexpr unsigned entry=0x10009b60;
        const auto full=[&](unsigned staff,unsigned club,unsigned flag,unsigned reporter) {
            Scene s;s.field={staff,club,flag,0x111,0x222,0x333,reporter,0x444,0x555,0x666};return s;
        };
        for(const unsigned flag:{0u,1u,none,0x100u,0x80000000u})
            for(const unsigned reporter:{none,0xfffffffeu,0x80000000u,0u,7u,0x7fffffffu})
                for(const unsigned club:clubs3)
                    for(const unsigned brief:{1u,0u}) {
                        add(entry,"flag"+label(flag)+"-rep"+label(reporter)+"-club"+label(club)+"-brief"+label(brief),full(5,club,flag,reporter),brief);
                    }
        for(const unsigned brief:{1u,0x7fu,0x80u,0xffu,0x1ffu,0xffffff80u,0x7fffff01u}) add(entry,"brief-byte-full-"+label(brief),full(5,3,1,7),brief);
        for(const unsigned brief:{0u,0x100u,0xffffff00u,0x12345600u}) add(entry,"brief-byte-brief-"+label(brief),full(5,3,1,7),brief);
        for(const unsigned staff:ids)
            for(const unsigned reporter:{none,9u})
                for(const unsigned brief:{0u,1u})
                    for(const auto& answers:{std::vector<unsigned>{0,0},{1,0},{0,1},{1,1},{0x180,0x100},{0x100,0x180},{0xffffff01,0x7f}}) {
                        auto s=full(staff,0,1,reporter);s.answers=answers;
                        add(entry,"staff"+label(staff)+"-rep"+label(reporter)+"-brief"+label(brief)+"-answers"+label(answers[0])+"-"+label(answers[1]),s,brief);
                    }
        for(const unsigned brief:{0u,1u})
            for(const unsigned flag:{0u,1u}) {
                auto s=full(5,3,flag,8);s.drift=true;add(entry,"drift-brief"+label(brief)+"-flag"+label(flag),s,brief);
            }
        for(const unsigned club:{none,8u,9u}) add(entry,"club-out-of-table"+label(club),full(5,club,1,8),1);
    }
}

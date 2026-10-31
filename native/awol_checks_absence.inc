// Cases of the AWOL checker for awol_absence.cpp.
namespace absence_cases {
using namespace world;
constexpr unsigned relation=0x100d9b80,club_query=0x1014c2e0,date_add=0x10717890,message_set=0x102cb4b0,message_new=0x102bdf90,
    message_post=0x102cdbf0,message_staff=0x102cc140,message_manager=0x102cc400,reseed_routine=0x1077b770;
unsigned table(Cpu& c){return c.toc(-0x7b8c);}
unsigned pool_end(Cpu& c){return c.u32(c.toc(-0x7b88));}
// Pseudo-random words for the whole table (and the word at its end).
void fill_words(Cpu& c,unsigned seed) {
    for(unsigned n=0;n<=64;++n)c.put16(table(c)+n*2,std::uint16_t(seed*0x9e37u+n*0x3039u+n*n*7u));
}
// Words for the draws that follow: draw k (1-based, before any wrap) reads word k.
void put_words(Cpu& c,const std::vector<unsigned>& words) {
    unsigned n=1;for(const auto w:words)c.put16(table(c)+(n++)*2,std::uint16_t(w));
}
void set_salt(Cpu& c,unsigned salt){c.put32(c.toc(-0x7b90),salt);}
// The cursor such that the j-th draw is the one that wraps.
void wrap_at(Cpu& c,unsigned j){c.put32(c.toc(-0x7b84),pool_end(c)+2-2*j);}
void peers(Cpu& c,unsigned club_at,std::vector<unsigned> list) {
    for(unsigned slot=0;slot<50;++slot)c.put32(club_at+0x98+slot*4,slot<list.size()?list[slot]:0);
}
void traits(Cpu& c,unsigned a,unsigned b) {
    for(unsigned id=0;id<staff_count;++id){c.put8(staff(id)+0x65,a);c.put8(staff(id)+0x67,b);}
}
// The world around register_absence: every dependency answers, successive
// calls in an order the case can follow.
void absence_services(Cpu& c,unsigned query=0) {
    script.services[relation]=[](Cpu&){return 0u;};
    script.services[club_query]=[=](Cpu&){return query;};
    script.services[date_add]=[](Cpu& c) {
        c.put16(c.r(3),c.u16(c.r(4))+c.r(5)*3);c.put16(c.r(3)+2,c.u16(c.r(4)+2)^c.r(5));c.put8(c.r(3)+4,c.u8(c.r(4)+4)+c.r(5));
        return c.r(3);
    };
    fill_words(c,1);set_salt(c,0);
    c.put16(profile+3*0x20+8,5000);c.put16(profile+3*0x20+0xa,7000);c.put16(profile+3*0x20+0xc,9000);
    traits(c,2,2);
}
}
void add_absence_cases(std::vector<Case>& cases) {
    using namespace world;
    using namespace absence_cases;
    constexpr unsigned records_at=records;
    const auto add_absence=[&](const std::string& name,unsigned kind,std::function<void(Cpu&)> more,unsigned club_arg=0,unsigned query=0) {
        cases.push_back({0x10006900,name,[=](Cpu& c){
            absence_services(c,query);
            c.set({{3,records_at},{4,staff(3)},{5,club_arg},{6,kind}});
            more(c);
        }});
    };
    const auto both=[&](const std::string& name,std::function<void(Cpu&)> more,unsigned club_arg=0,unsigned query=0) {
        for(unsigned kind:{4u,1u,7u})add_absence(name+"-kind"+std::to_string(kind),kind,more,club_arg,query);
    };
    // Guards.
    add_absence("null-staff",4,[](Cpu& c){c.set(4,0);});
    add_absence("no-profile",4,[](Cpu& c){c.put32(staff(3)+0x14,0);});
    for(unsigned id:{16u,17u,0x7fffffffu,15u,0u,0xffffffffu,0xfffffff0u})
        add_absence("id-"+std::to_string(id),4,[=](Cpu& c){
            c.put32(staff(3),id);if(id>=0xfffffff0u){c.put8(flags+int(id),0);}
        });
    for(unsigned flag:{0u,1u,2u,3u,0x80u,0x81u,0xfeu,0xffu})
        add_absence("flag-"+std::to_string(flag),4,[=](Cpu& c){c.put8(flags+3,flag);});
    // Staff without a club, with and without a reporting club, every kind.
    for(unsigned kind:{0u,1u,2u,3u,4u,5u,0x7fu,0x80u,0xffu,0x101u,0x104u,0x1ffu,0xffffff04u,0x80000001u,0xffffffffu})
        for(unsigned reporting:{0u,1u}) {
            add_absence("no-club-kind"+std::to_string(kind)+"-reporting"+std::to_string(reporting),kind,[](Cpu& c){c.put32(staff(3)+0x24,0);},reporting?club(5):0);
            add_absence("club-kind"+std::to_string(kind)+"-reporting"+std::to_string(reporting),kind,[](Cpu&){},reporting?club(5):0);
        }
    // The club query's answer (only its low byte counts).
    for(unsigned answer:{0u,1u,0x100u,0x80u,0xffu,0x7f00u,0xdeadbe00u})
        both("query-"+std::to_string(answer),[](Cpu&){},0,answer);
    // Peer slots: none, empty and self among them, all fifty.
    both("no-peers",[](Cpu& c){peers(c,club(3),{});});
    both("self-first",[](Cpu& c){peers(c,club(3),{staff(3),staff(5)});});
    both("empty-first",[](Cpu& c){peers(c,club(3),{0,staff(5),0,staff(3)});});
    both("full-roster",[](Cpu& c){
        std::vector<unsigned> all;for(unsigned slot=0;slot<50;++slot)all.push_back(slot%7==3?(slot%2?0:staff(3)):staff((slot+4)%16!=3?(slot+4)%16:5));
        peers(c,club(3),all);
    });
    both("last-slot",[](Cpu& c){std::vector<unsigned> all(49,0);all.push_back(staff(6));peers(c,club(3),all);});
    // Each combination of the two trait rolls for one peer, in both signs.
    for(unsigned a:{2u,0xfeu})for(unsigned b:{2u,0xfeu})for(unsigned first:{2u,3u})for(unsigned second:{2u,3u})
        for(unsigned kind:{4u,1u})
            add_absence("peer-"+std::to_string(a)+"-"+std::to_string(b)+"-"+std::to_string(first)+"-"+std::to_string(second)+"-kind"+std::to_string(kind),kind,[=](Cpu& c){
                peers(c,club(3),{staff(5)});traits(c,a,b);
                // Draws 1-3 are the rating cuts; 4 and 5 the peer's rolls; 6 the duration.
                put_words(c,{11,12,13,first,second,17});
            });
    // Trait values: zero, one, extremes, in each position.
    for(unsigned a:{0u,1u,0x7fu,0x80u,0xffu,0xa5u})for(unsigned b:{0u,1u,0x7fu,0x80u,0xffu,0xa5u})
        add_absence("traits-"+std::to_string(a)+"-"+std::to_string(b),4,[=](Cpu& c){
            peers(c,club(3),{staff(5),staff(6),staff(7)});traits(c,a,b);
        });
    // Pool contents, salts and kinds: signs, remainders, durations.
    for(unsigned seed:{1u,2u,3u,7u,0x55u,0xffffu})
        for(unsigned salt:{0u,1u,40u,41u,0x5a5au,0xffffu,0x7fffffffu,0x80000000u,0xffffffffu,0xfffffff7u,0x80000029u})
            for(unsigned kind:{4u,1u})
                add_absence("pool-"+std::to_string(seed)+"-"+std::to_string(salt)+"-kind"+std::to_string(kind),kind,[=](Cpu& c){
                    fill_words(c,seed);set_salt(c,salt);peers(c,club(3),{staff(5),staff(6)});traits(c,3,5);
                });
    for(unsigned word:{0u,1u,0x7fffu,0x8000u,0xffffu})for(unsigned salt:{0u,0x7fffffffu,0x80000000u,0xffffffffu,0x1fu,0x96u,0xffffffe1u})
        for(unsigned kind:{4u,1u})
            add_absence("word-"+std::to_string(word)+"-"+std::to_string(salt)+"-kind"+std::to_string(kind),kind,[=](Cpu& c){
                for(unsigned n=0;n<=64;++n)c.put16(table(c)+n*2,word);set_salt(c,salt);
            });
    // Rating values (16 bits, signed) around the cuts.
    for(unsigned rating:{0u,1u,0xffffu,0x7fffu,0x8000u,100u,0xff9cu,41u,40u})for(unsigned salt:{0u,13u,0xffffffe0u,0x80000000u})
        add_absence("rating-"+std::to_string(rating)+"-"+std::to_string(salt),4,[=](Cpu& c){
            for(unsigned f:{8u,0xau,0xcu})c.put16(profile+3*0x20+f,rating);set_salt(c,salt);
        });
    // The cursor wraps at each draw; the reseed answers with its low 16 bits.
    for(unsigned j=1;j<=10;++j)for(unsigned kind:{4u,1u})for(unsigned reseeded:{0x1234u,0x12345678u,0xfffffu})
        add_absence("wrap-"+std::to_string(j)+"-kind"+std::to_string(kind)+"-"+std::to_string(reseeded),kind,[=](Cpu& c){
            peers(c,club(3),{staff(5),staff(6)});put_words(c,{11,12,13,3,2,5,3,7});script.reseed=reseeded;wrap_at(c,j);
        });
    // A wrap in the loop of a full roster.
    for(unsigned j=20;j<=60;j+=7)
        add_absence("wrap-full-"+std::to_string(j),4,[=](Cpu& c){
            std::vector<unsigned> all;for(unsigned slot=0;slot<50;++slot)all.push_back(staff(4+slot%12));
            peers(c,club(3),all);wrap_at(c,j);
        });
    // The date the record stores.
    for(unsigned year:{0u,0x7fffu,0x8000u,0xffffu})for(unsigned month:{0u,0xffffu})for(unsigned day:{0u,0xffu})
        add_absence("date-"+std::to_string(year)+"-"+std::to_string(month)+"-"+std::to_string(day),4,[=](Cpu& c){
            const auto today=c.toc(-0x7b80);c.put16(today,year);c.put16(today+2,month);c.put8(today+4,day);
        });
    // The dependencies change the world between the calls the original rereads.
    for(unsigned n=1;n<=4;++n)for(unsigned kind:{4u,1u})for(unsigned change:{0u,1u,2u})
        add_absence("change-after-relation-"+std::to_string(n)+"-"+std::to_string(change)+"-kind"+std::to_string(kind),kind,[=](Cpu& c){
            peers(c,club(3),{staff(5),staff(6),staff(7)});peers(c,club(4),{staff(8),staff(5)});
            const auto count=std::make_shared<unsigned>(0);
            script.services[relation]=[=](Cpu& c) {
                if(++*count==n) {
                    if(change==0)c.put32(staff(3)+0x24,club(4));
                    if(change==1)c.put8(staff(5)+0x65,0);
                    if(change==2){c.put32(staff(3)+0x14,profile+4*0x20);c.put32(staff(3),5);}
                }
                return 0u;
            };
        });
    for(unsigned answer:{0u,1u})for(unsigned kind:{4u,1u})
        add_absence("change-after-query-"+std::to_string(answer)+"-kind"+std::to_string(kind),kind,[=](Cpu& c){
            peers(c,club(4),{staff(8),staff(5)});
            script.services[club_query]=[=](Cpu& c){c.put32(staff(3)+0x24,club(4));return answer;};
        });
    add_absence("change-by-date",4,[](Cpu&){
        script.services[date_add]=[](Cpu& c){c.put32(staff(3)+0x24,club(4));c.put32(c.r(3),0x01020304);return 0u;};
    });

    // notify_training.
    const auto add_training=[&](const std::string& name,std::function<void(Cpu&)> more,unsigned first=0,unsigned second=0,unsigned posted=0) {
        cases.push_back({0x10008770,name,[=](Cpu& c){
            script.services[club_query]=[=](Cpu& c){return c.r(4)==0?first:second;};
            script.services[message_post]=[=](Cpu&){return posted;};
            script.services[message_staff]=[](Cpu&){return 0u;};
            fill_words(c,5);
            c.put32(club(3)+0x94,staff(7));
            for(unsigned slot=0;slot<5;++slot)c.put32(club(3)+0x160+slot*4,0);
            c.set({{3,records},{4,staff(3)},{5,club(3)}});
            more(c);
        }});
    };
    add_training("ordinary",[](Cpu&){});
    add_training("null-staff",[](Cpu& c){c.set(4,0);});
    add_training("no-profile",[](Cpu& c){c.put32(staff(3)+0x14,0);});
    add_training("staff-club-cleared-with-post",[](Cpu& c){c.put32(staff(3)+0x24,0);script.services[message_post]=[](Cpu&){return 1u;};});
    cases.push_back({0x10008770,"null-club",[](Cpu& c){absence_services(c);c.set({{3,records},{4,staff(3)},{5,0}});}});
    add_training("club-without-second",[](Cpu& c){c.put32(club(3)+0x90,0);});
    add_training("second-cleared-by-message",[](Cpu&){
        script.services[message_new]=[](Cpu& c){c.put32(club(3)+0x90,0);return 0u;};
    });
    // The first query answer is a field; the second decides the manager.
    for(unsigned first:{0u,1u,0x80u,0xffu,0x100u})for(unsigned second:{0u,1u,0x80u,0x100u,0x7fu,0xff00u})
        add_training("query-"+std::to_string(first)+"-"+std::to_string(second),[](Cpu&){},first,second);
    // Field 5: the drawn word modulo three, signs of the sum.
    for(unsigned salt:{0u,1u,2u,3u,0x5a5au,0x7fffffffu,0x80000000u,0xffffffffu,0xfffffffdu})for(unsigned word:{0u,1u,2u,0x7fffu,0xffffu})
        add_training("field5-"+std::to_string(salt)+"-"+std::to_string(word),[=](Cpu& c){
            set_salt(c,salt);for(unsigned n=0;n<=64;++n)c.put16(table(c)+n*2,word);
        });
    // Alternate reporter: the parity draw, the second slot, the coach loop.
    for(unsigned salt:{0u,1u,0x80000000u,0x80000001u,0xffffffffu,0xfffffffeu})for(unsigned parity:{0u,1u,2u,3u})
        for(unsigned second:{0u,1u})for(unsigned third:{0u,1u,2u})
            add_training("alternate-"+std::to_string(salt)+"-"+std::to_string(parity)+"-"+std::to_string(second)+"-"+std::to_string(third),[=](Cpu& c){
                set_salt(c,salt);put_words(c,{1,parity,parity,parity+2});
                c.put32(club(3)+0x94,third==0?0:staff(third==1?7:9));
                // A coach in each slot.
                for(unsigned slot=0;slot<5;++slot)c.put32(club(3)+0x160+slot*4,staff(10+slot));
                c.put32(club(3)+0x94,third==0?0:staff(third==1?7:9));
            },second,second);
    // The coach loop: empty slots until a filled one, 1000 attempts, a negative remainder.
    for(unsigned filled=0;filled<5;++filled)for(unsigned wait:{0u,1u,5u})
        add_training("coach-slot-"+std::to_string(filled)+"-after-"+std::to_string(wait),[=](Cpu& c){
            set_salt(c,0);
            // Draw 1 is field 5, draw 2 the parity (odd: coaches); then the slots.
            std::vector<unsigned> words{0,1};for(unsigned n=0;n<wait;++n)words.push_back((filled+1)%5);
            words.push_back(filled);words.push_back(0);
            put_words(c,words);
            c.put32(club(3)+0x160+filled*4,staff(10+filled));
        },0,1);
    add_training("coach-none",[](Cpu& c){set_salt(c,0);put_words(c,{0,1});},0,1);
    add_training("coach-none-reseeded",[](Cpu& c){set_salt(c,0);put_words(c,{0,1});script.reseed=0x12345678;},0,1);
    add_training("coach-at-attempt-1000",[](Cpu& c){
        set_salt(c,4);
        for(unsigned n=0;n<=64;++n)c.put16(table(c)+n*2,5*(n+1));
        c.put16(table(c)+1*2,5);c.put16(table(c)+27*2,6);
        c.put32(club(3)+0x160+1*4,staff(12));
        // The table repeats every 65 draws; the 15th reseed (the 975th draw) turns
        // the salt so that only the word at index 27, draw 1002, lands on slot 1.
        const auto reseeds=std::make_shared<unsigned>(0);
        script.services[reseed_routine]=[=](Cpu&){return ++*reseeds<15?2u:0u;};
    },0,1);
    add_training("coach-last-attempt-empty",[](Cpu& c){
        set_salt(c,4);
        for(unsigned n=0;n<=64;++n)c.put16(table(c)+n*2,5*(n+1));
        c.put16(table(c)+1*2,5);
        script.services[reseed_routine]=[](Cpu&){return 0u;};
    },0,1);
    for(unsigned salt:{0x80000000u,0xfffffff8u,0xfffffffeu})for(unsigned slot:{0u,1u,2u,3u,4u}) {
        // A negative sum gives a negative slot, which reads the club's peer table.
        add_training("coach-negative-"+std::to_string(salt)+"-"+std::to_string(slot),[=](Cpu& c){
            set_salt(c,salt);put_words(c,{0x0003,0x0001,slot});
            for(unsigned s=0;s<5;++s)c.put32(club(3)+0x15c-s*4,s==slot?staff(11):0);
            c.put32(club(3)+0x160+slot*4,staff(12));
        },0,1);
    }
    add_training("coach-id-minus-one",[](Cpu& c){
        set_salt(c,0);put_words(c,{0,1,0});c.put32(club(3)+0x160,staff(13));c.put32(staff(13),0xffffffff);
    },0,1);
    add_training("coach-id-zero",[](Cpu& c){
        set_salt(c,0);put_words(c,{0,1,0});c.put32(club(3)+0x160,staff(13));c.put32(staff(13),0);
    },0,1);
    add_training("second-slot-id-minus-one",[](Cpu& c){
        set_salt(c,0);put_words(c,{0,0});c.put32(club(3)+0x94,staff(13));c.put32(staff(13),0xffffffff);
    },1,1);
    // Wraps at each draw (field 5, the parity, then three coach attempts).
    for(unsigned j=1;j<=5;++j)for(unsigned parity:{0u,1u})
        add_training("wrap-"+std::to_string(j)+"-"+std::to_string(parity),[=](Cpu& c){
            set_salt(c,0);put_words(c,{0,parity,3,2,1});
            for(unsigned slot=0;slot<5;++slot)c.put32(club(3)+0x160+slot*4,slot==1?staff(12):0);
            script.reseed=0xabcd;wrap_at(c,j);
        },1,1);
    // Sending: the post's answer, the manager, the profile threshold.
    for(unsigned posted:{0u,1u,0x100u,0x80u})for(unsigned manager:{0u,1u})for(unsigned level:{0x1c52u,0x1c53u,0x1c51u,0x8000u,0x7fffu,0u})
        add_training("send-"+std::to_string(posted)+"-"+std::to_string(manager)+"-"+std::to_string(level),[=](Cpu& c){
            c.put32(club(3)+0x2c,manager?c.u32(club(3)+0x2c):0);c.put16(profile+3*0x20+0xa,level);
        },0,0,posted);
    // The message setters change the world the next reads depend on.
    for(unsigned field=0;field<=10;++field)for(unsigned what:{0u,1u}) {
        // Clearing the second slot under the manager's fields would fault in the original.
        if(what==0 && field>=7 && field<10)continue;
        add_training("setter-"+std::to_string(field)+"-"+std::to_string(what),[=](Cpu&){
            script.services[message_set]=[=](Cpu& c){
                if(c.r(4)==field) {
                    if(what==0)c.put32(club(3)+0x90,0);
                    else c.put32(club(3)+0x2c,0);
                }
                return 0u;
            };
        });
    }
}

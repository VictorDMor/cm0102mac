// Cases of the AWOL checker for awol_discipline.cpp.
namespace discipline_cases {
using namespace world;
constexpr unsigned training=0x10007a50,fitness=0x100090a0;
constexpr unsigned contract_of=0x10082f20,club_query=0x1014c2e0,message_post=0x102cdbf0,manager_value_of=0x100d2f50,report_out=0x1010a080,
    clock_of=0x101878d0,fitness_out=0x10186a20,staff_notice=0x102cc140,reseed_routine=0x1077b770,relation=0x100d9b80,date_add=0x10717890;
constexpr unsigned record_table=data+0x2e000,managers=data+0x2f800;
unsigned boss(unsigned id){return managers+id*0x80;}
unsigned pool_base(Cpu& c){return c.toc(-0x7b8c);}
unsigned pool_end(Cpu& c){return c.u32(c.toc(-0x7b88));}
// Words for the draws that follow: draw k (before any wrap) reads word k.
struct Words {
    std::vector<unsigned> list{0};
    // A draw on `bound` that comes out as `want` (zero bounds draw nothing).
    Words& draw(int bound,int want){if(bound)list.push_back(unsigned(want<0?-want:want));return *this;}
    Words& raw(unsigned word){list.push_back(word);return *this;}
};
// The world of both events. Each field is one thing a branch looks at.
struct Rig {
    int mood=-120;unsigned flags=0,flag_byte=0,record_byte=0x20;
    int trait_a=-10,trait_b=-10;            // staff+0x67, staff+0x65
    int ability=21;                          // profile+0x34
    int manager_trait=0;                     // manager+0x34
    unsigned manager_value=0,clock=0,query=0,posted=0;
    std::vector<unsigned> contracts;         // successive answers of the contract lookup
    bool reporter[5]={true,true,true,true,true};
    std::vector<unsigned> words;unsigned salt=0;
    unsigned wrap_at=0;                      // the j-th pool advance wraps
    bool fill_constant=false;unsigned constant=0;
    std::function<void(Cpu&)> after;         // memory changes once the world is built
    void plan(const Words& w){words=w.list;}
};
// The discipline words: bare advance (parity word, X only), first draw on the
// manager trait, gate word, second draw on the value, last word.
Words discipline_words(bool with_parity,int trait,int value,int want1,unsigned gate,int want2,unsigned last) {
    Words w;if(with_parity)w.raw(6);
    w.draw(trait,want1).raw(gate).draw(int(std::int8_t(value)),want2).raw(last);
    return w;
}
void install(Cpu& c,const Rig& r) {
    c.put32(c.toc(-0x7b9c),record_table);
    for(unsigned id=0;id<staff_count;++id) {
        c.put32(staff(id)+0x1c,boss(id));
        c.put8(boss(id)+0x34,r.manager_trait);c.put8(boss(id)+0x39,0x11);
        c.put8(staff(id)+0x65,r.trait_b);c.put8(staff(id)+0x67,r.trait_a);
    }
    for(unsigned id=0;id<club_count;++id)for(unsigned k=0;k<5;++k)c.put32(club(id)+0x160+4*k,r.reporter[k]?staff((id+k)%staff_count):0);
    for(unsigned id=0;id<club_count;++id)c.put32(club(id)+0x94,staff((id+2)%staff_count));
    c.put8(contract+0x4c,r.mood);c.put32(contract+0x50,r.flags);
    script.contract=contract;script.contracts=r.contracts;
    c.put8(flags+3,r.flag_byte);
    c.put8(profile+3*0x20+0x34,r.ability);c.put16(profile+3*0x20+8,5000);c.put16(profile+3*0x20+0xa,7000);c.put16(profile+3*0x20+0xc,9000);
    c.put8(record_table+3*100+0xa,r.record_byte);
    if(r.fill_constant)for(unsigned n=0;n<=70;++n)c.put16(pool_base(c)+n*2,r.constant);
    for(unsigned n=1;n<r.words.size();++n)c.put16(pool_base(c)+n*2,r.words[n]);
    c.put32(c.toc(-0x7b90),r.salt);
    if(r.wrap_at)c.put32(c.toc(-0x7b84),pool_end(c)+2-2*r.wrap_at);
    script.services[relation]=[](Cpu&){return 0u;};
    script.services[date_add]=[](Cpu& c) {
        c.put16(c.r(3),c.u16(c.r(4))+c.r(5)*3);c.put16(c.r(3)+2,c.u16(c.r(4)+2)^c.r(5));c.put8(c.r(3)+4,c.u8(c.r(4)+4)+c.r(5));
        return c.r(3);
    };
    script.services[club_query]=[=](Cpu&){return r.query;};
    script.services[message_post]=[=](Cpu&){return r.posted;};
    script.services[manager_value_of]=[=](Cpu&){return r.manager_value;};
    script.services[clock_of]=[=](Cpu&){return r.clock;};
    script.services[report_out]=[](Cpu&){return 0u;};
    script.services[fitness_out]=[](Cpu&){return 0u;};
    script.services[staff_notice]=[](Cpu&){return 0u;};
    if(r.after)r.after(c);
}
}
void add_discipline_cases(std::vector<Case>& cases) {
    using namespace world;
    using namespace discipline_cases;
    // One training case: a rig, then the event's arguments.
    const auto train=[&](const std::string& name,std::function<void(Rig&)> shape,unsigned staff_at=0) {
        cases.push_back({training,name,[=](Cpu& c){
            Rig r;shape(r);install(c,r);c.set({{3,records},{4,staff_at?staff_at:staff(3)}});
        }});
    };
    const auto fitness_case=[&](const std::string& name,std::function<void(Rig&)> shape,unsigned staff_at=0,unsigned club_at=0) {
        cases.push_back({fitness,name,[=](Cpu& c){
            Rig r;shape(r);install(c,r);c.set({{3,records},{4,staff_at?staff_at:staff(3)},{5,club_at?club_at:club(1)}});
        }});
    };
    constexpr unsigned all_bits=0x2800;
    // The daily review (0x10004070) running the whole event after a training
    // absence ends, not only its early exits.
    for(unsigned mode:{0u,1u})for(unsigned bits:{0u,0x2000u,0x800u})for(unsigned salt:{0u,10u}) {
        review_world::Scene s;s.list[1]={review_world::secondary(3,mode)};s.words=review_world::noise(salt,3);s.salt=salt;
        s.tweak=[=](Cpu& c){
            Rig r;r.flags=bits;r.salt=salt;r.contracts={contract,contract,contract,contract,contract};
            // A return from leave reaches the event only when both its draws
            // (bounds -10 and 5) come out zero: words of zero with these salts.
            r.after=[=](Cpu& c){for(unsigned at=0;at<64;++at)c.put16(pool_base(c)+at*2,mode?0:(salt+at*37)%3);};
            install(c,r);
        };
        cases.push_back({0x10004070,"whole-event-"+std::to_string(mode)+"-"+std::to_string(bits)+"-"+std::to_string(salt),[=](Cpu& c){s.apply(c);c.set(3,records);}});
    }
    // ---- training_event ----
    // Guards: null staff, null profile, id bound (signed), lookups.
    cases.push_back({training,"null-staff",[](Cpu& c){Rig r;install(c,r);c.set({{3,records},{4,0}});}});
    train("null-profile",[](Rig& r){r.after=[](Cpu& c){c.put32(staff(3)+0x14,0);};});
    train("id-at-count",[](Rig& r){r.after=[](Cpu& c){c.put32(staff(staff_count),staff_count);};},staff(staff_count));
    train("id-last",[](Rig& r){r.after=[](Cpu& c){c.put32(staff(3),staff_count-1);};});
    train("id-high",[](Rig& r){r.after=[](Cpu& c){c.put32(staff(3),0x7fffffff);};});
    train("id-negative",[](Rig& r){r.after=[](Cpu& c){c.put32(staff(3),0xffffffff);c.put8(flags-1,0);};});
    train("id-negative-flagged",[](Rig& r){r.after=[](Cpu& c){c.put32(staff(3),0xffffffff);c.put8(flags-1,2);};});
    train("no-contract",[](Rig& r){r.contracts={0,contract,contract};});
    train("club-table-null",[](Rig& r){r.after=[](Cpu& c){c.put32(c.toc(-0x7be8),0);c.put32(contract+4,0);};});
    train("contract-index-other",[](Rig& r){r.after=[](Cpu& c){c.put32(contract+4,5);};});
    for(unsigned flag:{0u,1u,2u,3u,0x80u,0x81u,0x82u,0xffu,0xfdu})
        train("flag-byte-"+std::to_string(flag),[=](Rig& r){r.flag_byte=flag;});
    train("second-lookup-null",[](Rig& r){r.contracts={contract,0,contract};r.flags=all_bits;r.mood=-120;});
    train("third-lookup-null",[](Rig& r){r.contracts={contract,contract,0};});
    train("third-lookup-null-conversion-declined",[](Rig& r){
        r.contracts={contract,contract,0};r.flags=0x2000;r.trait_a=100;Words w;w.draw(110,5);r.plan(w);
    });
    // Conversion to a primary absence: mood, contract flags, trait draws.
    for(int mood:{-128,-120,-100,-99,-1,0,127})
        train("mood-"+std::to_string(mood)+"-registered",[=](Rig& r){r.mood=mood;r.flags=0x2000;});
    for(unsigned flag:{0u,0x2000u,0x800u,all_bits,0x7fffu,0x1800u,0x4000u,0xffffffffu})
        train("flags-"+std::to_string(flag),[=](Rig& r){r.flags=flag;});
    train("a-registered-second-bound-one",[](Rig& r){r.flags=0x2000;r.trait_b=-9;Words w;w.draw(1,0);r.plan(w);});
    train("a-registered-bounds-zero",[](Rig& r){r.flags=0x2000;});
    train("a-declined-first",[](Rig& r){r.flags=0x2000;r.trait_a=100;Words w;w.draw(110,5);r.plan(w);});
    train("a-declined-first-negative-bound",[](Rig& r){r.flags=0x2000;r.trait_a=-50;Words w;w.draw(-40,-3);r.plan(w);});
    train("a-declined-second",[](Rig& r){r.flags=0x2000;r.trait_a=-9;r.trait_b=100;Words w;w.draw(1,0).draw(110,7);r.plan(w);});
    train("a-declined-second-negative-bound",[](Rig& r){r.flags=0x2000;r.trait_a=-9;r.trait_b=-70;Words w;w.draw(1,0).draw(-60,-9);r.plan(w);});
    train("b-registered",[](Rig& r){r.flags=0x800;});
    train("b-registered-bound-one",[](Rig& r){r.flags=0x800;r.trait_a=-9;r.trait_b=-9;Words w;w.draw(1,0).draw(1,0);r.plan(w);});
    train("b-declined-first",[](Rig& r){r.flags=0x800;r.trait_a=100;Words w;w.draw(110,3);r.plan(w);});
    train("b-declined-second",[](Rig& r){r.flags=0x800;r.trait_a=-9;r.trait_b=100;Words w;w.draw(1,0).draw(110,4);r.plan(w);});
    train("both-a-registered",[](Rig& r){r.flags=all_bits;});
    train("a-declined-b-registered",[](Rig& r){
        r.flags=all_bits;r.trait_a=-9;r.trait_b=-7;Words w;w.draw(1,0).draw(3,2).draw(1,0).draw(3,0);r.plan(w);});
    train("a-declined-b-declined-first",[](Rig& r){r.flags=all_bits;r.trait_a=100;Words w;w.draw(110,5).draw(110,6);r.plan(w);});
    train("a-declined-b-declined-second",[](Rig& r){
        r.flags=all_bits;r.trait_a=-9;r.trait_b=-7;Words w;w.draw(1,0).draw(3,2).draw(1,0).draw(3,1);r.plan(w);});
    // The reread of the contract's flags after a draw (the reseed service clears them).
    train("flags-cleared-during-a",[](Rig& r){
        r.flags=all_bits;r.trait_a=100;r.wrap_at=1;
        Words w;w.draw(110,5).draw(110,6);r.plan(w);
        r.after=[](Cpu&){script.services[reseed_routine]=[](Cpu& c){c.put32(contract+0x50,0);return 0x1234u;};};
    });
    train("mood-changed-during-a",[](Rig& r){
        r.flags=all_bits;r.trait_a=100;r.wrap_at=1;
        r.after=[](Cpu&){script.services[reseed_routine]=[](Cpu& c){c.put8(contract+0x4c,0x7f);return 0x1234u;};};
    });
    train("b-flags-cleared-during-b",[](Rig& r){
        r.flags=0x800;r.trait_a=100;r.wrap_at=1;
        r.after=[](Cpu&){script.services[reseed_routine]=[](Cpu& c){c.put32(contract+0x50,0x2000);return 0x1234u;};};
    });
    // The training flag and the fitness cost: ability, clock, record byte wrap,
    // digit. notify_training's field-5 draw takes the first word, hence the
    // leading raw(0) in these plans.
    for(unsigned byte:{0x20u,0xf8u,0xf6u,0xffu,0u})for(unsigned digit:{0u,3u,9u,17u,29u})
        train("byte-"+std::to_string(byte)+"-digit-"+std::to_string(digit),[=](Rig& r){r.record_byte=byte;Words w;w.raw(0).raw(digit);r.plan(w);});
    for(int ability:{21,22,20,0,127,-128,-1,100,50})
        for(unsigned clock:{0u,100u,0x7fffu,0x8000u,0x18000u,0xffff8000u,0xffffffffu}) {
            train("ability-"+std::to_string(ability)+"-clock-"+std::to_string(clock),[=](Rig& r){
                r.ability=ability;r.clock=clock;
                const int bound=int(std::int16_t((21-ability)*25));Words w;w.raw(0).raw(4).draw(bound,bound>0?bound/2:bound/3);r.plan(w);
            });
        }
    train("salt-negative-digit",[](Rig& r){r.salt=0xffffffff;Words w;w.raw(0).raw(0);r.plan(w);});
    train("salt-large",[](Rig& r){r.salt=0x80000001;Words w;w.raw(0).raw(7);r.plan(w);});
    // The id is read again after these calls.
    train("id-changed-by-lookup",[](Rig& r){r.after=[](Cpu&){script.services[contract_of]=[count=0u](Cpu& c) mutable {if(++count==3)c.put32(staff(3),5);return contract;};};});
    train("id-changed-by-notice",[](Rig& r){r.after=[](Cpu&){script.services[club_query]=[](Cpu& c){c.put32(staff(3),6);return 0u;};};});
    train("id-changed-by-clock",[](Rig& r){r.after=[](Cpu&){script.services[clock_of]=[](Cpu& c){c.put32(staff(3),7);return 0x40u;};};});
    train("id-changed-by-fitness",[](Rig& r){r.after=[](Cpu&){script.services[fitness_out]=[](Cpu& c){c.put32(staff(3),9);return 0u;};};});
    train("profile-changed-by-clock",[](Rig& r){
        r.ability=21;
        r.after=[](Cpu& c){c.put8(profile+5*0x20+0x34,52);script.services[clock_of]=[](Cpu& c){c.put32(staff(3)+0x14,profile+5*0x20);return 0x40u;};};
    });
    // Discipline: the club query, the manager chain, both draws, both actions.
    for(unsigned answer:{0u,1u,0x100u,0x80u,0xffu,0x7f00u,0xdeadbe00u,0xdeadbe01u})
        train("query-"+std::to_string(answer),[=](Rig& r){r.query=answer;r.manager_trait=5;r.manager_value=7;Words w;w.raw(0).raw(0).draw(5,3).draw(7,4).raw(2);r.plan(w);});
    train("no-manager-slot",[](Rig& r){r.after=[](Cpu& c){c.put32(club(1)+0x90,0);};});
    train("manager-without-record",[](Rig& r){r.after=[](Cpu& c){c.put32(staff(1)+0x1c,0);};});
    train("manager-trait-zero",[](Rig& r){r.manager_trait=0;});
    for(int trait:{1,-1,5,-5,127,-128,70,-100})
        for(int want:{0,2}) {
            train("manager-trait-"+std::to_string(trait)+"-want-"+std::to_string(want),[=](Rig& r){
                r.manager_trait=trait;r.manager_value=3;
                Words w;w.raw(0).raw(1).draw(trait,trait>0?(want%trait):(want?-(want%(-trait)):0)).draw(3,1).raw(0);r.plan(w);
            });
        }
    for(unsigned value:{0u,1u,0xffu,0x7fu,0x80u,0x181u,0xffffff85u,3u,0x100u}) {
        for(unsigned last:{0u,2u,4u,9u}) {
            train("value-"+std::to_string(value)+"-last-"+std::to_string(last),[=](Rig& r){
                r.manager_trait=5;r.manager_value=value;
                const int bound=int(std::int8_t(value));
                Words w;w.raw(0).raw(1).draw(5,3).draw(bound,bound>1?bound-1:bound<-1?bound+1:0).raw(last);r.plan(w);
            });
        }
    }
    for(unsigned last:{0u,1u,2u,3u,4u,5u,0xffffu})for(int want:{0,1,2,4}) {
        train("final-"+std::to_string(last)+"-"+std::to_string(want),[=](Rig& r){
            r.manager_trait=5;r.manager_value=5;Words w;w.raw(0).raw(1).draw(5,2).draw(5,want).raw(last);r.plan(w);
        });
    }
    train("final-negative-sum",[](Rig& r){r.manager_trait=5;r.manager_value=5;r.salt=0xffffffff;Words w;w.raw(0).raw(1).draw(5,2).draw(5,1).raw(0);r.plan(w);});
    train("final-negative-sum-big",[](Rig& r){r.manager_trait=5;r.manager_value=5;r.salt=0x80000000;Words w;w.raw(0).raw(1).draw(5,2).draw(5,1).raw(9);r.plan(w);});
    train("manager-changed-by-reseed",[](Rig& r){
        r.manager_trait=5;r.manager_value=7;r.fill_constant=true;r.constant=1;r.wrap_at=3;
        r.after=[](Cpu& c){c.put8(boss(2)+0x39,0x22);script.services[reseed_routine]=[](Cpu& c){c.put32(staff(1)+0x1c,boss(2));return 0x1234u;};};
    });
    train("manager-chain-cleared-late",[](Rig& r){
        r.manager_trait=5;r.manager_value=7;Words w;w.raw(0).raw(1).draw(5,3);r.plan(w);
        r.after=[](Cpu&){script.services[manager_value_of]=[](Cpu& c){c.put32(club(1)+0x90,staff(2));return 3u;};};
    });
    // The club query clears the club (the manager reads follow it).
    train("club-query-moves-manager",[](Rig& r){
        r.manager_trait=5;r.manager_value=7;Words w;w.raw(1).draw(5,3).draw(7,5).raw(1);r.plan(w);
        r.after=[](Cpu&){script.services[club_query]=[](Cpu& c){c.put32(club(1)+0x90,staff(2));return 0u;};};
    });
    // Pool wraps at every advance, with three shapes of path.
    for(unsigned j=1;j<=13;++j) {
        for(const unsigned shape:{0u,1u,2u,3u,4u}) {
            train("wrap-"+std::to_string(j)+"-shape-"+std::to_string(shape),[=](Rig& r){
                r.wrap_at=j;r.fill_constant=true;r.constant=7+shape;
                r.manager_trait=5+int(shape);r.manager_value=6+shape;r.ability=shape==2?3:21;
                r.clock=shape*50;
                switch(shape) {
                case 0:r.flags=0x2000;r.trait_a=-9;r.trait_b=-9;break;           // registered at A
                case 1:r.flags=0x800;r.trait_a=-9;r.trait_b=-9;break;            // registered at B
                case 2:r.flags=all_bits;r.trait_a=100;r.trait_b=100;break;       // declined at A1 and B1
                case 3:r.flags=all_bits;r.trait_a=-9;r.trait_b=100;break;        // declined at A2 and B2
                default:break;                                                    // no conversion
                }
            });
        }
    }
    // Fixtures with the real pool words: nothing planned.
    for(unsigned seed=0;seed<6;++seed)
        train("pool-seed-"+std::to_string(seed),[=](Rig& r){
            r.flags=seed&1?all_bits:0;r.trait_a=seed<3?100:-9;r.trait_b=seed<3?-60:100;r.manager_trait=seed+3;r.manager_value=seed*9+1;r.ability=int(seed*30)-60;
            r.salt=seed*0x2345;r.record_byte=0xf0+seed;
        });

    // ---- fitness_event ----
    cases.push_back({fitness,"null-staff",[](Cpu& c){Rig r;install(c,r);c.set({{3,records},{4,0},{5,club(1)}});}});
    cases.push_back({fitness,"null-club",[](Cpu& c){Rig r;install(c,r);c.set({{3,records},{4,staff(3)},{5,0}});}});
    cases.push_back({fitness,"both-null",[](Cpu& c){Rig r;install(c,r);c.set({{3,records},{4,0},{5,0}});}});
    fitness_case("club-other",[](Rig& r){r.posted=0x100;},0,club(5));
    // Y: the club was not told before (posted low byte 0).
    for(unsigned posted:{0u,0x100u,0xdeadbe00u,0x7f00u})
        for(unsigned answer:{0u,1u,0x100u,0x80u,0xdeadbe00u})
            fitness_case("y-posted-"+std::to_string(posted)+"-query-"+std::to_string(answer),[=](Rig& r){
                r.posted=posted;r.query=answer;r.manager_trait=5;r.manager_value=7;r.plan(discipline_words(false,5,7,3,2,4,1));
            });
    fitness_case("y-no-manager-slot",[](Rig& r){r.after=[](Cpu& c){c.put32(club(1)+0x90,0);};});
    fitness_case("y-manager-without-record",[](Rig& r){r.after=[](Cpu& c){c.put32(staff(1)+0x1c,0);};});
    for(int trait:{0,1,-1,5,-5,127,-128,70,-100})
        for(int want:{0,2}) {
            fitness_case("y-trait-"+std::to_string(trait)+"-want-"+std::to_string(want),[=](Rig& r){
                r.manager_trait=trait;r.manager_value=3;
                r.plan(discipline_words(false,trait,3,trait>0?(want%trait):(want?-(want%(-trait)):0),1,1,0));
            });
        }
    for(unsigned gate:{0u,1u,5u,9u,10u,19u})for(int want:{0,1,4,5,8,-3}) {
        fitness_case("y-gate-"+std::to_string(gate)+"-"+std::to_string(want),[=](Rig& r){
            r.manager_trait=9;r.manager_value=4;r.plan(discipline_words(false,9,4,want<0?-(-want%9):want%9,gate,1,2));
        });
    }
    for(unsigned value:{0u,1u,0xffu,0x7fu,0x80u,0x181u,0xffffff85u,3u,0x100u})
        for(unsigned last:{0u,2u,4u,9u}) {
            fitness_case("y-value-"+std::to_string(value)+"-last-"+std::to_string(last),[=](Rig& r){
                r.manager_trait=5;r.manager_value=value;const int bound=int(std::int8_t(value));
                r.plan(discipline_words(false,5,int(value),4,0,bound>1?bound-1:bound<-1?bound+1:0,last));
            });
        }
    for(unsigned last:{0u,1u,2u,3u,4u,5u,0xffffu})for(int want:{0,1,2,4}) {
        fitness_case("y-final-"+std::to_string(last)+"-"+std::to_string(want),[=](Rig& r){
            r.manager_trait=5;r.manager_value=5;r.plan(discipline_words(false,5,5,4,0,want,last));
        });
    }
    fitness_case("y-negative-sums",[](Rig& r){r.manager_trait=5;r.manager_value=5;r.salt=0xffffffff;r.plan(discipline_words(false,5,5,4,0,1,0));});
    fitness_case("y-big-salt",[](Rig& r){r.manager_trait=5;r.manager_value=5;r.salt=0x80000000;r.plan(discipline_words(false,5,5,4,0,1,9));});
    // The reseed lands on the gate's word and its salt lets the gate pass, so
    // the manager is read again after it.
    fitness_case("y-manager-changed-by-reseed",[](Rig& r){
        r.manager_trait=5;r.manager_value=7;r.fill_constant=true;r.constant=1;r.wrap_at=2;
        r.after=[](Cpu& c){c.put8(boss(2)+0x39,0x22);script.services[reseed_routine]=[](Cpu& c){c.put32(staff(1)+0x1c,boss(2));return 0x1233u;};};
    });
    // X: the club was told before (posted low byte nonzero): a reporter is
    // searched for. With salt 0: an iteration reads two words, the slot word
    // (slot = w%5) and the grade word (grade = w%20; 0 stops, else a reporter
    // must exist at the slot). Every case ends its loop by construction.
    // The loop words are followed by the discipline words.
    const auto reporting=[&](const std::string& name,std::vector<unsigned> loop,std::function<void(Rig&)> more=[](Rig&){},
                             Words rest=discipline_words(true,5,7,3,1,4,2),unsigned posted=1) {
        fitness_case("x-"+name,[=](Rig& r){
            r.posted=posted;r.manager_trait=5;r.manager_value=7;
            Words w;for(const auto word:loop)w.raw(word);
            for(unsigned n=1;n<rest.list.size();++n)w.raw(rest.list[n]);
            r.plan(w);more(r);
        });
    };
    const auto nobody=[](Rig& r){for(auto& x:r.reporter)x=false;};
    reporting("first-slot",{2,7});
    reporting("slot-1",{1,7});
    reporting("slot-4",{4,19});
    reporting("slot-by-five",{9,7});
    reporting("grade-zero-reporter",{3,20});
    reporting("grade-zero-no-reporter",{3,20},nobody);
    reporting("grade-zero-slot-null",{1,40},[](Rig& r){r.reporter[1]=false;});
    reporting("retry-once",{1,3,2,7},[](Rig& r){r.reporter[1]=false;});
    reporting("retry-twice",{1,3,1,3,3,7},[](Rig& r){r.reporter[1]=false;});
    reporting("retry-then-grade-zero",{1,3,1,40},[](Rig& r){r.reporter[1]=false;});
    reporting("retry-then-grade-zero-nobody",{1,3,1,40},nobody);
    reporting("retry-last-slot",{0,3,0,5,4,7},[=](Rig& r){nobody(r);r.reporter[4]=true;});
    reporting("reporter-names-shared",{2,7},[](Rig& r){r.after=[](Cpu& c){c.put32(staff(3)+0x28,staff(0)+0x28);};});
    const auto grade_zero_word=[](unsigned salt) {
        for(unsigned w=0;w<64;++w)if(std::int32_t(salt+w)%20==0)return w;
        return 0u;
    };
    for(unsigned salt:{1u,0xffffu,0xfffffff0u,0x80000000u,0xffffffffu,0x7fffffffu,0x7ffffff0u,0xfffffffbu})
        for(unsigned seed:{0u,1u,2u})
            reporting("salt-"+std::to_string(salt)+"-"+std::to_string(seed),{seed*3+1,grade_zero_word(salt)},[=](Rig& r){
                r.salt=salt;if(seed==1)r.reporter[2]=false;if(seed==2)for(auto& x:r.reporter)x=false;
            },discipline_words(true,5,7,3,1,4,2));
    for(unsigned posted:{1u,0x80u,0xff01u,0xdeadbe01u})
        reporting("posted-"+std::to_string(posted),{2,7},[](Rig&){},discipline_words(true,5,7,3,1,4,2),posted);
    // The parity word and its sign.
    for(unsigned parity:{0u,1u,2u,3u}) {
        Words w;w.raw(parity).draw(5,3).raw(1).draw(7,4).raw(2);
        reporting("parity-"+std::to_string(parity),{2,7},[](Rig&){},w);
    }
    for(unsigned parity:{0u,1u,2u,3u}) {
        Words w;w.raw(parity).draw(5,3).raw(1).draw(7,4).raw(2);
        reporting("parity-negative-"+std::to_string(parity),{2,grade_zero_word(0xfffffff0u)},[](Rig& r){r.salt=0xfffffff0;},w);
    }
    // The late discipline: query, chain and draws, as in Y.
    for(unsigned answer:{0u,1u,0x100u,0x80u})
        reporting("late-query-"+std::to_string(answer),{2,7},[=](Rig& r){r.query=answer;});
    reporting("late-no-manager",{2,7},[](Rig& r){r.after=[](Cpu& c){c.put32(club(1)+0x90,0);};});
    reporting("late-manager-without-record",{2,7},[](Rig& r){r.after=[](Cpu& c){c.put32(staff(1)+0x1c,0);};});
    for(int trait:{0,1,-1,5,-5,127,-128})
        reporting("late-trait-"+std::to_string(trait),{2,7},[=](Rig& r){r.manager_trait=trait;},
                  discipline_words(true,trait,3,trait>0?trait-1:trait<0?trait+1:0,1,1,0));
    for(unsigned value:{0u,1u,0x80u,0xffu,0x181u,0x100u})
        for(unsigned last:{0u,4u}) {
            const int bound=int(std::int8_t(value));
            reporting("late-value-"+std::to_string(value)+"-"+std::to_string(last),{2,7},[=](Rig& r){r.manager_value=value;},
                      discipline_words(true,5,int(value),3,0,bound>1?bound-1:bound<-1?bound+1:0,last));
        }
    for(unsigned gate:{0u,3u,9u,15u})for(unsigned last:{0u,1u,4u,9u})
        reporting("late-final-"+std::to_string(gate)+"-"+std::to_string(last),{2,7},[](Rig& r){r.manager_value=6;},
                  discipline_words(true,5,6,4,gate,3,last));
    reporting("late-manager-changed-by-reseed",{},[](Rig& r){
        r.fill_constant=true;r.constant=1;r.wrap_at=5;
        r.after=[](Cpu& c){c.put8(boss(2)+0x39,0x22);script.services[reseed_routine]=[](Cpu& c){c.put32(staff(1)+0x1c,boss(2));return 0x1233u;};};
    });
    // Wraps at every advance; constant pools, so the loop ends on the first try
    // (a reporter exists, or the constant 20 makes the grade 0).
    for(unsigned j=1;j<=16;++j) {
        for(unsigned shape=0;shape<5;++shape) {
            fitness_case("wrap-"+std::to_string(j)+"-shape-"+std::to_string(shape),[=](Rig& r){
                r.wrap_at=j;r.fill_constant=true;r.constant=shape==3?20:7+shape*2;
                r.manager_trait=5+int(shape);r.manager_value=6+shape;
                r.posted=shape<2?0:1;
                if(shape==3)for(auto& x:r.reporter)x=false;
                if(shape==4)r.salt=0xffffffff;
            });
        }
    }
    for(unsigned seed=0;seed<8;++seed)
        fitness_case("pool-seed-"+std::to_string(seed),[=](Rig& r){
            r.posted=seed&1;r.manager_trait=int(seed*17)-40;r.manager_value=seed*37;r.salt=seed*0x777;
        });
}

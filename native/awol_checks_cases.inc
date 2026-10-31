// Cases of the AWOL checker. Each prepares one state from the default world.
void file_bytes(std::vector<std::uint8_t>& out,unsigned value){for(int shift=24;shift>=0;shift-=8)out.push_back(std::uint8_t(value>>shift));}
void add_records_cases(std::vector<Case>& cases) {
    using namespace world;
    const auto pointer=[](unsigned entry,const std::string& name,unsigned r3,unsigned r4,unsigned r5=0) {
        return Case{entry,name,[=](Cpu& c){c.set({{3,r3},{4,r4},{5,r5}});}};
    };
    // construct: fresh and from a file, success and failed load.
    cases.push_back(pointer(0x100031b0,"fresh",data+0x180,0));
    const auto full_file=[](std::vector<std::uint8_t>& f,unsigned flags,unsigned a,unsigned b,unsigned p) {
        file_bytes(f,flags);for(unsigned n=0;n<flags;++n)f.push_back(std::uint8_t(n&3));
        file_bytes(f,a);for(unsigned n=0;n<a*12;++n)f.push_back(std::uint8_t(n+1));
        file_bytes(f,b);for(unsigned n=0;n<b*16;++n)f.push_back(std::uint8_t(n+2));
        file_bytes(f,p);for(unsigned n=0;n<p*20;++n)f.push_back(std::uint8_t(n+3));
    };
    cases.push_back({0x100031b0,"from-file",[=](Cpu& c){full_file(script.input,4,2,2,2);c.set({{3,data+0x180},{4,data+0x3000}});}});
    cases.push_back({0x100031b0,"fresh-list-alloc-fails",[=](Cpu& c){script.services[0x10773520]=[](Cpu&){return 0u;};c.set({{3,data+0x180},{4,0}});}});
    cases.push_back({0x100031b0,"failed-load",[=](Cpu& c){script.open_fails=true;c.set({{3,data+0x180},{4,data+0x3000}});}});
    cases.push_back({0x100031b0,"partial-load",[=](Cpu& c){full_file(script.input,4,2,2,2);script.fail_read=4;c.set({{3,data+0x180},{4,data+0x3000}});}});
    // destroy: null, each flag sign, with and without storage.
    cases.push_back(pointer(0x10003300,"null",0,1));
    for(unsigned deleting:{0u,1u,0xffffu,0x8000u,0x10000u})cases.push_back({0x10003300,"deleting-"+std::to_string(deleting),[=](Cpu& c){
        // Storage from real allocations so the frees and the delete are legal.
        original_mode=true;tracing=false;
        c.set({{3,data+0x180},{4,0}});c.set_lr(sentinel);c.s.pc=0x100031b0;run_until(c.s,sentinel);
        original_mode=false;seed(c);
        if(deleting&&short(deleting)>0){c.set(3,0x10);c.set_lr(sentinel);c.s.pc=0x10773520;run_until(c.s,sentinel);const auto copy=c.r(3);for(unsigned n=0;n<4;++n)c.put32(copy+n*4,c.u32(data+0x180+n*4));seed(c);c.set(3,copy);}
        else c.set(3,data+0x180);
        c.set(4,deleting);
    }});
    cases.push_back({0x10003300,"cleared-storage",[=](Cpu& c){for(unsigned n=0;n<4;++n)c.put32(data+0x180+n*4,0);c.set({{3,data+0x180},{4,0}});}});
    // load: every failure, versions, empty and long lists.
    const auto load=[&](const std::string& name,std::function<void(Cpu&)> more) {
        cases.push_back({0x100033c0,name,[=](Cpu& c){for(unsigned n=0;n<4;++n)c.put32(data+0x180+n*4,0);c.set({{3,data+0x180},{4,data+0x3000}});more(c);}});
    };
    cases.push_back(pointer(0x100033c0,"null-file",data+0x180,0));
    load("open-fails",[](Cpu&){script.open_fails=true;});
    load("zero-flags",[=](Cpu&){full_file(script.input,0,0,0,0);});
    for(unsigned n=1;n<=10;++n)load("read-fails-"+std::to_string(n),[=](Cpu&){full_file(script.input,4,1,1,1);script.fail_read=n;});
    load("no-records",[=](Cpu&){full_file(script.input,4,0,0,0);});
    load("records",[=](Cpu&){full_file(script.input,8,3,2,4);});
    load("old-version",[=](Cpu& c){full_file(script.input,8,3,2,0);c.put32(c.toc(-0x7b74),0x3e005);});
    load("newest-version-boundary",[=](Cpu& c){full_file(script.input,8,1,1,1);c.put32(c.toc(-0x7b74),0x3e006);});
    load("negative-version",[=](Cpu& c){full_file(script.input,8,1,1,1);c.put32(c.toc(-0x7b74),0xfffc1ffa);});
    load("alloc-fails",[=](Cpu&){full_file(script.input,4,0,0,0);script.services[0x10775130]=[](Cpu&){return 0u;};});
    load("list-alloc-fails",[=](Cpu&){full_file(script.input,4,0,0,0);script.services[0x10773520]=[](Cpu&){return 0u;};});
    // save: every failure, empty and filled lists.
    const auto save=[&](const std::string& name,unsigned a,unsigned b,unsigned p,std::function<void(Cpu&)> more) {
        cases.push_back({0x100036e0,name,[=](Cpu& c){
            original_mode=true;tracing=false;
            append_records(c,tables,0xc,1,a);append_records(c,tables+0x40,0x10,2,b);append_records(c,tables+0x80,0x14,3,p);
            original_mode=false;seed(c);c.set({{3,records},{4,data+0x3000}});more(c);
        }});
    };
    cases.push_back(pointer(0x100036e0,"null-file",records,0));
    save("empty",0,0,0,[](Cpu&){});
    save("filled",2,3,4,[](Cpu&){});
    save("open-fails",1,1,1,[](Cpu&){script.open_fails=true;});
    for(unsigned n=1;n<=8;++n)save("write-fails-"+std::to_string(n),1,1,1,[=](Cpu&){script.fail_write=n;});
    for(unsigned member=0;member<3;++member)save("no-next-"+std::to_string(member),1,1,1,[=](Cpu& c){c.put32(tables+member*0x40+4,0);});
    // status: null, id bound, raw, flags, contract and club.
    for(unsigned entry:{0x10003dc0u,0x10003ec0u,0x10003fa0u}) {
        cases.push_back(pointer(entry,"null-staff",records,0));
        cases.push_back({entry,"id-too-high",[=](Cpu& c){c.put32(staff(staff_count),staff_count);c.set({{3,records},{4,staff(staff_count)}});}});
        cases.push_back({entry,"id-negative",[=](Cpu& c){c.put32(staff(0),0xffffffff);c.put8(flags-1,1);c.set({{3,records},{4,staff(0)},{5,1}});}});
    }
    for(unsigned raw:{0u,1u,0x100u,0xffu})for(unsigned flag:{0u,1u,2u,3u,0x80u})for(unsigned have_contract:{0u,1u}) {
        cases.push_back({0x10003dc0,"primary-"+std::to_string(raw)+"-"+std::to_string(flag)+"-"+std::to_string(have_contract),[=](Cpu& c){
            c.put8(flags+3,flag);script.contract=have_contract?contract:0;
            original_mode=true;tracing=false;append_records(c,tables,0xc,3,1);original_mode=false;seed(c);
            c.set({{3,records},{4,staff(3)},{5,raw}});
        }});
    }
    for(unsigned flag:{0u,1u,2u,3u,0x82u})for(unsigned have_contract:{0u,1u}) {
        cases.push_back({0x10003ec0,"secondary-"+std::to_string(flag)+"-"+std::to_string(have_contract),[=](Cpu& c){
            c.put8(flags+3,flag);script.contract=have_contract?contract:0;
            original_mode=true;tracing=false;append_records(c,tables+0x40,0x10,3,1);original_mode=false;seed(c);
            c.set({{3,records},{4,staff(3)}});
        }});
    }
    for(unsigned count:{0u,1u,3u})for(unsigned found:{0u,1u}) {
        cases.push_back({0x10003fa0,"pending-"+std::to_string(count)+"-"+std::to_string(found),[=](Cpu& c){
            original_mode=true;tracing=false;append_records(c,tables+0x80,0x14,found?2:8,count);original_mode=false;seed(c);
            c.set({{3,records},{4,staff(3)}});
        }});
    }
    cases.push_back({0x10003fa0,"pending-no-list",[=](Cpu& c){c.put32(records+8,0);c.set({{3,records},{4,staff(3)}});}});
    cases.push_back({0x10003fa0,"pending-next-null",[=](Cpu& c){c.put16(tables+0x80,1);c.put32(tables+0x84,0);c.set({{3,records},{4,staff(3)}});}});
    // Removal by staff and by item.
    for(unsigned entry:{0x10006600u,0x10006830u}) {
        const auto member=entry==0x10006600?0u:1u;const auto size=entry==0x10006600?0xcu:0x10u;
        cases.push_back(pointer(entry,"null",records,0));
        cases.push_back({entry,"no-flags",[=](Cpu& c){c.put32(records+12,0);c.set({{3,records},{4,staff(3)}});}});
        for(unsigned flag:{0u,1u,2u,3u,0x83u})for(unsigned count:{0u,1u,4u})for(unsigned hit:{0u,1u}) {
            cases.push_back({entry,"flag-"+std::to_string(flag)+"-"+std::to_string(count)+"-"+std::to_string(hit),[=](Cpu& c){
                c.put8(flags+3,flag);
                original_mode=true;tracing=false;append_records(c,tables+member*0x40,size,hit?1:8,count);original_mode=false;seed(c);
                c.set({{3,records},{4,staff(3)}});
            }});
        }
        cases.push_back({entry,"no-list",[=](Cpu& c){c.put8(flags+3,3);c.put32(records+member*4,0);c.set({{3,records},{4,staff(3)}});}});
        cases.push_back({entry,"next-null",[=](Cpu& c){c.put8(flags+3,3);c.put16(tables+member*0x40,1);c.put32(tables+member*0x40+4,0);c.set({{3,records},{4,staff(3)}});}});
    }
    cases.push_back(pointer(0x100066d0,"null",records,0));
    for(unsigned flag:{0u,1u,2u,3u,0x80u,0x81u})for(unsigned have_contract:{0u,1u}) {
        cases.push_back({0x100066d0,"item-"+std::to_string(flag)+"-"+std::to_string(have_contract),[=](Cpu& c){
            c.put8(flags+3,flag);script.contract=have_contract?contract:0;
            original_mode=true;tracing=false;append_records(c,tables,0xc,3,2);c.set(3,tables);c.set_lr(sentinel);c.put32(tables+12,c.u32(tables+4));c.s.pc=0x10738e00;run_until(c.s,sentinel);
            const auto item=c.r(3);original_mode=false;seed(c);c.set({{3,records},{4,item}});
        }});
    }
    cases.push_back({0x100066d0,"item-null-club-table",[=](Cpu& c){
        c.put8(flags+3,1);script.contract=contract;c.put32(c.toc(-0x7be8),0);c.put32(contract+4,0);
        original_mode=true;tracing=false;append_records(c,tables,0xc,3,1);c.put32(tables+12,c.u32(tables+4));c.set(3,tables);c.set_lr(sentinel);c.s.pc=0x10738e00;run_until(c.s,sentinel);
        const auto item=c.r(3);original_mode=false;seed(c);c.set({{3,records},{4,item}});
    }});
    cases.push_back(pointer(0x100067d0,"null",records,0));
    for(unsigned flag:{0u,1u,2u,3u,0x80u,0x82u}) {
        cases.push_back({0x100067d0,"item-"+std::to_string(flag),[=](Cpu& c){
            c.put8(flags+3,flag);
            original_mode=true;tracing=false;append_records(c,tables+0x40,0x10,3,2);c.put32(tables+0x4c,c.u32(tables+0x44));c.set(3,tables+0x40);c.set_lr(sentinel);c.s.pc=0x10738e00;run_until(c.s,sentinel);
            const auto item=c.r(3);original_mode=false;seed(c);c.set({{3,records},{4,item}});
        }});
    }
    // notify: each guard, club cleared by a setter, manager, thresholds, kinds.
    const auto notice=[&](const std::string& name,std::function<void(Cpu&)> more) {
        cases.push_back({0x10007100,name,[=](Cpu& c){c.set({{3,records},{4,staff(3)},{5,club(3)},{6,0x82},{7,0x18000}});more(c);}});
    };
    notice("ordinary",[](Cpu& c){c.put32(staff(3)+0xc,manager+5*0x80);});
    notice("null-staff",[](Cpu& c){c.set(4,0);});
    notice("no-club",[](Cpu& c){c.put32(staff(3)+0x24,0);});
    notice("no-profile",[](Cpu& c){c.put32(staff(3)+0x14,0);});
    notice("null-reporting-club",[](Cpu& c){c.set(5,0);});
    notice("no-manager",[](Cpu& c){c.put32(club(3)+0x2c,0);});
    notice("same-manager",[](Cpu& c){c.put32(staff(3)+0xc,c.u32(club(3)+0x2c));});
    notice("public-above-threshold",[](Cpu& c){c.put16(profile+3*0x20+0xc,0x1d4d);});
    notice("threshold",[](Cpu& c){c.put16(profile+3*0x20+0xc,0x1d4c);});
    notice("negative-ability",[](Cpu& c){c.put16(profile+3*0x20+0xc,0x8000);});
    // A setter clears the club and a later one restores it: the original's
    // last manager comparison reads through the club unguarded, so a club
    // still null there is a guest fault, not a path.
    for(unsigned field=0;field<8;++field)notice("club-cleared-at-field-"+std::to_string(field),[=](Cpu&){
        script.services[0x102cb4b0]=[=](Cpu& c){
            if(c.r(4)==field)c.put32(staff(3)+0x24,0);
            if(c.r(4)==8)c.put32(staff(3)+0x24,club(4));
            return 0u;
        };
    });
    // Null at the end: the unguarded read goes through address 0x2c.
    notice("club-null-at-end",[=](Cpu& c){
        c.s.memory.map(0,0x1000);c.put32(0x2c,manager+0x80);
        script.services[0x102cb4b0]=[=](Cpu& c){if(c.r(4)==8)c.put32(staff(3)+0x24,0);return 0u;};
    });
    notice("club-replaced-by-setter",[=](Cpu&){
        script.services[0x102cb4b0]=[=](Cpu& c){if(c.r(4)==8)c.put32(staff(3)+0x24,club(4));return 0u;};
    });
    // sample: zero, small, wide of each sign, wraps, reseed.
    for(unsigned bound:{0u,1u,2u,7u,100u,65535u,65536u,65537u,100000u,429496u,429497u,0x7fffffffu,0x80000000u,0xffffffffu,0xffff0001u,0xffff0000u,0xfffeffffu})
        for(unsigned salt:{0u,0x5a5au,0xffffu})for(unsigned wrap:{0u,1u}) {
            cases.push_back({0x10006080,"bound-"+std::to_string(bound)+"-"+std::to_string(salt)+"-"+std::to_string(wrap),[=](Cpu& c){
                c.put32(c.toc(-0x7b90),salt);
                if(wrap)c.put32(c.toc(-0x7b84),c.u32(c.toc(-0x7b88)));
                c.set(3,bound);
            }});
        }
    for(unsigned word:{0u,0x7fffu,0x8000u,0xffffu,0xfffeu})for(unsigned salt:{0u,1u,0xffffu,0x7fffffffu,0x80000000u,0xffffffffu})for(unsigned bound:{70000u,100000000u,0x80000000u})
        cases.push_back({0x10006080,"word-"+std::to_string(word)+"-"+std::to_string(salt)+"-"+std::to_string(bound),[=](Cpu& c){
            const auto at=c.u32(c.toc(-0x7b84))+2;c.put16(at,word);c.put32(c.toc(-0x7b90),salt);c.set(3,bound);
        }});
}
#include "awol_checks_absence.inc"
#include "awol_checks_text.inc"
#include "awol_checks_review.inc"
#include "awol_checks_discipline.inc"
std::vector<Case> all_cases() {
    std::vector<Case> cases;
    add_records_cases(cases);
    add_absence_cases(cases);
    add_text_cases(cases);
    add_review_cases(cases);
    add_discipline_cases(cases);
    return cases;
}

// Included in the database checks' private namespace.
struct CroatianCupDivisions {
    std::vector<unsigned> lower, seconds, first;
};

CroatianCupDivisions croatian_cup_pool(guest::Cpu& c,bool historical) {
    CroatianCupDivisions clubs;
    const auto first=c.u32(c.toc(-0x6a40)),national=c.u32(c.toc(-0x6a20));
    const auto a=c.u32(c.toc(-0x6a44)),b=c.u32(c.toc(-0x6a48));
    const std::array lower{c.u32(c.toc(-0x7a0c)),c.u32(c.toc(-0x6a28)),c.u32(c.toc(-0x6a2c)),
                           c.u32(c.toc(-0x6a30)),c.u32(c.toc(-0x6a34))};
    const auto country=c.u32(c.toc(-0x7590));
    const auto table=c.u32(c.toc(-0x7be8)),count=c.u32(c.toc(-0x7b50))-2*c.u32(c.toc(-0x7b04));
    for(unsigned i=0;i<count;++i) {
        const auto club=table+i*0x248,division=c.u32(club+0x30),nation=c.u32(club+0x2c);
        if(!division)continue;
        const auto id=c.u32(division);
        if(id==first)clubs.first.push_back(club);
        else if(historical?id==national:(id==a || id==b))clubs.seconds.push_back(club);
        else if(historical && clubs.lower.size()<200 && nation && c.u32(nation+0x68)==country &&
                std::ranges::find(lower,id)!=lower.end())clubs.lower.push_back(club);
    }
    return clubs;
}

std::vector<unsigned> croatian_cup_entrants(guest::Cpu& c,unsigned cup) {
    const auto entries=c.u32(cup+0xa4);
    require(entries && c.u16(cup+0x3e)==48,"Croatian Cup must contain 48 entries");
    std::vector<unsigned> clubs;
    for(unsigned i=0;i<48;++i) {
        const auto club=c.u32(entries+8*i);
        require(club && std::ranges::find(clubs,club)==clubs.end(),"Croatian Cup club missing or duplicated");
        clubs.push_back(club);
    }
    return clubs;
}

void check_croatian_cup_cohorts(guest::Cpu& c,unsigned cup,bool historical) {
    const auto entrants=croatian_cup_entrants(c,cup);
    const auto pool=croatian_cup_pool(c,historical);
    const auto first=historical?36:32;
    require(std::equal(entrants.begin()+first,entrants.end(),pool.first.begin(),pool.first.end()),
            "Croatian Cup top-flight clubs or database order");
    if(!historical) {
        require(std::equal(entrants.begin(),entrants.begin()+first,pool.seconds.begin(),pool.seconds.end()),
                "Mac Croatian Cup regional entrants or database order");
        return;
    }
    require(pool.seconds.size()==18 && pool.first.size()==12 && pool.lower.size()>=18,"Croatian Cup source pools");
    for(unsigned i=0;i<18;++i)
        require(std::ranges::find(pool.lower,entrants[i])!=pool.lower.end(),"Croatian Cup lower-division qualifier");
    auto seconds=pool.seconds;
    std::sort(seconds.begin(),seconds.end());
    std::vector<unsigned> found(entrants.begin()+18,entrants.begin()+36);
    std::sort(found.begin(),found.end());
    require(seconds==found,"Croatian Cup second division entrants");
}

void check_croatian_cup_date(guest::Cpu& c,unsigned at,unsigned season,unsigned month_day,unsigned shift,unsigned wanted) {
    using namespace std::chrono;
    const auto y=year{int(season+shift)};
    const auto anchor=sys_days{y/(month_day/100)/(month_day%100)};
    const int weekday0=weekday(anchor).c_encoding();
    const auto actual=anchor+days{(int(wanted)+10-weekday0)%7-3}; // nearest weekday, Sunday=0
    require(c.u16(at)==unsigned((actual-sys_days{y/January/1}).count()) && c.u8(at+2)==shift,
            "Croatian Cup draw or match date differs from its edition");
}

void check_croatian_cup_rounds(guest::Cpu& c,unsigned cup,bool historical) {
    const auto calendar=c.u32(cup+0xa0),draw=c.u32(cup+0xac);
    require(calendar && draw && c.u16(cup+0x3c)==6,"Croatian Cup rounds missing");
    constexpr std::array codes{230,10,20,120,130,150},ties{16,16,8,4,2,1},arrivals{32,16,0,0,0,0};
    for(unsigned i=0;i<6;++i) {
        const auto round=calendar+i*0x78;
        require(c.u32(draw+4*i) && c.u16(round+8)==codes[i] && c.u16(round+0x1c)==ties[i] &&
                c.u16(round+0x1e)==arrivals[i],"Croatian Cup stage, ties or new entrants");
        const std::array draw_date{716,820,924,1029,historical?317:1123,historical?420:404};
        const std::array match_date{815,922,1027,historical?308:1107,historical?405:320,historical?503:524};
        const unsigned season=c.u16(cup+0x40);
        check_croatian_cup_date(c,round,season,draw_date[i],i>=unsigned(historical?4:5),i==5?4:5);
        check_croatian_cup_date(c,round+4,season,match_date[i],i>=unsigned(historical?3:4),i==0?0:3);
        require(c.u16(round+0x1a)==2*ties[i] && c.u16(round+0x20)==(i==1?32:0) &&
                c.u32(round+0x70)==UINT32_MAX,"Croatian Cup entrants, byes or prize field");
        const std::array days{0,0,0,historical?7:14,14,historical?11:7};
        require(c.u8(round+0x23)==(i<3?1:2) && c.u8(round+0x24)==days[i],
                "Croatian Cup leg rules or dates between legs");
    }
}

void check_mac_cup_reference(State& s,unsigned cup) {
    guest::Cpu c(s);
    const auto copy=allocate(s,0xb4,true),out=allocate(s,8,true);
    const auto cursor=c.toc(-0x7b84),old_cursor=c.u32(cursor),salt=c.toc(-0x7b90),old_salt=c.u32(salt);
    const auto pool=c.toc(-0x7b8c);
    const auto* current=std::getenv("CM_CRO_CUP_ORIGINAL");
    const std::optional<std::string> previous=current?std::optional<std::string>(current):std::nullopt;
    for(bool calendar:{false,true}) {
        const unsigned size=calendar?6*0x78:48*8;
        const auto memory=allocate(s,size,true);
        const std::vector<std::uint8_t> poison(size,0xed);
        std::vector<std::uint8_t> original,expected_rng;
        unsigned expected_result=0;
        const auto registers=s.registers;
        const auto original_unique=s.unique;
        for(bool reference:{true,false}) {
            s.registers=registers;
            s.unique=original_unique;
            c.put32(cursor,pool+128); c.put32(salt,31);
            if(reference)setenv("CM_CRO_CUP_ORIGINAL","1",1); else unsetenv("CM_CRO_CUP_ORIGINAL");
            s.memory.copy_in(copy,s.memory.copy_out(cup,0xb4));
            s.memory.copy_in(memory,poison);
            c.put32(out,0xcccccccc); c.put32(out+4,0xcccccccc);
            cup_fixture_allocation=memory;
            cup_fixture_allocation_size=size;
            const auto result=calendar?invoke(s,0x106bf4c0,{copy,UINT32_MAX,out,out+2,out+4}):
                                       invoke(s,0x106bf9b0,{copy});
            cup_fixture_allocation=0;
            require(calendar?result==memory:(result==1 && c.u32(copy+0xa4)==memory && c.u16(copy+0x3e)==48),
                    "Croatian Cup allocation fixture did not reach the reference entry");
            auto bytes=s.memory.copy_out(memory,size);
            const auto outputs=s.memory.copy_out(out,8);
            bytes.insert(bytes.end(),outputs.begin(),outputs.end());
            if(reference) {
                original=bytes; expected_result=result; expected_rng=random_state(s);
            } else require(result==expected_result && bytes==original && random_state(s)==expected_rng,
                    "Croatian Cup native and original Mac roster or calendar differs");
        }
        deallocate(s,memory);
    }
    if(previous)setenv("CM_CRO_CUP_ORIGINAL",previous->c_str(),1); else unsetenv("CM_CRO_CUP_ORIGINAL");
    c.put32(cursor,old_cursor); c.put32(salt,old_salt);
    deallocate(s,out); deallocate(s,copy);
}

void check_historical_cup_seeds(State& s,unsigned cup) {
    guest::Cpu c(s);
    const auto pool=croatian_cup_pool(c,true);
    require(pool.lower.size()>=18 && pool.seconds.size()==18,"Croatian Cup seeded fixture has insufficient clubs");
    std::vector<std::pair<unsigned,int>> original_reputations;
    const auto distinct_reputations=[&](const std::vector<unsigned>& clubs){
        for(unsigned i=0;i<clubs.size();++i) {
            original_reputations.emplace_back(clubs[i],c.i16(clubs[i]+0x1ec));
            c.put16(clubs[i]+0x1ec,100+i);
        }
    };
    distinct_reputations(pool.lower);
    distinct_reputations(pool.seconds);
    const auto cursor=c.toc(-0x7b84),old_cursor=c.u32(cursor),salt=c.toc(-0x7b90),old_salt=c.u32(salt);
    const unsigned begin=c.toc(-0x7b8c)+256,span=2*(2*(pool.lower.size()/10)+18)+2;
    const auto samples=s.memory.copy_out(begin,span);
    // Controlled non-wrapping stream with repeated seed values. Distinct reputations make the
    // secondary comparison observable without relying on implementation-defined std::sort tie order.
    for(unsigned i=0;i<span/2;++i)c.put16(begin+2*i,11*(i%7)+i%3);
    c.put32(cursor,begin); c.put32(salt,13);
    unsigned position=begin;
    const auto draw=[&](unsigned bound){position+=2;return (13+c.u16(position))%bound;};
    auto qualifiers=pool.lower;
    std::reverse(qualifiers.begin(),qualifiers.end()); // reputation descending in the controlled fixture
    const unsigned swaps=qualifiers.size()/10;
    for(unsigned i=0;i<swaps;++i) {
        const auto a=draw(swaps),b=draw(swaps);
        std::swap(qualifiers[a],qualifiers[b]);
    }
    std::vector<std::pair<unsigned,unsigned>> second;
    for(const auto club:pool.seconds)second.emplace_back(club,draw(50));
    std::sort(second.begin(),second.end(),[&](auto a,auto b){
        return a.second!=b.second?a.second>b.second:c.i16(a.first+0x1ec)<c.i16(b.first+0x1ec);
    });
    const auto copy=allocate(s,0xb4,true),entries=allocate(s,48*8,true);
    s.memory.copy_in(copy,s.memory.copy_out(cup,0xb4));
    s.memory.copy_in(entries,std::vector<std::uint8_t>(48*8,0xed));
    cup_fixture_allocation=entries; cup_fixture_allocation_size=48*8;
    require(invoke(s,0x106bf9b0,{copy})==1 && c.u32(copy+0xa4)==entries && c.u32(cursor)==position && c.u32(salt)==13,
            "Croatian Cup 3.89 selector allocated, drew or seeded differently");
    cup_fixture_allocation=0;
    for(unsigned i=0;i<48;++i) {
        const auto at=entries+8*i;
        const unsigned expected=i<18?qualifiers[i]:i<36?second[i-18].first:pool.first[i-36];
        const unsigned seed=i<18?0:i<36?second[i-18].second:0xed;
        require(c.u32(at)==expected && c.u8(at+4)==seed && c.u8(at+5)==0 && c.u16(at+6)==0xeded,
                "Croatian Cup 3.89 qualification order, seeding or entry bytes");
    }
    deallocate(s,entries); deallocate(s,copy);
    for(auto [club,rep]:original_reputations)c.put16(club+0x1ec,rep);
    s.memory.copy_in(begin,samples);
    c.put32(cursor,old_cursor); c.put32(salt,old_salt);
}

std::vector<std::uint8_t> croatian_cup_bracket(State& s,unsigned cup) {
    guest::Cpu c(s);
    std::vector<std::uint8_t> bracket;
    const auto rounds=c.u32(cup+0xa0),tree=c.u32(cup+0xac);
    for(unsigned i=0;i<6;++i) {
        const auto ties=c.u16(rounds+i*0x78+0x1c);
        const auto bytes=s.memory.copy_out(c.u32(tree+i*4),ties*0x36);
        bracket.insert(bracket.end(),bytes.begin(),bytes.end());
    }
    return bracket;
}

void check_croatian_cup_tree(guest::Cpu& c,unsigned cup) {
    const auto rounds=c.u32(cup+0xa0),tree=c.u32(cup+0xac);
    for(unsigned round=0;round<6;++round) {
        const auto ties=c.u16(rounds+round*0x78+0x1c);
        std::vector<int> direct, winners;
        for(unsigned j=0;j<ties;++j) {
            const auto match=c.u32(tree+4*round)+j*0x36;
            for(unsigned side:{0,2}) {
                const int entrant=c.i16(match+side);
                if(entrant>=0)direct.push_back(entrant);
                else {
                    require(round && c.i16(match+0xe + side)==int(round)-1,"Croatian Cup winner feeds the wrong round");
                    winners.push_back(c.i16(match+0xa+side));
                }
            }
        }
        std::sort(direct.begin(),direct.end()); std::sort(winners.begin(),winners.end());
        require(direct.size()==(round==0?32u:round==1?16u:0u) &&
                winners.size()==(round?c.u16(rounds+(round-1)*0x78+0x1c):0u),
                "Croatian Cup bracket has the wrong number of byes or winners");
        for(unsigned i=0;i<direct.size();++i)require(direct[i]==int(i+(round?32:0)),"Croatian Cup bye awarded to wrong entrant");
        for(unsigned i=0;i<winners.size();++i)require(winners[i]==int(i),"Croatian Cup did not advance every preceding tie");
    }
}

constexpr unsigned croatian_cup_ctor=0x106bf0f0, croatian_cup_dtor=0x106bf3d0, croatian_cup_rollover=0x106bfbe0;
bool croatian_lifecycle_interior(unsigned pc) {
    return (pc>croatian_cup_ctor && pc<croatian_cup_dtor) ||
           (pc>croatian_cup_dtor && pc<0x106bf4c0) ||
           (pc>croatian_cup_rollover && pc<0x106bfe30);
}

constexpr unsigned croatian_configure=0x106bfe30, croatian_override_entry=0x0fff0000;
struct CroatianCupOverride {
    unsigned slot=0, back=0, dialog=0, result=0;
};
struct CroatianCupRun {
    CroatianCupOverride method;
    unsigned cup=0, descriptor=0, method_calls=0;
    unsigned fail_allocation=0, failures=0, interior_steps=0;
    std::vector<std::pair<unsigned,std::string>> dialogs;
    std::vector<std::pair<unsigned,unsigned>> freed;
};
CroatianCupRun* croatian_cup_run=nullptr;

bool observe_croatian_cup_lifecycle(State& s) {
    if(!croatian_cup_run)return false;
    auto& run=*croatian_cup_run;
    if(croatian_lifecycle_interior(s.pc) || (run.method.slot==0x44 && s.pc>croatian_configure && s.pc<0x106bfe8c))
        ++run.interior_steps;
    guest::Cpu c(s);
    if(s.pc==croatian_override_entry && run.method.slot) {
        ++run.method_calls;
        require(c.lr()==run.method.back && c.r(3)==run.cup && c.r(12)==run.descriptor &&
                c.r(2)==c.u32(run.descriptor+4) && c.u32(c.sp()+0x14)+4==c.r(2),
                "Croatian Cup virtual override lost its receiver, return address, descriptor or TOC");
        if(run.method.slot==0x44) {
            require(c.r(4)==UINT32_MAX && c.r(5)==run.cup+0x3c && c.r(6)==run.cup+0x3a && c.r(7)==0,
                    "Croatian Cup virtual calendar arguments");
            c.put16(c.r(6),0x751);
        }
        c.set(3,run.method.result); s.pc=c.lr(); return true;
    }
    if(run.fail_allocation && c.lr()==run.fail_allocation &&
       (s.pc==comp::callee::allocate_object || s.pc==comp::callee::allocate_block)) {
        ++run.failures; c.set(3,0); s.pc=c.lr(); return true;
    }
    if((run.fail_allocation || run.method.dialog) && s.pc==comp::callee::error_dialog) {
        std::string dialog;
        for(auto text:{c.r(3),c.r(4)}) {
            for(unsigned n=0;n<500 && c.u8(text+n);++n)dialog+=char(c.u8(text+n));
            dialog.push_back('\0');
        }
        run.dialogs.emplace_back(c.lr(),std::move(dialog));
        s.pc=c.lr(); return true;
    }
    if(croatian_lifecycle_interior(c.lr()) && (s.pc==comp::callee::free_block || s.pc==comp::callee::free_object))
        run.freed.emplace_back(c.lr(),c.r(3));
    return false;
}

// Execute the original and the replacement from identical mapped guest memory. Only the
// three lifecycle entries switch implementations. Both runs use the selected edition's rules.
unsigned compare_croatian_cup_lifecycle(State& s,unsigned entry,std::initializer_list<unsigned> args,
                                        unsigned fail_allocation=0,CroatianCupOverride method={}) {
    const auto cup=*args.begin();
    guest::Cpu cpu(s);
    unsigned old_table=0,table=0,descriptor=0;
    if(method.slot) {
        old_table=cpu.u32(cup);
        table=allocate(s,0xb0,true);
        descriptor=table+0xa8;
        s.memory.copy_in(table,s.memory.copy_out(old_table,0xa8));
        cpu.put32(table+method.slot,descriptor);
        cpu.put32(descriptor,croatian_override_entry);
        cpu.put32(descriptor+4,cpu.r(2)+4);
        cpu.put32(cup,table);
    }
    auto reference=cm::semantic_io_checks::clone(s);
    reference->platform=s.platform;
    copy_configuration(*reference,s);
    CroatianCupRun original,rewrite;
    original.fail_allocation=rewrite.fail_allocation=fail_allocation;
    original.method=rewrite.method=method;
    original.cup=rewrite.cup=cup;
    original.descriptor=rewrite.descriptor=descriptor;
    const char* variable=entry==croatian_configure?"CM_CRO_CUP_ORIGINAL":"CM_CRO_CUP_LIFECYCLE_ORIGINAL";
    auto run=[&](State& state,CroatianCupRun& trace,bool use_original) {
        const auto previous=std::getenv(variable);
        const std::optional<std::string> restore=previous?std::optional<std::string>(previous):std::nullopt;
        if(use_original)setenv(variable,"1",1); else unsetenv(variable);
        croatian_cup_run=&trace;
        auto reset=[&]{
            croatian_cup_run=nullptr;
            if(restore)setenv(variable,restore->c_str(),1); else unsetenv(variable);
        };
        try {
            const auto result=invoke(state,entry,args);
            reset();
            return result;
        } catch(...) { reset(); throw; }
    };
    // COMP_STATS appends its season record to this real member and stores the byte offset in the cup.
    // Give both runs the same initial file as well as the same initial guest state.
    std::filesystem::path history;
    if(entry==croatian_cup_rollover) {
        const auto files=s.platform->writable_directory()/"Files";
        history=(std::filesystem::is_directory(files/"Temp")?files/"Temp":files)/"comp records history.tmp";
    }
    const auto read_history=[&]() -> std::optional<std::string> {
        if(history.empty() || !std::filesystem::exists(history))return std::nullopt;
        std::ifstream input(history,std::ios::binary);
        require(bool(input),"Opening Croatian Cup history for comparison");
        return std::string(std::istreambuf_iterator<char>(input),{});
    };
    const auto prior_history=read_history();
    const auto expected=run(*reference,original,true);
    const auto expected_history=read_history();
    if(!history.empty()) {
        if(prior_history) {
            std::ofstream output(history,std::ios::binary|std::ios::trunc);
            require(bool(output.write(prior_history->data(),prior_history->size())),"Restoring Croatian Cup history");
        } else std::filesystem::remove(history);
    }
    const auto result=run(s,rewrite,false);
    auto check=[&](bool same,const std::string& field) {
        if(!same)throw std::runtime_error("Croatian Cup lifecycle "+hex_address(entry)+" "+field);
    };
    check(read_history()==expected_history,"competition history member bytes");
    check(rewrite.interior_steps==0,"production dispatched into an original instruction block");
    if(cup)check(original.interior_steps>0,"reference never executed an original instruction block");
    check(entry==croatian_cup_rollover || result==expected,"return value");
    check(rewrite.freed==original.freed,"round or competition deallocation");
    check(rewrite.failures==(fail_allocation?1u:0u) && rewrite.failures==original.failures,"allocation fault injection");
    check(rewrite.dialogs==original.dialogs && (!fail_allocation || !rewrite.dialogs.empty()),"error dialog source or message");
    if(method.slot) {
        check(rewrite.method_calls==1 && original.method_calls==1,"virtual override call count");
        if(method.dialog)check(rewrite.dialogs.size()==1 && rewrite.dialogs.front().first==method.dialog,
                              "wrong assertion return address for a failing virtual override");
        cpu.put32(cup,old_table);
        reference->memory.write<4>(cup,old_table);
        deallocate(s,table); deallocate(*reference,table);
    }
    guest::Cpu c(s),r(*reference);
    for(unsigned reg=1;reg<=31;++reg)
        if(reg<=2 || reg>=13)check(c.r(reg)==r.r(reg),"preserved register r"+std::to_string(reg));
    check(c.lr()==r.lr() && s.pc==reference->pc,"return address");
    check(s.allocated==reference->allocated && s.available==reference->available && s.heap_end==reference->heap_end &&
          s.memory_error==reference->memory_error,"guest heap");
    check(random_state(s)==random_state(*reference),"RNG pool or cursor");
    check(c.u8(c.toc(-0x7bac))==r.u8(r.toc(-0x7bac)) && (!(fail_allocation || method.dialog) || !c.u8(c.toc(-0x7bac))),"success flag");
    if(!cup)return result;
    auto compare=[&](unsigned address,unsigned length,const char* name) {
        if(!address)return;
        const auto actual=s.memory.copy_out(address,length),wanted=reference->memory.copy_out(address,length);
        const auto diff=std::mismatch(actual.begin(),actual.end(),wanted.begin());
        if(diff.first!=actual.end())check(false,std::string(name)+" byte "+hex_address(unsigned(diff.first-actual.begin())));
    };
    // The deleting destructor can release the outer storage too.
    if(entry==croatian_cup_dtor && std::int16_t(*(args.begin()+1))>0)return result;
    compare(cup,0xb4,"object");
    if(entry==croatian_cup_dtor)return result;
    compare(c.u32(cup+0xa4),c.u16(cup+0x3e)*8,"entrants");
    compare(c.u32(cup+0xa0),c.u16(cup+0x3c)*0x78,"calendar");
    compare(c.u32(cup+8),0x634,"stats");
    const auto tree=c.u32(cup+0xac),rounds=c.u32(cup+0xa0);
    if(tree && rounds) {
        compare(tree,c.u16(cup+0x3c)*4,"round pointers");
        for(unsigned i=0;i<c.u16(cup+0x3c);++i)
            compare(c.u32(tree+i*4),c.u16(rounds+i*0x78+0x1c)*0x36,"ties");
    }
    return result;
}

void check_fresh_croatian_cup_lifecycle(State& s,unsigned cup,unsigned year) {
    guest::Cpu c(s);
    const auto definition=c.u32(cup+4);
    const auto fresh=invoke(s,comp::callee::allocate_object,{0xb4});
    require(compare_croatian_cup_lifecycle(s,croatian_cup_ctor,{fresh,year,definition})==fresh &&
            c.u16(fresh+0x3e)==48 && c.u32(fresh+8),"Croatian Cup fresh construction");
    if(year==2000) {
        const auto copy=allocate(s,0xb4,true), rows=c.u32(fresh+0xa0);
        s.memory.copy_in(copy,s.memory.copy_out(fresh,0xb4));
        c.put32(copy+0xa0,0);
        require(compare_croatian_cup_lifecycle(s,croatian_configure,{copy},0,{0x44,0x106bfe6c,0,rows})==1 &&
                c.u32(copy+0xa0)==rows && c.u16(copy+0x3a)==0x751,"Croatian Cup ignored the calendar override");
        deallocate(s,copy);
        c.put8(c.toc(-0x7bac),1);
        compare_croatian_cup_lifecycle(s,croatian_cup_rollover,{fresh},0x106bfa0c);
        require(c.u16(fresh+0x40)==year+1 && !c.u32(fresh+0xa4) && !c.u32(fresh+0xa0) && !c.u32(fresh+0xac),
                "Croatian Cup failed rollover continued after the selector failed");
        for(const auto method:{CroatianCupOverride{0x94,0x106bfd38,0x106bfd8c,UINT32_MAX},
                               CroatianCupOverride{0x9c,0x106bfda8,0x106bfdfc,UINT32_MAX}}) {
            c.put8(c.toc(-0x7bac),1);
            compare_croatian_cup_lifecycle(s,croatian_cup_rollover,{fresh},0,method);
            require(c.u32(fresh+0xa4) && bool(c.u32(fresh+0xa0))==(method.slot==0x9c) && !c.u32(fresh+0xac),
                    "Croatian Cup continued past a failed configuration or tree override");
        }
    }
    compare_croatian_cup_lifecycle(s,croatian_cup_dtor,{fresh,1});
    if(year!=2000)return;
    const auto no_entrants=invoke(s,comp::callee::allocate_object,{0xb4});
    c.put8(c.toc(-0x7bac),1);
    compare_croatian_cup_lifecycle(s,croatian_cup_ctor,{no_entrants,year,definition},0x106bfa0c);
    require(!c.u32(no_entrants+0xa4) && !c.u32(no_entrants+8),"Croatian Cup constructed stats after selector failure");
    compare_croatian_cup_lifecycle(s,croatian_cup_dtor,{no_entrants,1});
    const auto partial=invoke(s,comp::callee::allocate_object,{0xb4});
    c.put8(c.toc(-0x7bac),1);
    compare_croatian_cup_lifecycle(s,croatian_cup_ctor,{partial,year,definition},0x106bf31c);
    require(!c.u32(partial+8) && c.u32(partial+0xac),"Croatian Cup stats allocation failure staging");
    compare_croatian_cup_lifecycle(s,croatian_cup_dtor,{partial,0});
    invoke(s,comp::callee::free_object,{partial});
    compare_croatian_cup_lifecycle(s,croatian_cup_dtor,{0,1});
    c.put8(c.toc(-0x7bac),1);
}

void check_croatian_cup_save_and_rollover(State& s,unsigned cup,const std::vector<CroatianFixture>& fixtures) {
    guest::Cpu c(s);
    const auto calendar=c.u32(cup+0xa0),entries=c.u32(cup+0xa4),definition=c.u32(cup+4);
    const auto old_calendar=s.memory.copy_out(calendar,6*0x78),old_entries=s.memory.copy_out(entries,48*8);
    const auto old_tree=croatian_cup_bracket(s,cup),rng=random_state(s);
    const auto matches=fixtures.size();
    const auto version=c.toc(-0x7b74),prior=c.u32(version);
    c.put32(version,0u-0x3e046);
    invoke(s,0x10484420,{cup,1});
    const auto restored=invoke(s,comp::callee::allocate_object,{0xb4});
    require(restored && compare_croatian_cup_lifecycle(s,croatian_cup_ctor,{restored,1999,definition})==restored && c.u16(restored+0x40)==2000 &&
            c.u32(restored+4)==definition && c.u16(restored+0x3c)==6 && c.u16(restored+0x3e)==48 &&
            c.u8(restored+0xa8)==c.u8(cup+0xa8) && random_state(s)==rng && fixtures.size()==matches &&
            s.memory.copy_out(c.u32(restored+0xa4),48*8)==old_entries &&
            s.memory.copy_out(c.u32(restored+0xa0),6*0x78)==old_calendar && croatian_cup_bracket(s,restored)==old_tree,
            "Croatian Cup member lost its roster, seeds, dates, bracket or current round on reload");
    compare_croatian_cup_lifecycle(s,croatian_cup_dtor,{restored,1});
    c.put32(version,prior);

    const auto pool=croatian_cup_pool(c,true);
    const auto before=croatian_cup_entrants(c,cup);
    const auto outside=std::ranges::find_if(pool.lower,[&](unsigned club){return std::ranges::find(before,club)==before.end();});
    require(outside!=pool.lower.end(),"Croatian Cup rollover needs one previously absent club");
    // A lower-division club wins promotion into A2; another A2 club replaces a relegated A1 club.
    const auto swap_division=[&](unsigned a,unsigned b) {
        const auto division=c.u32(a+0x30);
        c.put32(a+0x30,c.u32(b+0x30)); c.put32(b+0x30,division);
    };
    swap_division(*outside,pool.seconds.back());
    swap_division(pool.first.front(),pool.seconds.front());
    c.put8(c.toc(-0x7bac),1);
    compare_croatian_cup_lifecycle(s,croatian_cup_rollover,{cup});
    require(c.u8(c.toc(-0x7bac)) && c.u16(cup+0x40)==2001,"Croatian Cup failed to advance into next season");
    check_croatian_cup_cohorts(c,cup,true);
    check_croatian_cup_rounds(c,cup,true);
    check_croatian_cup_tree(c,cup);
    const auto after=croatian_cup_entrants(c,cup);
    require(std::ranges::find(after.begin()+18,after.begin()+36,*outside)!=after.begin()+36 &&
            std::ranges::find(after.begin()+36,after.end(),pool.seconds.front())!=after.end(),
            "Croatian Cup rollover did not requalify promoted clubs");
    swap_division(pool.first.front(),pool.seconds.front());
    swap_division(*outside,pool.seconds.back());
}

void check_croatia_cup(State& s,unsigned year) {
    guest::Cpu c(s);
    const bool historical=year==2000;
    const auto rows=c.toc(-0x7b44);
    unsigned country=0;
    for(unsigned i=0;i<34;++i) {
        const auto row=rows+i*0x4c,entry=c.u32(row+8);
        if(entry && c.u32(entry)==0x10350bb0)country=row;
    }
    require(country,"Croatian Cup country constructor missing");
    c.put8(c.u32(country)+0x120,6);
    invoke(s,0x10191e20);
    std::vector<CroatianFixture> fixtures;
    croatian_fixtures=&fixtures;
    require(invoke(s,0x10350bb0,{country})==1,"Croatian Cup country construction");
    require(c.u32(country+0xc)==(historical?3:4),"Croatian Cup country publication count");
    const auto cup=c.u32(c.u32(country+0x10)+(c.u32(country+0xc)-1)*4);
    require(cup && c.u32(cup+4)==c.u32(c.toc(-0x7bbc))+0x6c*c.u32(c.toc(-0x6a38)) &&
            c.u16(cup+0x40)==year,"Croatian Cup publication");
    require(c.u32(cup+0x28)==100000,"Croatian Cup prize pool differs from the release");
    check_croatian_cup_cohorts(c,cup,historical);
    check_croatian_cup_rounds(c,cup,historical);
    check_croatian_cup_tree(c,cup);
    check_fresh_croatian_cup_lifecycle(s,cup,year);
    if(historical) {
        check_historical_cup_seeds(s,cup);
        check_croatian_cup_save_and_rollover(s,cup,fixtures);
    } else check_mac_cup_reference(s,cup);
    std::cout<<"Croatian Cup "<<year<<": 48 entrants, six rounds, bracket and seed checks passed\n";
    croatian_fixtures=nullptr;
}

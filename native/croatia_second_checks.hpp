// Included by the database season integration checks.
void prepare_croatian_rollover_services(State& s) {
    const auto registers=s.registers;
    const auto saved_unique=s.unique;
    s.wr<4>(4128,return_sentinel); s.pc=0x10343630; s.budget=s.executed+22;
    try { run_until(s,return_sentinel); }
    catch(const std::runtime_error& e) { if(std::string_view(e.what())!="Native instruction budget exhausted")throw; }
    require(s.pc==0x10343688,"Croatian rollover fixture missed the world initialization prologue");
    s.pc=0x10343990; s.budget=s.executed+20000000000ull;
    run_until(s,0x10344200);
    s.wr<4>(14*4,2); s.wr<4>(13*4,0x2c);
    preparing_croatian_world=true;
    try {
        run_until(s,return_sentinel);
        throw std::runtime_error("Croatian rollover fixture missed the competition dispatcher");
    } catch(const CompetitionServicesReady&) {}
    preparing_croatian_world=false;
    s.registers=registers; s.unique=saved_unique; s.pc=return_sentinel;
}

std::vector<unsigned> croatian_standings(guest::Cpu& c,unsigned league) {
    std::vector<unsigned> clubs;
    for(unsigned i=0;i<c.u16(league+0x3e);++i)clubs.push_back(c.u32(c.u32(league+0xb0)+i*0x44));
    return clubs;
}

std::vector<std::pair<unsigned,unsigned>> croatian_rounds(guest::Cpu& c,unsigned league) {
    std::vector<std::pair<unsigned,unsigned>> dates;
    for(unsigned j=0;j<c.u16(league+0xa6);++j) {
        const auto slot=c.u32(league+0xbc)+j*0x4c;
        dates.emplace_back(c.u16(league+0x40)+c.u16(slot+2),c.u16(slot));
    }
    return dates;
}

void check_croatian_second_dates(const std::vector<std::pair<unsigned,unsigned>>& dates) {
    // Civil dates of the 34 matchdays in Windows 3.89, including the adjustment from the
    // second half's February-May anchors to the nearest Saturday or Tuesday in 2001.
    constexpr std::array<unsigned,34> month_day{826,902,909,912,916,923,930,1007,1014,1021,1024,1028,
        1104,1111,1118,1125,1202,217,220,224,303,306,310,313,317,324,327,331,403,407,414,421,428,505};
    using namespace std::chrono;
    for(unsigned i=0;i<month_day.size();++i) {
        const auto y=year{2000+int(i>=17)};
        const auto day=sys_days{y/(month_day[i]/100)/(month_day[i]%100)};
        require(dates[i]==std::pair{unsigned(int(y)),unsigned((day-sys_days{y/January/1}).count())},
                "Croatian A2 date differs from the 3.89 season");
    }
}

void check_croatian_league_draw(guest::Cpu& c,unsigned league,const std::vector<CroatianFixture>& fixtures) {
    const auto definition=c.u32(league+4);
    const auto clubs=croatian_standings(c,league);
    auto distinct=clubs;
    std::ranges::sort(distinct);
    require(std::adjacent_find(distinct.begin(),distinct.end())==distinct.end(),"Croatian division duplicated a club");
    for(const auto club:clubs)require(club && c.u32(club+0x30)==definition,"Croatian division has a stale entrant");
    std::vector<CroatianFixture> matches;
    for(const auto& fixture:fixtures)if(fixture.definition==definition)matches.push_back(fixture);
    check_croatian_pairings(matches,clubs,croatian_rounds(c,league),false);
}

void check_croatian_members_reload(State& s,unsigned country,const std::vector<CroatianFixture>& fixtures) {
    guest::Cpu c(s);
    const auto objects=c.u32(country+0x10),save_version=c.toc(-0x7b74),previous=c.u32(save_version);
    c.put32(save_version,0u-0x3e046);
    const auto matches=fixtures.size();
    // A1 members written by the earlier pilot stored the regional A2A id. Loading that
    // member joins the A1 back to the national division used by this edition.
    const auto first=c.u32(objects),prior_link=c.u32(first+0x20);
    require(prior_link==c.u32(c.toc(-0x6a20)),"Croatian A1 save fixture has an invalid national link");
    c.put32(first+0x20,c.u32(c.toc(-0x6a44)));
    invoke(s,0x104a8530,{first,1});
    c.put32(first+0x20,prior_link);
    invoke(s,0x104a8530,{c.u32(objects+4),1});
    const auto rng=random_state(s);
    const auto copied_row=allocate(s,0x4c,true);
    s.memory.copy_in(copied_row,s.memory.copy_out(country,0x4c));
    c.put32(copied_row+0x10,0);
    isolate_croatian_cup=true;
    require(invoke(s,0x10350bb0,{copied_row})==1 && c.u32(copied_row+0xc)==3,"Croatian country member reload");
    isolate_croatian_cup=false;
    for(unsigned i=0;i<2;++i) {
        const auto original=c.u32(objects+i*4), loaded=c.u32(c.u32(copied_row+0x10)+i*4);
        const auto rounds=c.u16(original+0xa6);
        require(loaded!=original && c.u32(loaded+4)==c.u32(original+4) && c.u16(loaded+0x40)==c.u16(original+0x40) &&
                c.u16(loaded+0xa6)==rounds && c.u16(loaded+0x3c)==c.u16(original+0x3c) &&
                croatian_standings(c,loaded)==croatian_standings(c,original) &&
                s.memory.copy_out(c.u32(loaded+0xbc),rounds*0x4c)==s.memory.copy_out(c.u32(original+0xbc),rounds*0x4c),
                "Croatian league member reload lost entrants, year, calendar or format");
    }
    require(c.u32(c.u32(c.u32(copied_row+0x10))+0x20)==prior_link,
            "Croatian A1 member retained the earlier pilot's regional second-division link");
    require(fixtures.size()==matches && random_state(s)==rng,"Croatian country member reload redrew fixtures or clubs");
    c.put32(save_version,previous);
}

void check_croatian_rollover(State& s,unsigned first,unsigned second,std::vector<CroatianFixture>& fixtures) {
    guest::Cpu c(s);
    const auto first_definition=c.u32(first+4),second_definition=c.u32(second+4);
    require(c.i32(first+0x1c)==-1 && c.u32(first+0x20)==c.u32(second_definition) &&
            c.u32(second+0x1c)==c.u32(first_definition) && c.i32(second+0x20)==-1,"Croatian pyramid links");
    require(c.u8(first+0xc3)==2 && c.u8(second+0xc0)==2 && c.u8(second+0xc3)==5,"Croatian places at season end");

    const auto directory=c.toc(-0x7b5c),count=c.u32(c.toc(-0x7b64));
    if(!c.u32(directory)) {
        c.put32(directory,allocate(s,count*4,true));
        c.put32(c.toc(-0x7b60),count);
    }
    for(const auto league:{first,second})
        c.put32(c.u32(directory)+4*c.u32(c.u32(league+4)),league);
    // New Game creates these services after country construction. Division changes update the
    // database and club finances, and a promoted club can trigger a manager appointment.
    const auto service=[&](int slot,unsigned bytes,unsigned constructor) {
        const auto cell=c.toc(slot);
        if(!c.u32(cell)) {
            const auto object=invoke(s,0x10773520,{bytes});
            require(object,"Croatian rollover service allocation");
            invoke(s,constructor,{object,0});
            c.put32(cell,object);
        }
        return c.u32(cell);
    };
    service(-0x7a98,2,0x104aca50);
    service(-0x7ab8,4,0x100f84e0);
    service(-0x7b7c,0x3c,0x10077ee0);
    // Observe the request to recruit a manager and each season-opening notification at the
    // receiving subsystem. Their arguments are checked after both leagues have turned over.
    RolloverObservers observers;
    croatian_rollover=&observers;
    const auto first_before=croatian_standings(c,first),second_before=croatian_standings(c,second);
    const auto club_table=c.u32(c.toc(-0x7be8)),club_count=c.u32(c.toc(-0x7b50));
    std::vector<unsigned> previous_divisions;
    for(unsigned i=0;i<club_count;++i)previous_divisions.push_back(c.u32(club_table+i*0x248+0x30));
    // Open the A1's second phase on its real callback before taking both tables to season end.
    for(unsigned i=0;i<first_before.size();++i) {
        const auto row=c.u32(first+0xb0)+i*0x44;
        c.put8(row+0x14,22); c.put16(row+0x1c,2*(12-i));
    }
    invoke(s,0x1049eb30,{first,0});
    const auto today=c.toc(-0x7b80);
    const auto first_dates=croatian_rounds(c,first);
    c.put16(today,first_dates.back().second+1); c.put16(today+2,first_dates.back().first); c.put8(today+4,0);
    s.memory.copy_in(first+0xd4,s.memory.copy_out(today,5));
    invoke(s,0x106dcf20,{first,today,1});
    require(c.u16(first+0x3c)==3,"Croatian rollover fixture did not reach the A1 second phase");
    // Distinct standings at the end of the competition. The game's end-of-season routine decides
    // the actual promotion/relegation markers from these results and the edition's rules.
    for(const auto league:{first,second}) {
        const unsigned teams=c.u16(league+0x3e), games=league==first?32:34;
        for(unsigned i=0;i<teams;++i) {
            const auto row=c.u32(league+0xb0)+i*0x44;
            c.put8(row+0x14,games); c.put16(row+0x1c,2*(teams-i));
        }
        invoke(s,0x104a1c30,{league});
    }
    const auto first_ranked=croatian_standings(c,first),second_ranked=croatian_standings(c,second);
    fixtures.clear();
    const auto success=c.toc(-0x7bac);
    c.put8(success,1);
    invoke(s,0x106dbf10,{first});
    if(!c.u8(success) || c.u16(first+0x40)!=2001 || c.u16(second+0x40)!=2001)
        throw std::runtime_error("Croatian rollover: success="+std::to_string(c.u8(success))+
                ", A1 year="+std::to_string(c.u16(first+0x40))+", A2 year="+std::to_string(c.u16(second+0x40)));
    croatian_rollover=nullptr;
    require(observers.notifications==std::vector<unsigned>{second_definition,first_definition},
            "Croatian rollover failed to notify both new seasons");
    const auto lower=c.u32(c.toc(-0x7bbc))+c.u32(c.toc(-0x7a0c))*0x6c;
    for(unsigned i=0;i<first_ranked.size();++i)
        require(c.u32(first_ranked[i]+0x30)==(i<10?first_definition:second_definition),"Croatian A1 relegation");
    for(unsigned i=0;i<second_ranked.size();++i)
        require(c.u32(second_ranked[i]+0x30)==(i<2?first_definition:i<13?second_definition:lower),
                "Croatian A2 promotion, retention or relegation");
    const auto first_after=croatian_standings(c,first),second_after=croatian_standings(c,second);
    require(first_after.size()==12 && second_after.size()==18,"Croatian pyramid shrank on rollover");
    std::vector<unsigned> admitted;
    for(const auto club:second_after) {
        require(c.u32(club+0x30)==second_definition,"Croatian A2 admitted a club from another division");
        if(std::ranges::find(first_before,club)!=first_before.end() || std::ranges::find(second_before,club)!=second_before.end())continue;
        const auto former=previous_divisions[(club-club_table)/0x248];
        require(former && c.u32(c.u32(club+0x2c)+0x68)==c.u32(c.toc(-0x7590)),
                "Croatian A2 drew a replacement from another nation");
        bool lower_origin=false;
        for(int slot:{-0x7a0c,-0x6a28,-0x6a30,-0x6a2c,-0x6a34})lower_origin|=c.u32(former)==c.u32(c.toc(slot));
        require(lower_origin,"Croatian A2 replacement was not from a lower division");
        admitted.push_back(club);
    }
    unsigned moved=0;
    for(unsigned i=0;i<club_count;++i)
        if(c.u32(club_table+i*0x248+0x30)!=previous_divisions[i])++moved;
    require(moved==14,"Croatian rollover changed clubs outside the two exchanges");
    require(admitted.size()==5 && fixtures.size()==132+306,"Croatian pyramid did not replace five clubs and redraw both leagues");
    for(const auto club:observers.vacancies)
        require(std::ranges::find(admitted,club)!=admitted.end(),"Croatian rollover referred an ineligible club for a manager");
    check_croatian_league_draw(c,first,fixtures);
    check_croatian_league_draw(c,second,fixtures);
}

void check_croatian_super_cup(State& s,unsigned country,std::vector<CroatianFixture>& fixtures) {
    guest::Cpu c(s);
    auto& career=s.platform->career_rules();
    require(invoke(s,0x10354cb0)==1,"Could not register the Croatian test competitions");
    const auto directory=c.u32(c.toc(-0x7b5c));
    const auto super_id=c.u32(c.toc(-0x6adc)), entry=directory+super_id*4;
    require(super_id==197 && c.u32(entry)==0,"Original 2002/03 constructor registered a Croatian Super Cup");
    const auto count=fixtures.size();
    const auto rng=random_state(s);
    croatia_super_cup::new_career(s);
    require(c.u32(entry)==0 && fixtures.size()==count && random_state(s)==rng,
            "Croatian Super Cup scheduled with historical rules off");
    require(career.read("CM-NATIVE-RULES 1\nedition si_3968\nrevision 1\nhistorical_rules on\n").empty() &&
            career.revision()==1,"Revision 1 Croatian Super Cup fixture");
    croatia_super_cup::new_career(s);
    require(c.u32(entry)==0 && fixtures.size()==count && random_state(s)==rng,
            "Revision 1 career inherited a new historical rule");

    career.set_historical_rules(true);
    croatia_super_cup::new_career(s);
    const auto club_table=c.u32(c.toc(-0x7be8));
    const auto zagreb=club_table+13073*0x248, dinamo=club_table+3099*0x248;
    const auto super=c.u32(entry);
    require(super && c.u32(country+0xc)==5 && croatia_super_cup::owns(s,super) &&
            c.u32(c.u32(country+0x10)+16)==super,"Croatian Super Cup missing from the country or fixture registry");
    const auto& fixture=fixtures.back();
    require(fixtures.size()==count+1 && fixture.day==200 && fixture.year==2002 && fixture.home==zagreb &&
            fixture.away==dinamo && (fixture.flags&0x83)==0x83 && random_state(s)==rng,
            "2002 Super Cup did not schedule NK Zagreb vs Dinamo on 20 July with golden goal and penalties");
    const auto score=allocate(s,0x50,true);
    c.put32(score,super_id);
    c.put32(score+0x0c,13073);c.put32(score+0x10,3099);
    c.put32(score+0x14,fixture.definition);
    c.put32(score+0x1c,fixture.home);c.put32(score+0x20,fixture.away);
    c.put16(score+0x28,fixture.year);c.put16(score+0x2a,fixture.day);c.put16(score+0x2c,fixture.year);
    c.put8(score+0x3d,2);c.put8(score+0x3e,3);c.put8(score+0x42,fixture.owner);
    c.put16(score+0x38,fixture.flags);
    c.put8(score+0x43,2);c.put8(score+0x44,2);
    c.put8(score+0x47,2);c.put8(score+0x48,3);
    require(invoke(s,0x1047e540,{super,score})==2,"The cup result reader did not recognise the golden-goal winner");
    c.put8(score+0x48,2);c.put8(score+0x49,5);c.put8(score+0x4a,4);
    require(invoke(s,0x1047e540,{super,score})==1,"The cup result reader did not recognise the shootout winner");
    croatia_super_cup::new_career(s);
    require(fixtures.size()==count+1,"Repeat setup duplicated the Super Cup fixture");

    const auto calendar=s.memory.copy_out(c.u32(super+0xa0),0x78);
    const auto table=s.memory.copy_out(c.u32(super+0xa4),16);
    const auto bracket=s.memory.copy_out(c.u32(c.u32(super+0xac)),0x36);
    const auto version=c.toc(-0x7b74), previous_version=c.u32(version);
    c.put32(version,0u-0x3e046);
    invoke(s,0x10484420,{super,1});
    const auto copy=allocate(s,0x4c,true);
    s.memory.copy_in(copy,s.memory.copy_out(country,0x4c));
    c.put32(copy+0xc,0);c.put32(copy+0x10,0);
    career.set_historical_rules(false);
    s.platform->quick_save_load_begin(s);
    require(croatia_super_cup::restore(s,copy) && c.u32(copy+0xc)==1,"Saved Super Cup could not be restored");
    const auto loaded=c.u32(c.u32(copy+0x10));
    require(loaded!=super && c.u16(loaded+0x40)==2002 && c.u8(loaded+0xa8)==1 &&
            s.memory.copy_out(c.u32(loaded+0xa0),0x78)==calendar && s.memory.copy_out(c.u32(loaded+0xa4),16)==table &&
            s.memory.copy_out(c.u32(c.u32(loaded+0xac)),0x36)==bracket &&
            fixtures.size()==count+1 && random_state(s)==rng,"Reopening the Super Cup redrew or changed the fixture");
    invoke(s,0x106cfc00,{loaded,1});
    invoke(s,comp::callee::free_block,{c.u32(copy+0x10)});
    deallocate(s,copy);
    c.set(3,1);s.platform->quick_save_load_end(s);
    c.put32(version,previous_version);
    career.set_historical_rules(true);

    c.put8(score+0x48,3);c.put8(score+0x49,0xff);c.put8(score+0x4a,0xff);
    const auto today=c.toc(-0x7b80);
    const auto saved_day=c.u16(today);
    c.put16(today,200);
    // COMP_STATS creates the two club rows before recording the players' performance.
    const auto stats=c.u32(super+8);
    require(invoke(s,0x10460b60,{stats,zagreb}) && invoke(s,0x10460b60,{stats,dinamo}),
            "Could not initialize the synthetic Super Cup club statistics");
    synthetic_super_cup_result=score;
    const auto result=invoke(s,0x1047bfb0,{super,score,0});
    require(result==1 && c.u8(super+0xb0)==1 &&
            c.u8(c.u32(super+0xa4)+5)==2 && c.u8(c.u32(super+0xa4)+13)==1,
            "The Super Cup did not finish with the away golden-goal winner");
    invoke(s,0x10482ca0,{super,today,1});
    synthetic_super_cup_result=0;
    deallocate(s,score);
    c.put16(today,saved_day);
    namespace history=comp_util::history_log;
    const auto history_rows=c.u32(c.toc(history::rows)), history_count=c.u32(c.toc(history::count));
    bool recorded=false;
    for(unsigned i=0;i<history_count;++i) {
        const auto row=history_rows+i*history::row_size;
        if(c.u32(row+history::competition)==fixture.definition && c.u16(row+history::year)==2002 &&
           c.u32(row+history::winner)==dinamo && c.u32(row+history::runner_up)==zagreb) recorded=true;
    }
    require(c.u8(super+0x49)==1 && recorded,
            "The daily update did not close the Super Cup and record the finalists in history");
    const auto after_result_rng=random_state(s);

    // Supply completed results to the same before/after boundaries used by the annual country loop.
    const auto league=c.u32(c.u32(country+0x10)), knockout=c.u32(c.u32(country+0x10)+12);
    const auto standings=c.u32(league+0xb0), entrants=c.u32(knockout+0xa4);
    const auto simulated_league_winner=c.u32(standings+0x44), simulated_cup_winner=c.u32(standings+0x88);
    c.put32(standings,simulated_league_winner);c.put8(league+0xe0,1);
    for(unsigned i=0;i<c.u16(knockout+0x3e);++i)c.put8(entrants+i*8+5,2);
    c.put32(entrants,simulated_league_winner);c.put8(entrants+5,1);
    const auto rollover=[&](int from,bool cup_finished,unsigned cup_winner) {
        c.put16(league+0x40,from);c.put16(knockout+0x40,from);
        c.put8(knockout+0xb0,cup_finished);
        c.put32(entrants,cup_winner);
        c.put16(c.toc(-0x7b80)+2,from+1);
        croatia_super_cup::before_rollover(s,country);
        c.put16(league+0x40,from+1);c.put16(knockout+0x40,from+1);
        croatia_super_cup::after_rollover(s,country);
    };
    rollover(2002,true,simulated_league_winner);
    rollover(2003,false,simulated_cup_winner);
    require(c.u32(entry)==super && fixtures.size()==count+1 && random_state(s)==after_result_rng,
            "Domestic double or unfinished cup produced a match");
    rollover(2004,true,simulated_cup_winner);
    const auto& annual=fixtures.back();
    require(fixtures.size()==count+2 && annual.year==2005 && annual.home==simulated_league_winner &&
            annual.away==simulated_cup_winner && (annual.flags&0x83)==3 && c.u32(country+0xc)==5 &&
            c.u32(entry)==c.u32(c.u32(country+0x10)+16) && random_state(s)==after_result_rng,
            "Annual Super Cup ignored completed-season winners or the new extra-time rule");
    career.reset();
    std::cout<<"Croatian Super Cup launch, duplicate prevention, save member, double and annual qualification passed\n";
}

// CM_SEASON_WORLD_DIGEST=1: FNV-1a of guest memory outside the stacks
// (0x70000000 up) and, apart, of the registers, to compare two runs of one
// scenario (CM_RELATION_SORT_ORIGINAL=1 against the native relation sort).
void print_world_digest(const State& s,const char* where) {
    const char* mode=std::getenv("CM_SEASON_WORLD_DIGEST");
    if(!mode)return;
    guest::Cpu c(const_cast<State&>(s));
    const auto stats=c.u32(c.toc(-0x7b3c)),relations=stats?c.u32(stats+0x3c):0u;
    const auto relation_count=stats?c.i32(stats+0x44):0;
    // =swap, the negative control: two neighbouring relation rows exchanged,
    // as a sort that orders one pair wrongly would leave them.
    if(std::string_view(mode)=="swap" && relations && relation_count>1) {
        const auto middle=relations+std::uint32_t(relation_count/2)*8;
        auto a=c.s.memory.copy_out(middle,8),b=c.s.memory.copy_out(middle+8,8);
        c.s.memory.copy_in(middle,b); c.s.memory.copy_in(middle+8,a);
    }
    std::uint64_t rows=UINT64_C(14695981039346656037);
    if(relations)for(const auto byte:s.memory.copy_out(relations,std::size_t(relation_count)*8))rows=(rows^byte)*UINT64_C(0x100000001b3);
    std::cout<<"relations "<<where<<": "<<relation_count<<" rows, digest "<<std::hex<<rows<<std::dec<<std::endl;
    std::uint64_t memory=UINT64_C(14695981039346656037),registers=memory;
    for(const auto& [start,size]:s.memory.mapped_ranges()) {
        if(start>=0x70000000 && start<0x80000000)continue;
        for(const auto byte:s.memory.copy_out(start,size))memory=(memory^byte)*UINT64_C(0x100000001b3);
    }
    for(const auto byte:s.registers)registers=(registers^byte)*UINT64_C(0x100000001b3);
    // CM_SEASON_WORLD_DUMP=DIR: also the memory outside the stacks, one file a
    // point (start and size words, then the bytes, per range), to diff two runs.
    if(const char* dump=std::getenv("CM_SEASON_WORLD_DUMP")) {
        static unsigned point=0;
        std::ofstream out(std::string(dump)+"/point-"+std::to_string(point++)+".bin",std::ios::binary);
        for(const auto& [start,size]:s.memory.mapped_ranges()) {
            if(start>=0x70000000 && start<0x80000000)continue;
            const std::uint64_t header[2]{start,size};
            out.write(reinterpret_cast<const char*>(header),sizeof header);
            const auto bytes=s.memory.copy_out(start,size);
            out.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));
        }
    }
    std::cout<<"world digest "<<where<<": memory "<<std::hex<<memory<<" registers "<<registers<<std::dec<<std::endl;
}

void check_croatia_second(State& s,unsigned year,bool super_cup=false) {
    guest::Cpu c(s);
    const bool legacy=year==2000;
    // CM_SEASON_STACK_AUDIT=FILE: the complete stack audit (stack_audit.hpp) of
    // the scenarios that build player stats, written to FILE, for the leftover
    // contract of a rewrite inside New Game creation.
    std::optional<StackAudit> audit;
    const char* audit_file=(legacy || super_cup)?std::getenv("CM_SEASON_STACK_AUDIT"):nullptr;
    if(audit_file) { audit.emplace(s); s.memory.date_observer=audit->observer(); }
    struct AuditReport {
        State& s; std::optional<StackAudit>& audit; const char* file;
        ~AuditReport() {
            if(!audit)return;
            s.memory.date_observer=nullptr;
            std::ofstream out(file); audit->report(out,true);
        }
    } audit_report{s,audit,audit_file};
    auto rows=c.toc(-0x7b44);
    unsigned country=0;
    for(unsigned i=0;i<34;++i) {
        const auto row=rows+i*0x4c,entry=c.u32(row+8);
        if(entry && c.u32(entry)==0x10350bb0)country=row;
    }
    require(country,"Croatian country constructor missing");
    c.put8(c.u32(country)+0x120,6);
    invoke(s,0x10191e20);
    print_world_digest(s,"after the new world's staff stage");
    if(legacy || super_cup)prepare_croatian_rollover_services(s);
    isolate_croatian_cup=!super_cup;
    std::vector<CroatianFixture> fixtures;
    croatian_fixtures=&fixtures;
    require(invoke(s,0x10350bb0,{country})==1,"Croatian country constructor failed");
    isolate_croatian_cup=false;
    const std::vector<int> slots=legacy?std::vector<int>{-0x6a40,-0x6a20,-0x6a38}:
                                    std::vector<int>{-0x6a40,-0x6a44,-0x6a48,-0x6a38};
    require(c.u32(country+0xc)==slots.size(),"Croatian country published the wrong number of competitions");
    const auto definitions=c.u32(c.toc(-0x7bbc)),objects=c.u32(country+0x10);
    for(unsigned i=0;i<slots.size();++i) {
        const auto object=c.u32(objects+i*4), definition=definitions+c.u32(c.toc(slots[i]))*0x6c;
        require(object && c.u32(object+4)==definition,"Croatian country published the wrong competition definition");
        if(i==0 || i+1==slots.size())continue;
        const auto count=c.u16(object+0x3e),rounds=c.u16(object+0xa6);
        require(count==(legacy?18:16) && rounds==2*(count-1),"Croatian second division size or length");
        check_croatian_league_draw(c,object,fixtures);
        if(legacy)check_croatian_second_dates(croatian_rounds(c,object));
        std::cout<<"Croatian "<<(legacy?"A2":i==1?"A2A":"A2B")<<" "<<year<<": "<<count<<" clubs, "<<count*(count-1)<<" fixtures\n";
    }
    if(legacy) {
        check_croatian_members_reload(s,country,fixtures);
        check_croatian_rollover(s,c.u32(objects),c.u32(objects+4),fixtures);
        std::cout<<"Croatian A1/A2 save members restored; both leagues redrawn after 14 verified club movements\n";
    }
    if(super_cup)check_croatian_super_cup(s,country,fixtures);
    print_world_digest(s,"at the end of the scenario");
    croatian_fixtures=nullptr;
}

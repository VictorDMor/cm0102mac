// Included in database_seasons_checks.cpp's private namespace. The real database supplies the clubs,
// definitions and names. Capture each fixture when the league hands it to the fixture manager.
// Each directed pairing must occur exactly once. Every club plays exactly once per round.
void check_croatian_pairings(const std::vector<CroatianFixture>& matches, const std::vector<unsigned>& clubs,
                             const std::vector<std::pair<unsigned,unsigned>>& dates, bool groups) {
    const auto size=clubs.size();
    require(matches.size()==dates.size()*size/2,"Croatian A1 fixture count");
    std::vector<unsigned> pairings(size*size), appearances(dates.size()*size);
    for(const auto& match:matches) {
        const auto home=std::ranges::find(clubs,match.home), away=std::ranges::find(clubs,match.away);
        require(home!=clubs.end() && away!=clubs.end() && match.round<dates.size() &&
                dates[match.round]==std::pair{match.year,match.day} && match.owner==255,
                "Croatian A1 fixture has an invalid club, round, date or competition stage");
        const auto h=home-clubs.begin(), a=away-clubs.begin();
        ++pairings[h*size+a];
        ++appearances[match.round*size+h];
        ++appearances[match.round*size+a];
    }
    require(std::ranges::all_of(appearances,[](unsigned n){return n==1;}),"Croatian A1 club missing or doubled in a round");
    for(unsigned h=0;h<size;++h)for(unsigned a=0;a<size;++a)
        require(pairings[h*size+a]==unsigned(h!=a && (!groups || h/6==a/6)),
                "Croatian A1 pairings violate the group boundary or home/away balance");
}

void check_croatian_split(State& s,unsigned league,const std::vector<unsigned>& clubs,
                          std::vector<CroatianFixture>& matches,const std::vector<std::pair<unsigned,unsigned>>& dates) {
    guest::Cpu c(s);
    const auto rows=c.u32(league+0xb0);
    // Controlled first phase: each team beat every lower seed at home and away. Give the final
    // club one outstanding match to exercise the transition boundary before submitting its result.
    for(unsigned i=0;i<clubs.size();++i) {
        const auto row=rows+i*0x44;
        for(unsigned group=0;group<3;++group) {
            const unsigned meetings=group?1:2, won=meetings*(11-i), lost=meetings*i;
            const auto totals=row+0x14+group*0x10;
            c.put8(totals,won+lost); c.put8(totals+1,won); c.put8(totals+2,0); c.put8(totals+3,lost);
            c.put16(totals+4,won); c.put16(totals+6,lost); c.put16(totals+8,3*won);
        }
    }
    // The full competition dispatcher publishes the constructed objects. This fixture builds A1
    // directly to isolate it from the two 01/02 second-division constructors.
    const auto id=c.u32(c.u32(league+4)), directory_cell=c.toc(-0x7b5c);
    if(!c.u32(directory_cell)) {
        const auto count=c.u32(c.toc(-0x7b64)); // one slot per loaded competition definition
        c.put32(directory_cell,allocate(s,count*4,true));
        c.put32(c.toc(-0x7b60),count);
    }
    require(id<c.u32(c.toc(-0x7b60)),"Croatian A1 outside the competition directory");
    c.put32(c.u32(directory_cell)+4*id,league);
    matches.clear();
    const auto today=c.toc(-0x7b80);
    c.put16(today,dates[21].second+1); c.put16(today+2,2001); c.put8(today+4,0);
    // The isolated league has no season records manager. Its ordinary per-day records update
    // already ran on this date; the Croatian split check runs on every active callback.
    s.memory.copy_in(league+0xd4,s.memory.copy_out(today,5));
    c.put8(rows+11*0x44+0x14,21);
    invoke(s,0x106dcf20,{league,today,1});
    require(matches.empty() && c.u16(league+0x3c)==2,"Croatian split started with an outstanding match");
    c.put8(rows+11*0x44+0x14,22);
    require(invoke(s,0x1049eb30,{league,0})==1,"Croatian A1 round 22 sort");
    for(unsigned i=0;i<clubs.size();++i) {
        const auto row=rows+i*0x44,history=c.u32(row+0xc);
        require(c.u32(row)==clubs[i] && history && c.u8(history+21)==i,"Croatian A1 round 22 position history");
    }
    invoke(s,0x106dcf20,{league,today,0});
    require(matches.empty() && c.u16(league+0x3c)==2,"Inactive Croatian A1 update started the split");
    invoke(s,0x106dcf20,{league,today,1});
    require(c.u32(league+0xb0)==rows && c.u16(league+0x3e)==12 && c.u16(league+0x3c)==3,
            "Croatian A1 split replaced the championship table or left it in the first phase");
    check_croatian_pairings(matches,clubs,{dates.begin()+22,dates.end()},true);
    for(unsigned i=0;i<clubs.size();++i) {
        const auto row=rows+i*0x44;
        require(c.u32(row)==clubs[i] && c.u8(row+0x14)==22 && c.u16(row+0x1c)==6*(11-i),
                "Croatian A1 lost points or matches played at the split");
    }
    matches.clear();
    invoke(s,0x106dcf20,{league,today,1});
    require(matches.empty(),"Croatian A1 scheduled the second phase twice");
    // After one more round, make the bottom group's leader outscore all six championship clubs.
    // The actual standings sort must still keep the groups intact while allowing a change within each.
    for(unsigned i=0;i<clubs.size();++i) {
        const auto row=rows+i*0x44;
        c.put8(row+0x14,23); c.put16(row+0x1c,i);
    }
    require(invoke(s,0x1049eb30,{league,0})==1,"Croatian A1 group sort");
    for(unsigned i=0;i<clubs.size();++i)
        require(c.u32(rows+i*0x44)==clubs[(i<6?5:17)-i],"Croatian A1 standings crossed groups or ignored points");
}

void check_croatia_first(State& s, unsigned year) {
    guest::Cpu c(s);
    const bool split=year==2000;
    const unsigned size=split?12:16;
    const auto definition_id=c.u32(c.toc(-0x6a40));
    const auto definition=c.u32(c.toc(-0x7bbc))+definition_id*0x6c;
    std::vector<unsigned> eligible;
    const auto club_count=c.u32(c.toc(-0x7b50)),club_table=c.u32(c.toc(-0x7be8));
    for(unsigned i=0;i<club_count;++i) {
        const auto club=club_table+i*0x248, division=c.u32(club+0x30);
        if(division && c.u32(division)==definition_id)eligible.push_back(club);
    }
    require(eligible.size()==size,"Croatian A1 source division has the wrong number of clubs");

    invoke(s,0x10191e20);
    const auto league=allocate(s,0xf0,true);
    std::vector<CroatianFixture> matches;
    croatian_fixtures=&matches;
    require(invoke(s,0x106dadb0,{league,year,definition})==league && c.u32(league+8),
            "Croatian A1 constructor did not finish");
    require(c.u16(league+0x3e)==size && c.u16(league+0x40)==year && c.u16(league+0x3c)==2,
            "Croatian A1 size, season or initial phase");
    const auto rows=c.u32(league+0xb0),calendar=c.u32(league+0xbc);
    require(rows && calendar,"Croatian A1 table or calendar missing");
    std::vector<unsigned> clubs;
    for(unsigned i=0;i<size;++i)clubs.push_back(c.u32(rows+i*0x44));
    auto ordered=clubs;
    std::ranges::sort(ordered);
    require(ordered==eligible,"Croatian A1 participants differ from the selected database");
    require(matches.size()==size*(size-1),"Croatian A1 first phase did not schedule a double round-robin");
    const unsigned opening_rounds=2*(size-1);
    require(c.u16(league+0xa6)==opening_rounds,"Croatian A1 first phase calendar length");
    std::vector<std::pair<unsigned,unsigned>> dates;
    for(unsigned i=0;i<(split?32:opening_rounds);++i) {
        const auto slot=calendar+i*0x4c;
        dates.emplace_back(year+c.u16(slot+2),c.u16(slot));
    }
    if(split) {
        // Official 3.89 fixture dates. Compare the stored day-of-year independently of the game's calendar builder.
        constexpr std::array<unsigned,32> month_days{729,805,812,819,826,909,916,923,930,1007,1014,1021,1028,
            1104,1111,1118,1125,217,224,303,310,317,331,403,407,414,421,428,505,512,519,526};
        using namespace std::chrono;
        for(unsigned i=0;i<dates.size();++i) {
            const auto y=2000+int(i>=17);
            const auto day=sys_days{std::chrono::year{y}/(month_days[i]/100)/(month_days[i]%100)};
            require(dates[i]==std::pair{unsigned(y),unsigned((day-sys_days{std::chrono::year{y}/January/1}).count())},
                    "Croatian A1 round on a different day than in 3.89");
        }
    }
    check_croatian_pairings(matches,clubs,{dates.begin(),dates.begin()+opening_rounds},false);
    if(split) {
        check_croatian_split(s,league,clubs,matches,dates);
        const auto league_before=s.memory.copy_out(league,0xf0), rows_before=s.memory.copy_out(rows,size*0x44);
        const auto rng=random_state(s);
        const auto success=c.toc(-0x7bac);
        // A2 has not been constructed in this isolated world. Rollover must reject it before any movement.
        c.put8(success,1);
        invoke(s,0x106dbf10,{league});
        require(c.u8(success)==0 && s.memory.copy_out(league,0xf0)==league_before &&
                s.memory.copy_out(rows,size*0x44)==rows_before && random_state(s)==rng && matches.empty(),
                "Croatian A1 advanced or changed its table with no historical second division");
    }
    croatian_fixtures=nullptr;
    std::cout<<"Croatian A1 "<<year<<" first phase: "<<size<<" clubs, "<<size*(size-1)<<" fixtures"<<(split?", 60 split fixtures with unchanged points":"")<<"\n";
}

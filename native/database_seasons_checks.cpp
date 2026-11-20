#include "database_seasons.hpp"
#include "career_rules.hpp"
#include "season_competition_rules.hpp"
#include "guest_abi.hpp"
#include "platform.hpp"
#include "adaptive_gui.hpp"
#include "new_screens.hpp"
#include "code_fragment_adapter.hpp"
#include "form_layout.hpp"
#include "gui_guest_scope.hpp"
#include "routine_check.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <array>

using namespace cm::aot;
namespace {
// The bootstrap reads screen bounds while constructing preferences. This
// component fixture has no window and must also run without a connected display.
struct DisplayFixture final : CodeFragmentBoundaryObserver {
    unsigned handle = 0, toc = 0;
    void enter(const State&, unsigned, bool) override {}
    void leave(State&, unsigned, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        const std::string_view import(name);
        if (import != "CarbonLib::GetMainDevice" && import != "CarbonLib::GetGrayRgn" &&
            import != "CarbonLib::GetRegionBounds") return false;
        if (!handle) {
            handle = allocate(s, 128, true);
            s.memory.write<4>(handle, handle + 4);
            s.memory.write<2>(handle + 4 + 38, 900);
            s.memory.write<2>(handle + 4 + 40, 1280);
        }
        if (import == "CarbonLib::GetRegionBounds") {
            s.memory.copy_in(s.r<4>(16), s.memory.copy_out(handle + 4 + 34, 8));
            s.wr<4>(12, s.r<4>(16));
        } else s.wr<4>(12, handle);
        s.pc = s.r<4>(4128); return true;
    }
};
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::vector<unsigned>* constructor_rows=nullptr;
std::vector<unsigned> constructor_targets;
bool constructing_england=false;
unsigned fixture_insertions=0;
std::vector<std::string>* survey_messages=nullptr;
unsigned survey_row=0xffffffff;
// The country whose constructor is running (survey only); the global competitions have no nation.
std::string survey_country(guest::Cpu& c) {
    if(survey_row>=34) return "no constructor running";
    const auto nation=c.u32(c.toc(-0x7b44)+survey_row*0x4c);
    if(!nation) return "global competitions";
    std::string name; for(unsigned j=0;j<50 && c.u8(nation + 0xae + j);++j) name+=char(c.u8(nation + 0xae + j));
    return name;
}
bool reject_dialog(State& s) {
    // Exercise the real competition constructors and participant selectors.
    // Scheduling inserts are outside this fixture: no full career/fixture
    // manager is created, and all writes remain in the disposable runtime.
    if(constructing_england && s.pc==0x104888f0) {
        ++fixture_insertions;
        s.wr<4>(12,1);
        s.pc=s.r<4>(4128); return true;
    }
    if(survey_messages && !constructor_rows && std::ranges::find(constructor_targets,s.pc)!=constructor_targets.end()) {
        // Survey only: note which country's constructor is running; execution continues unchanged.
        guest::Cpu c(s); survey_row=(c.r(3)-c.toc(-0x7b44))/0x4c; return false;
    }
    if(constructor_rows && std::ranges::find(constructor_targets,s.pc)!=constructor_targets.end()) {
        guest::Cpu c(s);
        constructor_rows->push_back((c.r(3)-c.toc(-0x7b44))/0x4c);
        // Constructors normally publish these calendar components for the
        // dispatcher to turn into Dates. Supply valid fixture-only values.
        for(auto offset:{0x34u,0x44u}) {
            c.put16(c.r(3)+offset,1); c.put8(c.r(3)+offset+2,0);
            c.put16(c.r(3)+offset+4,2000); c.put16(c.r(3)+offset+6,0xffff);
        }
        c.set(3,1); s.pc=c.lr(); return true;
    }
    constexpr unsigned dialog=0x10769b60;
    if(s.pc!=dialog)return false;
    guest::Cpu c(s); std::string message;
    for(auto p:{c.r(3),c.r(4),survey_messages?0u:c.toc(-0x7bb8)}) {
        message+=" | ";
        for(unsigned i=0;p && i<500 && c.u8(p+i);++i)message+=char(c.u8(p+i));
    }
    // The survey answers the dialog's Ok and goes on, as a player would.
    if(survey_messages) { survey_messages->push_back("row "+std::to_string(survey_row)+" ("+survey_country(c)+")"+message+" (from "+hex_address(c.lr())+")"); s.pc=c.lr(); return true; }
    throw std::runtime_error("Unexpected original error dialog"+message);
}
unsigned invoke(State& s, unsigned entry, std::initializer_list<unsigned> args={}, unsigned stop=return_sentinel) {
    unsigned reg=3; for(auto value:args)s.wr<4>(reg++*4,value);
    s.wr<4>(4128,stop);s.pc=entry;s.budget=s.executed+20000000000ull;
    try {run_until(s,stop);}
    catch(const std::exception& e){throw std::runtime_error(hex_address(entry)+" at "+hex_address(s.pc)+": "+e.what());}
    return s.r<4>(12);
}
std::vector<std::uint8_t> random_state(State& s) {
    std::vector<std::uint8_t> result;
    for (auto global : {0x108b3f00u, 0x108b3f08u, 0x108b3f0cu}) {
        const auto bytes=s.memory.copy_out(s.memory.read<4>(global),4);
        result.insert(result.end(),bytes.begin(),bytes.end());
    }
    const auto samples=s.memory.copy_out(s.memory.read<4>(0x108b3f04),102000);
    result.insert(result.end(),samples.begin(),samples.end()); return result;
}
void check_extra_division_availability(State& s) {
    guest::Cpu c(s);
    const auto nation=c.u32(c.toc(-0x7b44)+14*0x4c);
    const auto flags=c.u8(nation+0x120);
    const auto rng=random_state(s);
    require(invoke(s,0x10192d60,{nation,0,0})==1,"English Conference toggle unavailable");
    require(invoke(s,0x10192d60,{0,0,0})==0,"Null-nation availability changed");
    // Scotland retains its original optional lower divisions in every season.
    const auto scotland=c.u32(c.toc(-0x7b98))+c.u32(c.toc(-0x7af4))*0x128;
    require(invoke(s,0x10192d60,{scotland,0,0})==1,"Other country's extra divisions changed");
    c.put8(nation+0x120,6);
    const auto registers=s.registers; const auto unique=s.unique; const auto pc=s.pc;
    const auto stack=s.memory.copy_out(c.sp()-0x1000,0x1000);
    database_season_limit_leagues(s);
    require(c.u8(nation+0x120)==6,"Season gate cleared the Conference flag");
    require(s.registers==registers && s.unique==unique && s.pc==pc &&
            s.memory.copy_out(c.sp()-0x1000,0x1000)==stack && random_state(s)==rng,
            "Compatibility gate changed ABI, stack or RNG");
    c.put8(nation+0x120,flags);
}

void check_trophy_entrants(guest::Cpu& c, unsigned trophy) {
    const auto entries=c.u32(trophy+0xa4), england=c.u32(c.u32(trophy+4)+8);
    const auto lower=c.u32(c.toc(-0x7a0c)), conference=c.u32(c.toc(-0x6874));
    std::vector<unsigned> entrants;
    require(entries && c.i16(trophy+0x3e)==32,"FA Trophy entrant array incomplete");
    for(unsigned i=0;i<32;++i) {
        const auto club=c.u32(entries+i*8);
        require(club && c.u32(club+0x2c)==england,"FA Trophy entrant is not English");
        require(c.u32(c.u32(club+0x30))==(i<10?lower:conference),"Wrong FA Trophy qualifier division");
        require(std::ranges::find(entrants,club)==entrants.end(),"Duplicate FA Trophy entrant");
        entrants.push_back(club);
    }
}

void check_trophy_reputation_ties(State& s, unsigned trophy) {
    guest::Cpu c(s);
    const auto england=c.u32(c.u32(trophy+4)+8), lower=c.u32(c.toc(-0x7a0c));
    const auto clubs=c.u32(c.toc(-0x7be8)), count=c.u32(c.toc(-0x7b50));
    std::vector<std::pair<unsigned,int>> candidates;
    for(unsigned i=0;i<count && candidates.size()<51;++i) {
        const auto club=clubs+i*0x248, division=c.u32(club+0x30);
        if(division && c.u32(division)==lower && c.u32(club+0x2c)==england) {
            candidates.emplace_back(club,c.i16(club+0x1ec));
            c.put16(club+0x1ec,1000);
        }
    }
    require(candidates.size()==51,"Tied fixture must exercise the original 50-club cap");
    const auto cursor=c.toc(-0x7b84), original_cursor=c.u32(cursor);
    const auto pool=c.toc(-0x7b8c);
    c.put32(cursor,pool); // Exercise the normal path, away from RNG pool wrap.
    require(invoke(s,0x104c7150,{trophy})==1,"Tied FA Trophy qualifier selection");
    require(c.u32(cursor)==pool+20,"FA Trophy must make ten bounded draws for 50 candidates");
    c.put32(cursor,original_cursor);
    check_trophy_entrants(c,trophy);
    // The original Windows comparison with fifty equal keys puts candidate 25
    // in the first ten instead of candidate 0. The subsequent swaps reorder
    // only the first five entries.
    const auto entries=c.u32(trophy+0xa4);
    for(unsigned expected:{25u,1u,2u,3u,4u,5u,6u,7u,8u,9u}) {
        bool found=false;
        for(unsigned i=0;i<10;++i) found |= c.u32(entries+i*8)==candidates[expected].first;
        require(found,"FA Trophy reputation ties differ from original Windows selection");
    }
    for(auto [club,reputation]:candidates)c.put16(club+0x1ec,reputation);
    invoke(s,0x10775170,{entries});
    c.put32(trophy+0xa4,0);
}

std::array<unsigned,22> conference_cup_entrants(guest::Cpu& c, unsigned cup) {
    const auto entries=c.u32(cup+0xa4), division=c.u32(c.toc(-0x6874));
    require(entries && c.u16(cup+0x3e)==22,"Conference Cup must have 22 entrants");
    std::array<unsigned,22> clubs{};
    for(unsigned i=0;i<clubs.size();++i) {
        const auto club=c.u32(entries+i*8);
        require(club && c.u32(c.u32(club+0x30))==division,"Conference Cup selected a club outside the Conference");
        require(std::ranges::find(clubs,club)==clubs.end(),"Duplicate Conference Cup entrant");
        clubs[i]=club;
    }
    return clubs;
}

void check_conference_cup(State& s, unsigned cup) {
    guest::Cpu c(s);
    auto clubs=conference_cup_entrants(c,cup);
    const auto calendar=c.u32(cup+0xa0), draw=c.u32(cup+0xac);
    require(calendar && draw && c.u16(cup+0x3c)==5,"Conference Cup bracket missing");
    constexpr std::array ties{6,8,4,2,1}, direct{12,10,0,0,0}, entrant_start{0,12,0,0,0};
    for(unsigned i=0;i<ties.size();++i) {
        const auto round=calendar+i*0x78;
        require(c.u32(draw+i*4) && c.u16(round+0x1c)==ties[i] && c.u16(round+0x1e)==direct[i] &&
                c.u16(round+0x20)==entrant_start[i],"Conference Cup entries in wrong round");
        require(c.u16(round+0x12)==0,"Conference Cup second leg rule is undefined");
    }
    for(auto [index,days]:{std::pair{3u,49u},std::pair{4u,14u}}) {
        const auto round=calendar+index*0x78;
        require(c.u8(round+0x23)==2 && c.u8(round+0x24)==days,"Conference Cup two-legged tie configuration");
    }

    // Exercise the real competition member writer and load it via the normal
    // constructor. A fresh draw would consume RNG and produce a new calendar.
    const auto calendar_bytes=s.memory.copy_out(calendar,5*0x78);
    const auto rng=random_state(s);
    const auto definition=c.u32(cup+4);
    const auto save_version=c.toc(-0x7b74), prior_version=c.u32(save_version);
    // The full writer sets this before writing any competition members.
    // A member-only fixture has no archive header to supply the version.
    c.put32(save_version,0u-0x3e046);
    invoke(s,0x10484420,{cup,1});
    const auto restored=invoke(s,0x10773520,{0xb4});
    require(restored && invoke(s,0x104c6930,{restored,1999,definition})==restored,"Saved Conference Cup constructor");
    require(c.u16(restored+0x40)==2000 && c.u32(restored+4)==definition &&
            conference_cup_entrants(c,restored)==clubs &&
            s.memory.copy_out(c.u32(restored+0xa0),5*0x78)==calendar_bytes && random_state(s)==rng,
            "Conference Cup member did not restore its calendar, entrants and year");
    invoke(s,0x104c6c00,{restored,1});
    c.put32(save_version,prior_version);

    // Target the original fixed-slot swap quirk with two relegated clubs
    // and two promoted clubs. The expected order uses the original comparer.
    std::ranges::sort(clubs);
    const auto lower=c.u32(c.toc(-0x7bbc))+c.u32(c.toc(-0x7a0c))*0x6c;
    const auto third=c.u32(c.toc(-0x7bbc))+c.u32(c.toc(-0x67ac))*0x6c;
    for(unsigned i=0;i<clubs.size();++i) {
        const auto club=clubs[i];
        auto previous=c.u32(club+0x30);
        if(i==1) previous=0;
        if(i==8) previous=lower;
        if(i==2 || i==4) previous=third;
        c.put32(club+0x34,previous);
        c.put8(club+0x23c,i);
    }
    invoke(s,0x104c7480,{cup});
    constexpr std::array<unsigned,22> order{1,8,21,18,17,16,15,14,13,12,11,10,9,7,6,5,3,2,0,19,20,4};
    const auto moved=conference_cup_entrants(c,cup);
    require(c.u16(cup+0x40)==2001 && c.u16(cup+0x3c)==5,"Conference Cup lost its edition on rollover");
    for(unsigned i=0;i<order.size();++i)
        require(moved[i]==clubs[order[i]],"Conference Cup rollover seed order differs from Windows 3.89");
}

// The career's edition follows its database, and each edition inherits the
// global rules except where competition_rules.cpp says otherwise.
void check_edition_rules(const State& s, unsigned year) {
    const auto expected=year==2000?CompetitionEdition::cm0001_389:year==2002?CompetitionEdition::si_3968:CompetitionEdition::cm0102;
    require(competition_edition(s)==expected,"Database did not select its competition edition");
    // The checked-in edition, without the environment's test presets.
    const auto& rules=edition_rules(competition_edition(s));
    require(rules.england.promotion.conference_promoted==global_rules.england.promotion.conference_promoted &&
            rules.england.premier_named_reserves==global_rules.england.premier_named_reserves &&
            rules.brazil.serie_b_single_table==global_rules.brazil.serie_b_single_table &&
            rules.international.inter_american_cup_last_edition==global_rules.international.inter_american_cup_last_edition,
            "Edition lost an inherited global rule");
    require((rules.england.conference_cup!=nullptr)==(year==2000) && rules.unavailable_countries.size()==(year==2000?7u:0u) &&
            (rules.england.conference.club_payment==0)==(year==2000),"Edition rules leaked across seasons");
}

// The career's own rules: the member's text, what it applies, and what it
// keeps for a build that understands more.
void check_career_rules_text() {
    CareerRules career;
    require(career.text(CompetitionEdition::cm0001_389)==
            "CM-NATIVE-RULES 1\nedition cm0001_389\nrevision 1\nhistorical_rules off\n" &&
            !career.edition() && !career.customized(),"New career's rules member");
    const std::string member="CM-NATIVE-RULES 1\nedition cm0001_389\nrevision 1\nhistorical_rules on\n"
        "set england.premier_named_reserves 0\nset future.rule 5\nset brazil.serie_b_single_table 2\n"
        "set england.promotion.conference_promoted 1\nset england.promotion.third_relegated 1\nlater statement\n";
    auto ignored=career.read(member);
    require(ignored.size()==3 && career.edition()==CompetitionEdition::cm0001_389 && career.historical_rules(),
            "Career rules member header or ignored statements");
    auto* rules=career.customized();
    require(rules && (*rules)[0][0].england.premier_named_reserves==0 &&
            (*rules)[0][0].england.promotion.conference_promoted==1 && (*rules)[0][0].england.promotion.third_relegated==1 &&
            (*rules)[0][0].brazil.serie_b_single_table && (*rules)[0][0].england.conference_cup &&
            (*rules)[0][0].england.conference.club_payment==0,"Career override or inherited edition rule");
    // Written back with every statement, the ones this build ignored included.
    require(career.text(CompetitionEdition::cm0102)==member,"Career rules member changed in a round trip");
    // The test presets stay last: an override that names the port's values loses to them.
    ignored=career.read("CM-NATIVE-RULES 1\nedition cm0102\nrevision 1\nset england.premier_named_reserves 7\n"
                        "set brazil.serie_b_single_table 1\nset england.promotion.conference_promoted 3\n"
                        "set england.promotion.third_relegated 3\n");
    rules=career.customized();
    require(ignored.empty() && rules && (*rules)[0][0].england.premier_named_reserves==7 &&
            (*rules)[1][0].england.premier_named_reserves==0 && (*rules)[1][0].england.promotion.conference_promoted==1 &&
            (*rules)[1][0].brazil.serie_b_single_table && !(*rules)[0][1].brazil.serie_b_single_table,
            "Original presets no longer apply after career overrides");
    // One side of the Conference exchange alone changes neither.
    ignored=career.read("CM-NATIVE-RULES 1\nedition cm0102\nrevision 1\nset england.promotion.conference_promoted 1\n");
    require(ignored.size()==1 && !career.customized() && !career.historical_rules(),
            "Unbalanced Conference exchange was applied");
    // A format, edition or revision this build does not know: nothing applied, text intact.
    using namespace std::string_view_literals;
    for(std::string_view unknown:{"CM-NATIVE-RULES 2\nedition cm0102\nrevision 1\nset brazil.serie_b_single_table 0\n"sv,
                                  "CM-NATIVE-RULES 1\nedition cm0304\nrevision 1\nset brazil.serie_b_single_table 0\n"sv,
                                  "CM-NATIVE-RULES 1\nedition cm0102\nrevision 2\nset brazil.serie_b_single_table 0\n"sv,
                                  "CM-NATIVE-RULES 1\nedition cm0102\n"sv,"garbage\0bytes\n"sv}) {
        ignored=career.read(unknown);
        require(ignored.size()==1 && !career.edition() && !career.customized() &&
                career.text(CompetitionEdition::cm0102)==unknown,"Unknown career rules member was applied or rewritten");
    }
    // An empty member (an interrupted write) is no member: the next save writes a whole one.
    require(career.read("\n").empty() && !career.edition() &&
            career.text(CompetitionEdition::cm0102).starts_with("CM-NATIVE-RULES 1\nedition cm0102\n"),"Empty rules member was kept");
    career.reset();
    require(career.text(CompetitionEdition::si_3968).find("edition si_3968\n")!=std::string::npos,"Reset kept a rules member");
}

// Optional historical rule changes: each competition follows its own
// timeline, only with the career's switch on, and the career's overrides
// still come last.
void check_rule_timelines() {
    // Listed out of season order on purpose: two successive reforms of the
    // English exchange, one unrelated Brazilian change, nothing for the Trophy.
    static constexpr std::array timeline{
        RuleChange{RuleCompetition::english_leagues,2006,[](CompetitionRules& rules){rules.england.promotion.playoff_clubs=8;},"fixture"},
        RuleChange{RuleCompetition::brazilian_serie_b,2003,[](CompetitionRules& rules){rules.brazil.serie_b_single_table=false;},"fixture"},
        RuleChange{RuleCompetition::english_leagues,2004,[](CompetitionRules& rules){
            rules.england.promotion.conference_promoted=1;rules.england.promotion.third_relegated=1;},"fixture"},
        RuleChange{RuleCompetition::english_leagues,2006,[](CompetitionRules& rules){rules.england.promotion.conference_promoted=2;},"fixture"},
    };
    const auto& edition=edition_rules(CompetitionEdition::cm0102);
    const auto english=[&](int season,bool historical=true,const CareerRules* career=nullptr) {
        return resolve_rules(edition,timeline,historical,career,RuleCompetition::english_leagues,season,2001).england.promotion;
    };
    require(english(2003).conference_promoted==3 && english(2003).playoff_clubs==4,"Rule change applied before its first season");
    require(english(2004).conference_promoted==1 && english(2004).third_relegated==1 && english(2005).playoff_clubs==4,
            "Rule change missing at or after its first season");
    // The second reform changes two fields and carries the rest of the first forward.
    require(english(2006).conference_promoted==2 && english(2006).third_relegated==1 && english(2006).playoff_clubs==8 &&
            english(2030).conference_promoted==2,"Successive rule changes did not accumulate in season order");
    require(english(2030,false).conference_promoted==3 && english(2030,false).playoff_clubs==4,
            "Historical rules applied with the career's switch off");
    // Timelines are independent: one competition's reform is not another's.
    const auto serie_b=[&](int season,bool historical) {
        return resolve_rules(edition,timeline,historical,nullptr,RuleCompetition::brazilian_serie_b,season,2001);
    };
    require(serie_b(2002,true).brazil.serie_b_single_table && !serie_b(2003,true).brazil.serie_b_single_table &&
            serie_b(2003,false).brazil.serie_b_single_table && serie_b(2010,true).england.promotion.conference_promoted==3,
            "Competition timelines are not independent");
    const auto trophy=resolve_rules(edition,timeline,true,nullptr,RuleCompetition::fa_trophy,2030,2001);
    require(trophy.england.promotion.conference_promoted==3 && trophy.brazil.serie_b_single_table &&
            trophy.england.fa_trophy.qualification==edition.england.fa_trophy.qualification,"Empty timeline changed rules");
    // The career's explicit overrides apply after the historical changes.
    CareerRules career;
    require(career.read("CM-NATIVE-RULES 1\nedition cm0102\nrevision 1\nhistorical_rules on\n"
                        "set england.promotion.conference_promoted 3\nset england.promotion.third_relegated 3\n").empty(),
            "Timeline fixture member");
    require(english(2006,true,&career).conference_promoted==3 && english(2006,true,&career).third_relegated==3 &&
            english(2006,true,&career).playoff_clubs==8,"Career override did not apply after the historical change");
    // A one-sided override that a historical change leaves unbalanced changes neither side.
    require(career.read("CM-NATIVE-RULES 1\nedition cm0102\nrevision 1\nhistorical_rules on\n"
                        "set england.promotion.conference_promoted 3\n").empty() &&
            english(2003,true,&career).conference_promoted==3 && english(2004,true,&career).conference_promoted==1 &&
            english(2004,true,&career).third_relegated==1,"Historical change left a one-sided override unbalanced");
    // An edition's rules are its first season's: a later database does not replay older reforms.
    const auto later=resolve_rules(edition,timeline,true,nullptr,RuleCompetition::english_leagues,2030,2004).england.promotion;
    require(later.conference_promoted==2 && later.third_relegated==3 && later.playoff_clubs==8,
            "A change from before the edition's season was applied over its rules");
    // No change is verified yet, so the checked-in timelines are empty.
    require(rule_timeline().empty(),"A historical rule change was added without its season checks");
}

void check_english_construction(State& s) {
    guest::Cpu c(s);
    // Negative control: hide the Lower Division binding only in this fixture.
    // An insufficient candidate pool must return without drawing or leaving
    // a partially initialized entrant array.
    const auto object=allocate(s,0xb4,true);
    c.put32(object+4,c.u32(c.toc(-0x7bbc))+c.u32(c.toc(-0x6878))*0x6c);
    const auto lower_slot=c.toc(-0x7a0c), lower=c.u32(lower_slot);
    const auto rng=random_state(s);
    c.put32(lower_slot,0xffffffff);
    require(invoke(s,0x104c7150,{object})==0 && c.u32(object+0xa4)==0 && random_state(s)==rng,
            "FA Trophy drew from an insufficient qualifier pool");
    c.put32(lower_slot,lower);
    // Windows 3.89 sorts, draws and stores the ten qualifiers before it finds
    // the Conference short: a direct selector call reports failure with the
    // RNG advanced and the array kept (the constructor then raises the error).
    const auto conference_slot=c.toc(-0x6874), conference_binding=c.u32(conference_slot);
    const auto cursor=c.toc(-0x7b84), original_cursor=c.u32(cursor), pool=c.toc(-0x7b8c);
    c.put32(conference_slot,0xffffffff);
    c.put32(cursor,pool);
    require(invoke(s,0x104c7150,{object})==0 && c.u32(object+0xa4)!=0 && c.u32(cursor)==pool+20,
            "FA Trophy with a short Conference did not draw as Windows 3.89");
    // The next-season caller (return address 0x104c756c) takes 0 for success,
    // so there the short Conference fails before any draw, with no array left
    // and the caller's stack pointer and saved registers as at the call.
    {
        const auto kept=c.u32(object+0xa4);
        invoke(s,0x10775170,{kept});c.put32(object+0xa4,0);
        c.put32(cursor,pool);
        const auto random=random_state(s);
        std::array<std::uint32_t,20> saved{};
        const auto kept_registers=[&]{
            std::array<std::uint32_t,20> now{};
            now[0]=std::uint32_t(s.r<4>(4));
            for(unsigned n=13;n<32;++n)now[n-12]=std::uint32_t(s.r<4>(n*4));
            return now;
        };
        saved=kept_registers();
        const auto result=invoke(s,0x104c7150,{object},0x104c756c);
        require(std::int32_t(result)<0 && c.u32(object+0xa4)==0 && random_state(s)==random && kept_registers()==saved,
                "FA Trophy rollover with a short Conference did not fail cleanly before drawing");
        // The lines below read the array of a constructor-path selection.
        require(invoke(s,0x104c7150,{object})==0 && c.u32(object+0xa4)!=0,"FA Trophy fixture restore");
    }
    c.put32(cursor,original_cursor);
    c.put32(conference_slot,conference_binding);
    {
        const auto entries=c.u32(object+0xa4), england=c.u32(c.u32(object+4)+8);
        std::vector<unsigned> stored;
        for(unsigned i=0;i<10;++i) {
            const auto club=c.u32(entries+i*8);
            require(club>=c.u32(c.toc(-0x7be8)) && c.u32(club+0x2c)==england && c.u32(c.u32(club+0x30))==lower &&
                    std::ranges::find(stored,club)==stored.end(),"Short-Conference FA Trophy lost a stored qualifier");
            stored.push_back(club);
        }
        invoke(s,0x10775170,{entries}); c.put32(object+0xa4,0);
    }
    check_trophy_reputation_ties(s,object);
    deallocate(s,object);

    const auto row=c.toc(-0x7b44)+14*0x4c;
    c.put8(c.u32(row)+0x120,6);
    database_season_limit_leagues(s);
    require(c.u8(c.u32(row)+0x120)==6,"English Conference selection was lost");
    constructing_england=true;
    // Enter the saved-career loader's bracket before any Conference Cup
    // member exists in the fixture's temporary directory.
    const bool routed=s.platform->database_seasons().active();
    s.platform->quick_save_load_begin(s);
    require(!s.platform->database_seasons().active() && s.platform->career_loading() &&
            english_season_rules(s).conference_cup,"Load entry lost the running career's edition");
    require(invoke(s,0x103510d0,{row})==1 && c.u32(row+0xc)==10,
            "Old save grew a Conference Cup with no saved member");
    // A load that never reaches its database returns to the same career.
    s.platform->quick_save_load_end(s);
    require(s.platform->database_seasons().active()==routed && !s.platform->career_loading() &&
            english_season_rules(s).conference_cup,"Refused load changed the running career's season state");
    invoke(s,0x10191e20);
    require(invoke(s,0x103510d0,{row})==1,"Actual England constructor");
    require(c.u32(row+0xc)==11 && fixture_insertions>0,"Incomplete English constructor coverage");
    constexpr std::array definitions{-0x67a0,-0x67a4,-0x67a8,-0x67ac,-0x6874,-0x6918,-0x691c,-0x6924,-0x6920,-0x6878,-0x687c};
    constexpr std::array league_sizes{20,24,24,24,22};
    for(unsigned i=0;i<definitions.size();++i) {
        const auto competition=c.u32(c.u32(row+0x10)+i*4);
        require(competition && c.u32(competition+4)==c.u32(c.toc(-0x7bbc))+c.u32(c.toc(definitions[i]))*0x6c,
                "Missing/wrong English competition object");
        require(c.i16(competition+0x40)==2000,"English competition constructed for the wrong year");
        if(i<league_sizes.size())
            require(c.i16(competition+0x3e)==league_sizes[i] && c.u32(competition+0xb0),
                    "English league participant selection incomplete");
    }
    const auto competitions=c.u32(row+0x10), conference=c.u32(competitions+4*4), trophy=c.u32(competitions+9*4);
    const auto calendar=c.u32(conference+0xbc);
    require(calendar && c.u16(conference+0xa6)==42,"Conference calendar incomplete");
    for(unsigned round=0;round<42;++round) {
        const auto slot=calendar+round*0x4c;
        require(c.u8(slot+0xc)==255 && c.u8(slot+0xd)==255 && c.u8(slot+0xe)==255,
                "Conference round contains a later edition's kickoff option");
    }
    check_trophy_entrants(c,trophy);
    require(c.u32(trophy+0xa0) && c.u8(c.u32(trophy+0xa0)+3*0x78+0x23)==2,
            "FA Trophy semifinal lost the original leg rule (2 on the Mac and in Windows 3.89)");
    invoke(s,0x104c7480,{trophy});
    require(c.u16(trophy+0x40)==2001 && c.u8(c.u32(trophy+0xa0)+3*0x78+0x23)==2,
            "FA Trophy switched editions on rollover");
    check_trophy_entrants(c,trophy);
    check_conference_cup(s,c.u32(competitions+10*4));
    constructing_england=false;
    std::cout<<"All eleven English competitions, shared cup member reload and both cup rollovers passed (fixture insertion stubbed)\n";
}
// Not a check: lists what each country's constructor says on the loaded database. Every selectable country is
// selected with its optional lower divisions; fixture insertion is stubbed as in check_english_construction.
void survey_constructors(State& s) {
    guest::Cpu c(s);
    const auto rows=c.toc(-0x7b44), ui=c.toc(-0x7bb0);
    c.put16(ui+0x25819e,10); // Original Select All command.
    invoke(s,0x103366f0,{0xffffffff});
    for(unsigned i=0;i<34;++i) {
        const auto nation=c.u32(rows+i*0x4c);
        if(nation && (c.u8(nation+0x120)&2) && invoke(s,0x10192d60,{nation,0,0})==1) c.put8(nation+0x120,c.u8(nation+0x120)|4);
    }
    database_season_limit_leagues(s);
    constructing_england=true;
    invoke(s,0x10191e20);
    // Run the original dispatcher itself, as New Game does after Select All. Dialogs are recorded with the row
    // whose constructor was running; the constructors are not stubbed.
    constructor_targets.clear();
    for(unsigned i=0;i<34;++i) if(c.u32(rows+i*0x4c+8)) constructor_targets.push_back(c.u32(c.u32(rows+i*0x4c+8)));
    std::vector<std::string> messages; survey_messages=&messages; survey_row=0xffffffff;
    std::string outcome;
    try {
        // Enter the new-world routine (0x10343630, the page that calls 0x10191cd0) through its own prologue, then
        // continue at 0x10344200 on its second pass (r14=2, r13=0x2c progress total, as 0x10346b68..0x10346b7c
        // set them): the first pass only counts progress and everything before is staff/club preparation, skipped here.
        // Every service-creation stage up to the 0x10191cd0 call then runs as the original. Stop when 0x10191cd0 returns.
        s.wr<4>(4128,return_sentinel); s.pc=0x10343630; s.budget=s.executed+22;
        try { run_until(s,return_sentinel); }
        catch(const std::runtime_error& e) { if(std::string_view(e.what())!="Native instruction budget exhausted") throw; }
        require(s.pc==0x10343688,"New-world prologue did not reach its frame");
        s.pc=0x10343990; s.budget=s.executed+20000000000ull;
        run_until(s,0x10344200);
        std::cout<<"reached 0x10344200 after "<<s.executed<<" instructions\n";
        s.wr<4>(14*4,2); s.wr<4>(13*4,0x2c);
        // The generated page reaches 0x10345bb4 through the native 0x10191cd0 wrapper, so this stop is never seen and
        // the page runs on into its next stage; that later stage fails (fixture) after all constructors have spoken.
        run_until(s,0x10345bb4);
        outcome="0x10191cd0 returned "+std::to_string(s.r<4>(12));
    }
    catch(const std::exception& e) {
        guest::Cpu c2(s); std::string chain; auto sp=c2.sp();
        for(unsigned depth=0;depth<12 && sp;++depth) { const auto back=c2.u32(sp); if(!back) break; chain+=" "+hex_address(c2.u32(back+8)); sp=back; }
        outcome="run ended (after the dispatcher; later stage) at "+hex_address(s.pc)+", return addresses up the stack:"+chain+": "+e.what(); }
    survey_messages=nullptr; constructor_targets.clear();
    std::cout<<outcome<<", "<<messages.size()<<" dialogs\n";
    for(const auto& message:messages)std::cout<<"  dialog "<<message<<"\n";
    // The two conditions read from the original (0x105d2330 for BEL_THIRD, 0x106dbcb0 for cro_a1): each walks the
    // club table (stride 0x248) counting clubs whose division (club+0x30 -> first word) is the competition's
    // definition, and wants exactly 16. Recount with the same tests on the memory the run left.
    {
        const auto clubs=c.u32(c.toc(-0x7be8)), total=c.u32(c.toc(-0x7b50)), nations=c.u32(c.toc(-0x7b04));
        const auto definition_name=[&](unsigned id) {
            std::string name; const auto record=c.u32(c.toc(-0x7bbc))+id*0x6c;
            for(unsigned j=0;j<50 && c.u8(record + 0x1a + j);++j) name+=char(c.u8(record + 0x1a + j));
            return name;
        };
        const auto belgium_division=c.u32(c.toc(-0x6968)), belgium_nation=c.u32(c.toc(-0x6788));
        const auto croatia_division=c.u32(c.toc(-0x6a40));
        unsigned belgium=0, belgium_by_division=0, croatia=0;
        for(unsigned i=0;i<total;++i) {
            const auto club=clubs+i*0x248, division=c.u32(club+0x30), group=c.u32(club+0x38);
            if(division && c.u32(division)==croatia_division) ++croatia;
            if(i+2*nations>=total || !division || c.u32(division)!=belgium_division) continue;
            ++belgium_by_division;
            if(group && c.u32(group)==belgium_nation) ++belgium;
        }
        std::cout<<"BEL_THIRD 0x105d2330: wants 16 clubs (club index < "<<total-2*nations<<", club+0x30 -> first word == "<<belgium_division
                 <<" ["<<definition_name(belgium_division)<<"], club+0x38 -> first word == "<<belgium_nation<<"): found "<<belgium
                 <<" ("<<belgium_by_division<<" by division alone)\n";
        {
            // Context for Belgium: where the Belgian clubs sit (club+0x2c is the club's nation, as in check_trophy_entrants).
            const auto nation=c.u32(c.toc(-0x7b44)+10*0x4c);
            std::vector<std::pair<unsigned,unsigned>> by_division; unsigned undivided=0;
            for(unsigned i=0;i+2*nations<total;++i) {
                const auto club=clubs+i*0x248, division=c.u32(club+0x30);
                if(c.u32(club+0x2c)!=nation) continue;
                if(!division) { ++undivided; continue; }
                const auto id=c.u32(division);
                const auto found=std::ranges::find_if(by_division,[id](const auto& entry){return entry.first==id;});
                if(found==by_division.end()) by_division.emplace_back(id,1); else ++found->second;
            }
            std::cout<<"Belgian clubs by division:";
            for(auto [id,count]:by_division) std::cout<<" "<<definition_name(id)<<"="<<count;
            std::cout<<" no division="<<undivided<<"\n";
        }
        std::cout<<"cro_a1 0x106dbcb0: wants 16 clubs (club+0x30 -> first word == "<<croatia_division<<" ["<<definition_name(croatia_division)
                 <<"], first 16 taken): found "<<croatia<<"\n";
    }
    constructing_england=false;
}
void check_legacy_boundaries(State& s) {
    guest::Cpu c(s);
    require(s.platform->database_seasons().legacy_2000(),"CM00 profile missing");
    const auto rows=c.toc(-0x7b44);
    constexpr std::array blocked{8u,11u,15u,20u,21u,23u,31u};
    for(auto i:blocked) {
        const auto row=rows+i*0x4c,nation=c.u32(row);
        require(nation && c.u8(nation+0x120)==0 && !c.u32(row+8),"Unavailable country remains selectable");
    }
    require(c.u32(c.toc(-0x6b88))==65,"Historical Brazilian top-flight alias missing");
    const auto registers=s.registers; const auto unique=s.unique;
    s.wr<4>(4128,0x10152f90);
    require(database_season_optional_index(s),"Audited unavailable Serie D index rejected");
    s.wr<4>(4128,0x10152f94);
    require(!database_season_optional_index(s),"Unknown index failure was suppressed");
    s.registers=registers;s.unique=unique;
    // Exercise the original constructor dispatcher. Stub only the country/
    // global constructors themselves, and inspect which descriptors it calls.
    const auto descriptors=s.memory.copy_out(rows,34*0x4c);
    std::vector<std::pair<unsigned,unsigned>> flags;
    const auto ui=c.toc(-0x7bb0); const auto category=c.u16(ui+0x25819e);
    const auto total=c.u32(c.toc(-0x62b8)),full=c.u32(c.toc(-0x62b4)),start_nation=c.u32(c.toc(-0x64c4));
    constructor_targets.clear();
    for(unsigned i=0;i<34;++i) {
        const auto callback=c.u32(rows+i*0x4c+8);
        if(callback)constructor_targets.push_back(c.u32(callback));
        const auto nation=c.u32(rows+i*0x4c);
        if(!nation)continue;
        flags.emplace_back(nation,c.u8(nation+0x120));
    }
    c.put16(ui+0x25819e,10); // Original Select All command.
    invoke(s,0x103366f0,{0xffffffff});
    require(c.u32(c.toc(-0x62b8))==19 && c.u32(c.toc(-0x62b4))==19,"Select All enabled unavailable countries");
    // Select All activates countries. The optional lower divisions still
    // have their separate toggle, tested through the original league builder.
    require((c.u8(c.u32(rows+14*0x4c)+0x120)&2)!=0,"Select All did not activate England");
    for(auto i:blocked)require(c.u8(c.u32(rows+i*0x4c)+0x120)==0,"Select All bypassed country compatibility");
    std::vector<unsigned> called;
    constructor_rows=&called;
    try { require(invoke(s,0x10191cd0)==1,"Original country constructor dispatcher"); }
    catch(...) { constructor_rows=nullptr; throw; }
    constructor_rows=nullptr;
    constructor_targets.clear();
    require(called.size()==27,"Expected 19 supported countries and eight global competitions");
    for(auto i:blocked)require(std::ranges::find(called,i)==called.end(),"Unavailable country constructor executed");
    s.memory.copy_in(rows,descriptors);
    for(auto [nation,flag]:flags)c.put8(nation+0x120,flag);
    c.put16(ui+0x25819e,category);c.put32(c.toc(-0x62b8),total);c.put32(c.toc(-0x62b4),full);c.put32(c.toc(-0x64c4),start_nation);
    // Date arithmetic uses the same five-byte representation as saved games.
    const auto dates=allocate(s,24,true);
    invoke(s,0x107158a0,{dates,28,1,2000,0xffffffff});
    invoke(s,0x10717890,{dates+8,dates,1});
    require(c.u16(dates+8)==59 && c.u16(dates+10)==2000 && c.u8(dates+12)==1,"February 29 was skipped");
    invoke(s,0x10717890,{dates+16,dates+8,1});
    require(c.u16(dates+16)==60 && c.u16(dates+18)==2000,"March 1 leap ordinal");
    invoke(s,0x107158a0,{dates,31,11,2000,0xffffffff});
    invoke(s,0x10717890,{dates+8,dates,1});
    require(c.u16(dates+8)==0 && c.u16(dates+10)==2001 && c.u8(dates+12)==0,"Leap-year rollover");
    deallocate(s,dates);
    std::cout<<"CM00 identity, bounded missing indexes, 19-country constructor gating and leap boundaries passed\n";
}
void check_layout(State& s) {
    guest::Cpu c(s); auto& seasons=s.platform->database_seasons();
    const auto ui=c.toc(-0x7bb0),display=c.u32(ui+0x24);
    const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
    const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
    const auto rect=[&](unsigned at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
    const auto scratch=allocate(s,32,true);
    c.put16(scratch+4,600);c.put16(scratch+6,800);
    c.set({{3,scratch+16},{4,16},{5,scratch},{6,0},{7,0},{8,0}});
    require(s.platform->dispatch(s,"CarbonLib::NewGWorld"),"Offscreen canvas");
    c.put32(c.r(2)-0x355c,c.u32(scratch+16));
    c.set(3,c.u32(scratch+16));require(s.platform->dispatch(s,"CarbonLib::GetPortPixMap"),"Offscreen pixel map");
    c.put32(c.r(2)-0x3560,c.r(3));
    deallocate(s,scratch);
    for(auto canvas:{GuiCanvas{800,600},GuiCanvas{1280,900},GuiCanvas{3200,1800}}) {
        require(s.platform->resize_gui_canvas(s,{801,601}) && s.platform->resize_gui_canvas(s,canvas),"League canvas resize");
        invoke(s,0x10719560,{display,1}); invoke(s,0x10335500);
        const unsigned count=c.u16(display+0x129da6);
        if(seasons.legacy_2000()) {
            const auto england=c.u32(c.toc(-0x7b44)+14*0x4c);
            bool has_conference=false;
            for(unsigned i=0;i<count;++i)
            {
                has_conference |= c.u32(control(i)+0x48)==england && c.i16(control(i)+10)==0x38;
                for(auto blocked:{8u,11u,15u,20u,21u,23u,31u})
                    require(c.u32(control(i)+0x48)!=c.u32(c.toc(-0x7b44)+blocked*0x4c),"Unavailable nation leaked into league controls");
            }
            require(has_conference,"Conference toggle missing from league controls");
        }
        std::vector<unsigned> actions;
        for(unsigned i=0;i<count;++i) {
            actions.push_back(c.u32(control(i)+8)); actions.push_back(c.u32(control(i)+0x48));
        }
        const auto registers=s.registers; const auto unique=s.unique; const auto pc=s.pc,sp=c.sp();
        const auto stack=s.memory.copy_out(sp-0x8000,0x8000),rng=random_state(s);
        database_season_after_build(s,0x10335500);
        require(s.registers==registers && s.unique==unique && s.pc==pc &&
                s.memory.copy_out(sp-0x8000,0x8000)==stack && random_state(s)==rng,"League layout changed ABI/stack/RNG");
        require(c.u16(display+0x129da6)==count+3,"Season selector missing");
        for(unsigned i=0;i<count;++i)
            require(c.u32(control(i)+8)==actions[i*2] && c.u32(control(i)+0x48)==actions[i*2+1],"Original league action or nation changed");
        int countries=-1;
        for(unsigned i=0;i<c.u16(display+0x129da4);++i)
            if(c.u8(grid(i)+0xbb0)==7 && c.u8(grid(i)+0xbb1)>1) countries=i;
        require(countries>=0,"League table missing");
        const auto box=rect(grid(countries));
        require(box.top>=225 && box.bottom<=canvas.height-65 && box.left>=110 && box.right<=canvas.width-20,
                "League table overlaps selector/footer or escapes canvas");
        require(c.u8(grid(countries)+0xbb1)<=30,"League table exceeds original row capacity");
        for(int id:{seasons.previous_control,seasons.next_control}) {
            require(id>=0 && unsigned(id)>=count,"Invalid season control id");
            const auto at=control(id); const auto button=rect(at+0x10);
            require(button.bottom<=box.top && button.top>=145 && button.width()>=30,"Season control overlaps leagues");
            if(!(c.u32(at+0x20)&0x800)) {
                c.put32(c.toc(-0x5c10),(button.left+button.right)/2);
                c.put32(c.toc(-0x5c14),(button.top+button.bottom)/2);
                c.put8(c.toc(-0x7bac),1);c.put8(c.toc(-0x5c0c),0);
                require(std::int16_t(invoke(s,c.u8(display)==1?0x1071aa10:0x1071a280,{display}))==id,"Original picker missed season arrow");
            }
        }
    }
    std::cout<<"League Yoga geometry, original actions, selector picking and ABI/RNG passed\n";
}
void check_missing_pack(State& s, const std::filesystem::path& runtime) {
    guest::Cpu c(s); auto& seasons=s.platform->database_seasons();
    const auto file=runtime/"Files/Databases"/seasons.packs()[seasons.default_index()].directory/"Data/index.dam";
    const auto held=file.string()+".fixture-missing";
    struct Restore {
        std::filesystem::path file, held;
        ~Restore(){std::error_code error; std::filesystem::rename(held,file,error);}
    } restore{file,held};
    std::filesystem::rename(file,held);
    const auto clubs=c.u32(c.toc(-0x7be8)); const auto rng=random_state(s);
    require(seasons.request(-1),"Missing-pack switch request");
    invoke(s,0x10719560,{c.u32(c.toc(-0x7bb0)+0x24),1});
    database_season_before_build(s,0x10335500);
    require(seasons.year()==2002 && seasons.loaded()==seasons.selected() && !seasons.pending() && !seasons.error.empty(),"Missing pack changed selected source");
    require(c.u32(c.toc(-0x7be8))==clubs && c.u16(c.toc(-0x7b80)+2)==2002 && random_state(s)==rng,"Preflight destroyed the previous world");
    seasons.error.clear();
}
void check_2002_history(State& s, bool expected) {
    guest::Cpu c(s);
    const auto histories=c.u32(c.toc(-0x7a34)), count=c.u32(c.toc(-0x7a38));
    const auto competitions=c.u32(c.toc(-0x7bbc)), clubs=c.u32(c.toc(-0x7be8));
    struct Result { unsigned competition, winner, runner_up; int third; };
    for(const auto result:{Result{447,13241,13288,13406},Result{326,9413,6524,-1},Result{328,4012,3234,-1}}) {
        unsigned found=0;
        for(unsigned i=0;i<count;++i) {
            const auto row=histories+i*0x1c;
            if(c.u16(row+0x18)!=2002 || c.u32(row+4)!=competitions+result.competition*0x6c)continue;
            ++found;
            require(c.u32(row+8)==clubs+result.winner*0x248 &&
                    c.u32(row+0xc)==clubs+result.runner_up*0x248,"2002 history finalist links");
            require(c.u32(row+0x10)==(result.third<0?0:clubs+unsigned(result.third)*0x248),"2002 history third place");
            require(c.u32(row+0x14)==0,"Unknown/joint host must not be stored as fourth place");
        }
        require(found==(expected?1u:0u),"2002 competition history missing/duplicated or leaked into 2001 base");
    }
    unsigned awards_found=0;
    const auto awards=c.u32(c.toc(-0x74cc)), award_count=c.u32(c.toc(-0x74d0));
    for(unsigned i=0;i<award_count;++i) {
        const auto row=awards+i*0x3c;
        if(c.u16(row+0x38)!=2002 || c.u32(row+0x10)!=159)continue;
        ++awards_found;
        require(c.u32(row+4)==8 && c.u32(row+0x14)==166160 &&
                c.u32(row+0x20)==40261 && c.u32(row+0x24)==99682,"Ronaldo award name/staff/goal fields");
        for(unsigned offset:{0x18,0x1c,0x28,0x2c,0x30,0x34})
            require(c.i32(row+offset)==-1,"Tied Silver Shoe must not become a false ranking");
    }
    require(awards_found==(expected?1u:0u),"2002 Golden Shoe missing/duplicated or leaked into 2001 base");
}
void check_archive(State& s) {
    guest::Cpu c(s);
    const auto year=s.platform->database_seasons().year();
    const auto language=c.u8(c.toc(-0x7ab0));
    const auto club_count=c.u32(c.toc(-0x7b50)),staff_count=c.u32(c.toc(-0x7be4)),taiwan=c.u32(c.toc(-0x66d0));
    const auto scratch=allocate(s,2048,true);
    const auto text=[&](unsigned offset,std::string_view value) {
        for(unsigned i=0;i<=value.size();++i)c.put8(scratch+offset+i,i<value.size()?value[i]:0);
        return scratch+offset;
    };
    static unsigned archive_sequence=0;
    const auto archive=text(0,"CM User Data:SeasonFixture"+std::to_string(++archive_sequence)+".sav"),member=text(128,"fixture-dates");
    const auto current=c.toc(-0x7b80),start=c.toc(-0x7a9c);
    // Exercise the actual start-date selector for a calendar-year country and
    // a summer-start country, then serialize the resulting original Date bytes.
    for(auto country:{std::string_view("Brazil"),std::string_view("England")}) {
        bool found=false;
        for(unsigned i=0;i<34;++i) {
            const auto row=c.toc(-0x7b44)+i*0x4c,nation=c.u32(row);
            if(!nation)continue;
            std::string name;
            for(unsigned j=0;j<50 && c.u8(nation + 0xae + j);++j)name+=char(c.u8(nation + 0xae + j));
            if(name!=country)continue;
            c.put32(c.toc(-0x64c4),nation);require(invoke(s,0x1034e8a0)==1,"Original start date selection");
            require(s.memory.copy_out(start,5)==s.memory.copy_out(row+0x2c,5),"Selected competition date differs from descriptor");
            // Fresh-world entry10343990 subtracts four preseason weeks before
            // publishing the actual career start. Brazil's January competition
            // therefore starts its playable career in December of this year.
            invoke(s,0x10717cb0,{scratch+640,start,4});
            s.memory.copy_in(start,s.memory.copy_out(scratch+640,5));
            require(c.u16(start+2)==year,"Actual preseason career date differs from selected year");
            if(year==2000) {
                require(c.u16(start)==(country=="Brazil"?355:196) && c.u8(start+4)==1,"Leap-year preseason date");
                require(c.u16(row+0x2c)==(country=="Brazil"?17:224) &&
                        c.u8(row+0x30)==(country=="Brazil"?0:1),"Leap-year competition date");
            }
            found=true;break;
        }
        require(found,"Representative country descriptor missing");
    }
    if(year==2000) {
        c.put8(c.toc(-0x7ab0),9); // Serialized Portuguese name-table marker.
        const auto brazil=c.u32(c.toc(-0x7b98))+c.u32(c.toc(-0x7b68))*0x128;
        constexpr std::string_view localized="Brasil";
        for(unsigned i=0;i<=localized.size();++i)c.put8(brazil + 0xae + i,i<localized.size()?localized[i]:0);
        const auto registers=s.registers;
        c.put32(c.toc(-0x6b88),0xffffffff);
        c.set({{3,text(256,"Brazilian National Championship")},{4,65}});
        database_season_competition_index_name(s);
        require(c.u32(c.toc(-0x6b88))==65,"Localized nation name broke competition identity");
        s.registers=registers;
    }
    require(invoke(s,0x1009ef60,{archive,0,4})==1,"Database archive write");
    const auto current_bytes=s.memory.copy_out(current,8),start_bytes=s.memory.copy_out(start,8);
    const auto stream=invoke(s,0x107475b0,{archive,member,1,0,4});require(stream,"Date fixture section open");
    require(invoke(s,0x10747cc0,{current,8,1,stream})==1 && invoke(s,0x10747cc0,{start,8,1,stream})==1,"Date fixture write");
    c.put32(scratch+900,stream);invoke(s,0x10747900,{scratch+900});
    // The save's rules member, as the original loader's extraction leaves it
    // in the temporary folder. It belongs to the career being loaded: nothing
    // reads it before the running world is released.
    const auto edition=competition_edition(s);
    auto& career=s.platform->career_rules();
    const auto rules_file=s.platform->writable_directory()/
        (std::filesystem::is_directory(s.platform->writable_directory()/"Files/Temp")?"Files/Temp":"Files")/CareerRules::member_name;
    const auto rules_member="CM-NATIVE-RULES 1\nedition "+std::string(edition_name(edition))+
        "\nrevision 1\nhistorical_rules on\nset brazil.serie_b_single_table 0\nset future.rule 5\n";
    std::ofstream(rules_file,std::ios::binary)<<rules_member;
    s.platform->quick_save_load_begin(s);
    require(!career.edition() && !career.customized(),"Load entry read the incoming career's rules");
    require(!s.platform->database_seasons().active(),"Save load kept routing to a season pack");
    // Deliberately leave the other release's names in the writable overlay.
    // The actual packed loader must extract its own languages before the
    // compatibility profile is inspected; pre-extraction hides ordering bugs.
    const auto& packs=s.platform->database_seasons().packs();
    const auto other=year==2000?s.platform->database_seasons().default_index():0;
    const auto runtime=s.platform->writable_directory();
    std::filesystem::copy_file(runtime/"Files/Databases"/packs[other].directory/"Data/eng.lng",
                               runtime/"Files/Data/eng.lng",std::filesystem::copy_options::overwrite_existing);
    require(s.platform->database_seasons().loaded().has_value(),"Loader entry forgot the running career's database");
    s.platform->career_database_replaced(); // The loader's hook at 0x10346dd4.
    require(!s.platform->career_present(),"A career being replaced is still offered for editing");
    require(invoke(s,0x1009ef30,{archive,1})==1,"Packed database reload");
    {
        // The loader's hook at 0x10346ee8, after its extraction of the
        // temporary members: a failed extraction (r3 = 0) reads nothing.
        const auto registers=s.registers;
        c.set(3,0);s.platform->career_members_extracted(s);
        require(!career.edition() && !career.customized(),"Failed extraction read a rules member");
        c.set(3,1);s.platform->career_members_extracted(s);
        s.registers=registers;
        require(career.edition()==edition && career.historical_rules() && career.customized() &&
                !(*career.customized())[0][0].brazil.serie_b_single_table &&
                competition_edition(s)==edition,"Loaded career's rules member was not applied");
        // With the switch on, a season's rules are the career's own: no change is checked in.
        const auto season=competition_rules(s,RuleCompetition::brazilian_serie_b,2030);
        require(!season.brazil.serie_b_single_table &&
                season.england.promotion.conference_promoted==competition_rules(s).england.promotion.conference_promoted &&
                (season.england.conference_cup!=nullptr)==(year==2000),"Season rules differ from the career's with an empty timeline");
    }
    require(s.platform->database_seasons().legacy_2000()==(year==2000),"Embedded names did not restore compatibility identity");
    check_edition_rules(s,year);
    require(!s.platform->database_seasons().loaded(),"Save load retained pack provenance");
    {
        const auto registers=s.registers;
        c.set(3,1);s.platform->quick_save_load_end(s);
        s.registers=registers;
    }
    require(s.platform->career_present(),"Loaded career is not offered for editing");
    require(!s.platform->database_seasons().active(),"Completed save load resumed pack routing");
    {
        // The career writer's entry (mode 0x16) leaves the member for the
        // original code to bundle, unknown statement included.
        std::filesystem::remove(rules_file);
        const auto registers=s.registers;
        c.set(4,0x16);
        require(s.platform->quick_save_write_begin(s),"Career writer entry refused");
        s.registers=registers;
        std::ostringstream written;written<<std::ifstream(rules_file,std::ios::binary).rdbuf();
        require(written.str()==rules_member,"Career writer did not leave the rules member");
        // A new career starts without the previous one's rules.
        require(career.edition()==edition,"Saving dropped the career's rules");
        s.platform->career_load_reset();
        require(!career.edition() && !career.historical_rules() && !career.customized() &&
                !std::filesystem::exists(rules_file),"New career kept another career's rules or member");
        // So does a database indexed outside a load (the New Game path), before any world is built.
        std::ofstream(rules_file,std::ios::binary)<<rules_member;
        c.set(3,1);s.platform->career_members_extracted(s);s.registers=registers;
        require(career.edition()==edition,"Rules member fixture");
        s.platform->inspect_database_season_names();
        require(!career.edition() && !career.historical_rules() && !std::filesystem::exists(rules_file) &&
                s.platform->database_seasons().legacy_2000()==(year==2000),"New database kept the loaded career's rules");
        // A switch chosen on the new game's setup screens survives choosing another season there.
        s.platform->career_load_reset();
        s.platform->edit_career_historical_rules(true);
        s.platform->inspect_database_season_names();
        require(career.historical_rules() && !career.edition(),"New game lost the switch chosen during its setup");
        s.platform->career_load_reset();
        require(!career.historical_rules(),"Next new game inherited the switch");
    }
    if(year==2000)require(c.u32(c.toc(-0x6b88))==65,"Localized packed archive lost Brazilian competition identity");
    check_2002_history(s,year==2002);
    require(c.u32(c.toc(-0x7b50))==club_count && c.u32(c.toc(-0x7be4))==staff_count,"Packed database counts changed");
    require(c.u32(c.toc(-0x66d0))==taiwan,"Archived Taiwan symbolic index");
    const auto read=invoke(s,0x107475b0,{archive,member,0,0x15,4});require(read,"Date fixture section read");
    require(invoke(s,0x10747a80,{scratch+1024,8,1,read})==1 && invoke(s,0x10747a80,{scratch+1032,8,1,read})==1,"Date fixture read");
    c.put32(scratch+900,read);invoke(s,0x10747900,{scratch+900});
    require(s.memory.copy_out(scratch+1024,8)==current_bytes && s.memory.copy_out(scratch+1032,8)==start_bytes,"Serialized date bytes changed");
    deallocate(s,scratch);
    c.put8(c.toc(-0x7ab0),language);
    std::cout<<"Summer/calendar-year start selection, original packed database/language reload and date bytes passed\n";
}
}
// A custom database made from the 2001/02 data is taken by a New Game on the
// real 2001 names (the Mac 3.9.62 release) and refused on the other packs.
void check_custom_database(State& s, unsigned year) {
    auto& platform=*s.platform; auto& career=platform.career_rules();
    const auto file=platform.writable_directory()/"custom-database-check.toml";
    std::ofstream(file)<<"format = 1\nbase = \"cm0102\"\nname = \"check\"\n\n[league.0]\npoints.win = 2\n";
    require(platform.select_custom_database(file).empty(),"Custom database fixture");
    platform.career_load_reset();
    const bool used=!career.competition_overrides().empty();
    require(used==(year==2001),year==2001?"The 2001 names refused a custom database made from them":"Another database took a 2001 custom database");
    platform.select_custom_database(std::nullopt);
    platform.career_load_reset();
    require(career.competition_overrides().empty(),"A cleared custom database reached the next career");
    std::filesystem::remove(file);
}
int main(int argc,char**argv) {
    try {
        check_career_rules_text();
        check_rule_timelines();
        const bool english_only=argc==5 && std::string_view(argv[4])=="--english-2000";
        const bool survey=argc==5 && std::string_view(argv[4])=="--survey-2000";
        if(argc!=4 && (argc!=5 || (!english_only && !survey && std::string_view(argv[4])!="--history-2002")))
            throw std::runtime_error("Usage: cm-database-seasons-checks PROGRAM GAME PRIVATE_RUNTIME [--history-2002|--english-2000]");
        const bool history_only=argc==5 && !english_only && !survey;
        auto state=std::make_unique<State>(); auto& s=*state;
        load_program(s,argv[1]);
        dispatch_probe=reject_dialog;
        DisplayFixture display_fixture; s.code_fragment_observer = &display_fixture;
        s.platform=std::make_shared<NativePlatform>(argv[2],argv[3],AudioMode::offline,EventMode::isolated);
        initialize_new_screens(std::filesystem::path(argv[3])/"FixtureScreens.plist");select_new_screens(true);
        auto& seasons=s.platform->database_seasons();
        if(!history_only)
            require(seasons.packs().size()==3 && seasons.default_index()==1,"Fixture needs chronological 2000, 2001 and 2002 packs with 2001 default");
        const auto original=seasons.default_index(), updated=original+1;
        require(updated<seasons.packs().size() && seasons.packs()[original].year==2001 &&
                seasons.packs()[updated].year==2002,"History fixture needs adjacent 2001 and 2002 packs");
        require(!seasons.data_path("SI_DATA","index.dat"),"Season path active before a new game");
        for(unsigned i=0;i<seasons.packs().size();++i)seasons.preflight(i);
        s.budget=2000000000ull;s.pc=0x10773710;run_until(s,0x10773754);
        display_fixture.toc=s.r<4>(8);
        // MAIN's GetProcessInformation supplies this application FSSpec. The
        // command-line fixture has no AppKit process, so obtain the same virtual
        // spec through the native filesystem bridge before any path is cached.
        {
            guest::Cpu boot(s);const auto name=allocate(s,64,true);
            constexpr std::string_view application="CM 01-02:CM 01-02";
            boot.put8(name,application.size());
            for(unsigned i=0;i<application.size();++i)boot.put8(name+1+i,application[i]);
            boot.set({{3,0},{4,0},{5,name},{6,boot.toc(-0x5bac)}});
            require(s.platform->dispatch(s,"CarbonLib::FSMakeFSSpec") &&
                    (boot.r(3)==0 || boot.r(3)==unsigned(-43)),"Application directory fixture");
            invoke(s,0x1076faf0);
            constexpr std::string_view variable="CM3_DATA";
            for(unsigned i=0;i<=variable.size();++i)boot.put8(name+i,i<variable.size()?variable[i]:0);
            boot.put8(name+32,0);
            const auto path=invoke(s,0x10746280,{name,name+32});
            std::string resolved;
            for(unsigned i=0;i<255 && boot.u8(path+i);++i)resolved+=char(boot.u8(path+i));
            require(resolved=="CM 01-02:Data:","Inactive Data path bootstrap differs from application directory");
            deallocate(s,name);
        }
        require(invoke(s,0x10118ef0,{0})==1,"Application bootstrap");
        const bool explicit_temp = std::filesystem::is_directory(std::filesystem::path(argv[3])/"Files/Temp");
        if (explicit_temp) {
            // Configure the original folder cache through its own insertion
            // API. This tests a resolved destination independently of classic
            // FindFolder, whose headless fallback remains the current folder.
            guest::Cpu boot(s); const auto scratch=allocate(s,64,true);
            const auto duplicate=[&](std::string_view value) {
                for(unsigned i=0;i<=value.size();++i)boot.put8(scratch+i,i<value.size()?value[i]:0);
                return invoke(s,0x107395f0,{scratch});
            };
            const auto key=duplicate("temp"), destination=duplicate("CM 01-02:Temp:");
            invoke(s,0x1074df30,{boot.toc(-0x3e70),key,destination});
            deallocate(s,scratch);
        }
        invoke(s,0x107474a0,{12345});
        guest::Cpu c(s);
        const auto ui=c.toc(-0x7bb0),display=c.u32(ui+0x24);
        c.put16(ui+0x25819e,1);
        require(!database_season_event(s,0x10334940,-1),"Original New Game command intercepted");
        require(seasons.active() && !seasons.pending(),"Cold opener queued a duplicate database load");
        invoke(s,0x10335330);
        const auto temporary_names = explicit_temp ? "CM 01-02:Temp:" : "";
        if (!seasons.names_directory() || *seasons.names_directory()!=temporary_names)
            throw std::runtime_error("Database temporary-directory resolution: ["+
                                     seasons.names_directory().value_or("<missing>")+"] expected ["+temporary_names+"]");
        const std::vector<std::size_t> sequence=english_only||survey?std::vector<std::size_t>{1,0}:history_only?std::vector<std::size_t>{original,updated,original,updated}:
            std::vector<std::size_t>{1,0,1,2,1,0,1,2};
        for(auto index:sequence) {
            if(index!=seasons.selected())require(seasons.request(index>seasons.selected()?1:-1),"Queue season switch");
            invoke(s,0x10719560,{display,1});
            const auto cutoff=s.memory.copy_out(c.toc(-0x79e8),5),rng=random_state(s);
            const bool reuse=!seasons.pending(); const auto club_table=c.u32(c.toc(-0x7be8));
            s.budget=s.executed+20000000000ull;
            database_season_before_build(s,0x10335500);
            if(reuse)require(random_state(s)==rng && c.u32(c.toc(-0x7be8))==club_table,"Default opener reloaded data or changed RNG");
            require(seasons.error.empty(),seasons.error.c_str());
            require(seasons.selected()==index && !seasons.pending(),"Selection committed");
            const auto year=seasons.packs()[index].year;
            require(seasons.legacy_2000()==(year==2000),"Database compatibility profile leaked across seasons");
            check_edition_rules(s,year);
            check_custom_database(s,year);
            require(c.u16(c.toc(-0x7b80)+2)==year && c.u8(c.toc(-0x7b80)+4)==(year==2000),"Actual current year/leap flag did not change");
            require(s.memory.copy_out(c.toc(-0x79e8),5)==cutoff,"Historical contract cutoff changed");
            const auto clubs=c.u32(c.toc(-0x7be8)),people=c.u32(c.toc(-0x7be0));
            const auto club_count=c.u32(c.toc(-0x7b50)),staff_count=c.u32(c.toc(-0x7be4));
            require(clubs && people && club_count>7000 && staff_count>100000,"Loaded database counts");
            // Windows v1 staff adds the original 0x5208 reserve and sixteen
            // managers. Club storage includes national teams as well.
            if((year==2002 && (club_count!=13214+426 || staff_count!=169342+0x5208+16)) ||
               (year==2000 && (club_count!=6993+426 || staff_count!=89798+0x5208+16)))
                throw std::runtime_error("Official counts: clubs="+std::to_string(club_count)+" staff="+std::to_string(staff_count));
            check_2002_history(s,year==2002);
            check_extra_division_availability(s);
            for(unsigned i=0;i<34;++i) {
                const auto row=c.toc(-0x7b44)+i*0x4c;
                if(!c.u32(row))continue;
                const auto nation=c.u32(row),base=c.u32(c.toc(-0x7b98));
                require(nation>=base && (nation-base)%0x128==0 && (nation-base)/0x128<c.u32(c.toc(-0x7b04)),"Stale nation descriptor");
                require(c.u16(row+0x2e)>=year && c.u16(row+0x2e)<=year+1,"League start year");
            }
            std::cout<<seasons.packs()[index].label<<": clubs="<<club_count<<" staff="<<staff_count<<" year="<<c.u16(c.toc(-0x7b80)+2)<<std::endl;
            if(year==2000 && survey) { survey_constructors(s); return 0; }
            if(year==2000 && english_only) { check_english_construction(s); return 0; }
            if(year==2000) { check_legacy_boundaries(s); check_layout(s); check_archive(s); seasons.begin(true); }
        }
        check_missing_pack(s,argv[3]);
        check_layout(s);
        check_archive(s);
        std::cout<<"2002 World Cup/European finalists and Ronaldo's eight goals survive conversion, switching and packed reload\n";
        seasons.end();require(!seasons.data_path("SI_DATA","eng.lng"),"Save language extraction still uses a season pack");
        seasons.begin(true);require(seasons.selected()==seasons.default_index() && seasons.pending()==seasons.default_index(),"New wizard did not default to 2001");
        std::cout<<"Database source switching, actual year, historical cutoff and pointer isolation passed\n";
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

#include "match_debugger_database.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <algorithm>

namespace db=cm::aot::match_debugger;
namespace {
unsigned checks=0,negatives=0;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);++checks;}
void reject(const std::function<void()>& action){try{action();}catch(const std::exception&){++negatives;return;}throw std::runtime_error("Malformed club catalog was accepted");}
void word(std::vector<std::uint8_t>& bytes,std::size_t offset,std::uint32_t value,bool little=false) {
    for(unsigned n=0;n<4;++n)bytes.at(offset+n)=std::uint8_t(value>>(8*(little?n:3-n)));
}
struct Fixture {
    std::vector<std::uint8_t> index=std::vector<std::uint8_t>(8+2*68),clubs=std::vector<std::uint8_t>(2*584+4),national=std::vector<std::uint8_t>(584+4),names=std::vector<std::uint8_t>(32+3*79);
    Fixture() {
        const auto copy=[](auto& data,std::size_t offset,const std::string& text){std::copy(text.begin(),text.end(),data.begin()+offset);};
        copy(index,8,"clubs club.dam");word(index,60,0);word(index,64,2);word(index,72,2);
        copy(index,76,"clubs nat_club.dam");word(index,128,1);word(index,132,1);word(index,140,2);
        word(clubs,0,0);word(clubs,4,0x12345678);word(clubs,584,1);word(clubs,1168,0xf1fffff1);
        word(national,0,2);word(national,584,0xf1fffff1);word(names,0,3,true);
        copy(names,32,"First Club");copy(names,32+52,"First");
        copy(names,32+79,"1.FC K\xf6ln");copy(names,32+79+52,"K\xf6ln");
        copy(names,32+158,"National Team");copy(names,32+158+52,"National");
    }
    db::DatabaseCatalog parse()const{return db::DatabaseCatalog::parse(index,clubs,national,names);}
};
void component() {
    Fixture f;const auto catalog=f.parse();require(catalog.clubs().size()==3,"Actual IDs from both table classes");
    require(catalog.club(0).name=="First Club"&&catalog.club(0).short_name=="First","Original name fields");
    require(catalog.club(1).name=="1.FC K\xc3\xb6ln","Windows Latin-1 club name decoded as UTF-8");
    require(catalog.club(2).kind==db::ClubKind::national_team,"National table classification");
    require(catalog.original_record(0).size()==584&&catalog.original_record(0)[4]==0x12&&catalog.original_record(1)[3]==1,
        "Raw records begin at zero, preserve the first ID and retain all field bytes");
    reject([&]{(void)catalog.club(3);});reject([&]{(void)catalog.original_record(UINT32_MAX);});
    for(unsigned kind=0;kind<13;++kind) {
        auto bad=f;
        if(kind==0)bad.index.pop_back();
        if(kind==1)word(bad.index,0,1);
        if(kind==2)word(bad.index,128,0);
        if(kind==3)bad.index[8]='/';
        if(kind==4)word(bad.index,72,1);
        if(kind==5)word(bad.index,68,1);
        if(kind==6)bad.clubs.pop_back();
        if(kind==7)word(bad.clubs,1168,0);
        if(kind==8)word(bad.clubs,0,0xf1fffff1);
        if(kind==9)word(bad.clubs,584,0);
        if(kind==10)word(bad.national,0,0);
        if(kind==11)word(bad.names,0,UINT32_MAX,true);
        if(kind==12)bad.names.resize(32+158);
        reject([&]{(void)bad.parse();});
    }
}
}
int main(int argc,char** argv) {
    try {
        component();
        if(argc==2) {
            const auto catalog=db::DatabaseCatalog::load(argv[1]);
            require(catalog.clubs().size()==11019,"Full real catalog has 10593 clubs and 426 national teams");
            require(catalog.club(0).name=="1.FC Bocholt","First real club is retained");
            require(catalog.club(1).name=="1.FC K\xc3\xb6ln","Real non-ASCII club name decoded correctly");
            require(catalog.club(5791).name=="Mat\xc3\xa1v-Sopron"&&catalog.club(5791).short_name=="Sopron","Actual aligned English long/short club fields retained");
            unsigned club_count=0,national_count=0;
            for(const auto& club:catalog.clubs()){if(club.kind==db::ClubKind::club)++club_count;else ++national_count;}
            require(club_count==10593&&national_count==426,"Every real source record is represented exactly once");
            std::cout<<"actual clubs="<<club_count<<" national="<<national_count<<" aligned-ID-5791="<<catalog.club(5791).name<<'\n';
        } else require(argc==1,"Optional explicit original game directory");
        std::cout<<"PASS: "<<checks<<" club catalog assertions and "<<negatives<<" rejected malformed/unknown inputs; read-only original database, no career or snapshot\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

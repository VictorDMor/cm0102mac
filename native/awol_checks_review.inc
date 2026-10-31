// Cases of the AWOL checker for awol_review.cpp (the daily review at 0x10004070).
namespace review_world {
using Bytes=std::vector<std::uint8_t>;
constexpr unsigned relation_at=data+0x2f900,peer_id=4;
// A list item: big-endian id at 0, a date (year, day) at `date_at`, a mode byte.
Bytes item(unsigned size,unsigned id,unsigned year,unsigned day,unsigned date_at,unsigned mode,unsigned mode_at) {
    Bytes b(size,0x77);
    const auto put=[&](unsigned at,unsigned v,unsigned n){for(unsigned i=0;i<n;++i)b[at+i]=std::uint8_t(v>>(8*(n-1-i)));};
    put(0,id,4);put(date_at,year,2);put(date_at+2,day,2);b[mode_at]=std::uint8_t(mode);
    return b;
}
Bytes primary(unsigned id,unsigned year=2001,unsigned day=99){return item(0xc,id,year,day,4,0,0xb);}
Bytes secondary(unsigned id,unsigned mode,unsigned year=2001,unsigned day=99){return item(0x10,id,year,day,4,mode,0xc);}
Bytes pending(unsigned id,unsigned mode,unsigned year=2001,unsigned day=99){return item(0x14,id,year,day,8,mode,0x10);}
std::vector<unsigned> bits(unsigned mask,unsigned count) {
    std::vector<unsigned> words(count+1,0);
    for(unsigned n=0;n<count;++n)words[n+1]=(mask>>n)&1;
    return words;
}
std::vector<unsigned> noise(unsigned seed,unsigned modulus) {
    std::vector<unsigned> words(64);
    for(unsigned n=0;n<64;++n)words[n]=((seed+1)*(n*37+11)+n*n*5+seed*seed)%modulus;
    return words;
}
struct Scene {
    std::vector<Bytes> list[3];
    bool no_list[3]={false,false,false};
    unsigned year=2001,day=100,shift=0,year_shift=0,salt=0,start=0,contract=world::contract,flag=1;
    bool first_contract_only=false;   // later contract lookups (the training event's) find none
    std::vector<unsigned> words,relations{1},kinds{0};
    std::function<void(Cpu&)> tweak;
    void apply(Cpu& c) const {
        using namespace world;
        original_mode=true;tracing=false;
        const unsigned objects[3]={tables,tables+0x40,tables+0x80},sizes[3]={0xc,0x10,0x14};
        for(unsigned n=0;n<3;++n) {
            c.set(3,objects[n]);c.set_lr(sentinel);c.s.pc=0x10738c00;run_until(c.s,sentinel);
            for(const auto& bytes:list[n]) {
                for(unsigned i=0;i<sizes[n];++i)c.put8(tables+0x800+i,bytes[i]);
                c.set({{3,objects[n]},{4,tables+0x800},{5,sizes[n]}});c.set_lr(sentinel);c.s.pc=0x10738d10;run_until(c.s,sentinel);
            }
        }
        original_mode=false;seed(c);
        for(unsigned n=0;n<3;++n)if(no_list[n])c.put32(records+n*4,0);
        // The date, the staff and the club's roster: every gate passes by default.
        const auto now_at=c.toc(-0x7b80);c.put16(now_at,year);c.put16(now_at+2,day);c.put32(now_at+4,0x55667788);
        for(unsigned id=0;id<staff_count;++id) {
            const auto at=staff(id),p=profile+id*0x20;
            c.put8(at+0x67,5);c.put8(at+0x69,5);c.put8(at+0x63,12);c.put16(at+0x5c,18);
            c.put16(p+0xa,0x1500);c.put8(p+0x44,5);c.put8(flags+id,flag);
        }
        const auto base=c.toc(-0x7b8c);
        for(unsigned n=0;n<64;++n)c.put16(base+n*2,n<words.size()?words[n]:0);
        c.put32(c.toc(-0x7b84),base+start*2);c.put32(c.toc(-0x7b90),salt);
        script.reseed=0x1234;script.contract=contract;
        if(first_contract_only){script.contracts={contract};script.contract=0;}
        const auto moved=shift,moved_year=year_shift;const auto relation_list=relations,kind_list=kinds;
        script.services[0x10717a10]=[=](Cpu& c){
            const auto out=c.r(3),from=c.r(4);
            c.put16(out,c.u16(from)+moved_year);c.put16(out+2,c.u16(from+2)+moved);c.put32(out+4,c.r(5));
            return out;
        };
        // 0x1037fa70 is a leaf that reads the relation's word at +0x48: it runs
        // for real, so the record holds the next value.
        script.services[0x10082f90]=[=,count=0u,kinds=0u](Cpu& c) mutable {
            const auto kind=relation_list[count++%relation_list.size()];
            c.put32(relation_at+0x80,c.r(3));c.put32(relation_at+0x84,c.r(4));c.put32(relation_at+0x88,c.r(5));
            if(kind==0x100)return 0u;
            c.put32(relation_at+0x48,kind_list[kinds++%kind_list.size()]);
            c.put8(relation_at+0x60,kind);return relation_at;
        };
        script.services[0x1037f8a0]=[](Cpu& c){c.put32(relation_at+0x90,c.r(3));c.put32(relation_at+0x94,c.r(4));return 0u;};
        script.services[0x10649fb0]=[](Cpu& c){for(unsigned r=3;r<=8;++r)c.put32(relation_at+0xa0+r*4,c.r(r));return 0u;};
        if(tweak)tweak(c);
    }
};
}
void add_review_cases(std::vector<Case>& cases) {
    using namespace world;
    using namespace review_world;
    const unsigned entry=0x10004070;
    const auto add=[&](const std::string& name,const Scene& scene){
        cases.push_back({entry,name,[=](Cpu& c){scene.apply(c);c.set(3,records);}});
    };
    const auto n=[](unsigned v){return std::to_string(v);};
    const auto staff3=staff(3);
    // Lists: absent, empty, and the list objects' states.
    for(unsigned mask=0;mask<8;++mask) {
        Scene s;for(unsigned i=0;i<3;++i)s.no_list[i]=mask>>i&1;
        add("no-lists-"+n(mask),s);
    }
    add("empty-lists",Scene{});
    // Dates: expired when later; equal dates are not expired. Each list.
    for(unsigned today_year:{2001u,0xffffu,0x8000u,0x7fffu,0u})
        for(unsigned item_year:{2000u,2001u,2002u,0xffffu,0x8000u,0x7fffu,0u,1u})
            for(unsigned item_day:{99u,100u,101u,0u,0xffffu}) {
                Scene s;s.year=today_year;s.day=100;
                const std::string tag=n(today_year)+"-"+n(item_year)+"-"+n(item_day);
                {auto p=s;p.list[0]={primary(3,item_year,item_day),primary(4,2001,50)};add("primary-date-"+tag,p);}
                {auto q=s;q.list[1]={secondary(3,1,item_year,item_day),secondary(4,1,2001,50)};add("secondary-date-"+tag,q);}
                {auto e=s;e.list[2]={pending(3,1,item_year,item_day),pending(4,1,2001,50)};add("pending-date-"+tag,e);}
            }
    // The adjusting routine's output decides secondary and pending expiry.
    for(int shift:{-1,0,1,-100,100})for(unsigned year_shift:{0u,1u,0xffffu}) {
        Scene s;s.shift=unsigned(shift);s.year_shift=year_shift;
        auto q=s;q.list[1]={secondary(3,1,2001,100),secondary(4,0,2001,100)};add("secondary-shift-"+std::to_string(shift)+"-"+n(year_shift),q);
        auto e=s;e.list[2]={pending(3,1,2001,100),pending(4,0,2001,100)};add("pending-shift-"+std::to_string(shift)+"-"+n(year_shift),e);
    }
    // The real date routines (no script), where they run in the fixture.
    for(unsigned day:{1u,99u,100u,101u,200u,365u})for(unsigned mode:{0u,1u}) {
        Scene s;s.list[1]={secondary(3,mode,2001,day)};s.list[2]={pending(3,mode,2001,day)};s.words=bits(5,6);
        s.tweak=[=](Cpu&){script.services.erase(0x10717a10);};
        add("real-adjust-"+n(day)+"-"+n(mode),s);
    }
    // Primary: which item is removed, flags, and the end of the list.
    for(unsigned flag:{0u,1u,2u,0x80u})for(unsigned layout=0;layout<6;++layout) {
        Scene s;s.flag=flag;
        switch(layout) {
        case 0:s.list[0]={primary(3)};break;
        case 1:s.list[0]={primary(3,2001,200),primary(4)};break;
        case 2:s.list[0]={primary(3),primary(4)};break;
        case 3:s.list[0]={primary(3,2001,200),primary(4,2001,200),primary(5)};break;
        case 4:s.list[0]={primary(3,2001,200),primary(4,2001,200)};break;
        default:s.list[0]={primary(3),primary(4)};s.contract=0;break;
        }
        add("primary-flag-"+n(flag)+"-layout-"+n(layout),s);
    }
    // A removal does not stop the other two lists.
    {Scene s;s.list[0]={primary(3)};s.list[1]={secondary(5,0)};s.list[2]={pending(6,1)};add("all-three-expired",s);}
    // Null iterator results end the whole review, in each list.
    for(unsigned member=0;member<3;++member)for(unsigned count:{1u,2u,5u}) {
        Scene s;
        s.list[0]={primary(3,2001,200)};s.list[1]={secondary(3,1,2001,200)};s.list[2]={pending(3,1,2001,200)};
        s.tweak=[=](Cpu& c){c.put16(tables+member*0x40,count);if(count==1)c.put32(tables+member*0x40+4,0);};
        add("null-iterator-"+n(member)+"-"+n(count),s);
        auto t=s;t.list[member]={};t.tweak=[=](Cpu& c){c.put16(tables+member*0x40,count);c.put32(tables+member*0x40+4,0);};
        add("null-iterator-empty-"+n(member)+"-"+n(count),t);
    }
    // Live list changes: a service resizes the list under the loop.
    for(unsigned member=1;member<3;++member)for(unsigned count:{0u,1u,2u,3u,0x8000u,0xffffu}) {
        Scene s;
        s.list[1]={secondary(3,1,2001,200),secondary(4,1,2001,200),secondary(5,1,2001,200)};
        s.list[2]={pending(3,1,2001,200),pending(4,1,2001,200),pending(5,1,2001,200)};
        s.tweak=[=](Cpu&){
            const auto next=script.services[0x10717a10];
            script.services[0x10717a10]=[=](Cpu& c){c.put16(tables+member*0x40,count);return next(c);};
        };
        add("live-count-"+n(member)+"-"+n(count),s);
    }
    // A service clears the flag byte of the staff member under review.
    for(unsigned flag:{0u,1u,2u,3u}) {
        Scene s;s.contract=0;s.list[1]={secondary(3,1)};s.list[2]={pending(3,1)};
        s.tweak=[=](Cpu&){
            const auto next=script.services[0x10717a10];
            script.services[0x10717a10]=[=](Cpu& c){c.put8(flags+3,flag);return next(c);};
        };
        add("flag-changed-"+n(flag),s);
    }

    // Secondary, return from leave: both traits must draw zero... or the record
    // is only removed; the decay of both traits when the draws allow it.
    const unsigned traits[]={0,1,2,5,0x7f,0x80,0xff};
    for(unsigned a:traits)for(unsigned b:traits)for(unsigned mask=0;mask<16;++mask) {
        Scene s;s.list[1]={secondary(3,1)};s.words=bits(mask,4);s.contract=(mask&1)?0:world::contract;
        s.tweak=[=](Cpu& c){c.put8(staff3+0x67,a);c.put8(staff3+0x69,b);};
        add("leave-"+n(a)+"-"+n(b)+"-"+n(mask),s);
    }
    {Scene s;s.list[1]={secondary(3,1)};s.tweak=[=](Cpu& c){c.put32(staff3+0x14,0);};add("leave-no-profile",s);}
    for(unsigned mode:{1u,2u,0x80u,0xffu}){Scene s;s.list[1]={secondary(3,mode)};s.words=bits(0,4);add("leave-mode-"+n(mode),s);}
    for(unsigned flag:{0u,1u,2u,3u,0x80u}) {
        Scene s;s.flag=flag;s.list[1]={secondary(3,1)};s.words=bits(0,4);if(flag!=1&&flag!=0x80)s.contract=0;add("leave-flag-"+n(flag),s);
        s.contract=0;add("leave-flag-noclub-"+n(flag),s);
    }
    for(unsigned start=1;start<=6;++start)for(unsigned seedv=0;seedv<3;++seedv) {
        Scene s;s.list[1]={secondary(3,1)};s.words=noise(seedv,3);s.start=64-(start-1);
        s.tweak=[=](Cpu& c){c.put8(staff3+0x67,5);c.put8(staff3+0x69,5);};
        add("leave-wrap-"+n(start)+"-"+n(seedv),s);
    }
    for(unsigned seedv=0;seedv<40;++seedv){Scene s;s.list[1]={secondary(3,1),secondary(4,1),secondary(5,0)};s.words=noise(seedv,3);add("leave-noise-"+n(seedv),s);}

    // Secondary, unapproved return. Peers: gates one by one, draws by mask.
    struct Gate{std::string name;std::function<void(Cpu&,unsigned,unsigned)> set;unsigned relation=1,kind=0;};
    const auto peer=staff(peer_id),peer_profile=profile+peer_id*0x20;
    const auto single=[&](Scene& s){
        s.list[1]={secondary(3,0)};
        s.tweak=[=,old=s.tweak](Cpu& c){
            for(unsigned slot=0;slot<50;++slot)c.put32(club(1)+0x98+slot*4,slot==0?peer:0);
            c.put32(staff3+0x14,0);if(old)old(c);
        };
    };
    std::vector<Gate> gates;
    gates.push_back({"base",[](Cpu&,unsigned,unsigned){}});
    for(unsigned v:{0xfu,0x10u,0x80u,0x7fu,0u})gates.push_back({"senior-trait-"+n(v),[=](Cpu& c,unsigned s,unsigned){c.put8(s+0x67,v);}});
    for(unsigned v:{0xau,0xbu,0x80u,0xffu})gates.push_back({"level-"+n(v),[=](Cpu& c,unsigned s,unsigned){c.put8(s+0x63,v);}});
    for(unsigned v:{0u,1u,2u,3u,0x11u,0x12u,0x60u,0xf2u})gates.push_back({"kind-"+n(v),[=](Cpu& c,unsigned,unsigned){c.put8(relation_at+0x60,v);},v,0});
    for(unsigned v:{0x17u,0x18u,0x1018u,0x1100u,0x80u,0xffffu,0x14u,0x15u,0x16u})gates.push_back({"age-"+n(v),[=](Cpu& c,unsigned s,unsigned){c.put16(s+0x5c,v);}});
    for(unsigned v:{0x1482u,0x1483u,0x8000u,0u,0xffffu,0x1481u})gates.push_back({"ability-"+n(v),[=](Cpu& c,unsigned,unsigned p){c.put16(p+0xa,v);}});
    gates.push_back({"no-peer-profile",[](Cpu& c,unsigned s,unsigned){c.put32(s+0x14,0);}});
    gates.push_back({"relation-none",[](Cpu&,unsigned,unsigned){},0x100,0});
    for(unsigned k:{1u,2u,3u,0xffu})gates.push_back({"peer-kind-"+n(k),[](Cpu&,unsigned,unsigned){},1,k});
    for(unsigned v:{0u,1u,2u,0x80u,0xffu})gates.push_back({"young-trait-a-"+n(v),[=](Cpu& c,unsigned s,unsigned){c.put8(s+0x67,v);c.put16(s+0x5c,18);}});
    for(unsigned v:{0u,1u,2u,0x80u,0xffu})gates.push_back({"young-profile-"+n(v),[=](Cpu& c,unsigned,unsigned p){c.put8(p+0x44,v);}});
    for(unsigned v:{0u,1u,2u,0x80u,0xffu})gates.push_back({"young-trait-b-"+n(v),[=](Cpu& c,unsigned s,unsigned){c.put8(s+0x69,v);}});
    for(const auto& gate:gates) {
        const bool base=gate.name=="base";
        for(unsigned mask=0;mask<32;mask+=base?1:5)
            for(unsigned young:{0u,1u}) {
                Scene s;single(s);s.words=bits(mask,6);s.relations={gate.relation};s.kinds={gate.kind};
                const auto prior=s.tweak;
                s.tweak=[=](Cpu& c){
                    prior(c);
                    c.put8(relation_at+0x60,1);
                    c.put8(peer+0x67,young?5:0x14);c.put8(peer+0x63,12);c.put16(peer+0x5c,young?18:0x20);
                    gate.set(c,peer,peer_profile);
                };
                add("peer-"+gate.name+"-"+n(mask)+"-"+n(young),s);
            }
    }
    // The roster: slots empty, the absentee itself, no profile, many peers.
    for(unsigned layout=0;layout<9;++layout)for(unsigned seedv=0;seedv<4;++seedv) {
        Scene s;s.list[1]={secondary(3,0)};s.words=noise(seedv,2);s.relations={1,0x100,1,1};s.kinds={0,2,0,0};
        s.tweak=[=](Cpu& c){
            for(unsigned slot=0;slot<50;++slot)c.put32(club(1)+0x98+slot*4,0);
            const auto put=[&](unsigned slot,unsigned v){c.put32(club(1)+0x98+slot*4,v);};
            switch(layout) {
            case 0:break;
            case 1:put(0,staff3);break;
            case 2:put(0,staff(4));put(1,staff(5));put(2,staff(6));put(3,staff(7));break;
            case 3:put(0,staff(4));c.put32(staff(4)+0x14,0);put(1,staff(5));break;
            case 4:put(49,staff(5));put(48,staff(6));break;
            case 5:for(unsigned slot=0;slot<50;++slot)put(slot,staff(4+slot%8));break;
            case 6:put(0,staff3);put(1,staff(4));put(2,staff3);break;
            case 7:put(0,staff(4));put(1,0);put(2,staff(5));break;
            default:for(unsigned slot=0;slot<50;++slot)put(slot,slot%2?staff(4):0);break;
            }
        };
        add("roster-"+n(layout)+"-"+n(seedv),s);
    }
    // The absentee's own stage: profile, the trait draw, the mod-15 gate, decays.
    for(unsigned a:{0u,1u,2u,5u,0x7fu,0x80u,0xffu})for(unsigned prof:{0u,1u,2u,5u,0x80u})for(unsigned mask=0;mask<16;++mask) {
        Scene s;s.list[1]={secondary(3,0)};s.words=bits(mask,4);
        s.tweak=[=](Cpu& c){
            for(unsigned slot=0;slot<50;++slot)c.put32(club(1)+0x98+slot*4,0);
            c.put8(staff3+0x67,a);c.put8(profile+3*0x20+0x44,prof);
        };
        add("absentee-"+n(a)+"-"+n(prof)+"-"+n(mask),s);
    }
    for(unsigned mask=0;mask<64;++mask)for(unsigned salt:{0u,1u,14u}) {
        Scene s;s.list[1]={secondary(3,0)};s.words=bits(mask,6);s.salt=salt;
        s.tweak=[=](Cpu& c){
            for(unsigned slot=0;slot<50;++slot)c.put32(club(1)+0x98+slot*4,0);
            c.put8(staff3+0x67,9);
        };
        add("absentee-gate-"+n(mask)+"-"+n(salt),s);
    }
    {Scene s;s.list[1]={secondary(3,0)};s.tweak=[=](Cpu& c){c.put32(staff3+0x14,0);};add("absentee-no-profile",s);}
    {Scene s;s.list[1]={secondary(3,0)};s.contract=0;add("absentee-no-contract",s);}
    {}
    for(unsigned flag:{0u,1u,2u,3u}) {
        Scene s;s.flag=flag;s.first_contract_only=true;s.list[1]={secondary(3,0)};s.words=bits(0,4);add("absentee-flag-"+n(flag),s);
    }
    for(unsigned start=1;start<=24;++start)for(unsigned seedv=0;seedv<3;++seedv) {
        Scene s;s.list[1]={secondary(3,0)};s.words=noise(seedv,2);s.start=64-(start-1);s.relations={1,1,0x100};s.kinds={0,2,0,0};
        s.tweak=[=](Cpu& c){c.put8(staff3+0x67,9);c.put8(profile+3*0x20+0x44,7);};
        add("unapproved-wrap-"+n(start)+"-"+n(seedv),s);
    }
    for(unsigned seedv=0;seedv<60;++seedv) {
        Scene s;s.list[1]={secondary(3,0),secondary(5,0)};s.words=noise(seedv,3);s.relations={1,0x100,1};s.kinds={0,0,2};
        add("unapproved-noise-"+n(seedv),s);
    }
    // Secondary mixes: mode per item, fresh items skipped, several expired.
    for(unsigned layout=0;layout<6;++layout) {
        Scene s;
        switch(layout) {
        case 0:s.list[1]={secondary(3,1,2001,200),secondary(4,0),secondary(5,1)};break;
        case 1:s.list[1]={secondary(3,1),secondary(4,0)};break;
        case 2:s.list[1]={secondary(3,0),secondary(4,1)};break;
        case 3:s.list[1]={secondary(3,1,2001,200),secondary(4,1,2001,200)};break;
        case 4:s.list[1]={secondary(3,0,2001,200),secondary(4,0,2001,200),secondary(5,0)};break;
        default:s.list[1]={secondary(15,1)};break;
        }
        s.words=bits(0,6);add("secondary-layout-"+n(layout),s);
    }

    // Pending: recovered leave (id profile gate, four draws), unapproved absence.
    for(unsigned a:traits)for(unsigned b:traits)for(unsigned mask=0;mask<16;++mask) {
        Scene s;s.list[2]={pending(3,1)};s.words=bits(mask,4);
        s.tweak=[=](Cpu& c){c.put8(staff3+0x67,a);c.put8(staff3+0x69,b);};
        add("pending-leave-"+n(a)+"-"+n(b)+"-"+n(mask),s);
    }
    {Scene s;s.list[2]={pending(3,1),pending(4,1)};s.tweak=[=](Cpu& c){c.put32(staff3+0x14,0);};add("pending-leave-no-profile",s);}
    for(unsigned layout=0;layout<6;++layout)for(unsigned mask:{0u,1u,2u,15u}) {
        Scene s;s.words=bits(mask,4);
        switch(layout) {
        case 0:s.list[2]={pending(3,1,2001,200),pending(4,1)};break;
        case 1:s.list[2]={pending(3,1),pending(4,1)};break;
        case 2:s.list[2]={pending(3,0),pending(4,1)};break;
        case 3:s.list[2]={pending(3,1,2001,200),pending(4,1,2001,200)};break;
        case 4:s.list[2]={pending(3,1),pending(4,1),pending(5,1)};s.words=noise(mask,2);break;
        default:s.list[2]={pending(3,0),pending(4,0),pending(5,0)};s.words=noise(mask,2);break;
        }
        add("pending-layout-"+n(layout)+"-"+n(mask),s);
    }
    for(unsigned a:traits)for(unsigned prof:{0u,1u,2u,5u,0x80u})for(unsigned mask=0;mask<16;++mask) {
        Scene s;s.list[2]={pending(3,0),pending(4,0)};s.words=bits(mask,4);
        s.tweak=[=](Cpu& c){c.put8(staff3+0x67,a);c.put8(profile+3*0x20+0x44,prof);};
        add("pending-absence-"+n(a)+"-"+n(prof)+"-"+n(mask),s);
    }
    for(unsigned mask=0;mask<16;++mask)for(unsigned salt:{0u,1u,3u,4u}) {
        Scene s;s.list[2]={pending(3,0)};s.words=bits(mask,4);s.salt=salt;add("pending-gate-"+n(mask)+"-"+n(salt),s);
    }
    {Scene s;s.list[2]={pending(3,0)};s.tweak=[=](Cpu& c){c.put32(staff3+0x14,0);};add("pending-absence-no-profile",s);}
    for(unsigned start=1;start<=10;++start)for(unsigned seedv=0;seedv<3;++seedv)for(unsigned mode:{0u,1u}) {
        Scene s;s.list[2]={pending(3,mode),pending(4,mode)};s.words=noise(seedv,2);s.start=64-(start-1);
        add("pending-wrap-"+n(start)+"-"+n(seedv)+"-"+n(mode),s);
    }
    for(unsigned seedv=0;seedv<40;++seedv)for(unsigned mode:{0u,1u}) {
        Scene s;s.list[2]={pending(3,mode),pending(4,1-mode),pending(5,mode)};s.words=noise(seedv,3);
        add("pending-noise-"+n(seedv)+"-"+n(mode),s);
    }
    // The training event's lookup takes the staff member's profile away.
    {
        Scene s;s.list[1]={secondary(3,0)};s.words=noise(1,3);
        s.tweak=[=](Cpu&){
            script.services[0x10082f20]=[=,count=0u](Cpu& c) mutable {
                if(count++==0)return world::contract;
                c.put32(staff3+0x14,0);return 0u;
            };
        };
        add("profile-gone-after-event",s);
    }
    // Everything at once, with the real date routine left to its own devices below.
    for(unsigned seedv=0;seedv<30;++seedv) {
        Scene s;s.list[0]={primary(2,2001,200),primary(3)};s.list[1]={secondary(3,0),secondary(4,1)};s.list[2]={pending(3,0),pending(5,1)};
        s.words=noise(seedv,3);s.relations={1,0x100,1};s.kinds={0,2};s.first_contract_only=true;
        add("all-"+n(seedv),s);
    }
}

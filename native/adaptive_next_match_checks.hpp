// Included inside adaptive_gui_checks.cpp's anonymous namespace. Existing
// calendar records drive the original opener, tabs and date commands. The
// capture lacks player-history archives: only while building, mark queried
// roster entries absent using the archive reader's original size==-1 branch.
// Restore those words before comparing records. Rich text and overflow below
// are declared GUIO-only fixtures, never invented matches or career records.
void next_match_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path/"Screens.plist"); select_new_screens(true);
    for(unsigned scenario:{0u,1u}) {
        Clock clock; clock.idle_input=true;
        auto state=std::make_unique<State>(); auto& s=*state;
        load_match_snapshot(s,path); s.code_fragment_observer=&clock;
        guest::Cpu c(s); c.set(2,c.u32(c.sp()+0x14));
        const auto manager=c.toc(-0x7bb0); c.put16(manager+0x2c58,0);
        const auto captured=c.u32(manager+0x10),managed=c.u32(captured+0x14),person=c.u32(manager+4);
        require(c.u32(c.u32(captured))==0x1001e2b0,"Next Match fixture is not a club page");
        const auto captured_next=c.u32(captured+0x14+3*8);
        require(captured_next,"Next Match capture lacks its owned pending fixture");
        const auto club=scenario?c.u32(captured_next+(c.u32(captured_next+0x1c)==managed?0x20:0x1c)):managed;
        GuiCanvas active_canvas{800,600}; const char* phase="open";
        const auto invoke=[&](unsigned entry,std::initializer_list<unsigned> args) {
            unsigned reg=3; for(auto arg:args)c.set(reg++,arg);
            s.budget=s.executed+400000000;
            try { c.call(entry,return_sentinel); }
            catch(const std::exception& error) {
                throw std::runtime_error("Next Match scenario="+std::to_string(scenario)+" phase="+phase+
                    " canvas="+std::to_string(active_canvas.width)+"x"+std::to_string(active_canvas.height)+
                    " entry="+hex_address(entry)+" pc="+hex_address(s.pc)+": "+error.what());
            }
            return c.r(3);
        };
        std::filesystem::create_directories(temp.path/"user"/"Files");
        s.platform=std::make_shared<NativePlatform>(temp.path/"game",temp.path/"user",AudioMode::offline);
        const auto scratch=allocate(s,32,true); world(s,scratch,scratch+16);
        require(s.platform->resize_gui_canvas(s,{801,601}) && s.platform->resize_gui_canvas(s,{800,600}),
                "Next Match pixel initialization");
        invoke(0x1001d410,{club,0,0,0,0});
        const auto history=c.u32(manager+0x10),display=c.u32(manager+0x24);
        const auto setting=[history](unsigned slot){return history+0x14+slot*8;};
        const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
        const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
        const auto rect=[&](unsigned at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
        const auto refresh=[&] {
            for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
            }
        };
        const auto hit=[&](unsigned index) {
            const auto box=rect(control(index)+0x10);
            require(box.left<=box.right && box.top<=box.bottom,"Next Match control has inverted hit bounds");
            c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
            c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
            return std::int16_t(invoke(c.u8(display)==1?0x1071aa10:0x1071a280,{display}));
        };
        // Let the genuine initial Squad build populate h2/h3 before collecting
        // either team's player-history dependencies. h3 owns a copied fixture.
        invoke(0x10719560,{display,1,0}); invoke(0x1001e2b0,{}); refresh();
        require(c.u32(setting(0))==club && c.u32(setting(4))==1 && c.u32(setting(3)),
                "Next Match opener did not establish its genuine next fixture");
        unsigned tab=0;
        for(unsigned i=0;i<c.u16(display+0x129da6);++i)
            if(c.u32(control(i)+8)==2 && c.u32(control(i)+0x48)==3)tab=i;
        require(tab && hit(tab)==int(tab),"Original Next Match tab missed its hit area");
        require(std::int32_t(invoke(0x10036f10,{tab}))==-11 && c.u32(setting(4))==3,
                "Original Next Match tab changed its return/history contract");
        const auto stats=c.u32(c.toc(-0x7b3c));
        const auto roster_records=[&] {
            std::vector<unsigned> result;
            const auto next=c.u32(setting(3));
            for(auto team:{club,next?c.u32(next+0x1c):club,next?c.u32(next+0x20):club}) {
                if(!team)continue;
                for(unsigned slot=0;slot<50;++slot) {
                    const auto staff=c.u32(team+0x98+slot*4);
                    if(staff && c.u32(staff+0x14) && std::find(result.begin(),result.end(),staff)==result.end())
                        result.push_back(staff);
                }
            }
            return result;
        };
        const auto original_build=[&](bool empty) {
            std::vector<std::pair<unsigned,unsigned>> archive_sizes;
            for(auto staff:roster_records()) {
                const auto entry=c.u32(stats+0x30)+c.u32(c.u32(staff+0x14))*0x28;
                archive_sizes.emplace_back(entry+0x1c,c.u32(entry+0x1c));
            }
            for(auto [at,value]:archive_sizes)c.put32(at,0xffffffffu);
            const auto next=c.u32(setting(3));
            try {
                invoke(0x10719560,{display,1,0});
                if(empty) {
                    // A direct original section build avoids mainbuilder's
                    // lazy h3 refill. No fixture is freed, borrowed or changed.
                    c.put32(setting(3),0); invoke(0x1002a930,{125,500});
                } else invoke(0x1001e2b0,{});
            } catch(...) {
                c.put32(setting(3),next);
                for(auto [at,value]:archive_sizes)c.put32(at,value);
                throw;
            }
            if(empty)c.put32(setting(3),next);
            for(auto [at,value]:archive_sizes)c.put32(at,value);
            refresh(); // Match the original completed popup ownership stage.
        };
        const auto meanings=[&] {
            std::vector<std::uint8_t> bytes;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                for(const auto [offset,size]:{std::pair{8u,8u},{0x20u,8u},{0x38u,20u},{0x68u,0x114u}}) {
                    const auto part=s.memory.copy_out(control(i)+offset,size);
                    bytes.insert(bytes.end(),part.begin(),part.end());
                }
            return bytes;
        };
        const auto records=[&](bool include_history) {
            std::vector<std::pair<unsigned,std::vector<std::uint8_t>>> result;
            const auto append=[&](unsigned at,unsigned size) {
                if(at && std::none_of(result.begin(),result.end(),[at](const auto& item){return item.first==at;}))
                    result.emplace_back(at,s.memory.copy_out(at,size));
            };
            append(club,0x248); append(managed,0x248); append(person,0x70);
            append(c.toc(-0x7b78),4); append(c.toc(-0x7b80),8);
            if(include_history)append(history,0x180);
            const auto next=c.u32(setting(3));
            append(next,0x50); append(c.u32(setting(2)),0x50);
            if(next) { append(c.u32(next+0x1c),0x248); append(c.u32(next+0x20),0x248); }
            const auto humans=c.u32(c.toc(-0x7a54));
            append(c.u32(humans)+(c.u32(person)-c.u32(c.toc(-0x7be4))+16)*0xa65c,0xa65c);
            for(auto staff:roster_records()) {
                const auto player=c.u32(staff+0x14),entry=c.u32(stats+0x30)+c.u32(player)*0x28;
                append(staff,0x70); append(player,0x48); append(entry,0x28);
                for(unsigned category=0;category<6;++category) {
                    const auto index=c.i32(entry+category*4);
                    if(index>=0 && index<c.i32(stats+0xc))append(c.u32(stats+4)+unsigned(index)*0x20,0x20);
                }
            }
            return result;
        };
        const auto unchanged=[&](const auto& saved) {
            for(const auto& [at,bytes]:saved)
                require(s.memory.copy_out(at,bytes.size())==bytes,"Next Match changed fixture, club, player, stats, history or time records");
        };
        phase="gates";
        const auto national=invoke(0x100c76c0,{c.u32(managed+0x2c)});
        require(national,"Next Match capture lacks its real national team");
        for(auto candidate:{managed,club,national}) {
            c.put32(setting(0),candidate);
            const auto registers=s.registers; const auto unique=s.unique; const auto pc=s.pc;
            require(adaptive_screen_available(s,0x1001e2b0),"Public Next Match failed its eligibility gate");
            require(s.registers==registers && s.unique==unique && s.pc==pc,"Next Match gate entered guest code");
        }
        c.put32(setting(0),club);
        c.put16(manager+0x2c58,1); require(!adaptive_screen_available(s,0x1001e2b0),"Secondary Next Match adapted"); c.put16(manager+0x2c58,0);
        const auto network=c.u32(manager+0x2c64);
        c.put32(manager+0x2c64,1); require(!adaptive_screen_available(s,0x1001e2b0),"Network Next Match adapted"); c.put32(manager+0x2c64,network);
        select_new_screens(false); require(!adaptive_screen_available(s,0x1001e2b0),"Disabled Next Match adapted"); select_new_screens(true);
        int body=-1; unsigned active_date_action=0;
        const auto build=[&](GuiCanvas size,bool adaptive,bool synthetic=false,bool overflow=false,bool empty=false) {
            active_canvas=size; phase=empty?"empty build":synthetic?"rich text build":"build";
            select_new_screens(adaptive);
            require(s.platform->resize_gui_canvas(s,size),"Next Match resize failed");
            original_build(empty);
            body=-1; bool lower_tabs=false;
            for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                const auto at=grid(i); const auto box=rect(at);
                if(box==ScreenRect{100,510,790,545})lower_tabs=true;
                if(c.u32(at+0x18)==2 && c.u8(at+0xbb0)==(empty?1:2) && box.left==110 && box.right==780 &&
                   box.top==(empty?125:190) && (box.bottom==500 || box.bottom==545))body=int(i);
            }
            require(body>=0,"Next Match original body missing");
            const auto area=grid(unsigned(body)); const auto old=rect(area);
            const auto old_rows=c.u8(area+0xbb1);
            require(empty || old_rows==(old.bottom==500?14:16),"Next Match original row capacity changed");
            std::vector<unsigned> synthetic_links;
            std::vector<std::pair<unsigned,bool>> synthetic_value_groups;
            if(synthetic) {
                unsigned label=0,link=0;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                    const auto at=control(i);
                    if(c.i16(at+0x72)!=body)continue;
                    if(c.u32(at+8)==0 && c.u32(at+0x38)==1)label=i;
                    if(c.u32(at+8)==2021)link=i;
                }
                require(label && link,"Next Match rich text fixture lacks original source controls");
                const auto append=[&](unsigned source,unsigned row,int left,int right) {
                    const auto index=unsigned(c.u16(display+0x129da6)),at=control(index);
                    require(index<1199,"Next Match controlled GUIO capacity exhausted");
                    s.memory.copy_in(at,s.memory.copy_out(control(source),0x17c));
                    c.put32(at+0x4c,0); c.put32(at+0x5c,0); // Never duplicate owned render caches.
                    c.put16(at+0x68,index); c.put16(at+0x72,body); c.put16(at+0x74,0xffff);
                    c.put32(at+0x20,0); c.put32(at+0x24,row);
                    c.put32(at+0x28,left); c.put32(at+0x2c,right); c.put32(at+0x30,0); c.put32(at+0x34,0);
                    c.put32(at+0x3c,1);
                    c.put16(display+0x129da6,index+1); invoke(0x107144c0,{area,index});
                    return index;
                };
                // The original formatter can emit a left heading and right
                // value paragraph on the SAME row. The heading's late link
                // also exceeds the value threshold; each base/link group must
                // retain its own delta, in the original emission order.
                synthetic_value_groups.emplace_back(append(label,8,113,600),false);
                const auto left_link=append(link,8,400,420);
                synthetic_links.push_back(left_link); synthetic_value_groups.emplace_back(left_link,false);
                const int value_left=overflow?355:362;
                synthetic_value_groups.emplace_back(append(label,8,value_left,value_left+390),true);
                const auto right_link=append(link,8,value_left+68,value_left+118);
                synthetic_links.push_back(right_link); synthetic_value_groups.emplace_back(right_link,true);
                synthetic_value_groups.emplace_back(append(label,9,value_left,value_left+380),true);
                const auto other_link=append(link,9,value_left+58,value_left+108);
                synthetic_links.push_back(other_link); synthetic_value_groups.emplace_back(other_link,true);
                if(overflow)for(unsigned row=11;row<=38;++row)append(label,row,0,0);
                invoke(0x10714630,{area}); invoke(0x107145f0,{area,overflow?2u:0u});
                refresh();
                require(bool(c.u8(area+0xbef))==overflow,"Next Match controlled overflow did not initialize");
            }
            struct Span { unsigned index,row; int left,right; };
            std::vector<Span> spans;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i);
                if(c.i16(at+0x72)==body && (c.i32(at+0x28)!=0 || c.i32(at+0x2c)!=0))
                    spans.push_back({i,c.u32(at+0x24),c.i32(at+0x28),c.i32(at+0x2c)});
            }
            const auto old_scroll=c.u32(area+0x1fc); const auto max_row=c.i32(area+0x14);
            const bool old_overflow=max_row+1>old_rows;
            const auto semantic=meanings(),original=s.memory.copy_out(display,0x129da8),rng=random_state(s);
            const auto saved=records(true); const auto registers=s.registers; const auto unique=s.unique;
            const auto pc=s.pc,stack_base=c.sp()-0x8000; const auto stack=s.memory.copy_out(stack_base,0x8100);
            phase="layout"; layout_built_screen(s,display,0x1001e2b0);
            require(meanings()==semantic,"Next Match layout changed original text, style, action, payload or ownership");
            unchanged(saved); require(random_state(s)==rng,"Next Match layout changed RNG");
            require(s.registers==registers && s.unique==unique && s.pc==pc && s.memory.copy_out(stack_base,0x8100)==stack,
                    "Next Match layout clobbered guest ABI/stack");
            if(!adaptive || size.original()) {
                require(s.memory.copy_out(display,0x129da8)==original,"Classic Next Match display changed");
                return semantic;
            }
            const auto panel=rect(area); auto available=ScreenLayout{size}.content(empty?125:190);
            if(lower_tabs)available.bottom-=45;
            const auto width=std::min(1600,available.width());
            require(panel.left==available.left+(available.width()-width)/2 && panel.width()==width &&
                    panel.top==old.top && panel.bottom<=available.bottom,"Next Match body lost bounded centering");
            const auto rows=c.u8(area+0xbb1);
            if(empty)require(rows==old_rows && c.u32(area+0x1fc)==old_scroll,"Empty Next Match changed its row/scroll policy");
            else {
                require(c.u8(area+0xbb0)==2 && c.u8(area+0xbb2)==3 && c.u8(area+0xbb3)==5 && rows>=old_rows && rows<=30 &&
                        panel.height()<=646,"Next Match changed columns or exceeded its row bound");
                if(size.height>=900)require(rows>old_rows,"Tall Next Match did not reveal additional rows");
                for(unsigned row=old_rows;row<rows;++row)require(c.u8(area+0xbd0+row)==1,"Next Match new row weight was not initialized");
                require(c.u32(area+0x1fc)==unsigned(std::clamp(std::int32_t(old_scroll),0,std::max(0,max_row-int(rows)+1))),
                        "Next Match expansion lost its original clamped scroll origin");
            }
            const auto value_origin=[](ScreenRect box,bool scrolling) {
                return box.left+3+3*(box.right-2-(scrolling?20:0)-box.left-2)/8;
            };
            for(const auto& span:spans) {
                int minimum=span.left;
                for(const auto& other:spans)if(other.row==span.row)minimum=std::min(minimum,other.left);
                bool value_group=minimum>=(old_overflow?355:362);
                for(const auto& [index,is_value]:synthetic_value_groups)if(index==span.index)value_group=is_value;
                const int delta=empty?(panel.left+panel.right-old.left-old.right)/2:
                    value_group?value_origin(panel,max_row+1>rows)-value_origin(old,old_overflow):panel.left-old.left;
                const auto at=control(span.index);
                require(c.i32(at+0x28)==span.left+delta && c.i32(at+0x2c)==span.right+delta,
                        "Next Match rich-text base/link group lost its common translation or original width");
            }
            if(!empty) {
                bool left_nav=false,right_nav=false,heading=false,top_tabs=false,bottom_tabs=!lower_tabs,footer=false;
                for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                    const auto box=rect(grid(i));
                    if(box==ScreenRect{panel.left,125,panel.left+250,145})left_nav=true;
                    if(box==ScreenRect{panel.right-250,125,panel.right,145})right_nav=true;
                    if(box==ScreenRect{100,80,size.width-10,115})top_tabs=true;
                    if(box==ScreenRect{100,size.height-90,size.width-10,size.height-55})bottom_tabs=true;
                    if(box==ScreenLayout{size}.footer())footer=true;
                }
                for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                    if(c.i16(control(i)+0x72)<0 && c.u32(control(i)+8)==2005 &&
                       rect(control(i)+0x10)==ScreenRect{panel.left,150,panel.right,185})heading=true;
                require(left_nav && right_nav && heading && top_tabs && bottom_tabs && footer,
                        "Next Match navigation, opponent heading or shared shell did not follow the body/canvas");
            }
            refresh();
            phase="hits";
            unsigned links=0;
            const auto first=c.i16(area+0xb76);
            const auto first_control=first>=0 && first<c.i16(area+0xb74)?c.i16(area+0x212+unsigned(first)*2):-1;
            const auto origin=first_control>=0?c.u32(control(unsigned(first_control))+0x24):0;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i); const auto action=c.u32(at+8); const auto owner=c.i16(at+0x72);
                if(owner!=body || c.u32(at+0x24)<origin || c.u32(at+0x24)>=origin+rows)continue;
                const auto box=rect(at+0x10);
                require(box.left<=box.right && box.top<=box.bottom && box.left>=panel.left && box.right<=panel.right &&
                        box.top>=panel.top && box.bottom<=panel.bottom,"Next Match visible row exceeds its body or has inverted bounds");
                if(c.i32(at+0x28) || c.i32(at+0x2c))
                    require(box.right<=panel.right-2-(max_row+1>rows?20:0),"Next Match explicit span overlaps the scrollbar gutter");
                if(action<2000)continue;
                const auto got=hit(i);
                if(got!=int(i))throw std::runtime_error("Next Match link missed: scenario="+std::to_string(scenario)+
                    " canvas="+std::to_string(size.width)+"x"+std::to_string(size.height)+" index="+std::to_string(i)+
                    " row="+std::to_string(c.u32(at+0x24))+" origin="+std::to_string(origin)+" got="+std::to_string(got));
                ++links;
            }
            if(!empty && !scenario && !links) {
                std::string actions;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                    const auto at=control(i);
                    if(c.i16(at+0x72)==body || (c.i16(at+0x72)<0 && c.i32(at+0x14)==150))
                        actions+=" ["+std::to_string(i)+":g"+std::to_string(c.i16(at+0x72))+":r"+
                            std::to_string(c.u32(at+0x24))+":a"+std::to_string(c.u32(at+8))+
                            ":kind"+std::to_string(c.u32(at+0xc))+"]";
                }
                const auto next=c.u32(setting(3));
                throw std::runtime_error("Next Match fixture has no visible original links: scenario="+
                    std::to_string(scenario)+" canvas="+std::to_string(size.width)+"x"+std::to_string(size.height)+
                    " phase="+phase+" synthetic="+std::to_string(synthetic)+" overflow="+std::to_string(overflow)+
                    " after_date_action="+std::to_string(active_date_action)+" body="+std::to_string(body)+
                    " scroll="+std::to_string(c.i32(area+0x1fc))+" origin="+std::to_string(origin)+
                    " max_row="+std::to_string(max_row)+" rows="+std::to_string(rows)+
                    " fixture="+hex_address(next)+" competition="+std::to_string(c.u32(next))+
                    " date="+std::to_string(c.u16(next+0x2a))+":"+std::to_string(c.u16(next+0x28))+
                    " real_actions="+actions);
            }
            if(!empty) {
                unsigned opponent=0;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                    if(c.i16(control(i)+0x72)<0 && c.u32(control(i)+8)==2005 &&
                       rect(control(i)+0x10)==ScreenRect{panel.left,150,panel.right,185})opponent=i;
                require(opponent && hit(opponent)==int(opponent),"Next Match opponent heading missed its visible hit area");
            }
            if(synthetic)for(auto index:synthetic_links)require(hit(index)==int(index),"Next Match synthetic span link missed its row");
            return semantic;
        };
        build({800,600},true); // Warm original current-season query caches.
        const auto baseline=build({800,600},true),rng=random_state(s);
        for(auto size:{GuiCanvas{801,601},GuiCanvas{800,900},GuiCanvas{1280,900},GuiCanvas{1440,600},GuiCanvas{3200,1800}}) {
            require(build(size,true)==baseline,"Next Match resize changed original labels, values or commands");
            require(random_state(s)==rng,"Next Match resize changed RNG");
        }
        require(build({1280,900},false)==baseline,"Disabled Next Match changed original controls");
        if(!scenario) {
            for(auto size:{GuiCanvas{801,601},GuiCanvas{1280,900},GuiCanvas{3200,1800}})
                for(bool overflow:{false,true})build(size,true,true,overflow);
            const auto empty=build({800,600},true,false,false,true);
            for(auto size:{GuiCanvas{801,601},GuiCanvas{1280,900},GuiCanvas{3200,1800}})
                require(build(size,true,false,false,true)==empty,"Empty Next Match changed original text or commands");
            require(build({1280,900},false,false,false,true)==empty,"Disabled empty Next Match changed original controls");
            // Exercise both original date controls through the actual main
            // handler. These query another fixture; they never advance time.
            for(bool adaptive:{false,true}) {
                const auto size=adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600};
                build(size,adaptive);
                const auto first_fixture=s.memory.copy_out(c.u32(setting(3)),0x50);
                for(unsigned action:{39u,38u}) {
                    active_date_action=action;
                    phase="date command"; unsigned button=0;
                    for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                        if(c.u32(control(i)+8)==action)button=i;
                    require(button && hit(button)==int(button),"Next Match date button missed its original hit area");
                    const auto saved=records(false); const auto before_rng=random_state(s); const auto before=c.u32(setting(3));
                    const auto before_date=std::pair{c.u16(before+0x28),c.u16(before+0x2a)};
                    require(std::int32_t(invoke(0x10036f10,{button}))==-11 && c.u32(setting(4))==3,
                            "Next Match date command changed its return/section contract");
                    const auto after=c.u32(setting(3));
                    const auto after_date=std::pair{c.u16(after+0x28),c.u16(after+0x2a)};
                    require(after && after!=before && (action==39?after_date>before_date:after_date<before_date),
                            "Next Match date command did not select an owned fixture in the requested direction");
                    unchanged(saved); require(random_state(s)==before_rng,"Next Match date command changed RNG");
                    build(size,adaptive);
                }
                require(s.memory.copy_out(c.u32(setting(3)),0x50)==first_fixture,
                        "Next Match date round trip changed the original pending fixture payload");
            }
        }
    }
    initialize_new_screens();
    std::cout<<"Next Match original tabs/date commands, public gates, rich spans/scrolling, records/RNG and post-layout ABI passed\n";
}

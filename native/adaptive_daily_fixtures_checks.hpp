// Existing match-day queue; no match advancement, new capture or career I/O.
void daily_fixtures_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path/"Screens.plist"); select_new_screens(true);
    initialize_native_debugging(true);
    const unsigned features=(1u<<unsigned(DebugFeature::match_result))|(1u<<unsigned(DebugFeature::all_draws));
    for(unsigned group_index:{0u,4u,7u}) {
        Clock clock; clock.idle_input=true;
        auto state=std::make_unique<State>(); auto& s=*state;
        load_match_snapshot(s,path); s.code_fragment_observer=&clock;
        guest::Cpu c(s); c.set(2,c.u32(c.sp()+0x14));
        const auto ui=c.toc(-0x7bb0); c.put16(ui+0x2c58,0);
        const auto invoke=[&](unsigned entry,std::initializer_list<unsigned> args) {
            unsigned reg=3; for(auto arg:args)c.set(reg++,arg);
            s.budget=s.executed+400000000;
            try { c.call(entry,return_sentinel); }
            catch(const std::exception& error) {
                throw std::runtime_error("Daily fixtures group="+std::to_string(group_index)+" entry="+hex_address(entry)+
                    " pc="+hex_address(s.pc)+": "+error.what());
            }
            return c.r(3);
        };
        s.platform=std::make_shared<NativePlatform>(temp.path/"game",temp.path/"user",AudioMode::offline);
        const auto scratch=allocate(s,32,true); world(s,scratch,scratch+16);
        require(s.platform->resize_gui_canvas(s,{801,601}) && s.platform->resize_gui_canvas(s,{800,600}),"Daily fixtures pixel initialization");
        const auto queue=c.u32(c.toc(-0x7420));
        require(queue && group_index<c.u32(queue+0x18),"Existing match-day group missing");
        const auto group=c.u32(queue+0x10)+group_index*0x20;
        const auto entries=c.u32(queue+0x1c), entry_count=c.u32(queue+0x24);
        invoke(0x1028ad60,{c.u32(ui+4),group});
        const auto history=c.u32(ui+0x10),display=c.u32(ui+0x24);
        const auto setting=[history](unsigned slot){return history+0x14+slot*8;};
        require(c.u32(c.u32(history))==0x1028ae80 && c.u32(setting(1))==group,"Daily fixtures opener changed history ownership");
        const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
        const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
        const auto rect=[&](unsigned at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
        const auto refresh=[&] {
            for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
            }
        };
        const auto hit=[&](unsigned index,bool right=false) {
            const auto box=rect(control(index)+0x10);
            c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
            c.put8(c.toc(-0x7bac),!right); c.put8(c.toc(-0x5c0c),right);
            return std::int16_t(invoke(c.u8(display)==1?0x1071aa10:0x1071a280,{display}));
        };
        const auto find=[&](unsigned action,unsigned payload) {
            for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                if(c.u32(control(i)+8)==action && c.u32(control(i)+0x48)==payload)return i;
            throw std::runtime_error("Missing fixture action "+std::to_string(action));
        };
        const auto navigation=[&](unsigned action) {
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i); const int owner=c.i16(at+0x72);
                if(c.u32(at+8)==action && owner>=0 && c.i32(grid(owner)+4)==gui_canvas(s).height-45 &&
                    c.u8(grid(owner)+0xbb0)==2 && c.u8(grid(owner)+0xbb1)==1)return i;
            }
            throw std::runtime_error("Missing footer navigation "+std::to_string(action));
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
        const auto records=[&] {
            std::vector<std::pair<unsigned,std::vector<std::uint8_t>>> saved;
            const auto append=[&](unsigned at,unsigned size) {
                if(at && size && std::none_of(saved.begin(),saved.end(),[at](const auto& x){return x.first==at;}))
                    saved.emplace_back(at,s.memory.copy_out(at,size));
            };
            append(history,0x1fc); append(queue,0x3c);
            append(c.u32(queue+4),c.u32(queue+0xc)*0x18);
            append(c.u32(queue+0x10),c.u32(queue+0x18)*0x20);
            append(entries,entry_count*0x6c);
            append(c.toc(-0x7b80),8); append(c.toc(-0x7b78),4);
            for(unsigned i=0;i<entry_count;++i) {
                const auto fixture=c.u32(entries+i*0x6c);
                if(!fixture)continue;
                append(fixture,0x50); append(c.u32(fixture+0x1c),0x248); append(c.u32(fixture+0x20),0x248);
            }
            return saved;
        };
        const auto unchanged=[&](const auto& saved) {
            for(const auto& [at,bytes]:saved)require(s.memory.copy_out(at,bytes.size())==bytes,"Daily layout changed queue/fixture/club/history/date records");
        };
        int body=-1;
        const auto build=[&](GuiCanvas size,bool adaptive,unsigned debug_features,unsigned fixture_rows=0) {
            select_new_screens(adaptive); select_native_debugging_features(debug_features);
            require(s.platform->resize_gui_canvas(s,size),"Daily fixtures resize failed");
            invoke(0x10719560,{display,1,0}); invoke(0x1028ae80,{});
            body=int(c.u32(setting(3))); const auto area=grid(body);
            require(body>=0 && unsigned(body)<c.u16(display+0x129da4) && c.u8(area+0xbb0)==9,"Original fixture grid missing");
            if(fixture_rows) {
                // Additional GUI rows exercise scrolling without fabricating
                // matches or changing the queue consumed by game/debug logic.
                unsigned source=0;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                    if(c.i16(control(i)+0x72)==body && c.u32(control(i)+8)==2005 && c.u32(control(i)+0x20)==2)source=i;
                require(source,"Overflow fixture lacks a club control");
                for(unsigned row=c.u32(area+0x14)+1;row<fixture_rows;++row) {
                    const auto index=unsigned(c.u16(display+0x129da6)),at=control(index);
                    require(index<1199,"Daily controlled GUIO capacity exceeded");
                    s.memory.copy_in(at,s.memory.copy_out(control(source),0x17c));
                    c.put32(at+0x4c,0); c.put32(at+0x5c,0);
                    c.put16(at+0x68,index); c.put16(at+0x72,body); c.put16(at+0x74,0xffff);
                    c.put32(at+0x24,row); c.put32(at+0x3c,1);
                    c.put16(display+0x129da6,index+1); invoke(0x107144c0,{area,index});
                }
                invoke(0x107145f0,{area,6});
            }
            const auto before=rect(area); const auto old_rows=c.u8(area+0xbb1);
            const auto last=c.i32(area+0x14), old_scroll=c.i32(area+0x1fc);
            const auto original=s.memory.copy_out(display,0x129da8);
            // Obtain measured offsets from the genuine original renderer, then
            // restore its fresh grid before production's layout boundary.
            invoke(0x10714630,{area}); invoke(0x10714110,{area,0});
            const int home_left=before.left+c.i32(area+0x24);
            std::vector<std::array<int,3>> spans;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i); if(c.i16(at+0x72)!=body)continue;
                if(c.i32(at+0x28))spans.push_back({int(i),c.i32(at+0x28),c.i32(at+0x2c)});
            }
            s.memory.copy_in(display,original);
            const auto saved=records(); const auto semantic=meanings(),rng=random_state(s);
            const auto registers=s.registers; const auto unique=s.unique;
            const auto pc=s.pc,stack_base=c.sp()-0x8000; const auto stack=s.memory.copy_out(stack_base,0x8100);
            const auto result_plans=match_result_debugging(static_cast<const State&>(s)).fixtures.size();
            const auto draw_plans=match_result_debugging(static_cast<const State&>(s)).draw_days.size();
            layout_built_screen(s,display,0x1028ae80);
            unchanged(saved); require(random_state(s)==rng,"Daily layout changed RNG");
            require(meanings()==semantic,"Daily layout changed original/debug text, color, style, action or payload");
            require(s.registers==registers && s.unique==unique && s.pc==pc && s.memory.copy_out(stack_base,0x8100)==stack,
                    "Daily layout changed guest ABI/stack");
            require(match_result_debugging(static_cast<const State&>(s)).fixtures.size()==result_plans &&
                    match_result_debugging(static_cast<const State&>(s)).draw_days.size()==draw_plans,"Daily layout changed debugging plans");
            if(!adaptive || size.original()) {
                require(s.memory.copy_out(display,0x129da8)==original,"Classic daily fixtures changed");
                refresh(); return;
            }
            const auto panel=rect(area); const int rows=c.u8(area+0xbb1);
            require(panel.top==before.top && panel.bottom<=size.height-65 && panel.width()<=1440 && panel.height()<=646 &&
                    panel.left>=110 && panel.right<=size.width-20,"Daily panel exceeds bounds");
            require(rows>=old_rows && rows<=30 && (size.height<900 || rows>old_rows),"Daily capacity did not expand safely");
            require(c.i32(area+0x1fc)==std::clamp(old_scroll,0,std::max(0,last+1-rows)),"Daily scrolling lost original clamp");
            require(bool(c.u8(area+0xbef))==(last+1>rows),"Daily scrollbar disagrees with visible rows");
            for(int row=old_rows;row<rows;++row)require(c.u8(area+0xbd0+row)==1,"New daily row lacks unit weight");
            refresh(); const int new_home_left=panel.left+c.i32(area+0x24);
            unsigned links=0,overlays=0,subtitle=0,title_link=0,competition_button=0,debug_button=0,footer=0;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i); const auto box=rect(at+0x10); const auto action=c.u32(at+8);
                if(c.i16(at+0x72)==body) {
                    if(box.bottom<=panel.top || box.top>=panel.bottom)continue;
                    require(box.left>=panel.left && box.right<=panel.right && box.top>=panel.top && box.bottom<=panel.bottom,
                            "Daily fixture control outside body");
                    const bool overlay=action>=30230 && action<=30232;
                    if(overlay || action==2005 || action==2006 || action==2002 || action==2003 || action==2021 || action==2) {
                        require(hit(i,overlay)==int(i),"Daily fixture link/right-click missed its bounds");
                        if(overlay)++overlays; else ++links;
                    }
                } else if(c.i16(at+0x72)<0) {
                    if(box==ScreenLayout{size}.subtitle())++subtitle;
                    if(action==2000 && c.u32(at+0xc)==6) {
                        ++title_link; require(box==ScreenRect{125,10,size.width-10,70},"Competition title link not expanded");
                        require(hit(i)==int(i),"Competition title link missed");
                    }
                    if(action==2000 && box.top==145) {
                        ++competition_button;
                        require(box==ScreenRect{panel.right-125,145,panel.right,165} && hit(i)==int(i),"Competition button misaligned");
                    }
                }
            }
            for(const auto span:spans) {
                require(span[1]==115 || span[1]==180,"Unexpected fixture measured-text origin");
                const int shift=span[1]==180 ? new_home_left-home_left : panel.left-before.left;
                if(c.i32(control(span[0])+0x28)!=span[1]+shift || c.i32(control(span[0])+0x2c)!=span[2]+shift)
                    throw std::runtime_error("Fixture measured span mismatch: canvas="+std::to_string(size.width)+"x"+std::to_string(size.height)+
                        " rows="+std::to_string(fixture_rows)+" original="+std::to_string(span[1])+","+std::to_string(span[2])+
                        " oldHome="+std::to_string(home_left)+" newHome="+std::to_string(new_home_left)+
                        " actual="+std::to_string(c.i32(control(span[0])+0x28))+","+std::to_string(c.i32(control(span[0])+0x2c)));
            }
            for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                const auto at=grid(i); const auto box=rect(at);
                if(box==ScreenRect{panel.right-252,145,panel.right-127,165})++debug_button;
                if(box.top==size.height-45 && c.u8(at+0xbb0)==2) {
                    ++footer; require(box==ScreenRect{panel.right-340,size.height-45,panel.right,size.height-10} &&
                        c.u8(at+0xbb2)==1 && c.u8(at+0xbb3)==1,"Daily navigation did not become compact/equal");
                }
            }
            const bool has_competition=before.top==170;
            require(links && subtitle==1 && title_link==unsigned(has_competition) && competition_button==unsigned(has_competition) && footer==1,
                    "Daily fixture shell controls were not verified");
            require(debug_button==((has_competition && (debug_features&(1u<<unsigned(DebugFeature::all_draws))))?1u:0u),"Daily debugging toolbar missing/unexpected");
            if(debug_features && group_index!=7)require(overlays,"Daily right-click overlays not exercised");
        };
        select_new_screens(true);
        const auto saved_gate=records(); const auto gate_registers=s.registers; const auto gate_unique=s.unique; const auto gate_pc=s.pc;
        require(adaptive_screen_available(s,0x1028ae80),"Daily fixture gate rejected genuine queue group");
        unchanged(saved_gate); require(s.registers==gate_registers && s.unique==gate_unique && s.pc==gate_pc,"Daily gate called guest code");
        c.put16(ui+0x2c58,1); require(!adaptive_screen_available(s,0x1028ae80),"Secondary daily fixtures adapted"); c.put16(ui+0x2c58,0);
        const auto network=c.u32(ui+0x2c64);
        c.put32(ui+0x2c64,1); require(!adaptive_screen_available(s,0x1028ae80),"Network daily fixtures adapted"); c.put32(ui+0x2c64,network);
        for(unsigned invalid:{0u,group+1,c.u32(queue+0x10)+c.u32(queue+0x18)*0x20}) {
            c.put32(setting(1),invalid); require(!adaptive_screen_available(s,0x1028ae80),"Invalid queue sublist adapted");
        }
        c.put32(setting(1),group);
        const auto first=c.u32(group+4),last=c.u32(group+8);
        c.put32(group+4,0xffffffffu); require(!adaptive_screen_available(s,0x1028ae80),"Negative queue start adapted"); c.put32(group+4,first);
        c.put32(group+8,entry_count); require(!adaptive_screen_available(s,0x1028ae80),"Out-of-range queue end adapted"); c.put32(group+8,last);
        const auto first_fixture=c.u32(entries+first*0x6c);
        c.put32(entries+first*0x6c,0); require(!adaptive_screen_available(s,0x1028ae80),"Missing first fixture adapted"); c.put32(entries+first*0x6c,first_fixture);
        for(unsigned debug_features:{0u,features}) {
            for(auto size:{GuiCanvas{800,600},GuiCanvas{801,601},GuiCanvas{1280,900},GuiCanvas{1600,1000},GuiCanvas{3200,1800},GuiCanvas{800,900}})
                build(size,true,debug_features);
            build({1280,900},false,debug_features);
        }
        if(group_index==0) {
            for(unsigned rows:{22u,39u})for(auto size:{GuiCanvas{1280,900},GuiCanvas{3200,1800}})
                build(size,true,features,rows);
            // Original friendly-competition branch omits its toolbar and starts
            // at145. This capture has only competitive sublists, so change the
            // copied sublist ID temporarily, leaving every core fixture intact.
            const auto competition_id=c.u32(group);
            c.put32(group,72);
            build({1280,900},true,features); build({3200,1800},true,0);
            c.put32(group,competition_id);
            const auto original_fixture=s.memory.copy_out(first_fixture,0x50);
            for(unsigned variant:{0u,1u,2u,3u}) {
                c.put32(first_fixture+8,0xffffffffu); // No archived report is queried.
                c.put8(first_fixture+0x43,variant==2 ? 0xfe : variant==3 ? 0xfd : 2);
                c.put8(first_fixture+0x44,1);
                c.put8(first_fixture+0x47,variant==1?4:0xff); c.put8(first_fixture+0x48,variant==1?3:0xff);
                c.put8(first_fixture+0x4b,variant==1?5:0xff); c.put8(first_fixture+0x4c,variant==1?4:0xff);
                if(variant==1) {c.put8(first_fixture+0x3d,4); c.put8(first_fixture+0x3e,4);}
                build({1280,900},true,0); build({800,600},true,0);
                s.memory.copy_in(first_fixture,original_fixture);
            }
        }
        build({1280,900},true,features);
        if(group_index!=7) {
            unsigned fixture=0;
            for(unsigned i=c.u32(group+4);i<=c.u32(group+8);++i)
                if(debug_match_result_eligible(s,c.u32(entries+i*0x6c))) {fixture=c.u32(entries+i*0x6c);break;}
            require(fixture,"Daily debug fixture missing");
            for(const auto [action,expected]:{std::pair{30230u,ForcedMatchResult::home_win},
                    {30231u,ForcedMatchResult::draw},{30232u,ForcedMatchResult::away_win},{30232u,ForcedMatchResult::normal}}) {
                const auto choice=find(action,fixture);
                require(hit(choice,true)==int(choice),"Daily result right-click missed");
                require(std::int32_t(invoke(0x1028bc90,{choice}))==-11,"Daily result action did not rebuild");
                require(debug_match_result(s,fixture)==expected,"Daily result action changed target choice");
                require(!c.u8(c.toc(-0x5c0c)) && !c.u8(c.toc(-0x7bac)),"Daily right-click was not consumed");
                build({1280,900},true,features);
            }
            for(bool enabled:{true,false}) {
                unsigned parent=0;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                    if(c.i16(control(i)+0x72)>=0 && c.i16(control(i)+0x74)>=0 && c.i32(control(i)+0x14)==145)parent=i;
                require(parent && hit(parent)==-1 && c.u8(display)==1,"Daily Debugging popup failed to open");
                const auto choice=find(30030,0);
                require(hit(choice)==int(choice) && std::int32_t(invoke(0x1028bc90,{choice}))==-11 && debug_draw_day(s,fixture)==enabled,
                        "Daily All Draw menu changed behavior");
                build({1280,900},true,features);
            }
        }
        const auto back=navigation(0xfffffffeu);
        require(hit(back)==int(back) && std::int32_t(invoke(0x1028bc90,{back}))==-2,"Daily Back lost its original command");
        // Append a genuine forward history node, then model the manager's Back
        // pointer selection. The original footer must expose Next on this node.
        invoke(0x1028ad60,{c.u32(ui+4),group});
        const auto forward=c.u32(ui+0x10);
        require(forward!=history && c.u32(history+0x1f8)==forward,"Original daily opener did not create a forward history link");
        c.put32(ui+0x10,history);
        build({1280,900},true,features);
        const auto next=navigation(0xfffffffdu);
        require(hit(next)==int(next) && std::int32_t(invoke(0x1028bc90,{next}))==-3,"Daily Next lost its original command");
        c.put32(ui+0x10,forward);
    }
    initialize_native_debugging(false); initialize_new_screens();
    std::cout<<"Daily fixture queue, links/debugging menus, compact navigation, original fallback and records/RNG/ABI passed\n";
}

// Included inside adaptive_gui_checks.cpp's anonymous namespace. All builders,
// View commands and hit tests run on the existing capture in disposable memory.
// General Info owns h28/h29; Stats deliberately has no new scroll persistence.
void club_info_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path/"Screens.plist"); select_new_screens(true);
    for(unsigned scenario:{0u,1u,2u}) {
        Clock clock; clock.idle_input=true;
        auto state=std::make_unique<State>(); auto& s=*state;
        load_match_snapshot(s,path); s.code_fragment_observer=&clock;
        guest::Cpu c(s); c.set(2,c.u32(c.sp()+0x14));
        const auto manager=c.toc(-0x7bb0); c.put16(manager+0x2c58,0);
        const auto captured=c.u32(manager+0x10),managed=c.u32(captured+0x14),person=c.u32(manager+4);
        require(c.u32(c.u32(captured))==0x1001e2b0,"Club information fixture is not a club page");
        unsigned active_view=1;
        const auto invoke=[&](unsigned entry,std::initializer_list<unsigned> args) {
            unsigned reg=3; for(auto arg:args)c.set(reg++,arg);
            s.budget=s.executed+400000000;
            try { c.call(entry,return_sentinel); }
            catch(const std::exception& error) {
                throw std::runtime_error("Club information scenario "+std::to_string(scenario)+
                    " view "+std::to_string(active_view)+" entry "+hex_address(entry)+
                    " at "+hex_address(s.pc)+": "+error.what());
            }
            return c.r(3);
        };
        s.platform=std::make_shared<NativePlatform>(temp.path/"game",temp.path/"user",AudioMode::offline);
        const auto scratch=allocate(s,32,true); world(s,scratch,scratch+16);
        require(s.platform->resize_gui_canvas(s,{801,601}) && s.platform->resize_gui_canvas(s,{800,600}),
                "Club information pixel initialization");
        unsigned club=managed;
        if(scenario==1) {
            club=0;
            const auto base=c.u32(c.toc(-0x7be8));
            const auto normal=c.u32(c.toc(-0x7b50))-2*c.u32(c.toc(-0x7b04));
            for(unsigned i=0;i<normal && !club;++i) {
                const auto candidate=base+i*0x248;
                if(candidate!=managed && c.u32(candidate+0x2c)==c.u32(managed+0x2c) &&
                   c.u32(candidate+0x98) && c.u32(candidate+0x90)!=person)club=candidate;
            }
            require(club,"Club information capture lacks another populated normal club");
        } else if(scenario==2) {
            club=invoke(0x100c76c0,{c.u32(managed+0x2c)});
            require(club && invoke(0x100c7d00,{club}),"Club information capture lacks its national team");
        }
        // Do not fabricate history indexes or a national team record. The
        // original opener establishes all of its history slots and resources.
        invoke(0x1001d410,{club,0,0,0,0});
        const auto history=c.u32(manager+0x10),display=c.u32(manager+0x24);
        const auto setting=[history](unsigned slot){return history+0x14+slot*8;};
        c.put32(setting(4),5);
        require(c.u32(c.u32(history))==0x1001e2b0 && c.u32(setting(0))==club,
                "Club information opener changed its target");
        const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
        const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
        const auto rect=[&](unsigned at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
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
            std::vector<std::pair<unsigned,std::vector<std::uint8_t>>> result;
            const auto append=[&](unsigned at,unsigned size) {
                if(at && std::none_of(result.begin(),result.end(),[at](const auto& item){return item.first==at;}))
                    result.emplace_back(at,s.memory.copy_out(at,size));
            };
            append(club,0x248); append(managed,0x248); append(person,0x70); append(history,0x180);
            append(c.toc(-0x7b78),4);
            const auto humans=c.u32(c.toc(-0x7a54));
            append(c.u32(humans)+(c.u32(person)-c.u32(c.toc(-0x7be4))+16)*0xa65c,0xa65c);
            for(unsigned slot=0;slot<50;++slot) {
                const auto staff=c.u32(club+0x98+slot*4);
                if(staff) { append(staff,0x70); append(c.u32(staff+0x14),0x48); }
            }
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i);
                if(c.u32(at+8)!=2007)continue;
                const auto staff=c.u32(at+0x48);
                if(staff) { append(staff,0x70); append(c.u32(staff+0x14),0x48); }
            }
            return result;
        };
        const auto refresh=[&] {
            for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
            }
        };
        const auto hit=[&](unsigned index) {
            const auto box=rect(control(index)+0x10);
            c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
            c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
            return std::int16_t(invoke(c.u8(display)==1?0x1071aa10:0x1071a280,{display}));
        };
        for(unsigned view:{1u,16u}) {
            c.put32(setting(27),view);
            require(adaptive_screen_available(s,0x1001e2b0),"Public club information failed its eligibility gate");
            const auto registers=s.registers; const auto unique=s.unique; const auto pc=s.pc;
            require(adaptive_screen_available(s,0x1001e2b0) && s.registers==registers && s.unique==unique && s.pc==pc,
                    "Club information gate entered guest code");
        }
        c.put32(setting(27),5);
        require(!adaptive_screen_available(s,0x1001e2b0),"Invalid combined information view entered layout");
        c.put32(setting(27),1);
        c.put16(manager+0x2c58,1); require(!adaptive_screen_available(s,0x1001e2b0),"Secondary club information adapted"); c.put16(manager+0x2c58,0);
        const auto network=c.u32(manager+0x2c64);
        c.put32(manager+0x2c64,1); require(!adaptive_screen_available(s,0x1001e2b0),"Network club information adapted"); c.put32(manager+0x2c64,network);
        select_new_screens(false);
        require(!adaptive_screen_available(s,0x1001e2b0),"Disabled club information adapted");
        select_new_screens(true);
        int summary=-1,body=-1;
        const auto build=[&](GuiCanvas size,bool adaptive) {
            active_view=c.u32(setting(27))&31;
            select_new_screens(adaptive);
            require(s.platform->resize_gui_canvas(s,size),"Club information resize failed");
            invoke(0x10719560,{display,1,0}); invoke(0x1001e2b0,{});
            summary=body=-1;
            for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                const auto at=grid(i);
                if(c.u8(at+0xbb0)!=2 || c.u32(at+0x18)!=2)continue;
                if(active_view==1 && rect(at)==ScreenRect{110,190,780,332})summary=int(i);
                if(rect(at)==(active_view==1?ScreenRect{110,337,780,500}:ScreenRect{110,190,780,500}))body=int(i);
            }
            require(body>=0 && (active_view!=1 || summary>=0),"Original club information panels missing");
            require(c.u8(grid(body)+0xbb1)==(active_view==1?7:17),"Original club information capacity changed");
            if(active_view==1)require(c.u32(setting(29))==unsigned(body),"General Info history no longer owns staff grid");
            refresh(); // Resolve popup ownership before comparing original control metadata.
            const auto old_scroll=c.u32(grid(body)+0x1fc);
            const auto max_row=c.i32(grid(body)+0x14);
            std::vector<std::array<unsigned,3>> explicit_x;
            if(active_view==1)for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i);
                if(c.i16(at+0x72)==body && c.u32(at+0x28))
                    explicit_x.push_back({i,c.u32(at+0x28),c.u32(at+0x2c)});
            }
            const auto semantic=meanings(),original=s.memory.copy_out(display,0x129da8),rng=random_state(s);
            const auto saved=records(); const auto registers=s.registers;
            const auto unique=s.unique; const auto pc=s.pc,stack_base=c.sp()-0x8000;
            const auto stack=s.memory.copy_out(stack_base,0x8100);
            layout_built_screen(s,display,0x1001e2b0);
            require(meanings()==semantic,"Club information layout changed text, styling, actions or ownership");
            require(random_state(s)==rng,"Club information layout changed RNG");
            for(const auto& [at,bytes]:saved)
                require(s.memory.copy_out(at,bytes.size())==bytes,"Club information layout changed club/person/history/cache records");
            require(s.registers==registers && s.unique==unique && s.pc==pc && s.memory.copy_out(stack_base,0x8100)==stack,
                    "Club information layout clobbered guest ABI/stack");
            if(!adaptive || size.original()) {
                require(s.memory.copy_out(display,0x129da8)==original,"Classic club information layout changed");
                return semantic;
            }
            const auto panel=rect(grid(body));
            require(panel.left>=110 && panel.width()<=1600 && panel.right<=size.width-20 && panel.bottom<=size.height-100,
                    "Club information panel exceeds shared content bounds");
            const auto available=ScreenLayout{size}.content(190);
            require(panel.width()==std::min(1600,available.width()) && panel.left==available.left+(available.width()-panel.width())/2,
                    "Club information panel lost bounded centering");
            if(active_view==1) {
                const auto info=rect(grid(summary));
                require(info==ScreenRect{panel.left,190,panel.right,332} && panel.top==337 && c.u8(grid(summary)+0xbb1)==6,
                        "General Info summary or original panel gap changed");
                require(c.u8(grid(body)+0xbb1)>=7 && c.u8(grid(body)+0xbb1)<=30,
                        "Staff visible capacity exceeded AutoGrid limits");
                if(size.height>=900)require(c.u8(grid(body)+0xbb1)>7,"Tall General Info did not reveal more staff");
                const auto maximum=unsigned(std::max(0,max_row-int(c.u8(grid(body)+0xbb1))+1));
                require(c.u32(grid(body)+0x1fc)==std::min(old_scroll,maximum),"Staff expansion lost the original clamped scroll origin");
                require(explicit_x.size()==2,"Original staff header/separator fixture changed");
                for(const auto& value:explicit_x) {
                    const auto at=control(value[0]);
                    require(c.u32(at+0x28)==value[1]+panel.left-110 && c.u32(at+0x2c)==value[2]+panel.left-110,
                            "Staff header/separator was stretched instead of translated");
                }
                if(!scenario && size.height==1800) {
                    require(max_row>=19 && c.u32(grid(body)+0x1fc)==0 && c.u8(grid(body)+0xbb1)>max_row,
                            "Tall General Info did not expose every captured staff row");
                    std::vector<bool> populated(unsigned(max_row)+1,false);
                    for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                        const auto at=control(i);
                        if(c.i16(at+0x72)!=body)continue;
                        const auto row=c.u32(at+0x24); require(row<populated.size(),"Staff row exceeds original maximum");
                        populated[row]=true;
                    }
                    require(std::all_of(populated.begin(),populated.end(),[](bool value){return value;}),
                            "Staff expansion dropped a populated row");
                }
            } else {
                require(panel.top==190 && panel.height()>=310 && panel.height()<=17*28 && c.u8(grid(body)+0xbb1)==17 &&
                        c.u32(grid(body)+0x1fc)==old_scroll,"Stats changed capacity/scroll or exceeded its row-height bound");
                require(scenario!=2 || max_row==8,"National Stats did not retain its nine original populated rows");
            }
            bool heading=false,view_menu=false,lower_tabs=false,upper_tabs=false,footer=false,print=false;
            unsigned links=0,link_candidates=0;
            for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
                const auto box=rect(grid(i));
                if(box==ScreenRect{100,size.height-90,size.width-10,size.height-55})lower_tabs=true;
                if(box==ScreenRect{100,80,size.width-10,115})upper_tabs=true;
                if(box==ScreenLayout{size}.footer())footer=true;
            }
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i); const auto box=rect(at+0x10);
                if(c.i16(at+0x72)<0 && box.top==150)heading=box==ScreenRect{panel.left,150,panel.right,185};
                if(c.i16(at+0x72)<0 && box.top==125)view_menu=box==ScreenRect{panel.left,125,panel.left+125,145};
                if(box==ScreenRect{size.width-90,15,size.width-15,35})print=true;
                const auto owner=c.i16(at+0x72); const auto action=c.u32(at+8);
                if(owner<0 || (owner!=body && owner!=summary) || (action!=2005 && action!=2007))continue;
                ++link_candidates;
                const auto area=grid(unsigned(owner));
                const auto row=c.u32(at+0x24),scroll=c.u32(area+0x1fc);
                // Stats keeps the original -1 scroll sentinel when all rows
                // fit. Renderer10714308 and input1071a280 use the first visible
                // control's row, not the unsigned scroll word, as their origin.
                const auto first=c.i16(area+0xb76);
                require(first>=0 && first<c.i16(area+0xb74),"Club information grid has no first visible entry");
                const auto first_control=c.i16(area+0x212+unsigned(first)*2);
                require(first_control>=0 && first_control<c.u16(display+0x129da6),
                        "Club information grid has an invalid first visible control");
                const auto origin=c.u32(control(unsigned(first_control))+0x24);
                if(row<origin || row>=origin+c.u8(area+0xbb1))continue;
                const auto bounds=rect(area);
                require(box.left>=bounds.left && box.right<=bounds.right && box.top>=bounds.top && box.bottom<=bounds.bottom,
                        "Club information link has stale visible hit bounds");
                const auto mode=c.u8(display);
                const auto clicked=hit(i);
                if(clicked!=int(i))throw std::runtime_error("Club information link missed: scenario="+
                    std::to_string(scenario)+" view="+std::to_string(active_view)+" canvas="+
                    std::to_string(size.width)+"x"+std::to_string(size.height)+" index="+std::to_string(i)+
                    " box="+std::to_string(box.left)+","+std::to_string(box.top)+","+
                    std::to_string(box.right)+","+std::to_string(box.bottom)+" got="+std::to_string(clicked)+
                    " owner="+std::to_string(owner)+" row="+std::to_string(row)+" scroll="+std::to_string(scroll)+
                    " origin="+std::to_string(origin)+
                    " visible="+std::to_string(c.u8(area+0xbb1))+" enabled="+std::to_string(c.u8(area+0xbee))+
                    " parent="+std::to_string(c.i16(area+0x20e))+" mode="+std::to_string(mode)+
                    " flags="+hex_address(c.u32(at+0xc))+" ordered_areas="+std::to_string(c.u16(display+0x12a8fc))+
                    " entry_start="+std::to_string(c.i16(area+0xb76))+" entries="+std::to_string(c.i16(area+0xb74)));
                ++links;
            }
            require(heading && view_menu && lower_tabs && upper_tabs && footer && print,
                    "Club information shared shell did not follow the canvas");
            if(!scenario && !links)throw std::runtime_error("Own club information fixture has no visible original links: view="+
                std::to_string(active_view)+" canvas="+std::to_string(size.width)+"x"+std::to_string(size.height)+
                " body="+std::to_string(body)+" summary="+std::to_string(summary)+
                " scroll="+std::to_string(c.i32(grid(body)+0x1fc))+" max_row="+std::to_string(max_row)+
                " visible="+std::to_string(c.u8(grid(body)+0xbb1))+" first_entry="+std::to_string(c.i16(grid(body)+0xb76))+
                " entries="+std::to_string(c.i16(grid(body)+0xb74))+" candidates="+std::to_string(link_candidates)+
                " controls="+std::to_string(c.u16(display+0x129da6))+" mode="+std::to_string(c.u8(display)));
            return semantic;
        };
        for(unsigned view:{1u,16u}) {
            c.put32(setting(27),view); c.put32(setting(28),3);
            build({800,600},true); // Warm the original status/statistics services.
            const auto baseline=build({800,600},true),rng=random_state(s);
            for(auto size:{GuiCanvas{801,601},GuiCanvas{800,900},GuiCanvas{1280,900},GuiCanvas{1440,600},GuiCanvas{3200,1800}}) {
                require(build(size,true)==baseline,"Club information resize changed original values or commands");
                require(random_state(s)==rng,"Club information resize changed RNG");
            }
            require(build({1280,900},false)==baseline,"Disabled club information changed original controls");
        }
        // Exercise the original popup and action32 handler with real controls.
        // Link hit tests above deliberately do not open another career screen.
        for(bool adaptive:{false,true}) {
            c.put32(setting(27),0x101); c.put32(setting(28),3);
            const auto size=adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600};
            build(size,adaptive);
            for(unsigned view:{16u,1u,16u,1u}) {
                unsigned parent=0,choice=0;
                for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                    const auto at=control(i);
                    if(c.i16(at+0x72)<0 && c.i32(at+0x14)==125 && c.i16(at+0x74)>=0)parent=i;
                    if(c.u32(at+8)==32 && c.u32(at+0x48)==view)choice=i;
                }
                require(parent && choice,"Club information View popup missing");
                const auto previous_view=c.u32(setting(27))&31;
                const auto previous_scroll=c.u32(grid(body)+0x1fc),previous_grid=c.u32(setting(29));
                const auto rng=random_state(s),saved_club=s.memory.copy_out(club,0x248);
                require(hit(parent)==-1 && c.u8(display)==1 && c.i16(display+0x12a930)==c.i16(control(parent)+0x74),
                        "Club information View popup failed to open");
                require(hit(choice)==int(choice),"Club information View choice missed its bounds");
                require(std::int32_t(invoke(0x10036f10,{choice}))==-11 && c.u32(setting(27))==(0x100|view),
                        "Club information command lost original return code or high view flags");
                if(previous_view==1)require(c.u32(setting(28))==previous_scroll && c.u32(setting(29))==previous_grid,
                        "General Info command did not persist its original staff scroll/grid");
                require(random_state(s)==rng && s.memory.copy_out(club,0x248)==saved_club,
                        "Club information View command changed club records or RNG");
                build(size,adaptive);
            }
        }
    }
    initialize_new_screens();
    std::cout<<"Club information original values/commands, staff scrolling, public gates, records/RNG and post-layout ABI passed\n";
}

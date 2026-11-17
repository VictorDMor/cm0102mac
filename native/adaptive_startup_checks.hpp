// Initialized startup capture; no game creation, save loading or file actions.
void startup_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist"); select_new_screens(true);
    const std::array canvases{GuiCanvas{800,600}, GuiCanvas{801,601}, GuiCanvas{800,900},
                             GuiCanvas{1440,600}, GuiCanvas{1280,900}, GuiCanvas{3200,1800}};
    unsigned available_hits = 0, unavailable = 0;
    for (bool rebuild : {false,true}) for (auto canvas : canvases) {
        struct Imports final : CodeFragmentBoundaryObserver {
            Clock clock;
            Imports() { clock.idle_input = true; }
            void enter(const State&, std::uint32_t, bool) override {}
            void leave(State&, std::uint32_t, bool) override {}
            bool import_fixture(State& s, const char* name) override {
                if (std::string_view(name) != "CarbonLib::FSMakeFSSpec") return clock.import_fixture(s,name);
                s.wr<4>(12,std::uint32_t(-43)); s.pc = s.r<4>(4128); return true;
            }
        } imports;
        auto state = std::make_unique<State>(); auto& s = *state;
        load_match_snapshot(s,path); s.code_fragment_observer = &imports;
        guest::Cpu c(s); c.set(2,c.u32(c.sp()+0x14));
        const auto ui = c.toc(-0x7bb0); c.put16(ui+0x2c58,0);
        const auto history = c.u32(ui+0x10), display = c.u32(ui+0x24);
        require(history && display && c.u32(c.u32(history)) == 0x10334550 && !c.u32(ui+4),
                "Startup fixture must be the actual initialized screen without a career");
        const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
            unsigned reg=3; for (auto arg:args) c.set(reg++,arg);
            s.budget=s.executed+100000000;
            try { c.call(entry,return_sentinel); }
            catch (const std::exception& error) {
                throw std::runtime_error("Startup entry="+hex_address(entry)+" pc="+hex_address(s.pc)+": "+error.what());
            }
            return c.r(3);
        };
        s.platform=std::make_shared<NativePlatform>(temp.path/"game",temp.path/"user",AudioMode::offline,EventMode::isolated);
        const auto scratch=allocate(s,32,true); world(s,scratch,scratch+16);
        require(s.platform->resize_gui_canvas(s,{801,601}) && s.platform->resize_gui_canvas(s,canvas),"Startup canvas allocation");
        if (rebuild) {
            invoke(0x10719560,{display,1,0}); invoke(0x10334550,{});
        }
        const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
        const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
        const auto rect=[&](unsigned at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
        const auto refresh=[&] {
            for (unsigned i=0;i<c.u16(display+0x129da4);++i) {
                invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
            }
        };
        refresh();
        int body=-1,footer=-1;
        for (unsigned i=0;i<c.u16(display+0x129da4);++i) {
            if (rect(grid(i))==ScreenRect{110,145,780,535}) body=int(i);
            if ((rect(grid(i))==ScreenRect{100,555,790,590} || rect(grid(i))==ScreenLayout{canvas}.footer()) &&
                c.u8(grid(i)+0xbb0)==2 && c.u8(grid(i)+0xbb1)==1) footer=int(i);
        }
        require(body>=0 && footer>=0,"Startup choice/navigation grids missing");
        std::vector<unsigned> choices, navigation;
        const auto meanings=[&] {
            std::vector<std::uint8_t> result;
            for (unsigned i=0;i<c.u16(display+0x129da6);++i)
                for (auto [offset,size]:{std::pair{8u,8u},{0x20u,8u},{0x38u,20u},{0x68u,0x114u}}) {
                    const auto part=s.memory.copy_out(control(i)+offset,size);
                    result.insert(result.end(),part.begin(),part.end());
                }
            return result;
        };
        for (unsigned i=0;i<c.u16(display+0x129da6);++i) {
            if (c.i16(control(i)+0x72)==body) choices.push_back(i);
            if (c.i16(control(i)+0x72)==footer) navigation.push_back(i);
        }
        require(choices.size()==9 && navigation.size()==2,"Startup fixture lost its nine choices or disabled footer");
        const auto before=meanings(), history_before=s.memory.copy_out(history,0x1fc), rng=random_state(s);
        const auto config=c.u32(c.toc(-0x7ac8));
        const auto config_before=s.memory.copy_out(config,0x130);
        const auto registers=s.registers;
        const auto unique=s.unique;
        const auto pc=s.pc, sp=c.sp();
        const auto stack=s.memory.copy_out(sp-0x8000,0x8100);
        require(adaptive_screen_available(s,0x10334550),"Startup gate incorrectly requires a loaded career");
        layout_built_screen(s,display,0x10334550);
        require(s.registers==registers && s.unique==unique && s.pc==pc &&
                s.memory.copy_out(sp-0x8000,0x8100)==stack,"Startup layout changed guest ABI/stack");
        require(meanings()==before,"Startup layout changed original text/styles/actions/owners");
        const auto panel=rect(grid(body));
        if (!canvas.original()) {
            require(panel.width()<=1120 && panel.height()<=360 && panel.left>=110 && panel.right<=canvas.width-20 &&
                panel.top>=145 && panel.bottom<=canvas.height-65 && c.u8(grid(body)+0xbb1)==5,
                "Startup choices did not use bounded five-row content");
            require(rect(grid(footer))==ScreenRect{panel.right-340,canvas.height-45,panel.right,canvas.height-10} &&
                c.u8(grid(footer)+0xbb2)==1 && c.u8(grid(footer)+0xbb3)==1,"Startup navigation is not compact/equal");
        } else require(panel==ScreenRect{110,145,780,535} && c.u8(grid(body)+0xbb1)==6,"Original startup geometry changed");
        const auto hit=[&](int x,int y) {
            c.put32(c.toc(-0x5c10),x); c.put32(c.toc(-0x5c14),y);
            c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
            return std::int16_t(invoke(c.u8(display)==1?0x1071aa10:0x1071a280,{display}));
        };
        for (unsigned i=0;i<choices.size();++i) {
            const auto at=control(choices[i]); const auto box=rect(at+0x10);
            require(box.left>=panel.left && box.right<=panel.right && box.top>=panel.top && box.bottom<=panel.bottom,
                "Startup control escaped its visible panel");
            if (!canvas.original()) {
                require(box.width()>=325 && box.height()>=50,"Startup choice is too small for original text");
                if (i==8) require(std::abs(box.left+box.right-panel.left-panel.right)<=1,"Odd startup choice is not centered");
                if (i%2==0 && i<8) require(rect(control(choices[i+1])+0x10).left-box.right>=9,"Startup columns have no gap");
                if (i>=2) require(box.top-rect(control(choices[i-2])+0x10).bottom>=7,"Startup rows have no gap");
            }
            const auto selected=hit((box.left+box.right)/2,(box.top+box.bottom)/2);
            require(selected==std::int16_t(choices[i]),"Original picker missed a resized startup choice");
            if (c.u32(at+8)) ++available_hits;
            else ++unavailable; // Original disabled choices retain action zero.
        }
        for (auto index:navigation) require(!c.u32(control(index)+8),"Fresh startup navigation became actionable");
        require(s.memory.copy_out(history,0x1fc)==history_before && s.memory.copy_out(config,0x130)==config_before &&
                random_state(s)==rng && !c.u32(ui+4),"Startup presentation/input changed config/history/RNG or created a career");
        if (rebuild && !canvas.original()) {
            bool outer=false,menu=false;
            for (unsigned i=0;i<c.u16(display+0x129da4);++i) {
                const auto box=rect(grid(i));
                outer |= box==ScreenRect{0,0,89,canvas.height-1};
                menu |= box==ScreenRect{5,10,85,canvas.height-10};
            }
            require(outer && menu,"Startup sidebar did not inherit adaptive chrome");
        }
        const auto gui=s.memory.copy_out(display,0x130000);
        for (unsigned variant=0;variant<4;++variant) {
            if (variant==0) c.put32(history+0x14,1);
            if (variant==1) c.put32(ui+0x2c64,1);
            if (variant==2) c.put16(ui+0x2c58,1);
            if (variant==3) select_new_screens(false);
            require(!adaptive_screen_available(s,0x10334550),"Startup accepted first-run/network/nested/classic context");
            layout_built_screen(s,display,0x10334550);
            require(s.memory.copy_out(display,0x130000)==gui,"Startup fallback changed the original controls");
            c.put32(history+0x14,0); c.put32(ui+0x2c64,0); c.put16(ui+0x2c58,0); select_new_screens(true);
        }
        if (!canvas.original()) {
            select_new_screens(false);
            require(s.platform->request_adaptive_screen(0x10334550),"Startup Off did not request the classic canvas");
            invoke(0x10719560,{display,1,0}); s.platform->apply_adaptive_screen(s,display);
            require(gui_canvas(s).original(),"Startup Off failed to restore the original canvas");
            invoke(0x10334550,{});
            layout_built_screen(s,display,0x10334550);
            require(rect(grid(body))==ScreenRect{110,145,780,535},"Startup Off failed to restore original menu bounds");
            select_new_screens(true);
        }
    }
    require(available_hits && unavailable,"Startup check did not cover enabled and unavailable choices");
    initialize_new_screens();
    std::cout<<"Adaptive startup original actions/text/styles, choice hits, disabled entries, chrome, fallback and ABI/RNG passed\n";
}

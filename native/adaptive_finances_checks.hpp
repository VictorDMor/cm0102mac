// Original club finance builders and commands in a disposable capture. The
// presentation pass must never book money or alter budget-query side effects.
void finances_builder(const char* path) {
    Temporary temp; Clock clock; clock.idle_input=true;
    auto state=std::make_unique<State>(); auto& s=*state;
    load_match_snapshot(s,path); s.code_fragment_observer=&clock;
    guest::Cpu c(s); c.set(2,c.u32(c.sp()+0x14));
    const auto manager=c.toc(-0x7bb0); c.put16(manager+0x2c58,0);
    const auto history=c.u32(manager+0x10),display=c.u32(manager+0x24),person=c.u32(manager+4);
    const auto setting=[history](unsigned slot){return history+0x14+slot*8;};
    const auto club=c.u32(setting(0));
    const auto invoke=[&](unsigned entry,std::initializer_list<unsigned> args) {
        unsigned reg=3; for(auto arg:args)c.set(reg++,arg);
        s.budget=s.executed+400000000; c.call(entry,return_sentinel); return c.r(3);
    };
    s.platform=std::make_shared<NativePlatform>(temp.path/"game",temp.path/"user",AudioMode::offline);
    const auto scratch=allocate(s,32,true); world(s,scratch,scratch+16);
    initialize_new_screens(temp.path/"Screens.plist"); select_new_screens(true);
    require(s.platform->resize_gui_canvas(s,{801,601}),"Finance pixel initialization");
    c.put32(setting(4),5); c.put32(setting(27),2);
    require(adaptive_screen_available(s,0x1001e2b0),"Own finance view did not pass gate");
    const auto humans=c.u32(c.toc(-0x7a54));
    const auto human=c.u32(humans)+(c.u32(person)-c.u32(c.toc(-0x7be4))+16)*0xa65c;
    for(unsigned field:{0x9a65u,0x9984u}) {
        const auto before=c.u8(human+field); c.put8(human+field,1);
        require(!invoke(0x1014c490,{club,0,1}) && !adaptive_screen_available(s,0x1001e2b0),"Absent human passed finance gate");
        c.put8(human+field,before);
    }
    for(unsigned view:{1u,16u}) {
        c.put32(setting(27),view); require(adaptive_screen_available(s,0x1001e2b0),"Public club information view did not pass gate");
    }
    c.put32(setting(27),2);
    c.put16(manager+0x2c58,1); require(!adaptive_screen_available(s,0x1001e2b0),"Secondary finances adapted"); c.put16(manager+0x2c58,0);
    c.put32(manager+0x2c64,1); require(!adaptive_screen_available(s,0x1001e2b0),"Network finances adapted"); c.put32(manager+0x2c64,0);
    const auto owner=c.u32(club+0x90); c.put32(club+0x90,0);
    require(!adaptive_screen_available(s,0x1001e2b0),"Unowned finances adapted"); c.put32(club+0x90,owner);
    const auto id=c.u32(club); c.put32(club,c.u32(c.toc(-0x7b50))-2*c.u32(c.toc(-0x7b04)));
    require(!adaptive_screen_available(s,0x1001e2b0),"National finances adapted"); c.put32(club,id);
    const auto finance=c.u32(c.toc(-0x7ab8));
    const auto account=invoke(0x10103510,{finance,club});
    const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
    const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
    const auto rect=[&](unsigned at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
    const auto meanings=[&] {
        std::vector<std::uint8_t> bytes;
        for(unsigned i=0;i<c.u16(display+0x129da6);++i)
            for(const auto [offset,size]:{std::pair{8u,8u},{0x20u,8u},{0x38u,20u},{0x68u,0x114u}}) {
                const auto part=s.memory.copy_out(control(i)+offset,size); bytes.insert(bytes.end(),part.begin(),part.end());
            }
        return bytes;
    };
    const auto records=[&] {
        auto bytes=s.memory.copy_out(account,0x168);
        for(const auto [at,size]:{std::pair{club,0x248u},{person,0x70u},{history,0x180u},{human,0xa65cu},
                                  {c.toc(-0x7b78),4u}}) {
            const auto part=s.memory.copy_out(at,size); bytes.insert(bytes.end(),part.begin(),part.end());
        }
        const auto rng=random_state(s); bytes.insert(bytes.end(),rng.begin(),rng.end()); return bytes;
    };
    const auto build=[&](GuiCanvas size,bool adaptive,bool overflow=false) {
        select_new_screens(adaptive);
        require(s.platform->resize_gui_canvas(s,size),"Finance resize failed");
        invoke(0x10719560,{display,1,0}); invoke(0x1001e2b0,{});
        int body=-1,backing=-1;
        for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
            if(rect(grid(i))==ScreenRect{110,190,770,500})body=int(i);
            if(rect(grid(i))==ScreenRect{110,190,780,500})backing=int(i);
        }
        require(body>=0 && backing>=0 && c.u8(grid(body)+0xbb1)==14,"Finance original shape/capacity changed");
        if(overflow) {
            c.put32(grid(body)+0x14,20); c.put32(grid(body)+0x1fc,2);
        }
        const auto semantic=meanings(),saved=records(),original=s.memory.copy_out(display,0x129da8);
        const auto registers=s.registers;
        const auto unique=s.unique;
        const auto pc=s.pc,stack_base=c.sp()-0x8000; const auto stack=s.memory.copy_out(stack_base,0x8100);
        layout_built_screen(s,display,0x1001e2b0);
        require(meanings()==semantic,"Finance layout changed labels, values, styles or actions");
        require(records()==saved,"Finance layout changed ledger, records, history, budget cache or RNG");
        require(s.registers==registers && s.unique==unique && s.pc==pc && s.memory.copy_out(stack_base,0x8100)==stack,
                "Finance layout clobbered guest ABI/stack");
        if(!adaptive || size.original())require(s.memory.copy_out(display,0x129da8)==original,"Classic finance layout changed");
        else {
            const auto panel=rect(grid(backing)),cells=rect(grid(body));
            require(panel.left>=110 && panel.width()<=1600 && panel.right<=size.width-20 && panel.top==190 &&
                    panel.bottom<=size.height-100 && panel.height()<=14*28,"Finance panel exceeds its bounds");
            require(cells==ScreenRect{panel.left,panel.top,panel.right-10,panel.bottom},"Finance layers lost original inset");
            require(c.u8(grid(body)+0xbb1)==14 && c.u8(grid(backing)+0xbb1)==14,"Finance capacity changed");
            if(overflow)require(c.u32(grid(body)+0x1fc)==2,"Finance layout lost scroll");
            bool divider=false,heading=false,view=false,lower_tabs=false;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i),box=at+0x10;
                if(c.i16(at+0x72)==body && c.u32(at+0x38)==0xc400) {
                    require(c.i32(at+0x28)==panel.left+15 && c.i32(at+0x2c)==cells.right-5-(overflow?22:0),"Finance separator lost inset/gutter");
                    divider=true;
                }
                if(c.i16(at+0x72)<0 && c.i32(box+4)==150)heading=rect(box)==ScreenRect{panel.left,150,panel.right,185};
                if(c.i16(at+0x72)<0 && c.i32(box+4)==125)view=rect(box)==ScreenRect{panel.left,125,panel.left+125,145};
            }
            for(unsigned i=0;i<c.u16(display+0x129da4);++i)
                if(rect(grid(i))==ScreenRect{100,size.height-90,size.width-10,size.height-55})lower_tabs=true;
            require(divider && heading && view && lower_tabs,"Finance shell or separator did not adapt");
        }
        for(unsigned i=0;i<c.u16(display+0x129da4);++i) {
            invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
        }
        return meanings();
    };
    for(unsigned view:{2u,4u,8u}) {
        c.put32(setting(27),view);
        const auto baseline=build({800,600},true);
        const auto ledger=s.memory.copy_out(account,0x168),rng=random_state(s);
        for(auto size:{GuiCanvas{801,601},GuiCanvas{800,900},GuiCanvas{1280,900},GuiCanvas{1440,600},GuiCanvas{3200,1800}}) {
            require(build(size,true)==baseline,"Finance resize changed original values or controls");
            require(s.memory.copy_out(account,0x168)==ledger && random_state(s)==rng,"Finance resize changed money or RNG");
        }
        require(build({1280,900},false)==baseline,"Disabled finance controls changed");
        build({800,900},true,true);
    }
    // The original View popup and handler; no Print or financial action runs.
    const auto hit=[&](unsigned index) {
        const auto box=rect(control(index)+0x10);
        c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
        c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
        // Open popups use DISPLAY's nested-area pointer pass.
        return std::int16_t(invoke(c.u8(display)==1?0x1071aa10:0x1071a280,{display}));
    };
    for(bool adaptive:{false,true}) {
        c.put32(setting(27),0x102);
        build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600},adaptive);
        const auto ledger=s.memory.copy_out(account,0x168),rng=random_state(s);
        for(unsigned view:{4u,8u,2u}) {
            unsigned parent=0,choice=0;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
                const auto at=control(i);
                if(c.i16(at+0x72)<0 && c.i32(at+0x14)==125 && c.i16(at+0x74)>=0)parent=i;
                if(c.u32(at+8)==32 && c.u32(at+0x48)==view)choice=i;
            }
            require(parent && choice,"Finance menu missing");
            require(hit(parent)==-1 && c.u8(display)==1 && c.i16(display+0x12a930)==c.i16(control(parent)+0x74),"Finance View popup did not open");
            require(hit(choice)==int(choice),"Finance menu choice missed its bounds");
            require(std::int32_t(invoke(0x10036f10,{choice}))==-11 && c.u32(setting(27))==(0x100|view),"Finance command changed view flags incorrectly");
            build(adaptive?GuiCanvas{1280,900}:GuiCanvas{800,600},adaptive);
            require(s.memory.copy_out(account,0x168)==ledger && random_state(s)==rng,"Finance menu changed money or RNG");
        }
    }
    initialize_new_screens();
    std::cout<<"Finance original values/commands, ledger/RNG, gate exclusions and post-layout ABI passed\n";
}

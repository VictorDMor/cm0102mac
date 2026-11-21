// Controlled catalog and save-header responses only; no save is opened/loaded.
struct LoadBrowserFiles {
    unsigned count=0, next=0, opened=0;
    bool (*previous)(State&)=nullptr;
    static inline LoadBrowserFiles* active=nullptr;
    static bool probe(State& s) {
        auto& files=*active; guest::Cpu c(s);
        if (s.pc != 0x10355820 && (c.lr()<0x100ee220 || c.lr()>=0x100ef7d0))
            return files.previous && files.previous(s);
        const auto emit=[&](unsigned at) {
            if (files.next==files.count) return 1u;
            const auto name="Fixture_"+std::to_string(files.next++)+".sav";
            for (unsigned i=0;i<=name.size();++i) c.put8(at+i,i<name.size()?name[i]:0);
            return 0u;
        };
        switch (s.pc) {
        case 0x10739690: files.next=0; c.set(3,emit(c.r(4))); break;
        case 0x107397d0: c.set(3,emit(c.r(3))); break;
        case 0x107156a0: c.set(3,0); break;
        case 0x10355820: c.set(3,files.next==1?5:files.next==2?9:1); break;
        case 0x10739d60: c.set(3,1); break;
        case 0x107475b0: ++files.opened; c.set(3,0x12345678); break;
        case 0x10747a80:
            require(c.r(4)==4 && c.r(5)==1,"Load-browser fixture attempted to read career data");
            c.put32(c.r(3),0x3e027); c.set(3,1); break;
        case 0x10747900: c.put32(c.r(3),0); c.set(3,0); break;
        default: return files.previous && files.previous(s);
        }
        s.pc=c.lr(); return true;
    }
    explicit LoadBrowserFiles(unsigned total):count(total),previous(dispatch_probe) { active=this; dispatch_probe=probe; }
    ~LoadBrowserFiles() { dispatch_probe=previous; active=nullptr; }
};

void load_browser_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path/"Screens.plist"); select_new_screens(true);
    // Load with Info (the workflow every later check follows), then Save and Delete with and without it:
    // those are checked for the gate, the geometry, the picker and Cancel.
    for (auto [mode,info]:{std::pair{0u,1u},{1u,1u},{1u,0u},{2u,0u}})
    for (unsigned files_count:{0u,3u,12u,40u})
    for (auto canvas:{GuiCanvas{800,600},GuiCanvas{801,601},GuiCanvas{800,900},
                      GuiCanvas{1440,600},GuiCanvas{1280,900},GuiCanvas{3200,1800}}) {
        Clock clock; clock.idle_input=true;
        auto state=std::make_unique<State>(); auto& s=*state;
        load_match_snapshot(s,path); s.code_fragment_observer=&clock;
        guest::Cpu c(s); c.set(2,c.u32(c.sp()+0x14));
        const auto ui=c.toc(-0x7bb0); c.put16(ui+0x2c58,0);
        const auto display=c.u32(ui+0x24);
        require(display && !c.u32(ui+4),"Load-browser fixture must start before a career is loaded");
        s.platform=std::make_shared<NativePlatform>(temp.path/"game",temp.path/"user",AudioMode::offline,EventMode::isolated);
        const auto scratch=allocate(s,2048,true);
        world(s,scratch,scratch+16);
        require(s.platform->resize_gui_canvas(s,{801,601}) && s.platform->resize_gui_canvas(s,canvas),"Load-browser canvas allocation");
        const auto text=[&](unsigned at,std::string_view value) {
            for (unsigned i=0;i<=value.size();++i)c.put8(at+i,i<value.size()?value[i]:0);
            return at;
        };
        const auto string_at=[&](unsigned at) {
            std::string value;
            for (unsigned i=0;i<256 && c.u8(at+i);++i)value+=char(c.u8(at+i));
            return value;
        };
        const auto invoke=[&](unsigned entry,std::initializer_list<unsigned> args) {
            unsigned reg=3; for(auto arg:args)c.set(reg++,arg);
            s.budget=s.executed+100000000;
            try { c.call(entry,return_sentinel); }
            catch(const std::exception& error) {
                throw std::runtime_error("Load browser files="+std::to_string(files_count)+" entry="+hex_address(entry)+
                    " pc="+hex_address(s.pc)+": "+error.what());
            }
            return c.r(3);
        };
        const auto filter=c.toc(-0x63c4);
        c.put32(c.sp()+0x38,c.u16(c.toc(-0x7bc8))); c.put32(c.sp()+0x3c,5);
        for (unsigned offset:{0x40u,0x44u,0x48u,0x4cu})c.put32(c.sp()+offset,offset==0x4c?info:0);
        invoke(0x100edf20,{text(scratch+64,"Restore Saved Game"),text(scratch+128,"Select File"),mode,
            text(scratch+256,"fixture-saves"),text(scratch+512,".sav"),0,filter,c.u16(c.toc(-0x7bc4))});
        const auto history=c.u32(ui+0x10);
        const auto setting=[history](unsigned index){return history+0x14+index*8;};
        require(c.u32(c.u32(history))==0x100ee220 && c.u32(filter)==0x10355820,"Load opener did not publish the expected browser");
        const auto name=c.u32(setting(7)), result_path=c.u32(setting(8));
        const auto grid=[display](unsigned i){return display+4+i*0xbf0;};
        const auto control=[display](unsigned i){return display+0xba864+i*0x17c;};
        const auto rect=[&](unsigned at){return ScreenRect{c.i32(at),c.i32(at+4),c.i32(at+8),c.i32(at+12)};};
        const auto refresh=[&] {
            for (unsigned i=0;i<c.u16(display+0x129da4);++i) {
                invoke(0x10714630,{grid(i)}); invoke(0x10714110,{grid(i),0});
            }
        };
        LoadBrowserFiles files(files_count);
        c.put32(setting(14),files_count>6?files_count-2:0);
        const auto build=[&] {
            invoke(0x10719560,{display,1,0}); invoke(0x100ee220,{}); refresh();
        };
        build();
        const unsigned list=c.u32(setting(15));
        int field=-1,footer=-1,editor=-1;
        std::vector<unsigned> entries;
        for (unsigned i=0;i<c.u16(display+0x129da4);++i) {
            if (rect(grid(i))==ScreenRect{190,195,700,231})field=int(i);
            if (rect(grid(i))==ScreenRect{100,555,790,590} && c.u8(grid(i)+0xbb0)==2+info)footer=int(i);
        }
        const auto action=[&](unsigned code) {
            for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                if(c.u32(control(i)+8)==code)return i;
            throw std::runtime_error("Missing browser action "+std::to_string(code));
        };
        for(unsigned i=0;i<c.u16(display+0x129da6);++i) {
            if(c.i16(control(i)+0x72)==int(list) && c.u32(control(i)+8)==2)entries.push_back(i);
            if(c.u32(control(i)+12)==0x18)editor=int(i);
        }
        require(field>=0 && footer>=0 && editor>=0 && entries.size()==files_count,"Browser original controls/catalog mismatch");
        const auto meanings=[&] {
            std::vector<std::uint8_t> value;
            for(unsigned i=0;i<c.u16(display+0x129da6);++i)
                for(auto [offset,size]:{std::pair{8u,8u},{0x20u,8u},{0x38u,20u},{0x68u,0x114u}}) {
                    const auto part=s.memory.copy_out(control(i)+offset,size);
                    value.insert(value.end(),part.begin(),part.end());
                }
            return value;
        };
        const auto before=meanings(), history_before=s.memory.copy_out(history,0x1fc), rng=random_state(s);
        const auto name_before=s.memory.copy_out(name,31), path_before=s.memory.copy_out(result_path,256);
        const auto registers=s.registers;
        const auto unique=s.unique;
        const auto pc=s.pc,sp=c.sp();
        const auto stack=s.memory.copy_out(sp-0x8000,0x8100);
        require(adaptive_screen_available(s,0x100ee220),"Saved-career load gate rejected original arguments");
        layout_built_screen(s,display,0x100ee220);
        require(s.registers==registers && s.unique==unique && s.pc==pc && s.memory.copy_out(sp-0x8000,0x8100)==stack,
                "Load layout changed guest ABI/stack");
        require(meanings()==before && s.memory.copy_out(history,0x1fc)==history_before &&
            s.memory.copy_out(name,31)==name_before && s.memory.copy_out(result_path,256)==path_before && random_state(s)==rng,
            "Load layout changed commands/styles/text/history/filename/path/RNG");
        const auto list_box=rect(grid(list)),field_box=rect(grid(field));
        const auto capacity=c.u8(grid(list)+0xbb1);
        require(capacity>=6 && capacity<=30 && c.i32(grid(list)+0x1fc)<=int(std::max(files_count,6u)-std::min<unsigned>(capacity,std::max(files_count,6u))),
                "Load list scroll/capacity is invalid");
        require(bool(c.u8(grid(list)+0xbef))==(std::max(files_count,6u)>capacity),"Load list retained a stale scrollbar");
        if(!canvas.original()) {
            require(list_box.width()<=1120 && list_box.left>=110 && list_box.right<=canvas.width-20 && list_box.bottom<=canvas.height-65 &&
                field_box.left==list_box.left && field_box.right==list_box.right && field_box.height()==36 &&
                list_box.top-field_box.bottom==8 && capacity>6,"Adaptive load list/filename bounds are not aligned");
            require(rect(grid(footer))==ScreenRect{list_box.right-170*int(2+info),canvas.height-45,list_box.right,canvas.height-10},
                "Load footer did not follow the panel");
            for(unsigned col=0;col<2+info;++col)require(c.u8(grid(footer)+0xbb2+col)==1,"Load footer buttons are unequal");
        }
        const auto hit=[&](unsigned index) {
            const auto box=rect(control(index)+0x10);
            c.put32(c.toc(-0x5c10),(box.left+box.right)/2); c.put32(c.toc(-0x5c14),(box.top+box.bottom)/2);
            c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
            c.put8(c.toc(-0x74b0),0); c.put32(c.toc(-0x5c1c),0);
            return std::int16_t(invoke(c.u8(display)==1?0x1071aa10:0x1071a280,{display}));
        };
        for(unsigned code:{3u,5u,4u}) {
            if(code==5 && !info)continue;
            const auto index=action(code);
            require(hit(index)==std::int16_t(index),"Original picker missed a load footer command");
        }
        const auto gui=s.memory.copy_out(display,0x130000);
        for(auto [slot,replacement]:{std::pair{2u,3u},{13u,1u},{17u,1u},{18u,2u},
                                     {3u,0u},{4u,0u},{5u,0u},{6u,0u},{7u,0u},{8u,0u}}) {
            const auto saved=c.u32(setting(slot)); c.put32(setting(slot),replacement);
            require(!adaptive_screen_available(s,0x100ee220),"Load layout accepted another chooser mode or incomplete buffers");
            layout_built_screen(s,display,0x100ee220);
            require(s.memory.copy_out(display,0x130000)==gui,"Excluded chooser changed geometry");
            c.put32(setting(slot),saved);
        }
        const auto extension=c.u32(setting(4));
        c.put8(extension+1,'q'); require(!adaptive_screen_available(s,0x100ee220),"Load accepted a different file extension"); c.put8(extension+1,'s');
        for(unsigned variant=0;variant<3;++variant) {
            if(variant==0)c.put32(ui+0x2c64,1);
            if(variant==1)c.put16(ui+0x2c58,1);
            if(variant==2)select_new_screens(false);
            require(!adaptive_screen_available(s,0x100ee220),"Load layout accepted network/secondary/Off context");
            layout_built_screen(s,display,0x100ee220);
            require(s.memory.copy_out(display,0x130000)==gui,"Load fallback changed controls");
            c.put32(ui+0x2c64,0); c.put16(ui+0x2c58,0); select_new_screens(true);
        }
        const bool load=!mode && info;
        if(load && !files_count && canvas==GuiCanvas{1280,900}) {
            hit(unsigned(editor));
            require(c.i32(control(editor)+0x44)>=0,"Resized filename field did not receive focus");
            c.put8(c.toc(-0x7bac),0);
            for(unsigned i=0;i<40;++i) {
                c.put8(c.toc(-0x74b0),'x'); c.put32(c.toc(-0x5c1c),7);
                invoke(0x1071b520,{display,unsigned(editor)});
            }
            c.put8(c.toc(-0x74b0),13); c.put32(c.toc(-0x5c1c),0x24);
            invoke(0x1071b520,{display,unsigned(editor)});
            require(string_at(name)==std::string(30,'x'),"Resized filename field lost its original buffer/length limit");
            select_new_screens(false);
            require(s.platform->request_adaptive_screen(0x100ee220),"Load Off did not request classic canvas");
            invoke(0x10719560,{display,1,0}); s.platform->apply_adaptive_screen(s,display);
            invoke(0x100ee220,{}); layout_built_screen(s,display,0x100ee220); refresh();
            require(gui_canvas(s).original() && rect(grid(list))==ScreenRect{190,234,700,510} &&
                c.u8(grid(list)+0xbb1)==6 && string_at(name)==std::string(30,'x'),
                "Load Off failed to restore classic layout while retaining filename");
            select_new_screens(true);
        }
        if(load && files_count) {
            const auto first=unsigned(c.i32(grid(list)+0x1fc));
            const auto chosen=entries.at(first);
            const auto label=string_at(control(chosen)+0x78);
            require(hit(chosen)==std::int16_t(chosen) && std::int32_t(invoke(0x100ee9d0,{chosen}))==-11,
                    "Original file selection did not request its ordinary redraw");
            require(string_at(name)==label.substr(0,30) && !c.u32(ui+4) && files.opened==0,
                    "Selecting a save changed its name or prematurely opened a career");
            const auto retained=c.u32(setting(14)); build(); layout_built_screen(s,display,0x100ee220);
            require(c.u32(setting(14))==retained && unsigned(c.i32(grid(list)+0x1fc))==retained && string_at(name)==label.substr(0,30),
                    "Load redraw lost selection/scroll");
        }
        if(load && files_count==12) {
            const auto about=action(5); require(hit(about)==std::int16_t(about),"Info hit failed after selection");
            require(std::int32_t(invoke(0x100ee9d0,{about}))==-4 && files.opened==1 &&
                c.u32(c.u32(c.u32(ui+0x10)))==0x100ef840 && !adaptive_screen_available(s,0x100ee220),
                    "Info did not enter its original separate screen");
        } else if(load && files_count==40) {
            const auto selected=string_at(name); const auto ok=action(4);
            require(hit(ok)==std::int16_t(ok) && std::int32_t(invoke(0x100ee9d0,{ok}))==-9 &&
                c.u32(ui+0x2581a0)==5 && c.u32(ui+0x2581a4)==result_path && string_at(result_path).ends_with(selected+".sav") && files.opened==0,
                "OK did not return only the selected path to the original loader");
        } else {
            const auto cancel=action(3);
            require(hit(cancel)==std::int16_t(cancel),"Cancel picker failed");
            const auto result=std::int32_t(invoke(0x100ee9d0,{cancel}));
            require(result==-9 && c.u32(ui+0x2581a0)==5 && !c.u32(ui+0x2581a4) && files.opened==0,
                ("Cancel contract: result="+std::to_string(result)+" parent="+std::to_string(c.u32(ui+0x2581a0))+
                " answer="+std::to_string(c.u32(ui+0x2581a4))+" opens="+std::to_string(files.opened)).c_str());
        }
        require(!c.u32(ui+4) && random_state(s)==rng,"Load dialog actions started a career or changed RNG");
    }
    initialize_new_screens();
    std::cout<<"Adaptive saved-game list/scroll/selection, original Info/OK/Cancel, fallback and ABI/RNG passed\n";
}

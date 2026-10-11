#include "cpp_object_storage_internal.hpp"
#include "cpp_object_storage_adapter.hpp"
#include "semantic_io_checks.hpp"
#include "application_exit.hpp"
using namespace cm::semantic_io_checks;
namespace {
constexpr auto entries=cpp_object_storage::entries;
constexpr std::uint32_t objects=fixture+0x2000, context=fixture+0x4000,
    vector=fixture+0x8000, handlers=fixture+0x5000, enabled=fixture+0x6000;
unsigned cases=0,faults=0,terminals=0,negatives=0;
std::array<std::uint64_t,12> witnesses{};
void args(State& s,unsigned i,std::initializer_list<std::uint32_t> values) {
    unsigned n=3;for(auto value:values)s.wr<4>(n++*4,value);
    s.pc=entries[i];s.wr<4>(4128,return_sentinel);s.budget=100000;
}
std::uint32_t allocate(State& s,std::uint32_t size) {
    s.wr<4>(12,size);s.pc=0x10775130;s.wr<4>(4128,return_sentinel);s.budget=100000;run(s);
    const auto p=std::uint32_t(s.r<4>(12));require(p,"Fixture guest allocation");return p;
}
std::unique_ptr<State> setup(const char* program,unsigned i) {
    auto s=std::make_unique<State>();load_program(*s,program);s->memory.map(fixture,0x10000);s->memory.clear(fixture,0x10000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);
    std::uint32_t raw=objects;if(i==4||i==6||i==8)raw=allocate(*s,128);
    s->memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));
    s->registers.fill(0x5b);s->unique.fill(0x6d);for(unsigned n=0;n<32;++n)s->wr<4>(n*4,0xcafe0000+n);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(2440,0xbadc0ffe);s->wr<4>(4132,0x31415926);
    s->wr<1>(2304,0xa6);s->wr<1>(1024,i%4);s->wr<1>(1027,(i/2)%2);
    s->memory.write<4>(vector,0x10773700);s->memory.write<4>(vector+4,toc);
    s->memory.write<4>(toc-0x5aac,handlers);s->memory.write<4>(handlers,0);
    s->memory.write<4>(toc-0x5aa0,enabled);s->memory.write<1>(enabled,0);
    s->memory.write<4>(context,objects);s->memory.write<4>(context+4,4);
    s->memory.write<4>(context+8,8);s->memory.write<4>(context+12,vector);s->memory.write<4>(context+16,3);
    if(i==0)args(*s,i,{objects,vector,vector,4,3});
    else if(i==1)args(*s,i,{context,0});
    else if(i==2)args(*s,i,{objects,vector,vector,4,3});
    else if(i==3)args(*s,i,{objects,vector,4,3});
    else if(i==4){s->memory.write<4>(raw,4);s->memory.write<4>(raw+4,3);args(*s,i,{raw+16,vector});}
    else if(i==5||i==7)args(*s,i,{20});
    else if(i==6||i==8)args(*s,i,{raw});
    else if(i==10)args(*s,i,{objects,0});
    else args(*s,i,{});
    return s;
}
struct Imports:CppObjectStorageBoundaryObserver {
    struct Record {std::string name;decltype(State::registers) registers;std::uint32_t pc,heap_end;std::uint64_t calls;
        std::int16_t error;decltype(State::allocated) allocated,available;
        std::vector<std::pair<std::uint32_t,std::uint64_t>> mappings;bool operator==(const Record&)const=default;};
    std::vector<Record> records,callback_records;
    Record capture(const State& s, const char* name) const {
        return {name,s.registers,s.pc,s.heap_end,s.native_calls,s.memory_error,s.allocated,s.available,s.memory.mapped_ranges()};
    }
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State&,std::uint32_t,bool,bool)override{}
    void import_enter(const State&s,const char*n)override{records.push_back(capture(s,n));}
    void import_leave(const State&s,const char*n)override{import_enter(s,n);}
};
using Expect = std::function<void(const State&, const Effects&, const Imports&)>;
std::uint32_t peek(const State& s, std::uint32_t address) {
    const auto bytes = s.memory.copy_out(address, 4);
    std::uint32_t value = 0; for (auto byte : bytes) value = (value << 8) | byte; return value;
}
void check(std::unique_ptr<State> original,unsigned i,const std::string& label,
    const std::function<void(State&,Effects&,std::uint32_t,unsigned)>& mutation={},bool fault=false,bool terminal=false,
    const Expect& expected = {}, const std::array<bool, 12>& selected_original = {}) {
    auto modern=clone(*original);original->cpp_object_storage_original=true;modern->cpp_object_storage_entry_original=selected_original;
    Effects ea,eb;ea.state=original.get();eb.state=modern.get();ea.on_read=eb.on_read=mutation;
    Imports ia,ib;original->cpp_object_storage_observer=&ia;modern->cpp_object_storage_observer=&ib;
    original->memory.date_observer=&ea;modern->memory.date_observer=&eb;std::string ax,bx;bool at=false,bt=false;
    ea.on_read = [&ia, mutation](State& s, Effects& e, std::uint32_t address, unsigned size) {
        if (s.pc == 0x10781320) ia.callback_records.push_back(ia.capture(s, "CFM callback"));
        if (mutation) mutation(s, e, address, size);
    };
    eb.on_read = [&ib, mutation](State& s, Effects& e, std::uint32_t address, unsigned size) {
        if (s.pc == 0x10781320) ib.callback_records.push_back(ib.capture(s, "CFM callback"));
        if (mutation) mutation(s, e, address, size);
    };
    try{run(*original);}catch(const ApplicationExit&){at=true;}catch(const std::exception&e){ax=e.what();}
    try{run(*modern);}catch(const ApplicationExit&){bt=true;}catch(const std::exception&e){bx=e.what();}
    original->memory.date_observer=modern->memory.date_observer=nullptr;original->cpp_object_storage_observer=modern->cpp_object_storage_observer=nullptr;
    require(ax==bx&&at==bt,label+" outcome original="+ax+" modern="+bx);
    require(fault==!ax.empty()&&terminal==at,label+" expected outcome "+ax);
    if (fault) require(ax.starts_with("Unmapped data access at "), label+" mapping fault");
    Image(*original).compare(*modern,label);require(ea.bytes==eb.bytes,label+" ordered effects");require(ea.callbacks==eb.callbacks&&ia.records==ib.records&&ia.callback_records==ib.callback_records,label+" callback/import ABI and heap state");
    require(!modern->cpp_object_storage_dispatch_escapes,label+" dispatch escape");
    for(unsigned n=0;n<12;++n){
        if(selected_original[n]) require(modern->cpp_object_storage_original_calls[n]&&!modern->cpp_object_storage_calls[n],label+" mixed original witness");
        else require(!modern->cpp_object_storage_original_calls[n],label+" unexpected original call");
        if(!fault)witnesses[n]+=modern->cpp_object_storage_calls[n];
    }
    if(!fault&&!selected_original[i])require(modern->cpp_object_storage_calls[i],label+" root production witness");
    if (expected) expected(*modern, eb, ib);
    ++cases;faults+=fault;terminals+=terminal;
}
struct Negative : Imports {
    struct Boundary { Image image; std::vector<std::uint8_t> effects; std::vector<Record> imports; bool terminal; };
    std::uint32_t target;
    std::vector<Boundary> reference;
    Effects* effects = nullptr;
    bool mutated = false;
    unsigned parent_boundaries = 0;
    explicit Negative(std::uint32_t entry) : target(entry) {}
    void leave(State& s, std::uint32_t entry, bool original, bool terminal) override {
        if (entry != target) { if (mutated) ++parent_boundaries; return; }
        if (original) { reference.push_back({Image(s), effects->bytes, records, terminal}); return; }
        require(!reference.empty(), "Missing object-storage negative boundary");
        const auto& expected = reference.front(); expected.image.compare(s, "Unmutated object-storage boundary");
        require(expected.effects == effects->bytes && expected.imports == records && expected.terminal == terminal,
            "Object-storage boundary effect/import/termination prefix");
        s.wr<4>(12, s.r<4>(12) ^ 1); mutated = true;
        expected.image.compare(s, "Own object-storage boundary " + hex_address(entry));
    }
};
void negative(std::unique_ptr<State> original, std::uint32_t target) {
    auto modern = clone(*original); original->cpp_object_storage_original = true;
    Effects ea, eb; ea.state = original.get(); eb.state = modern.get(); Negative observer(target); observer.effects = &ea;
    original->cpp_object_storage_observer = &observer; original->memory.date_observer = &ea;
    try { run(*original); } catch (const ApplicationExit&) {}
    require(!observer.reference.empty(), "Unreached object-storage negative"); observer.records.clear(); observer.effects = &eb;
    modern->cpp_object_storage_observer = &observer; modern->memory.date_observer = &eb; bool rejected = false; std::string error;
    try { run(*modern); } catch (const std::runtime_error& e) {
        error = e.what(); rejected = error.starts_with("Own object-storage boundary " + hex_address(target));
    }
    original->cpp_object_storage_observer = modern->cpp_object_storage_observer = nullptr;
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    require(rejected && observer.mutated && !observer.parent_boundaries, "Own/child object-storage mutation escaped: " + error); ++negatives;
}
std::unique_ptr<State> sparse(const State& initial, std::uint64_t omitted = UINT64_MAX) {
    auto probe = clone(initial); probe->cpp_object_storage_original = true; Effects effects; effects.state = probe.get(); Imports imports;
    probe->memory.date_observer = &effects; probe->cpp_object_storage_observer = &imports;
    try { run(*probe); } catch (const ApplicationExit&) {}
    probe->memory.date_observer = nullptr; probe->cpp_object_storage_observer = nullptr;
    auto s = std::make_unique<State>(); s->registers = initial.registers; s->unique = initial.unique;
    s->pc = initial.pc; s->budget = initial.budget; s->heap_end = initial.heap_end;
    s->allocated = initial.allocated; s->available = initial.available; s->memory_error = initial.memory_error;
    for (auto [begin, length] : effects.accesses) for (auto [address, size] : initial.memory.mapped_ranges()) {
        const auto low = std::max(std::uint64_t(begin), std::uint64_t(address));
        const auto high = std::min(std::uint64_t(begin) + length, std::uint64_t(address) + size);
        if (low >= high) continue;
        if (low <= omitted && omitted < high) {
            if (low < omitted) s->memory.map(std::uint32_t(low), omitted - low);
            if (omitted + 1 < high) s->memory.map(std::uint32_t(omitted + 1), high - omitted - 1);
        } else s->memory.map(std::uint32_t(low), high - low);
    }
    for (auto [address, size] : s->memory.mapped_ranges()) s->memory.copy_in(address, initial.memory.copy_out(address, size));
    return s;
}
void composition(const char* program) {
    auto original = setup(program, 7); args(*original, 7, {28}); auto modern = clone(*original);
    original->cpp_object_storage_original = true; Imports ia, ib; Effects ea, eb; ea.state = original.get(); eb.state = modern.get();
    original->cpp_object_storage_observer = &ia; modern->cpp_object_storage_observer = &ib;
    original->memory.date_observer = &ea; modern->memory.date_observer = &eb;
    std::uint32_t raw = 0;
    for (unsigned stage = 0; stage < 5; ++stage) {
        if (stage == 1) {
            original->memory.copy_in(raw, std::vector<std::uint8_t>(28, 0x6b));
            modern->memory.copy_in(raw, std::vector<std::uint8_t>(28, 0x6b));
            args(*original, 0, {raw, vector, vector, 4, 3}); args(*modern, 0, {raw, vector, vector, 4, 3});
        } else if (stage == 2) { args(*original, 4, {raw + 16, vector}); args(*modern, 4, {raw + 16, vector}); }
        else if (stage == 3) { args(*original, 7, {28}); args(*modern, 7, {28}); }
        else if (stage == 4) { args(*original, 8, {raw}); args(*modern, 8, {raw}); }
        run(*original); run(*modern); Image(*original).compare(*modern, "Production object-storage composition " + std::to_string(stage));
        require(ea.bytes == eb.bytes && ea.callbacks == eb.callbacks && ia.records == ib.records, "Cumulative object-storage composition effects/import ABI");
        if (stage == 0) { raw = std::uint32_t(modern->r<4>(12)); require(raw && !(raw & 3) && modern->allocated.size() == 1, "Composition real guest storage"); }
        if (stage == 1) {
            require(modern->r<4>(12) == raw + 16 && peek(*modern, raw) == 4 && peek(*modern, raw + 4) == 3, "Composition cookie and payload pointer");
            require(modern->memory.copy_out(raw + 8, 20) == std::vector<std::uint8_t>(20, 0x6b), "Composition preserves padding and payload");
        }
        if (stage == 2) {
            require(modern->allocated.empty() && ea.callbacks.size() == 6, "Composition reverse destruction and native storage release");
            for (unsigned n = 0; n < 3; ++n) {
                require(eb.callbacks[n][0] == raw + 16 + n * 4 && eb.callbacks[n][1] == 1, "Composition ascending constructors");
                require(eb.callbacks[n + 3][0] == raw + 24 - n * 4 && eb.callbacks[n + 3][1] == UINT32_MAX, "Composition descending destructors");
            }
        }
        if (stage == 3) require(modern->r<4>(12) == raw, "Composition reuses released guest address");
        if (stage == 4) require(modern->allocated.empty() && ib.records.size() == 8, "Composition closes both native allocation lifetimes");
        ++cases;
    }
    original->cpp_object_storage_observer = modern->cpp_object_storage_observer = nullptr;
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    require(!modern->cpp_object_storage_dispatch_escapes, "Composition dispatch escape");
    for (unsigned n = 0; n < 12; ++n) { require(!modern->cpp_object_storage_original_calls[n], "Composition original selection"); witnesses[n] += modern->cpp_object_storage_calls[n]; }
}
}
int main(int argc,char**argv){if(argc!=2)return 2;try{
    for(unsigned i=0;i<12;++i)check(setup(argv[1],i),i,"Direct production "+std::to_string(i));
    for (unsigned i = 0; i < 12; ++i) {
        auto s = setup(argv[1], i); s->memory.write<4>(vector + 16, entries[i]); s->memory.write<4>(vector + 20, toc);
        s->wr<4>(48, vector + 16); s->pc = 0x10781320;
        check(std::move(s), i, "Generated CFM object storage " + std::to_string(i));
        negative(setup(argv[1], i), entries[i]);
        auto original = setup(argv[1], i); original->cpp_object_storage_original = true; run(*original);
        require(original->cpp_object_storage_original_calls[i] && !original->cpp_object_storage_calls[i], "Original no-observer selection");
    }
    for (auto [parent, child] : {std::pair{7u, 5u}, {8u, 6u}, {4u, 8u}})
        negative(setup(argv[1], parent), entries[child]);
    for (unsigned parent : {0u, 2u}) {
        auto s = setup(argv[1], parent); s->memory.write<4>(vector, entries[11]);
        negative(std::move(s), entries[11]);
    }
    for (unsigned parent : {1u, 10u}) {
        auto s = setup(argv[1], parent); const auto p = allocate(*s, 32);
        if (parent == 1) s->memory.copy_in(p, s->memory.copy_out(context, 20));
        args(*s, parent, {p, 1}); negative(std::move(s), entries[6]);
    }
    auto failed_new = setup(argv[1], 5); failed_new->wr<4>(12, UINT32_MAX); negative(std::move(failed_new), entries[9]);
    for(unsigned i:{0u,1u,4u,6u,8u,10u}){auto s=setup(argv[1],i);s->wr<4>(12,0);check(std::move(s),i,"Null production "+std::to_string(i));}
    for(unsigned i:{0u,2u,3u})for(unsigned count:{0u,1u,2u,8u}){auto s=setup(argv[1],i);s->wr<4>((i==3?6:7)*4,count);check(std::move(s),i,"Array count "+std::to_string(i)+"/"+std::to_string(count));}
    for(unsigned i:{0u,2u,3u})for(unsigned stride:{0u,1u,16u,0xfffffffcu}){auto s=setup(argv[1],i);s->wr<4>((i==3?5:6)*4,stride);check(std::move(s),i,"Word32 array stride "+std::to_string(i));}
    for(unsigned i:{5u,7u})for(unsigned size:{0u,1u,76u,77u,0xffffffb8u,UINT32_MAX}){auto s=setup(argv[1],i);s->wr<4>(12,size);check(std::move(s),i,"Allocation size "+std::to_string(size));}
    for(unsigned count:{0u,3u,8u,9u})for(std::uint32_t dtor:{0u,vector}){auto s=setup(argv[1],1);s->memory.write<4>(context+16,count);s->memory.write<4>(context+12,dtor);check(std::move(s),1,"Partial cleanup count and destructor");}
    for(std::uint32_t flag:{0u,0xffffu,0x8000u,0x10000u}){auto s=setup(argv[1],10);s->wr<4>(16,flag);check(std::move(s),10,"Failure deleting flag");}
    for(unsigned i:{0u,4u}){auto s=setup(argv[1],i);s->wr<4>(16,0);check(std::move(s),i,"Null array callback");}
    auto retry=setup(argv[1],5);retry->wr<4>(12,UINT32_MAX);retry->memory.write<4>(handlers,vector);
    check(std::move(retry),5,"Allocation handler reread",[](State&s,Effects&e,std::uint32_t address,unsigned){if(s.pc==0x10781320&&address==vector&&!e.mutated){e.mutated=true;s.memory.write<4>(handlers,0);}},false,false,
        [](const State& s,const Effects& e,const Imports&){require(e.mutated&&e.callbacks.size()==1&&!s.r<4>(12),"Failed allocation retries after one live handler then returns disabled failure");});
    auto handler_cell = setup(argv[1], 5); handler_cell->wr<4>(12, UINT32_MAX); handler_cell->memory.write<4>(handlers, vector);
    check(std::move(handler_cell), 5, "New-handler cell captured once, callback reread each retry",
        [](State& s, Effects& e, std::uint32_t address, unsigned) {
            if (s.pc != 0x10781320 || address != vector) return;
            if (e.callbacks.size() == 1) { e.mutated = true; s.memory.write<4>(toc - 0x5aac, handlers + 4); }
            if (e.callbacks.size() == 2) s.memory.write<4>(handlers, 0);
        }, false, false, [](const State& s, const Effects& e, const Imports&) {
            require(e.mutated && e.callbacks.size() == 2 && !s.r<4>(12) && peek(s, toc - 0x5aac) == handlers + 4,
                "Captured handler owner survives TOC slot replacement");
        });
    auto enabled_failure=setup(argv[1],9);enabled_failure->memory.write<1>(enabled,1);
    enabled_failure->memory.write<4>(toc-0x3524,0);enabled_failure->memory.write<4>(toc-0x3528,0);
    check(std::move(enabled_failure),9,"Enabled allocation failure",{},false,true);
    auto terminal_negative = setup(argv[1], 9); terminal_negative->memory.write<1>(enabled, 1);
    terminal_negative->memory.write<4>(toc - 0x3524, 0); terminal_negative->memory.write<4>(toc - 0x3528, 0);
    negative(std::move(terminal_negative), entries[9]);
    auto terminal_parent = setup(argv[1], 5); terminal_parent->wr<4>(12, UINT32_MAX); terminal_parent->memory.write<1>(enabled, 1);
    terminal_parent->memory.write<4>(toc - 0x3524, 0); terminal_parent->memory.write<4>(toc - 0x3528, 0);
    negative(clone(*terminal_parent), entries[9]);
    check(std::move(terminal_parent), 5, "Allocation failure native termination", {}, false, true);
    for (auto [parent, child] : {std::pair{7u, 5u}, {8u, 6u}, {4u, 8u}}) {
        std::array<bool, 12> selected{}; selected[child] = true;
        check(setup(argv[1], parent), parent, "Mixed original object-storage child", {}, false, false, {}, selected);
        selected.fill(false); selected[parent] = true;
        check(setup(argv[1], parent), parent, "Mixed original object-storage parent", {}, false, false, {}, selected);
    }
    for (unsigned i : {0u, 2u}) {
        check(setup(argv[1], i), i, "Callback mutation triggers partial construction cleanup",
            [i](State& s, Effects& e, std::uint32_t address, unsigned) {
                if (s.pc == 0x10781320 && address == vector && !e.mutated) {
                    e.mutated = true; s.memory.write<4>(stack - (i == 0 ? 112 : 96) + 64, 4);
                }
            }, false, false, [i](const State&, const Effects& e, const Imports&) {
                const auto first = objects + (i == 0 ? 16 : 0);
                require(e.mutated && e.callbacks.size() == 6, "Partial construction callback guard is reread");
                for (unsigned n = 0; n < 3; ++n) require(e.callbacks[n][0] == first + n * 4 && e.callbacks[n][1] == 1 &&
                    e.callbacks[n + 3][0] == first + 8 - n * 4 && e.callbacks[n + 3][1] == UINT32_MAX, "Partial construction reverse cleanup order");
            });
    }
    check(setup(argv[1], 1), 1, "Partial cleanup rereads live stride",
        [](State& s, Effects& e, std::uint32_t address, unsigned) {
            if (s.pc == 0x10781320 && address == vector && !e.mutated) { e.mutated = true; s.memory.write<4>(context + 4, 8); }
        }, false, false, [](const State& s, const Effects& e, const Imports&) {
            require(e.mutated && e.callbacks.size() == 3 && !peek(s, context + 16), "Live partial cleanup reaches zero count");
            require(e.callbacks[0][0] == objects + 8 && e.callbacks[1][0] == objects && e.callbacks[2][0] == objects - 8, "Live stride affects each subsequent destructor");
        });
    for (unsigned i : {1u, 10u}) for (unsigned flag : {1u, 0x7fffu, 0x10001u}) {
        auto s = setup(argv[1], i); const auto p = allocate(*s, 32);
        if (i == 1) s->memory.copy_in(p, s->memory.copy_out(context, 20));
        args(*s, i, {p, flag});
        check(std::move(s), i, "Positive signed-short deleting flag", {}, false, false,
            [p](const State& s, const Effects&, const Imports&) { require(s.r<4>(12) == p && s.allocated.empty(), "Deleting cleanup preserves result and releases native arena"); });
    }
    for (unsigned i : {0u, 1u, 2u, 3u, 4u}) {
        check(setup(argv[1], i), i, "Independent array callback order", {}, false, false,
            [i](const State& s, const Effects& e, const Imports&) {
                const bool constructing = i == 0 || i == 2; const auto first = objects + (i == 0 ? 16 : 0);
                require(e.callbacks.size() == 3, "Array callback count");
                if (i == 4) return; // Its live payload is a separately allocated guest address.
                for (unsigned n = 0; n < 3; ++n) require(e.callbacks[n][0] == first + (constructing ? n * 4 : (2 - n) * 4) &&
                    e.callbacks[n][1] == (constructing ? 1u : UINT32_MAX), "Original array callback direction and flag");
                if (i == 0) require(peek(s, objects) == 4 && peek(s, objects + 4) == 3 && s.r<4>(12) == objects + 16, "Cookie fields and payload result");
            });
    }
    for (unsigned i : {0u, 2u}) {
        auto s = setup(argv[1], i); s->wr<4>(12, stack - (i == 0 ? 112 : 96) + 64);
        check(std::move(s), i, "Array payload aliases cleanup frame");
    }
    for (unsigned i = 0; i < 12; ++i) {
        auto s = setup(argv[1], i); const auto relocated = fixture + 0x40000; s->memory.map(relocated - 0x9000, 0xb000);
        const auto ranges = s->memory.mapped_ranges();
        for (auto [address, size] : ranges) {
            const auto low = std::max(std::uint64_t(address), std::uint64_t(toc - 0x9000));
            const auto high = std::min(std::uint64_t(address) + size, std::uint64_t(toc + 0x2000));
            if (low < high) s->memory.copy_in(std::uint32_t(relocated + low - toc), s->memory.copy_out(std::uint32_t(low), high - low));
        }
        s->wr<4>(8, relocated); check(std::move(s), i, "Relocated object-storage TOC " + std::to_string(i));
    }
    composition(argv[1]);
    for (unsigned i = 0; i < 12; ++i) {
        auto s = setup(argv[1], i); check(sparse(*s), i, "Minimal object-storage mapping " + std::to_string(i));
        const auto omitted = i == 0 ? stack - 20 : i == 4 ? stack - 24 : i == 7 || i == 8 ? stack + 8 :
            i == 9 ? toc - 0x5aa0 : i == 11 ? toc - 0x3b84 : stack - 4;
        check(sparse(*s, omitted), i, "Object-storage mapping fault " + std::to_string(i), {}, true);
    }
    for (auto [i, address] : {std::pair{0u, objects}, {0u, objects + 4}, {0u, stack - 112 + 72},
            {0u, vector}, {0u, vector + 4}, {1u, context}, {1u, context + 4}, {1u, context + 8},
            {1u, context + 12}, {1u, context + 16}, {2u, stack - 96 + 56}, {2u, stack - 96 + 72},
            {3u, vector}, {3u, vector + 4}, {5u, toc - 0x5aac}, {9u, enabled},
            {10u, toc - 0x5aa4}, {10u, objects}, {10u, toc - 0x5ab4}}) {
        auto s = setup(argv[1], i); check(sparse(*s, address), i, "Object-storage ordered fault prefix " + hex_address(address), {}, true);
    }
    for (std::uint32_t address : {toc - 0x5aa4, toc - 0x5ab4, toc - 0x3b80, toc - 0x5aa8, stack - 64 + 56}) {
        auto s = setup(argv[1], 9); s->memory.write<1>(enabled, 1);
        s->memory.write<4>(toc - 0x3524, 0); s->memory.write<4>(toc - 0x3528, 0);
        check(sparse(*s, address), 9, "Enabled failure publication fault prefix " + hex_address(address), {}, true);
    }
    for (unsigned i = 0; i < 12; ++i) {
        auto s = setup(argv[1], i); s->cpp_object_storage_original = true; s->cpp_object_storage_entry_original.fill(true);
        s->cpp_object_storage_calls.fill(9); s->cpp_object_storage_original_calls.fill(9); s->cpp_object_storage_returns.fill(9);
        s->cpp_object_storage_terminations.fill(9); s->cpp_object_storage_dispatch_escapes = 9;
        s->cpp_object_storage_original_resume = entries[i]; Imports observer; s->cpp_object_storage_observer = &observer;
        Image before(*s); reset_cpp_object_storage_diagnostics(*s); before.compare(*s, "Object-storage reset architectural isolation");
        require(!s->cpp_object_storage_original && !s->cpp_object_storage_observer && !s->cpp_object_storage_original_resume &&
            !s->cpp_object_storage_dispatch_escapes, "Reset object-storage scalar diagnostics");
        for (unsigned n = 0; n < 12; ++n) require(!s->cpp_object_storage_entry_original[n] && !s->cpp_object_storage_calls[n] &&
            !s->cpp_object_storage_original_calls[n] && !s->cpp_object_storage_returns[n] && !s->cpp_object_storage_terminations[n], "Reset object-storage entry diagnostics");
    }
    for(unsigned i=0;i<12;++i)std::cout<<"cpp_object_storage.entry="<<hex_address(entries[i])<<" modern="<<witnesses[i]<<'\n';
    std::cout<<cases<<" object-storage production comparisons; "<<faults<<" faults; "<<terminals<<" native terminations; "<<negatives<<" own/child negatives\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

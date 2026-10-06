#include "integer_parsing_adapter.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
namespace {
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void set(State& s, unsigned n, std::uint32_t value) { s.wr<4>(n * 4, value); }
std::int32_t signed_word(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
void compare(State& s, std::int64_t a, std::int64_t b = 0, bool record = false) {
    s.wr<1>(2304, (record ? s.r<1>(2304) & 0xf0 : 0) |
        (a < b ? 8 : a > b ? 4 : 2) | (s.r<1>(1024) & 1));
}
void record(State& s, std::uint32_t value) { compare(s, signed_word(value), 0, true); }
template<unsigned N> std::uint32_t read(State& s, std::uint32_t address, std::uint32_t pc) {
    s.pc = pc; return std::uint32_t(s.memory.read<N>(address));
}
void write(State& s, std::uint32_t address, std::uint32_t value, std::uint32_t pc) {
    s.pc = pc; s.memory.write<4>(address, value);
}
void call(State& s, std::uint32_t entry, std::uint32_t link) {
    s.wr<4>(2440, reg(s, 2)); s.wr<4>(4128, link); s.pc = entry; run_until(s, link);
}
void frame(State& s, unsigned size, std::uint32_t pc) {
    const auto previous = reg(s, 1); write(s, previous - size, previous, pc); set(s, 1, previous - size);
}
void finish(State& s) { s.pc = std::uint32_t(s.r<4>(4128)); }
// stmw/lmw expose a register-space cursor as well as ordered guest accesses.
// Advance it only after a successful word, including after the final word.
void nonvolatile_words(State& s, unsigned first, std::uint32_t pc, bool restore) {
    s.wr<4>(2432, reg(s, 1) - (32 - first) * 4);
    for (unsigned n = first; n < 32; ++n) {
        const auto address = std::uint32_t(s.r<4>(2432));
        if (restore) set(s, n, read<4>(s, address, pc));
        else write(s, address, reg(s, n), pc);
        s.wr<4>(2432, s.r<4>(2432) + 4);
    }
}
std::uint32_t sign_extension(State& s, std::uint32_t value) {
    s.wr<1>(1027, bool(value & 0x80000000u) && bool(value & 0x7fffffffu));
    return value & 0x80000000u ? UINT32_MAX : 0;
}
std::uint32_t difference(State& s, std::uint32_t minuend, std::uint32_t subtrahend, bool extend = false) {
    const auto borrow = unsigned(extend && !s.r<1>(1027));
    s.wr<1>(1027, std::uint64_t(minuend) >= std::uint64_t(subtrahend) + borrow);
    return minuend - subtrahend - borrow;
}

// The two originals share phases, but use different nonvolatile ABI slots.
// Keep those slots live throughout callbacks: dependencies may clobber them,
// and outputs or tables may alias the saved frame.
template<bool Wide> class MagnitudeParser {
    State& s;
    static constexpr unsigned base=Wide?31:29, limit=Wide?18:22, descriptor=Wide?28:30,
        context=Wide?27:28, consumed=Wide?14:18, sign=Wide?17:21, overflow=Wide?22:24,
        classification=Wide?15:19, mapping=Wide?16:20, phase=Wide?30:25,
        count=Wide?29:31, spaces=Wide?19:23, low=Wide?26:27, threshold_low=Wide?24:26,
        character=Wide?20:4;
    static constexpr std::uint32_t pc(std::uint32_t narrow, std::uint32_t wide) { return Wide ? wide : narrow; }
    std::uint32_t get(unsigned n) const { return reg(s, n); }
    void put(unsigned n, std::uint32_t value) { set(s, n, value); }
    void signed_compare(unsigned n, std::int32_t value) { compare(s, signed_word(get(n)), value); }
    void fetch(std::uint32_t link) {
        // Always use the generated descriptor stub, including its mutable TOC,
        // CTR and frame spill; no char* specialization or host callback.
        call(s, 0x10781320, link);
        put(2, read<4>(s, get(1) + 20, link)); put(character, get(3));
    }
    void read_arguments() { put(3, get(context)); put(12, get(descriptor)); put(4, 0); put(5, 0); }
    void threshold(std::uint32_t link) {
        if constexpr (Wide) {
            put(5, sign_extension(s, get(base))); put(6, get(base)); put(3, UINT32_MAX); put(4, UINT32_MAX);
            call(s, 0x10773870, link); put(threshold_low, get(4)); put(25, get(3));
        } else { put(0, UINT32_MAX); put(threshold_low, UINT32_MAX / get(base)); }
    }
    void initialize() {
        nonvolatile_words(s, Wide?14:18, pc(0x1077ee20,0x1077f180), false);
        put(0, s.r<4>(4128)); put(base, get(3)); record(s, get(base));
        put(mapping, read<4>(s, get(2)-0x62bc, pc(0x1077ee2c,0x1077f18c)));
        put(classification, read<4>(s, get(2)-0x5a48, pc(0x1077ee30,0x1077f190)));
        put(overflow,get(9)); put(sign,get(8)); put(limit,get(4)); put(descriptor,get(5));
        put(context,get(6)); put(consumed,get(7)); put(phase,1); put(count,0); put(spaces,0);
        put(low,0); if constexpr(Wide) put(23,0);
        put(threshold_low,0); if constexpr(Wide) put(25,0);
        write(s,get(1)+8,get(0),pc(0x1077ee60,0x1077f1c8)); put(0,0);
        frame(s,Wide?128:112,pc(0x1077ee68,0x1077f1d0));
        write(s,get(9),get(0),pc(0x1077ee6c,0x1077f1d4));
        write(s,get(8),get(0),pc(0x1077ee70,0x1077f1d8));
        bool invalid = (s.r<1>(2304)&8)!=0;
        if(!invalid) { signed_compare(base,1); invalid=get(base)==1; }
        if(!invalid) { signed_compare(base,36); invalid=signed_word(get(base))>36; }
        if(!invalid) { signed_compare(limit,1); invalid=signed_word(get(limit))<1; }
        if(invalid) put(phase,64);
        else { read_arguments(); put(count,1); fetch(pc(0x1077eeb0,0x1077f218)); }
        signed_compare(base,0);
        if(get(base)) threshold(0x1077f23c);
    }
    void whitespace_and_sign() {
        put(0,get(character)&255);
        put(0,read<1>(s,get(classification)+get(0),pc(0x1077eef0,0x1077f268)));
        put(0,get(0)&6); record(s,get(0));
        if(get(0)) {
            read_arguments(); fetch(pc(0x1077ef10,0x1077f288)); put(spaces,get(spaces)+1); return;
        }
        signed_compare(character,'+');
        if(get(character)=='+') {
            read_arguments(); put(count,get(count)+1); fetch(pc(0x1077ef40,0x1077f2b8));
        } else {
            signed_compare(character,'-');
            if(get(character)=='-') {
                read_arguments(); put(count,get(count)+1);
                call(s,0x10781320,pc(0x1077ef6c,0x1077f2e4));
                put(2,read<4>(s,get(1)+20,pc(0x1077ef6c,0x1077f2e4)));
                put(0,1); write(s,get(sign),get(0),pc(0x1077ef74,0x1077f2ec)); put(character,get(3));
            }
        }
        put(phase,2);
    }
    void leading_zero() {
        signed_compare(base,0);
        if(get(base)) signed_compare(base,16);
        if(!get(base)||get(base)==16) {
            signed_compare(character,'0');
            if(get(character)=='0') {
                put(3,get(context)); put(12,get(descriptor)); put(phase,4); put(4,0); put(5,0);
                put(count,get(count)+1); fetch(pc(0x1077efb8,0x1077f330)); return;
            }
        }
        put(phase,8);
    }
    void hexadecimal_prefix() {
        signed_compare(character,'X');
        if(get(character)!='X') signed_compare(character,'x');
        if(get(character)=='X'||get(character)=='x') {
            put(3,get(context)); put(12,get(descriptor)); put(base,16); put(phase,8); put(4,0); put(5,0);
            put(count,get(count)+1); fetch(pc(0x1077effc,0x1077f374));
        } else { signed_compare(base,0); if(!get(base)) put(base,8); put(phase,16); }
    }
    bool digit_value() {
        signed_compare(base,0); if(!get(base)) put(base,10);
        if constexpr(Wide) {
            put(3,0); put(0,sign_extension(s,get(3))); put(3,get(threshold_low)); put(0,get(25));
            put(0,get(3)|get(0)); record(s,get(0)); if(!get(0)) threshold(0x1077f3cc);
        } else { compare(s,get(threshold_low)); if(!get(threshold_low)) threshold(0); }
        put(3,get(character)&255);
        constexpr unsigned category=Wide?4:5;
        put(category,read<1>(s,get(classification)+get(3),pc(0x1077f03c,0x1077f3d8)));
        put(0,get(category)&16); record(s,get(0));
        if(get(0)) {
            put(character,get(character)-'0'); compare(s,signed_word(get(character)),signed_word(get(base)));
            if(signed_word(get(character))<signed_word(get(base))) return true;
            signed_compare(phase,16); put(0,get(phase)==16?32:64); put(phase,get(0)); put(character,get(character)+'0'); return false;
        }
        put(0,get(category)&192); record(s,get(0));
        if(get(0)) {
            signed_compare(character,-1);
            put(category,get(character)==UINT32_MAX?UINT32_MAX:
                read<1>(s,get(mapping)+get(3),pc(0x1077f088,0x1077f424)));
            put(0,get(category)-55); compare(s,signed_word(get(0)),signed_word(get(base)));
            if(signed_word(get(0))<signed_word(get(base))) {
                signed_compare(character,-1);
                put(3,get(character)==UINT32_MAX?UINT32_MAX:
                    read<1>(s,get(mapping)+get(3),pc(0x1077f0c0,0x1077f45c)));
                put(character,get(3)-55); return true;
            }
        }
        signed_compare(phase,16); put(phase,get(phase)==16?32:64); return false;
    }
    void accumulate() {
        if constexpr(Wide) {
            // Compare the live word pair with UINT64_MAX/base. The borrow is
            // recorded in r0/CR before any overflow-output store can fault.
            put(0,difference(s,get(threshold_low),get(low)));
            put(0,difference(s,get(25),get(23),true));
            put(0,difference(s,get(21),get(21),true)); put(0,0-get(0)); record(s,get(0));
            if(get(0)) { put(0,1); write(s,get(overflow),get(0),0x1077f47c); }
            put(0,std::uint32_t((std::uint64_t(get(low))*get(base))>>32));
            put(4,get(23)*get(base)); put(5,get(low)*get(base));
            put(6,sign_extension(s,get(character))); put(3,UINT32_MAX); put(4,get(0)+get(4));
            // Remaining headroom is compared to the sign-extended live digit;
            // classification/mapping mutations may produce negative digits.
            put(0,difference(s,get(3),get(5))); put(3,difference(s,get(3),get(4),true));
            put(0,difference(s,get(0),get(character))); put(0,difference(s,get(3),get(6),true));
            put(0,difference(s,get(21),get(21),true)); put(0,0-get(0)); record(s,get(0));
            if(get(0)) { put(0,1); write(s,get(overflow),get(0),0x1077f4b8); }
            const auto sum=std::uint64_t(get(5))+get(character); s.wr<1>(1027,sum>>32); put(3,std::uint32_t(sum));
            const auto high_sum=std::uint64_t(get(4))+get(6)+s.r<1>(1027);
            s.wr<1>(1027,high_sum>>32); put(0,std::uint32_t(high_sum)); put(low,get(3)); put(23,get(0));
        } else {
            compare(s,get(low),get(threshold_low));
            if(get(low)>get(threshold_low)) { put(0,1); write(s,get(overflow),get(0),0x1077f0d4); }
            put(low,get(low)*get(base)); put(0,~get(low)); s.wr<1>(1027,1); compare(s,get(character),get(0));
            if(get(character)>get(0)) { put(0,1); write(s,get(overflow),get(0),0x1077f0ec); }
            put(low,get(low)+get(character));
        }
        put(3,get(context)); put(12,get(descriptor)); put(phase,16); put(4,0); put(5,0);
        put(count,get(count)+1); fetch(pc(0x1077f110,0x1077f4e8));
    }
    bool continue_field() {
        compare(s,signed_word(get(count)),signed_word(get(limit)));
        if(signed_word(get(count))>signed_word(get(limit))) return false;
        signed_compare(character,-1); if(get(character)==UINT32_MAX) return false;
        put(0,get(phase)&96); record(s,get(0)); return !get(0);
    }
    void terminate() {
        put(0,get(phase)&52); record(s,get(0));
        if(!get(0)) {
            put(0,0); write(s,get(consumed),get(0),pc(0x1077f13c,0x1077f514));
            if constexpr(Wide) put(23,sign_extension(s,get(0)));
            put(low,0);
        } else {
            put(3,get(count)+get(spaces)); put(0,get(3)-1);
            write(s,get(consumed),get(0),pc(0x1077f150,0x1077f52c));
        }
        put(3,get(context)); if constexpr(Wide) put(4,get(character)); put(12,get(descriptor)); put(5,1);
        call(s,0x10781320,pc(0x1077f164,0x1077f544));
        put(2,read<4>(s,get(1)+20,pc(0x1077f164,0x1077f544)));
        if constexpr(Wide) {
            put(0,read<4>(s,get(1)+136,0x1077f548)); put(4,get(low)); put(3,get(23));
        } else { put(3,get(low)); put(0,read<4>(s,get(1)+120,0x1077f16c)); }
        put(1,get(1)+(Wide?128:112)); s.wr<4>(4128,get(0));
        nonvolatile_words(s,Wide?14:18,pc(0x1077f178,0x1077f55c),true); finish(s);
    }
public:
    explicit MagnitudeParser(State& state):s(state){}
    void parse() {
        const auto caller=std::uint32_t(s.r<4>(4128)); initialize();
        while(continue_field()) {
            s.step(pc(0x1077eed0,0x1077f248)); compare(s,get(phase),16);
            if(get(phase)>16) continue;
            put(3,read<4>(s,get(2)-(Wide?0x3b28:0x3b24),pc(0x1077eed8,0x1077f250)));
            put(0,get(phase)<<2);
            put(3,read<4>(s,get(3)+get(0),pc(0x1077eee0,0x1077f258))); s.wr<4>(4132,get(3));
            // Destinations, rather than just phase numbers, select behavior.
            // Mutated tables can redirect to any characterized phase.
            const auto target=get(3);
            if(target==pc(0x1077eeec,0x1077f264)) whitespace_and_sign();
            else if(target==pc(0x1077ef84,0x1077f2fc)) leading_zero();
            else if(target==pc(0x1077efcc,0x1077f344)) hexadecimal_prefix();
            else if(target==pc(0x1077f01c,0x1077f394)) { if(digit_value()) accumulate(); }
            else if(target!=pc(0x1077f118,0x1077f4f0)) {
                // Outside the identified phase contract, retain the existing
                // generated dynamic-transfer behavior (including unknown-PC
                // failure). Count this escape separately from normal coverage.
                ++s.integer_parsing_phase_escapes[Wide?1:0]; s.pc=target; run_until(s,caller); return;
            }
        }
        terminate();
    }
};
void signed_conversion(State& s) {
    write(s,reg(s,1)-4,reg(s,31),0x1077f570); set(s,0,s.r<4>(4128)); set(s,6,0x80000000u);
    write(s,reg(s,1)-8,reg(s,30),0x1077f57c); set(s,30,reg(s,3)); set(s,3,reg(s,5));
    set(s,5,read<4>(s,reg(s,2)-0x5a44,0x1077f588)); write(s,reg(s,1)+8,reg(s,0),0x1077f58c);
    set(s,31,reg(s,4)); frame(s,96,0x1077f594); set(s,0,0); set(s,4,reg(s,6)-1);
    write(s,reg(s,1)+68,reg(s,30),0x1077f5a0); set(s,6,reg(s,1)+68); set(s,7,reg(s,1)+64);
    write(s,reg(s,1)+72,reg(s,0),0x1077f5ac); set(s,8,reg(s,1)+60); set(s,9,reg(s,1)+56);
    call(s,0x1077ee20,0x1077f5bc); compare(s,reg(s,31));
    if(reg(s,31)) {
        set(s,0,read<4>(s,reg(s,1)+64,0x1077f5c4)); set(s,0,reg(s,30)+reg(s,0));
        write(s,reg(s,31),reg(s,0),0x1077f5cc);
    }
    set(s,0,read<4>(s,reg(s,1)+56,0x1077f5d0)); compare(s,signed_word(reg(s,0)));
    bool range=reg(s,0)!=0;
    if(!range) {
        set(s,5,read<4>(s,reg(s,1)+60,0x1077f5dc)); compare(s,signed_word(reg(s,5)));
        if(!reg(s,5)) {
            set(s,4,0x80000000u); set(s,0,reg(s,4)-1); compare(s,reg(s,3),reg(s,0)); range=reg(s,3)>reg(s,0);
        }
        if(!range) {
            compare(s,signed_word(reg(s,5)));
            if(reg(s,5)) { set(s,4,0x80000000u); set(s,0,reg(s,4)); compare(s,reg(s,3),reg(s,0)); range=reg(s,3)>reg(s,0); }
        }
    }
    if(range) {
        set(s,6,read<4>(s,reg(s,1)+60,0x1077f610)); set(s,4,0x80000000u);
        set(s,3,read<4>(s,reg(s,2)-0x5a58,0x1077f618)); set(s,0,34);
        set(s,5,0-reg(s,6)); set(s,5,reg(s,5)|reg(s,6)); write(s,reg(s,3),reg(s,0),0x1077f628);
        set(s,3,reg(s,5)>>31); set(s,0,reg(s,4)-1); set(s,0,reg(s,3)+reg(s,0)); set(s,3,reg(s,0));
    } else { compare(s,signed_word(reg(s,5))); if(reg(s,5)) set(s,3,0-reg(s,3)); }
    set(s,0,read<4>(s,reg(s,1)+104,0x1077f64c)); set(s,1,reg(s,1)+96);
    set(s,31,read<4>(s,reg(s,1)-4,0x1077f654)); s.wr<4>(4128,reg(s,0));
    set(s,30,read<4>(s,reg(s,1)-8,0x1077f65c)); finish(s);
}
void decimal_conversion(State& s, std::uint32_t entry) {
    set(s,0,s.r<4>(4128)); set(s,4,0); write(s,reg(s,1)+8,reg(s,0),entry+8); set(s,5,10);
    frame(s,64,entry+16); call(s,0x1077f570,entry+24);
    set(s,0,read<4>(s,reg(s,1)+72,entry+24)); set(s,1,reg(s,1)+64); s.wr<4>(4128,reg(s,0)); finish(s);
}
}
bool try_modern_integer_parsing(State& s, std::uint32_t entry) {
    s.pc=entry;
    if(s.integer_parsing_original_resume==entry) { s.integer_parsing_original_resume=0; return false; }
    const auto found=std::find(integer_parsing_entries.begin(),integer_parsing_entries.end(),entry);
    if(found==integer_parsing_entries.end()) throw std::runtime_error("Unknown integer parsing entry");
    const auto index=std::size_t(found-integer_parsing_entries.begin());
    const bool original=s.integer_parsing_original||s.integer_parsing_entry_original[index];
    ++(original?s.integer_parsing_original_calls[index]:s.integer_parsing_calls[index]);
    if(original&&!s.integer_parsing_observer) return false;
    const auto caller=std::uint32_t(s.r<4>(4128));
    if(s.integer_parsing_observer) s.integer_parsing_observer->enter(s,entry,original);
    if(original) {
        s.integer_parsing_original_resume=entry;
        try { run_until(s,caller); } catch(...) { s.integer_parsing_original_resume=0; throw; }
    } else {
        s.step(entry);
        switch(index) {
        case 0: MagnitudeParser<false>(s).parse(); break;
        case 1: MagnitudeParser<true>(s).parse(); break;
        case 2: signed_conversion(s); break;
        default: decimal_conversion(s,entry); break;
        }
    }
    if(s.integer_parsing_observer) s.integer_parsing_observer->leave(s,entry,original);
    return true;
}
void reset_integer_parsing_diagnostics(State& s) {
    s.integer_parsing_original=false; s.integer_parsing_entry_original.fill(false);
    s.integer_parsing_calls.fill(0); s.integer_parsing_original_calls.fill(0);
    s.integer_parsing_phase_escapes.fill(0); s.integer_parsing_original_resume=0; s.integer_parsing_observer=nullptr;
}
std::string integer_parsing_call_summary(const State& s) {
    std::ostringstream out;
    for(unsigned i=0;i<integer_parsing_entries.size();++i)
        out<<"integer.entry="<<hex_address(integer_parsing_entries[i])<<" mode="
            <<(s.integer_parsing_original||s.integer_parsing_entry_original[i]?"original":"modern")
            <<" modern="<<s.integer_parsing_calls[i]<<" original="<<s.integer_parsing_original_calls[i]<<'\n';
    out<<"integer.phase_escapes32="<<s.integer_parsing_phase_escapes[0]
        <<" phase_escapes64="<<s.integer_parsing_phase_escapes[1]<<'\n'; return out.str();
}
}

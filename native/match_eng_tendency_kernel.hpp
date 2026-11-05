#pragma once
// Native numeric interior of the side-tendency chain. Keep the legacy stack
// image and publish exact register state at the audited external boundaries.
#include "match_eng_shots.hpp"
#include <array>
#include <cstring>
#include "match_eng_adapter.hpp"
#include "match_pl_adapter.hpp"
#include "routine_check.hpp"
#include "native_drawing.hpp"
#include "season_reference.hpp"
#include "match_turbo.hpp"
#include <type_traits>

namespace cm::aot::match_eng::tendency_detail {
inline bool native_allowed(const State& s) {
    const auto& memory = s.memory;
    const bool passive = !memory.observer ||
        dynamic_cast<const NativeDrawingObserver*>(memory.observer) ||
        passive_season_replay_observer(memory.observer);
    // These children cannot install a new state-inspecting observer. The only
    // modal call is the assertion, after which this routine immediately returns.
    // Re-audit this boundary assumption when adding a child callback/import.
    return passive && !memory.date_observer && !memory.read_trace && s.budget == UINT64_MAX &&
        !dispatch_probe && !routine_checker && !match_eng_call_observer && !match_pc_profile_enabled();
}

// A call-free numeric kernel owns its status in native values. Legacy
// callees see the exact status at every boundary. No relaxed FP arithmetic.
class Arithmetic {
    guest::Cpu& c;
    std::array<std::uint8_t, 32> flags{};
    bool dirty = false;
    bool conversion_summary = false;
    bool summaries_pending = false;
    void set(unsigned bit, unsigned value) { flags[bit] = value & 1; }
    void classify(double value) {
        set(16, value < 0); set(17, value > 0);
        set(18, value == 0); set(19, std::isnan(value));
    }
    void sticky_summary() {
        set(0, flags[0] | ((flags[3] | flags[4] | flags[5] | flags[6]) & 1));
        dirty = true;
        summaries_pending = true;
        conversion_summary = false;
    }
    double status(double value, bool inexact, std::initializer_list<double> inputs) {
        classify(value);
        if (inexact) set(6, flags[6] | flags[14]);
        unsigned nan = flags[7];
        for (double input : inputs) nan |= std::isnan(input);
        set(7, nan);
        sticky_summary();
        return value;
    }
public:
    explicit Arithmetic(guest::Cpu& cpu) : c(cpu) { reload(); }
    ~Arithmetic() { publish(); }
    void reload() {
        std::memcpy(flags.data(), c.s.registers.data() + 1280, flags.size());
        dirty = false;
        summaries_pending = false;
    }
    void publish() {
        if (!dirty) return;
        if (summaries_pending) {
        unsigned invalid = 0;
        for (unsigned bit : {7u,8u,9u,10u,11u,12u,21u,22u,23u}) invalid |= conversion_summary ? (flags[bit] != 0) : (flags[bit] & 1);
        set(2, invalid != 0);
        unsigned enabled = 0;
        for (unsigned bit = 2; bit <= 6; ++bit) {
            if (conversion_summary) enabled |= flags[bit] && flags[bit+22];
            else enabled ^= flags[bit] & flags[bit+22] & 1;
        }
        set(1, enabled);
        summaries_pending = false;
        }
        std::memcpy(c.s.registers.data() + 1280, flags.data(), flags.size());
        dirty = false;
    }
    double fadd(double a,double b) { return status(a+b,true,{a,b}); }
    double fsub(double a,double b) { return status(a-b,false,{a,b}); }
    double fmul(double a,double b) { return status(a*b,true,{a,b}); }
    double fadds(double a,double b) { return status(double(float(a+b)),true,{a,b}); }
    double fsubs(double a,double b) { return status(double(float(a-b)),false,{a,b}); }
    double fmuls(double a,double b) { return status(double(float(a*b)),true,{a,b}); }
    double frsp(double a) { return status(double(float(a)),true,{a}); }
    double fdiv(double a,double b) {
        const double value=a/b;
        classify(value);
        set(5,flags[5] | (b==0)); set(6,flags[6] | flags[14]);
        set(7,flags[7] | std::isnan(a) | std::isnan(b));
        set(10,flags[10] | (a==0 && b==0));
        sticky_summary();
        return value;
    }
    double fdivs(double a,double b) {
        const double value=double(float(a/b));
        classify(value);
        set(5,flags[5] | (b==0)); set(6,flags[6] | flags[14]);
        set(7,flags[7] | std::isnan(a) | std::isnan(b));
        set(10,flags[10] | (a==0 && b==0));
        sticky_summary();
        return value;
    }
    void fcmp(unsigned field,double a,double b) {
        const bool less=a<b,greater=b<a,equal=a==b;
        set(16,less);set(17,greater);set(18,equal);
        c.set_cr(field,std::uint8_t(less<<3 | greater<<2 | equal<<1 | (std::isnan(a)||std::isnan(b))));
        dirty=true;
    }
    double multiply_add(std::uint32_t site,double a,double c0,double b) {
        return status(original_multiply_add(fused_site_kind(site),std::bit_cast<std::uint64_t>(a),
                      std::bit_cast<std::uint64_t>(c0),std::bit_cast<std::uint64_t>(b)),true,{a,b,c0});
    }
    std::uint64_t fctiwz(unsigned n,double value) {
        const auto input_bits=std::bit_cast<std::uint64_t>(value);
        if ((input_bits & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000)) {
            publish();
            const auto bits=guest::Fpu(c).fctiwz(n,value);
            reload();
            return bits;
        }
        const double rounded=std::trunc(value);
        const bool inexact=rounded!=value;
        if (rounded < -2147483648.0 || rounded >= 2147483648.0 || (inexact && flags[28])) {
            publish();
            const auto bits=guest::Fpu(c).fctiwz(n,value);
            reload();
            return bits;
        }
        // The native valid conversion has the same sticky flags, signed
        // spill bits and final OR-based exception summary as fctiwz.
        if (inexact) { if (!flags[6]) flags[0]=1; flags[6]=1; }
        flags[13]=0; flags[14]=inexact;
        dirty=true; summaries_pending=true; conversion_summary=true;
        const auto bits=std::uint64_t(std::int64_t(std::int32_t(rounded)));
        c.s.wr<8>(guest::reg::fpr(n),bits);
        return bits;
    }
};
struct Ratings {
    double auxiliary, seventh, eighth, sixth, fourth, first, third, second;
    void reload(guest::Cpu& c) {
        auxiliary=c.f(24); seventh=c.f(25); eighth=c.f(26); sixth=c.f(27);
        fourth=c.f(28); first=c.f(29); third=c.f(30); second=c.f(31);
    }
    void publish(guest::Cpu& c) const {
        c.set_f(24,auxiliary); c.set_f(25,seventh); c.set_f(26,eighth); c.set_f(27,sixth);
        c.set_f(28,fourth); c.set_f(29,first); c.set_f(30,third); c.set_f(31,second);
    }
};
struct NativeKernel {
    guest::Cpu& c;
    Arithmetic fp;
    Ratings ratings;
    bool active = true;
    explicit NativeKernel(guest::Cpu& cpu):c(cpu),fp(cpu) { ratings.reload(c); }
    ~NativeKernel() { if(active) ratings.publish(c); }
    std::uint32_t draw(std::int32_t limit,std::uint32_t back) {
        if (routine_checker || dispatch_probe || match_eng_declining || &c.s == match_eng_original_state ||
            match_eng_call_observer || limit > 65535 || limit < -65535) {
            return boundary([&] { return match_eng::draw(c,std::uint32_t(limit),back); });
        }
        c.set(3,std::uint32_t(limit));c.link(back);
        guest::Frame frame(c,0x101d3c90,0x70,{.first_gpr=27});
        guest::keep(c,{{30,std::uint32_t(limit)},{28,c.toc(-0x7b90)},{29,c.toc(-0x7b84)}});
        const auto seed_at=c.r(28),cursor_at=c.r(29);
        if (guest::compare(c,limit)==0)return guest::result(c,0);
        c.set(4,c.u32(cursor_at)+2);c.put32(cursor_at,c.r(4));
        if (guest::compare_logical(c,c.r(4),c.u32(c.toc(-0x7b88)))>0) {
            c.put32(cursor_at,c.toc(-0x7b8c));
            boundary([&] {c.call(0x1077b770,0x101d3ce4);});
            c.put32(seed_at,c.r(3)&0xffff);
        }
        // Keep both compares and the signed remainder of the original draw.
        guest::compare(c,limit,0xffff);guest::compare(c,limit,-0xffff);
        const auto sum=std::uint32_t(c.u16(c.u32(cursor_at)))+c.u32(seed_at);
        const auto remainder=std::int32_t(sum-std::uint32_t(std::int32_t(sum)/limit)*std::uint32_t(limit));
        guest::keep(c,{{31,std::uint32_t(remainder)}});
        c.set(27,std::uint32_t(guest::compare(c,limit)>0 ? remainder : -remainder));
        return guest::result(c,c.r(27));
    }
    void consume_work(std::uint32_t player,double work) {
        if (routine_checker || dispatch_probe || !match_pl_route_allowed(0x10239590)) {
            boundary([&] {c.call(0x10239590,0x101df294);});return;
        }
        c.link(0x101df294);
        c.s.pc=0x10239590;
        GuestNativeScope scope(c.s,0x10239590);
        const auto doubles=c.u32(c.r(2)-0x51e8),singles=c.u32(c.r(2)-0x51e0);
        c.set(7,doubles);guest::compare(c,-1,-1);c.set(6,singles);
        unsigned factor=0xcc;
        for (const auto [threshold,coefficient]:{std::pair{0xb8u,0x64u},std::pair{0xc0u,0x5cu},std::pair{0xc4u,0xc8u}}) {
            const double condition=c.f32(player+0x50);c.set_f(2,condition);
            fp.fcmp(0,condition,c.f32(singles+threshold));
            if(c.cr(0)&4) {factor=coefficient;break;}
        }
        const double coefficient=c.f32(singles+factor);c.set_f(2,coefficient);
        const double scaled=fp.fmuls(coefficient,work);c.set_f(1,scaled);
        const double before=c.f32(player+0x50);c.set_f(2,before);
        const double cost=fp.fdivs(scaled,c.f32(singles+0xbc));
        c.put_f32(player+0x50,float(fp.fsubs(before,cost)));
        const double after=c.f32(player+0x50);c.set_f(1,after);
        fp.fcmp(0,after,c.f32(singles));
        if(c.cr(0)&8)c.put32(player+0x50,c.u32(singles));
        c.finish();
    }
    template<class F> std::invoke_result_t<F> boundary(F&& f) {
        ratings.publish(c); fp.publish(); active=false;
        if constexpr(std::is_void_v<std::invoke_result_t<F>>) {
            f(); ratings.reload(c); fp.reload(); active=true;
        } else {
            auto value=f(); ratings.reload(c); fp.reload(); active=true; return value;
        }
    }
};
template<class Floating>
inline double from_int(guest::Cpu& c,Floating& fp,std::uint32_t slot,std::uint32_t value,double magic) {
    c.put32(slot,0x43300000);c.put32(slot+4,value^0x80000000);
    return fp.fsub(c.f64(slot),magic);
}
template<class Floating>
inline double from_int_single(guest::Cpu& c,Floating& fp,std::uint32_t slot,std::uint32_t value,double magic) {
    c.put32(slot,0x43300000);c.put32(slot+4,value^0x80000000);
    return fp.fsubs(c.f64(slot),magic);
}
// Diagnostic mode uses the same formulas, publishing every rating immediately.
// Observers may inspect or change registers during any memory access.
struct ObservedRating {
    guest::Cpu& cpu;
    unsigned number;
    operator double() const { return cpu.f(number); }
    ObservedRating& operator=(double value) { cpu.set_f(number, value); return *this; }
};
struct ObservedRatings {
    ObservedRating auxiliary, seventh, eighth, sixth, fourth, first, third, second;
    explicit ObservedRatings(guest::Cpu& c)
        : auxiliary{c,24}, seventh{c,25}, eighth{c,26}, sixth{c,27},
          fourth{c,28}, first{c,29}, third{c,30}, second{c,31} {}
};
struct ObservedKernel {
    guest::Cpu& c;
    guest::Fpu fp;
    ObservedRatings ratings;
    explicit ObservedKernel(guest::Cpu& cpu) : c(cpu), fp(cpu), ratings(cpu) {}
    std::uint32_t draw(std::int32_t limit, std::uint32_t back) {
        return match_eng::draw(c, std::uint32_t(limit), back);
    }
    void consume_work(std::uint32_t, double) { c.call(0x10239590, 0x101df294); }
    template<class F> std::invoke_result_t<F> boundary(F&& operation) { return operation(); }
};

}

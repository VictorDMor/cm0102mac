#include "transfer_manager_offer_news.hpp"
#include "transfer_manager_check_coverage.hpp"
#include "semantic_io_checks.hpp"
#include <string_view>

namespace cm::aot { bool transfer_manager_reference(State&); }

namespace {
using namespace cm::aot::guest;
using namespace cm::semantic_io_checks;
using namespace cm::aot::transfer_manager;

constexpr std::uint32_t response = 0x104024e0, termination = 0x10403250, conclude = 0x103e4c90,
    collect = 0x103fb840, create = 0x102bdf90, put = 0x102cb4b0, publish = 0x102cc140,
    forget = 0x103ec4f0, schedule = 0x103eb4a0, clear = 0x103f6190;
constexpr std::uint32_t manager = fixture, offer = fixture + 0x1000, offer_slot = fixture + 0x1100,
    staff = fixture + 0x1200, clubs = fixture + 0x2000, names = fixture + 0x4000, roles = fixture + 0x4200,
    contract = fixture + 0x4400, shortlist = fixture + 0x4500, globals = fixture + 0x5000,
    effects = fixture + 0x6000, trace = fixture + 0x7000;
constexpr std::uint32_t seller = clubs + 0x248, buyer = seller + 0x248, reason = 0x00112233;
constexpr std::uint32_t status_flags = globals + 0x400;
constexpr unsigned person_id = 8;
constexpr std::array<std::uint32_t, 4> person_arguments{person_id, 25, 30, 38};

struct Case {
    unsigned recipients = 2, contract_type = 0, firm = 0, terminate = 0, close_flag = 0xff, contract_bits = 0;
    bool has_contract = true, seller_rep = true, buyer_rep = true, no_buyer = false,
         real_candidates = false, direct_candidates = false, include_buyer = true, direct_wrapper = false;
};
Case active;
bool original_route = false, production_route = false;
std::uint32_t current = 0;

void original(State&, std::uint32_t);
void rewrite(State&, std::uint32_t);
bool observe(State&);

struct DirectBoundary final : MemoryObserver {
    State& s;
    unsigned entered = 0;
    explicit DirectBoundary(State& state) : s(state) {
        const auto frame = stack - (current == response ? 0x1b0 : 0x1c0) - 0x1e0;
        if (active.direct_wrapper) restrict_write_notifications(stack - 0x210, stack - 0x8c);
        else restrict_write_notifications(frame, frame + 4);
    }
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t address, const std::uint8_t*, std::size_t) override {
        if (!active.direct_wrapper) {
            require(s.pc == collect && dispatch_probe == nullptr, "Offer news candidate direct entry");
            entered = 1;
            dispatch_probe = observe;
            return;
        }
        constexpr std::array children{response, termination, forget, schedule, clear};
        constexpr std::array sizes{0x1b0u, 0x1c0u, 0x50u, 0x90u, 0x40u};
        for (unsigned i = 0; i < children.size(); ++i) {
            if (s.pc != children[i] || address != stack - 0x50 - sizes[i]) continue;
            require(dispatch_probe == nullptr && !(entered & (1u << i)), "Offer response child direct entry");
            entered |= 1u << i;
            if (s.pc != clear) dispatch_probe = observe;
        }
    }
};

void record(Cpu& c) {
    const auto count = c.u32(trace);
    require(count < 100, "Offer news trace exhausted");
    c.put32(trace, count + 1);
    auto out = trace + 4 + count * 80;
    const auto add = [&](std::uint32_t value) { c.put32(out, value); out += 4; };
    add(c.s.pc);
    add(c.lr());
    add(c.r(0));
    for (unsigned r = 3; r <= 9; ++r) add(c.r(r));
    for (unsigned r = 24; r <= 31; ++r) add(c.r(r));
    add(c.cr(0));
    add(c.s.r<1>(reg::xer_ca));
}

bool external(State& s, unsigned allowed) {
    if (!production_route || original_route) return false;
    const auto back = Cpu(s).lr();
    s.budget = s.executed + allowed;
    require(dispatch_page(s), "Missing offer news service");
    if (s.pc != back) run_until(s, back);
    s.budget = s.executed;
    return true;
}

std::array<std::uint32_t, 10> payload(bool withdrawn) {
    std::array<std::uint32_t, 10> values{};
    std::ranges::copy(person_arguments, values.begin());
    const bool has_seller = active.has_contract && active.contract_type != 5;
    values[4] = has_seller ? 1 : 0xffffffff;
    values[5] = active.no_buyer ? 0xffffffff : 2;
    if (!withdrawn) {
        values[6] = active.direct_wrapper ? 0 : roles;
        values[7] = 12;
        values[8] = 0x99887766;
        values[9] = std::uint32_t(std::int8_t(active.close_flag));
    } else {
        values[6] = std::uint32_t(std::int8_t(active.close_flag));
        values[7] = current == conclude ? 0 : reason;
        values[8] = active.seller_rep && has_seller ? 33 : 0;
        values[9] = active.buyer_rep && !active.no_buyer ? 44 : 0;
    }
    return values;
}

bool observe(State& s) {
    Cpu c(s);
    const auto entry = s.pc, back = c.lr(), a = c.r(3), b = c.r(4), d = c.r(5);
    std::uint32_t answer = 0;
    switch (entry) {
    case 0x10082f20:
        require((back == 0x10402528 || back == 0x104032a0 || back == 0x103fb8a8 || back == 0x103ec534) &&
                a == globals + 0x100 && b == staff, "Offer news primary contract");
        answer = active.has_contract ? contract : 0;
        break;
    case collect:
        require((back == 0x10402590 || back == 0x10403304) && a == manager && b == offer &&
                d == (back == 0x10402590 ? 0x2d : 0x2f) && c.r(6) == c.sp() + 0x38 && c.r(7) == 30,
                "Offer news candidate sources");
        if (active.real_candidates) {
            if (!active.direct_candidates) record(c);
            if (original_route) {
                original(s, entry);
                return true;
            }
            return false;
        }
        for (unsigned i = 0; i < active.recipients; ++i) c.put32(c.r(6) + i * 4, i ? seller : buyer);
        answer = active.recipients;
        break;
    case 0x102cdbf0:
        require(active.real_candidates && a == globals + 0x104 && (b == seller || b == buyer),
                "Offer news real candidate club");
        answer = b == seller || active.include_buyer;
        break;
    case 0x10083110:
        require(active.real_candidates && back == 0x103fb994 && a == globals + 0x100 && b == staff,
                "Offer news loan lookup");
        break;
    case 0x10362ef0:
        require(active.real_candidates && a == shortlist && b == staff && d == c.sp() + 0x54 &&
                c.r(6) == 1 && c.r(7) == 1 && c.r(8) == 1, "Offer news real candidate shortlist");
        break;
    case 0x10738c00:
    case 0x10738c20:
    case 0x10738e00:
        require(active.real_candidates && a == c.sp() + 0x54, "Offer news real candidate list");
        return external(s, 160);
    case create:
        require(a == c.sp() + 0xb0 && (b == 0xfa3 || b == 0xfae) && !d, "Offer news creation");
        record(c);
        return external(s, 5000);
    case put: {
        const bool withdrawn = c.u32(a) == 0xfae;
        require(a == c.sp() + 0xb0 && b < 10 && d == payload(withdrawn)[b], "Offer news parameter");
        record(c);
        return external(s, 100);
    }
    case publish: {
        const auto category = c.u32(b);
        const bool withdrawn = category == 0xfae;
        require(a == globals + 0x104 && b == c.sp() + 0xb0 && (d == seller || d == buyer) &&
                (withdrawn || category == 0xfa3) && c.u16(b + 0xd2) == 2001, "Offer news delivered item");
        const auto words = payload(withdrawn);
        for (unsigned i = 0; i < words.size(); ++i)
            require(c.u32(b + 8 + i * 4) == words[i], "Offer news delivered payload");
        const auto index = c.u32(effects);
        c.put32(effects + 4 + index * 8, category);
        c.put32(effects + 8 + index * 8, d);
        c.put32(effects, index + 1);
        break;
    }
    case 0x1007f610:
        require(back == 0x103e4cd4 && a == globals + 0x100 && b == offer &&
                c.u8(offer + 0x35) == (std::int8_t(active.firm) != 0), "Offer response contract transition");
        answer = active.terminate;
        break;
    case 0x1036f100:
        require(active.direct_wrapper && back == 0x103ec574 && a == shortlist && b == buyer && d == staff,
                "Offer news direct shortlist cleanup");
        c.put8(effects + 0x40, 1);
        break;
    case 0x10717890:
    case 0x10718130:
        require(active.direct_wrapper && (back == 0x103eb56c || back == 0x103eb5e8),
                "Offer response direct calendar");
        return external(s, 5000);
    case 0x10738d10:
        require(active.direct_wrapper && back == 0x103eb634 && a == manager + 0x18 + 10 * 48 &&
                b == offer + 0x28 && d == 4, "Offer response direct schedule");
        c.put8(effects + 0x41, 1);
        break;
    case response:
    case termination:
        if (back == return_sentinel) return false;
        require(a == manager && b == offer && c.u8(offer + 0x34) == 8 &&
                (entry == response ? back == 0x103e4cf0 : back == 0x103e4d08 && d == 0), "Offer news from response");
        record(c);
        if (!production_route) {
            rewrite(s, entry);
            return true;
        }
        return false;
    case forget:
    case schedule:
    case clear:
        require(a == manager && b == offer && c.u8(offer + 0x34) == 15 && c.u8(offer + 0x36) == 3 &&
                (entry != schedule || (d == 10 && c.r(6) == 0)), "Offer response termination cleanup");
        c.put8(effects + (entry == forget ? 0x40 : entry == schedule ? 0x41 : 0x42), 1);
        break;
    default:
        return false;
    }
    record(c);
    for (unsigned r = 4; r <= 10; ++r) c.set(r, 0xad000000 + r);
    c.set(3, answer);
    c.set_cr(0, 4);
    c.finish();
    if (production_route && !original_route && entry == 0x10082f20) {
        if (back == 0x103fb8a8) s.budget = s.executed + 6;
        else if (active.direct_candidates) dispatch_probe = nullptr;
    }
    if (production_route && !original_route && active.direct_wrapper &&
        (entry == 0x1007f610 || entry == 0x1036f100 || entry == 0x10738d10 ||
         (entry == publish && ((back == 0x10402688 && c.r(26) + 1 == active.recipients) ||
                               (back == 0x10403430 && c.r(24) + 1 == active.recipients)))))
        dispatch_probe = nullptr;
    return true;
}

std::unique_ptr<State> make_state(std::uint32_t entry) {
    auto s = std::make_unique<State>();
    s->registers.fill(0x65);
    s->memory.map(fixture, 0xa000);
    s->memory.map(toc - 0x8000, 0x5000);
    s->memory.map(stack - 0x3000, 0x4000);
    s->pc = entry;
    s->budget = 100000;
    Cpu c(*s);
    for (unsigned i = 0; i < 8; ++i) c.set_cr(i, i);
    for (unsigned i = 1024; i < 1028; ++i) s->wr<1>(i, 0);
    for (unsigned i = 1280; i < 1312; ++i) s->wr<1>(i, 0);
    c.set({{1, stack}, {2, toc}, {3, manager}, {4, entry == conclude ? offer_slot : offer},
           {5, entry == conclude ? active.firm : reason}});
    c.set_lr(return_sentinel);
    c.put32(toc - 0x7be0, globals);
    c.put32(toc - 0x7be8, globals + 4);
    c.put32(toc - 0x7b7c, globals + 8);
    c.put32(toc - 0x7b78, globals + 12);
    c.put32(toc - 0x7bf0, globals + 16);
    c.put32(toc - 0x7b80, globals + 0x200);
    c.put32(toc - 0x79ec, globals + 0x208);
    c.put32(toc - 0x3fac, globals + 0x300);
    c.put32(toc - 0x3fc0, globals + 0x320);
    c.put32(globals, staff - person_id * 0x70);
    c.put32(globals + 4, clubs);
    c.put32(globals + 8, globals + 0x100);
    c.put32(globals + 16, globals + 0x104);
    c.put16(globals + 0x200, 120);
    c.put16(globals + 0x202, 2001);
    c.put32(manager + 0x988, shortlist);
    c.put32(manager + 0x9a0, status_flags);
    c.put8(status_flags + person_id * 10, 1);
    c.put32(offer_slot, offer);
    c.put32(offer + 4, active.direct_wrapper ? 0 : roles);
    c.put32(offer + 8, active.no_buyer ? 0xffffffff : 2);
    c.put32(offer + 0xc, person_id);
    c.put8(offer + 0x24, active.close_flag);
    c.put32(offer + 0x28, 12);
    c.put32(offer + 0x2c, 0x99887766);
    c.put32(staff, person_id);
    for (unsigned i = 1; i <= 3; ++i) {
        c.put32(staff + 0x24 + i * 4, names + i * 0x30);
        c.put32(names + i * 0x30 + 0x20, person_arguments[i]);
    }
    c.put32(contract + 4, 1);
    c.put8(contract + 0x38, active.contract_type << 2 | active.contract_bits);
    c.put32(seller, 1);
    c.put32(buyer, 2);
    c.put32(seller + 0x90, active.seller_rep ? names + 0x100 : 0);
    c.put32(buyer + 0x90, active.buyer_rep ? names + 0x110 : 0);
    c.put32(names + 0x100, 33);
    c.put32(names + 0x110, 44);
    return s;
}

void original(State& s, std::uint32_t entry) {
    const bool previous = original_route;
    original_route = true;
    s.pc = entry;
    const auto stop = Cpu(s).lr();
    while (s.pc != stop) {
        const auto pc = s.pc;
        if (pc == collect && entry != collect) {
            require(observe(s), "Original offer recipient boundary");
        } else if ((pc == response || pc == termination) && entry == conclude) {
            Cpu c(s);
            if (!active.direct_wrapper) record(c);
            original(s, pc);
        } else if ((pc == forget || pc == schedule || pc == clear) && entry == conclude) {
            if (active.direct_wrapper) original(s, pc);
            else require(observe(s), "Original offer response cleanup boundary");
        } else if (!transfer_manager_reference(s)) {
            run_until(s, Cpu(s).lr());
        }
    }
    original_route = previous;
}

void rewrite(State& s, std::uint32_t entry) {
    Cpu c(s);
    switch (entry) {
    case response: announce_offer_response(c, c.r(3), c.r(4)); break;
    case termination: announce_offer_termination(c, c.r(3), c.r(4), c.r(5)); break;
    default: conclude_offer_response(c, c.r(3), c.r(4), c.r(5)); break;
    }
    c.finish();
}

void production(State& s, std::uint32_t entry) {
    DirectBoundary boundary(s);
    if (active.direct_candidates || active.direct_wrapper) s.memory.observer = &boundary;
    production_route = true;
    s.pc = entry;
    s.budget = s.executed;
    run_until(s, return_sentinel);
    s.memory.observer = nullptr;
    require(boundary.entered == (active.direct_wrapper ? 31u : active.direct_candidates ? 1u : 0u),
            "Offer news direct call coverage");
    dispatch_probe = observe;
    production_route = false;
}

unsigned checked = 0;
void check(std::uint32_t entry, const Case& test, RoutineRunner selected = rewrite) {
    active = test;
    current = entry;
    auto s = make_state(entry);
    auto control = clone(*s);
    original(*control, entry);
    Cpu c(*control);
    const bool ended = entry == conclude && std::int8_t(test.terminate);
    const auto sent = test.real_candidates ? unsigned(test.include_buyer) +
        (entry == termination && test.has_contract && test.contract_type != 5) : test.recipients;
    require(c.u32(effects) == sent * (ended ? 2 : 1), "Original offer news delivery count");
    if (sent) require(c.u32(effects + 4) == (entry == termination ? 0xfae : 0xfa3), "Original offer news category");
    if (entry == conclude) {
        require(c.u8(offer + 0x34) == (ended ? 15 : 8) && c.u8(offer + 0x36) == (ended ? 3 : 0) &&
                c.u8(effects + 0x40) == ended && c.u8(effects + 0x41) == ended &&
                (test.direct_wrapper ? c.u8(status_flags + person_id * 10) == !ended : c.u8(effects + 0x42) == ended),
                "Original offer transition and cleanup order");
    }
    const auto differences = compare_routine_call(*s, entry, original, selected,
        RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
    for (const auto& difference : differences) std::cerr << difference << '\n';
    require(differences.empty(), "Offer news case " + std::to_string(checked));
    ++checked;
}

void lose_notice(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu(s).put32(effects, 0);
}
}

int cm::aot::transfer_manager::checks::offer_news(int argc, char** argv) {
    try {
        install_check_coverage();
        dispatch_probe = observe;
        if (argc == 2 && std::string_view(argv[1]) == "--production") {
            check(response, {}, production);
            check(response, {.real_candidates = true, .direct_candidates = true}, production);
            check(termination, {.real_candidates = true, .direct_candidates = true}, production);
            check(termination, {.recipients = 0, .has_contract = false, .no_buyer = true}, production);
            check(conclude, {.firm = 0x80, .terminate = 1}, production);
            check(conclude, {.firm = 1, .terminate = 1, .direct_wrapper = true}, production);
            std::cout << checked << " offer news production comparisons\n";
            return 0;
        }
        require(argc == 1, "Unknown offer news check mode");
        for (auto entry : {response, termination}) {
            check(entry, {});
            check(entry, {.recipients = 0});
            check(entry, {.recipients = 1, .has_contract = false});
            check(entry, {.contract_type = 5});
            check(entry, {.contract_type = 63, .close_flag = 0x80});
            check(entry, {.contract_type = 63, .contract_bits = 2});
            check(entry, {.seller_rep = false, .buyer_rep = false});
            check(entry, {.no_buyer = true});
        }
        check(conclude, {});
        check(conclude, {.firm = 1});
        check(conclude, {.terminate = 0xff});
        check(conclude, {.firm = 0x80, .terminate = 0x80});
        check(conclude, {.recipients = 0, .firm = 0x100, .terminate = 0x100});
        active = {};
        current = response;
        auto s = make_state(response);
        const auto differences = compare_routine_call(*s, response, original, lose_notice,
            RoutineChecker::result_r3, false, true);
        require(std::ranges::any_of(differences, [](const auto& line) { return line.contains("byte 30006003"); }),
                "The offer news comparison missed a lost notice");
        std::cout << checked << " offer news comparisons and a negative control\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}

#include "localization_lookup.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::localization_lookup {
namespace {
// Query matching ignores braces and spaces and decodes the two original escapes.
// Guest registers remain live across each individual access and dependency.
void normalize_key(SemanticGuestCall& a) {
    a.at(0x10728430);a.jump(0x107284f4);
    for (;;) {
        a.at(0x107284f4);a.indexed_load<1>(0,4,29);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x10728434);a.indexed_load<1>(0,4,29);a.sign8(0,0);a.compare(0,123);
        if (!a.equal()) {
            a.at(0x10728444);a.indexed_load<1>(0,4,29);a.sign8(0,0);a.compare(0,125);
            if (!a.equal()) {
                a.at(0x10728454);a.indexed_load<1>(0,4,29);a.sign8(0,0);a.compare(0,32);
                if (!a.equal()) {
                    a.at(0x10728464);a.indexed_load<1>(0,4,29);a.sign8(0,0);a.compare(0,92);
                    bool newline = false;
                    if (a.equal()) {
                        a.at(0x10728474);a.add(3,4,29);a.load<1>(0,3,0x1);a.compare(0,110);
                        newline = a.equal();
                    }
                    if (newline) {
                        a.at(0x10728484);a.move(0,24);a.offset(3,1,0x2bc);a.value(5,0xa);a.offset(29,29,0x1);
                        a.indexed_store<1>(5,3,0);a.offset(24,24,0x1);a.jump(0x107284f0);
                    } else {
                        a.at(0x107284a0);a.indexed_load<1>(0,4,29);a.sign8(0,0);a.compare(0,92);
                        bool quote = false;
                        if (a.equal()) {
                            a.at(0x107284b0);a.add(3,4,29);a.load<1>(0,3,0x1);a.compare(0,34);
                            quote = a.equal();
                        }
                        if (quote) {
                            a.at(0x107284c0);a.move(0,24);a.offset(3,1,0x2bc);a.value(5,0x22);a.offset(29,29,0x1);
                            a.indexed_store<1>(5,3,0);a.offset(24,24,0x1);a.jump(0x107284f0);
                        } else {
                            a.at(0x107284dc);a.indexed_load<1>(5,4,29);a.move(0,24);a.offset(3,1,0x2bc);
                            a.offset(24,24,0x1);a.indexed_store<1>(5,3,0);
                        }
                    }
                }
            }
        }
        a.at(0x107284f0);a.offset(29,29,0x1);
    }
    a.at(0x10728500);a.offset(3,1,0x2bc);a.value(0,0x0);a.indexed_store<1>(0,3,24);a.offset(24,24,-0x1);
    a.jump(0x10728524);
    for (;;) {
        a.at(0x10728524);a.offset(3,1,0x2bc);a.indexed_load<1>(0,3,24);a.sign8(0,0);a.compare(0,32);
        if (!a.equal()) break;
        a.at(0x10728538);a.compare(24,0);
        if (!a.at_least()) break;
        a.at(0x10728514);a.offset(3,1,0x2bc);a.value(0,0x0);a.indexed_store<1>(0,3,24);a.offset(24,24,-0x1);
    }
}

// Cached records own three strings and carry ten additional word32 fields.
void find_cached_record(SemanticGuestCall& a) {
    a.at(0x1072854c);a.value(0,0x0);a.value(4,0x0);a.store(1,0x60,0);a.value(3,0x0);a.load(0,1,0x60);
    a.store(1,0x64,4);a.compare(0,0,false);a.store(1,0x68,3);
    if (!a.equal()) {
        a.at(0x10728570);a.load(3,1,0x60);a.call(0x10773610);
    }
    a.at(0x10728578);a.offset(3,1,0x2bc);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
    a.call(0x107735f0);a.store(1,0x60,3);a.offset(4,1,0x2bc);a.load(3,1,0x60);a.call(0x1077dbf0);a.load(4,2,-0x3590);
    a.offset(3,1,0x60);a.load(5,2,-0x3594);a.value(6,0x34);a.load(7,2,-0x3ea4);a.call(0x10775600);a.move(25,3,true);
    if (!a.equal()) {
        a.at(0x107285bc);a.load(0,31,0x0);a.load(24,25,0x0);a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x107285cc);a.load(3,31,0x0);a.call(0x10773610);
        }
        a.at(0x107285d4);a.move(3,24);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
        a.call(0x107735f0);a.store(31,0x0,3);a.move(4,24);a.load(3,31,0x0);a.call(0x1077dbf0);a.load(0,31,0x4);
        a.load(24,25,0x4);a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x10728608);a.load(3,31,0x4);a.call(0x10773610);
        }
        a.at(0x10728610);a.move(3,24);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
        a.call(0x107735f0);a.store(31,0x4,3);a.move(4,24);a.load(3,31,0x4);a.call(0x1077dbf0);a.load(0,31,0x8);
        a.load(24,25,0x8);a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x10728644);a.load(3,31,0x8);a.call(0x10773610);
        }
        a.at(0x1072864c);a.move(3,24);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
        a.call(0x107735f0);a.store(31,0x8,3);a.move(4,24);a.load(3,31,0x8);a.call(0x1077dbf0);a.value(24,0x0);
        a.jump(0x10728694);
        for (;;) {
            a.at(0x10728694);a.compare(24,10);
            if (!a.less()) break;
            a.at(0x10728678);a.mask(3,24,0xfffffffc,false,2);a.mask(0,24,0xfffffffc,false,2);a.add(3,25,3);
            a.offset(24,24,0x1);a.load(4,3,0xc);a.add(3,31,0);a.store(3,0xc,4);
        }
        a.at(0x1072869c);a.compare(28,0,false);
        if (!a.equal()) {
            a.at(0x107286a4);a.load(0,2,-0x3590);a.value(3,0x4ec50000);a.offset(3,3,-0x13b1);a.subtract(0,0,25);
            a.unsigned_multiply_high(0,3,0);a.mask(0,0,0xfffffff,false,28);a.store(28,0x0,0);
        }
        a.at(0x107286c0);a.value(27,0x1);
    }
    a.at(0x107286c4);a.load(0,1,0x60);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x107286d0);a.load(3,1,0x60);a.call(0x10773610);
    }
    a.at(0x107286d8);a.load(0,1,0x64);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x107286e4);a.load(3,1,0x64);a.call(0x10773610);
    }
    a.at(0x107286ec);a.load(0,1,0x68);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x107286f8);a.load(3,1,0x68);a.call(0x10773610);a.jump(0x10728aa8);
    }
}

// The file path retains original fixed-width records, unchecked read results,
// language-column skips and per-access byte swapping. It owns the opened stream.
bool find_file_record(SemanticGuestCall& a) {
    a.at(0x10728704);a.load(3,2,-0x3e90);a.offset(4,26,0xd8);a.call(0x10739de0);a.move(28,3,true);
    if (a.equal()) {
        a.at(0x10728718);a.offset(3,26,0x92);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0x0);a.value(5,0x0);
        a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,25);a.offset(4,26,0x9e);a.offset(6,1,0x40);
        a.offset(7,1,0x38);a.value(8,0xd62);a.call(0x1077b720);a.move(4,25);a.offset(3,26,0xab);a.value(5,0x0);
        a.call(0x10769b60);a.load(5,2,-0x7bac);a.value(0,0x0);a.load(3,2,-0x3e8c);a.offset(4,26,0xdb);
        a.store<1>(5,0x0,0);a.call(0x1077dbf0);a.value(3,0x0);a.jump(0x10728aac);
        return false;
    }
    a.at(0x10728780);a.load(0,2,-0x3598);a.move(3,28);a.value(5,0x0);a.mask(4,0,0xfffffffc,false,2);a.offset(4,4,0xc);
    a.call(0x10777f10);a.value(29,0x0);a.jump(0x10728a8c);
    for (;;) {
        a.at(0x10728a8c);a.load(0,2,-0x3594);a.compare_registers(29,0);
        if (a.at_least()) break;
        a.at(0x10728a98);a.sign8(0,27,true);
        if (!a.equal()) break;
        a.at(0x107287a0);a.move(3,30);a.move(6,28);a.value(4,0x1);a.value(5,0x226);a.call(0x10775fe0);
        a.load(0,31,0x0);a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x107287c0);a.load(3,31,0x0);a.call(0x10773610);
        }
        a.at(0x107287c8);a.move(3,30);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
        a.call(0x107735f0);a.store(31,0x0,3);a.move(4,30);a.load(3,31,0x0);a.call(0x1077dbf0);a.value(24,0x0);
        a.value(25,0x0);a.jump(0x1072887c);
        for (;;) {
            a.at(0x1072887c);a.load(3,31,0x0);a.sign16(0,25);a.indexed_load<1>(0,3,0);a.sign8(0,0,true);
            if (a.equal()) break;
            a.at(0x107287f8);a.load(3,31,0x0);a.sign16(0,25);a.indexed_load<1>(0,3,0);a.sign8(0,0);a.compare(0,32);
            if (!a.equal()) {
                a.at(0x10728810);a.load(3,31,0x0);a.sign16(0,25);a.indexed_load<1>(0,3,0);a.sign8(0,0);
                a.compare(0,92);
                bool newline = false;
                if (a.equal()) {
                    a.at(0x10728828);a.load(3,31,0x0);a.sign16(0,25);a.add(3,3,0);a.load<1>(0,3,0x1);a.compare(0,110);
                    newline = a.equal();
                }
                if (newline) {
                    a.at(0x10728840);a.move(0,24);a.offset(3,1,0x94);a.value(4,0xa);a.offset(25,25,0x1);
                    a.indexed_store<1>(4,3,0);a.offset(24,24,0x1);a.jump(0x10728878);
                } else {
                    a.at(0x1072885c);a.load(5,31,0x0);a.sign16(4,25);a.move(0,24);a.offset(3,1,0x94);
                    a.indexed_load<1>(4,5,4);a.offset(24,24,0x1);a.indexed_store<1>(4,3,0);
                }
            }
            a.at(0x10728878);a.offset(25,25,0x1);
        }
        a.at(0x10728890);a.offset(3,1,0x94);a.value(0,0x0);a.indexed_store<1>(0,3,24);a.offset(3,1,0x94);
        a.offset(4,1,0x2bc);a.call(0x1077dc90);a.compare(3,0);
        if (a.equal()) {
            a.at(0x107288b0);a.move(3,30);a.move(6,28);a.value(4,0x1);a.value(5,0x1e);a.call(0x10775fe0);
            a.load(0,31,0x4);a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x107288d0);a.load(3,31,0x4);a.call(0x10773610);
            }
            a.at(0x107288d8);a.move(3,30);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
            a.call(0x107735f0);a.store(31,0x4,3);a.move(4,30);a.load(3,31,0x4);a.call(0x1077dbf0);a.value(24,0x1);
            a.jump(0x10728930);
            for (;;) {
                a.at(0x10728930);a.load(0,2,-0x35a0);a.compare_registers(24,0);
                if (!a.less()) break;
                a.at(0x10728904);a.move(3,30);a.move(6,28);a.value(4,0x1);a.value(5,0x226);a.call(0x10775fe0);
                a.move(3,30);a.move(6,28);a.value(4,0x4);a.value(5,0xa);a.call(0x10775fe0);a.offset(24,24,0x1);
            }
            a.at(0x1072893c);a.move(3,30);a.move(6,28);a.value(4,0x1);a.value(5,0x226);a.call(0x10775fe0);
            a.load(0,31,0x8);a.compare(0,0,false);
            if (!a.equal()) {
                a.at(0x1072895c);a.load(3,31,0x8);a.call(0x10773610);
            }
            a.at(0x10728964);a.move(3,30);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
            a.call(0x107735f0);a.store(31,0x8,3);a.move(4,30);a.load(3,31,0x8);a.call(0x1077dbf0);a.value(25,0x0);
            a.jump(0x10728a14);
            for (;;) {
                a.at(0x10728a14);a.sign16(0,25);a.compare(0,10);
                if (!a.less()) break;
                a.at(0x10728990);a.sign16(0,25);a.move(6,28);a.mask(3,0,0xfffffffc,false,2);a.value(4,0x4);
                a.offset(3,3,0xc);a.value(5,0x1);a.add(3,31,3);a.call(0x10775fe0);a.sign16(3,25);a.sign16(0,25);
                a.sign16(4,25);a.sign16(6,25);a.mask(3,3,0xfffffffc,false,2);a.mask(0,0,0xfffffffc,false,2);
                a.mask(5,4,0xfffffffc,false,2);a.mask(7,6,0xfffffffc,false,2);a.add(4,31,3);a.add(3,31,0);
                a.add(6,31,5);a.sign16(0,25);a.load(5,4,0xc);a.add(4,31,7);a.load(3,3,0xc);
                a.mask(0,0,0xfffffffc,false,2);a.load(7,4,0xc);a.offset(25,25,0x1);a.mask(4,3,0xff0000,false,8);
                a.load(6,6,0xc);a.insert_mask(4,5,0xff000000,24);a.add(3,31,0);a.insert_mask(4,6,0xff00,24);
                a.insert_mask(4,7,0xff,8);a.store(3,0xc,4);
            }
            a.at(0x10728a20);a.load(3,2,-0x35a0);a.offset(24,3,0x1);a.jump(0x10728a58);
            for (;;) {
                a.at(0x10728a58);a.load(0,2,-0x3598);a.compare_registers(24,0);
                if (!a.less()) break;
                a.at(0x10728a2c);a.move(3,30);a.move(6,28);a.value(4,0x1);a.value(5,0x226);a.call(0x10775fe0);
                a.move(3,30);a.move(6,28);a.value(4,0x4);a.value(5,0xa);a.call(0x10775fe0);a.offset(24,24,0x1);
            }
            a.at(0x10728a64);a.value(27,0x1);a.jump(0x10728a88);
        } else {
            a.at(0x10728a6c);a.load(4,2,-0x3598);a.move(3,28);a.value(5,0x1);a.offset(0,4,-0x1);a.multiply(4,0,0x24e);
            a.offset(4,4,0x1e);a.call(0x10777f10);
        }
        a.at(0x10728a88);a.offset(29,29,0x1);
    }
    a.at(0x10728aa0);a.move(3,28);a.call(0x107766f0);
    return true;
}
}

void initialize_record(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    SemanticGuestCall a(s, initialize_record_entry, d, o);
    a.at(0x10727860);a.value(0,0x0);a.value(4,0x0);a.store(3,0x0,0);a.value(0,0x0);a.store(3,0x4,4);a.store(3,0x8,0);
    a.finish();
}

void current_language(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    SemanticGuestCall a(s, current_language_entry, d, o);
    a.at(0x10727880);a.load<1>(3,2,-0x35ac);a.finish();
}

void lookup(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    SemanticGuestCall a(s, lookup_entry, d, o);
    a.at(0x10728400);a.link();a.multiple(24);a.load(30,2,-0x3ea0);a.move(31,3);a.load(25,2,-0x7ba8);a.move(28,5);
    a.load(26,2,-0x3e98);a.value(27,0x0);a.value(24,0x0);a.value(29,0x0);a.store(1,0x8,0);a.frame(0x510);
    normalize_key(a);
    a.at(0x10728540);a.load<1>(0,2,-0x359c);a.compare(0,1,false);
    const bool cached = a.equal();
    bool finish_lookup = true;
    if (cached) find_cached_record(a);
    else finish_lookup = find_file_record(a);
    if (finish_lookup) {
        a.at(0x10728aa8);a.move(3,27);
    }
    a.at(0x10728aac);a.load(0,1,0x518);a.offset(1,1,0x510);a.restore_link();a.multiple(24,true);a.finish();
}
}

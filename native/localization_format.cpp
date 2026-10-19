#include "localization_format.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::localization_format {
namespace {
// Pair each positional argument with its live prefix/suffix modifier bytes.
void collect_bindings(SemanticGuestCall& a) {
    a.at(0x10727978);a.load(0,2,-0x35a8);a.value(20,0x0);a.compare(0,0);
    if (a.equal()) {
        a.at(0x10727988);a.offset(22,1,0x600);
    }
    a.at(0x1072798c);a.value(25,0x0);a.jump(0x107279b8);
    for (;;) {
        a.at(0x107279b8);a.compare(25,10);
        if (!a.less()) break;
        a.at(0x10727994);a.mask(4,25,0xfffffffe,false,1);a.offset(3,1,0x38);a.value(5,0xffffffff);
        a.mask(0,25,0xfffffffe,false,1);a.indexed_store<1>(5,3,4);a.offset(3,1,0x39);a.value(4,0xffffffff);
        a.offset(25,25,0x1);a.indexed_store<1>(4,3,0);
    }
    a.at(0x107279c0);a.value(25,0x0);a.jump(0x10727a8c);
    for (;;) {
        a.at(0x10727a8c);a.indexed_load<1>(0,19,25);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x107279c8);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,60);
        if (a.equal()) {
            a.at(0x107279d8);a.offset(18,18,0x1);
        }
        a.at(0x107279dc);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,123);
        bool marker = a.equal();
        if (!marker) {
            a.at(0x107279ec);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,91);
            marker = a.equal();
        }
        if (marker) {
            a.at(0x107279fc);a.compare(25,0);
            bool trailing = !a.nonpositive();
            if (trailing) {
                a.at(0x10727a04);a.add(3,19,25);a.load<1>(0,3,-0x1);a.compare(0,62);
                trailing = a.equal();
                if (trailing) {
                    a.at(0x10727a14);a.compare(18,0);
                    trailing = !a.nonpositive();
                }
            }
            if (trailing) {
                a.at(0x10727a1c);a.load(0,2,-0x35a8);a.compare(0,0);
                if (!a.equal()) {
                    a.at(0x10727a28);a.mask(0,18,0xfffffffe,false,1);a.offset(3,1,0x37);a.value(4,0x0);
                    a.indexed_store<1>(4,3,0);a.jump(0x10727a88);
                } else {
                    a.at(0x10727a3c);a.load(4,22,0x0);a.mask(0,18,0xfffffffe,false,1);a.offset(3,1,0x37);
                    a.offset(22,22,0x4);a.indexed_store<1>(4,3,0);a.jump(0x10727a88);
                }
            } else {
                a.at(0x10727a54);a.load(0,2,-0x35a8);a.compare(0,0);
                if (a.equal()) {
                    a.at(0x10727a74);a.load(4,22,0x0);a.mask(0,18,0xfffffffe,false,1);a.offset(3,1,0x38);
                    a.offset(22,22,0x4);a.indexed_store<1>(4,3,0);
                } else {
                    a.at(0x10727a60);a.mask(0,18,0xfffffffe,false,1);a.offset(3,1,0x38);a.value(4,0x0);
                    a.indexed_store<1>(4,3,0);a.jump(0x10727a88);
                }
            }
        }
        a.at(0x10727a88);a.offset(25,25,0x1);
    }
    a.at(0x10727a98);a.load(0,2,-0x35a8);a.compare(0,0);
    if (!a.equal()) {
        a.at(0x10727aa4);a.value(18,0x0);a.value(25,0x0);a.jump(0x10727ad4);
        for (;;) {
            a.at(0x10727ad4);a.compare(25,10);
            if (!a.less()) break;
            a.at(0x10727ab0);a.move(0,18);a.value(3,0x20);a.indexed_store<1>(3,29,0);a.offset(18,18,0x1);a.move(0,18);
            a.value(3,0x7e);a.indexed_store<1>(3,29,0);a.offset(18,18,0x1);a.offset(25,25,0x1);
        }
        a.at(0x10727adc);a.value(0,0x0);a.indexed_store<1>(0,29,18);a.jump(0x10727af8);
    } else {
        a.at(0x10727ae8);a.load(4,1,0x50);a.move(3,29);a.move(5,22);a.call(0x1077b6f0);
    }
}

// The temporary formatted argument list is split in place on each tilde.
void split_arguments(SemanticGuestCall& a) {
    a.at(0x10727af8);a.value(25,0x0);a.mask(0,25,0xfffffffc,false,2);a.move(18,25);a.indexed_store(29,28,0);
    a.offset(18,18,0x1);a.jump(0x10727b40);
    for (;;) {
        a.at(0x10727b40);a.indexed_load<1>(0,29,25);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x10727b10);a.indexed_load<1>(0,29,25);a.sign8(0,0);a.compare(0,126);
        if (a.equal()) {
            a.at(0x10727b20);a.add(3,25,29);a.value(4,0x0);a.mask(0,18,0xfffffffc,false,2);
            a.indexed_store<1>(4,29,25);a.offset(3,3,0x1);a.offset(18,18,0x1);a.indexed_store(3,28,0);
        }
        a.at(0x10727b3c);a.offset(25,25,0x1);
    }
}

// A translated backspace byte selects a reordered argument by its metadata word.
void append_translated_argument(SemanticGuestCall& a) {
    a.at(0x10727bf4);a.mask(0,18,0xfffffffc,false,2);a.offset(3,1,0x58);a.indexed_load(0,3,0);a.offset(3,1,0x38);
    a.mask(0,0,0xfffffffe,false,1);a.indexed_load<1>(0,3,0);a.sign8(0,0);a.compare(0,-1);
    bool plain = a.equal();
    if (plain) {
        a.at(0x10727c18);a.mask(0,18,0xfffffffc,false,2);a.offset(3,1,0x58);a.indexed_load(0,3,0);a.offset(3,1,0x39);
        a.mask(0,0,0xfffffffe,false,1);a.indexed_load<1>(0,3,0);a.sign8(0,0);a.compare(0,-1);
        plain = a.equal();
    }
    if (plain) {
        a.at(0x10727c3c);a.mask(0,18,0xfffffffc,false,2);a.offset(3,1,0x58);a.indexed_load(0,3,0);
        a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,28,0);a.jump(0x10727d10);
    } else {
        a.at(0x10727c54);a.load(0,1,0x54);a.value(3,0x0);a.store<1>(26,0x0,3);a.value(3,0x0);a.add(4,0,25);
        a.load<1>(0,4,0x1);a.compare(0,123);
        if (a.equal()) {
            a.at(0x10727c74);a.offset(25,25,0x2);a.jump(0x10727c94);
            for (;;) {
                a.at(0x10727c94);a.load(4,1,0x54);a.indexed_load<1>(0,4,25);a.sign8(0,0);a.compare(0,125);
                if (a.equal()) break;
                a.at(0x10727ca8);a.load(4,1,0x54);a.indexed_load<1>(0,4,25);a.sign8(0,0,true);
                if (a.equal()) break;
                a.at(0x10727c7c);a.load(4,1,0x54);a.move(0,3);a.offset(3,3,0x1);a.indexed_load<1>(4,4,25);
                a.offset(25,25,0x1);a.indexed_store<1>(4,26,0);
            }
            a.at(0x10727cb8);a.value(0,0x0);a.indexed_store<1>(0,26,3);
        }
        a.at(0x10727cc0);a.mask(0,18,0xfffffffc,false,2);a.offset(3,1,0x58);a.indexed_load(6,3,0);
        a.mask(5,18,0xfffffffc,false,2);a.offset(4,1,0x58);a.mask(0,18,0xfffffffc,false,2);a.offset(3,1,0x58);
        a.indexed_load(4,4,5);a.indexed_load(0,3,0);a.mask(3,6,0xfffffffc,false,2);a.mask(6,4,0xfffffffe,false,1);
        a.offset(5,1,0x38);a.mask(0,0,0xfffffffe,false,1);a.offset(7,1,0x39);a.indexed_load(3,28,3);a.move(4,27);
        a.indexed_load<1>(6,5,6);a.move(5,26);a.indexed_load<1>(7,7,0);a.call(0x10728ac0);
    }
    a.at(0x10727d10);a.value(23,0x0);a.jump(0x10727d2c);
    for (;;) {
        a.at(0x10727d2c);a.indexed_load<1>(0,3,23);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x10727d18);a.indexed_load<1>(4,3,23);a.move(0,24);a.offset(24,24,0x1);a.offset(23,23,0x1);
        a.indexed_store<1>(4,31,0);
    }
    a.at(0x10727d38);a.load(3,1,0x54);a.offset(25,25,0x1);a.offset(18,18,0x1);a.indexed_load<1>(0,3,25);a.sign8(0,0);
    a.compare(0,32);
    if (a.equal()) {
        a.at(0x10727d54);a.value(0,0x0);a.store<1>(27,0x0,0);
    }
}

void render_translation(SemanticGuestCall& a) {
    a.at(0x10727b4c);a.value(0,0x0);a.value(18,0x0);a.store<1>(27,0x0,0);a.value(25,0x0);a.jump(0x10727db4);
    for (;;) {
        a.at(0x10727db4);a.load(3,1,0x54);a.indexed_load<1>(0,3,25);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x10727b60);a.load(3,1,0x54);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,32);
        if (a.equal()) {
            a.at(0x10727b74);a.value(0,0x0);a.store<1>(27,0x0,0);
        }
        a.at(0x10727b7c);a.load(3,1,0x54);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,123);
        if (a.equal()) {
            a.at(0x10727b90);a.value(23,0x0);a.offset(25,25,0x1);a.jump(0x10727bb0);
            for (;;) {
                a.at(0x10727bb0);a.load(3,1,0x54);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,125);
                if (a.equal()) break;
                a.at(0x10727bc4);a.load(3,1,0x54);a.indexed_load<1>(0,3,25);a.sign8(0,0,true);
                if (a.equal()) break;
                a.at(0x10727b9c);a.load(3,1,0x54);a.indexed_load<1>(0,3,25);a.offset(25,25,0x1);
                a.indexed_store<1>(0,27,23);a.offset(23,23,0x1);
            }
            a.at(0x10727bd4);a.value(0,0x0);a.offset(25,25,0x1);a.indexed_store<1>(0,27,23);
        }
        a.at(0x10727be0);a.load(3,1,0x54);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,8);
        if (a.equal()) append_translated_argument(a);
        a.at(0x10727d5c);a.load(3,1,0x54);a.indexed_load<1>(0,3,25);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x10727d6c);a.load(3,1,0x54);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,37);
        bool doubled = a.equal();
        if (doubled) {
            a.at(0x10727d80);a.sign8(0,30);a.compare(0,37);
            doubled = a.equal();
        }
        if (doubled) {
            a.at(0x10727d8c);a.value(30,0x20);a.jump(0x10727db0);
        } else {
            a.at(0x10727d94);a.load(4,1,0x54);a.move(0,24);a.load(3,1,0x54);a.offset(24,24,0x1);
            a.indexed_load<1>(30,4,25);a.indexed_load<1>(3,3,25);a.indexed_store<1>(3,31,0);
        }
        a.at(0x10727db0);a.offset(25,25,0x1);
    }
    a.at(0x10727dc4);a.value(0,0x0);a.indexed_store<1>(0,31,24);
}

// Marker syntax consumes modifier arguments but is removed before base formatting.
// Keep the two original marker passes distinct, including their byte-width choices.
bool strip_initial_marker(SemanticGuestCall& a) {
    a.at(0x10727dec);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,123);
    bool opening = a.equal();
    if (!opening) {
        a.at(0x10727dfc);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,91);
        opening = a.equal();
    }
    if (opening) {
        a.at(0x10727e0c);a.load(0,2,-0x35a8);a.compare(0,0);
        if (!a.equal()) {
            a.at(0x10727e18);a.value(0,0x0);a.jump(0x10727e2c);
        } else {
            a.at(0x10727e20);a.load(0,22,0x0);a.offset(22,22,0x4);a.sign8(0,0);
        }
        a.at(0x10727e2c);a.add(3,19,25);a.store<1>(1,0x38,0);a.load<1>(0,3,0x1);a.compare(0,125);
        if (a.equal()) return true;
        a.at(0x10727e40);a.add(3,19,25);a.load<1>(0,3,0x1);a.compare(0,93);
        if (a.equal()) return true;
    }
    a.at(0x10727e50);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,125);
    bool closing = a.equal();
    if (!closing) {
        a.at(0x10727e60);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,93);
        closing = a.equal();
    }
    if (closing) {
        a.at(0x10727e70);a.compare(25,0);
        if (a.equal()) return true;
        a.at(0x10727e78);a.add(3,19,25);a.load<1>(0,3,-0x1);a.compare(0,123);
        if (a.equal()) return true;
        a.at(0x10727e88);a.add(3,19,25);a.load<1>(0,3,-0x1);a.compare(0,91);
        if (a.equal()) return true;
    }
    return false;
}

bool strip_following_marker(SemanticGuestCall& a) {
    a.at(0x10727f14);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,123);
    bool opening = a.equal();
    if (!opening) {
        a.at(0x10727f24);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,91);
        opening = a.equal();
    }
    if (opening) {
        a.at(0x10727f34);a.load(0,2,-0x35a8);a.compare(0,0);
        if (a.equal()) {
            a.at(0x10727f40);a.load(0,22,0x0);a.offset(22,22,0x4);a.store<1>(1,0x38,0);
        }
        a.at(0x10727f4c);a.add(3,19,25);a.load<1>(0,3,0x1);a.compare(0,125);
        if (a.equal()) return true;
        a.at(0x10727f5c);a.add(3,19,25);a.load<1>(0,3,0x1);a.compare(0,93);
        if (a.equal()) return true;
    }
    a.at(0x10727f6c);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,125);
    bool closing = a.equal();
    if (!closing) {
        a.at(0x10727f7c);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,93);
        closing = a.equal();
    }
    if (closing) {
        a.at(0x10727f8c);a.compare(25,0);
        if (a.equal()) return true;
        a.at(0x10727f94);a.add(3,19,25);a.load<1>(0,3,-0x1);a.compare(0,123);
        if (a.equal()) return true;
        a.at(0x10727fa4);a.add(3,19,25);a.load<1>(0,3,-0x1);a.compare(0,91);
        if (a.equal()) return true;
    }
    return false;
}

void prepare_fallback(SemanticGuestCall& a) {
    a.at(0x10727dd4);a.load(0,2,-0x35a8);a.compare(0,0);
    if (a.equal()) {
        a.at(0x10727de0);a.offset(22,1,0x600);
    }
    a.at(0x10727de4);a.value(25,0x0);a.jump(0x10727fd4);
    for (;;) {
        a.at(0x10727fd4);a.indexed_load<1>(0,19,25);a.sign8(0,0,true);
        if (a.equal()) break;
        bool skip = strip_initial_marker(a);
        if (!skip) {
            a.at(0x10727e98);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,60);
            if (a.equal()) {
                a.at(0x10727ea8);a.jump(0x10727eb0);
                for (;;) {
                    a.at(0x10727eb0);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,37);
                    if (a.equal()) break;
                    a.at(0x10727ec0);a.indexed_load<1>(0,19,25);a.sign8(0,0,true);
                    if (a.equal()) break;
                    a.at(0x10727eac);a.offset(25,25,0x1);
                }
                a.at(0x10727ecc);a.value(21,0x1);
            }
            a.at(0x10727ed0);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,32);
            bool skip_angle = a.equal();
            if (skip_angle) {
                a.at(0x10727ee0);a.sign8(0,21,true);
                skip_angle = !a.equal();
            }
            if (skip_angle) {
                a.at(0x10727ee8);a.jump(0x10727ef0);
                for (;;) {
                    a.at(0x10727ef0);a.indexed_load<1>(0,19,25);a.sign8(0,0);a.compare(0,62);
                    if (a.equal()) break;
                    a.at(0x10727f00);a.indexed_load<1>(0,19,25);a.sign8(0,0,true);
                    if (a.equal()) break;
                    a.at(0x10727eec);a.offset(25,25,0x1);
                }
                a.at(0x10727f0c);a.offset(25,25,0x1);a.value(21,0x0);
                skip = strip_following_marker(a);
            }
        }
        if (!skip) {
            a.at(0x10727fb4);a.indexed_load<1>(3,19,25);a.move(0,23);a.offset(23,23,0x1);a.indexed_store<1>(3,29,0);
            a.indexed_load<1>(0,19,25);a.sign8(0,0,true);
            if (a.equal()) break;
        }
        a.at(0x10727fd0);a.offset(25,25,0x1);
    }
    a.at(0x10727fe0);a.load(0,2,-0x35a8);a.value(3,0x0);a.indexed_store<1>(3,29,23);a.compare(0,0);
}

void rewrite_prefix_marker(SemanticGuestCall& a) {
    a.at(0x107280bc);a.value(3,0x0);a.jump(0x107280dc);
    for (;;) {
        a.at(0x107280dc);a.offset(25,25,0x1);a.offset(4,1,0x1ac);a.indexed_load<1>(0,4,25);a.sign8(0,0);
        a.compare(0,125);
        if (a.equal()) break;
        a.at(0x107280c4);a.offset(4,1,0x1ac);a.move(0,3);a.indexed_load<1>(5,4,25);a.offset(4,1,0x148);
        a.offset(3,3,0x1);a.indexed_store<1>(5,4,0);
    }
    a.at(0x107280f4);a.offset(4,1,0x148);a.value(0,0x0);a.indexed_store<1>(0,4,3);a.value(4,0x0);a.offset(25,25,0x1);
    a.jump(0x10728124);
    for (;;) {
        a.at(0x10728124);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,25);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x10728134);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,32);
        if (a.equal()) break;
        a.at(0x10728148);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,123);
        if (a.equal()) break;
        a.at(0x1072815c);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,91);
        if (a.equal()) break;
        a.at(0x1072810c);a.offset(5,1,0x1ac);a.offset(3,1,0x80);a.indexed_load<1>(0,5,25);a.offset(25,25,0x1);
        a.indexed_store<1>(0,3,4);a.offset(4,4,0x1);
    }
    a.at(0x10728170);a.offset(3,1,0x1ac);a.offset(5,1,0x80);a.indexed_load<1>(0,3,25);a.value(3,0x0);
    a.indexed_store<1>(3,5,4);a.sign8(0,0);a.compare(0,123);
    bool suffix = a.equal();
    if (!suffix) {
        a.at(0x10728190);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,91);
        suffix = a.equal();
    }
    if (suffix) {
        a.at(0x107281a4);a.value(3,0x0);a.jump(0x107281c4);
        for (;;) {
            a.at(0x107281c4);a.offset(25,25,0x1);a.offset(4,1,0x1ac);a.indexed_load<1>(0,4,25);a.sign8(0,0);
            a.compare(0,125);
            if (a.equal()) break;
            a.at(0x107281ac);a.offset(4,1,0x1ac);a.move(0,3);a.indexed_load<1>(5,4,25);a.offset(4,1,0xe4);
            a.offset(3,3,0x1);a.indexed_store<1>(5,4,0);
        }
        a.at(0x107281dc);a.offset(4,1,0xe4);a.value(0,0x0);a.indexed_store<1>(0,4,3);a.offset(25,25,0x1);
    }
    a.at(0x107281ec);a.offset(3,1,0x80);a.offset(4,1,0x148);a.offset(5,1,0xe4);a.value(6,0x64);a.value(7,0x64);
    a.call(0x10728ac0);a.value(4,0x0);a.jump(0x10728220);
    for (;;) {
        a.at(0x10728220);a.indexed_load<1>(0,3,4);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x1072820c);a.indexed_load<1>(5,3,4);a.move(0,24);a.offset(24,24,0x1);a.offset(4,4,0x1);
        a.indexed_store<1>(5,31,0);
    }
    a.at(0x1072822c);a.jump(0x10728380);
}

void rewrite_suffix_marker(SemanticGuestCall& a) {
    a.at(0x10728230);a.offset(4,25,0x1);a.value(3,0x0);a.jump(0x10728254);
    for (;;) {
        a.at(0x10728254);a.offset(25,25,0x1);a.offset(5,1,0x1ac);a.indexed_load<1>(0,5,25);a.sign8(0,0);
        a.compare(0,125);
        if (a.equal()) break;
        a.at(0x1072823c);a.offset(5,1,0x1ac);a.move(0,3);a.indexed_load<1>(6,5,25);a.offset(5,1,0xe4);
        a.offset(3,3,0x1);a.indexed_store<1>(6,5,0);
    }
    a.at(0x1072826c);a.offset(5,1,0xe4);a.value(0,0x0);a.indexed_store<1>(0,5,3);a.offset(25,25,0x1);
    a.jump(0x10728288);
    for (;;) {
        a.at(0x10728288);a.compare(4,0);
        if (a.equal()) break;
        a.at(0x10728290);a.compare(24,0);
        if (a.equal()) break;
        a.at(0x10728298);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,4);a.sign8(0,0);a.compare(0,32);
        if (a.equal()) break;
        a.at(0x107282ac);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,4);a.sign8(0,0);a.compare(0,10);
        if (a.equal()) break;
        a.at(0x10728280);a.offset(4,4,-0x1);a.offset(24,24,-0x1);
    }
    a.at(0x107282c0);a.value(6,0x0);a.offset(4,4,-0x1);a.jump(0x107282ec);
    for (;;) {
        a.at(0x107282ec);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,4);a.sign8(0,0);a.compare(0,123);
        if (a.equal()) break;
        a.at(0x10728300);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,4);a.sign8(0,0);a.compare(0,91);
        if (a.equal()) break;
        a.at(0x107282cc);a.move(0,4);a.offset(3,1,0x1ac);a.indexed_load<1>(5,3,0);a.move(0,6);a.offset(3,1,0x80);
        a.offset(6,6,0x1);a.indexed_store<1>(5,3,0);a.offset(4,4,0x1);
    }
    a.at(0x10728314);a.offset(3,1,0x80);a.value(0,0x0);a.indexed_store<1>(0,3,6);a.offset(3,1,0x80);
    a.offset(4,1,0x148);a.offset(5,1,0xe4);a.value(6,0x64);a.value(7,0x64);a.call(0x10728ac0);a.value(4,0x0);
    a.jump(0x10728354);
    for (;;) {
        a.at(0x10728354);a.indexed_load<1>(0,3,4);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x10728340);a.indexed_load<1>(5,3,4);a.move(0,24);a.offset(24,24,0x1);a.offset(4,4,0x1);
        a.indexed_store<1>(5,31,0);
    }
    a.at(0x10728360);a.jump(0x10728380);
}

void rewrite_fallback_markers(SemanticGuestCall& a) {
    a.at(0x1072803c);a.move(4,31);a.offset(3,1,0x1ac);a.call(0x1077dbf0);a.value(25,0x0);a.value(24,0x0);
    a.jump(0x10728380);
    for (;;) {
        a.at(0x10728380);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,25);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x10728054);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,123);
        bool marker = a.equal();
        if (!marker) {
            a.at(0x10728068);a.offset(3,1,0x1ac);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,91);
            marker = a.equal();
        }
        if (marker) {
            a.at(0x1072807c);a.value(3,0x0);a.value(0,0x0);a.compare(25,0);a.store<1>(1,0x148,3);a.store<1>(1,0xe4,0);
            bool prefix = a.equal();
            if (!prefix) {
                a.at(0x10728094);a.offset(3,1,0x1ab);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,32);
                prefix = a.equal();
                if (!prefix) {
                    a.at(0x107280a8);a.offset(3,1,0x1ab);a.indexed_load<1>(0,3,25);a.sign8(0,0);a.compare(0,10);
                    prefix = a.equal();
                }
            }
            if (prefix) rewrite_prefix_marker(a);
            else rewrite_suffix_marker(a);
        } else {
            a.at(0x10728364);a.move(0,25);a.offset(3,1,0x1ac);a.indexed_load<1>(3,3,0);a.move(0,24);
            a.offset(25,25,0x1);a.offset(24,24,0x1);a.indexed_store<1>(3,31,0);
        }
    }
    a.at(0x10728390);a.value(0,0x0);a.indexed_store<1>(0,31,24);
}

void render_fallback(SemanticGuestCall& a) {
    prepare_fallback(a);
    if (a.equal()) {
        a.at(0x10727ff4);a.move(3,31);a.move(4,29);a.move(5,22);a.call(0x1077b6f0);a.move(3,31);a.offset(4,17,0x1b7);
        a.call(0x1077dda0);a.compare(3,0,false);
        bool markers = !a.equal();
        if (!markers) {
            a.at(0x10728018);a.move(3,31);a.offset(4,17,0x1b9);a.call(0x1077dda0);a.compare(3,0,false);
            markers = !a.equal();
        }
        if (markers) {
            a.at(0x1072802c);a.move(3,31);a.call(0x1077dbd0);a.compare(3,1000,false);
            if (!a.at_least()) rewrite_fallback_markers(a);
        }
    }
    a.at(0x10728398);a.load<1>(0,2,-0x35ac);a.sign8(0,0,true);
}
}

void format_phrase(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    SemanticGuestCall a(s, format_phrase_entry, d, o);
    a.at(0x10727890);a.link();a.multiple(17);a.value(11,0x0);a.load(26,2,-0x3eb0);a.load(27,2,-0x3eac);
    a.value(18,0x0);a.load(28,2,-0x3ea8);a.value(24,0x0);a.load(29,2,-0x3ea0);a.value(23,0x0);a.load(17,2,-0x3e98);
    a.value(21,0x0);a.value(20,0x1);a.value(30,0x20);a.value(25,0x0);a.store(1,0x8,0);a.value(0,0x0);a.frame(0x5e0);
    a.store(1,0x5fc,4);a.value(4,0x0);a.store(1,0x600,5);a.store(1,0x604,6);a.store(1,0x608,7);a.store(1,0x60c,8);
    a.store(1,0x610,9);a.store(1,0x614,10);a.store(1,0x4c,11);a.store(1,0x50,4);a.store(1,0x54,0);a.jump(0x10727914);
    for (;;) {
        a.at(0x10727914);a.load(4,1,0x5fc);a.indexed_load<1>(0,4,25);a.sign8(0,0);a.compare(0,32);
        if (!a.equal()) break;
        a.at(0x10727908);a.value(0,0x20);a.indexed_store<1>(0,3,25);a.offset(25,25,0x1);
    }
    a.at(0x10727928);a.load<1>(0,2,-0x35ac);a.add(31,3,25);a.load(3,1,0x5fc);a.sign8(0,0);a.compare(0,-1);
    a.add(19,3,25);
    bool translated = !a.equal();
    if (translated) {
        a.at(0x10727944);a.load<1>(0,2,-0x35ac);a.sign8(0,0,true);
        translated = !a.equal();
        if (translated) {
            a.at(0x10727950);a.move(4,19);a.offset(3,1,0x4c);a.value(5,0x0);a.call(0x10728400);a.sign8(0,3,true);
            translated = !a.equal();
            if (translated) {
                a.at(0x10727968);a.load(3,1,0x54);a.load<1>(0,3,0x0);a.sign8(0,0,true);
                translated = !a.equal();
            }
        }
    }
    if (translated) {
        collect_bindings(a);
        split_arguments(a);
        render_translation(a);
    }
    a.at(0x10727dcc);a.sign8(0,20,true);
    if (!a.equal()) render_fallback(a);
    a.at(0x107283a0);a.load(0,1,0x4c);a.move(17,31);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x107283b0);a.load(3,1,0x4c);a.call(0x10773610);
    }
    a.at(0x107283b8);a.load(0,1,0x50);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x107283c4);a.load(3,1,0x50);a.call(0x10773610);
    }
    a.at(0x107283cc);a.load(0,1,0x54);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x107283d8);a.load(3,1,0x54);a.call(0x10773610);
    }
    a.at(0x107283e0);a.move(3,17);a.load(0,1,0x5e8);a.offset(1,1,0x5e0);a.restore_link();a.multiple(17,true);
    a.finish();
}
}

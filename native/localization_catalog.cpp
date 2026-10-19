#include "localization_catalog.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::localization_catalog {
namespace {
// The original compacts braces in place before selecting the translation column.
void remove_key_braces(SemanticGuestCall& a) {
    a.at(0x10727440);a.value(4,0x0);a.move(3,4);a.jump(0x107274b8);
    for (;;) {
        a.at(0x107274b8);a.multiply(0,31,0x34);a.load(5,2,-0x3590);a.indexed_load(5,5,0);a.indexed_load<1>(0,5,4);
        a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x1072744c);a.multiply(0,31,0x34);a.load(5,2,-0x3590);a.indexed_load(5,5,0);a.indexed_load<1>(0,5,4);
        a.sign8(0,0);a.compare(0,123);
        bool brace = a.equal();
        if (!brace) {
            a.at(0x10727468);a.multiply(0,31,0x34);a.load(5,2,-0x3590);a.indexed_load(5,5,0);a.indexed_load<1>(0,5,4);
            a.sign8(0,0);a.compare(0,125);
            brace = a.equal();
        }
        if (brace) {
            a.at(0x10727484);a.offset(4,4,0x1);a.jump(0x107274b8);
        } else {
            a.at(0x1072748c);a.multiply(7,31,0x34);a.load(8,2,-0x3590);a.move(0,3);a.load(6,2,-0x3590);
            a.offset(3,3,0x1);a.multiply(5,31,0x34);a.indexed_load(7,8,7);a.indexed_load<1>(7,7,4);a.offset(4,4,0x1);
            a.indexed_load(5,6,5);a.indexed_store<1>(7,5,0);
        }
    }
    a.at(0x107274d0);a.multiply(0,31,0x34);a.load(4,2,-0x3590);a.value(5,0x0);a.value(18,0x1);a.indexed_load(4,4,0);
    a.indexed_store<1>(5,4,3);a.jump(0x10727518);
}

// Retain individual live pointer reloads while reading and byte-reordering fields.
void read_catalog_row(SemanticGuestCall& a) {
    a.at(0x10727390);a.move(3,29);a.move(6,30);a.value(4,0x1);a.value(5,0x226);a.call(0x10775fe0);
    a.multiply(0,31,0x34);a.load(3,2,-0x3590);a.add(18,3,0);a.load(0,18,0x0);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x107273bc);a.load(3,18,0x0);a.call(0x10773610);
    }
    a.at(0x107273c4);a.move(3,29);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
    a.call(0x107735f0);a.store(18,0x0,3);a.move(4,29);a.load(3,18,0x0);a.call(0x1077dbf0);a.move(3,29);a.move(6,30);
    a.value(4,0x1);a.value(5,0x1e);a.call(0x10775fe0);a.multiply(0,31,0x34);a.load(3,2,-0x3590);a.add(18,3,0);
    a.load(0,18,0x4);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x10727414);a.load(3,18,0x4);a.call(0x10773610);
    }
    a.at(0x1072741c);a.move(3,29);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
    a.call(0x107735f0);a.store(18,0x4,3);a.move(4,29);a.load(3,18,0x4);a.call(0x1077dbf0);
    remove_key_braces(a);
    for (;;) {
        a.at(0x10727518);a.load(0,2,-0x35a0);a.compare_registers(18,0);
        if (!a.less()) break;
        a.at(0x107274ec);a.move(3,29);a.move(6,30);a.value(4,0x1);a.value(5,0x226);a.call(0x10775fe0);a.move(3,29);
        a.move(6,30);a.value(4,0x4);a.value(5,0xa);a.call(0x10775fe0);a.offset(18,18,0x1);
    }
    a.at(0x10727524);a.move(3,29);a.move(6,30);a.value(4,0x1);a.value(5,0x226);a.call(0x10775fe0);
    a.multiply(0,31,0x34);a.load(3,2,-0x3590);a.add(18,3,0);a.load(0,18,0x8);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x10727550);a.load(3,18,0x8);a.call(0x10773610);
    }
    a.at(0x10727558);a.move(3,29);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
    a.call(0x107735f0);a.store(18,0x8,3);a.move(4,29);a.load(3,18,0x8);a.call(0x1077dbf0);a.value(19,0x0);
    a.jump(0x10727658);
    for (;;) {
        a.at(0x10727658);a.sign16(0,19);a.compare(0,10);
        if (!a.less()) break;
        a.at(0x10727584);a.multiply(3,31,0x34);a.sign16(0,19);a.load(5,2,-0x3590);a.move(6,30);
        a.mask(0,0,0xfffffffc,false,2);a.add(3,3,0);a.offset(3,3,0xc);a.value(4,0x4);a.add(3,5,3);a.value(5,0x1);
        a.call(0x10775fe0);a.sign8(0,28,true);
        if (a.equal()) {
            a.at(0x107275b8);a.multiply(5,31,0x34);a.load(9,2,-0x3590);a.sign16(6,19);a.load(8,2,-0x3590);
            a.sign16(4,19);a.load(18,2,-0x3590);a.add(10,9,5);a.mask(9,4,0xfffffffc,false,2);a.add(9,10,9);
            a.sign16(11,19);a.multiply(7,31,0x34);a.sign16(0,19);a.load(3,2,-0x3590);a.mask(6,6,0xfffffffc,false,2);
            a.mask(0,0,0xfffffffc,false,2);a.load(5,2,-0x3590);a.add(7,8,7);a.load(9,9,0xc);a.add(6,7,6);
            a.sign16(4,19);a.multiply(12,31,0x34);a.load(6,6,0xc);a.mask(7,11,0xfffffffc,false,2);
            a.mask(6,6,0xff0000,false,8);a.add(8,18,12);a.multiply(10,31,0x34);a.insert_mask(6,9,0xff000000,24);
            a.add(7,8,7);a.add(8,3,10);a.load(3,7,0xc);a.add(7,8,0);a.multiply(0,31,0x34);
            a.insert_mask(6,3,0xff00,24);a.load(7,7,0xc);a.add(3,5,0);a.insert_mask(6,7,0xff,8);
            a.mask(0,4,0xfffffffc,false,2);a.add(3,3,0);a.store(3,0xc,6);
        }
        a.at(0x10727654);a.offset(19,19,0x1);
    }
    a.at(0x10727664);a.load(3,2,-0x35a0);a.offset(18,3,0x1);a.jump(0x1072769c);
    for (;;) {
        a.at(0x1072769c);a.load(0,2,-0x3598);a.compare_registers(18,0);
        if (!a.less()) break;
        a.at(0x10727670);a.move(3,29);a.move(6,30);a.value(4,0x1);a.value(5,0x226);a.call(0x10775fe0);a.move(3,29);
        a.move(6,30);a.value(4,0x4);a.value(5,0xa);a.call(0x10775fe0);a.offset(18,18,0x1);
    }
}

// Source keys ignore spaces and decode newline escapes, then replace their storage.
void normalize_catalog_key(SemanticGuestCall& a) {
    a.at(0x107276fc);a.value(18,0x0);a.value(19,0x0);a.jump(0x107277a4);
    for (;;) {
        a.at(0x107277a4);a.multiply(3,31,0x34);a.load(4,2,-0x3590);a.sign16(0,19);a.indexed_load(3,4,3);
        a.indexed_load<1>(0,3,0);a.sign8(0,0,true);
        if (a.equal()) break;
        a.at(0x10727708);a.multiply(3,31,0x34);a.load(4,2,-0x3590);a.sign16(0,19);a.indexed_load(3,4,3);
        a.indexed_load<1>(0,3,0);a.sign8(0,0);a.compare(0,32);
        if (!a.equal()) {
            a.at(0x10727728);a.multiply(3,31,0x34);a.load(4,2,-0x3590);a.sign16(0,19);a.indexed_load(3,4,3);
            a.indexed_load<1>(0,3,0);a.sign8(0,0);a.compare(0,92);
            bool newline = a.equal();
            if (newline) {
                a.at(0x10727748);a.multiply(3,31,0x34);a.load(4,2,-0x3590);a.sign16(0,19);a.indexed_load(3,4,3);
                a.add(3,3,0);a.load<1>(0,3,0x1);a.compare(0,110);
                newline = a.equal();
            }
            if (newline) {
                a.at(0x10727768);a.move(0,18);a.value(3,0xa);a.indexed_store<1>(3,29,0);a.offset(18,18,0x1);
                a.offset(19,19,0x1);a.jump(0x107277a0);
            } else {
                a.at(0x10727780);a.multiply(4,31,0x34);a.load(5,2,-0x3590);a.move(0,18);a.sign16(3,19);
                a.indexed_load(4,5,4);a.offset(18,18,0x1);a.indexed_load<1>(3,4,3);a.indexed_store<1>(3,29,0);
            }
        }
        a.at(0x107277a0);a.offset(19,19,0x1);
    }
    a.at(0x107277c0);a.multiply(0,31,0x34);a.load(3,2,-0x3590);a.value(4,0x0);a.indexed_store<1>(4,29,18);
    a.add(18,3,0);a.load(0,18,0x0);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x107277e0);a.load(3,18,0x0);a.call(0x10773610);
    }
    a.at(0x107277e8);a.move(3,29);a.call(0x1077dbd0);a.offset(0,3,0x1);a.mask(3,0,0xffffffff,false,0);
    a.call(0x107735f0);a.store(18,0x0,3);a.move(4,29);a.load(3,18,0x0);a.call(0x1077dbf0);
}

void report_catalog_progress(SemanticGuestCall& a) {
    a.at(0x107276a8);a.sign8(0,24,true);
    if (!a.equal()) {
        a.at(0x107276b0);a.value(3,0x10620000);a.offset(3,3,0x4dd3);a.multiply_high(0,3,31);a.shift_right(0,0,4);
        a.mask(3,0,0x1,false,1);a.add(0,0,3);a.multiply(0,0,0xfa);a.subtract(0,0,31,true);
        bool update = a.equal();
        if (!update) {
            a.at(0x107276d4);a.load(3,2,-0x3594);a.offset(0,3,-0x1);a.compare_registers(31,0);
            update = a.equal();
        }
        if (update) {
            a.at(0x107276e4);a.load(6,2,-0x3594);a.move(3,21);a.offset(4,1,0xe0);a.offset(5,31,0x1);
            a.value(7,0xffffffff);a.call(0x10743120);
        }
    }
}

void close_and_refresh(SemanticGuestCall& a) {
    a.at(0x10727830);a.move(3,30);a.call(0x107766f0);a.call(0x1076fa20);a.value(3,0x1);
}

// Parse all advertised IDs. Version two omits 0/1 from the available-language
// list while retaining each selected column's physical position in the file.
void read_language_ids(SemanticGuestCall& a) {
    a.at(0x10727158);a.value(3,0xffffffff);a.value(0,0x0);a.store(2,-0x35a0,3);a.value(31,0x0);a.store(2,-0x35a4,0);
    a.jump(0x1072720c);
    for (;;) {
        a.at(0x1072720c);a.load(0,2,-0x3598);a.compare_registers(31,0);
        if (!a.less()) break;
        a.at(0x10727170);a.move(6,30);a.offset(3,1,0x38);a.value(4,0x4);a.value(5,0x1);a.call(0x10775fe0);
        a.sign8(0,28,true);
        if (a.equal()) {
            a.at(0x1072718c);a.load(0,1,0x38);a.load(3,1,0x38);a.mask(0,0,0xff0000,false,8);a.load(4,1,0x38);
            a.insert_mask(0,3,0xff000000,24);a.load(5,1,0x38);a.insert_mask(0,4,0xff00,24);a.insert_mask(0,5,0xff,8);
            a.store(1,0x38,0);
        }
        a.at(0x107271b0);a.load(0,1,0x3c);a.load(4,1,0x38);a.load(3,2,-0x35a4);a.compare(0,2);
        a.indexed_store<1>(4,26,3);
        bool omit = false;
        if (a.equal()) {
            a.at(0x107271c8);a.load(0,1,0x38);a.compare(0,0);
            omit = a.equal();
            if (!omit) {
                a.at(0x107271d4);a.load(0,1,0x38);a.compare(0,1);
                omit = a.equal();
            }
        }
        if (!omit) {
            a.at(0x107271e0);a.load(3,2,-0x35a4);a.sign8(0,27);a.indexed_load<1>(3,26,3);a.sign8(3,3);
            a.compare_registers(3,0);
            if (a.equal()) {
                a.at(0x107271f8);a.store(2,-0x35a0,31);
            }
            a.at(0x107271fc);a.load(3,2,-0x35a4);a.offset(0,3,0x1);a.store(2,-0x35a4,0);
        }
        a.at(0x10727208);a.offset(31,31,0x1);
    }
}

// Error paths keep their original messages, status stores, stream ownership and
// return registers. Recursion uses the complete original initializer boundary.
void load_catalog(SemanticGuestCall& a) {
    a.at(0x10726f3c);a.sign8(0,24,true);
    if (!a.equal()) {
        a.at(0x10726f44);a.offset(3,1,0xe0);a.offset(4,25,0xc6);a.call(0x10727890);a.jump(0x10726f5c);
    } else {
        a.at(0x10726f54);a.value(0,0x0);a.store<1>(1,0xe0,0);
    }
    a.at(0x10726f5c);a.load(0,2,-0x3590);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x10726f68);a.load(3,2,-0x3590);a.move(4,20);a.call(0x107734a0);a.value(0,0x0);a.store(2,-0x3590,0);
    }
    a.at(0x10726f7c);a.value(0,0xffffffff);a.load(3,2,-0x3e90);a.store<1>(2,-0x35ac,0);a.offset(4,25,0xd8);
    a.call(0x10739de0);a.move(30,3,true);
    if (a.equal()) {
        a.at(0x10726f98);a.offset(3,25,0x92);a.offset(6,1,0xa0);a.offset(7,1,0x50);a.value(4,0x0);a.value(5,0x0);
        a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,22);a.offset(4,25,0x9e);a.offset(6,1,0xa0);
        a.offset(7,1,0x50);a.value(8,0x1b8);a.call(0x1077b720);a.move(4,22);a.offset(3,25,0xab);a.value(5,0x0);
        a.call(0x10769b60);a.value(0,0x0);a.move(3,19);a.store<1>(18,0x0,0);a.offset(4,25,0xdb);a.call(0x1077dbf0);
        a.value(3,0x0);a.jump(0x10727840);
        return;
    }
    a.at(0x10726ffc);a.move(3,30);a.call(0x107464d0);a.value(4,0xf2000000);a.move(6,30);a.offset(4,4,-0xf);
    a.value(5,0x1);a.subtract(0,3,4);a.offset(3,1,0x3c);a.leading_zeroes(0,0);a.value(4,0x4);
    a.mask(0,0,0x7ffffff,false,27);a.sign8(28,0);a.call(0x10775fe0);a.sign8(0,28,true);
    if (a.equal()) {
        a.at(0x10727038);a.load(0,1,0x3c);a.load(3,1,0x3c);a.mask(0,0,0xff0000,false,8);a.load(4,1,0x3c);
        a.insert_mask(0,3,0xff000000,24);a.load(5,1,0x3c);a.insert_mask(0,4,0xff00,24);a.insert_mask(0,5,0xff,8);
        a.store(1,0x3c,0);
    }
    a.at(0x1072705c);a.load(0,1,0x3c);a.compare(0,1);
    bool supported = a.equal();
    if (!supported) {
        a.at(0x10727068);a.load(0,1,0x3c);a.compare(0,2);
        supported = a.equal();
    }
    if (!supported) {
        a.at(0x10727074);a.offset(3,25,0x92);a.offset(6,1,0x80);a.offset(7,1,0x48);a.value(4,0x0);a.value(5,0x0);
        a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,22);a.offset(4,25,0x9e);a.offset(6,1,0x80);
        a.offset(7,1,0x48);a.value(8,0x1c8);a.call(0x1077b720);a.move(4,22);a.offset(3,25,0xab);a.value(5,0x0);
        a.call(0x10769b60);a.value(0,0x0);a.move(3,19);a.store<1>(18,0x0,0);a.offset(4,25,0xf8);a.call(0x1077dbf0);
        a.move(3,30);a.call(0x107766f0);a.value(3,0x0);a.jump(0x10727840);
        return;
    }
    a.at(0x107270e0);a.move(6,30);a.offset(3,2,-0x3594);a.value(4,0x4);a.value(5,0x1);a.call(0x10775fe0);a.move(6,30);
    a.offset(3,2,-0x3598);a.value(4,0x4);a.value(5,0x1);a.call(0x10775fe0);a.sign8(0,28,true);
    if (a.equal()) {
        a.at(0x10727110);a.load(4,2,-0x3594);a.load(0,2,-0x3598);a.mask(6,4,0xff0000,false,8);a.load(7,2,-0x3594);
        a.load(3,2,-0x3598);a.mask(0,0,0xff0000,false,8);a.load(8,2,-0x3594);a.insert_mask(6,7,0xff000000,24);
        a.load(4,2,-0x3598);a.insert_mask(0,3,0xff000000,24);a.load(9,2,-0x3594);a.insert_mask(6,8,0xff00,24);
        a.load(5,2,-0x3598);a.insert_mask(0,4,0xff00,24);a.insert_mask(6,9,0xff,8);a.insert_mask(0,5,0xff,8);
        a.store(2,-0x3594,6);a.store(2,-0x3598,0);
    }
    read_language_ids(a);
    a.at(0x10727218);a.load(0,2,-0x35a0);a.compare(0,-1);
    if (a.equal()) {
        a.at(0x10727224);a.move(3,19);a.offset(4,25,0x114);a.call(0x1077dbf0);a.move(3,30);a.call(0x107766f0);
        a.load(0,2,-0x35a4);a.compare(0,0);
        if (!a.equal()) {
            a.at(0x10727244);a.load<1>(0,26,0x0);a.move(4,24);a.load<1>(3,26,0x0);a.store<1>(2,-0x35ac,0);
            a.call(0x10726e90);a.jump(0x10727840);
        } else {
            a.at(0x1072725c);a.value(3,0x0);a.jump(0x10727840);
        }
        return;
    }
    a.at(0x10727264);a.load<1>(0,2,-0x359c);a.store<1>(2,-0x35ac,27);a.compare(0,1,false);
    if (!a.equal()) {
        close_and_refresh(a);
        return;
    }
    a.at(0x10727274);a.sign8(0,27,true);
    if (a.equal()) {
        close_and_refresh(a);
        return;
    }
    a.at(0x1072727c);a.load(26,2,-0x3594);a.value(27,0x0);a.multiply(3,26,0x34);a.offset(3,3,0x10);a.call(0x107735f0);
    a.move(23,3,true);
    if (!a.equal()) {
        a.at(0x10727298);a.load(4,2,-0x5bcc);a.value(27,0x1);a.move(3,23);a.move(5,20);a.move(7,26);a.value(6,0x34);
        a.call(0x10773150);a.value(0,0x0);a.move(23,3);a.sign8(0,0,true);
    }
    a.at(0x107272c0);a.move(0,23);a.store(2,-0x3590,0);a.load(0,2,-0x3590);a.compare(0,0,false);
    if (a.equal()) {
        a.at(0x107272d4);a.offset(3,25,0x92);a.offset(6,1,0x60);a.offset(7,1,0x40);a.value(4,0x0);a.value(5,0x0);
        a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,22);a.offset(4,25,0x9e);a.offset(6,1,0x60);
        a.offset(7,1,0x40);a.value(8,0x20e);a.call(0x1077b720);a.move(4,22);a.offset(3,25,0xab);a.value(5,0x0);
        a.call(0x10769b60);a.value(0,0x0);a.move(3,19);a.store<1>(18,0x0,0);a.offset(4,25,0x13d);a.call(0x1077dbf0);
        a.load(0,2,-0x3590);a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x1072733c);a.load(3,2,-0x3590);a.move(4,20);a.call(0x107734a0);a.value(0,0x0);a.store(2,-0x3590,0);
        }
        a.at(0x10727350);a.value(0,0xffffffff);a.move(3,30);a.store<1>(2,-0x35ac,0);a.call(0x107766f0);a.value(3,0x0);
        a.jump(0x10727840);
        return;
    }
    a.at(0x10727368);a.sign8(0,24,true);
    if (!a.equal()) {
        a.at(0x10727370);a.load(6,2,-0x3594);a.move(3,21);a.offset(4,1,0xe0);a.value(5,0x0);a.value(7,0xffffffff);
        a.call(0x10743120);
    }
    a.at(0x10727388);a.value(31,0x0);a.jump(0x10727810);
    for (;;) {
        a.at(0x10727810);a.load(0,2,-0x3594);a.compare_registers(31,0);
        if (!a.less()) break;
        read_catalog_row(a);
        report_catalog_progress(a);
        normalize_catalog_key(a);
        a.at(0x1072780c);a.offset(31,31,0x1);
    }
    a.at(0x1072781c);a.load(3,2,-0x3590);a.value(5,0x34);a.load(4,2,-0x3594);a.load(6,2,-0x3ea4);a.call(0x10768300);
    close_and_refresh(a);
}
}

void release_catalog(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    SemanticGuestCall a(s,release_catalog_entry,d,o);
    a.at(0x10726cc0);a.link();a.store(1,0x8,0);a.frame(0x40);a.load(0,2,-0x3590);a.compare(0,0,false);
    if (!a.equal()) {
        a.at(0x10726cd8);a.load(3,2,-0x3590);a.load(4,2,-0x5bc8);a.call(0x107734a0);a.value(0,0x0);
        a.store(2,-0x3590,0);
    }
    a.at(0x10726cec);a.value(0,0xffffffff);a.store<1>(2,-0x35ac,0);a.load(0,1,0x48);a.offset(1,1,0x40);
    a.restore_link();a.finish();
}

// Owned fields are released in order and remain in the record. The low signed
// halfword alone decides whether the record itself must also be deallocated.
void destroy_record(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    SemanticGuestCall a(s,destroy_record_entry,d,o);
    a.at(0x10726d10);a.link();a.store(1,-0x4,31);a.move(31,4);a.store(1,-0x8,30);a.move(30,3,true);a.store(1,0x8,0);
    a.frame(0x40);
    if (!a.equal()) {
        a.at(0x10726d30);a.load(0,30,0x0);a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x10726d3c);a.load(3,30,0x0);a.call(0x10773610);
        }
        a.at(0x10726d44);a.load(0,30,0x4);a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x10726d50);a.load(3,30,0x4);a.call(0x10773610);
        }
        a.at(0x10726d58);a.load(0,30,0x8);a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x10726d64);a.load(3,30,0x8);a.call(0x10773610);
        }
        a.at(0x10726d6c);a.sign16(0,31,true);
        if (!a.nonpositive()) {
            a.at(0x10726d74);a.move(3,30);a.call(0x107735a0);
        }
    }
    a.at(0x10726d7c);a.move(3,30);a.load(0,1,0x48);a.offset(1,1,0x40);a.restore_link();a.load(31,1,-0x4);
    a.load(30,1,-0x8);a.finish();
}

void language_availability(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    SemanticGuestCall a(s,language_availability_entry,d,o);
    a.at(0x10726da0);a.link();a.store(1,-0x4,31);a.load(5,2,-0x3e9c);a.store(1,-0x8,30);a.load(30,2,-0x7ba8);
    a.store(1,0x8,0);a.load(31,2,-0x3e98);a.frame(0x70);a.load<1>(0,2,-0x35ac);a.sign8(0,0);a.compare(0,-1);
    if (a.equal()) {
        a.at(0x10726dd0);a.offset(3,31,0x92);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0x0);a.value(5,0x0);
        a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,30);a.offset(4,31,0x9e);a.offset(6,1,0x40);
        a.offset(7,1,0x38);a.value(8,0x183);a.call(0x1077b720);a.move(4,30);a.offset(3,31,0xab);a.value(5,0x0);
        a.call(0x10769b60);a.load(5,2,-0x7bac);a.value(0,0x0);a.load(3,2,-0x3e8c);a.offset(4,31,0xb1);
        a.store<1>(5,0x0,0);a.call(0x1077dbf0);a.value(3,0x0);a.jump(0x10726e70);
    } else {
        a.at(0x10726e38);a.value(6,0x0);a.jump(0x10726e60);
        bool found = false;
        for (;;) {
            a.at(0x10726e60);a.load(0,2,-0x35a4);a.compare_registers(6,0);
            if (!a.less()) break;
            a.at(0x10726e40);a.indexed_load<1>(4,5,6);a.sign8(0,3);a.sign8(4,4);a.compare_registers(4,0);
            if (a.equal()) {
                a.at(0x10726e54);a.value(3,0x1);a.jump(0x10726e70);
                found = true;
                break;
            }
            a.at(0x10726e5c);a.offset(6,6,0x1);
        }
        if (!found) {
            a.at(0x10726e6c);a.value(3,0x0);
        }
    }
    a.at(0x10726e70);a.load(0,1,0x78);a.offset(1,1,0x70);a.restore_link();a.load(31,1,-0x4);a.load(30,1,-0x8);
    a.finish();
}

void initialize_catalog(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    SemanticGuestCall a(s,initialize_catalog_entry,d,o);
    a.at(0x10726e90);a.link();a.multiple(18);a.load(29,2,-0x3ea0);a.move(27,3);a.load(21,2,-0x7bb0);a.move(24,4);
    a.load(26,2,-0x3e9c);a.load(18,2,-0x7bac);a.load(22,2,-0x7ba8);a.load(25,2,-0x3e98);a.load(20,2,-0x5bc8);
    a.load(19,2,-0x3e8c);a.store(1,0x8,0);a.frame(0x1e0);a.load<1>(0,2,-0x35ac);a.sign8(0,0);a.compare(0,-1);
    if (a.equal()) {
        a.at(0x10726ed8);a.offset(3,25,0x92);a.offset(6,1,0xc0);a.offset(7,1,0x58);a.value(4,0x0);a.value(5,0x0);
        a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,22);a.offset(4,25,0x9e);a.offset(6,1,0xc0);
        a.offset(7,1,0x58);a.value(8,0x1a7);a.call(0x1077b720);a.move(4,22);a.offset(3,25,0xab);a.value(5,0x0);
        a.call(0x10769b60);a.value(0,0x0);a.move(3,19);a.store<1>(18,0x0,0);a.offset(4,25,0xb1);a.call(0x1077dbf0);
        a.value(3,0x0);a.jump(0x10727840);
    } else {
        load_catalog(a);
    }
    a.at(0x10727840);a.load(0,1,0x1e8);a.offset(1,1,0x1e0);a.restore_link();a.multiple(18,true);a.finish();
}
}

#include "money_configuration_detail.hpp"
namespace cm::aot::money_configuration::detail {
namespace {
// Original currency index 0; floating loads retain their original register reuse.
void profile_0(Call& a){
    a.at(0x1000aeec);
    a.floating_load(0,5,0x18);a.offset(4,30,0x13b);a.floating_store(0,2,-0x3aec);
    a.floating_store(0,2,-0x3af4);a.floating_store(0,2,-0x3afc);a.call(0x1077dbf0);
    copy_suffixes(a,0x1e,0x1f,0x21,0x23,0x25);
    distinct_separators(a,44,46,0);a.jump(0x1000b134);
}
// Original currency index 1; floating loads retain their original register reuse.
void profile_1(Call& a){
    a.at(0x1000a498);
    a.floating_load(2,5,0x8);a.offset(4,30,0x1c);a.floating_load(1,5,0x10);a.floating_load(0,5,0x18);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x1e,0x1f,0x21,0x23,0x25);
    shared_separator(a,46);a.jump(0x1000b134);
}
// Original currency index 2; floating loads retain their original register reuse.
void profile_2(Call& a){
    a.at(0x1000a514);
    a.floating_load(2,5,0x20);a.offset(4,30,0x1c);a.floating_load(1,5,0x28);a.floating_load(0,5,0x18);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x1e,0x1f,0x21,0x23,0x25);
    distinct_separators(a,44,46,0);a.jump(0x1000b134);
}
// Original currency index 3; floating loads retain their original register reuse.
void profile_3(Call& a){
    a.at(0x1000a594);
    a.floating_load(2,5,0x30);a.offset(4,30,0x1e);a.floating_load(1,5,0x38);a.floating_load(0,5,0x40);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x27,0x2b,0x31,0x37,0x3d);
    shared_separator(a,46);a.jump(0x1000b134);
}
// Original currency index 4; floating loads retain their original register reuse.
void profile_4(Call& a){
    a.at(0x1000a610);
    a.floating_load(2,5,0x48);a.offset(4,30,0x43);a.floating_load(1,5,0x50);a.floating_load(0,5,0x18);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x46,0x1f,0x21,0x23,0x25);
    distinct_separators(a,46,44,0);a.jump(0x1000b134);
}
// Original currency index 5; floating loads retain their original register reuse.
void profile_5(Call& a){
    a.at(0x1000a690);
    a.floating_load(0,5,0x60);a.offset(4,30,0x1e);a.floating_load(1,5,0x58);a.floating_store(0,2,-0x3af4);
    a.floating_store(1,2,-0x3aec);a.floating_store(0,2,-0x3afc);a.call(0x1077dbf0);
    copy_suffixes(a,0x4a,0x4e,0x51,0x57,0x5a);
    distinct_separators(a,46,44,1);a.jump(0x1000b134);
}
// Original currency index 6; floating loads retain their original register reuse.
void profile_6(Call& a){
    a.at(0x1000a70c);
    a.floating_load(0,5,0x60);a.offset(4,30,0x1e);a.floating_load(1,5,0x68);a.floating_store(0,2,-0x3af4);
    a.floating_store(1,2,-0x3aec);a.floating_store(0,2,-0x3afc);a.call(0x1077dbf0);
    copy_suffixes(a,0x5d,0x61,0x67,0x6d,0x73);
    distinct_separators(a,46,44,0);a.jump(0x1000b134);
}
// Original currency index 7; floating loads retain their original register reuse.
void profile_7(Call& a){
    a.at(0x1000a788);
    a.floating_load(2,5,0x70);a.offset(4,30,0x79);a.floating_load(1,5,0x10);a.floating_load(0,5,0x18);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x1e,0x1f,0x21,0x23,0x25);
    distinct_separators(a,44,46,0);a.jump(0x1000b134);
}
// Original currency index 8; floating loads retain their original register reuse.
void profile_8(Call& a){
    a.at(0x1000a808);
    a.floating_load(0,5,0x60);a.offset(4,30,0x1e);a.floating_load(1,5,0x78);a.floating_store(0,2,-0x3af4);
    a.floating_store(1,2,-0x3aec);a.floating_store(0,2,-0x3afc);a.call(0x1077dbf0);
    copy_suffixes(a,0x7b,0x1f,0x21,0x23,0x25);
    distinct_separators(a,44,46,1);a.jump(0x1000b134);
}
// Original currency index 9; floating loads retain their original register reuse.
void profile_9(Call& a){
    a.at(0x1000a884);
    a.floating_load(0,5,0x60);a.offset(4,30,0x1e);a.floating_load(1,5,0x80);a.floating_store(0,2,-0x3af4);
    a.floating_store(1,2,-0x3aec);a.floating_store(0,2,-0x3afc);a.call(0x1077dbf0);
    copy_suffixes(a,0x7f,0x83,0x87,0x27,0x8b);
    distinct_separators(a,46,44,1);a.jump(0x1000b134);
}
// Original currency index 10; floating loads retain their original register reuse.
void profile_10(Call& a){
    a.at(0x1000a900);
    a.floating_load(2,5,0x88);a.offset(4,30,0x1e);a.floating_load(1,5,0x50);a.floating_load(0,5,0x18);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x8f,0x93,0x99,0x9f,0xa5);
    distinct_separators(a,46,44,1);a.jump(0x1000b134);
}
// Original currency index 11; floating loads retain their original register reuse.
void profile_11(Call& a){
    a.at(0x1000a980);
    a.floating_load(2,5,0x90);a.offset(4,30,0xac);a.floating_load(1,5,0x50);a.floating_load(0,5,0x18);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0xaf,0x1f,0x21,0x23,0x25);
    shared_separator(a,46);a.jump(0x1000b134);
}
// Original currency index 12; floating loads retain their original register reuse.
void profile_12(Call& a){
    a.at(0x1000a9fc);
    a.floating_load(2,5,0x98);a.offset(4,30,0x1e);a.floating_load(1,5,0xa0);a.floating_load(0,5,0x40);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0xb2,0xb5,0xba,0x23,0x25);
    shared_separator(a,44);a.jump(0x1000b134);
}
// Original currency index 13; floating loads retain their original register reuse.
void profile_13(Call& a){
    a.at(0x1000aa78);
    a.floating_load(0,5,0x18);a.offset(4,30,0xbe);a.floating_load(1,5,0xa8);a.floating_store(0,2,-0x3af4);
    a.floating_store(1,2,-0x3aec);a.floating_store(0,2,-0x3afc);a.call(0x1077dbf0);
    copy_suffixes(a,0x1e,0xc3,0x21,0x23,0x25);
    distinct_separators(a,44,46,0);a.jump(0x1000b134);
}
// Original currency index 14; floating loads retain their original register reuse.
void profile_14(Call& a){
    a.at(0x1000aaf4);
    a.floating_load(2,5,0xb0);a.offset(4,30,0xc6);a.floating_load(1,5,0xb8);a.floating_load(0,5,0xc0);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x1e,0x1f,0xcc,0x21,0x25);
    distinct_separators(a,46,44,0);a.jump(0x1000b134);
}
// Original currency index 15; floating loads retain their original register reuse.
void profile_15(Call& a){
    a.at(0x1000ab74);
    a.floating_load(2,5,0xc8);a.offset(4,30,0xce);a.floating_load(1,5,0xd0);a.floating_load(0,5,0x40);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x1e,0x1f,0x21,0x23,0x25);
    distinct_separators(a,44,46,0);a.jump(0x1000b134);
}
// Original currency index 16; floating loads retain their original register reuse.
void profile_16(Call& a){
    a.at(0x1000abf4);
    a.floating_load(2,5,0xd8);a.offset(4,30,0xd0);a.floating_load(1,5,0xe0);a.floating_load(0,5,0x60);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x1e,0x4e,0xd4,0x57,0x5a);
    shared_separator(a,44);a.jump(0x1000b134);
}
// Original currency index 17; floating loads retain their original register reuse.
void profile_17(Call& a){
    a.at(0x1000ac70);
    a.floating_load(2,5,0xe8);a.offset(4,30,0x1e);a.floating_load(1,5,0xf0);a.floating_load(0,5,0x60);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0xd7,0xdb,0xe0,0x57,0x5a);
    shared_separator(a,46);a.jump(0x1000b134);
}
// Original currency index 18; floating loads retain their original register reuse.
void profile_18(Call& a){
    a.at(0x1000acec);
    a.floating_load(2,5,0xf8);a.offset(4,30,0x1e);a.floating_load(1,5,0x100);a.floating_load(0,5,0x18);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0xe5,0xe8,0xec,0xf0,0xf4);
    distinct_separators(a,46,44,0);a.jump(0x1000b134);
}
// Original currency index 19; floating loads retain their original register reuse.
void profile_19(Call& a){
    a.at(0x1000ad6c);
    a.floating_load(2,5,0x108);a.offset(4,30,0x1e);a.floating_load(1,5,0x38);a.floating_load(0,5,0x60);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0xf8,0xfb,0x100,0x57,0x5a);
    distinct_separators(a,44,46,0);a.jump(0x1000b134);
}
// Original currency index 20; floating loads retain their original register reuse.
void profile_20(Call& a){
    a.at(0x1000adec);
    a.floating_load(2,5,0x110);a.offset(4,30,0x1e);a.floating_load(1,5,0x118);a.floating_load(0,5,0x40);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x109,0x110,0x118,0x1e,0x120);
    distinct_separators(a,46,44,0);a.jump(0x1000b134);
}
// Original currency index 21; floating loads retain their original register reuse.
void profile_21(Call& a){
    a.at(0x1000ae6c);
    a.floating_load(2,5,0x120);a.offset(4,30,0x1e);a.floating_load(1,5,0xe0);a.floating_load(0,5,0x60);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x128,0x12c,0x131,0x136,0x12c);
    distinct_separators(a,32,44,0);a.jump(0x1000b134);
}
// Original currency index 22; floating loads retain their original register reuse.
void profile_22(Call& a){
    a.at(0x1000af64);
    a.floating_load(2,5,0x128);a.offset(4,30,0x1e);a.floating_load(1,5,0x28);a.floating_load(0,5,0x18);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x13d,0x4e,0x142,0x57,0x5a);
    distinct_separators(a,44,46,1);a.jump(0x1000b134);
}
// Original currency index 23; floating loads retain their original register reuse.
void profile_23(Call& a){
    a.at(0x1000afe4);
    a.floating_load(0,5,0x138);a.offset(4,30,0x1e);a.floating_load(1,5,0x130);a.floating_store(0,2,-0x3af4);
    a.floating_store(1,2,-0x3aec);a.floating_store(0,2,-0x3afc);a.call(0x1077dbf0);
    copy_suffixes(a,0x148,0x4e,0x14c,0x154,0x15c);
    shared_separator(a,46);a.jump(0x1000b134);
}
// Original currency index 24; floating loads retain their original register reuse.
void profile_24(Call& a){
    a.at(0x1000b05c);
    a.floating_load(2,5,0x140);a.offset(4,30,0x1c);a.floating_load(1,5,0x10);a.floating_load(0,5,0x18);
    a.floating_store(2,2,-0x3aec);a.floating_store(1,2,-0x3af4);a.floating_store(0,2,-0x3afc);
    a.call(0x1077dbf0);
    copy_suffixes(a,0x1e,0x1f,0x21,0x23,0x25);
    distinct_separators(a,44,46,0);a.jump(0x1000b134);
}
}
void apply_profile(Call& a,std::uint32_t destination){
    switch(destination){
    case 0x1000aeec:profile_0(a);return;
    case 0x1000a498:profile_1(a);return;
    case 0x1000a514:profile_2(a);return;
    case 0x1000a594:profile_3(a);return;
    case 0x1000a610:profile_4(a);return;
    case 0x1000a690:profile_5(a);return;
    case 0x1000a70c:profile_6(a);return;
    case 0x1000a788:profile_7(a);return;
    case 0x1000a808:profile_8(a);return;
    case 0x1000a884:profile_9(a);return;
    case 0x1000a900:profile_10(a);return;
    case 0x1000a980:profile_11(a);return;
    case 0x1000a9fc:profile_12(a);return;
    case 0x1000aa78:profile_13(a);return;
    case 0x1000aaf4:profile_14(a);return;
    case 0x1000ab74:profile_15(a);return;
    case 0x1000abf4:profile_16(a);return;
    case 0x1000ac70:profile_17(a);return;
    case 0x1000acec:profile_18(a);return;
    case 0x1000ad6c:profile_19(a);return;
    case 0x1000adec:profile_20(a);return;
    case 0x1000ae6c:profile_21(a);return;
    case 0x1000af64:profile_22(a);return;
    case 0x1000afe4:profile_23(a);return;
    case 0x1000b05c:profile_24(a);return;
    default:throw std::runtime_error("Unknown currency configuration destination");
    }
}
}

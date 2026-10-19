#include "match_target.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_target {
using Call = SemanticGuestCall;
namespace {
void bounded_ball_cell(Call&);
void choose_target_cell(Call&);
void alternate_phase_target(Call&);
void read_live_phase_target(Call&);
void bound_phase_target(Call&);
void read_normal_target(Call&);
void adjust_normal_target(Call&);
void adjust_target_direction(Call&);
void alternate_target_direction(Call&);
void apply_deep_target_limit(Call&);
void ball_relative_column(Call&);
void ball_relative_column(Call& a) {
    a.at(0x101e2e90); a.sign8(0,28); a.compare(0,0);
    if (a.equal()) {
        a.at(0x101e2f68); a.load<1>(0,29,0x791e); a.sign8(3,0); a.offset(3,3,-0x2); a.load<1>(0,31,0x0);
        a.sign8(0,0); a.compare_registers(3,0);
        if (a.at_least()) {
            a.at(0x101e2f94); a.load<1>(17,31,0x0); a.sign8(17,17);
        } else {
            a.at(0x101e2f84); a.load<1>(0,29,0x791e); a.sign8(3,0); a.offset(17,3,-0x2); a.jump(0x101e2f9c);
        }
        a.at(0x101e2f9c); a.sign8(0,17); a.store<1>(31,0x0,0); a.load<1>(0,31,0x0); a.sign8(3,0); a.compare(3,2);
        if (a.nonpositive()) {
            a.at(0x101e2fbc); a.load<1>(16,31,0x0); a.sign8(16,16);
        } else {
            a.at(0x101e2fb4); a.value(16,0x2); a.jump(0x101e2fc4);
        }
        a.at(0x101e2fc4); a.sign8(4,16); a.store<1>(31,0x0,4); a.load<1>(3,29,0x791e); a.sign8(3,3);
        a.offset(4,3,-0x4); a.load<1>(0,31,0x0); a.sign8(3,0); a.compare_registers(4,3);
        if (a.nonpositive()) {
            a.at(0x101e2ffc); a.load<1>(3,31,0x0); a.sign8(0,3); a.store(1,0x3c,0);
        } else {
            a.at(0x101e2fe8); a.load<1>(0,29,0x791e); a.sign8(3,0); a.offset(0,3,-0x4); a.store(1,0x3c,0);
            a.jump(0x101e3008);
        }
        a.at(0x101e3008); a.load(0,1,0x3c); a.sign8(3,0); a.store<1>(31,0x0,3); a.load<1>(0,31,0x0); a.sign8(3,0);
        a.compare(3,0);
        if (a.at_least()) {
            a.at(0x101e3030); a.load<1>(3,31,0x0); a.sign8(0,3); a.store(1,0x38,0);
        } else {
            a.at(0x101e3024); a.value(0,0x0); a.store(1,0x38,0); a.jump(0x101e303c);
        }
        a.at(0x101e303c); a.load(3,1,0x38); a.sign8(0,3); a.store<1>(31,0x0,0);
    } else {
        a.at(0x101e2e9c); a.load<1>(0,29,0x791e); a.sign8(3,0); a.offset(3,3,0x2); a.load<1>(0,31,0x0);
        a.sign8(0,0); a.compare_registers(3,0);
        if (a.nonpositive()) {
            a.at(0x101e2ec8); a.load<1>(21,31,0x0); a.sign8(21,21);
        } else {
            a.at(0x101e2eb8); a.load<1>(0,29,0x791e); a.sign8(3,0); a.offset(21,3,0x2); a.jump(0x101e2ed0);
        }
        a.at(0x101e2ed0); a.sign8(0,21); a.store<1>(31,0x0,0); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,9);
        if (a.at_least()) {
            a.at(0x101e2ef0); a.load<1>(20,31,0x0); a.sign8(20,20);
        } else {
            a.at(0x101e2ee8); a.value(20,0x9); a.jump(0x101e2ef8);
        }
        a.at(0x101e2ef8); a.sign8(0,20); a.store<1>(31,0x0,0); a.load<1>(0,29,0x791e); a.sign8(3,0);
        a.offset(3,3,0x4); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare_registers(3,0);
        if (a.at_least()) {
            a.at(0x101e2f2c); a.load<1>(19,31,0x0); a.sign8(19,19);
        } else {
            a.at(0x101e2f1c); a.load<1>(0,29,0x791e); a.sign8(3,0); a.offset(19,3,0x4); a.jump(0x101e2f34);
        }
        a.at(0x101e2f34); a.sign8(0,19); a.store<1>(31,0x0,0); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,11);
        if (a.nonpositive()) {
            a.at(0x101e2f54); a.load<1>(18,31,0x0); a.sign8(18,18);
        } else {
            a.at(0x101e2f4c); a.value(18,0xb); a.jump(0x101e2f5c);
        }
        a.at(0x101e2f5c); a.sign8(0,18); a.store<1>(31,0x0,0); a.jump(0x101e3048);
    }
    a.at(0x101e3048);
}
void apply_deep_target_limit(Call& a) {
    a.at(0x101e2dd0); a.sign8(0,28); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,29,3);
    a.call(0x101d56f0); a.mask(0,3,0x4000,false,0); a.compare(0,0);
    if (!a.equal()) {
        a.at(0x101e2df0); a.sign8(0,26); a.compare(0,0);
        if (a.equal()) {
            a.at(0x101e2e90);
            ball_relative_column(a);
        } else {
            a.at(0x101e2dfc); a.sign8(3,28); a.offset(0,3,0x6); a.indexed_load<1>(0,29,0); a.sign8(0,0);
            a.compare(0,0);
            if (!a.equal()) {
                a.at(0x101e2e90);
                ball_relative_column(a);
            } else {
                a.at(0x101e2e14); a.sign8(0,28); a.compare(0,0);
                if (a.equal()) {
                    a.at(0x101e2e58); a.load<1>(3,31,0x0); a.offset(0,3,-0x2); a.store<1>(31,0x0,0);
                    a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,0);
                    if (a.at_least()) {
                        a.at(0x101e2e7c); a.load<1>(22,31,0x0); a.sign8(22,22);
                    } else {
                        a.at(0x101e2e74); a.value(22,0x0); a.jump(0x101e2e84);
                    }
                    a.at(0x101e2e84); a.sign8(0,22); a.store<1>(31,0x0,0); a.jump(0x101e3048);
                } else {
                    a.at(0x101e2e20); a.load<1>(3,31,0x0); a.offset(0,3,0x2); a.store<1>(31,0x0,0);
                    a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,11);
                    if (a.nonpositive()) {
                        a.at(0x101e2e44); a.load<1>(23,31,0x0); a.sign8(23,23);
                    } else {
                        a.at(0x101e2e3c); a.value(23,0xb); a.jump(0x101e2e4c);
                    }
                    a.at(0x101e2e4c); a.sign8(0,23); a.store<1>(31,0x0,0); a.jump(0x101e3048);
                }
                a.at(0x101e3048);
            }
            a.at(0x101e3048);
        }
        a.at(0x101e3048);
    }
    a.at(0x101e3048);
}
void alternate_target_direction(Call& a) {
    a.at(0x101e2d1c); a.sign8(0,28); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,29,3);
    a.call(0x101d56f0); a.mask(0,3,0x8000,false,0); a.compare(0,0);
    if (!a.equal()) {
        a.at(0x101e2d3c); a.sign8(0,26); a.compare(0,0);
        if (a.equal()) {
            a.at(0x101e2d48); a.sign8(0,28); a.compare(0,0);
            if (a.equal()) {
                a.at(0x101e2d64); a.load<1>(3,31,0x0); a.offset(0,3,-0x1); a.store<1>(31,0x0,0);
            } else {
                a.at(0x101e2d54); a.load<1>(3,31,0x0); a.offset(0,3,0x1); a.store<1>(31,0x0,0); a.jump(0x101e2d70);
            }
            a.at(0x101e2d70); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,0);
            if (a.less()) {
                a.at(0x101e2d80); a.value(0,0x0); a.store<1>(31,0x0,0);
            }
            a.at(0x101e2d88); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,12);
            if (a.at_least()) {
                a.at(0x101e2d98); a.value(0,0xb); a.store<1>(31,0x0,0);
            }
            a.at(0x101e2da0); a.load<1>(0,30,0x0); a.sign8(0,0); a.compare(0,0);
            if (a.less()) {
                a.at(0x101e2db0); a.value(0,0x0); a.store<1>(30,0x0,0);
            }
            a.at(0x101e2db8); a.load<1>(0,30,0x0); a.sign8(0,0); a.compare(0,9);
            if (a.at_least()) {
                a.at(0x101e2dc8); a.value(0,0x8); a.store<1>(30,0x0,0);
            }
            a.at(0x101e2dd0);
        }
        a.at(0x101e2dd0);
    }
    a.at(0x101e2dd0);
}
void adjust_target_direction(Call& a) {
    a.at(0x101e2c64); a.sign8(0,28); a.multiply(3,0,0x18e0); a.offset(3,3,0x7c6c); a.add(3,29,3);
    a.call(0x101d56f0); a.mask(0,3,0x10000,false,0); a.compare(0,0);
    if (a.equal()) {
        a.at(0x101e2d1c);
        alternate_target_direction(a);
    } else {
        a.at(0x101e2c84); a.sign8(0,26); a.compare(0,0);
        if (a.equal()) {
            a.at(0x101e2d1c);
            alternate_target_direction(a);
        } else {
            a.at(0x101e2c90); a.sign8(0,28); a.compare(0,0);
            if (a.equal()) {
                a.at(0x101e2cac); a.load<1>(3,31,0x0); a.offset(0,3,0x1); a.store<1>(31,0x0,0);
            } else {
                a.at(0x101e2c9c); a.load<1>(3,31,0x0); a.offset(0,3,-0x1); a.store<1>(31,0x0,0);
                a.jump(0x101e2cb8);
            }
            a.at(0x101e2cb8); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,0);
            if (a.less()) {
                a.at(0x101e2cc8); a.value(0,0x0); a.store<1>(31,0x0,0);
            }
            a.at(0x101e2cd0); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,12);
            if (a.at_least()) {
                a.at(0x101e2ce0); a.value(0,0xb); a.store<1>(31,0x0,0);
            }
            a.at(0x101e2ce8); a.load<1>(0,30,0x0); a.sign8(0,0); a.compare(0,0);
            if (a.less()) {
                a.at(0x101e2cf8); a.value(0,0x0); a.store<1>(30,0x0,0);
            }
            a.at(0x101e2d00); a.load<1>(0,30,0x0); a.sign8(0,0); a.compare(0,9);
            if (a.at_least()) {
                a.at(0x101e2d10); a.value(0,0x8); a.store<1>(30,0x0,0); a.jump(0x101e2dd0);
            }
            a.at(0x101e2dd0);
        }
        a.at(0x101e2dd0);
    }
    a.at(0x101e2dd0);
}
void adjust_normal_target(Call& a) {
    a.at(0x101e2b94); a.move(3,27); a.call(0x10289560); a.sign16(0,3); a.compare(0,1);
    if (!a.equal()) {
        a.at(0x101e2ba8); a.move(3,29); a.move(4,28); a.call(0x101d5760); a.sign8(0,3); a.compare(0,0);
        if (a.equal()) {
            a.at(0x101e2c64);
            adjust_target_direction(a);
        } else {
            a.at(0x101e2bc0); a.sign8(3,28); a.offset(0,3,0x6); a.indexed_load<1>(0,29,0); a.sign8(0,0);
            a.compare(0,0);
            if (!a.equal()) {
                a.at(0x101e2c64);
                adjust_target_direction(a);
            } else {
                a.at(0x101e2bd8); a.sign8(0,28); a.compare(0,0);
                if (a.equal()) {
                    a.at(0x101e2bf4); a.load<1>(3,31,0x0); a.offset(0,3,-0x1); a.store<1>(31,0x0,0);
                } else {
                    a.at(0x101e2be4); a.load<1>(3,31,0x0); a.offset(0,3,0x1); a.store<1>(31,0x0,0);
                    a.jump(0x101e2c00);
                }
                a.at(0x101e2c00); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,0);
                if (a.less()) {
                    a.at(0x101e2c10); a.value(0,0x0); a.store<1>(31,0x0,0);
                }
                a.at(0x101e2c18); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,12);
                if (a.at_least()) {
                    a.at(0x101e2c28); a.value(0,0xb); a.store<1>(31,0x0,0);
                }
                a.at(0x101e2c30); a.load<1>(0,30,0x0); a.sign8(0,0); a.compare(0,0);
                if (a.less()) {
                    a.at(0x101e2c40); a.value(0,0x0); a.store<1>(30,0x0,0);
                }
                a.at(0x101e2c48); a.load<1>(0,30,0x0); a.sign8(0,0); a.compare(0,9);
                if (a.at_least()) {
                    a.at(0x101e2c58); a.value(0,0x8); a.store<1>(30,0x0,0); a.jump(0x101e2dd0);
                }
                a.at(0x101e2dd0);
            }
            a.at(0x101e2dd0);
        }
        a.at(0x101e2dd0);
        apply_deep_target_limit(a);
    }
    a.at(0x101e3048);
}
void read_normal_target(Call& a) {
    a.at(0x101e2ae8); a.move(3,27); a.call(0x101d3b50); a.sign8(0,3); a.multiply(0,0,0xd8); a.sign8(3,28);
    a.mask(3,3,0xfffffffc,false,2); a.offset(3,3,0x10000); a.offset(3,3,-0x2014); a.indexed_load(3,29,3);
    a.sign8(4,26); a.multiply(4,4,0x6c); a.add(4,0,4); a.sign8(0,25); a.multiply(0,0,0xc); a.add(4,4,0);
    a.sign8(0,24); a.add(0,4,0); a.indexed_load<1>(0,3,0); a.mask(0,0,0xf,false,0); a.sign8(0,0);
    a.store<1>(30,0x0,0); a.move(3,27); a.call(0x101d3b50); a.sign8(0,3); a.multiply(0,0,0xd8); a.sign8(3,28);
    a.mask(3,3,0xfffffffc,false,2); a.offset(3,3,0x10000); a.offset(3,3,-0x2014); a.indexed_load(3,29,3);
    a.sign8(4,26); a.multiply(4,4,0x6c); a.add(4,0,4); a.sign8(0,25); a.multiply(0,0,0xc); a.add(4,4,0);
    a.sign8(0,24); a.add(0,4,0); a.indexed_load<1>(0,3,0); a.mask(0,0,0xf0,false,0); a.shift_right(0,0,0x4);
    a.sign8(0,0); a.store<1>(31,0x0,0);
}
void bound_phase_target(Call& a) {
    a.at(0x101e2a84); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,0);
    if (a.less()) {
        a.at(0x101e2a94); a.value(0,0x0); a.store<1>(31,0x0,0);
    }
    a.at(0x101e2a9c); a.load<1>(0,31,0x0); a.sign8(0,0); a.compare(0,12);
    if (a.at_least()) {
        a.at(0x101e2aac); a.value(0,0xb); a.store<1>(31,0x0,0);
    }
    a.at(0x101e2ab4); a.load<1>(0,30,0x0); a.sign8(0,0); a.compare(0,0);
    if (a.less()) {
        a.at(0x101e2ac4); a.value(0,0x0); a.store<1>(30,0x0,0);
    }
    a.at(0x101e2acc); a.load<1>(0,30,0x0); a.sign8(0,0); a.compare(0,9);
    if (a.at_least()) {
        a.at(0x101e2adc); a.value(0,0x8); a.store<1>(30,0x0,0); a.jump(0x101e3048);
    }
    a.at(0x101e3048);
}
void read_live_phase_target(Call& a) {
    a.at(0x101e29d8); a.move(3,27); a.call(0x101d3b50); a.sign8(0,3); a.multiply(0,0,0xd8); a.sign8(3,28);
    a.mask(3,3,0xfffffffc,false,2); a.offset(3,3,0x10000); a.offset(3,3,-0x2014); a.indexed_load(3,29,3);
    a.sign8(4,26); a.multiply(4,4,0x6c); a.add(4,0,4); a.sign8(0,25); a.multiply(0,0,0xc); a.add(4,4,0);
    a.sign8(0,24); a.add(0,4,0); a.indexed_load<1>(0,3,0); a.mask(0,0,0xf,false,0); a.sign8(0,0);
    a.store<1>(30,0x0,0); a.move(3,27); a.call(0x101d3b50); a.sign8(0,3); a.multiply(4,0,0xd8); a.sign8(0,28);
    a.mask(3,0,0xfffffffc,false,2); a.offset(3,3,0x10000); a.offset(3,3,-0x2014); a.indexed_load(3,29,3);
    a.sign8(0,26); a.multiply(0,0,0x6c); a.add(4,4,0); a.sign8(0,25); a.multiply(0,0,0xc); a.add(4,4,0);
    a.sign8(0,24); a.add(0,4,0); a.indexed_load<1>(0,3,0); a.mask(0,0,0xf0,false,0); a.shift_right(0,0,0x4);
    a.sign8(0,0); a.store<1>(31,0x0,0);
}
void alternate_phase_target(Call& a) {
    a.at(0x101e2994); a.sign8(0,26); a.compare(0,0);
    if (!a.equal()) {
        a.at(0x101e29d8);
        read_live_phase_target(a);
    } else {
        a.at(0x101e29a0); a.move(3,27); a.call(0x102861b0); a.sign8(0,3); a.compare(0,0);
        if (a.equal()) {
            a.at(0x101e29d8);
            read_live_phase_target(a);
        } else {
            a.at(0x101e29b4); a.move(3,27); a.call(0x101d5690); a.sign8(0,3); a.store<1>(30,0x0,0); a.move(3,27);
            a.call(0x101d5680); a.sign8(0,3); a.store<1>(31,0x0,0); a.jump(0x101e2a84);
        }
        a.at(0x101e2a84);
    }
    a.at(0x101e2a84);
}
void choose_target_cell(Call& a) {
    a.at(0x101e2940); a.load<1>(3,29,0x7927); a.sign8(4,3); a.compare(4,0);
    if (a.nonpositive()) {
        a.at(0x101e2ae8);
        read_normal_target(a);
        adjust_normal_target(a);
    } else {
        a.at(0x101e2950); a.sign8(0,26); a.compare(0,0);
        if (a.equal()) {
            a.at(0x101e2994);
            alternate_phase_target(a);
        } else {
            a.at(0x101e295c); a.move(3,27); a.call(0x102860c0); a.sign8(0,3); a.compare(0,0);
            if (a.equal()) {
                a.at(0x101e2994);
                alternate_phase_target(a);
            } else {
                a.at(0x101e2970); a.move(3,27); a.call(0x101d5690); a.sign8(0,3); a.store<1>(30,0x0,0);
                a.move(3,27); a.call(0x101d5680); a.sign8(0,3); a.store<1>(31,0x0,0); a.jump(0x101e2a84);
            }
            a.at(0x101e2a84);
        }
        a.at(0x101e2a84);
        bound_phase_target(a);
    }
    a.at(0x101e3048);
}
void bounded_ball_cell(Call& a) {
    a.at(0x101e28d4); a.load<1>(25,29,0x791d); a.load<1>(24,29,0x791e); a.sign8(0,25); a.compare(0,0);
    if (a.less()) {
        a.at(0x101e28e8); a.value(25,0x0);
    }
    a.at(0x101e28ec); a.sign8(3,25); a.compare(3,9);
    if (a.at_least()) {
        a.at(0x101e28f8); a.value(25,0x8);
    }
    a.at(0x101e28fc); a.sign8(0,24); a.compare(0,0);
    if (a.less()) {
        a.at(0x101e2908); a.value(24,0x0);
    }
    a.at(0x101e290c); a.sign8(0,24); a.compare(0,12);
    if (a.at_least()) {
        a.at(0x101e2918); a.value(24,0xb);
    }
    a.at(0x101e291c);
}
}
void resolve_target_cell(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101e28b0,dependencies,observer);
    a.at(0x101e28b0); a.link(); a.multiple(16); a.save_link(); a.frame(128); a.move(29,3); a.move(27,4);
    a.move(30,5); a.move(31,6); a.store<1>(1,0xab,7);
    bounded_ball_cell(a);
    a.move(3,27); a.call(0x101e3150); a.move(28,3); a.move(3,27); a.call(0x10283690); a.move(4,3); a.move(3,29);
    a.call(0x101e3060); a.move(26,3);
    choose_target_cell(a);
    a.pop(128); a.multiple(16,true); a.finish();
}
}

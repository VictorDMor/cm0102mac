#pragma once

#include "routine_check.hpp"

#if defined(CM_TRANSFER_MANAGER_COVERAGE)
#include <cstdlib>
#include <unistd.h>
extern "C" int __llvm_profile_write_file();
extern "C" void __llvm_profile_set_filename(const char*);
#endif

namespace cm::aot::transfer_manager {
inline void install_check_coverage() {
#if defined(CM_TRANSFER_MANAGER_COVERAGE)
    routine_child_exit = [] {
        // The profiling runtime resolves %p before the comparison forks.
        const auto profile = std::string(CM_TRANSFER_MANAGER_PROFILE_DIR) + "/check-" +
                             std::to_string(getpid()) + ".profraw";
        __llvm_profile_set_filename(profile.c_str());
        if (__llvm_profile_write_file() != 0) std::abort();
    };
#endif
}
}

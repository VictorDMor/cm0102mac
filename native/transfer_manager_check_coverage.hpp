#pragma once

#include "routine_check.hpp"

#if defined(CM_TRANSFER_MANAGER_COVERAGE)
#include <cstdlib>
#include <unistd.h>
extern "C" int __llvm_profile_write_file();
extern "C" void __llvm_profile_set_filename(const char*);
#endif

namespace cm::aot::transfer_manager {
namespace checks {
int pending(int argc, char** argv);
int valuation(int argc, char** argv);
int callbacks(int argc, char** argv);
int collections(int argc, char** argv);
int selection(int argc, char** argv);
int rejection(int argc, char** argv);
int permissions(int argc, char** argv);
int approvals(int argc, char** argv);
int offer_contracts(int argc, char** argv);
int approval_bootstrap(int argc, char** argv);
int fees(int argc, char** argv);
int installments(int argc, char** argv);
int future(int argc, char** argv);
int history(int argc, char** argv);
int lifecycle(int argc, char** argv);
int save_section(int argc, char** argv);
int load_section(int argc, char** argv);
}

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

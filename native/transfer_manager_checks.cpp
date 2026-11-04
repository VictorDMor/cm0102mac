#include "transfer_manager_check_coverage.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    using namespace cm::aot::transfer_manager;
    struct Suite {
        std::string_view name;
        int (*run)(int, char**);
    };
    constexpr std::array suites{
        Suite{"pending", checks::pending},
        Suite{"valuation", checks::valuation},
        Suite{"callbacks", checks::callbacks},
        Suite{"collections", checks::collections},
        Suite{"selection", checks::selection},
        Suite{"rejection", checks::rejection},
        Suite{"permissions", checks::permissions},
        Suite{"approvals", checks::approvals},
    };
    if (argc > 1) {
        const auto suite = std::ranges::find(suites, std::string_view(argv[1]), &Suite::name);
        if (suite != suites.end()) return suite->run(argc - 1, argv + 1);
    }
    std::cerr << "Select a transfer manager check suite\n";
    return 1;
}

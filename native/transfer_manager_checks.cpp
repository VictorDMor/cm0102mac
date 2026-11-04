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
        Suite{"offer_contracts", checks::offer_contracts},
        Suite{"approval_bootstrap", checks::approval_bootstrap},
        Suite{"fees", checks::fees},
        Suite{"installments", checks::installments},
        Suite{"future", checks::future},
        Suite{"history", checks::history},
        Suite{"lifecycle", checks::lifecycle},
        Suite{"save_section", checks::save_section},
        Suite{"load_section", checks::load_section},
        Suite{"nations", checks::nations},
        Suite{"construct", checks::construct},
        Suite{"offer_state", checks::offer_state},
        Suite{"schedule", checks::schedule},
        Suite{"nation_rules", checks::nation_rules},
        Suite{"candidate_clubs", checks::candidate_club_collection},
        Suite{"squad_quotas", checks::squad_quotas},
        Suite{"offer_news", checks::offer_news},
    };
    if (argc > 1) {
        const auto suite = std::ranges::find(suites, std::string_view(argv[1]), &Suite::name);
        if (suite != suites.end()) return suite->run(argc - 1, argv + 1);
    }
    std::cerr << "Select a transfer manager check suite\n";
    return 1;
}

// SHORTLIST_MANAGER.CPP: the sort and search comparators the module passes to qsort and bsearch (shortlist_manager_compare.hpp).
SHORTLIST_MANAGER_ROUTINE(0x10358f20, 0x10358f50, R3, "9ce932e21c307190", [](Cpu& c) { c.set(3, shortlist_manager::compare_by_value_then_key(c, c.r(3), c.r(4))); })
SHORTLIST_MANAGER_ROUTINE(0x10358f50, 0x10358f80, R3, "324ea966e08e6856", [](Cpu& c) { c.set(3, shortlist_manager::compare_by_value_then_key(c, c.r(3), c.r(4))); })
SHORTLIST_MANAGER_ROUTINE(0x10358f80, 0x10358ff0, R3, "36bd7593414be937", [](Cpu& c) { c.set(3, shortlist_manager::compare_linked_first(c, c.r(3), c.r(4))); })
SHORTLIST_MANAGER_ROUTINE(0x10358ff0, 0x10359020, R3, "d0a62c28c1576ee1", [](Cpu& c) { c.set(3, shortlist_manager::compare_by_rank(c, c.r(3), c.r(4))); })
SHORTLIST_MANAGER_ROUTINE(0x103663b0, 0x10366410, R3, "bda8830a61816368", [](Cpu& c) { c.set(3, shortlist_manager::compare_pairs_by_second(c, c.r(3), c.r(4))); })
SHORTLIST_MANAGER_ROUTINE(0x10366410, 0x10366470, R3, "3ec6f2f832ad7445", [](Cpu& c) { c.set(3, shortlist_manager::compare_pairs_by_first(c, c.r(3), c.r(4))); })

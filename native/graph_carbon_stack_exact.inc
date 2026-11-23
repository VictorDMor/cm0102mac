// Entries compared on the dead stack too (graph_carbon_checks.cpp): none yet. The stack contract is the leaver check (the twelve
// leavers of graph_carbon_audit.LEAVERS): the checker records each entry into one, on the original and on the C++ side, with the stack
// pointer relative to the checked entry's, the link register and the registers its prologue saves. It is live once a leaver is hooked.

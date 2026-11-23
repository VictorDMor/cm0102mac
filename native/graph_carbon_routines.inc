// graph_carbon.cpp (tools/graph_carbon_audit.py): entry, end, results, digest of the original body, hook.
// GRAPH_CARBON_ROUTINE runs in production; GRAPH_CARBON_DRAFT only in the check tools. Entries not listed run generated. The hooks
// read the guest ABI's arguments and set r3 and the registers the original leaves.
// Each block appends the #include of its graph_carbon_routines_<block>.inc below.
#include "graph_carbon_routines_leaves.inc"

// COMP_UTIL.CPP block `record_stream` (comp_util_stream.cpp): a fixture reference to and from a news item. Neither
// routine's registers are read after a call (tools/register_contract.py --after), so neither declares a result.
COMP_UTIL_ROUTINE(0x10463be0, 0x10463dbc, NONE, "3e7e3bd51d4149a8", [](Cpu& c) { comp_util::write_fixture_reference(c, c.r(3), c.r(4), c.r(5)); })
COMP_UTIL_ROUTINE(0x10463dc0, 0x10464008, NONE, "2e68df70fcb6f0d1", [](Cpu& c) { comp_util::read_fixture_reference(c, c.r(3), c.r(4), c.r(5)); })

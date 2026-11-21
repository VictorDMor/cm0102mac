// Job Information (0x101c74e0), opened through its original opener 0x101c7370 for the managed club's nation.
// The capture's world has no managerial vacancy, so the fixture makes some the way the screen finds them: the
// builder lists every club of the chosen nation whose manager's job is insecure, which is a value of the club's
// 0x4c-byte statistics row (short at +6, below 4050) that the fixture writes for the first N clubs the original
// "has a manager" test accepts; the original builder then reads them. States: no jobs (the one-column message),
// five (a short table), sixteen (a full page) and twenty (a scrollbar), the last also scrolled.
// The rows written are part of the compared records. See adaptive_paged_checks.hpp.
inline std::vector<unsigned>& job_rows() { static std::vector<unsigned> rows; return rows; }
void job_information_jobs(TransferScene& t, unsigned count) {
    auto& c = t.c;
    job_rows().clear();
    const auto clubs = c.u32(c.toc(-0x7be8)), total = c.u32(c.toc(-0x7b50)), rows = c.u32(c.u32(c.toc(-0x79f0)));
    for (unsigned k = 0; k < total && job_rows().size() < count; ++k) {
        const auto club = clubs + k * 0x248, manager = c.u32(club + 0x2c);
        if (!(std::uint8_t(t.invoke(0x100d3c40, {club})) != 0) || !manager || !c.u32(manager + 0x74)) continue;
        const auto row = rows + c.u32(club) * 0x4c;
        c.put16(row + 6, 100 + unsigned(job_rows().size()) * 150 % 3900);
        job_rows().push_back(row);
    }
    require(job_rows().size() == count, "Job Information fixture: not enough clubs with a manager");
}
std::vector<std::uint8_t> job_information_records(TransferScene& t) {
    std::vector<std::uint8_t> bytes;
    for (const auto row : job_rows()) { const auto part = t.s.memory.copy_out(row, 0x4c); bytes.insert(bytes.end(), part.begin(), part.end()); }
    return bytes;
}
void job_information_builder(const char* path) {
    paged_screen_fixture(path, "Job information", 0x101c74e0, ScreenRect{110, 170, 780, 535}, false, {
        {"no jobs", [](TransferScene& t) { job_rows().clear(); t.invoke(0x101c7370, {}); }},
        {"five jobs", [](TransferScene& t) { job_information_jobs(t, 5); t.invoke(0x101c7370, {}); }},
        {"sixteen jobs", [](TransferScene& t) { job_information_jobs(t, 16); t.invoke(0x101c7370, {}); }},
        {"twenty jobs", [](TransferScene& t) { job_information_jobs(t, 20); t.invoke(0x101c7370, {}); }},
        {"twenty jobs scrolled", [](TransferScene& t) { job_information_jobs(t, 20); t.invoke(0x101c7370, {}); t.set_value(1, 3); }},
    }, [](TransferScene& t) {
        const auto instance = t.c.u32(t.manager + 4); t.c.put32(t.manager + 4, 0);
        require(!adaptive_screen_available(t.s, 0x101c74e0), "Job information gate accepted a missing instance");
        t.c.put32(t.manager + 4, instance);
    }, job_information_records);
}

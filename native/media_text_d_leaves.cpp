// The sentences the stories share: the media, a former player or a club's manager on the club and the
// competition, and the tournament tips by host nation.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

std::uint32_t Story::job_of_subject(Job job) const {
    call(report::job_title, job.back, {{3, sb(staff_job)}, {4, 0}, {5, 0}, {6, at(job.marker)}});
    return c.r(3);
}

void Story::adjective_of_nation(Adjective adjective) const {
    call(nationality_adjective, adjective.back, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, adjective.form}});
}

Wording Story::by_margin(const std::array<Wording, 3>& wordings) const {
    if (cmp(goal_margin(), 3) > 0) return wordings[0];
    if (cmp(goal_total(), 5) <= 0) return wordings[1];
    return wordings[2];
}

Wording Story::pick(const Pick& p) const { return (!p.holding || cmp(title_holder, 0) == 0) ? p.fresh : *p.holding; }

void Story::media_on_competition(const Pick& p, Form f) const {
    const auto w = pick(p);
    tell(w, {home_marker(), home_marker(), competition_marker(f), competition_marker(f), home_short(), competition_name(f)});
}

void Story::former_player_on_competition(const Pick& p, Form f) const {
    const auto w = pick(p);
    outgoing(0, competition_name(f));
    tell(w, {home_marker(), home_marker(), competition_marker(f), competition_marker(f), subject_full, home_short()});
}

// "<club> <job> <name> says ...": the subject's club and job lead the sentence; the job's marker was left in
// the frame by the job routine.
void Story::staff_on_competition(Job job, const Pick& p, Form f) const {
    const auto job_text = job_of_subject(job);
    const auto w = pick(p);
    outgoing(0, competition_marker(f));
    outgoing(1, competition_marker(f));
    outgoing(2, staff_club_view().short_name());
    outgoing(3, job_text);
    outgoing(4, subject_full);
    outgoing(5, home_short());
    outgoing(6, competition_name(f));
    tell(w, {staff_club_view().short_marker(), staff_club_view().short_marker(), job_marker(job), job_marker(job), home_marker(), home_marker()});
}

void Story::staff_remark(Job job, Wording w) const {
    const auto job_text = job_of_subject(job);
    outgoing(0, staff_club_view().short_name());
    outgoing(1, job_text);
    outgoing(2, subject_full);
    outgoing(3, home_short());
    tell(w, {staff_club_view().short_marker(), staff_club_view().short_marker(), job_marker(job), job_marker(job), home_marker(), home_marker()});
}

void Story::foreign_media_on_competition(Adjective adjective, const Pick& p, Order order, Form f) const {
    adjective_of_nation(adjective);
    const auto w = pick(p);
    switch (order) {
    case Order::club_competition_nation:
        outgoing(0, home_short());
        outgoing(1, competition_name(f));
        outgoing(2, nationality);
        tell(w, {home_marker(), home_marker(), competition_marker(f), competition_marker(f), unmarked, unmarked});
        break;
    case Order::club_nation_competition:
        outgoing(0, home_short());
        outgoing(1, nationality);
        outgoing(2, competition_name(f));
        tell(w, {home_marker(), home_marker(), unmarked, unmarked, competition_marker(f), competition_marker(f)});
        break;
    default:  // Order::nation_club_competition
        outgoing(0, nationality);
        outgoing(1, home_short());
        outgoing(2, competition_name(f));
        tell(w, {unmarked, unmarked, home_marker(), home_marker(), competition_marker(f), competition_marker(f)});
        break;
    }
}

void Story::media_in_nation_on_competition(const Pick& p, Form f) const {
    const auto w = pick(p);
    outgoing(0, nation_name());
    outgoing(1, home_short());
    outgoing(2, competition_name(f));
    tell(w, {nation_marker(), nation_marker(), home_marker(), home_marker(), competition_marker(f), competition_marker(f)});
}

void Story::media_in_nation(const Pick& p) const {
    const auto w = pick(p);
    tell(w, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation_name(), home_short()});
}

// (The formatter is also given the club table after the name.)
void Story::media_on_club(const Pick& p) const {
    const auto w = pick(p);
    tell(w, {home_marker(), home_marker(), home_short(), clubs()});
}

// A tournament tip depends on the club's nation: one of the hosts, or not (with one other host or two),
// and on who tells it: the media, a former player, or a club's manager.
void Story::host_tips(const HostTips& tips) const {
    if (club_in_host_nation()) return own_host_tip(tips.own);
    if (cmp(host_two, -1) <= 0) return one_host_tip(tips.one);
    two_hosts_tip(tips.two);
}

void Story::one_host_media(Wording w, Form f) const {
    const auto host = host_nation(host_one);
    outgoing(0, home_short());
    outgoing(1, competition_name(f));
    outgoing(2, host.name());
    tell(w, {home_marker(), home_marker(), competition_marker(f), competition_marker(f), host.marker(), host.marker()});
}

void Story::two_hosts_media(Wording w, Form f) const {
    const auto first = host_nation(host_one), second = host_nation(host_two);
    outgoing(0, second.marker());
    outgoing(1, second.marker());
    outgoing(2, home_short());
    outgoing(3, competition_name(f));
    outgoing(4, first.name());
    outgoing(5, second.name());
    const auto home_row = scratch<13>(home());  // (the first marker is read through it)
    tell(w, {club_view(home_row).short_marker(), home_marker(), competition_marker(f), competition_marker(f), first.marker(), first.marker()});
}

void Story::own_host_tip(const TipLeaf& leaf) const {
    if (subject_has_club_and_job()) return staff_on_competition(leaf.job, leaf.claim, leaf.claim_form);
    if (!subject_is_former_player()) return media_on_competition(leaf.generic, leaf.generic_form);
    former_player_on_competition(leaf.former, leaf.former_form);
}

void Story::one_host_tip(const TipLeaf& leaf) const {
    const auto host = host_nation(host_one);
    if (subject_has_club_and_job()) {
        const auto f = leaf.claim_form;
        const auto job_text = job_of_subject(leaf.job);
        outgoing(0, competition_marker(f));
        outgoing(1, competition_marker(f));
        outgoing(2, host.marker());
        outgoing(3, host.marker());
        // The club's name was built in r13 from the club table in r14.
        scratch<14>(clubs());
        const auto club_name = scratch<13>(staff_club_view().short_name());
        outgoing(4, club_name);
        outgoing(5, job_text);
        outgoing(6, subject_full);
        outgoing(7, home_short());
        outgoing(8, competition_name(f));
        outgoing(9, host.name());
        tell(leaf.claim, {staff_club_view().short_marker(), staff_club_view().short_marker(), job_marker(leaf.job), job_marker(leaf.job), home_marker(), home_marker()});
        return;
    }
    if (!subject_is_former_player()) return one_host_media(leaf.generic, leaf.generic_form);
    const auto f = leaf.former_form;
    outgoing(0, subject_full);
    outgoing(1, home_short());
    outgoing(2, competition_name(f));
    outgoing(3, host.name());
    tell(leaf.former, {home_marker(), home_marker(), competition_marker(f), competition_marker(f), host.marker(), host.marker()});
}

void Story::two_hosts_tip(const TipLeaf& leaf) const {
    const auto first = host_nation(host_one), second = host_nation(host_two);
    if (subject_has_club_and_job()) {
        const auto f = leaf.claim_form;
        const auto job_text = job_of_subject(leaf.job);
        outgoing(0, competition_marker(f));
        outgoing(1, competition_marker(f));
        outgoing(2, first.marker());
        outgoing(3, first.marker());
        outgoing(4, second.marker());
        outgoing(5, second.marker());
        // r13, r14 and r16 hold the first host's name and the competition's name (both as offsets in their
        // tables) and the club table; r16 was the domestic flag.
        scratch<14>(competition_name(f) - competitions());
        scratch<13>(first.name() - nation_table());
        scratch<16>(clubs());
        outgoing(6, staff_club_view().short_name());
        outgoing(7, job_text);
        outgoing(8, subject_full);
        outgoing(9, home_short());
        outgoing(10, competition_name(f));
        outgoing(11, first.name());
        outgoing(12, second.name());
        tell(leaf.claim, {staff_club_view().short_marker(), staff_club_view().short_marker(), job_marker(leaf.job), job_marker(leaf.job), home_marker(), home_marker()});
        return;
    }
    if (!subject_is_former_player()) return two_hosts_media(leaf.generic, leaf.generic_form);
    const auto f = leaf.former_form;
    outgoing(0, second.marker());
    outgoing(1, second.marker());
    outgoing(2, subject_full);
    outgoing(3, home_short());
    outgoing(4, competition_name(f));
    outgoing(5, first.name());
    outgoing(6, second.name());
    const auto home_row = scratch<13>(home());
    tell(leaf.former, {club_view(home_row).short_marker(), home_marker(), competition_marker(f), competition_marker(f), first.marker(), first.marker()});
}
}

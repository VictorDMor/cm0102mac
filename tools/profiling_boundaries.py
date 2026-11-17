"""Sparse, read-only events at audited original UI and daily call sites."""

# Messages span dispatcher returns; a generated C++ scope cannot span a guest
# call. Guard both its call and post-call instructions. A post-call address may
# also be a branch join: its "after" marker alone does not prove a call occurred.
# At adjacent calls, emit previous-after before next-begin. Addresses distinguish
# repeated services.
DAY_CALLS = (
    (0x10119660, 0x1073c730, 'lwz r3,0x0(r28)', 'Screen interaction (includes input wait)'),
    (0x10119678, 0x10773520, 'or. r15,r3,r3', 'Service 0x10773520'),
    (0x10119694, 0x101cb060, 'lwz r3,-0x7420(r2)', 'Service 0x101cb060'),
    (0x101196a8, 0x101cb8d0, 'lwz r3,-0x7420(r2)', 'Service 0x101cb8d0'),
    (0x101196b4, 0x101cb9d0, 'li r3,0x0', 'Service 0x101cb9d0'),
    (0x101196bc, 0x1011b680, 'lwz r3,-0x7b48(r2)', 'Service 0x1011b680'),
    (0x101196cc, 0x103868c0, 'lwz r3,-0x7420(r2)', 'Service 0x103868c0'),
    (0x101196d8, 0x101cb8d0, 'lwz r3,-0x7420(r2)', 'Service 0x101cb8d0'),
    (0x101196e8, 0x101cb780, 'lwz r3,-0x7434(r2)', 'Service 0x101cb780'),
    (0x10119774, 0x100c6a10, 'extsb r0,r3', 'Service 0x100c6a10'),
    (0x101197a8, 0x103ea840, 'lwz r3,-0x7b7c(r2)', 'Service 0x103ea840'),
    (0x101197c4, 0x10079050, 'lbz r0,0x0(r30)', 'Service 0x10079050'),
    (0x101197dc, 0x10700400, 'or r3,r31,r31', 'Service 0x10700400'),
    (0x101197e4, 0x10715b20, 'extsh r0,r3', 'Service 0x10715b20'),
    (0x101197f4, 0x1011b710, 'bl 0x1011aa60', 'Service 0x1011b710'),
    (0x101197f8, 0x1011aa60, 'lwz r3,-0x7aac(r2)', 'Service 0x1011aa60'),
    (0x10119804, 0x10433080, 'lwz r3,-0x7ba0(r2)', 'Service 0x10433080'),
    (0x10119810, 0x10185410, 'lwz r3,-0x7aa4(r2)', 'Daily injuries'),
    (0x10119820, 0x100df2d0, 'lwz r3,-0x7a48(r2)', 'Discipline notifications'),
    (0x10119830, 0x104b3b60, 'lwz r3,-0x7bb4(r2)', 'Service 0x104b3b60'),
    (0x10119840, 0x104364e0, 'li r3,0x1', 'Award date advance'),
    (0x10119848, 0x1011b680, 'bl 0x1011c330', 'Service 0x1011b680'),
    (0x1011984c, 0x1011c330, 'bl 0x1011b920', 'Service 0x1011c330'),
    (0x10119850, 0x1011b920, 'lwz r3,-0x7424(r2)', 'Service 0x1011b920'),
    (0x10119860, 0x1049d430, 'bl 0x10131070', 'Service 0x1049d430'),
    (0x10119864, 0x10131070, 'bl 0x10131520', 'Service 0x10131070'),
    (0x10119868, 0x10131520, 'lwz r3,-0x7a64(r2)', 'Service 0x10131520'),
    (0x10119878, 0x102abbc0, 'lis r3,-0x7777', 'Service 0x102abbc0'),
    (0x101198b8, 0x103568e0, 'lwz r3,-0x7b40(r2)', 'Shortlist storage reset'),
    (0x101198c8, 0x10357d30, 'lwz r3,-0x7ab8(r2)', 'Shortlist ratings'),
    (0x101198d8, 0x100fa6e0, 'lwz r3,-0x7b28(r2)', 'Daily finance'),
    (0x101198e8, 0x102d8920, 'or r3,r15,r15', 'Service 0x102d8920'),
    (0x101198f0, 0x102dabb0, 'lwz r3,-0x7a7c(r2)', 'Service 0x102dabb0'),
    (0x10119900, 0x103d0950, 'lha r3,0x0(r31)', 'Service 0x103d0950'),
    (0x10119908, 0x1013ae10, 'extsb. r0,r3', 'Service 0x1013ae10'),
    (0x1011991c, 0x102d86c0, 'li r3,0x0', 'Service 0x102d86c0'),
    (0x10119924, 0x100cae30, 'lwz r3,-0x7428(r2)', 'Club ID sort'),
    (0x10119930, 0x102d15d0, 'lwz r3,-0x7444(r2)', 'Service 0x102d15d0'),
    (0x1011993c, 0x10370e10, 'lwz r3,-0x79f0(r2)', 'Simulated statistics'),
    (0x10119948, 0x10196390, 'lwz r3,-0x742c(r2)', 'Daily managers'),
    (0x10119958, 0x104af1d0, 'lwz r3,-0x7b24(r2)', 'Service 0x104af1d0'),
    (0x10119964, 0x10004070, 'lwz r3,-0x7484(r2)', 'Daily absence review'),
    (0x10119970, 0x1070c510, 'lwz r3,-0x7ba4(r2)', 'Disputes'),
    (0x1011997c, 0x10109e20, 'bl 0x1013d7a0', 'Service 0x10109e20'),
    (0x10119980, 0x1013d7a0, 'bl 0x1013cd10', 'Service 0x1013d7a0'),
    (0x10119984, 0x1013cd10, 'lwz r3,-0x797c(r2)', 'Service 0x1013cd10'),
    (0x101199b0, 0x106fd990, 'lwz r3,-0x7430(r2)', 'Fog of war ageing'),
    (0x10119a60, 0x100edaf0, 'b 0x10119ae4', 'Service 0x100edaf0'),
    (0x10119a98, 0x10717b20, 'lwz r0,0x58(r1)', 'Service 0x10717b20'),
    (0x10119ae0, 0x100edaf0, 'lwz r3,-0x7afc(r2)', 'Service 0x100edaf0'),
    (0x10119aec, 0x10013650, 'lwz r3,-0x7b1c(r2)', 'Service 0x10013650'),
    (0x10119af8, 0x10311230, 'lhz r0,0x2(r31)', 'Service 0x10311230'),
    (0x10119b30, 0x1011b330, 'extsb. r0,r3', 'Service 0x1011b330'),
    (0x10119b6c, 0x10717890, 'lhz r4,0x52(r1)', 'Service 0x10717890'),
    (0x10119c5c, 0x103660b0, 'extsb. r0,r3', 'Service 0x103660b0'),
    (0x10119c7c, 0x10357cb0, 'b 0x10119d58', 'Service 0x10357cb0'),
    (0x10119d54, 0x10357cb0, 'lwz r3,0x2c(r17)', 'Service 0x10357cb0'),
    (0x10119da8, 0x1036d6e0, 'b 0x10119e18', 'Service 0x1036d6e0'),
    (0x10119dd8, 0x1036d6e0, 'b 0x10119e18', 'Service 0x1036d6e0'),
    (0x10119e14, 0x1036d6e0, 'addi r15,r15,0x1', 'Service 0x1036d6e0'),
    (0x10119e30, 0x10698f50, 'lwz r3,-0x7b94(r2)', 'Service 0x10698f50'),
    (0x10119e3c, 0x1069e480, 'lbz r6,0x0(r30)', 'Service 0x1069e480'),
    (0x10119e50, 0x10716eb0, 'lwz r4,-0x7bb8(r2)', 'Date formatting'),
    (0x10119e68, 0x10743120, 'bl 0x1076fe80', 'Screen message'),
    (0x10119e6c, 0x1076fe80, 'b 0x101195e8', 'Event pump (includes input wait)'),
)

# The Continue branch has passed command == 1000, but news/network gates can
# still divert it. The world routine is a long-lived loop, not one Continue.
BOUNDARY_EVENTS = {
    0x102a6dbc: ("add r3,r29,r0", ("Continue requested",)),
    0x102a6eb4: ("li r3,-0x5", ("Continue accepted",)),
    0x10119530: ("mfspr r0,LR", ("World loop begin",)),
    0x101195e8: ("lhz r4,0x2(r31)", ("Day iteration begin",)),
    0x10119e70: ("b 0x101195e8", ("Day iteration end",)),
    0x10119e84: ("blr", ("World loop end",)),
    0x1076e5f0: ("mfspr r0,LR", ("Progress draw begin",)),
    0x1076e7a4: ("bl 0x107692f0", ("Progress submit",)),
    0x1076e7bc: ("blr", ("Progress draw end",)),
}


def add_boundary(boundaries, address, assembly, name):
    expected, names = boundaries.get(address, (assembly, ()))
    if expected != assembly:
        raise ValueError(f"Conflicting profiling instructions at {address:x}")
    boundaries[address] = (assembly, (*names, name))


PROFILE_BOUNDARIES = {}
for address, target, return_assembly, name in DAY_CALLS:
    label = f"Day: {name} [{address:x}]"
    add_boundary(PROFILE_BOUNDARIES, address, f"bl 0x{target:x}", f"{label} begin")
    add_boundary(PROFILE_BOUNDARIES, address + 4, return_assembly, f"{label} after")

for address, (assembly, names) in BOUNDARY_EVENTS.items():
    for name in names:
        add_boundary(PROFILE_BOUNDARIES, address, assembly, name)


def profiling_hook(address, assembly):
    expected, names = PROFILE_BOUNDARIES[address]
    if assembly != expected:
        raise ValueError(f"Profiling boundary changed at {address:x}: {assembly}")
    return "\n".join(f'CM_PROFILE_EVENT("{name}");' for name in names)

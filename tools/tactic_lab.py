#!/usr/bin/env python3
"""Measure and search CM 01/02 tactics with cm-tactic-lab (docs/tactic-lab.md).

  tactic_lab.py decode FILE.tct...
  tactic_lab.py evaluate [--panel train|holdout] [--seeds N] [--first-seed S] TACTIC...
  tactic_lab.py optimize honest|exploit START.tct OUTDIR [--generations G]
  tactic_lab.py race [--keep K] [--first-seed S] TACTIC...
  tactic_lab.py improve honest|exploit START.tct OUTDIR [--kinds team,instructions,spots,codes]

TACTIC is a .tct file, `respot:FILE` or `ai` (see native/tactic_lab_main.cpp).
Evaluation plays every tactic on the same seeds of every matchup of a panel
(common random numbers), in parallel processes, and reports points per game.
The search is a (1+16) hill climb with racing: 16 mutants on 8 seeds per
matchup, the best 4 again with the incumbent on 24 fresh seeds, the best
second-stage paired gain accepted when it exceeds one standard error.
"""
import argparse, copy, json, math, os, random, struct, subprocess, sys, tempfile, time
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, '.local/current/build')


def cache_path(name):
    for line in open(os.path.join(BUILD, 'CMakeCache.txt')):
        if line.startswith(name + ':'):
            return line.split('=', 1)[1].strip()
    raise SystemExit(f'{name} is not in the build cache')


# --- .tct files (current format 0x98ec5e; see native/formation_files.hpp) ---
LINES = {0x1: 'GK', 0x2: 'SW', 0x4: 'D', 0x8: 'DM', 0x10: 'M', 0x20: 'AM', 0x40: 'F'}
SIDES = {0x80: 'R', 0x100: 'RC', 0x200: 'C', 0x400: 'LC', 0x800: 'L'}
TEAM = {'mentality': {0x8000: 'Def', 0x10000: 'Att', 0x20000: 'Norm'},
        'passing': {1: 'Mixed', 2: 'Short', 4: 'Direct', 8: 'Long'},
        'tackling': {0x10: 'Norm', 0x20: 'Easy', 0x40: 'Hard'},
        'pressing': {0x80: 'No', 0x100: 'Yes'}, 'offside': {0x200: 'No', 0x400: 'Yes'},
        'counter': {0x800: 'No', 0x1000: 'Yes'}, 'behind': {0x2000: 'No', 0x4000: 'Yes'}}
PLAYER = {'pass': {1: 'Team', 2: 'Mixed', 4: 'Short', 8: 'Direct', 0x10: 'Long'},
          'passto': {0x20: 'pt0', 0x40: 'pt1', 0x80: 'pt2', 0x100: 'pt3'},
          'tackle': {0x200: 'Team', 0x400: 'Norm', 0x800: 'Easy', 0x1000: 'Hard'},
          'press': {0x2000: 'p0', 0x4000: 'p1', 0x8000: 'p2'},
          'setA': {0x10000: 'a0', 0x40000000: 'a1', 0x20000: 'a2'},
          'setD': {0x40000: 'd0', 0x80000: 'd1', 0x80000000: 'd2'},
          'free': {0x100000: 'N', 0x200000: 'Y'}, 'fwdrun': {0x400000: 'N', 0x800000: 'Y'},
          'runball': {0x1000000: 'N', 0x2000000: 'Y'}, 'holdup': {0x4000000: 'N', 0x8000000: 'Y'},
          'longsh': {0x10000000: 'N', 0x20000000: 'Y'}, 'thru': {0x200000000: 'N', 0x100000000: 'Y'},
          'cross': {0x800000000: 'N', 0x400000000: 'Y'}}
# Every code the tactics screen can produce (formation's code table, TOC -0x7474).
UI_CODES = [0x1] + [l | s for l in (0x4, 0x8, 0x10, 0x20, 0x40) for s in SIDES] + [0x202]


class Tactic:
    def __init__(self, path):
        d = open(path, 'rb').read()
        big = 0x98ec54 <= struct.unpack('>I', d[:4])[0] <= 0x98ec5e
        self.order = '>' if big else '<'
        E = self.order
        if struct.unpack(E + 'I', d[:4])[0] != 0x98ec5e:
            raise ValueError(f'{path}: only the current .tct format is read')
        o = 4 + 0x32 + 0xc8
        self.head = d[:o]
        (self.shape,) = struct.unpack(E + 'I', d[o:o + 4]); o += 4
        self.pos = list(struct.unpack(E + '11h', d[o:o + 22])); o += 22
        self.display = d[o:o + 11]; o += 11
        self.posb = list(struct.unpack(E + '11h', d[o:o + 22])); o += 22
        self.spots = list(struct.unpack(E + '528H', d[o:o + 0x420])); o += 0x420
        (self.team,) = struct.unpack(E + 'I', d[o:o + 4]); o += 4
        self.instr = list(struct.unpack(E + '11Q', d[o:o + 88])); o += 88
        self.role = d[o:o + 11]; o += 11
        self.trailer = d[o:]

    def save(self, path):
        E = self.order
        with open(path, 'wb') as f:
            f.write(self.head + struct.pack(E + 'I', self.shape) + struct.pack(E + '11h', *self.pos) + self.display +
                    struct.pack(E + '11h', *self.posb) + struct.pack(E + '528H', *self.spots) +
                    struct.pack(E + 'I', self.team) + struct.pack(E + '11Q', *self.instr) + self.role + self.trailer)


def spot_index(player, ball, row, column):
    """Index of x in Tactic.spots (y follows): player, without (0) or with (1)
    the ball, the ball's cell (3 rows across the pitch, 4 columns up it)."""
    return (((player * 2 + ball) * 3 + row) * 4 + column) * 2


def code_name(code):
    lines = '/'.join(n for b, n in LINES.items() if code & b)
    sides = '/'.join(n for b, n in SIDES.items() if code & b)
    return lines + ('-' + sides if sides else '') + (f'+{code & ~0xfff:#x}' if code & ~0xfff else '')


def field(value, table):
    return '|'.join(n for b, n in table.items() if value & b) or '-'


def describe(t):
    lines = ['team ' + ' '.join(f'{k}={field(t.team, v)}' for k, v in TEAM.items())]
    for p in range(11):
        lines.append(f'{p:2d} {code_name(t.pos[p]):>8} -> {code_name(t.posb[p]):<8} role={t.role[p]:#04x} ' +
                     ' '.join(f'{k}={field(t.instr[p], v)}' for k, v in PLAYER.items()))
    for ball in (0, 1):
        lines.append(('with' if ball else 'without') + ' the ball: (x 0..26 across, y 0..35 from our goal) per cell row.column')
        for p in range(11):
            cells = [t.spots[spot_index(p, ball, r, c):spot_index(p, ball, r, c) + 2] for r in range(3) for c in range(4)]
            lines.append(f'{p:2d} ' + ' '.join(f'{x:2d},{y:2d}' for x, y in cells))
    return '\n'.join(lines)


# --- evaluation ---
# (name, home club, away club, side the tactic plays)
PANELS = {
    'train': [('ARS-MUN', 677, 5677, 0), ('MUN-ars', 5677, 677, 1), ('LEI-LIV', 5206, 5346, 0),
              ('CHE-bol', 1953, 1376, 1), ('TOT-sou', 9409, 8670, 1), ('CRU-LEE', 2262, 5190, 0),
              ('FUL-NEW', 3416, 6377, 0), ('EVE-ips', 2993, 4383, 1)],
    'holdout': [('DER-AST', 2433, 731, 0), ('NEW-lee', 6377, 5190, 1), ('SUN-CHA', 8992, 1936, 0),
                ('JUV-RMA', 4605, 7559, 0), ('BOL-LIV', 1376, 5346, 0), ('LIV-ars', 5346, 677, 1),
                ('IPS-MUN', 4383, 5677, 0), ('FUL-tot', 3416, 9409, 1)],
    # Never used by a search: the final verdict.
    'validation': [('LEE-CHE', 5190, 1953, 0), ('SOU-ast', 8670, 731, 1), ('EVE-MUN', 2993, 5677, 0),
                   ('CHA-ARS', 1936, 677, 0), ('IPS-der', 4383, 2433, 1), ('TOT-sun', 9409, 8992, 1),
                   ('JUV-rma', 4605, 7559, 1), ('NEW-LIV', 6377, 5346, 0)],
}


class Lab:
    def __init__(self, processes=10):
        self.lab = os.path.join(BUILD, 'cm-tactic-lab')
        self.program, self.game = cache_path('CM_ORACLE_DIR'), cache_path('CM_MATCH_DEBUGGER_GAME_DIR')
        self.scratch = tempfile.mkdtemp(prefix='tactic-lab-')
        self.processes = processes
        self.assertions = {}  # tactic index -> assertion reports the game would show

    def chunk(self, tactics, matchup, first, count, indexes=None):
        """Rows (matchup, tactic index, seed, (for, against) or None). A
        process that dies (an engine crash) is replayed one tactic at a time;
        the crashing tactic's missing matches count as failed."""
        indexes = indexes or list(range(len(tactics)))
        name, home, away, side = matchup
        out = subprocess.run([self.lab, self.program, self.game, self.scratch, '--home', str(home), '--away', str(away),
                              '--side', str(side), '--seeds', str(count), '--first-seed', str(first)] + list(tactics),
                             capture_output=True, text=True)
        rows = []
        for line in out.stdout.splitlines():
            f = line.split()
            if f[0] == 'match':
                rows.append((name, indexes[int(f[1])], int(f[2]), (int(f[3]), int(f[4]))))
                self.assertions[indexes[int(f[1])]] = self.assertions.get(indexes[int(f[1])], 0) + int(f[5])
            elif f[0] == 'error':  # a failed match counts against its tactic
                rows.append((name, indexes[int(f[1])], int(f[2]), None))
        if out.returncode:
            if 'Usage' in out.stderr or 'Bad arguments' in out.stderr or 'Cannot read' in out.stderr:
                raise RuntimeError(f'{name}: {out.stderr.strip()}')
            if len(tactics) > 1:
                return [row for t, path in zip(indexes, tactics) for row in self.chunk([path], matchup, first, count, [t])]
            seen = {row[2] for row in rows}
            rows += [(name, indexes[0], seed, None) for seed in range(first, first + count) if seed not in seen]
        return rows

    def play(self, tactics, panel, first, seeds, per_process=4):
        """{tactic index: {(matchup, seed): (for, against) or None}}"""
        tasks = [(m, s, min(per_process, first + seeds - s)) for m in panel for s in range(first, first + seeds, per_process)]
        results = {i: {} for i in range(len(tactics))}
        with ThreadPoolExecutor(self.processes) as pool:
            for rows in pool.map(lambda task: self.chunk(tactics, *task), tasks):
                for name, t, seed, score in rows:
                    results[t][(name, seed)] = score
        return results


def points(score):
    return 3 if score[0] > score[1] else 1 if score[0] == score[1] else 0


def objective(score):
    """Points, with goal difference as a small tie-break (lower variance)."""
    return points(score) + 0.2 * (score[0] - score[1])


def paired(a, b, value=objective):
    """Mean and standard error of value(a) - value(b) over common matches; a
    failed match of either side counts as a heavy loss."""
    keys = [k for k in a if k in b]
    d = [(value(a[k]) if a[k] else -10) - (value(b[k]) if b[k] else -10) for k in keys]
    n = len(d) or 1
    m = sum(d) / n
    sd = math.sqrt(sum((x - m) ** 2 for x in d) / max(1, n - 1))
    return m, sd / math.sqrt(n), len(d)


def summary(name, rows, panel):
    played = [s for s in rows.values() if s]
    n = len(played) or 1
    ppg = sum(map(points, played)) / n
    sd = math.sqrt(sum((points(s) - ppg) ** 2 for s in played) / max(1, n - 1))
    per = []
    for m in panel:
        mine = [s for (k, _), s in rows.items() if k == m[0] and s]
        per.append(f'{m[0]}:{sum(map(points, mine)) / max(1, len(mine)):.2f}')
    gf, ga = sum(s[0] for s in played), sum(s[1] for s in played)
    won, drawn = sum(1 for s in played if s[0] > s[1]), sum(1 for s in played if s[0] == s[1])
    failed = sum(1 for s in rows.values() if not s)
    return (f'{ppg:.3f}+-{sd / math.sqrt(n):.3f} ppg  W-D-L {won}-{drawn}-{len(played) - won - drawn}'
            f'  goals {gf}-{ga} ({gf / n:.2f}-{ga / n:.2f})  n={len(played)}'
            f'{f" FAILED {failed}" if failed else ""}  {name}  ' + ' '.join(per))


def race(tactics, lab, keep=4, first=200000, log=sys.stdout):
    """Successive halving on both panels: every round plays the field on fresh
    seeds, twice as many as the round before, and keeps the better half by
    points over all its matches so far (every survivor played the same ones)."""
    panel = PANELS['train'] + PANELS['holdout']
    alive = list(range(len(tactics)))
    totals = {i: {} for i in alive}
    seeds, seed = 2, first
    while True:
        results = lab.play([tactics[i] for i in alive], panel, seed, seeds)
        for j, i in enumerate(alive):
            totals[i].update({(k, s): v for (k, s), v in results[j].items()})
        seed += seeds
        ranked = sorted(alive, key=lambda i: -sum(points(v) if v else -10 for v in totals[i].values()) / len(totals[i]))
        for i in ranked:
            print(summary(tactics[i], totals[i], panel), file=log, flush=True)
        print(f'-- {len(alive)} tactics, {seeds} seeds per matchup this round', file=log, flush=True)
        if len(alive) <= keep:
            return ranked, totals
        alive = ranked[:max(keep, len(alive) // 2)]
        seeds *= 2


def neighbours(t, kind, mode):
    """Every single change of one kind: (description, tactic)."""
    out = []
    keep = (lambda b, r, c: mode == 'exploit' and (b, r, c) == (0, 0, 0))
    if kind == 'team':
        for name, values in TEAM.items():
            for value in values:
                if not t.team & value:
                    n = copy.deepcopy(t)
                    n.team = (t.team & ~sum(values)) | value
                    out.append((f'team {name}={values[value]}', n))
    elif kind == 'instructions':
        for p in range(1, 11):
            for name, values in PLAYER.items():
                for value in values:
                    if not t.instr[p] & value:
                        n = copy.deepcopy(t)
                        n.instr[p] = (t.instr[p] & ~sum(values)) | value
                        out.append((f'p{p} {name}={values[value]}', n))
    elif kind == 'spots':
        for p in range(1, 11):
            for b in range(2):
                for dx, dy in ((-2, 0), (2, 0), (0, -2), (0, 2)):
                    n = copy.deepcopy(t)
                    for row in range(3):
                        for col in range(4):
                            if not keep(b, row, col):
                                i = spot_index(p, b, row, col)
                                n.spots[i] = max(0, min(26, n.spots[i] + dx))
                                n.spots[i + 1] = max(0, min(35, n.spots[i + 1] + dy))
                    out.append((f'shift p{p} b{b} {dx:+d},{dy:+d}', n))
                for col in range(4):
                    for dy in (-3, 3):
                        n = copy.deepcopy(t)
                        for row in range(3):
                            if not keep(b, row, col):
                                i = spot_index(p, b, row, col)
                                n.spots[i + 1] = max(0, min(35, n.spots[i + 1] + dy))
                        out.append((f'column p{p} b{b} {col} {dy:+d}', n))
    elif kind == 'codes':
        for p in range(1, 11):
            for code in UI_CODES[1:]:
                if code != t.pos[p] or code != t.posb[p]:
                    n = copy.deepcopy(t)
                    n.pos[p] = n.posb[p] = code
                    out.append((f'code p{p}={code_name(code)}', n))
    return out


def improve(mode, start, outdir, kinds, lab, screen_seeds=8, confirm_seeds=64, top=6, log=None):
    """Coordinate rounds: every single change of a kind is screened on
    screen_seeds per matchup of both panels; the best `top` are played again
    with the incumbent on confirm_seeds fresh seeds per matchup, and the best
    is accepted when its paired gain exceeds twice its standard error. Ends
    when a whole cycle of kinds accepts nothing: no single change of those
    kinds improves the tactic by more than the confirmation can detect."""
    os.makedirs(outdir, exist_ok=True)
    log = log or open(os.path.join(outdir, 'log.txt'), 'a')
    panel = PANELS['train'] + PANELS['holdout']
    incumbent = Tactic(start)
    if mode == 'exploit':
        protect(incumbent)
    seed, round_number, quiet = 300000, 0, 0
    while quiet < len(kinds):
        kind = kinds[round_number % len(kinds)]
        folder = os.path.join(outdir, f'r{round_number:02d}-{kind}')
        os.makedirs(folder, exist_ok=True)
        current = os.path.join(folder, 'incumbent.tct')
        incumbent.save(current)
        changes = neighbours(incumbent, kind, mode)
        paths = []
        for k, (_, t) in enumerate(changes):
            paths.append(os.path.join(folder, f'n{k:03d}.tct'))
            t.save(paths[-1])
        began = time.time()
        screen = lab.play([current] + paths, panel, seed, screen_seeds)
        seed += screen_seeds
        gains = [paired(screen[k + 1], screen[0])[0] for k in range(len(paths))]
        best = sorted(range(len(paths)), key=lambda k: -gains[k])[:top]
        confirm = lab.play([current] + [paths[k] for k in best], panel, seed, confirm_seeds)
        seed += confirm_seeds
        accepted = None
        for j, k in enumerate(best):
            m, se, n = paired(confirm[j + 1], confirm[0], points)
            print(f'r{round_number} {kind} screen {gains[k]:+.3f} confirm {m:+.3f}+-{se:.3f} n={n} {changes[k][0]}',
                  file=log, flush=True)
            if m > 2 * se and (accepted is None or m > accepted[1]):
                accepted = (k, m, se)
        base = [v for v in confirm[0].values() if v]
        print(f'r{round_number} {kind}: incumbent {sum(map(points, base)) / max(1, len(base)):.3f} ppg, '
              f'{len(paths)} changes, {time.time() - began:.0f}s', file=log, flush=True)
        if accepted:
            incumbent = Tactic(paths[accepted[0]])
            quiet = 0
            print(f'r{round_number} ACCEPT {changes[accepted[0]][0]} {accepted[1]:+.3f}+-{accepted[2]:.3f}', file=log, flush=True)
        else:
            quiet += 1
        incumbent.save(os.path.join(outdir, 'incumbent.tct'))
        round_number += 1
    print('done: no single change of ' + ','.join(kinds) + ' passed', file=log, flush=True)
    return incumbent


# --- search ---
def mutate(t, rng, mode):
    """One or two random changes. Honest moves keep every value the tactics
    screen can make (spots inside 0..26 x 0..35, one value per field); the
    exploit search may also give a player any screen position code
    independently of his spots (kept by the loader bypass, see protect)."""
    t = copy.deepcopy(t)
    done = []
    keep = (lambda b, r, c: mode == 'exploit' and (b, r, c) == (0, 0, 0))

    def move(i, dx, dy):
        t.spots[i] = max(0, min(26, t.spots[i] + dx))
        t.spots[i + 1] = max(0, min(35, t.spots[i + 1] + dy))

    for _ in range(rng.choice([1, 1, 2])):
        r, p = rng.random(), rng.randint(1, 10)
        if r < 0.30:
            b, row, col = rng.randint(0, 1), rng.randint(0, 2), rng.randint(0, 3)
            if keep(b, row, col):
                continue
            dx, dy = rng.randint(-4, 4), rng.randint(-4, 4)
            move(spot_index(p, b, row, col), dx, dy)
            done.append(f'spot p{p} b{b} {row}.{col} {dx:+d},{dy:+d}')
        elif r < 0.42:
            b, col, dy = rng.randint(0, 1), rng.randint(0, 3), rng.choice([-4, -3, -2, 2, 3, 4])
            for row in range(3):
                if not keep(b, row, col):
                    move(spot_index(p, b, row, col), 0, dy)
            done.append(f'column p{p} b{b} {col} {dy:+d}')
        elif r < 0.52:
            b = rng.randint(0, 1)
            dx, dy = rng.choice([(-3, 0), (3, 0), (0, -3), (0, 3), (-2, 0), (2, 0), (0, -2), (0, 2)])
            for row in range(3):
                for col in range(4):
                    if not keep(b, row, col):
                        move(spot_index(p, b, row, col), dx, dy)
            done.append(f'shift p{p} b{b} {dx:+d},{dy:+d}')
        elif r < 0.80:
            name = rng.choice(list(PLAYER))
            value = rng.choice(list(PLAYER[name]))
            t.instr[p] = (t.instr[p] & ~sum(PLAYER[name])) | value
            done.append(f'p{p} {name}={PLAYER[name][value]}')
        elif r < 0.88:
            name = rng.choice(list(TEAM))
            value = rng.choice(list(TEAM[name]))
            t.team = (t.team & ~sum(TEAM[name])) | value
            done.append(f'team {name}={TEAM[name][value]}')
        elif r < 0.94 or mode != 'exploit':
            q = rng.randint(1, 10)
            if q == p:
                continue
            for b in range(2):
                for row in range(3):
                    for col in range(4):
                        if keep(b, row, col):
                            continue
                        i, j = spot_index(p, b, row, col), spot_index(q, b, row, col)
                        t.spots[i:i + 2], t.spots[j:j + 2] = t.spots[j:j + 2], t.spots[i:i + 2]
            t.instr[p], t.instr[q] = t.instr[q], t.instr[p]
            if mode == 'exploit':
                t.pos[p], t.pos[q], t.posb[p], t.posb[q] = t.pos[q], t.pos[p], t.posb[q], t.posb[p]
            done.append(f'swap p{p} p{q}')
        else:
            code, which = rng.choice(UI_CODES[1:]), rng.choice(['pos', 'posb', 'both'])
            if which != 'posb':
                t.pos[p] = code
            if which != 'pos':
                t.posb[p] = code
            done.append(f'code p{p} {which}={code_name(code)}')
    return t, '; '.join(done) or 'none'


def protect(t):
    """The loader bypass: the loader re-chooses each player's position codes
    from his spots through his first spot without the ball (0x10111e60),
    which refuses x > 26 (one assertion report per player) and so keeps the
    file's codes; x + 48 lands in the same zone (the zone grid keeps x / 3
    & 0xf)."""
    for p in range(1, 11):
        i = spot_index(p, 0, 0, 0)
        if t.spots[i] < 48:
            t.spots[i] += 48


def optimize(mode, start, outdir, generations, lab):
    os.makedirs(outdir, exist_ok=True)
    state_path = os.path.join(outdir, 'state.json')
    if os.path.exists(state_path):
        state = json.load(open(state_path))
        first, incumbent = state['generation'], Tactic(state['incumbent'])
    else:
        first, incumbent = 0, Tactic(start)
        if mode == 'exploit':
            protect(incumbent)
    panel = PANELS['train']
    with open(os.path.join(outdir, 'log.txt'), 'a') as log:
        for g in range(first, generations):
            began = time.time()
            rng = random.Random(f'{mode}:{os.path.basename(start)}:{g}')  # a resumed search goes on as it would have
            folder = os.path.join(outdir, f'g{g:03d}')
            os.makedirs(folder, exist_ok=True)
            current = os.path.join(folder, 'incumbent.tct')
            incumbent.save(current)
            candidates, changes = [], []
            for k in range(16):
                t, change = mutate(incumbent, rng, mode)
                path = os.path.join(folder, f'c{k:02d}.tct')
                t.save(path)
                candidates.append(path)
                changes.append(change)
            seed = 100000 + g * 1000
            screen = lab.play([current] + candidates, panel, seed, 8)
            scores = [paired(screen[k + 1], screen[0])[0] for k in range(16)]
            best = sorted(range(16), key=lambda k: -scores[k])[:4]
            race = lab.play([current] + [candidates[k] for k in best], panel, seed + 500, 24)
            accepted = None
            for j, k in enumerate(best):
                m, se, n = paired(race[j + 1], race[0])
                print(f'g{g} c{k:02d} screen {scores[k]:+.3f} race {m:+.3f}+-{se:.3f} n={n} {changes[k]}', file=log, flush=True)
                if m > se and (accepted is None or m > accepted[1]):
                    accepted = (k, m, se)
            if accepted:
                incumbent = Tactic(candidates[accepted[0]])
                print(f'g{g} ACCEPT c{accepted[0]:02d} {accepted[1]:+.3f}+-{accepted[2]:.3f} {changes[accepted[0]]}',
                      file=log, flush=True)
            print(f'g{g} done in {time.time() - began:.0f}s', file=log, flush=True)
            incumbent.save(os.path.join(outdir, 'incumbent.tct'))
            json.dump({'generation': g + 1, 'incumbent': os.path.join(outdir, 'incumbent.tct')}, open(state_path, 'w'))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    d = sub.add_parser('decode')
    d.add_argument('files', nargs='+')
    e = sub.add_parser('evaluate')
    e.add_argument('--panel', choices=PANELS, default='train')
    e.add_argument('--seeds', type=int, default=20)
    e.add_argument('--first-seed', type=int, default=1)
    e.add_argument('tactics', nargs='+')
    o = sub.add_parser('optimize')
    o.add_argument('mode', choices=['honest', 'exploit'])
    o.add_argument('start')
    o.add_argument('outdir')
    o.add_argument('--generations', type=int, default=40)
    i = sub.add_parser('improve')
    i.add_argument('mode', choices=['honest', 'exploit'])
    i.add_argument('start')
    i.add_argument('outdir')
    i.add_argument('--kinds', default='team,instructions,spots')
    r = sub.add_parser('race')
    r.add_argument('--keep', type=int, default=4)
    r.add_argument('--first-seed', type=int, default=200000)
    r.add_argument('tactics', nargs='+')
    args = parser.parse_args()
    if args.command == 'decode':
        for path in args.files:
            print(f'== {path}\n{describe(Tactic(path))}')
    elif args.command == 'evaluate':
        lab = Lab()
        panel = PANELS[args.panel]
        results = lab.play(args.tactics, panel, args.first_seed, args.seeds)
        for i, name in sorted(enumerate(args.tactics), key=lambda x: -sum(map(points, filter(None, results[x[0]].values())))):
            line = summary(name, results[i], panel)
            if lab.assertions.get(i):
                line += f'  ASSERTION REPORTS {lab.assertions[i]}'  # dialogs in the game
            if i:
                m, se, _ = paired(results[i], results[0], points)
                line += f'  vs first {m:+.3f}+-{se:.3f}'
            print(line)
    elif args.command == 'improve':
        improve(args.mode, args.start, args.outdir, args.kinds.split(','), Lab())
    elif args.command == 'race':
        race(args.tactics, Lab(), args.keep, args.first_seed)
    else:
        optimize(args.mode, args.start, args.outdir, args.generations, Lab())


if __name__ == '__main__':
    main()

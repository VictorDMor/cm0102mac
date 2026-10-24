# Tactic lab: measuring tactics in the real engine

`cm-tactic-lab` (native/tactic_lab_main.cpp) plays complete detailed matches
between two original clubs with a chosen .tct tactic on one side, and
`tools/tactic_lab.py` evaluates tactics over panels of matchups and searches
for better ones. About 0.45 s per match per core; ten processes play ~25
matches per second.

## How a tactic gets into a match

The isolated match factory (match_debugger_factory) prepares the world once
(`prepare_match`); the lab checkpoints it and, per match, reseeds the game's
random pool (`0x107474a0`) and starts the match (`start_match`). Both sides
are AI clubs. The AI team setup `0x10206ea0` chooses a formation
(`0x1020d7d0`), lets the original selector `0x10210c50` pick the players for
its positions, then adjusts the formation to its manager (`0x1020a290`:
passing from a manager attribute, pressing, offside trap, positions,
instructions). A dispatch probe copies the tactic over the side's formation
record in the match (`match + 0x7c6c + side * 0x18e0`) on the return from both
calls, so the selector picks the best players for the tactic's positions and
the manager's adjustments are undone. During the match the AI changes its own
side's mentality, counter attack and forward runs (`0x101e6c50`,
`0x101f7280`, `0x101f7500`); a human side gets none of that, so a write
observer puts the tactic back at once. The side keeps the AI's substitutions,
injury changes and set-piece takers (the human-side test `0x101da5a0` guards
those too).

The tactic is built as the game's loader (`0x10115000`) leaves it: fields
read raw, the team-option fix-up, each player's first spot set again through
`0x10111e60` (which re-chooses his position codes from his spots), zone grids
rebuilt (`0x10116560`).

## What the engine reads (static analysis, 2026-10-24)

- **Spots decide where players go.** For each of 12 ball cells (3 across x
  4 up the pitch) and each ball state a player has a spot, x 0..26 across,
  y 0..35 from his own goal. The grid builder turns them into a 9 x 12 zone
  grid (`x / 3`, `y / 3`, each `& 0xf`); `0x101e28b0` looks up the target zone.
  The tactics screen keeps spots inside 0..26 x 0..35 (`0x10111e60` refuses
  others); a file can hold anything, but spots past the grid are not clamped
  everywhere and corrupt memory: y >= 36 crashes the match.
- **Position codes** (line bit | side bit) give a positional penalty: each
  line and side bit costs `20 - rating there`, a goalkeeper bit twice; a third
  of the total is added to every derived match rating (`0x1022ba10`). Code 0
  costs nothing but every split of it asserts (`0x10116780`, ~500 reports a
  match). Codes also switch behaviour: exactly 1 is the goalkeeper (offside
  line, targets, marking), a central side bit with a DM/M/AM line lets a
  player close down the ball (`0x102836a0`), wide bits with M/AM drive wide
  play. The screen derives codes from spots; a file keeps its own codes when
  each player's first spot without the ball has x > 26 (the loader's re-fit
  refuses it, one assertion report per player, and x + 48 lands in the same
  zone).
- **Team options** the match reads: attacking (push a row up in possession)
  and defensive (drop a row without it) mentality, counter attack (sit a row
  deeper until the opponent commits; replaces forward runs), men behind the
  ball (deep, no man marking, clearances), pressing (with the player's
  closing-down setting), offside trap (defenders step up; opponents' passes
  carry offside risk), passing and tackling (only through players set to
  Team). Normal mentality, Mixed passing, Normal tackling and every "No" bit
  are never read. Several values of one field resolve by a fixed priority,
  except mentality: Attacking and Defensive together apply both.
- **Player instructions**: yes/no fields read only the yes bit; passing,
  tackling and set-piece fields take the first bit in a fixed order; pass-to
  bits add independent bonuses; set pieces place players in the box (attack
  0x20000) or up the pitch (defence 0x80000000).

## Measurements

Panels are in tools/tactic_lab.py (`PANELS`): searches use `train` and
`holdout`; `validation` (8 other matchups) is kept for the final verdict.
Points per match have an SD of ~1.3, so a comparison needs ~1,000 paired
matches (common seeds) for an SE of ~0.04 ppg; 200-match comparisons let a
hill climb accept noise.

- The best community tactics plateau together: of 150 raced on 16 matchups,
  the top five end within 0.06 ppg. Hand-placed spots are worth ~0.4 ppg
  against spots the screen derives from codes.
- From 4132 Marc no single team option, instruction or spot shift gains
  2 SE over 1,024 matches. Three near-misses combined (the playmaker's
  forward runs off, the right back's runs with the ball off, Free Role for the
  holding midfielder) give the honest tactic.
- The file-only exploit: decoupled position codes (the loader bypass above).
  Wide central midfielders coded M-C and both strikers coded F-RC while they
  keep their spots, and the left back's no-ball column 1 three rows up
  (+0.112 +- 0.039; worth nothing without the central codes: -0.002 +-
  0.041 on the honest tactic).
- Both final files are certified on fresh seeds: no single change of a team
  option (11), instruction (230), spot shift (240) or, for the exploit,
  position code (251) gains 2 SE over 1,024 paired matches.
- Validation (8 unseen matchups x 256 seeds = 2,048 matches each, paired with
  4132 Marc), two seed sets: exploit 2.153 ppg (+0.219 +- 0.035), its earlier
  version without the left-back move 2.092 / 2.114, honest 2.002 / 1.987
  (+0.077 / +0.052 +- 0.035), scemoka 1.954, Marc 1.925 / 1.935, the AI's own
  choice 0.562.
- `load:FILE` reads a file with the game's own loader: the exploit raises 10
  assertion reports (one per outfield player) and keeps its codes, the honest
  file none; both play the same matches as the lab's own build. Neither raises
  an assertion during matches. In the app the tactics screen's Load shows the
  ten reports as "Error v3.9.62 FORMATION..CPP 712" (Ok or Return each); the
  record then holds the file unchanged, and Ok makes it the club's. A save and
  reload keeps it, with no further dialogs.
- Multi-start (honest): improve from 4-1-2-1-2 scemoka and 262 MMJ accepted
  nothing in a full cycle (team, instructions, spots); both starts are already
  single-change optima, in other basins than Lab Honest. Validation on a
  third seed set (2,048 matches each, paired with Lab Honest): exploit 2.124
  (+0.152 +- 0.035), honest 1.972, scemoka 1.925 (-0.046), Marc 1.915
  (-0.057), MMJ 1.859 (-0.112). Lab Honest's basin stays the best found.

## Running another search

The final files are in `.local/tactics-optimal/` (`Lab Honest.tct`,
`Lab Exploit.tct`, certificates and validation logs). Starting points: the
3,526 community tactics in `.local/community-tactics-full/tested/<name>/`,
ranked in `ranking.json`. Build the lab with `tools/dev.sh build cm-tactic-lab`.

```
python3 tools/tactic_lab.py decode FILE.tct              # readable dump
python3 tools/tactic_lab.py evaluate --panel validation --seeds 256 \
    --first-seed N A.tct B.tct ...                       # paired with the first file
python3 tools/tactic_lab.py improve honest|exploit START.tct OUTDIR \
    --kinds team,instructions,spots[,codes] --first-seed N --processes 10
python3 tools/tactic_lab.py race --first-seed N *.tct    # successive halving
```

A tactic argument can also be `ai` (the AI's own choice) or `load:FILE` (read
through the game's loader). `improve` writes `OUTDIR/log.txt` (every
confirmation, `ACCEPT` lines, `done:`) and the current `OUTDIR/incumbent.tct`.
The improve run is the certificate: it ends when a whole cycle of the kinds
accepts nothing. Costs at 10 processes: a team round takes ~8 min, an
instructions or spots round ~30 min, and a codes round about the same.

Rules that kept results honest:
- Seeds 1..1,199,999 are spent (search, certificates, validation). Start new
  work at 2,000,000 or above, and give every run its own range. Never search on
  the `validation` panel; use it only for the final verdict.
- Accept only on paired comparisons of about 1,000 matches (gain > 2 SE). The
  8-seed screen ranks candidates but its top gains are mostly noise: +0.37 on
  the screen, -0.01 on confirmation.
- Exploit tactics must keep the bypass (`protect()`, cell-0 x + 48) or the
  loader re-fits the codes. Spots with y >= 36 crash matches.

What a coordinate search cannot see, and what a next loop could try:
- Two-step moves. The left-back move is worth +0.11 only together with the
  central codes. A pair search over the near-misses (screen gain high,
  confirmation +1 to +2 SE) is the obvious next step.
- Other basins. Every strong start (Marc, scemoka, MMJ) is its own local
  optimum within ~0.1 ppg. More starts from `ranking.json`, or a search on
  formation shape (positions plus spots together), could find a higher one.
- The panel. All matchups are clubs from the original database with AI
  opponents, and the tactic is fitted to the 16 search matchups. Per-matchup
  spread is large (CHA-ARS ~0.4 ppg, SOU-ast ~3.0), so a tactic fitted to
  weak or strong squads may differ.
- Not measured: the exploit after editing it in the tactics screen (the screen
  may re-fit codes), human-controlled sides other than the lab's injection,
  and set-piece instruction bits beyond single changes.

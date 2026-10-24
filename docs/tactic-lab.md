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
  an assertion during matches.

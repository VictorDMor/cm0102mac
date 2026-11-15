# Debugging: player attributes

**Maximize Attributes** (**Maximizar Atributos**) is a one-shot action under a
player's **Action → Debugging** menu, with its own toggle under **Debugging
Options → Enable/Disable Options**. It works for real players, including those
without a contract. The action refreshes the profile immediately.

The 50 changed bytes use the original scales: 18 signed technical attributes
receive raw 100 (displayed as 20 by the original ability-dependent profile getter),
22 other positive attributes and eight personality attributes receive 20.
**Dirtiness** and **Injury Proneness** receive **1**, their best valid value.
Current/potential ability, positions, employment, identity and contract terms
are unchanged. The action consumes no game RNG.

Values remain ordinary player data and are written by normal saves. Training,
ageing and other game logic can change them afterwards; the command can be
applied again. There is no lock, timer, per-store interception or added work in
the simulation loops. Restore Original Attributes is not restored, and the old
`DebugAttributeLocks` metadata is not read or written.

The previous locking implementation was removed on 2026-11-05: it checked every
guest memory store and disabled native fast paths. Captured weeks with five
locked players took about 65–75% longer; match replays with ten took about
85–105% longer. This implementation does not restore that mechanism. No new
performance improvement is claimed.

`debugging_player_attributes` uses an existing career snapshot and the original
profile getter/normalizer to check values, hidden and negative attributes,
record boundaries, caller/RNG preservation, disabled/invalid actions and later
unlocked writes. `debugging_options` covers the toggle, submenu construction
from a controlled original hook frame (with and without a contract ledger), and
the production command route, including a stale disabled action. Full profile
drawing needs QuickDraw services absent from the headless fixture; these checks
do not launch the app or perform manual career save/reload.

# Debugging: player attributes

With Debugging enabled, **Action → Debugging → Maximize Attributes** sets the
selected player's positive playing and personality attributes to 20, including
hidden attributes, and sets dirtiness and injury proneness to 1. The player's
current and potential ability, positional familiarity and other records are not
changed. The resulting attribute values stay locked even if Debugging is later
disabled.

**Action → Debugging → Restore Original Attributes** restores the exact attribute
bytes captured before the first maximization and removes the lock. It appears
only for a player with a retained snapshot. Repeating Maximize Attributes keeps
the first snapshot; maximizing after restoration captures a new one. Only the
50 protected bytes are restored, preserving subsequent changes to ability,
position, contract, condition and other fields. Technical display values still
use the current ability-dependent conversion.

Locks created by older versions have no original snapshot and cannot be restored.
They continue to load and stay locked; maximizing again does not invent a snapshot
from already-maximized values.

Technical bytes use the original ability-dependent profile conversion: raw 100
displays as 20 without changing current ability, including the goalkeeper-specific
conversion. The lock protects the selected bytes from guest writes while leaving
other bytes and players writable. Registrations are discarded when a new career
starts, the known regeneration and player-to-staff paths replace a record, or
the corresponding record pool is destroyed.

The original save format remains unchanged. The native app records locked player
identities and their pre-maximize bytes in its writable data directory after a
successful save. Metadata is associated with both the save path and the saved file contents, and each player
identity is checked again on load. The game's Save As creates the corresponding
metadata for the new save. Moving or copying a save outside the game does not
transfer its lock metadata or original snapshot; its saved attribute values remain
in the save, but the lock must be applied again at the new path.

Native metadata version 2 stores each snapshot as a count and unsigned raw bytes
in technical/positive/negative/personality order. Version 1 remains readable with
no snapshot. Metadata is fully validated before applying locks; invalid lengths,
bytes, duplicate identities and trailing data are rejected. Restoring the last
locked player still writes empty metadata on the next successful save, so an old
lock cannot return. Record invalidation discards both the lock and its snapshot.

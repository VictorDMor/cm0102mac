# Debugging: player attributes

With Debugging enabled, **Action → Debugging → Maximize Attributes** sets the
selected player's positive playing and personality attributes to 20, including
hidden attributes, and sets dirtiness and injury proneness to 1. The player's
current and potential ability, positional familiarity and other records are not
changed. The resulting attribute values stay locked even if Debugging is later
disabled.

Technical bytes use the original ability-dependent profile conversion: raw 100
displays as 20 without changing current ability, including the goalkeeper-specific
conversion. The lock protects the selected bytes from guest writes while leaving
other bytes and players writable. Registrations are discarded when a new career
starts, the known regeneration and player-to-staff paths replace a record, or
the corresponding record pool is destroyed.

The original save format remains unchanged. The native app records locked player
identities in its writable data directory after a successful save. Metadata is
associated with both the save path and the saved file contents, and each player
identity is checked again on load. The game's Save As creates the corresponding
metadata for the new save. Moving or copying a save outside the game does not
transfer its lock metadata; its saved attribute values remain in the save, but
the lock must be applied again at the new path.

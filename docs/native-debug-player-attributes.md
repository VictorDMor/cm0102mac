# Debugging: player attributes (removed)

The Debugging options **Maximize Attributes** and **Restore Original Attributes**
were removed on 2026-11-05. Keeping the maximized values locked required a check
on every guest memory store and disabled several native fast paths. With five
locked players, a captured week replay took about 65-75% longer, and captured
match replays with ten took about 85-105% longer.

Careers saved while players were locked keep their maximized attribute values,
because the original save format always stored them. They are no longer
protected, so the game changes them as it changes any other player's. The
`DebugAttributeLocks` metadata in the writable data directory is no longer read
or written; the old pre-maximize snapshots there are not applied.

# SpellBinding

Recreated from `Sources/SpellBinding` and the reusable `Template.zip`.
CommonLibSSE-NG is pinned to `a6d3ea4642f71fb92ac3e0d7fef5808217f8aae9`.
Original source snapshots remain untouched. Author and release version metadata were recovered read-only from the existing root Git history.

Build with `cmake --preset ng` then `cmake --build --preset ng-release`.
Packages are written to `dist`. These are migration builds pending in-game testing,
including verification of mod-specific hook slots and runtime integrations on 1.7.x.

The original serialization callbacks, input registrations, mastery systems and UI initialization order are retained. The menu and HUD are built from `web` and packaged under `PrismaUI/views/SpellBinding`. Install PrismaUI separately at runtime.

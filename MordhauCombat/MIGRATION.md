# MordhauCombat

Recreated from `Sources/MordhauCombat` and the reusable `Template.zip`.
CommonLibSSE-NG is pinned to `a6d3ea4642f71fb92ac3e0d7fef5808217f8aae9`.
Original source snapshots remain untouched. Author and release version metadata were recovered read-only from the existing root Git history.

Build with `cmake --preset ng` then `cmake --build --preset ng-release`.
Packages are written to `dist`. These are migration builds pending in-game testing,
including verification of mod-specific hook slots and runtime integrations on 1.7.x.

All 108 HKX animations and 19 routing JSON files are under `data/meshes` and included in the package. The vector overlay renderer hooks use legacy fixed offsets and are logged and skipped outside 1.5.97/1.6.1170, or if the expected CALL opcodes are absent. Combat/input/OAR registration remains active.

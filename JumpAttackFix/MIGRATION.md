# JumpAttackFix

Recreated from `Sources/JumpAttackFix` and the reusable `Template.zip`.
CommonLibSSE-NG is pinned to `a6d3ea4642f71fb92ac3e0d7fef5808217f8aae9`.
The active, buildable source is in this project folder. The original migration layout remains in the prior Git history. Author and release version metadata were recovered from that history.

Build with `cmake --preset ng` then `cmake --build --preset ng-release`.
Packages are written to `dist`. These are migration builds pending in-game testing,
including verification of mod-specific hook slots and runtime integrations on 1.7.x.

The first-person landing/JumpLandEnd behavior is retained in the player-update hook. The unrelated template console demonstration has been removed from the compiled mod code.

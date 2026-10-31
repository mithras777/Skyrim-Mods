# DiagonalSprint

Recreated from `Sources/DiagonalSprint` and the reusable `Template.zip`.
CommonLibSSE-NG is pinned to `a6d3ea4642f71fb92ac3e0d7fef5808217f8aae9`.
The active, buildable source is in this project folder. The original migration layout remains in the prior Git history. Author and release version metadata were recovered from that history.

Build with `cmake --preset ng` then `cmake --build --preset ng-release`.
Packages are written to `dist`. These are migration builds pending in-game testing,
including verification of mod-specific hook slots and runtime integrations on 1.7.x.

The custom `test reload` console handler is retained. Its legacy call-site hook runs only on 1.5.97 and 1.6.1170 after a CALL-opcode check; it is logged and skipped on 1.7.x until its offsets are verified. Sprint input and player-update hooks are independent of that console handler.

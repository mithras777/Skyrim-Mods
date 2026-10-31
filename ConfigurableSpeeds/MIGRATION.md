# ConfigurableSpeeds

Recreated from `Sources/ConfigurableSpeeds` and the reusable `Template.zip`.
CommonLibSSE-NG is pinned to `a6d3ea4642f71fb92ac3e0d7fef5808217f8aae9`.
The active, buildable source is in this project folder. The original migration layout remains in the prior Git history. Author and release version metadata were recovered from that history.

Build with `cmake --preset ng` then `cmake --build --preset ng-release`.
Packages are written to `dist`. These are migration builds pending in-game testing,
including verification of mod-specific hook slots and runtime integrations on 1.7.x.

The original shutdown restoration and settings/startup ordering are retained. The settings panel uses the cached SKSE Menu Framework consumer SDK.

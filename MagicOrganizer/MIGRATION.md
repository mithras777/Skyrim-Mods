# MagicOrganizer

Recreated from `Sources/MagicOrganizer` and the reusable `Template.zip`.
CommonLibSSE-NG is pinned to `a6d3ea4642f71fb92ac3e0d7fef5808217f8aae9`.
Original source snapshots remain untouched. Author and release version metadata were recovered read-only from the existing root Git history.

Build with `cmake --preset ng` then `cmake --build --preset ng-release`.
Packages are written to `dist`. These are migration builds pending in-game testing,
including verification of mod-specific hook slots and runtime integrations on 1.7.x.

Menu registration and drawing use the cached SKSE Menu Framework consumer SDK (ImGuiMCP); the mod no longer links a separate ImGui implementation for framework menu callbacks.

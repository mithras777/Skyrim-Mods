# Skyrim SKSE template with shared Libraries

Extract `Template.zip` into a new mod folder beside `Libraries`, then run:

```powershell
cmake --preset ng
cmake --build --preset ng-release
# Or: cmake --build --preset ng-debug
```

The extracted folder name becomes the plugin name and DLL name. Folder names with
spaces work. Edit `project.cmake` for your name, version and description, or use
`-DMOD_NAME=MyPlugin` to override the folder name. Project targets use an internal
identifier while the DLL and SKSE metadata preserve the requested name.

Requires Windows x64, CMake 3.24+, Visual Studio 2022 with v143 C++ tools and a Windows
SDK (including fxc). Node/npm are needed only to prepare or build web UI packages.
The dependency cache contains sources: each mod compiles its own libraries using
`/MT` for RelWithDebInfo and `/MTd` for Debug. VR is disabled.

## Libraries

By default the cache is the sibling `../Libraries` directory. It contains:

- `native/<library>/<full-commit>/`: immutable upstream source snapshots.
- `.archives/`: archives verified against SHA-256 in `dependencies.lock.json`.
- `web/<package-lock-hash>/win32-x64/node_modules/`: reusable web packages.
- `npm-cache/`: npm's local download cache.

Each project gets independent native copies in `extern`, and web dependencies in
its own `node_modules`. No shared source is patched or built in place. Completed
copies are reused on later configurations, preserving local edits. New dependency
revisions get separate directories. Move a damaged or intentionally replaced local
copy aside before configuring; the resolver refuses to overwrite unmanaged copies.

Missing dependencies download automatically at their locked revision. A warm cache
works without Git or network access. To fill the entire cache, including optional
recipes and the web packages used by SpellBinding:

```powershell
cmake -P tools/PrepareLibraries.cmake
# Native sources only:
cmake -DSKYRIM_PREPARE_WEB=OFF -P tools/PrepareLibraries.cmake
# Override the cache location:
cmake --preset ng -DSKYRIM_LIBRARIES_DIR="D:/Skyrim Libraries"
# Require an existing cache (no network fallback):
cmake --preset ng -DSKYRIM_ALLOW_DOWNLOADS=OFF
```

Downloads and cache publication use per-dependency locks and staging directories.
Interrupted staging is recovered on retry. A corrupt archive fails its hash check
without becoming a ready cache entry. Remove the named corrupt archive and retry.
Licenses/notices remain in both cache and project snapshots.

## Optional libraries

All integrations default off. Enable only what a mod uses:

```powershell
cmake --preset ng -DMOD_WITH_JSON=ON -DMOD_WITH_SKSE_MENU=ON
```

| Option | Consumer interface |
| --- | --- |
| `MOD_WITH_JSON` | `<nlohmann/json.hpp>`, REX JSON; `Skyrim::JSON` |
| `MOD_WITH_INI` | `<SimpleIni.h>`, REX INI; `Skyrim::INI` |
| `MOD_WITH_TOML` | `<toml.hpp>`, REX TOML; `Skyrim::TOML` |
| `MOD_WITH_IMGUI` | `<imgui.h>`, DX11/Win32 backends; `Skyrim::ImGui` |
| `MOD_WITH_SKSE_MENU` | `<SKSEMenuFramework.h>`; `Skyrim::Menu` |
| `MOD_WITH_PRISMA_UI` | `<PrismaUI_API.h>`; `Skyrim::Prisma` |
| `MOD_WITH_OAR` | `<OpenAnimationReplacerAPI-Conditions.h>`, Functions/Animations APIs; `Skyrim::OAR` |

SKSE Menu Framework provides its own ImGui forwarding API (`ImGuiMCP`); it does not
require linking an independent ImGui renderer. The separate ImGui option is for
mods that render their own overlay. OAR UI/context-sharing helpers are not linked;
the Conditions, Functions and Animations consumer helpers are available.

The pinned menu SDK needs explicit namespace-local struct forward declarations to
coexist with standalone ImGui regardless of include order. Configuration applies
that small, idempotent compatibility adjustment only to the project's SDK copy and
adds a patch notice. Cached originals are never changed.
The PrismaUI consumer header also gets an explicit `GetModuleHandleW` call for its
wide DLL-name literal, allowing both Unicode and non-Unicode host projects.

The cache also preserves PO3/binary_io and RapidJSON recipes for later custom
integration. The supported ready-to-build preset uses CommonLibSSE-NG.

These options supply development headers/sources. Players still install compatible
SKSE, Address Library, and any optional runtime framework your mod actually uses.

## Starter behavior

The Basic starter includes logging, hook entry points, console example sources and
menu open/close event registration. `src/api/skse_api.cpp` initializes these through
SKSE messages. Add your mod behavior in `src` and its declarations in `include`.

The legacy console demo's hardcoded offsets have not been verified for 1.7.x. It is
off by default and logs that decision. The opt-in `MOD_ENABLE_DEMO_CONSOLE_HOOK`
only attempts the legacy 1.5.97 / 1.6.1170 sites and checks for a CALL opcode before
installing; it always skips other runtimes. Do not treat compiling against updated
NG as proof that a custom hook is correct on every game version.

## Web builds

Put a web project with `package.json` and `package-lock.json` in `web`, then enable
`MOD_BUILD_WEB=ON`. `SKYRIM_WEB_DIR` can point to a different web project. The build
copies the matching cached Windows x64 package tree, runs `npm run build`, and
stages top-level HTML/CSS/JS and `assets` under `PrismaUI/views/<plugin-name>`.
It does not import SpellBinding's UI into a new mod. Other output layouts can be
placed under `data` by your web build script.

For a standalone web project:

```powershell
cmake -DSKYRIM_WEB_DIR="C:/path/to/web" -P tools/BuildWeb.cmake
```

## Packaging and maintenance

Put mod-manager Data files under `data`. Every successful plugin build recreates
its configuration's staging tree and writes a ZIP under `dist` containing
`SKSE/Plugins/<name>.dll`, its PDB, `data`, and dependency license notices. The ZIP
opens directly to Data-folder contents, without a containing project directory.
Your own plugin license is independent of dependency license obligations.

Export the reusable source archive with:

```powershell
cmake -P tools/ExportTemplate.cmake
```

This writes `../Template.zip` using an explicit source allowlist. It excludes Git
metadata, library snapshots, node_modules, old upstream scaffolds and build output.
The upstream `cmake/templates` and old dependency scripts remain reference material
in this checkout; the new root project does not execute them or replace upstream
CommonLib source files with old patches.

`dependencies.lock.json` is the version authority. For an intentional upgrade,
edit the requested refs in `tools/lock_dependencies.py`, run it with Python and Git
(the maintainer tools only), prepare Libraries, validate the new sources, and export
again. Normal configuration never follows a moving branch. Source archives omit
submodules; each required transitive dependency is a separate locked cache entry.

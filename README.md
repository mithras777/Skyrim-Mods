# Create and build Skyrim SKSE mods

Use `Template.zip` to create a new mod. Keep each mod folder directly beside
`Libraries`; CMake will copy the dependencies it needs into that mod's `extern`
folder. You do not need to clone the template again, run an interactive setup,
or copy libraries by hand.

## 1. Check the tools

Native builds require:

- Windows x64.
- Visual Studio 2022 with **Desktop development with C++**, the **v143** toolset,
  and a Windows SDK including the HLSL compiler (`fxc`).
- CMake 3.24 or newer, available in your terminal's PATH.

Open PowerShell or Windows Terminal. The Visual Studio CMake preset finds the
installed compiler automatically. Node.js and npm are needed only for web UI work.
Git is needed for template maintenance tooling, not ordinary warm-cache builds.

## 2. Extract a new mod folder

Choose the mod's name first. This example creates **MyNewMod**:

```powershell
Set-Location 'C:\Coding\Skyrim Mods'

# Choose another name if this folder already exists.
if (Test-Path -LiteralPath '.\MyNewMod') {
    throw 'MyNewMod already exists. Choose a new folder name.'
}

Expand-Archive -LiteralPath '.\Template.zip' -DestinationPath '.\MyNewMod'
Set-Location '.\MyNewMod'
```

Replace `MyNewMod` with your chosen name throughout the examples. You can also
extract the ZIP with File Explorer and name the destination folder yourself.

The result must look like this:

```text
C:\Coding\Skyrim Mods\
├── Libraries\
├── Template.zip
├── README.MD
└── MyNewMod\
    ├── CMakeLists.txt
    ├── CMakePresets.json
    ├── project.cmake
    ├── dependencies.lock.json
    ├── src\
    ├── include\
    ├── pch\
    ├── resource\
    ├── data\
    ├── cmake\
    └── tools\
```

`CMakeLists.txt` must be directly inside `MyNewMod`. If extraction created an
extra wrapper folder, use the folder containing `CMakeLists.txt` as the project
folder and keep it beside `Libraries`.

**Run all remaining CMake commands from inside the new mod folder**, not from
`C:\Coding\Skyrim Mods`.

## 3. Set the mod's details and libraries

Before the first configure, open `MyNewMod\project.cmake` and edit the existing
metadata lines. For example:

```cmake
set(MOD_NAME "" CACHE STRING "Plugin name; empty uses the extracted folder name")
set(MOD_AUTHOR "Mithras666" CACHE STRING "Plugin author")
set(MOD_VERSION "1.0.0" CACHE STRING "Plugin version")
set(MOD_DESCRIPTION "Describe what your mod does" CACHE STRING "Plugin description")
set(MOD_LICENSE "GPL-3.0-or-later" CACHE STRING "License for your own plugin code")
set(MOD_SOURCE_URL "" CACHE STRING "Public source repository URL")
```

Leave `MOD_NAME` empty to use the folder name: `MyNewMod` produces `MyNewMod.dll`.
Set it explicitly only when you want a different plugin name. Folder names with
spaces are supported. Keep the option declarations below the metadata lines.
Before packaging for release, set `MOD_SOURCE_URL` to the public GitHub folder
containing this mod's source. The release ZIP writes that link into its root
`README.txt` alongside the full `LICENSE.txt`.

All optional integrations default to `OFF`. In the same file, change the options
your mod needs to `ON`. For example, JSON settings and a framework settings panel
use `MOD_WITH_JSON` and `MOD_WITH_SKSE_MENU`. See the options table below.

The template uses pinned CommonLibSSE-NG revision
`a6d3ea4642f71fb92ac3e0d7fef5808217f8aae9`, with SE/AE enabled and VR disabled.
Native code uses C++23 and the static MSVC runtime: `/MT` for release and `/MTd`
for Debug. Updating a dependency is a separate maintenance step; normal builds
keep the locked revisions.

## 4. Configure and build the starter

From `C:\Coding\Skyrim Mods\MyNewMod`:

```powershell
cmake --preset ng
cmake --build --preset ng-release
```

`CMake_Build.bat` inside the extracted mod folder runs these same configure and
release-build commands if you prefer the batch helper.

Wait for configuration to succeed before running the build. Configuration copies
selected dependency sources from `Libraries` into this project's `extern` folder.
If a required locked revision is missing, it downloads that revision into
`Libraries` first. Existing cached revisions are reused.

The first build compiles the dependencies separately for this mod and can take
considerably longer than later builds. Later builds reuse its compiled outputs.
A successful first build produces the **starter plugin**; add your actual mod
behavior in the next step.

For a Debug build, configure with the same `ng` preset, then run:

```powershell
cmake --build --preset ng-debug
```

`ng` is the configure preset. `ng-release` and `ng-debug` are build presets.
Release builds use the CMake configuration name `RelWithDebInfo` and include a PDB.

## 5. Add the mod's behavior and rebuild

Edit the source files inside your new project:

| Location inside the mod | Purpose |
| --- | --- |
| `src` and `include` | Your C++ implementations and headers. Matching subfolders are supported. |
| `src/api/skse_api.cpp` | SKSE message callbacks and registration timing. |
| `src/plugin.cpp` | Plugin startup and logging. |
| `src/hook/MainHook.cpp` and `src/hook/InputHook.cpp` | Hook setup and implementations. |
| `src/event/GameEventManager.cpp` | Starter event handling. |
| `data` | Game files to package, using paths relative to the game's Data folder. |
| `project.cmake` | Metadata and optional integration defaults. |
| `dependencies.lock.json` | Exact dependency revisions and archive hashes. |
| `extern` and `build` | Local dependency copies and generated build files. Keep your mod code in `src`/`include`. |

For example, place animations under `data\meshes\...` and a shipped plugin
configuration under `data\SKSE\Plugins\...`.

The starter already has the Basic hooks/events structure. Write and register the
behavior your mod needs; enabling an SDK option only supplies its development
interface, not a finished menu, renderer or gameplay feature. The template's old
console demonstration remains disabled by default because its patch offsets have
not been verified for 1.7.x.

After adding source files or changing configuration, run:

```powershell
cmake --preset ng
cmake --build --preset ng-release
```

New `.cpp` files anywhere under `src` are picked up during configuration. For
ordinary edits to existing C++ files, just the build command is sufficient.
Generated version headers come from `project.cmake`; edit the metadata there
rather than editing generated files under `build`.

## 6. Find the DLL and install for testing

With the example name and version above:

| Output | Full example path |
| --- | --- |
| DLL | `C:\Coding\Skyrim Mods\MyNewMod\build\ng\bin\RelWithDebInfo\MyNewMod.dll` |
| Symbols | `C:\Coding\Skyrim Mods\MyNewMod\build\ng\bin\RelWithDebInfo\MyNewMod.pdb` |
| Installable ZIP | `C:\Coding\Skyrim Mods\MyNewMod\dist\MyNewMod-1.0.0-RelWithDebInfo.zip` |
| Uncompressed package | `C:\Coding\Skyrim Mods\MyNewMod\dist\stage\RelWithDebInfo` |

Each successful normal build refreshes the ZIP with the DLL, PDB, your `data`
contents and applicable dependency notices. Changes to the plugin name or version
also change the output filenames. Debug outputs use `Debug` instead of
`RelWithDebInfo`.

Install the ZIP through MO2 or your mod manager so all required assets are included.
For a manual install, extract the ZIP contents into the game's `Data` folder; the
DLL belongs under `Data\SKSE\Plugins`. The ZIP already has that internal layout.

Players need SKSE and Address Library matching their game runtime, plus whichever
runtime frameworks your mod uses, such as SKSE Menu Framework, PrismaUI or Open
Animation Replacer. These framework DLLs are installed separately; enabling their
SDK options does not bundle the runtime frameworks.

Launch Skyrim through SKSE and test the mod's behavior. A successful build verifies
compilation and packaging, not the correctness of custom hooks on a game runtime.

## Changing settings after the first configure

CMake caches metadata and option values. Changing a `set(... CACHE ...)` default
in `project.cmake` after the first configure does **not** replace its cached value.

Update `project.cmake` for future fresh builds, then pass changed values explicitly
for the current build folder. For example:

```powershell
cmake --preset ng -DMOD_AUTHOR="Mithras666" -DMOD_VERSION=1.0.1 -DMOD_DESCRIPTION="My updated description"
cmake --build --preset ng-release
```

Options work the same way:

```powershell
cmake --preset ng -DMOD_WITH_JSON=ON -DMOD_WITH_SKSE_MENU=ON
cmake --build --preset ng-release
```

Alternatively, reconfigure with `cmake --fresh --preset ng` to discard cached
configuration values and read the current defaults from `project.cmake`. Supply
any command-line overrides you still want on that fresh configure. The shared
`Libraries` cache and your local `extern` source copies remain in place.

## Optional integrations

| Option | Supplies | Use it for |
| --- | --- | --- |
| `MOD_WITH_JSON` | nlohmann JSON and REX JSON; `Skyrim::JSON` | JSON configuration or data. |
| `MOD_WITH_INI` | SimpleIni and REX INI; `Skyrim::INI` | INI configuration. |
| `MOD_WITH_TOML` | toml11 and REX TOML; `Skyrim::TOML` | TOML configuration. |
| `MOD_WITH_SKSE_MENU` | SKSE Menu Framework SDK; `Skyrim::Menu` | Settings panels using its `ImGuiMCP` forwarding API. |
| `MOD_WITH_IMGUI` | ImGui and DX11/Win32 backends; `Skyrim::ImGui` | A renderer or overlay you implement yourself. |
| `MOD_WITH_PRISMA_UI` | PrismaUI SDK; `Skyrim::Prisma` | C++ integration with PrismaUI views. |
| `MOD_WITH_OAR` | OAR Conditions, Functions and Animations API helpers; `Skyrim::OAR` | Custom OAR conditions and other API consumers. |
| `MOD_BUILD_WEB` | Build and packaging helper for your web project | Building HTML/CSS/JS alongside the plugin. |

SKSE Menu Framework's settings panels use its own ImGui forwarding API. They do
not require `MOD_WITH_IMGUI`. The separate ImGui option is for an independent
renderer. OAR UI/context-sharing helpers are not linked by this template.

Configuration applies small compatibility adjustments to the project's copied
menu/Prisma SDK headers and includes their patch notices. Shared cached originals
stay unchanged. PO3/binary_io and RapidJSON recipes are also available in the lock
and preparation tools for custom integration; they are not enabled by a separate
ready-made project option.

### Optional web UI

Create your own `web` folder with `package.json`, `package-lock.json`, frontend
sources and a working `npm run build` script. Enable `MOD_BUILD_WEB=ON`; enable
`MOD_WITH_PRISMA_UI=ON` as well if your C++ code uses PrismaUI.

The template does not include SpellBinding's application or a ready-made frontend.
The manifests under `tools/web-package` describe the prepared React, React DOM,
Lucide and esbuild package set. Their build command is `node build.mjs`; provide
your own `web/build.mjs` and frontend files, or change that command to your chosen
builder. The manifests alone are not a complete web project.

The web helper copies the Windows x64 `node_modules` tree matching your exact
package-lock hash from `Libraries`, populating that cache with `npm ci` when
missing and downloads are allowed. A changed lock gets a separate cache entry.
Node/npm are required for this work.

By default the helper builds `web` before the DLL and packages top-level HTML,
CSS, JS and an `assets` folder under `PrismaUI/views/<plugin-name>`. Use
`SKYRIM_WEB_DIR` to select a different web project. Other output layouts can be
placed under `data` by your build script.

## How the shared Libraries folder works

Keep the workspace layout flat: each mod folder and `Libraries` share the same
parent. Ordinary projects copy only their enabled dependencies. Cached source
versions are kept side by side, and each mod builds its own local copies.

| Location | Contents |
| --- | --- |
| `Libraries/native/<library>/<revision>` | Pinned native source snapshots and their licenses. |
| `Libraries/.archives` | Downloaded archives checked against the lock-file SHA-256 hashes. |
| `Libraries/web/<lock-hash>/win32-x64` | Cached web package trees. |
| `Libraries/npm-cache` | npm's download cache. |
| `<Mod>/extern` | That mod's independent native dependency source copies. |

Existing complete project copies are reused before the shared cache is consulted.
Shared originals are not patched or built in place. Keep project-specific
adjustments in your project, not in `Libraries`.

From inside a mod or extracted template, these commands are available when needed:

```powershell
# Prepare all locked native and web dependencies in advance; web preparation needs Node/npm.
cmake -P tools/PrepareLibraries.cmake

# Prepare native sources only.
cmake -DSKYRIM_PREPARE_WEB=OFF -P tools/PrepareLibraries.cmake

# Use a cache in another location.
cmake --preset ng "-DSKYRIM_LIBRARIES_DIR=C:/Coding/Skyrim Mods/Libraries"

# Require the warm cache without downloading missing dependencies.
cmake --preset ng -DSKYRIM_ALLOW_DOWNLOADS=OFF
```

Cache population uses locks and staging directories. Downloaded archives must
match the hashes recorded in `dependencies.lock.json`. Normal builds do not follow
moving branches or overwrite local dependency edits.

## Common setup problems

| Symptom | What to check |
| --- | --- |
| CMake cannot find a preset or `CMakeLists.txt`. | Open the terminal in the mod folder containing `CMakeLists.txt`, not the workspace root or an extra extraction wrapper. |
| Visual Studio, v143 or the Windows SDK is missing. | Install the C++ workload/toolset/SDK listed in step 1, then configure again. |
| Libraries are missing or downloading unexpectedly. | Keep the mod beside `Libraries`, or set `SKYRIM_LIBRARIES_DIR` explicitly. Only a missing locked revision needs downloading. |
| Author, version, plugin name or options did not change. | Set the cached values with `-D`, or use the fresh configure described above. |
| The resolver reports an unmanaged, incomplete or mismatched local dependency copy. | Move the named project `extern` copy aside and configure again. Preserve any intentional local edits. A shared-cache error instead names the cache entry to inspect. |
| A web package lock changed or `node_modules` is unmanaged. | Move that web project's old `node_modules` aside, then rerun the web build with the matching lock and cache. |
| A hook compiles but does not work in-game. | Verify its addresses, offsets and runtime assumptions on the target game version; the NG update alone cannot validate a custom patch. |

## Maintaining Template.zip

For a generic template change, extract `Template.zip` into a fresh temporary
maintenance folder beside `Libraries`, make the change there, and validate it.
Then run this command from that extracted starter:

```powershell
cmake -P tools/ExportTemplate.cmake
```

The command writes `../Template.zip`, replacing the shared starter archive. It
includes starter sources and tools while excluding Git metadata, dependencies,
caches and build outputs. Export from the generic starter: exporting from a
finished mod would put that mod's code and data into the template.

`dependencies.lock.json` controls dependency versions. For an intentional upgrade,
use `tools/lock_dependencies.py`, prepare the changed sources, validate builds,
then export the updated starter. This is separate from creating a normal new mod.

## Existing rebuilt mods

The existing seven mod folders are independent working projects using the same
build commands. Their RelWithDebInfo builds, metadata, DLL exports, packages and
required assets passed validation on 2026-10-29. Debug builds of these migrated
mods and gameplay testing remain pending. Each has a `MIGRATION.md` with details.

| Mod | Release package | Additional runtime integrations |
| --- | --- | --- |
| AssassinationSKSE | [ZIP](<C:/Coding/Skyrim Mods/AssassinationSKSE/dist/AssassinationSKSE-1.0.0-RelWithDebInfo.zip>) | SKSE Menu Framework for settings. |
| ConfigurableSpeeds | [ZIP](<C:/Coding/Skyrim Mods/ConfigurableSpeeds/dist/ConfigurableSpeeds-1.0.0-RelWithDebInfo.zip>) | SKSE Menu Framework for settings. |
| DiagonalSprint | [ZIP](<C:/Coding/Skyrim Mods/DiagonalSprint/dist/DiagonalSprint-1.0.5-RelWithDebInfo.zip>) | SKSE Menu Framework for settings. |
| JumpAttackFix | [ZIP](<C:/Coding/Skyrim Mods/JumpAttackFix/dist/JumpAttackFix-1.0.0-RelWithDebInfo.zip>) | No additional UI/animation framework. |
| MagicOrganizer | [ZIP](<C:/Coding/Skyrim Mods/MagicOrganizer/dist/MagicOrganizer-1.0.0-RelWithDebInfo.zip>) | SKSE Menu Framework for settings. |
| MordhauCombat | [ZIP](<C:/Coding/Skyrim Mods/MordhauCombat/dist/MordhauCombat-1.0.0-RelWithDebInfo.zip>) | Open Animation Replacer for animation routing; SKSE Menu Framework for settings. |
| SpellBinding | [ZIP](<C:/Coding/Skyrim Mods/SpellBinding/dist/SpellBinding-1.0.0-RelWithDebInfo.zip>) | PrismaUI for the menu and HUD. |

## License and public source

The original source code for each of the seven mods is licensed under the GNU
GPL, version 3 or any later version (GPL-3.0-or-later). The repository root
`LICENSE` and each mod folder's `LICENSE.txt` contain the full, unmodified
GPL-3.0 text. Each release archive places the same file at its root and
includes a `README.txt` with the public source link and copyright and warranty
notice.

The official GPL-3.0 license text is also available at
https://www.gnu.org/licenses/gpl-3.0.html. This code license does not replace
separate terms for third-party libraries or bundled assets.

| Mod | Public source |
| --- | --- |
| AssassinationSKSE | [GitHub source](https://github.com/mithras777/Skyrim-Mods/tree/main/AssassinationSKSE) |
| ConfigurableSpeeds | [GitHub source](https://github.com/mithras777/Skyrim-Mods/tree/main/ConfigurableSpeeds) |
| DiagonalSprint | [GitHub source](https://github.com/mithras777/Skyrim-Mods/tree/main/DiagonalSprint) |
| JumpAttackFix | [GitHub source](https://github.com/mithras777/Skyrim-Mods/tree/main/JumpAttackFix) |
| MagicOrganizer | [GitHub source](https://github.com/mithras777/Skyrim-Mods/tree/main/MagicOrganizer) |
| MordhauCombat | [GitHub source](https://github.com/mithras777/Skyrim-Mods/tree/main/MordhauCombat) |
| SpellBinding | [GitHub source](https://github.com/mithras777/Skyrim-Mods/tree/main/SpellBinding) |

CommonLibSSE-NG is pinned to revision
`a6d3ea4642f71fb92ac3e0d7fef5808217f8aae9`. Upstream licenses it under
GPL-3.0-or-later with its Modding Exception and GPL-3.0 Linking Exception.
The release archives include the pinned dependency's `COPYING.txt`,
`EXCEPTIONS.md`, and the notices for the other locked dependencies. The
template-derived portions retain ArranzCNL's MIT notice. Other bundled assets
and third-party components retain their applicable notices and terms.

All seven also need SKSE and Address Library matching the game. Install the full
ZIP when testing SpellBinding or MordhauCombat so their web UI/animations are
included. DiagonalSprint's legacy console interception and MordhauCombat's vector
renderer patches skip unverified runtimes, including 1.7.x; their other mod hooks
remain enabled.

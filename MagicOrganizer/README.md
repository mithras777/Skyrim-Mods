# MagicOrganizer build

## Build and create the ZIP

Run `Build.bat` from this folder, or from a terminal:

```bat
Build.bat
```

The script starts the MSVC command-line environment, runs CMake with Ninja, and builds the release DLL. CMake packages the result as:

```text
Nexus Release/MagicOrganizer.zip
```

The build needs Git, CMake 3.24 or newer, the Visual C++ x64/x86 build tools, and Visual Studio's vcpkg component. In this workspace, MagicOrganizer and the starter template share the existing CommonLibSSE-NG v12.1.0 source and release bundle cached under `MagicOrganizer/build/cmake-ng/_deps`; a standalone checkout falls back to FetchContent on its first build. vcpkg manages the remaining dependencies.

No Visual Studio project or IDE build button is used.

## Build dependencies

- CommonLibSSE-NG `v12.1.0`, fetched directly with CMake
- Dear ImGui `v1.92.5`
- vcpkg-managed `spdlog`, DirectXTK, rapidcsv, and nlohmann-json
- Xbyak `v7.30`

The CMake preset uses the upstream CommonLib release bundle when its compiler and runtime library settings match; otherwise CommonLib builds from source.

## Runtime notes

The plugin metadata supports Skyrim SE `1.5.97.0` and Address Library based post-AE runtimes, including the `1.7.104` Address Library format. Install the matching Address Library for the running game.

That covers dependency and loader compatibility. The plugin still installs a call-site hook using the existing SE/AE offsets, so a successful build does not prove that the hook has been checked in-game on every runtime. Skyrim VR is intentionally rejected by the plugin.

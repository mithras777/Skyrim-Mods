# Skyrim SKSE Plugin Template

A small, command-line CMake project for creating a Skyrim SE/AE SKSE plugin. It uses CMake, Ninja, MSVC Build Tools, and CommonLibSSE-NG. No Visual Studio project setup or IDE build button is required.

## Start a plugin

1. Extract `Template.zip`.
2. Rename the extracted `Template` folder to your plugin name. Use letters, numbers, and underscores, with no spaces; the folder name becomes the DLL, CMake target, and release ZIP name.
3. Edit `CMakeLists.txt` and replace `Your Name` and the other project metadata.
4. Add your code under `include/` and `src/`.
5. Run `Build.bat`.

The script configures and builds the release DLL, then creates `Nexus Release/<PluginName>.zip` with the standard `SKSE/Plugins` folder and the PDB alongside the DLL.

## Build requirements

- Windows 10 or 11 x64
- CMake 3.24 or newer and Git available on `PATH`
- Visual Studio or Visual Studio Build Tools with the MSVC x64/x86 C++ tools, CMake/Ninja, and vcpkg components installed
- The project reuses the CommonLibSSE-NG v12.1.0 source and bundle already cached by MagicOrganizer when both projects are in this workspace. A standalone copy fetches CommonLib on its first build.
- An internet connection may be needed for first-time vcpkg dependencies

`Build.bat` uses the Visual Studio command-line environment. You do not need to open Visual Studio or click Build. The ZIP contains the project starter and build scripts, not compilers or downloaded third-party libraries.

## Runtime notes

The preset builds against CommonLibSSE-NG v12.1.0 with Skyrim SE and AE enabled. The starter plugin declares Address Library compatibility for newer SE/AE runtimes and explicitly includes Skyrim 1.5.97. This covers the supported SE/AE runtimes through 1.7.104 when the matching Address Library is installed; future game or Address Library format changes still need a compatible CommonLib update. Its sample loader rejects Skyrim VR. Actual compatibility also depends on the APIs and address offsets used by code added to the template.

## Main files

- `CMakeLists.txt` — plugin name and author/version metadata
- `CMakePresets.json` — pinned command-line CMake/Ninja configuration
- `Build.bat` — configures, builds, and packages the plugin
- `cmake/` — CMake project setup and pinned CommonLibSSE-NG fetch configuration
- `include/`, `src/`, `pch/`, and `resource/` — clean plugin source skeleton

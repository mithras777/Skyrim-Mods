# Skyrim SKSE Plugin Template

The reusable command-line CMake template is in [`Template.zip`](Template.zip). Extract it, rename the resulting `Template` folder to your plugin name, edit the project metadata in `CMakeLists.txt`, and run `Build.bat` from that folder.

The build uses CMake, Ninja, MSVC Build Tools, and CommonLibSSE-NG. It creates `Nexus Release/<PluginName>.zip` with the plugin DLL and PDB in the SKSE plugin folder. See [`Template/README.md`](Template/README.md) for setup requirements and runtime notes.

The starter reuses the CommonLibSSE-NG v12.1.0 source and release bundle cached by MagicOrganizer in this workspace. The older Visual Studio projects use the v6.7.1 checkout already used by DiagonalSprint, so they share the format-5 Address Library support needed by Skyrim 1.7.104 without cloning CommonLib separately for every mod. Both versions target Skyrim SE/AE; matching Address Library files are still required, and future runtime formats need a compatible CommonLib update.

`Template.7z` and `CMake_Build.bat` are the previous generated-project workflow; the new ZIP contains a complete starter project that can be extracted and built directly.

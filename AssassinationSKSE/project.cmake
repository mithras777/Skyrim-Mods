# Edit these values, or override them with -D on the configure command.
set(MOD_NAME "" CACHE STRING "Plugin name; empty uses the extracted folder name")
set(MOD_AUTHOR "Mithras666" CACHE STRING "Plugin author")
set(MOD_VERSION "1.0.0" CACHE STRING "Plugin version")
set(MOD_DESCRIPTION "AssassinationSKSE migrated to pinned CommonLibSSE-NG" CACHE STRING "Plugin description")
set(MOD_LICENSE "MIT" CACHE STRING "License for your own plugin code")

option(MOD_WITH_JSON "Enable nlohmann JSON and REX JSON support" ON)
option(MOD_WITH_INI "Enable SimpleIni and REX INI support" OFF)
option(MOD_WITH_TOML "Enable toml11 and REX TOML support" OFF)
option(MOD_WITH_IMGUI "Build an independent ImGui renderer library" OFF)
option(MOD_WITH_SKSE_MENU "Expose the SKSE Menu Framework consumer SDK" ON)
option(MOD_WITH_PRISMA_UI "Expose the PrismaUI consumer SDK" OFF)
option(MOD_WITH_OAR "Build Open Animation Replacer consumer API helpers" OFF)
option(MOD_BUILD_WEB "Build a web project before packaging the plugin" OFF)
# The upstream console demo uses offsets that have not been verified on 1.7.x.
option(MOD_ENABLE_DEMO_CONSOLE_HOOK "Attempt the legacy console demo on SE 1.5.97 / AE 1.6.1170 only" OFF)

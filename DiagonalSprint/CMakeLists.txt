# Mimimum CMake version
cmake_minimum_required(VERSION 3.24.0)

# =========================
# Project Configuration
# =========================
project(
    # Must match folder name
    DiagonalSprint
    DESCRIPTION "DiagonalSprint"
    VERSION     1.0.5
    LANGUAGES   CXX
)

# =========================
# Metadata / Licensing
# =========================
set(PROJECT_AUTHOR  "Mithras666")
set(PROJECT_YEAR    "(C) 2025")
set(PROJECT_LICENSE "MIT")

# =========================
# CMake Module Path
# =========================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# =========================
# Project Modules
# =========================
include(utility_config)
include(project_config)

# =========================
# CommonLibSSE Variant
# =========================
set(COMMONLIB_VARIANT "vs2022-ng")
option(SKYRIM_SUPPORT_AE "Enables support for Skyrim AE" ON)
option(INSTALL_IMGUI "Installs imgui framework" OFF)

# =========================
# Submodules / Dependencies
# =========================
include(submodule_config)

# Include licenses for the plugin and statically linked CommonLibSSE-NG in the
# distributable package.
set(NEXUS_RELEASE_MOD_DIR "${PROJECT_SOURCE_DIR}/Nexus Release/${PROJECT_NAME}")
file(MAKE_DIRECTORY "${NEXUS_RELEASE_MOD_DIR}")
file(COPY_FILE "${PROJECT_SOURCE_DIR}/LICENSE" "${NEXUS_RELEASE_MOD_DIR}/LICENSE-DiagonalSprint.txt" ONLY_IF_DIFFERENT)
file(COPY_FILE "${COMMONLIBSSE_ROOT_DIR}/COPYING" "${NEXUS_RELEASE_MOD_DIR}/COPYING-CommonLibSSE-NG" ONLY_IF_DIFFERENT)
file(COPY_FILE "${COMMONLIBSSE_ROOT_DIR}/EXCEPTIONS.md" "${NEXUS_RELEASE_MOD_DIR}/EXCEPTIONS-CommonLibSSE-NG.md" ONLY_IF_DIFFERENT)

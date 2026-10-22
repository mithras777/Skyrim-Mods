# Command-line CMake build for MagicOrganizer.
cmake_minimum_required(VERSION 3.24)

project(
	MagicOrganizer
	DESCRIPTION "MagicOrganizer"
	VERSION 1.0.0
	LANGUAGES CXX
)

set(PROJECT_AUTHOR "Mithras666")
set(PROJECT_YEAR "(C) 2025")
set(PROJECT_LICENSE "MIT")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(utility_config)
include(project_config)

# Build the current CommonLibSSE-NG line for SE and AE. CMake presets pin the
# exact upstream release and enable its matching prebuilt library for release builds.
include(submodule_config)

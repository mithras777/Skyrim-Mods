cmake_minimum_required(VERSION 3.24)

# The extracted folder name becomes the CMake target and packaged plugin name.
get_filename_component(PLUGIN_PROJECT_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

project(
	"${PLUGIN_PROJECT_NAME}"
	DESCRIPTION "A Skyrim SKSE plugin"
	VERSION 1.0.0
	LANGUAGES CXX
)

# Edit these values for your plugin.
set(PROJECT_AUTHOR "Your Name")
set(PROJECT_YEAR "(C) 2026")
set(PROJECT_LICENSE "MIT")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(utility_config)
include(project_config)
include(submodule_config)

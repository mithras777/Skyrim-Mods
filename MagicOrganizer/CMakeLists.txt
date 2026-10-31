cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0091 NEW)
include("${CMAKE_CURRENT_LIST_DIR}/project.cmake")
if(MOD_NAME STREQUAL "")
    get_filename_component(MOD_NAME "${CMAKE_CURRENT_LIST_DIR}" NAME)
endif()
if(MOD_NAME MATCHES "[\";\\/]")
    message(FATAL_ERROR "MOD_NAME cannot contain quotes, semicolons or path separators")
endif()
set(PLUGIN_NAME "${MOD_NAME}")
string(REGEX REPLACE "[^A-Za-z0-9_]" "_" target_name "${PLUGIN_NAME}")
project("${target_name}" VERSION "${MOD_VERSION}" DESCRIPTION "${MOD_DESCRIPTION}" LANGUAGES CXX C RC)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This template requires Windows x64 and the Visual Studio 2022 C++ tools")
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
include("${CMAKE_CURRENT_LIST_DIR}/cmake/Dependencies.cmake")

file(GLOB_RECURSE plugin_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
list(FILTER plugin_sources EXCLUDE REGEX "/src/hook/InputHook.cpp$")
add_library("${PROJECT_NAME}" MODULE ${plugin_sources} "${CMAKE_CURRENT_SOURCE_DIR}/resource/plugin.rc")
target_link_libraries("${PROJECT_NAME}" PRIVATE CommonLibSSE::CommonLibSSE Psapi)
get_property(optional_targets GLOBAL PROPERTY SKYRIM_OPTIONAL_TARGETS)
target_link_libraries("${PROJECT_NAME}" PRIVATE ${optional_targets})
target_include_directories("${PROJECT_NAME}" PRIVATE include "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_precompile_headers("${PROJECT_NAME}" PRIVATE pch/PCH.h)
target_compile_options("${PROJECT_NAME}" PRIVATE /W4 /MP4 /bigobj /Zc:preprocessor /permissive- /EHsc)
target_compile_definitions("${PROJECT_NAME}" PRIVATE SKYRIM_SUPPORT_NG=1
    "$<$<BOOL:${MOD_ENABLE_DEMO_CONSOLE_HOOK}>:MOD_ENABLE_DEMO_CONSOLE_HOOK=1>")
foreach(feature JSON INI TOML IMGUI SKSE_MENU PRISMA_UI OAR)
    if(MOD_WITH_${feature})
        target_compile_definitions("${PROJECT_NAME}" PRIVATE MOD_WITH_${feature}=1)
    endif()
endforeach()
set_target_properties("${PROJECT_NAME}" PROPERTIES OUTPUT_NAME "${PLUGIN_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/$<CONFIG>"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>")
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(VERSION_TWEAK 0)
if(DEFINED PROJECT_VERSION_TWEAK AND NOT PROJECT_VERSION_TWEAK STREQUAL "")
    set(VERSION_TWEAK ${PROJECT_VERSION_TWEAK})
endif()
set(PROJECT_AUTHOR "${MOD_AUTHOR}")
set(PROJECT_LICENSE "${MOD_LICENSE}")
string(TIMESTAMP PROJECT_YEAR "%Y")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PluginVersion.h.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h" @ONLY)

if(MOD_BUILD_WEB)
    set(SKYRIM_WEB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/web" CACHE PATH "Web project containing package-lock.json")
    if(NOT EXISTS "${SKYRIM_WEB_DIR}/package-lock.json")
        message(FATAL_ERROR "MOD_BUILD_WEB requires a web project at SKYRIM_WEB_DIR")
    endif()
    add_custom_target(web COMMAND "${CMAKE_COMMAND}" "-DSKYRIM_WEB_DIR=${SKYRIM_WEB_DIR}"
        "-DSKYRIM_LIBRARIES_DIR=${SKYRIM_LIBRARIES_DIR}" "-DSKYRIM_ALLOW_DOWNLOADS=${SKYRIM_ALLOW_DOWNLOADS}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/BuildWeb.cmake" VERBATIM)
    add_dependencies("${PROJECT_NAME}" web)
endif()

get_property(used_libraries GLOBAL PROPERTY SKYRIM_USED_LIBRARIES)
list(REMOVE_DUPLICATES used_libraries)
set(notices "${CMAKE_CURRENT_BINARY_DIR}/dependency-notices")
file(MAKE_DIRECTORY "${notices}")
foreach(id IN LISTS used_libraries)
    skyrim_library("${id}" local_source)
    file(GLOB license_files "${local_source}/LICENSE*" "${local_source}/LICENCE*" "${local_source}/COPYING*" "${local_source}/COPYRIGHT*" "${local_source}/NOTICE*" "${local_source}/EXCEPTIONS*")
    if(license_files)
        file(MAKE_DIRECTORY "${notices}/${id}")
        file(COPY ${license_files} DESTINATION "${notices}/${id}")
    endif()
endforeach()
file(WRITE "${notices}/Dependencies.txt" "Sources are pinned in dependencies.lock.json.\nUsed libraries: ${used_libraries}\n")
# Package on every normal build so data/web changes are included even without a relink.
add_custom_target(package_mod ALL
    COMMAND "${CMAKE_COMMAND}" "-DPLUGIN_FILE=$<TARGET_FILE:${PROJECT_NAME}>"
        "-DSYMBOL_FILE=$<TARGET_PDB_FILE:${PROJECT_NAME}>" "-DPLUGIN_NAME=${PLUGIN_NAME}"
        "-DPLUGIN_VERSION=${PROJECT_VERSION}" "-DBUILD_CONFIG=$<CONFIG>" "-DPLUGIN_AUTHOR=${MOD_AUTHOR}" "-DMOD_SOURCE_URL=${MOD_SOURCE_URL}"
        "-DPROJECT_DIR=${CMAKE_CURRENT_SOURCE_DIR}" "-DNOTICE_DIR=${notices}"
        "-DWEB_DIR=${SKYRIM_WEB_DIR}" "-DINCLUDE_WEB=${MOD_BUILD_WEB}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/PackageMod.cmake"
    DEPENDS "${PROJECT_NAME}" VERBATIM)

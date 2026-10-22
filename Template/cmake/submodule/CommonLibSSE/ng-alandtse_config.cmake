# Use upstream CommonLibSSE-NG directly; no template-specific patched copy.
include(FetchContent)

# Reuse the CommonLibSSE-NG checkout and bundle already present in this
# workspace. A standalone clone still falls back to FetchContent's normal
# first-time download behavior.
get_filename_component(SKYRIM_MODS_WORKSPACE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../" ABSOLUTE)
set(COMMONLIBSSE_SHARED_SOURCE_DIR
	"${SKYRIM_MODS_WORKSPACE_ROOT}/MagicOrganizer/build/cmake-ng/_deps/commonlibsseupstream-src")
set(COMMONLIBSSE_SHARED_PREBUILT_DIR
	"${SKYRIM_MODS_WORKSPACE_ROOT}/MagicOrganizer/build/cmake-ng/_deps/commonlibsseupstream-build/.prebuilt/v12.1.0")
set(COMMONLIBSSE_SHARED_HDE64_DIR
	"${SKYRIM_MODS_WORKSPACE_ROOT}/MagicOrganizer/build/cmake-ng/_deps/hde64-src")
set(COMMONLIBSSE_SHARED_XBYAK_DIR
	"${SKYRIM_MODS_WORKSPACE_ROOT}/MagicOrganizer/MagicOrganizer/extern/xbyak")

if(NOT FETCHCONTENT_SOURCE_DIR_COMMONLIBSSEUPSTREAM
	AND EXISTS "${COMMONLIBSSE_SHARED_SOURCE_DIR}/CMakeLists.txt")
	set(FETCHCONTENT_SOURCE_DIR_COMMONLIBSSEUPSTREAM "${COMMONLIBSSE_SHARED_SOURCE_DIR}")
	message(STATUS "Reusing workspace CommonLibSSE-NG v12.1.0 at ${COMMONLIBSSE_SHARED_SOURCE_DIR}")
endif()
if(NOT FETCHCONTENT_SOURCE_DIR_HDE64 AND EXISTS "${COMMONLIBSSE_SHARED_HDE64_DIR}/CMakeLists.txt")
	set(FETCHCONTENT_SOURCE_DIR_HDE64 "${COMMONLIBSSE_SHARED_HDE64_DIR}")
endif()
if(NOT FETCHCONTENT_SOURCE_DIR_XBYAK AND EXISTS "${COMMONLIBSSE_SHARED_XBYAK_DIR}/CMakeLists.txt")
	set(FETCHCONTENT_SOURCE_DIR_XBYAK "${COMMONLIBSSE_SHARED_XBYAK_DIR}")
endif()
if(NOT COMMONLIB_PREBUILT_DIR AND IS_DIRECTORY "${COMMONLIBSSE_SHARED_PREBUILT_DIR}")
	set(COMMONLIB_PREBUILT_DIR "${COMMONLIBSSE_SHARED_PREBUILT_DIR}")
endif()

set(BUILD_TESTS OFF CACHE BOOL "Do not build CommonLib's unit-test executable" FORCE)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "Support Skyrim SE runtimes" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "Support Skyrim AE runtimes" FORCE)
set(ENABLE_SKYRIM_VR ON CACHE BOOL "Keep CommonLib's all-runtime ABI for its release bundle" FORCE)
set(SKSE_SUPPORT_XBYAK ON CACHE BOOL "Enable the plugin trampoline interface" FORCE)
set(SKSE_SUPPORT_PATCH_SAFETY ON CACHE BOOL "Enable CommonLib trampoline patch checks" FORCE)

FetchContent_Declare(
	CommonLibSSEUpstream
	GIT_REPOSITORY https://github.com/alandtse/CommonLibSSE-NG.git
	GIT_TAG        v12.1.0
	GIT_SHALLOW    TRUE
	GIT_SUBMODULES ""
)
FetchContent_MakeAvailable(CommonLibSSEUpstream)
FetchContent_GetProperties(
	CommonLibSSEUpstream
	SOURCE_DIR COMMONLIBSSE_ROOT_DIR
	BINARY_DIR COMMONLIBSSE_BINARY_DIR
)

if(COMMONLIB_PREBUILT_DIR AND IS_DIRECTORY "${COMMONLIB_PREBUILT_DIR}")
	set(COMMONLIBSSE_PREBUILT_BUNDLE_DIR "${COMMONLIB_PREBUILT_DIR}")
else()
	set(COMMONLIBSSE_PREBUILT_BUNDLE_DIR
		"${COMMONLIBSSE_BINARY_DIR}/.prebuilt/v12.1.0")
endif()

# If the current MSVC toolset does not match CommonLib's release bundle,
# CommonLib correctly builds its own library from source. The checkout skips
# OpenVR's full Git submodule history; reuse the matching SDK/import library
# shipped in that same, checksum-verified release bundle for the source build.
get_target_property(COMMONLIBSSE_IS_IMPORTED CommonLibSSE IMPORTED)
if(NOT COMMONLIBSSE_IS_IMPORTED)
	set(COMMONLIBSSE_OPENVR_BUNDLE
		"${COMMONLIBSSE_PREBUILT_BUNDLE_DIR}/extern/openvr")
	set(COMMONLIBSSE_OPENVR_SOURCE_LIB
		"${COMMONLIBSSE_PREBUILT_BUNDLE_DIR}/lib/openvr_api.lib")
	set(COMMONLIBSSE_OPENVR_SOURCE_HEADERS
		"${COMMONLIBSSE_OPENVR_BUNDLE}/headers")
	set(COMMONLIBSSE_OPENVR_SOURCE_INCLUDE
		"${COMMONLIBSSE_ROOT_DIR}/extern/openvr/headers")
	set(COMMONLIBSSE_OPENVR_SOURCE_DEST_LIB
		"${COMMONLIBSSE_ROOT_DIR}/extern/openvr/lib/win64/openvr_api.lib")
	if(EXISTS "${COMMONLIBSSE_OPENVR_SOURCE_LIB}"
		AND IS_DIRECTORY "${COMMONLIBSSE_OPENVR_SOURCE_HEADERS}")
		if(NOT EXISTS "${COMMONLIBSSE_OPENVR_SOURCE_INCLUDE}/openvr.h")
			file(COPY "${COMMONLIBSSE_OPENVR_SOURCE_HEADERS}"
				DESTINATION "${COMMONLIBSSE_ROOT_DIR}/extern/openvr")
		endif()
		if(NOT EXISTS "${COMMONLIBSSE_OPENVR_SOURCE_DEST_LIB}")
			file(MAKE_DIRECTORY "${COMMONLIBSSE_ROOT_DIR}/extern/openvr/lib/win64")
			file(COPY_FILE "${COMMONLIBSSE_OPENVR_SOURCE_LIB}"
				"${COMMONLIBSSE_OPENVR_SOURCE_DEST_LIB}")
		endif()
	endif()
	if(NOT EXISTS "${COMMONLIBSSE_OPENVR_SOURCE_INCLUDE}/openvr.h"
		OR NOT EXISTS "${COMMONLIBSSE_OPENVR_SOURCE_DEST_LIB}")
		message(FATAL_ERROR
			"CommonLibSSE-NG is building from source, but its matching OpenVR files "
			"are not available in the downloaded CommonLib release bundle.")
	endif()
endif()

# The project uses Xbyak directly for its call-site hook and CommonLib's trampoline.
# Reuse the existing checkout when present, otherwise fetch the same pinned version.
set(XBYAK_ROOT_DIR "${PROJECT_EXTERN_ROOT_DIR}/xbyak")
if(EXISTS "${XBYAK_ROOT_DIR}/xbyak/xbyak.h")
	add_subdirectory("${XBYAK_ROOT_DIR}" "${PROJECT_BINARY_DIR}/xbyak" EXCLUDE_FROM_ALL)
else()
	FetchContent_Declare(
		Xbyak
		GIT_REPOSITORY https://github.com/herumi/xbyak.git
		GIT_TAG        v7.30
		GIT_SHALLOW    TRUE
	)
	FetchContent_MakeAvailable(Xbyak)
	FetchContent_GetProperties(Xbyak SOURCE_DIR XBYAK_ROOT_DIR)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE xbyak CommonLibSSE::CommonLibSSE)

# CommonLib needs Xbyak headers when built from source. The release bundle is an
# imported target and already contains the trampoline implementation.
if(NOT COMMONLIBSSE_IS_IMPORTED)
	target_link_libraries(CommonLibSSE PUBLIC xbyak)
endif()

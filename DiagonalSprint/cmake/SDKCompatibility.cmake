include_guard(GLOBAL)
# Correct namespace-local forward declarations in this pinned consumer header.
# Upstream's `typedef struct T T` can bind to ::T if imgui.h was included first.
function(skyrim_patch_menu_sdk source)
    string(FIND "${source}/" "${SKYRIM_EXTERN_DIR}/" project_prefix)
    if(NOT project_prefix EQUAL 0)
        message(FATAL_ERROR "SDK compatibility adjustments are restricted to this project's extern copy")
    endif()
    file(READ "${source}/SKSEMenuFramework.h" sdk)
    if(NOT sdk MATCHES "SkyrimTemplate forward declarations v1")
        string(REGEX REPLACE "typedef struct ([A-Za-z_][A-Za-z0-9_]*) ([A-Za-z_][A-Za-z0-9_]*);"
            "struct \\1; typedef struct \\1 \\2;" fixed_sdk "${sdk}")
        file(WRITE "${source}/SKSEMenuFramework.h" "${fixed_sdk}\n// SkyrimTemplate forward declarations v1\n")
        file(WRITE "${source}/NOTICE_TEMPLATE_PATCHES.txt"
            "This project copy adds explicit namespace-local struct forward declarations to SKSEMenuFramework.h.\nThis resolves C++ name lookup conflicts when standalone ImGui is included first. No structure layouts or function signatures were changed. The shared upstream cache remains unmodified.\n")
    endif()
endfunction()

function(skyrim_patch_prisma_sdk source)
    string(FIND "${source}/" "${SKYRIM_EXTERN_DIR}/" project_prefix)
    if(NOT project_prefix EQUAL 0)
        message(FATAL_ERROR "SDK compatibility adjustments are restricted to this project's extern copy")
    endif()
    file(READ "${source}/src/PrismaUI_API.h" sdk)
    string(REPLACE "GetModuleHandle(L\"PrismaUI.dll\")" "GetModuleHandleW(L\"PrismaUI.dll\")" fixed_sdk "${sdk}")
    if(NOT fixed_sdk STREQUAL sdk)
        file(WRITE "${source}/src/PrismaUI_API.h" "${fixed_sdk}")
        file(WRITE "${source}/NOTICE_TEMPLATE_PATCHES.txt"
            "This project copy calls GetModuleHandleW explicitly for the wide PrismaUI.dll literal.\nThis makes the consumer header independent of the project's UNICODE setting. API layouts and signatures are unchanged; the shared upstream cache remains unmodified.\n")
    endif()
endfunction()

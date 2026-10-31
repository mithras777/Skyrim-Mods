// These translation-unit checks keep optional SDK wiring buildable without requiring
// the corresponding runtime frameworks to be installed on the development machine.
#if defined(MOD_WITH_JSON)
#include <nlohmann/json.hpp>
static_assert(sizeof(nlohmann::json) > 0);
#endif
#if defined(MOD_WITH_INI)
#include <SimpleIni.h>
static_assert(sizeof(CSimpleIniA) > 0);
#endif
#if defined(MOD_WITH_TOML)
#include <toml.hpp>
static_assert(sizeof(toml::value) > 0);
#endif
#if defined(MOD_WITH_IMGUI)
#include <imgui.h>
static_assert(sizeof(ImVec2) == sizeof(float) * 2);
#endif
#if defined(MOD_WITH_SKSE_MENU)
#include <SKSEMenuFramework.h>
static_assert(sizeof(SKSEMenuFramework::Model::WindowInterface) > 0);
#endif
#if defined(MOD_WITH_PRISMA_UI)
#include <PrismaUI_API.h>
static_assert(sizeof(PrismaView) == sizeof(std::uint64_t));
#endif
#if defined(MOD_WITH_OAR)
#include <OpenAnimationReplacerAPI-Conditions.h>
#include <OpenAnimationReplacerAPI-Functions.h>
#include <OpenAnimationReplacerAPI-Animations.h>
static_assert(sizeof(OAR_API::Conditions::APIResult) > 0);
#endif
